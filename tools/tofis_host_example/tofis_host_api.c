// tofis_host_api.c
#include "tofis_host_api.h"
#include "checksum.h"
#include "tofis_host_serial.h"
#include "tofis_input_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <unistd.h>
#endif

#include <stdbool.h>

// 定義一個 mutex 和 condition variable 來同步數據
#ifdef _WIN32
static HANDLE data_mutex;
static HANDLE data_cond;
#else
static pthread_mutex_t data_mutex_p = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t data_cond_p = PTHREAD_COND_INITIALIZER;
#endif

static SerialPort serial_port;
static bool data_ready = false;
static tofis_data_packet_t latest_packet;

// 接收線程函數
#ifdef _WIN32
DWORD WINAPI receive_thread_func(LPVOID lpParam) {
#else
static void *receive_thread_func(void *arg) {
#endif
  while (1) {
    // 讀取 header（4 bytes）
    uint8_t header[4];
    int bytes_read = read_serial(&serial_port, header, 4);
    if (bytes_read < 4) {
#ifdef TOFIS_API_DEBUG
      printf("Error: Unable to read header from serial port.\n");
#endif
      continue;
    }

    // 驗證 start_byte 和 end_byte
    if (header[0] != 0xAA || header[3] != 0x55) {
#ifdef TOFIS_API_DEBUG
      printf("Error: Invalid start or end byte. Received: 0x%02X ... 0x%02X\n",
             header[0], header[3]);
#endif
      continue;
    }

    uint8_t resolution = header[1];
    uint8_t received_checksum = header[2];

    // 根據 resolution 計算 RANGING_SENSOR_Result_t 的大小
    size_t data_size = sizeof(RANGING_SENSOR_Result_t);
    uint8_t *data_buffer = (uint8_t *)malloc(data_size);
    if (data_buffer == NULL) {
      printf("Error: Memory allocation failed.\n");
      break;
    }

    // 讀取數據部分
    bytes_read = read_serial(&serial_port, data_buffer, data_size);
    if (bytes_read < data_size) {
      printf("Error: Unable to read data from serial port.\n");
      free(data_buffer);
      continue;
    }

    // 計算 checksum，從 data 開始
    uint8_t calculated_checksum = calculate_checksum(data_buffer, data_size);
    if (calculated_checksum != received_checksum) {
      printf("Error: Checksum mismatch. Calculated: 0x%02X, Received: 0x%02X\n",
             calculated_checksum, received_checksum);
      free(data_buffer);
      continue;
    }

    // 將數據轉換為結構體
    RANGING_SENSOR_Result_t result;
    memcpy(&result, data_buffer, data_size);
    free(data_buffer);

    // 更新 Profile 參數（根據具體需求進行設置）
    // 這裡假設您有某種方式來設置 Profile，這裡僅作示例
    // Profile 的定義應在主程序中或全局變數中定義
    // 此處不處理 Profile 的設定，因為 print_result 會使用全局 Profile

    // 構建完整的數據包
    tofis_data_packet_t packet;
    packet.start_byte = header[0];
    packet.resolution = resolution;
    packet.checksum = received_checksum;
    packet.end_byte = header[3];
    memcpy(&packet.data, &result, sizeof(RANGING_SENSOR_Result_t));

    // 更新最新數據包並通知主線程
#ifdef _WIN32
    WaitForSingleObject(data_mutex, INFINITE);
    latest_packet = packet;
    data_ready = true;
    ReleaseMutex(data_mutex);
    SetEvent(data_cond);
#else
    pthread_mutex_lock(&data_mutex_p);
    latest_packet = packet;
    data_ready = true;
    pthread_cond_signal(&data_cond_p);
    pthread_mutex_unlock(&data_mutex_p);
#endif
  }

#ifdef _WIN32
  return 0;
#else
  return NULL;
#endif
}

#ifdef _WIN32
static HANDLE receive_thread;
static HANDLE input_thread;
#else
static pthread_t receive_thread_p;
static pthread_t input_thread_p;
#endif

#ifdef _WIN32
DWORD WINAPI user_input_thread_func(LPVOID lpParam) {
#else
static void *user_input_thread_func(void *arg) {
#endif

#ifdef _WIN32
  SerialPort *port = (SerialPort *)lpParam; // 傳入的串列埠結構
#else
  SerialPort *port = (SerialPort *)arg; // 傳入的串列埠結構
#endif
  char user_input_section[TOFIS_USER_INPUT_BUF_SIZE];
  uint8_t to_tofis_buf[TOFIS_USER_INPUT_BUF_SIZE];

  while (1) {
    printf("Enter command: ");
    if (fgets(user_input_section, sizeof(user_input_section), stdin) == NULL) {
      printf("Error: Failed to read user input.\n");
      continue;
    }

    // 移除換行符
    user_input_section[strcspn(user_input_section, "\n")] = 0;

    // 解析輸入並寫入 cmd_buf
    size_t buf_len = 0;
    parse_to_cmd_buf(user_input_section, to_tofis_buf, &buf_len);

    // 寫入串列埠
    if (write_serial(port, to_tofis_buf, strlen((char *)to_tofis_buf)) < 0) {
      printf("Error: Failed to write to serial port.\n");
    } else {
      printf("Command sent: %s\n", to_tofis_buf);
    }
  }

#ifdef _WIN32
  return 0;
}
#else
  return NULL;
}
#endif

int tofis_host_api_init(const char *port_name, int baud_rate) {
  // 初始化串口
  if (init_serial(&serial_port, port_name, baud_rate) < 0) {
    return -1;
  }

  // 初始化 mutex 和 condition variable
#ifdef _WIN32
  data_mutex = CreateMutex(NULL, FALSE, NULL);
  if (data_mutex == NULL) {
    printf("Error: Unable to create mutex.\n");
    close_serial(&serial_port);
    return -1;
  }

  data_cond = CreateEvent(NULL, FALSE, FALSE, NULL);
  if (data_cond == NULL) {
    printf("Error: Unable to create event.\n");
    CloseHandle(data_mutex);
    close_serial(&serial_port);
    return -1;
  }

  // 創建接收線程
  receive_thread = CreateThread(NULL, 0, receive_thread_func, NULL, 0, NULL);
  if (receive_thread == NULL) {
    printf("Error: Unable to create receive thread.\n");
    CloseHandle(data_cond);
    CloseHandle(data_mutex);
    close_serial(&serial_port);
    return -1;
  }

  // 創建用戶輸入線程
  input_thread = CreateThread(NULL, 0, user_input_thread_func,
                              (LPVOID)&serial_port, 0, NULL);
  if (input_thread == NULL) {
    printf("Error: Unable to create input thread.\n");
    TerminateThread(receive_thread, 0);
    CloseHandle(receive_thread);
    CloseHandle(data_cond);
    CloseHandle(data_mutex);
    close_serial(&serial_port);
    return -1;
  }
#else
  // 創建接收線程
  if (pthread_create(&receive_thread_p, NULL, receive_thread_func, NULL) != 0) {
    printf("Error: Unable to create receive thread.\n");
    close_serial(&serial_port);
    return -1;
  }

  // 創建用戶輸入線程
  if (pthread_create(&input_thread_p, NULL, user_input_thread_func,
                     (void *)&serial_port) != 0) {
    printf("Error: Unable to create input thread.\n");
    pthread_cancel(receive_thread_p);
    pthread_join(receive_thread_p, NULL);
    close_serial(&serial_port);
    return -1;
  }
#endif

  return 0;
}

int tofis_host_api_start() {
  // 在這個實現中，接收線程已經在初始化時啟動
  // 可以根據需要添加額外的啟動邏輯
  return 0;
}

int tofis_host_api_wait_for_data(tofis_data_packet_t *packet) {
  // 等待數據到來
#ifdef _WIN32
  WaitForSingleObject(data_cond, INFINITE);
  WaitForSingleObject(data_mutex, INFINITE);
  if (data_ready) {
    *packet = latest_packet;
    data_ready = false;
  }
  ReleaseMutex(data_mutex);
#else
  pthread_mutex_lock(&data_mutex_p);
  while (!data_ready) {
    pthread_cond_wait(&data_cond_p, &data_mutex_p);
  }
  *packet = latest_packet;
  data_ready = false;
  pthread_mutex_unlock(&data_mutex_p);
#endif
  return 0;
}

void tofis_host_api_cleanup() {
  // 關閉串口
  close_serial(&serial_port);

  // 關閉 mutex 和 condition variable
#ifdef _WIN32
  // 終止接收線程和輸入線程（可選，需更完善的終止機制）
  TerminateThread(receive_thread, 0);
  CloseHandle(receive_thread);
  TerminateThread(input_thread, 0);
  CloseHandle(input_thread);

  CloseHandle(data_cond);
  CloseHandle(data_mutex);
#else
  // 停止接收線程和輸入線程（需要更完善的終止機制，例如使用全局變量來通知線程退出）
  // 目前無法直接停止 pthread，僅示範
  pthread_cancel(receive_thread_p);
  pthread_join(receive_thread_p, NULL);
  pthread_cancel(input_thread_p);
  pthread_join(input_thread_p, NULL);

  pthread_cond_destroy(&data_cond_p);
  pthread_mutex_destroy(&data_mutex_p);
#endif
}
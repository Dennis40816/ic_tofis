// main.c
#include "tofis_data.h"
#include "tofis_host_api.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Timer related
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

static uint64_t packet_count = 0;
#ifdef _WIN32
LARGE_INTEGER frequency, last_time, current_time;
#else
struct timespec last_time, current_time;
#endif
double time_diff;

// 定義 Profile 結構體
typedef struct {
  uint8_t RangingProfile; // 8 for 8x8, 4 for 4x4
  uint8_t EnableAmbient;
  uint8_t EnableSignal;
} Profile_t;

// 全局 Profile 變數
Profile_t Profile = {0};

// 顯示命令橫幅
static void display_commands_banner(void) {
  static const int col_len = 40;

  // move to second row
  printf("%c[H", 27);

  printf("TOFIS Simple Ranging demo application\033[K\n");
  printf("--------------------------------------\033[K\n\033[K\n");

  printf("Use the following keys to control application\033[K\n");
  printf(" %-*s %-*s\033[K\n", col_len, " 'r' : change resolution", col_len,
         " 's' : enable signal and ambient");
  printf(" %-*s %-*s\033[K\n", col_len, " 'c' : clear screen", col_len,
         " 't' : toggle target order");
  printf("\033[K\n");
}

// 初始化計時器
void init_timer() {
#ifdef _WIN32
  QueryPerformanceFrequency(&frequency); // 獲取計時器的頻率
  QueryPerformanceCounter(&last_time);   // 獲取當前的計時器計數
#else
  clock_gettime(CLOCK_MONOTONIC, &last_time); // 獲取初始時間
#endif
}

// 獲取當前時間，並計算與上次的時間差
void calculate_time_diff() {
#ifdef _WIN32
  QueryPerformanceCounter(&current_time);
  time_diff =
      (double)(current_time.QuadPart - last_time.QuadPart) / frequency.QuadPart;
  last_time = current_time; // 更新 last_time
#else
  clock_gettime(CLOCK_MONOTONIC, &current_time);
  time_diff = (current_time.tv_sec - last_time.tv_sec) +
              (current_time.tv_nsec - last_time.tv_nsec) / 1e9;
  last_time = current_time; // 更新 last_time
#endif
}

static void clear_rest() { printf("\033[J"); }

// 打印結果函數
static void print_result(RANGING_SENSOR_Result_t *Result) {
  int8_t i, j, k, l;
  uint8_t zones_per_line;

  zones_per_line =
      ((Profile.RangingProfile == 8) ? 8 : 4); // 假設 Profile 設置為 8x8 或 4x4

  display_commands_banner();

  printf("Cell Format :\033[K\n\033[K\n");
  for (l = 0; l < RANGING_SENSOR_NB_TARGET_PER_ZONE; l++) {
    printf(" \033[38;5;10m%20s\033[0m : %20s\033[K\n", "Distance [mm]",
           "Status");
    if ((Profile.EnableAmbient != 0) || (Profile.EnableSignal != 0)) {
      printf(" %20s : %20s\033[K\n", "Signal [kcps/spad]",
             "Ambient [kcps/spad]\033[K");
    }
  }

  printf("\033[K\n\033[K\n");

  for (j = 0; j < Result->NumberOfZones; j += zones_per_line) {
    // 打印上邊框
    for (i = 0; i < zones_per_line; i++) /* number of zones per line */
    {
      printf(" -----------------");
    }
    printf("\033[K\n");

    // 打印中間空白行
    for (i = 0; i < zones_per_line; i++) {
      printf("|                 ");
    }
    printf("|\033[K\n");

    for (l = 0; l < RANGING_SENSOR_NB_TARGET_PER_ZONE; l++) {
      /* Print distance and status */
      for (k = (zones_per_line - 1); k >= 0; k--) {
        if (j + k >= Result->NumberOfZones) {
          printf("| %5s  :  %5s ", "X", "X");
          continue;
        }

        if (Result->ZoneResult[j + k].NumberOfTargets > 0)
          printf("| \033[38;5;10m%5ld\033[0m  :  %5ld ",
                 (long)Result->ZoneResult[j + k].Distance[l],
                 (long)Result->ZoneResult[j + k].Status[l]);
        else
          printf("| %5s  :  %5s ", "X", "X");
      }
      printf("|\033[K\n");

      if ((Profile.EnableAmbient != 0) || (Profile.EnableSignal != 0)) {
        /* Print Signal and Ambient */
        for (k = (zones_per_line - 1); k >= 0; k--) {
          if (j + k >= Result->NumberOfZones) {
            printf("| %5s  :  %5s ", "X", "X");
            continue;
          }

          if (Result->ZoneResult[j + k].NumberOfTargets > 0) {
            if (Profile.EnableSignal != 0) {
              printf("| %5ld  :  ", (long)Result->ZoneResult[j + k].Signal[l]);
            } else
              printf("| %5s  :  ", "X");

            if (Profile.EnableAmbient != 0) {
              printf("%5ld ", (long)Result->ZoneResult[j + k].Ambient[l]);
            } else
              printf("%5s ", "X");
          } else {
            printf("| %5s  :  %5s ", "X", "X");
          }
        }
        printf("|\033[K\n");
      }
    }
  }

  // 打印下邊框
  for (i = 0; i < zones_per_line; i++) {
    printf(" -----------------");
  }
  printf("\033[K\n");
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Usage: %s <serial_port>\n", argv[0]);
    printf("Example:\n");
#ifdef _WIN32
    printf("  %s COM3\n", argv[0]);
#else
    printf("  %s /dev/ttyUSB0\n", argv[0]);
#endif
    return -1;
  }

  const char *port_name = argv[1];

  // 初始化 Host API，波特率設為 460800
  if (tofis_host_api_init(port_name, 460800) < 0) {
    return -1;
  }

  // 啟動接收（在此實現中，已在初始化時啟動）
  tofis_host_api_start();
  init_timer();

  printf("Waiting for data on %s...\n", port_name);

  while (1) {
    tofis_data_packet_t packet;
    if (tofis_host_api_wait_for_data(&packet) == 0) {
      calculate_time_diff();

      print_result(&packet.data);
      printf("Packet frequency: %6.2f Hz\033[K\n", 1.0 / time_diff);

      // 更新 Profile 參數
      Profile.RangingProfile = (packet.resolution == 8) ? 8 : 4;
      Profile.EnableAmbient = 1; // 假設啟用 Ambient
      Profile.EnableSignal = 1;  // 假設啟用 Signal
      clear_rest();
    }
  }

  // 清理 Host API
  tofis_host_api_cleanup();

  return 0;
}

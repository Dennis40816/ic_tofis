// serial_port.h
#pragma once

#include <stddef.h>
#include <stdint.h>


#ifdef _WIN32
#include <windows.h>
typedef HANDLE serial_handle_t;
#else
#include <termios.h>
typedef int serial_handle_t;
#endif

typedef struct {
  serial_handle_t handle;
} SerialPort;

// 初始化串口
int init_serial(SerialPort *port, const char *port_name, int baud_rate);

// 關閉串口
void close_serial(SerialPort *port);

// 串口接收數據
int read_serial(SerialPort *port, uint8_t *buffer, size_t size);

// 串口傳輸數據
int write_serial(SerialPort *port, const uint8_t *buffer, size_t size);
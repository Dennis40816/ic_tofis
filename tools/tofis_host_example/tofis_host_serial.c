// serial_port.c
#include "tofis_host_serial.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#endif

int init_serial(SerialPort *port, const char *port_name, int baud_rate) {
#ifdef _WIN32
  // Windows 串口初始化
  port->handle = CreateFileA(port_name, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                             OPEN_EXISTING, 0, NULL);
  if (port->handle == INVALID_HANDLE_VALUE) {
    printf("Error: Unable to open serial port %s\n", port_name);
    return -1;
  }

  DCB dcbSerialParams = {0};
  dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
  if (!GetCommState(port->handle, &dcbSerialParams)) {
    printf("Error: Getting serial state\n");
    CloseHandle(port->handle);
    return -1;
  }

  dcbSerialParams.BaudRate = baud_rate;
  dcbSerialParams.ByteSize = 8;
  dcbSerialParams.StopBits = ONESTOPBIT;
  dcbSerialParams.Parity = NOPARITY;

  if (!SetCommState(port->handle, &dcbSerialParams)) {
    printf("Error: Setting serial parameters\n");
    CloseHandle(port->handle);
    return -1;
  }

  // 设置超时
  COMMTIMEOUTS timeouts = {0};
  timeouts.ReadIntervalTimeout = 50;
  timeouts.ReadTotalTimeoutConstant = 50;
  timeouts.ReadTotalTimeoutMultiplier = 10;
  timeouts.WriteTotalTimeoutConstant = 50;
  timeouts.WriteTotalTimeoutMultiplier = 10;

  if (!SetCommTimeouts(port->handle, &timeouts)) {
    printf("Error: Setting serial timeouts\n");
    CloseHandle(port->handle);
    return -1;
  }

  return 0;
#else
  // Linux 串口初始化
  port->handle = open(port_name, O_RDWR | O_NOCTTY | O_SYNC);
  if (port->handle < 0) {
    printf("Error: Unable to open serial port %s\n", port_name);
    return -1;
  }

  struct termios tty;
  memset(&tty, 0, sizeof(tty));
  if (tcgetattr(port->handle, &tty) != 0) {
    printf("Error: Getting serial attributes\n");
    close(port->handle);
    return -1;
  }

  speed_t speed;
  switch (baud_rate) {
  case 9600:
    speed = B9600;
    break;
  case 19200:
    speed = B19200;
    break;
  case 38400:
    speed = B38400;
    break;
  case 57600:
    speed = B57600;
    break;
  case 115200:
    speed = B115200;
    break;
  default:
    speed = B9600;
  }

  cfsetospeed(&tty, speed);
  cfsetispeed(&tty, speed);

  tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8 bits
  tty.c_iflag &= ~IGNBRK;                     // Disable break processing
  tty.c_lflag = 0;      // No signaling chars, no echo, no canonical processing
  tty.c_oflag = 0;      // No remapping, no delays
  tty.c_cc[VMIN] = 1;   // Read doesn't block
  tty.c_cc[VTIME] = 10; // 1 second read timeout

  tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Disable XON/XOFF
  tty.c_cflag |= (CLOCAL | CREAD);   // Ignore modem controls, enable reading
  tty.c_cflag &= ~(PARENB | PARODD); // No parity
  tty.c_cflag &= ~CSTOPB;            // One stop bit
  tty.c_cflag &= ~CRTSCTS;           // No hardware flow control

  if (tcsetattr(port->handle, TCSANOW, &tty) != 0) {
    printf("Error: Setting serial attributes\n");
    close(port->handle);
    return -1;
  }

  return 0;
#endif
}

void close_serial(SerialPort *port) {
#ifdef _WIN32
  CloseHandle(port->handle);
#else
  close(port->handle);
#endif
}

int read_serial(SerialPort *port, uint8_t *buffer, size_t size) {
#ifdef _WIN32
  DWORD bytes_read;
  if (!ReadFile(port->handle, buffer, (DWORD)size, &bytes_read, NULL)) {
    return -1;
  }
  return (int)bytes_read;
#else
  int bytes_read = read(port->handle, buffer, size);
  if (bytes_read < 0) {
    return -1;
  }
  return bytes_read;
#endif
}

int write_serial(SerialPort *port, const uint8_t *buffer, size_t size) {
#ifdef _WIN32
  DWORD bytes_written;
  if (!WriteFile(port->handle, buffer, (DWORD)size, &bytes_written, NULL)) {
    return -1;
  }
  return (int)bytes_written;
#else
  int bytes_written = write(port->handle, buffer, size);
  if (bytes_written < 0) {
    return -1;
  }
  return bytes_written;
#endif
}

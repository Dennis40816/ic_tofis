# HOST C API

## WARN

1. baud rate is 460800 (STM32 demo is 115200)

## Compile
```bash
## Linux
gcc -o host_program tofis_main.c tofis_host_api.c tofis_host_serial.c -lpthread


## Windows
gcc -o host_program.exe tofis_main.c tofis_host_api.c tofis_host_serial.c
```

## Usage
```bash
## Linux(Not Tested)
# you should change /dev/ttyUSB0 to your device USB
./host_program /dev/ttyUSB0

## Windows
# you should change COM5 to your device COM
host_program.exe COM5
```
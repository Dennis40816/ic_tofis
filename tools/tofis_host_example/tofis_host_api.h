// tofis_host_api.h
#pragma once

#include "tofis_main.h"

#include "tofis_data.h"

#define TOFIS_USER_INPUT_BUF_SIZE (256)

// 初始化 Host API
int tofis_host_api_init(const char *port_name, int baud_rate);

// 啟動接收線程
int tofis_host_api_start();

// 等待並獲取最新的數據包
int tofis_host_api_wait_for_data(tofis_data_packet_t *packet);

// 清理 Host API
void tofis_host_api_cleanup();

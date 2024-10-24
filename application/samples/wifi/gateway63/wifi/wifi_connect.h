/**
 * Copyright (c) HiSilicon (Shanghai) Technologies Co., Ltd. 2022-2023. All rights reserved.
 *
 * Description: Application core main function for standard \n
 *
 * History: \n
 * 2022-07-27, Create file. \n
 */


#ifndef WIFI_CONNECT_H
#define WIFI_CONNECT_H
#include "wifi_linked_info.h"
#include <stdint.h>
#include "td_type.h"
#define CONFIG_WIFI_SSID            "test"          //test     // 要连接的WiFi 热点账号
#define CONFIG_WIFI_PWD             "88888888"                        // 要连接的WiFi 热点密码

int wifi_connect(const char *ssid, const char *psk);
td_bool example_check_connect_status(td_void);
td_s32 example_sta_function(const char *ssid, const char *psk);
extern td_u8 g_wifi_state_while;
#endif
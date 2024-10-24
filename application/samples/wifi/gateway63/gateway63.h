/**
 * Copyright (c) HiSilicon (Shanghai) Technologies Co., Ltd. 2023-2023. All rights reserved.
 *
 * Description: BLE uart server Config. \n
 *
 * History: \n
 * 2023-07-26, Create file. \n
 */
#ifndef GATEWAY63_H
#define GATEWAY63_H

#include <stdint.h>
#include "errcode.h"

#define HI3863_OPENVALLEY_WIFI_LEN 32
#define HI3863_OPENVALLEY_PWD_LEN 64
#define MEASURE_DIS_MSG_QUEUE_SIZE 16

typedef enum {
    HI3863_MSG_E_NO = 0,
    HI3863_MSG_E_CONNECT,
    HI3863_MSG_E_REPORT,
    HI3863_MSG_E_SAVE_WIFI,
    HI3863_MSG_E_SAVE_SERVER,
    HI3863_MSG_E_MAX
}hi3863_msg_type_e;

//WiFi信息
typedef struct {
    uint8_t wifi[HI3863_OPENVALLEY_WIFI_LEN];
    uint8_t passwd[HI3863_OPENVALLEY_PWD_LEN];
    uint32_t encrypt;    /* 用来表示WiFi是否有加密，0表示不加密，1表示加密*/
} hi3863_wifi_info_t;

//服务器信息
typedef struct {
    uint8_t server_ip[HI3863_OPENVALLEY_WIFI_LEN];
    uint32_t server_port;
} hi3863_server_info_t;


typedef struct {
    hi3863_msg_type_e    type;         /*!< 消息类型 */
    void                 *data;        /*!< 消息数据 */
    uint16_t             data_len;     /*!< 消息数据长度 */
} measure_dis_msg_t;

void hi3863_msg_queue_send_msg(const void *data, uint16_t length, hi3863_msg_type_e msgId);

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */









#ifdef __cplusplus
#if __cplusplus
}
#endif  /* __cplusplus */
#endif  /* __cplusplus */
#endif
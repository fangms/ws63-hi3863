
#include "lwip/nettool/misc.h"
#include "lwip/ip4_addr.h"
#include "lwip/netif.h"
#include "lwip/netifapi.h"
#include "lwip/sockets.h"

#include "cmsis_os2.h"
#include "app_init.h"
#include "soc_osal.h"
#include "wifi_connect.h"
#include "lwip/api.h"
#include "lwip/sockets.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/dns.h"
#include "lwip/netdb.h" 
#include <lwip/netdb.h>
#include <lwip/tcpip.h>
#include <lwip/api.h>
#include <stdio.h>
#include <string.h>
#include <lwip/tcp.h>
#include "lwip/init.h"

#include "nv.h"
#include "key_id.h"
#include <cJSON.h>
#include "uart.h"
#include "pinctrl.h"
#include "wifi_linked_info.h"
#include "tcp_http_dns.h"
#include "gateway63.h"

// #include "ble_uart_server/ble_uart_server.h"
#define GATEWAY63_TASK_PRIO                  (osPriority_t)(13)
#define GATEWAY63_TASK_DURATION_MS           2000
#define GATEWAY63_TASK_STACK_SIZE            0x4000

#define CONFIG_SERVER_IP          "120.55.170.12"
#define CONFIG_SERVER_PORT          6655                              // 要连接的服务器IP、端口                          // 要连接的服务器端口

#define SLE_UART_TRANSFER_SIZE  512
// #define CONFIG_SLE_UART_BUS 1  // 0
// #define CONFIG_UART_TXD_PIN  15  // 17
// #define CONFIG_UART_RXD_PIN  14  // 18
#define SLE_UART_BAUDRATE 115200

static uint8_t g_app_uart_rx_buff[SLE_UART_TRANSFER_SIZE] = { 0 };
static int IS_READ_DATA_FLAG = 0; // 接收数据中断标志
int g_socket_handle;

hi3863_wifi_info_t g_wifiInfo = {0};
hi3863_server_info_t g_serverInfo = {0};

static err_t gateway63_nvConfigRead(void)
{
    uint16_t key_len = 0;
    uint16_t real_len = 0;
    hi3863_wifi_info_t *wifi_value = NULL;
    hi3863_server_info_t *server_value = NULL;

    memset_s(&g_wifiInfo, sizeof(hi3863_wifi_info_t), 0 , sizeof(hi3863_wifi_info_t));
    memset_s(&g_serverInfo, sizeof(hi3863_server_info_t), 0 , sizeof(hi3863_server_info_t));

    key_len = (uint16_t)sizeof(hi3863_wifi_info_t);
    wifi_value = (hi3863_wifi_info_t *) osal_vmalloc(key_len);
    if(wifi_value == NULL) {
        osal_printk("[ERROR]:gateway63_nvConfigRead osal_vmalloc fail %d\r\n", __LINE__);
        return -1;
    }
    memset_s(wifi_value, sizeof(hi3863_wifi_info_t), 0 , sizeof(hi3863_wifi_info_t));

#if defined(CONFIG_MIDDLEWARE_SUPPORT_NV)
    if (uapi_nv_read(NV_ID_OPENVALLEY_WIFI_INFO, key_len, &real_len, (uint8_t *)wifi_value) != ERRCODE_SUCC) {
        osal_printk("[ERROR]:gateway63_nvConfigRead uapi_nv_read fail %d\r\n", __LINE__);
        osal_vfree(wifi_value);
        wifi_value = NULL;
        return -1;
    }
#endif
    osal_printk("[NV]: Old:wifi=%s,passwd=%s,encrypt=%d\r\n", wifi_value->wifi, 
                    wifi_value->passwd, wifi_value->encrypt);
    if (memcpy_s(g_wifiInfo.wifi, HI3863_OPENVALLEY_PWD_LEN, 
                wifi_value->wifi, strlen((char *)(wifi_value->wifi))) != EOK) {
        return -1;
    };
    if (memcpy_s(g_wifiInfo.passwd, HI3863_OPENVALLEY_PWD_LEN, 
                wifi_value->passwd, strlen((char *)(wifi_value->passwd))) != EOK) {
        return -1;
    };
    g_wifiInfo.encrypt = wifi_value->encrypt;
    
    if (wifi_value != NULL) {
        osal_vfree(wifi_value);
        wifi_value = NULL;
    }

    real_len = 0;
    key_len = (uint16_t)sizeof(hi3863_server_info_t);
    server_value = (hi3863_server_info_t *) osal_vmalloc(key_len);
    if(server_value == NULL) {
        osal_printk("[ERROR]:gateway63_nvConfigRead osal_vmalloc fail %d\r\n", __LINE__);
        return -1;
    }
    memset_s(server_value, sizeof(hi3863_server_info_t), 0 , sizeof(hi3863_server_info_t));

#if defined(CONFIG_MIDDLEWARE_SUPPORT_NV)
    if (uapi_nv_read(NV_ID_OPENVALLEY_SERVER_INFO, key_len, &real_len, (uint8_t *)server_value) != ERRCODE_SUCC) {
        osal_printk("[ERROR]:gateway63_nvConfigRead uapi_nv_read fail %d\r\n", __LINE__);
        osal_vfree(server_value);
        server_value = NULL;
        return -1;
    }
#endif

    osal_printk("[NV]:server_ip=%s,server_port=%d\r\n", server_value->server_ip, server_value->server_port);
    if (memcpy_s(g_serverInfo.server_ip, HI3863_OPENVALLEY_PWD_LEN, 
            server_value->server_ip, strlen((char *)(server_value->server_ip))) != EOK) {
        return -1;
    };
    g_serverInfo.server_port = server_value->server_port;

    if (server_value != NULL) {
        osal_vfree(server_value);
        server_value = NULL;
    }

    return ERR_OK;
}

#if 0
static err_t gateway63_saveWifiInfo(hi3863_wifi_info_t * wifiInfo)
{
    uint16_t key_len = (uint16_t)sizeof(hi3863_wifi_info_t);
    uint16_t real_len = 0;
    hi3863_wifi_info_t *read_value = NULL;

    osal_printk("[NV]:AT:wifi:%s, passwd:%s, encrypt=%d\r\n", wifiInfo->wifi, wifiInfo->passwd, wifiInfo->encrypt);
    if(strlen(wifiInfo->wifi) >= HI3863_OPENVALLEY_WIFI_LEN ||
       strlen(wifiInfo->passwd) >= HI3863_OPENVALLEY_PWD_LEN  ||
       wifiInfo->encrypt > 1) {
        osal_printk("[ERROR]:gateway63_saveWifiInfo wifiInfo is error %d\r\n", __LINE__);
        return -1;
    }

    read_value = (hi3863_wifi_info_t *) osal_vmalloc(key_len);
    if(read_value == NULL) {
        osal_printk("[ERROR]:gateway63_saveWifiInfo osal_vmalloc fail %d\r\n", __LINE__);
        return -1;
    }
    memset_s(read_value, sizeof(hi3863_wifi_info_t), 0 , sizeof(hi3863_wifi_info_t));

#if defined(CONFIG_MIDDLEWARE_SUPPORT_NV)
    if (uapi_nv_read(NV_ID_OPENVALLEY_WIFI_INFO, key_len, &real_len, (uint8_t *)read_value) != ERRCODE_SUCC) {
        osal_printk("[ERROR]:gateway63_saveWifiInfo uapi_nv_read fail %d\r\n", __LINE__);
        osal_vfree(read_value);
        read_value = NULL;
        return -1;
    }
#endif

    osal_printk("[NV]: Old:wifi=%s,passwd=%s,encrypt=%d\r\n", read_value->wifi, 
                    read_value->passwd, read_value->encrypt);

    memset_s(read_value->wifi, HI3863_OPENVALLEY_PWD_LEN, 0, HI3863_OPENVALLEY_PWD_LEN);
    if (memcpy_s(read_value->wifi, HI3863_OPENVALLEY_PWD_LEN, wifiInfo->wifi, strlen(wifiInfo->wifi)) != EOK) {
        return -1;
    };
    memset_s(read_value->passwd, HI3863_OPENVALLEY_PWD_LEN, 0, HI3863_OPENVALLEY_PWD_LEN);
    if (memcpy_s(read_value->passwd, HI3863_OPENVALLEY_PWD_LEN, wifiInfo->passwd, strlen(wifiInfo->passwd)) != EOK) {
        return -1;
    };
    read_value->encrypt = wifiInfo->encrypt;

    errcode_t nv_ret_value = uapi_nv_write(NV_ID_OPENVALLEY_WIFI_INFO, (uint8_t *)read_value, key_len);
    if (nv_ret_value != ERRCODE_SUCC) {
        osal_printk("[ERROR]write nv fail! %d, ret:%x \r\n", __LINE__, nv_ret_value);
        osal_vfree(read_value);
        read_value = NULL;
        return nv_ret_value;
    }

    if (read_value != NULL) {
        osal_vfree(read_value);
        read_value = NULL;
    }
    return ERR_OK;
}

static err_t gateway63_saveServerInfo(hi3863_server_info_t *serverInfo)
{
    uint16_t key_len = (uint16_t)sizeof(hi3863_server_info_t);
    uint16_t real_len = 0;
    hi3863_server_info_t *read_value = NULL;

    osal_printk("[NV]:AT:server_ip:%s,server_port:%d\r\n", serverInfo->server_ip, serverInfo->server_port);
    if(strlen(serverInfo->server_ip) >= HI3863_OPENVALLEY_WIFI_LEN) {
        osal_printk("[ERROR]:gateway63_saveServerInfo serverInfo is error %d\r\n", __LINE__);
        return -1;
    }

    read_value = (hi3863_server_info_t *) osal_vmalloc(key_len);
    if(read_value == NULL) {
        osal_printk("[ERROR]:gateway63_saveServerInfo osal_vmalloc fail %d\r\n", __LINE__);
        return -1;
    }
    memset_s(read_value, sizeof(hi3863_server_info_t), 0 , sizeof(hi3863_server_info_t));

#if defined(CONFIG_MIDDLEWARE_SUPPORT_NV)
    if (uapi_nv_read(NV_ID_OPENVALLEY_SERVER_INFO, key_len, &real_len, (uint8_t *)read_value) != ERRCODE_SUCC) {
        osal_printk("[ERROR]:gateway63_saveServerInfo uapi_nv_read fail %d\r\n", __LINE__);
        osal_vfree(read_value);
        read_value = NULL;
        return -1;
    }
#endif

    osal_printk("[NV] Old:server_ip=%s,server_port=%d\r\n", read_value->server_ip, read_value->server_port);
    memset_s(read_value->server_ip, HI3863_OPENVALLEY_PWD_LEN, 0, HI3863_OPENVALLEY_PWD_LEN);
    if (memcpy_s(read_value->server_ip, HI3863_OPENVALLEY_PWD_LEN, 
                serverInfo->server_ip, strlen(serverInfo->server_ip)) != EOK) {
        return -1;
    };
    read_value->server_port = serverInfo->server_port;
    
    errcode_t nv_ret_value = uapi_nv_write(NV_ID_OPENVALLEY_SERVER_INFO, (uint8_t *)read_value, key_len);
    if (nv_ret_value != ERRCODE_SUCC) {
        osal_printk("[ERROR]write nv fail! %d, ret:%x \r\n", __LINE__, nv_ret_value);
        osal_vfree(read_value);
        read_value = NULL;
        return nv_ret_value;
    }

    if (read_value != NULL) {
        osal_vfree(read_value);
        read_value = NULL;
    }
    return ERR_OK;
}
#endif

err_t socket_connect(void)
{
    struct sockaddr_in server_addr;
    err_t err;
    g_socket_handle = lwip_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_socket_handle < 0) {
        return -1;
    }
 
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(CONFIG_SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(CONFIG_SERVER_IP);
 
    printf("lwip_connecting\r\n");
    err = lwip_connect(g_socket_handle, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (err != ERR_OK) {
        lwip_close(g_socket_handle);
        printf("lwip_connect fail %d\r\n", err);
        return err;
    }
    return 0;
}

err_t http_get_request(const char *hostname, const char *path)
{
    UNUSED(hostname);
    UNUSED(path);

    // struct sockaddr_in server_addr;
    err_t err;
 
    // g_socket_handle = lwip_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    socket_connect();
    if (g_socket_handle <= 0) {
        socket_connect();
        return -1;
    }
 
    // server_addr.sin_family = AF_INET;
    // server_addr.sin_port = htons(CONFIG_SERVER_PORT);
    // server_addr.sin_addr.s_addr = inet_addr(hostname);
 
    // printf("lwip_connecting\r\n");
    // err = lwip_connect(g_socket_handle, (struct sockaddr *)&server_addr, sizeof(server_addr));
    // if (err != ERR_OK) {
    //     lwip_close(g_socket_handle);
    //     printf("lwip_connect fail %d\r\n", err);
    //     return err;
    // }
    if(strlen((char *)g_app_uart_rx_buff)< 20)
    {
        printf("g_app_uart_rx_buff length<20 \r\n");
        memset((char *)g_app_uart_rx_buff, '\0', SLE_UART_TRANSFER_SIZE);
        return -1;
    }
    char request_buff[512]; // 构造post数据
    snprintf(request_buff, sizeof(request_buff), "POST   /v1/device/update_position HTTP/1.1\r\n"
                        "Host: http://120.55.170.12:6655\r\n"
                        "Content-Type: application/json\r\n"
                        "Connection: keep-alive\r\n"
                        "Content-Length: %d\r\n"
                        "\r\n"
                        "%s\r\n",
              strlen((char *)g_app_uart_rx_buff), g_app_uart_rx_buff);    
    printf("--------SEND:%s LEN:%d---------\r\n",request_buff,strlen(request_buff));
    printf("lwip_will_send\r\n");
    err = lwip_send(g_socket_handle, request_buff, strlen(request_buff), 0);   // POST JSON
    if (err < 0) {
        lwip_close(g_socket_handle);
        printf("lwip_send fail %d\r\n", err);
        return err;
    }
    lwip_close(g_socket_handle);
    return ERR_OK;
}
 
static uart_buffer_config_t g_app_uart_buffer_config = {
    .rx_buffer = g_app_uart_rx_buff,
    .rx_buffer_size = SLE_UART_TRANSFER_SIZE
};

static void uart_init_pin(void)
{
    if (CONFIG_SLE_UART_BUS == 0) {
        uapi_pin_set_mode(CONFIG_UART_TXD_PIN, PIN_MODE_1);
        uapi_pin_set_mode(CONFIG_UART_RXD_PIN, PIN_MODE_1);       
    }else if (CONFIG_SLE_UART_BUS == 1) {
        uapi_pin_set_mode(CONFIG_UART_TXD_PIN, PIN_MODE_1);
        uapi_pin_set_mode(CONFIG_UART_RXD_PIN, PIN_MODE_1);       
    }
}

static void uart_init_config(void)
{
    uart_attr_t attr = {
        .baud_rate = SLE_UART_BAUDRATE,
        .data_bits = UART_DATA_BIT_8,
        .stop_bits = UART_STOP_BIT_1,
        .parity = UART_PARITY_NONE
    };

    uart_pin_config_t pin_config = {
        .tx_pin = CONFIG_UART_TXD_PIN,
        .rx_pin = CONFIG_UART_RXD_PIN,
        .cts_pin = PIN_NONE,
        .rts_pin = PIN_NONE
    };
    uapi_uart_deinit(CONFIG_SLE_UART_BUS);
    uapi_uart_init(CONFIG_SLE_UART_BUS, &pin_config, &attr, NULL, &g_app_uart_buffer_config);

}
#define SLE_UART_SERVER_LOG                 "[uart http post task]"
// 串口接收中断，开启中断标志
static void sle_uart_server_read_int_handler(const void *buffer, uint16_t length, bool error)
{
    unused(error);
    memcpy((char *)g_app_uart_rx_buff,buffer,length);
    osal_printk("%s buffer = %s length = %d\r\n", SLE_UART_SERVER_LOG, buffer, length);
    IS_READ_DATA_FLAG = 1;
}

void uart_task(void)
{
    /* UART pinmux. */
    uart_init_pin();

    /* UART init config. */
    uart_init_config();
    // 注册接收回调 
    uapi_uart_unregister_rx_callback(CONFIG_SLE_UART_BUS);
    errcode_t ret = uapi_uart_register_rx_callback(CONFIG_SLE_UART_BUS,
                                                   UART_RX_CONDITION_MASK_IDLE,  // UART_RX_CONDITION_FULL_OR_SUFFICIENT_DATA_OR_IDLE
                                                   1, sle_uart_server_read_int_handler);
    if(ret !=ERRCODE_SUCC)
    {
        printf("uapi_uart_register_rx_callback failed!\r\n");
    }
}

int gateway63_sample_task(void *param)
{
    param = param;
    // 连接Wifi
    gateway63_nvConfigRead();
    wifi_connect(CONFIG_WIFI_SSID, CONFIG_WIFI_PWD);
    printf("will into while(1)!\r\n");
    // 注册串口接收中断
    uart_task();
    // socket_connect();
    while(1)
    {
        if(g_wifi_state_while != WIFI_CONNECTED) // 断开就重连
        {
            printf("example_check_connect_status UNCONNECTED!\r\n");
            IS_READ_DATA_FLAG = 0;
            example_sta_function(CONFIG_WIFI_SSID, CONFIG_WIFI_PWD);
            // socket_connect();
        }
        // else 
        if(IS_READ_DATA_FLAG && strlen((char*)g_app_uart_rx_buff))
        {
            http_get_request(CONFIG_SERVER_IP, "/upload_distance");
            IS_READ_DATA_FLAG = 0; // 清除串口中断标志
            memset((char *)g_app_uart_rx_buff, '\0', SLE_UART_TRANSFER_SIZE);
        }
        // 线程休眠一段时间
        osDelay(1); // 10ms
    }
    return 0;
}
static void gateway63_sample_entry(void)
{
    osThreadAttr_t attr;
    attr.name       = "gateway63_sample_task";
    attr.attr_bits  = 0U;
    attr.cb_mem     = NULL;
    attr.cb_size    = 0U;
    attr.stack_mem  = NULL;
    attr.stack_size = GATEWAY63_TASK_STACK_SIZE;
    attr.priority   = GATEWAY63_TASK_PRIO;
    if (osThreadNew((osThreadFunc_t)gateway63_sample_task, NULL, &attr) == NULL) {
        printf("Create gateway63_sample_task fail.\r\n");
    }
    printf("Create gateway63_sample_task succ.\r\n");
}

/* Run the gateway63_sample_task. */
app_run(gateway63_sample_entry);
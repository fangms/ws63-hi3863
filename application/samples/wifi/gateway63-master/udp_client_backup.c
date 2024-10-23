
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

#include <cJSON.h>
#include "uart.h"
#include "pinctrl.h"

#include "tcp_http_dns.h"
// #include "ble_uart_server/ble_uart_server.h"
#define UDP_CLIENT_TASK_PRIO                  (osPriority_t)(13)
#define UDP_CLIENT_TASK_DURATION_MS           2000
#define UDP_CLIENT_TASK_STACK_SIZE            0x4000

#define CONFIG_WIFI_SSID            "test"          //lichuanzhao     // 要连接的WiFi 热点账号
#define CONFIG_WIFI_PWD             "88888888"                        // 要连接的WiFi 热点密码
// #define CONFIG_SERVER_IP          "192.168.137.1"    // "120.55.170.12"                   // 要连接的服务器IP
#define CONFIG_SERVER_IP          "120.55.170.12"
#define CONFIG_SERVER_PORT          6655                              // 要连接的服务器端口                          // 要连接的服务器端口

#define SLE_UART_TRANSFER_SIZE  512
// #define CONFIG_SLE_UART_BUS 1  // 0
// #define CONFIG_UART_TXD_PIN  15  // 17
// #define CONFIG_UART_RXD_PIN  14  // 18
#define SLE_UART_BAUDRATE 115200

uint8_t g_get_sle_data[500] = {0}; // 全局数据
int g_IS_READ_DATA_FLAG = 0; // 接收中断标志
uint8_t g_buffer_data[SLE_UART_TRANSFER_SIZE] = {0}; // 全局数据

static uint8_t g_app_uart_rx_buff[SLE_UART_TRANSFER_SIZE] = { 0 };
static int IS_READ_DATA_FLAG = 0; // 中断标志
// char content[SLE_UART_TRANSFER_SIZE] = { 0 };
/**数据格式
 * {
    "identify": "ibs0976312",
    "distance": 49.8,
    "conf": 0.9783562459007697,
    "user_identify": "uid0009002"
}
 */
// 创建一个简单的JSON对象
// char* create_json_data(void) {
//     cJSON *json = cJSON_CreateObject();
//     cJSON_AddStringToObject(json, "identify", (char *)g_app_uart_rx_buff); //  g_get_sle_data
//     cJSON_AddNumberToObject(json, "distance", 49.8);
//     cJSON_AddNumberToObject(json, "conf", 0.9783562459007697);
//     cJSON_AddStringToObject(json, "user_identify", "uid0009002");    
//     char *jsonData = cJSON_PrintUnformatted(json); // 不使用格式化输出以节省资源
//     cJSON_Delete(json);
//     return jsonData;
// }

err_t http_get_request(const char *hostname, const char *path)
{
    UNUSED(path);
    int s;
    struct sockaddr_in server_addr;
    err_t err;
 
    s = lwip_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s < 0) {
        return -1;
    }
 
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(CONFIG_SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(hostname);
 
    printf("lwip_connecting\r\n");
    err = lwip_connect(s, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (err != ERR_OK) {
        lwip_close(s);
        printf("lwip_connect fail %d\r\n", err);
        return err;
    }
    if(strlen((char *)g_buffer_data)< 20)
    {
        printf("g_app_uart_rx_buff length<20 \r\n");
        memset((char *)g_buffer_data, '\0', SLE_UART_TRANSFER_SIZE);
        return -1;
    }
    // char *JsonData = create_json_data();
    char request_buff[512];
    snprintf(request_buff, sizeof(request_buff), "POST /upload_distance HTTP/1.1\r\n"
                        "Host: https://120.55.170.12:6655\r\n"
                        "Content-Type: application/json\r\n"
                        "Content-Length: %d\r\n"
                        "\r\n"
                        "%s\r\n",
              strlen((char *)g_buffer_data), g_buffer_data);
                            // strlen(JsonData), JsonData);
              
    printf("--------SEND:%s LEN:%d---------\r\n",request_buff,strlen(request_buff));
    printf("lwip_will_send\r\n");
    err = lwip_send(s, request_buff, strlen(request_buff), 0);   // POST JSON
    if (err < 0) {
        lwip_close(s);
        printf("lwip_send fail %d\r\n", err);
        return err;
    }
 
    // char buffer[512];
    // snprintf(buffer, sizeof(buffer), "GET %s HTTP/1.1\r\n"
    //                                  "Host: %s\r\n"
    //                                  "Connection: close\r\n\r\n",
    //          path, hostname);
    // err = lwip_send(s, buffer, strlen(buffer), 0); // GET

    // 接收数据
    // size_t bytes_read;
    // do {
    //     bytes_read = lwip_recv(s, buffer, sizeof(buffer) - 1, 0);
    //     if (bytes_read > 0) {
    //         buffer[bytes_read] = 0;
    //         // 处理接收到的数据
    //         printf("%s", buffer);
    //     }
    // } while(bytes_read > 0);
 
    lwip_close(s);
    return ERR_OK;
}
 
// 使用方法:
void get_http_data(void)
{
    http_get_request(CONFIG_SERVER_IP, "/index.html");
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
    // unused(buffer);
    // if(length < 50)
    // {
    //     printf("g_app_uart_rx_buff<50\r\n");
    //     // memset((char *)g_app_uart_rx_buff, '\0', SLE_UART_TRANSFER_SIZE);
    //     // IS_READ_DATA_FLAG = 0;
    //     return;
    // }
    memcpy((char *)g_buffer_data,buffer,length);
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
                                                   UART_RX_CONDITION_MASK_IDLE,
                                                   1, sle_uart_server_read_int_handler);
    if(ret !=ERRCODE_SUCC)
    {
        printf("uapi_uart_register_rx_callback failed!\r\n");
    }
}

int udp_client_sample_task(void *param)
{
    param = param;
    // 连接Wifi
    wifi_connect(CONFIG_WIFI_SSID, CONFIG_WIFI_PWD);
    printf("will into while(1)!\r\n");
    // 注册串口接收中断
    uart_task();
    while(1)
    {
        // uapi_uart_read(CONFIG_SLE_UART_BUS, g_buffer_data, SLE_UART_TRANSFER_SIZE, 1000);
        if(example_check_connect_status() != 0)
        {
            printf("example_check_connect_status UNCONNECTED!\r\n");
            example_sta_function(CONFIG_WIFI_SSID, CONFIG_WIFI_PWD);
            // wifi_connect(CONFIG_WIFI_SSID, CONFIG_WIFI_PWD);
            IS_READ_DATA_FLAG = 0;
        }
        else if(IS_READ_DATA_FLAG && strlen((char*)g_buffer_data))
        {
            // vDnsClientTask();
            // dns_main();
            // printf("will http_get_request!\r\n");
            get_http_data();
            IS_READ_DATA_FLAG = 0; // 清除串口中断标志
            memset((char *)g_app_uart_rx_buff, '\0', SLE_UART_TRANSFER_SIZE);
        }
        // 线程休眠一段时间
        osDelay(10); // 100ms
    }
#ifdef UDP_CLIENT_DEMO
    static const char *send_data = "Hello! I'm UDP Client!\r\n";
    // 在sock_fd 进行监听，在 new_fd 接收新的链接
    int sock_fd;

    // 服务器的地址信息
    struct sockaddr_in send_addr;
    socklen_t addr_length = sizeof(send_addr);
    char recvBuf[512];
    // 创建socket
    printf("create socket start!\r\n");
    if ((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        printf("create socket failed!\r\n");
        return 0;
    }
    printf("create socket end!\r\n");
    // 初始化预连接的服务端地址
    send_addr.sin_family = AF_INET;
    send_addr.sin_port = htons(CONFIG_SERVER_PORT);
    send_addr.sin_addr.s_addr = inet_addr(CONFIG_SERVER_IP);
    addr_length = sizeof(send_addr);

    while (1) {
        bzero(recvBuf, sizeof(recvBuf));

        // 发送数据到服务远端
        printf("sendto start!\r\n");
        sendto(sock_fd, send_data, strlen(send_data), 0, (struct sockaddr *)&send_addr, addr_length);
        printf("sendto end!\r\n");
        // 线程休眠一段时间
        osDelay(100);

        // 接收服务端返回的字符串
        recvfrom(sock_fd, recvBuf, sizeof(recvBuf), 0, (struct sockaddr *)&send_addr, &addr_length);
        printf("%s:%d=>%s\n", inet_ntoa(send_addr.sin_addr), ntohs(send_addr.sin_port), recvBuf);
    }
    // 关闭这个 socket
    lwip_close(sock_fd);
#endif
    return 0;
}
static void udp_client_sample_entry(void)
{
    osThreadAttr_t attr;
    attr.name       = "udp_client_sample_task";
    attr.attr_bits  = 0U;
    attr.cb_mem     = NULL;
    attr.cb_size    = 0U;
    attr.stack_mem  = NULL;
    attr.stack_size = UDP_CLIENT_TASK_STACK_SIZE;
    attr.priority   = UDP_CLIENT_TASK_PRIO;
    if (osThreadNew((osThreadFunc_t)udp_client_sample_task, NULL, &attr) == NULL) {
        printf("Create udp_client_sample_task fail.\r\n");
    }
    printf("Create udp_client_sample_task succ.\r\n");
}

/* Run the udp_client_sample_task. */
app_run(udp_client_sample_entry);
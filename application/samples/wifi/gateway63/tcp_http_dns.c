
#include "tcp_http_dns.h"
#include "ctype.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "lwip/ip4_addr.h"
#include "lwip/tcpip.h"

#include "lwip/err.h"
#include "lwip/dns.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/tcp.h"
#include "lwip/init.h"

#include <stdio.h>
#include <string.h>

// 定义JSON数据和目标服务器和端口
#include <cJSON.h>
static char* create_json_data(void) {
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "identify", "hello");
    cJSON_AddNumberToObject(json, "distance", 49.8);
    cJSON_AddNumberToObject(json, "conf", 0.9783562459007697);
    cJSON_AddStringToObject(json, "user_identify", "uid0009002");    
    char *jsonData = cJSON_PrintUnformatted(json); // 不使用格式化输出以节省资源
    cJSON_Delete(json);
    return jsonData;
}

#include "lwip/dns.h"
#include "lwip/tcp.h"
#include "lwip/inet.h"
#include <stdio.h>
#include <string.h>

#define PORT 6655
#define BUFFER_SIZE 1024

// TCP连接回调函数
static err_t tcp_connected(void *arg, struct tcp_pcb *tpcb, err_t err)
{
    LWIP_UNUSED_ARG(arg);
    if (err != ERR_OK) {
        printf("TCP connection failed with error: %d\n", err);
        return ERR_OK;
    }

    // 发送JSON数据
    char *JsonData = create_json_data();
    char request_buff[512];
    snprintf(request_buff, sizeof(request_buff), "POST /upload_distance HTTP/1.1\r\n"
                        "Host: 120.55.170.12:6655\r\n"
                        "Content-Type: application/json\r\n"
                        "Content-Length: %d\r\n"
                        "\r\n"
                        "%s\r\n",
              strlen(JsonData), JsonData);


    // 构建HTTP POST请求
    // const char *request_buff = "POST /upload_distance HTTP/1.1\r\n"
    //                       "Host: 120.55.170.12:6655\r\n"
    //                       "Content-Type: application/json\r\n"
    //                       "Content-Length: 28\r\n"
    //                       "\r\n"
    //                       "{\"key\":\"value\"}";

    printf("will tcp_write\r\n");
    // 发送HTTP请求
    err_t ret = tcp_write(tpcb, request_buff, strlen(request_buff), TCP_WRITE_FLAG_COPY);
    if(ret != ERR_OK)
    {
      printf("tcp_write \r\n");
    }
    // 关闭连接
    tcp_close(tpcb);

    return ERR_OK;
}

// TCP数据接收回调函数
err_t my_tcp_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(tpcb);
    if (err != ERR_OK) {
        printf("TCP receive failed with error: %d\n", err);
        return -1;
    }

    char *buf = (char *)malloc(p->tot_len + 1);
    if (buf == NULL) {
        printf("Memory allocation failed\n");
        return -1;
    }

    // 将接收到的数据复制到缓冲区
    memcpy(buf, p->payload, p->tot_len);
    buf[p->tot_len] = '\0';

    printf("Received response:\n%s\n", buf);

    free(buf);
    pbuf_free(p);
    return 0;
}
#include "cmsis_os2.h"
// 主函数
int dns_main(void)
{
    // 初始化LwIP
    // 这里省略了LwIP的初始化代码
    // 假设LwIP已经正确初始化

    // 创建TCP连接
    struct tcp_pcb *tpcb = tcp_new();
    tcp_arg(tpcb, NULL); // 设置回调参数为NULL
    tcp_recv(tpcb, my_tcp_recv); // 设置接收回调函数
    printf("will tcp_connect\r\n");
    ip_addr_t ip_server;
    // IP_ADDR4(&ip_server, 120,55,170,12);
    IP_ADDR4(&ip_server, 192,168,137,1);
    err_t ret = tcp_connect(tpcb, &ip_server, PORT, tcp_connected);
    if(ret != ERR_OK)
    {
      printf("tcp_connect fialed \r\n");
    }

    // 循环处理网络事件
    while (1) {
        // 处理网络事件
        // sys_check_timeouts();
        osDelay(10);
    }

    return 0;
}






/*
void setup_network_interface(void) {
    struct netif netif;
    ip4_addr_t ipaddr, netmask, gw;

    // 初始化 IP 地址、子网掩码和网关
    IP4_ADDR(&ipaddr, 192, 168, 0, 2);
    IP4_ADDR(&netmask, 255, 255, 255, 0);
    IP4_ADDR(&gw, 192, 168, 0, 1);

    // 添加网络接口
    netif_add(&netif, &ipaddr, &netmask, &gw);

    // 设置为默认接口
    netif_set_default(&netif);

    // 启动 DHCP 客户端（可选）
    // dhcp_start(&netif);
}

void setup_dns_servers(void) {
    ip_addr_t dns_server;
    IP_ADDR4(&dns_server, 8, 8, 8, 8); // Google DNS 服务器

    // 设置 DNS 服务器地址
    dns_setserver(0, &dns_server);
}

// void resolve_hostname(const char *hostname) {
//     ip_addr_t resolved_ip;

//     // 发起 DNS 查询
//     err_t err = dns_gethostbyname(hostname, &resolved_ip, NULL, NULL);

//     if (err == ERR_OK) {
//         // 解析成功
//         printf("Resolved IP address for %s: %s\n", hostname, ip4addr_ntoa(&resolved_ip));
//     } else {
//         // 解析失败
//         printf("Failed to resolve %s\n", hostname);
//     }
// }


void vDnsClientTask(void){
//   dns_init();
    // 初始化 LWIP 协议栈
  lwip_init();

    // 设置网络接口
  setup_network_interface();

    // 配置 DNS 服务器
  setup_dns_servers();
  int i;
  //dns 域名解析功能
  struct hostent *p_hostent = NULL;	
  p_hostent = gethostbyname("http://hellokun.cn");
  if(p_hostent){
    for(i = 0; p_hostent->h_addr_list[i]; i++){	
        // inet_ntoa(*((struct in_addr *)p_hostent->h_addr_list[i]));
        printf("host ip:%s\r\n", inet_ntoa(*(struct in_addr *)p_hostent->h_addr_list[i]));	
    }
  }else{	
    printf("get host ip fail!\r\n");
  }	
}


// 回调函数处理DNS解析结果
// (const char *hostname, const ip_addr_t *ipaddr, u32_t count, void *arg)
static void my_dns_found_callback(const char *hostname, const ip_addr_t *ipaddr, u32_t count, void *arg) {
    LWIP_UNUSED_ARG(arg);
    LWIP_UNUSED_ARG(count);
    int err;
    struct sockaddr_in dest_addr;
    int sock;

    if (ipaddr == NULL) {
      printf("DNS failed for %s\n", hostname);
      // return;
    }

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);
    dest_addr.sin_addr.s_addr = ipaddr->u_addr.ip4.addr;
    // dest_addr.sin_addr.s_addr = inet_addr("192.168.137.1"); //222.247.54.29

    sock = lwip_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // IPPROTO_IP = 0 
    if (sock < 0) {
      printf("socket failed\n");
      return;
    }

    err = connect(sock, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
      printf("connect %d failed\n", dest_addr.sin_addr.s_addr);
      lwip_close(sock);
      return;
    }

    // 发送JSON数据
    char *JsonData = create_json_data();
    char request_buff[512];
    snprintf(request_buff, sizeof(request_buff), "POST /gateway/indoor-location-service/upload_distance HTTP/1.1\r\n"
                        "Host: cloud-dev.openvalley.net\r\n"
                        "Content-Type: application/json\r\n"
                        "Content-Length: %d\r\n"
                        "%s\r\n",
              strlen(JsonData), JsonData);

    err = send(sock, request_buff, strlen(request_buff), 0);
    if (err < 0) {
      printf("send failed\n");
      lwip_close(sock);
      return;
    
        // 接收数据
    size_t bytes_read;
    do {
        bytes_read = lwip_recv(sock, request_buff, sizeof(request_buff) - 1, 0);
        if (bytes_read > 0) {
            request_buff[bytes_read] = 0;
            // 处理接收到的数据
            printf("%s", request_buff);
        }
    } while(bytes_read > 0);
  }
 
  // 接收响应数据（根据需要实现）
  // ...
 
  lwip_close(sock);
}
 
void resolve_hostname(const char *hostname) {
  ip_addr_t ipaddr;
  err_t err;
  u32_t count = 1;
  // my_dns_found_callback(hostname, &ipaddr, count, NULL);
  err = dns_gethostbyname(hostname, &ipaddr, &count, my_dns_found_callback, NULL);
  if (err == ERR_OK) {
    // DNS解析成功，直接调用回调函数
    my_dns_found_callback(hostname, &ipaddr, count, NULL);
  } else if (err == ERR_INPROGRESS) {
    // DNS解析还在进行中
    printf("DNS in progress\n");
  } else {
    // DNS解析失败
    printf("DNS failed\n");
  }
}
 
int dns_main(void) {
  // 初始化lwIP

  // 解析域名
  resolve_hostname(HOSTNAME);
 
 
  return 0;
}

*/
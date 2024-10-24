#include "lwip/dns.h"
#include "lwip/tcp.h"
#include "lwip/inet.h"
#include "mbedtls/ssl.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/debug.h"
#include <stdio.h>
#include <string.h>

#define MAX_IP_LENGTH 16
#define PORT 443
#define BUFFER_SIZE 1024

typedef struct {
    ip4_addr_t resolved_ip;
    bool dns_query_done;
} DnsQueryState;

// DNS查询回调函数
void dns_query_callback(struct dns_msg *msg, void *arg)
{
    if (msg->err == ERR_OK && msg->qcount > 0 && msg->ancount > 0) {
        // DNS查询成功，获取IP地址
        ip4_addr_t ip;
        ip4_addr_set_u32(&ip, msg->an[0].ip);
        char ip_str[MAX_IP_LENGTH];
        ip4_addr_ntoa_r(&ip, ip_str, sizeof(ip_str));
        printf("Resolved IP: %s\n", ip_str);

        // 将IP地址传递给下一步
        DnsQueryState *state = (DnsQueryState *)arg;
        state->resolved_ip = ip;
        state->dns_query_done = true;
    } else {
        printf("DNS query failed with error: %d\n", msg->err);
    }
}

// TLS连接回调函数
static int tls_connected(void *ctx, mbedtls_ssl_socket *ssl)
{
    printf("TLS connection established\n");
    return 0;
}

// TLS数据接收回调函数
static int tls_recv(void *ctx, mbedtls_ssl_socket *ssl, unsigned char *buf, size_t len)
{
    size_t bytes_received = 0;
    mbedtls_net_recv((mbedtls_net_context *)ctx, buf, len, &bytes_received);
    printf("Received %zu bytes\n", bytes_received);
    return 0;
}

// TLS数据发送回调函数
static int tls_send(void *ctx, mbedtls_ssl_socket *ssl, const unsigned char *buf, size_t len)
{
    size_t bytes_sent = 0;
    mbedtls_net_send((mbedtls_net_context *)ctx, buf, len, &bytes_sent);
    printf("Sent %zu bytes\n", bytes_sent);
    return 0;
}

// 主函数
int main()
{
    // 初始化LwIP
    // 这里省略了LwIP的初始化代码
    // 假设LwIP已经正确初始化

    // 定义变量
    DnsQueryState state = { {0}, false };

    // 创建DNS查询请求
    const char *domain = "cloud-dev.openvalley.net";

    // 发送DNS查询并等待回调
    dns_gethostbyname(domain, &state.resolved_ip, dns_query_callback, &state);

    // 循环处理网络事件，直到DNS查询完成
    while (!state.dns_query_done) {
        // 处理网络事件
        // 这里假设有一个事件循环
        // sys_check_timeouts();
    }

    // DNS查询完成后，继续下一步
    char ip_str[MAX_IP_LENGTH];
    ip4_addr_ntoa_r(&state.resolved_ip, ip_str, sizeof(ip_str));
    printf("Resolved IP: %s\n", ip_str);

    // 创建TCP连接
    struct tcp_pcb *tpcb = tcp_new();
    tcp_arg(tpcb, NULL); // 设置回调参数为NULL
    tcp_recv(tpcb, NULL); // 不需要接收回调
    tcp_connect(tpcb, ip4_addr_ntohl(state.resolved_ip.addr), PORT, NULL);

    // 等待TCP连接建立
    while (tpcb->state != ESTABLISHED) {
        // 处理网络事件
        // sys_check_timeouts();
    }

    // 初始化mbedTLS
    mbedtls_net_context net;
    mbedtls_ssl_config *conf;
    mbedtls_ssl_session *sess;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_debug_set_threshold(1);

    mbedtls_net_init(&net);
    mbedtls_ssl_config_init(&conf);
    mbedtls_ssl_session_init(&sess);
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    // 设置随机数生成器
    mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *)"mbedtls example", 20);

    // 配置SSL上下文
    mbedtls_ssl_config_defaults(conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
    mbedtls_ssl_conf_rng(conf, mbedtls_ctr_drbg_random, &ctr_drbg);
    mbedtls_ssl_conf_dbg(conf, my_platform_print, stdout);

    // 初始化网络上下文
    mbedtls_net_set_socket(&net, tpcb, mbedtls_net_poll, mbedtls_net_recv, mbedtls_net_send);

    // 设置TLS回调函数
    mbedtls_ssl_set_bio(sess, &net, tls_send, tls_recv, tls_connected);

    // 进行TLS握手
    int ret;
    do {
        ret = mbedtls_ssl_handshake(sess);
        if (ret != 0) {
            printf("TLS handshake failed: %d\n", ret);
            break;
        }
    } while (ret != 0);

    // 构建HTTP POST请求
    const char *request = "POST /gateway/indoor-location-service/upload_distance HTTP/1.1\r\n"
                          "Host: cloud-dev.openvalley.net\r\n"
                          "Content-Type: application/json\r\n"
                          "Content-Length: 28\r\n"
                          "\r\n"
                          "{\"key\":\"value\"}";

    // 发送HTTP请求
    mbedtls_ssl_write(sess, (const unsigned char *)request, strlen(request));

    // 接收响应
    char buffer[BUFFER_SIZE];
    size_t len = 0;
    while ((len = mbedtls_ssl_read(sess, (unsigned char *)buffer, BUFFER_SIZE)) > 0) {
        buffer[len] = '\0';
        printf("Received response:\n%s\n", buffer);
    }

    // 清理资源
    mbedtls_net_free(&net);
    mbedtls_ssl_free(&sess);
    mbedtls_ssl_config_free(&conf);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

    // 关闭TCP连接
    tcp_close(tpcb);

    return 0;
}
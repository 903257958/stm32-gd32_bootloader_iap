#include "bsp_net.h"
#include "bsp_delay.h"
#include "bsp_console.h"
#include "drv_uart.h"
#include "drv_esp8266.h"
#include <stdarg.h>
#include <stdio.h>

/* --- 驱动设备 --- */
static uart_dev_t uart_net_dev;
static uint8_t uart_net_tx_buf[256];
static uint8_t uart_net_rx_buf[513];
static const uart_cfg_t uart_net_cfg = {
    .uart_periph     = USART2,
    .baudrate        = 115200,
    .tx_port         = GPIOA,
    .tx_pin          = GPIO_Pin_2,
    .rx_port         = GPIOA,
    .rx_pin          = GPIO_Pin_3,
    .tx_buf          = uart_net_tx_buf,
    .rx_buf          = uart_net_rx_buf,
    .tx_buf_size     = sizeof(uart_net_tx_buf),
    .rx_buf_size     = sizeof(uart_net_rx_buf),
    .rx_single_max   = 512,
    .rx_pre_priority = 0,
    .rx_sub_priority = 0
};

static esp8266_dev_t esp8266_dev;
static uint8_t esp8266_tx_buf[256];
static uint8_t esp8266_rx_buf[512];

int esp8266_uart_send_data(uint8_t *data, uint32_t len)
{
    return uart_net_dev.ops->send_data(&uart_net_dev, data, len);
}

int esp8266_uart_recv_data(uint8_t **data, uint32_t *len)
{
    return uart_net_dev.ops->recv_data(&uart_net_dev, data, len);
}

static esp8266_uart_ops_t esp8266_uart_ops = {
    .send_data = esp8266_uart_send_data,
    .recv_data = esp8266_uart_recv_data
};

static esp8266_cfg_t esp8266_cfg = {
    .uart_ops      = NULL,  /* in bsp_net_init_impl */
    .log_ops       = NULL,  /* in bsp_net_init_impl */
    .delay_ms      = bsp_delay_ms,
    .tx_buf        = esp8266_tx_buf,
    .rx_buf        = esp8266_rx_buf,
    .tx_buf_size   = sizeof(esp8266_tx_buf),
    .rx_buf_size   = sizeof(esp8266_rx_buf)
};

/**
 * @brief   BSP 初始化网络模块
 * @param[in] self 指向 BSP 对象的指针
 * @return  0 表示成功，其他值表示失败
 */
static int bsp_net_init_impl(bsp_net_t *self)
{
    esp8266_dev_t *dev = (esp8266_dev_t *)self->drv;

    int ret = drv_uart_init(&uart_net_dev, &uart_net_cfg);
    if (ret)
        return ret;
    
    esp8266_cfg.uart_ops = &esp8266_uart_ops;
    esp8266_cfg.log_ops = (const esp8266_log_ops_t *)bsp_console_get_esp8266_log_ops();

    return drv_esp8266_init(dev, &esp8266_cfg);
}

/**
 * @brief   BSP 网络发送 AT 命令
 * @param[in]  self       指向 BSP 对象的指针
 * @param[in]  cmd        要发送的 AT 命令（不包含 "\r\n"）
 * @param[in]  expect     期望的关键字，例如 "OK"、"ERROR"、">"
 * @param[out] out        若不为 NULL，当检测到 expect 时返回 rx_buf（整段接收内容）
 * @param[out] timeout_ms 超时时间（毫秒）
 * @return  0 表示成功，其他值表示失败
 */
static int bsp_net_send_at_cmd_impl(bsp_net_t *self,
                                    const char *cmd,
                                    const char *expect,
                                    char **out,
                                    uint32_t timeout_ms)
{
    esp8266_dev_t *dev = (esp8266_dev_t *)self->drv;

    return dev->ops->send_cmd(dev, cmd, expect, out, timeout_ms);
}

/**
 * @brief   BSP 网络模块连接 WiFi
 * @param[in] self 指向 BSP 对象的指针
 * @param[in] ssid WiFi 名称
 * @param[in] pwd  WiFi 密码
 * @return  0 表示成功，其他值表示失败
 */
static int bsp_net_wifi_connect_impl(bsp_net_t *self, const char *ssid, const char *pwd)
{
    esp8266_dev_t *dev = (esp8266_dev_t *)self->drv;

    return dev->ops->wifi_connect(dev, ssid, pwd);
}

/**
 * @brief   BSP 网络模块断开 WiFi
 * @param[in] self 指向 BSP 对象的指针
 * @return  0 表示成功，其他值表示失败
 */
static int bsp_net_wifi_disconnect_impl(bsp_net_t *self)
{
    esp8266_dev_t *dev = (esp8266_dev_t *)self->drv;

    return dev->ops->wifi_disconnect(dev);
}

/**
 * @brief   BSP 网络模块建立 TCP 连接（作为客户端）
 * @param[in] self 指向 BSP 对象的指针
 * @param[in] ip   服务器IP地址字符串（如 "192.168.4.1"）
 * @param[in] port 服务器端口（如 8080）
 * @return  0 表示成功，其他值表示失败
 */
static int bsp_net_tcp_connect_impl(bsp_net_t *self, const char *ip, uint16_t port)
{
    esp8266_dev_t *dev = (esp8266_dev_t *)self->drv;

    return dev->ops->tcp_connect(dev, ip, port);
}

/**
 * @brief   BSP 网络模块断开 TCP 连接
 * @param[in] self 指向 BSP 对象的指针
 * @return  0 表示成功，其他值表示失败
 */
static int bsp_net_tcp_disconnect_impl(bsp_net_t *self)
{
    esp8266_dev_t *dev = (esp8266_dev_t *)self->drv;

    return dev->ops->tcp_disconnect(dev);
}

/**
 * @brief   BSP 网络模块进入 TCP 透传模式
 * @param[in] self 指向 BSP 对象的指针
 * @param[in] len  要发送的数据长度，传入 0 表示不定长
 * @return  0 表示成功，其他值表示失败
 */
static int bsp_net_enter_transparent_mode_impl(bsp_net_t *self, uint32_t len)
{
    esp8266_dev_t *dev = (esp8266_dev_t *)self->drv;

    return dev->ops->enter_transparent_mode(dev, len);
}

/**
 * @brief   BSP 网络模块退出 TCP 透传模式
 * @param[in] self 指向 BSP 对象的指针
 * @return  0 表示成功，其他值表示失败
 */
static int bsp_net_exit_transparent_mode_impl(bsp_net_t *self)
{
    esp8266_dev_t *dev = (esp8266_dev_t *)self->drv;

    return dev->ops->exit_transparent_mode(dev);
}

/**
 * @brief   BSP 网络模块发送数据
 * @param[in] self 指向 BSP 对象的指针
 * @param[in] data 发送的数据
 * @param[in] len  数据长度
 * @return  0 表示成功，其他值表示失败
 */
static int bsp_net_send_data_impl(bsp_net_t *self, uint8_t *data, uint32_t len)
{
    esp8266_dev_t *dev = (esp8266_dev_t *)self->drv;

    return dev->ops->send_data(dev, data, len);
}

/**
 * @brief   BSP 网络模块接收数据
 * @param[in]  self       指向 BSP 对象的指针
 * @param[out] data       输出参数，接收数据首地址（无需调用方分配缓冲区，只需传入指针的地址）
 * @param[out] len        输出参数，接收数据长度
 * @param[in]  timeout_ms 超时时间（毫秒）
 * @return  0 表示成功，其他值表示失败
 */
static int bsp_net_recv_data_impl(bsp_net_t *self, uint8_t **data, uint32_t *len, uint32_t timeout_ms)
{
    esp8266_dev_t *dev = (esp8266_dev_t *)self->drv;

    return dev->ops->recv_data(dev, data, len, timeout_ms);
}


/* --- 操作表 --- */
static const bsp_net_ops_t bsp_net_ops = {
    .init                   = bsp_net_init_impl,
    .send_at_cmd            = bsp_net_send_at_cmd_impl,
    .wifi_connect           = bsp_net_wifi_connect_impl,
    .wifi_disconnect        = bsp_net_wifi_disconnect_impl,
    .tcp_connect            = bsp_net_tcp_connect_impl,
    .tcp_disconnect         = bsp_net_tcp_disconnect_impl,
    .enter_transparent_mode = bsp_net_enter_transparent_mode_impl,
    .exit_transparent_mode  = bsp_net_exit_transparent_mode_impl,
    .send_data              = bsp_net_send_data_impl,
    .recv_data              = bsp_net_recv_data_impl
};

/* --- 单例对象 --- */
static bsp_net_t bsp_net = {
    .ops = &bsp_net_ops,
    .drv = &esp8266_dev,
};

/**
 * @brief   获取 BSP 单例对象
 * @return  指向全局 BSP 对象的指针
 */
bsp_net_t *bsp_net_get(void)
{
    return &bsp_net;
}

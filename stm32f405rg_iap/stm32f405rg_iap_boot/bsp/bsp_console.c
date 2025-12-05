#include "bsp_console.h"
#include "drv_uart.h"
#include <stdarg.h>
#include <stdio.h>

/* --- 驱动设备 --- */
static uart_dev_t uart_console_dev;
static uint8_t uart_console_tx_buf[256];
static uint8_t uart_console_rx_buf[1024];
static const uart_cfg_t uart_console_cfg = {
    .uart_periph     = USART1,
    .baudrate        = 921600,
    .tx_port         = GPIOA,
    .tx_pin          = GPIO_Pin_9,
    .rx_port         = GPIOA,
    .rx_pin          = GPIO_Pin_10,
    .tx_buf          = uart_console_tx_buf,
    .rx_buf          = uart_console_rx_buf,
    .tx_buf_size     = sizeof(uart_console_tx_buf),
    .rx_buf_size     = sizeof(uart_console_rx_buf),
    .rx_single_max   = 512,
    .rx_pre_priority = 0,
    .rx_sub_priority = 0
};

/**
 * @brief   BSP 初始化控制台
 * @param[in] self 指向 BSP 对象的指针
 * @return  0 表示成功，其他值表示失败
 */
static int bsp_console_init_impl(bsp_console_t *self)
{
    return drv_uart_init((uart_dev_t *)self->drv, &uart_console_cfg);
}

/**
 * @brief   BSP 控制台 vprintf
 * @param[in] self   指向 BSP 对象的指针
 * @param[in] format 格式化字符串
 * @param[in] args   已经初始化的 va_list
 */
static void bsp_console_vprintf_impl(bsp_console_t *self, const char *format, va_list args)
{
    uart_dev_t *dev = (uart_dev_t *)self->drv;
    dev->ops->vprintf(dev, format, args);
}

/**
 * @brief   BSP 控制台打印
 * @param[in] self   指向 BSP 对象的指针
 * @param[in] format 格式化字符串
 * @param[in] ...    可变参数列表
 */
static void bsp_console_printf_impl(bsp_console_t *self, const char *format, ...)
{
    uart_dev_t *dev = (uart_dev_t *)self->drv;
    va_list args;
    va_start(args, format);

    if (dev && dev->ops && dev->ops->vprintf)
        dev->ops->vprintf(dev, format, args);

    va_end(args);
}

/**
 * @brief   BSP 控制台发送数据
 * @param[in] self 指向 BSP 对象的指针
 * @param[in] data 要发送数据的首地址
 * @param[in] len  要发送数据的长度
 * @return  0 表示成功，其他值表示失败
 */
static int bsp_console_send_data_impl(bsp_console_t *self, uint8_t *data, uint32_t len)
{
    uart_dev_t *dev = (uart_dev_t *)self->drv;

    dev->ops->send_data(dev, data, len);
    return 0;
}

/**
 * @brief   BSP 控制台接收原始二进制数据（不添加 '\0'）
 * @details 该函数不进行 memcpy，也不在数据后追加 '\0'，完全适用于二进制协议（如 Xmodem、IAP、文件传输）
 *          返回的 data 指针指向环形缓冲区的原始内存区域，下一次接收后内容可能会被覆盖。
 * @param[in]  self 指向 BSP 对象的指针
 * @param[out] data 返回指向接收数据首地址的指针（指的是 ring buffer 里的原始区域）
 * @param[out] len  返回本次接收到的数据长度
 * @return 0 表示成功接收到数据，其他表示无数据
 */
static int bsp_console_recv_data_impl(bsp_console_t *self, uint8_t **data, uint32_t *len)
{
    uart_dev_t *dev = (uart_dev_t *)self->drv;

    return dev->ops->recv_data(dev, data, len);
}

/* --- 操作表 --- */
static const bsp_console_ops_t bsp_console_ops = {
    .init      = bsp_console_init_impl,
    .vprintf   = bsp_console_vprintf_impl,
    .printf    = bsp_console_printf_impl,
    .send_data = bsp_console_send_data_impl,
    .recv_data = bsp_console_recv_data_impl
};

/* --- 单例对象 --- */
static bsp_console_t bsp_console = {
    .ops = &bsp_console_ops,
    .drv = &uart_console_dev,
};

/**
 * @brief   获取 BSP 单例对象
 * @return  指向全局 BSP 对象的指针
 */
bsp_console_t *bsp_console_get(void)
{
    return &bsp_console;
}

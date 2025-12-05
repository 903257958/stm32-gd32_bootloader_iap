#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "bsp_console.h"
#include "log.h"

/**
 * @brief   从串口发送数据
 * @param[in] data 要发送数据的首地址
 * @param[in] len  要发送数据的长度
 * @return  0 表示成功，其他值表示失败
 */
int boot_send_data(uint8_t *data, uint32_t len)
{
    bsp_console_t *console = bsp_console_get();

    return console->ops->send_data(console, data, len);
}

/**
 * @brief   从串口接收原始二进制数据
 * @details 不进行复制，不添加 '\0'，完全适用于 BootLoader 命令行、IAP/Xmodem 等协议
 * @param[out] data 指向接收数据起始地址（串口的环形缓冲区内），出参需要修改调用者的指针，用**
 *                  C语言中，实参的值会拷贝一份给形参，函数内部修改「形参」，不会影响「实参」本身；
 *                  只有传递「实参的地址」，并通过 * 解引用，才能修改实参的值。
 * @param[out] len  本次接收到的字节数
 * @return  0 表示成功接收到数据，-EAGAIN 表示无数据
 */
int boot_recv_data(uint8_t **data, uint32_t *len)
{
    bsp_console_t *console = bsp_console_get();
    uint8_t *d = NULL;
    uint32_t l = 0;
    int ret;

    ret = console->ops->recv_data(console, &d, &l);
    if (ret) {
        *data = NULL;
        *len = 0;
        return ret; /* -EAGAIN 表示没有数据 */
    }

    *data = d;
    *len  = l;
    return 0;
}

/**
 * @brief   控制台打印接收到的数据
 * @param[in] data 数据的首地址
 * @param[in] len  数据的长度
 */
void boot_print_recv(uint8_t *data, uint32_t len)
{
    bsp_console_t *console = bsp_console_get();
    
    if (len == 0)
        return;

    log_info("Recv %d bytes:", len);
    boot_send_data(data, len);
    boot_send_data((uint8_t *)"\r\n", 4);

    log_info("Recv %d bytes (hex):", len);
    for (uint16_t i = 0; i < len; i++) {
        console->ops->printf(console, "%x ", data[i]);
    }
    console->ops->printf(console, "\r\n\r\n");
}

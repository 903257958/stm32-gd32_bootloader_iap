#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "bsp_log.h"
#include "log.h"

/**
 * @brief   从串口发送数据
 * @param[in] data 要发送数据的首地址
 * @param[in] len  要发送数据的长度
 * @return  0 表示成功，其他值表示失败
 */
int boot_send_data(uint8_t *data, uint32_t len)
{
    bsp_log_t *log = bsp_log_get();

    return log->ops->send_data(log, data, len);
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
    bsp_log_t *log = bsp_log_get();
    uint8_t *d = NULL;
    uint32_t l = 0;
    int ret;

    ret = log->ops->recv_data(log, &d, &l);
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
 * @brief   调试用：从日志串口接收原始数据并打印
 * @details 用于验证 log->ops->recv_data() 是否正确处理指针与长度。
 *          如果收到二进制数据，本函数会将其拷贝到一个本地缓冲区，
 *          在末尾追加 '\0'，便于调试打印。
 */
void boot_recv_data_test(void)
{
    bsp_log_t *log = bsp_log_get();
    uint8_t *d = NULL;
    uint32_t l = 0;
    int ret;

    ret = log->ops->recv_data(log, &d, &l);
    if (ret < 0) {
        // log_debug("recv_data: no data, ret=%d", ret);
        return;
    }

    /* 防止长度超出测试缓冲区 */
    uint32_t copy_len = l;
    if (copy_len > 255)
        copy_len = 255;

    /* 本地缓冲区，用来安全打印 */
    static uint8_t dump_buf[256];

    memcpy(dump_buf, d, copy_len);
    dump_buf[copy_len] = '\0';   /* 便于打印 */

    /* 打印模式：
     * 1) 作为字符串打印（适合 ASCII）
     * 2) 逐字节十六进制打印（适合二进制协议）
     */
    log_debug("recv_data: len=%u, str=\"%s\"", l, dump_buf);

    /* 打印二进制十六进制格式 */
    log_debug("recv_data hex:");
    for (uint32_t i = 0; i < l; i++)
        log_debug("  [%02u] = 0x%02X", i, d[i]);
}

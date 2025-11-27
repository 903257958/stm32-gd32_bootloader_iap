#ifndef BOOT_COMM_H
#define BOOT_COMM_H

#include <stdint.h>

/**
 * @brief   从串口发送数据
 * @param[in] data 要发送数据的首地址
 * @param[in] len  要发送数据的长度
 * @return  0 表示成功，其他值表示失败
 */
int boot_send_data(uint8_t *data, uint32_t len);

/**
 * @brief   从串口接收原始二进制数据
 * @details 不进行复制，不添加 '\0'，完全适用于 BootLoader 命令行、IAP/Xmodem 等协议
 * @param[out] data 指向接收数据起始地址（串口的环形缓冲区内）
 * @param[out] len  本次接收到的字节数
 * @return  0 表示成功接收到数据，-EAGAIN 表示无数据
 */
int boot_recv_data(uint8_t **data, uint32_t *len);

/**
 * @brief   调试用：从日志串口接收原始数据并打印
 * @details 用于验证 log->ops->recv_data() 是否正确处理指针与长度。
 *          如果收到二进制数据，本函数会将其拷贝到一个本地缓冲区，
 *          在末尾追加 '\0'，便于调试打印。
 */
void boot_recv_data_test(void);

#endif

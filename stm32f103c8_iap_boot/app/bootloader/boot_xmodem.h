#ifndef BOOT_XMODEM_H
#define BOOT_XMODEM_H

#include <stdint.h>

/**
 * @brief   Xmodem 协议初始化
 */
void boot_xmodem_init(void);

/**
 * @brief   Xmodem 协议发 C
 */
void boot_xmodem_send_c(void);

/**
 * @brief   Xmodem 协议接收数据
 * @param[in] data 接收数据的首地址
 * @param[in] len  接收数据的长度
 */
void boot_xmodem_recv_data(uint8_t *data, uint16_t len);

#endif

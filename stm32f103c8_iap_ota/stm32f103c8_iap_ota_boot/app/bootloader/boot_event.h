#ifndef BOOT_EVENT_H
#define BOOT_EVENT_H

#include <stdint.h>

/**
 * @brief   Bootloader 事件处理
 * @param[in] data 接收数据的首地址
 * @param[in] len  接收数据的长度
 */
void boot_process_event(uint8_t *data, uint32_t len);

#endif

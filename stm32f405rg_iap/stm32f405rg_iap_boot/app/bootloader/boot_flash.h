#ifndef BOOT_FLASH_H
#define BOOT_FLASH_H

#include <stdint.h>
#include "bsp_flash.h"

/**
 * @brief   擦除 Flash APP 程序
 * @return	0 表示成功，其他值表示失败
 */
int boot_flash_erase_app(void);

/**
 * @brief   将完整 update_chunk 数据块写入内部 Flash
 * @param[in] flash     指向内部 Flash BSP 对象的指针
 * @param[in] chunk_idx update_chunk 的块索引（从 0 开始），表示从应用起始地址起要写入的第几个块
 * @return	0 表示成功，其他值表示失败
 */
int boot_flash_write_chunk(bsp_flash_t *flash, uint32_t chunk_idx);

#endif

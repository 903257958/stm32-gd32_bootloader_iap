#ifndef BOOT_EXT_FLASH_H
#define BOOT_EXT_FLASH_H

#include <stdint.h>
#include "bsp_ext_flash.h"

/**
 * @brief   将完整 update_chunk 数据块写入外部 Flash
 * @param[in] ext_flash 指向外部 Flash BSP 对象的指针
 * @param[in] chunk_idx update_chunk 的块索引（从 0 开始），表示从应用起始地址起要写入的第几个块
 */
void boot_ext_flash_write_chunk(bsp_ext_flash_t *ext_flash, uint32_t chunk_idx);

/**
 * @brief   请求下载程序到外部 Flash
 * @param[in] data 接收数据的首地址
 * @param[in] len  接收数据的长度
 */
void boot_ext_flash_download_request(uint8_t *data, uint32_t len);

/**
 * @brief   请求加载外部 Flash 程序到内部 Flash
 * @param[in] data 接收数据的首地址
 * @param[in] len  接收数据的长度
 */
void boot_ext_flash_load_request(uint8_t *data, uint32_t len);

/**
 * @brief   加载外部 Flash 程序到内部 Flash
 */
void boot_ext_flash_load(void);

/**
 * @brief   返回当前外部 Flash 程序索引
 * @return  当前外部 Flash 程序索引
 */
uint8_t boot_ext_flash_get_cur_slot_idx(void);

/**
 * @brief   外部 Flash OTA 初始化
 */
void boot_ext_flash_ota_init(void);

#endif

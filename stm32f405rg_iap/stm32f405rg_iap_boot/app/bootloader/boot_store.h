#ifndef BOOT_STORE_H
#define BOOT_STORE_H

#include <stdint.h>
#include "boot_config.h"

/* 此结构体大小要为 EEPROM 页大小的整数倍 */
typedef struct {
    uint32_t app_size[BOOT_EXT_FLASH_APP_SLOT_COUNT];   // 外部 Flash 中存储的应用程序字节数，0 号位预留给 OTA
    uint8_t  version[BOOT_OTA_VERSION_LEN_MAX];
    uint32_t ota_flag;
} boot_app_info_t;

/**
 * @brief   读取 IAP 信息
 * @param[out] boot_app_info boot_app_info_t 结构体指针
 * @return	0 表示成功，其他值表示失败
 */
int boot_app_info_load(boot_app_info_t *boot_app_info);

/**
 * @brief   保存 IAP 信息
 * @param[in] boot_app_info boot_app_info_t 结构体指针
 * @return	0 表示成功，其他值表示失败
 */
int boot_app_info_save(boot_app_info_t *boot_app_info);

#endif

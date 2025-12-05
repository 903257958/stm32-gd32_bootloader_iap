#ifndef BOOT_OTA_H
#define BOOT_OTA_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief   判断是否需要进行 OTA 升级
 * @return  true 表示需要进行 OTA 升级，false 表示不需要
 */
bool boot_ota_should_upgrade(void);

/**
 * @brief   OTA 初始化版本号（调试/首次运行用）
 * @details 此接口仅用于调试或系统首次运行时初始化 EEPROM 中的版本号。
 *          在规范的 OTA 流程中，版本号应由云服务器在生成升级包时写入；
 *          APP 在完成 OTA 升级后读取云端版本号并保存到 EEPROM；
 *          APP 每次启动会上报当前版本号供云端判断是否需要更新。
 *          Bootloader 在正式流程中只读取版本号，不负责写入。
 *
 * @param[in] data   版本号数据的首地址
 * @param[in] len    数据长度
 */
void boot_ota_version_init(uint8_t *data, uint32_t len);

#endif

#include <string.h>
#include "boot_ota.h"
#include "boot_config.h"
#include "boot_store.h"
#include "boot_core.h"
#include "log.h"

/**
 * @brief   判断是否需要进行 OTA 升级
 * @return  true 表示需要进行 OTA 升级，false 表示不需要
 */
bool boot_ota_should_upgrade(void)
{
    boot_app_info_t boot_app_info;

    boot_app_info_load(&boot_app_info);
    if (boot_app_info.ota_flag == BOOT_OTA_FLAG) {
        log_info("OTA upgrade!");
        return true;
    }

    log_info("No OTA event, OTA flag: 0x%x", boot_app_info.ota_flag);
    return false;
}

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
void boot_ota_version_init(uint8_t *data, uint32_t len)
{
    boot_app_info_t boot_app_info;

    if (len >= sizeof(boot_app_info.version)) { // 需要留空间给 \0
        log_error("");
        log_error("Invalid version len (must < %d bytes): %d", 
                  BOOT_OTA_VERSION_LEN_MAX, len);
        return;
    }

    boot_app_info_load(&boot_app_info);
    memset(boot_app_info.version, 0, sizeof(boot_app_info.version));
    memcpy(boot_app_info.version, data, len);
    boot_app_info.version[len] = '\0';

    if (boot_app_info_save(&boot_app_info) != 0)
        return;

    log_info("Version init successfully: %s", boot_app_info.version);
    boot_clear_flag(BOOT_FLAG_OTA_VERSION_INIT);
}

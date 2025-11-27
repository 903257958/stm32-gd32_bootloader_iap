#include <string.h>
#include "bsp_eeprom.h"
#include "bsp_delay.h"
#include "boot_store.h"
#include "log.h"

#define BOOT_APP_INFO_ADDR     0x0000

/**
 * @brief   读取 IAP 信息
 * @param[out] boot_app_info boot_app_info_t 结构体指针
 * @return	0 表示成功，其他值表示失败
 */
int boot_app_info_load(boot_app_info_t *boot_app_info)
{
    int ret;
    bsp_eeprom_t *eeprom = bsp_eeprom_get();

    memset(boot_app_info, 0, sizeof(boot_app_info_t));

    ret = eeprom->ops->read_data(eeprom, 
                                 BOOT_APP_INFO_ADDR, 
                                 sizeof(boot_app_info_t), 
                                 (uint8_t *)boot_app_info);
    if (ret) {
        log_error("Failed to read eeprom data: %d", ret);
        return ret;
    }
    return 0;
}

/**
 * @brief   保存 IAP 信息
 * @param[in] boot_app_info boot_app_info_t 结构体指针
 * @return	0 表示成功，其他值表示失败
 */
int boot_app_info_save(boot_app_info_t *boot_app_info)
{
    int ret;
    uint8_t i;
	uint8_t *write_ptr = (uint8_t *)boot_app_info;
    bsp_eeprom_t *eeprom = bsp_eeprom_get();

	for (i = 0; i < sizeof(boot_app_info_t) / EEPROM_PAGE_SIZE; i++) {
        ret = eeprom->ops->write_page(eeprom, 
                                      i * EEPROM_PAGE_SIZE, 
                                      write_ptr + i * EEPROM_PAGE_SIZE);
        if (ret) {
            log_error("Failed to write eeprom page: %d", ret);
            return ret;
        }

		bsp_delay_ms(5);
	}
    return 0;
}

/**
 * @brief   调试用：IAP 信息读取与保存测试
 */
void boot_app_info_test(void)
{
    boot_app_info_t write_info;
    boot_app_info_t read_info;
    int ret;
    int i;

    /* 构造测试数据 */
    for (i = 0; i < BOOT_EXT_FLASH_APP_SLOT_COUNT; i++)
        write_info.app_size[i] = (i + 1) * 12345;

    log_debug("boot_store test: writing test data...");

    ret = boot_app_info_save(&write_info);
    if (ret) {
        log_debug("boot_store test: save failed, ret=%d", ret);
        return;
    }

    log_debug("boot_store test: write success, now reading...");

    ret = boot_app_info_load(&read_info);
    if (ret) {
        log_debug("boot_store test: load failed, ret=%d", ret);
        return;
    }

    /* 打印读出的数据 */
    log_debug("boot_store test: read_info.app_size:");
    for (i = 0; i < BOOT_EXT_FLASH_APP_SLOT_COUNT; i++) {
        log_debug("  index %d: %lu", i, (unsigned long)read_info.app_size[i]);
    }

    /* 数据校验 */
    for (i = 0; i < BOOT_EXT_FLASH_APP_SLOT_COUNT; i++) {
        if (read_info.app_size[i] != write_info.app_size[i]) {
            log_debug("boot_store test: mismatch at index %d!", i);
            log_debug("  write=%lu read=%lu",
                      (unsigned long)write_info.app_size[i],
                      (unsigned long)read_info.app_size[i]);
            log_debug("boot_store test: FAILED");
            return;
        }
    }

    log_debug("boot_store test: PASSED");
}

#include <string.h>
#include "bsp_delay.h"
#include "bsp_eeprom.h"
#include "boot_store.h"
#include "log.h"

#define BOOT_APP_INFO_ADDR     0x0000

/**
 * @brief   读取 APP 信息
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
 * @brief   保存 APP 信息
 * @param[in] boot_app_info boot_app_info_t 结构体指针
 * @return	0 表示成功，其他值表示失败
 */
int boot_app_info_save(boot_app_info_t *boot_app_info)
{
    int ret;
    uint8_t i;
	uint8_t *write_ptr = (uint8_t *)boot_app_info;
    bsp_eeprom_t *eeprom = bsp_eeprom_get();

    if (sizeof(boot_app_info_t) % EEPROM_PAGE_SIZE != 0) {
        log_error("Invalid app info size (must be %d-byte aligned): %d", 
                  EEPROM_PAGE_SIZE, sizeof(boot_app_info_t));
        return -1;
    }

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

    log_info("App info saved to EEPROM successfully");
    log_info("  Size      : %d bytes", sizeof(boot_app_info_t));
    log_info("  Page size : %d bytes", EEPROM_PAGE_SIZE);
    log_info("  Pages     : %d", sizeof(boot_app_info_t) / EEPROM_PAGE_SIZE);
    return 0;
}

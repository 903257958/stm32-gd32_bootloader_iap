#include "bsp_flash.h"
#include "boot_config.h"
#include "boot_core.h"
#include "boot_flash.h"
#include "log.h"

/**
 * @brief   擦除 Flash APP 程序
 * @return	0 表示成功，其他值表示失败
 */
int boot_flash_erase_app(void)
{
    bsp_flash_t *flash = bsp_flash_get();

    log_info("Erase APP.");

    /* 擦除 APP 区 */
#if BOOT_PLATFORM_STM32F1 || BOOT_PLATFORM_GD32F1
    if (flash->ops->erase(flash, BOOT_FLASH_APP_PAGE_COUNT, BOOT_FLASH_APP_START_PAGE) != 0) {
#elif BOOT_PLATFORM_STM32F4
    if (flash->ops->erase(flash, BOOT_FLASH_APP_SECOTR_COUNT, BOOT_FLASH_APP_START_SECOTR) != 0) {
#endif
        log_error("Flash erase operation failed!");
        return -1;
    }

#if BOOT_PLATFORM_STM32F1 || BOOT_PLATFORM_GD32F1
    /* 验证擦除是否成功 */
    int32_t i;
    uint32_t error_cnt = 0;
    volatile uint32_t *p;
    p = (volatile uint32_t *)BOOT_FLASH_APP_START_ADDR;
    for (i = 0; i < BOOT_FLASH_APP_PAGE_COUNT * BOOT_FLASH_PAGE_SIZE / 4; i++) {
        if (p[i] != 0xFFFFFFFF)
            error_cnt++;
    }
    if (error_cnt != 0) {
        log_error("Failed to erase APP (err=%d)!", error_cnt);
        return -2;
    }
#endif
    
    log_info("APP erased successfully!\r\n");
    return 0;
}

/**
 * @brief   将完整 update_chunk 数据块写入内部 Flash
 * @param[in] flash     指向内部 Flash BSP 对象的指针
 * @param[in] chunk_idx update_chunk 的块索引（从 0 开始），表示从应用起始地址起要写入的第几个块
 * @return	0 表示成功，其他值表示失败
 */
int boot_flash_write_chunk(bsp_flash_t *flash, uint32_t chunk_idx)
{
    uint32_t addr = BOOT_FLASH_APP_START_ADDR + chunk_idx * BOOT_APP_UPDATE_CHUNK_SIZE;
    uint8_t *update_chunk = boot_get_update_chunk();

    return flash->ops->write(flash, addr,
                             BOOT_APP_UPDATE_CHUNK_SIZE,
                             (uint32_t *)update_chunk);
}

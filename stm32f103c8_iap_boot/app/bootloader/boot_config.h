#ifndef BOOT_CONFIG_H
#define BOOT_CONFIG_H

/* 内部 Flash (B 区: BootLoader, A 区: APP) */
#define BOOT_FLASH_BASE_ADDR        (0x08000000UL)  // Flash 起始地址
#define BOOT_FLASH_PAGE_SIZE        (1024UL)        // Flash 页大小
#define BOOT_FLASH_PAGE_COUNT       (64UL)          // Flash 总页数
#define BOOT_FLASH_BOOT_PAGE_COUNT  (24UL)          // B 区 Flash 页数
#define BOOT_FLASH_APP_PAGE_COUNT   (BOOT_FLASH_PAGE_COUNT - BOOT_FLASH_BOOT_PAGE_COUNT)    // A 区 Flash 页数
#define BOOT_FLASH_APP_START_PAGE   (BOOT_FLASH_BOOT_PAGE_COUNT)                            // A 区 Flash 起始页编号
#define BOOT_FLASH_APP_START_ADDR   (BOOT_FLASH_BASE_ADDR + BOOT_FLASH_APP_START_PAGE * BOOT_FLASH_PAGE_SIZE)  // A 区 Flash 起始地址

/* RAM 地址范围 */
#define BOOT_RAM_SIZE       (20UL * 1024UL) // STM32F103C8T6 RAM: 20KB
#define BOOT_RAM_BASE_ADDR  (0x20000000UL)
#define BOOT_RAM_END_ADDR   (BOOT_RAM_BASE_ADDR + BOOT_RAM_SIZE - 1UL)

/* APP 更新块大小 */
#define BOOT_APP_UPDATE_CHUNK_SIZE  (1024)

/* 外部Flash */
#define BOOT_EXT_FLASH_BLOCK_SIZE       (64UL * 1024UL) // 外部 Flash 块大小
#define BOOT_EXT_FLASH_PAGE_SIZE        (256UL)         // 外部 Flash 页大小
#define BOOT_EXT_FLASH_APP_BLOCK_COUNT  (16UL)          // 外部 Flash 存储的每个 APP 的块数
#define BOOT_EXT_FLASH_APP_MAX_SIZE     (BOOT_EXT_FLASH_BLOCK_SIZE * \
                                         BOOT_EXT_FLASH_APP_BLOCK_COUNT)    // 外部 Flash 存储的每个 APP 的最大字节数（1MB）
#define BOOT_EXT_FLASH_APP_SLOT_COUNT   (10UL)          // 外部 Flash 存储的 APP 槽位数量

#endif

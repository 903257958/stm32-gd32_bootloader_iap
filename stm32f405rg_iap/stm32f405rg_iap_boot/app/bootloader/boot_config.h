#ifndef BOOT_CONFIG_H
#define BOOT_CONFIG_H

#if defined(STM32F10X_HD) || defined(STM32F10X_MD)
#define BOOT_PLATFORM_STM32F1 1

#elif defined(STM32F40_41xxx) || defined(STM32F429_439xx) || defined(STM32F411xE)
#define BOOT_PLATFORM_STM32F4 1

#elif defined (GD32F10X_MD) || defined (GD32F10X_HD)
#define BOOT_PLATFORM_GD32F1 1

#endif

/* 内部 Flash (B 区: BootLoader, A 区: APP) */
#if BOOT_PLATFORM_STM32F1 || BOOT_PLATFORM_GD32F1
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

#elif BOOT_PLATFORM_STM32F4

#define BOOT_FLASH_BASE_ADDR            (0x08000000UL)  // Flash 起始地址
#define BOOT_FLASH_SECOTR_COUNT         (12UL)          // Flash 总扇区数
#define BOOT_FLASH_BOOT_SECOTR_COUNT    (2UL)           // B 区 Flash 扇区数
#define BOOT_FLASH_APP_SECOTR_COUNT     (BOOT_FLASH_SECOTR_COUNT - BOOT_FLASH_BOOT_SECOTR_COUNT)    // A 区 Flash 扇区数
#define BOOT_FLASH_APP_START_SECOTR     (BOOT_FLASH_BOOT_SECOTR_COUNT)                              // A 区 Flash 起始扇区编号
#define BOOT_FLASH_APP_START_ADDR       (BOOT_FLASH_BASE_ADDR + 0x8000UL)   // A 区 Flash 起始地址

/* RAM 地址范围 */
#define BOOT_RAM_SIZE       (128UL * 1024UL)    // STM32F405RGT6 RAM: 128KB+64KB
#define BOOT_RAM_BASE_ADDR  (0x20000000UL)
#define BOOT_RAM_END_ADDR   (BOOT_RAM_BASE_ADDR + BOOT_RAM_SIZE - 1UL)
#endif

/* APP 更新块大小 */
#define BOOT_APP_UPDATE_CHUNK_SIZE  (1024)

/* 外部Flash */
#define BOOT_EXT_FLASH_BLOCK_SIZE       (64UL * 1024UL) // 外部 Flash 块大小
#define BOOT_EXT_FLASH_PAGE_SIZE        (256UL)         // 外部 Flash 页大小
#define BOOT_EXT_FLASH_APP_BLOCK_COUNT  (16UL)          // 外部 Flash 存储的每个 APP 的块数
#define BOOT_EXT_FLASH_APP_MAX_SIZE     (BOOT_EXT_FLASH_BLOCK_SIZE * \
                                         BOOT_EXT_FLASH_APP_BLOCK_COUNT)    // 外部 Flash 存储的每个 APP 的最大字节数（1MB）
#define BOOT_EXT_FLASH_APP_SLOT_COUNT   (10UL)          // 外部 Flash 存储的 APP 槽位数量，0 号位预留给 OTA

/* OTA */
#define BOOT_OTA_VERSION_LEN_MAX    (20)            // 版本号最大长度
#define BOOT_OTA_FLAG               (0xAABB1122)    // OTA 标志位，用于判断是否进行 OTA

#endif


#include "bsp_flash.h"
#include "bsp_ext_flash.h"
#include "boot_config.h"
#include "boot_core.h"
#include "boot_flash.h"
#include "boot_ext_flash.h"
#include "boot_cmd.h"
#include "boot_store.h"
#include "boot_xmodem.h"
#include "log.h"

typedef struct {
    uint8_t slot_idx;  // 外部 Flash 程序索引，判断更新第几个程序到 A 区（第 0 个保留）
} boot_ext_flash_ctx_t;

static boot_ext_flash_ctx_t boot_ext_flash_ctx;

/**
 * @brief   将完整 update_chunk 数据块写入外部 Flash
 * @param[in] ext_flash 指向外部 Flash BSP 对象的指针
 * @param[in] chunk_idx update_chunk 的块索引（从 0 开始），表示从应用起始地址起要写入的第几个块
 */
void boot_ext_flash_write_chunk(bsp_ext_flash_t *ext_flash, uint32_t chunk_idx)
{
	uint8_t i;
	uint32_t base_addr = boot_ext_flash_ctx.slot_idx * BOOT_EXT_FLASH_APP_MAX_SIZE +
                    	 chunk_idx * BOOT_APP_UPDATE_CHUNK_SIZE;
    uint8_t *update_chunk = boot_get_update_chunk();
	
	/* 外部 Flash 必须按页写，按页循环写入 update_chunk 中的数据 */
	for (i = 0; i < BOOT_APP_UPDATE_CHUNK_SIZE / BOOT_EXT_FLASH_PAGE_SIZE; i++) {
		ext_flash->ops->write_page(ext_flash, 
								   base_addr + i * BOOT_EXT_FLASH_PAGE_SIZE, 
								   BOOT_EXT_FLASH_PAGE_SIZE, 
								   &update_chunk[i * BOOT_EXT_FLASH_PAGE_SIZE]);
	}
}

/**
 * @brief   请求下载程序到外部 Flash
 * @param[in] data 接收数据的首地址
 * @param[in] len  接收数据的长度
 */
void boot_ext_flash_download_request(uint8_t *data, uint16_t len)
{
    uint8_t i;
    int ret;
    boot_app_info_t boot_app_info;
    bsp_ext_flash_t *ext_flash = bsp_ext_flash_get();

    if (len != 1) {
        log_warn("Invalid input length: %d", len);
        return;
    }

    /* 0 号位保留 */
    if (data[0] < '1' || data[0] > ('1' + BOOT_EXT_FLASH_APP_SLOT_COUNT - 2)) {
        log_warn("Invalid input num: %c", (char *)data[0]);
        return;
    }

    boot_ext_flash_ctx.slot_idx = data[0] - '0';
    boot_clear_flag(BOOT_FLAG_EXT_DOWNLOAD_REQUEST);
    boot_set_flag(BOOT_FLAG_IAP_XMODEM_SEND_C);
    boot_set_flag(BOOT_FLAG_IAP_XMODEM_RECV_DATA);
    boot_set_flag(BOOT_FLAG_EXT_DOWNLOAD_XMODEM);
    boot_xmodem_init();

    boot_app_info_load(&boot_app_info);
    boot_app_info.app_size[boot_ext_flash_ctx.slot_idx] = 0;
    boot_app_info_save(&boot_app_info);

    /* 擦除要写入程序的块 */
    log_info("Erasing the %dth firmware of external Flash...", boot_ext_flash_ctx.slot_idx);
    for (i = 0; i < BOOT_EXT_FLASH_APP_BLOCK_COUNT; i++) {
        ret = ext_flash->ops->erase_block(ext_flash, 
                                          boot_ext_flash_ctx.slot_idx * BOOT_EXT_FLASH_APP_BLOCK_COUNT + i);
        if (ret) {
            log_error("Failed to erase block (err=%d)", ret);
        }
        log_info("Erased block %d/%d", i + 1, BOOT_EXT_FLASH_APP_BLOCK_COUNT);
    }

    log_info("Use Xmodem to download a BIN file to external Flash slot %d.",
             boot_ext_flash_ctx.slot_idx);
}

/**
 * @brief   请求加载外部 Flash 程序到内部 Flash
 * @param[in] data 接收数据的首地址
 * @param[in] len  接收数据的长度
 */
void boot_ext_flash_load_request(uint8_t *data, uint16_t len)
{
    if (len != 1) {
        log_warn("Invalid input length: %d", len);
        return;
    }

    /* 0 号位保留 */
    if (data[0] < '1' || data[0] > ('1' + BOOT_EXT_FLASH_APP_SLOT_COUNT - 2)) {
        log_warn("Invalid input num: %c", (char *)data[0]);
        return;
    }

    boot_clear_flag(BOOT_FLAG_EXT_LOAD_REQUEST);
    boot_ext_flash_ctx.slot_idx = data[0] - '0';
    boot_set_flag(BOOT_FLAG_EXT_LOAD);
}

/**
 * @brief   加载外部 Flash 程序到内部 Flash
 */
void boot_ext_flash_load(void)
{
    bsp_flash_t *flash = bsp_flash_get();
    bsp_ext_flash_t *ext_flash = bsp_ext_flash_get();
    boot_app_info_t boot_app_info;
    uint8_t *update_chunk = boot_get_update_chunk();
    uint8_t ext_flash_slot_idx = boot_ext_flash_ctx.slot_idx;
    uint32_t app_size;
    uint32_t chunk_idx;
    uint8_t i;
    
    boot_app_info_load(&boot_app_info);
    app_size = boot_app_info.app_size[ext_flash_slot_idx];

    log_info("Loading firmware from slot %d (size=%d bytes)", ext_flash_slot_idx, app_size);
    
    /* 判断下载长度是否为4字节对齐 */
    if (app_size % 4 != 0) {
		log_error("Invalid firmware size (must be 4-byte aligned): %d", app_size);
        boot_clear_flag(BOOT_FLAG_EXT_LOAD);
        return;
    }

    /* 擦除内部Flash A区 */
    boot_flash_erase_app();

    /* 下载长度为4字节对齐，先写完整的页 */
    for (i = 0; i < app_size / BOOT_APP_UPDATE_CHUNK_SIZE; i++) {
        /* 从W25QX中搬运出一次数据到update_buf */
        ext_flash->ops->read_data(ext_flash, 
                                  ext_flash_slot_idx * BOOT_EXT_FLASH_APP_MAX_SIZE + i * BOOT_APP_UPDATE_CHUNK_SIZE, 
                                  BOOT_APP_UPDATE_CHUNK_SIZE, 
                                  update_chunk);

        /* 将本次数据写入内部 Flash */
        chunk_idx = i;
        boot_flash_write_chunk(flash, chunk_idx);
    }

    /* 处理剩余不足一页的字节 */
    if (app_size % BOOT_APP_UPDATE_CHUNK_SIZE != 0) {
        /* 从W25QX中搬运出剩余数据 */
        ext_flash->ops->read_data(ext_flash, 
                                  ext_flash_slot_idx * BOOT_EXT_FLASH_APP_MAX_SIZE + i * BOOT_APP_UPDATE_CHUNK_SIZE, 
                                  app_size % BOOT_APP_UPDATE_CHUNK_SIZE, 
                                  update_chunk);

        /* 将剩余数据写入内部 Flash */
        flash->ops->write(flash, 
                          BOOT_FLASH_APP_START_ADDR + i * BOOT_APP_UPDATE_CHUNK_SIZE, 
                          app_size % BOOT_APP_UPDATE_CHUNK_SIZE, 
                          (uint32_t *)update_chunk);
    }
    
    /* 系统复位 */
    log_info("Program loaded successfully, restarting system...");
    boot_system_reset();
}

/**
 * @brief   返回当前外部 Flash 程序索引
 * @return  当前外部 Flash 程序索引
 */
uint8_t boot_ext_flash_get_cur_slot_idx(void)
{
    return boot_ext_flash_ctx.slot_idx;
}

#include <string.h>
#include "bsp_delay.h"
#include "bsp_eeprom.h"
#include "bsp_flash.h"
#include "bsp_ext_flash.h"
#include "log.h"

/**
 * @brief   进行 EEPROM 测试
 */
void eeprom_test(void)
{
    bsp_eeprom_t *eeprom = bsp_eeprom_get();
    uint8_t rbuf[EEPROM_PAGE_SIZE + 2];
    uint8_t wbuf[EEPROM_PAGE_SIZE] = { 0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88 };
    int ret;
    
    log_info("=========== EEPROM Test Start ============");

    /* 1. 单字节写读测试 */
    ret = eeprom->ops->write_byte(eeprom, 0x00, 0xAB);
    bsp_delay_ms(5);
    ret |= eeprom->ops->read_data(eeprom, 0x00, 1, rbuf);
    log_info("1. Byte %s (W=AB R=%02X)\r\n",
             (!ret && rbuf[0] == 0xAB) ? "PASS" : "FAIL", rbuf[0]);

    /* 2. 整页写读测试 */
    ret = eeprom->ops->write_page(eeprom, 0x08, wbuf);
    bsp_delay_ms(5);
    ret |= eeprom->ops->read_data(eeprom, 0x08, sizeof(wbuf), rbuf);
    log_info("2. Page %s\r\n", (!ret && !memcmp(wbuf,rbuf,sizeof(wbuf)))?"PASS":"FAIL");

    /* 3. 连续读取测试 */
    ret = eeprom->ops->read_data(eeprom, 0x00, 10, rbuf);
    log_info("3. Continuous Read %s: ", ret?"FAIL":"PASS");
    for (uint8_t i = 0; i < 10; i++) 
        log_info("%02X ", rbuf[i]);

    log_info("========= EEPROM Test Complete ===========");
}

/**
 * @brief   进行内部 Flash 测试
 */
void flash_test(void)
{
    bsp_flash_t *flash = bsp_flash_get();
    uint32_t i, error_cnt = 0;
    uint32_t test_addr = 0x08000000 + 60 * 1024;
    uint32_t write_buf[16], read_buf[16];
    int ret;

    log_info("============ Flash Test Start ============");

    /* 初始化测试数据 */
    for (i = 0; i < 16; i++)
        write_buf[i] = 0xAA55AA00 + i;

    /* 1. 擦除测试 */
    log_info("1. Erase test: ");
    ret = flash->ops->erase(flash, 1, 60);
    if (ret != 0) {
        log_error("FAILED (erase ret=%d)\r\n", ret);
        goto flash_test_end;
    }
    /* 验证擦除（应为0xFFFFFFFF） */
    for (i = 0; i < 16; i++) {
        if (*(uint32_t*)(test_addr + i*4) != 0xFFFFFFFF)
            error_cnt++;
    }
    if (error_cnt != 0) {
        log_error("FAILED (erase verify err=%d)\r\n", error_cnt);
        goto flash_test_end;
    }
    log_info("OK\r\n");

    /* 2. 写入测试 */
    log_info("2. Write test: ");
    ret = flash->ops->write(flash, test_addr, 64, write_buf);
    if (ret != 0) {
        log_error("FAILED (write ret=%d)\r\n", ret);
        goto flash_test_end;
    }
    log_info("OK\r\n");

    /* 3. 读取验证 */
    log_info("3. Read verify: ");
    error_cnt = 0;
    for (i = 0; i < 16; i++) {
        read_buf[i] = *(uint32_t*)(test_addr + i*4);
        if (read_buf[i] != write_buf[i])
            error_cnt++;
    }
    if (error_cnt != 0) {
        log_error("FAILED (mismatch=%d)\r\n", error_cnt);
        goto flash_test_end;
    }
    log_info("OK\r\n");

flash_test_end:
    log_info("========== Flash Test Complete ===========");
}

/**
 * @brief   进行外部 Flash 测试
 */
void ext_flash_test(void)
{
    uint8_t mid;
    uint16_t did;
    bsp_ext_flash_t *ext_flash = bsp_ext_flash_get();
    int ret;
    uint32_t i;
    uint32_t error_cnt;
    uint8_t write_buf[EXT_FLASH_PAGE_SIZE];
    uint8_t read_buf[EXT_FLASH_PAGE_SIZE];
    
    log_info("======= External Flash Test Start ========");

    /* 1. 读 ID 测试 */
    log_info("1. Read ID test");
    ret = ext_flash->ops->read_id(ext_flash, &mid, &did);
    if (ret != 0) {
        log_error("FAILED (ret=%d)", ret);
        goto ext_flash_test_end;
    }
    log_info("OK (mid:0x%02X, did:0x%04X)\r\n", mid, did);

    /* 2. 块擦除测试 */
    log_info("2. Block(64KB) Erase test: ");
    ret = ext_flash->ops->erase_block(ext_flash, 0);
    if (ret != 0) {
        log_error("FAILED (ret=%d)", ret);
        goto ext_flash_test_end;
    }
   
    ext_flash->ops->read_data(ext_flash, 0, EXT_FLASH_PAGE_SIZE, read_buf);
    for (i = 0; i < EXT_FLASH_PAGE_SIZE; i++) {
        if (read_buf[i] != 0xFF) {
            log_error("FAILED (erase incomplete)");
            goto ext_flash_test_end;
        }
    }
    log_info("OK\r\n");

    /* 3. 页写入测试 */
    log_info("3. Page Write test: ");
    for (i = 0; i < EXT_FLASH_PAGE_SIZE; i++) {
        write_buf[i] = i;  // 填充测试数据
    }
    ret = ext_flash->ops->write_page(ext_flash, 0, EXT_FLASH_PAGE_SIZE, write_buf);
    if (ret != 0) {
        log_error("FAILED (ret=%d)", ret);
        goto ext_flash_test_end;
    }
    /* 验证页写入 */
    ext_flash->ops->read_data(ext_flash, 0, EXT_FLASH_PAGE_SIZE, read_buf);
    error_cnt = 0;
    for (i = 0; i < EXT_FLASH_PAGE_SIZE; i++) {
        if (read_buf[i] != write_buf[i])
			error_cnt++;
    }
    if (error_cnt != 0) {
        log_error("FAILED (errors=%d)", error_cnt);
        goto ext_flash_test_end;
    }
    log_info("OK\r\n");

    /* 4. 扇区擦除测试 */
    log_info("4. Sector(4KB) Erase test: ");
    ret = ext_flash->ops->erase_sector(ext_flash, 0);
    if (ret != 0) {
        log_error("FAILED (ret=%d)", ret);
        goto ext_flash_test_end;
    }
    /* 验证扇区擦除 */
    ext_flash->ops->read_data(ext_flash, 0, EXT_FLASH_PAGE_SIZE, read_buf);
    for (i = 0; i < EXT_FLASH_PAGE_SIZE; i++) {
        if (read_buf[i] != 0xFF) {
            log_error("FAILED (erase incomplete)");
            goto ext_flash_test_end;
        }
    }
    log_info("OK\r\n");

    /* 5. 跨页写入测试 */
    log_info("5. Cross-page Write test: ");
    for (i = 0; i < EXT_FLASH_PAGE_SIZE * 2; i++) {
        write_buf[i % EXT_FLASH_PAGE_SIZE] = i;  // 生成跨页数据
    }
    /* 从页末尾开始写入，触发跨页操作 */
    ret = ext_flash->ops->write_data(ext_flash, EXT_FLASH_PAGE_SIZE - 10, 20, write_buf);
    if (ret != 0) {
        log_error("FAILED (ret=%d)", ret);
        goto ext_flash_test_end;
    }
    /* 验证跨页写入 */
    ext_flash->ops->read_data(ext_flash, EXT_FLASH_PAGE_SIZE - 10, 20, read_buf);
    error_cnt = 0;
    for (i = 0; i < 20; i++) {
        if (read_buf[i] != i)
			error_cnt++;
    }
    if (error_cnt != 0) {
        log_error("FAILED (errors=%d)", error_cnt);
        goto ext_flash_test_end;
    }
    log_info("OK\r\n");

ext_flash_test_end:
    log_info("====== External Flash Test Complete ======\r\n");
}

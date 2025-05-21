#ifndef __BOOTLOADER_H
#define __BOOTLOADER_H

/* 标准库头文件 */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

/* 驱动头文件 */
#include "delay.h"
#include "uart.h"
#include "eeprom.h"
#include "w25qx.h"
#include "fmc.h"

/* 硬件设备 */
extern EEPROMDev_t at24c02;
extern W25QXDev_t w25q64;
extern FMCDev_t fmc;

/* 打印调试信息 */
#define BOOT_DEBUG(fmt, ...) uart0_printf(fmt, ##__VA_ARGS__)

/* 单片机内部Flash */
#define FLASH_START_ADDR   0x08000000                                               // Flash起始地址
#define FLASH_PAGE_SIZE    1024                                                     // Flash页大小
#define FLASH_PAGE_NUM     64                                                       // Flash总页数
#define FLASH_B_PAGE_NUM   20                                                       // B区Flash页数
#define FLASH_A_PAGE_NUM   FLASH_PAGE_NUM - FLASH_B_PAGE_NUM                        // A区Flash页数
#define FLASH_A_START_PAGE FLASH_B_PAGE_NUM                                         // A区Flash起始页编号
#define FLASH_A_START_ADDR FLASH_START_ADDR + FLASH_A_START_PAGE * FLASH_PAGE_SIZE  // A区Flash起始地址

/* 外部Flash */
#define E_FLASH_BLOCK_SIZE  64 * 1024   // 外部Flash块大小
#define E_FLASH_PAGE_SIZE   256         // 外部Flash页大小

/* BootLoader标志位 */
#define FLASH_A_UPDATE_FLAG                     0x00000001  // Flash A区更新标志位
#define IAP_XMODEM_SEND_C_FLAG                  0x00000002  // 串口IAP Xmodem协议发C标志位
#define IAP_XMODEM_SEND_DATA_FLAG               0x00000004  // 串口IAP Xmodem协议发数据标志位
#define E_FLASH_PROGRAM_DOWNLOAD_SELECT_FLAG    0x00000008  // 下载程序到外部Flash选择标志位
#define E_FLASH_PROGRAM_DOWNLOAD_XMODEM_FLAG    0x00000010  // 下载程序到外部Flash Xmodem协议传输标志位
#define E_FLASH_PROGRAM_LOAD_SELECT_FLAG        0x00000020  // 加载外部Flash内程序选择标志位

/* Xmodem协议 */
#define XMODEM_PACKET_LEN       133     // 数据包总长度
#define XMODEM_PACKET_DATA_LEN  128     // 数据包有效数据长度
#define XMODEM_SOH              0x01    // SOH
#define XMODEM_EOT              0x04    // EOT

/* IAP信息结构体 */
typedef struct {
    uint32_t program_byte_num[12];  // 更新的程序字节数
}IAPInfo_t;
extern IAPInfo_t iap_info;

#define IAP_INFO_SIZE   sizeof(IAPInfo_t)   // IAP信息结构体大小

/* Flash A区更新控制块结构体 */
typedef struct {
    uint8_t update_buf[FLASH_PAGE_SIZE];    // 更新内部Flash A区程序时，每次从外部Flash搬运的数据量，大小为内部Flash的页大小
    uint32_t eflash_block_index;            // 外部Flash块索引，判断更新哪个块的程序到A区（每块为64KB，每块存储一个应用程序，第0块留给OTA）
    uint32_t xmodem_timeout;                // Xmodem协议延时
    uint32_t xmodem_packet_cnt;             // Xmodem协议数据包接收计数，1个包128字节，8个包为1024字节，写满一页内部Flash
    uint32_t xmodem_crc_val;                // Xmodem协议CRC校验值
}FlashAUpdateControlBlock_t;
extern FlashAUpdateControlBlock_t flash_a_update_cb;

/* 函数指针，用于跳转到A区应用程序入口 */
typedef void (*p_load_flash_a)(void);

/* 状态变量 */
extern uint32_t g_bootloader_status;

/* 函数声明 */
void eeprom_read_iap_info(void);
void eeprom_write_iap_info(void);
void bootloader_branch(void);
void bootloader_event(uint8_t *data, uint16_t len);
void handle_xmodem_send_c(void);
void handle_flash_a_update(void);

#endif

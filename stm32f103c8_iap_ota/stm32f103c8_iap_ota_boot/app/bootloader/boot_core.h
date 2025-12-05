#ifndef BOOT_CORE_H
#define BOOT_CORE_H

#include <stdint.h>
#include <stdbool.h>

/* BootLoader 标志位 */
typedef enum {
    BOOT_FLAG_IAP_XMODEM_SEND_C    = 0x00000001,    // 串口 IAP Xmodem 协议发 C
    BOOT_FLAG_IAP_XMODEM_RECV_DATA = 0x00000002,    // 串口 IAP Xmodem 协议接收数据
    BOOT_FLAG_EXT_DOWNLOAD_REQUEST = 0x00000004,    // 请求下载程序到外部 Flash（选择）
    BOOT_FLAG_EXT_DOWNLOAD_XMODEM  = 0x00000008,    // 外部 Flash 下载 Xmodem 协议传输
    BOOT_FLAG_EXT_LOAD_REQUEST     = 0x00000010,    // 请求加载外部 Flash 程序到内部 Flash（选择）
    BOOT_FLAG_EXT_LOAD             = 0x00000020,    // 执行加载外部 Flash 程序到内部 Flash
    BOOT_FLAG_OTA_VERSION_INIT     = 0x00000040,    // 初始化 OTA 版本号
} boot_flag_t;

/**
 * @brief   BootLoader 入口处理
 */
void boot_process_entry(void);

/**
 * @brief   BootLoader 系统复位
 */
void boot_system_reset(void);

/**
 * @brief   BootLoader 设置标志位
 * @param[in] flag 标志位
 */
void boot_set_flag(boot_flag_t flag);

/**
 * @brief   BootLoader 清除标志位
 * @param[in] flag 标志位
 */
void boot_clear_flag(boot_flag_t flag);

/**
 * @brief   BootLoader 是否存在某标志位
 * @param[in] flag 标志位
 * @return  true 表示标志位存在，false 表示标志位不存在
 */
bool boot_has_flag(boot_flag_t flag);

/**
 * @brief   BootLoader 获取当前标志位
 * @return  true 表示标志位存在，false 表示标志位不存在
 */
uint32_t boot_get_flag(void);

/**
 * @brief   BootLoader 获取 APP 更新块
 * @return  APP 更新块首地址
 */
uint8_t* boot_get_update_chunk(void);

#endif

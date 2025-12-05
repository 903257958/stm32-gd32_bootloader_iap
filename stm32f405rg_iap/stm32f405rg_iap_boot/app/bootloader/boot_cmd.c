#include <string.h>
#include "boot_config.h"
#include "boot_cmd.h"
#include "boot_core.h"
#include "boot_flash.h"
#include "boot_xmodem.h"
#include "boot_store.h"
#include "log.h"

/**
 * @brief   擦除 Flash APP 程序
 * @return	0 表示成功，其他值表示失败
 */
static int boot_cmd_erase_app(void)
{
    return boot_flash_erase_app();
}

/**
 * @brief   IAP 开始下载程序到内部 Flash
 * @return	0 表示成功，其他值表示失败
 */
static int boot_cmd_start_iap_download(void)
{
    log_info("IAP download firmware to Flash.");
    boot_flash_erase_app();
    log_info("Use Xmodem to download a BIN file to Flash.");

    boot_set_flag(BOOT_FLAG_IAP_XMODEM_SEND_C);
    boot_set_flag(BOOT_FLAG_IAP_XMODEM_RECV_DATA);

    boot_xmodem_init();
    return 0;
}

/**
 * @brief   IAP 开始下载程序到外部 Flash
 * @return	0 表示成功，其他值表示失败
 */
static int boot_cmd_start_iap_download_ext(void)
{
    log_info("IAP download firmware to External Flash, please enter the firmware location (1-%d).", 
             BOOT_EXT_FLASH_APP_SLOT_COUNT - 1);

    boot_set_flag(BOOT_FLAG_EXT_DOWNLOAD_REQUEST);
    return 0;
}

/**
 * @brief   从外部 Flash 加载固件
 * @return	0 表示成功，其他值表示失败
 */
static int boot_cmd_load_from_ext(void)
{
    log_info("Load firmware from External Flash, please enter the firmware location (1-%d).", 
             BOOT_EXT_FLASH_APP_SLOT_COUNT - 1);

    boot_set_flag(BOOT_FLAG_EXT_LOAD_REQUEST);
    return 0;
}

/**
 * @brief   初始化 OTA 版本号
 * @details 此命令仅用于调试或系统首次运行时初始化 EEPROM 中的版本号。
 *          在规范的 OTA 流程中，版本号应由云服务器在生成升级包时写入；
 *          APP 在完成 OTA 升级后读取云端版本号并保存到 EEPROM；
 *          APP 每次启动会上报当前版本号供云端判断是否需要更新。
 *          Bootloader 在正式流程中只读取版本号，不负责写入。
 * @return	0 表示成功
 */
static int boot_cmd_ota_version_init(void)
{
    log_info("Init OTA version...");
    log_warn("************************************************************");
    log_warn("*  WARNING: OTA version initializer invoked                *");
    log_warn("*  This operation is only for debugging or factory setup   *");
    log_warn("*  Not part of normal OTA workflow                         *");
    log_warn("*  Incorrect use may reset version state unexpectedly      *");
    log_warn("************************************************************");
    boot_set_flag(BOOT_FLAG_OTA_VERSION_INIT);
    return 0;
}

/**
 * @brief   查看 OTA 版本号
 * @return	0 表示成功
 */
static int boot_cmd_check_ota_version(void)
{
    log_info("Check OTA version...");
    boot_app_info_t boot_app_info;
    boot_app_info_load(&boot_app_info);
    log_info("Version: %s\r\n", boot_app_info.version);
    return 0;
}

/**
 * @brief   系统重启
 * @return	0 表示成功
 */
static int boot_cmd_system_reset(void)
{
    log_info("System restarting...");
    boot_system_reset();
    return 0;
}

static const boot_menu_item_t menu_items[] = {
    { "Erase APP partition"                     , boot_cmd_erase_app              },
    { "IAP: Download firmware to Internal Flash", boot_cmd_start_iap_download     },
    { "IAP: Download firmware to External Flash", boot_cmd_start_iap_download_ext },
    { "Load firmware from External Flash"       , boot_cmd_load_from_ext          },
    { "Init OTA version"                        , boot_cmd_ota_version_init       },
    { "Check OTA version"                       , boot_cmd_check_ota_version      },
    { "System restart"                          , boot_cmd_system_reset           }
};

/**
 * @brief   BootLoader 打印菜单信息
 */
void boot_cmd_print_menu(void)
{
    log_info("================================================");
    log_info("|           BootLoader Operation Menu           |");
    log_info("================================================");

    for (uint8_t i = 0; i < sizeof(menu_items) / sizeof(menu_items[0]); i++)
        log_info("[%d] %-30s", i + 1, menu_items[i].desc);

    log_info("================================================");
}

/**
 * @brief   Bootloader 处理命令
 * @param[in] data 接收数据的首地址
 * @param[in] len  接收数据的长度
 */
void boot_handle_cmd(uint8_t *data, uint32_t len)
{
    int ret;

    if (len != 1) {
        log_warn("Invalid input length: %d", len);
        return;
    }

    uint8_t cmd = data[0] - '1'; // 输入 '1' -> 索引 0
    if (cmd >= sizeof(menu_items)/sizeof(menu_items[0])) {
        log_warn("Invalid command: %c", data[0]);
        return;
    }

    if (menu_items[cmd].handler) {
        ret = menu_items[cmd].handler();
        if (ret != 0)
            log_error("Command [%c] execute failed (err=%d)", data[0], ret);
    } else {
        log_error("Command [%c] has no handler", data[0]);
    }
}

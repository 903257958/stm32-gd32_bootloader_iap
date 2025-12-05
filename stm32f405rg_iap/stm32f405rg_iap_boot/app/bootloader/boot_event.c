#include <stdint.h>
#include "boot_core.h"
#include "boot_comm.h"
#include "boot_cmd.h"
#include "boot_xmodem.h"
#include "boot_ext_flash.h"
#include "boot_ota.h"
#include "log.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

typedef struct boot_event_handler {
    boot_flag_t flag;
    void (*handler_no_data)(void);
    void (*handler_with_data)(uint8_t *data, uint32_t len);
} boot_event_handler_t;

/* 事件处理表 */
static const boot_event_handler_t boot_handlers[] = {
    { BOOT_FLAG_IAP_XMODEM_SEND_C,    boot_xmodem_send_c,  NULL                            },
    { BOOT_FLAG_EXT_LOAD,             boot_ext_flash_load, NULL                            },
    { BOOT_FLAG_IAP_XMODEM_RECV_DATA, NULL,                boot_xmodem_recv_data           },
    { BOOT_FLAG_EXT_DOWNLOAD_REQUEST, NULL,                boot_ext_flash_download_request },
    { BOOT_FLAG_EXT_LOAD_REQUEST,     NULL,                boot_ext_flash_load_request     },
    { BOOT_FLAG_OTA_VERSION_INIT,     NULL,                boot_ota_version_init           }
};

/**
 * @brief   Bootloader 事件处理
 * @param[in] data 接收数据的首地址
 * @param[in] len  接收数据的长度
 */
void boot_process_event(uint8_t *data, uint32_t len)
{
    uint8_t i;

    /* 处理无输入事件 */
    if (len == 0) {
        for (i = 0; i < ARRAY_SIZE(boot_handlers); i++) {
            if (boot_has_flag(boot_handlers[i].flag) &&
                boot_handlers[i].handler_no_data) {
                boot_handlers[i].handler_no_data();
                return;
            }
        }
        return;
    }

    /* 处理有输入事件 */
    if (boot_get_flag() == 0) {
        boot_handle_cmd(data, len); // 只有所有标志位都被清除后，才会处理输入命令
        return;
    }

    for (i = 0; i < ARRAY_SIZE(boot_handlers); i++) {
        if (boot_has_flag(boot_handlers[i].flag) &&
            boot_handlers[i].handler_with_data) {
            boot_handlers[i].handler_with_data(data, len);
            return;
        }
    }

    log_warn("Invalid flag value: 0x%08X", boot_get_flag());
}

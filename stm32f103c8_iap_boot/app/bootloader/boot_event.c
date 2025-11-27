#include <stdint.h>
#include "boot_core.h"
#include "boot_comm.h"
#include "boot_cmd.h"
#include "boot_xmodem.h"
#include "boot_ext_flash.h"
#include "log.h"

/**
 * @brief   Bootloader 事件处理
 * @param[in] data 接收数据的首地址
 * @param[in] len  接收数据的长度
 */
void boot_process_event(uint8_t *data, uint32_t len)
{
    if (len == 0) {
        if (boot_has_flag(BOOT_FLAG_IAP_XMODEM_SEND_C)) {
            /* 串口IAP，发C事件 */
            boot_xmodem_send_c();
            
        } else if (boot_has_flag(BOOT_FLAG_EXT_LOAD)) {
            /* 加载外部 Flash 程序到内部 Flash */
            boot_ext_flash_load();
        }
    } else {
        if (boot_get_flag() == 0) {
            /* 处理命令行输入 */
            boot_handle_cmd(data, len);

        } else if (boot_has_flag(BOOT_FLAG_IAP_XMODEM_RECV_DATA)) {
            /* Xmodem 协议接收数据 */
            boot_xmodem_recv_data(data, len);

        } else if (boot_has_flag(BOOT_FLAG_EXT_DOWNLOAD_REQUEST)) {
            /* 请求下载程序到外部 Flash */
            boot_ext_flash_download_request(data, len);

        } else if (boot_has_flag(BOOT_FLAG_EXT_LOAD_REQUEST)) {
            /* 请求加载外部 Flash 程序到内部 Flash */
            boot_ext_flash_load_request(data, len);

        } else {
            log_warn("Invalid flag value: 0x%08X", boot_get_flag());
        }
    }
}

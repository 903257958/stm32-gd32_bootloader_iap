#include <string.h>
#include "bsp_common.h"
#include "bsp_delay.h"
#include "bsp_log.h"
#include "log.h"
#include "test_core.h"
#include "test_module.h"

/**
 * @brief   打印帮助信息
 */
void log_cmd_show_help(void)
{
    log_info("================== Help ==================");
    log_info("Available commands:");
    log_info(" help     - Show command list");
    log_info(" eeprom   - EEPROM test");
    log_info(" flash    - Flash test");
    log_info(" extflash - External flash test");
    log_info(" rst      - Restart system");
    log_info("==========================================\r\n");
}

/**
 * @brief   处理收到的命令
 */
void log_handle_cmd(void)
{
    char *rx_data;
    bsp_log_t *log = bsp_log_get();

    rx_data = log->ops->recv_str(log);
    if (!rx_data)
        return;

    if (strcmp(rx_data, "rst") == 0) {
        system_reset();
    } else if (strncmp(rx_data, "eeprom", 6) == 0) {
        eeprom_test();
    } else if (strncmp(rx_data, "flash", 5) == 0) {
        flash_test();
    } else if (strncmp(rx_data, "extflash", 8) == 0) {
        ext_flash_test();
    } else if (strncmp(rx_data, "help", 4) == 0) {
        log_cmd_show_help();
    } else {
        log_info("Unknown command: %s", rx_data);
        log_info("Type 'help' for available commands.");
    }
}

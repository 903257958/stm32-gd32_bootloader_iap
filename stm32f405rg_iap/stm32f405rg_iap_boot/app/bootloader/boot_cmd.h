#ifndef BOOT_CMD_H
#define BOOT_CMD_H

#include <stdint.h>

typedef int (*boot_cmd_handler_t)(void);

typedef struct {
    const char *desc;
    boot_cmd_handler_t handler;
} boot_menu_item_t;

/**
 * @brief   BootLoader 打印菜单信息
 */
void boot_cmd_print_menu(void);

/**
 * @brief   Bootloader 处理命令
 * @param[in] data 接收数据的首地址
 * @param[in] len  接收数据的长度
 */
void boot_handle_cmd(uint8_t *data, uint32_t len);

#endif

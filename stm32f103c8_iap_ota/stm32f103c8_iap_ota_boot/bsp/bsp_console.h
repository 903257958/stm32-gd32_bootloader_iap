#ifndef BSP_CONSOLE_H
#define BSP_CONSOLE_H

#include <stdint.h>
#include <stdarg.h>

typedef struct bsp_console bsp_console_t;

/* 操作接口 */
typedef struct {
    int (*init)(bsp_console_t *self);
    void (*vprintf)(bsp_console_t *self, const char *format, va_list args);
    void (*printf)(bsp_console_t *self, const char *format, ...);
    int (*send_data)(bsp_console_t *self, uint8_t *data, uint32_t len);
    int (*recv_data)(bsp_console_t *self, uint8_t **data, uint32_t *len);
} bsp_console_ops_t;

/* 设备实例结构体 */
struct bsp_console {
    const bsp_console_ops_t *ops;
    void *drv;  /* 指向底层驱动对象 */
};

/* 获取 BSP 单例对象 */
bsp_console_t *bsp_console_get(void);

#endif  /* BSP_CONSOLE_H */

#ifndef BSP_LOG_H
#define BSP_LOG_H

#include <stdint.h>

typedef struct bsp_log bsp_log_t;

/* 操作接口 */
typedef struct {
    int (*init)(bsp_log_t *self);
    int (*send_data)(bsp_log_t *self, uint8_t *data, uint32_t len);
    void (*printf)(bsp_log_t *self, const char *format, ...);
    char *(*recv_str)(bsp_log_t *self);
    int (*recv_data)(bsp_log_t *self, uint8_t **data, uint32_t *len);
} bsp_log_ops_t;

/* 设备实例结构体 */
struct bsp_log {
    const bsp_log_ops_t *ops;
    void *drv;  /* 指向底层驱动对象 */
};

/* 获取 BSP 单例对象 */
bsp_log_t *bsp_log_get(void);

#endif  /* BSP_LOG_H */

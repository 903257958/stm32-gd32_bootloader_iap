#ifndef BSP_DELAY_H
#define BSP_DELAY_H

#include <stdint.h>

typedef struct bsp_delay bsp_delay_t;

/* 操作接口 */
typedef struct {
    int (*init)(bsp_delay_t *self);
} bsp_delay_ops_t;

/* 设备实例结构体 */
struct bsp_delay {
    const bsp_delay_ops_t *ops;
    void *drv;  /* 指向底层驱动对象 */
};

/* 获取 BSP 单例对象 */
bsp_delay_t *bsp_delay_get(void);
void bsp_delay_us(uint32_t us);
void bsp_delay_ms(uint32_t ms);

#endif  /* BSP_DELAY_H */

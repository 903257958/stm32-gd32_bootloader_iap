#ifndef BSP_FLASH_H
#define BSP_FLASH_H

#include <stdint.h>

typedef struct bsp_flash bsp_flash_t;

/* 操作接口 */
typedef struct {
    int (*init)(bsp_flash_t *self);
    int (*erase)(bsp_flash_t *self, uint16_t cnt, uint16_t idx);
	int (*write)(bsp_flash_t *self, uint32_t addr, uint32_t cnt, uint32_t *data);
} bsp_flash_ops_t;

/* 设备实例结构体 */
struct bsp_flash {
    const bsp_flash_ops_t *ops;
    void *drv;  /* 指向底层驱动对象 */
};

/* 获取 BSP 单例对象 */
bsp_flash_t *bsp_flash_get(void);

#endif  /* BSP_FLASH_H */

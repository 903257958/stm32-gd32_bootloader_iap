#ifndef BSP_I2C_BUS_H
#define BSP_I2C_BUS_H

#include <stdint.h>
#include <stdbool.h>

typedef struct bsp_i2c_bus bsp_i2c_bus_t;

/* 操作接口 */
typedef struct {
    int (*init)(bsp_i2c_bus_t *self);
} bsp_i2c_bus_ops_t;

/* 设备实例结构体 */
struct bsp_i2c_bus {
    const bsp_i2c_bus_ops_t *ops;
    void *drv;  /* 指向底层驱动对象 */
};

/* 获取 BSP 单例对象 */
bsp_i2c_bus_t *bsp_i2c_bus_get(void);

void *bsp_i2c_bus_get_eeprom_ops(void);

#endif  /* BSP_I2C_BUS_H */

#ifndef BSP_EEPROM_H
#define BSP_EEPROM_H

#include <stdint.h>

#define EEPROM_PAGE_SIZE    8

typedef struct bsp_eeprom bsp_eeprom_t;

/* 操作接口 */
typedef struct {
    int (*init)(bsp_eeprom_t *self);
    int (*write_byte)(bsp_eeprom_t *self, uint8_t addr, uint8_t data);
	int (*write_page)(bsp_eeprom_t *self, uint8_t addr, uint8_t *data);
	int (*read_data)(bsp_eeprom_t *self, uint8_t addr, uint16_t cnt, uint8_t *data);
} bsp_eeprom_ops_t;

/* 设备实例结构体 */
struct bsp_eeprom {
    const bsp_eeprom_ops_t *ops;
    void *drv;  /* 指向底层驱动对象 */
};

/* 获取 BSP 单例对象 */
bsp_eeprom_t *bsp_eeprom_get(void);

#endif  /* BSP_EEPROM_H */

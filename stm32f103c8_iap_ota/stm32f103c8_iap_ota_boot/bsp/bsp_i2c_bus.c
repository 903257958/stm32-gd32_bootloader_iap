#include "bsp_i2c_bus.h"
#include "bsp_delay.h"
#include "drv_i2c_soft.h"
#include "drv_eeprom.h"

/* --- 驱动设备 --- */

/* 共享 I2C 总线 */
static i2c_soft_dev_t i2c_soft_bus;
static const i2c_soft_cfg_t i2c_soft_cfg = {
    .scl_port     = GPIOB,
    .scl_pin      = GPIO_Pin_6,
    .sda_port     = GPIOB,
    .sda_pin      = GPIO_Pin_7,
    .delay_us     = bsp_delay_us,
    .bit_delay_us = 1
};

/**
 * @brief   BSP 初始化共享 I2C 总线
 * @param[in] self 指向 BSP 对象的指针
 * @return  0 表示成功，其他值表示失败
 */
static int bsp_i2c_bus_init_impl(bsp_i2c_bus_t *self)
{
    return drv_i2c_soft_init((i2c_soft_dev_t *)self->drv, &i2c_soft_cfg);
}

/* --- 操作表 --- */
static const bsp_i2c_bus_ops_t bsp_i2c_bus_ops = {
    .init = bsp_i2c_bus_init_impl,
};

/* --- 单例对象 --- */
static bsp_i2c_bus_t bsp_i2c_bus = {
    .ops = &bsp_i2c_bus_ops,
    .drv = &i2c_soft_bus,
};

/**
 * @brief   获取 BSP 单例对象
 * @return  指向全局 BSP 对象的指针
 */
bsp_i2c_bus_t *bsp_i2c_bus_get(void)
{
    return &bsp_i2c_bus;
}

/* --- 提供给外部的接口函数 --- */
static int bsp_i2c_write_reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t data)
{
    return i2c_soft_bus.ops->write_reg(&i2c_soft_bus, dev_addr, reg_addr, data);
}

static int bsp_i2c_write_regs(uint8_t dev_addr, uint8_t reg_addr, uint16_t num, uint8_t *data)
{
    return i2c_soft_bus.ops->write_regs(&i2c_soft_bus, dev_addr, reg_addr, num, data);
}

static int bsp_i2c_read_regs(uint8_t dev_addr, uint8_t reg_addr, uint16_t num, uint8_t *data)
{
    return i2c_soft_bus.ops->read_regs(&i2c_soft_bus, dev_addr, reg_addr, num, data);
}

/* --- 接口函数表 --- */
static const eeprom_i2c_ops_t bsp_i2c_eeprom_ops = {
    .write_reg  = bsp_i2c_write_reg,
    .write_regs = bsp_i2c_write_regs,
    .read_regs  = bsp_i2c_read_regs,
};

/**
 * @brief   获取给外部的操作函数接口表
 * @return  指向操作函数接口表的指针
 */
void *bsp_i2c_bus_get_eeprom_ops(void)
{
    return (void *)&bsp_i2c_eeprom_ops;
}

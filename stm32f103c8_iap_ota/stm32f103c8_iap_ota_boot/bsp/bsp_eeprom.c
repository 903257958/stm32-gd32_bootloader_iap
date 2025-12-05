#include <stddef.h>
#include <errno.h>
#include "bsp_eeprom.h"
#include "bsp_i2c_bus.h"
#include "drv_eeprom.h"

/* --- 驱动设备 --- */
static eeprom_dev_t eeprom_dev;

/**
 * @brief   BSP 初始化 EEPROM
 * @param[in] self 指向 BSP 对象的指针
 * @return  0 表示成功，其他值表示失败
 */
static int bsp_eeprom_init_impl(bsp_eeprom_t *self)
{
    static eeprom_cfg_t eeprom_cfg;
    eeprom_cfg.i2c_ops = (const eeprom_i2c_ops_t *)bsp_i2c_bus_get_eeprom_ops();
    if (!eeprom_cfg.i2c_ops)
        return -EINVAL;
    
    eeprom_cfg.page_size = EEPROM_AT24C02_PAGE_SIZE;

    return drv_eeprom_init((eeprom_dev_t *)self->drv, &eeprom_cfg);
}

/**
 * @brief   BSP EEPROM 写入单字节数据
 * @param[in] self 指向 BSP 对象的指针
 * @param[in] addr 写入的地址
 * @param[in] data 写入的数据
 * @return	0 表示成功，其他值表示失败
 */
static int bsp_eeprom_write_byte_impl(bsp_eeprom_t *self, uint8_t addr, uint8_t data)
{
    eeprom_dev_t *dev = (eeprom_dev_t *)self->drv;
    if (!dev)
        return -EINVAL;

    return dev->ops->write_byte(dev, addr, data);
}

/**
 * @brief   BSP EEPROM 按页写入数据
 * @param[in] self 指向 BSP 对象的指针
 * @param[in] addr 写入的起始地址
 * @param[in] data 写入的数据
 * @return	0 表示成功，其他值表示失败
 */
static int bsp_eeprom_write_page_impl(bsp_eeprom_t *self, uint8_t addr, uint8_t *data)
{
    eeprom_dev_t *dev = (eeprom_dev_t *)self->drv;
    if (!dev)
        return -EINVAL;

    return dev->ops->write_page(dev, addr, data);
}

/**
 * @brief   BSP EEPROM 读取数据
 * @param[in]  self 指向 BSP 对象的指针
 * @param[in]  addr 读取的起始地址
 * @param[in]  cnt  读取数据的个数
 * @param[out] data 读取的数据
 * @return	0 表示成功，其他值表示失败
 */
static int bsp_eeprom_read_data_impl(bsp_eeprom_t *self, uint8_t addr, uint16_t cnt, uint8_t *data)
{
    eeprom_dev_t *dev = (eeprom_dev_t *)self->drv;
    if (!dev)
        return -EINVAL;

    return dev->ops->read_data(dev, addr, cnt, data);
}

/* --- 操作表 --- */
static const bsp_eeprom_ops_t bsp_eeprom_ops = {
    .init       = bsp_eeprom_init_impl,
    .write_byte = bsp_eeprom_write_byte_impl,
    .write_page = bsp_eeprom_write_page_impl,
    .read_data  = bsp_eeprom_read_data_impl
};

/* --- 单例对象 --- */
static bsp_eeprom_t bsp_eeprom = {
    .ops = &bsp_eeprom_ops,
    .drv = &eeprom_dev,
};

/**
 * @brief   获取 BSP 单例对象
 * @return  指向全局 BSP 对象的指针
 */
bsp_eeprom_t *bsp_eeprom_get(void)
{
    return &bsp_eeprom;
}

#include <stddef.h>
#include <errno.h>
#include "bsp_flash.h"
#include "drv_flash.h"

/* --- 驱动设备 --- */
static flash_dev_t flash_dev;

/**
 * @brief   BSP 初始化内部 Flash
 * @param[in] self 指向 BSP 对象的指针
 * @return  0 表示成功，其他值表示失败
 */
static int bsp_flash_init_impl(bsp_flash_t *self)
{
    return drv_flash_init((flash_dev_t *)self->drv);
}

/**
 * @brief   BSP 内部 Flash 擦除
 * @param[in] self 指向 BSP 对象的指针
 * @param[in] cnt  擦除页/扇区的数量
 * @param[in] idx  擦除页/扇区的起始索引
 * @return	0 表示成功，其他值表示失败
 */
static int bsp_flash_erase_impl(bsp_flash_t *self, uint16_t cnt, uint16_t idx)
{
    flash_dev_t *dev = (flash_dev_t *)self->drv;
    if (!dev)
        return -EINVAL;

    return dev->ops->sector_erase(dev, cnt, idx);
}

/**
 * @brief   BSP 内部 Flash 写入
 * @param[in] self 指向 BSP 对象的指针
 * @param[in] addr 起始地址
 * @param[in] cnt  写入数据的数量（单位：字节）
 * @param[in] data 要写入的数据（数组元素为字，4字节）
 * @return	0 表示成功，其他值表示失败
 */
static int bsp_flash_write_impl(bsp_flash_t *self, uint32_t addr, uint32_t cnt, uint32_t *data)
{
    flash_dev_t *dev = (flash_dev_t *)self->drv;
    if (!dev)
        return -EINVAL;

    return dev->ops->write(dev, addr, cnt, data);
}

/* --- 操作表 --- */
static const bsp_flash_ops_t bsp_flash_ops = {
    .init  = bsp_flash_init_impl,
    .erase = bsp_flash_erase_impl,
    .write = bsp_flash_write_impl
};

/* --- 单例对象 --- */
static bsp_flash_t bsp_flash = {
    .ops = &bsp_flash_ops,
    .drv = &flash_dev,
};

/**
 * @brief   获取 BSP 单例对象
 * @return  指向全局 BSP 对象的指针
 */
bsp_flash_t *bsp_flash_get(void)
{
    return &bsp_flash;
}

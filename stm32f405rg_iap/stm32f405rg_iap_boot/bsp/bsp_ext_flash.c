#include "bsp_ext_flash.h"
#include "drv_spi.h"
#include "drv_w25qx.h"

/* --- 驱动设备 --- */

static spi_dev_t spi2;
static const spi_cfg_t spi2_cfg = {
	.spi_periph = SPI2,
	.sck_port   = GPIOB,
	.sck_pin    = GPIO_Pin_10,
	.miso_port  = GPIOB,
	.miso_pin   = GPIO_Pin_14,
	.mosi_port  = GPIOB,
	.mosi_pin   = GPIO_Pin_15,
	.prescaler  = SPI_BaudRatePrescaler_2,
	.mode       = SPI_MODE_0,
};

static int spi2_start(gpio_port_t cs_port, gpio_pin_t cs_pin)
{
	return spi2.ops->start(&spi2, cs_port, cs_pin);
}

static int spi2_swap_byte(uint8_t send, uint8_t *recv)
{
	return spi2.ops->swap_byte(&spi2, send, recv);
}
static int spi2_stop(gpio_port_t cs_port, gpio_pin_t cs_pin)
{
	return spi2.ops->stop(&spi2, cs_port, cs_pin);
}

static w25qx_spi_ops_t w25qx_spi_ops = {
	.start     = spi2_start,
	.swap_byte = spi2_swap_byte,
	.stop      = spi2_stop	
};

static w25qx_dev_t w25qx_dev;
static const w25qx_cfg_t w25qx_cfg = {
	.spi_ops = &w25qx_spi_ops,
	.cs_port = GPIOC,
	.cs_pin  = GPIO_Pin_12,
};

/**
 * @brief   BSP 初始化外部 Flash
 * @param[in] self 指向 BSP 对象的指针
 * @return  0 表示成功，其他值表示失败
 */
static int bsp_ext_flash_init_impl(bsp_ext_flash_t *self)
{
    int8_t ret;
    ret = drv_spi_init(&spi2, &spi2_cfg);
    if (ret)
        return ret;
    
    return drv_w25qx_init((w25qx_dev_t *)self->drv, &w25qx_cfg);
}

/**
 * @brief   BSP 外部 Flash 读取 ID
 * @param[in]  self 指向 BSP 对象的指针
 * @param[out] mid  工厂 ID
 * @param[out] did  设备 ID
 * @return  0 表示成功，其他值表示失败
 */
static int bsp_ext_flash_read_id_impl(bsp_ext_flash_t *self, uint8_t *mid, uint16_t *did)
{
    w25qx_dev_t *dev = (w25qx_dev_t *)self->drv;

    return dev->ops->read_id(dev, mid, did);
}

/**
 * @brief   BSP 外部 Flash 按页写入，写入的地址范围不能跨页，每页 256 字节
 * @param[in] self 指向 BSP 对象的指针
 * @param[in] addr 起始地址
 * @param[in] cnt  要写入数据的数量
 * @param[in] data 用于写入数据的数组
 * @return  0 表示成功，其他值表示失败
 */
static int bsp_ext_flash_write_page_impl(bsp_ext_flash_t *self, uint32_t addr, uint32_t cnt, uint8_t *data)
{
    w25qx_dev_t *dev = (w25qx_dev_t *)self->drv;

    return dev->ops->write_page(dev, addr, cnt, data);
}

/**
 * @brief   BSP 外部 Flash 写入不定量数据
 * @param[in] self 指向 BSP 对象的指针
 * @param[in] addr 起始地址
 * @param[in] cnt  要写入数据的数量
 * @param[in] data 用于写入数据的数组
 * @return  0 表示成功，其他值表示失败
 */
static int bsp_ext_flash_write_data_impl(bsp_ext_flash_t *self, uint32_t addr, uint32_t cnt, uint8_t *data)
{
    w25qx_dev_t *dev = (w25qx_dev_t *)self->drv;

    return dev->ops->write_data(dev, addr, cnt, data);
}

/**
 * @brief   BSP 外部 Flash 扇区擦除（4KB）
 * @param[in] self 指向 BSP 对象的指针
 * @param[in] addr 指定扇区的地址
 * @return  0 表示成功，其他值表示失败
 */
static int bsp_ext_flash_erase_sector_impl(bsp_ext_flash_t *self, uint32_t addr)
{
    w25qx_dev_t *dev = (w25qx_dev_t *)self->drv;

    return dev->ops->erase_sector_4kb(dev, addr);
}

/**
 * @brief   BSP 外部 Flash 块擦除（64KB）
 * @details 以 W25Q64 为例，8MB=8192KB，共128个block
 * @param[in] self 指向 BSP 对象的指针
 * @param[in] idx  指定擦除块的索引
 * @return  0 表示成功，其他值表示失败
 */
static int bsp_ext_flash_erase_block_impl(bsp_ext_flash_t *self, uint16_t idx)
{
    w25qx_dev_t *dev = (w25qx_dev_t *)self->drv;

    return dev->ops->erase_block_64kb(dev, idx);
}

/**
 * @brief   BSP 外部 Flash 读取数据
 * @param[in]  self 指向 BSP 对象的指针
 * @param[in]  addr 读取数据的起始地址
 * @param[in]  cnt  要读取数据的数量
 * @param[out] data 用于接收读取数据的数组
 * @return  0 表示成功，其他值表示失败
 */
static int bsp_ext_flash_read_data_impl(bsp_ext_flash_t *self, uint32_t addr, uint32_t cnt, uint8_t *data)
{
    w25qx_dev_t *dev = (w25qx_dev_t *)self->drv;

    return dev->ops->read_data(dev, addr, cnt, data);
}

/* --- 操作表 --- */
static const bsp_ext_flash_ops_t bsp_ext_flash_ops = {
    .init         = bsp_ext_flash_init_impl,
    .read_id      = bsp_ext_flash_read_id_impl,
	.write_page   = bsp_ext_flash_write_page_impl,
	.write_data   = bsp_ext_flash_write_data_impl,
	.erase_sector = bsp_ext_flash_erase_sector_impl,
	.erase_block  = bsp_ext_flash_erase_block_impl,
	.read_data    = bsp_ext_flash_read_data_impl,
};

/* --- 单例对象 --- */
static bsp_ext_flash_t bsp_ext_flash = {
    .ops = &bsp_ext_flash_ops,
    .drv = &w25qx_dev,
};

/**
 * @brief   获取 BSP 单例对象
 * @return  指向全局 BSP 对象的指针
 */
bsp_ext_flash_t *bsp_ext_flash_get(void)
{
    return &bsp_ext_flash;
}

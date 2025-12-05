#ifndef BSP_EXT_FLASH_H
#define BSP_EXT_FLASH_H

#include <stdint.h>

/* 外部 Flash 存储结构宏定义 */
#define EXT_FLASH_PAGE_SIZE         	256  							    /* 每页256字节 */
#define EXT_FLASH_BLOCK_64KB_PAGE_CNT	(64 * 1024 / EXT_FLASH_PAGE_SIZE)	/* 每块包含256页 */
#define EXT_FLASH_SECTOR_4KB_PAGE_CNT   (4 * 1024 / EXT_FLASH_PAGE_SIZE)	/* 每扇区包含16页 */

typedef struct bsp_ext_flash bsp_ext_flash_t;

/* 操作接口 */
typedef struct {
    int (*init)(bsp_ext_flash_t *self);
    int (*read_id)(bsp_ext_flash_t *self, uint8_t *mid, uint16_t *did);
    int (*write_page)(bsp_ext_flash_t *self, uint32_t addr, uint32_t cnt, uint8_t *data);
	int (*write_data)(bsp_ext_flash_t *self, uint32_t addr, uint32_t cnt, uint8_t *data);
	int (*erase_sector)(bsp_ext_flash_t *self, uint32_t addr);
	int (*erase_block)(bsp_ext_flash_t *self, uint16_t idx);
	int (*read_data)(bsp_ext_flash_t *self, uint32_t addr, uint32_t cnt, uint8_t *data);
} bsp_ext_flash_ops_t;

/* 设备实例结构体 */
struct bsp_ext_flash {
    const bsp_ext_flash_ops_t *ops;
    void *drv;  /* 指向底层驱动对象 */
};

/* 获取 BSP 单例对象 */
bsp_ext_flash_t *bsp_ext_flash_get(void);

#endif  /* BSP_EXT_FLASH_H */

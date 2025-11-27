#include "bsp_delay.h"
#include "drv_delay.h"
#include <stddef.h>
#include <errno.h>

/**
 * @brief   BSP 初始化延时
 * @param[in] self 指向 BSP 对象的指针
 * @return  0 表示成功，其他值表示失败
 */
static int bsp_delay_init_impl(bsp_delay_t *self)
{
    return 0;
}

/* --- 操作表 --- */
static const bsp_delay_ops_t bsp_delay_ops = {
    .init = bsp_delay_init_impl
};

/* --- 单例对象 --- */
static bsp_delay_t bsp_delay = {
    .ops = &bsp_delay_ops,
    .drv = NULL,
};

/**
 * @brief   获取 BSP 单例对象
 * @return  指向全局 BSP 对象的指针
 */
bsp_delay_t *bsp_delay_get(void)
{
    return &bsp_delay;
}

/**
 * @brief   BSP 微秒级延时
 * @param[in] self 指向 BSP 对象的指针
 * @param[in] us   要延时的微秒数
 * @return  0 表示成功，其他值表示失败
 */
void bsp_delay_us(uint32_t us)
{
    delay_us(us);
}

/**
 * @brief   BSP 毫秒级延时
 * @param[in] self 指向 BSP 对象的指针
 * @param[in] ms   要延时的毫秒数
 * @return  0 表示成功，其他值表示失败
 */
void bsp_delay_ms(uint32_t ms)
{
    delay_ms(ms);
}

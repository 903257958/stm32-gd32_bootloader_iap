#include <stddef.h>
#include <stdint.h>
#include "stm32f10x.h"
#include "bsp_common.h"
#include "bsp_delay.h"
#include "bsp_i2c_bus.h"
#include "bsp_eeprom.h"
#include "bsp_ext_flash.h"
#include "bsp_net.h"
#include "log.h"

/**
 * @brief   共享 BSP 初始化
 * @return  0 表示成功
 */
int bsp_common_init(void)
{
    int ret;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    
    bsp_delay_t *delay = bsp_delay_get();
    ret = delay->ops->init(delay);
    if (ret) {
        log_error("Failed to init bsp delay: %d", ret);
        return ret;
    }

    bsp_i2c_bus_t *i2c_bus = bsp_i2c_bus_get();
    ret = i2c_bus->ops->init(i2c_bus);
    if (ret) {
        log_error("Failed to init bsp i2c_bus: %d", ret);
        return ret;
    }
    
    bsp_eeprom_t *eeprom = bsp_eeprom_get();
    ret = eeprom->ops->init(eeprom);
    if (ret) {
        log_error("Failed to init bsp eeprom: %d", ret);
        return ret;
    }
    
    bsp_ext_flash_t *ext_flash = bsp_ext_flash_get();
    ret = ext_flash->ops->init(ext_flash);
    if (ret) {
        log_error("Failed to init bsp ext flash: %d", ret);
        return ret;
    }

    bsp_net_t *net = bsp_net_get();
    ret = net->ops->init(net);
    if (ret) {
        log_error("Failed to init bsp net: %d", ret);
        return ret;
    }

    log_info("====== [BSP Init Successfully] ======\r\n");
    return 0;
}

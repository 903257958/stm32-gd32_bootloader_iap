#include <stdbool.h>
#include "bsp_delay.h"
#include "boot_core.h"
#include "boot_config.h"
#include "boot_comm.h"
#include "boot_cmd.h"
#include "boot_store.h"
#include "boot_ota.h"
#include "boot_ext_flash.h"
#include "log.h"

#if BOOT_PLATFORM_STM32F1
#include "stm32f10x.h"
#elif BOOT_PLATFORM_STM32F4
#include "stm32f4xx.h"
#elif BOOT_PLATFORM_GD32F1
#include "gd32f10x.h"
#else
#error boot_core.h: No processor defined!
#endif

/* 函数指针，用于跳转到A区应用程序入口 */
typedef void (*app_entry_t)(void);

typedef struct {
    uint8_t  update_chunk[BOOT_APP_UPDATE_CHUNK_SIZE];  // 更新 APP 时，每次搬运的数据块（内部 Flash 页大小）
    uint32_t flag;  // 标志位
} boot_ctx_t;

static boot_ctx_t g_boot_ctx;

/**
 * @brief   设置目标应用程序的主堆栈指针（MSP）
 * @param[in] addr 启动向量表第 0 项的值（APP 初始 MSP）
 */
__asm static void boot_set_msp(uint32_t addr)
{
    MSR MSP, r0	// 把寄存器r0里的值写入MSP寄存器，作为主栈起始地址，r0的值是第一个参数，即addr
    BX r14		// 返回上一层（即执行跳转后的函数），r14是链接寄存器（LR），记录调用者的返回地址
}

/**
 * @brief   关闭外设，恢复系统状态
 */
static void boot_reset_periph(void)
{

}

/**
 * @brief   跳转到目标 APP 程序入口
 * @param[in] addr APP 启动向量表的起始地址
 */
static void boot_jump_to_app(uint32_t addr)
{
    uint32_t msp;
    uint32_t reset_handler;
    app_entry_t app_entry;

    /* 读取初始 MSP（向量表第 0 项）*/
    msp = *(uint32_t *)addr;

    /* 检查 MSP 是否位于有效 SRAM 区间 */
    if (msp < BOOT_RAM_BASE_ADDR || msp > BOOT_RAM_END_ADDR) {
        log_warn("Invalid MSP: 0x%X, abort jump", msp);
        return;
    }
    log_info("MSP: 0x%X", msp);

    /* 设置主堆栈指针 */
    boot_set_msp(msp);

    /* 读取复位向量（向量表第 1 项） */
    reset_handler = *(uint32_t *)(addr + 4);
    app_entry = (app_entry_t)reset_handler;

    /* 切换中断向量表到 APP 区 */
    SCB->VTOR = addr;
    __DSB();
    __ISB();

    /* 关闭外设，恢复系统状态 */
    boot_reset_periph();

    /* 跳转到 APP reset handler */
    app_entry();
}

/**
 * @brief   BootLoader 检查是否进入命令行
 * @param[in] timeout_ms 超时时间（毫秒）
 */
static bool boot_check_enter_cmd(uint16_t timeout_ms)
{
    uint8_t *rx_data = NULL;
    uint32_t rx_len = 0;

    while (timeout_ms--) {
        boot_recv_data(&rx_data, &rx_len);
        if (rx_data && rx_len == 1)
            if (rx_data[0] == 'w' || rx_data[0] == 'W')
                return true;
		bsp_delay_ms(1);
	}

    return false;
}

/**
 * @brief   BootLoader 入口处理
 */
void boot_process_entry(void)
{
    const uint16_t timeout_ms = 2000;

    log_info("Bootloader: Press 'w' within %d seconds to enter command line.", timeout_ms / 1000);

    /* 不进入命令行 */
    if (!boot_check_enter_cmd(timeout_ms)) {
        if (boot_ota_should_upgrade()) {
            boot_set_flag(BOOT_FLAG_EXT_LOAD);
            boot_ext_flash_ota_init();
        } else {
            log_info("Bootloader: Jump to APP...");
            boot_jump_to_app(BOOT_FLASH_APP_START_ADDR);
        }
    }

    /* 进入命令行 */
	log_info("Bootloader: Enter command line.");
}

/**
 * @brief   BootLoader 系统复位
 */
void boot_system_reset(void)
{
    bsp_delay_ms(200);
    NVIC_SystemReset();
}

/**
 * @brief   BootLoader 设置标志位
 * @param[in] flag 标志位
 */
void boot_set_flag(boot_flag_t flag)
{
    g_boot_ctx.flag |= flag;
}

/**
 * @brief   BootLoader 清除标志位
 * @param[in] flag 标志位
 */
void boot_clear_flag(boot_flag_t flag)
{
    g_boot_ctx.flag &= ~flag;
}

/**
 * @brief   BootLoader 是否存在某标志位
 * @param[in] flag 标志位
 * @return  true 表示标志位存在，false 表示标志位不存在
 */
bool boot_has_flag(boot_flag_t flag)
{
    return (g_boot_ctx.flag & flag) != 0;
}

/**
 * @brief   BootLoader 获取当前标志位
 * @return  true 表示标志位存在，false 表示标志位不存在
 */
uint32_t boot_get_flag(void)
{
    return g_boot_ctx.flag;
}

/**
 * @brief   BootLoader 获取 APP 更新块
 * @return  APP 更新块首地址
 */
uint8_t* boot_get_update_chunk(void)
{
    return g_boot_ctx.update_chunk;
}

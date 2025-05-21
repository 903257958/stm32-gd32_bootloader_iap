#include "main.h"

/* 硬件设备定义 */
EEPROMDev_t at24c02 = {.config = {GPIOB, GPIO_PIN_6, GPIOB, GPIO_PIN_7}};
W25QXDev_t w25q64 = {.config = {SPI0, GPIOA, GPIO_PIN_5, GPIOA, GPIO_PIN_6, GPIOA, GPIO_PIN_7, GPIOA, GPIO_PIN_4}};
FMCDev_t fmc;

int main(void)
{
	/* 设置中断分组 */
	nvic_priority_group_set(NVIC_PRIGROUP_PRE2_SUB2);

	/* 硬件设备初始化 */
    delay_init(108);
	uart0_init(921600);
    eeprom_init(&at24c02);
    w25qx_init(&w25q64);
	fmc_init(&fmc);

	uart0_printf("\r\n");

	/* EEPROM读取IAP信息 */
	eeprom_read_iap_info();

	/* BootLoader分支判断 */
	bootloader_branch();
	
	while(1)
	{
		/* 串口0接收处理 */
		if (uart0_rx_control_block.index_in != uart0_rx_control_block.index_out)
		{
			/* 事件处理 */
			bootloader_event(uart0_rx_control_block.index_out->start, uart0_rx_control_block.index_out->end - uart0_rx_control_block.index_out->start + 1);

			/* 处理完当前数据段后，移动index_out指针到下一个位置，准备处理下一段数据 */
			uart0_rx_control_block.index_out++;

			/* 如果index_out到达索引数组末尾，则回卷到开始位置 */
			if (uart0_rx_control_block.index_out == uart0_rx_control_block.index_end)
			{
				uart0_rx_control_block.index_out = &uart0_rx_control_block.index_buf[0];
			}
		}
		
		/* 串口IAP，发C事件 */
		if (g_bootloader_status & IAP_XMODEM_SEND_C_FLAG)
		{
			handle_xmodem_send_c();
		}

		/* Flash A区程序更新 */
		if (g_bootloader_status & FLASH_A_UPDATE_FLAG)
		{
			handle_flash_a_update();
		}
	}
}

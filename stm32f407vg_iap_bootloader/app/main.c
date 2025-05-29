#include "main.h"

/* 硬件设备定义 */
UARTDev_t debug = {.config = {USART1, 921600, GPIOA, GPIO_Pin_9, GPIOA, GPIO_Pin_10}};
EEPROMDev_t at24c02 = {.config = {GPIOB, GPIO_Pin_6, GPIOB, GPIO_Pin_7}};
W25QXDev_t w25q128 = {.config = {SPI1, GPIOA, GPIO_Pin_5, GPIOA, GPIO_Pin_6, GPIOA, GPIO_Pin_7, GPIOC, GPIO_Pin_13}};
FlashDev_t flash;

int main(void)
{
    /* 设置中断分组 */
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

    /* 硬件设备初始化 */
    delay_init(168);
	uart_init(&debug);
    eeprom_init(&at24c02);
    w25qx_init(&w25q128);
	flash_init(&flash);

	debug.printf("\r\n\r\n");

	/* EEPROM读取IAP信息 */
	eeprom_read_iap_info();

	/* BootLoader分支判断 */
	bootloader_branch();
	
	while(1)
	{
		/* 串口1接收处理 */
		if (uart1_rx_cb.index_in != uart1_rx_cb.index_out)
		{
			/* 事件处理 */
			bootloader_event(uart1_rx_cb.index_out->start, uart1_rx_cb.index_out->end - uart1_rx_cb.index_out->start + 1);

			/* 处理完当前数据段后，移动index_out指针到下一个位置，准备处理下一段数据 */
			uart1_rx_cb.index_out++;

			/* 如果index_out到达索引数组末尾，则回卷到开始位置 */
			if (uart1_rx_cb.index_out == uart1_rx_cb.index_end)
			{
				uart1_rx_cb.index_out = &uart1_rx_cb.index_buf[0];
			}
		}
		
		/* 串口IAP，发C事件 */
		if (g_bootloader_status & IAP_XMODEM_SEND_C_FLAG)
		{
			handle_xmodem_send_c();
		}

		/* Flash A区程序更新 */
		if (g_bootloader_status & APP_UPDATE_FLAG)
		{
			handle_app_update();
		}
	}
}

#include "main.h"

/* 硬件设备定义 */
static uint8_t uart1_tx_buf[2048];
static uint8_t uart1_rx_buf[2048];
uart_dev_t uart1 = {
    .config = {
        .uartx          = USART1,
        .baud           = 115200,
        .tx_port        = GPIOA,
        .tx_pin         = GPIO_Pin_9,
        .rx_port        = GPIOA,
        .rx_pin         = GPIO_Pin_10,
        .tx_buf         = uart1_tx_buf,
        .rx_buf         = uart1_rx_buf,
        .tx_buf_size    = sizeof(uart1_tx_buf),
        .rx_buf_size    = sizeof(uart1_rx_buf),
        .rx_single_max  = 512
    }
};

eeprom_dev_t at24c02 = {
    .config = {GPIOB, GPIO_Pin_6, GPIOB, GPIO_Pin_7}
};

w25qx_dev_t w25q128 = {
    .config = {SPI2, GPIOB, GPIO_Pin_13, GPIOB, GPIO_Pin_14, GPIOB, GPIO_Pin_15, GPIOA, GPIO_Pin_15}
};

flash_dev_t flash;

int main(void)
{
    /* 设置中断分组 */
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

	/* 解除JTAG */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
	DBGMCU->CR &= ~((uint32_t)1 << 5);

    /* 硬件设备初始化 */
	uart_init(&uart1);
    eeprom_init(&at24c02);
    w25qx_init(&w25q128);
	flash_init(&flash);

	uart1.printf("\r\n");

	/* EEPROM读取IAP信息 */
	eeprom_read_iap_info();

	/* BootLoader分支判断 */
	bootloader_branch();
	
	while (1)
	{
		/* 串口1接收处理 */
		if (uart1.rx_cb.index_in != uart1.rx_cb.index_out)
		{
			/* 事件处理 */
			bootloader_event(uart1.rx_cb.index_out->start, uart1.rx_cb.index_out->end - uart1.rx_cb.index_out->start + 1);

			/* 处理完当前数据段后，移动index_out指针到下一个位置，准备处理下一段数据 */
			uart1.rx_cb.index_out++;

			/* 如果index_out到达索引数组末尾，则回卷到开始位置 */
			if (uart1.rx_cb.index_out == uart1.rx_cb.index_end)
			{
				uart1.rx_cb.index_out = &uart1.rx_cb.index_buf[0];
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

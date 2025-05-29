#include "bootloader.h"

p_load_app g_load_app;					// 声明全局函数指针
IAPInfo_t iap_info;						// IAP信息结构体，存储在EEPROM中
APPUpdateControlBlock_t app_update_cb;	// APP更新控制块结构体
uint32_t g_bootloader_status;			// 状态变量

/* 函数声明 */
static int8_t __enter_command_line(uint16_t timeout);
static void __command_line_info(void);
__asm static void __msr_sp(uint32_t addr);
static void __load_app(uint32_t addr);
static void __erase_app(void);
static void __bootloader_clear(void);
static uint16_t __xmodem_crc16(uint8_t *data, uint16_t len);
static void __handle_command_line_input(uint8_t *data, uint16_t len);
static void __handle_xmodem_send_data(uint8_t *data, uint16_t len);
static void __handle_eflash_program_download_select(uint8_t *data, uint16_t len);
static void __handle_eflash_program_load_select(uint8_t *data, uint16_t len);

/******************************************************************************
 * @brief	EEPROM读取IAP信息
 * @param	无
 * @return	无
 ******************************************************************************/
void eeprom_read_iap_info(void)
{
	memset(&iap_info, 0, IAP_INFO_SIZE);
	at24c02.read_data(&at24c02, 0, IAP_INFO_SIZE, (uint8_t *)&iap_info);
}

/******************************************************************************
 * @brief	EEPROM写入IAP信息
 * @param	无
 * @return	无
 ******************************************************************************/
void eeprom_write_iap_info(void)
{
	uint8_t i;
	uint8_t *write_ptr;

	write_ptr = (uint8_t *)&iap_info;

	for (i = 0; i < IAP_INFO_SIZE / EEPROM_PAGE_SIZE; i++)
	{
		at24c02.write_page(&at24c02, i * EEPROM_PAGE_SIZE, write_ptr + i * EEPROM_PAGE_SIZE);
		delay_ms(5);
	}
}

/******************************************************************************
 * @brief	BootLoader进入命令行
 * @param	timeout	:	超时时间（单位：毫秒）
 * @return	0, 表示成功, 其他值表示失败
 ******************************************************************************/
static int8_t __enter_command_line(uint16_t timeout)
{
	BOOT_DEBUG("\r\nEnter \"w\" within %d seconds to enter the command line.\r\n", timeout / 1000);

	while (timeout--)
	{
		if (uart1_rx_buf[0] == 'w')
		{
			return 0;	// 进入命令行
		}
		delay_ms(1);
	}

	return -1;			// 不进入命令行
}

/******************************************************************************
 * @brief	BootLoader命令行信息
 * @param	无
 * @return	无
 ******************************************************************************/
static void __command_line_info(void)
{
	BOOT_DEBUG("\r\n");
	BOOT_DEBUG("[1]Erase APP\r\n");									// 擦除APP
	BOOT_DEBUG("[2]IAP download the program to Flash\r\n");			// 串口IAP下载程序到内部Flash
	BOOT_DEBUG("[3]IAP download programs to external Flash\r\n");	// 串口IAP下载程序到外部Flash
	BOOT_DEBUG("[4]Loading programs from external Flash\r\n");		// 加载外部Flash内的程序
	BOOT_DEBUG("[5]Restart\r\n");									// 重启
}

/******************************************************************************
 * @brief	BootLoader分支判断
 * @param	无
 * @return	无
 ******************************************************************************/
void bootloader_branch(void)
{
	/* 不进入命令行 */
	if (__enter_command_line(2000) == -1)
	{
		BOOT_DEBUG("\r\nJump to APP!\r\n");
		__load_app(FLASH_A_START_ADDR);	// 跳转到Flash的A区主程序APP
	}

	/* 进入命令行 */
	BOOT_DEBUG("\r\nEnter the command line.\r\n");
	__command_line_info();
}

/******************************************************************************
 * @brief	BootLoader事件处理
 * @param	data	:	数据
 * @param	len		:	数据长度
 * @return	无
 ******************************************************************************/
void bootloader_event(uint8_t *data, uint16_t len)
{
	if (g_bootloader_status == 0)
	{
		/* 无任何标志位置位时，只进行命令行输入处理 */
		__handle_command_line_input(data, len);
	}
	else if (g_bootloader_status & IAP_XMODEM_SEND_DATA_FLAG)
	{
		/* 串口IAP Xmodem发送数据（选择串口IAP下载程序到内部Flash或外部Flash时被置位） */
		__handle_xmodem_send_data(data, len);
	}
	else if (g_bootloader_status & E_FLASH_PROGRAM_DOWNLOAD_SELECT_FLAG)
	{
		/* 串口IAP下载程序到外部Flash-选择 */
		__handle_eflash_program_download_select(data, len);
	}
	else if (g_bootloader_status & E_FLASH_PROGRAM_LOAD_SELECT_FLAG)
	{
		/* 加载外部Flash的程序-选择 */
		__handle_eflash_program_load_select(data, len);
	}
}

/******************************************************************************
 * @brief	事件处理：Xmodem协议发C
 * @param	无
 * @return	无
 ******************************************************************************/
void handle_xmodem_send_c(void)
{
	delay_ms(1);
	if (app_update_cb.xmodem_timeout >= 1000)
	{
		BOOT_DEBUG("C");
		app_update_cb.xmodem_timeout = 0;
	}
	app_update_cb.xmodem_timeout++;
}

/******************************************************************************
 * @brief	事件处理：APP程序更新，将程序从外部Flash写进内部Flash
 * @param	无
 * @return	无
 ******************************************************************************/
void handle_app_update(void)
{
	uint8_t i;
	
	/* 更新A区 */
	BOOT_DEBUG("\r\nLoad the %dth program, length is %d bytes.\r\n", app_update_cb.eflash_program_index, iap_info.app_size[app_update_cb.eflash_program_index]);

	/* 判断下载长度是否为4字节对齐 */
	if (iap_info.app_size[app_update_cb.eflash_program_index] % 4 == 0)
	{
		/* 擦除内部Flash A区 */
		__erase_app();

		/* 下载长度为4字节对齐，先写完整的页 */
		for (i = 0; i < iap_info.app_size[app_update_cb.eflash_program_index] / SINGLE_UPDATE_SIZE; i++)
		{
			/* 从W25QX中搬运出一次数据到update_buf */
			w25q128.read_data(&w25q128, 
							app_update_cb.eflash_program_index * E_FLASH_PROGRAM_MAX_SIZE + i * SINGLE_UPDATE_SIZE, 
							app_update_cb.update_buf, 
							SINGLE_UPDATE_SIZE);

			/* 将本次数据写入内部Flash */
			flash.write(&flash, 
						FLASH_A_START_ADDR + i * SINGLE_UPDATE_SIZE, 
						(uint32_t *)app_update_cb.update_buf, 
						SINGLE_UPDATE_SIZE);
		}

		/* 处理剩余不足一页的字节 */
		if (iap_info.app_size[app_update_cb.eflash_program_index] % SINGLE_UPDATE_SIZE != 0)
		{
			/* 从W25QX中搬运出剩余数据 */
			w25q128.read_data(	&w25q128, 
								app_update_cb.eflash_program_index * E_FLASH_PROGRAM_MAX_SIZE + i * SINGLE_UPDATE_SIZE, 
								app_update_cb.update_buf, 
								iap_info.app_size[app_update_cb.eflash_program_index] % SINGLE_UPDATE_SIZE);

			/* 将剩余数据写入内部Flash */
			flash.write(&flash, 
						FLASH_A_START_ADDR + i * SINGLE_UPDATE_SIZE, 
						(uint32_t *)app_update_cb.update_buf, 
						iap_info.app_size[app_update_cb.eflash_program_index] % SINGLE_UPDATE_SIZE);
		}
		
		/* 系统复位 */
		BOOT_DEBUG("\r\nAPP update completed, restart!\r\n");
		NVIC_SystemReset();
	}
	else
	{
		/* 长度错误 */
		BOOT_DEBUG("\r\nLength error!\r\n");
		g_bootloader_status &= ~APP_UPDATE_FLAG;
	}
}

/******************************************************************************
 * @brief	事件处理：命令行输入
 * @param	data	:	串口接收到的数据
 * @param	dalenta	:	串口接收到的数据长度
 * @return	无
 ******************************************************************************/
static void __handle_command_line_input(uint8_t *data, uint16_t len)
{
	if (len == 1 && data[0] == '1')
	{
		/* 擦除APP */
		__erase_app();
	}
	else if (len == 1 && data[0] == '2')
	{
		/* 串口IAP下载程序到内部Flash */
		BOOT_DEBUG("\r\nIAP download the program to Flash.\r\n");
		__erase_app();
		BOOT_DEBUG("\r\nPlease use bin format file.\r\n");
		g_bootloader_status |= (IAP_XMODEM_SEND_C_FLAG | IAP_XMODEM_SEND_DATA_FLAG);	// 置位IAP事件
		app_update_cb.xmodem_timeout = 0;
		app_update_cb.xmodem_packet_cnt = 0;
	}
	else if (len == 1 && data[0] == '3')
	{
		/* 串口IAP下载程序到外部Flash */
		BOOT_DEBUG("\r\nIAP download programs to external Flash, please enter the program location (1-9).\r\n");
		g_bootloader_status |= E_FLASH_PROGRAM_DOWNLOAD_SELECT_FLAG;
	}
	else if (len == 1 && data[0] == '4')
	{
		/* 加载外部Flash内的程序 */
		BOOT_DEBUG("\r\nLoading programs from external Flash, please enter the program location (1-9).\r\n");
		g_bootloader_status |= E_FLASH_PROGRAM_LOAD_SELECT_FLAG;
		
	}
	else if (len == 1 && data[0] == '5')
	{
		/* 重启 */
		BOOT_DEBUG("\r\nRestart!\r\n");
		delay_ms(100);
		NVIC_SystemReset();
	}
}

/******************************************************************************
 * @brief	事件处理：Xmodem发送数据，将程序从串口下载到外部Flash或内部Flash
 * @param	data	:	串口接收到的数据
 * @param	dalenta	:	串口接收到的数据长度
 * @return	无
 ******************************************************************************/
static void __handle_xmodem_send_data(uint8_t *data, uint16_t len)
{
	uint8_t i;

	/* 接收到一个数据包 */
	if (len == XMODEM_PACKET_LEN && data[0] == XMODEM_SOH)
	{
		g_bootloader_status &= ~IAP_XMODEM_SEND_C_FLAG;											// 清除发C标志位
		app_update_cb.xmodem_crc_val = __xmodem_crc16(&data[3], XMODEM_PACKET_DATA_LEN);	// 只对有效数据进行CRC校验

		/* 对接收到的数据包进行CRC16校验 */
		if (app_update_cb.xmodem_crc_val == data[131] * 256 + data[132])	// CRC校验值正确
		{
			app_update_cb.xmodem_packet_cnt++;	// 接收数据包计数

			/* 保存接收到的数据包到update_buf */
			memcpy(	&app_update_cb.update_buf[((app_update_cb.xmodem_packet_cnt - 1) % (SINGLE_UPDATE_SIZE / XMODEM_PACKET_DATA_LEN)) * XMODEM_PACKET_DATA_LEN], 
					&data[3], 
					XMODEM_PACKET_DATA_LEN);

			/* 每接收完一个数据包，都判断当前update_buf是否写满（接收了1KB数据 = 内部Flash页大小） */
			if ((app_update_cb.xmodem_packet_cnt % (SINGLE_UPDATE_SIZE / XMODEM_PACKET_DATA_LEN)) == 0)
			{
				if (g_bootloader_status & E_FLASH_PROGRAM_DOWNLOAD_XMODEM_FLAG)
				{
					/* 向外部Flash写入：每页256字节，所以1KB的数据需要分4次写入 */
					for (i = 0; i < SINGLE_UPDATE_SIZE / E_FLASH_PAGE_SIZE; i++)
					{
						w25q128.page_write(	&w25q128, 
											app_update_cb.eflash_program_index * E_FLASH_PROGRAM_MAX_SIZE + (app_update_cb.xmodem_packet_cnt / 8 - 1) * SINGLE_UPDATE_SIZE + i * E_FLASH_PAGE_SIZE, 
											&app_update_cb.update_buf[i * E_FLASH_PAGE_SIZE], 
											E_FLASH_PAGE_SIZE);
					}
				}
				else
				{
					/* 向内部Flash写入：凑满n个包，能够写满一页内部Flash，写入 */
					flash.write(&flash, 
								FLASH_A_START_ADDR + ((app_update_cb.xmodem_packet_cnt / (SINGLE_UPDATE_SIZE / XMODEM_PACKET_DATA_LEN)) - 1) * SINGLE_UPDATE_SIZE, 
								(uint32_t *)app_update_cb.update_buf, 
								SINGLE_UPDATE_SIZE);
				}
			}
			BOOT_DEBUG("\x06");	// CRC校验错误发送ACK
		}
		else
		{
			BOOT_DEBUG("\x15");	// CRC校验错误发送NCK
		}
	}

	/* 接收到EOT，传输完成 */
	if (len == 1 && data[0] == XMODEM_EOT)	// EOT = 0x04
	{
		BOOT_DEBUG("\x06");	// 发送ACK

		/* 把剩余不足一页内部Flash（1KB）的包写入 */
		if ((app_update_cb.xmodem_packet_cnt % (SINGLE_UPDATE_SIZE / XMODEM_PACKET_DATA_LEN)) != 0)
		{
			if (g_bootloader_status & E_FLASH_PROGRAM_DOWNLOAD_XMODEM_FLAG)
			{
				/* 写入外部Flash，每页256字节，循环4次会多写但是不影响 */
				for (i = 0; i < SINGLE_UPDATE_SIZE / E_FLASH_PAGE_SIZE; i++)
				{
					w25q128.page_write(	&w25q128, 
										app_update_cb.eflash_program_index * E_FLASH_PROGRAM_MAX_SIZE + (app_update_cb.xmodem_packet_cnt / 8) * SINGLE_UPDATE_SIZE + i * E_FLASH_PAGE_SIZE, 
										&app_update_cb.update_buf[i * E_FLASH_PAGE_SIZE], 
										E_FLASH_PAGE_SIZE);
				}
			}
			else
			{
				/* 写入内部Flash */
				flash.write(&flash, 
							FLASH_A_START_ADDR + (app_update_cb.xmodem_packet_cnt / (SINGLE_UPDATE_SIZE / XMODEM_PACKET_DATA_LEN)) * SINGLE_UPDATE_SIZE, 
							(uint32_t *)app_update_cb.update_buf, 
							(app_update_cb.xmodem_packet_cnt % (SINGLE_UPDATE_SIZE / XMODEM_PACKET_DATA_LEN)) * XMODEM_PACKET_DATA_LEN);
			}
		}

		g_bootloader_status &= ~IAP_XMODEM_SEND_DATA_FLAG;	// 清除发送数据标志位

		if (g_bootloader_status & E_FLASH_PROGRAM_DOWNLOAD_XMODEM_FLAG)
		{
			/* 下载到外部Flash */
			g_bootloader_status &= ~E_FLASH_PROGRAM_DOWNLOAD_XMODEM_FLAG;	// 清除标志位

			/* 保存此程序的大小 */
			iap_info.app_size[app_update_cb.eflash_program_index] = app_update_cb.xmodem_packet_cnt * XMODEM_PACKET_DATA_LEN;

			/* EEPROM写入IAP信息 */
			eeprom_write_iap_info();

			/* 下载完成 */
			BOOT_DEBUG("Download completed!\r\n");
			delay_ms(100);
			__command_line_info();
		}
		else
		{
			/* 下载到内部Flash，重启 */
			delay_ms(200);
			BOOT_DEBUG("IAP update completed, restart!\r\n");
			delay_ms(200);
			NVIC_SystemReset();
		}
	}
}

/******************************************************************************
 * @brief	事件处理：下载程序到外部Flash-选择
 * @param	data	:	串口接收到的数据
 * @param	dalenta	:	串口接收到的数据长度
 * @return	无
 ******************************************************************************/
static void __handle_eflash_program_download_select(uint8_t *data, uint16_t len)
{
	int i;

	if (len == 1)
	{
		/* 数据长度正确 */
		if (data[0] >= 0x31 && data[0] <= 0x39) 
		{
			/* 编号在‘1’和‘9’之间 */
			g_bootloader_status &= ~E_FLASH_PROGRAM_DOWNLOAD_SELECT_FLAG;			// 清除标志位

			app_update_cb.eflash_program_index = data[0] - 0x30;					// 选择写入外部Flash的块索引
			g_bootloader_status |= (IAP_XMODEM_SEND_C_FLAG | IAP_XMODEM_SEND_DATA_FLAG | E_FLASH_PROGRAM_DOWNLOAD_XMODEM_FLAG);
			app_update_cb.xmodem_timeout = 0;
			app_update_cb.xmodem_packet_cnt = 0;
			iap_info.app_size[app_update_cb.eflash_program_index] = 0;

			/* 先擦除要写入程序的块 */
			BOOT_DEBUG("\r\nErasing the %dth program block of external Flash...\r\r\n", app_update_cb.eflash_program_index);
			for (i = 0; i < E_FLASH_PROGRAM_MAX_SIZE / E_FLASH_BLOCK_SIZE; i++)
			{
				w25q128.block_erase_64kb(&w25q128, app_update_cb.eflash_program_index * (E_FLASH_PROGRAM_MAX_SIZE / E_FLASH_BLOCK_SIZE) + i);
			}

			BOOT_DEBUG("\r\nDownload the program to the %dth location of external Flash using the Xmodem protocol, please use bin format file.\r\n", app_update_cb.eflash_program_index);
		}
		else
		{
			/* 编号错误 */
			BOOT_DEBUG("Number error!\r\n");
		}
	}
	else
	{
		/* 数据长度错误 */
		BOOT_DEBUG("Data length error!\r\n");
	}
}

/******************************************************************************
 * @brief	事件处理：加载外部Flash的程序-选择
 * @param	data	:	串口接收到的数据
 * @param	dalenta	:	串口接收到的数据长度
 * @return	无
 ******************************************************************************/
static void __handle_eflash_program_load_select(uint8_t *data, uint16_t len)
{
	if (len == 1)
	{
		/* 数据长度正确 */
		if (data[0] >= 0x31 && data[0] <= 0x39)
		{
			/* 编号在‘1’和‘9’之间 */
			g_bootloader_status &= ~E_FLASH_PROGRAM_LOAD_SELECT_FLAG;	// 清除标志位

			app_update_cb.eflash_program_index = data[0] - 0x30;
			g_bootloader_status |= APP_UPDATE_FLAG;
		}
		else
		{
			/* 编号错误 */
			BOOT_DEBUG("Number error!\r\n");
		}
	}
	else
	{
		/* 数据长度错误 */
		BOOT_DEBUG("Data length error!\r\n");
	}
}

/******************************************************************************
 * @brief	设置跳转目标程序的主堆栈指针MSP
 * @param	addr
 * @return	无
 ******************************************************************************/
__asm static void __msr_sp(uint32_t addr)
{
    MSR MSP, r0	// 把寄存器r0里的值写入MSP寄存器，作为主栈起始地址，r0的值就是addr这个参数
    BX r14		// 返回上一层（即执行跳转后的函数），r14是链接寄存器（LR），记录调用者的返回地址
}

/******************************************************************************
 * @brief	跳转到Flash的A区主程序
 * @param	addr
 * @return	无
 ******************************************************************************/
static void __load_app(uint32_t addr)
{
	/* 判断起始地址是否在SRAM区间（合法堆栈指针范围） */
    if ((*(uint32_t *)addr >= RAM_ADDR_MIN) && (*(uint32_t *)addr <= RAM_ADDR_MAX))
    {
        __msr_sp(*(uint32_t *)addr);	// 设置MSP

		/* 让函数指针指向复位向量的位置并复位 */
		g_load_app = (p_load_app)*(uint32_t *)(addr + 4);	// 读取A区复位向量（程序入口地址）
		__bootloader_clear();	// BootLoader清理，把B区用到的外设、寄存器RESET
		g_load_app();		// 执行跳转，reset
    }
	else
	{
		BOOT_DEBUG("\r\nJumping to APP failed!\r\n");
	}
}

/******************************************************************************
 * @brief	擦除Flash的A区主程序
 * @param	addr
 * @return	无
 ******************************************************************************/
static void __erase_app(void)
{
	BOOT_DEBUG("\r\nErase APP.\r\n");
	
	#if defined(STM32F10X_HD) || defined(STM32F10X_MD) || defined (GD32F10X_MD) || defined (GD32F10X_HD)

	flash.page_erase(&flash, FLASH_A_START_PAGE, FLASH_A_PAGE_NUM);

	#elif defined(STM32F40_41xxx) || defined(STM32F429_439xx)

	uint8_t i; 

	for (i = FLASH_A_START_SECTOR; i < FLASH_SECTOR_NUM; i++)
	{
		flash.sector_erase(&flash, i);
		BOOT_DEBUG("Successfully erased sector %d.\r\n", i);
	}

	#endif
	
	BOOT_DEBUG("Successfully erased APP!\r\n");
}

/******************************************************************************
 * @brief	BootLoader清理，把B区用到的外设、寄存器RESET
 * @param	无
 * @return	无
 ******************************************************************************/
static void __bootloader_clear(void)
{
    #if defined(STM32F10X_HD) || defined(STM32F10X_MD)

	USART_DeInit(USART1);
	DMA_DeInit(DMA1_Channel5);	
	GPIO_DeInit(GPIOA);
	GPIO_DeInit(GPIOB);
	SPI_I2S_DeInit(SPI2);

	#elif defined(STM32F40_41xxx) || defined(STM32F429_439xx)

    USART_DeInit(USART1);
	DMA_DeInit(DMA2_Stream5);	
	GPIO_DeInit(GPIOA);
	GPIO_DeInit(GPIOB);
	GPIO_DeInit(GPIOC);
	SPI_I2S_DeInit(SPI1);
    
    #elif  defined (GD32F10X_MD) || defined (GD32F10X_HD)

    usart_deinit(USART0);
	gpio_deinit(GPIOA);
	gpio_deinit(GPIOB);
    
	#endif
    
}

/******************************************************************************
 * @brief	Xmodem协议CRC16
 * @param	data	:	待校验的数据
 * @param	len		:	数据长度
 * @return	CRC16校验值
 ******************************************************************************/
static uint16_t __xmodem_crc16(uint8_t *data, uint16_t len)
{
	uint8_t i;
	uint16_t crc_init = 0x0000;
	uint16_t crc_poly = 0x1021;

	while (len--)
	{
		crc_init = (*data << 8) ^ crc_init;
		for (i = 0; i < 8; i++)
		{
			if (crc_init & 0x8000)
			{
				crc_init = (crc_init << 1) ^ crc_poly;
			}
			else
			{
				crc_init = (crc_init << 1);
			}
		}
		data++;
	}

	return crc_init;
}

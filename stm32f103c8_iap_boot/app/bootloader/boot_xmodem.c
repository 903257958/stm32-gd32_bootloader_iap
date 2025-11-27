#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "bsp_delay.h"
#include "bsp_flash.h"
#include "bsp_ext_flash.h"
#include "boot_core.h"
#include "boot_cmd.h"
#include "boot_comm.h"
#include "boot_store.h"
#include "boot_flash.h"
#include "boot_ext_flash.h"
#include "log.h"

/* Xmodem协议 */
#define XMODEM_PACKET_LEN       133	// 数据包总长度 SOH + pkt_no + ~pkt_no + 128 bytes + CRC(2 bytes) 
#define XMODEM_PACKET_DATA_LEN  128	// 数据包有效数据长度
#define XMODEM_SOH              0x01
#define XMODEM_EOT              0x04 

/* 每个 update_chunk 包含多少个 Xmodem 包 */
#define XMODEM_PACKETS_PER_CHUNK	(BOOT_APP_UPDATE_CHUNK_SIZE / XMODEM_PACKET_DATA_LEN)

typedef struct {
    uint32_t xmodem_timeout_ms;	// Xmodem 协议延时
    uint32_t xmodem_packet_cnt;	// Xmodem 协议数据包接收计数，一个包 128 字节，8 个包写满一页内部 Flash
} boot_xmodem_ctx_t;

static boot_xmodem_ctx_t boot_xmodem_ctx;

/**
 * @brief   Xmodem 协议初始化
 */
void boot_xmodem_init(void)
{
    boot_xmodem_ctx.xmodem_timeout_ms = 0;
    boot_xmodem_ctx.xmodem_packet_cnt = 0;
}

/**
 * @brief   按周期发送 'C' 字符开始 Xmodem CRC 模式握手
 */
void boot_xmodem_send_c(void)
{
    bsp_delay_ms(1);
    if (boot_xmodem_ctx.xmodem_timeout_ms >= 1000)
	{
		boot_send_data("C", 1);
		boot_xmodem_ctx.xmodem_timeout_ms = 0;
	}
	boot_xmodem_ctx.xmodem_timeout_ms++;
}

/**
 * @brief   计算 Xmodem 协议 CRC16 校验值
 * @param[in] data 待校验的数据
 * @param[in] len  数据长度
 * @return	CRC16 校验值
 */
static uint16_t boot_xmodem_crc16(uint8_t *data, uint16_t len)
{
    uint16_t crc = 0x0000;
    uint16_t i;
    
    while(len--) {
        crc ^= *data++ << 8;
        for(i = 0; i < 8; i++) {
            if(crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

/**
 * @brief   发送 Xmodem 协议 ACK/NACK
 * @param[in] is_ack true 发送 ACK（0x06），false 发送 NACK（0x15）
 */
static void boot_xmodem_send_ack_nack(bool is_ack)
{
	uint8_t ch = is_ack ? 0x06 : 0x15;
	boot_send_data(&ch, 1);
}

/**
 * @brief   处理一个完整的 Xmodem 数据包
 * @details	工作流程：
 *   		1. 校验 CRC，若不通过则发送 NACK 并退出。
 *   		2. 根据当前 Xmodem 包序号计算写入 update_chunk 的偏移。
 *   		3. 若一个 update_chunk 被填满，则写入内/外部 Flash。
 *   		4. 最终对本包发送 ACK。
 * @param[in] data 数据包首地址（133 字节）
 */
static void boot_xmodem_process_packet(uint8_t *data)
{
	bsp_flash_t *flash = bsp_flash_get();
    bsp_ext_flash_t *ext_flash = bsp_ext_flash_get();
	uint8_t *update_chunk = boot_get_update_chunk();
	uint32_t packet_idx_in_chunk;	// 当前 update_chunk 内的 Xmodem 包索引（从 0 开始）
	uint32_t chunk_idx;				// update_chunk 的块索引

	/* 提取 CRC，校验数据部分 */
	uint16_t recv_crc = (data[131] << 8) | data[132];
	uint16_t crc = boot_xmodem_crc16(&data[3], XMODEM_PACKET_DATA_LEN);

	if (crc != recv_crc) {
        boot_xmodem_send_ack_nack(false);	// CRC校验错误，发送 NACK	
        return;
    }

	/* 累积接收到的包数量，从 1 开始计数 */
	boot_xmodem_ctx.xmodem_packet_cnt++;

	/* 将本 Xmodem 数据包（128 字节）拷贝到当前 update_chunk 的对应偏移位置 */
	packet_idx_in_chunk = (boot_xmodem_ctx.xmodem_packet_cnt - 1) % XMODEM_PACKETS_PER_CHUNK;
	memcpy(&update_chunk[packet_idx_in_chunk * XMODEM_PACKET_DATA_LEN], 
		   &data[3], 
		   XMODEM_PACKET_DATA_LEN);
	
	/*
	 * 若 X 个包已经填满一个 chunk（packet_cnt % PACKETS_PER_CHUNK == 0），
	 * 则可计算当前 chunk 的完整索引（从 0 开始）。
	 *
	 * 例如：收到第 8 个包（XMODEM_PACKETS_PER_CHUNK = 8）
	 *   packet_cnt = 8
	 *   chunk_idx = 8 / 8 - 1 = 0
	 *   → 第 0 个 chunk 收满，可以写 Flash
	 * 
	 * 当未收到 8 个 Xmodem 包时，chunk_idx = -1，此时无意义
	 */
	chunk_idx = (boot_xmodem_ctx.xmodem_packet_cnt / XMODEM_PACKETS_PER_CHUNK) - 1;

	/* 如果 update_chunk 填满，则写入内部或外部 Flash */
	if ((boot_xmodem_ctx.xmodem_packet_cnt % XMODEM_PACKETS_PER_CHUNK) == 0) {
		if (boot_has_flag(BOOT_FLAG_EXT_DOWNLOAD_XMODEM)) {
			boot_ext_flash_write_chunk(ext_flash, chunk_idx);
		} else {
			boot_flash_write_chunk(flash, chunk_idx);
		}
		
	}

	boot_xmodem_send_ack_nack(true);	// 接收成功，发送 ACK
}

/**
 * @brief   写入最后不足一个 update_chunk 的剩余数据
 */
static void boot_xmodem_flush_final_chunk(void)
{
	bsp_flash_t *flash = bsp_flash_get();
    bsp_ext_flash_t *ext_flash = bsp_ext_flash_get();
	uint8_t *update_chunk = boot_get_update_chunk();
	uint32_t remaining_packets = boot_xmodem_ctx.xmodem_packet_cnt % XMODEM_PACKETS_PER_CHUNK;
	uint32_t remaining_bytes = remaining_packets * XMODEM_PACKET_DATA_LEN;

	/*
	 * chunk_idx 表示之前已经写满的 update_chunk 数量（索引从 0 开始）
	 * 例： 收到 10 个包，8 个包写满第 0 个 chunk，还剩 2 个包属于 chunk_idx = 1
	 */
	uint32_t chunk_idx = (boot_xmodem_ctx.xmodem_packet_cnt / XMODEM_PACKETS_PER_CHUNK);

	if (!remaining_packets)
        return;

	if (boot_has_flag(BOOT_FLAG_EXT_DOWNLOAD_XMODEM)) {
		/* 外部 Flash 始终按 1KB 写，多写无影响 */
		boot_ext_flash_write_chunk(ext_flash, chunk_idx);

	} else {
		/* 内部 Flash 写有效字节，避免影响后续空间 */
		uint32_t addr = BOOT_FLASH_APP_START_ADDR + chunk_idx * BOOT_APP_UPDATE_CHUNK_SIZE;
		flash->ops->write(flash, addr, remaining_bytes, (uint32_t *)update_chunk);
	}
}

/**
 * @brief   写入内/外部 Flash 完毕，完成 Xmodem 更新流程
 */
static void boot_xmodem_finalize_update(void)
{
	boot_app_info_t boot_app_info;
	uint8_t ext_flash_slot_idx = boot_ext_flash_get_cur_slot_idx();

	boot_clear_flag(BOOT_FLAG_IAP_XMODEM_RECV_DATA);

	if (boot_has_flag(BOOT_FLAG_EXT_DOWNLOAD_XMODEM)) {
		boot_clear_flag(BOOT_FLAG_EXT_DOWNLOAD_XMODEM);

		boot_app_info_load(&boot_app_info);
		boot_app_info.app_size[ext_flash_slot_idx] = boot_xmodem_ctx.xmodem_packet_cnt * XMODEM_PACKET_DATA_LEN;
		boot_app_info_save(&boot_app_info);

		log_info("Download completed!\r\n");
		boot_cmd_print_menu();

	} else {
		log_info("IAP update completed, restart!\r\n");
		boot_system_reset();
	}
}

/**
 * @brief   Xmodem 协议接收数据
 * @param[in] data 接收数据的首地址
 * @param[in] len  接收数据的长度
 */
void boot_xmodem_recv_data(uint8_t *data, uint16_t len)
{
	if (len == XMODEM_PACKET_LEN && data[0] == XMODEM_SOH) {
		/* 接收到一个数据包 */
		boot_clear_flag(BOOT_FLAG_IAP_XMODEM_SEND_C);
		boot_xmodem_process_packet(data);	// 将接收到的数据包按照 update_chunk 容量分块写入内/外部 Flash
    
    } else if (len == 1 && data[0] == XMODEM_EOT) {
		/* 接收到 EOT，传输完成 */
		boot_xmodem_send_ack_nack(true);	// 发送 ACK
		boot_xmodem_flush_final_chunk();	// 把剩余不足一个 update_chunk 的数据写入内/外部 Flash
		boot_xmodem_finalize_update();		// 写入内/外部 Flash 完成，更新操作
	}
}

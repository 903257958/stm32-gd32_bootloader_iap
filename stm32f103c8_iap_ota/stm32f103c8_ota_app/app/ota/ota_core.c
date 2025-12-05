#include <stdio.h>
#include <string.h>
#include "bsp_delay.h"
#include "ota_config.h"
#include "ota_core.h"
#include "ota_mqtt.h"
#include "boot_store.h"
#include "log.h"

#if defined (STM32F10X_MD) || defined (STM32F10X_HD)
#include "stm32f10x.h"
#elif defined(STM32F40_41xxx) || defined(STM32F429_439xx) || defined(STM32F411xE)
#include "stm32f4xx.h"
#elif defined (GD32F10X_MD) || defined (GD32F10X_HD)
#include "gd32f10x.h"
#else
#error ota_core.h: No processor defined!
#endif

typedef struct {
    uint32_t flag;  // 标志位
} ota_ctx_t;

static ota_ctx_t g_ota_ctx = {0};

/**
 * @brief   OTA 发布 OTA 版本号
 * @details 用于对比阿里云上的版本号和当前是否一致
 * @return	0 表示成功，其他值表示失败
 */
int ota_publish_ota_version(void)
{
	char publish_data[128];
	boot_app_info_t boot_app_info;
	ota_mqtt_pkt_t pkt;
	int ret;

	/* 读取存储在 EEPROM 中的版本号信息（在 Bootloader 中设置保存） */
	ret = boot_app_info_load(&boot_app_info);
	if (ret) {
		log_error("Failed to load boot app info (err=%d)", ret);
		return ret;
	}

	/* 发布 */
	memset(publish_data, 0, sizeof(publish_data));
	sprintf(publish_data, "{\"id\":\"1\",\"params\":{\"version\":\"%s\"}}", boot_app_info.ota_version);

	ota_mqtt_build_publish_packet_aliyun(&pkt, 
										 OTA_MQTT_PUBLISH_QOS_1, 
										 OTA_ALIYUN_MQTT_TOPIC_PUBLISH_OTA_VERSION, 
										 publish_data);
	ret = ota_mqtt_publish(&pkt);
	if (ret) {
		log_error("Failed to publish OTA version (err=%d)", ret);
		return ret;
	}

	log_info("MQTT OTA version (%s) publish message sent successfully\r\n", boot_app_info.ota_version);
	return 0;
}

/**
 * @brief   OTA 请求下载分片
 * @param[in] download_info ota_download_t 结构体指针
 * @param[in] stream_id     阿里云 streamId
 * @return	0 表示成功，其他值表示失败
 */
int ota_publish_ota_request(ota_download_info_t *download_info, uint32_t stream_id)
{
	char publish_data[256];
	ota_mqtt_pkt_t pkt;
	int ret;
	uint32_t offset = download_info->slice_downloaded * BOOT_EXT_FLASH_PAGE_SIZE;

	memset(publish_data, 0, sizeof(publish_data));
	sprintf(publish_data, 
			"{\"id\":\"1\",\"params\":{\"fileInfo\":{\"streamId\":%d,\"fileId\":1},\"fileBlock\":{\"size\":%d,\"offset\":%d}}}", 
			stream_id, download_info->slice_size, offset);

	ota_mqtt_build_publish_packet_aliyun(&pkt, 
										 OTA_MQTT_PUBLISH_QOS_0, 
										 OTA_ALIYUN_MQTT_TOPIC_PUBLISH_OTA_REQUEST, 
										 publish_data);
	ret = ota_mqtt_publish(&pkt);
	if (ret) {
		log_error("Failed to publish OTA request (err=%d)", ret);
		return ret;
	}

	log_info("Current download slice: %d/%d", download_info->slice_downloaded + 1, download_info->slice_total);
	log_info("MQTT OTA request publish message sent successfully\r\n");
	bsp_delay_ms(250);	// 消息上下行 TPS 限制 5
	return 0;
}

/**
 * @brief   OTA 解析升级信息
 * @param[in]  raw  原始 JSON 数据
 * @param[out] info ota_upgrade_info_t 结构体指针
 * @return	0 表示成功，其他值表示失败
 */
int ota_parse_upgrade_info(const char *raw, ota_upgrade_info_t *info)
{
    const char *json;
    const char *p;

    if (!raw || !info)
        return -1;

    /* 定位 JSON 起始位置 */
    json = strchr(raw, '{');
    if (!json)
        return -1;

    /* 解析 size */
    p = strstr(json, "\"size\":");
    if (!p)
        return -1;
    sscanf(p, "\"size\":%d", &info->size);

    /* 解析 streamId */
    p = strstr(json, "\"streamId\":");
    if (!p)
        return -1;
    sscanf(p, "\"streamId\":%d", &info->stream_id);

    /* 解析 version */
    p = strstr(json, "\"version\":");
    if (!p)
        return -1;
    sscanf(p, "\"version\":\"%15[^\"]\"", info->version);

	log_info("OTA upgrage info:");
	log_info("  Size: %d", info->size);
	log_info("  Stream ID: %d", info->stream_id);
	log_info("  Version: %s\r\n", info->version);
    return 0;
}

/**
 * @brief   OTA 系统复位
 */
void ota_system_reset(void)
{
    bsp_delay_ms(800);
    NVIC_SystemReset();
}

/**
 * @brief   OTA 设置标志位
 * @param[in] flag 标志位
 */
void ota_set_flag(ota_flag_t flag)
{
    g_ota_ctx.flag |= flag;
}

/**
 * @brief   OTA 清除标志位
 * @param[in] flag 标志位
 */
void ota_clear_flag(ota_flag_t flag)
{
    g_ota_ctx.flag &= ~flag;
}

/**
 * @brief   OTA 是否存在某标志位
 * @param[in] flag 标志位
 * @return  true 表示标志位存在，false 表示标志位不存在
 */
bool ota_has_flag(ota_flag_t flag)
{
    return (g_ota_ctx.flag & flag) != 0;
}

/**
 * @brief   OTA 获取当前标志位
 * @return  true 表示标志位存在，false 表示标志位不存在
 */
uint32_t ota_get_flag(void)
{
    return g_ota_ctx.flag;
}

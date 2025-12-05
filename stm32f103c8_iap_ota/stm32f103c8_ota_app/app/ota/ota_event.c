#include <string.h>
#include "bsp_net.h"
#include "bsp_delay.h"
#include "bsp_ext_flash.h"
#include "ota_config.h"
#include "ota_core.h"
#include "ota_mqtt.h"
#include "boot_store.h"
#include "log.h"

typedef enum {
    OTA_EVENT_CONN_CLOSED = 0,  /* MQTT 连接断开 */
    OTA_EVENT_CONNACK,          /* MQTT CONNECT 成功 */
    OTA_EVENT_SUBACK,           /* MQTT SUBSCRIBE 成功 */
    OTA_EVENT_PUBLISH,          /* MQTT PUBLISH 接收消息 */
    OTA_EVENT_UNKNOWN
} ota_event_t;

typedef struct {
    ota_event_t event;
    bool (*matcher)(uint8_t *data, uint32_t len);
    void (*handler)(uint8_t *data, uint32_t len);
} ota_event_handler_t;

/**
 * @brief   OTA 匹配 MQTT 连接断开事件
 * @param[in] data 接收数据的首地址
 * @param[in] len  接收数据的长度
 * @return  true 表示事件发生，false 表示事件未发生
 */
static bool ota_match_conn_closed(uint8_t *data, uint32_t len)
{
    return (len == 8 && memcmp(data, "CLOSED\r\n", 8) == 0);
}

/**
 * @brief   OTA 匹配 MQTT CONNECT 成功事件
 * @param[in] data 接收数据的首地址
 * @param[in] len  接收数据的长度
 * @return  true 表示事件发生，false 表示事件未发生
 */
static bool ota_match_connack(uint8_t *data, uint32_t len)
{
    /* 
    * 如果接收到 24 字节：
    * 0d 0a 53 45 4e 44 20 4f 4b 0d 0a 0d 0a 2b 49 50 44 2c 34 3a 20 02 00 [00]
    * 
    * 或接收到 13 字节：
    * 0d 0a 2b 49 50 44 2c 34 3a 20 02 00 [00]
    *  
    * 说明发送 CONNECT 报文成功
    */
    return (((len == 24) && (data[20] == 0x20) && (data[21] == 0x02)) ||
            ((len == 13) && (data[9]  == 0x20) && (data[10] == 0x02)));
}

/**
 * @brief   OTA 匹配 MQTT SUBSCRIBE 成功事件
 * @param[in] data 接收数据的首地址
 * @param[in] len  接收数据的长度
 * @return  true 表示事件发生，false 表示事件未发生
 */
static bool ota_match_suback(uint8_t *data, uint32_t len)
{
    /* 
    * 如果接收到 25 字节：
    * 0d 0a 53 45 4e 44 20 4f 4b 0d 0a 0d 0a 2b 49 50 44 2c 35 3a 90 03 00 00 [01]
    * 
    * 或接收到 14 字节：
    * 0d 0a 2b 49 50 44 2c 35 3a 90 03 00 00 [01]
    * 说明发送 SUBSCRIBE 报文成功
    */
    return (((len == 25) && (data[20] == 0x90)) ||
            ((len == 14) && (data[9]  == 0x90)));
}

/**
 * @brief   OTA 匹配 MQTT PUBLISH 接收消息事件
 * @param[in] data 接收数据的首地址
 * @param[in] len  接收数据的长度
 * @return  true 表示事件发生，false 表示事件未发生
 */
static bool ota_match_publish(uint8_t *data, uint32_t len)
{
    /* 
    * 如果接收到大于 150 字节：
    * 0d 0a 2b 49 50 44 2c 31 35 32 3a [30] 95 01 ...
    * 
    * 或
    * 0d 0a 53 45 4e 44 20 4f 4b 0d 0a 0d 0a 2b 49 50 44 2c 34 30 35 3a [30] 92 03 ...
    * 说明接收 PUBLISH 成功（等级为 0）
    */
    // return ((ota_has_flag(OTA_FLAG_MQTT_CONNECT)) &&((len > 150) && (data[11] == 0x30)) ||
    //         (ota_has_flag(OTA_FLAG_MQTT_CONNECT)) &&((len > 150) && (data[19] == 0x30)));
    return ((ota_has_flag(OTA_FLAG_MQTT_CONNECT)) &&((len > 150)));
}

/**
 * @brief   OTA 处理 MQTT 连接断开事件
 * @details 未建立连接时会很快被服务器踢出，建立连接后只有未按时发送保活包才会被踢出
 * @param[in] data 接收数据的首地址
 * @param[in] len  接收数据的长度
 */
static void ota_handle_conn_closed(uint8_t *data, uint32_t len)
{
    bsp_net_t *net = bsp_net_get();
    int ret;

    log_info("Connection closed, attempting to reconnect to MQTT server...");
    ret = net->ops->tcp_connect(net, OTA_ALIYUN_MQTT_HOST_URL, OTA_ALIYUN_PORT);
    if (ret)
        log_error("Failed to reconnect to MQTT server (err=%d)", ret);
    else
        log_info("Reconnected to MQTT server successfully");
}

/**
 * @brief   OTA 处理 MQTT CONNECT 成功事件
 * @param[in] data 接收数据的首地址
 * @param[in] len  接收数据的长度
 */
static void ota_handle_connack(uint8_t *data, uint32_t len)
{
    if (data[12] == 0x00) {
        /* 连接成功 */
        log_info("MQTT CONNACK: connected successfully\r\n");
        ota_set_flag(OTA_FLAG_MQTT_CONNECT);

        /* 订阅 */
        ota_mqtt_pkt_t mqtt_pkt;
        ota_mqtt_build_subscribe_packet_aliyun(&mqtt_pkt, OTA_ALIYUN_MQTT_TOPIC_SUBSCRIBE);
        ota_mqtt_subscribe(&mqtt_pkt);

        /* 订阅下载回应 */
        ota_mqtt_pkt_t mqtt_pkt1;
        ota_mqtt_build_subscribe_packet_aliyun(&mqtt_pkt1, OTA_ALIYUN_MQTT_TOPIC_SUBSCRIBE_OTA_DOWNLOAD_REPLY);
        ota_mqtt_subscribe(&mqtt_pkt1);

    } else {
        log_error("Failed to establish MQTT connection!");
    }
}

/**
 * @brief   OTA 处理 MQTT SUBSCRIBE 成功事件
 * @param[in] data 接收数据的首地址
 * @param[in] len  接收数据的长度
 */
static void ota_handle_suback(uint8_t *data, uint32_t len)
{
    if ((data[len - 1] == 0x00) || (data[len - 1] == 0x01)) {
        /* 订阅成功 */
        log_info("MQTT SUBACK: subscribed successfully\r\n");

        /* 发送 OTA 版本号 */
        ota_publish_ota_version();

    } else {
        log_error("Failed to subscribe to topic!");
    }
}

/**
 * @brief   OTA 处理 LED 测试消息
 * @param[in] pkt ota_mqtt_publish_pkt_t 结构体指针
 * @return	0 表示成功，其他值表示失败
 */
static int ota_handle_publish_led_test(ota_mqtt_publish_pkt_t *pkt)
{
    if (strstr((const char *)pkt->valid_data, "{\"led_test\":1}") != NULL) {
        log_debug("LED: ON");
        ota_mqtt_pkt_t mqtt_pkt;
        const char *publish_data = "{\"params\":{\"led_test\":1}}";
        ota_mqtt_build_publish_packet_aliyun(&mqtt_pkt,
                                        OTA_MQTT_PUBLISH_QOS_0,
                                        OTA_ALIYUN_MQTT_TOPIC_PUBLISH,
                                        publish_data);
        ota_mqtt_publish(&mqtt_pkt);
        return 0;
    }

    if (strstr((const char *)pkt->valid_data, "{\"led_test\":0}") != NULL) {
        log_debug("LED: OFF");
        ota_mqtt_pkt_t mqtt_pkt;
        const char *publish_data = "{\"params\":{\"led_test\":0}}";

        ota_mqtt_build_publish_packet_aliyun(&mqtt_pkt,
                                            OTA_MQTT_PUBLISH_QOS_0,
                                            OTA_ALIYUN_MQTT_TOPIC_PUBLISH,
                                            publish_data);
        ota_mqtt_publish(&mqtt_pkt);
        return 0;
    }

    return -1;
}

/**
 * @brief   OTA 处理 OTA 升级消息
 * @param[in]  pkt           ota_mqtt_publish_pkt_t 结构体指针
 * @param[out] download_info ota_download_info_t 结构体指针
 * @param[out] upgrade_info  ota_upgrade_info_t 结构体指针
 * @return	0 表示成功，其他值表示失败
 */
static int ota_handle_publish_ota_upgrade(ota_mqtt_publish_pkt_t *pkt, 
                                        ota_download_info_t *download_info, 
                                        ota_upgrade_info_t *upgrade_info)
{
    if (strstr((const char *)pkt->valid_data, "/ota/device/upgrade/") == NULL)
        return -1;
        
    int ret = ota_parse_upgrade_info((const char *)pkt->valid_data, upgrade_info);
    if (ret) {
        log_error("Failed to parse OTA upgrade info (err=%d)", ret);
        return ret;
    }

    /* 擦除外部 Flash （0 号为固定用于 OTA） */
    bsp_ext_flash_t *ext_flash = bsp_ext_flash_get();
    log_info("Erasing the 0th firmware of external Flash...");
    for (uint8_t i = 0; i < BOOT_EXT_FLASH_APP_BLOCK_COUNT; i++) {
        ret = ext_flash->ops->erase_block(ext_flash, i);
        if (ret) {
            log_error("Failed to erase block (err=%d)", ret);
            return ret;
        }
        log_info("Erased block %d/%d", i + 1, BOOT_EXT_FLASH_APP_BLOCK_COUNT);
    }

    /* 分片下载，一次 265 字节（外部 Flash 页大小） */
    if (upgrade_info->size % BOOT_EXT_FLASH_PAGE_SIZE == 0) {
        download_info->slice_total = upgrade_info->size / BOOT_EXT_FLASH_PAGE_SIZE;
    } else {
        download_info->slice_total = upgrade_info->size / BOOT_EXT_FLASH_PAGE_SIZE + 1;
    }
    download_info->slice_downloaded = 0;                  // TODO: 封装为init函数
    download_info->slice_size = BOOT_EXT_FLASH_PAGE_SIZE; // TODO: 封装为init函数

    ota_publish_ota_request(download_info, upgrade_info->stream_id);

    return 0;
}

/**
 * @brief   OTA 处理 OTA 分片下载消息
 * @param[in] data          接收数据的首地址
 * @param[in] len           接收数据的长度
 * @param[in] pkt           ota_mqtt_publish_pkt_t 结构体指针
 * @param[in] download_info ota_download_info_t 结构体指针
 * @param[in] upgrade_info  ota_upgrade_info_t 结构体指针
 * @return	0 表示成功，其他值表示失败
 */
static int ota_handle_publish_ota_download_reply(uint8_t *data, 
                                                uint32_t len, 
                                                ota_mqtt_publish_pkt_t *pkt, 
                                                ota_download_info_t *download_info,
                                                ota_upgrade_info_t *upgrade_info)
{
    if (strstr((const char *)pkt->valid_data, "/thing/file/download_reply") == NULL)
        return -1;
    
    /* 接收到一次 OTA 分片的数据，写入外部 Flash */
    bsp_ext_flash_t *ext_flash = bsp_ext_flash_get();
    int ret;
    ret = ext_flash->ops->write_page(ext_flash, 
                                    download_info->slice_downloaded * BOOT_EXT_FLASH_PAGE_SIZE, 
                                    BOOT_EXT_FLASH_PAGE_SIZE, 
                                    &data[len - download_info->slice_size - 2]);
    if (ret) {
        log_error("Failed to write ext flash page (err=%d)", ret);
    }
    download_info->slice_downloaded++;

    if (download_info->slice_downloaded == download_info->slice_total - 1) {
        /* 还剩最后一片没下载 */
        if (upgrade_info->size % BOOT_EXT_FLASH_PAGE_SIZE == 0) {
            /* NULL */
        } else {
            download_info->slice_size = upgrade_info->size % BOOT_EXT_FLASH_PAGE_SIZE;
        }
    } else if (download_info->slice_downloaded == download_info->slice_total) {
        /* 下载完成 */
        boot_app_info_t boot_app_info;
        boot_app_info_load(&boot_app_info);
        boot_app_info.app_size[0] = upgrade_info->size;  // 0 号位固定用于 OTA
        memset(boot_app_info.ota_version, 0, BOOT_OTA_VERSION_LEN_MAX);
        memcpy(boot_app_info.ota_version, upgrade_info->version, BOOT_OTA_VERSION_LEN_MAX);
        boot_app_info.ota_flag = BOOT_OTA_FLAG;
        boot_app_info_save(&boot_app_info);

        log_info("OTA download successfully!");
        ota_system_reset();
    }
    ota_publish_ota_request(download_info, upgrade_info->stream_id);

    return 0;
}

/**
 * @brief   OTA 处理 MQTT PUBLISH 接收消息事件
 * @param[in] data 接收数据的首地址
 * @param[in] len  接收数据的长度
 */
static void ota_handle_publish(uint8_t *data, uint32_t len)
{
    log_info("MQTT PUBLISH: QoS 0 message received successfully\r\n");
        
    ota_mqtt_publish_pkt_t pkt;
    ota_download_info_t download_info;
    ota_upgrade_info_t upgrade_info;

    memset(&download_info, 0, sizeof(download_info));
    memset(&upgrade_info, 0, sizeof(upgrade_info));

    ota_mqtt_process_publish_packet_aliyun(data, len, &pkt);

    /* LED 测试消息 */
    if (ota_handle_publish_led_test(&pkt) == 0)
        return;

    /* OTA 升级消息 */
    if (ota_handle_publish_ota_upgrade(&pkt, &download_info, &upgrade_info) == 0)
        return;

    /* OTA 分片下载消息 */
    if (ota_handle_publish_ota_download_reply(data, len, &pkt, &download_info, &upgrade_info) == 0)
        return;
}

static const ota_event_handler_t ota_handlers[] = {
    { OTA_EVENT_CONN_CLOSED, ota_match_conn_closed, ota_handle_conn_closed },
    { OTA_EVENT_CONNACK,     ota_match_connack,     ota_handle_connack     },
    { OTA_EVENT_SUBACK,      ota_match_suback,      ota_handle_suback      },
    { OTA_EVENT_PUBLISH,     ota_match_publish,     ota_handle_publish     }
};

/**
 * @brief   OTA 事件处理
 * @param[in] data 接收数据的首地址
 * @param[in] len  接收数据的长度
 */
void ota_process_event(uint8_t *data, uint32_t len)
{
    for (size_t i = 0; i < sizeof(ota_handlers) / sizeof(ota_handlers[0]); i++) {
        if (ota_handlers[i].matcher(data, len)) {
            ota_handlers[i].handler(data, len);
            return;
        }
    }
}

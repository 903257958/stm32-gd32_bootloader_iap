#ifndef OTA_CORE_H
#define OTA_CORE_H

#include <stdint.h>
#include <stdbool.h>
#include "ota_mqtt.h"
#include "ota_config.h"

/* OTA 标志位 */
typedef enum {
    OTA_FLAG_MQTT_CONNECT = 0x00000001, // MQTT 连接已被服务端接受
    OTA_FLAG_OTA_EVENT    = 0x00000002  // 产生 OTA 事件
} ota_flag_t;

typedef struct {
    uint32_t size;
    uint32_t stream_id;
    char version[BOOT_OTA_VERSION_LEN_MAX];
} ota_upgrade_info_t;

typedef struct {
    uint32_t slice_total;       // 总分片下载次数
    uint32_t slice_downloaded;  // 已下载分片数量
    uint32_t slice_size;        // 单次下载量
} ota_download_info_t;

/**
 * @brief   OTA 发布 OTA 版本号
 * @details 用于对比阿里云上的版本号和当前是否一致
 * @return	0 表示成功，其他值表示失败
 */
int ota_publish_ota_version(void);

/**
 * @brief   OTA 请求下载分片
 * @param[in] download_info ota_download_info_t 结构体指针
 * @param[in] stream_id     阿里云 streamId
 * @return	0 表示成功，其他值表示失败
 */
int ota_publish_ota_request(ota_download_info_t *download_info, uint32_t stream_id);

/**
 * @brief   OTA 解析升级信息
 * @param[in]  raw  原始 JSON 数据
 * @param[out] info ota_upgrade_info_t 结构体指针
 * @return	0 表示成功，其他值表示失败
 */
int ota_parse_upgrade_info(const char *raw, ota_upgrade_info_t *info);

/**
 * @brief   OTA 系统复位
 */
void ota_system_reset(void);

/**
 * @brief   OTA 设置标志位
 * @param[in] flag 标志位
 */
void ota_set_flag(ota_flag_t flag);

/**
 * @brief   OTA 清除标志位
 * @param[in] flag 标志位
 */
void ota_clear_flag(ota_flag_t flag);

/**
 * @brief   OTA 是否存在某标志位
 * @param[in] flag 标志位
 * @return  true 表示标志位存在，false 表示标志位不存在
 */
bool ota_has_flag(ota_flag_t flag);

/**
 * @brief   OTA 获取当前标志位
 * @return  true 表示标志位存在，false 表示标志位不存在
 */
uint32_t ota_get_flag(void);

#endif

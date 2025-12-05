#ifndef OTA_MQTT_H
#define OTA_MQTT_H

#include <stdint.h>

typedef struct {
	uint8_t  buf[512]; 		// MQTT 报文缓冲区
	uint16_t message_id;    // 报文标识符（Message ID）
	uint16_t fixed_len;     // 固定头长度
	uint16_t variable_len;  // 可变头长度
	uint16_t payload_len;   // 负载长度
	uint16_t remaining_len; // 剩余长度 = 可变头 + 负载
} ota_mqtt_pkt_t;

typedef struct {
	uint8_t valid_data[512];
	uint32_t valid_len;
} ota_mqtt_publish_pkt_t;

typedef enum {
	OTA_MQTT_PUBLISH_QOS_0,
	OTA_MQTT_PUBLISH_QOS_1
} ota_mqtt_publish_qos_t;

/**
 * @brief   构建 MQTT CONNECT 报文（阿里云）
 * @details 使用固定头 + 可变头 + 负载，包含阿里云要求的 clientID、用户名和密码
 * @param[out] pkt ota_mqtt_pkt_t 结构体指针
 * @return	0 表示成功，其他值表示失败
 */
void ota_mqtt_build_connect_packet_aliyun(ota_mqtt_pkt_t *pkt);

/**
 * @brief   构建 MQTT 订阅报文（阿里云）
 * @param[out] pkt   ota_mqtt_pkt_t 结构体指针
 * @param[in]  topic Topic 字符串
 */
void ota_mqtt_build_subscribe_packet_aliyun(ota_mqtt_pkt_t *pkt, const char *topic);

/**
 * @brief   构建 MQTT PUBLISH 报文（阿里云）
 * @param[out] pkt   ota_mqtt_pkt_t 结构体指针
 * @param[in]  qos   等级
 * @param[in]  topic Topic 字符串
 * @param[in]  data  数据
 */
void ota_mqtt_build_publish_packet_aliyun(ota_mqtt_pkt_t *pkt, 
									 	  ota_mqtt_publish_qos_t qos, 
									 	  const char *topic, 
										  const char *data);

/**
 * @brief   MQTT 处理来自服务器的 PUBLISH 数据包（阿里云）
 * @param[in]  data 	   接收数据的首地址
 * @param[in]  len  	   接收数据的长度
 * @param[out] publish_pkt MQTT PUBLISH 数据包结构体指针
 * @return	0 表示成功，其他值表示失败
 */
int ota_mqtt_process_publish_packet_aliyun(uint8_t *data, uint32_t len, ota_mqtt_publish_pkt_t *publish_pkt);

/**
 * @brief   OTA MQTT 发送 PINGREQ（保活包）
 * @param[in] cycle_ms 发送周期（毫秒）
 * @return	0 表示成功，其他值表示失败
 */
int ota_mqtt_send_pingreq(uint32_t cycle_ms);

/**
 * @brief   OTA 连接 MQTT 服务器（阿里云）
 * @param[in] pkt ota_mqtt_pkt_t 结构体指针
 * @return	0 表示成功，其他值表示失败
 */
int ota_mqtt_connect(ota_mqtt_pkt_t *pkt);

/**
 * @brief   OTA MQTT 订阅（阿里云）
 * @param[in] pkt ota_mqtt_pkt_t 结构体指针
 * @return	0 表示成功，其他值表示失败
 */
int ota_mqtt_subscribe(ota_mqtt_pkt_t *pkt);

/**
 * @brief   OTA MQTT 发布（阿里云）
 * @param[in] pkt ota_mqtt_pkt_t 结构体指针
 * @return	0 表示成功，其他值表示失败
 */
int ota_mqtt_publish(ota_mqtt_pkt_t *pkt);

#endif

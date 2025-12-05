#include <string.h>
#include "bsp_net.h"
#include "bsp_delay.h"
#include "ota_config.h"
#include "ota_mqtt.h"
#include "log.h"

/**
 * @brief   构建 MQTT CONNECT 报文（阿里云）
 * @details 使用固定头 + 可变头 + 负载，包含阿里云要求的 clientID、用户名和密码
 * @param[out] pkt ota_mqtt_pkt_t 结构体指针
 * @return	0 表示成功，其他值表示失败
 */
void ota_mqtt_build_connect_packet_aliyun(ota_mqtt_pkt_t *pkt)
{
	if (!pkt)
		return;

	/* 设置报文标识符，阿里云不要求，但推荐设置 */
    pkt->message_id = 1;

	/* 固定报头的第一个字节长度为 1，对应 MQTT 的连接包标志 0x10 */
    pkt->fixed_len = 1;

	/* 可变报头长度固定 10 字节，包含协议名、版本号、连接标志等 */
    pkt->variable_len = 10;

	/* 负载部分的长度，包括每个字段前面的 2 字节长度描述符 + clientID 、用户名和密码字符串的长度 */
    pkt->payload_len = 2 + strlen(OTA_ALIYUN_DEVICE_CLIENT_ID) + 
					   2 + strlen(OTA_ALIYUN_DEVICE_USERNAME) + 
					   2 + strlen(OTA_ALIYUN_DEVICE_PASSWORD);

	/* 剩余长度字段 = 可变头 + 负载的长度 */
    pkt->remaining_len = pkt->variable_len + pkt->payload_len;
	uint32_t remaining_len = pkt->remaining_len;

    /* ==== 构建固定报头部分 ==== */
    pkt->buf[0] = 0x10;	// 0x10 是 MQTT CONNECT 报文的标识，表示客户端想建立连接
    do {
        if (remaining_len / 128 == 0) {
			/* 如果长度小于 128，直接编码 */
			pkt->buf[pkt->fixed_len] = remaining_len;
		} else {
			/* 如果大于等于 128，就进行多字节编码，每字节只用 7 位存数据，第 8 位（最高位）设为 1 表示还有后续 */
			pkt->buf[pkt->fixed_len] = (remaining_len % 128) | 0x80;
		} 
		pkt->fixed_len++;
		remaining_len = remaining_len / 128;
    } while (remaining_len);

    /* ==== 构建可变报头部分 ==== */
	pkt->buf[pkt->fixed_len + 0] = 0x00;
	pkt->buf[pkt->fixed_len + 1] = 0x04;	// 协议名长度为 4
	pkt->buf[pkt->fixed_len + 2] = 0x4D; 	// M
	pkt->buf[pkt->fixed_len + 3] = 0x51; 	// Q
	pkt->buf[pkt->fixed_len + 4] = 0x54; 	// T
	pkt->buf[pkt->fixed_len + 5] = 0x54; 	// T
	pkt->buf[pkt->fixed_len + 6] = 0x04;	// 协议版本: 0x04 表示使用 MQTT 3.1.1 版本
	pkt->buf[pkt->fixed_len + 7] = 0xC2;	// 连接标志位
	pkt->buf[pkt->fixed_len + 8] = 0x00;	// 保活时间 100 秒，客户端每 100 秒内要向服务器发一次心跳
	pkt->buf[pkt->fixed_len + 9] = 0x64; 	// 保活时间 100 秒，客户端每 100 秒内要向服务器发一次心跳
	
	/* ==== 构建负载部分 ==== */
	/* 先写 clientID 的长度（2 字节），再写 clientID 本体内容 */
	pkt->buf[pkt->fixed_len + 10] = strlen(OTA_ALIYUN_DEVICE_CLIENT_ID) / 256;
	pkt->buf[pkt->fixed_len + 11] = strlen(OTA_ALIYUN_DEVICE_CLIENT_ID) % 256;
	memcpy(&pkt->buf[pkt->fixed_len + 12], 
		   OTA_ALIYUN_DEVICE_CLIENT_ID, 
		   strlen(OTA_ALIYUN_DEVICE_CLIENT_ID));
	
	/* 添加用户名字段，包含长度和内容 */
	pkt->buf[pkt->fixed_len + 12 + strlen(OTA_ALIYUN_DEVICE_CLIENT_ID)] = strlen(OTA_ALIYUN_DEVICE_USERNAME) / 256;
	pkt->buf[pkt->fixed_len + 13 + strlen(OTA_ALIYUN_DEVICE_CLIENT_ID)] = strlen(OTA_ALIYUN_DEVICE_USERNAME) % 256;
	memcpy(&pkt->buf[pkt->fixed_len + 14 + strlen(OTA_ALIYUN_DEVICE_CLIENT_ID)], 
		   OTA_ALIYUN_DEVICE_USERNAME, 
		   strlen(OTA_ALIYUN_DEVICE_USERNAME));

	/* 添加密码字段 */
	pkt->buf[pkt->fixed_len + 14 + strlen(OTA_ALIYUN_DEVICE_CLIENT_ID) + strlen(OTA_ALIYUN_DEVICE_USERNAME)] = 
		strlen(OTA_ALIYUN_DEVICE_PASSWORD) / 256;
	pkt->buf[pkt->fixed_len + 15 + strlen(OTA_ALIYUN_DEVICE_CLIENT_ID) + strlen(OTA_ALIYUN_DEVICE_USERNAME)] = 
		strlen(OTA_ALIYUN_DEVICE_PASSWORD) % 256;
	memcpy(&pkt->buf[pkt->fixed_len + 16 + strlen(OTA_ALIYUN_DEVICE_CLIENT_ID) + strlen(OTA_ALIYUN_DEVICE_USERNAME)], 
		   OTA_ALIYUN_DEVICE_PASSWORD, 
		   strlen(OTA_ALIYUN_DEVICE_PASSWORD));
}

/**
 * @brief   构建 MQTT 订阅报文（阿里云）
 * @param[out] pkt   ota_mqtt_pkt_t 结构体指针
 * @param[in]  topic Topic 字符串
 */
void ota_mqtt_build_subscribe_packet_aliyun(ota_mqtt_pkt_t *pkt, const char *topic)
{
	if (!pkt || !topic)
		return;

    pkt->fixed_len = 1;							// 固定报头
    pkt->variable_len = 2;						// 可变报头长度固定 2 字节报文标识符
    pkt->payload_len = 2 + strlen(topic) + 1;	// 长度 + Topic + 服务质量要求
    pkt->remaining_len = pkt->variable_len + pkt->payload_len;	// 剩余长度字段 = 可变头 + 负载的长度
	uint32_t remaining_len = pkt->remaining_len;

	/* 构建固定报头部分 */
    pkt->buf[0] = 0x82;	// MQTT SUBSCRIBE 报文的标识
    do {
        if (remaining_len / 128 == 0) {
			/* 如果长度小于 128，直接编码 */
			pkt->buf[pkt->fixed_len] = remaining_len;
		} else {
			/* 如果大于等于 128，就进行多字节编码，每字节只用 7 位存数据，第 8 位（最高位）设为 1 表示还有后续 */
			pkt->buf[pkt->fixed_len] = (remaining_len % 128) | 0x80;
		} 
		pkt->fixed_len++;
		remaining_len = remaining_len / 128;
    } while (remaining_len);

	/* 构建可变报头部分 */
	pkt->buf[pkt->fixed_len + 0] = pkt->message_id / 256;
	pkt->buf[pkt->fixed_len + 1] = pkt->message_id % 256;
	pkt->message_id++;
	
	/* 构建负载部分 */
	pkt->buf[pkt->fixed_len + 2] = strlen(topic) / 256;				// 长度
	pkt->buf[pkt->fixed_len + 3] = strlen(topic) % 256;				// 长度
	memcpy(&pkt->buf[pkt->fixed_len + 4], topic, strlen(topic));	// Topic
	pkt->buf[ pkt->fixed_len + 4 + strlen(topic)] = 0x00;			// 服务质量要求
}

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
										  const char *data)
{
	if (!pkt || !topic || !data)
		return;
	
	if (qos == OTA_MQTT_PUBLISH_QOS_0) {
		pkt->fixed_len = 1;						// 固定报头
		pkt->variable_len = 2 + strlen(topic);	// Topic length + Topic 长度（无报文标识符）
		pkt->payload_len = strlen(data);		// 数据
		pkt->remaining_len = pkt->variable_len + pkt->payload_len;	// 剩余长度字段 = 可变头 + 负载的长度
		uint32_t remaining_len = pkt->remaining_len;

		/* 构建固定报头部分 */
		pkt->buf[0] = 0x30;	// MQTT PUBLISH 报文的标识（Qos0）
		do {
			if (remaining_len / 128 == 0) {
				/* 如果长度小于 128，直接编码 */
				pkt->buf[pkt->fixed_len] = remaining_len;
			} else {
				/* 如果大于等于 128，就进行多字节编码，每字节只用 7 位存数据，第 8 位（最高位）设为 1 表示还有后续 */
				pkt->buf[pkt->fixed_len] = (remaining_len % 128) | 0x80;
			} 
			pkt->fixed_len++;
			remaining_len = remaining_len / 128;
		} while (remaining_len);

		/* 构建可变报头部分 */
		pkt->buf[pkt->fixed_len + 0] = strlen(topic) / 256;
		pkt->buf[pkt->fixed_len + 1] = strlen(topic) % 256;
		memcpy(&pkt->buf[pkt->fixed_len + 2], topic, strlen(topic));

		/* 构建负载部分 */
		memcpy(&pkt->buf[pkt->fixed_len + 2 + strlen(topic)], data, strlen(data));

	} else if (qos == OTA_MQTT_PUBLISH_QOS_1) {
		pkt->fixed_len = 1;							// 固定报头
		pkt->variable_len = 2 + strlen(topic) + 2;	// Topic length + Topic 长度 + 报文标识符
		pkt->payload_len = strlen(data);			// 数据
		pkt->remaining_len = pkt->variable_len + pkt->payload_len;	// 剩余长度字段 = 可变头 + 负载的长度
		uint32_t remaining_len = pkt->remaining_len;

		/* 构建固定报头部分 */
		pkt->buf[0] = 0x32;	// MQTT PUBLISH 报文的标识（Qos1）
		do {
			if (remaining_len / 128 == 0) {
				/* 如果长度小于 128，直接编码 */
				pkt->buf[pkt->fixed_len] = remaining_len;
			} else {
				/* 如果大于等于 128，就进行多字节编码，每字节只用 7 位存数据，第 8 位（最高位）设为 1 表示还有后续 */
				pkt->buf[pkt->fixed_len] = (remaining_len % 128) | 0x80;
			} 
			pkt->fixed_len++;
			remaining_len = remaining_len / 128;
		} while (remaining_len);

		/* 构建可变报头部分 */
		pkt->buf[pkt->fixed_len + 0] = strlen(topic) / 256;
		pkt->buf[pkt->fixed_len + 1] = strlen(topic) % 256;
		memcpy(&pkt->buf[pkt->fixed_len + 2], topic, strlen(topic));
		pkt->buf[pkt->fixed_len + 2 + strlen(topic)] = pkt->message_id / 256;
		pkt->buf[pkt->fixed_len + 3 + strlen(topic)] = pkt->message_id % 256;
		pkt->message_id++;

		/* 构建负载部分 */
		memcpy(&pkt->buf[pkt->fixed_len + 4 + strlen(topic)], data, strlen(data));
	}
}

/**
 * @brief	查找 +IPD 前缀后的冒号分隔符，定位 MQTT 数据包起始位置
 * @param[in]  data         接收数据首地址
 * @param[in]  len          接收数据长度
 * @param[out] start_offset MQTT 数据包的起始偏移（输出）
 * @retval	0：找到冒号；-1：未找到/参数错误
 */
static int mqtt_parse_return_start_offset(uint8_t *data, uint32_t len, uint32_t *start_offset)
{
    if (data == NULL || len < 8 || start_offset == NULL)	// 最小前缀：\r\n+IPD,1: → 8字节
        return -1;

    /* 遍历数据，找到冒号（0x3a）的位置 */
    for (uint32_t i = 6; i < len; i++) {	// 前6字节：0d 0a 2b 49 50 44（\r\n+IPD），从第7字节开始找
        if (data[i] == 0x3a) { 				// 找到冒号分隔符
            *start_offset = i + 1; 			// 冒号下一字节是 MQTT 数据包起始
			if (*start_offset >= len)
				return -2; 					// 冒号在最后 1 字节，无 MQTT 数据

            return 0;
        }
    }

    return -1; // 未找到冒号
}

/**
 * @brief	解析 MQTT 剩余长度（遵循 MQTT 变长编码规则）
 * @details 剩余长度 = 第一个字节低 7 位值 + 第二个字节低 7 位值 × 128（因为每多一个字节，权重乘以 128）
 * 			这里只计算了偏移，没有计算具体长度
 * @param[in]  data         接收数据首地址
 * @param[in]  len          接收数据长度
 * @param[in]  start_offset MQTT 数据包起始位置
 * @param[out] rem_offset   剩余长度字段占用的字节数（1~4）
 * @retval	0：找到冒号；-1：未找到/参数错误
 */
static int mqtt_parse_return_remaining_offset(uint8_t *data, uint32_t len, 
											  uint32_t start_offset, uint8_t *rem_offset)
{
	if (data == NULL || rem_offset == NULL)
        return -1;
	
	if (start_offset >= len)	// MQTT起始位置越界
        return -2;

	uint8_t i;
    *rem_offset = 0;

	/* 最多解析4字节（MQTT协议限制） */
	for (i = 0; i < 4; i++) {
		if ((start_offset + i) >= len)
            return -3; 	// 数据不足，越界

		if ((data[start_offset + 1 + i] & 0x80) == 0) {	// +1 跳过开始后第一个字节 0x30
			break;
		}
	}

	/* 检查是否超过4字节（非法数据包） */
    if (i >= 4) {
        if ((data[start_offset + 3] & 0x80) != 0) {
            return -4;
        }
    }

	*rem_offset = i + 1;
	return 0;
}

/**
 * @brief   MQTT 处理来自服务器的 PUBLISH 数据包（阿里云）
 * @param[in]  data 	   接收数据的首地址
 * @param[in]  len  	   接收数据的长度
 * @param[out] publish_pkt MQTT PUBLISH 数据包结构体指针
 * @return	0 表示成功，其他值表示失败
 */
int ota_mqtt_process_publish_packet_aliyun(uint8_t *data, uint32_t len, ota_mqtt_publish_pkt_t *publish_pkt)
{
	/* 
	 * 示例：
	 * +IPD,152:0/sys/k0p0bzqJdfA/D001/thing/service/property/set
	 * {"method":"thing.service.property.set","id":"1413863637","params":{"led_test":0},"version":"1.0.0"}
	 * 
	 * Hex：
	 * 第一个 [30] 表示 MQTT 数据包起始位置
	 * [95 01] 表示剩余长度
	 * 第二个 [30] 表示 Qos 0
	 * 从 [2f] 开始就是 0 之后的内容
	 * 
	 * 0d 0a 2b 49 50 44 2c 31 35 32 3a [30] [95 01] 00 [30]
	 * [2f] 73 79 73 2f 6b 30 70 30 62 7a 71 4a 64 66 41
	 * 2f 44 30 30 31 2f 74 68 69 6e 67 2f 73 65 72 76
	 * 69 63 65 2f 70 72 6f 70 65 72 74 79 2f 73 65 74
	 * 7b 22 6d 65 74 68 6f 64 22 3a 22 74 68 69 6e 67
	 * 2e 73 65 72 76 69 63 65 2e 70 72 6f 70 65 72 74
	 * 79 2e 73 65 74 22 2c 22 69 64 22 3a 22 31 34 31
	 * 33 38 36 33 36 33 37 22 2c 22 70 61 72 61 6d 73
	 * 22 3a 7b 22 6c 65 64 5f 74 65 73 74 22 3a 30 7d
	 * 2c 22 76 65 72 73 69 6f 6e 22 3a 22 31 2e 30 2e
     * 30 22 7d 
	 */

	uint32_t start_offset;
	uint8_t remaining_offset;
	uint32_t valid_data_offset;
	uint32_t payload_len;
	int ret;

	ret = mqtt_parse_return_start_offset(data, len, &start_offset);
	if (ret) {
		log_error("Failed to find return start offset (err=%d)", ret);
		return -1;
	}
	// log_debug("start_offset=%d\r\n", start_offset);

	ret = mqtt_parse_return_remaining_offset(data, len, start_offset, &remaining_offset);
	if (ret) {
		log_error("Failed to prase return remaining length (err=%d)", ret);
		return -1;
	}
	// log_debug("remaining_offset=%d\r\n", remaining_offset);

	valid_data_offset = start_offset + 1 + remaining_offset + 2;	// 1 是开始后的第一个 30 ，2 是剩余长度后的 00 30
	memset(&publish_pkt->valid_data, 0, sizeof(publish_pkt->valid_data));
	// memcpy(&publish_pkt->valid_data, &data[valid_data_offset], len - valid_data_offset);	
	
	payload_len = len - valid_data_offset;
	if (payload_len > sizeof(publish_pkt->valid_data))
    	payload_len = sizeof(publish_pkt->valid_data);  // 防止溢出
	
	memcpy(publish_pkt->valid_data, &data[valid_data_offset], payload_len);	/* /sys/... 为有效数据 */
	publish_pkt->valid_len = payload_len;
	// log_debug("publish_pkt valid data: ");
	// log_debug("%s", publish_pkt->valid_data);
	// log_debug("\r\n");
	
	return 0;
}

/**
 * @brief   OTA MQTT 发送 PINGREQ（保活包）
 * @param[in] cycle_ms 发送周期（毫秒）
 * @return	0 表示成功，其他值表示失败
 */
int ota_mqtt_send_pingreq(uint32_t cycle_ms)
{
	static uint32_t timeout_ms = 0;
    uint8_t pingreq[] = { 0xC0, 0x00 };
    char cmd_buf[32];
    bsp_net_t *net = bsp_net_get();
    int ret;

	bsp_delay_ms(1);
	timeout_ms++;
	if (timeout_ms < cycle_ms)
        return 0;

	sprintf(cmd_buf, "AT+CIPSEND=%u", (unsigned int)sizeof(pingreq));
	ret = net->ops->send_at_cmd(net, cmd_buf, ">", NULL, 3000);
	if (ret) {
		log_error("Failed to enter transparent transmission mode (err=%d)", ret);
		return ret;
	}

	ret = net->ops->send_data(net, pingreq, sizeof(pingreq));
	if (ret) {
		log_error("Failed to send PINGREQ (err=%d)", ret);
		return ret;
	}

	log_info("Send PINGREQ successfully\r\n");

	timeout_ms = 0;
	return 0;
}

/**
 * @brief   OTA 连接 MQTT 服务器（阿里云）
 * @param[in] pkt ota_mqtt_pkt_t 结构体指针
 * @return	0 表示成功，其他值表示失败
 */
int ota_mqtt_connect(ota_mqtt_pkt_t *pkt)
{
	bsp_net_t *net = bsp_net_get();
	char cmd_buf[32];
	int ret;

	log_info("Start MQTT connection ...");

	/* 连接 WiFi */
	ret = net->ops->wifi_connect(net, OTA_WIFI_SSID, OTA_WIFI_PWD);
	if (ret) {
		log_error("Failed to connect WiFi (err=%d)", ret);
		return ret;
	}
	log_info("WiFi connected successfully");

	/* 建立 TCP 连接 */
	ret = net->ops->tcp_connect(net, OTA_ALIYUN_MQTT_HOST_URL, OTA_ALIYUN_PORT);
	if (ret) {
		log_error("Failed to connect tcp (err=%d)", ret);
		return ret;
	}

	/* 检查是否已经成功建立了 TCP 连接 */
	ret = net->ops->send_at_cmd(net, "AT+CIPSTATUS", "STATUS:3", NULL, 3000);
	if (ret) {
		log_error("Failed to connect tcp (err=%d)", ret);
		return ret;
	}
	log_info("TCP connected successfully");

	/* 进入透传模式 */
	sprintf(cmd_buf, "AT+CIPSEND=%u", 
		    (unsigned int)(pkt->fixed_len + pkt->variable_len + pkt->payload_len));
	ret = net->ops->send_at_cmd(net, cmd_buf, ">", NULL, 3000);
	if (ret) {
		log_error("Failed to enter transparent transmission mode (err=%d)", ret);
		return ret;
	}
	log_info("Entered transparent transmission mode successfully");

	/* 发送 MQTT CONNECT 报文 */
	ret = net->ops->send_data(net, 
							  pkt->buf, 
							  pkt->fixed_len + pkt->variable_len + pkt->payload_len);
	if (ret) {
		log_error("Failed to send MQTT connect message (err=%d)", ret);
		return ret;
	}
	log_info("MQTT connect message sent successfully\r\n");

	return 0;
}

/**
 * @brief   OTA MQTT 订阅（阿里云）
 * @param[in] pkt ota_mqtt_pkt_t 结构体指针
 * @return	0 表示成功，其他值表示失败
 */
int ota_mqtt_subscribe(ota_mqtt_pkt_t *pkt)
{
	bsp_net_t *net = bsp_net_get();
	char cmd_buf[32];
	int ret;
    
	/* 进入透传模式 */
	sprintf(cmd_buf, "AT+CIPSEND=%u", 
		    (unsigned int)(pkt->fixed_len + pkt->variable_len + pkt->payload_len));
	ret = net->ops->send_at_cmd(net, cmd_buf, ">", NULL, 3000);
	if (ret) {
		log_error("Failed to enter transparent transmission mode (err=%d)", ret);
		return ret;
	}
	log_info("Entered transparent transmission mode successfully");

	/* 发送 MQTT SUBSCRIBE 报文 */
	ret = net->ops->send_data(net, 
							  pkt->buf, 
							  pkt->fixed_len + pkt->variable_len + pkt->payload_len);
	if (ret) {
		log_error("Failed to send MQTT subscribe message (err=%d)", ret);
		return ret;
	}
	log_info("MQTT subscribe message sent successfully\r\n");

	return 0;
}

/**
 * @brief   OTA MQTT 发布（阿里云）
 * @param[in] pkt ota_mqtt_pkt_t 结构体指针
 * @return	0 表示成功，其他值表示失败
 */
int ota_mqtt_publish(ota_mqtt_pkt_t *pkt)
{
	bsp_net_t *net = bsp_net_get();
	char cmd_buf[32];
	int ret;

	/* 进入透传模式 */
	sprintf(cmd_buf, "AT+CIPSEND=%u", 
		   (unsigned int)(pkt->fixed_len + pkt->variable_len + pkt->payload_len));
	ret = net->ops->send_at_cmd(net, cmd_buf, ">", NULL, 3000);
	if (ret) {
		log_error("Failed to enter transparent transmission mode (err=%d)", ret);
		return ret;
	}
	log_info("Entered transparent transmission mode successfully");

	/* 发送 MQTT PUBLISH 报文 */
	ret = net->ops->send_data(net, 
							  pkt->buf, 
							  pkt->fixed_len + pkt->variable_len + pkt->payload_len);
	if (ret) {
		log_error("Failed to send MQTT publish message (err=%d)", ret);
		return ret;
	}
	log_info("MQTT publish message sent successfully\r\n");

	return 0;
}

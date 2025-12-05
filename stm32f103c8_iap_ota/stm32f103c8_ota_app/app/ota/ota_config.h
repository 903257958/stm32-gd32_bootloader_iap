#ifndef OTA_CONFIG_H
#define OTA_CONFIG_H

/* WiFi 信息 */
#define OTA_WIFI_SSID	"shouji"
#define OTA_WIFI_PWD	"thxd156369168"

/* 阿里云 MQTT 连接参数 */
#define OTA_ALIYUN_DEVICE_CLIENT_ID	"k0p0bzqJdfA.D001|securemode=2,signmethod=hmacsha256,timestamp=1747657362028|"
#define OTA_ALIYUN_DEVICE_USERNAME  "D001&k0p0bzqJdfA"
#define OTA_ALIYUN_DEVICE_PASSWORD  "e9979626e86a9e1735db859f4a0f0c4b887e96ba0f3714b1998b79c442fa15e5"
#define OTA_ALIYUN_MQTT_HOST_URL    "iot-06z00g5skj718oo.mqtt.iothub.aliyuncs.com"
#define OTA_ALIYUN_PORT             1883

/* 阿里云 MQTT 物模型通信 Topic */
#define OTA_ALIYUN_MQTT_TOPIC_SUBSCRIBE                     "/sys/k0p0bzqJdfA/D001/thing/service/property/set"
#define OTA_ALIYUN_MQTT_TOPIC_PUBLISH                       "/sys/k0p0bzqJdfA/D001/thing/event/property/post"
#define OTA_ALIYUN_MQTT_TOPIC_SUBSCRIBE_OTA_DOWNLOAD_REPLY  "/sys/k0p0bzqJdfA/D001/thing/file/download_reply"
#define OTA_ALIYUN_MQTT_TOPIC_PUBLISH_OTA_VERSION           "/ota/device/inform/k0p0bzqJdfA/D001"
#define OTA_ALIYUN_MQTT_TOPIC_PUBLISH_OTA_REQUEST           "/sys/k0p0bzqJdfA/D001/thing/file/download"

/* Bootloader 存储信息相关，必须与 Bootloader 中的定义大小一致 */
#define BOOT_EXT_FLASH_PAGE_SIZE        (256UL)         // 外部 Flash 页大小
#define BOOT_EXT_FLASH_APP_BLOCK_COUNT  (16UL)          // 外部 Flash 存储的每个 APP 的块数
#define BOOT_EXT_FLASH_APP_SLOT_COUNT   (10UL)          // 外部 Flash 存储的 APP 槽位数量，0 号位预留给 OTA
#define BOOT_OTA_VERSION_LEN_MAX        (20)            // 版本号最大长度
#define BOOT_OTA_FLAG                   (0xAABB1122)    // OTA 标志位，用于判断是否进行 OTA

#endif

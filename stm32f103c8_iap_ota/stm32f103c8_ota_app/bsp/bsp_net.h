#ifndef BSP_NET_H
#define BSP_NET_H

#include <stdint.h>

typedef struct bsp_net bsp_net_t;

/* 操作接口 */
typedef struct {
    int (*init)(bsp_net_t *self);
    int (*send_at_cmd)(bsp_net_t *self,
                       const char *cmd,
                       const char *expect,
                       char **out,
                       uint32_t timeout_ms);
    int (*wifi_connect)(bsp_net_t *self, const char *ssid, const char *pwd);
	int (*wifi_disconnect)(bsp_net_t *self);
    int (*tcp_connect)(bsp_net_t *self, const char *ip, uint16_t port);
	int (*tcp_disconnect)(bsp_net_t *self);
	int (*enter_transparent_mode)(bsp_net_t *self, uint32_t len);
	int (*exit_transparent_mode)(bsp_net_t *self);
    int (*send_data)(bsp_net_t *self, uint8_t *data, uint32_t len);
    int (*recv_data)(bsp_net_t *self, uint8_t **data, uint32_t *len, uint32_t timeout_ms);
} bsp_net_ops_t;

/* 设备实例结构体 */
struct bsp_net {
    const bsp_net_ops_t *ops;
    void *drv;  /* 指向底层驱动对象 */
};

/* 获取 BSP 单例对象 */
bsp_net_t *bsp_net_get(void);

#endif  /* BSP_NET_H */

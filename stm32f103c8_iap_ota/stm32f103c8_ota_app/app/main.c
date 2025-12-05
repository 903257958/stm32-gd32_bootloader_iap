#include "bsp_common.h"
#include "ota_comm.h"
#include "ota_core.h"
#include "ota_event.h"
#include "log.h"

int main(void)
{
	uint8_t *net_rx_data = NULL;
    uint32_t net_rx_len = 0;
	ota_mqtt_pkt_t mqtt_pkt;

	log_init();
    bsp_common_init();

    log_info("This is OTA APP!");
    log_info("Version: 1.0.0");

	ota_mqtt_build_connect_packet_aliyun(&mqtt_pkt);
	ota_mqtt_connect(&mqtt_pkt);
	
	while (1) {
		ota_net_recv_data(&net_rx_data, &net_rx_len);
		// ota_console_print_recv("Net", net_rx_data, net_rx_len);
		ota_process_event(net_rx_data, net_rx_len);

		ota_mqtt_send_pingreq(60000);	/* 定时发送保活包 */
	}
}

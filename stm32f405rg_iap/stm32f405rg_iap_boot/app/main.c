#include "bsp_common.h"
#include "log.h"
#include "boot_core.h"
#include "boot_comm.h"
#include "boot_cmd.h"
#include "boot_event.h"

int main(void)
{
    uint8_t *rx_data = NULL;
    uint32_t rx_len = 0;

    log_init();
    bsp_common_init();
    
    boot_process_entry();
    boot_cmd_print_menu();

	while (1) {
        boot_recv_data(&rx_data, &rx_len);
        boot_process_event(rx_data, rx_len);
	}
}

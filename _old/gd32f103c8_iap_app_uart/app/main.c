#include "main.h"

int main(void)
{
	nvic_priority_group_set(NVIC_PRIGROUP_PRE2_SUB2);

	uart0_init(921600);

	uart0_printf("This is the Flash A APP!\r\n", 10);
	
	while(1)
	{
		uart0_recv_data();
	}
}

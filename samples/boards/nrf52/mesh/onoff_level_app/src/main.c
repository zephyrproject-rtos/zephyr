#include "common.h"

void main(void)
{
	int err;

	#ifdef corvi_mesh
		corvi_pcb_init();
  	#endif

	gpio_init();

	printk("Initializing...\n");

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(bt_ready);

	if (err) 
	{
		printk("Bluetooth init failed (err %d)\n", err);
	}
}

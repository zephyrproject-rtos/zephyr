#include "common.h"

void main(void)
{
	int err;

	gpio_init();

	printk("Initializing...\n");

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(bt_ready);

	if (err) 
	{
		printk("Bluetooth init failed (err %d)\n", err);
	}

}

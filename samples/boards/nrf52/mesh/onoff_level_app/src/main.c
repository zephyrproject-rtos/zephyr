#include "common.h"

void main(void)
{
	int err;

	//*********************************************************************************
	
  	int rc = 0;

	rc = nvs_init(&fs, FLASH_DEV_NAME, STORAGE_MAGIC);

	if(rc) 
	{
		printk("Flash Init failed\n");
	}

	if(NVS_read(NVS_LED_STATE_ID, &light_state_current, sizeof(struct light_state_t)) != 0)
	{
		light_state_current.OnOff = 1;
		light_state_current.power = LEVEL_100;
	}

	printk("\n\rnvs_led_on_off -> %d, nvs_led_level -> %d\n\r", light_state_current.OnOff, light_state_current.power);
	
  	//*********************************************************************************

	gpio_init();

	update_light_state();

	printk("Initializing...\n");

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(bt_ready);

	if (err) 
	{
		printk("Bluetooth init failed (err %d)\n", err);
	}
}

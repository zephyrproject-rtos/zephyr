#include "common.h"

void main(void)
{
	int err, rc = 0;

	rc = nvs_init(&fs, FLASH_DEV_NAME, STORAGE_MAGIC);

	if (rc) {
		printk("Flash Init failed\n");
	}

	if (nvs_read(&fs, NVS_LED_STATE_ID, &light_state_current, sizeof(struct light_state_t)) < 0) {

		printk("\n\rentry for NVS_LED_STATE_ID is not found on NVS\n\r");

		light_state_current.OnOff = 1;
		light_state_current.power = LEVEL_100;
	}

	printk("\n\rled_on_off_status -> %d, led_level_status -> %d\n\r", light_state_current.OnOff, light_state_current.power);

	gpio_init();

	update_light_state();

	printk("Initializing....\n\r");

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(bt_ready);

	if (err) {
		printk("Bluetooth init failed (err %d)\n\r", err);
	}
}

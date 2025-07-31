/*
 * Private Porting
 * by David Hor - Xtooltech 2025
 * david.hor@xtooltech.com
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   K_MSEC(1000)

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

int main(void)
{
	int ret;
	bool led_state = true;

	if (!gpio_is_ready_dt(&led)) {
		printk("Error: LED device %s is not ready\n", led.port->name);
		return 0;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printk("Error %d: failed to configure LED pin\n", ret);
		return 0;
	}

	printk("LPC54S018 LED Blinky Demo\n");
	printk("LED on GPIO%d pin %d\n", led.port == DEVICE_DT_GET(DT_NODELABEL(gpio0)) ? 0 :
		   led.port == DEVICE_DT_GET(DT_NODELABEL(gpio1)) ? 1 :
		   led.port == DEVICE_DT_GET(DT_NODELABEL(gpio2)) ? 2 :
		   led.port == DEVICE_DT_GET(DT_NODELABEL(gpio3)) ? 3 : -1,
		   led.pin);

	while (1) {
		ret = gpio_pin_toggle_dt(&led);
		if (ret < 0) {
			printk("Error %d: failed to toggle LED\n", ret);
			return 0;
		}

		led_state = !led_state;
		printk("LED is %s\n", led_state ? "ON" : "OFF");
		k_sleep(SLEEP_TIME_MS);
	}
	return 0;
}
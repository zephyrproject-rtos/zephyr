#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

/* Aliases defined in the board's device tree */
#define LED_NODE    DT_ALIAS(led1)

#if !DT_NODE_HAS_STATUS(LED_NODE, okay)
#error "Unsupported board: led1 devicetree alias is not defined"
#endif

LOG_MODULE_REGISTER(main_CM4);

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);

int main(void)
{
	int ret;
	bool led_state = false;

	LOG_INF("Hello from CM4 - Dual Core Sample");

	/* Initialize LED1 */
	if (!device_is_ready(led.port)) {
		LOG_ERR("LED1 device not ready");
		return 0;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure LED1");
		return 0;
	}

	LOG_INF("CM4 initialized - LED1 will blink continuously");

	/* Blink LED1 continuously */
	while (1) {
		led_state = !led_state;
		gpio_pin_set_dt(&led, led_state);
		LOG_INF("LED1 blinking - %s", led_state ? "ON" : "OFF");
		k_sleep(K_MSEC(1000));
	}

	return 0;
}

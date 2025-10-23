#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

/* Aliases defined in the board's device tree */
#define LED_NODE    DT_ALIAS(led0)
#define BUTTON_NODE DT_ALIAS(sw0)

#if !DT_NODE_HAS_STATUS(LED_NODE, okay)
#error "Unsupported board: led0 devicetree alias is not defined"
#endif

#if !DT_NODE_HAS_STATUS(BUTTON_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif

LOG_MODULE_REGISTER(main_CM0);

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(BUTTON_NODE, gpios);

static struct gpio_callback button_cb_data;

/* Interrupt callback for button press */
static void button_pressed(const struct device *dev, struct gpio_callback *cb,
			   uint32_t pins)
{
	static bool led_on;

	led_on = !led_on;
	gpio_pin_set_dt(&led, led_on);
	LOG_INF("Button pressed - LED0 toggled to %s", led_on ? "ON" : "OFF");
}

int main(void)
{
	int ret;

	LOG_INF("Hello from CM0+ - Dual Core Sample");

	/* Initialize LED0 */
	if (!device_is_ready(led.port)) {
		LOG_ERR("LED0 device not ready");
		return 0;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure LED0");
		return 0;
	}

	/* Blink LED0 5 times to indicate CM0+ is running */
	for (int i = 0; i < 5; i++) {
		gpio_pin_toggle_dt(&led);
		k_sleep(K_MSEC(500));
	}

	/* Initialize button */
	if (!device_is_ready(button.port)) {
		LOG_ERR("Button device not ready");
		return 0;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure button");
		return 0;
	}

	/* Configure interrupt on button pin */
	ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure button interrupt");
		return 0;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);

	LOG_INF("CM0+ initialized - Button press will toggle LED0");

	/* Wait 3 seconds then start CM4 core */
	k_sleep(K_SECONDS(3));

	LOG_INF("Starting CM4 core...");
	Cy_SysEnableCM4(0x10080000);

	LOG_INF("CM4 core started - CM0+ now handling button/LED0");

	while (1) {
		k_sleep(K_FOREVER);
	}

	return 0;
}

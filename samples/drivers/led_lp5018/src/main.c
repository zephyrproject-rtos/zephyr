/*
 * Copyright (c) 2023 Kistler Instrumente AG
 * based on led_lp503x example
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/dt-bindings/led/led.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app, CONFIG_LOG_DEFAULT_LEVEL);

#include <zephyr/drivers/led/lp50xx.h>

#define INDEX_RED 0
#define INDEX_GREEN 1
#define INDEX_BLUE 2
#define NUM_SINGLE_LEDS 4
#define TIMEBASE_MS 250
#define PERIOD_MS (LP5018_24_MAX_LEDS * TIMEBASE_MS)
#define BLINK_1HZ 500
#define BLINK_3HZ 167
#define TEST_BRIGHTNESS_PERCENT 10
#ifdef CONFIG_LP50XX_ALLOW_SOFTWARE_BLINK
#define RESOLUTION_MS CONFIG_LP50XX_TIMER_RESOLUTION_MS
#else
#define RESOLUTION_MS 1
#endif
BUILD_ASSERT((BLINK_3HZ / RESOLUTION_MS) >= 1, "Timer resolution does not fit blink half-period.");
#define NUM_DEFINED_COLORS 8
#define MAX_BRIGHTNESS 100
#define BRIGHTNESS_TEST_STEPS 10
#define BUTTON_TIMEOUT_IN_S 10
#define SW0_NODE DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS(SW0_NODE, okay)
#error "Unsupported board: sw0 device tree alias is not defined"
#endif

struct led_data {
	const struct device *dev;
	uint32_t led;
	bool led_on;
	uint32_t delay_on;
	uint32_t delay_off;
};

static const uint8_t rgb_colors[NUM_DEFINED_COLORS][3] = {
	{ 0xFF, 0x00, 0x00 }, /* Red    */
	{ 0x00, 0xFF, 0x00 }, /* Green  */
	{ 0x00, 0x00, 0xFF }, /* Blue   */
	{ 0xFF, 0xFF, 0xFF }, /* White  */
	{ 0xFF, 0xFF, 0x00 }, /* Yellow */
	{ 0xFF, 0x00, 0xFF }, /* Purple */
	{ 0x00, 0xFF, 0xFF }, /* Cyan   */
	{ 0xF4, 0x79, 0x20 }, /* Orange */
};

static const struct device *lp5018 = DEVICE_DT_GET_ONE(ti_lp5018);
static const struct led_info *lp5018_info[LP5018_24_MAX_LEDS];
static const struct gpio_dt_spec sw = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});
static struct k_timer tim[NUM_SINGLE_LEDS];
static struct led_data pdata[NUM_SINGLE_LEDS];

/**
 * @brief Timer stop callback.
 */
static void tim_stop(struct k_timer *timer)
{
	struct led_data	*p = (struct led_data *)k_timer_user_data_get(timer);
	int ret;

	if (!p) {
		LOG_ERR("timer %p has no user data!", timer);
		return;
	}

	ret = led_off(p->dev, p->led);
	if (ret < 0) {
		LOG_ERR("led_off(%s, %d) %d", p->dev->name, p->led, ret);
	}
}

/**
 * @brief Timer expire callback.
 */
static void tim_toggle_led(struct k_timer *timer)
{
	struct led_data	*p = (struct led_data *)k_timer_user_data_get(timer);
	int ret;

	if (!p) {
		LOG_ERR("timer %p has no user data!", timer);
		return;
	}

	if (p->led_on) {
		ret = led_off(p->dev, p->led);
		if (ret < 0) {
			LOG_ERR("led_off(%s, %d) %d", p->dev->name, p->led, ret);
			return;
		}
		k_timer_start(timer, K_MSEC(p->delay_off), K_NO_WAIT);
		p->led_on = false;
	} else {
		ret = led_on(p->dev, p->led);
		if (ret < 0) {
			LOG_ERR("led_on(%s, %d) %d", p->dev->name, p->led, ret);
			return;
		}
		k_timer_start(timer, K_MSEC(p->delay_on), K_NO_WAIT);
		p->led_on = true;
	}
}

/**
 * @brief Setup GPIO LEDs defined in DT
 *
 * @return 0 on success, negative on error
 */
static int init_gpio_led_timer(void)
{
	const struct device *gpio_leds = DEVICE_DT_GET_ANY(gpio_leds);
	int ret;

	if (!device_is_ready(gpio_leds)) {
		return -ENODEV;
	}

	for (int led = 0; led < NUM_SINGLE_LEDS; led++) {
		ret = led_off(gpio_leds, led);
		if (ret < 0) {
			LOG_ERR("led_off(%s, %d) %d", gpio_leds->name, led, ret);
			continue;
		}

		k_timer_init(&tim[led], tim_toggle_led, tim_stop);
		pdata[led].dev = gpio_leds;
		pdata[led].led = led;
		pdata[led].led_on = false;
		pdata[led].delay_on = TIMEBASE_MS * (led+1);
		pdata[led].delay_off = PERIOD_MS - pdata[led].delay_on;
		k_timer_user_data_set(&tim[led], &pdata[led]);
	}

	return 0;
}

/**
 * @brief Wait for button to be pressed or timeout.
 *
 * @param timeout_seconds timeout in seconds
 */
static void wait_for_button_press(uint32_t timeout_seconds)
{
	int64_t start = k_uptime_get();
	int64_t end = start + timeout_seconds * 1000;

	while (gpio_pin_get(sw.port, sw.pin)) {
		/* wait for button release */
		if (k_uptime_get() >= end) {
			LOG_WRN("Button timeout (%d s).", timeout_seconds);
			return;
		}
		k_msleep(50);
	};

	while (!gpio_pin_get(sw.port, sw.pin)) {
		if (k_uptime_get() >= end) {
			LOG_WRN("Button timeout (%d s).", timeout_seconds);
			return;
		}
		k_msleep(50);
	}
}

/**
 * @brief Get LP5018 info from DT and display.
 *
 * @return 0 on success, negative on error
 */
static int init_led_controller(void)
{
	int led;

	if (!device_is_ready(lp5018)) {
		LOG_ERR("Could not get led controller");
		return -ENODEV;
	}

	for (led = 0; led < LP5018_24_MAX_LEDS; led++) {
		if (led_get_info(lp5018, led, &lp5018_info[led])) {
			LOG_INF("LED %d not present", led);
		} else {
			/* Display LED information */
			LOG_INF("Found LED %d", led);
			if (lp5018_info[led]->label) {
				LOG_INF(" - %s", lp5018_info[led]->label);
			}

			LOG_INF(" - index: %d", lp5018_info[led]->index);
			LOG_INF(" - %d colors", lp5018_info[led]->num_colors);
			if (lp5018_info[led]->color_mapping) {
				LOG_INF(" - %d:%d:%d", lp5018_info[led]->color_mapping[0],
					lp5018_info[led]->color_mapping[1],
					lp5018_info[led]->color_mapping[2]);
			}
		}
	}

	return 0;
}

/**
 * @brief Test handling of hardware color mapping (RGB, BGR, GBR)
 *
 * @param color_index index of color in rgb_color array
 * @return 0 on success, negative on hard error, positive number of errors
 */
static int test_color_mapping(int color_index)
{
	uint8_t color[] = {0x00, 0x00, 0x00};
	int errors = 0;
	int ret;

	for (int led = 0; led < LP5018_24_MAX_LEDS; led++) {
		if (!lp5018_info[led]) {
			continue;
		}

		ret = led_off(lp5018, led);
		if (ret) {
			LOG_ERR("Failed to disable LED %d! (%d)", led, ret);
			errors++;
		}

		for (int i = 0; i < LP50XX_COLORS_PER_LED; i++) {
			switch (lp5018_info[led]->color_mapping[i]) {
			case LED_COLOR_ID_RED:
				color[i] = rgb_colors[color_index][INDEX_RED];
				break;
			case LED_COLOR_ID_GREEN:
				color[i] = rgb_colors[color_index][INDEX_GREEN];
				break;
			case LED_COLOR_ID_BLUE:
				color[i] = rgb_colors[color_index][INDEX_BLUE];
				break;
			default:
				return -EINVAL;
			}
		}

		ret = led_set_color(lp5018, led, LP50XX_COLORS_PER_LED, color);
		if (ret) {
			LOG_ERR("Failed to set color of LED %d! (%d)", led, ret);
			errors++;
		}

		ret = led_on(lp5018, led);
		if (ret) {
			LOG_ERR("Failed to enable LED %d! (%d)", led, ret);
			errors++;
		}
	}

	return errors;
}

/**
 * @brief Test handling of brightness control of LP5018 driver.
 *
 * @param steps number of steps from 0 to 100 percent
 * @return 0 on success, positive number of errors
 */
static int test_brightness_control(uint8_t steps)
{
	uint8_t color[] = {0x00, 0x00, 0x00};
	int errors = 0;
	int led;
	int ret;
	int i;

	for (led = 0; led < LP5018_24_MAX_LEDS; led++) {
		if (!lp5018_info[led]) {
			continue;
		}

		for (i = 0; i < LP50XX_COLORS_PER_LED; i++) {
			switch (lp5018_info[led]->color_mapping[i]) {
			case LED_COLOR_ID_RED:
				color[i] = rgb_colors[led][INDEX_RED];
				break;
			case LED_COLOR_ID_GREEN:
				color[i] = rgb_colors[led][INDEX_GREEN];
				break;
			case LED_COLOR_ID_BLUE:
				color[i] = rgb_colors[led][INDEX_BLUE];
				break;
			default:
				return -EINVAL;
			}
		}

		ret = led_set_color(lp5018, led, LP50XX_COLORS_PER_LED,
				    color);
		if (ret) {
			LOG_ERR("Failed to set color of LED %d! (%d)", led,
				ret);
			errors++;
		}
	}

	for (i = 0; i <= MAX_BRIGHTNESS; i += (int)(MAX_BRIGHTNESS / steps)) {
		for (led = 0; led < LP5018_24_MAX_LEDS; led++) {
			if (!lp5018_info[led]) {
				continue;
			}

			if (led_set_brightness(lp5018, led, i)) {
				LOG_ERR("Failed to set brightness of LED %d!",
					led);
				errors++;
			}


		}
		LOG_INF("LED brightness at %d %%", i);
		wait_for_button_press(BUTTON_TIMEOUT_IN_S);
	}

	for (led = 0; led < LP5018_24_MAX_LEDS; led++) {
		if (!lp5018_info[led]) {
			continue;
		}

		ret = led_off(lp5018, led);
		if (ret) {
			LOG_ERR("Failed to disable LED %d! (%d)", led, ret);
			errors++;
		}
	}

	return errors;
}

/**
 * @brief Test software blink of LP5018 driver.
 *
 * @return 0 on success, positive number of errors
 */
static int test_software_blink(void)
{
	uint32_t prime[] = {719, 811, 919, 997, 1117, 773, 557, 1039,
			    3191, 2137, 2153, 1997, 3313, 3833, 1319, 733};
	/* reduce brightness of white */
	const uint8_t c[] = {0x80, 0x80, 0x80};
	int errors = 0;
	int led;
	int ret;

	for (led = 0; led < LP5018_24_MAX_LEDS; led++) {
		if (!lp5018_info[led]) {
			continue;
		}

		if (led_set_color(lp5018, led, LP50XX_COLORS_PER_LED, c)) {
			LOG_ERR("Failed to set color of LED %d!", led);
			errors++;
		}

		if (led_set_brightness(lp5018, led, TEST_BRIGHTNESS_PERCENT)) {
			LOG_ERR("Failed to set brightness of LED %d!", led);
			errors++;
		}

		ret = led_blink(lp5018, led, TIMEBASE_MS * (led+1),
				PERIOD_MS - (TIMEBASE_MS * (led+1)));
		if (ret) {
			LOG_ERR("Failed to blink LED %d! (%d)", led, ret);
			errors++;
		} else {
			LOG_INF("Blink LED %d: on=%d off=%d", led,
				TIMEBASE_MS * (led+1),
				PERIOD_MS - (TIMEBASE_MS * (led+1)));
		}
	}

	LOG_INF("LEDs should blink with same period...");
	wait_for_button_press(BUTTON_TIMEOUT_IN_S);
	LOG_INF("LEDs blink with arbitrary periods...");
	for (led = 0; led < LP5018_24_MAX_LEDS; led++) {
		if (!lp5018_info[led]) {
			continue;
		}

		if ((prime[led] / RESOLUTION_MS) < 1 ||
		    (prime[led + LP5018_24_MAX_LEDS] / RESOLUTION_MS) < 1) {
			LOG_WRN("Timer resolution does not fit half-period.");
		}

		ret = led_blink(lp5018, led, prime[led], prime[led + LP5018_24_MAX_LEDS]);
		if (ret) {
			LOG_ERR("Failed to blink LED %d! (%d)", led, ret);
			errors++;
		} else {
			LOG_INF("Blink LED %d: on=%d off=%d", led, prime[led],
				prime[led + LP5018_24_MAX_LEDS]);
		}
	}

	wait_for_button_press(BUTTON_TIMEOUT_IN_S);
	LOG_INF("Binary count with LEDs...");
	BUILD_ASSERT(((PERIOD_MS / (1<<(LP5018_24_MAX_LEDS-1))) /
		      RESOLUTION_MS) >= 1,
		     "Timer resolution does not fit blink half-period.");
	for (led = 0; led < LP5018_24_MAX_LEDS; led++) {
		if (!lp5018_info[led]) {
			continue;
		}

		ret = led_blink(lp5018, led, PERIOD_MS / (1<<led), PERIOD_MS / (1<<led));
		if (ret) {
			LOG_ERR("Failed to blink LED %d! (%d)", led, ret);
			errors++;
		} else {
			LOG_INF("Blink LED %d: on=%d off=%d", led, PERIOD_MS / (1<<led),
				PERIOD_MS / (1<<led));
		}
	}

	wait_for_button_press(BUTTON_TIMEOUT_IN_S);
	LOG_INF("Stop blinking and all LED on.");
	for (led = 0; led < LP5018_24_MAX_LEDS; led++) {
		if (!lp5018_info[led]) {
			continue;
		}

		ret = led_blink(lp5018, led, RESOLUTION_MS, 0);
		if (ret) {
			LOG_ERR("Failed to enable LED %d! (%d)", led, ret);
			errors++;
		}
	}

	wait_for_button_press(BUTTON_TIMEOUT_IN_S);
	LOG_INF("All LEDs off.");
	for (led = 0; led < LP5018_24_MAX_LEDS; led++) {
		if (!lp5018_info[led]) {
			continue;
		}

		ret = led_blink(lp5018, led, 0, RESOLUTION_MS);
		if (ret) {
			LOG_ERR("Failed to disable LED %d! (%d)", led, ret);
			errors++;
		}
	}

	LOG_INF("LEDs sync test, press to continue...");
	LOG_INF("running...");
	wait_for_button_press(BUTTON_TIMEOUT_IN_S);
	for (led = 0; led < LP5018_24_MAX_LEDS; led++) {
		if (!lp5018_info[led]) {
			continue;
		}

		/* 1 Hz */
		ret = led_blink(lp5018, led, BLINK_1HZ, BLINK_1HZ);
		if (ret) {
			LOG_ERR("Failed to blink LED %d! (%d)", led, ret);
			errors++;
		} else {
			LOG_INF("Blink LED %d: on=%d off=%d", led, BLINK_1HZ, BLINK_1HZ);
		}

		k_sleep(K_MSEC(2999));
	}

	for (led = 0; led < LP5018_24_MAX_LEDS; led++) {
		if (!lp5018_info[led]) {
			continue;
		}

		/* 3 Hz */
		ret = led_blink(lp5018, led, BLINK_3HZ, BLINK_3HZ);
		if (ret) {
			LOG_ERR("Failed to blink LED %d! (%d)", led, ret);
			errors++;
		} else {
			LOG_INF("Blink LED %d: on=%d off=%d", led, BLINK_3HZ, BLINK_3HZ);
		}

		k_sleep(K_MSEC(2003));
	}

	LOG_INF("LEDs sync test, done.");
	wait_for_button_press(BUTTON_TIMEOUT_IN_S / BUTTON_TIMEOUT_IN_S);
	LOG_INF("All LEDs off.");
	for (led = 0; led < LP5018_24_MAX_LEDS; led++) {
		if (!lp5018_info[led]) {
			continue;
		}

		ret = led_blink(lp5018, led, 0, 1);
		if (ret) {
			LOG_ERR("Failed to disable LED %d! (%d)", led, ret);
			errors++;
		}
	}

	return errors;
}

/**
 * @brief LED channel API write all colors
 *
 * @param values RGB color values (color mapping ordered)
 * @return 0 on success, 1 on error
 */
static int write_color_channel(uint8_t *values)
{
	int err;

	err = led_write_channels(lp5018, LP5018_24_LED_COL1_CHAN(0),
				 LP50XX_COLORS_PER_LED * LP5018_24_MAX_LEDS, values);
	if (err < 0) {
		LOG_ERR("Failed to write channels, start=%d num=%d err=%d",
			LP5018_24_LED_COL1_CHAN(0), LP50XX_COLORS_PER_LED * LP5018_24_MAX_LEDS,
			err);
	}

	return err < 0 ? 1 : 0;
}

/**
 * @brief LED channel API write all brightnesses
 *
 * @param values brightness values per LED
 * @return 0 on success, 1 on error
 */
static int write_brightness_channel(uint8_t *values)
{
	int err;

	err = led_write_channels(lp5018, LP50XX_LED_BRIGHT_CHAN(0), LP5018_24_MAX_LEDS, values);
	if (err < 0) {
		LOG_ERR("Failed to write channels, start=%d num=%d err=%d",
			LP50XX_LED_BRIGHT_CHAN(0), LP5018_24_MAX_LEDS, err);
	}

	return err < 0 ? 1 : 0;
}

/**
 * @brief Run tests on a all the LEDs using the channel-based API syscalls.
 */
static int run_channel_test(void)
{
	uint8_t buffer[LP50XX_COLORS_PER_LED * LP5018_24_MAX_LEDS];
	int errors = 0;
	int led;
	int i;

	/* loop through colors */
	for (led = 0; led < LP5018_24_MAX_LEDS; led++) {
		uint8_t *color = &buffer[led * 3];
		int idx = led % NUM_DEFINED_COLORS;

		if (!lp5018_info[led]) {
			continue;
		}

		for (i = 0; i < LP50XX_COLORS_PER_LED; i++) {
			switch (lp5018_info[led]->color_mapping[i]) {
			case LED_COLOR_ID_RED:
				color[i] = rgb_colors[idx][INDEX_RED];
				break;
			case LED_COLOR_ID_GREEN:
				color[i] = rgb_colors[idx][INDEX_GREEN];
				break;
			case LED_COLOR_ID_BLUE:
				color[i] = rgb_colors[idx][INDEX_BLUE];
				break;
			default:
				return -EINVAL;
			}
		}
	}

	errors += write_color_channel(buffer);

	/* Turn LEDs on (10%). */
	for (led = 0 ; led < LP5018_24_MAX_LEDS; led++) {
		buffer[led] = TEST_BRIGHTNESS_PERCENT;
	}

	errors += write_brightness_channel(buffer);

	LOG_INF("Color order: Red, Green, Blue, White, Yellow, Purple");
	wait_for_button_press(BUTTON_TIMEOUT_IN_S);

	/* Set all LEDs to white. */
	for (i = 0; i < sizeof(buffer); i++) {
		buffer[i] = 0xFF;
	}

	errors += write_color_channel(buffer);

	/* Turn LEDs on increasing brightness. */
	for (led = 0; led < LP5018_24_MAX_LEDS; led++) {
		buffer[led] = 100 / LP5018_24_MAX_LEDS * (led + 1);
	}

	errors += write_brightness_channel(buffer);

	LOG_INF("White brightness: %d %%, %d %%, ...", buffer[0], buffer[1]);
	wait_for_button_press(BUTTON_TIMEOUT_IN_S);

	/* Turn LEDs off. */
	for (led = 0; led < LP5018_24_MAX_LEDS; led++) {
		buffer[led] = 0;
	}

	errors += write_brightness_channel(buffer);

	LOG_INF("All LEDs off.");

	return errors;
}

/**
 * @brief Run implemented test for LP5018 driver.
 *
 * @return 0 on success, positive number of errors
 */
static int run_tests(void)
{
	int errors = 0;
	int ret;
	int i;

	LOG_INF("Test color mapping: all LEDs should have the same color.");
	LOG_INF("Press button to start and for next color.");
	for (i = 0; i < NUM_DEFINED_COLORS; i++) {
		wait_for_button_press(BUTTON_TIMEOUT_IN_S);
		ret = test_color_mapping(i);
		if (ret < 0) {
			LOG_ERR("Unexpected error in color mapping! (%d)", ret);
			errors++;
		} else {
			errors += ret;
		}
	}

	LOG_INF("Test brightness control, press button to increase.");
	wait_for_button_press(BUTTON_TIMEOUT_IN_S);
	errors += test_brightness_control(BRIGHTNESS_TEST_STEPS);

	if (IS_ENABLED(CONFIG_LP50XX_ALLOW_SOFTWARE_BLINK)) {
		LOG_INF("Test blink, press button to continue.");
		wait_for_button_press(BUTTON_TIMEOUT_IN_S);
		errors += test_software_blink();
	}

	LOG_INF("Test channel API, press button to continue.");
	wait_for_button_press(BUTTON_TIMEOUT_IN_S);
	errors += run_channel_test();

	return errors;
}

/**
 * @brief Let the GPIO LEDs blink with timer and run tests on LP5018 driver.
 */
void main(void)
{
	int ret;
	int i;

	LOG_INF("Running LP5018 test bench...");

	if (!device_is_ready(sw.port)) {
		LOG_ERR("Error: button device %s is not ready", sw.port->name);
		return;
	}

	ret = gpio_pin_configure_dt(&sw, GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure %s pin %d", ret, sw.port->name, sw.pin);
		return;
	}

	if (!init_gpio_led_timer()) {
		for (i = 0; i < NUM_SINGLE_LEDS; i++) {
			k_timer_start(&tim[i], K_NO_WAIT, K_NO_WAIT);
		}
	}

	if (init_led_controller()) {
		/* just blink if init_gpio_led_timer() did not fail */
		wait_for_button_press(BUTTON_TIMEOUT_IN_S);
		goto exit;
	}

	ret = run_tests();
	LOG_INF("%s test had %d error(s).", lp5018->name, ret);

exit:
	for (i = 0; i < NUM_SINGLE_LEDS; i++) {
		k_timer_stop(&tim[i]);
	}

	LOG_INF("LP5018 test bench done.");
}

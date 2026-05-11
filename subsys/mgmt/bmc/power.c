/*
 * SPDX-FileCopyrightText: © 2025-2026 Tenstorrent USA, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell.h>
#include <stdlib.h>
#include <ctype.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bmc_power);

#include "config.h"

#define GPIO_POWER_1 DT_ALIAS(power_gpio_1)
#define GPIO_POWER_2 DT_ALIAS(power_gpio_2)
#define GPIO_RESET DT_ALIAS(reset_gpio_1)
#define STATUS_LED DT_ALIAS(status_led)

static const struct gpio_dt_spec power_gpios[] = {
#if DT_NODE_HAS_STATUS_OKAY(GPIO_POWER_1)
	GPIO_DT_SPEC_GET(GPIO_POWER_1, gpios),
#endif
#if DT_NODE_HAS_STATUS_OKAY(GPIO_POWER_2)
	GPIO_DT_SPEC_GET(GPIO_POWER_2, gpios),
#endif
};

static bool system_power_state = false;

bool power_get_state(void)
{
	return system_power_state;
}

static int power_on(void)
{
	int i, ret;

	for (i = 0; i < ARRAY_SIZE(power_gpios); i++) {
		ret = gpio_pin_set_dt(&power_gpios[i], 1);
		if (ret < 0) {
			LOG_INF("Could not toggle power GPIO %d\n", i);
			return -1;
		}
	}

	system_power_state = true;

	return 0;
}

static int power_off(void)
{
	int i, ret;

	for (i = 0; i < ARRAY_SIZE(power_gpios); i++) {
		ret = gpio_pin_set_dt(&power_gpios[i], 0);
		if (ret < 0) {
			LOG_INF("Could not toggle power GPIO %d\n", i);
			return -1;
		}
	}

	system_power_state = false;

	return 0;
}

void power_set_state(bool on)
{
	int ret;

	if (on)
		ret = power_on();
	else
		ret = power_off();

	if (ret < 0) {
		LOG_ERR("Failed to set power state: %d", ret);
		return;
	}

	LOG_INF("System Power State changed to: %s", power_get_state() ? "ON" : "OFF");
}

int power_init(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(power_gpios); i++) {
		if (!gpio_is_ready_dt(&power_gpios[i])) {
			LOG_INF("Power GPIO %d not ready\n", i);
			return -1;
		}

		if (gpio_pin_configure_dt(&power_gpios[i], GPIO_OUTPUT_INACTIVE) < 0) {
			LOG_INF("Could not configure power GPIO %d\n", i);
			return -1;
		}
	}

	if (config_host_auto_poweron()) {
		// Power on at BMC boot
		return power_on();
	}

	return 0;
}

static int cmd_power_on(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	return power_on();
}

static int cmd_power_off(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	return power_off();
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_power_cmds,
	SHELL_CMD(on,    NULL, "Power on.", cmd_power_on),
	SHELL_CMD(off,   NULL, "Power off.", cmd_power_off),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(power, &sub_power_cmds, "Power commands", NULL);

static const struct gpio_dt_spec gpio_reset =
#if DT_NODE_HAS_STATUS_OKAY(GPIO_RESET)
	GPIO_DT_SPEC_GET(GPIO_RESET, gpios);
#else
	{ 0 };
#endif

int reset_init(void)
{
	int ret;

	if (!DT_NODE_HAS_STATUS_OKAY(GPIO_RESET)) {
		/* No reset GPIO */
		return 0;
	}

	if (!gpio_is_ready_dt(&gpio_reset)) {
		LOG_INF("Reset GPIO not ready\n");
		return -1;
	}

	ret = gpio_pin_configure_dt(&gpio_reset, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		LOG_INF("Could not configure reset GPIO\n");
		return -1;
	}

	ret = gpio_pin_set_dt(&gpio_reset, 0);
	if (ret < 0) {
		LOG_INF("Could not toggle reset GPIO\n");
		return -1;
	}

	return 0;
}

int power_reset(void)
{
	int ret;

	if (!DT_NODE_HAS_STATUS_OKAY(GPIO_RESET)) {
		LOG_ERR("No reset GPIO");
		return -1;
	}
	ret = gpio_pin_set_dt(&gpio_reset, 1);
	if (ret < 0) {
		LOG_INF("Could not toggle RESET GPIO\n");
		return -1;
	}

	k_msleep(1000);

	ret = gpio_pin_set_dt(&gpio_reset, 0);
	if (ret < 0) {
		LOG_INF("Could not toggle RESET GPIO\n");
		return -1;
	}

	return 0;
}

static int cmd_reset(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	return power_reset();
}

SHELL_CMD_REGISTER(reset, NULL, "Reset.", cmd_reset);

#if DT_NODE_HAS_STATUS_OKAY(STATUS_LED)
static const struct gpio_dt_spec status_led = GPIO_DT_SPEC_GET(STATUS_LED, gpios);

/* LED blinking thread */
#define LED_BLINK_STACK_SIZE 256
#define LED_BLINK_PRIORITY   (CONFIG_NUM_PREEMPT_PRIORITIES - 1)
#define LED_BLINK_PERIOD_DOT    250
#define LED_BLINK_PERIOD_DASH   (3 * LED_BLINK_PERIOD_DOT)
#define LED_BLINK_PERIOD_PAUSE  (LED_BLINK_PERIOD_DOT)
#define LED_BLINK_PERIOD_LETTER (3 * LED_BLINK_PERIOD_DOT)
#define LED_BLINK_PERIOD_WORD   (7 * LED_BLINK_PERIOD_DOT)

static K_KERNEL_STACK_DEFINE(led_blink_stack, LED_BLINK_STACK_SIZE);
static struct k_thread led_blink_thread_data;

static void led_dash(void)
{
	gpio_pin_set_dt(&status_led, 1);
	k_msleep(LED_BLINK_PERIOD_DASH);
	gpio_pin_set_dt(&status_led, 0);
	k_msleep(LED_BLINK_PERIOD_PAUSE);
}

static void led_dot(void)
{
	gpio_pin_set_dt(&status_led, 1);
	k_msleep(LED_BLINK_PERIOD_DOT);
	gpio_pin_set_dt(&status_led, 0);
	k_msleep(LED_BLINK_PERIOD_PAUSE);
}

static void led_blink_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	if (!gpio_is_ready_dt(&status_led)) {
		return;
	}

	// Flash OK in morse code: - - -   - . -
	while (1) {
		// O
		led_dash();
		led_dash();
		led_dash();

		k_msleep(LED_BLINK_PERIOD_LETTER);

		// K
		led_dash();
		led_dot();
		led_dash();

		// Pause
		k_msleep(LED_BLINK_PERIOD_WORD);
	}
}

int status_led_init(void)
{
	int ret;

	/* Initialize status LED */
	if (gpio_is_ready_dt(&status_led)) {
		ret = gpio_pin_configure_dt(&status_led, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_INF("Could not configure LED1 GPIO\n");
			return -1;
		}
	} else {
		LOG_INF("LED1 GPIO not ready\n");
		return -1;
	}

	k_thread_create(&led_blink_thread_data, led_blink_stack,
			K_THREAD_STACK_SIZEOF(led_blink_stack),
			led_blink_thread,
			NULL, NULL, NULL,
			LED_BLINK_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&led_blink_thread_data, "led_blink");

	return 0;
}
#else /* DT_NODE_HAS_STATUS_OKAY(STATUS_LED) */
int status_led_init(void)
{
	return 0;
}
#endif /* DT_NODE_HAS_STATUS_OKAY(STATUS_LED) */

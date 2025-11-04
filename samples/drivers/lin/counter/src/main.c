/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/lin.h>
#include <zephyr/drivers/gpio.h>

#if defined(CONFIG_SAMPLE_LIN_BUS_BAUDRATE_2400)
#define LIN_BUS_BAUDRATE 2400
#elif defined(CONFIG_SAMPLE_LIN_BUS_BAUDRATE_9600)
#define LIN_BUS_BAUDRATE 9600
#elif defined(CONFIG_SAMPLE_LIN_BUS_BAUDRATE_10400)
#define LIN_BUS_BAUDRATE 10400
#elif defined(CONFIG_SAMPLE_LIN_BUS_BAUDRATE_19200)
#define LIN_BUS_BAUDRATE 19200
#else
#error "Invalid baudrate setting"
#endif

#if defined(CONFIG_SAMPLE_LIN_BUS_CLASSIC_CHECKSUM)
#define LIN_CHECKSUM_TYPE LIN_CHECKSUM_CLASSIC
#elif defined(CONFIG_SAMPLE_LIN_BUS_ENHANCED_CHECKSUM)
#define LIN_CHECKSUM_TYPE LIN_CHECKSUM_ENHANCED
#else
#error "Invalid checksum type setting"
#endif

/* Test configuration */
#define COUNTER_UPDATE_FRAME_ID    CONFIG_SAMPLE_COUNTER_UPDATE_ID
#define COUNTER_UPDATE_INTERVAL_MS CONFIG_SAMPLE_COUNTER_UPDATE_PERIOD_MS

/* LED blink duration */
#define LED_BLINK_DURATION_MS 100

/* Recommended responder node framing configuration as mentioned in the LIN 2.1 specification */
#define LIN_BUS_BREAK_LEN           11
#define LIN_BUS_BREAK_DELIMITER_LEN 1

static const struct device *lin_dut = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(dut));
static struct gpio_dt_spec led = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led0), gpios, {0});

/* LIN DUT configuration */
static const struct lin_config dut_config = {
	.mode = LIN_MODE_RESPONDER,
	.baudrate = LIN_BUS_BAUDRATE,
	.break_len = LIN_BUS_BREAK_LEN,
	.break_delimiter_len = LIN_BUS_BREAK_DELIMITER_LEN,
	.flags = 0,
};

static uint32_t counter;

static void led_blink_work_handler(struct k_work *work)
{
	gpio_pin_set_dt(&led, 0);
}

K_WORK_DELAYABLE_DEFINE(led_blink_work, led_blink_work_handler);

void lin_dut_event_handler(const struct device *dev, const struct lin_event *event, void *user_data)
{
	int ret;

	if (event->type == LIN_EVT_RX_HEADER) {
		struct lin_msg msg = {
			.data_len = sizeof(uint32_t),
			.checksum_type = LIN_CHECKSUM_TYPE,
			.id = lin_get_frame_id(event->header.pid),
			.data = &counter,
			.flags = 0,
		};

		if (msg.id == COUNTER_UPDATE_FRAME_ID) {
			/* Send response with the current counter value */
			printk("Counter Update Frame has been transmitted\n");
			ret = lin_response(dev, &msg, K_FOREVER);
			if (ret < 0) {
				printk("Error: Failed to send Counter Update response\n");
				return;
			}
		} else {
			/* Do nothing for other IDs */
			return;
		}

		if (led.port != NULL) {
			/* Blink LED to indicate activity */
			gpio_pin_set_dt(&led, 1);

			/* Schedule work to toggle LED off after duration */
			k_work_schedule(&led_blink_work, K_MSEC(LED_BLINK_DURATION_MS));
		}
	}
}

int main(void)
{
	int ret;

	if (!device_is_ready(lin_dut)) {
		printk("LIN DUT device not found\n");
		return 0;
	}

	if (led.port != NULL) {
		if (!device_is_ready(led.port)) {
			printk("LED device not found\n");
			return 0;
		}

		ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			printk("Failed to configure LED GPIO pin\n");
			return 0;
		}
	}

	ret = lin_configure(lin_dut, &dut_config);
	if (ret < 0) {
		printk("Error configuring LIN DUT [%d]", ret);
		return 0;
	}

	ret = lin_set_callback(lin_dut, lin_dut_event_handler, NULL);
	if (ret < 0) {
		printk("Error setting LIN DUT event handler [%d]", ret);
		return 0;
	}

	ret = lin_start(lin_dut);
	if (ret) {
		printk("LIN start failed: %d\n", ret);
		return 0;
	}

	printk("LIN DUT started in RESPONDER mode\n");
	printk("Counter ID: 0x%02X\n", COUNTER_UPDATE_FRAME_ID);

	counter = 0;

	while (true) {
		printk("Counter: %u\n", counter++);
		k_msleep(COUNTER_UPDATE_INTERVAL_MS);
	}

	return 0;
}

/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/mgmt/osdp.h>

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

#if !DT_NODE_HAS_STATUS(LED0_NODE, okay)
#error "BOARD does not define a debug LED"
#endif

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET_OR(LED0_NODE, gpios, {0});

#define SLEEP_TIME_MS                  (20)
#define CNT_PER_SEC                    (1000 / SLEEP_TIME_MS)
#define COMMAND_WAIT_TIME_SEC          (10)
#define COMMAND_WAIT_COUNT             (COMMAND_WAIT_TIME_SEC * CNT_PER_SEC)

enum osdp_pd_e {
	OSDP_PD_0,
	OSDP_PD_SENTINEL,
};

int key_press_callback(int pd, uint8_t *data, int len)
{
	printk("CP PD[%d] key press - data: 0x%02x\n", pd, data[0]);
	return 0;
}

int card_read_callback(int pd, int format, uint8_t *data, int len)
{
	int i;

	printk("CP PD[%d] card read - fmt: %d len: %d card_data: [ ",
	       pd, format, len);

	for (i = 0; i < len; i++) {
		printk("0x%02x ", data[i]);
	}

	printk("]\n");
	return 0;
}

int event_handler(void *unused, int pd, struct osdp_event *e)
{
	switch (e->type) {
	case OSDP_EVENT_CARDREAD:
		card_read_callback(pd, e->cardread.format,
				   e->cardread.data, e->cardread.length);
		break;
	case OSDP_EVENT_KEYPRESS:
		key_press_callback(pd, e->keypress.data, e->keypress.length);
		break;
	default:
		break;
	}
	return 0;
}

int main(void)
{
	int ret, led_state;
	uint32_t cnt = 0;
	struct osdp_cmd pulse_output = {
		.id = OSDP_CMD_OUTPUT,
		.output.output_no = 0,     /* First output */
		.output.control_code = 5,  /* Temporarily turn on output */
		.output.timer_count = 10,  /* Timer: 10 * 100ms = 1 second */
	};

	if (!gpio_is_ready_dt(&led0)) {
		printk("Failed to get LED GPIO port %s\n", led0.port->name);
		return 0;
	}

	ret = gpio_pin_configure_dt(&led0, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printk("Failed to configure gpio pin\n");
		return 0;
	}

	osdp_cp_set_event_callback(event_handler, NULL);

	led_state = 0;
	while (1) {
		if ((cnt & 0x7f) == 0x7f) {
			/* show a sign of life */
			led_state = !led_state;
		}
		if ((cnt % COMMAND_WAIT_COUNT) == 0) {
			osdp_cp_send_command(OSDP_PD_0, &pulse_output);
		}
		gpio_pin_set(led0.port, led0.pin, led_state);
		k_msleep(SLEEP_TIME_MS);
		cnt++;
	}
	return 0;
}

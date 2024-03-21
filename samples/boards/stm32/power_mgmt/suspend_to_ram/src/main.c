/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/entropy.h>
#include <string.h>

#define SLEEP_TIME_STOP0_MS	800
#define SLEEP_TIME_STOP1_MS	1500
#define SLEEP_TIME_STANDBY_MS	3000
#define SLEEP_TIME_BUSY_MS	2000

static const struct gpio_dt_spec led =
	GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
	!DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif

#define DT_SPEC_AND_COMMA(node_id, prop, idx) \
	ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

/* Data of ADC io-channels specified in devicetree. */
static const struct adc_dt_spec adc_channels[] = {
	DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels,
			     DT_SPEC_AND_COMMA)
};

const struct device *rng_dev;

#define BUFFER_LENGTH	3

static uint8_t entropy_buffer[BUFFER_LENGTH] = {0};

static int adc_test(void)
{
	int err;
	static uint32_t count;
	uint16_t buf;
	struct adc_sequence sequence = {
		.buffer = &buf,
		/* buffer size in bytes, not number of samples */
		.buffer_size = sizeof(buf),
	};

	/* Configure channels individually prior to sampling. */
	for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
		if (!adc_is_ready_dt(&adc_channels[i])) {
			printk("ADC controller device %s not ready\n", adc_channels[i].dev->name);
			return 0;
		}

		err = adc_channel_setup_dt(&adc_channels[i]);
		if (err < 0) {
			printk("Could not setup channel #%d (%d)\n", i, err);
			return 0;
		}
	}

	printk("ADC reading[%u]:\n", count++);
	for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
		int32_t val_mv;

		printk("- %s, channel %d: ",
			adc_channels[i].dev->name,
			adc_channels[i].channel_id);

		(void)adc_sequence_init_dt(&adc_channels[i], &sequence);

		err = adc_read_dt(&adc_channels[i], &sequence);
		if (err < 0) {
			printk("Could not read (%d)\n", err);
			continue;
		}

		/*
		 * If using differential mode, the 16 bit value
		 * in the ADC sample buffer should be a signed 2's
		 * complement value.
		 */
		if (adc_channels[i].channel_cfg.differential) {
			val_mv = (int32_t)((int16_t)buf);
		} else {
			val_mv = (int32_t)buf;
		}
		printk("%"PRId32, val_mv);
		err = adc_raw_to_millivolts_dt(&adc_channels[i],
						&val_mv);
		/* conversion to mV may not be supported, skip if not */
		if (err < 0) {
			printk(" (value in mV not available)\n");
		} else {
			printk(" = %"PRId32" mV\n", val_mv);
		}
	}

	return 0;
}

void print_buf(uint8_t *buffer)
{
	int i;
	int count = 0;

	for (i = 0; i < BUFFER_LENGTH; i++) {
		printk("  0x%02x", buffer[i]);
		if (buffer[i] == 0x00) {
			count++;
		}
	}
	printk("\n");
}

int main(void)
{
	__ASSERT_NO_MSG(gpio_is_ready_dt(&led));

	rng_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_entropy));
	if (!device_is_ready(rng_dev)) {
		printk("error: random device not ready");
	}

	printk("Device ready\n");

	while (true) {
		gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
		adc_test();
		k_busy_wait(SLEEP_TIME_BUSY_MS*1000);
		gpio_pin_set_dt(&led, 0);
		k_msleep(SLEEP_TIME_STOP0_MS);
		printk("Exit Stop0\n");

		gpio_pin_set_dt(&led, 1);
		adc_test();
		k_busy_wait(SLEEP_TIME_BUSY_MS*1000);
		gpio_pin_set_dt(&led, 0);
		k_msleep(SLEEP_TIME_STOP1_MS);
		printk("Exit Stop1\n");

		(void)memset(entropy_buffer, 0x00, BUFFER_LENGTH);
		entropy_get_entropy(rng_dev, (char *)entropy_buffer, BUFFER_LENGTH);
		printk("Sync entropy: ");
		print_buf(entropy_buffer);

		gpio_pin_set_dt(&led, 1);
		adc_test();
		k_busy_wait(SLEEP_TIME_BUSY_MS*1000);
		gpio_pin_configure_dt(&led, GPIO_DISCONNECTED);
		k_msleep(SLEEP_TIME_STANDBY_MS);
		printk("Exit Standby\n");
	}
	return 0;
}

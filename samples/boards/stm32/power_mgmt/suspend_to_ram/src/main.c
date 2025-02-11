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
#include <zephyr/drivers/spi.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#define SLEEP_TIME_STOP0_MS	800
#define SLEEP_TIME_STOP1_MS	1500
#define SLEEP_TIME_STANDBY_MS	3000
#define SLEEP_TIME_BUSY_MS	2000

static const struct gpio_dt_spec led =
	GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

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

#define SPI_TEST_DEV	DT_COMPAT_GET_ANY_STATUS_OKAY(test_spi_loopback)

#define FRAME_SIZE (8)

#define SPI_OP(frame_size) SPI_OP_MODE_MASTER | SPI_MODE_CPOL | \
	       SPI_MODE_CPHA | SPI_WORD_SET(frame_size) | SPI_LINES_SINGLE

static struct spi_dt_spec spi_test_dev = SPI_DT_SPEC_GET(SPI_TEST_DEV, SPI_OP(FRAME_SIZE), 0);

#define SPI_BUF_SIZE 18

static const char spi_tx_data[SPI_BUF_SIZE] = "0123456789abcdef-\0";
static __aligned(32) char spi_buffer_tx[SPI_BUF_SIZE] __used;
static __aligned(32) char spi_buffer_rx[SPI_BUF_SIZE] __used;

static uint8_t spi_buffer_print_tx[SPI_BUF_SIZE * 5 + 1];
static uint8_t spi_buffer_print_rx[SPI_BUF_SIZE * 5 + 1];

static void to_display_format(const uint8_t *src, size_t size, char *dst)
{
	size_t i;

	for (i = 0; i < size; i++) {
		sprintf(dst + 5 * i, "0x%02x,", src[i]);
	}
}

static int spi_test(void)
{
	const struct spi_buf tx_bufs[] = {
		{
			.buf = spi_buffer_tx,
			.len = SPI_BUF_SIZE,
		},
	};
	const struct spi_buf rx_bufs[] = {
		{
			.buf = spi_buffer_rx,
			.len = SPI_BUF_SIZE,
		},
	};
	const struct spi_buf_set tx = {
		.buffers = tx_bufs,
		.count = ARRAY_SIZE(tx_bufs)
	};
	const struct spi_buf_set rx = {
		.buffers = rx_bufs,
		.count = ARRAY_SIZE(rx_bufs)
	};

	int ret;

	ret = spi_transceive_dt(&spi_test_dev, &tx, &rx);
	if (ret) {
		printk("SPI transceive failed: %d\n", ret);
		return ret;
	}

	if (memcmp(spi_buffer_tx, spi_buffer_rx, SPI_BUF_SIZE)) {
		to_display_format(spi_buffer_tx, SPI_BUF_SIZE, spi_buffer_print_tx);
		to_display_format(spi_buffer_rx, SPI_BUF_SIZE, spi_buffer_print_rx);
		printk("Buffer contents are different\n");
		printk("tx: %s\n", spi_buffer_print_tx);
		printk("rx: %s\n", spi_buffer_print_rx);
		return -1;
	}

	return 0;
}

static void spi_setup(void)
{
	memset(spi_buffer_tx, 0, sizeof(spi_buffer_tx));
	memcpy(spi_buffer_tx, spi_tx_data, sizeof(spi_tx_data));

	if (!spi_is_ready_dt(&spi_test_dev)) {
		printk("Fast spi lookback device is not ready\n");
	}
}

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

static void loop(void)
{
	gpio_pin_set_dt(&led, 1);
	adc_test();
	if (!IS_ENABLED(CONFIG_PM_S2RAM)) {
		spi_test();
	}
	k_busy_wait(SLEEP_TIME_BUSY_MS*1000);
	gpio_pin_set_dt(&led, 0);
}

int main(void)
{
	__ASSERT_NO_MSG(gpio_is_ready_dt(&led));

	rng_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_entropy));
	if (!device_is_ready(rng_dev)) {
		printk("error: random device not ready");
	}

	spi_setup();

	printk("Device ready\n");

	while (true) {
		gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
		loop();
		k_msleep(SLEEP_TIME_STOP0_MS);
		printk("Exit Stop0\n");

		loop();
		k_msleep(SLEEP_TIME_STOP1_MS);
		printk("Exit Stop1\n");

		(void)memset(entropy_buffer, 0x00, BUFFER_LENGTH);
		entropy_get_entropy(rng_dev, (char *)entropy_buffer, BUFFER_LENGTH);
		printk("Sync entropy: ");
		print_buf(entropy_buffer);

		loop();
		gpio_pin_configure_dt(&led, GPIO_DISCONNECTED);
		k_msleep(SLEEP_TIME_STANDBY_MS);
		printk("Exit Standby\n");
	}
	return 0;
}

/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/pulse_io.h>
#include <stdio.h>

#define RESOLUTION_HZ 1000000
#define NUM_TRANSFERS 5

static const struct device *dev = DEVICE_DT_GET(DT_ALIAS(pulse_io0));

static const struct pulse_io_bit_template tmpl = {
	.zero = {{.duration = 25, .level = 1}, {.duration = 75, .level = 0}},
	.one = {{.duration = 75, .level = 1}, {.duration = 25, .level = 0}},
	.msb_first = true,
};

static struct pulse_io_channel *tx_chan;
static struct pulse_io_channel *rx_chan;
static struct pulse_symbol tx_syms[24];
static struct pulse_symbol rx_syms[32];

static K_THREAD_STACK_DEFINE(tx_stack, 2048);
static struct k_thread tx_thread;
static struct pulse_io_tx_req tx_req;
static volatile int tx_result;

static void tx_thread_fn(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	k_sleep(K_MSEC(50));
	tx_result = pulse_io_transmit_sync(dev, tx_chan, &tx_req, K_SECONDS(1));
}

static int pick_and_configure(uint32_t mask, const struct pulse_io_config *cfg,
			      struct pulse_io_channel **chan, uint8_t *index)
{
	for (uint8_t i = 0; i < 32; i++) {
		if (!(mask & BIT(i))) {
			continue;
		}
		if (pulse_io_channel_get(dev, i, chan) != 0) {
			continue;
		}
		if (pulse_io_channel_configure(dev, *chan, cfg) == 0) {
			*index = i;
			return 0;
		}
		pulse_io_channel_release(dev, *chan);
	}
	return -ENODEV;
}

int main(void)
{
	struct pulse_io_caps caps;
	struct pulse_io_config cfg = {
		.mode = PULSE_IO_MODE_SYMBOL,
		.resolution_hz = RESOLUTION_HZ,
	};
	uint8_t tx_index;
	uint8_t rx_index;
	int ret;

	if (!device_is_ready(dev)) {
		printf("pulse_io device not ready\n");
		return 0;
	}

	pulse_io_get_capabilities(dev, &caps);

	cfg.dir = PULSE_IO_DIR_TX;
	ret = pick_and_configure(caps.tx_channel_mask, &cfg, &tx_chan, &tx_index);
	if (ret == 0) {
		cfg.dir = PULSE_IO_DIR_RX;
		cfg.rx_idle_threshold_ticks = 300;
		cfg.rx_filter_ticks = 1;
		ret = pick_and_configure(caps.rx_channel_mask & ~BIT(tx_index), &cfg, &rx_chan,
					 &rx_index);
	}
	if (ret != 0) {
		printf("channel configuration failed (%d)\n", ret);
		return 0;
	}

	printf("pulse_io byte transfer: TX channel %u, RX channel %u\n", tx_index, rx_index);

	for (uint8_t value = 0; value < NUM_TRANSFERS; value++) {
		uint8_t decoded = 0xff;
		size_t produced = 0;
		size_t received = 0;

		pulse_io_encode_bytes(&tmpl, &value, 1, tx_syms, ARRAY_SIZE(tx_syms), &produced);
		tx_syms[produced++] = (struct pulse_symbol){.duration = 25, .level = 1};
		tx_req = (struct pulse_io_tx_req){.symbols = tx_syms, .count = produced};

		k_thread_create(&tx_thread, tx_stack, K_THREAD_STACK_SIZEOF(tx_stack), tx_thread_fn,
				NULL, NULL, NULL, K_PRIO_PREEMPT(1), 0, K_NO_WAIT);

		ret = pulse_io_receive_sync(dev, rx_chan,
					    &(struct pulse_io_rx_req){
						    .symbols = rx_syms,
						    .capacity = ARRAY_SIZE(rx_syms),
					    },
					    &received, K_SECONDS(1));
		k_thread_join(&tx_thread, K_FOREVER);
		if (tx_result != 0) {
			printf("transmit failed (%d)\n", tx_result);
			return 0;
		}
		if (ret != 0) {
			printf("receive failed (%d)\n", ret);
			return 0;
		}

		pulse_io_decode_bytes(&tmpl, 12, rx_syms, 16, &decoded, sizeof(decoded), &produced);
		printf("sent 0x%02x received 0x%02x\n", value, decoded);
		if (decoded != value) {
			printf("transfer mismatch\n");
			return 0;
		}

		k_sleep(K_MSEC(500));
	}

	printf("byte transfer done\n");
	return 0;
}

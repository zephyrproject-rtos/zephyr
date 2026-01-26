/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Sample application for Espressif RMT.
 */

#include <stdlib.h>
#include <zephyr/device.h>
#include <zephyr/drivers/misc/espressif_rmt/rmt.h>
#include <zephyr/drivers/misc/espressif_rmt/rmt_tx.h>
#include <zephyr/drivers/misc/espressif_rmt/rmt_rx.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/pinctrl/pinctrl_esp32_common.h>
#include <zephyr/kernel.h>

#define RMT_NODE DT_NODELABEL(rmt)

static const struct device *rmt_dev = DEVICE_DT_GET(RMT_NODE);

/* Set the number of block symbol */
#define MEM_BLOCK_SYMBOL (64)

/* 10MHz resolution, 1 tick = 0.1us */
#define RESOLUTION_HZ (10000000)

/* Set the number of transactions that can be pending in the background */
#define TRANS_QUEUE_DEPTH (4)

rmt_tx_channel_config_t tx_chan_config;
rmt_rx_channel_config_t rx_chan_config;
rmt_channel_handle_t tx_chan;
rmt_channel_handle_t rx_chan;

rmt_encoder_handle_t tx_encoder;

rmt_symbol_word_t raw_symbols[64];
rmt_receive_config_t receive_config = {
	.signal_range_min_ns = 125,
	.signal_range_max_ns = 2500000,
};

static const rmt_symbol_word_t symbol_zero = {
	.level0 = 1,
	.duration0 = 10 * RESOLUTION_HZ / 1000000, /* T0H=10us */
	.level1 = 0,
	.duration1 = 90 * RESOLUTION_HZ / 1000000, /* T0L=90us */
};

static const rmt_symbol_word_t symbol_one = {
	.level0 = 1,
	.duration0 = 90 * RESOLUTION_HZ / 1000000, /* T0L=90us */
	.level1 = 0,
	.duration1 = 10 * RESOLUTION_HZ / 1000000, /* T0H=10us */
};

/* reset code duration defaults to 500us */
static const rmt_symbol_word_t symbol_reset = {
	.level0 = 0,
	.duration0 = RESOLUTION_HZ / 1000000 * 500 / 2,
	.level1 = 0,
	.duration1 = RESOLUTION_HZ / 1000000 * 500 / 2,
};

static size_t encoder_callback(const void *data, size_t data_size, size_t symbols_written,
	size_t symbols_free, rmt_symbol_word_t *symbols, bool *done, void *arg)
{
	size_t data_pos = symbols_written / 8;
	uint8_t *data_bytes = (uint8_t *)data;
	size_t symbol_pos = 0;

	/*
	 * We need a minimum of 8 symbol spaces to encode a byte. We only
	 * need one to encode a reset, but it's simpler to simply demand that
	 * there are 8 symbol spaces free to write anything.
	 */
	if (symbols_free < 8) {
		return 0;
	}

	/*
	 * We can calculate where in the data we are from the symbol pos.
	 * Alternatively, we could use some counter referenced by the arg
	 * parameter to keep track of this.
	 */
	if (data_pos < data_size) {
		/* Encode a byte */
		for (int bitmask = 0x80; bitmask != 0; bitmask >>= 1) {
			if (data_bytes[data_pos] & bitmask) {
				symbols[symbol_pos++] = symbol_one;
			} else {
				symbols[symbol_pos++] = symbol_zero;
			}
		}
		/* We're done; we should have written 8 symbols. */
		return symbol_pos;
	}

	/*
	 * All bytes already are encoded.
	 * Encode the reset, and we're done.
	 */
	symbols[0] = symbol_reset;
	*done = 1; /* Indicate end of the transaction. */
	return 1; /* We only wrote one symbol */
}

static int example_encoder_new(rmt_encoder_handle_t *ret_encoder)
{
	rmt_simple_encoder_config_t simple_encoder_config;
	int rc;

	if (!ret_encoder) {
		printk("Invalid argument\n");
		return -EINVAL;
	}
	simple_encoder_config.callback = encoder_callback;
	simple_encoder_config.arg = NULL;
	simple_encoder_config.min_chunk_size = 64;
	rc = rmt_new_simple_encoder(&simple_encoder_config, ret_encoder);
	if (rc) {
		printk("Create simple encoder failed %d\n", rc);
		return rc;
	}

	return 0;
}

static bool rmt_rx_done_callback(rmt_channel_handle_t channel,
	const rmt_rx_done_event_data_t *edata, void *user_data)
{
	uint8_t val = 0;
	int rc;

	/* Compute received value */
	if (edata->num_symbols > 0) {
		for (int i = 0; i < edata->num_symbols; i++) {
			rmt_symbol_word_t symbol = edata->received_symbols[i];
			/* Threshold=50us */
			if (symbol.level0 == 1
				&& symbol.duration0 > 50 * RESOLUTION_HZ / 1000000) {
				val |= 1 << (edata->num_symbols - i - 1);
			} else if (symbol.level1 == 1
				&& symbol.duration1 > 50 * RESOLUTION_HZ / 1000000) {
				val |= 1 << (edata->num_symbols - i - 1);
			}
		}
		printk("RMT RX value 0x%02x\n", val);
	}

	/* Restart reception */
	rc = rmt_receive(channel, raw_symbols, sizeof(raw_symbols), &receive_config);
	if (rc) {
		printk("Unable to start RMT RX channel\n");
	}

	return false;
}

int main(void)
{
	const struct espressif_rmt_config *cfg = rmt_dev->config;
	uint8_t counter = 0;
	void *buffer = &counter;
	size_t buf_len = sizeof(uint8_t);
	int rc;

	if (!device_is_ready(rmt_dev)) {
		printk("Espressif RMT device is not ready\n");
		return -EINVAL;
	}

	/* RMT Rx channel configuration */
	rx_chan_config.clk_src = RMT_CLK_SRC_DEFAULT;
	rx_chan_config.mem_block_symbols = MEM_BLOCK_SYMBOL;
	rx_chan_config.resolution_hz = RESOLUTION_HZ;
	rx_chan_config.gpio_pinmux = cfg->pcfg->states[0].pins[1].pinmux;
	rc = rmt_new_rx_channel(rmt_dev, &rx_chan_config, &rx_chan);
	if (rc) {
		printk("RMT RX channel creation failed\n");
		return rc;
	}
	rmt_rx_event_callbacks_t cbs = {
		.on_recv_done = rmt_rx_done_callback,
	};
	rc = rmt_rx_register_event_callbacks(rx_chan, &cbs, NULL);
	if (rc) {
		printk("Unable to register RX callback\n");
		return rc;
	}

	/* RMT Tx channel configuration */
	tx_chan_config.clk_src = RMT_CLK_SRC_DEFAULT;
	tx_chan_config.mem_block_symbols = MEM_BLOCK_SYMBOL;
	tx_chan_config.resolution_hz = RESOLUTION_HZ;
	tx_chan_config.trans_queue_depth = TRANS_QUEUE_DEPTH;
	tx_chan_config.gpio_pinmux = cfg->pcfg->states[0].pins[0].pinmux;
	rc = rmt_new_tx_channel(rmt_dev, &tx_chan_config, &tx_chan);
	if (rc) {
		printk("RMT TX channel creation failed\n");
		return rc;
	}
	rc = example_encoder_new(&tx_encoder);
	if (rc) {
		printk("Unable to create encoder\n");
		return rc;
	}

	/* Enable channels */
	printk("Enable RMT RX channel\n");
	rc = rmt_enable(rx_chan);
	if (rc) {
		printk("Unable to enable RMT RX channel\n");
		return rc;
	}
	printk("Enable RMT TX channel\n");
	rc = rmt_enable(tx_chan);
	if (rc) {
		printk("Unable to enable RMT TX channel\n");
		return rc;
	}

	/* Start reception */
	rc = rmt_receive(rx_chan, raw_symbols, sizeof(raw_symbols), &receive_config);
	if (rc) {
		printk("Unable to start RMT RX channel\n");
		return rc;
	}

	while (counter < 5) {

		rmt_transmit_config_t tx_config = {
			.loop_count = 0, /* no transfer loop */
		};

		/* Transmit data */
		printk("RMT TX value 0x%02x\n", counter);
		rc = rmt_transmit(tx_chan, tx_encoder, buffer, buf_len, &tx_config);
		if (rc) {
			printk("Unable to transmit data over TX channel\n");
			return rc;
		}
		rc = rmt_tx_wait_all_done(tx_chan, K_FOREVER);
		if (rc) {
			printk("Waiting until all done failed\n");
			return rc;
		}

		k_sleep(K_MSEC(500));

		/* Increment counter for next transmission */
		counter++;
	}

	/* Disable and delete channels */
	rmt_disable(tx_chan);
	rmt_disable(rx_chan);
	rmt_del_channel(tx_chan);
	rmt_del_channel(rx_chan);

	/* Delete encoder */
	rmt_del_encoder(tx_encoder);

	return 0;
}

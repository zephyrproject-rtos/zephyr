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

typedef struct {
	rmt_encoder_t base;
	rmt_encoder_t *bytes_encoder;
	rmt_encoder_t *copy_encoder;
	int state;
	rmt_symbol_word_t reset_code;
} rmt_example_encoder_t;

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

static size_t example_encoder_encode(rmt_encoder_t *encoder,
	rmt_channel_handle_t channel, const void *primary_data, size_t data_size,
	rmt_encode_state_t *ret_state)
{
	rmt_example_encoder_t *example_encoder =
		__containerof(encoder, rmt_example_encoder_t, base);
	rmt_encoder_handle_t bytes_encoder = example_encoder->bytes_encoder;
	rmt_encoder_handle_t copy_encoder = example_encoder->copy_encoder;
	rmt_encode_state_t session_state = RMT_ENCODING_RESET;
	rmt_encode_state_t state = RMT_ENCODING_RESET;
	size_t encoded_symbols = 0;

	switch (example_encoder->state) {
	case 0: /* send data */
		encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, primary_data,
			data_size, &session_state);
		if (session_state & RMT_ENCODING_COMPLETE) {
			/* switch to next state when current encoding session finished */
			example_encoder->state = 1;
		}
		if (session_state & RMT_ENCODING_MEM_FULL) {
			state |= RMT_ENCODING_MEM_FULL;
			goto out; /* yield if there's no free space for encoding artifacts */
		}
	/* fall-through */
	case 1: /* send reset code */
		encoded_symbols += copy_encoder->encode(copy_encoder, channel,
			&example_encoder->reset_code, sizeof(example_encoder->reset_code),
			&session_state);
		if (session_state & RMT_ENCODING_COMPLETE) {
			/* back to the initial encoding session */
			example_encoder->state = RMT_ENCODING_RESET;
			state |= RMT_ENCODING_COMPLETE;
		}
		if (session_state & RMT_ENCODING_MEM_FULL) {
			state |= RMT_ENCODING_MEM_FULL;
			goto out; /* yield if there's no free space for encoding artifacts */
		}
		break;
	default:
		break;
	}
out:
	*ret_state = state;
	return encoded_symbols;
}

static int example_encoder_del(rmt_encoder_t *encoder)
{
	rmt_example_encoder_t *example_encoder =
		__containerof(encoder, rmt_example_encoder_t, base);

	rmt_del_encoder(example_encoder->bytes_encoder);
	rmt_del_encoder(example_encoder->copy_encoder);
	free(example_encoder);
	return 0;
}

static int example_encoder_reset(rmt_encoder_t *encoder)
{
	rmt_example_encoder_t *example_encoder =
		__containerof(encoder, rmt_example_encoder_t, base);

	rmt_encoder_reset(example_encoder->bytes_encoder);
	rmt_encoder_reset(example_encoder->copy_encoder);
	example_encoder->state = RMT_ENCODING_RESET;
	return 0;
}

static int example_encoder_new(rmt_encoder_handle_t *ret_encoder)
{
	rmt_example_encoder_t *example_encoder = NULL;
	rmt_copy_encoder_config_t copy_encoder_config;
	int rc;

	if (!ret_encoder) {
		printk("Invalid argument\n");
		return -EINVAL;
	}
	example_encoder = rmt_alloc_encoder_mem(sizeof(rmt_example_encoder_t));
	if (!example_encoder) {
		printk("Unable to allocate memory for example encoder\n");
		return -ENOMEM;
	}
	example_encoder->base.encode = example_encoder_encode;
	example_encoder->base.del = example_encoder_del;
	example_encoder->base.reset = example_encoder_reset;
	rmt_bytes_encoder_config_t bytes_encoder_config = {
		.bit0 = {
			.level0 = 1,
			.duration0 = 10 * RESOLUTION_HZ / 1000000, /* T0H=10us */
			.level1 = 0,
			.duration1 = 90 * RESOLUTION_HZ / 1000000, /* T0L=90us */
		},
		.bit1 = {
			.level0 = 1,
			.duration0 = 90 * RESOLUTION_HZ / 1000000, /* T1H=90us */
			.level1 = 0,
			.duration1 = 10 * RESOLUTION_HZ / 1000000, /* T1L=10us */
		},
		.flags.msb_first = 1
	};
	rc = rmt_new_bytes_encoder(&bytes_encoder_config, &example_encoder->bytes_encoder);
	if (rc) {
		printk("Create bytes encoder failed\n");
		goto err;
	}
	rc = rmt_new_copy_encoder(&copy_encoder_config, &example_encoder->copy_encoder);
	if (rc) {
		printk("Create copy encoder failed\n");
		goto err;
	}

	/* reset code duration defaults to 500us */
	uint32_t reset_ticks = RESOLUTION_HZ / 1000000 * 500 / 2;

	example_encoder->reset_code = (rmt_symbol_word_t) {
		.level0 = 0,
		.duration0 = reset_ticks,
		.level1 = 0,
		.duration1 = reset_ticks,
	};
	*ret_encoder = &example_encoder->base;
	return 0;

err:
	if (example_encoder) {
		if (example_encoder->bytes_encoder) {
			rmt_del_encoder(example_encoder->bytes_encoder);
		}
		if (example_encoder->copy_encoder) {
			rmt_del_encoder(example_encoder->copy_encoder);
		}
		free(example_encoder);
	}
	return rc;
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

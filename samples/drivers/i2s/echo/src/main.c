/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "codec.h"
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>


#if DT_NODE_EXISTS(DT_NODELABEL(i2s_rxtx))
#define I2S_RX_NODE  DT_NODELABEL(i2s_rxtx)
#define I2S_TX_NODE  I2S_RX_NODE
#else
#define I2S_RX_NODE  DT_NODELABEL(i2s_rx)
#define I2S_TX_NODE  DT_NODELABEL(i2s_tx)
#endif

#define SAMPLE_FREQUENCY    44100
#define SAMPLE_BIT_WIDTH    16
#define BYTES_PER_SAMPLE    sizeof(int16_t)
#define NUMBER_OF_CHANNELS  2
/* Such block length provides an echo with the delay of 100 ms. */
#define SAMPLES_PER_BLOCK   ((SAMPLE_FREQUENCY / 10) * NUMBER_OF_CHANNELS)
#define INITIAL_BLOCKS      2
#define TIMEOUT             1000

#define SW0_NODE        DT_ALIAS(sw0)
#if DT_NODE_HAS_STATUS(SW0_NODE, okay)
static struct gpio_dt_spec sw0_spec = GPIO_DT_SPEC_GET(SW0_NODE, gpios);
#endif

#define SW1_NODE        DT_ALIAS(sw1)
#if DT_NODE_HAS_STATUS(SW1_NODE, okay)
static struct gpio_dt_spec sw1_spec = GPIO_DT_SPEC_GET(SW1_NODE, gpios);
#endif

#define BLOCK_SIZE  (BYTES_PER_SAMPLE * SAMPLES_PER_BLOCK)
#define BLOCK_COUNT (INITIAL_BLOCKS + 2)
K_MEM_SLAB_DEFINE_STATIC(mem_slab, BLOCK_SIZE, BLOCK_COUNT, 4);

static int16_t echo_block[SAMPLES_PER_BLOCK];
static volatile bool echo_enabled = true;
static K_SEM_DEFINE(toggle_transfer, 1, 1);

#if DT_NODE_HAS_STATUS(SW0_NODE, okay)
static void sw0_handler(const struct device *dev, struct gpio_callback *cb,
			uint32_t pins)
{
	bool enable = !echo_enabled;

	echo_enabled = enable;
	printk("Echo %sabled\n", (enable ? "en" : "dis"));
}
#endif

#if DT_NODE_HAS_STATUS(SW1_NODE, okay)
static void sw1_handler(const struct device *dev, struct gpio_callback *cb,
			uint32_t pins)
{
	k_sem_give(&toggle_transfer);
}
#endif

static bool init_buttons(void)
{
	int ret;

#if DT_NODE_HAS_STATUS(SW0_NODE, okay)
	static struct gpio_callback sw0_cb_data;

	if (!device_is_ready(sw0_spec.port)) {
		printk("%s is not ready\n", sw0_spec.port->name);
		return false;
	}

	ret = gpio_pin_configure_dt(&sw0_spec, GPIO_INPUT);
	if (ret < 0) {
		printk("Failed to configure %s pin %d: %d\n",
		       sw0_spec.port->name, sw0_spec.pin, ret);
		return false;
	}

	ret = gpio_pin_interrupt_configure_dt(&sw0_spec,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		printk("Failed to configure interrupt on %s pin %d: %d\n",
		       sw0_spec.port->name, sw0_spec.pin, ret);
		return false;
	}

	gpio_init_callback(&sw0_cb_data, sw0_handler, BIT(sw0_spec.pin));
	gpio_add_callback(sw0_spec.port, &sw0_cb_data);
	printk("Press \"%s\" to toggle the echo effect\n", sw0_spec.port->name);
#endif

#if DT_NODE_HAS_STATUS(SW1_NODE, okay)
	static struct gpio_callback sw1_cb_data;

	if (!device_is_ready(sw1_spec.port)) {
		printk("%s is not ready\n", sw1_spec.port->name);
		return false;
	}

	ret = gpio_pin_configure_dt(&sw1_spec, GPIO_INPUT);
	if (ret < 0) {
		printk("Failed to configure %s pin %d: %d\n",
		       sw1_spec.port->name, sw1_spec.pin, ret);
		return false;
	}

	ret = gpio_pin_interrupt_configure_dt(&sw1_spec,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		printk("Failed to configure interrupt on %s pin %d: %d\n",
		       sw1_spec.port->name, sw1_spec.pin, ret);
		return false;
	}

	gpio_init_callback(&sw1_cb_data, sw1_handler, BIT(sw1_spec.pin));
	gpio_add_callback(sw1_spec.port, &sw1_cb_data);
	printk("Press \"%s\" to stop/restart I2S streams\n", sw1_spec.port->name);
#endif

	(void)ret;
	return true;
}

static void process_block_data(void *mem_block, uint32_t number_of_samples)
{
	static bool clear_echo_block;

	if (echo_enabled) {
		for (int i = 0; i < number_of_samples; ++i) {
			int16_t *sample = &((int16_t *)mem_block)[i];
			*sample += echo_block[i];
			echo_block[i] = (*sample) / 2;
		}

		clear_echo_block = true;
	} else if (clear_echo_block) {
		clear_echo_block = false;
		memset(echo_block, 0, sizeof(echo_block));
	}
}

static bool configure_streams(const struct device *i2s_dev_rx,
			      const struct device *i2s_dev_tx,
			      const struct i2s_config *config)
{
	int ret;

	if (i2s_dev_rx == i2s_dev_tx) {
		ret = i2s_configure(i2s_dev_rx, I2S_DIR_BOTH, config);
		if (ret == 0) {
			return true;
		}
		/* -ENOSYS means that the RX and TX streams need to be
		 * configured separately.
		 */
		if (ret != -ENOSYS) {
			printk("Failed to configure streams: %d\n", ret);
			return false;
		}
	}

	ret = i2s_configure(i2s_dev_rx, I2S_DIR_RX, config);
	if (ret < 0) {
		printk("Failed to configure RX stream: %d\n", ret);
		return false;
	}

	ret = i2s_configure(i2s_dev_tx, I2S_DIR_TX, config);
	if (ret < 0) {
		printk("Failed to configure TX stream: %d\n", ret);
		return false;
	}

	return true;
}

static bool prepare_transfer(const struct device *i2s_dev_rx,
			     const struct device *i2s_dev_tx)
{
	int ret;

	for (int i = 0; i < INITIAL_BLOCKS; ++i) {
		void *mem_block;

		ret = k_mem_slab_alloc(&mem_slab, &mem_block, K_NO_WAIT);
		if (ret < 0) {
			printk("Failed to allocate TX block %d: %d\n", i, ret);
			return false;
		}

		memset(mem_block, 0, BLOCK_SIZE);

		ret = i2s_write(i2s_dev_tx, mem_block, BLOCK_SIZE);
		if (ret < 0) {
			printk("Failed to write block %d: %d\n", i, ret);
			return false;
		}
	}

	return true;
}

static bool trigger_command(const struct device *i2s_dev_rx,
			    const struct device *i2s_dev_tx,
			    enum i2s_trigger_cmd cmd)
{
	int ret;

	if (i2s_dev_rx == i2s_dev_tx) {
		ret = i2s_trigger(i2s_dev_rx, I2S_DIR_BOTH, cmd);
		if (ret == 0) {
			return true;
		}
		/* -ENOSYS means that commands for the RX and TX streams need
		 * to be triggered separately.
		 */
		if (ret != -ENOSYS) {
			printk("Failed to trigger command %d: %d\n", cmd, ret);
			return false;
		}
	}

	ret = i2s_trigger(i2s_dev_rx, I2S_DIR_RX, cmd);
	if (ret < 0) {
		printk("Failed to trigger command %d on RX: %d\n", cmd, ret);
		return false;
	}

	ret = i2s_trigger(i2s_dev_tx, I2S_DIR_TX, cmd);
	if (ret < 0) {
		printk("Failed to trigger command %d on TX: %d\n", cmd, ret);
		return false;
	}

	return true;
}

int main(void)
{
	const struct device *const i2s_dev_rx = DEVICE_DT_GET(I2S_RX_NODE);
	const struct device *const i2s_dev_tx = DEVICE_DT_GET(I2S_TX_NODE);
	struct i2s_config config;

	printk("I2S echo sample\n");

#if DT_ON_BUS(DT_NODELABEL(wm8731), i2c)
	if (!init_wm8731_i2c()) {
		return 0;
	}
#endif

	if (!init_buttons()) {
		return 0;
	}

	if (!device_is_ready(i2s_dev_rx)) {
		printk("%s is not ready\n", i2s_dev_rx->name);
		return 0;
	}

	if (i2s_dev_rx != i2s_dev_tx && !device_is_ready(i2s_dev_tx)) {
		printk("%s is not ready\n", i2s_dev_tx->name);
		return 0;
	}

	config.word_size = SAMPLE_BIT_WIDTH;
	config.channels = NUMBER_OF_CHANNELS;
	config.format = I2S_FMT_DATA_FORMAT_I2S;
	config.options = I2S_OPT_BIT_CLK_MASTER | I2S_OPT_FRAME_CLK_MASTER;
	config.frame_clk_freq = SAMPLE_FREQUENCY;
	config.mem_slab = &mem_slab;
	config.block_size = BLOCK_SIZE;
	config.timeout = TIMEOUT;
	if (!configure_streams(i2s_dev_rx, i2s_dev_tx, &config)) {
		return 0;
	}

	for (;;) {
		k_sem_take(&toggle_transfer, K_FOREVER);

		if (!prepare_transfer(i2s_dev_rx, i2s_dev_tx)) {
			return 0;
		}

		if (!trigger_command(i2s_dev_rx, i2s_dev_tx,
				     I2S_TRIGGER_START)) {
			return 0;
		}

		printk("Streams started\n");

		while (k_sem_take(&toggle_transfer, K_NO_WAIT) != 0) {
			void *mem_block;
			uint32_t block_size;
			int ret;

			ret = i2s_read(i2s_dev_rx, &mem_block, &block_size);
			if (ret < 0) {
				printk("Failed to read data: %d\n", ret);
				break;
			}

			process_block_data(mem_block, SAMPLES_PER_BLOCK);

			ret = i2s_write(i2s_dev_tx, mem_block, block_size);
			if (ret < 0) {
				printk("Failed to write data: %d\n", ret);
				break;
			}
		}

		if (!trigger_command(i2s_dev_rx, i2s_dev_tx,
				     I2S_TRIGGER_DROP)) {
			return 0;
		}

		printk("Streams stopped\n");
	}
}

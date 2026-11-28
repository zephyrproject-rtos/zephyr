/*
 * Copyright (c) 2026 iomico
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tlc6c59xx

#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mfd/tlc6c59xx.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(ti_tlc6c59xx, CONFIG_MFD_LOG_LEVEL);

struct tlc6c59xx_config {
	const struct spi_dt_spec bus;
	const struct gpio_dt_spec gpio_reset;
	const uint8_t *positions;
	const uint8_t chain_length;
	const uint8_t word_size;
};

struct tlc6c59xx_data {
	uint16_t *output_values;
	uint8_t *packed_buffer;
	struct k_mutex lock;
};

static int tlc6c59xx_write_state_helper(const struct device *dev)
{
	const struct tlc6c59xx_config *config = dev->config;
	struct tlc6c59xx_data *data = dev->data;
	size_t total_bits = (size_t)config->chain_length * config->word_size;
	size_t total_bytes = DIV_ROUND_UP(total_bits, 8U);
	/*
	 * A shift register only remembers the last total_bits it was sent, so
	 * any padding needed to reach a whole number of bytes must be clocked
	 * out first: by the time the real data finishes shifting in, the
	 * padding has already been pushed out the far end of the chain.
	 */
	size_t bit_pos = total_bytes * 8U - total_bits;

	memset(data->packed_buffer, 0, total_bytes);

	for (uint8_t chip = 0U; chip < config->chain_length; chip++) {
		uint16_t value = data->output_values[chip];

		for (uint8_t bit = config->word_size; bit > 0U; bit--) {
			if ((value & BIT(bit - 1U)) != 0U) {
				size_t byte_index = bit_pos / 8U;
				uint8_t bit_in_byte = (uint8_t)(7U - (bit_pos % 8U));

				data->packed_buffer[byte_index] |= (uint8_t)BIT(bit_in_byte);
			}
			bit_pos++;
		}
	}

	struct spi_buf buf = {
		.buf = data->packed_buffer,
		.len = total_bytes,
	};
	struct spi_buf_set tx = {
		.buffers = &buf,
		.count = 1U,
	};
	return spi_write_dt(&config->bus, &tx);
}

int tlc6c59xx_write_state(const struct device *dev, uint8_t pos, uint16_t value)
{
	const struct tlc6c59xx_config *config = dev->config;
	struct tlc6c59xx_data *data = dev->data;
	uint16_t previous_value;
	uint8_t index;
	int ret;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	if (pos >= config->chain_length || (value & ~BIT_MASK(config->word_size)) != 0U) {
		return -EINVAL;
	}

	index = (uint8_t)(config->chain_length - 1U - pos);
	ret = k_mutex_lock(&data->lock, K_FOREVER);
	if (ret != 0) {
		return ret;
	}

	previous_value = data->output_values[index];
	data->output_values[index] = value;
	ret = tlc6c59xx_write_state_helper(dev);
	if (ret != 0) {
		/* Do not replay a failed update during another child's next write. */
		data->output_values[index] = previous_value;
	}
	k_mutex_unlock(&data->lock);

	return ret;
}

static int tlc6c59xx_init(const struct device *dev)
{
	const struct tlc6c59xx_config *config = dev->config;
	struct tlc6c59xx_data *data = dev->data;
	int ret;

	for (size_t i = 0U; i < config->chain_length; i++) {
		for (size_t j = i + 1U; j < config->chain_length; j++) {
			if (config->positions[i] == config->positions[j]) {
				LOG_ERR("duplicate chain position %u", config->positions[i]);
				return -EINVAL;
			}
		}
	}

	ret = k_mutex_init(&data->lock);
	if (ret != 0) {
		LOG_ERR("unable to initialize mutex");
		return ret;
	}

	if (!spi_is_ready_dt(&config->bus)) {
		LOG_ERR("SPI bus %s is not ready", config->bus.bus->name);
		return -ENODEV;
	}

	if (config->gpio_reset.port != NULL) {
		if (!gpio_is_ready_dt(&config->gpio_reset)) {
			LOG_ERR("reset GPIO controller is not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&config->gpio_reset, GPIO_OUTPUT_INACTIVE);
		if (ret != 0) {
			LOG_ERR("failed to configure reset GPIO: %d", ret);
			return ret;
		}

		ret = gpio_pin_set_dt(&config->gpio_reset, 1);
		if (ret != 0) {
			LOG_ERR("failed to set reset GPIO");
			return ret;
		}
		k_busy_wait(1); /* TI datasheet suggests a pulse duration of at least 40 ns */

		ret = gpio_pin_set_dt(&config->gpio_reset, 0);
		if (ret != 0) {
			LOG_ERR("failed to set reset GPIO");
			return ret;
		}
		k_busy_wait(1);
	}

	ret = k_mutex_lock(&data->lock, K_FOREVER);
	if (ret != 0) {
		return ret;
	}
	memset(data->output_values, 0, config->chain_length * sizeof(uint16_t));
	ret = tlc6c59xx_write_state_helper(dev);
	k_mutex_unlock(&data->lock);

	return ret;
}

/*
 * ngpios must match across every chip in the chain; read from the children
 * so the parent can't drift out of sync with them.
 */
#define TLC6C59XX_CHILD_NGPIOS_LIST(inst) \
	DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP_VARGS(inst, DT_PROP, (,), ngpios)
/* Use the first child to determine the serial word width. */
#define TLC6C59XX_WORD_SIZE(inst) GET_ARG_N(1, TLC6C59XX_CHILD_NGPIOS_LIST(inst))

#define TLC6C59XX_CHECK_CHILD(node_id, inst)                                                   \
	BUILD_ASSERT(DT_PROP(node_id, ngpios) == TLC6C59XX_WORD_SIZE(inst),                     \
		     "all TLC6C59XX chips in one chain must have the same ngpios");            \
	BUILD_ASSERT(DT_REG_ADDR(node_id) < DT_INST_CHILD_NUM_STATUS_OKAY(inst),               \
		     "TLC6C59XX chain position must be smaller than the chain length");

#define TLC6C59XX_INIT(inst)                                                                 \
	BUILD_ASSERT(DT_INST_CHILD_NUM_STATUS_OKAY(inst) > 0U,                                  \
		     "missing children nodes for TLC6C59XX driver");                          \
	BUILD_ASSERT(DT_INST_CHILD_NUM_STATUS_OKAY(inst) <= UINT8_MAX,                         \
		     "TLC6C59XX chain is too long");                                          \
	BUILD_ASSERT(DT_INST_CHILD_NUM(inst) == DT_INST_CHILD_NUM_STATUS_OKAY(inst),           \
		     "all physical TLC6C59XX chips must be enabled");                         \
	DT_INST_FOREACH_CHILD_STATUS_OKAY_VARGS(inst, TLC6C59XX_CHECK_CHILD, inst)              \
	static const uint8_t positions_##inst[] = {                                           \
		DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP(inst, DT_REG_ADDR, (,))                  \
	};                                                                                  \
	static uint16_t output_values_##inst[DT_INST_CHILD_NUM_STATUS_OKAY(inst)];             \
	static uint8_t packed_buffer_##inst[DIV_ROUND_UP(                                     \
		TLC6C59XX_WORD_SIZE(inst) * DT_INST_CHILD_NUM_STATUS_OKAY(inst), 8)];           \
	static struct tlc6c59xx_data tlc6c59xx_data_##inst = {                                  \
		.output_values = output_values_##inst,                                       \
		.packed_buffer = packed_buffer_##inst,                                       \
	};                                                                                  \
	static const struct tlc6c59xx_config tlc6c59xx_cfg_##inst = {                           \
		.bus = SPI_DT_SPEC_INST_GET(                                                 \
			inst, SPI_OP_MODE_CONTROLLER | SPI_TRANSFER_MSB | SPI_WORD_SET(8)),   \
		.gpio_reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),               \
		.positions = positions_##inst,                                              \
		.chain_length = DT_INST_CHILD_NUM_STATUS_OKAY(inst),                         \
		.word_size = TLC6C59XX_WORD_SIZE(inst),                                      \
	};                                                                                  \
	DEVICE_DT_INST_DEFINE(inst, tlc6c59xx_init, NULL, &tlc6c59xx_data_##inst,                \
			      &tlc6c59xx_cfg_##inst, POST_KERNEL, CONFIG_MFD_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(TLC6C59XX_INIT)

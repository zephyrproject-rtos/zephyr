/*
 * MIPI DBI Type B (write only) driver using PIO
 *
 * Copyright 2025 Christoph Schnetzler
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT raspberrypi_pico_mipi_dbi_pio

#include <hardware/pio.h>
#include <hardware/dma.h>

#include <zephyr/dt-bindings/dma/rpi-pico-dma-common.h>
#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/misc/pio_rpi_pico/pio_rpi_pico.h>
#include <zephyr/logging/log.h>

#ifdef pio2
#include <zephyr/dt-bindings/dma/rpi-pico-dma-rp2350.h>
#endif

LOG_MODULE_REGISTER(mipi_dbi_pico_pio, CONFIG_MIPI_DBI_LOG_LEVEL);

/* The MIPI DBI spec allows 8, 9, and 16 bits */
#define MIPI_DBI_MAX_DATA_BUS_WIDTH 16

/* max 4 splits as max 4 state machines per PIO instance */
#define MIPI_DBI_MAX_SPLITS 4

#define CLK_DIV           CONFIG_MIPI_DBI_RPI_PICO_PIO_CLOCK_DIV
#define SIDESET_BIT_COUNT 2
#define CYCLES_PER_BIT    4
#define SM_SEND_BIT_DURATION_US                                                                    \
	MAX(((1e6 * CYCLES_PER_BIT * CLK_DIV) / CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC), 1)

struct mipi_dbi_pico_pio_config {
	const struct device *dev_dma;
	const struct device *dev_pio;
	struct k_msgq *msq_dma;

	/* Parallel 8080 data GPIOs */
	const struct gpio_dt_spec *data;
	const uint8_t data_bus_width;

	/* Write (type B) GPIO */
	const struct gpio_dt_spec wr;

	/* Chip-select GPIO */
	const struct gpio_dt_spec cs;

	/* Command/Data GPIO */
	const struct gpio_dt_spec cmd_data;

	/* Reset GPIO */
	const struct gpio_dt_spec reset;
};

struct mipi_dbi_pico_pio_dma {
	int channel;
	struct dma_config config;
	struct dma_block_config head_block;
};

struct mipi_dbi_pico_pio_sm {
	size_t sm;
	uint32_t wrap;
	uint32_t wrap_target;
	struct pio_program program;
	uint16_t program_instructions[3];
};

struct mipi_dbi_pico_pio_split {
	uint8_t pin_count;
	int pin_base;
	int pin_discarded;
	const struct device *port;
	struct mipi_dbi_pico_pio_sm sm;
	struct mipi_dbi_pico_pio_dma dma;
};

struct mipi_dbi_pico_pio_data {
	PIO pio;
	struct k_mutex lock;
	uint32_t sm_mask;
	uint8_t split_count;
	struct mipi_dbi_pico_pio_split split[MIPI_DBI_MAX_SPLITS];
};

static void mipi_dbi_pio_dma_handler(const struct device *dev, void *user_data, uint32_t channel,
				     int status)
{
	const struct device *dev_mipi_dbi = (const struct device *)user_data;
	const struct mipi_dbi_pico_pio_config *config = dev_mipi_dbi->config;
	struct mipi_dbi_pico_pio_data *data = dev_mipi_dbi->data;

	for (int i = 0; i < data->split_count; ++i) {
		if (data->split[i].dma.channel == channel) {
			k_msgq_put(config->msq_dma, &channel, K_NO_WAIT);
		}
	}
}

static bool mipi_dbi_pio_get_dma_slot(PIO pio, size_t sm, uint32_t *dma_slot)
{
	struct dma_slot_map {
		PIO pio;
		size_t sm;
		uint32_t dma_slot;
	};
	static struct dma_slot_map dma_slots[NUM_PIO_STATE_MACHINES * NUM_PIOS] = {
		/* clang-format off */
		{pio0, 0, RPI_PICO_DMA_SLOT_PIO0_TX0},
		{pio0, 1, RPI_PICO_DMA_SLOT_PIO0_TX1},
		{pio0, 2, RPI_PICO_DMA_SLOT_PIO0_TX2},
		{pio0, 3, RPI_PICO_DMA_SLOT_PIO0_TX3},
		{pio1, 0, RPI_PICO_DMA_SLOT_PIO1_TX0},
		{pio1, 1, RPI_PICO_DMA_SLOT_PIO1_TX1},
		{pio1, 2, RPI_PICO_DMA_SLOT_PIO1_TX2},
		{pio1, 3, RPI_PICO_DMA_SLOT_PIO1_TX3},
#ifdef pio2
		{pio2, 0, RPI_PICO_DMA_SLOT_PIO2_TX0},
		{pio2, 1, RPI_PICO_DMA_SLOT_PIO2_TX1},
		{pio2, 2, RPI_PICO_DMA_SLOT_PIO2_TX2},
		{pio2, 3, RPI_PICO_DMA_SLOT_PIO2_TX3},
#endif
		/* clang-format on */
	};
	static_assert(ARRAY_SIZE(dma_slots) == NUM_PIOS * NUM_PIO_STATE_MACHINES);

	for (int i = 0; i < NUM_PIO_STATE_MACHINES * NUM_PIOS; ++i) {
		if (dma_slots[i].pio == pio && dma_slots[i].sm == sm) {
			*dma_slot = dma_slots[i].dma_slot;
			return true;
		}
	}

	return false;
}

static int mipi_dbi_pio_setup_dma(const struct device *dev, struct mipi_dbi_pico_pio_split *split)
{
	split->dma.channel = dma_claim_unused_channel(true);
	if (split->dma.channel == -1) {
		return -EPROTO;
	}

	struct mipi_dbi_pico_pio_data *data = dev->data;

	uint32_t dma_slot;

	if (!mipi_dbi_pio_get_dma_slot(data->pio, split->sm.sm, &dma_slot)) {
		assert("no dma slot found for given pio and sm");
		return -EPROTO;
	}
	split->dma.config.dma_slot = dma_slot;

	/* memory to peripheral */
	split->dma.config.channel_direction = MEMORY_TO_PERIPHERAL;
	split->dma.config.complete_callback_en = false;
	split->dma.config.error_callback_dis = false;
	/* either true == high, or false == default */
	split->dma.config.channel_priority = false;
	/* src and dest size have to be the same */
	split->dma.config.source_data_size = 1;
	split->dma.config.dest_data_size = 1;
	split->dma.config.block_count = 1;
	split->dma.head_block.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	split->dma.head_block.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	split->dma.head_block.dest_address = (uint32_t)&data->pio->txf[split->sm.sm];
	split->dma.config.head_block = &split->dma.head_block;
	split->dma.config.user_data = (void *)dev;
	split->dma.config.dma_callback = mipi_dbi_pio_dma_handler;

	return 0;
}

static int mipi_dbi_pio_count_splits(const struct mipi_dbi_pico_pio_config *dev_cfg,
				     struct mipi_dbi_pico_pio_split *splits)
{
	int offset = 0;

	splits[offset].port = dev_cfg->data[0].port;
	splits[offset].pin_base = dev_cfg->data[0].pin;
	splits[offset].pin_count = 1;

	for (int i = 1; i < dev_cfg->data_bus_width; ++i) {
		if (dev_cfg->data[i].pin == splits[offset].pin_base + splits[offset].pin_count &&
		    dev_cfg->data[i].port == splits[offset].port) {
			splits[offset].pin_count++;
		} else if (++offset < MIPI_DBI_MAX_SPLITS) {
			splits[offset].port = dev_cfg->data[i].port;
			splits[offset].pin_base = dev_cfg->data[i].pin;
			splits[offset].pin_count = 1;
		} else {
			/* error, max number of splits reached! */
			return -1;
		}
	}

	return offset + 1;
}

#define CONFIG_SM_PROGRAM(sm_index, _wrap_target, _wrap, _length)                                  \
	split = &splits[sm_index];                                                                 \
	split->pin_discarded = 0;                                                                  \
	split->sm.wrap_target = (_wrap_target);                                                    \
	split->sm.wrap = (_wrap);                                                                  \
	split->sm.program.length = (_length);                                                      \
	split->sm.program.origin = -1;                                                             \
	split->sm.program.instructions = split->sm.program_instructions;

#define SET_SM_PROGRAM(sm_index, _pin_discarded)                                                   \
	CONFIG_SM_PROGRAM(sm_index, 0, 2, 3)                                                       \
	split->pin_discarded = (_pin_discarded);                                                   \
	/*  0: pull   ifempty block */                                                             \
	split->sm.program_instructions[0] = 0x80e0;                                                \
	/*  1: out    null, #discarded_pins */                                                     \
	split->sm.program_instructions[1] = 0x6060 | split->pin_discarded;                         \
	/*  2: out    pins, #pins [1] */                                                           \
	split->sm.program_instructions[2] = 0x6100 | split->pin_count;

static void mipi_dbi_pio_configure_program(struct mipi_dbi_pico_pio_split *splits, int split_count)
{
	struct mipi_dbi_pico_pio_split *split = NULL;

	switch (split_count) {
	case 4:
		SET_SM_PROGRAM(3, splits[0].pin_count + splits[1].pin_count + splits[2].pin_count)
		__fallthrough;
	case 3:
		SET_SM_PROGRAM(2, splits[0].pin_count + splits[1].pin_count)
		__fallthrough;
	case 2:
		SET_SM_PROGRAM(1, splits[0].pin_count)
		__fallthrough;
	case 1:
		CONFIG_SM_PROGRAM(0, 0, 1, 2)
		/*  0: pull   ifempty block   side 1 [1] */
		split->sm.program_instructions[0] = 0x99e0;
		/*  1: out    pins, #pins     side 0 [1] */
		split->sm.program_instructions[1] = 0x7100 | split->pin_count;
		__fallthrough;
	default:
		break;
	}
}

static int mipi_dbi_pio_configure(const struct device *dev)
{
	const struct mipi_dbi_pico_pio_config *dev_cfg = dev->config;
	struct mipi_dbi_pico_pio_data *data = dev->data;

	data->pio = pio_rpi_pico_get_pio(dev_cfg->dev_pio);

	data->split_count = mipi_dbi_pio_count_splits(dev_cfg, data->split);
	if (data->split_count <= 0) {
		return -EPROTO;
	}

	mipi_dbi_pio_configure_program(data->split, data->split_count);

	if (gpio_is_ready_dt(&dev_cfg->wr)) {
		pio_gpio_init(data->pio, dev_cfg->wr.pin);
	}

	for (int i = 0; i < data->split_count; ++i) {
		struct mipi_dbi_pico_pio_split *p_split = &data->split[i];

		int rc = pio_rpi_pico_allocate_sm(dev_cfg->dev_pio, &p_split->sm.sm);

		if (rc < 0) {
			return rc;
		}

		for (int j = 0; j < p_split->pin_count; ++j) {
			pio_gpio_init(data->pio, p_split->pin_base + j);
		}

		rc = pio_sm_set_consecutive_pindirs(data->pio, p_split->sm.sm, p_split->pin_base,
						    p_split->pin_count, true);
		if (rc < 0) {
			return rc;
		}

		if (i == 0) {
			rc = pio_sm_set_consecutive_pindirs(data->pio, p_split->sm.sm,
							    dev_cfg->wr.pin, 1, true);
			if (rc < 0) {
				return rc;
			}
		}

		uint32_t offset = pio_add_program(data->pio, &p_split->sm.program);

		pio_sm_config sm_config = pio_get_default_sm_config();

		sm_config_set_out_pins(&sm_config, p_split->pin_base, p_split->pin_count);
		sm_config_set_clkdiv_int_frac(&sm_config, CLK_DIV, 0);
		sm_config_set_fifo_join(&sm_config, PIO_FIFO_JOIN_TX);
		sm_config_set_wrap(&sm_config, offset + p_split->sm.wrap_target,
				   offset + p_split->sm.wrap);
		sm_config_set_out_shift(&sm_config, true, true,
					p_split->pin_count + p_split->pin_discarded);

		if (i == 0) {
			sm_config_set_sideset(&sm_config, SIDESET_BIT_COUNT, true, false);
			sm_config_set_sideset_pins(&sm_config, dev_cfg->wr.pin);
		}

		rc = pio_sm_init(data->pio, p_split->sm.sm, offset, &sm_config);
		if (rc < 0) {
			return rc;
		}
		WRITE_BIT(data->sm_mask, p_split->sm.sm, true);

		rc = mipi_dbi_pio_setup_dma(dev, p_split);
		if (rc < 0) {
			return rc;
		}
	}

	return 0;
}

static bool mipi_dbi_pico_pio_all_fifos_empty(struct mipi_dbi_pico_pio_data *data)
{
	bool empty = true;

	for (int i = 0; i < data->split_count; ++i) {
		empty &= pio_sm_is_tx_fifo_empty(data->pio, data->split[i].sm.sm);
	}
	return empty;
}

static void mipi_dbi_pico_pio_wait_end_of_transmission(struct mipi_dbi_pico_pio_data *data)
{
	while (!mipi_dbi_pico_pio_all_fifos_empty(data)) {
	};
	/*
	 * When the fifo is empty, the last data byte is in the pio's statemachine.
	 * We need to wait until this one is processed before continuing.
	 */
	k_busy_wait(SM_SEND_BIT_DURATION_US);
}

static int mipi_dbi_pico_pio_write_helper(const struct device *dev,
					  const struct mipi_dbi_config *dbi_config,
					  bool cmd_present, uint8_t cmd, const uint8_t *data_buf,
					  size_t len)
{
	int ret = 0;

	const struct mipi_dbi_pico_pio_config *config = dev->config;
	struct mipi_dbi_pico_pio_data *data = dev->data;

	ret = k_mutex_lock(&data->lock, K_FOREVER);
	if (ret < 0) {
		return ret;
	}

	switch (dbi_config->mode) {
	case MIPI_DBI_MODE_8080_BUS_8_BIT:
	case MIPI_DBI_MODE_8080_BUS_9_BIT:
	case MIPI_DBI_MODE_8080_BUS_16_BIT:
		gpio_pin_set_dt(&config->cs, 1);
		if (cmd_present) {
			pio_set_sm_mask_enabled(data->pio, data->sm_mask, false);
			gpio_pin_set_dt(&config->cmd_data, 0);
			for (int i = 0; i < data->split_count; ++i) {
				pio_sm_put_blocking(data->pio, data->split[i].sm.sm, (uint32_t)cmd);
			}
			pio_enable_sm_mask_in_sync(data->pio, data->sm_mask);
			mipi_dbi_pico_pio_wait_end_of_transmission(data);
			gpio_pin_set_dt(&config->cmd_data, 1);
		}
		if (len > 0) {
			k_msgq_purge(config->msq_dma);

			pio_set_sm_mask_enabled(data->pio, data->sm_mask, false);

			uint32_t dma_channel_mask = 0;

			for (int i = 0; i < data->split_count; ++i) {
				WRITE_BIT(dma_channel_mask, data->split[i].dma.channel, true);
				data->split[i].dma.head_block.block_size = len;
				data->split[i].dma.head_block.source_address =
					(uint32_t)&data_buf[0];
				dma_config(config->dev_dma, data->split[i].dma.channel,
					   &data->split[i].dma.config);
				dma_start(config->dev_dma, data->split[i].dma.channel);
			}

			pio_enable_sm_mask_in_sync(data->pio, data->sm_mask);

			/* wait for end of dma(s) */
			int channel;

			while (dma_channel_mask != 0) {
				k_msgq_get(config->msq_dma, &channel, K_FOREVER);
				WRITE_BIT(dma_channel_mask, channel, false);
			}

			mipi_dbi_pico_pio_wait_end_of_transmission(data);
		}
		gpio_pin_set_dt(&config->cs, 0);
		break;

	default:
		LOG_ERR("MIPI DBI mode %u is not supported.", dbi_config->mode);
		ret = -ENOTSUP;
	}

	k_mutex_unlock(&data->lock);
	return ret;
}

static int mipi_dbi_pico_pio_command_write(const struct device *dev,
					   const struct mipi_dbi_config *dbi_config, uint8_t cmd,
					   const uint8_t *data_buf, size_t len)
{
	return mipi_dbi_pico_pio_write_helper(dev, dbi_config, true, cmd, data_buf, len);
}

static int mipi_dbi_pico_pio_write_display(const struct device *dev,
					   const struct mipi_dbi_config *dbi_config,
					   const uint8_t *framebuf,
					   struct display_buffer_descriptor *desc,
					   enum display_pixel_format pixfmt)
{
	ARG_UNUSED(pixfmt);

	return mipi_dbi_pico_pio_write_helper(dev, dbi_config, false, 0x0, framebuf,
					      desc->buf_size);
}

static int mipi_dbi_pico_pio_reset(const struct device *dev, k_timeout_t delay)
{
	const struct mipi_dbi_pico_pio_config *config = dev->config;
	int ret;

	LOG_DBG("Resetting the display.");

	ret = gpio_pin_set_dt(&config->reset, 1);
	if (ret < 0) {
		return ret;
	}
	k_sleep(delay);
	return gpio_pin_set_dt(&config->reset, 0);
}

static int mipi_dbi_pico_pio_init(const struct device *dev)
{
	const struct mipi_dbi_pico_pio_config *config = dev->config;
	const char *failed_pin = NULL;
	int ret = 0;

	ret = mipi_dbi_pio_configure(dev);
	if (ret < 0) {
		LOG_ERR("Failed to configure PIOs");
		return ret;
	}

	if (gpio_is_ready_dt(&config->cmd_data)) {
		ret = gpio_pin_configure_dt(&config->cmd_data, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			failed_pin = "cmd_data";
			goto fail;
		}
		gpio_pin_set_dt(&config->cmd_data, 0);
	}
	if (gpio_is_ready_dt(&config->cs)) {
		ret = gpio_pin_configure_dt(&config->cs, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			failed_pin = "cs";
			goto fail;
		}
		gpio_pin_set_dt(&config->cs, 0);
	}
	if (gpio_is_ready_dt(&config->reset)) {
		ret = gpio_pin_configure_dt(&config->reset, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			failed_pin = "reset";
			goto fail;
		}
		gpio_pin_set_dt(&config->reset, 0);
	}

	return ret;
fail:
	LOG_ERR("Failed to configure %s GPIO pin.", failed_pin);
	return ret;
}

static DEVICE_API(mipi_dbi, mipi_dbi_pico_pio_driver_api) = {
	.reset = mipi_dbi_pico_pio_reset,
	.command_write = mipi_dbi_pico_pio_command_write,
	.write_display = mipi_dbi_pico_pio_write_display};

#define PIO_MIPI_DBI_INIT(n)                                                                       \
	K_MSGQ_DEFINE(dma_msgq##n, sizeof(int), MIPI_DBI_MAX_SPLITS, 4);                           \
                                                                                                   \
	static const struct gpio_dt_spec data_bus_gpios_##n[] = {                                  \
		DT_INST_FOREACH_PROP_ELEM_SEP(n, data_gpios, GPIO_DT_SPEC_GET_BY_IDX, (,))};       \
                                                                                                   \
	static const struct mipi_dbi_pico_pio_config mipi_dbi_pico_pio_config_##n = {              \
		.dev_dma = DEVICE_DT_GET(DT_NODELABEL(dma)),                                       \
		.dev_pio = DEVICE_DT_GET(DT_INST_PARENT(n)),                                       \
		.msq_dma = &dma_msgq##n,                                                           \
		.data = data_bus_gpios_##n,                                                        \
		.data_bus_width = DT_INST_PROP_LEN(n, data_gpios),                                 \
		.wr = GPIO_DT_SPEC_INST_GET_OR(n, wr_gpios, {}),                                   \
		.cs = GPIO_DT_SPEC_INST_GET_OR(n, cs_gpios, {}),                                   \
		.cmd_data = GPIO_DT_SPEC_INST_GET_OR(n, dc_gpios, {}),                             \
		.reset = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {}),                             \
	};                                                                                         \
	BUILD_ASSERT(DT_INST_PROP_LEN(n, data_gpios) <= MIPI_DBI_MAX_DATA_BUS_WIDTH,               \
		     "Number of data GPIOs in DT exceeds MIPI_DBI_MAX_DATA_BUS_WIDTH");            \
	static struct mipi_dbi_pico_pio_data mipi_dbi_pico_pio_data_##n;                           \
	DEVICE_DT_INST_DEFINE(n, mipi_dbi_pico_pio_init, NULL, &mipi_dbi_pico_pio_data_##n,        \
			      &mipi_dbi_pico_pio_config_##n, POST_KERNEL,                          \
			      CONFIG_MIPI_DBI_INIT_PRIORITY, &mipi_dbi_pico_pio_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PIO_MIPI_DBI_INIT)

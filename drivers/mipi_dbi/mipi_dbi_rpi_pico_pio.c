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

#if defined(CONFIG_SOC_SERIES_RP2350)
#include <zephyr/dt-bindings/dma/rpi-pico-dma-rp2350.h>
#endif

LOG_MODULE_REGISTER(mipi_dbi_pico_pio, CONFIG_MIPI_DBI_LOG_LEVEL);

/* The MIPI DBI spec allows 8, 9, and 16 bits */
#define MIPI_DBI_MAX_DATA_BUS_WIDTH 16

/* max splits limited by max state machines per PIO instance */
#define MIPI_DBI_MAX_SPLITS NUM_PIO_STATE_MACHINES

/* TODO: make this DT dependent? */
#define PIO_INTERRUPT_NUM 0

struct mipi_dbi_pico_pio_sideset {
	const int base_pin;
	const uint8_t bit_count;
	const bool optional;
};

struct mipi_dbi_pico_pio_config {
	const struct device *dev_dma;
	const struct device *dev_pio;
	struct k_msgq *msq;
	void (*irq_config_func)(void);
	int (*pio_tx_func)(const struct device *dev, bool cmd_present, uint8_t cmd,
			   const uint8_t *data_buf, size_t len);

	/* PIO clock divider */
	const uint16_t pio_clock_div;

	/* Parallel data GPIOs splits */
	const uint8_t split_count;
	struct mipi_dbi_pico_pio_split *splits;
	const struct mipi_dbi_pico_pio_sideset *sideset;

	/* If CS, DC and WR are consecutive */
	const bool ctrl_pins_consecutive;

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
	pio_sm_config sm_config;
	uint8_t initial_pc;
	uint32_t wrap;
	uint32_t wrap_target;
	struct pio_program program;
};

struct mipi_dbi_pico_pio_split {
	const uint8_t pin_count;
	const int pin_base;
	int pin_discarded;
	struct mipi_dbi_pico_pio_sm *sm;
	struct mipi_dbi_pico_pio_dma dma;
};

struct mipi_dbi_pico_pio_data {
	PIO pio;
	struct k_mutex lock;
	uint32_t sm_mask;
};

/*
 * The DMA interrupt handler only puts a message if an error is reported. This to
 * prevent stalling of the thread.
 */
static void mipi_dbi_pio_dma_irq_handler(const struct device *dev, void *user_data,
					 uint32_t channel, int status)
{
	ARG_UNUSED(dev);

	if (status >= 0) {
		return;
	}

	const struct mipi_dbi_pico_pio_config *config = ((const struct device *)user_data)->config;

	for (int i = 0; i < config->split_count; ++i) {
		if (config->splits[i].dma.channel == channel) {
			k_msgq_put(config->msq, &channel, K_NO_WAIT);
		}
	}
}

/*
 * PIO interrupt handler puts a message as soon as the statemachine
 * has finished the transmission.
 */
static void mipi_dbi_pio_pio_irq_handler(const struct device *dev)
{
	int status = 0;
	const struct mipi_dbi_pico_pio_config *config = dev->config;
	struct mipi_dbi_pico_pio_data *data = dev->data;

	if (pio_interrupt_get(data->pio, PIO_INTERRUPT_NUM)) {
		pio_interrupt_clear(data->pio, PIO_INTERRUPT_NUM);
		k_msgq_put(config->msq, &status, K_NO_WAIT);
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
#if defined(CONFIG_SOC_SERIES_RP2350)
		{pio2, 0, RPI_PICO_DMA_SLOT_PIO2_TX0},
		{pio2, 1, RPI_PICO_DMA_SLOT_PIO2_TX1},
		{pio2, 2, RPI_PICO_DMA_SLOT_PIO2_TX2},
		{pio2, 3, RPI_PICO_DMA_SLOT_PIO2_TX3},
#endif
		/* clang-format on */
	};
	static_assert(ARRAY_SIZE(dma_slots) == NUM_PIOS * NUM_PIO_STATE_MACHINES,
		      "dma slots mismatch!");

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

	if (!mipi_dbi_pio_get_dma_slot(data->pio, split->sm->sm, &dma_slot)) {
		assert("no dma slot found for given pio and sm");
		return -EPROTO;
	}
	split->dma.config.dma_slot = dma_slot;
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
	split->dma.head_block.dest_address = (uint32_t)&data->pio->txf[split->sm->sm];
	split->dma.config.head_block = &split->dma.head_block;
	split->dma.config.user_data = (void *)dev;
	split->dma.config.dma_callback = mipi_dbi_pio_dma_irq_handler;

	return 0;
}

static int mipi_dbi_pio_configure(const struct device *dev)
{
	int rc = 0;
	const struct mipi_dbi_pico_pio_config *dev_cfg = dev->config;
	struct mipi_dbi_pico_pio_data *data = dev->data;

	data->pio = pio_rpi_pico_get_pio(dev_cfg->dev_pio);

	if (dev_cfg->ctrl_pins_consecutive) {
		pio_gpio_init(data->pio, dev_cfg->cs.pin);
		pio_gpio_init(data->pio, dev_cfg->cmd_data.pin);
	}
	pio_gpio_init(data->pio, dev_cfg->wr.pin);

	for (int i = 0; i < dev_cfg->split_count; ++i) {
		struct mipi_dbi_pico_pio_split *p_split = &dev_cfg->splits[i];

		rc = pio_rpi_pico_allocate_sm(dev_cfg->dev_pio, &p_split->sm->sm);
		if (rc < 0) {
			return rc;
		}

		for (int j = 0; j < p_split->pin_count; ++j) {
			pio_gpio_init(data->pio, p_split->pin_base + j);
		}

		rc = pio_sm_set_consecutive_pindirs(data->pio, p_split->sm->sm, p_split->pin_base,
						    p_split->pin_count, true);
		if (rc < 0) {
			return rc;
		}

		if (i == 0) {
			rc = pio_sm_set_consecutive_pindirs(data->pio, p_split->sm->sm,
							    dev_cfg->sideset->base_pin,
							    dev_cfg->sideset->bit_count, true);
			if (rc < 0) {
				return rc;
			}
		}

		p_split->sm->initial_pc = pio_add_program(data->pio, &p_split->sm->program);
		p_split->sm->sm_config = pio_get_default_sm_config();

		sm_config_set_out_pins(&p_split->sm->sm_config, p_split->pin_base,
				       p_split->pin_count);
		sm_config_set_clkdiv_int_frac(&p_split->sm->sm_config, dev_cfg->pio_clock_div, 0);
		sm_config_set_fifo_join(&p_split->sm->sm_config, PIO_FIFO_JOIN_TX);
		sm_config_set_wrap(&p_split->sm->sm_config,
				   p_split->sm->initial_pc + p_split->sm->wrap_target,
				   p_split->sm->initial_pc + p_split->sm->wrap);
		sm_config_set_out_shift(&p_split->sm->sm_config, true, true,
					p_split->pin_count + p_split->pin_discarded);

		if (i == 0) {
			sm_config_set_sideset(&p_split->sm->sm_config,
					      dev_cfg->sideset->bit_count +
						      dev_cfg->sideset->optional,
					      dev_cfg->sideset->optional, false);
			sm_config_set_sideset_pins(&p_split->sm->sm_config,
						   dev_cfg->sideset->base_pin);
		}

		rc = pio_sm_init(data->pio, p_split->sm->sm, p_split->sm->initial_pc,
				 &p_split->sm->sm_config);
		if (rc < 0) {
			return rc;
		}

		WRITE_BIT(data->sm_mask, p_split->sm->sm, true);

		rc = mipi_dbi_pio_setup_dma(dev, p_split);
		if (rc < 0) {
			return rc;
		}
	}

	dev_cfg->irq_config_func();
	pio_set_irq0_source_enabled(data->pio, pis_interrupt0, true);

	return 0;
}

static int mipi_dbi_pico_pio_start_wait_reset_sm(const struct device *dev)
{
	int ret = 0;

	const struct mipi_dbi_pico_pio_config *config = dev->config;
	struct mipi_dbi_pico_pio_data *data = dev->data;

	pio_enable_sm_mask_in_sync(data->pio, data->sm_mask);

	/* Wait for interrupt from state machine or dma, in case of an error */
	k_msgq_get(config->msq, &ret, K_FOREVER);

	/* reset pio state machines to be in a proper state for the next transmission */
	for (int i = 0; i < config->split_count; ++i) {
		pio_sm_init(data->pio, config->splits[i].sm->sm, config->splits[i].sm->initial_pc,
			    &config->splits[i].sm->sm_config);
	}

	k_msgq_purge(config->msq);

	return ret;
}

static void mipi_dbi_pico_pio_load_dma(const struct mipi_dbi_pico_pio_config *config,
				       struct mipi_dbi_pico_pio_split *split,
				       const uint8_t *data_buf, size_t len)
{
	split->dma.head_block.block_size = len;
	split->dma.head_block.source_address = (uint32_t)&data_buf[0];

	dma_config(config->dev_dma, split->dma.channel, &split->dma.config);
	dma_start(config->dev_dma, split->dma.channel);
}

static int mipi_dbi_pico_pio_tx_ctrl_pins_non_consecutive(const struct device *dev,
							  bool cmd_present, uint8_t cmd,
							  const uint8_t *data_buf, size_t len)
{
	int ret = 0;
	const struct mipi_dbi_pico_pio_config *config = dev->config;
	struct mipi_dbi_pico_pio_data *data = dev->data;

	gpio_pin_set_dt(&config->cs, 1);

	if (cmd_present) {
		/* Set data length to 0 to only transmit cmd */
		pio_sm_put_blocking(data->pio, config->splits[0].sm->sm, 0);

		for (int i = 0; i < config->split_count; ++i) {
			pio_sm_put_blocking(data->pio, config->splits[i].sm->sm, cmd_present);
			pio_sm_put_blocking(data->pio, config->splits[i].sm->sm, (uint32_t)cmd);
		}

		gpio_pin_set_dt(&config->cmd_data, 0);
		ret = mipi_dbi_pico_pio_start_wait_reset_sm(dev);
		gpio_pin_set_dt(&config->cmd_data, 1);
	}

	if (len > 0) {
		pio_sm_put_blocking(data->pio, config->splits[0].sm->sm, len);
		for (int i = 0; i < config->split_count; ++i) {
			/* Set cmd present to 0 to only transmit data */
			pio_sm_put_blocking(data->pio, config->splits[i].sm->sm, 0);
			mipi_dbi_pico_pio_load_dma(config, &config->splits[i], data_buf, len);
		}
		ret = mipi_dbi_pico_pio_start_wait_reset_sm(dev);
	}

	gpio_pin_set_dt(&config->cs, 0);

	return ret;
}

static int mipi_dbi_pico_pio_tx_ctrl_pins_consecutive(const struct device *dev, bool cmd_present,
						      uint8_t cmd, const uint8_t *data_buf,
						      size_t len)
{
	int ret = 0;
	const struct mipi_dbi_pico_pio_config *config = dev->config;
	struct mipi_dbi_pico_pio_data *data = dev->data;

	pio_sm_put_blocking(data->pio, config->splits[0].sm->sm, len);

	for (int i = 0; i < config->split_count; ++i) {
		pio_sm_put_blocking(data->pio, config->splits[i].sm->sm, cmd_present);

		if (cmd_present) {
			pio_sm_put_blocking(data->pio, config->splits[i].sm->sm, (uint32_t)cmd);
		}

		if (len > 0) {
			mipi_dbi_pico_pio_load_dma(config, &config->splits[i], data_buf, len);
		}
	}

	ret = mipi_dbi_pico_pio_start_wait_reset_sm(dev);

	return ret;
}

static int mipi_dbi_pico_pio_write_helper(const struct device *dev,
					  const struct mipi_dbi_config *dbi_config,
					  bool cmd_present, uint8_t cmd, const uint8_t *data_buf,
					  size_t len)
{
	int ret = 0;
	const struct mipi_dbi_pico_pio_config *config = dev->config;
	struct mipi_dbi_pico_pio_data *data = dev->data;

	/* early return if nothing to do */
	if (!cmd_present && len == 0) {
		return ret;
	}

	ret = k_mutex_lock(&data->lock, K_FOREVER);
	if (ret < 0) {
		return ret;
	}

	switch (dbi_config->mode) {
	case MIPI_DBI_MODE_8080_BUS_8_BIT:
	case MIPI_DBI_MODE_8080_BUS_9_BIT:
	case MIPI_DBI_MODE_8080_BUS_16_BIT:
		ret = config->pio_tx_func(dev, cmd_present, cmd, data_buf, len);
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

	if (!gpio_is_ready_dt(&config->cmd_data) || !gpio_is_ready_dt(&config->cs) ||
	    !gpio_is_ready_dt(&config->reset) || !gpio_is_ready_dt(&config->wr)) {
		LOG_ERR("GPIO pin(s) not ready");
		ret = -EIO;
		return ret;
	}

	ret = mipi_dbi_pio_configure(dev);
	if (ret < 0) {
		LOG_ERR("Failed to configure PIOs");
		return ret;
	}

	ret = gpio_pin_configure_dt(&config->reset, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		failed_pin = "reset";
		goto fail;
	}
	gpio_pin_set_dt(&config->reset, 0);

	if (!config->ctrl_pins_consecutive) {
		ret = gpio_pin_configure_dt(&config->cmd_data, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			failed_pin = "cmd_data";
			goto fail;
		}
		gpio_pin_set_dt(&config->cmd_data, 0);

		ret = gpio_pin_configure_dt(&config->cs, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			failed_pin = "cs";
			goto fail;
		}
		gpio_pin_set_dt(&config->cs, 0);
	}

	return ret;
fail:
	LOG_ERR("Failed to configure %s GPIO pin.", failed_pin);
	return ret;
}

static DEVICE_API(mipi_dbi, mipi_dbi_pico_pio_driver_api) = {
	.reset = mipi_dbi_pico_pio_reset,
	.command_write = mipi_dbi_pico_pio_command_write,
	.write_display = mipi_dbi_pico_pio_write_display,
};

#define CS_POS(node_id) DT_GPIO_PIN(node_id, cs_gpios) - MIN_CTRL_PIN(node_id)
#define DC_POS(node_id) DT_GPIO_PIN(node_id, dc_gpios) - MIN_CTRL_PIN(node_id)
#define WR_POS(node_id) DT_GPIO_PIN(node_id, wr_gpios) - MIN_CTRL_PIN(node_id)

#define SIDE(wr, dc, cs, ctrl_pins_consecutive, inst)                                              \
	!ctrl_pins_consecutive ? (wr)                                                              \
			       : (((wr) * (1 << (WR_POS(DT_DRV_INST(inst)))) +                     \
				   (dc) * (1 << (DC_POS(DT_DRV_INST(inst)))) +                     \
				   (cs) * (1 << (CS_POS(DT_DRV_INST(inst))))))

#define SET_BITS(bits_val, lsb) ((bits_val) << (lsb))

#define SET_BIT_COUNT(value, bit_count) (value | SET_BITS(bit_count, 0))

#define SET_DELAY_SIDESET(value, opt, side_set, side, delay)                                       \
	(value | SET_BITS(opt, 12) | SET_BITS(side, 13 - (opt) - (side_set)) | SET_BITS(delay, 8))

#define CREATE_SM(_wrap_target, _wrap, _length, _instructions)                                     \
	.wrap_target = (_wrap_target), .wrap = (_wrap), .program.length = (_length),               \
	.program.origin = -1, .program.instructions = (_instructions),

/*
 * The "1" in out pins, <1> is a placeholder and is set via pin_count. Also <number> of "side
 * <number>" is just a placeholder and replaced by DELAY_SIDESET. The side depends on various
 * things. First if the control pins (cs, dc, wr) are consecutive or not. If not, there is
 * only one side pin, wr and an optional one. If they are consecutive, there are 3 side pins, cs,
 * dc, wr. The value of side depends now on the order of the three pins which is calculated in
 * SIDE().
 *
 * To have one instruction per line, the following abbreviations are used:
 *	cpc: ctrl pins consecutive
 *	opt: sideset optional
 *	pc: pin count
 *	ss: side set
 *
 * PIO source code
 *    .side_set 3 opt
 *
 *        pull ifempty side 7
 *        out x, 32
 *        out y, 1 side 6       ; auto-pull needs an additional cycle
 *        jmp !Y data           ; no cmd, jmp to "data"
 *    cmd:
 *        nop side 4                  ; to be in sync with other sms
 *        pull ifempty [1] side 4
 *        out pins, 1 [1] side 0
 *        jmp !X end [1] side 4       ; no data, jmp to "end"
 *        jmp x-- data_loop           ; handle length == 1
 *    data_loop:
 *        pull ifempty [1] side 6
 *        out pins, 1 side 2
 *    data:
 *        jmp x-- data_loop    ; repeat until data length == 0
 *    end:
 *        irq 0 [1] side 6     ; trigger interrupt
 */
#define BASE_SM_INSTRUCTION(n, idx, cpc, pc, ss, opt)                                              \
	static uint16_t inst_##n##_program_##idx##_instructions[] = {                              \
		SET_DELAY_SIDESET(0x90e0, opt, ss, SIDE(1, 1, 1, cpc, n), 0),                      \
		0x6020,                                                                            \
		SET_DELAY_SIDESET(SET_BIT_COUNT(0x7040, pc), opt, ss, SIDE(1, 1, 0, cpc, n), 0),   \
		0x006b,                                                                            \
		SET_DELAY_SIDESET(0xb042, opt, ss, SIDE(1, 0, 0, cpc, n), 0),                      \
		SET_DELAY_SIDESET(0x90e0, opt, ss, SIDE(1, 0, 0, cpc, n), 1),                      \
		SET_DELAY_SIDESET(SET_BIT_COUNT(0x7000, pc), 1, ss, SIDE(0, 0, 0, cpc, n), 1),     \
		SET_DELAY_SIDESET(0x102c, opt, ss, SIDE(1, 0, 0, cpc, n), 1),                      \
		0x0049,                                                                            \
		SET_DELAY_SIDESET(0x90e0, opt, ss, SIDE(1, 1, 0, cpc, n), 1),                      \
		SET_DELAY_SIDESET(SET_BIT_COUNT(0x7000, pc), opt, ss, SIDE(0, 1, 0, cpc, n), 0),   \
		0x0049,                                                                            \
		SET_DELAY_SIDESET(0xd000, opt, ss, SIDE(1, 1, 0, cpc, n), 1),                      \
	};                                                                                         \
	static struct mipi_dbi_pico_pio_sm inst_##n##_sm_##idx = {                                 \
		CREATE_SM(0, 12, 13, inst_##n##_program_##idx##_instructions)};

/*
 * The SM_INSTRUCTION just follows the BASE_SM_INSTRUCTION and outputs the bits in sync.
 *
 * PIO source code:
 *        out x, 32
 *        jmp loop [3]
 *    .wrap_target
 *        jmp x-- loop [2]
 *    loop:
 *        out null, 1 [1]
 *        out pins 1
 *        jmp !X loop
 */
#define SM_INSTRUCTION(n, idx, pin_count, pin_discarded)                                           \
	static uint16_t inst_##n##_program_##idx##_instructions[] = {                              \
		SET_BIT_COUNT(0x6020, (pin_count) + (pin_discarded)),                              \
		0x0303,                                                                            \
		0x0243,                                                                            \
		SET_BIT_COUNT(0x6160, pin_discarded),                                              \
		SET_BIT_COUNT(0x6000, pin_count),                                                  \
		0x0023,                                                                            \
	};                                                                                         \
	struct mipi_dbi_pico_pio_sm inst_##n##_sm_##idx = {                                        \
		CREATE_SM(2, 5, 6, inst_##n##_program_##idx##_instructions)};

#define MIN_CTRL_PIN(node_id)                                                                      \
	MIN(DT_GPIO_PIN(node_id, cs_gpios),                                                        \
	    MIN(DT_GPIO_PIN(node_id, dc_gpios), DT_GPIO_PIN(node_id, wr_gpios)))

#define MAX_CTRL_PIN(node_id)                                                                      \
	MAX(DT_GPIO_PIN(node_id, cs_gpios),                                                        \
	    MAX(DT_GPIO_PIN(node_id, dc_gpios), DT_GPIO_PIN(node_id, wr_gpios)))

#define CTRL_PINS_CONSECUTIVE(node_id) (MAX_CTRL_PIN(node_id) - MIN_CTRL_PIN(node_id) == 2)

#define DISCARDED_PINS(node_id, prop, idx, current_idx)                                            \
	(DT_PHA_BY_IDX(node_id, prop, idx, consecutive_pins) * (idx < current_idx))

#define GET_DISCARDED_PINS_BY_IDX(node_id, prop, idx)                                              \
	DT_FOREACH_PROP_ELEM_SEP_VARGS(node_id, prop, DISCARDED_PINS, (+), idx)

#define GET_PIN_COUNT_BY_IDX(node_id, prop, idx) DT_PHA_BY_IDX(node_id, prop, idx, consecutive_pins)

#define DATA_PIN_SPLIT_GET_BY_IDX(node_id, prop, idx, n)                                           \
	{.pin_base = DT_PHA_BY_IDX(node_id, prop, idx, base_pin),                                  \
	 .pin_count = GET_PIN_COUNT_BY_IDX(node_id, prop, idx),                                    \
	 .pin_discarded = GET_DISCARDED_PINS_BY_IDX(node_id, prop, idx),                           \
	 .sm = &inst_##n##_sm_##idx},

#define CREATE_SPLIT_BY_IDX(node_id, prop, idx, n, ctrl_pins_consecutive, side_sets, optional)     \
	COND_CODE_0(idx,                                                                           \
		(BASE_SM_INSTRUCTION(n, idx, ctrl_pins_consecutive,                                \
		GET_PIN_COUNT_BY_IDX(node_id, prop, idx), side_sets, optional)),                   \
		(SM_INSTRUCTION(n, idx, GET_PIN_COUNT_BY_IDX(node_id, prop, idx),                  \
		GET_DISCARDED_PINS_BY_IDX(node_id, prop, idx))))

#define PIO_MIPI_DBI_INIT(n)                                                                       \
	BUILD_ASSERT(DT_INST_FOREACH_PROP_ELEM_SEP_VARGS(n, data_pin_splits, DT_PHA_BY_IDX, (+),   \
							 consecutive_pins) <=                      \
			     MIPI_DBI_MAX_DATA_BUS_WIDTH,                                          \
		     "Sum of consecutive_pins of data_pin_splits in DT exceeds "                   \
		     "MIPI_DBI_MAX_DATA_BUS_WIDTH");                                               \
                                                                                                   \
	BUILD_ASSERT(DT_INST_PROP_LEN(n, data_pin_splits) <= MIPI_DBI_MAX_SPLITS,                  \
		     "Number of data pin splits in DT exceeds MIPI_DBI_MAX_SPLITS");               \
                                                                                                   \
	BUILD_ASSERT(DT_INST_PROP(n, pio_clock_div) > 0 &&                                         \
			     DT_INST_PROP(n, pio_clock_div) <= UINT16_MAX,                         \
		     "pio-clock-div has to be between 1 and 65536");                               \
                                                                                                   \
	K_MSGQ_DEFINE(msgq_##n, sizeof(int), MIPI_DBI_MAX_SPLITS, 4);                              \
                                                                                                   \
	static void inst_##n##_irq_config(void)                                                    \
	{                                                                                          \
		IRQ_CONNECT(DT_IRQN(DT_INST_PARENT(n)), DT_IRQ(DT_INST_PARENT(n), priority),       \
			    mipi_dbi_pio_pio_irq_handler, DEVICE_DT_INST_GET(n), 0);               \
		irq_enable(DT_IRQN(DT_INST_PARENT(n)));                                            \
	}                                                                                          \
                                                                                                   \
	static const bool pins_consecutive_##n = CTRL_PINS_CONSECUTIVE(DT_DRV_INST(n));            \
                                                                                                   \
	static const struct mipi_dbi_pico_pio_sideset sideset_##n = {                              \
		.base_pin = pins_consecutive_##n ? MIN_CTRL_PIN(DT_DRV_INST(n))                    \
						 : DT_GPIO_PIN(DT_DRV_INST(n), wr_gpios),          \
		.bit_count = pins_consecutive_##n ? 3 : 1,                                         \
		.optional = true,                                                                  \
	};                                                                                         \
                                                                                                   \
	DT_INST_FOREACH_PROP_ELEM_VARGS(n, data_pin_splits, CREATE_SPLIT_BY_IDX, n,                \
					pins_consecutive_##n, sideset_##n.bit_count,               \
					sideset_##n.optional)                                      \
                                                                                                   \
	static struct mipi_dbi_pico_pio_split splits_##n[] = {DT_INST_FOREACH_PROP_ELEM_VARGS(     \
		n, data_pin_splits, DATA_PIN_SPLIT_GET_BY_IDX, n)};                                \
                                                                                                   \
	static const struct mipi_dbi_pico_pio_config mipi_dbi_pico_pio_config_##n = {              \
		.dev_dma = DEVICE_DT_GET(DT_NODELABEL(dma)),                                       \
		.dev_pio = DEVICE_DT_GET(DT_INST_PARENT(n)),                                       \
		.pio_clock_div = DT_INST_PROP(n, pio_clock_div),                                   \
		.split_count = DT_INST_PROP_LEN(n, data_pin_splits),                               \
		.splits = splits_##n,                                                              \
		.sideset = &sideset_##n,                                                           \
		.ctrl_pins_consecutive = pins_consecutive_##n,                                     \
		.pio_tx_func = pins_consecutive_##n                                                \
				       ? mipi_dbi_pico_pio_tx_ctrl_pins_consecutive                \
				       : mipi_dbi_pico_pio_tx_ctrl_pins_non_consecutive,           \
		.irq_config_func = inst_##n##_irq_config,                                          \
		.msq = &msgq_##n,                                                                  \
		.wr = GPIO_DT_SPEC_INST_GET(n, wr_gpios),                                          \
		.cs = GPIO_DT_SPEC_INST_GET(n, cs_gpios),                                          \
		.cmd_data = GPIO_DT_SPEC_INST_GET(n, dc_gpios),                                    \
		.reset = GPIO_DT_SPEC_INST_GET(n, reset_gpios),                                    \
	};                                                                                         \
                                                                                                   \
	static struct mipi_dbi_pico_pio_data mipi_dbi_pico_pio_data_##n;                           \
	DEVICE_DT_INST_DEFINE(n, mipi_dbi_pico_pio_init, NULL, &mipi_dbi_pico_pio_data_##n,        \
			      &mipi_dbi_pico_pio_config_##n, POST_KERNEL,                          \
			      CONFIG_MIPI_DBI_INIT_PRIORITY, &mipi_dbi_pico_pio_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PIO_MIPI_DBI_INIT)

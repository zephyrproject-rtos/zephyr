/*
 * Copyright (c) 2026 Robin Sachsenweger Ballantyne <makenenjoy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT raspberrypi_pico_i2s_pio

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/misc/pio_rpi_pico/pio_rpi_pico.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/sys/util.h>
#if defined(CONFIG_SOC_SERIES_RP2040)
#include <zephyr/dt-bindings/dma/rpi-pico-dma-rp2040.h>
#elif defined(CONFIG_SOC_SERIES_RP2350)
#include <zephyr/dt-bindings/dma/rpi-pico-dma-rp2350.h>
#endif

#include <hardware/pio.h>
#include <stdint.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2s_pico_pio, CONFIG_I2S_LOG_LEVEL);

#define PIO_I2S_NUM_INST_OK              DT_NUM_INST_STATUS_OKAY(raspberrypi_pico_i2s_pio)
#define PIO_I2S_IS_DIR_INST_EN(idx, dir) DT_INST_DMAS_HAS_NAME(idx, dir)

#define PIO_I2S_IS_DIR_EN(dir) (LISTIFY(PIO_I2S_NUM_INST_OK, PIO_I2S_IS_DIR_INST_EN, (||), dir))

struct queue_item {
	void *mem_block;
	size_t size;
};

struct pio_i2s_config {
	const struct device *piodev;
	const struct pinctrl_dev_config *pcfg;
	const struct device *clk_dev;
	clock_control_subsys_t clk_id;
	const uint32_t clock_pin;
	const uint32_t ws_pin;
	const uint32_t in_base_pin;
};

/* A PIO program loaded into instruction memory. One copy is shared by every
 * state machine running it, so the load is reference counted.
 */
struct pio_prog {
	const pio_program_t *prog; /* NULL = not loaded */
	uint32_t offset;
	uint8_t users; /* state machines currently running it */
};

struct pio_i2s_stream {
	enum i2s_state state;
	bool tx_stop_without_draining;
	struct k_msgq *msgq;
	uint32_t dma_channel;
	const struct device *dev_dma;
	struct dma_config dma_cfg;
	struct dma_block_config dma_blk_cfg;

	struct i2s_config cfg;
	void *mem_block;

	const uint32_t data_pin;

	size_t sm; /* (size_t)-1 = not claimed */
};

struct pio_i2s_data {
	struct pio_i2s_stream tx;
	struct pio_i2s_stream rx;
	uint32_t channel_length;
	uint32_t sampling_freq;
	size_t clks_sm; /* (size_t)-1 = not claimed */
	struct pio_prog clks_prog;
	struct pio_prog target_prog;
	struct k_spinlock lock;
};

static int i2s_rpi_pico_write(const struct device *dev, void *mem_block, size_t size)
{
	struct pio_i2s_data *data = dev->data;
	const struct pio_i2s_stream *stream = &data->tx;
	enum i2s_state state = stream->state;
	int retval = 0;

	if (state != I2S_STATE_RUNNING && state != I2S_STATE_READY) {
		LOG_DBG("Invalid state: %d", (int)state);
		return -EIO;
	}

	if (size > stream->cfg.block_size) {
		LOG_DBG("Max write size is: %u", stream->cfg.block_size);
		return -EIO;
	}

	struct queue_item item = {.mem_block = mem_block, .size = size};

	retval = k_msgq_put(stream->msgq, &item, SYS_TIMEOUT_MS(stream->cfg.timeout));
	if (retval < 0) {
		LOG_DBG("TX queue full");
		return retval;
	}

	return 0;
}

static int i2s_rpi_pico_read(const struct device *dev, void **mem_block, size_t *size)
{
	struct pio_i2s_data *dev_data = dev->data;
	const struct pio_i2s_stream *stream = &dev_data->rx;
	enum i2s_state state = stream->state;

	if (state == I2S_STATE_NOT_READY) {
		LOG_DBG("Invalid state: %d", (int)state);
		return -EIO;
	}

	struct queue_item item;
	int retval = k_msgq_get(stream->msgq, &item,
				(state == I2S_STATE_ERROR) ? K_NO_WAIT
							   : SYS_TIMEOUT_MS(stream->cfg.timeout));

	if (retval < 0) {
		if (retval == -ENOMSG) {
			retval = -EIO;
		}
		return retval;
	}
	*mem_block = item.mem_block;
	*size = item.size;

	return 0;
}

/* WS and BCLK program
 * 1-bit sideset = BCLK
 * `mov pins, !pins` toggles WS because OUT_BASE == IN_BASE == WS pin
 * 2 PIO cycles per BCLK period.
 */
RPI_PICO_PIO_DEFINE_PROGRAM(clks, 0, 3,
			    /*     .wrap_target */
			    0xb022, /*  0: mov    x, y            side 1 */
			    0xa042, /*  1: nop                    side 0 */
			    0x1041, /*  2: jmp    x--, 1          side 1 */
			    0xa008, /*  3: mov    pins, ~pins     side 0 */
			    /* .wrap */);

static const uint32_t clks_cycles_factor = 2u;

#define CLKS_ENTRY_POINT 0u

/* TX/RX I2S target program.
 * Each pull/push transfers one 8/16/32 bit word.
 * DMA is configured with narrow writes when words are not 32 bits long.
 */
RPI_PICO_PIO_DEFINE_PROGRAM(target, 7, 18, 0x20a2, /*  0: wait   1 pin, 2  */
			    0x2022,                /*  1: wait   0 pin, 2  */
			    0x2021,                /*  2: wait   0 pin, 1  */
			    0x20a1,                /*  3: wait   1 pin, 1  */
			    0x0026,                /*  4: jmp    !x, 6     */
			    0x0007,                /*  5: jmp    7         */
			    0x80a0,                /*  6: pull   block     */
			    /* .wrap_target                 */
			    0x2021, /*  7: wait   0 pin, 1  */
			    0x00eb, /*  8: jmp    !osre, 11 */
			    0xe000, /*  9: set    pins, 0   */
			    0x000c, /* 10: jmp    12        */
			    0x6001, /* 11: out    pins, 1   */
			    0x20a1, /* 12: wait   1 pin, 1  */
			    0x4001, /* 13: in     pins, 1   */
			    0x00d3, /* 14: jmp    pin, 19   */
			    0x0067, /* 15: jmp    !y, 7     */
			    0xa04a, /* 16: mov    y, ~y     */
			    0x0026, /* 17: jmp    !x, 6     */
			    0x8020, /* 18: push   block     */
			    /* .wrap                        */
			    0x0070, /* 19: jmp    !y, 16    */
			    0x0007, /* 20: jmp    7         */
);

/* The target PIO program needs to perform a maximum of ~8 instructions (12,13,14,15,16,17,6,7)
 * during a high BCLK which implies an upper bound on sampling frequency.
 */
static const uint32_t target_prog_instr_per_bclk_toggle = 8u;

static int prog_load(const struct device *piodev, struct pio_prog *res, const pio_program_t *prog)
{
	PIO pio = pio_rpi_pico_get_pio(piodev);

	if (res->users > 0) {
		__ASSERT_NO_MSG(res->prog == prog);
		res->users++;
		return 0;
	}

	if (!pio_can_add_program(pio, prog)) {
		LOG_ERR("no PIO instruction memory left for program");
		return -EBUSY;
	}

	res->offset = pio_add_program(pio, prog);
	res->prog = prog;
	res->users = 1;

	return 0;
}

static void prog_unload(const struct device *piodev, struct pio_prog *res)
{
	PIO pio = pio_rpi_pico_get_pio(piodev);

	__ASSERT_NO_MSG(res->users > 0);

	if (--res->users > 0) {
		return;
	}

	pio_remove_program(pio, res->prog, res->offset);
	res->prog = NULL;
}

static int sm_claim(const struct device *piodev, size_t *sm, struct pio_prog *prog_res,
		    const pio_program_t *prog)
{
	PIO pio = pio_rpi_pico_get_pio(piodev);
	int retval;

	__ASSERT_NO_MSG(*sm == (size_t)-1);

	retval = prog_load(piodev, prog_res, prog);
	if (retval < 0) {
		return retval;
	}

	retval = pio_rpi_pico_allocate_sm(piodev, sm);
	if (retval < 0) {
		prog_unload(piodev, prog_res);
		return retval;
	}

	pio_sm_set_enabled(pio, *sm, false);

	return 0;
}

static void sm_release(const struct device *piodev, size_t *sm, struct pio_prog *prog_res,
		       uint32_t out_pins)
{
	PIO pio = pio_rpi_pico_get_pio(piodev);

	if (*sm == (size_t)-1) {
		return;
	}

	pio_sm_set_enabled(pio, *sm, false);

	if (out_pins != 0) {
		pio_sm_set_pindirs_with_mask(pio, *sm, 0, out_pins);
	}

	pio_sm_unclaim(pio, *sm);
	*sm = (size_t)-1;

	prog_unload(piodev, prog_res);
}

static int sm_set_stream_and_clk(const struct device *dev, enum i2s_dir dir, bool need_clk_sm)
{
	const struct pio_i2s_config *dev_config = dev->config;
	struct pio_i2s_data *dev_data = dev->data;
	const struct device *piodev = dev_config->piodev;

	__ASSERT_NO_MSG(dir != I2S_DIR_BOTH);

	int retval;
	bool free_clk_sm_during_error = false;

	if (need_clk_sm && dev_data->clks_sm == (size_t)-1) {
		retval = sm_claim(piodev, &dev_data->clks_sm, &dev_data->clks_prog,
				  RPI_PICO_PIO_GET_PROGRAM(clks));
		if (retval < 0) {
			return -EBUSY;
		}

		free_clk_sm_during_error = true;
	}

	struct pio_i2s_stream *stream = dir == I2S_DIR_TX ? &dev_data->tx : &dev_data->rx;

	if (stream->sm == (size_t)-1) {
		retval = sm_claim(piodev, &stream->sm, &dev_data->target_prog,
				  RPI_PICO_PIO_GET_PROGRAM(target));
		if (retval < 0) {
			goto cleanup;
		}
	}

	if (!need_clk_sm && dev_data->clks_sm != (size_t)-1) {
		sm_release(piodev, &dev_data->clks_sm, &dev_data->clks_prog,
			   (1u << dev_config->clock_pin) | (1u << dev_config->ws_pin));
	}

	return 0;

cleanup:
	if (free_clk_sm_during_error) {
		sm_release(piodev, &dev_data->clks_sm, &dev_data->clks_prog, 0);
	}
	return -EBUSY;
}

static uint64_t calculate_divider_shift_8(uint64_t system_clock_frequency, uint64_t sample_freq,
					  uint64_t channel_length)
{
	/* system_clock_frequency = clock frequency of system
	 * f_bclk = frequency of BCLK
	 * f_pio = instruction frequency of state machine running clks program
	 * sample_freq = sampling frequency
	 * clks_cycles_factor = number of PIO cycles per BCLK period in clks program
	 *
	 * f_bclk = sample_freq * channel_length * num_channels
	 * f_pio = system_clock_frequency / divider
	 * f_pio = clks_cycles_factor * f_bclk
	 *
	 * => clks_cycles_factor * f_bclk = system_clock_frequency / divider
	 * => divider = system_clock_frequency / (clks_cycles_factor * f_bclk)
	 *            = system_clock_frequency /
	 *                      (clks_cycles_factor * sample_freq * channel_length * num_channels)
	 */

	/* Only I2S supported at this time. Number of channels is always 2 for I2S data format */
	const uint64_t num_channels = 2;
	uint64_t divider_shift_8 =
		(system_clock_frequency << 8u) /
		(clks_cycles_factor * sample_freq * channel_length * num_channels);
	return divider_shift_8;
}

static void pio_i2s_setup_clks(const struct device *dev, uint32_t system_clock_frequency)
{
	const struct pio_i2s_config *dev_config = dev->config;
	struct pio_i2s_data *dev_data = dev->data;
	PIO pio = pio_rpi_pico_get_pio(dev_config->piodev);
	uint32_t bclk_pin = dev_config->clock_pin;
	uint32_t ws_pin = dev_config->ws_pin;
	size_t sm = dev_data->clks_sm;
	uint32_t offset = dev_data->clks_prog.offset;
	pio_sm_config c;

	c = pio_get_default_sm_config();
	sm_config_set_wrap(&c, offset + clks_wrap_target, offset + clks_wrap);
	sm_config_set_sideset_pin_base(&c, bclk_pin);
	sm_config_set_sideset(&c, 1, false, false);
	sm_config_set_out_pins(&c, ws_pin, 1);
	sm_config_set_in_pins(&c, ws_pin);
	pio_sm_init(pio, sm, offset, &c);

	/* clks drives BCLK + WS; they are independent pins, so set both bits. */
	uint32_t pin_mask = (1u << bclk_pin) | (1u << ws_pin);

	pio_sm_set_pins_with_mask(pio, sm, 0, pin_mask); /* clear pins */
	pio_sm_set_pindirs_with_mask(pio, sm, pin_mask, pin_mask);

	uint32_t sample_freq = dev_data->sampling_freq;
	uint32_t channel_length = dev_data->channel_length;

	uint64_t divider =
		calculate_divider_shift_8(system_clock_frequency, sample_freq, channel_length);
	uint64_t div_int = divider >> 8u;
	uint64_t div_frac = divider & 0xffu;

	__ASSERT_NO_MSG(div_int != 0);
	__ASSERT_NO_MSG(div_int <= UINT16_MAX);

	pio_sm_set_clkdiv_int_frac(pio, sm, div_int, div_frac);

	pio_sm_exec(pio, sm, pio_encode_set(pio_y, channel_length - 2));
	pio_sm_exec(pio, sm, pio_encode_jmp(offset + CLKS_ENTRY_POINT));
	pio_sm_set_enabled(pio, sm, true);
}

static void pio_i2s_setup_stream(const struct device *dev, struct pio_i2s_stream *stream,
				 enum i2s_dir dir, bool is_controller)
{
	const struct pio_i2s_config *dev_config = dev->config;
	struct pio_i2s_data *dev_data = dev->data;
	PIO pio = pio_rpi_pico_get_pio(dev_config->piodev);
	uint32_t bclk_pin = dev_config->clock_pin;
	uint32_t ws_pin = dev_config->ws_pin;
	size_t sm = stream->sm;
	uint32_t offset = dev_data->target_prog.offset;
	pio_sm_config c;

	if (dir == I2S_DIR_TX) {
		uint32_t tx_out_pin = stream->data_pin;

		c = pio_get_default_sm_config();
		sm_config_set_wrap(&c, offset + target_wrap_target, offset + target_wrap);
		sm_config_set_out_pins(&c, tx_out_pin, 1);
		sm_config_set_in_pins(&c, dev_config->in_base_pin);
		sm_config_set_jmp_pin(&c, ws_pin);
		sm_config_set_out_shift(&c, false, false, stream->cfg.word_size);
		sm_config_set_set_pins(&c, tx_out_pin, 1);
		sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
		pio_sm_init(pio, sm, offset, &c);
		pio_sm_exec(pio, sm, pio_encode_set(pio_x, 0));
		pio_sm_set_clkdiv_int_frac(pio, sm, 1, 0);
		pio_sm_set_pins_with_mask(pio, sm, 0, 1u << tx_out_pin);
		pio_sm_set_pindirs_with_mask(pio, sm, 1u << tx_out_pin, 1u << tx_out_pin);
	} else {
		c = pio_get_default_sm_config();
		sm_config_set_wrap(&c, offset + target_wrap_target, offset + target_wrap);
		sm_config_set_in_pins(&c, dev_config->in_base_pin);
		sm_config_set_jmp_pin(&c, ws_pin);
		sm_config_set_in_shift(&c, false, false, 1);
		sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_RX);
		pio_sm_init(pio, sm, offset, &c);
		pio_sm_exec(pio, sm, pio_encode_set(pio_x, 1));
		pio_sm_set_clkdiv_int_frac(pio, sm, 1, 0);

		bool loopback = dev_data->tx.dev_dma != NULL &&
				dev_data->tx.data_pin == dev_data->rx.data_pin;

		if (!loopback) {
			pio_sm_set_pindirs_with_mask(pio, sm, 0, 1u << stream->data_pin);
		}
	}

	if (!is_controller) {
		uint32_t clk_pins = (1u << bclk_pin) | (1u << ws_pin);

		pio_sm_set_pindirs_with_mask(pio, sm, 0, clk_pins);
	}
}

static void drop_stream(const struct device *dev, struct pio_i2s_stream *stream)
{
	const struct pio_i2s_config *dev_config = dev->config;
	PIO pio = pio_rpi_pico_get_pio(dev_config->piodev);

	if (stream->sm == (size_t)-1) {
		return;
	}

	dma_stop(stream->dev_dma, stream->dma_channel);

	struct dma_status stat;

	if (!WAIT_FOR(dma_get_status(stream->dev_dma, stream->dma_channel, &stat) == 0 &&
			      !stat.busy,
		      5, k_busy_wait(1))) {
		LOG_WRN("DMA ch%u did not become idle after stop", stream->dma_channel);
	}

	pio_sm_set_enabled(pio, stream->sm, false);

	struct queue_item item;

	while (k_msgq_get(stream->msgq, &item, K_NO_WAIT) == 0) {
		k_mem_slab_free(stream->cfg.mem_slab, item.mem_block);
	}
	if (stream->mem_block != NULL) {
		k_mem_slab_free(stream->cfg.mem_slab, stream->mem_block);
		stream->mem_block = NULL;
	}
}

static int i2s_rpi_pico_configure(const struct device *dev, enum i2s_dir dir,
				  const struct i2s_config *i2s_cfg)
{
	const struct pio_i2s_config *dev_config = dev->config;
	struct pio_i2s_data *dev_data = dev->data;
	int retval;

	if (dir != I2S_DIR_RX && dir != I2S_DIR_TX) {
		LOG_ERR("Unsupported I2S direction (%d), configure RX and TX separately", dir);
		return -ENOSYS;
	}

	struct pio_i2s_stream *stream = dir == I2S_DIR_RX ? &dev_data->rx : &dev_data->tx;
	struct pio_i2s_stream *other_stream = dir == I2S_DIR_RX ? &dev_data->tx : &dev_data->rx;

	if (stream->dev_dma == NULL) {
		LOG_ERR("%s not enabled", dir == I2S_DIR_RX ? "RX" : "TX");
		return -EINVAL;
	}

	bool other_is_controller =
		other_stream->state != I2S_STATE_NOT_READY &&
		!(other_stream->cfg.options & (I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET));

	if (stream->state != I2S_STATE_NOT_READY && stream->state != I2S_STATE_READY) {
		LOG_ERR("stream in invalid state (%d)", stream->state);
		return -EINVAL;
	}

	if (i2s_cfg->frame_clk_freq == 0U) {
		drop_stream(dev, stream);

		sm_release(dev_config->piodev, &stream->sm, &dev_data->target_prog,
			   dir == I2S_DIR_TX ? (1u << stream->data_pin) : 0);
		memset(&stream->cfg, 0, sizeof(struct i2s_config));

		if (!other_is_controller) {
			sm_release(dev_config->piodev, &dev_data->clks_sm, &dev_data->clks_prog,
				   (1u << dev_config->clock_pin) | (1u << dev_config->ws_pin));
		}

		stream->state = I2S_STATE_NOT_READY;
		return 0;
	}

	if (other_stream->state != I2S_STATE_NOT_READY && other_stream->state != I2S_STATE_READY) {
		LOG_ERR("other stream in invalid state (%d)", other_stream->state);
		return -EINVAL;
	}

	if (I2S_FMT_DATA_FORMAT_I2S != (i2s_cfg->format & I2S_FMT_DATA_FORMAT_MASK)) {
		LOG_ERR("Only I2S data format is supported");
		return -ENOTSUP;
	}

	if (i2s_cfg->format & I2S_FMT_DATA_ORDER_LSB) {
		return -ENOTSUP;
	}

	if (i2s_cfg->channels != 2) {
		LOG_ERR("Number of channels not 2 when configured with I2S data format");
		return -EINVAL;
	}

	if (i2s_cfg->word_size != 8 && i2s_cfg->word_size != 16 && i2s_cfg->word_size != 32) {
		LOG_ERR("I2S word size (%d) is unsupported, must be 8, 16 or 32",
			i2s_cfg->word_size);
		return -EINVAL;
	}
	uint32_t channel_length = i2s_cfg->word_size;

	if (i2s_cfg->options & I2S_OPT_LOOPBACK) {
		LOG_ERR("I2S loopback mode unsupported");
		LOG_DBG("To enable loopback, use the same SD for TX and RX pinctrl");
		return -EINVAL;
	}

	if (i2s_cfg->options & I2S_OPT_PINGPONG) {
		LOG_ERR("I2S_OPT_PINGPONG is unsupported");
		return -EINVAL;
	}

	bool is_bit_clk_target = i2s_cfg->options & I2S_OPT_BIT_CLK_TARGET;
	bool is_frame_clk_target = i2s_cfg->options & I2S_OPT_FRAME_CLK_TARGET;

	if (is_bit_clk_target != is_frame_clk_target) {
		LOG_ERR("I2S bit CLK and frame CLK must be either both target or both controller");
		return -EINVAL;
	}

	bool i2s_cfg_is_controller =
		!(i2s_cfg->options & (I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET));

	bool is_controller = i2s_cfg_is_controller || other_is_controller;

	if (i2s_cfg_is_controller) {
		if (i2s_cfg->options & I2S_OPT_BIT_CLK_GATED) {
			LOG_ERR("Gated bit clock is unsupported");
			return -EINVAL;
		}

		if (other_is_controller &&
		    i2s_cfg->frame_clk_freq != other_stream->cfg.frame_clk_freq) {
			LOG_ERR("Simultaneously configured controller streams have different "
				"frame_clk_freq (%u) (%u)",
				i2s_cfg->frame_clk_freq, other_stream->cfg.frame_clk_freq);
			return -EINVAL;
		}
	}

	uint32_t system_clock_frequency;

	retval = clock_control_get_rate(dev_config->clk_dev, dev_config->clk_id,
					&system_clock_frequency);
	if (retval < 0) {
		LOG_ERR("Failed to get clock frequency");
		return retval;
	}

	uint64_t divider_shift_8 = calculate_divider_shift_8(
		system_clock_frequency, i2s_cfg->frame_clk_freq, channel_length);
	uint64_t divider = divider_shift_8 >> 8u;

	if (divider < target_prog_instr_per_bclk_toggle) {
		LOG_ERR("frame_clk_freq %u Hz gives a bit-clock half-period of %llu system "
			"clocks, the data state machine needs %u. Maximum with %u-bit "
			"channels is %llu Hz",
			i2s_cfg->frame_clk_freq, divider, target_prog_instr_per_bclk_toggle,
			channel_length,
			(uint64_t)system_clock_frequency /
				(4u * channel_length * target_prog_instr_per_bclk_toggle));
		return -EINVAL;
	}

	if (i2s_cfg_is_controller && divider > UINT16_MAX) {
		LOG_ERR("frame_clk_freq %u Hz needs a clks divider of %llu, maximum is %u",
			i2s_cfg->frame_clk_freq, divider, UINT16_MAX);
		return -EINVAL;
	}

	if (other_stream->state != I2S_STATE_NOT_READY &&
	    i2s_cfg->word_size != other_stream->cfg.word_size) {
		LOG_ERR("Simultaneously configured streams have different word_size (%d) (%d)",
			i2s_cfg->word_size, other_stream->cfg.word_size);
		return -EINVAL;
	}

	retval = sm_set_stream_and_clk(dev, dir, is_controller);
	if (retval < 0) {
		return retval;
	}

	PIO pio = pio_rpi_pico_get_pio(dev_config->piodev);

	stream->dma_cfg.user_data = (void *)dev;
	stream->dma_cfg.dma_slot =
		RPI_PICO_DMA_DREQ_TO_SLOT(pio_get_dreq(pio, stream->sm, dir == I2S_DIR_TX));
	stream->dma_cfg.source_data_size = channel_length / 8u;
	stream->dma_cfg.dest_data_size = channel_length / 8u;

	memcpy(&stream->cfg, i2s_cfg, sizeof(struct i2s_config));

	dev_data->channel_length = channel_length;

	pio_i2s_setup_stream(dev, stream, dir, is_controller);

	if (i2s_cfg_is_controller) {
		dev_data->sampling_freq = i2s_cfg->frame_clk_freq;
		pio_i2s_setup_clks(dev, system_clock_frequency);
	}

	stream->state = I2S_STATE_READY;

	return 0;
}

#if PIO_I2S_IS_DIR_EN(tx)
static void dma_tx_callback(const struct device *dma_dev, void *arg, uint32_t channel, int status)
{
	const struct device *dev = (const struct device *)arg;
	const struct pio_i2s_config *config = dev->config;
	struct pio_i2s_data *data = dev->data;
	PIO pio = pio_rpi_pico_get_pio(config->piodev);

	int retval;

	struct pio_i2s_stream *stream = &data->tx;

	if (status < 0) {
		LOG_ERR("TX DMA transfer failed: %d", status);
		stream->state = I2S_STATE_ERROR;
		return;
	}

	/* I2S_TRIGGER_STOP / I2S_TRIGGER_DRAIN */
	if (stream->state == I2S_STATE_STOPPING &&
	    (stream->tx_stop_without_draining || k_msgq_num_used_get(stream->msgq) == 0)) {
		stream->state = I2S_STATE_READY;
		k_mem_slab_free(stream->cfg.mem_slab, stream->mem_block);
		stream->mem_block = NULL;
		return;
	}

	struct queue_item item;
	size_t mem_block_size;
	int ret = k_msgq_get(stream->msgq, &item, K_NO_WAIT);

	if (ret < 0) {
		LOG_ERR("TX buffer underrun");
		stream->state = I2S_STATE_ERROR;
		return;
	}

	mem_block_size = item.size;

	retval = dma_reload(stream->dev_dma, stream->dma_channel, (uint32_t)item.mem_block,
			    (uint32_t)&pio->txf[stream->sm], mem_block_size);

	if (retval < 0) {
		LOG_ERR("Failed to reload TX DMA transfer: %d", retval);
		stream->state = I2S_STATE_ERROR;
	}

	k_mem_slab_free(stream->cfg.mem_slab, stream->mem_block);
	stream->mem_block = item.mem_block;
}
#endif /* PIO_I2S_IS_DIR_EN(tx) */

#if PIO_I2S_IS_DIR_EN(rx)
static void dma_rx_callback(const struct device *dma_dev, void *arg, uint32_t channel, int status)
{
	const struct device *dev = (const struct device *)arg;
	const struct pio_i2s_config *dev_config = dev->config;
	struct pio_i2s_data *dev_data = dev->data;
	PIO pio = pio_rpi_pico_get_pio(dev_config->piodev);

	int retval;

	struct pio_i2s_stream *stream = &dev_data->rx;

	if (status < 0) {
		LOG_ERR("RX DMA transfer failed: %d", status);
		stream->state = I2S_STATE_ERROR;
		return;
	}

	if (stream->state == I2S_STATE_ERROR) {
		return;
	}

	struct queue_item item = {.mem_block = stream->mem_block, .size = stream->cfg.block_size};

	stream->mem_block = NULL;

	if (stream->state == I2S_STATE_STOPPING) {
		stream->state = I2S_STATE_READY;
		goto put_item;
	}

	retval = k_mem_slab_alloc(stream->cfg.mem_slab, &stream->mem_block, K_NO_WAIT);
	if (retval < 0) {
		stream->state = I2S_STATE_ERROR;
		LOG_ERR("RX callback failed to allocate block");
		goto put_item;
	}

	retval = dma_reload(stream->dev_dma, stream->dma_channel, (uint32_t)&pio->rxf[stream->sm],
			    (uint32_t)stream->mem_block, stream->cfg.block_size);

	if (retval < 0) {
		LOG_ERR("Failed to reload RX DMA transfer: %d", retval);
		stream->state = I2S_STATE_ERROR;
		goto put_item;
	}

put_item:
	retval = k_msgq_put(stream->msgq, &item, K_NO_WAIT);

	if (retval < 0) {
		LOG_ERR("RX overrun");
		stream->state = I2S_STATE_ERROR;
		k_mem_slab_free(stream->cfg.mem_slab, item.mem_block);
		return;
	}
}
#endif /* PIO_I2S_IS_DIR_EN(rx) */

static int pio_i2s_init(const struct device *dev)
{
	const struct pio_i2s_config *dev_config = dev->config;
	struct pio_i2s_data *dev_data = dev->data;
	int retval;

	if (!device_is_ready(dev_config->piodev)) {
		LOG_ERR("%s: PIO device not ready", dev->name);
		return -ENODEV;
	}

	if (!device_is_ready(dev_config->clk_dev)) {
		LOG_ERR("%s: clock controller not ready", dev->name);
		return -ENODEV;
	}

	retval = clock_control_on(dev_config->clk_dev, dev_config->clk_id);
	if (retval < 0) {
		LOG_ERR("Failed to enable the clock");
		return retval;
	}

	if (dev_data->tx.dev_dma != NULL && !device_is_ready(dev_data->tx.dev_dma)) {
		LOG_ERR("%s: TX DMA device not ready", dev->name);
		return -ENODEV;
	}

	if (dev_data->rx.dev_dma != NULL && !device_is_ready(dev_data->rx.dev_dma)) {
		LOG_ERR("%s: RX DMA device not ready", dev->name);
		return -ENODEV;
	}

	retval = pinctrl_apply_state(dev_config->pcfg, PINCTRL_STATE_DEFAULT);
	if (retval < 0) {
		LOG_ERR("pinctrl_apply_state failed with ret = %d", retval);
		return retval;
	}
	return 0;
}

static void i2s_reset_stream_sm(const struct device *dev, struct pio_i2s_stream *stream)
{
	const struct pio_i2s_config *config = dev->config;
	struct pio_i2s_data *data = dev->data;
	PIO pio = pio_rpi_pico_get_pio(config->piodev);

	pio_sm_set_enabled(pio, stream->sm, false);
	pio_sm_clear_fifos(pio, stream->sm);
	pio_sm_restart(pio, stream->sm);

	pio_sm_exec(pio, stream->sm, pio_encode_set(pio_y, 0));
	pio_sm_exec(pio, stream->sm, pio_encode_jmp(data->target_prog.offset));
}

static int i2s_start_rx_stream_dma(const struct device *dev, struct pio_i2s_stream *stream)
{
	const struct pio_i2s_config *config = dev->config;
	PIO pio = pio_rpi_pico_get_pio(config->piodev);

	struct dma_block_config *blk_cfg = &stream->dma_blk_cfg;
	int retval;

	retval = k_mem_slab_alloc(stream->cfg.mem_slab, &stream->mem_block, K_NO_WAIT);
	if (retval < 0) {
		LOG_ERR("While starting rx stream dma, failed to allocate mem slab");
		return -ENOMEM;
	}

	i2s_reset_stream_sm(dev, stream);

	memset(blk_cfg, 0, sizeof(*blk_cfg));
	blk_cfg->block_size = stream->cfg.block_size;
	blk_cfg->source_address = (uint32_t)&pio->rxf[stream->sm];
	blk_cfg->source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	blk_cfg->dest_address = (uint32_t)stream->mem_block;
	blk_cfg->dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;

	stream->dma_cfg.head_block = blk_cfg;

	retval = dma_config(stream->dev_dma, stream->dma_channel, &stream->dma_cfg);
	if (retval < 0) {
		LOG_ERR("Failed to configure RX DMA transfer: %d", retval);
		k_mem_slab_free(stream->cfg.mem_slab, stream->mem_block);
		stream->mem_block = NULL;
		return retval;
	}

	retval = dma_start(stream->dev_dma, stream->dma_channel);
	if (retval < 0) {
		LOG_ERR("Failed to start RX DMA transfer: %d", retval);
		k_mem_slab_free(stream->cfg.mem_slab, stream->mem_block);
		stream->mem_block = NULL;
		return retval;
	}

	pio_sm_set_enabled(pio, stream->sm, true);

	return 0;
}

static int i2s_start_tx_stream_dma(const struct device *dev, struct pio_i2s_stream *stream)
{
	const struct pio_i2s_config *config = dev->config;
	PIO pio = pio_rpi_pico_get_pio(config->piodev);

	struct dma_block_config *blk_cfg = &stream->dma_blk_cfg;
	size_t mem_block_size;
	struct queue_item item;

	int ret = k_msgq_get(stream->msgq, &item, K_NO_WAIT);

	if (ret < 0) {
		LOG_ERR("TX buffer is empty");
		return -ENOMEM;
	}

	stream->mem_block = item.mem_block;
	mem_block_size = item.size;

	i2s_reset_stream_sm(dev, stream);

	memset(blk_cfg, 0, sizeof(*blk_cfg));
	blk_cfg->block_size = mem_block_size;
	blk_cfg->source_address = (uint32_t)stream->mem_block;
	blk_cfg->source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	blk_cfg->dest_address = (uint32_t)&pio->txf[stream->sm];
	blk_cfg->dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;

	stream->dma_cfg.head_block = blk_cfg;

	ret = dma_config(stream->dev_dma, stream->dma_channel, &stream->dma_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to configure TX DMA transfer: %d", ret);
		k_mem_slab_free(stream->cfg.mem_slab, stream->mem_block);
		stream->mem_block = NULL;
		return ret;
	}

	ret = dma_start(stream->dev_dma, stream->dma_channel);
	if (ret < 0) {
		LOG_ERR("Failed to start TX DMA transfer: %d", ret);
		k_mem_slab_free(stream->cfg.mem_slab, stream->mem_block);
		stream->mem_block = NULL;
		return ret;
	}

	pio_sm_set_enabled(pio, stream->sm, true);

	return 0;
}

static int i2s_rpi_pico_trigger(const struct device *dev, enum i2s_dir dir,
				enum i2s_trigger_cmd cmd)
{
	struct pio_i2s_data *dev_data = dev->data;
	k_spinlock_key_t key;
	int ret = 0;

	if (dir != I2S_DIR_RX && dir != I2S_DIR_TX) {
		LOG_DBG("Unsupported trigger direction %d", dir);
		return -ENOSYS;
	}

	struct pio_i2s_stream *stream = dir == I2S_DIR_RX ? &dev_data->rx : &dev_data->tx;

	key = k_spin_lock(&dev_data->lock);

	switch (cmd) {
	case I2S_TRIGGER_START:
		if (stream->state != I2S_STATE_READY) {
			LOG_DBG("START trigger: invalid state %d", stream->state);
			ret = -EIO;
			break;
		}

		if (dir == I2S_DIR_TX) {
			stream->tx_stop_without_draining = false;
			ret = i2s_start_tx_stream_dma(dev, stream);
		} else {
			ret = i2s_start_rx_stream_dma(dev, stream);
		}

		if (ret < 0) {
			LOG_DBG("START trigger failed %d", ret);
			break;
		}

		stream->state = I2S_STATE_RUNNING;
		break;

	case I2S_TRIGGER_STOP:
		if (stream->state != I2S_STATE_RUNNING) {
			LOG_DBG("STOP trigger: invalid state %d", stream->state);
			ret = -EIO;
			break;
		}

		stream->state = I2S_STATE_STOPPING;
		stream->tx_stop_without_draining = true;
		break;

	case I2S_TRIGGER_DRAIN:
		if (stream->state != I2S_STATE_RUNNING) {
			LOG_DBG("DRAIN trigger: invalid state %d", stream->state);
			ret = -EIO;
			break;
		}

		stream->state = I2S_STATE_STOPPING;
		break;

	case I2S_TRIGGER_DROP:
		if (stream->state == I2S_STATE_NOT_READY) {
			LOG_DBG("DROP trigger: invalid state %d", stream->state);
			ret = -EIO;
			break;
		}

		drop_stream(dev, stream);
		stream->state = I2S_STATE_READY;
		break;

	case I2S_TRIGGER_PREPARE:
		if (stream->state != I2S_STATE_ERROR) {
			LOG_DBG("PREPARE trigger: invalid state %d", stream->state);
			ret = -EIO;
			break;
		}

		drop_stream(dev, stream);
		stream->state = I2S_STATE_READY;
		break;

	default:
		LOG_DBG("Unsupported trigger command: %d", (int)cmd);
		ret = -EINVAL;
	}

	k_spin_unlock(&dev_data->lock, key);

	return ret;
}

static const struct i2s_config *i2s_rpi_pico_config_get(const struct device *dev, enum i2s_dir dir)
{
	struct pio_i2s_data *const dev_data = dev->data;
	struct pio_i2s_stream *stream = NULL;

	if (dir == I2S_DIR_RX) {
		stream = &dev_data->rx;
	} else if (dir == I2S_DIR_TX) {
		stream = &dev_data->tx;
	} else {
		LOG_ERR("Invalid direction");
		return NULL;
	}

	if (stream->state == I2S_STATE_NOT_READY) {
		return NULL;
	}

	return &stream->cfg;
}

static DEVICE_API(i2s, i2s_rpi_pico_driver_api) = {
	.configure = i2s_rpi_pico_configure,
	.config_get = i2s_rpi_pico_config_get,
	.read = i2s_rpi_pico_read,
	.write = i2s_rpi_pico_write,
	.trigger = i2s_rpi_pico_trigger,
};

#define PIO_I2S_BCLK_PIN(idx)    DT_INST_RPI_PICO_PIO_PIN_BY_NAME(idx, default, 0, bclk_pins, 0)
#define PIO_I2S_WS_PIN(idx)      DT_INST_RPI_PICO_PIO_PIN_BY_NAME(idx, default, 0, ws_pins, 0)
#define PIO_I2S_TX_DATA_PIN(idx) DT_INST_RPI_PICO_PIO_PIN_BY_NAME(idx, default, 0, sdout_pins, 0)
#define PIO_I2S_RX_DATA_PIN(idx) DT_INST_RPI_PICO_PIO_PIN_BY_NAME(idx, default, 0, sdin_pins, 0)

#define PIO_I2S_HAS_GROUP(idx, group)                                                              \
	DT_NODE_EXISTS(DT_CHILD(DT_PINCTRL_BY_NAME(DT_DRV_INST(idx), default, 0), group))

#define PIO_I2S_HAS_TX(idx) PIO_I2S_IS_DIR_INST_EN(idx, tx)
#define PIO_I2S_HAS_RX(idx) PIO_I2S_IS_DIR_INST_EN(idx, rx)

#define PIO_I2S_PIN_LIMIT 32

#define PIO_I2S_TX_MSGQ(idx)     COND_CODE_1(PIO_I2S_HAS_TX(idx), (&tx_##idx##_queue), (NULL))
#define PIO_I2S_RX_MSGQ(idx)     COND_CODE_1(PIO_I2S_HAS_RX(idx), (&rx_##idx##_queue), (NULL))
#define PIO_I2S_TX_CALLBACK(idx) COND_CODE_1(PIO_I2S_HAS_TX(idx), (dma_tx_callback), (NULL))
#define PIO_I2S_RX_CALLBACK(idx) COND_CODE_1(PIO_I2S_HAS_RX(idx), (dma_rx_callback), (NULL))
#define PIO_I2S_TX_PIN(idx)      COND_CODE_1(PIO_I2S_HAS_TX(idx), (PIO_I2S_TX_DATA_PIN(idx)), (0))
#define PIO_I2S_RX_PIN(idx)      COND_CODE_1(PIO_I2S_HAS_RX(idx), (PIO_I2S_RX_DATA_PIN(idx)), (0))

#define PIO_I2S_TX_ASSERT(idx)                                                                     \
	BUILD_ASSERT(PIO_I2S_TX_DATA_PIN(idx) < PIO_I2S_PIN_LIMIT,                                 \
		     "I2S sdout_pins pin must be in the first PIO pin bank (GPIO 0..31).")

#define PIO_I2S_RX_ASSERT(idx)                                                                     \
	BUILD_ASSERT(PIO_I2S_RX_DATA_PIN(idx) == PIO_I2S_BCLK_PIN(idx) - 1,                        \
		     "I2S sdin_pins pin must be equal to bit-clock pin - 1 "                       \
		     "due to limitations in the PIO program.");                                    \
	BUILD_ASSERT(PIO_I2S_RX_DATA_PIN(idx) < PIO_I2S_PIN_LIMIT,                                 \
		     "I2S sdin_pins pin must be in the first PIO pin bank (GPIO 0..31).")

#define PIO_I2S_MSGQ(idx, dir, dir_cap)                                                            \
	K_MSGQ_DEFINE(dir##_##idx##_queue, sizeof(struct queue_item),                              \
		      CONFIG_I2S_RPI_PICO_PIO_##dir_cap##_QUEUE_SIZE, 1)

#define PIO_I2S_INIT(idx)                                                                          \
	BUILD_ASSERT(PIO_I2S_HAS_TX(idx) || PIO_I2S_HAS_RX(idx),                                   \
		     "I2S node needs at least one of the \"tx\" / \"rx\" dma-names.");             \
	BUILD_ASSERT(!PIO_I2S_HAS_TX(idx) || PIO_I2S_HAS_GROUP(idx, sdout_pins),                   \
		     "I2S sdout_pins not defined.");                                               \
	BUILD_ASSERT(!PIO_I2S_HAS_RX(idx) || PIO_I2S_HAS_GROUP(idx, sdin_pins),                    \
		     "I2S sdin_pins not defined.");                                                \
	BUILD_ASSERT(PIO_I2S_WS_PIN(idx) == PIO_I2S_BCLK_PIN(idx) + 1,                             \
		     "I2S ws_pins pin must be equal to bit-clock pin + 1 "                         \
		     "due to limitations in the PIO program.");                                    \
	BUILD_ASSERT(PIO_I2S_BCLK_PIN(idx) >= 1,                                                   \
		     "I2S bclk_pins pin must be >= 1 due to limitations in the PIO program.");     \
	BUILD_ASSERT(PIO_I2S_BCLK_PIN(idx) < PIO_I2S_PIN_LIMIT,                                    \
		     "I2S bclk_pins pin must be in the first PIO pin bank (GPIO 0..31).");         \
	BUILD_ASSERT(PIO_I2S_WS_PIN(idx) < PIO_I2S_PIN_LIMIT,                                      \
		     "I2S ws_pins pin must be in the first PIO pin bank (GPIO 0..31).");           \
	IF_ENABLED(PIO_I2S_HAS_TX(idx),                                                            \
		   (PIO_I2S_TX_ASSERT(idx)));                                  \
	IF_ENABLED(PIO_I2S_HAS_RX(idx),                                                            \
		   (PIO_I2S_RX_ASSERT(idx)));                                  \
	PINCTRL_DT_INST_DEFINE(idx);                                                               \
	static const struct pio_i2s_config pio_i2s##idx##_config = {                               \
		.piodev = DEVICE_DT_GET(DT_INST_PARENT(idx)),                                      \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),                                       \
		.clk_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_INST_PARENT(idx))),                     \
		.clk_id = (clock_control_subsys_t)DT_PHA_BY_IDX(DT_INST_PARENT(idx), clocks, 0,    \
								clk_id),                           \
		.clock_pin = PIO_I2S_BCLK_PIN(idx),                                                \
		.ws_pin = PIO_I2S_WS_PIN(idx),                                                     \
		.in_base_pin = PIO_I2S_BCLK_PIN(idx) - 1,                                          \
	};                                                                                         \
	IF_ENABLED(PIO_I2S_HAS_TX(idx),                                                            \
		   (PIO_I2S_MSGQ(idx, tx, TX)));                                 \
	IF_ENABLED(PIO_I2S_HAS_RX(idx),                                                            \
		   (PIO_I2S_MSGQ(idx, rx, RX)));                                 \
	static struct pio_i2s_data pio_i2s##idx##_data = {                                         \
		.tx =                                                                              \
			{                                                                          \
				.msgq = PIO_I2S_TX_MSGQ(idx),                                      \
				.state = I2S_STATE_NOT_READY,                                      \
				.tx_stop_without_draining = false,                                 \
				.dev_dma = UTIL_AND(                                               \
					DT_INST_DMAS_HAS_NAME(idx, tx),                            \
					DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(idx, tx))),        \
				.dma_channel =                                                     \
					UTIL_AND(DT_INST_DMAS_HAS_NAME(idx, tx),                   \
						 DT_INST_DMAS_CELL_BY_NAME(idx, tx, channel)),     \
				.data_pin = PIO_I2S_TX_PIN(idx),                                   \
				.dma_cfg =                                                         \
					{                                                          \
						.block_count = 1,                                  \
						.channel_direction = MEMORY_TO_PERIPHERAL,         \
						.source_burst_length = 1,                          \
						.dest_burst_length = 1,                            \
						.channel_priority = 1,                             \
						.dma_callback = PIO_I2S_TX_CALLBACK(idx),          \
					},                                                         \
				.sm = (size_t)-1,                                                  \
			},                                                                         \
		.rx =                                                                              \
			{                                                                          \
				.msgq = PIO_I2S_RX_MSGQ(idx),                                      \
				.state = I2S_STATE_NOT_READY,                                      \
				.tx_stop_without_draining = false,                                 \
				.dev_dma = UTIL_AND(                                               \
					DT_INST_DMAS_HAS_NAME(idx, rx),                            \
					DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(idx, rx))),        \
				.dma_channel =                                                     \
					UTIL_AND(DT_INST_DMAS_HAS_NAME(idx, rx),                   \
						 DT_INST_DMAS_CELL_BY_NAME(idx, rx, channel)),     \
				.data_pin = PIO_I2S_RX_PIN(idx),                                   \
				.dma_cfg =                                                         \
					{                                                          \
						.block_count = 1,                                  \
						.channel_direction = PERIPHERAL_TO_MEMORY,         \
						.source_burst_length = 1,                          \
						.dest_burst_length = 1,                            \
						.channel_priority = 1,                             \
						.dma_callback = PIO_I2S_RX_CALLBACK(idx),          \
					},                                                         \
				.sm = (size_t)-1,                                                  \
			},                                                                         \
		.clks_sm = (size_t)-1,                                                             \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(idx, pio_i2s_init, NULL, &pio_i2s##idx##_data,                       \
			      &pio_i2s##idx##_config, POST_KERNEL, CONFIG_I2S_INIT_PRIORITY,       \
			      &i2s_rpi_pico_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PIO_I2S_INIT)

/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 *
 * I2S driver for TI CC27XX (LPF3) SoCs
 */

#define DT_DRV_COMPAT ti_cc27xx_i2s

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>

#include <zephyr/pm/policy.h>

#include <ti/devices/DeviceFamily.h>
/* clang-format off */
#include DeviceFamily_constructPath(driverlib/i2s.h)
#include DeviceFamily_constructPath(driverlib/ckmd.h)
#include DeviceFamily_constructPath(driverlib/clkctl.h)
#include DeviceFamily_constructPath(inc/hw_memmap.h)
#include DeviceFamily_constructPath(inc/hw_i2s.h)
#include DeviceFamily_constructPath(inc/hw_clkctl.h)
/* clang-format on */
#include <ti/drivers/power/PowerCC27XX.h>

#include "i2s_ti_cc27xx.h"

LOG_MODULE_REGISTER(i2s_ti_cc27xx, CONFIG_I2S_LOG_LEVEL);

#define TI_CC27XX_I2S_SLAB_CHUNK_SIZE     sizeof(struct ti_cc27xx_i2s_transfer)
#define TI_CC27XX_I2S_TX_SLAB_CHUNK_COUNT CONFIG_I2S_TI_CC27XX_TXQ_SIZE
#define TI_CC27XX_I2S_RX_SLAB_CHUNK_COUNT CONFIG_I2S_TI_CC27XX_RXQ_SIZE
#define TI_CC27XX_I2S_SLAB_ALIGN          4

#define CC27XX_I2S_TIMEOUT_ERROR   (0x0100U)
#define CC27XX_I2S_BUS_ERROR       (0x0200U)
#define CC27XX_I2S_WS_ERROR        (0x0400U)
#define CC27XX_I2S_PTR_READ_ERROR  (0x0800U)
#define CC27XX_I2S_PTR_WRITE_ERROR (0x1000U)

#define TI_CC27XX_I2S_DMA_MIN_FRAMES_PER_TRANSFER 2
#define TI_CC27XX_I2S_DMA_MAX_FRAMES_PER_TRANSFER 256

#define TI_CC27XX_I2S_HW_STOP_TIMEOUT_US 40000

static inline struct ti_cc27xx_i2s_transfer *
peek_next_tx_transfer(struct ti_cc27xx_i2s_stream *stream)
{
	return k_fifo_peek_head(&stream->queue);
}

static inline struct ti_cc27xx_i2s_transfer *
peek_next_rx_transfer(struct ti_cc27xx_i2s_stream *stream)
{
	return stream->next_transfer;
}

static void purge_queue(struct ti_cc27xx_i2s_stream *stream)
{
	struct ti_cc27xx_i2s_transfer *tr;

	if (k_fifo_is_empty(&stream->queue)) {
		return;
	}

	while ((tr = k_fifo_get(&stream->queue, K_NO_WAIT)) != NULL) {
		/* Free both audio buffer and transfer metadata */
		k_mem_slab_free(tr->audio_mem_slab, tr->mem_block);
		k_mem_slab_free(tr->own_mem_slab, tr);
	}
}

static inline void set_dma_tx_pointer(const struct device *dev, void *addr)
{
	const struct ti_cc27xx_i2s_cfg *config = dev->config;

	I2SSetOutPointer(config->reg_base, (uintptr_t)addr);
}

static inline void set_dma_rx_pointer(const struct device *dev, void *addr)
{
	const struct ti_cc27xx_i2s_cfg *config = dev->config;

	I2SSetInPointer(config->reg_base, (uintptr_t)addr);
}

static inline void set_tx_sample_stamp_trigger(const struct device *dev, uint32_t value)
{
	const struct ti_cc27xx_i2s_cfg *config = dev->config;

	I2SConfigureOutSampleStampTrigger(config->reg_base, value);
}

static inline void set_rx_sample_stamp_trigger(const struct device *dev, uint32_t value)
{
	const struct ti_cc27xx_i2s_cfg *config = dev->config;

	I2SConfigureInSampleStampTrigger(config->reg_base, value);
}

static inline uint32_t hw_frame_count_for(const struct device *dev, uint32_t logical_frame_count);
static void purge_stream(struct ti_cc27xx_i2s_stream *stream);

static void config_serial_format(const struct device *dev,
				 struct ti_cc27xx_i2s_stream_cfg *stream_cfg,
				 uint32_t dma_frame_count)
{
	const struct ti_cc27xx_i2s_cfg *config = dev->config;
	uint8_t data_delay;
	uint32_t memory_length;
	uint32_t sampling_edge;
	uint8_t bits_per_sample;
	bool is_dual_phase;

	/* Determine memory word size (16-bit or 32-bit) for DMA transfers */
	if (stream_cfg->bits_per_memory_word <= TI_CC27XX_I2S_WORD_SIZE_16) {
		memory_length = TI_CC27XX_I2S_MEMORY_LENGTH_16BITS;
	} else {
		memory_length = TI_CC27XX_I2S_MEMORY_LENGTH_32BITS;
	}

	/* Calculate total data delay: padding before word + additional bit shift */
	data_delay = stream_cfg->before_word_padding + stream_cfg->data_shift;

	/* Map Zephyr sampling edge config to hardware register values */
	if (config->sampling_edge == TI_CC27XX_I2S_SAMPLING_EDGE_RISING) {
		sampling_edge = I2S_POS_EDGE;
	} else {
		sampling_edge = I2S_NEG_EDGE;
	}

	/* Check if using dual-phase format (I2S standard) or single-phase (TDM/DSP) */
	is_dual_phase = (config->phase_type == TI_CC27XX_I2S_PHASE_TYPE_DUAL);

	if (config->phase_type == TI_CC27XX_I2S_PHASE_TYPE_DUAL) {
		bits_per_sample = stream_cfg->bits_per_audio_word + stream_cfg->after_word_padding;
	} else {
		bits_per_sample = stream_cfg->bits_per_audio_word;
	}

	I2SConfigureFormat(config->reg_base, data_delay, /* beforeWordPadding(=0) + dataShift */
			   memory_length,                /* I2S_MEM_LENGTH_16 or _32 */
			   sampling_edge,                /* samplingEdge = RISING */
			   is_dual_phase,                /* dualPhase = true for I2S/LJF/RJF */
			   bits_per_sample);             /* wordLength (no afterPadding for dual) */

	/* Set WCLK counter period for sample stamp generation (used for DMA timing).
	 * STMPWPER must be a multiple of (END_FRAME_IDX + 1) per the TRM (section
	 * 26.7), so when start_dma() doubles the END_FRAME_IDX in MEMLEN32 mode
	 * we double STMPWPER here too to keep them coordinated.
	 */
	I2SConfigureWclkCounterPeriod(config->reg_base, hw_frame_count_for(dev, dma_frame_count));

	I2SConfigureInSampleStampTrigger(config->reg_base, I2S_STMP_SATURATION);
	I2SConfigureOutSampleStampTrigger(config->reg_base, I2S_STMP_SATURATION);
}

static inline enum TI_CC27XX_I2S_CHANNEL_BITMASK get_pin_bitmask(const struct device *dev,
								 enum TI_CC27XX_I2S_PIN_DIR pin_dir)
{
	struct ti_cc27xx_i2s_data *data = dev->data;

	switch (pin_dir) {
	case TI_CC27XX_I2S_PIN_DIR_IN:
		return data->stream_rx.cfg.channel_bitmask;
	case TI_CC27XX_I2S_PIN_DIR_OUT:
		return data->stream_tx.cfg.channel_bitmask;
	default:
		return TI_CC27XX_I2S_CHANNEL_BITMASK_NONE;
	}
}
static void config_channels(const struct device *dev)
{
	const struct ti_cc27xx_i2s_cfg *config = dev->config;
	enum TI_CC27XX_I2S_CHANNEL_BITMASK sd0_bitmask;
	enum TI_CC27XX_I2S_CHANNEL_BITMASK sd1_bitmask;

	sd0_bitmask = get_pin_bitmask(dev, config->pin_dir_sd0);
	sd1_bitmask = get_pin_bitmask(dev, config->pin_dir_sd1);

	I2SConfigureFrame(config->reg_base,
			  (uint8_t)config->pin_dir_sd0,        /* I2S_SD0_OUT / IN / DIS */
			  sd0_bitmask,                         /* channel bitmask for SD0 */
			  (uint8_t)(config->pin_dir_sd1 << 4), /* I2S_SD1_OUT / IN / DIS */
			  sd1_bitmask);                        /* channel bitmask for SD1 */
}

static void config_clocks(const struct device *dev)
{
	const struct ti_cc27xx_i2s_cfg *config = dev->config;
	struct ti_cc27xx_i2s_data *data = dev->data;

	I2SConfigureClocks(config->reg_base, config->module_role, data->invert_ws,
			   config->phase_type == TI_CC27XX_I2S_PHASE_TYPE_DUAL,
			   config->cclk_divider, data->ws_divider, data->sck_divider);
}

/* When the audio sample is held in a 32-bit memory slot (MEMLEN32=EN, used
 * for 24-bit data), the cc27xx I2S DMA programs internally on a half-frame
 * granularity: the AIFDMACFG.END_FRAME_IDX field reaches its target after
 * (programmed_value / 2) WCLK frames, not the full programmed value. To make
 * the user-visible "DMA fills N frames per pointer-refresh" contract hold for
 * 16-bit AND 24-bit data, double the value programmed into the register when
 * MEMLEN32 is in use. The software-visible `dma_frame_count` remains the
 * logical (user) frame count and the rest of the driver's bookkeeping is
 * unaffected.
 */
static inline uint32_t hw_frame_count_for(const struct device *dev, uint32_t logical_frame_count)
{
	struct ti_cc27xx_i2s_data *data = dev->data;
	bool is_memlen32 = (data->stream_rx.enabled && data->stream_rx.cfg.bits_per_memory_word >
							       TI_CC27XX_I2S_WORD_SIZE_16) ||
			   (data->stream_tx.enabled &&
			    data->stream_tx.cfg.bits_per_memory_word > TI_CC27XX_I2S_WORD_SIZE_16);

	/* Before either stream is enabled (initial start path), inspect either
	 * stream's configured memory width directly.
	 */
	if (!data->stream_rx.enabled && !data->stream_tx.enabled) {
		is_memlen32 =
			(data->stream_rx.configured &&
			 data->stream_rx.cfg.bits_per_memory_word > TI_CC27XX_I2S_WORD_SIZE_16) ||
			(data->stream_tx.configured &&
			 data->stream_tx.cfg.bits_per_memory_word > TI_CC27XX_I2S_WORD_SIZE_16);
	}

	return is_memlen32 ? (logical_frame_count * 2U) : logical_frame_count;
}

static inline void start_dma(const struct device *dev, uint32_t frame_count)
{
	const struct ti_cc27xx_i2s_cfg *config = dev->config;
	struct ti_cc27xx_i2s_data *data = dev->data;

	if (!data->is_dma_frame_count_fixed) {
		data->dma_frame_count = frame_count;
	}
	I2SStart(config->reg_base, hw_frame_count_for(dev, data->dma_frame_count));
}

static void enable_clocks(const struct device *dev)
{
	const struct ti_cc27xx_i2s_cfg *cfg = dev->config;
	struct ti_cc27xx_i2s_data *data = dev->data;

	start_dma(dev, data->dma_frame_count);

	I2SEnableSampleStamp(cfg->reg_base);

	I2SResetWclkCounter(cfg->reg_base);

	if (cfg->module_role == TI_CC27XX_I2S_ROLE_CONTROLLER) {
		CKMDSelectAfclk(cfg->afclk_src);
		I2SEnableControllerClocks(cfg->reg_base);
	}
}

static void init_hw(const struct device *dev, struct ti_cc27xx_i2s_stream_cfg *stream_cfg,
		    uint32_t dma_frame_count)
{
	config_serial_format(dev, stream_cfg, dma_frame_count);
	config_channels(dev);
	config_clocks(dev);
}

static void start_clocks(const struct device *dev, struct ti_cc27xx_i2s_stream_cfg *stream_cfg,
			 uint32_t dma_frame_count)
{
	const struct ti_cc27xx_i2s_cfg *config = dev->config;
	struct ti_cc27xx_i2s_data *data = dev->data;
	unsigned int key;

	/* Prevent standby and idle before enabling AFOSC.
	 * - STANDBY: AFOSC auto-disables on standby entry (CKMD_AFOSCCTL_AUTODIS),
	 *   so the lock must be acquired before startAFOSC to avoid a race.
	 * - RUNTIME_IDLE: I2S DMA pointer registers must be refreshed within one
	 *   WCLK frame period. CPU idle latency can delay the ISR past that
	 *   window, causing the hardware to raise PTR_ERR and halt streaming.
	 */
	pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	pm_policy_state_lock_get(PM_STATE_RUNTIME_IDLE, PM_ALL_SUBSTATES);
	CLKCTLEnable(CLKCTL_BASE, CLKCTL_CLKENSET0_I2S);
	data->clocks_active = true;

	if (config->afclk_src == CKMD_AFCLKSEL_SRC_CLKAF) {
		key = irq_lock();
		PowerLPF3_startAFOSC(config->afosc_freq);
		irq_unlock(key);
	}
	init_hw(dev, stream_cfg, dma_frame_count);
	enable_clocks(dev);
}

static inline bool is_stream_running(struct ti_cc27xx_i2s_stream *stream)
{
	return stream->state == I2S_STATE_RUNNING;
}

static inline bool is_stream_draining(struct ti_cc27xx_i2s_stream *stream)
{
	return (stream->state == I2S_STATE_STOPPING && stream->drain);
}

static inline bool is_stream_stopping(struct ti_cc27xx_i2s_stream *stream)
{
	return (stream->state == I2S_STATE_STOPPING && !stream->drain);
}

static bool is_stream_writeable(struct ti_cc27xx_i2s_stream *stream)
{
	return ((stream->state == I2S_STATE_RUNNING) || (stream->state == I2S_STATE_READY));
}

static bool is_stream_configurable(struct ti_cc27xx_i2s_stream *stream)
{
	return ((stream->state == I2S_STATE_NOT_READY) || (stream->state == I2S_STATE_READY) ||
		(stream->state == I2S_STATE_ERROR));
}

static inline k_timeout_t translate_timeout(int32_t timeout_ms)
{
	switch (timeout_ms) {
	case 0:
		return K_NO_WAIT;
	case SYS_FOREVER_MS:
		return K_FOREVER;
	default:
		return K_MSEC(timeout_ms);
	}
}

static int validate_args_configure(enum i2s_dir dir, const struct i2s_config *cfg)
{
	switch (dir) {
	case I2S_DIR_BOTH:
		LOG_ERR("RX and TX streams must be configured separately");
		return -ENOSYS;
	case I2S_DIR_RX:
	case I2S_DIR_TX:
		break;
	default:
		LOG_ERR("Invalid I2S direction: %d", dir);
		return -EINVAL;
	}

	if (cfg->frame_clk_freq == 0) {
		return 0;
	}

	if (cfg->word_size < 8U || cfg->word_size > 24U) {
		LOG_ERR("Unsupported word size: %u", cfg->word_size);
		return -EINVAL;
	}

	if (cfg->channels < 1U || cfg->channels > 2U) {
		LOG_ERR("Unsupported channel count: %u", cfg->channels);
		return -EINVAL;
	}
	if (cfg->format & I2S_FMT_DATA_ORDER_LSB) {
		LOG_ERR("Unsupported stream format: 0x%02x", cfg->format);
		return -EINVAL;
	}

	uint32_t fmt = cfg->format & I2S_FMT_DATA_FORMAT_MASK;

	if (fmt != I2S_FMT_DATA_FORMAT_I2S && fmt != I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED &&
	    fmt != I2S_FMT_DATA_FORMAT_RIGHT_JUSTIFIED) {
		LOG_ERR("Unsupported data format: 0x%02x", cfg->format);
		return -EINVAL;
	}

	if ((cfg->options & I2S_OPT_BIT_CLK_TARGET) || (cfg->options & I2S_OPT_FRAME_CLK_TARGET)) {
		LOG_ERR("Unsupported operation mode: 0x%02x", cfg->options);
		return -EINVAL;
	}
	if (cfg->mem_slab == NULL) {
		LOG_ERR("Memory slab required");
		return -EINVAL;
	}

	if (cfg->block_size == 0U) {
		LOG_ERR("Invalid block size: %d", cfg->block_size);
		return -EINVAL;
	}
	return 0;
}
static int is_frame_count_valid(uint32_t frame_count)
{
	uint32_t remainder;

	if (frame_count == 0) {
		return false;
	}

	remainder = frame_count % TI_CC27XX_I2S_DMA_MAX_FRAMES_PER_TRANSFER;

	if ((remainder == 0) || (remainder >= TI_CC27XX_I2S_DMA_MIN_FRAMES_PER_TRANSFER)) {
		return true;
	}

	return false;
}

static uint32_t get_bit_rate(const struct device *dev, const struct ti_cc27xx_i2s_stream *stream)
{
	uint32_t data_length;
	uint32_t sample_length;

	sample_length = stream->cfg.before_word_padding;
	sample_length += stream->cfg.bits_per_audio_word;
	sample_length += stream->cfg.after_word_padding;

	data_length = stream->cfg.channel_count * sample_length;
	return data_length * stream->cfg.frame_clk_freq;
}

static bool compute_sck_divider(const struct device *dev, const struct ti_cc27xx_i2s_stream *stream,
				uint32_t *divider)
{
	const struct ti_cc27xx_i2s_cfg *config = dev->config;
	uint32_t out;
	uint32_t bit_rate = get_bit_rate(dev, stream);

	if (bit_rate == 0U) {
		return -EINVAL;
	}

	out = DIV_ROUND_CLOSEST(config->afosc_freq, bit_rate);

	if (IN_RANGE(out, TI_CC27XX_I2S_CLOCK_DIVIDER_MIN, TI_CC27XX_I2S_CLOCK_DIVIDER_MAX)) {
		*divider = out;
		return true;
	}

	return false;
}

static bool compute_ws_divider(const struct device *dev, const struct ti_cc27xx_i2s_stream *stream,
			       uint32_t *divider)
{
	const struct ti_cc27xx_i2s_cfg *config = dev->config;
	uint32_t channel_count = stream->cfg.channel_count;
	uint16_t sample_length = 0;

	switch (config->phase_type) {
	case TI_CC27XX_I2S_PHASE_TYPE_DUAL:

		sample_length += stream->cfg.before_word_padding;
		sample_length += stream->cfg.bits_per_audio_word;
		sample_length += stream->cfg.after_word_padding;

		if (channel_count > TI_CC27XX_I2S_DUAL_PHASE_CHANNEL_MAX) {
			return false;
		}
		break;
	case TI_CC27XX_I2S_PHASE_TYPE_SINGLE:

		sample_length += stream->cfg.before_word_padding;
		sample_length += (stream->cfg.bits_per_audio_word * channel_count);
		sample_length += stream->cfg.after_word_padding;

		if (channel_count > TI_CC27XX_I2S_SINGLE_PHASE_CHANNEL_MAX) {
			return false;
		}
		break;
	default:
		return false;
	}

	*divider = sample_length;
	return true;
}

static int ti_cc27xx_i2s_configure(const struct device *dev, enum i2s_dir dir,
				   const struct i2s_config *i2s_cfg)
{
	struct ti_cc27xx_i2s_data *data = dev->data;
	struct ti_cc27xx_i2s_stream *stream;
	struct ti_cc27xx_i2s_stream_cfg *stream_cfg;
	int ret;

	ret = validate_args_configure(dir, i2s_cfg);
	if (ret < 0) {
		return ret;
	}

	if (dir == I2S_DIR_RX) {
		stream = &data->stream_rx; /* RX stream */
		stream_cfg = &stream->cfg; /* RX configuration */
	} else if (dir == I2S_DIR_TX) {
		stream = &data->stream_tx; /* TX stream */
		stream_cfg = &stream->cfg; /* TX configuration */
	}

	if (stream->state == I2S_STATE_STOPPING) {
		const uint32_t timeout_ms = 50; /* Max wait: ~12x buffer time at 8kHz */
		uint32_t elapsed = 0;

		while (stream->state == I2S_STATE_STOPPING && elapsed < timeout_ms) {
			/* Poll every 2ms */
			k_sleep(K_MSEC(2));
			elapsed += 2;
		}

		if (stream->state == I2S_STATE_STOPPING) {
			LOG_WRN("Timeout waiting for STOPPING->READY transition");
		}
	}

	if (!is_stream_configurable(stream)) {
		/* Stream is busy (RUNNING) */
		return -EINVAL;
	}

	/* On ERROR, the ISR zeroes DMA pointers but leaves transfer descriptors
	 * allocated. Free active_transfer, next_transfer, and queued transfers
	 * to avoid memory slab exhaustion on the next start.
	 */
	if (stream->state == I2S_STATE_ERROR) {
		purge_stream(stream);
	}

	if (i2s_cfg->frame_clk_freq == 0) {
		memset(stream_cfg, 0, sizeof(*stream_cfg)); /* Clear config */
		stream->enabled = false;                    /* Mark disabled */
		stream->drain = false;                      /* Clear drain flag */
		stream->configured = false;                 /* Not configured */
		stream->state = I2S_STATE_NOT_READY;        /* Back to NOT_READY */
		return 0;                                   /* Successfully disabled */
	}

	stream_cfg->frame_clk_freq = i2s_cfg->frame_clk_freq;    /* Sample rate (Hz) */
	stream->audio_mem_slab = i2s_cfg->mem_slab;              /* Memory pool for buffers */
	stream->cfg.bytes_per_audio_block = i2s_cfg->block_size; /* DMA buffer size (bytes) */

	switch (i2s_cfg->format & I2S_FMT_DATA_FORMAT_MASK) {
	case I2S_FMT_DATA_FORMAT_I2S:
		data->invert_ws = true;              /* I2S: WS is inverted */
		stream_cfg->data_shift = 1;          /* MSB sent 1 clock after WS edge */
		stream_cfg->before_word_padding = 0; /* No padding before sample */
		stream_cfg->after_word_padding = 0;  /* No padding after sample */
		break;
	case I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED:
		data->invert_ws = false;
		stream_cfg->data_shift = 0;
		stream_cfg->before_word_padding = 0;
		stream_cfg->after_word_padding = 0;
		break;
	case I2S_FMT_DATA_FORMAT_RIGHT_JUSTIFIED:
		data->invert_ws = false;
		stream_cfg->data_shift = 0;
		stream_cfg->before_word_padding = 0;
		stream_cfg->after_word_padding = 0;
		break;
	}

	if (i2s_cfg->word_size <= 16U) {
		stream_cfg->bits_per_memory_word = TI_CC27XX_I2S_MEMORY_LENGTH_16BITS;
		stream_cfg->bits_per_audio_word = i2s_cfg->word_size;
	} else {
		stream_cfg->bits_per_memory_word = TI_CC27XX_I2S_MEMORY_LENGTH_32BITS;
		stream_cfg->bits_per_audio_word = i2s_cfg->word_size;
	}

	switch (i2s_cfg->channels) {
	case TI_CC27XX_I2S_CHANNEL_COUNT_1:
		stream_cfg->channel_bitmask = TI_CC27XX_I2S_CHANNEL_BITMASK_MONO;
		break;
	case TI_CC27XX_I2S_CHANNEL_COUNT_2:
		stream_cfg->channel_bitmask = TI_CC27XX_I2S_CHANNEL_BITMASK_STEREO;
		break;
	}

	stream_cfg->timeout = translate_timeout(i2s_cfg->timeout);

	stream_cfg->channel_count = i2s_cfg->channels;
	stream_cfg->bytes_per_frame =
		(stream_cfg->bits_per_memory_word / 8) * stream_cfg->channel_count;

	stream->cfg.frames_per_audio_block = i2s_cfg->block_size / stream_cfg->bytes_per_frame;

	if (!is_frame_count_valid(stream_cfg->frames_per_audio_block)) {
		return -EINVAL;
	}

	if (!compute_sck_divider(dev, stream, &data->sck_divider)) {
		return -EINVAL;
	}

	if (!compute_ws_divider(dev, stream, &data->ws_divider)) {
		return -EINVAL;
	}

	stream->i2s_config_copy = *i2s_cfg;
	stream->state = I2S_STATE_READY;
	stream->configured = true;
	return 0;
}

static const struct i2s_config *ti_cc27xx_i2s_config_get(const struct device *dev, enum i2s_dir dir)
{
	struct ti_cc27xx_i2s_data *data = dev->data;
	struct ti_cc27xx_i2s_stream *stream;

	if (dir == I2S_DIR_BOTH) {
		return NULL;
	}

	stream = (dir == I2S_DIR_TX) ? &data->stream_tx : &data->stream_rx;
	return stream->configured ? &stream->i2s_config_copy : NULL;
}

static int validate_args_trigger(enum i2s_dir dir, enum i2s_trigger_cmd cmd)
{
	/* Verify direction is one of: TX only, RX only, or both */
	switch (dir) {
	case I2S_DIR_BOTH:      /* Bidirectional: simultaneous TX and RX */
	case I2S_DIR_RX:        /* Receive only: audio input */
	case I2S_DIR_TX:        /* Transmit only: audio output */
		break;          /* Valid direction, continue */
	default:                /* Unknown/unsupported direction value */
		return -EINVAL; /* Return error code: invalid argument */
	}

	/* Verify command is a supported trigger operation */
	switch (cmd) {
	case I2S_TRIGGER_START:   /* Begin streaming audio */
	case I2S_TRIGGER_STOP:    /* Stop after current buffer completes */
	case I2S_TRIGGER_DRAIN:   /* Stop after all queued buffers complete */
	case I2S_TRIGGER_DROP:    /* Abort immediately, discard buffers */
	case I2S_TRIGGER_PREPARE: /* Transition to READY state */
		break;            /* Valid command, continue */
	default:                  /* Unknown/unsupported command value */
		return -EINVAL;   /* Return error code: invalid argument */
	}

	return 0; /* All validation passed, return success */
}

static inline bool is_i2s_config_equal(const struct i2s_config *a, const struct i2s_config *b)
{
	struct i2s_config confa = *a;
	struct i2s_config confb = *b;

	confa.mem_slab = 0;
	confb.mem_slab = 0;
	return (memcmp(&confa, &confb, sizeof(struct i2s_config)) == 0);
}

static int validate_args_trigger_start(enum i2s_dir dir, struct ti_cc27xx_i2s_stream *stream_tx,
				       struct ti_cc27xx_i2s_stream *stream_rx)
{
	/* For bidirectional I2S, TX and RX must use identical clock/format settings */
	/* Check if user requested bidirectional mode */
	if (dir == I2S_DIR_BOTH) {
		/* Get pointer to TX stream's saved configuration */
		struct i2s_config *tx_cfg = &stream_tx->i2s_config_copy;
		/* Get pointer to RX stream's saved configuration */
		struct i2s_config *rx_cfg = &stream_rx->i2s_config_copy;

		/* Compare configurations: sample rate, word size, channels, etc. */
		if (!is_i2s_config_equal(rx_cfg, tx_cfg)) {
			/* Return I/O error: configs must match for bidirectional */
			return -EIO;
		}
	}

	/* User wants TX only, check RX status */
	if (dir == I2S_DIR_TX && stream_rx->enabled) {
		/* Error: RX already using shared I2S clocks (SCK/WS) */
		return -EIO;
	}

	/* Cannot start RX alone if TX is already running (hardware shares clock) */
	/* User wants RX only, check TX status */
	if (dir == I2S_DIR_RX && stream_tx->enabled) {
		/* Error: TX already using shared I2S clocks (SCK/WS) */
		return -EIO;
	}

	/* RX must be in READY state */
	if (dir == I2S_DIR_RX || dir == I2S_DIR_BOTH) {
		if (stream_rx->state != I2S_STATE_READY) {
			return -EIO;
		}
	}

	/* TX must be in READY state */
	if (dir == I2S_DIR_TX || dir == I2S_DIR_BOTH) {
		if (stream_tx->state != I2S_STATE_READY) {
			return -EIO;
		}
	}

	return 0; /* All preconditions satisfied, safe to start */
}

static int compute_fixed_frame_count(uint32_t frames_per_audio_block)
{
	int ret = -EINVAL;
	uint32_t max = TI_CC27XX_I2S_DMA_MAX_FRAMES_PER_TRANSFER;
	uint32_t min = TI_CC27XX_I2S_DMA_MIN_FRAMES_PER_TRANSFER;

	if (IN_RANGE(frames_per_audio_block, min, max)) {
		ret = frames_per_audio_block;
	} else if (frames_per_audio_block > max) {
		if (frames_per_audio_block % 2) {
			ret = -EINVAL;
		} else if ((frames_per_audio_block % max) == 0) {
			ret = max;
		} else if (frames_per_audio_block < (2 * max)) {
			ret = frames_per_audio_block / 2;
		} else {
			ret = -EINVAL;
		}
	}

	return ret;
}

static uint32_t get_next_frame_count(struct ti_cc27xx_i2s_data *data,
				     struct ti_cc27xx_i2s_transfer *tr)
{
	uint32_t frames_left; /* Frames remaining to transfer in this buffer */

	/* Bidirectional mode: use fixed frame count to keep TX and RX synchronized */
	if (data->is_dma_frame_count_fixed) {
		/* Fixed count (set at START time) */
		return data->dma_frame_count;
	}

	/* Unidirectional mode: calculate frames left in current transfer */
	frames_left = tr->frames_total - tr->frames_transferred; /* Remaining frames */
	/* Return smaller of: frames left, or hardware max (256 frames) */
	return MIN(frames_left, TI_CC27XX_I2S_DMA_MAX_FRAMES_PER_TRANSFER);
}

static void *get_next_mem_block(struct ti_cc27xx_i2s_stream *stream,
				struct ti_cc27xx_i2s_transfer *tr)
{
	struct ti_cc27xx_i2s_transfer *next_tr; /* Next transfer in queue */
	/* Number of bytes in one audio frame (channels × bytes_per_sample) */
	uint32_t bytes_per_frame = stream->cfg.bytes_per_frame;
	/* Frames still remaining in current transfer after this DMA completes */
	uint32_t frames_left = tr->frames_total - tr->frames_transferred;
	uint32_t bytes_offset; /* Byte offset into current buffer */

	/* Check if current transfer will need another DMA pass */
	if (frames_left > tr->frames_scheduled) {
		/* Current buffer not finished: calculate offset for continuation */
		bytes_offset = (tr->frames_transferred + tr->frames_scheduled) *
			       bytes_per_frame; /* Offset = frames processed × frame size */
		/* Return pointer to next chunk within same buffer */
		return (uint8_t *)tr->mem_block + bytes_offset;
	}

	/* Current buffer will complete: look for next transfer in queue */
	/* Peek at next queued transfer */
	next_tr = stream->peek_next_transfer(stream);
	/* Return next buffer address if exists, NULL if queue empty */
	return next_tr ? next_tr->mem_block : 0;
}

static int prep_tx_stream(const struct device *dev, struct ti_cc27xx_i2s_stream *stream,
			  void **mem_block_1, void **mem_block_2)
{
	/* Get runtime data containing DMA frame count settings */
	struct ti_cc27xx_i2s_data *data = dev->data;
	/* Get currently active transfer (may be NULL if starting fresh) */
	struct ti_cc27xx_i2s_transfer *tr = stream->active_transfer;

	/* If no active transfer, need to get first buffer from user queue */
	if (tr == NULL) {
		/* Dequeue first transfer from user-submitted queue (non-blocking) */
		tr = k_fifo_get(&stream->queue, K_NO_WAIT);
		/* Queue is empty, no data to transmit */
		if (tr == NULL) {
			return -ENOMEM;
		}
		/* Make this the active transfer (will be processed by DMA) */
		stream->active_transfer = tr;
	}

	/* Set first buffer address (this will be loaded into hardware immediately) */
	*mem_block_1 = tr->mem_block;

	/* Calculate how many frames will be transferred in next DMA operation */
	tr->frames_scheduled = get_next_frame_count(data, tr);
	/* Get address for second buffer (for double-buffering), may be NULL */
	*mem_block_2 = get_next_mem_block(stream, tr);

	return 0;
}

static inline struct ti_cc27xx_i2s_transfer *alloc_rx_transfer(struct ti_cc27xx_i2s_stream *stream)
{
	struct k_mem_slab *transfer_slab = stream->transfer_mem_slab;
	struct k_mem_slab *audio_slab = stream->audio_mem_slab;
	struct ti_cc27xx_i2s_transfer *transfer;
	size_t buf_size = stream->cfg.bytes_per_audio_block;
	void *buf;

	if (k_mem_slab_alloc(transfer_slab, (void **)&transfer, K_NO_WAIT)) {
		return NULL;
	}

	if (k_mem_slab_alloc(audio_slab, &buf, K_NO_WAIT)) {
		k_mem_slab_free(transfer_slab, transfer);
		return NULL;
	}

	memset(transfer, 0, sizeof(*transfer));
	memset(buf, 0, buf_size);
	transfer->own_mem_slab = transfer_slab;
	transfer->audio_mem_slab = audio_slab;
	transfer->mem_block = buf;
	transfer->frames_total = stream->cfg.frames_per_audio_block;

	return transfer;
}

static inline void purge_transfer(struct ti_cc27xx_i2s_transfer *transfer)
{
	k_mem_slab_free(transfer->audio_mem_slab, transfer->mem_block);
	k_mem_slab_free(transfer->own_mem_slab, transfer);
}

static int prep_rx_stream(const struct device *dev, struct ti_cc27xx_i2s_stream *stream,
			  void **mem_block_1, void **mem_block_2)
{
	struct ti_cc27xx_i2s_data *data = dev->data;
	struct ti_cc27xx_i2s_transfer *tr;

	stream->active_transfer = alloc_rx_transfer(stream);
	if (stream->active_transfer == NULL) {
		return -ENOMEM;
	}

	if (stream->next_transfer == NULL) {
		stream->next_transfer = alloc_rx_transfer(stream);
	}

	if (stream->next_transfer == NULL) {
		purge_transfer(stream->active_transfer);
		stream->active_transfer = NULL;
		return -ENOMEM;
	}

	tr = stream->active_transfer;
	*mem_block_1 = tr->mem_block;

	tr->frames_scheduled = get_next_frame_count(data, tr);
	*mem_block_2 = get_next_mem_block(stream, tr);

	return 0;
}

static inline void enable_common_irqs(const struct device *dev)
{
	const struct ti_cc27xx_i2s_cfg *config = dev->config;
	/* Enable I2S Hardware Interrupts */
	I2SEnableInt(config->reg_base, (uint32_t)I2S_INT_TIMEOUT | (uint32_t)I2S_INT_BUS_ERR |
					       (uint32_t)I2S_INT_WCLK_ERR |
					       (uint32_t)I2S_INT_PTR_ERR);
}

static inline void enable_stream_irq(const struct device *dev, struct ti_cc27xx_i2s_stream *stream)
{
	const struct ti_cc27xx_i2s_cfg *config = dev->config;

	I2SEnableInt(config->reg_base, stream->irq_flag);
}

static void start_stream(const struct device *dev, struct ti_cc27xx_i2s_stream *stream)
{
	enable_stream_irq(dev, stream);

	stream->enabled = true;

	stream->state = I2S_STATE_RUNNING;
}

static int handle_trigger_start(const struct device *dev, enum i2s_dir dir,
				struct ti_cc27xx_i2s_stream *stream_tx,
				struct ti_cc27xx_i2s_stream *stream_rx)
{
	/* Get device tree config: register base, pin config, clock sources */
	const struct ti_cc27xx_i2s_cfg *config = dev->config;
	/* Get runtime data: stream state, DMA pointers, memory pools */
	struct ti_cc27xx_i2s_data *data = dev->data;
	/* Generic stream pointer (set to TX or RX based on direction) */
	struct ti_cc27xx_i2s_stream *stream;
	/* Stream-specific config: block size, channel count, sample rate */
	struct ti_cc27xx_i2s_stream_cfg *stream_cfg;
	/* Flag: true if RX stream should be started */
	bool start_rx = false;
	/* Flag: true if TX stream should be started */
	bool start_tx = false;
	/* Number of audio frames per DMA transfer (must match hardware alignment) */
	uint32_t dma_frame_count;
	void *tx_mem_addr_1 = NULL; /* First TX buffer (loaded immediately) */
	void *tx_mem_addr_2 = NULL; /* Second TX buffer (queued for next DMA) */
	void *rx_mem_addr_1 = NULL; /* First RX buffer (loaded immediately) */
	void *rx_mem_addr_2 = NULL; /* Second RX buffer (queued for next DMA) */
	uint32_t trig;              /* Sample counter value at which streaming starts */
	int ret;                    /* Return value for error checking */

	/* Determine which streams to activate */
	/* If RX is requested */
	if (dir == I2S_DIR_RX || dir == I2S_DIR_BOTH) {
		start_rx = true;              /* Set RX flag */
		stream = stream_rx;           /* Point to RX stream structure */
		stream_cfg = &stream_rx->cfg; /* Use RX stream config */
	}

	/* If TX is requested */
	if (dir == I2S_DIR_TX || dir == I2S_DIR_BOTH) {
		start_tx = true;    /* Set TX flag */
		stream = stream_tx; /* Point to TX stream structure (overwrites if BOTH) */
		stream_cfg = &stream_tx->cfg; /* Use TX stream config (overwrites if BOTH) */
	}

	/* Bidirectional requires synchronized DMA */
	if (dir == I2S_DIR_BOTH) {
		/* Calculate aligned frame count (must be multiple of MIN, within MAX) */
		ret = compute_fixed_frame_count(
			stream_cfg->frames_per_audio_block); /* User's requested block size */
		/* Frame count validation failed */
		if (ret < 0) {
			return ret;
		}
		data->dma_frame_count = ret;           /* Store validated frame count */
		data->is_dma_frame_count_fixed = true; /* Lock frame count for sync */
	} else {
		/* Unidirectional: frame count can vary per transfer */
		data->is_dma_frame_count_fixed = false; /* Allow dynamic frame count */
	}

	/* Prepare TX: dequeue up to 2 buffers from user queue, get addresses */
	/* If TX stream is being started */
	if (start_tx) {
		/* Dequeue TX buffers, get memory addresses, update stream state */
		ret = prep_tx_stream(dev, stream_tx, &tx_mem_addr_1, &tx_mem_addr_2);
		/* Buffer preparation failed (queue empty, etc.) */
		if (ret < 0) {
			return ret;
		}
		/* Get actual frame count from prepared transfer (may differ from request) */
		dma_frame_count = stream->active_transfer->frames_scheduled;
	}

	/* Prepare RX: dequeue up to 2 buffers from user queue, get addresses */
	/* If RX stream is being started */
	if (start_rx) {
		/* Dequeue RX buffers, get memory addresses, update stream state */
		ret = prep_rx_stream(dev, stream_rx, &rx_mem_addr_1, &rx_mem_addr_2);
		/* Buffer preparation failed (queue empty, etc.) */
		if (ret < 0) {
			return ret;
		}
		/* Get actual frame count from prepared transfer (may differ from request) */
		dma_frame_count = stream->active_transfer->frames_scheduled;
	}

	start_clocks(dev, stream_cfg, dma_frame_count);

	/* Clear interrupts */
	I2SClearInt(config->reg_base, I2S_INT_ALL);
	enable_common_irqs(dev);

	/* Calculate trigger point */
	trig = HWREGH(config->reg_base + I2S_O_STMPWCNT);
	trig += config->startup_delay;

	/* SimpleLink I2SLPF3.c:295-341: I2S_startRead() / I2S_startWrite() */
	if (start_tx) {
		/* Load first TX buffer into OUTPTR */
		stream_tx->set_dma_pointer(dev, tx_mem_addr_1);
		/* Enable TX stream (sets enabled=true, state=RUNNING) */
		start_stream(dev, stream_tx);
		/* Preload second TX buffer into OUTPTRNEXT (NULL=0 if no second buffer) */
		stream_tx->set_dma_pointer(dev, tx_mem_addr_2);
		stream_tx->set_stamp_trigger(dev, trig);
	}

	if (start_rx) {
		/* Same race-free ordering for RX: INPTR must be valid before enabled=true. */
		/* Load first RX buffer into INPTR */
		stream_rx->set_dma_pointer(dev, rx_mem_addr_1);
		/* Enable RX stream (sets enabled=true, state=RUNNING) */
		start_stream(dev, stream_rx);
		/* Preload second RX buffer into INPTRNEXT */
		stream_rx->set_dma_pointer(
			dev, rx_mem_addr_2 ? rx_mem_addr_2 : stream_rx->next_transfer->mem_block);
		stream_rx->set_stamp_trigger(dev, trig);
	}

	return 0;
}

static int validate_args_trigger_drain(enum i2s_dir dir, struct ti_cc27xx_i2s_stream *stream_tx,
				       struct ti_cc27xx_i2s_stream *stream_rx)
{
	if (dir == I2S_DIR_RX || dir == I2S_DIR_BOTH) {
		if (stream_rx->state != I2S_STATE_RUNNING) {
			return -EIO;
		}
	}

	if (dir == I2S_DIR_TX || dir == I2S_DIR_BOTH) {
		if (stream_tx->state != I2S_STATE_RUNNING) {
			return -EIO;
		}
	}

	return 0;
}

static int handle_trigger_drain(const struct device *dev, enum i2s_dir dir,
				struct ti_cc27xx_i2s_stream *stream_tx,
				struct ti_cc27xx_i2s_stream *stream_rx)
{
	if (dir == I2S_DIR_RX || dir == I2S_DIR_BOTH) {
		stream_rx->state = I2S_STATE_STOPPING;
		stream_rx->drain = true;
	}

	if (dir == I2S_DIR_TX || dir == I2S_DIR_BOTH) {
		stream_tx->state = I2S_STATE_STOPPING;
		stream_tx->drain = true;
	}

	return 0;
}

static int validate_args_trigger_stop(enum i2s_dir dir, struct ti_cc27xx_i2s_stream *stream_tx,
				      struct ti_cc27xx_i2s_stream *stream_rx)
{
	if (dir == I2S_DIR_RX || dir == I2S_DIR_BOTH) {
		if (stream_rx->state != I2S_STATE_RUNNING) {
			return -EIO;
		}
	}

	if (dir == I2S_DIR_TX || dir == I2S_DIR_BOTH) {
		if (stream_tx->state != I2S_STATE_RUNNING) {
			return -EIO;
		}
	}

	return 0;
}

static void handle_trigger_stop(const struct device *dev, enum i2s_dir dir,
				struct ti_cc27xx_i2s_stream *stream_tx,
				struct ti_cc27xx_i2s_stream *stream_rx)
{
	if (dir == I2S_DIR_RX || dir == I2S_DIR_BOTH) {
		stream_rx->state = I2S_STATE_STOPPING;
	}

	if (dir == I2S_DIR_TX || dir == I2S_DIR_BOTH) {
		stream_tx->state = I2S_STATE_STOPPING;
	}
}

static int validate_args_trigger_drop(enum i2s_dir dir, struct ti_cc27xx_i2s_stream *stream_tx,
				      struct ti_cc27xx_i2s_stream *stream_rx)
{
	if (dir == I2S_DIR_RX || dir == I2S_DIR_BOTH) {
		if (stream_rx->state == I2S_STATE_NOT_READY) {
			return -EIO;
		}
	}

	if (dir == I2S_DIR_TX || dir == I2S_DIR_BOTH) {
		if (stream_tx->state == I2S_STATE_NOT_READY) {
			return -EIO;
		}
	}

	return 0;
}

static void purge_stream(struct ti_cc27xx_i2s_stream *stream)
{
	if (stream->active_transfer) {
		purge_transfer(stream->active_transfer);
		stream->active_transfer = NULL;
	}

	if (stream->next_transfer) {
		purge_transfer(stream->next_transfer);
		stream->next_transfer = NULL;
	}

	purge_queue(stream);
}

static void stop_dma(const struct device *dev)
{
	const struct ti_cc27xx_i2s_cfg *config = dev->config;
	struct ti_cc27xx_i2s_data *data = dev->data;

	data->dma_frame_count = 0;
	data->is_dma_frame_count_fixed = false;
	I2SStop(config->reg_base);
}

static inline void disable_stream_irq(const struct device *dev, struct ti_cc27xx_i2s_stream *stream)
{
	const struct ti_cc27xx_i2s_cfg *config = dev->config;

	I2SDisableInt(config->reg_base, stream->irq_flag);
	I2SClearInt(config->reg_base, stream->irq_flag);
}

static inline void stop_stream(const struct device *dev, struct ti_cc27xx_i2s_stream *stream)
{
	disable_stream_irq(dev, stream);
	stream->set_dma_pointer(dev, 0);
	stream->set_stamp_trigger(dev, I2S_STMP_SATURATION);
	stream->enabled = false;
	stream->drain = false;
}

static void disable_clocks(const struct device *dev)
{
	const struct ti_cc27xx_i2s_cfg *config = dev->config;
	struct ti_cc27xx_i2s_data *data = dev->data;

	if (!data->clocks_active) {
		return;
	}
	data->clocks_active = false;

	I2SClearInt(config->reg_base, I2S_INT_ALL);
	I2SDisableInt(config->reg_base, (uint32_t)I2S_INT_TIMEOUT | (uint32_t)I2S_INT_BUS_ERR |
						(uint32_t)I2S_INT_WCLK_ERR |
						(uint32_t)I2S_INT_PTR_ERR);

	I2SDisableSampleStamp(config->reg_base);
	/* Disable controller generated clocks (if they were previously enabled) */
	I2SDisableControllerClocks(config->reg_base);
	/* Disable the audio clock */
	CKMDSelectAfclk(CKMD_AFCLK_SOURCE_NONE);
	/* Stop AFOSC if CLKAF was selected (maps to I2S_close → stopAFOSC) */
	if (config->afclk_src == CKMD_AFCLKSEL_SRC_CLKAF) {
		PowerLPF3_stopAFOSC();
	}

	pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	pm_policy_state_lock_put(PM_STATE_RUNTIME_IDLE, PM_ALL_SUBSTATES);
	CLKCTLDisable(CLKCTL_BASE, CLKCTL_CLKENSET0_I2S);
}

static int handle_trigger_drop(const struct device *dev, enum i2s_dir dir,
			       struct ti_cc27xx_i2s_stream *stream_tx,
			       struct ti_cc27xx_i2s_stream *stream_rx)
{
	const struct ti_cc27xx_i2s_cfg *config = dev->config;
	int32_t timeout;

	/* Handle RX stream drop if requested */
	if (dir == I2S_DIR_RX || dir == I2S_DIR_BOTH) {

		stop_stream(dev, stream_rx);

		for (timeout = TI_CC27XX_I2S_HW_STOP_TIMEOUT_US;
		     I2SGetInPointer(config->reg_base) != 0 && timeout > 0; timeout--) {
			k_busy_wait(1);
		}
		if (timeout <= 0) {
			LOG_WRN("RX DMA pointer did not clear within timeout");
		}

		purge_stream(stream_rx);

		stream_rx->state = I2S_STATE_READY;
	}

	/* Handle TX stream drop if requested */
	if (dir == I2S_DIR_TX || dir == I2S_DIR_BOTH) {

		stop_stream(dev, stream_tx);

		for (timeout = TI_CC27XX_I2S_HW_STOP_TIMEOUT_US;
		     I2SGetOutPointer(config->reg_base) != 0 && timeout > 0; timeout--) {
			k_busy_wait(1);
		}
		if (timeout <= 0) {
			LOG_WRN("TX DMA pointer did not clear within timeout");
		}

		purge_stream(stream_tx);

		stream_tx->state = I2S_STATE_READY;
	}

	if (!stream_tx->enabled && !stream_rx->enabled) {
		stop_dma(dev);
		disable_clocks(dev);
	}

	return 0;
}

static int validate_args_trigger_prepare(enum i2s_dir dir, struct ti_cc27xx_i2s_stream *stream_tx,
					 struct ti_cc27xx_i2s_stream *stream_rx)
{
	if (dir == I2S_DIR_RX || dir == I2S_DIR_BOTH) {
		if (stream_rx->state != I2S_STATE_ERROR) {
			return -EIO;
		}
	}

	if (dir == I2S_DIR_TX || dir == I2S_DIR_BOTH) {
		if (stream_tx->state != I2S_STATE_ERROR) {
			return -EIO;
		}
	}

	return 0;
}

static int handle_trigger_prepare(const struct device *dev, enum i2s_dir dir,
				  struct ti_cc27xx_i2s_stream *stream_tx,
				  struct ti_cc27xx_i2s_stream *stream_rx)
{
	if (dir == I2S_DIR_RX || dir == I2S_DIR_BOTH) {
		purge_stream(stream_rx);
		stream_rx->state = I2S_STATE_READY;
	}

	if (dir == I2S_DIR_TX || dir == I2S_DIR_BOTH) {
		purge_stream(stream_tx);
		stream_tx->state = I2S_STATE_READY;
	}

	return 0;
}

static int ti_cc27xx_i2s_trigger(const struct device *dev, enum i2s_dir dir,
				 enum i2s_trigger_cmd cmd)
{
	/* Get runtime data structure containing stream states and queues */
	struct ti_cc27xx_i2s_data *data = dev->data;
	/* Get pointer to RX stream structure */
	struct ti_cc27xx_i2s_stream *stream_rx = &data->stream_rx;
	/* Get pointer to TX stream structure */
	struct ti_cc27xx_i2s_stream *stream_tx = &data->stream_tx;
	/* Return value for error checking */
	int ret;

	/* Validate direction and command type */
	ret = validate_args_trigger(dir, cmd);
	/* Validation failed (invalid direction or command) */
	if (ret < 0) {
		return ret;
	}

	/* Dispatch to command-specific handler based on trigger command */
	switch (cmd) {
	case I2S_TRIGGER_START:
		ret = validate_args_trigger_start(dir, stream_tx, stream_rx);
		if (ret < 0) {
			return ret;
		}

		return handle_trigger_start(dev, dir, stream_tx, stream_rx);
	case I2S_TRIGGER_DRAIN:
		ret = validate_args_trigger_drain(dir, stream_tx, stream_rx);
		if (ret < 0) {
			return ret;
		}

		return handle_trigger_drain(dev, dir, stream_tx, stream_rx);
	case I2S_TRIGGER_STOP:
		ret = validate_args_trigger_stop(dir, stream_tx, stream_rx);
		if (ret < 0) {
			return ret;
		}

		handle_trigger_stop(dev, dir, stream_tx, stream_rx);
		return 0;
	case I2S_TRIGGER_DROP:
		ret = validate_args_trigger_drop(dir, stream_tx, stream_rx);
		if (ret < 0) {
			return ret;
		}

		return handle_trigger_drop(dev, dir, stream_tx, stream_rx);
	case I2S_TRIGGER_PREPARE:
		ret = validate_args_trigger_prepare(dir, stream_tx, stream_rx);
		if (ret < 0) {
			return ret;
		}

		return handle_trigger_prepare(dev, dir, stream_tx, stream_rx);
	/* Unknown command value (should never happen after validation) */
	default:
		return -EINVAL;
	}
}

static inline void free_transfer(struct ti_cc27xx_i2s_transfer *transfer)
{
	k_mem_slab_free(transfer->own_mem_slab, transfer);
}
static int ti_cc27xx_i2s_read(const struct device *dev, void **mem_block, size_t *size)
{
	struct ti_cc27xx_i2s_data *data = dev->data;
	struct ti_cc27xx_i2s_stream *stream = &data->stream_rx;
	struct ti_cc27xx_i2s_transfer *transfer;
	k_timeout_t timeout;
	int32_t err = 0;

	if (stream->state == I2S_STATE_NOT_READY) {
		return -EIO;
	}

	if (stream->state == I2S_STATE_ERROR) {
		timeout = K_NO_WAIT;
		err = -EIO;
	} else {
		timeout = stream->cfg.timeout;
		err = -EAGAIN;
	}

	transfer = k_fifo_get(&stream->queue, timeout);
	if (transfer == NULL) {
		return err;
	}

	*mem_block = transfer->mem_block;
	*size = transfer->frames_transferred * stream->cfg.bytes_per_frame;

	free_transfer(transfer);

	return 0;
}

static int validate_args_write(const struct device *dev, void *mem_block, size_t size)
{
	struct ti_cc27xx_i2s_data *data = dev->data;
	struct ti_cc27xx_i2s_stream *stream = &data->stream_tx;
	uint32_t frame_count = size / stream->cfg.bytes_per_frame;

	if (size > ((size_t)stream->cfg.bytes_per_audio_block)) {
		return -EINVAL;
	}

	if (!is_frame_count_valid(frame_count)) {
		return -EINVAL;
	}

	if ((data->dir == I2S_DIR_BOTH) && (size % stream->cfg.bytes_per_audio_block)) {
		return -EINVAL;
	}

	return 0;
}

static inline bool is_next_pointer_null(const struct device *dev)
{
	const struct ti_cc27xx_i2s_cfg *config = dev->config;

	return !(bool)I2SGetOutPointerNext(config->reg_base);
}

static int ti_cc27xx_i2s_write(const struct device *dev, void *mem_block, size_t size)
{
	struct ti_cc27xx_i2s_data *data = dev->data;
	struct ti_cc27xx_i2s_stream *stream = &data->stream_tx;
	struct k_mem_slab *transfer_slab = stream->transfer_mem_slab;
	struct k_mem_slab *audio_slab = stream->audio_mem_slab;
	k_timeout_t timeout = stream->cfg.timeout;
	struct ti_cc27xx_i2s_transfer *transfer;
	int ret;

	ret = validate_args_write(dev, mem_block, size);
	if (ret < 0) {
		return ret;
	}

	if (!is_stream_writeable(stream)) {
		return -EIO;
	}

	ret = k_mem_slab_alloc(transfer_slab, (void **)&transfer, timeout);
	if (ret == -ENOMEM) {
		return -EBUSY;
	} else if (ret < 0) {
		return ret;
	}

	memset(transfer, 0, sizeof(*transfer));
	transfer->own_mem_slab = transfer_slab;
	transfer->audio_mem_slab = audio_slab;
	transfer->mem_block = mem_block;
	transfer->size = size;
	transfer->frames_total = size / stream->cfg.bytes_per_frame;

	k_fifo_put(&stream->queue, transfer);

	if (is_next_pointer_null(dev) && stream->state == I2S_STATE_RUNNING) {
		stream->set_dma_pointer(dev, transfer->mem_block);
	}

	return 0;
}

static inline bool update_transfer(struct ti_cc27xx_i2s_transfer *transfer)
{
	if (transfer == NULL) {
		return false;
	}

	transfer->frames_transferred += transfer->frames_scheduled;

	return (transfer->frames_transferred == transfer->frames_total);
}

static inline bool is_int_dma_in(uint32_t status)
{
	return (bool)(status & (uint32_t)I2S_INT_DMA_IN);
}

static inline bool is_int_dma_out(uint32_t status)
{
	return (bool)(status & (uint32_t)I2S_INT_DMA_OUT);
}

static inline bool is_int_ptr_err(uint32_t status)
{
	return (bool)(status & (uint32_t)I2S_INT_PTR_ERR);
}

static inline bool is_int_in_ptr_err(const struct device *dev, uint32_t status)
{
	const struct ti_cc27xx_i2s_cfg *config = dev->config;

	return (is_int_dma_in(status) && is_int_ptr_err(status) &&
		!(bool)I2SGetInPointer(config->reg_base));
}

static inline bool is_int_out_ptr_err(const struct device *dev, uint32_t status)
{
	const struct ti_cc27xx_i2s_cfg *config = dev->config;

	return (is_int_dma_out(status) && is_int_ptr_err(status) &&
		!(bool)I2SGetOutPointer(config->reg_base));
}

static inline bool is_int_timeout(uint32_t status)
{
	return (bool)(status & (uint32_t)I2S_INT_TIMEOUT);
}

static inline bool is_int_bus_err(uint32_t status)
{
	return (bool)(status & (uint32_t)I2S_INT_BUS_ERR);
}

static inline bool is_int_wclk_err(uint32_t status)
{
	return (bool)(status & (uint32_t)I2S_INT_WCLK_ERR);
}

static void ti_cc27xx_i2s_isr(const struct device *dev)
{
	/* Get hardware config (register base, clock settings) from device tree */
	const struct ti_cc27xx_i2s_cfg *config = dev->config;
	/* Get runtime data (stream state, DMA pointers, queues) */
	struct ti_cc27xx_i2s_data *data = dev->data;
	/* Pointer to TX stream structure (transmit direction) */
	struct ti_cc27xx_i2s_stream *stream_tx = &data->stream_tx;
	/* Pointer to RX stream structure (receive direction) */
	struct ti_cc27xx_i2s_stream *stream_rx = &data->stream_rx;
	/* Transfer descriptor pointer (used for both TX and RX operations) */
	struct ti_cc27xx_i2s_transfer *tr;
	/* Next DMA buffer address (for preloading OUTPTRNXT/INPTRNXT) */
	void *next_addr;
	/* Flag: true when current transfer has transmitted/received all frames */
	bool tr_completed;
	/* Read interrupt status register (false = raw status, not masked) */
	uint32_t status = I2SIntStatus(config->reg_base, false);
	/* Flag: true if any fatal error occurred (clock timeout, bus error, etc.) */
	bool error = false;

	bool full_teardown = false;

	if (is_int_out_ptr_err(dev, status) && stream_tx->enabled) {
		/* Stop TX hardware: disable interrupts, clear OUTPTR/OUTPTRNXT */
		stop_stream(dev, stream_tx);
		/* Check if underrun was expected (graceful drain/stop) */
		if (is_stream_draining(stream_tx) || is_stream_stopping(stream_tx)) {
			/* Expected underrun during drain/stop → clean completion */
			stream_tx->state = I2S_STATE_READY;
		} else {
			/* Unexpected underrun during active streaming → error */
			stream_tx->state = I2S_STATE_ERROR;
		}
	}

	if (is_int_in_ptr_err(dev, status) && stream_rx->enabled) {

		if (is_stream_draining(stream_rx) && stream_rx->active_transfer != NULL) {
			update_transfer(stream_rx->active_transfer);
			k_fifo_put(&stream_rx->queue, stream_rx->active_transfer);
			stream_rx->active_transfer = NULL;
		}
		stop_stream(dev, stream_rx);
		if (is_stream_draining(stream_rx) || is_stream_stopping(stream_rx)) {
			stream_rx->state = I2S_STATE_READY;
		} else {
			stream_rx->state = I2S_STATE_ERROR;
		}
	}

	if (is_int_dma_out(status)) {

		tr_completed = update_transfer(stream_tx->active_transfer);

		if (tr_completed) {

			purge_transfer(stream_tx->active_transfer);
			stream_tx->active_transfer = NULL;
		}

		if (!stream_tx->enabled) {
			goto int_dma_out_done;
		}

		if (tr_completed &&
		    (is_stream_running(stream_tx) || is_stream_draining(stream_tx))) {
			/* Dequeue next TX transfer from application queue (non-blocking) */
			stream_tx->active_transfer = k_fifo_get(&stream_tx->queue, K_NO_WAIT);
		}

		if (stream_tx->active_transfer) {
			tr = stream_tx->active_transfer;

			tr->frames_scheduled = get_next_frame_count(data, tr);

			start_dma(dev, tr->frames_scheduled);

			next_addr = get_next_mem_block(stream_tx, tr);

			if (next_addr) {

				stream_tx->set_dma_pointer(dev, next_addr);
			} else {

				stream_tx->set_dma_pointer(dev, 0);
			}
		} else {

			stop_stream(dev, stream_tx);
			stream_tx->state = I2S_STATE_READY;
		}
	}

int_dma_out_done:

	if (is_int_dma_in(status)) {

		tr_completed = update_transfer(stream_rx->active_transfer);

		if (tr_completed) {

			k_fifo_put(&stream_rx->queue, stream_rx->active_transfer);
			stream_rx->active_transfer = NULL;
		}

		if (!stream_rx->enabled) {
			goto int_dma_in_done;
		}

		if (tr_completed && is_stream_running(stream_rx)) {
			/* Normal RUNNING: promote next_transfer and allocate a new one. */
			stream_rx->active_transfer = stream_rx->next_transfer;
			stream_rx->next_transfer = alloc_rx_transfer(stream_rx);
		} else if (tr_completed && is_stream_draining(stream_rx)) {

			stream_rx->active_transfer = stream_rx->next_transfer;
			stream_rx->next_transfer = NULL;
		}

		if (stream_rx->active_transfer) {
			tr = stream_rx->active_transfer;
			tr->frames_scheduled = get_next_frame_count(data, tr);

			if (!stream_tx->enabled) {
				start_dma(dev, tr->frames_scheduled);
			}

			if (is_stream_running(stream_rx)) {
				/* RUNNING: preload INPTRNXT with the next buffer. */
				next_addr = get_next_mem_block(stream_rx, tr);
				stream_rx->set_dma_pointer(dev, next_addr ? next_addr : 0);
			} else {

				stream_rx->set_dma_pointer(dev, 0);
			}
		} else {
			stop_stream(dev, stream_rx);
			stream_rx->state = I2S_STATE_READY;
		}
	}

int_dma_in_done:

	if (is_int_timeout(status)) {
		error = true;
	}

	if (is_int_bus_err(status)) {
		error = true;
	}

	if (is_int_wclk_err(status)) {
		error = true;
	}

	if (!error) {
		goto end;
	}

	stop_stream(dev, stream_tx);
	stream_tx->state = I2S_STATE_ERROR;
	stop_stream(dev, stream_rx);
	stream_rx->state = I2S_STATE_ERROR;
	full_teardown = true;

end:

	if (!stream_tx->enabled && !stream_rx->enabled) {
		if (full_teardown) {
			/* Fatal error: stop DMA module and clocks immediately. */
			stop_dma(dev);
			disable_clocks(dev);
		}
		/*
		 * Non-fatal stop (PTR_ERR / slab exhaustion): DMA pointers are
		 * already 0 from stop_stream(). Do NOT call I2SStop() here —
		 * that writes DMACFG=0, disabling the entire I2S module and
		 * cutting the master SCK/WS output. SimpleLink mirrors this:
		 * when its DMA queue empties it zeroes the pointers but keeps
		 * DMACFG non-zero so clocks continue until I2S_stopClocks() is
		 * explicitly called. An explicit TRIGGER_DROP or the next
		 * TRIGGER_START will call stop_dma()/disable_clocks() when
		 * needed.
		 */
	}

	I2SClearInt(config->reg_base, status);
}

static DEVICE_API(i2s, ti_cc27xx_i2s_driver_api) = {
	.configure = ti_cc27xx_i2s_configure,
	.config_get = ti_cc27xx_i2s_config_get,
	.read = ti_cc27xx_i2s_read,
	.write = ti_cc27xx_i2s_write,
	.trigger = ti_cc27xx_i2s_trigger,
};

#define I2S_NODE(inst) DT_NODELABEL(i2s##inst)

#define TI_CC27XX_I2S_INIT(inst)                                                                   \
	K_MEM_SLAB_DEFINE_STATIC(i2s_##inst##_tx_mem_slab, TI_CC27XX_I2S_SLAB_CHUNK_SIZE,          \
				 TI_CC27XX_I2S_TX_SLAB_CHUNK_COUNT, TI_CC27XX_I2S_SLAB_ALIGN);     \
	K_MEM_SLAB_DEFINE_STATIC(i2s_##inst##_rx_mem_slab, TI_CC27XX_I2S_SLAB_CHUNK_SIZE,          \
				 TI_CC27XX_I2S_RX_SLAB_CHUNK_COUNT, TI_CC27XX_I2S_SLAB_ALIGN);     \
	PINCTRL_DT_DEFINE(I2S_NODE(inst));                                                         \
	static struct ti_cc27xx_i2s_cfg i2s_##inst##_cfg = {                                       \
		.pin_cfg = PINCTRL_DT_DEV_CONFIG_GET(I2S_NODE(inst)),                              \
		.reg_base = DT_REG_ADDR(I2S_NODE(inst)),                                           \
		.startup_delay = DT_PROP(I2S_NODE(inst), startup_delay),                           \
		.cclk_divider = DT_PROP(I2S_NODE(inst), cclk_divider),                             \
		.module_role = TI_CC27XX_I2S_ROLE_CONTROLLER,                                      \
		.phase_type = TI_CC27XX_I2S_PHASE_TYPE_DUAL,                                       \
		.sampling_edge = TI_CC27XX_I2S_SAMPLING_EDGE_RISING,                               \
		.afclk_src = DT_PROP(I2S_NODE(inst), afclk_source),                                \
		.afosc_freq = DT_PROP(I2S_NODE(inst), afosc_freq),                                 \
		.pin_dir_sd0 = DT_PROP(I2S_NODE(inst), pin_dir_sd0),                               \
		.pin_dir_sd1 = DT_PROP(I2S_NODE(inst), pin_dir_sd1),                               \
	};                                                                                         \
	static struct ti_cc27xx_i2s_data i2s_##inst##_data = {                                     \
		.stream_rx.state = I2S_STATE_NOT_READY,                                            \
		.stream_rx.transfer_mem_slab = &i2s_##inst##_rx_mem_slab,                          \
		.stream_tx.state = I2S_STATE_NOT_READY,                                            \
		.stream_tx.transfer_mem_slab = &i2s_##inst##_tx_mem_slab,                          \
	};                                                                                         \
	static int ti_cc27xx_i2s_init##inst(const struct device *dev)                              \
	{                                                                                          \
		const struct ti_cc27xx_i2s_cfg *const config = dev->config;                        \
		struct ti_cc27xx_i2s_data *data = dev->data;                                       \
		int ret;                                                                           \
		ret = pinctrl_apply_state(config->pin_cfg, PINCTRL_STATE_DEFAULT);                 \
		if (ret < 0) {                                                                     \
			return ret;                                                                \
		}                                                                                  \
		data->stream_rx.set_dma_pointer = set_dma_rx_pointer;                              \
		data->stream_rx.irq_flag = I2S_INT_DMA_IN;                                         \
		data->stream_rx.set_stamp_trigger = set_rx_sample_stamp_trigger;                   \
		data->stream_rx.peek_next_transfer = peek_next_rx_transfer;                        \
		k_fifo_init(&data->stream_rx.queue);                                               \
		data->stream_tx.set_dma_pointer = set_dma_tx_pointer;                              \
		data->stream_tx.irq_flag = I2S_INT_DMA_OUT;                                        \
		data->stream_tx.set_stamp_trigger = set_tx_sample_stamp_trigger;                   \
		data->stream_tx.peek_next_transfer = peek_next_tx_transfer;                        \
		k_fifo_init(&data->stream_tx.queue);                                               \
		I2SClearInt(config->reg_base, I2S_INT_ALL);                                        \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), ti_cc27xx_i2s_isr,    \
			    DEVICE_DT_GET(DT_INST(inst, DT_DRV_COMPAT)), 0);                       \
		irq_enable(DT_INST_IRQN(inst));                                                    \
		return 0;                                                                          \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(inst, &ti_cc27xx_i2s_init##inst, NULL, &i2s_##inst##_data,           \
			      &i2s_##inst##_cfg, POST_KERNEL, CONFIG_I2S_INIT_PRIORITY,            \
			      &ti_cc27xx_i2s_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TI_CC27XX_I2S_INIT)

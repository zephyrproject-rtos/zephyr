/*
 * Copyright (c) 2026 Conclusive Engineering Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc35xx_i2s

#include "i2s_ti_cc35xx.h"

#include <assert.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(ti_cc35xx_i2s, CONFIG_I2S_LOG_LEVEL);

#define TI_CC35XX_I2S_DMA_MIN_FRAMES_PER_TRANSFER 2
#define TI_CC35XX_I2S_DMA_MAX_FRAMES_PER_TRANSFER 256

#define TI_CC35XX_I2S_I2S_CLOCK_DIVIDER_MAX 1024
#define TI_CC35XX_I2S_I2S_CLOCK_DIVIDER_MIN 2

#define TI_CC35XX_I2S_SLAB_CHUNK_SIZE     sizeof(struct ti_cc35xx_i2s_transfer)
#define TI_CC35XX_I2S_TX_SLAB_CHUNK_COUNT CONFIG_I2S_TI_CC35XX_TXQ_SIZE
#define TI_CC35XX_I2S_RX_SLAB_CHUNK_COUNT CONFIG_I2S_TI_CC35XX_RXQ_SIZE
#define TI_CC35XX_I2S_SLAB_ALIGN          4

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
	const struct ti_cc35xx_i2s_cfg *config = dev->config;

	return (is_int_dma_in(status) && is_int_ptr_err(status) &&
		!(bool)I2SGetInPointer(config->reg_base));
}

static inline bool is_int_out_ptr_err(const struct device *dev, uint32_t status)
{
	const struct ti_cc35xx_i2s_cfg *config = dev->config;

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

static inline bool is_next_pointer_null(const struct device *dev)
{
	const struct ti_cc35xx_i2s_cfg *config = dev->config;

	return !(bool)I2SGetOutPointerNext(config->reg_base);
}

static inline struct ti_cc35xx_i2s_transfer *
peek_next_tx_transfer(struct ti_cc35xx_i2s_stream *stream)
{
	return k_fifo_peek_head(&stream->queue);
}

static inline struct ti_cc35xx_i2s_transfer *
peek_next_rx_transfer(struct ti_cc35xx_i2s_stream *stream)
{
	return stream->next_transfer;
}

static inline struct ti_cc35xx_i2s_transfer *alloc_rx_transfer(struct ti_cc35xx_i2s_stream *stream)
{
	struct k_mem_slab *transfer_slab = stream->transfer_mem_slab;
	struct k_mem_slab *audio_slab = stream->audio_mem_slab;
	struct ti_cc35xx_i2s_transfer *transfer;
	size_t buf_size = stream->cfg.bytes_per_audio_block;
	void *buf;

	if (k_mem_slab_alloc(transfer_slab, (void **)&transfer, K_NO_WAIT)) {
		LOG_ERR("Failed to allocate RX metadata buffer");
		return NULL;
	}

	if (k_mem_slab_alloc(audio_slab, &buf, K_NO_WAIT)) {
		LOG_ERR("Failed to allocate RX data buffer");
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

static inline void free_transfer(struct ti_cc35xx_i2s_transfer *transfer)
{
	k_mem_slab_free(transfer->own_mem_slab, transfer);
}

static inline void purge_transfer(struct ti_cc35xx_i2s_transfer *transfer)
{
	k_mem_slab_free(transfer->audio_mem_slab, transfer->mem_block);
	k_mem_slab_free(transfer->own_mem_slab, transfer);
}

static void purge_queue(struct ti_cc35xx_i2s_stream *stream)
{
	struct ti_cc35xx_i2s_transfer *tr;

	if (k_fifo_is_empty(&stream->queue)) {
		return;
	}

	while ((tr = k_fifo_get(&stream->queue, K_NO_WAIT)) != NULL) {
		purge_transfer(tr);
	}
}

static void purge_stream(struct ti_cc35xx_i2s_stream *stream)
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

static uint32_t get_next_frame_count(struct ti_cc35xx_i2s_data *data,
				     struct ti_cc35xx_i2s_transfer *tr)
{
	uint32_t frames_left;

	if (data->is_dma_frame_count_fixed) {
		return data->dma_frame_count;
	}

	frames_left = tr->frames_total - tr->frames_transferred;
	return MIN(frames_left, TI_CC35XX_I2S_DMA_MAX_FRAMES_PER_TRANSFER);
}

static void *get_next_mem_block(struct ti_cc35xx_i2s_stream *stream,
				struct ti_cc35xx_i2s_transfer *tr)
{
	struct ti_cc35xx_i2s_transfer *next_tr;
	uint32_t bytes_per_frame = stream->cfg.bytes_per_frame;
	uint32_t frames_left = tr->frames_total - tr->frames_transferred;
	uint32_t bytes_offset;

	if (frames_left > tr->frames_scheduled) {
		bytes_offset = (tr->frames_transferred + tr->frames_scheduled) * bytes_per_frame;
		return (uint8_t *)tr->mem_block + bytes_offset;
	}

	next_tr = stream->peek_next_transfer(stream);
	return next_tr ? next_tr->mem_block : 0;
}

static inline bool update_transfer(struct ti_cc35xx_i2s_transfer *transfer)
{
	if (transfer == NULL) {
		return false;
	}

	transfer->frames_transferred += transfer->frames_scheduled;

	return (transfer->frames_transferred == transfer->frames_total);
}

static inline bool is_stream_running(struct ti_cc35xx_i2s_stream *stream)
{
	return stream->state == I2S_STATE_RUNNING;
}

static inline bool is_stream_draining(struct ti_cc35xx_i2s_stream *stream)
{
	return (stream->state == I2S_STATE_STOPPING && stream->drain);
}

static inline bool is_stream_stopping(struct ti_cc35xx_i2s_stream *stream)
{
	return (stream->state == I2S_STATE_STOPPING && !stream->drain);
}

static bool is_stream_writeable(struct ti_cc35xx_i2s_stream *stream)
{
	return ((stream->state == I2S_STATE_RUNNING) || (stream->state == I2S_STATE_READY));
}

static bool is_stream_configurable(struct ti_cc35xx_i2s_stream *stream)
{
	return ((stream->state == I2S_STATE_NOT_READY) || (stream->state == I2S_STATE_READY));
}

static int prep_tx_stream(const struct device *dev, struct ti_cc35xx_i2s_stream *stream,
			  void **mem_block_1, void **mem_block_2)
{
	struct ti_cc35xx_i2s_data *data = dev->data;
	struct ti_cc35xx_i2s_transfer *tr = stream->active_transfer;

	if (tr == NULL) {
		tr = k_fifo_get(&stream->queue, K_NO_WAIT);
		if (tr == NULL) {
			return -ENOMEM;
		}
		stream->active_transfer = tr;
	}

	*mem_block_1 = tr->mem_block;

	tr->frames_scheduled = get_next_frame_count(data, tr);
	*mem_block_2 = get_next_mem_block(stream, tr);

	return 0;
}

static int prep_rx_stream(const struct device *dev, struct ti_cc35xx_i2s_stream *stream,
			  void **mem_block_1, void **mem_block_2)
{
	struct ti_cc35xx_i2s_data *data = dev->data;
	struct ti_cc35xx_i2s_transfer *tr;

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

static inline bool is_i2s_config_equal(const struct i2s_config *a, const struct i2s_config *b)
{
	struct i2s_config confa = *a;
	struct i2s_config confb = *b;

	confa.mem_slab = 0;
	confb.mem_slab = 0;
	return (memcmp(&confa, &confb, sizeof(struct i2s_config)) == 0);
}

static inline void enable_common_irqs(const struct device *dev)
{
	const struct ti_cc35xx_i2s_cfg *config = dev->config;

	I2SEnableInt(config->reg_base, (uint32_t)I2S_INT_TIMEOUT | (uint32_t)I2S_INT_BUS_ERR |
					       (uint32_t)I2S_INT_WCLK_ERR |
					       (uint32_t)I2S_INT_PTR_ERR);
}

static inline void enable_stream_irq(const struct device *dev, struct ti_cc35xx_i2s_stream *stream)
{
	const struct ti_cc35xx_i2s_cfg *config = dev->config;

	I2SEnableInt(config->reg_base, stream->irq_flag);
}

static inline void disable_stream_irq(const struct device *dev, struct ti_cc35xx_i2s_stream *stream)
{
	const struct ti_cc35xx_i2s_cfg *config = dev->config;

	I2SDisableInt(config->reg_base, stream->irq_flag);
	I2SClearInt(config->reg_base, stream->irq_flag);
}

static inline void set_dma_tx_pointer(const struct device *dev, void *addr)
{
	const struct ti_cc35xx_i2s_cfg *config = dev->config;

	I2SSetOutPointer(config->reg_base, (uintptr_t)addr);
}

static inline void set_dma_rx_pointer(const struct device *dev, void *addr)
{
	const struct ti_cc35xx_i2s_cfg *config = dev->config;

	I2SSetInPointer(config->reg_base, (uintptr_t)addr);
}

static inline void set_tx_sample_stamp_trigger(const struct device *dev, uint32_t value)
{
	const struct ti_cc35xx_i2s_cfg *config = dev->config;

	I2SConfigureOutSampleStampTrigger(config->reg_base, value);
}

static inline void set_rx_sample_stamp_trigger(const struct device *dev, uint32_t value)
{
	const struct ti_cc35xx_i2s_cfg *config = dev->config;

	I2SConfigureInSampleStampTrigger(config->reg_base, value);
}

static int compute_fixed_frame_count(uint32_t frames_per_audio_block)
{
	int ret = -EINVAL;
	uint32_t max = TI_CC35XX_I2S_DMA_MAX_FRAMES_PER_TRANSFER;
	uint32_t min = TI_CC35XX_I2S_DMA_MIN_FRAMES_PER_TRANSFER;

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

static void config_serial_format(const struct device *dev,
				 struct ti_cc35xx_i2s_stream_cfg *stream_cfg,
				 uint32_t dma_frame_count)
{
	const struct ti_cc35xx_i2s_cfg *config = dev->config;
	uint8_t data_delay;
	uint32_t memory_length;
	uint32_t sampling_edge;
	uint8_t bits_per_sample;
	bool is_dual_phase;

	if (stream_cfg->bits_per_memory_word == TI_CC35XX_I2S_WORD_SIZE_16) {
		memory_length = I2S_MEM_LENGTH_16;
	} else {
		memory_length = I2S_MEM_LENGTH_32;
	}

	data_delay = stream_cfg->before_word_padding + stream_cfg->data_shift;

	if (config->sampling_edge == TI_CC35XX_I2S_SAMPLING_EDGE_RISING) {
		sampling_edge = I2S_POS_EDGE;
	} else {
		sampling_edge = I2S_NEG_EDGE;
	}

	is_dual_phase = (config->phase_type == TI_CC35XX_I2S_PHASE_TYPE_DUAL);

	if (config->phase_type == TI_CC35XX_I2S_PHASE_TYPE_DUAL) {
		bits_per_sample = stream_cfg->bits_per_audio_word + stream_cfg->after_word_padding;
	} else {
		bits_per_sample = stream_cfg->bits_per_audio_word;
	}

	I2SConfigureFormat(config->reg_base, data_delay, memory_length, sampling_edge,
			   is_dual_phase, bits_per_sample);

	I2SConfigureWclkCounterPeriod(config->reg_base, dma_frame_count);

	/*
	 * To avoid false start-up triggers, the input and output triggers must
	 * initially be equal to or higher than the WCLK counter period.
	 */
	I2SConfigureInSampleStampTrigger(config->reg_base, I2S_STMP_SATURATION);
	I2SConfigureOutSampleStampTrigger(config->reg_base, I2S_STMP_SATURATION);
}

static uint32_t get_bit_rate(const struct device *dev, const struct ti_cc35xx_i2s_stream *stream)
{
	uint32_t data_length;
	uint32_t sample_length;

	sample_length = stream->cfg.before_word_padding;
	sample_length += stream->cfg.bits_per_audio_word;
	sample_length += stream->cfg.after_word_padding;

	data_length = stream->cfg.channel_count * sample_length;
	return data_length * stream->cfg.frame_clk_freq;
}

static bool compute_sck_divider(const struct device *dev, const struct ti_cc35xx_i2s_stream *stream,
				uint32_t *divider)
{
	const struct ti_cc35xx_i2s_cfg *config = dev->config;
	uint32_t out;
	uint32_t bit_rate = get_bit_rate(dev, stream);

	if (bit_rate == 0) {
		return false;
	}

	out = DIV_ROUND_CLOSEST(config->adfs_audio_clock_freq, bit_rate);

	if (IN_RANGE(out, TI_CC35XX_I2S_I2S_CLOCK_DIVIDER_MIN,
		     TI_CC35XX_I2S_I2S_CLOCK_DIVIDER_MAX)) {
		*divider = out;
		return true;
	}

	return false;
}

static bool compute_ws_divider(const struct device *dev, const struct ti_cc35xx_i2s_stream *stream,
			       uint32_t *divider)
{
	const struct ti_cc35xx_i2s_cfg *config = dev->config;
	uint32_t channel_count = stream->cfg.channel_count;
	uint16_t sample_length = 0;

	switch (config->phase_type) {
	case TI_CC35XX_I2S_PHASE_TYPE_DUAL:
		/*
		 * In dual-phase format, each phase represents a channel and is
		 * divided into the three intervals:
		 *
		 * Data delay (optional): BCLK periods between the first
		 * WCLK edge and MSB of the audio channel data
		 * transferred during the phase.
		 *
		 * Word: BCLK periods during which one sample word (a single
		 * channel) is transferred.
		 *
		 * Idle (optional): BCLK periods between the word interval
		 * and the next phase.
		 */
		sample_length += stream->cfg.before_word_padding;
		sample_length += stream->cfg.bits_per_audio_word;
		sample_length += stream->cfg.after_word_padding;
		/*
		 * WS is high for WDIV[9:0] (1 to 1023) SCK periods and low for
		 * WDIV[9:0] (1 to 1023) SCK periods.
		 * WS frequency = SCK frequency / (2 x WDIV[9:0])
		 * Dual phase protocols don't accept more than two channels.
		 */
		if (channel_count > TI_CC35XX_I2S_DUAL_PHASE_CHANNEL_MAX) {
			return false;
		}
		break;
	case TI_CC35XX_I2S_PHASE_TYPE_SINGLE:
		/*
		 * In single phase format, from 1 to 8 sample words (channels)
		 * are transferred back-to-back using a single phase.
		 * The phase is divided into the three intervals:
		 *
		 * Data delay (optional) : BCLK periods between the first
		 * WCLK edge and MSB of the FIRST audio channel data transferred.
		 *
		 * Word: BCLK periods during which from 1 to 8 channels are
		 * transferred back-to-back.
		 *
		 * Idle (optional): BCLK periods between the word interval
		 * and the next phase.
		 */
		sample_length += stream->cfg.before_word_padding;
		sample_length += (stream->cfg.bits_per_audio_word * channel_count);
		sample_length += stream->cfg.after_word_padding;

		/*
		 * WS is high for 1 SCK period and low for WDIV[9:0]
		 * (1 to 1023) SCK periods.
		 * WS frequency = SCK frequency / (1 + I2S:I2SWCLKDIV.WDIV[9:0])
		 * Single phase protocols don't accept more than 8 channels
		 */
		if (channel_count > TI_CC35XX_I2S_SINGLE_PHASE_CHANNEL_MAX) {
			return false;
		}
		break;
	default:
		return false;
	}

	*divider = sample_length;
	return true;
}

static inline enum TI_CC35XX_I2S_CHANNEL_BITMASK get_pin_bitmask(const struct device *dev,
								 enum TI_CC35XX_I2S_PIN_DIR pin_dir)
{
	struct ti_cc35xx_i2s_data *data = dev->data;

	switch (pin_dir) {
	case TI_CC35XX_I2S_PIN_DIR_IN:
		return data->stream_rx.cfg.channel_bitmask;
	case TI_CC35XX_I2S_PIN_DIR_OUT:
		return data->stream_tx.cfg.channel_bitmask;
	default:
		return TI_CC35XX_I2S_CHANNEL_BITMASK_NONE;
	}
}

static void config_channels(const struct device *dev)
{

	const struct ti_cc35xx_i2s_cfg *config = dev->config;
	enum TI_CC35XX_I2S_CHANNEL_BITMASK sd0_bitmask;
	enum TI_CC35XX_I2S_CHANNEL_BITMASK sd1_bitmask;

	sd0_bitmask = get_pin_bitmask(dev, config->pin_dir_sd0);
	sd1_bitmask = get_pin_bitmask(dev, config->pin_dir_sd1);

	I2SConfigureFrame(config->reg_base, (uint8_t)config->pin_dir_sd0, sd0_bitmask,
			  (uint8_t)(config->pin_dir_sd1 << 4), sd1_bitmask);
}

static void start_stream(const struct device *dev, struct ti_cc35xx_i2s_stream *stream,
			 uint32_t trig, void *mem_block)
{
	enable_stream_irq(dev, stream);
	stream->set_dma_pointer(dev, mem_block);
	stream->set_stamp_trigger(dev, trig);
	stream->enabled = true;
	stream->state = I2S_STATE_RUNNING;
}

static inline void stop_stream(const struct device *dev, struct ti_cc35xx_i2s_stream *stream)
{
	disable_stream_irq(dev, stream);
	stream->set_dma_pointer(dev, 0);
	stream->set_stamp_trigger(dev, I2S_STMP_SATURATION);
	stream->enabled = false;
	stream->drain = false;
}

static inline void start_dma(const struct device *dev, uint32_t frame_count)
{
	const struct ti_cc35xx_i2s_cfg *config = dev->config;
	struct ti_cc35xx_i2s_data *data = dev->data;

	if (!data->is_dma_frame_count_fixed) {
		data->dma_frame_count = frame_count;
	}

	I2SStart(config->reg_base, data->dma_frame_count);
}

static void stop_dma(const struct device *dev)
{
	const struct ti_cc35xx_i2s_cfg *config = dev->config;
	struct ti_cc35xx_i2s_data *data = dev->data;

	data->dma_frame_count = 0;
	data->is_dma_frame_count_fixed = false;
	I2SStop(config->reg_base);
}

static inline void enable_core_clock(const struct device *dev)
{
	const struct ti_cc35xx_i2s_cfg *config = dev->config;

	I2SEnableClk(config->reg_base);
}

static void config_clocks(const struct device *dev)
{
	const struct ti_cc35xx_i2s_cfg *config = dev->config;
	struct ti_cc35xx_i2s_data *data = dev->data;

	I2SConfigureClocks(config->reg_base, config->module_role, data->invert_ws,
			   config->phase_type == TI_CC35XX_I2S_PHASE_TYPE_DUAL,
			   config->cclk_divider, data->ws_divider, data->sck_divider);
}

static void enable_clocks(const struct device *dev)
{

	const struct ti_cc35xx_i2s_cfg *config = dev->config;
	struct ti_cc35xx_i2s_adfs adfs;

	I2SEnableSampleStamp(config->reg_base);
	I2SResetWclkCounter(config->reg_base);

	if (config->module_role == TI_CC35XX_I2S_ROLE_CONTROLLER) {
		adfs = config->adfs;

		/*
		 * To select the ADFS input and to configure the ADFS
		 * it is necessary to disable the I2S module clock.
		 */
		I2SDisableClk(config->reg_base);

		/*
		 * Select the source clock source and configure the ADFS.
		 * The selected clock source is used by the ADFS
		 * to generate the audio clock, which is used to generate
		 * the I2S clocks.
		 */
		I2SSelectAdfsInputClk(config->reg_base, config->core_clk_src);
		I2SConfigureAdfs(config->reg_base, adfs.tref, adfs.delta, adfs.delta_sign,
				 adfs.div);

		/* Enable the ADFS module and re-enable the I2S module clock */
		I2SEnableAdfs(config->reg_base);
		I2SEnableClk(config->reg_base);

		/* Enable controller generated clocks */
		I2SEnableControllerClocks(config->reg_base);
	}
}

static void disable_clocks(const struct device *dev)
{
	const struct ti_cc35xx_i2s_cfg *config = dev->config;

	I2SDisableClk(config->reg_base);
	I2SDisableAdfs(config->reg_base);
	I2SDisableSampleStamp(config->reg_base);
	I2SDisableControllerClocks(config->reg_base);
}

static void init_hw(const struct device *dev, struct ti_cc35xx_i2s_stream_cfg *stream_cfg,
		    uint32_t dma_frame_count)
{
	config_serial_format(dev, stream_cfg, dma_frame_count);
	config_channels(dev);
	config_clocks(dev);
}

static void start_clocks(const struct device *dev, struct ti_cc35xx_i2s_stream_cfg *stream_cfg,
			 uint32_t dma_frame_count)
{
	init_hw(dev, stream_cfg, dma_frame_count);
	enable_clocks(dev);
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

/*
 * Number of frames must be >= 2.
 * This is a limitation of driverlib API (I2SStart function).
 * This rule applies also to transfers > 256 frames,
 * in such case the remainder part must be >= 2.
 */
static int is_frame_count_valid(uint32_t frame_count)
{
	uint32_t remainder;

	if (frame_count == 0) {
		return false;
	}

	remainder = frame_count % TI_CC35XX_I2S_DMA_MAX_FRAMES_PER_TRANSFER;

	if ((remainder == 0) || (remainder >= TI_CC35XX_I2S_DMA_MIN_FRAMES_PER_TRANSFER)) {
		return true;
	}

	return false;
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

	/*
	 * Frame clock frequency = 0 disables the interface.
	 * Other parameters are ignored.
	 */
	if (cfg->frame_clk_freq == 0) {
		return 0;
	}

	if (cfg->mem_slab == NULL) {
		LOG_ERR("Memory slab required");
		return -EINVAL;
	}

	if (cfg->block_size == 0) {
		LOG_ERR("Invalid block size: %d", cfg->block_size);
		return -EINVAL;
	}

	switch (cfg->word_size) {
	case TI_CC35XX_I2S_WORD_SIZE_8:
	case TI_CC35XX_I2S_WORD_SIZE_16:
	case TI_CC35XX_I2S_WORD_SIZE_24:
		break;
	case TI_CC35XX_I2S_WORD_SIZE_32:
	default:
		LOG_ERR("Unsupported word size: %u", cfg->word_size);
		return -EINVAL;
	}

	switch (cfg->format & I2S_FMT_DATA_FORMAT_MASK) {
	case I2S_FMT_DATA_FORMAT_I2S:
	case I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED:
	case I2S_FMT_DATA_FORMAT_RIGHT_JUSTIFIED:
		break;
	default:
		LOG_ERR("Unsupported data format: 0x%02x", cfg->format);
		return -EINVAL;
	}

	if ((cfg->format & I2S_FMT_DATA_ORDER_LSB) || (cfg->format & I2S_FMT_BIT_CLK_INV) ||
	    (cfg->format & I2S_FMT_FRAME_CLK_INV)) {
		LOG_ERR("Unsupported stream format: 0x%02x", cfg->format);
		return -EINVAL;
	}

	if ((cfg->options & I2S_OPT_BIT_CLK_TARGET) || (cfg->options & I2S_OPT_FRAME_CLK_TARGET)) {
		LOG_ERR("Unsupported operation mode: 0x%02x", cfg->options);
		return -EINVAL;
	}

	if ((cfg->options & I2S_OPT_LOOPBACK) || (cfg->options & I2S_OPT_PINGPONG)) {
		LOG_ERR("Unsupported options: 0x%02x", cfg->options);
		return -EINVAL;
	}

	switch (cfg->channels) {
	case TI_CC35XX_I2S_CHANNEL_COUNT_1:
	case TI_CC35XX_I2S_CHANNEL_COUNT_2:
		break;
	default:
		LOG_ERR("Unsupported channel count: %u", cfg->channels);
		return -EINVAL;
	}

	return 0;
}

static int ti_cc35xx_i2s_configure(const struct device *dev, enum i2s_dir dir,
				   const struct i2s_config *i2s_cfg)
{
	struct ti_cc35xx_i2s_data *data = dev->data;
	struct ti_cc35xx_i2s_stream *stream;
	struct ti_cc35xx_i2s_stream_cfg *stream_cfg;
	int ret;

	ret = validate_args_configure(dir, i2s_cfg);
	if (ret < 0) {
		return ret;
	}

	if (dir == I2S_DIR_RX) {
		stream = &data->stream_rx;
		stream_cfg = &stream->cfg;
	} else if (dir == I2S_DIR_TX) {
		stream = &data->stream_tx;
		stream_cfg = &stream->cfg;
	}

	if (!is_stream_configurable(stream)) {
		return -EINVAL;
	}

	if (i2s_cfg->frame_clk_freq == 0) {
		memset(stream_cfg, 0, sizeof(*stream_cfg));
		stream->enabled = false;
		stream->drain = false;
		stream->configured = false;
		stream->state = I2S_STATE_NOT_READY;
		return 0;
	}

	stream_cfg->frame_clk_freq = i2s_cfg->frame_clk_freq;
	stream->audio_mem_slab = i2s_cfg->mem_slab;
	stream->cfg.bytes_per_audio_block = i2s_cfg->block_size;

	switch (i2s_cfg->word_size) {
	case TI_CC35XX_I2S_WORD_SIZE_8:
		stream_cfg->bits_per_memory_word = TI_CC35XX_I2S_WORD_SIZE_16;
		stream_cfg->bits_per_audio_word = i2s_cfg->word_size;
		break;
	case TI_CC35XX_I2S_WORD_SIZE_16:
		stream_cfg->bits_per_memory_word = i2s_cfg->word_size;
		stream_cfg->bits_per_audio_word = i2s_cfg->word_size;
		break;
	case TI_CC35XX_I2S_WORD_SIZE_24:
		stream_cfg->bits_per_memory_word = TI_CC35XX_I2S_WORD_SIZE_32;
		stream_cfg->bits_per_audio_word = i2s_cfg->word_size;
		break;
	}

	switch (i2s_cfg->format & I2S_FMT_DATA_FORMAT_MASK) {
	case I2S_FMT_DATA_FORMAT_I2S:
		data->invert_ws = true;
		stream_cfg->data_shift = 1;
		stream_cfg->before_word_padding = 0;
		stream_cfg->after_word_padding = 0;
		break;
	case I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED:
		data->invert_ws = false;
		stream_cfg->before_word_padding = 0;
		stream_cfg->after_word_padding = 0;
		break;
	case I2S_FMT_DATA_FORMAT_RIGHT_JUSTIFIED:
		data->invert_ws = false;
		stream_cfg->before_word_padding = 0;
		stream_cfg->after_word_padding = 0;
		break;
	}

	switch (i2s_cfg->channels) {
	case TI_CC35XX_I2S_CHANNEL_COUNT_1:
		stream_cfg->channel_bitmask = TI_CC35XX_I2S_CHANNEL_BITMASK_1;
		break;
	case TI_CC35XX_I2S_CHANNEL_COUNT_2:
		stream_cfg->channel_bitmask = TI_CC35XX_I2S_CHANNEL_BITMASK_2;
		break;
	}

	stream_cfg->timeout = translate_timeout(i2s_cfg->timeout);

	stream_cfg->channel_count = i2s_cfg->channels;
	stream_cfg->bytes_per_frame =
		stream_cfg->bits_per_memory_word / 8 * stream_cfg->channel_count;

	stream->cfg.frames_per_audio_block = i2s_cfg->block_size / stream_cfg->bytes_per_frame;

	if (!is_frame_count_valid(stream_cfg->frames_per_audio_block)) {
		LOG_ERR("Invalid block size: %d", i2s_cfg->block_size);
		return -EINVAL;
	}

	if (!compute_sck_divider(dev, stream, &data->sck_divider)) {
		LOG_ERR("Unable to calculate SCLK clock divider");
		return -EINVAL;
	}

	if (!compute_ws_divider(dev, stream, &data->ws_divider)) {
		LOG_ERR("Unable to calculate WCLK clock divider");
		return -EINVAL;
	}

	/*
	 * This config copy is used for comparison between RX and TX streams
	 * when I2S_DIR_BOTH is selected with the START trigger.
	 */
	stream->i2s_config_copy = *i2s_cfg;
	stream->state = I2S_STATE_READY;
	stream->configured = true;
	return 0;
}

static const struct i2s_config *ti_cc35xx_i2s_config_get(const struct device *dev, enum i2s_dir dir)
{
	struct ti_cc35xx_i2s_data *data = dev->data;
	struct ti_cc35xx_i2s_stream *stream;

	if (dir == I2S_DIR_BOTH) {
		return NULL;
	}

	stream = (dir == I2S_DIR_TX) ? &data->stream_tx : &data->stream_rx;
	return stream->configured ? &stream->i2s_config_copy : NULL;
}

static int ti_cc35xx_i2s_read(const struct device *dev, void **mem_block, size_t *size)
{
	struct ti_cc35xx_i2s_data *data = dev->data;
	struct ti_cc35xx_i2s_stream *stream = &data->stream_rx;
	struct ti_cc35xx_i2s_transfer *transfer;
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
	struct ti_cc35xx_i2s_data *data = dev->data;
	struct ti_cc35xx_i2s_stream *stream = &data->stream_tx;
	uint32_t frame_count = size / stream->cfg.bytes_per_frame;

	if (size > stream->cfg.bytes_per_audio_block) {
		LOG_ERR("Invalid block size: %d", size);
		return -EINVAL;
	}

	if (!is_frame_count_valid(frame_count)) {
		LOG_ERR("Invalid block size: %d", size);
		return -EINVAL;
	}

	/*
	 * For I2S_DIR_BOTH the TX data size must be multiple
	 * of the audio data block size to assure
	 * sync with RX stream.
	 */
	if ((data->dir == I2S_DIR_BOTH) && (size % stream->cfg.bytes_per_audio_block)) {
		LOG_ERR("Invalid block size: %d", size);
		return -EINVAL;
	}

	return 0;
}

static int ti_cc35xx_i2s_write(const struct device *dev, void *mem_block, size_t size)
{
	struct ti_cc35xx_i2s_data *data = dev->data;
	struct ti_cc35xx_i2s_stream *stream = &data->stream_tx;
	struct k_mem_slab *transfer_slab = stream->transfer_mem_slab;
	struct k_mem_slab *audio_slab = stream->audio_mem_slab;
	k_timeout_t timeout = stream->cfg.timeout;
	struct ti_cc35xx_i2s_transfer *transfer;
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

	/*
	 * Irq lock isn't used here as it's not required.
	 * If the DMA "next pointer" register is empty at this point
	 * there's no path in the code which could change that
	 * - including the ISR, so there's no potential
	 * for race conditions.
	 */
	if (is_next_pointer_null(dev) && stream->state == I2S_STATE_RUNNING) {
		stream->set_dma_pointer(dev, transfer->mem_block);
	}

	return 0;
}

static int validate_args_trigger(enum i2s_dir dir, enum i2s_trigger_cmd cmd)
{
	switch (dir) {
	case I2S_DIR_BOTH:
	case I2S_DIR_RX:
	case I2S_DIR_TX:
		break;
	default:
		LOG_ERR("Invalid I2S direction: %d", dir);
		return -EINVAL;
	}

	switch (cmd) {
	case I2S_TRIGGER_START:
	case I2S_TRIGGER_STOP:
	case I2S_TRIGGER_DRAIN:
	case I2S_TRIGGER_DROP:
	case I2S_TRIGGER_PREPARE:
		break;
	default:
		LOG_ERR("Invalid I2S trigger: %d", cmd);
		return -EINVAL;
	}

	return 0;
}

static int validate_args_trigger_start(enum i2s_dir dir, struct ti_cc35xx_i2s_stream *stream_tx,
				       struct ti_cc35xx_i2s_stream *stream_rx)
{
	if (dir == I2S_DIR_BOTH) {
		struct i2s_config *tx_cfg = &stream_tx->i2s_config_copy;
		struct i2s_config *rx_cfg = &stream_rx->i2s_config_copy;

		if (!is_i2s_config_equal(rx_cfg, tx_cfg)) {
			LOG_ERR("TX and RX configurations are different");
			return -EIO;
		}
	}

	if (dir == I2S_DIR_TX && stream_rx->enabled) {
		return -EIO;
	}

	if (dir == I2S_DIR_RX && stream_tx->enabled) {
		return -EIO;
	}

	if (dir == I2S_DIR_RX || dir == I2S_DIR_BOTH) {
		if (stream_rx->state != I2S_STATE_READY) {
			return -EIO;
		}
	}

	if (dir == I2S_DIR_TX || dir == I2S_DIR_BOTH) {
		if (stream_tx->state != I2S_STATE_READY) {
			return -EIO;
		}
	}

	return 0;
}

static int handle_trigger_start(const struct device *dev, enum i2s_dir dir,
				struct ti_cc35xx_i2s_stream *stream_tx,
				struct ti_cc35xx_i2s_stream *stream_rx)
{
	const struct ti_cc35xx_i2s_cfg *config = dev->config;
	struct ti_cc35xx_i2s_data *data = dev->data;
	struct ti_cc35xx_i2s_stream *stream;
	struct ti_cc35xx_i2s_stream_cfg *stream_cfg;
	bool start_rx = false;
	bool start_tx = false;
	uint32_t dma_frame_count;
	void *tx_mem_addr_1 = NULL;
	void *tx_mem_addr_2 = NULL;
	void *rx_mem_addr_1 = NULL;
	void *rx_mem_addr_2 = NULL;
	uint32_t trig;
	int ret;

	if (dir == I2S_DIR_RX || dir == I2S_DIR_BOTH) {
		start_rx = true;
		stream = stream_rx;
		stream_cfg = &stream_rx->cfg;
	}

	if (dir == I2S_DIR_TX || dir == I2S_DIR_BOTH) {
		start_tx = true;
		stream = stream_tx;
		stream_cfg = &stream_tx->cfg;
	}

	/*
	 * For I2S_DIR_BOTH the dma frame count must be constant
	 * for the RX and TX streams to stay in sync.
	 */
	if (dir == I2S_DIR_BOTH) {
		ret = compute_fixed_frame_count(stream_cfg->frames_per_audio_block);
		if (ret < 0) {
			LOG_ERR("Invalid block size");
			return ret;
		}
		data->dma_frame_count = ret;
		data->is_dma_frame_count_fixed = true;
	} else {
		data->is_dma_frame_count_fixed = false;
	}

	if (start_tx) {
		ret = prep_tx_stream(dev, stream_tx, &tx_mem_addr_1, &tx_mem_addr_2);
		if (ret < 0) {
			LOG_ERR("Failed to prepare TX stream: %d", ret);
			return ret;
		}
		dma_frame_count = stream->active_transfer->frames_scheduled;
	}

	if (start_rx) {
		ret = prep_rx_stream(dev, stream_rx, &rx_mem_addr_1, &rx_mem_addr_2);
		if (ret < 0) {
			LOG_ERR("Failed to prepare RX stream: %d", ret);
			return ret;
		}
		dma_frame_count = stream->active_transfer->frames_scheduled;
	}

	start_clocks(dev, stream_cfg, dma_frame_count);

	trig = HWREGH(config->reg_base + I2S_O_STMPWCNT);
	trig += config->startup_delay;

	I2SClearInt(config->reg_base, I2S_INT_ALL);
	enable_common_irqs(dev);

	if (start_tx) {
		start_stream(dev, stream_tx, trig, tx_mem_addr_1);
	}

	if (start_rx) {
		start_stream(dev, stream_rx, trig, rx_mem_addr_1);
	}

	/* Start the DMA to load the Input/Output data pointers */
	start_dma(dev, dma_frame_count);

	/* Now we can load address of the next block, if exists */
	if (tx_mem_addr_2) {
		stream_tx->set_dma_pointer(dev, tx_mem_addr_2);
	}

	if (rx_mem_addr_2) {
		stream_rx->set_dma_pointer(dev, rx_mem_addr_2);
	}

	return 0;
}

static int validate_args_trigger_drain(enum i2s_dir dir, struct ti_cc35xx_i2s_stream *stream_tx,
				       struct ti_cc35xx_i2s_stream *stream_rx)
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
				struct ti_cc35xx_i2s_stream *stream_tx,
				struct ti_cc35xx_i2s_stream *stream_rx)
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

static int validate_args_trigger_stop(enum i2s_dir dir, struct ti_cc35xx_i2s_stream *stream_tx,
				      struct ti_cc35xx_i2s_stream *stream_rx)
{
	if (dir == I2S_DIR_RX || dir == I2S_DIR_BOTH) {
		if (stream_rx->state != I2S_STATE_RUNNING) {
			LOG_ERR("RX stream not ready");
			return -EIO;
		}
	}

	if (dir == I2S_DIR_TX || dir == I2S_DIR_BOTH) {
		if (stream_tx->state != I2S_STATE_RUNNING) {
			LOG_ERR("TX stream not ready");
			return -EIO;
		}
	}

	return 0;
}

static void handle_trigger_stop(const struct device *dev, enum i2s_dir dir,
				struct ti_cc35xx_i2s_stream *stream_tx,
				struct ti_cc35xx_i2s_stream *stream_rx)
{
	if (dir == I2S_DIR_RX || dir == I2S_DIR_BOTH) {
		stream_rx->state = I2S_STATE_STOPPING;
	}

	if (dir == I2S_DIR_TX || dir == I2S_DIR_BOTH) {
		stream_tx->state = I2S_STATE_STOPPING;
	}
}

static int validate_args_trigger_drop(enum i2s_dir dir, struct ti_cc35xx_i2s_stream *stream_tx,
				      struct ti_cc35xx_i2s_stream *stream_rx)
{
	if (dir == I2S_DIR_RX || dir == I2S_DIR_BOTH) {
		if (stream_rx->state == I2S_STATE_NOT_READY) {
			LOG_ERR("RX stream not ready");
			return -EIO;
		}
	}

	if (dir == I2S_DIR_TX || dir == I2S_DIR_BOTH) {
		if (stream_tx->state == I2S_STATE_NOT_READY) {
			LOG_ERR("TX stream not ready");
			return -EIO;
		}
	}

	return 0;
}

static int handle_trigger_drop(const struct device *dev, enum i2s_dir dir,
			       struct ti_cc35xx_i2s_stream *stream_tx,
			       struct ti_cc35xx_i2s_stream *stream_rx)
{
	if (dir == I2S_DIR_RX || dir == I2S_DIR_BOTH) {
		stop_stream(dev, stream_rx);
		purge_stream(stream_rx);
		stream_rx->state = I2S_STATE_READY;
	}

	if (dir == I2S_DIR_TX || dir == I2S_DIR_BOTH) {
		stop_stream(dev, stream_tx);
		purge_stream(stream_tx);
		stream_tx->state = I2S_STATE_READY;
	}

	if (!stream_tx->enabled && !stream_rx->enabled) {
		stop_dma(dev);
	}

	return 0;
}

static int validate_args_trigger_prepare(enum i2s_dir dir, struct ti_cc35xx_i2s_stream *stream_tx,
					 struct ti_cc35xx_i2s_stream *stream_rx)
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
				  struct ti_cc35xx_i2s_stream *stream_tx,
				  struct ti_cc35xx_i2s_stream *stream_rx)
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

static int ti_cc35xx_i2s_trigger(const struct device *dev, enum i2s_dir dir,
				 enum i2s_trigger_cmd cmd)
{
	struct ti_cc35xx_i2s_data *data = dev->data;
	struct ti_cc35xx_i2s_stream *stream_rx = &data->stream_rx;
	struct ti_cc35xx_i2s_stream *stream_tx = &data->stream_tx;
	int ret;

	ret = validate_args_trigger(dir, cmd);
	if (ret < 0) {
		return ret;
	}

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
	default:
		LOG_ERR("Unknown trigger value: %d", cmd);
		return -EINVAL;
	}
}

static void ti_cc35xx_i2s_isr(const struct device *dev)
{
	const struct ti_cc35xx_i2s_cfg *config = dev->config;
	struct ti_cc35xx_i2s_data *data = dev->data;
	struct ti_cc35xx_i2s_stream *stream_tx = &data->stream_tx;
	struct ti_cc35xx_i2s_stream *stream_rx = &data->stream_rx;
	struct ti_cc35xx_i2s_transfer *tr;
	void *next_addr;
	bool tr_completed;
	uint32_t status = I2SIntStatus(config->reg_base, false);
	bool error = false;

	if (is_int_out_ptr_err(dev, status) && stream_tx->enabled) {
		stop_stream(dev, stream_tx);
		if (is_stream_draining(stream_tx) || is_stream_stopping(stream_tx)) {
			stream_tx->state = I2S_STATE_READY;
		} else {
			stream_tx->state = I2S_STATE_ERROR;
		}
	}

	if (is_int_in_ptr_err(dev, status) && stream_rx->enabled) {
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
			stream_tx->active_transfer = k_fifo_get(&stream_tx->queue, K_NO_WAIT);
		}
		if (stream_tx->active_transfer) {
			tr = stream_tx->active_transfer;
			tr->frames_scheduled = get_next_frame_count(data, tr);
			start_dma(dev, tr->frames_scheduled);
			next_addr = get_next_mem_block(stream_tx, tr);
			if (next_addr) {
				stream_tx->set_dma_pointer(dev, next_addr);
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
			stream_rx->active_transfer = stream_rx->next_transfer;
			stream_rx->next_transfer = alloc_rx_transfer(stream_rx);
		}
		if (stream_rx->active_transfer) {
			tr = stream_rx->active_transfer;
			tr->frames_scheduled = get_next_frame_count(data, tr);
			/*
			 * RX stream drives the DMA only if TX is disabled.
			 * If both directions are enabled the TX stream
			 * handles the DMA burst size.
			 */
			if (!stream_tx->enabled) {
				start_dma(dev, tr->frames_scheduled);
			}

			next_addr = get_next_mem_block(stream_rx, tr);
			if (next_addr) {
				stream_rx->set_dma_pointer(dev, next_addr);
			}
		} else {
			stop_stream(dev, stream_rx);
			stream_rx->state = I2S_STATE_READY;
		}
	}

int_dma_in_done:
	if (is_int_timeout(status)) {
		LOG_ERR("Word clock timeout!");
		error = true;
	}

	if (is_int_bus_err(status)) {
		LOG_ERR("DMA bus error!");
		error = true;
	}

	if (is_int_wclk_err(status)) {
		LOG_ERR("Word clock error!");
		error = true;
	}

	if (!error) {
		goto end;
	}

	stop_stream(dev, stream_tx);
	stream_tx->state = I2S_STATE_ERROR;
	stop_stream(dev, stream_rx);
	stream_rx->state = I2S_STATE_ERROR;

end:
	if (!stream_tx->enabled && !stream_rx->enabled) {
		stop_dma(dev);
	}

	I2SClearInt(config->reg_base, status);
}

static DEVICE_API(i2s, i2s_api) = {
	.configure = ti_cc35xx_i2s_configure,
	.config_get = ti_cc35xx_i2s_config_get,
	.read = ti_cc35xx_i2s_read,
	.write = ti_cc35xx_i2s_write,
	.trigger = ti_cc35xx_i2s_trigger,
};

#define I2S_NODE(inst) DT_NODELABEL(i2s##inst)

#define TI_CC35XX_I2S_INIT(inst)                                                                   \
	K_MEM_SLAB_DEFINE_STATIC(i2s_##inst##_tx_mem_slab, TI_CC35XX_I2S_SLAB_CHUNK_SIZE,          \
				 TI_CC35XX_I2S_TX_SLAB_CHUNK_COUNT, TI_CC35XX_I2S_SLAB_ALIGN);     \
	K_MEM_SLAB_DEFINE_STATIC(i2s_##inst##_rx_mem_slab, TI_CC35XX_I2S_SLAB_CHUNK_SIZE,          \
				 TI_CC35XX_I2S_RX_SLAB_CHUNK_COUNT, TI_CC35XX_I2S_SLAB_ALIGN);     \
	PINCTRL_DT_DEFINE(I2S_NODE(inst));                                                         \
	static struct ti_cc35xx_i2s_cfg i2s_##inst##_cfg = {                                       \
		.pin_cfg = PINCTRL_DT_DEV_CONFIG_GET(I2S_NODE(inst)),                              \
		.reg_base = DT_REG_ADDR(I2S_NODE(inst)),                                           \
		.startup_delay = DT_PROP(I2S_NODE(inst), startup_delay),                           \
		.cclk_divider = DT_PROP(I2S_NODE(inst), cclk_divider),                             \
		.module_role = TI_CC35XX_I2S_ROLE_CONTROLLER,                                      \
		.phase_type = TI_CC35XX_I2S_PHASE_TYPE_DUAL,                                       \
		.sampling_edge = TI_CC35XX_I2S_SAMPLING_EDGE_RISING,                               \
		.core_clk_src = DT_PROP(I2S_NODE(inst), clock_source),                             \
		.adfs =                                                                            \
			{                                                                          \
				.tref = DT_PROP(I2S_NODE(inst), adfs_tref),                        \
				.div = DT_PROP(I2S_NODE(inst), adfs_div),                          \
				.delta = DT_PROP(I2S_NODE(inst), adfs_delta),                      \
				.delta_sign = DT_PROP(I2S_NODE(inst), adfs_delta_sign),            \
			},                                                                         \
		.adfs_audio_clock_freq = DT_PROP(I2S_NODE(inst), adfs_audio_clock_freq),           \
		.pin_dir_sd0 = DT_PROP(I2S_NODE(inst), pin_dir_sd0),                               \
		.pin_dir_sd1 = DT_PROP(I2S_NODE(inst), pin_dir_sd1),                               \
	};                                                                                         \
	static struct ti_cc35xx_i2s_data i2s_##inst##_data = {                                     \
		.stream_rx.state = I2S_STATE_NOT_READY,                                            \
		.stream_rx.transfer_mem_slab = &i2s_##inst##_rx_mem_slab,                          \
		.stream_tx.state = I2S_STATE_NOT_READY,                                            \
		.stream_tx.transfer_mem_slab = &i2s_##inst##_tx_mem_slab,                          \
	};                                                                                         \
	static int ti_cc35xx_i2s_init##inst(const struct device *dev)                              \
	{                                                                                          \
		const struct ti_cc35xx_i2s_cfg *const config = dev->config;                        \
		struct ti_cc35xx_i2s_data *data = dev->data;                                       \
		uint32_t ret;                                                                      \
		ret = pinctrl_apply_state(config->pin_cfg, PINCTRL_STATE_DEFAULT);                 \
		if (ret < 0) {                                                                     \
			LOG_ERR("Failed to configure pins");                                       \
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
		enable_core_clock(dev);                                                            \
		I2SClearInt(config->reg_base, I2S_INT_ALL);                                        \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), ti_cc35xx_i2s_isr,    \
			    DEVICE_DT_GET(DT_INST(inst, DT_DRV_COMPAT)), 0);                       \
		irq_enable(DT_INST_IRQN(inst));                                                    \
		return 0;                                                                          \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(0, &ti_cc35xx_i2s_init##inst, NULL, &i2s_##inst##_data,              \
			      &i2s_##inst##_cfg, POST_KERNEL, CONFIG_I2S_INIT_PRIORITY, &i2s_api);

DT_INST_FOREACH_STATUS_OKAY(TI_CC35XX_I2S_INIT)

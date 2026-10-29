/*
 * Copyright (c) 2025-2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/cache.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mchp_sam_pmc.h>
#include <zephyr/sys/util.h>
#include <zephyr/irq.h>
#include <stdbool.h>

#define ADC_CONTEXT_ENABLE_ON_COMPLETE
#include "adc_context.h"

#define DT_DRV_COMPAT microchip_adc_g3

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adc_mchp_g3, CONFIG_ADC_LOG_LEVEL);

#define DMA_BUFFER_ALIGNED_SIZE	\
	ROUND_DOWN(CONFIG_ADC_MCHP_G3_DMA_BUFFER_SIZE / 2, \
		   CONFIG_DCACHE_LINE_SIZE)

#define T_CONV		14 /* Conversion time */
#define T_CH		2  /* Channel switching time */
#define T_TRACK		6  /* Defaule tracking time */
#define T_TRACK_15	7

#define MAX_CHANNELS	32

struct adc_oversampling {
	uint32_t osr:4;
	uint32_t rate:12;
	uint32_t bits:8;
	uint32_t oversampling:8;
};

struct adc_mchp_config {
	adc_registers_t *regs;
	const struct device *clk_dev;
	struct sam_clk_cfg clk_cfg;
	uint32_t clk_rate;
	const struct pinctrl_dev_config *pin_cfg;
	uint32_t min_adc_clk;
	uint32_t max_adc_clk;
	uint32_t startup_us;
	uint8_t max_osr_idx;
	uint8_t num_channels;
	bool temp_sensor;
	uint8_t temp_channel;
	uint32_t temp_startup_us;
	const struct adc_oversampling *osr_table;
	size_t osr_table_num;
	void (*irq_config_func)(void);

#ifdef CONFIG_ADC_MCHP_G3_DMA_DRIVEN
	const struct device *dma_dev;
	const uint32_t dma_channel;
	const uint32_t dma_perid;
#endif
};

struct adc_mchp_data {
	struct adc_context ctx;
	adc_registers_t *regs;
	uint32_t initialized;
	uint32_t channels;
	uint16_t acq_times[MAX_CHANNELS];
	uint8_t resolution;
	uint8_t res_shift;
	const struct adc_oversampling *osr;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	uint8_t trigger_mode;
	bool use_timer;
	struct k_timer timer;
	bool use_dma;

#ifdef CONFIG_ADC_MCHP_G3_DMA_DRIVEN
	struct dma_block_config dma_block_cfg;
	struct dma_config dma_cfg;

	/* Ping-pong buffer for ADC DMA */
	uint16_t dma_buffer[2][DMA_BUFFER_ALIGNED_SIZE / sizeof(uint16_t)]
		 __aligned(CONFIG_DCACHE_LINE_SIZE);
	uint32_t dma_buf_size;
	uint32_t dma_buf_idx;
	uint32_t dma_xfer_size;
	uint32_t dma_xfer_chunk;
#endif
};

static const struct adc_oversampling oversampling_table[] = {
	{ .osr = 0, .rate = 1,   .bits = 12, .oversampling = 0  },
	{ .osr = 1, .rate = 4,   .bits = 13, .oversampling = 2  },
	{ .osr = 2, .rate = 16,  .bits = 14, .oversampling = 4  },
	{ .osr = 3, .rate = 64,  .bits = 15, .oversampling = 8  },
	{ .osr = 4, .rate = 256, .bits = 16, .oversampling = 16 },
};

static inline bool is_temp_channel(const struct adc_mchp_config *cfg,
				   uint32_t channels)
{
	if ((cfg->temp_sensor) &&
	    (channels == BIT(cfg->temp_channel))) {
		return true;
	}

	return false;
}

static void adc_reset(adc_registers_t *adc)
{
	adc->ADC_CR = ADC_CR_SWRST_1;
	adc->ADC_MR = ADC_MR_ALWAYS1(1) |
		      ADC_MR_TRANSFER(2) |
		      ADC_MR_ANACH_ALLOWED;
}

static void adc_start(adc_registers_t *adc, uint8_t trigger)
{
	if ((adc->ADC_TRGR & ADC_TRGR_TRGMOD_Msk) !=
	    ADC_TRGR_TRGMOD(trigger)) {
		adc->ADC_TRGR |= ADC_TRGR_TRGMOD(trigger);
	}

	if (trigger == ADC_TRGR_TRGMOD_NO_TRIGGER_Val) {
		/* Software trigger */
		adc->ADC_CR = ADC_CR_START_1;
	}
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_mchp_data *data = CONTAINER_OF(ctx, struct adc_mchp_data, ctx);

	adc_start(data->regs, data->trigger_mode);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
					      bool repeat_sampling)
{
	struct adc_mchp_data *data = CONTAINER_OF(ctx, struct adc_mchp_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static void adc_context_on_complete(struct adc_context *ctx, int status)
{
	ARG_UNUSED(status);

	struct adc_mchp_data *data = CONTAINER_OF(ctx, struct adc_mchp_data, ctx);

	adc_reset(data->regs);
}

static inline void adc_context_enable_timer(struct adc_context *ctx)
{
	struct adc_mchp_data *data = CONTAINER_OF(ctx, struct adc_mchp_data, ctx);

	if (data->use_timer) {
		k_timer_start(&data->timer, K_NO_WAIT, K_USEC(ctx->options.interval_us));
	} else {
		adc_context_request_next_sampling(&data->ctx);
	}
}

static inline void adc_context_disable_timer(struct adc_context *ctx)
{
	struct adc_mchp_data *data = CONTAINER_OF(ctx, struct adc_mchp_data, ctx);

	if (data->use_timer) {
		k_timer_stop(&data->timer);
	}
}

static void adc_mchp_timer_handler(struct k_timer *timer)
{
	struct adc_mchp_data *data = CONTAINER_OF(timer, struct adc_mchp_data, timer);

#ifdef CONFIG_ADC_MCHP_G3_DMA_DRIVEN
	if (data->use_dma) {
		adc_context_start_sampling(&data->ctx);
		return;
	}
#endif

	adc_context_request_next_sampling(&data->ctx);
}

static inline void adc_context_abort(struct adc_context *ctx, int status)
{
	adc_context_disable_timer(ctx);
	adc_context_complete(ctx, status);
}

#ifdef CONFIG_ADC_MCHP_G3_DMA_DRIVEN
static int adc_dma_start(const struct device *dev)
{
	const struct adc_mchp_config *cfg = dev->config;
	struct adc_mchp_data *data = dev->data;
	struct dma_block_config *dma_block_cfg = &data->dma_block_cfg;
	uint16_t *buffer;
	uint32_t size;
	int ret;

	buffer = data->dma_buffer[data->dma_buf_idx];
	size = ROUND_DOWN(MIN(data->dma_xfer_size, data->dma_buf_size),
			  data->dma_xfer_chunk * sizeof(uint16_t));

	sys_cache_data_invd_range(buffer, size);
	barrier_dsync_fence_full();

	dma_block_cfg->dest_address = (uintptr_t)buffer;
	dma_block_cfg->block_size = size;

	ret = dma_config(cfg->dma_dev, cfg->dma_channel, &data->dma_cfg);
	if (ret != 0) {
		LOG_ERR("failed to configure ADC DMA");
		return ret;
	}

	ret = dma_start(cfg->dma_dev, cfg->dma_channel);
	if (ret != 0) {
		LOG_ERR("failed to start ADC DMA");
		return ret;
	}

	return 0;
}

static void dma_callback(const struct device *dma_dev,
			 void *user_data, uint32_t channel, int status)
{
	ARG_UNUSED(dma_dev);
	ARG_UNUSED(channel);

	const struct device *dev = user_data;
	struct adc_mchp_data *data = dev->data;
	uint16_t *buffer;
	uint32_t size, i, j;
	int ret;

	if (status != 0) {
		LOG_ERR("DMA callback error, status=%08x", status);
		adc_context_abort(&data->ctx, -EIO);
		return;
	}

	buffer = data->dma_buffer[data->dma_buf_idx];
	size = ROUND_DOWN(MIN(data->dma_xfer_size, data->dma_buf_size),
			  data->dma_xfer_chunk * sizeof(uint16_t));

	data->dma_xfer_size -= size;
	if (data->dma_xfer_size > 0U) {
		data->dma_buf_idx ^= 1U;

		ret = adc_dma_start(dev);
		if (ret != 0) {
			adc_context_abort(&data->ctx, ret);
			return;
		}
	}

	sys_cache_data_invd_range(buffer, size);
	barrier_dsync_fence_full();

	for (i = 0; i < size / sizeof(uint16_t); i += data->dma_xfer_chunk) {
		for (j = 0; j < data->dma_xfer_chunk; j++) {
			*data->buffer++ = buffer[i + j] >> data->res_shift;
		}

		adc_context_on_sampling_done(&data->ctx, dev);
	}
}

static void adc_dma_init_config(const struct device *dev)
{
	const struct adc_mchp_config *cfg = dev->config;
	struct adc_mchp_data *data = dev->data;
	adc_registers_t *adc = cfg->regs;
	struct dma_block_config *dma_block_cfg = &data->dma_block_cfg;
	struct dma_config *dma_cfg = &data->dma_cfg;

	dma_block_cfg->source_address = (uintptr_t)&adc->ADC_LCDR;
	dma_block_cfg->dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;

	dma_cfg->dma_slot = cfg->dma_perid;
	dma_cfg->channel_direction = PERIPHERAL_TO_MEMORY;
	dma_cfg->complete_callback_en = true;
	dma_cfg->source_data_size = sizeof(uint16_t);
	dma_cfg->dest_data_size = sizeof(uint16_t);
	dma_cfg->source_burst_length = 1;
	dma_cfg->dest_burst_length = 1;
	dma_cfg->block_count = 1;
	dma_cfg->head_block = dma_block_cfg;
	dma_cfg->user_data = (void *)dev;
	dma_cfg->dma_callback = dma_callback;
}
#endif

static void adc_isr(const struct device *dev)
{
	const struct adc_mchp_config *cfg = dev->config;
	struct adc_mchp_data *data = dev->data;
	adc_registers_t *adc = cfg->regs;
	uint32_t isr, channels;

	isr = adc->ADC_ISR;

	LOG_DBG("isr:0x%08x eoc_isr:0x%08x channels:0x%08x",
		isr, adc->ADC_EOC_ISR, data->channels);

#ifdef CONFIG_ADC_MCHP_G3_DMA_DRIVEN
	if (data->use_dma) {
		if ((isr & ADC_IER_GOVRE_Msk) != 0U) {
			LOG_ERR("General overrun error occurred");
			adc_context_abort(&data->ctx, -EIO);
		}

		return;
	}
#endif

	if (adc->ADC_EOC_ISR != data->channels) {
		/* Check whether all sequence channels have been completed */
		LOG_DBG("Incorrect status, eoc_isr:0x%08x channels:0x%08x",
			adc->ADC_EOC_ISR, data->channels);
		adc_context_abort(&data->ctx, -EIO);
		return;
	}

	/* Check for overrun status. */
	if ((adc->ADC_OVER & data->channels) != 0U) {
		LOG_ERR("Overrun error occurred");
		adc_context_abort(&data->ctx, -EIO);
		return;
	}

	channels = data->channels;
	while (channels != 0U) {
		uint8_t id = find_lsb_set(channels) - 1;

		*data->buffer++ = (uint16_t)adc->ADC_CDR[id] >> data->res_shift;
		channels &= ~BIT(id);
	}

	adc_context_on_sampling_done(&data->ctx, dev);
}

static const struct adc_oversampling *
osr_table_lookup(const struct device *dev, int oversampling)
{
	const struct adc_mchp_config *cfg = dev->config;
	size_t i;

	for (i = 0; i < cfg->osr_table_num; i++) {
		if (cfg->osr_table[i].oversampling == oversampling) {
			return &cfg->osr_table[i];
		}
	}

	return NULL;
}

static uint32_t to_acq_cycles(uint16_t acq_time, uint32_t clk)
{
	uint32_t cycles;

	if (acq_time == ADC_ACQ_TIME_DEFAULT) {
		cycles = 0;
	} else {
		uint8_t unit = ADC_ACQ_TIME_UNIT(acq_time);
		uint16_t value = ADC_ACQ_TIME_VALUE(acq_time);

		if (unit == ADC_ACQ_TIME_MICROSECONDS) {
			cycles = DIV_ROUND_UP((uint64_t)clk * value,
						    1000000ULL);
		} else if (unit == ADC_ACQ_TIME_NANOSECONDS) {
			cycles = DIV_ROUND_UP((uint64_t)clk * value,
						    1000000000ULL);
		} else {
			cycles = value;
		}
	}

	return cycles;
}

static uint32_t max_acq_cycles(const struct device *dev, uint32_t clk)
{
	struct adc_mchp_data *data = dev->data;
	uint32_t channels = data->channels;
	uint32_t cycles, max_cycles = 0;
	uint16_t acq_time = 0;

	while (channels != 0U) {
		uint8_t id;

		id = find_lsb_set(channels) - 1;
		channels &= ~BIT(id);

		if (data->acq_times[id] == acq_time) {
			continue;
		}

		cycles = to_acq_cycles(data->acq_times[id], clk);
		if (cycles > max_cycles) {
			max_cycles = cycles;
		}

		acq_time = data->acq_times[id];
	}


	return max_cycles;
}

static int startup_time(uint32_t clk, uint32_t startup_us)
{
	static const uint16_t startup_lookup[] = {
		  0,   8,  16,  24,
		 64,  80,  96, 112,
		512, 576, 640, 704,
		768, 832, 896, 960
	};
	uint32_t cycles;
	size_t i;

	if (clk == 0U) {
		return -EINVAL;
	}

	cycles = DIV_ROUND_UP((uint64_t)clk * startup_us,
			      1000000ULL);
	for (i = 0; i < ARRAY_SIZE(startup_lookup); i++) {
		if (startup_lookup[i] >= cycles) {
			break;
		}
	}

	if (i >= ARRAY_SIZE(startup_lookup)) {
		return -EINVAL;
	}

	return i;
}

static int tracking_time(uint32_t clk, uint32_t acq_cycles, uint32_t *tracktim,
			 uint32_t *trackx, uint32_t *actual_cycles)
{
	static const uint8_t trackx_lookup[] = {
		1, 4, 8, 16
	};
	uint32_t cycles;
	size_t i, j;

	if (acq_cycles <= T_TRACK) {
		*tracktim = 0;
		*trackx = 0;
		if (actual_cycles != NULL) {
			*actual_cycles = T_TRACK;
		}

		return 0;
	}

	if (acq_cycles == T_TRACK_15) {
		*tracktim = 15;
		*trackx = 0;
		if (actual_cycles != NULL) {
			*actual_cycles = T_TRACK_15;
		}

		return 0;
	}

	for (i = 1; i < ARRAY_SIZE(trackx_lookup); i++) {
		for (j = 4; j <= 15; j++) {
			cycles = (trackx_lookup[i] * (j + 1)) - 10;

			if (cycles >= acq_cycles) {
				break;
			}
		}

		if (j <= 15) {
			break;
		}
	}

	if (i >= ARRAY_SIZE(trackx_lookup)) {
		return -EINVAL;
	}

	*tracktim = j;
	*trackx = i;
	if (actual_cycles != NULL) {
		*actual_cycles = cycles;
	}

	return 0;
}

static uint32_t adc_cycles_per_sequence(uint32_t channels, uint32_t ccr,
					uint32_t osr, uint32_t tracking)
{
	uint32_t cycles, ch_cycles = 0;

	ccr &= channels;
	cycles = (tracking + T_CONV) * osr;

	if ((ccr != 0U) && (ccr != channels)) {
		uint32_t mask = channels;
		uint8_t bit = find_msb_set(mask) - 1;
		bool previous = (ccr & BIT(bit)) != 0U;

		/*
		 * Compare adjacent channel settings, including
		 * the last-to-first transition.
		 */
		while (mask != 0U) {
			bool current;

			bit = find_lsb_set(mask) - 1;
			current = (ccr & BIT(bit)) != 0U;

			if (current != previous) {
				ch_cycles += T_CH;
			}

			previous = current;
			mask &= ~BIT(bit);
		}
	}

	return cycles * POPCOUNT(channels) + ch_cycles;
}

static int adc_configure(const struct device *dev,
			 const struct adc_sequence_options *options)
{
	const struct adc_mchp_config *cfg = dev->config;
	struct adc_mchp_data *data = dev->data;
	adc_registers_t *adc = cfg->regs;
	uint32_t prescal, adc_clk;
	uint32_t tracktim, trackx, track_cycles;
	uint32_t cycles, interval_cycles = 0;
	uint32_t mr, emr, trgr = 0;
	uint32_t startup_us, acq_cycles;
	int ret;

	if (is_temp_channel(cfg, data->channels)) {
		startup_us = cfg->temp_startup_us;
	} else {
		startup_us = cfg->startup_us;
	}

	prescal = DIV_ROUND_UP((uint64_t)cfg->clk_rate, 2 * cfg->max_adc_clk) - 1;
Retry:
	if (prescal > (ADC_MR_PRESCAL_Msk >> ADC_MR_PRESCAL_Pos)) {
		LOG_ERR("Prescal is out of range: 0x%x", prescal);
		return -EINVAL;
	}

	adc_clk = cfg->clk_rate / (prescal + 1) / 2;
	if (adc_clk < cfg->min_adc_clk) {
		LOG_ERR("ADC clock is out of range: %d", adc_clk);
		return -EINVAL;
	}
	mr = ADC_MR_PRESCAL(prescal);
	emr = ADC_EMR_ASTE_SINGLE_TRIG_AVERAGE |
	      ADC_EMR_OSR(data->osr->osr);

	/* Setup start-Up time */
	ret = startup_time(adc_clk, startup_us);
	if (ret < 0) {
		prescal++;
		goto Retry;
	}
	mr |= ADC_MR_STARTUP(ret);

	/* Setup tracking time */
	if (is_temp_channel(cfg, data->channels)) {
		acq_cycles = to_acq_cycles(data->acq_times[cfg->temp_channel], adc_clk);
	} else {
		/* Find the max acquisition time in this sequence */
		acq_cycles = max_acq_cycles(dev, adc_clk);
	}

	ret = tracking_time(adc_clk, acq_cycles,
			    &tracktim, &trackx, &track_cycles);
	if (ret < 0) {
		prescal++;
		goto Retry;
	}

	mr |= ADC_MR_TRACKTIM(tracktim);
	emr |= ADC_EMR_TRACKX(trackx);

	/* For checking the sampling rate */
	cycles = adc_cycles_per_sequence(data->channels, adc->ADC_CCR,
					 data->osr->rate, track_cycles);

	if (options == NULL) {
		data->trigger_mode = ADC_TRGR_TRGMOD_NO_TRIGGER_Val;
	} else if (options->interval_us == 0U) {
		data->trigger_mode = ADC_TRGR_TRGMOD_NO_TRIGGER_Val;
#ifdef CONFIG_ADC_MCHP_G3_DMA_DRIVEN
		if (data->use_dma) {
			data->trigger_mode = ADC_TRGR_TRGMOD_CONTINUOUS_Val;
		}
#endif
	} else {
		interval_cycles = DIV_ROUND_UP((uint64_t)adc_clk * options->interval_us,
					       1000000ULL);

		if (cycles > interval_cycles) {
			LOG_ERR("Unachievable sampling rate, "
				"adc_clk: %u, cycles: %u, interval_cycles: %u (%uus)",
				adc_clk, cycles, interval_cycles, options->interval_us);
			return -EINVAL;
		} else if (cycles == interval_cycles) {
			data->trigger_mode = ADC_TRGR_TRGMOD_NO_TRIGGER_Val;
#ifdef CONFIG_ADC_MCHP_G3_DMA_DRIVEN
			if (data->use_dma) {
				data->trigger_mode = ADC_TRGR_TRGMOD_CONTINUOUS_Val;
			}
#endif
		} else {
			if (interval_cycles <=
			    ((ADC_TRGR_TRGPER_Msk >> ADC_TRGR_TRGPER_Pos) + 1)) {
				/* Setup trigger period */
				trgr = ADC_TRGR_TRGPER(interval_cycles - 1);
				data->trigger_mode = ADC_TRGR_TRGMOD_PERIOD_TRIG_Val;
			} else {
				/* Use adc context timer */
				data->use_timer = true;
				data->trigger_mode = ADC_TRGR_TRGMOD_NO_TRIGGER_Val;
#ifdef CONFIG_ADC_MCHP_G3_DMA_DRIVEN
				if ((data->use_dma) &&
				    (is_temp_channel(cfg, data->channels))) {
					/*
					 * Temperature sensor channel won't update
					 * ADC_LCDR when ADC_TRGR.TRGMOD = 0.
					 * Force disable DMA function.
					 */
					data->use_dma = false;
				}
#endif
			}
		}
	}

	/* Update the ADC register settings */
	adc->ADC_MR |= mr;
	adc->ADC_EMR |= emr;
	adc->ADC_CHER = data->channels;
	adc->ADC_TRGR = trgr;

	if (is_temp_channel(cfg, data->channels)) {
		adc->ADC_TEMPMR = ADC_TEMPMR_TEMPON_1;
	}

#ifdef CONFIG_ADC_MCHP_G3_DMA_DRIVEN
	if (data->use_dma) {
		/* Enable FIFO for DMA */
		adc->ADC_FMR = ADC_FMR_ENFIFO_1;
		/* Enable general overrun error */
		adc->ADC_IER |= ADC_IER_GOVRE_Msk;

		ret = adc_dma_start(dev);
		if (ret != 0) {
			adc_context_abort(&data->ctx, ret);
			return ret;
		}
	} else {
		/*
		 * Only the interrupt for the channel with the highest number in the
		 * sequence is enabled to reduce the number of interrupts.
		 */
		adc->ADC_EOC_IER = BIT(find_msb_set(data->channels) - 1);
	}
#else
	adc->ADC_EOC_IER = BIT(find_msb_set(data->channels) - 1);
#endif

	LOG_DBG("clock:%u cycles:%u rate:%u interval:%u timer:%d dma:%d",
		adc_clk, cycles, adc_clk / cycles, interval_cycles,
		data->use_timer, data->use_dma);
	LOG_DBG("MR:0x%08x EMR:0x%08x CHSR:0x%08x TRGR:0x%08x",
		adc->ADC_MR, adc->ADC_EMR, adc->ADC_CHSR,
		adc->ADC_TRGR | ADC_TRGR_TRGMOD(data->trigger_mode));

	return 0;
}

static int adc_validate_sequence(const struct device *dev,
				 const struct adc_sequence *sequence)
{
	const struct adc_mchp_config *cfg = dev->config;
	struct adc_mchp_data *data = dev->data;
	const struct adc_sequence_options *options = sequence->options;
	uint32_t count, size;

	LOG_DBG("channels:0x%08x resolution:%u oversampling:%u calibrate:%d",
		sequence->channels, sequence->resolution,
		sequence->oversampling, sequence->calibrate);

	if (options != NULL) {
		LOG_DBG("interval_us:%u extra_samplings:%u",
			options->interval_us, options->extra_samplings);
	}

	if (sequence->channels == 0U) {
		LOG_ERR("No channel to read");
		return -EINVAL;
	}

	if ((cfg->temp_sensor) &&
	    ((sequence->channels & BIT(cfg->temp_channel)) != 0U) &&
	    (sequence->channels != BIT(cfg->temp_channel))) {
		LOG_ERR("Temperature channel cannot be mixed with others");
		return -EINVAL;
	}

	if ((sequence->channels & data->initialized) != sequence->channels) {
		LOG_ERR("Uninitialized channels mask: 0x%08x",
			sequence->channels & ~data->initialized);
		return -EINVAL;
	}

	if (sequence->buffer == NULL) {
		LOG_ERR("Sequence's buffer is NULL");
		return -EINVAL;
	}

	count = POPCOUNT(sequence->channels);
	size = count * sizeof(uint16_t);
	if (options != NULL) {
		size += options->extra_samplings * size;
	}
	if (sequence->buffer_size < size) {
		LOG_ERR("Sequence's buffer is too small (%u/%u)",
			sequence->buffer_size, size);
		return -EINVAL;
	}

	data->osr = osr_table_lookup(dev, sequence->oversampling);
	if (data->osr == NULL) {
		LOG_ERR("Unsupported oversampling %d, check oversampling_table",
			sequence->oversampling);
		return -EINVAL;
	}

	if (sequence->resolution > data->osr->bits) {
		LOG_ERR("Unreachable resolution %u, current maximum is %u bits",
			sequence->resolution,
			data->osr->bits);
		return -EINVAL;

	}

	if (sequence->calibrate) {
		/* TODO */
		return -EINVAL;
	}

	data->channels = sequence->channels;
	data->resolution = sequence->resolution;
	data->res_shift = data->osr->bits - sequence->resolution;
	data->buffer = sequence->buffer;
	data->repeat_buffer = sequence->buffer;
	data->use_timer = false;

#ifdef CONFIG_ADC_MCHP_G3_DMA_DRIVEN
	if ((options != NULL) && (options->extra_samplings > 0U)) {
		data->use_dma = true;
		data->dma_xfer_size = size;
		data->dma_xfer_chunk = count;
		data->dma_buf_idx = 0;
	} else {
		data->use_dma = false;
	}
#endif

	LOG_DBG("osr_rate:%u osr_bits:%u resolution_shift:%u",
		data->osr->rate, data->osr->bits, data->res_shift);

	return 0;
}

static int adc_start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct adc_mchp_data *data = dev->data;
	int ret;

	ret = adc_validate_sequence(dev, sequence);
	if (ret != 0) {
		return ret;
	}

	ret = adc_configure(dev, sequence->options);
	if (ret != 0) {
		return ret;
	}

	adc_context_start_read(&data->ctx, sequence);

	ret = adc_context_wait_for_completion(&data->ctx);

	return ret;
}

static int adc_mchp_channel_setup(const struct device *dev,
				  const struct adc_channel_cfg *channel_cfg)
{
	const struct adc_mchp_config *cfg = dev->config;
	struct adc_mchp_data *data = dev->data;
	adc_registers_t *adc = cfg->regs;
	uint32_t ccr_mask = ADC_CCR_DIFF1_Msk | ADC_CCR_DIFF0_Msk;

	if ((channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) &&
	    (ADC_ACQ_TIME_UNIT(channel_cfg->acquisition_time) == 0U)) {
		LOG_ERR("Invalid acquisition_time: 0x%04x",
			channel_cfg->acquisition_time);
		return -EINVAL;
	}

	/* Check if it is temperature channel */
	if ((cfg->temp_sensor) &&
	    (channel_cfg->channel_id == cfg->temp_channel)) {
		data->acq_times[channel_cfg->channel_id] = channel_cfg->acquisition_time;
		data->initialized |= BIT(channel_cfg->channel_id);
		return 0;
	}

	if (channel_cfg->channel_id >= cfg->num_channels) {
		LOG_ERR("Invalid channel id: %u", channel_cfg->channel_id);
		return -EINVAL;
	}

	data->initialized &= ~BIT(channel_cfg->channel_id);

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("Unsupported adc gain: %d", channel_cfg->gain);
		return -ENOTSUP;
	}

	if (channel_cfg->reference != ADC_REF_INTERNAL) {
		LOG_ERR("Unsupported adc reference: %d", channel_cfg->reference);
		return -ENOTSUP;
	}

	if (channel_cfg->differential) {
		if (channel_cfg->channel_id & 0x1) {
			LOG_ERR("Invalid differential channel id: %u",
				channel_cfg->channel_id);
			return -EINVAL;
		}

		adc->ADC_CCR |= ccr_mask << channel_cfg->channel_id;
	} else {
		adc->ADC_CCR &= ~(ccr_mask << (channel_cfg->channel_id & ~0x1));
	}

	data->acq_times[channel_cfg->channel_id] = channel_cfg->acquisition_time;
	data->initialized |= BIT(channel_cfg->channel_id);

	return 0;
}

static int adc_mchp_read(const struct device *dev,
			 const struct adc_sequence *sequence)
{
	struct adc_mchp_data *data = dev->data;
	int ret;

	adc_context_lock(&data->ctx, false, NULL);
	ret = adc_start_read(dev, sequence);
	adc_context_release(&data->ctx, ret);

	return ret;
}

#ifdef CONFIG_ADC_ASYNC
static int adc_mchp_read_async(const struct device *dev,
			       const struct adc_sequence *sequence,
			       struct k_poll_signal *async)
{
	struct adc_mchp_data *data = dev->data;
	int ret;

	adc_context_lock(&data->ctx, true, async);
	ret = adc_start_read(dev, sequence);
	adc_context_release(&data->ctx, ret);

	return ret;
}
#endif /* CONFIG_ADC_ASYNC */

static int adc_mchp_init(const struct device *dev)
{
	const struct adc_mchp_config *cfg = dev->config;
	struct adc_mchp_data *data = dev->data;
	int ret;

	if (!device_is_ready(cfg->clk_dev)) {
		LOG_ERR("Clock device not ready");
		return -ENODEV;
	}

	ret = clock_control_on(cfg->clk_dev,
			       (clock_control_subsys_t)(uintptr_t)&cfg->clk_cfg);
	if (ret) {
		LOG_ERR("Clock op failed");
		return ret;
	}

	ret = pinctrl_apply_state(cfg->pin_cfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

#ifdef CONFIG_ADC_MCHP_G3_DMA_DRIVEN
	adc_dma_init_config(dev);

	data->dma_buf_size = DMA_BUFFER_ALIGNED_SIZE;
#endif

	data->regs = cfg->regs;
	adc_context_unlock_unconditionally(&data->ctx);
	k_timer_init(&data->timer, adc_mchp_timer_handler, NULL);

	adc_reset(cfg->regs);

	cfg->irq_config_func();

	return 0;
}

static DEVICE_API(adc, adc_mchp_api) = {
	.channel_setup = adc_mchp_channel_setup,
	.read = adc_mchp_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = adc_mchp_read_async,
#endif
};

#define ADC_MCHP_G3_DMA_INIT(n)							\
	.dma_dev = DEVICE_DT_GET_OR_NULL(DT_INST_DMAS_CTLR_BY_NAME(n, rx)),	\
	.dma_channel = DT_INST_DMAS_CELL_BY_NAME(n, rx, channel),		\
	.dma_perid = DT_INST_DMAS_CELL_BY_NAME(n, rx, perid),

#ifdef CONFIG_ADC_MCHP_G3_DMA_DRIVEN
#define ADC_USE_DMA(n) DT_INST_DMAS_HAS_NAME(n, rx)
#else
#define ADC_USE_DMA(n) 0
#endif

#define ADC_MCHP_DEVICE_DEFINE(n)							\
	static void adc_mchp_irq_config_func_##n(void);					\
											\
	PINCTRL_DT_INST_DEFINE(n);							\
											\
	static struct adc_mchp_data adc_mchp_data_##n = {				\
		ADC_CONTEXT_INIT_LOCK(adc_mchp_data_##n, ctx),				\
		ADC_CONTEXT_INIT_SYNC(adc_mchp_data_##n, ctx),				\
	};										\
											\
	static struct adc_mchp_config adc_mchp_config_##n = {				\
		.regs = (adc_registers_t *)DT_INST_REG_ADDR(n),				\
		.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(n, adc_clk)),	\
		.clk_cfg = SAM_DT_INST_CLOCK_PMC_CFG(n),				\
		.clk_rate = DT_INST_PROP(n, assigned_clock_rates),			\
		.pin_cfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),				\
		.min_adc_clk = DT_INST_PROP(n, min_adc_clock_rates),			\
		.max_adc_clk = DT_INST_PROP(n, max_adc_clock_rates),			\
		.startup_us = DT_INST_PROP(n, startup_time_us),				\
		.max_osr_idx = DT_INST_ENUM_IDX(n, max_oversampling_ratio),		\
		.num_channels = DT_INST_PROP(n, num_channels),				\
		.temp_sensor = DT_INST_NODE_HAS_PROP(n, temperature_channel),		\
		.temp_channel = DT_INST_PROP_OR(n, temperature_channel, 0),		\
		.temp_startup_us = DT_INST_PROP_OR(n, temperature_startup_us, 0),	\
		.osr_table = oversampling_table,					\
		.osr_table_num = ARRAY_SIZE(oversampling_table),			\
		.irq_config_func = adc_mchp_irq_config_func_##n,			\
		COND_CODE_1(ADC_USE_DMA(n), (ADC_MCHP_G3_DMA_INIT(n)), ())		\
	};										\
											\
	DEVICE_DT_INST_DEFINE(n, adc_mchp_init, NULL,					\
			      &adc_mchp_data_##n, &adc_mchp_config_##n,			\
			      POST_KERNEL, CONFIG_ADC_INIT_PRIORITY,			\
			      &adc_mchp_api);						\
											\
	static void adc_mchp_irq_config_func_##n(void)					\
	{										\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),			\
			    adc_isr, DEVICE_DT_INST_GET(n), 0);				\
		irq_enable(DT_INST_IRQN(n));						\
	}

DT_INST_FOREACH_STATUS_OKAY(ADC_MCHP_DEVICE_DEFINE)

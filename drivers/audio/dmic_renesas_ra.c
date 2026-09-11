/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_pdm

#include <zephyr/audio/dmic.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/renesas_ra_cgc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <r_pdm.h>

#include <soc.h>

LOG_MODULE_REGISTER(dmic_renesas_ra, CONFIG_AUDIO_DMIC_LOG_LEVEL);

/*
 * How far, in per mille, the requested PCM rate may sit from the channel's nominal rate. The
 * decimation chain only reaches rates that divide the PDM source clock, so a nominal rate such
 * as 32 kHz is approached rather than met exactly.
 */
#define PDM_RA_RATE_TOLERANCE_PERMILLE 20U

/* Samples leave the FIFO right justified in a 32-bit word, sign bits included. */
#define PDM_RA_FIFO_SAMPLE_SIZE sizeof(uint32_t)

/* Largest data reception interrupt threshold, in samples. */
#define PDM_RA_MAX_THRESHOLD_SAMPLES 16U

/*
 * Filter coefficients and shifts are the RA Configuration tool defaults documented for r_pdm.
 * They do not depend on the sample rate, which the decimation properties select.
 */
#define PDM_RA_HPF_COEFFICIENT_S0 0x3F61
#define PDM_RA_HPF_COEFFICIENT_K1 0x3EC1
#define PDM_RA_LPF_COEFFICIENT_H0 0x0400

#define PDM_RA_HPF_COEFFICIENT_H {0x4000, 0xC000}

#define PDM_RA_CF_COEFFICIENT_H                                                                    \
	{0x1FE8, 0x0039, 0x003C, 0x1E56, 0x01DC, 0x06E1, 0x01DC, 0x1E56, 0x003C, 0x0039, 0x1FE8}

#define PDM_RA_LPF_COEFFICIENT_H1                                                                  \
	{0x1FF8, 0x000A, 0x1FF0, 0x0018, 0x1FDC, 0x0034, 0x1FB3, 0x0076, 0x1F2E, 0x0289,           \
	 0x0289, 0x1F2E, 0x0076, 0x1FB3, 0x0034, 0x1FDC, 0x0018, 0x1FF0, 0x000A, 0x1FF8}

struct dmic_renesas_ra_config {
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_dev;
	const struct clock_control_ra_subsys_cfg clock_subsys;
	void (*irq_config_func)(void);
	uint32_t *fifo_buf;
	uint32_t fifo_buf_samples;
	uint32_t pcm_rate;
	uint32_t settling_time_us;
};

struct dmic_renesas_ra_data {
	pdm_instance_ctrl_t fsp_ctrl;
	pdm_cfg_t fsp_cfg;
	pdm_extended_cfg_t fsp_ext_cfg;
	struct k_mem_slab *mem_slab;
	struct k_msgq rx_queue;
	char *rx_msgs;
	/* Portion of the FIFO buffer in use, a whole number of blocks. */
	uint32_t fifo_samples;
	uint32_t read_offset;
	uint32_t block_size;
	uint32_t block_samples;
	uint8_t pcm_width;
	volatile enum dmic_state state;
};

/*
 * Copy one block out of the FIFO buffer that the FSP driver fills circularly, narrowing the
 * samples to the configured PCM width on the way.
 */
static void dmic_renesas_ra_copy_block(struct dmic_renesas_ra_data *data, const uint32_t *src,
				       void *dst)
{
	if (data->pcm_width == 16U) {
		int16_t *out = dst;

		for (uint32_t i = 0; i < data->block_samples; i++) {
			out[i] = (int16_t)(src[i] & 0xFFFFU);
		}
	} else {
		int32_t *out = dst;

		/* Left justify the 20-bit samples so that full scale matches int32_t. */
		for (uint32_t i = 0; i < data->block_samples; i++) {
			out[i] = (int32_t)(src[i] << 12);
		}
	}
}

static void dmic_renesas_ra_data_event(const struct device *dev)
{
	const struct dmic_renesas_ra_config *cfg = dev->config;
	struct dmic_renesas_ra_data *data = dev->data;
	void *block;
	int ret;

	ret = k_mem_slab_alloc(data->mem_slab, &block, K_NO_WAIT);
	if (ret < 0) {
		LOG_ERR("no free block, dropping %u samples", data->block_samples);
		goto advance;
	}

	dmic_renesas_ra_copy_block(data, &cfg->fifo_buf[data->read_offset], block);

	ret = k_msgq_put(&data->rx_queue, &block, K_NO_WAIT);
	if (ret < 0) {
		LOG_ERR("rx queue full, dropping block");
		k_mem_slab_free(data->mem_slab, block);
	}

advance:
	data->read_offset += data->block_samples;
	if (data->read_offset >= data->fifo_samples) {
		data->read_offset = 0;
	}
}

static void dmic_renesas_ra_callback(pdm_callback_args_t *p_args)
{
	const struct device *dev = p_args->p_context;
	struct dmic_renesas_ra_data *data = dev->data;

	switch (p_args->event) {
	case PDM_EVENT_DATA:
		dmic_renesas_ra_data_event(dev);
		break;
	case PDM_EVENT_ERROR:
		LOG_ERR("PDM error 0x%x", (unsigned int)p_args->error);
		data->state = DMIC_STATE_ERROR;
		break;
	default:
		break;
	}
}

static void dmic_renesas_ra_purge_queue(struct dmic_renesas_ra_data *data)
{
	void *block;

	while (k_msgq_get(&data->rx_queue, &block, K_NO_WAIT) == 0) {
		k_mem_slab_free(data->mem_slab, block);
	}
}

/*
 * Pick the largest data reception interrupt threshold that divides a block, so that a block
 * boundary always lands on an interrupt.
 */
static pdm_interrupt_threshold_t dmic_renesas_ra_threshold(uint32_t block_samples)
{
	uint32_t threshold = 0U;

	for (uint32_t samples = 2U; samples <= PDM_RA_MAX_THRESHOLD_SAMPLES; samples *= 2U) {
		if ((block_samples % samples) != 0U) {
			break;
		}
		threshold++;
	}

	return (pdm_interrupt_threshold_t)threshold;
}

static int dmic_renesas_ra_configure(const struct device *dev, struct dmic_cfg *dmic_cfg)
{
	const struct dmic_renesas_ra_config *cfg = dev->config;
	struct dmic_renesas_ra_data *data = dev->data;
	struct pcm_stream_cfg *stream = &dmic_cfg->streams[0];
	struct pdm_chan_cfg *channel = &dmic_cfg->channel;
	uint32_t sample_bytes;
	uint32_t block_samples;
	uint32_t tolerance;
	fsp_err_t fsp_err;

	if (data->state == DMIC_STATE_ACTIVE) {
		LOG_ERR("cannot reconfigure while active");
		return -EBUSY;
	}

	if (channel->req_num_streams != 1U || channel->req_num_chan != 1U) {
		LOG_ERR("only a single mono stream is supported");
		return -EINVAL;
	}

	if (stream->pcm_width != 16U && stream->pcm_width != 32U) {
		LOG_ERR("unsupported PCM width %u", stream->pcm_width);
		return -EINVAL;
	}

	tolerance = (cfg->pcm_rate * PDM_RA_RATE_TOLERANCE_PERMILLE) / 1000U;
	if (stream->pcm_rate > cfg->pcm_rate + tolerance ||
	    stream->pcm_rate + tolerance < cfg->pcm_rate) {
		LOG_ERR("PCM rate %u out of reach, the channel is decimated to %u",
			stream->pcm_rate, cfg->pcm_rate);
		return -EINVAL;
	}

	sample_bytes = stream->pcm_width / 8U;
	if ((stream->block_size % sample_bytes) != 0U) {
		LOG_ERR("block size %u is not a whole number of samples", stream->block_size);
		return -EINVAL;
	}

	block_samples = stream->block_size / sample_bytes;

	/*
	 * The FSP driver fills the FIFO buffer circularly and reports every block, so the
	 * buffer must hold at least two blocks: one being copied out while the next fills.
	 */
	if (block_samples == 0U || block_samples * 2U > cfg->fifo_buf_samples) {
		LOG_ERR("block of %u samples does not fit twice in the %u sample FIFO buffer",
			block_samples, cfg->fifo_buf_samples);
		return -EINVAL;
	}

	if (data->state != DMIC_STATE_INITIALIZED) {
		R_PDM_Close(&data->fsp_ctrl);
		dmic_renesas_ra_purge_queue(data);
	}

	data->mem_slab = stream->mem_slab;
	data->block_size = stream->block_size;
	data->block_samples = block_samples;
	data->pcm_width = stream->pcm_width;
	data->fifo_samples = (cfg->fifo_buf_samples / block_samples) * block_samples;

	data->fsp_ext_cfg.interrupt_threshold = dmic_renesas_ra_threshold(block_samples);
	data->fsp_cfg.pcm_width = (stream->pcm_width == 16U) ? PDM_PCM_WIDTH_16_BITS_4_18
							     : PDM_PCM_WIDTH_20_BITS_0_18;

	fsp_err = R_PDM_Open(&data->fsp_ctrl, &data->fsp_cfg);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("R_PDM_Open failed (%d)", fsp_err);
		data->state = DMIC_STATE_ERROR;
		return -EIO;
	}

	/* Opening the channel starts the filters and the microphone clock; let both settle. */
	k_sleep(K_USEC(cfg->settling_time_us));

	channel->act_num_streams = 1U;
	channel->act_num_chan = 1U;
	channel->act_chan_map_lo = channel->req_chan_map_lo;
	stream->pcm_rate = cfg->pcm_rate;

	data->state = DMIC_STATE_CONFIGURED;

	return 0;
}

static int dmic_renesas_ra_start(const struct device *dev)
{
	const struct dmic_renesas_ra_config *cfg = dev->config;
	struct dmic_renesas_ra_data *data = dev->data;
	fsp_err_t fsp_err;

	data->read_offset = 0;

	fsp_err = R_PDM_Start(&data->fsp_ctrl, cfg->fifo_buf,
			      data->fifo_samples * PDM_RA_FIFO_SAMPLE_SIZE, data->block_samples);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("R_PDM_Start failed (%d)", fsp_err);
		return -EIO;
	}

	data->state = DMIC_STATE_ACTIVE;

	return 0;
}

static int dmic_renesas_ra_stop(const struct device *dev, enum dmic_state next_state)
{
	struct dmic_renesas_ra_data *data = dev->data;
	fsp_err_t fsp_err;

	fsp_err = R_PDM_Stop(&data->fsp_ctrl);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("R_PDM_Stop failed (%d)", fsp_err);
		return -EIO;
	}

	data->state = next_state;

	return 0;
}

static int dmic_renesas_ra_trigger(const struct device *dev, enum dmic_trigger cmd)
{
	struct dmic_renesas_ra_data *data = dev->data;
	int ret;

	switch (cmd) {
	case DMIC_TRIGGER_START:
	case DMIC_TRIGGER_RELEASE:
		if (data->state == DMIC_STATE_ACTIVE) {
			return 0;
		}
		if (data->state != DMIC_STATE_CONFIGURED && data->state != DMIC_STATE_PAUSED) {
			LOG_ERR("device is not configured");
			return -EIO;
		}
		return dmic_renesas_ra_start(dev);

	case DMIC_TRIGGER_PAUSE:
		if (data->state != DMIC_STATE_ACTIVE) {
			return 0;
		}
		return dmic_renesas_ra_stop(dev, DMIC_STATE_PAUSED);

	case DMIC_TRIGGER_STOP:
	case DMIC_TRIGGER_RESET:
		if (data->state == DMIC_STATE_ACTIVE || data->state == DMIC_STATE_PAUSED) {
			ret = dmic_renesas_ra_stop(dev, DMIC_STATE_CONFIGURED);
			if (ret < 0) {
				return ret;
			}
		}
		dmic_renesas_ra_purge_queue(data);
		return 0;

	default:
		LOG_ERR("invalid trigger %d", cmd);
		return -EINVAL;
	}
}

static int dmic_renesas_ra_read(const struct device *dev, uint8_t stream, void **buffer,
				size_t *size, int32_t timeout)
{
	struct dmic_renesas_ra_data *data = dev->data;
	int ret;

	ARG_UNUSED(stream);

	if (data->state != DMIC_STATE_ACTIVE && data->state != DMIC_STATE_PAUSED) {
		LOG_ERR("device is not streaming");
		return -EIO;
	}

	ret = k_msgq_get(&data->rx_queue, buffer, SYS_TIMEOUT_MS(timeout));
	if (ret < 0) {
		return ret;
	}

	*size = data->block_size;

	return 0;
}

static int dmic_renesas_ra_init(const struct device *dev)
{
	const struct dmic_renesas_ra_config *cfg = dev->config;
	struct dmic_renesas_ra_data *data = dev->data;
	int ret;

	ret = clock_control_on(cfg->clock_dev, (clock_control_subsys_t)&cfg->clock_subsys);
	if (ret < 0) {
		LOG_ERR("failed to enable the PDM clock (%d)", ret);
		return ret;
	}

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("failed to configure the PDM pins (%d)", ret);
		return ret;
	}

	k_msgq_init(&data->rx_queue, data->rx_msgs, sizeof(void *),
		    CONFIG_AUDIO_DMIC_RENESAS_RA_QUEUE_SIZE);

	data->fsp_cfg.p_context = (void *)dev;

	cfg->irq_config_func();

	data->state = DMIC_STATE_INITIALIZED;

	return 0;
}

static DEVICE_API(dmic, dmic_renesas_ra_api) = {
	.configure = dmic_renesas_ra_configure,
	.trigger = dmic_renesas_ra_trigger,
	.read = dmic_renesas_ra_read,
};

void pdm_dat_isr(void);
void pdm_err_isr(void);

#define PDM_RA_EVENT_DAT(channel) BSP_PRV_IELS_ENUM(CONCAT(EVENT_PDM_DAT, channel))
#define PDM_RA_EVENT_ERR(channel) BSP_PRV_IELS_ENUM(CONCAT(EVENT_PDM_ERR, channel))

#define PDM_RA_IRQ_CONNECT(index, name, isr, event)                                                \
	R_ICU->IELSR[DT_INST_IRQ_BY_NAME(index, name, irq)] = event;                               \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, name, irq),                                         \
		    DT_INST_IRQ_BY_NAME(index, name, priority), isr, NULL, 0);                     \
	irq_enable(DT_INST_IRQ_BY_NAME(index, name, irq));

#define PDM_RA_ERR_IRQ_CONNECT(index)                                                              \
	IF_ENABLED(DT_INST_IRQ_HAS_NAME(index, err),                                               \
		   (PDM_RA_IRQ_CONNECT(index, err, pdm_err_isr,                                    \
				       PDM_RA_EVENT_ERR(DT_INST_PROP(index, channel)))))

#define PDM_RA_IRQ_OR_INVALID(index, name)                                                         \
	COND_CODE_1(DT_INST_IRQ_HAS_NAME(index, name), (DT_INST_IRQ_BY_NAME(index, name, irq)),    \
		    (FSP_INVALID_VECTOR))

#define PDM_RA_IPL_OR_DISABLED(index, name)                                                        \
	COND_CODE_1(DT_INST_IRQ_HAS_NAME(index, name),                                             \
		    (DT_INST_IRQ_BY_NAME(index, name, priority)), (BSP_IRQ_DISABLED))

#define PDM_RA_INIT(index)                                                                         \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
                                                                                                   \
	static uint32_t dmic_renesas_ra_fifo_buf_##index                                           \
		[CONFIG_AUDIO_DMIC_RENESAS_RA_FIFO_BUFFER_SAMPLES] __aligned(4);                   \
                                                                                                   \
	static char dmic_renesas_ra_rx_msgs_##index[CONFIG_AUDIO_DMIC_RENESAS_RA_QUEUE_SIZE *      \
						    sizeof(void *)];                               \
                                                                                                   \
	static void dmic_renesas_ra_irq_config_##index(void)                                       \
	{                                                                                          \
		PDM_RA_IRQ_CONNECT(index, dat, pdm_dat_isr,                                        \
				   PDM_RA_EVENT_DAT(DT_INST_PROP(index, channel)))                 \
		PDM_RA_ERR_IRQ_CONNECT(index)                                                      \
	}                                                                                          \
                                                                                                   \
	static const struct dmic_renesas_ra_config dmic_renesas_ra_config_##index = {              \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(index)),                            \
		.clock_subsys =                                                                    \
			{                                                                          \
				.mstp = (uint32_t)DT_INST_CLOCKS_CELL_BY_NAME(index, pclk, mstp),  \
				.stop_bit = DT_INST_CLOCKS_CELL_BY_NAME(index, pclk, stop_bit),    \
			},                                                                         \
		.irq_config_func = dmic_renesas_ra_irq_config_##index,                             \
		.fifo_buf = dmic_renesas_ra_fifo_buf_##index,                                      \
		.fifo_buf_samples = ARRAY_SIZE(dmic_renesas_ra_fifo_buf_##index),                  \
		.pcm_rate = DT_INST_PROP(index, clock_frequency),                                  \
		.settling_time_us = DT_INST_PROP(index, renesas_filter_settling_time_us) +         \
				    DT_INST_PROP(index, renesas_mic_startup_time_us),              \
	};                                                                                         \
                                                                                                   \
	static struct dmic_renesas_ra_data dmic_renesas_ra_data_##index = {                        \
		.rx_msgs = dmic_renesas_ra_rx_msgs_##index,                                        \
		.fsp_ext_cfg =                                                                     \
			{                                                                          \
				.clock_div = (DT_INST_PROP(index, renesas_clock_divider) / 2) - 1, \
				.buffer_overwrite_detection_enable =                               \
					PDM_BUFFER_OVERWRITE_DETECTION_ENABLED,                    \
				.sinc_filter_mode = DT_INST_PROP(index, renesas_sinc_order),       \
				.sincrng = DT_INST_PROP(index, renesas_sinc_range),                \
				.sincdec = DT_INST_PROP(index, renesas_sinc_decimation),           \
				.hpf_coefficient_s0 = PDM_RA_HPF_COEFFICIENT_S0,                   \
				.hpf_coefficient_k1 = PDM_RA_HPF_COEFFICIENT_K1,                   \
				.hpf_coefficient_h = PDM_RA_HPF_COEFFICIENT_H,                     \
				.compensation_filter_coefficient_h = PDM_RA_CF_COEFFICIENT_H,      \
				.lpf_coefficient_h0 = PDM_RA_LPF_COEFFICIENT_H0,                   \
				.lpf_coefficient_h1 = PDM_RA_LPF_COEFFICIENT_H1,                   \
				.interrupt_threshold = PDM_INTERRUPT_THRESHOLD_1,                  \
			},                                                                         \
		.fsp_cfg =                                                                         \
			{                                                                          \
				.channel = DT_INST_PROP(index, channel),                           \
				.pcm_edge = DT_INST_PROP(index, renesas_fall_edge_data)            \
						    ? PDM_INPUT_DATA_EDGE_FALL                     \
						    : PDM_INPUT_DATA_EDGE_RISE,                    \
				.p_callback = dmic_renesas_ra_callback,                            \
				.p_extend = &dmic_renesas_ra_data_##index.fsp_ext_cfg,             \
				.dat_irq = DT_INST_IRQ_BY_NAME(index, dat, irq),                   \
				.dat_ipl = DT_INST_IRQ_BY_NAME(index, dat, priority),              \
				.sdet_irq = FSP_INVALID_VECTOR,                                    \
				.sdet_ipl = BSP_IRQ_DISABLED,                                      \
				.err_irq = PDM_RA_IRQ_OR_INVALID(index, err),                      \
				.err_ipl = PDM_RA_IPL_OR_DISABLED(index, err),                     \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	BUILD_ASSERT(DT_INST_PROP(index, renesas_sinc_decimation) >= 1 &&                          \
			     DT_INST_PROP(index, renesas_sinc_decimation) <= 255,                  \
		     "renesas,sinc-decimation must be between 1 and 255");                         \
	BUILD_ASSERT(DT_INST_PROP(index, renesas_sinc_range) <= 31,                                \
		     "renesas,sinc-range must be at most 31");                                     \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, dmic_renesas_ra_init, NULL, &dmic_renesas_ra_data_##index,    \
			      &dmic_renesas_ra_config_##index, POST_KERNEL,                        \
			      CONFIG_AUDIO_DMIC_INIT_PRIORITY, &dmic_renesas_ra_api);

DT_INST_FOREACH_STATUS_OKAY(PDM_RA_INIT)

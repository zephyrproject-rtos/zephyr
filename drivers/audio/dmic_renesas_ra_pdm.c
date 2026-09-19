/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
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
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <r_pdm.h>
#include <soc.h>

LOG_MODULE_REGISTER(dmic_renesas_ra_pdm, CONFIG_AUDIO_DMIC_LOG_LEVEL);

/* PDM_CLKn = PDMIFCLK / D, with D even and between 2 and 32 (hardware manual Table 50.5) */
#define PDM_RA_CLK_DIV_MIN 2U
#define PDM_RA_CLK_DIV_MAX 32U

/*
 * Sinc filter decimation ratio M. Below 4 the hardware manual declares the filter's operation
 * not guaranteed, and the field holding M - 1 is eight bits wide.
 */
#define PDM_RA_DECIMATION_MIN 4U
#define PDM_RA_DECIMATION_MAX 256U

/* Largest departure from the requested rate, in thousandths, that is still taken */
#define PDM_RA_RATE_ERROR_MAX 50U

/*
 * The sinc filter output is clipped to twenty bits taken from bits [32 - SINCRNG : 14 - SINCRNG]
 * of the accumulator. Its most significant bit sits at ceil(order * log2(M)), which puts the
 * window over the whole result when SINCRNG is this value less that bit position. 0x1F is a
 * prohibited setting.
 */
#define PDM_RA_SINCRNG_BASE 33U
#define PDM_RA_SINCRNG_MAX  0x1EU

/*
 * The threshold is held as the base two logarithm of a sample count. The data buffer is 32
 * samples deep, so interrupting every 16 is the largest setting that can be serviced in time.
 */
#define PDM_RA_INT_THRESHOLD_MAX 4U

/*
 * R_PDM_Open() writes every coefficient register, so the whole filter chain is handed to it
 * from devicetree. The binding's defaults are the registers' reset values.
 */

struct dmic_ra_pdm_config {
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_dev;
	const struct clock_control_ra_subsys_cfg clock_subsys;
	void (*irq_config_func)(void);
	/* PDMIFCLK, which the clock generator derives from MOCO without a divider */
	uint32_t pdmif_clk_freq;
	/* Samples the capture buffer holds, twice the largest block that can be requested */
	uint32_t capture_samples;
	uint32_t *capture_buf;
	struct k_msgq *rx_queue;
	uint8_t sinc_filter_order;
};

struct dmic_ra_pdm_data {
	pdm_instance_ctrl_t fsp_ctrl;
	pdm_cfg_t fsp_cfg;
	pdm_extended_cfg_t fsp_ext_cfg;
	struct k_mem_slab *mem_slab;
	uint32_t block_size;
	uint32_t block_samples;
	uint8_t sample_bytes;
	uint8_t sign_bit;
	uint8_t next_half;
	enum dmic_state state;
	bool opened;
};

/* FSP interrupt handlers */
void pdm_dat_isr(void);
void pdm_err_isr(void);

/**
 * @brief Sign extend one sample of the PDM-IF data buffer.
 *
 * The data read register carries the sample right aligned with its sign bit at bit 19 in
 * twenty-bit mode and at bit 15 in sixteen-bit mode, and reads the bits above it as zero.
 */
static inline int32_t dmic_ra_pdm_sample(uint32_t word, uint8_t sign_bit)
{
	uint32_t raw = word & (BIT(sign_bit + 1U) - 1U);

	if ((raw & BIT(sign_bit)) != 0U) {
		return (int32_t)raw - (int32_t)BIT(sign_bit + 1U);
	}

	return (int32_t)raw;
}

static void dmic_ra_pdm_convert(const struct dmic_ra_pdm_data *data, const uint32_t *src,
				uint8_t *dst)
{
	for (uint32_t i = 0; i < data->block_samples; i++) {
		int32_t sample = dmic_ra_pdm_sample(src[i], data->sign_bit);

		switch (data->sample_bytes) {
		case 2:
			sys_put_le16((uint16_t)sample, &dst[i * 2U]);
			break;
		case 3:
			sys_put_le24((uint32_t)sample, &dst[i * 3U]);
			break;
		default:
			sys_put_le32((uint32_t)sample, &dst[i * 4U]);
			break;
		}
	}
}

static void dmic_ra_pdm_callback(pdm_callback_args_t *p_args)
{
	const struct device *dev = p_args->p_context;
	const struct dmic_ra_pdm_config *config = dev->config;
	struct dmic_ra_pdm_data *data = dev->data;
	const uint32_t *half;
	void *block;
	int ret;

	if (p_args->event == PDM_EVENT_ERROR) {
		LOG_ERR("Capture error %#x", (unsigned int)p_args->error);
		return;
	}

	if (p_args->event != PDM_EVENT_DATA) {
		return;
	}

	half = &config->capture_buf[data->next_half * data->block_samples];
	data->next_half ^= 1U;

	ret = k_mem_slab_alloc(data->mem_slab, &block, K_NO_WAIT);
	if (ret != 0) {
		LOG_WRN("No block to capture into, dropping %u samples", data->block_samples);
		return;
	}

	dmic_ra_pdm_convert(data, half, block);

	ret = k_msgq_put(config->rx_queue, &block, K_NO_WAIT);
	if (ret != 0) {
		LOG_WRN("Capture queue full, dropping %u samples", data->block_samples);
		k_mem_slab_free(data->mem_slab, block);
	}
}

/**
 * @brief Position of the sinc filter output's most significant bit.
 *
 * A sinc filter of the given order decimating by @p decimation has a gain of
 * decimation ^ order, and its result runs up to and including that value.
 * Checked against every row of the hardware manual's Table 50.7.
 */
static uint8_t dmic_ra_pdm_sinc_msb(uint8_t order, uint32_t decimation)
{
	uint64_t gain = 1U;
	uint8_t msb = 0;

	for (uint8_t i = 0; i < order; i++) {
		gain *= decimation;
	}

	while ((UINT64_C(1) << msb) <= gain) {
		msb++;
	}

	return msb;
}

/**
 * @brief Pick the microphone clock and the decimation ratio closest to a stream request.
 *
 * The output rate is PDM_CLKn / (2 * M), and only a few of the sixteen clock dividers reach a
 * given rate with a whole decimation ratio: the hardware manual's own table of sixteen
 * kilohertz settings misses it by up to a percent on most of them. The closest combination is
 * taken, and the fastest clock among equally close ones, because a PDM microphone's noise
 * floor improves with its clock.
 */
static int dmic_ra_pdm_select_clock(const struct device *dev, const struct dmic_cfg *cfg,
				    pdm_clk_div_t *clock_div, uint32_t *decimation,
				    uint32_t *actual)
{
	const struct dmic_ra_pdm_config *config = dev->config;
	uint32_t rate = cfg->streams[0].pcm_rate;
	uint32_t best_error = UINT32_MAX;

	for (uint32_t div = PDM_RA_CLK_DIV_MIN; div <= PDM_RA_CLK_DIV_MAX; div += 2U) {
		uint32_t clk = config->pdmif_clk_freq / div;
		uint32_t m = DIV_ROUND_CLOSEST(clk, 2U * rate);
		uint32_t out;
		uint32_t error;

		if (clk < cfg->io.min_pdm_clk_freq || clk > cfg->io.max_pdm_clk_freq) {
			continue;
		}

		if (m < PDM_RA_DECIMATION_MIN || m > PDM_RA_DECIMATION_MAX) {
			continue;
		}

		/* The other filters are only guaranteed to run when D * M is above 12 */
		if ((div * m) <= 12U) {
			continue;
		}

		out = clk / (2U * m);
		error = (out > rate) ? (out - rate) : (rate - out);
		error = (uint32_t)(((uint64_t)error * 1000U) / rate);

		if (error < best_error) {
			best_error = error;
			*clock_div = (pdm_clk_div_t)((div / 2U) - 1U);
			*decimation = m;
			*actual = out;
		}
	}

	if (best_error > PDM_RA_RATE_ERROR_MAX) {
		return -EINVAL;
	}

	return 0;
}

static int dmic_ra_pdm_configure(const struct device *dev, struct dmic_cfg *cfg)
{
	const struct dmic_ra_pdm_config *config = dev->config;
	struct dmic_ra_pdm_data *data = dev->data;
	struct pcm_stream_cfg *stream = &cfg->streams[0];
	pdm_clk_div_t clock_div;
	uint32_t decimation;
	uint32_t actual_rate;
	uint32_t threshold;
	uint8_t msb;
	fsp_err_t fsp_err;
	int ret;

	if (data->state == DMIC_STATE_ACTIVE) {
		LOG_ERR("Cannot reconfigure a running capture");
		return -EBUSY;
	}

	if (cfg->channel.req_num_streams != 1U || cfg->channel.req_num_chan != 1U) {
		LOG_ERR("One channel of one stream is supported, %u of %u requested",
			cfg->channel.req_num_chan, cfg->channel.req_num_streams);
		return -EINVAL;
	}

	if (stream->pcm_rate == 0U) {
		LOG_ERR("A stream needs a rate");
		return -EINVAL;
	}

	if (stream->mem_slab == NULL || stream->block_size == 0U) {
		LOG_ERR("A stream needs a memory slab and a block size");
		return -EINVAL;
	}

	if (stream->gain_db != 0) {
		LOG_ERR("The PDM-IF has no gain stage");
		return -EINVAL;
	}

	/*
	 * Sixteen-bit mode clips the sample in hardware to the top sixteen bits of the filter
	 * output; the wider widths carry the twenty bits the filter produces as they are.
	 */
	switch (stream->pcm_width) {
	case 16:
		data->fsp_cfg.pcm_width = PDM_PCM_WIDTH_16_BITS_4_18;
		data->sign_bit = 15U;
		data->sample_bytes = 2U;
		break;
	case 24:
		data->fsp_cfg.pcm_width = PDM_PCM_WIDTH_20_BITS_0_18;
		data->sign_bit = 19U;
		data->sample_bytes = 3U;
		break;
	case 32:
		data->fsp_cfg.pcm_width = PDM_PCM_WIDTH_20_BITS_0_18;
		data->sign_bit = 19U;
		data->sample_bytes = 4U;
		break;
	default:
		LOG_ERR("Sample width %u is not supported", stream->pcm_width);
		return -EINVAL;
	}

	if ((stream->block_size % data->sample_bytes) != 0U) {
		LOG_ERR("Block of %u bytes does not hold whole samples", stream->block_size);
		return -EINVAL;
	}

	data->block_samples = stream->block_size / data->sample_bytes;

	/* One half of the capture buffer is filled while the other is converted */
	if ((2U * data->block_samples) > config->capture_samples) {
		LOG_ERR("Block of %u samples exceeds the %u the capture buffer holds",
			data->block_samples, config->capture_samples / 2U);
		return -EINVAL;
	}

	ret = dmic_ra_pdm_select_clock(dev, cfg, &clock_div, &decimation, &actual_rate);
	if (ret != 0) {
		LOG_ERR("No clock divider reaches %u Hz within %u to %u Hz", stream->pcm_rate,
			cfg->io.min_pdm_clk_freq, cfg->io.max_pdm_clk_freq);
		return ret;
	}

	if (actual_rate != stream->pcm_rate) {
		LOG_WRN("Capturing at %u Hz rather than the %u Hz asked for", actual_rate,
			stream->pcm_rate);
	}

	/*
	 * The data reception interrupt fires once per threshold samples, so the threshold has to
	 * divide a block for the callback to land on a block boundary.
	 */
	for (threshold = PDM_RA_INT_THRESHOLD_MAX; threshold > 0U; threshold--) {
		if ((data->block_samples % BIT(threshold)) == 0U) {
			break;
		}
	}

	msb = dmic_ra_pdm_sinc_msb(config->sinc_filter_order, decimation);
	if (msb >= PDM_RA_SINCRNG_BASE) {
		LOG_ERR("Decimation by %u overflows the sinc filter", decimation);
		return -EINVAL;
	}

	/* The channel has to be closed for its filter settings to be taken again */
	if (data->opened) {
		(void)R_PDM_Close(&data->fsp_ctrl);
		data->opened = false;
		data->state = DMIC_STATE_INITIALIZED;
	}

	data->fsp_ext_cfg.clock_div = clock_div;
	data->fsp_ext_cfg.sincdec = (uint8_t)(decimation - 1U);
	data->fsp_ext_cfg.sincrng = MIN(PDM_RA_SINCRNG_BASE - msb, PDM_RA_SINCRNG_MAX);
	data->fsp_ext_cfg.interrupt_threshold = (pdm_interrupt_threshold_t)threshold;

	fsp_err = R_PDM_Open(&data->fsp_ctrl, &data->fsp_cfg);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("Failed to open the PDM channel (%d)", fsp_err);
		return -EIO;
	}

	data->opened = true;
	data->block_size = stream->block_size;
	data->mem_slab = stream->mem_slab;
	data->state = DMIC_STATE_CONFIGURED;

	cfg->channel.act_num_streams = 1U;
	cfg->channel.act_num_chan = 1U;
	cfg->channel.act_chan_map_lo = cfg->channel.req_chan_map_lo;
	cfg->channel.act_chan_map_hi = cfg->channel.req_chan_map_hi;

	LOG_DBG("%u Hz from a %u Hz microphone clock decimated by %u", actual_rate,
		config->pdmif_clk_freq / ((clock_div + 1U) * 2U), decimation);

	return 0;
}

static void dmic_ra_pdm_purge(const struct device *dev)
{
	const struct dmic_ra_pdm_config *config = dev->config;
	struct dmic_ra_pdm_data *data = dev->data;
	void *block;

	while (k_msgq_get(config->rx_queue, &block, K_NO_WAIT) == 0) {
		k_mem_slab_free(data->mem_slab, block);
	}
}

static int dmic_ra_pdm_start(const struct device *dev)
{
	const struct dmic_ra_pdm_config *config = dev->config;
	struct dmic_ra_pdm_data *data = dev->data;
	fsp_err_t fsp_err;

	data->next_half = 0U;

	fsp_err = R_PDM_Start(&data->fsp_ctrl, config->capture_buf,
			      2U * data->block_samples * sizeof(uint32_t), data->block_samples);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("Failed to start the capture (%d)", fsp_err);
		return -EIO;
	}

	data->state = DMIC_STATE_ACTIVE;

	return 0;
}

static int dmic_ra_pdm_stop(const struct device *dev)
{
	struct dmic_ra_pdm_data *data = dev->data;
	fsp_err_t fsp_err;

	fsp_err = R_PDM_Stop(&data->fsp_ctrl);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("Failed to stop the capture (%d)", fsp_err);
		return -EIO;
	}

	dmic_ra_pdm_purge(dev);
	data->state = DMIC_STATE_CONFIGURED;

	return 0;
}

static int dmic_ra_pdm_trigger(const struct device *dev, enum dmic_trigger cmd)
{
	struct dmic_ra_pdm_data *data = dev->data;

	switch (cmd) {
	case DMIC_TRIGGER_START:
	case DMIC_TRIGGER_RELEASE:
		if (data->state == DMIC_STATE_ACTIVE) {
			return 0;
		}

		if (data->state != DMIC_STATE_CONFIGURED && data->state != DMIC_STATE_PAUSED) {
			LOG_ERR("Capture has not been configured");
			return -EIO;
		}

		return dmic_ra_pdm_start(dev);
	case DMIC_TRIGGER_STOP:
	case DMIC_TRIGGER_PAUSE:
		if (data->state != DMIC_STATE_ACTIVE) {
			return 0;
		}

		return dmic_ra_pdm_stop(dev);
	case DMIC_TRIGGER_RESET:
		if (data->state == DMIC_STATE_ACTIVE) {
			int ret = dmic_ra_pdm_stop(dev);

			if (ret != 0) {
				return ret;
			}
		}

		if (data->opened) {
			(void)R_PDM_Close(&data->fsp_ctrl);
			data->opened = false;
		}

		data->state = DMIC_STATE_INITIALIZED;

		return 0;
	default:
		LOG_ERR("Invalid trigger %d", cmd);
		return -EINVAL;
	}
}

static int dmic_ra_pdm_read(const struct device *dev, uint8_t stream, void **buffer, size_t *size,
			    int32_t timeout)
{
	const struct dmic_ra_pdm_config *config = dev->config;
	struct dmic_ra_pdm_data *data = dev->data;
	int ret;

	ARG_UNUSED(stream);

	if (data->state != DMIC_STATE_ACTIVE) {
		LOG_ERR("Capture is not running");
		return -EIO;
	}

	ret = k_msgq_get(config->rx_queue, buffer, SYS_TIMEOUT_MS(timeout));
	if (ret != 0) {
		return ret;
	}

	*size = data->block_size;

	return 0;
}

static int dmic_ra_pdm_init(const struct device *dev)
{
	const struct dmic_ra_pdm_config *config = dev->config;
	struct dmic_ra_pdm_data *data = dev->data;
	int ret;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("Clock controller is not ready");
		return -ENODEV;
	}

	ret = clock_control_on(config->clock_dev, (clock_control_subsys_t)&config->clock_subsys);
	if (ret < 0) {
		LOG_ERR("Failed to start the PDM-IF clock (%d)", ret);
		return ret;
	}

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Failed to configure the PDM pins (%d)", ret);
		return ret;
	}

	config->irq_config_func();
	data->state = DMIC_STATE_INITIALIZED;

	return 0;
}

static DEVICE_API(dmic, dmic_ra_pdm_driver_api) = {
	.configure = dmic_ra_pdm_configure,
	.trigger = dmic_ra_pdm_trigger,
	.read = dmic_ra_pdm_read,
};

#define EVENT_PDM_DAT(channel) BSP_PRV_IELS_ENUM(CONCAT(EVENT_PDM_DAT, channel))
#define EVENT_PDM_ERR(channel) BSP_PRV_IELS_ENUM(CONCAT(EVENT_PDM_ERR, channel))

#define DMIC_RA_PDM_INIT(index)                                                                    \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
                                                                                                   \
	static void dmic_ra_pdm_irq_config_##index(void)                                           \
	{                                                                                          \
		R_ICU->IELSR[DT_INST_IRQ_BY_NAME(index, dat, irq)] =                               \
			EVENT_PDM_DAT(DT_INST_PROP(index, channel));                               \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, dat, irq),                                  \
			    DT_INST_IRQ_BY_NAME(index, dat, priority), pdm_dat_isr, NULL, 0);      \
                                                                                                   \
		R_ICU->IELSR[DT_INST_IRQ_BY_NAME(index, err, irq)] =                               \
			EVENT_PDM_ERR(DT_INST_PROP(index, channel));                               \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, err, irq),                                  \
			    DT_INST_IRQ_BY_NAME(index, err, priority), pdm_err_isr, NULL, 0);      \
	}                                                                                          \
                                                                                                   \
	static uint32_t                                                                            \
		dmic_ra_pdm_capture_##index[2 * CONFIG_AUDIO_DMIC_RENESAS_RA_PDM_BLOCK_SAMPLES];   \
	K_MSGQ_DEFINE(dmic_ra_pdm_queue_##index, sizeof(void *),                                   \
		      CONFIG_AUDIO_DMIC_RENESAS_RA_PDM_BLOCK_COUNT, sizeof(void *));               \
                                                                                                   \
	static const struct dmic_ra_pdm_config dmic_ra_pdm_config_##index = {                      \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(index, pclk)),              \
		.clock_subsys = {.mstp = (uint32_t)DT_INST_CLOCKS_CELL_BY_NAME(index, pclk, mstp), \
				 .stop_bit = DT_INST_CLOCKS_CELL_BY_NAME(index, pclk, stop_bit)},  \
		.irq_config_func = dmic_ra_pdm_irq_config_##index,                                 \
		.pdmif_clk_freq =                                                                  \
			DT_PROP_BY_PHANDLE_IDX(DT_DRV_INST(index), clocks, 1, clock_frequency),    \
		.capture_samples = ARRAY_SIZE(dmic_ra_pdm_capture_##index),                        \
		.capture_buf = dmic_ra_pdm_capture_##index,                                        \
		.rx_queue = &dmic_ra_pdm_queue_##index,                                            \
		.sinc_filter_order = DT_INST_PROP(index, sinc_filter_order),               \
	};                                                                                         \
                                                                                                   \
	static struct dmic_ra_pdm_data dmic_ra_pdm_data_##index = {                                \
		.fsp_cfg =                                                                         \
			{                                                                          \
				.unit = 0,                                                         \
				.channel = DT_INST_PROP(index, channel),                           \
				.pcm_edge = DT_INST_PROP(index, falling_edge)              \
						    ? PDM_INPUT_DATA_EDGE_FALL                     \
						    : PDM_INPUT_DATA_EDGE_RISE,                    \
				.p_transfer_rx = NULL,                                             \
				.p_callback = dmic_ra_pdm_callback,                                \
				.p_context = (void *)DEVICE_DT_INST_GET(index),                    \
				.p_extend = &dmic_ra_pdm_data_##index.fsp_ext_cfg,                 \
				.dat_ipl = DT_INST_IRQ_BY_NAME(index, dat, priority),              \
				.sdet_ipl = BSP_IRQ_DISABLED,                                      \
				.err_ipl = DT_INST_IRQ_BY_NAME(index, err, priority),              \
				.dat_irq = DT_INST_IRQ_BY_NAME(index, dat, irq),                   \
				.sdet_irq = FSP_INVALID_VECTOR,                                    \
				.err_irq = DT_INST_IRQ_BY_NAME(index, err, irq),                   \
			},                                                                         \
		.fsp_ext_cfg =                                                                     \
			{                                                                          \
				.moving_average_mode = PDM_MOVING_AVERAGE_MODE_1_ORDER,            \
				.low_pass_filter_shift = (pdm_low_pass_filter_shift_t)             \
					DT_INST_PROP(index, lpf_shift),                            \
				.compensation_filter_shift =                                       \
					(pdm_compensation_filter_shift_t)DT_INST_PROP(             \
						index, compensation_shift),                        \
				.high_pass_filter_shift = (pdm_high_pass_filter_shift_t)           \
					DT_INST_PROP(index, hpf_shift),                            \
				.sinc_filter_mode = (pdm_sinc_filter_mode_t)DT_INST_PROP(          \
					index, sinc_filter_order),                         \
				.hpf_coefficient_s0 = DT_INST_PROP(index, hpf_coefficient_s0),     \
				.hpf_coefficient_k1 = DT_INST_PROP(index, hpf_coefficient_k1),     \
				.hpf_coefficient_h =                                               \
					DT_INST_PROP(index, hpf_coefficients_h),            \
				.compensation_filter_coefficient_h =                               \
					DT_INST_PROP(index, compensation_coefficients),     \
				.lpf_coefficient_h0 = DT_INST_PROP(index, lpf_coefficient_h0),     \
				.lpf_coefficient_h1 =                                              \
					DT_INST_PROP(index, lpf_coefficients_h1),           \
				.short_circuit_detection_enable = PDM_SHORT_CIRCUIT_DISABLED,      \
				.over_voltage_lower_limit_detection_enable =                       \
					PDM_OVERVOLTAGE_LOWER_LIMIT_DISABLED,                      \
				.over_voltage_upper_limit_detection_enable =                       \
					PDM_OVERVOLTAGE_UPPER_LIMIT_DISABLED,                      \
				.buffer_overwrite_detection_enable =                               \
					PDM_BUFFER_OVERWRITE_DETECTION_ENABLED,                    \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, dmic_ra_pdm_init, NULL, &dmic_ra_pdm_data_##index,            \
			      &dmic_ra_pdm_config_##index, POST_KERNEL,                            \
			      CONFIG_AUDIO_DMIC_INIT_PRIORITY, &dmic_ra_pdm_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DMIC_RA_PDM_INIT)

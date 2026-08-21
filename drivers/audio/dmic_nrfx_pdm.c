/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_pdm

#include <zephyr/audio/dmic.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/clock/nrf-auxpll.h>
#include <soc.h>
#include <dmm.h>
#include <nrfx_pdm.h>

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(dmic_nrfx_pdm, CONFIG_AUDIO_DMIC_LOG_LEVEL);

#define NODE_AUDIO_AUXPLL DT_NODELABEL(audio_auxpll)

#define DMIC_NRFX_CLOCK_FREQ MHZ(32)

#if defined(CONFIG_SOC_SERIES_NRF54H) || defined(CONFIG_SOC_SERIES_NRF92)
#undef DMIC_NRFX_CLOCK_FREQ
#define DMIC_NRFX_CLOCK_FREQ       MHZ(16)
#define DMIC_NRFX_AUDIO_CLOCK_FREQ DT_PROP_OR(DT_NODELABEL(audiopll), frequency, 0)
#define AUDIO_ASSERT_MSG                                                                           \
	"Clock source ACLK requires frequency property to be set in the audiopll node."
#define AUDIO_FREQUENCY_DEFINED DT_NODE_HAS_PROP(DT_NODELABEL(audiopll), frequency)

#elif DT_NODE_HAS_STATUS_OKAY(NODE_AUDIO_AUXPLL)
#define AUXPLL_FREQUENCY_SETTING DT_PROP(NODE_AUDIO_AUXPLL, nordic_frequency)
BUILD_ASSERT((AUXPLL_FREQUENCY_SETTING == NRF_AUXPLL_FREQ_DIV_AUDIO_48K) ||
		     (AUXPLL_FREQUENCY_SETTING == NRF_AUXPLL_FREQ_DIV_AUDIO_44K1),
	     "Unsupported Audio AUXPLL frequency selection for PDM");

#define DMIC_NRFX_AUDIO_CLOCK_FREQ CLOCK_CONTROL_NRF_AUXPLL_GET_FREQ(NODE_AUDIO_AUXPLL)
#define AUDIO_ASSERT_MSG                                                                           \
	"Clock source ACLK requires nordic_frequency property to be set in the audio_auxpll node."
#define AUDIO_FREQUENCY_DEFINED DT_NODE_HAS_PROP(DT_NODELABEL(audio_auxpll), nordic_frequency)

#elif CONFIG_CLOCK_CONTROL_NRF
#define DMIC_NRFX_AUDIO_CLOCK_FREQ                                                                 \
	DT_PROP_OR(DT_NODELABEL(aclk), clock_frequency,                                            \
		   DT_PROP_OR(DT_NODELABEL(clock), hfclkaudio_frequency, 0))
#define AUDIO_ASSERT_MSG                                                                           \
	"Clock source ACLK requires hfclkaudio_frequency property to be set in the clock node"     \
	"or clock_frequency property to be set in the aclk node."
#define AUDIO_FREQUENCY_DEFINED                                                                    \
	(DT_NODE_HAS_PROP(DT_NODELABEL(clock), hfclkaudio_frequency) ||                            \
	 DT_NODE_HAS_PROP(DT_NODELABEL(aclk), clock_frequency))

#elif defined(CONFIG_CLOCK_CONTROL_NRF_HFCLKAUDIO)
#define DMIC_NRFX_AUDIO_CLOCK_FREQ                                                                 \
	DT_PROP_OR(DT_NODELABEL(aclk), clock_frequency,                                            \
		   DT_PROP_OR(DT_COMPAT_GET_ANY_STATUS_OKAY(nordic_nrf_clock_hfclkaudio),          \
			      hfclkaudio_frequency, 0))
#define AUDIO_ASSERT_MSG                                                                           \
	"Clock source ACLK requires hfclkaudio_frequency property to be set in the hfclkaudio "    \
	"node."
#define AUDIO_FREQUENCY_DEFINED                                                                    \
	DT_NODE_HAS_PROP(DT_COMPAT_GET_ANY_STATUS_OKAY(nordic_nrf_clock_hfclkaudio),               \
			 hfclkaudio_frequency)

#elif defined(CONFIG_CLOCK_CONTROL_NRF_XO24M)
#define DMIC_NRFX_AUDIO_CLOCK_FREQ                                                                 \
	DT_PROP_OR(DT_NODELABEL(aclk), clock_frequency,                                            \
		   DT_PROP_OR(DT_COMPAT_GET_ANY_STATUS_OKAY(nordic_nrf_clock_xo24m),               \
			      clock_frequency, 0))
#define AUDIO_ASSERT_MSG                                                                           \
	"Clock source ACLK requires clock_frequency property to be set in the xo24m node."
#define AUDIO_FREQUENCY_DEFINED                                                                    \
	DT_NODE_HAS_PROP(DT_COMPAT_GET_ANY_STATUS_OKAY(nordic_nrf_clock_xo24m), clock_frequency)

#elif defined(CONFIG_CLOCK_CONTROL_NRF_XO)
#define DMIC_NRFX_AUDIO_CLOCK_FREQ                                                                 \
	DT_PROP_OR(DT_NODELABEL(aclk), clock_frequency,                                            \
		   DT_PROP_OR(DT_COMPAT_GET_ANY_STATUS_OKAY(nordic_nrf_clock_xo), clock_frequency, \
			      0))
#define AUDIO_ASSERT_MSG                                                                           \
	"Clock source ACLK requires clock_frequency property to be set in the xo node."
#define AUDIO_FREQUENCY_DEFINED                                                                    \
	DT_NODE_HAS_PROP(DT_COMPAT_GET_ANY_STATUS_OKAY(nordic_nrf_clock_xo), clock_frequency)

#else
#define DMIC_NRFX_AUDIO_CLOCK_FREQ 0
#define AUDIO_ASSERT_MSG           "ACLK clock source not available use another clock source."
#define AUDIO_FREQUENCY_DEFINED    0

#endif

/* Allow using custom ratio only if its supported by hardware.
 * Also, provided mechanism of calculating custom ratio assumes
 * presence of prescaler mechanism in PDM.
 */
#if CONFIG_AUDIO_DMIC_NRFX_PDM_CUSTOM_RATIO && NRF_PDM_HAS_CUSTOM_RATIO && NRF_PDM_HAS_PRESCALER
#define USE_CUSTOM_RATIO 1
#else
#define USE_CUSTOM_RATIO 0
#endif

struct dmic_nrfx_pdm_drv_data {
	nrfx_pdm_t pdm;
#if CONFIG_CLOCK_CONTROL_NRFS_AUDIOPLL || DT_NODE_HAS_STATUS_OKAY(NODE_AUDIO_AUXPLL)
	const struct device *audiopll_dev;
#elif CONFIG_CLOCK_CONTROL_NRF
	struct onoff_manager *clk_mgr;
#elif defined(CONFIG_CLOCK_CONTROL_NRF_HFCLKAUDIO) || defined(CONFIG_CLOCK_CONTROL_NRF_HFCLK) ||   \
	defined(CONFIG_CLOCK_CONTROL_NRF_XO) || defined(CONFIG_CLOCK_CONTROL_NRF_XO24M)
	const struct device *clk_dev;
#endif
	struct onoff_client clk_cli;
	struct k_mem_slab *mem_slab;
	uint32_t block_size;
	struct k_msgq mem_slab_queue;
	struct k_msgq rx_queue;
	bool request_clock : 1;
	bool configured    : 1;
	volatile bool active;
	volatile bool stopping;
};

struct dmic_nrfx_pdm_drv_cfg {
	nrfx_pdm_event_handler_t event_handler;
	nrfx_pdm_config_t nrfx_def_cfg;
	const struct pinctrl_dev_config *pcfg;
	enum clock_source {
		PCLK32M,
		PCLK32M_HFXO,
		ACLK
	} clk_src;
	void *mem_reg;
};

static void free_buffer(struct dmic_nrfx_pdm_drv_data *drv_data, void *buffer)
{
	k_mem_slab_free(drv_data->mem_slab, buffer);
	LOG_DBG("Freed buffer %p", buffer);
}

static void stop_pdm(struct dmic_nrfx_pdm_drv_data *drv_data)
{
	drv_data->stopping = true;
	nrfx_pdm_stop(&drv_data->pdm);
}

static int request_clock(struct dmic_nrfx_pdm_drv_data *drv_data)
{
	if (!drv_data->request_clock) {
		return 0;
	}
#if CONFIG_CLOCK_CONTROL_NRFS_AUDIOPLL || DT_NODE_HAS_STATUS_OKAY(NODE_AUDIO_AUXPLL)
	return nrf_clock_control_request(drv_data->audiopll_dev, NULL, &drv_data->clk_cli);
#elif CONFIG_CLOCK_CONTROL_NRF
	return onoff_request(drv_data->clk_mgr, &drv_data->clk_cli);
#elif defined(CONFIG_CLOCK_CONTROL_NRF_HFCLKAUDIO) || defined(CONFIG_CLOCK_CONTROL_NRF_HFCLK) ||   \
	defined(CONFIG_CLOCK_CONTROL_NRF_XO) || defined(CONFIG_CLOCK_CONTROL_NRF_XO24M)
	return nrf_clock_control_request(drv_data->clk_dev, NULL, &drv_data->clk_cli);
#else
	return -ENOTSUP;
#endif
}

static int release_clock(struct dmic_nrfx_pdm_drv_data *drv_data)
{
	if (!drv_data->request_clock) {
		return 0;
	}
#if CONFIG_CLOCK_CONTROL_NRFS_AUDIOPLL || DT_NODE_HAS_STATUS_OKAY(NODE_AUDIO_AUXPLL)
	return nrf_clock_control_release(drv_data->audiopll_dev, NULL);
#elif CONFIG_CLOCK_CONTROL_NRF
	return onoff_release(drv_data->clk_mgr);
#elif defined(CONFIG_CLOCK_CONTROL_NRF_HFCLKAUDIO) || defined(CONFIG_CLOCK_CONTROL_NRF_HFCLK) ||   \
	defined(CONFIG_CLOCK_CONTROL_NRF_XO) || defined(CONFIG_CLOCK_CONTROL_NRF_XO24M)
	return nrf_clock_control_release(drv_data->clk_dev, NULL);
#else
	return -ENOTSUP;
#endif
}

static void event_handler(const struct device *dev, const nrfx_pdm_evt_t *evt)
{
	struct dmic_nrfx_pdm_drv_data *drv_data = dev->data;
	const struct dmic_nrfx_pdm_drv_cfg *drv_cfg = dev->config;
	int ret;
	bool stop = false;
	void *mem_slab_buffer;

	if (evt->buffer_requested) {
		void *buffer;
		int err;

		ret = k_mem_slab_alloc(drv_data->mem_slab, &mem_slab_buffer, K_NO_WAIT);
		if (ret < 0) {
			LOG_ERR("Failed to allocate buffer: %d", ret);
			stop = true;
		} else {
			ret = dmm_buffer_in_prepare(drv_cfg->mem_reg, mem_slab_buffer,
						    drv_data->block_size, &buffer);
			if (ret < 0) {
				LOG_ERR("Failed to prepare buffer: %d", ret);
				free_buffer(drv_data, mem_slab_buffer);
				stop_pdm(drv_data);
				return;
			}
			ret = k_msgq_put(&drv_data->mem_slab_queue, &mem_slab_buffer, K_NO_WAIT);
			if (ret < 0) {
				LOG_ERR("Unable to put mem slab in queue");
				free_buffer(drv_data, mem_slab_buffer);
				stop_pdm(drv_data);
				return;
			}
			err = nrfx_pdm_buffer_set(&drv_data->pdm, buffer, drv_data->block_size / 2);
			if (err != 0) {
				LOG_ERR("Failed to set buffer: %d", err);
				stop = true;
			}
		}
	}

	if (drv_data->stopping) {
		if (evt->buffer_released) {
			ret = k_msgq_get(&drv_data->mem_slab_queue, &mem_slab_buffer, K_NO_WAIT);
			if (ret < 0) {
				LOG_ERR("No buffers to free");
				return;
			}
			ret = dmm_buffer_in_release(drv_cfg->mem_reg, mem_slab_buffer,
						    drv_data->block_size, evt->buffer_released);
			if (ret < 0) {
				LOG_ERR("Failed to release buffer: %d", ret);
				free_buffer(drv_data, mem_slab_buffer);
				return;
			}
			free_buffer(drv_data, mem_slab_buffer);
		}

		if (drv_data->active) {
			drv_data->active = false;
			ret = release_clock(drv_data);
			if (ret < 0) {
				LOG_ERR("Failed to release clock: %d", ret);
				return;
			}
		}
	} else if (evt->buffer_released) {
		ret = k_msgq_get(&drv_data->mem_slab_queue, &mem_slab_buffer, K_NO_WAIT);
		if (ret < 0) {
			LOG_ERR("No buffers to free");
			stop_pdm(drv_data);
			return;
		}
		ret = dmm_buffer_in_release(drv_cfg->mem_reg, mem_slab_buffer,
					    drv_data->block_size, evt->buffer_released);
		if (ret < 0) {
			LOG_ERR("Failed to release buffer: %d", ret);
			free_buffer(drv_data, mem_slab_buffer);
			stop_pdm(drv_data);
			return;
		}
		ret = k_msgq_put(&drv_data->rx_queue,
				 &mem_slab_buffer,
				 K_NO_WAIT);
		if (ret < 0) {
			LOG_ERR("No room in RX queue");
			stop = true;
			free_buffer(drv_data, mem_slab_buffer);
		} else {
			LOG_DBG("Queued buffer %p", evt->buffer_released);
		}
	}
	if (stop) {
		stop_pdm(drv_data);
	}
}

#if USE_CUSTOM_RATIO
#include <math.h>

#define CUSTOM_RATIO_MIN (2 * (PDM_FILTER_CTRL_DECRATIO_Min + 1))
#define CUSTOM_RATIO_MAX (2 * (PDM_FILTER_CTRL_DECRATIO_Max + 1))

static bool is_better(nrfx_pdm_output_t const *output_config,
		      uint16_t ratio, uint16_t prescaler,
		      uint32_t *best_diff)
{
	if (ratio < CUSTOM_RATIO_MIN || ratio > CUSTOM_RATIO_MAX) {
		return false;
	}

	uint32_t actual_rate = output_config->base_clock_freq / prescaler / ratio;
	uint32_t diff = actual_rate >= output_config->sampling_rate ?
			actual_rate - output_config->sampling_rate :
			output_config->sampling_rate - actual_rate;

	if (diff < *best_diff) {
		*best_diff = diff;
		return true;
	}

	return false;
}

static int custom_ratio_calculate(nrfx_pdm_output_t const *output_config,
				  nrfx_pdm_prescalers_t *prescalers)
{
	uint16_t ratio;
	uint32_t best_diff = UINT32_MAX;
	uint32_t total_ratio = output_config->base_clock_freq / output_config->sampling_rate;
	uint16_t prescaler_min = output_config->base_clock_freq /
				 output_config->output_freq_max + 1;
	uint16_t prescaler_max = output_config->base_clock_freq / output_config->output_freq_min;

	for (uint16_t prescaler = prescaler_min; prescaler < prescaler_max; prescaler++) {
		ratio = total_ratio / prescaler;

		/* Ratio rounded down. */
		if (is_better(output_config, ratio, prescaler, &best_diff)) {
			prescalers->prescaler = prescaler;
			prescalers->custom_ratio.ratio = ratio;

			if (best_diff == 0) {
				break;
			}
		}

		/* Ratio rounded up. */
		if (is_better(output_config, ratio++, prescaler, &best_diff)) {
			prescalers->prescaler = prescaler;
			prescalers->custom_ratio.ratio = ratio;

			if (best_diff == 0) {
				break;
			}
		}
	}

	if (best_diff == UINT32_MAX) {
		return -EINVAL;
	}

	/* Use selected ratio for filter configuration calculations. */
	ratio = prescalers->custom_ratio.ratio;

	static const struct
	{
		nrf_pdm_filter_cic_t cic_enum;
		uint16_t cic_high;

	} cics[] = {
		{ NRF_PDM_FILTER_CIC_MSB_RANGE_0,  32  },
		{ NRF_PDM_FILTER_CIC_MSB_RANGE_1,  36  },
		{ NRF_PDM_FILTER_CIC_MSB_RANGE_2,  42  },
		{ NRF_PDM_FILTER_CIC_MSB_RANGE_3,  48  },
		{ NRF_PDM_FILTER_CIC_MSB_RANGE_4,  54  },
		{ NRF_PDM_FILTER_CIC_MSB_RANGE_5,  64  },
		{ NRF_PDM_FILTER_CIC_MSB_RANGE_6,  72  },
		{ NRF_PDM_FILTER_CIC_MSB_RANGE_7,  84  },
		{ NRF_PDM_FILTER_CIC_MSB_RANGE_8,  96  },
		{ NRF_PDM_FILTER_CIC_MSB_RANGE_9,  110 },
		{ NRF_PDM_FILTER_CIC_MSB_RANGE_10, 128 },
		{ NRF_PDM_FILTER_CIC_MSB_RANGE_11, 146 },
		{ NRF_PDM_FILTER_CIC_MSB_RANGE_12, 168 },
		{ NRF_PDM_FILTER_CIC_MSB_RANGE_13, 194 },
		{ NRF_PDM_FILTER_CIC_MSB_RANGE_14, 222 },
		{ NRF_PDM_FILTER_CIC_MSB_RANGE_15, 256 }
	};

	for (int i = 0; i < ARRAY_SIZE(cics); i++) {
		if (ratio <= cics[i].cic_high) {
			prescalers->custom_ratio.filter_msb = cics[i].cic_enum;
			break;
		}
	}

	/* Compensation gain needs to be set using 0.5 dB steps with additional 0.25 step.
	 * To calculate it, following formula has to be used :
	 * gainCIC = OSR^5, where OSR is Oversampling Ratio
	 * closePow2 - floor(log2(gainCIC)) + 1, which is an equivalent of LOG2CEIL(gainCIC)
	 * gainComp = -20 * log10(gainCIC / (2^closePow2)) dB
	 */
	uint64_t gain_cic = (uint64_t)ratio * ratio * ratio * ratio * ratio;
	uint8_t close_pow2 = LOG2CEIL(gain_cic);
	double gain_comp = -20.0 * log10((double)gain_cic / (1ULL << close_pow2));
	uint8_t steps = gain_comp * 4;

	prescalers->custom_ratio.compensation_gain = steps / 2;
	prescalers->custom_ratio.minor_compensation_gain = steps % 2;
	prescalers->ratio = NRF_PDM_RATIO_CUSTOM;

	return 0;
}
#endif

static int dmic_nrfx_pdm_configure(const struct device *dev,
				   struct dmic_cfg *config)
{
	struct dmic_nrfx_pdm_drv_data *drv_data = dev->data;
	const struct dmic_nrfx_pdm_drv_cfg *drv_cfg = dev->config;
	struct pdm_chan_cfg *channel = &config->channel;
	struct pcm_stream_cfg *stream = &config->streams[0];
	uint32_t def_map, alt_map;
	nrfx_pdm_config_t nrfx_cfg;
	int8_t gain_limit;
	int err;

	if (drv_data->active) {
		LOG_ERR("Cannot configure device while it is active");
		return -EBUSY;
	}

	/*
	 * This device supports only one stream and can be configured to return
	 * 16-bit samples for two channels (Left+Right samples) or one channel
	 * (only Left samples). Left and Right samples can be optionally swapped
	 * by changing the PDM_CLK edge on which the sampling is done
	 * Provide the valid channel maps for both the above configurations
	 * (to inform the requester what is available) and check if what is
	 * requested can be actually configured.
	 */
	if (channel->req_num_chan == 1) {
		def_map = dmic_build_channel_map(0, 0, PDM_CHAN_LEFT);
		alt_map = dmic_build_channel_map(0, 0, PDM_CHAN_RIGHT);

		channel->act_num_chan = 1;
	} else {
		def_map = dmic_build_channel_map(0, 0, PDM_CHAN_LEFT)
			| dmic_build_channel_map(1, 0, PDM_CHAN_RIGHT);
		alt_map = dmic_build_channel_map(0, 0, PDM_CHAN_RIGHT)
			| dmic_build_channel_map(1, 0, PDM_CHAN_LEFT);

		channel->act_num_chan = 2;
	}

	channel->act_num_streams = 1;
	channel->act_chan_map_hi = 0;

	if (channel->req_num_streams != 1 ||
	    channel->req_num_chan > 2 ||
	    channel->req_num_chan < 1 ||
	    (channel->req_chan_map_lo != def_map &&
	     channel->req_chan_map_lo != alt_map) ||
	    channel->req_chan_map_hi != channel->act_chan_map_hi) {
		LOG_ERR("Requested configuration is not supported");
		return -EINVAL;
	}

	/* If either rate or width is 0, the stream is to be disabled. */
	if (stream->pcm_rate == 0 || stream->pcm_width == 0) {
		if (drv_data->configured) {
			nrfx_pdm_uninit(&drv_data->pdm);
			drv_data->configured = false;
		}

		return 0;
	}

	if (stream->pcm_width != 16) {
		LOG_ERR("Only 16-bit samples are supported");
		return -EINVAL;
	}

	nrfx_cfg = drv_cfg->nrfx_def_cfg;
	nrfx_cfg.mode = channel->req_num_chan == 1
		      ? NRF_PDM_MODE_MONO
		      : NRF_PDM_MODE_STEREO;
	if (channel->req_chan_map_lo == def_map) {
		nrfx_cfg.edge = NRF_PDM_EDGE_LEFTFALLING;
		channel->act_chan_map_lo = def_map;
	} else {
		nrfx_cfg.edge = NRF_PDM_EDGE_LEFTRISING;
		channel->act_chan_map_lo = alt_map;
	}

	/* Convert requested gain to 0.5 dB steps limited by defined bounds. */
	gain_limit = CLAMP((2 * stream->gain_db + NRF_PDM_GAIN_DEFAULT),
			   NRF_PDM_GAIN_MINIMUM,
			   NRF_PDM_GAIN_MAXIMUM);
	nrfx_cfg.gain_l = gain_limit;
	nrfx_cfg.gain_r = gain_limit;

#if NRF_PDM_HAS_SELECTABLE_CLOCK
	nrfx_cfg.mclksrc = drv_cfg->clk_src == ACLK
			 ? NRF_PDM_MCLKSRC_ACLK
			 : NRF_PDM_MCLKSRC_PCLK32M;
#endif
	nrfx_pdm_output_t output_config = {
		.base_clock_freq = (NRF_PDM_HAS_SELECTABLE_CLOCK && drv_cfg->clk_src == ACLK)
					? DMIC_NRFX_AUDIO_CLOCK_FREQ
					: DMIC_NRFX_CLOCK_FREQ,
		.sampling_rate = config->streams[0].pcm_rate,
		.output_freq_min = config->io.min_pdm_clk_freq,
		.output_freq_max = config->io.max_pdm_clk_freq
	};

#if USE_CUSTOM_RATIO
	err = custom_ratio_calculate(&output_config, &nrfx_cfg.prescalers);
#else
	err = nrfx_pdm_prescalers_calc(&output_config, &nrfx_cfg.prescalers);
#endif
	if (err != 0) {
		LOG_ERR("Cannot find suitable PDM clock configuration.");
		return -EINVAL;
	}

	if (drv_data->configured) {
		nrfx_pdm_uninit(&drv_data->pdm);
		drv_data->configured = false;
	}

	err = nrfx_pdm_init(&drv_data->pdm, &nrfx_cfg, drv_cfg->event_handler);
	if (err != 0) {
		LOG_ERR("Failed to initialize PDM: %d", err);
		return -EIO;
	}

	drv_data->block_size = stream->block_size;
	drv_data->mem_slab   = stream->mem_slab;

	/* Unless the PCLK32M source is used with the HFINT oscillator
	 * (which is always available without any additional actions),
	 * it is required to request the proper clock to be running
	 * before starting the transfer itself.
	 */
	drv_data->request_clock =
		(drv_cfg->clk_src != PCLK32M && (IS_ENABLED(CONFIG_CLOCK_CONTROL_NRF) ||
						 (IS_ENABLED(CONFIG_CLOCK_CONTROL_NRF_COMMON) &&
						  !(IS_ENABLED(CONFIG_SOC_SERIES_NRF54H) ||
						    IS_ENABLED(CONFIG_SOC_SERIES_NRF92)))));
	drv_data->configured = true;
	return 0;
}

static int start_transfer(struct dmic_nrfx_pdm_drv_data *drv_data)
{
	int err;
	int ret;

	err = nrfx_pdm_start(&drv_data->pdm);
	if (err == 0) {
		return 0;
	}

	LOG_ERR("Failed to start PDM: %d", err);

	ret = release_clock(drv_data);
	if (ret < 0) {
		LOG_ERR("Failed to release clock: %d", ret);
	}

	drv_data->active = false;
	return -EIO;
}

static void clock_started_callback(struct onoff_manager *mgr,
				   struct onoff_client *cli,
				   uint32_t state,
				   int res)
{
	struct dmic_nrfx_pdm_drv_data *drv_data =
		CONTAINER_OF(cli, struct dmic_nrfx_pdm_drv_data, clk_cli);

	/* The driver can turn out to be inactive at this point if the STOP
	 * command was triggered before the clock has started. Do not start
	 * the actual transfer in such case.
	 */
	if (!drv_data->active) {
		int ret = release_clock(drv_data);

		if (ret < 0) {
			LOG_ERR("Failed to release clock: %d", ret);
			return;
		}
	} else {
		(void)start_transfer(drv_data);
	}
}

static int trigger_start(const struct device *dev)
{
	struct dmic_nrfx_pdm_drv_data *drv_data = dev->data;
	int ret;

	drv_data->active = true;

	/* If it is required to use certain HF clock, request it to be running
	 * first. If not, start the transfer directly.
	 */
	if (drv_data->request_clock) {
		sys_notify_init_callback(&drv_data->clk_cli.notify,
					 clock_started_callback);
		ret = request_clock(drv_data);
		if (ret < 0) {
			drv_data->active = false;

			LOG_ERR("Failed to request clock: %d", ret);
			return -EIO;
		}
	} else {
		ret = start_transfer(drv_data);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int dmic_nrfx_pdm_trigger(const struct device *dev,
				 enum dmic_trigger cmd)
{
	struct dmic_nrfx_pdm_drv_data *drv_data = dev->data;

	switch (cmd) {
	case DMIC_TRIGGER_PAUSE:
	case DMIC_TRIGGER_STOP:
		if (drv_data->active) {
			drv_data->stopping = true;
			nrfx_pdm_stop(&drv_data->pdm);
		}
		break;

	case DMIC_TRIGGER_RELEASE:
	case DMIC_TRIGGER_START:
		if (!drv_data->configured) {
			LOG_ERR("Device is not configured");
			return -EIO;
		} else if (!drv_data->active) {
			drv_data->stopping = false;
			return trigger_start(dev);
		}
		break;

	default:
		LOG_ERR("Invalid command: %d", cmd);
		return -EINVAL;
	}

	return 0;
}

static int dmic_nrfx_pdm_read(const struct device *dev,
			      uint8_t stream,
			      void **buffer, size_t *size, int32_t timeout)
{
	struct dmic_nrfx_pdm_drv_data *drv_data = dev->data;
	int ret;

	ARG_UNUSED(stream);

	if (!drv_data->configured) {
		LOG_ERR("Device is not configured");
		return -EIO;
	}

	ret = k_msgq_get(&drv_data->rx_queue, buffer, SYS_TIMEOUT_MS(timeout));
	if (ret != 0) {
		LOG_DBG("No audio data to be read");
	} else {
		LOG_DBG("Released buffer %p", *buffer);

		*size = drv_data->block_size;
	}

	return ret;
}

static void init_clock_manager(const struct device *dev)
{
#if DT_NODE_HAS_STATUS_OKAY(NODE_AUDIO_AUXPLL)
	struct dmic_nrfx_pdm_drv_data *drv_data = dev->data;
	drv_data->audiopll_dev = DEVICE_DT_GET(NODE_AUDIO_AUXPLL);
#elif CONFIG_CLOCK_CONTROL_NRF
	clock_control_subsys_t subsys;
	struct dmic_nrfx_pdm_drv_data *drv_data = dev->data;
#if NRF_CLOCK_HAS_HFCLKAUDIO || NRF_CLOCK_HAS_HFCLK24M
	const struct dmic_nrfx_pdm_drv_cfg *drv_cfg = dev->config;

	if (drv_cfg->clk_src == ACLK) {
		subsys = COND_CODE_1(NRF_CLOCK_HAS_HFCLK24M, (CLOCK_CONTROL_NRF_SUBSYS_HF24M),
			(CLOCK_CONTROL_NRF_SUBSYS_HFAUDIO));
	} else {
		subsys = CLOCK_CONTROL_NRF_SUBSYS_HF;
	}
#else
	subsys = CLOCK_CONTROL_NRF_SUBSYS_HF;
#endif

	drv_data->clk_mgr = z_nrf_clock_control_get_onoff(subsys);
	__ASSERT_NO_MSG(drv_data->clk_mgr != NULL);
#elif defined(CONFIG_CLOCK_CONTROL_NRF_HFCLKAUDIO) || defined(CONFIG_CLOCK_CONTROL_NRF_HFCLK) ||   \
	defined(CONFIG_CLOCK_CONTROL_NRF_XO) || defined(CONFIG_CLOCK_CONTROL_NRF_XO24M)
	struct dmic_nrfx_pdm_drv_data *drv_data = dev->data;
#if NRF_CLOCK_HAS_HFCLKAUDIO || NRF_CLOCK_HAS_HFCLK24M
	const struct dmic_nrfx_pdm_drv_cfg *drv_cfg = dev->config;

	if (drv_cfg->clk_src == ACLK) {
		drv_data->clk_dev = DEVICE_DT_GET_ONE(
			COND_CODE_1(NRF_CLOCK_HAS_HFCLK24M,
			(nordic_nrf_clock_xo24m), (nordic_nrf_clock_hfclkaudio)));
	} else {
		drv_data->clk_dev = DEVICE_DT_GET_ONE(
			COND_CODE_1(NRF_CLOCK_HAS_HFCLK,
			(nordic_nrf_clock_hfclk), (nordic_nrf_clock_xo)));
	}
#else
	drv_data->clk_dev = DEVICE_DT_GET_ONE(
		COND_CODE_1(NRF_CLOCK_HAS_HFCLK, (nordic_nrf_clock_hfclk), (nordic_nrf_clock_xo)));
#endif

	__ASSERT_NO_MSG(drv_data->clk_dev != NULL);
#elif CONFIG_CLOCK_CONTROL_NRFS_AUDIOPLL
	struct dmic_nrfx_pdm_drv_data *drv_data = dev->data;

	drv_data->audiopll_dev = DEVICE_DT_GET(DT_NODELABEL(audiopll));
#endif
}

static DEVICE_API(dmic, dmic_ops) = {
	.configure = dmic_nrfx_pdm_configure,
	.trigger = dmic_nrfx_pdm_trigger,
	.read = dmic_nrfx_pdm_read,
};

#define PDM_CLK_SRC(inst) DT_STRING_TOKEN(DT_DRV_INST(inst), clock_source)

#define PDM_NRFX_DEVICE(inst)                                                                      \
	static void *rx_msgs##inst[DT_INST_PROP(inst, queue_size)];                                \
	static void *mem_slab_msgs##inst[DT_INST_PROP(inst, queue_size)];                          \
	static struct dmic_nrfx_pdm_drv_data dmic_nrfx_pdm_data##inst = {                          \
		.pdm = NRFX_PDM_INSTANCE(DT_INST_REG_ADDR(inst)),                                  \
	};                                                                                         \
	static int pdm_nrfx_init##inst(const struct device *dev)                                   \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), nrfx_pdm_irq_handler, \
			    &dmic_nrfx_pdm_data##inst.pdm, 0);                                     \
		const struct dmic_nrfx_pdm_drv_cfg *drv_cfg = dev->config;                         \
		int err = pinctrl_apply_state(drv_cfg->pcfg, PINCTRL_STATE_DEFAULT);               \
		if (err < 0) {                                                                     \
			return err;                                                                \
		}                                                                                  \
		k_msgq_init(&dmic_nrfx_pdm_data##inst.rx_queue, (char *)rx_msgs##inst,             \
			    sizeof(void *), ARRAY_SIZE(rx_msgs##inst));                            \
		k_msgq_init(&dmic_nrfx_pdm_data##inst.mem_slab_queue, (char *)mem_slab_msgs##inst, \
			    sizeof(void *), ARRAY_SIZE(mem_slab_msgs##inst));                      \
		init_clock_manager(dev);                                                           \
		return 0;                                                                          \
	}                                                                                          \
	static void event_handler##inst(const nrfx_pdm_evt_t *evt)                                 \
	{                                                                                          \
		event_handler(DEVICE_DT_INST_GET(inst), evt);                                      \
	}                                                                                          \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	static const struct dmic_nrfx_pdm_drv_cfg dmic_nrfx_pdm_cfg##inst = {                      \
		.event_handler = event_handler##inst,                                              \
		.nrfx_def_cfg = NRFX_PDM_DEFAULT_CONFIG(0, 0),                                     \
		.nrfx_def_cfg.skip_gpio_cfg = true,                                                \
		.nrfx_def_cfg.skip_psel_cfg = true,                                                \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
		.clk_src = PDM_CLK_SRC(inst),                                                      \
		.mem_reg = DMM_DEV_TO_REG(DT_DRV_INST(inst)),                                      \
	};                                                                                         \
	NRF_DT_CHECK_NODE_HAS_REQUIRED_MEMORY_REGIONS(DT_DRV_INST(inst));                          \
	BUILD_ASSERT(PDM_CLK_SRC(inst) != ACLK || NRF_PDM_HAS_SELECTABLE_CLOCK,                    \
		     "Clock source ACLK is not available.");                                       \
	BUILD_ASSERT(PDM_CLK_SRC(inst) != ACLK || AUDIO_FREQUENCY_DEFINED, AUDIO_ASSERT_MSG);      \
	DEVICE_DT_INST_DEFINE(inst, pdm_nrfx_init##inst, NULL, &dmic_nrfx_pdm_data##inst,          \
			      &dmic_nrfx_pdm_cfg##inst, POST_KERNEL,                               \
			      CONFIG_AUDIO_DMIC_INIT_PRIORITY, &dmic_ops);

DT_INST_FOREACH_STATUS_OKAY(PDM_NRFX_DEVICE)

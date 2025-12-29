/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_psoc4_adc

#include <zephyr/drivers/adc.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/clock_control/clock_control_ifx_cat1.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#include "cy_sar.h"
#include "cy_pdl.h"
#include "cy_gpio.h"
#include "cy_device.h"
#include "cyip_hsiom.h"

LOG_MODULE_REGISTER(psoc4_adc, CONFIG_ADC_LOG_LEVEL);

#define PSOC4_ADC_REF_INTERNAL_MV 1200U

/* SAR ADC acquisition timing:
 * PSoC4 SAR ADC clock runs at ~18 MHz.
 * One SAR clock period ≈ 55.5 ns.
 * Use 55 ns to ensure rounding up when converting time to ticks.
 */
#define PSOC4_ADC_ACQ_NS_PER_TICK 55U

/* PSoC4 SAR ADC sample time limits (hardware defined) */
#define PSOC4_ADC_MIN_ACQ_TICKS 3U
#define PSOC4_ADC_MAX_ACQ_TICKS 1023U

/* Number of hardware sample time registers in PSoC4 SAR ADC */
#define PSOC4_ADC_NUM_SAMPLE_TIMES 4U

/* Invalid sample time index */
#define PSOC4_ADC_INVALID_SAMPLE_TIME 0xFFU

#define PSOC4_ADC_SARMUX_PIN_MAX 7U /* SARMUX port 2, pins 0-7 */

enum psoc4_adc_vref_src {
	PSOC4_ADC_VREF_INTERNAL = 0,
	PSOC4_ADC_VREF_VDDA = 1,
	PSOC4_ADC_VREF_VDDA_DIV_2 = 2,
	PSOC4_ADC_VREF_EXT = 3,
};

struct psoc4_adc_config {
	SAR_Type *base;
	enum psoc4_adc_vref_src vref_src;
	uint32_t vref_mv;
	void (*irq_func)(const struct device *dev);
	en_clk_dst_t clk_dst;
};
struct psoc4_adc_channel_cfg {
	bool differential;
	uint8_t vplus;
	uint8_t vminus;
	uint8_t reference;
	uint8_t sample_time_idx;
	uint32_t sw_mask;
};

struct psoc4_adc_data {
	struct adc_context ctx;
	const struct device *dev;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	uint32_t channels;
	uint32_t channels_mask;
	struct ifx_cat1_clock clock;
	uint8_t resolution[CY_SAR_SEQ_NUM_CHANNELS];
	struct psoc4_adc_channel_cfg channel_cfg[CY_SAR_SEQ_NUM_CHANNELS];
	uint32_t sample_times[PSOC4_ADC_NUM_SAMPLE_TIMES];
	cy_stc_sar_config_t pdl_sar_cfg;
	cy_stc_sar_channel_config_t channel_configs[CY_SAR_SEQ_NUM_CHANNELS];
};

static const cy_stc_sar_config_t psoc4_sar_pdl_cfg_struct_default = {
	.vrefSel = CY_SAR_VREF_SEL_BGR,
	.vrefBypCapEn = true,
	.vrefMvValue = PSOC4_ADC_REF_INTERNAL_MV,
	.negSel = CY_SAR_NEG_SEL_VSSA_KELVIN,
	.power = CY_SAR_HALF_PWR,
	.singleEndedSigned = false,
	.differentialSigned = false,
	.trigMode = CY_SAR_TRIGGER_MODE_FW_ONLY,
	.eosEn = true,
	.sampleTime0 = 3,
	.sampleTime1 = 3,
	.sampleTime2 = 3,
	.sampleTime3 = 3,
};

static const cy_stc_sar_channel_config_t psoc4_default_channel_cfg = {
	.addr = CY_SAR_ADDR_SARMUX_0,
	.differential = false,
	.resolution = CY_SAR_MAX_RES,
	.avgEn = false,
	.sampleTimeSel = CY_SAR_SAMPLE_TIME_0,
	.rangeIntrEn = false,
	.satIntrEn = false,
#if (defined(CY_IP_M0S8PASS4A_SAR_VERSION) && (4U <= CY_IP_M0S8PASS4A_SAR_VERSION))
	.neg_addr = CY_SAR_NEG_ADDR_SARMUX_0,
	.negAddrEn = false,
#endif
};

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct psoc4_adc_data *data = CONTAINER_OF(ctx, struct psoc4_adc_data, ctx);
	const struct psoc4_adc_config *config = data->dev->config;

	data->repeat_buffer = data->buffer;
	Cy_SAR_StartConvert(config->base, CY_SAR_START_CONVERT_SINGLE_SHOT);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct psoc4_adc_data *data = CONTAINER_OF(ctx, struct psoc4_adc_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}
/* Convert Zephyr ADC acquisition time to hardware clock ticks */
static uint32_t psoc4_calculate_acq_ticks(uint32_t acquisition_time)
{
	uint32_t ticks;

	if (acquisition_time == ADC_ACQ_TIME_DEFAULT) {
		/* Use minimum acquisition time (3 ticks) for default */
		ticks = PSOC4_ADC_MIN_ACQ_TICKS;
	} else if (ADC_ACQ_TIME_UNIT(acquisition_time) == ADC_ACQ_TIME_TICKS) {
		ticks = ADC_ACQ_TIME_VALUE(acquisition_time);
	} else {
		/* Convert nanoseconds/microseconds to ADC clock ticks */
		uint32_t ns =
			ADC_ACQ_TIME_VALUE(acquisition_time) *
			((ADC_ACQ_TIME_UNIT(acquisition_time) == ADC_ACQ_TIME_MICROSECONDS) ? 1000U
											    : 1U);

		/*
		 * Calculate ticks with ceiling division: (ns + tick_period - 1) / tick_period
		 * PSOC4_ADC_ACQ_NS_PER_TICK = 55 ns (18 MHz ADC clock ≈ 55.5 ns period)
		 * Ensures we round up to guarantee minimum acquisition time
		 */
		ticks = (ns + PSOC4_ADC_ACQ_NS_PER_TICK - 1U) / PSOC4_ADC_ACQ_NS_PER_TICK;
	}

	/*
	 * Clamp ticks to valid hardware range.
	 * PSoC4 SAR ADC hardware supports 3-1023 clock ticks for sampling.
	 */
	return CLAMP(ticks, PSOC4_ADC_MIN_ACQ_TICKS, PSOC4_ADC_MAX_ACQ_TICKS);
}

/* Find an available sample time slot (0-3) */
static uint8_t psoc4_find_sample_time_idx(struct psoc4_adc_data *data, uint32_t ticks)
{
	/*
	 * Find an available sample time slot (0-3).
	 * PSoC4 has 4 hardware sample time registers that can be shared between channels.
	 * This allows different channels to have different acquisition times.
	 */
	for (int i = 0; i < PSOC4_ADC_NUM_SAMPLE_TIMES; i++) {
		if (data->sample_times[i] == ticks || data->sample_times[i] == 0U) {
			data->sample_times[i] = ticks;
			return i; /* Return slot index (0-3) */
		}
	}

	/* No available slots found */
	return PSOC4_ADC_INVALID_SAMPLE_TIME;
}

static int psoc4_adc_channel_setup(const struct device *dev,
				   const struct adc_channel_cfg *channel_cfg)
{
	struct psoc4_adc_data *data = dev->data;
	uint8_t ch = channel_cfg->channel_id;
	uint32_t sw_mask = 0;
	uint32_t port;
	uint32_t gpio_pin;
	GPIO_PRT_Type *port_base;

	/* Channel validation */
	if (ch >= CY_SAR_SEQ_NUM_CHANNELS) {
		LOG_ERR("Invalid channel ID: %u", ch);
		return -EINVAL;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("Invalid gain: %d", channel_cfg->gain);
		return -EINVAL;
	}

	if (channel_cfg->reference != ADC_REF_INTERNAL && channel_cfg->reference != ADC_REF_VDD_1 &&
	    channel_cfg->reference != ADC_REF_VDD_1_2 &&
	    channel_cfg->reference != ADC_REF_EXTERNAL0) {
		LOG_ERR("Invalid reference: %d", channel_cfg->reference);
		return -EINVAL;
	}

	/* Input validation */
	if (channel_cfg->input_positive > PSOC4_ADC_SARMUX_PIN_MAX) {
		LOG_ERR("Channel %u: invalid input_positive %u (only SARMUX 0-7 supported)", ch,
			channel_cfg->input_positive);
		return -EINVAL;
	}

	if (channel_cfg->differential && channel_cfg->input_negative > PSOC4_ADC_SARMUX_PIN_MAX) {
		LOG_ERR("Channel %u: invalid input_negative %u (only SARMUX 0-7 supported)", ch,
			channel_cfg->input_negative);
		return -EINVAL;
	}

	data->channel_cfg[ch].differential = channel_cfg->differential;
	data->channel_cfg[ch].vplus = channel_cfg->input_positive;

	/* SARMUX pins are on port 2, pins 0-7 directly mapped */
	port = 2U;
	gpio_pin = channel_cfg->input_positive;
	port_base = Cy_GPIO_PortToAddr(port);

	if (!port_base) {
		LOG_ERR("Channel %u: Cannot get GPIO port for pin %u", ch,
			channel_cfg->input_positive);
		return -EINVAL;
	}

	/* Configure GPIO pin for analog input mode */
	Cy_GPIO_SetDrivemode(port_base, gpio_pin, CY_GPIO_DM_ANALOG);

	/*
	 * Configure SAR switch mask based on which SARMUX pin is used.
	 * Each SARMUX pin (0-7) has a dedicated hardware switch that connects
	 * the pin to the SAR ADC input multiplexer.
	 */
	switch (channel_cfg->input_positive) {
	case 0:
		sw_mask |= CY_SAR_MUX_FW_P0_VPLUS;
		break;
	case 1:
		sw_mask |= CY_SAR_MUX_FW_P1_VPLUS;
		break;
	case 2:
		sw_mask |= CY_SAR_MUX_FW_P2_VPLUS;
		break;
	case 3:
		sw_mask |= CY_SAR_MUX_FW_P3_VPLUS;
		break;
	case 4:
		sw_mask |= CY_SAR_MUX_FW_P4_VPLUS;
		break;
	case 5:
		sw_mask |= CY_SAR_MUX_FW_P5_VPLUS;
		break;
	case 6:
		sw_mask |= CY_SAR_MUX_FW_P6_VPLUS;
		break;
	case 7:
		sw_mask |= CY_SAR_MUX_FW_P7_VPLUS;
		break;
	default:
		return -EINVAL;
	}

	/* configuration for Negative input (differential only) */
	if (channel_cfg->differential) {

		data->channel_cfg[ch].vminus = channel_cfg->input_negative;

		port = 2U;
		gpio_pin = channel_cfg->input_negative;
		port_base = Cy_GPIO_PortToAddr(port);

		if (!port_base) {
			return -EINVAL;
		}

		Cy_GPIO_SetDrivemode(port_base, gpio_pin, CY_GPIO_DM_ANALOG);

		switch (channel_cfg->input_negative) {
		case 0:
			sw_mask |= CY_SAR_MUX_FW_P0_VMINUS;
			break;
		case 1:
			sw_mask |= CY_SAR_MUX_FW_P1_VMINUS;
			break;
		case 2:
			sw_mask |= CY_SAR_MUX_FW_P2_VMINUS;
			break;
		case 3:
			sw_mask |= CY_SAR_MUX_FW_P3_VMINUS;
			break;
		case 4:
			sw_mask |= CY_SAR_MUX_FW_P4_VMINUS;
			break;
		case 5:
			sw_mask |= CY_SAR_MUX_FW_P5_VMINUS;
			break;
		case 6:
			sw_mask |= CY_SAR_MUX_FW_P6_VMINUS;
			break;
		case 7:
			sw_mask |= CY_SAR_MUX_FW_P7_VMINUS;
			break;
		}
	}

	/* Check if any switches were configured */
	if (sw_mask == 0) {
		LOG_ERR("Channel %u: No SAR switches configured", ch);
		return -EINVAL;
	}

	data->channel_cfg[ch].sw_mask = sw_mask;
	data->channel_cfg[ch].reference = channel_cfg->reference;

	/* Acquisition time handling */

	/* Number of ADC clock ticks for sampling */
	uint32_t ticks = psoc4_calculate_acq_ticks(channel_cfg->acquisition_time);

	/*
	 * Find an available sample time slot (0-3).
	 * PSoC4 has 4 hardware sample time registers that can be shared between channels.
	 * This allows different channels to have different acquisition times.
	 */
	uint8_t sample_time_idx = psoc4_find_sample_time_idx(data, ticks);

	/* Check if we found an available sample time slot */
	if (sample_time_idx == PSOC4_ADC_INVALID_SAMPLE_TIME) {
		LOG_ERR("Channel %u: No available sample time slots", ch);
		return -EINVAL;
	}

	/* Store which sample time slot this channel will use (0-3) */
	data->channel_cfg[ch].sample_time_idx = sample_time_idx;
	data->channels_mask |= BIT(ch);

	LOG_DBG("Channel %u configured: positive input pin %u, %s, sw_mask=0x%08x", ch,
		channel_cfg->input_positive,
		channel_cfg->differential ? "differential" : "single-ended", sw_mask);

	return 0;
}

static int validate_buffer_size(const struct adc_sequence *sequence)
{
	size_t active_channels = 0;
	size_t required_buffer_size;

	for (int i = 0; i < CY_SAR_SEQ_NUM_CHANNELS; i++) {
		if (sequence->channels & BIT(i)) {
			active_channels++;
		}
	}

	required_buffer_size = active_channels * sizeof(uint16_t);

	if (sequence->options) {
		required_buffer_size *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < required_buffer_size) {
		LOG_ERR("Buffer too small: need %zu, got %zu", required_buffer_size,
			sequence->buffer_size);
		return -ENOMEM;
	}

	return 0;
}

static void psoc4_map_oversampling(cy_stc_sar_config_t *cfg, uint8_t oversampling)
{
	if (oversampling == 0) {
		cfg->avgCnt = CY_SAR_AVG_CNT_2;

		/* No right shift of result */
		cfg->avgShift = false;
		return;
	}

	/* For oversampling > 0, enable result shifting (divide by N) */
	cfg->avgShift = true;

	/* Map oversampling value (1-8 from overlay) to hardware averaging count */
	switch (oversampling) {
	case 1:
		cfg->avgCnt = CY_SAR_AVG_CNT_2;
		break;
	case 2:
		cfg->avgCnt = CY_SAR_AVG_CNT_4;
		break;
	case 3:
		cfg->avgCnt = CY_SAR_AVG_CNT_8;
		break;
	case 4:
		cfg->avgCnt = CY_SAR_AVG_CNT_16;
		break;
	case 5:
		cfg->avgCnt = CY_SAR_AVG_CNT_32;
		break;
	case 6:
		cfg->avgCnt = CY_SAR_AVG_CNT_64;
		break;
	case 7:
		cfg->avgCnt = CY_SAR_AVG_CNT_128;
		break;
	case 8:
		cfg->avgCnt = CY_SAR_AVG_CNT_256;
		break;
	default:
		cfg->avgCnt = CY_SAR_AVG_CNT_2;
		cfg->avgShift = false;
		break;
	}
}

/* Configure ADC reference voltage */
static void psoc4_configure_reference(struct psoc4_adc_data *data,
				      const struct psoc4_adc_config *cfg)
{
	uint32_t vref_mv = cfg->vref_mv;

	switch (cfg->vref_src) {
	case PSOC4_ADC_VREF_INTERNAL:
		data->pdl_sar_cfg.vrefSel = CY_SAR_VREF_SEL_BGR;
		data->pdl_sar_cfg.vrefMvValue = PSOC4_ADC_REF_INTERNAL_MV;
		break;
	case PSOC4_ADC_VREF_VDDA:
		data->pdl_sar_cfg.vrefSel = CY_SAR_VREF_SEL_VDDA;
		data->pdl_sar_cfg.vrefMvValue = vref_mv;
		break;
	case PSOC4_ADC_VREF_VDDA_DIV_2:
		data->pdl_sar_cfg.vrefSel = CY_SAR_VREF_SEL_VDDA_DIV_2;
		data->pdl_sar_cfg.vrefMvValue = vref_mv / 2;
		break;
	case PSOC4_ADC_VREF_EXT:
		data->pdl_sar_cfg.vrefSel = CY_SAR_VREF_SEL_EXT;
		data->pdl_sar_cfg.vrefMvValue = vref_mv;
		break;
	default:
		LOG_ERR("Unsupported VREF source, using Internal");
		data->pdl_sar_cfg.vrefSel = CY_SAR_VREF_SEL_BGR;
		data->pdl_sar_cfg.vrefMvValue = PSOC4_ADC_REF_INTERNAL_MV;
		break;
	}
}

/* Configure ADC sample times */
static void psoc4_configure_sample_times(struct psoc4_adc_data *data)
{
	data->pdl_sar_cfg.sampleTime0 = data->sample_times[0] ? data->sample_times[0] : 3;
	data->pdl_sar_cfg.sampleTime1 = data->sample_times[1] ? data->sample_times[1] : 3;
	data->pdl_sar_cfg.sampleTime2 = data->sample_times[2] ? data->sample_times[2] : 3;
	data->pdl_sar_cfg.sampleTime3 = data->sample_times[3] ? data->sample_times[3] : 3;
}

static void psoc4_adc_configure_pdl(struct psoc4_adc_data *data,
				    const struct adc_sequence *sequence,
				    const struct psoc4_adc_config *cfg)
{
	bool has_differential = false;
	bool has_12bit = false;

	/* Initialize with defaults */
	data->pdl_sar_cfg = psoc4_sar_pdl_cfg_struct_default;

	/* Configure reference voltage */
	psoc4_configure_reference(data, cfg);

	/* Configure sample times */
	psoc4_configure_sample_times(data);

	/* Configure oversampling */
	psoc4_map_oversampling(&data->pdl_sar_cfg, sequence->oversampling);

	/* Configure channels */
	data->pdl_sar_cfg.chanEn = sequence->channels;

	for (int ch = 0; ch < CY_SAR_SEQ_NUM_CHANNELS; ch++) {
		if (!(sequence->channels & BIT(ch))) {
			data->pdl_sar_cfg.channelConfig[ch] = NULL;
			continue;
		}

		data->resolution[ch] = sequence->resolution;
		const struct psoc4_adc_channel_cfg *ch_cfg = &data->channel_cfg[ch];

		/* Check for differential mode */
		if (ch_cfg->differential) {
			has_differential = true;
		}

		/* Check for 12-bit resolution */
		if (sequence->resolution == 12) {
			has_12bit = true;
		}

		/* Calculate SAR address for positive input */
		cy_en_sar_chan_config_port_pin_addr_t vplus_addr =
			(cy_en_sar_chan_config_port_pin_addr_t)(CY_SAR_ADDR_SARMUX_0 +
								ch_cfg->vplus);

		/* Calculate SAR address for negative input (differential only) */
#if (defined(CY_IP_M0S8PASS4A_SAR_VERSION) && (4U <= CY_IP_M0S8PASS4A_SAR_VERSION))
		cy_en_sar_chan_config_neg_port_pin_addr_t vminus_addr = 0;
		bool vminus_addr_en = false;

		if (ch_cfg->differential) {
			uint32_t neg_addr = CY_SAR_NEG_ADDR_SARMUX_0 + ch_cfg->vminus;

			vminus_addr = (cy_en_sar_chan_config_neg_port_pin_addr_t)neg_addr;
			vminus_addr_en = true;
		}
#endif

		/* Configure channel structure */
		data->channel_configs[ch] = psoc4_default_channel_cfg;
		data->channel_configs[ch].addr = vplus_addr;
		data->channel_configs[ch].differential = ch_cfg->differential;
		data->channel_configs[ch].avgEn = (sequence->oversampling > 0);
		data->channel_configs[ch].resolution =
			(sequence->resolution == 12) ? CY_SAR_MAX_RES : CY_SAR_SUB_RES;
		data->channel_configs[ch].sampleTimeSel =
			(cy_en_sar_channel_sampletime_t)ch_cfg->sample_time_idx;

#if (defined(CY_IP_M0S8PASS4A_SAR_VERSION) && (4U <= CY_IP_M0S8PASS4A_SAR_VERSION))
		if (ch_cfg->differential && vminus_addr_en) {
			data->channel_configs[ch].neg_addr = vminus_addr;
			data->channel_configs[ch].negAddrEn = true;
		} else {
			data->channel_configs[ch].negAddrEn = false;
		}
#endif

		data->pdl_sar_cfg.channelConfig[ch] = &data->channel_configs[ch];
	}

	/* Set differential mode */
	data->pdl_sar_cfg.differentialSigned = has_differential;

	/*  Set Sub-resolution */
	if (has_12bit) {
		data->pdl_sar_cfg.subResolution = CY_SAR_SUB_RESOLUTION_10B;
	} else if (sequence->resolution == 10) {
		data->pdl_sar_cfg.subResolution = CY_SAR_SUB_RESOLUTION_10B;
	} else {
		data->pdl_sar_cfg.subResolution = CY_SAR_SUB_RESOLUTION_8B;
	}
}

static void psoc4_adc_set_switches(struct psoc4_adc_data *data, const struct adc_sequence *sequence,
				   const struct psoc4_adc_config *cfg)
{
	uint32_t total_sw_mask = 0;
	uint32_t hw_ctrl_mask = 0;
	bool has_differential = false;

	/* Calculate switch masks for both firmware and hardware control */
	for (int ch = 0; ch < CY_SAR_SEQ_NUM_CHANNELS; ch++) {
		if (sequence->channels & BIT(ch)) {
			const struct psoc4_adc_channel_cfg *ch_cfg = &data->channel_cfg[ch];

			total_sw_mask |= ch_cfg->sw_mask;

			if (ch_cfg->differential) {
				has_differential = true;
			}

			/* SARMUX pins 0-7: Enable hardware control for the pin */
			hw_ctrl_mask |=
				(SAR_MUX_SWITCH_HW_CTRL_MUX_HW_CTRL_P0_Msk << ch_cfg->vplus);

			/* Handle negative input hardware control for differential mode */
			if (ch_cfg->differential) {
				hw_ctrl_mask |= (SAR_MUX_SWITCH_HW_CTRL_MUX_HW_CTRL_P0_Msk
						 << ch_cfg->vminus);
			}
		}
	}

	/* Define all possible analog switches */
	uint32_t all_switches =
		CY_SAR_MUX_FW_P0_VPLUS | CY_SAR_MUX_FW_P1_VPLUS | CY_SAR_MUX_FW_P2_VPLUS |
		CY_SAR_MUX_FW_P3_VPLUS | CY_SAR_MUX_FW_P4_VPLUS | CY_SAR_MUX_FW_P5_VPLUS |
		CY_SAR_MUX_FW_P6_VPLUS | CY_SAR_MUX_FW_P7_VPLUS | CY_SAR_MUX_FW_P0_VMINUS |
		CY_SAR_MUX_FW_P1_VMINUS | CY_SAR_MUX_FW_P2_VMINUS | CY_SAR_MUX_FW_P3_VMINUS |
		CY_SAR_MUX_FW_P4_VMINUS | CY_SAR_MUX_FW_P5_VMINUS | CY_SAR_MUX_FW_P6_VMINUS |
		CY_SAR_MUX_FW_P7_VMINUS;

	/* Turn off all switches first */
	Cy_SAR_SetAnalogSwitch(cfg->base, all_switches, false);

	/* Turn on the needed switches via firmware */
	if (total_sw_mask != 0) {
		Cy_SAR_SetAnalogSwitch(cfg->base, total_sw_mask, true);
	}

	/*
	 * Enable SARSEQ (hardware sequencer) control of the switches.
	 * This allows the SAR hardware to automatically manage switch selection
	 * per channel during multi-channel sequences, overriding firmware control
	 * to prevent crosstalk by ensuring only one channel's switches are active
	 * at a time during conversion.
	 */
#ifdef CONFIG_ADC_INFINEON_PSOC4_SARSEQ_SWITCHING
	if (hw_ctrl_mask != 0) {
		Cy_SAR_SetSwitchSarSeqCtrl(cfg->base, hw_ctrl_mask, true);
	}
#endif
	/* Configure VSSA_VMINUS switch based on differential mode */
	if (has_differential) {
		Cy_SAR_SetAnalogSwitch(cfg->base, CY_SAR_MUX_FW_VSSA_VMINUS, false);
	} else {
		Cy_SAR_SetAnalogSwitch(cfg->base, CY_SAR_MUX_FW_VSSA_VMINUS, true);
	}
}

static int start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct psoc4_adc_data *data = dev->data;
	const struct psoc4_adc_config *cfg = dev->config;
	cy_en_sar_status_t sar_status;

	if (sequence->channels == 0) {
		LOG_ERR("No channels selected");
		return -EINVAL;
	}

	uint32_t unconfigured = sequence->channels & ~data->channels_mask;

	if (unconfigured) {
		LOG_ERR("Channel(s) 0x%08x not configured", unconfigured);
		return -EINVAL;
	}

	/* Validate oversampling */
	if (sequence->oversampling > 8) {
		LOG_ERR("Invalid oversampling: %d", sequence->oversampling);
		return -EINVAL;
	}

	/* Validate ADC resolution setting */
	if (sequence->resolution != 8 && sequence->resolution != 10 && sequence->resolution != 12) {
		LOG_ERR("Invalid resolution: %d", sequence->resolution);
		return -EINVAL;
	}

	int buffer_check = validate_buffer_size(sequence);

	if (buffer_check < 0) {
		return buffer_check;
	}

	/* Configure PDL Structure */
	psoc4_adc_configure_pdl(data, sequence, cfg);

	/* Initialize SAR Hardware */
	Cy_SAR_Disable(cfg->base);
	sar_status = Cy_SAR_Init(cfg->base, &data->pdl_sar_cfg);
	if (sar_status != CY_SAR_SUCCESS) {
		LOG_ERR("Failed to initialize SAR ADC: %d", sar_status);
		return -EIO;
	}

	Cy_SAR_Enable(cfg->base);
	Cy_SAR_SetInterruptMask(cfg->base, CY_SAR_INTR_EOS);

	/* Configure Analog Switches */
	psoc4_adc_set_switches(data, sequence, cfg);

	/* Start Conversion */
	data->channels = sequence->channels;
	data->buffer = sequence->buffer;

	adc_context_start_read(&data->ctx, sequence);
	return adc_context_wait_for_completion(&data->ctx);
}

static int psoc4_adc_read(const struct device *dev, const struct adc_sequence *sequence)
{
	int ret;
	struct psoc4_adc_data *data = dev->data;

	adc_context_lock(&data->ctx, false, NULL);
	ret = start_read(dev, sequence);
	adc_context_release(&data->ctx, ret);
	return ret;
}

#ifdef CONFIG_ADC_ASYNC
static int psoc4_adc_read_async(const struct device *dev, const struct adc_sequence *sequence,
				struct k_poll_signal *async)
{
	int ret;
	struct psoc4_adc_data *data = dev->data;

	adc_context_lock(&data->ctx, true, async);
	ret = start_read(dev, sequence);
	adc_context_release(&data->ctx, ret);

	return ret;
}
#endif

static void psoc4_adc_isr(const struct device *dev)
{
	struct psoc4_adc_data *data = dev->data;
	const struct psoc4_adc_config *cfg = dev->config;
	uint32_t intr;

	intr = Cy_SAR_GetInterruptStatus(cfg->base);

	if (!(intr & CY_SAR_INTR_EOS)) {
		return;
	}

	for (uint8_t ch = 0; ch < CY_SAR_SEQ_NUM_CHANNELS; ch++) {
		if (data->channels & BIT(ch)) {

			/* Get raw 16-bit ADC result (signed or unsigned) */
			int16_t raw = Cy_SAR_GetResult16(cfg->base, ch);
			uint16_t result;

			/* Process based on channel mode */
			if (data->channel_cfg[ch].differential) {

				/* DIFFERENTIAL MODE: Cast signed to unsigned directly */
				result = (uint16_t)(int16_t)raw;
			} else {

				/* SINGLE-ENDED MODE: Handle negative values (shouldn't occur) */
				if (raw < 0) {
					result = 0;
				} else {
					uint8_t res_bits = data->resolution[ch];
					uint16_t mask = (res_bits == 12) ? 0x0FFF /* 12-bit mask */
							: (res_bits == 10)
								? 0x03FF  /* 10-bit mask */
								: 0x00FF; /* 8-bit mask */
					result = (uint16_t)raw & mask;
				}
			}
			*data->buffer++ = result;
		}
	}

	Cy_SAR_ClearInterrupt(cfg->base, CY_SAR_INTR_EOS);
	adc_context_on_sampling_done(&data->ctx, dev);
}

/* ADC device driver initialization function */
static int psoc4_adc_init(const struct device *dev)
{
	struct psoc4_adc_data *data = dev->data;
	const struct psoc4_adc_config *cfg = dev->config;
	cy_en_sar_status_t sar_status;

	data->dev = dev;

	int rslt = ifx_cat1_utils_peri_pclk_assign_divider(cfg->clk_dst, &data->clock);

	if (rslt != CY_RSLT_SUCCESS) {
		return -EIO;
	}

	/* Initialize SAR configuration with defaults */
	data->pdl_sar_cfg = psoc4_sar_pdl_cfg_struct_default;

	/* Initialize SAR ADC */
	sar_status = Cy_SAR_Init(cfg->base, &data->pdl_sar_cfg);
	if (sar_status != CY_SAR_SUCCESS) {
		LOG_ERR("Failed to initialize SAR ADC: %d", sar_status);
		return -EIO;
	}

	Cy_SAR_SetInterruptMask(cfg->base, CY_SAR_INTR_EOS);

	/* Enable the SAR ADC hardware */
	Cy_SAR_Enable(cfg->base);

	cfg->irq_func(dev);
	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static const struct adc_driver_api adc_psoc4_driver_api = {
	.channel_setup = psoc4_adc_channel_setup,
	.read = psoc4_adc_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = psoc4_adc_read_async,
#endif
	.ref_internal = PSOC4_ADC_REF_INTERNAL_MV,
};

#define ADC_PERI_CLOCK_INIT(n)                                                                     \
	.clock = {                                                                                 \
		.block = IFX_CAT1_PERIPHERAL_GROUP_ADJUST(                                         \
			DT_PROP_BY_IDX(DT_INST_PHANDLE(n, clocks), peri_group, 1),                 \
			DT_INST_PROP_BY_PHANDLE(n, clocks, div_type)),                             \
		.channel = DT_INST_PROP_BY_PHANDLE(n, clocks, channel),                            \
	}

#define INFINEON_PSOC4_ADC_INIT(n)                                                                 \
	static void psoc4_adc_config_func_##n(const struct device *dev)                            \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), psoc4_adc_isr,              \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
	static struct psoc4_adc_data psoc4_adc_data_##n = {                                        \
		ADC_CONTEXT_INIT_TIMER(psoc4_adc_data_##n, ctx),                                   \
		ADC_CONTEXT_INIT_LOCK(psoc4_adc_data_##n, ctx),                                    \
		ADC_CONTEXT_INIT_SYNC(psoc4_adc_data_##n, ctx), ADC_PERI_CLOCK_INIT(n)};           \
	static const struct psoc4_adc_config psoc4_adc_cfg_##n = {                                 \
		.base = (SAR_Type *)DT_INST_REG_ADDR(n),                                           \
		.irq_func = psoc4_adc_config_func_##n,                                             \
		.vref_src = DT_INST_ENUM_IDX(n, vref_src),                                         \
		.vref_mv = DT_INST_PROP(n, vref_mv),                                               \
		.clk_dst = (en_clk_dst_t)DT_INST_PROP(n, clk_dst),                                 \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, psoc4_adc_init, NULL, &psoc4_adc_data_##n, &psoc4_adc_cfg_##n,    \
			      POST_KERNEL, CONFIG_ADC_INIT_PRIORITY, &adc_psoc4_driver_api);

DT_INST_FOREACH_STATUS_OKAY(INFINEON_PSOC4_ADC_INIT)

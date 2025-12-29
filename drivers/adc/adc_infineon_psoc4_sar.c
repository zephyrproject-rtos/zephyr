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

#include "cy_sar.h"
#include "cy_pdl.h"
#include "cy_gpio.h"

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(psoc4_adc, CONFIG_ADC_LOG_LEVEL);

#define PSOC4_ADC_NUM_CHANNELS    CY_SAR_SEQ_NUM_CHANNELS
#define PSOC4_ADC_REF_INTERNAL_MV 1200U

enum psoc4_adc_vref_source {
	PSOC4_ADC_VREF_INTERNAL = 0,
	PSOC4_ADC_VREF_VDDA = 1,
	PSOC4_ADC_VREF_VDDA_DIV_2 = 2,
	PSOC4_ADC_VREF_EXT = 3,
};

struct psoc4_adc_config {
	SAR_Type *base;
	uint8_t irq_priority;
	enum psoc4_adc_vref_source vref_source;
	uint32_t vref_mv;
	void (*irq_func)(const struct device *dev);
};

enum psoc4_adc_pin_routing {
	PSOC4_ADC_ROUTE_SARMUX,
	PSOC4_ADC_ROUTE_AMUXBUS_A,
	PSOC4_ADC_ROUTE_AMUXBUS_B,
#if (defined(CY_PASS0_SAR_EXPMUX_PRESENT) && (1U == CY_PASS0_SAR_EXPMUX_PRESENT))
	PSOC4_ADC_ROUTE_EXPMUX,
#endif
};

struct psoc4_adc_channel_cfg {
	bool differential;
	uint8_t vplus;
	uint8_t vminus;
	uint8_t reference;
	uint8_t sample_time_idx;
	enum psoc4_adc_pin_routing vplus_routing;
	enum psoc4_adc_pin_routing vminus_routing;
};

struct psoc4_adc_data {
	struct adc_context ctx;
	const struct device *dev;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	uint32_t channels;
	uint32_t channels_mask;
	uint8_t resolution[PSOC4_ADC_NUM_CHANNELS];
	struct psoc4_adc_channel_cfg channel_cfg[PSOC4_ADC_NUM_CHANNELS];
	uint32_t sample_times[4];
	cy_stc_sar_config_t pdl_sar_cfg;
	cy_stc_sar_channel_config_t channel_configs[PSOC4_ADC_NUM_CHANNELS];
};

const cy_stc_sar_config_t psoc4_sar_pdl_cfg_struct_default = {
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

static enum psoc4_adc_pin_routing psoc4_determine_routing(uint8_t pin)
{
	if (pin <= 7) {
		return PSOC4_ADC_ROUTE_SARMUX;
	}
#if (defined(CY_PASS0_SAR_EXPMUX_PRESENT) && (1U == CY_PASS0_SAR_EXPMUX_PRESENT))
	else if (pin >= 16 && pin <= 23) {
		return PSOC4_ADC_ROUTE_EXPMUX;
	}
#endif
	else {
		return (pin % 2 == 0) ? PSOC4_ADC_ROUTE_AMUXBUS_A : PSOC4_ADC_ROUTE_AMUXBUS_B;
	}
}

static uint32_t psoc4_get_gpio_port(uint8_t pin, enum psoc4_adc_pin_routing routing)
{
	switch (routing) {
	case PSOC4_ADC_ROUTE_SARMUX:
		return 2U;
	case PSOC4_ADC_ROUTE_AMUXBUS_A:
	case PSOC4_ADC_ROUTE_AMUXBUS_B:
		return 2U;
#if (defined(CY_PASS0_SAR_EXPMUX_PRESENT) && (1U == CY_PASS0_SAR_EXPMUX_PRESENT))
	case PSOC4_ADC_ROUTE_EXPMUX:
		return 2U;
#endif
	default:
		return 2U;
	}
}

static int psoc4_adc_channel_setup(const struct device *dev,
				   const struct adc_channel_cfg *channel_cfg)
{
	struct psoc4_adc_data *data = dev->data;
	uint8_t ch = channel_cfg->channel_id;

	if (ch >= PSOC4_ADC_NUM_CHANNELS) {
		return -EINVAL;
	}

	data->channel_cfg[ch].differential = channel_cfg->differential;
	data->channel_cfg[ch].vplus = channel_cfg->input_positive;
	data->channel_cfg[ch].vplus_routing = psoc4_determine_routing(channel_cfg->input_positive);

	if (data->channel_cfg[ch].vplus_routing == PSOC4_ADC_ROUTE_SARMUX) {
		uint32_t port = psoc4_get_gpio_port(channel_cfg->input_positive,
						    data->channel_cfg[ch].vplus_routing);
		GPIO_PRT_Type *port_base = Cy_GPIO_PortToAddr(port);

		Cy_GPIO_SetDrivemode(port_base, channel_cfg->input_positive, CY_GPIO_DM_ANALOG);
	} else if (data->channel_cfg[ch].vplus_routing == PSOC4_ADC_ROUTE_AMUXBUS_A ||
		   data->channel_cfg[ch].vplus_routing == PSOC4_ADC_ROUTE_AMUXBUS_B) {
		uint32_t port = psoc4_get_gpio_port(channel_cfg->input_positive,
						    data->channel_cfg[ch].vplus_routing);
		uint32_t gpio_pin = (channel_cfg->input_positive == 8)    ? 4
				    : (channel_cfg->input_positive == 9)  ? 5
				    : (channel_cfg->input_positive == 10) ? 4
				    : (channel_cfg->input_positive == 11) ? 5
									  : 4;
		GPIO_PRT_Type *port_base = Cy_GPIO_PortToAddr(port);
		en_hsiom_sel_t hsiom_sel =
			(data->channel_cfg[ch].vplus_routing == PSOC4_ADC_ROUTE_AMUXBUS_A)
				? (en_hsiom_sel_t)6
				: (en_hsiom_sel_t)7;
		Cy_GPIO_SetHSIOM(port_base, gpio_pin, hsiom_sel);
		Cy_GPIO_SetDrivemode(port_base, gpio_pin, CY_GPIO_DM_ANALOG);
	}

	data->channel_cfg[ch].vminus = channel_cfg->input_negative;
	data->channel_cfg[ch].vminus_routing = psoc4_determine_routing(channel_cfg->input_negative);

	if (channel_cfg->differential &&
	    data->channel_cfg[ch].vminus_routing == PSOC4_ADC_ROUTE_SARMUX) {
		uint32_t port = psoc4_get_gpio_port(channel_cfg->input_negative,
						    data->channel_cfg[ch].vminus_routing);
		GPIO_PRT_Type *port_base = Cy_GPIO_PortToAddr(port);

		Cy_GPIO_SetDrivemode(port_base, channel_cfg->input_negative, CY_GPIO_DM_ANALOG);
	}

	data->channel_cfg[ch].reference = channel_cfg->reference;

	uint32_t ticks = 0;

	if (channel_cfg->acquisition_time == ADC_ACQ_TIME_DEFAULT) {
		ticks = 3;
	} else if (ADC_ACQ_TIME_UNIT(channel_cfg->acquisition_time) == ADC_ACQ_TIME_TICKS) {
		ticks = ADC_ACQ_TIME_VALUE(channel_cfg->acquisition_time);
	} else {
		uint32_t ns = ADC_ACQ_TIME_VALUE(channel_cfg->acquisition_time) *
			      (ADC_ACQ_TIME_UNIT(channel_cfg->acquisition_time) ==
					       ADC_ACQ_TIME_MICROSECONDS
				       ? 1000
				       : 1);
		ticks = ns / 55;
	}
	if (ticks < 3) {
		ticks = 3;
	}
	if (ticks > 1023) {
		ticks = 1023;
	}

	uint8_t sample_time_idx = 0xFF;

	for (int i = 0; i < 4; i++) {
		if (data->sample_times[i] == ticks) {
			sample_time_idx = i;
			break;
		} else if (data->sample_times[i] == 0) {
			data->sample_times[i] = ticks;
			sample_time_idx = i;
			break;
		}
	}

	if (sample_time_idx == 0xFF) {
		LOG_ERR("No available sample time slots for requested acquisition time");
		return -EINVAL;
	}
	data->channel_cfg[ch].sample_time_idx = sample_time_idx;

	data->channels_mask |= BIT(ch);

	return 0;
}

static int validate_buffer_size(const struct adc_sequence *sequence)
{
	size_t active_channels = 0;
	size_t required_buffer_size;

	for (int i = 0; i < PSOC4_ADC_NUM_CHANNELS; i++) {
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
		cfg->avgShift = false;
		return;
	}

	cfg->avgShift = true;
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

static int start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct psoc4_adc_data *data = dev->data;
	const struct psoc4_adc_config *cfg = dev->config;
	uint32_t unconfigured = sequence->channels & ~data->channels_mask;

	if (sequence->channels == 0) {
		LOG_ERR("No channels selected");
		return -EINVAL;
	}

	if (unconfigured) {
		LOG_ERR("Channel(s) 0x%08x not configured", unconfigured);
		return -EINVAL;
	}

	if (sequence->resolution != 8 && sequence->resolution != 10 && sequence->resolution != 12) {
		LOG_ERR("Invalid resolution: %d", sequence->resolution);
		return -EINVAL;
	}

	int buffer_check = validate_buffer_size(sequence);

	if (buffer_check < 0) {
		return buffer_check;
	}

	data->pdl_sar_cfg = psoc4_sar_pdl_cfg_struct_default;

	uint32_t vref_mv = cfg->vref_mv;

	switch (cfg->vref_source) {
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

	data->pdl_sar_cfg.sampleTime0 = data->sample_times[0] ? data->sample_times[0] : 3;
	data->pdl_sar_cfg.sampleTime1 = data->sample_times[1] ? data->sample_times[1] : 3;
	data->pdl_sar_cfg.sampleTime2 = data->sample_times[2] ? data->sample_times[2] : 3;
	data->pdl_sar_cfg.sampleTime3 = data->sample_times[3] ? data->sample_times[3] : 3;

	psoc4_map_oversampling(&data->pdl_sar_cfg, sequence->oversampling);

	bool has_12bit = false;

	for (int ch = 0; ch < PSOC4_ADC_NUM_CHANNELS; ch++) {
		if ((sequence->channels & BIT(ch)) && sequence->resolution == 12) {
			has_12bit = true;
			break;
		}
	}

	if (has_12bit) {
		data->pdl_sar_cfg.subResolution = CY_SAR_SUB_RESOLUTION_10B;
	} else if (sequence->resolution == 10) {
		data->pdl_sar_cfg.subResolution = CY_SAR_SUB_RESOLUTION_10B;
	} else {
		data->pdl_sar_cfg.subResolution = CY_SAR_SUB_RESOLUTION_8B;
	}

	data->pdl_sar_cfg.chanEn = sequence->channels;

	for (int ch = 0; ch < PSOC4_ADC_NUM_CHANNELS; ch++) {
		if (!(sequence->channels & BIT(ch))) {
			data->pdl_sar_cfg.channelConfig[ch] = NULL;
			continue;
		}

		data->resolution[ch] = sequence->resolution;
		const struct psoc4_adc_channel_cfg *ch_cfg = &data->channel_cfg[ch];

		uint32_t sw_mask0 = 0;
#if (defined(CY_PASS0_SAR_EXPMUX_PRESENT) && (1U == CY_PASS0_SAR_EXPMUX_PRESENT))
		uint32_t sw_mask2 = 0;
#endif
		cy_en_sar_chan_config_port_pin_addr_t vplus_addr;
#if (defined(CY_IP_M0S8PASS4A_SAR_VERSION) && (4U <= CY_IP_M0S8PASS4A_SAR_VERSION))
		cy_en_sar_chan_config_neg_port_pin_addr_t vminus_addr = 0;
		bool vminus_addr_en = false;
#endif

		switch (ch_cfg->vplus_routing) {
		case PSOC4_ADC_ROUTE_SARMUX:
			if (ch_cfg->vplus > 7) {
				LOG_ERR("Invalid SARMUX pin number: %d", ch_cfg->vplus);
				return -EINVAL;
			}
			switch (ch_cfg->vplus) {
			case 0:
				sw_mask0 |= CY_SAR_MUX_FW_P0_VPLUS;
				vplus_addr = CY_SAR_ADDR_SARMUX_0;
				break;
			case 1:
				sw_mask0 |= CY_SAR_MUX_FW_P1_VPLUS;
				vplus_addr = CY_SAR_ADDR_SARMUX_1;
				break;
			case 2:
				sw_mask0 |= CY_SAR_MUX_FW_P2_VPLUS;
				vplus_addr = CY_SAR_ADDR_SARMUX_2;
				break;
			case 3:
				sw_mask0 |= CY_SAR_MUX_FW_P3_VPLUS;
				vplus_addr = CY_SAR_ADDR_SARMUX_3;
				break;
			case 4:
				sw_mask0 |= CY_SAR_MUX_FW_P4_VPLUS;
				vplus_addr = CY_SAR_ADDR_SARMUX_4;
				break;
			case 5:
				sw_mask0 |= CY_SAR_MUX_FW_P5_VPLUS;
				vplus_addr = CY_SAR_ADDR_SARMUX_5;
				break;
			case 6:
				sw_mask0 |= CY_SAR_MUX_FW_P6_VPLUS;
				vplus_addr = CY_SAR_ADDR_SARMUX_6;
				break;
			case 7:
				sw_mask0 |= CY_SAR_MUX_FW_P7_VPLUS;
				vplus_addr = CY_SAR_ADDR_SARMUX_7;
				break;
			default:
				LOG_ERR("Invalid V+ pin number: %d", ch_cfg->vplus);
				return -EINVAL;
			}
			break;

		case PSOC4_ADC_ROUTE_AMUXBUS_A:
			sw_mask0 |= CY_SAR_MUX_FW_AMUXBUSA_VPLUS;
			vplus_addr = CY_SAR_ADDR_SARMUX_AMUXBUS_A;
			break;

		case PSOC4_ADC_ROUTE_AMUXBUS_B:
			sw_mask0 |= CY_SAR_MUX_FW_AMUXBUSB_VPLUS;
			vplus_addr = CY_SAR_ADDR_SARMUX_AMUXBUS_B;
			break;

#if (defined(CY_PASS0_SAR_EXPMUX_PRESENT) && (1U == CY_PASS0_SAR_EXPMUX_PRESENT))
		case PSOC4_ADC_ROUTE_EXPMUX:
			if (ch_cfg->vplus < 16 || ch_cfg->vplus > 23) {
				LOG_ERR("Invalid EXPMUX pin number: %d (must be 16-23)",
					ch_cfg->vplus);
				return -EINVAL;
			}
			{
				uint8_t expmux_pin = ch_cfg->vplus - 16;

				switch (expmux_pin) {
				case 0:
					sw_mask2 |= CY_SAR_EXPMUX_FW_P0_VPLUS;
					vplus_addr = CY_SAR_ADDR_EXPMUX_0;
					break;
				case 1:
					sw_mask2 |= CY_SAR_EXPMUX_FW_P1_VPLUS;
					vplus_addr = CY_SAR_ADDR_EXPMUX_1;
					break;
				case 2:
					sw_mask2 |= CY_SAR_EXPMUX_FW_P2_VPLUS;
					vplus_addr = CY_SAR_ADDR_EXPMUX_2;
					break;
				case 3:
					sw_mask2 |= CY_SAR_EXPMUX_FW_P3_VPLUS;
					vplus_addr = CY_SAR_ADDR_EXPMUX_3;
					break;
				case 4:
					sw_mask2 |= CY_SAR_EXPMUX_FW_P4_VPLUS;
					vplus_addr = CY_SAR_ADDR_EXPMUX_4;
					break;
				case 5:
					sw_mask2 |= CY_SAR_EXPMUX_FW_P5_VPLUS;
					vplus_addr = CY_SAR_ADDR_EXPMUX_5;
					break;
				case 6:
					sw_mask2 |= CY_SAR_EXPMUX_FW_P6_VPLUS;
					vplus_addr = CY_SAR_ADDR_EXPMUX_6;
					break;
				case 7:
					sw_mask2 |= CY_SAR_EXPMUX_FW_P7_VPLUS;
					vplus_addr = CY_SAR_ADDR_EXPMUX_7;
					break;
				default:
					LOG_ERR("Invalid EXPMUX pin: %d", expmux_pin);
					return -EINVAL;
				}
			}
			break;
#endif

		default:
			LOG_ERR("Unsupported V+ routing type: %d", ch_cfg->vplus_routing);
			return -EINVAL;
		}

		if (ch_cfg->differential) {
			switch (ch_cfg->vminus_routing) {
			case PSOC4_ADC_ROUTE_SARMUX:
				if (ch_cfg->vminus > 7) {
					LOG_ERR("Invalid SARMUX V- pin number: %d", ch_cfg->vminus);
					return -EINVAL;
				}
				switch (ch_cfg->vminus) {
				case 0:
					sw_mask0 |= CY_SAR_MUX_FW_P0_VMINUS;
#if (defined(CY_IP_M0S8PASS4A_SAR_VERSION) && (4U <= CY_IP_M0S8PASS4A_SAR_VERSION))
					vminus_addr = CY_SAR_NEG_ADDR_SARMUX_0;
#endif
					break;
				case 1:
					sw_mask0 |= CY_SAR_MUX_FW_P1_VMINUS;
#if (defined(CY_IP_M0S8PASS4A_SAR_VERSION) && (4U <= CY_IP_M0S8PASS4A_SAR_VERSION))
					vminus_addr = CY_SAR_NEG_ADDR_SARMUX_1;
#endif
					break;
				case 2:
					sw_mask0 |= CY_SAR_MUX_FW_P2_VMINUS;
#if (defined(CY_IP_M0S8PASS4A_SAR_VERSION) && (4U <= CY_IP_M0S8PASS4A_SAR_VERSION))
					vminus_addr = CY_SAR_NEG_ADDR_SARMUX_2;
#endif
					break;
				case 3:
					sw_mask0 |= CY_SAR_MUX_FW_P3_VMINUS;
#if (defined(CY_IP_M0S8PASS4A_SAR_VERSION) && (4U <= CY_IP_M0S8PASS4A_SAR_VERSION))
					vminus_addr = CY_SAR_NEG_ADDR_SARMUX_3;
#endif
					break;
				case 4:
					sw_mask0 |= CY_SAR_MUX_FW_P4_VMINUS;
#if (defined(CY_IP_M0S8PASS4A_SAR_VERSION) && (4U <= CY_IP_M0S8PASS4A_SAR_VERSION))
					vminus_addr = CY_SAR_NEG_ADDR_SARMUX_4;
#endif
					break;
				case 5:
					sw_mask0 |= CY_SAR_MUX_FW_P5_VMINUS;
#if (defined(CY_IP_M0S8PASS4A_SAR_VERSION) && (4U <= CY_IP_M0S8PASS4A_SAR_VERSION))
					vminus_addr = CY_SAR_NEG_ADDR_SARMUX_5;
#endif
					break;
				case 6:
					sw_mask0 |= CY_SAR_MUX_FW_P6_VMINUS;
#if (defined(CY_IP_M0S8PASS4A_SAR_VERSION) && (4U <= CY_IP_M0S8PASS4A_SAR_VERSION))
					vminus_addr = CY_SAR_NEG_ADDR_SARMUX_6;
#endif
					break;
				case 7:
					sw_mask0 |= CY_SAR_MUX_FW_P7_VMINUS;
#if (defined(CY_IP_M0S8PASS4A_SAR_VERSION) && (4U <= CY_IP_M0S8PASS4A_SAR_VERSION))
					vminus_addr = CY_SAR_NEG_ADDR_SARMUX_7;
#endif
					break;
				default:
					LOG_ERR("Invalid V- pin number: %d", ch_cfg->vminus);
					return -EINVAL;
				}
#if (defined(CY_IP_M0S8PASS4A_SAR_VERSION) && (4U <= CY_IP_M0S8PASS4A_SAR_VERSION))
				vminus_addr_en = true;
#endif
				break;

			case PSOC4_ADC_ROUTE_AMUXBUS_A:
				sw_mask0 |= CY_SAR_MUX_FW_AMUXBUSA_VMINUS;
#if (defined(CY_IP_M0S8PASS4A_SAR_VERSION) && (4U <= CY_IP_M0S8PASS4A_SAR_VERSION))
				vminus_addr = CY_SAR_NEG_ADDR_SARMUX_AMUXBUS_A;
				vminus_addr_en = true;
#endif
				break;

			case PSOC4_ADC_ROUTE_AMUXBUS_B:
				sw_mask0 |= CY_SAR_MUX_FW_AMUXBUSB_VMINUS;
#if (defined(CY_IP_M0S8PASS4A_SAR_VERSION) && (4U <= CY_IP_M0S8PASS4A_SAR_VERSION))
				vminus_addr = CY_SAR_NEG_ADDR_SARMUX_AMUXBUS_B;
				vminus_addr_en = true;
#endif
				break;

#if (defined(CY_PASS0_SAR_EXPMUX_PRESENT) && (1U == CY_PASS0_SAR_EXPMUX_PRESENT))
			case PSOC4_ADC_ROUTE_EXPMUX:
				if (ch_cfg->vminus < 16 || ch_cfg->vminus > 23) {
					LOG_ERR("Invalid EXPMUX V- pin number: %d (must be 16-23)",
						ch_cfg->vminus);
					return -EINVAL;
				}
				{
					uint8_t expmux_pin = ch_cfg->vminus - 16;

					switch (expmux_pin) {
					case 0:
						sw_mask2 |= CY_SAR_EXPMUX_FW_P0_VMINUS;
#if (defined(CY_IP_M0S8PASS4A_SAR_VERSION) && (4U <= CY_IP_M0S8PASS4A_SAR_VERSION))
						vminus_addr = CY_SAR_NEG_ADDR_EXPMUX_0;
#endif
						break;
					case 1:
						sw_mask2 |= CY_SAR_EXPMUX_FW_P1_VMINUS;
#if (defined(CY_IP_M0S8PASS4A_SAR_VERSION) && (4U <= CY_IP_M0S8PASS4A_SAR_VERSION))
						vminus_addr = CY_SAR_NEG_ADDR_EXPMUX_1;
#endif
						break;
					case 2:
						sw_mask2 |= CY_SAR_EXPMUX_FW_P2_VMINUS;
#if (defined(CY_IP_M0S8PASS4A_SAR_VERSION) && (4U <= CY_IP_M0S8PASS4A_SAR_VERSION))
						vminus_addr = CY_SAR_NEG_ADDR_EXPMUX_2;
#endif
						break;
					case 3:
						sw_mask2 |= CY_SAR_EXPMUX_FW_P3_VMINUS;
#if (defined(CY_IP_M0S8PASS4A_SAR_VERSION) && (4U <= CY_IP_M0S8PASS4A_SAR_VERSION))
						vminus_addr = CY_SAR_NEG_ADDR_EXPMUX_3;
#endif
						break;
					case 4:
						sw_mask2 |= CY_SAR_EXPMUX_FW_P4_VMINUS;
#if (defined(CY_IP_M0S8PASS4A_SAR_VERSION) && (4U <= CY_IP_M0S8PASS4A_SAR_VERSION))
						vminus_addr = CY_SAR_NEG_ADDR_EXPMUX_4;
#endif
						break;
					case 5:
						sw_mask2 |= CY_SAR_EXPMUX_FW_P5_VMINUS;
#if (defined(CY_IP_M0S8PASS4A_SAR_VERSION) && (4U <= CY_IP_M0S8PASS4A_SAR_VERSION))
						vminus_addr = CY_SAR_NEG_ADDR_EXPMUX_5;
#endif
						break;
					case 6:
						sw_mask2 |= CY_SAR_EXPMUX_FW_P6_VMINUS;
#if (defined(CY_IP_M0S8PASS4A_SAR_VERSION) && (4U <= CY_IP_M0S8PASS4A_SAR_VERSION))
						vminus_addr = CY_SAR_NEG_ADDR_EXPMUX_6;
#endif
						break;
					case 7:
						sw_mask2 |= CY_SAR_EXPMUX_FW_P7_VMINUS;
#if (defined(CY_IP_M0S8PASS4A_SAR_VERSION) && (4U <= CY_IP_M0S8PASS4A_SAR_VERSION))
						vminus_addr = CY_SAR_NEG_ADDR_EXPMUX_7;
#endif
						break;
					default:
						LOG_ERR("Invalid EXPMUX V- pin: %d", expmux_pin);
						return -EINVAL;
					}
				}
#if (defined(CY_IP_M0S8PASS4A_SAR_VERSION) && (4U <= CY_IP_M0S8PASS4A_SAR_VERSION))
				vminus_addr_en = true;
#endif
				break;
#endif

			default:
				LOG_ERR("Unsupported V- routing type: %d", ch_cfg->vminus_routing);
				return -EINVAL;
			}
		} else {
			if (sw_mask0 == 0) {
				LOG_ERR("Channel %d: sw_mask0 is 0! Pin not configured", ch);
			}
		}

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

		if (ch_cfg->differential) {
			data->pdl_sar_cfg.differentialSigned = true;
		} else {
			data->pdl_sar_cfg.differentialSigned = false;
		}

		data->pdl_sar_cfg.channelConfig[ch] = &data->channel_configs[ch];
	}

	cy_en_sar_status_t sar_status = Cy_SAR_Init(cfg->base, &data->pdl_sar_cfg);

	if (sar_status != CY_SAR_SUCCESS) {
		LOG_ERR("Failed to initialize SAR ADC: %d", sar_status);
		return -EIO;
	}
	Cy_SAR_Enable(cfg->base);

	uint32_t total_sw_mask0 = 0;
#if (defined(CY_PASS0_SAR_EXPMUX_PRESENT) && (1U == CY_PASS0_SAR_EXPMUX_PRESENT))
	uint32_t total_sw_mask2 = 0;
#endif

	for (int ch = 0; ch < PSOC4_ADC_NUM_CHANNELS; ch++) {
		if (!(sequence->channels & BIT(ch))) {
			continue;
		}
		const struct psoc4_adc_channel_cfg *ch_cfg = &data->channel_cfg[ch];

		if (ch_cfg->vplus_routing == PSOC4_ADC_ROUTE_SARMUX) {
			switch (ch_cfg->vplus) {
			case 0:
				total_sw_mask0 |= CY_SAR_MUX_FW_P0_VPLUS;
				break;
			case 1:
				total_sw_mask0 |= CY_SAR_MUX_FW_P1_VPLUS;
				break;
			case 2:
				total_sw_mask0 |= CY_SAR_MUX_FW_P2_VPLUS;
				break;
			case 3:
				total_sw_mask0 |= CY_SAR_MUX_FW_P3_VPLUS;
				break;
			case 4:
				total_sw_mask0 |= CY_SAR_MUX_FW_P4_VPLUS;
				break;
			case 5:
				total_sw_mask0 |= CY_SAR_MUX_FW_P5_VPLUS;
				break;
			case 6:
				total_sw_mask0 |= CY_SAR_MUX_FW_P6_VPLUS;
				break;
			case 7:
				total_sw_mask0 |= CY_SAR_MUX_FW_P7_VPLUS;
				break;
			}
		} else if (ch_cfg->vplus_routing == PSOC4_ADC_ROUTE_AMUXBUS_A) {
			total_sw_mask0 |= CY_SAR_MUX_FW_AMUXBUSA_VPLUS;
		} else if (ch_cfg->vplus_routing == PSOC4_ADC_ROUTE_AMUXBUS_B) {
			total_sw_mask0 |= CY_SAR_MUX_FW_AMUXBUSB_VPLUS;
		}

		if (ch_cfg->differential) {
			if (ch_cfg->vminus_routing == PSOC4_ADC_ROUTE_SARMUX) {
				switch (ch_cfg->vminus) {
				case 0:
					total_sw_mask0 |= CY_SAR_MUX_FW_P0_VMINUS;
					break;
				case 1:
					total_sw_mask0 |= CY_SAR_MUX_FW_P1_VMINUS;
					break;
				case 2:
					total_sw_mask0 |= CY_SAR_MUX_FW_P2_VMINUS;
					break;
				case 3:
					total_sw_mask0 |= CY_SAR_MUX_FW_P3_VMINUS;
					break;
				case 4:
					total_sw_mask0 |= CY_SAR_MUX_FW_P4_VMINUS;
					break;
				case 5:
					total_sw_mask0 |= CY_SAR_MUX_FW_P5_VMINUS;
					break;
				case 6:
					total_sw_mask0 |= CY_SAR_MUX_FW_P6_VMINUS;
					break;
				case 7:
					total_sw_mask0 |= CY_SAR_MUX_FW_P7_VMINUS;
					break;
				}
			} else if (ch_cfg->vminus_routing == PSOC4_ADC_ROUTE_AMUXBUS_A) {
				total_sw_mask0 |= CY_SAR_MUX_FW_AMUXBUSA_VMINUS;
			} else if (ch_cfg->vminus_routing == PSOC4_ADC_ROUTE_AMUXBUS_B) {
				total_sw_mask0 |= CY_SAR_MUX_FW_AMUXBUSB_VMINUS;
			}
		}
	}

	uint32_t all_switches =
		CY_SAR_MUX_FW_P0_VPLUS | CY_SAR_MUX_FW_P1_VPLUS | CY_SAR_MUX_FW_P2_VPLUS |
		CY_SAR_MUX_FW_P3_VPLUS | CY_SAR_MUX_FW_P4_VPLUS | CY_SAR_MUX_FW_P5_VPLUS |
		CY_SAR_MUX_FW_P6_VPLUS | CY_SAR_MUX_FW_P7_VPLUS | CY_SAR_MUX_FW_AMUXBUSA_VPLUS |
		CY_SAR_MUX_FW_AMUXBUSB_VPLUS | CY_SAR_MUX_FW_P0_VMINUS | CY_SAR_MUX_FW_P1_VMINUS |
		CY_SAR_MUX_FW_P2_VMINUS | CY_SAR_MUX_FW_P3_VMINUS | CY_SAR_MUX_FW_P4_VMINUS |
		CY_SAR_MUX_FW_P5_VMINUS | CY_SAR_MUX_FW_P6_VMINUS | CY_SAR_MUX_FW_P7_VMINUS |
		CY_SAR_MUX_FW_AMUXBUSA_VMINUS | CY_SAR_MUX_FW_AMUXBUSB_VMINUS;
	Cy_SAR_SetAnalogSwitch(cfg->base, all_switches, false);

	if (total_sw_mask0 != 0) {
		Cy_SAR_SetAnalogSwitch(cfg->base, total_sw_mask0, true);
	}

	bool has_differential = false;

	for (int ch = 0; ch < PSOC4_ADC_NUM_CHANNELS; ch++) {
		if ((sequence->channels & BIT(ch)) && data->channel_cfg[ch].differential) {
			has_differential = true;
			break;
		}
	}

	if (has_differential) {
		Cy_SAR_SetAnalogSwitch(cfg->base, SAR_MUX_SWITCH0_MUX_FW_VSSA_VMINUS_Msk, false);
	} else {
		Cy_SAR_SetAnalogSwitch(cfg->base, SAR_MUX_SWITCH0_MUX_FW_VSSA_VMINUS_Msk, true);
	}

	for (int ch = 0; ch < PSOC4_ADC_NUM_CHANNELS; ch++) {
		if (!(sequence->channels & BIT(ch))) {
			continue;
		}

		if (data->channel_cfg[ch].vplus_routing == PSOC4_ADC_ROUTE_SARMUX) {
			uint32_t port = psoc4_get_gpio_port(data->channel_cfg[ch].vplus,
							    data->channel_cfg[ch].vplus_routing);
			GPIO_PRT_Type *port_base = Cy_GPIO_PortToAddr(port);
			uint32_t drivemode =
				Cy_GPIO_GetDrivemode(port_base, data->channel_cfg[ch].vplus);
			if (drivemode != CY_GPIO_DM_ANALOG) {
				Cy_GPIO_SetDrivemode(port_base, data->channel_cfg[ch].vplus,
						     CY_GPIO_DM_ANALOG);
			}
		} else if (data->channel_cfg[ch].vplus_routing == PSOC4_ADC_ROUTE_AMUXBUS_A ||
			   data->channel_cfg[ch].vplus_routing == PSOC4_ADC_ROUTE_AMUXBUS_B) {
			uint32_t port = psoc4_get_gpio_port(data->channel_cfg[ch].vplus,
							    data->channel_cfg[ch].vplus_routing);
			uint32_t gpio_pin = (data->channel_cfg[ch].vplus == 8)    ? 4
					    : (data->channel_cfg[ch].vplus == 9)  ? 5
					    : (data->channel_cfg[ch].vplus == 10) ? 4
					    : (data->channel_cfg[ch].vplus == 11) ? 5
										  : 4;
			GPIO_PRT_Type *port_base = Cy_GPIO_PortToAddr(port);

			en_hsiom_sel_t hsiom_sel =
				(data->channel_cfg[ch].vplus_routing == PSOC4_ADC_ROUTE_AMUXBUS_A)
					? (en_hsiom_sel_t)6
					: (en_hsiom_sel_t)7;
			Cy_GPIO_SetHSIOM(port_base, gpio_pin, hsiom_sel);
			uint32_t drivemode = Cy_GPIO_GetDrivemode(port_base, gpio_pin);

			if (drivemode != CY_GPIO_DM_ANALOG) {
				Cy_GPIO_SetDrivemode(port_base, gpio_pin, CY_GPIO_DM_ANALOG);
			}
		}

		if (data->channel_cfg[ch].differential &&
		    data->channel_cfg[ch].vminus_routing == PSOC4_ADC_ROUTE_SARMUX) {
			uint32_t port = psoc4_get_gpio_port(data->channel_cfg[ch].vminus,
							    data->channel_cfg[ch].vminus_routing);
			GPIO_PRT_Type *port_base = Cy_GPIO_PortToAddr(port);
			uint32_t drivemode =
				Cy_GPIO_GetDrivemode(port_base, data->channel_cfg[ch].vminus);
			if (drivemode != CY_GPIO_DM_ANALOG) {
				Cy_GPIO_SetDrivemode(port_base, data->channel_cfg[ch].vminus,
						     CY_GPIO_DM_ANALOG);
			}
		}
	}

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

	for (uint8_t ch = 0; ch < PSOC4_ADC_NUM_CHANNELS; ch++) {
		if (data->channels & BIT(ch)) {
			int16_t raw = Cy_SAR_GetResult16(cfg->base, ch);
			uint16_t result;

			if (data->channel_cfg[ch].differential) {
				result = (uint16_t)(int16_t)raw;
			} else {
				if (raw < 0) {
					result = 0;
				} else {
					uint8_t res_bits = data->resolution[ch];
					uint16_t mask = (res_bits == 12)   ? 0x0FFF
							: (res_bits == 10) ? 0x03FF
									   : 0x00FF;
					result = (uint16_t)raw & mask;
				}
			}
			*data->buffer++ = result;
		}
	}

	Cy_SAR_ClearInterrupt(cfg->base, CY_SAR_INTR_EOS);
	adc_context_on_sampling_done(&data->ctx, dev);
}

static int psoc4_adc_init(const struct device *dev)
{
	struct psoc4_adc_data *data = dev->data;
	const struct psoc4_adc_config *cfg = dev->config;
	cy_en_sar_status_t sar_status;

	data->dev = dev;

	Cy_SysClk_PeriphAssignDivider(PCLK_PASS0_CLOCK_SAR, CY_SYSCLK_DIV_16_BIT, 0U);
	Cy_SysClk_PeriphEnableDivider(CY_SYSCLK_DIV_16_BIT, 0U);

	data->pdl_sar_cfg = psoc4_sar_pdl_cfg_struct_default;
	sar_status = Cy_SAR_Init(cfg->base, &data->pdl_sar_cfg);
	if (sar_status != CY_SAR_SUCCESS) {
		LOG_ERR("Failed to initialize SAR ADC: %d", sar_status);
		return -EIO;
	}

	Cy_SAR_SetInterruptMask(cfg->base, CY_SAR_INTR_EOS);
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

#define PSOC4_ADC_VREF_MV_INIT(node_id)                                                            \
	[DT_REG_ADDR(node_id)] =                                                                   \
		DT_PROP_OR(node_id, zephyr_vref_mv, DT_PROP(DT_PARENT(node_id), vref_mv))

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
		ADC_CONTEXT_INIT_SYNC(psoc4_adc_data_##n, ctx),                                    \
	};                                                                                         \
	static const struct psoc4_adc_config psoc4_adc_cfg_##n = {                                 \
		.base = (SAR_Type *)DT_INST_REG_ADDR(n),                                           \
		.irq_priority = DT_INST_IRQ(n, priority),                                          \
		.irq_func = psoc4_adc_config_func_##n,                                             \
		.vref_source = DT_INST_ENUM_IDX(n, vref_source),                                   \
		.vref_mv = DT_INST_PROP(n, vref_mv),                                               \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, psoc4_adc_init, NULL, &psoc4_adc_data_##n, &psoc4_adc_cfg_##n,    \
			      POST_KERNEL, CONFIG_ADC_INIT_PRIORITY, &adc_psoc4_driver_api);

DT_INST_FOREACH_STATUS_OKAY(INFINEON_PSOC4_ADC_INIT)

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

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#include "cy_sar.h"
#include "cy_pdl.h"
#include "cy_gpio.h"
#include "cy_device.h"
#include "cyip_hsiom.h"

LOG_MODULE_REGISTER(psoc4_adc, CONFIG_ADC_LOG_LEVEL);

#define PSOC4_ADC_NUM_CHANNELS    CY_SAR_SEQ_NUM_CHANNELS
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

#define PSOC4_ADC_MAX_PIN_NUM        64U /* Board total pins 0-63 */
#define PSOC4_ADC_GPIO_PINS_PER_PORT 8U  /* 8 pins per port */
#define PSOC4_ADC_SARMUX_PIN_MAX     7U  /* SARMUX port 2, pins 0-7 */

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
};

enum psoc4_adc_pin_routing {
	PSOC4_ADC_ROUTE_SARMUX,
	PSOC4_ADC_ROUTE_AMUXBUS_A,
	PSOC4_ADC_ROUTE_AMUXBUS_B,
};

struct psoc4_adc_channel_cfg {
	bool differential;
	uint8_t vplus;
	uint8_t vminus;
	uint8_t reference;
	uint8_t sample_time_idx;
	enum psoc4_adc_pin_routing vplus_routing;
	enum psoc4_adc_pin_routing vminus_routing;
	uint32_t sw_mask;
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

/* Determine which routing method to use based on pin number */
static enum psoc4_adc_pin_routing psoc4_determine_routing(uint8_t pin)
{
	enum psoc4_adc_pin_routing routing;

	if (pin <= PSOC4_ADC_SARMUX_PIN_MAX) {
		routing = PSOC4_ADC_ROUTE_SARMUX;
	} else {
		/* AMUXBUS routing */
		routing = (pin % 2 == 0) ? PSOC4_ADC_ROUTE_AMUXBUS_A : PSOC4_ADC_ROUTE_AMUXBUS_B;
	}

	return routing;
}

/* Get GPIO port number based on pin number and routing method */
static uint32_t psoc4_get_gpio_port(uint8_t pin, enum psoc4_adc_pin_routing routing)
{
	uint32_t port;

	switch (routing) {
	case PSOC4_ADC_ROUTE_SARMUX:
		/* SARMUX pins are on port 2 */
		port = 2U;
		break;
	case PSOC4_ADC_ROUTE_AMUXBUS_A:
	case PSOC4_ADC_ROUTE_AMUXBUS_B:
		/* AMUXBUS pins */
		port = (pin / PSOC4_ADC_GPIO_PINS_PER_PORT);
		break;
	default:
		port = 2U;
		break;
	}

	return port;
}

/* Configure AMUXBUS splitter cells for proper analog signal routing */
static void psoc4_configure_amuxbus_splitter(enum psoc4_adc_pin_routing routing, uint32_t port)
{
	/* Select which AMUXBUS configuring A or B */
	cy_en_gpio_amuxselect_t amux_bus =
		(routing == PSOC4_ADC_ROUTE_AMUXBUS_A) ? CY_GPIO_AMUXBUSA : CY_GPIO_AMUXBUSB;

	/* Connect to global resource (GLR) - connects to ADC */
	cy_en_gpio_amuxconnect_t amux_connect = CY_GPIO_AMUX_GLR;

	/* Configure both splitter cells (0 and 1) for the selected AMUXBUS */
	for (cy_en_amux_split_t splitter_cell = AMUX_SPLIT_CTL_0; splitter_cell <= AMUX_SPLIT_CTL_1;
	     splitter_cell = (cy_en_amux_split_t)((uint32_t)splitter_cell + 1)) {

		/* Get current splitter configuration */
		cy_en_gpio_amuxconnect_t current_connect =
			Cy_GPIO_GetAmuxSplit(splitter_cell, amux_bus);

		/* Only update if different from desired configuration */
		if (current_connect != amux_connect) {
			Cy_GPIO_SetAmuxSplit(splitter_cell, amux_connect, amux_bus);
		}
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

	/* Channel validation */
	if (ch >= PSOC4_ADC_NUM_CHANNELS) {
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
	if (channel_cfg->input_positive >= PSOC4_ADC_MAX_PIN_NUM) {
		LOG_ERR("Channel %u: invalid input_positive %u", ch, channel_cfg->input_positive);
		return -EINVAL;
	}

	if (channel_cfg->differential && channel_cfg->input_negative >= PSOC4_ADC_MAX_PIN_NUM) {
		LOG_ERR("Channel %u: invalid input_negative %u", ch, channel_cfg->input_negative);
		return -EINVAL;
	}

	data->channel_cfg[ch].differential = channel_cfg->differential;

	data->channel_cfg[ch].vplus = channel_cfg->input_positive;
	data->channel_cfg[ch].vplus_routing = psoc4_determine_routing(channel_cfg->input_positive);

	if (data->channel_cfg[ch].vplus_routing == PSOC4_ADC_ROUTE_SARMUX) {

		if (channel_cfg->input_positive > PSOC4_ADC_SARMUX_PIN_MAX) {
			LOG_ERR("Channel %u: SARMUX pin %u > max %u", ch,
				channel_cfg->input_positive, PSOC4_ADC_SARMUX_PIN_MAX);
			return -EINVAL;
		}

		uint32_t port =
			psoc4_get_gpio_port(channel_cfg->input_positive, PSOC4_ADC_ROUTE_SARMUX);
		/* SARMUX pins are on port 2, pins 0-7 directly mapped */
		uint32_t gpio_pin = channel_cfg->input_positive;

		GPIO_PRT_Type *port_base = Cy_GPIO_PortToAddr(port);

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
		 *
		 * The switch mask bits enable the physical analog switches in the SAR MUX.
		 * These switches are controlled by the SAR_MUX_SWITCH registers.
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
			LOG_ERR("Channel %u: Invalid SARMUX pin %u in switch (global pin %u)", ch,
				gpio_pin, channel_cfg->input_positive);
			return -EINVAL;
		}

	} else if (data->channel_cfg[ch].vplus_routing == PSOC4_ADC_ROUTE_AMUXBUS_A ||
		   data->channel_cfg[ch].vplus_routing == PSOC4_ADC_ROUTE_AMUXBUS_B) {

		/* Calculate GPIO port and pin number within that port */
		uint32_t port = psoc4_get_gpio_port(channel_cfg->input_positive,
						    data->channel_cfg[ch].vplus_routing);
		uint32_t gpio_pin = channel_cfg->input_positive % PSOC4_ADC_GPIO_PINS_PER_PORT;

		GPIO_PRT_Type *port_base = Cy_GPIO_PortToAddr(port);

		if (!port_base) {
			LOG_ERR("Channel %u: Cannot get GPIO port %u for AMUXBUS pin %u", ch, port,
				channel_cfg->input_positive);
			return -EINVAL;
		}

		/*
		 * Configure AMUXBUS splitter cells to route analog signal to SAR ADC.
		 * The splitter connects the AMUXBUS line to the global resource (GLR)
		 * which routes to the SAR ADC input.
		 */
		psoc4_configure_amuxbus_splitter(data->channel_cfg[ch].vplus_routing, port);

		Cy_GPIO_SetDrivemode(port_base, gpio_pin, CY_GPIO_DM_ANALOG);

		/*
		 * Configure HSIOM (High-Speed I/O Matrix) to connect GPIO pin to AMUXBUS.
		 * HSIOM_SEL_AMUXA routes to AMUXBUS A, HSIOM_SEL_AMUXB routes to AMUXBUS B.
		 */
		en_hsiom_sel_t hsiom_sel =
			(data->channel_cfg[ch].vplus_routing == PSOC4_ADC_ROUTE_AMUXBUS_A)
				? HSIOM_SEL_AMUXA
				: HSIOM_SEL_AMUXB;
		Cy_GPIO_SetHSIOM(port_base, gpio_pin, hsiom_sel);

		sw_mask |= (data->channel_cfg[ch].vplus_routing == PSOC4_ADC_ROUTE_AMUXBUS_A)
				   ? CY_SAR_MUX_FW_AMUXBUSA_VPLUS
				   : CY_SAR_MUX_FW_AMUXBUSB_VPLUS;
	} else {
		LOG_ERR("Channel %u: Unknown routing type for positive input pin %u", ch,
			channel_cfg->input_positive);
		return -EINVAL;
	}

	/* configuration for Negative input (differential only) */
	if (channel_cfg->differential) {

		data->channel_cfg[ch].vminus = channel_cfg->input_negative;
		data->channel_cfg[ch].vminus_routing =
			psoc4_determine_routing(channel_cfg->input_negative);

		/* positive input and negative input must use same routing */
		if (data->channel_cfg[ch].vminus_routing != data->channel_cfg[ch].vplus_routing) {
			LOG_ERR("Channel %u: positive input and negative input routing mismatch",
				ch);
			return -EINVAL;
		}

		if (data->channel_cfg[ch].vminus_routing == PSOC4_ADC_ROUTE_SARMUX) {

			if (channel_cfg->input_negative > PSOC4_ADC_SARMUX_PIN_MAX) {
				LOG_ERR("Channel %u: SARMUX negative input pin %u > max %u", ch,
					channel_cfg->input_negative, PSOC4_ADC_SARMUX_PIN_MAX);
				return -EINVAL;
			}

			uint32_t port = psoc4_get_gpio_port(channel_cfg->input_negative,
							    PSOC4_ADC_ROUTE_SARMUX);
			/* SARMUX pins are on port 2, pins 0-7 directly mapped */
			uint32_t gpio_pin = channel_cfg->input_negative;

			GPIO_PRT_Type *port_base = Cy_GPIO_PortToAddr(port);

			if (!port_base) {
				LOG_ERR("Channel %u: Cannot get GPIO port for negative input pin "
					"%u",
					ch, channel_cfg->input_negative);
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
			default:
				LOG_ERR("Channel %u: Invalid SARMUX negative input pin %u in "
					"switch (global "
					"pin %u)",
					ch, gpio_pin, channel_cfg->input_negative);
				return -EINVAL;
			}
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

	for (int ch = 0; ch < PSOC4_ADC_NUM_CHANNELS; ch++) {
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
		cy_en_sar_chan_config_port_pin_addr_t vplus_addr;

		if (ch_cfg->vplus_routing == PSOC4_ADC_ROUTE_SARMUX) {
			if (ch_cfg->vplus > PSOC4_ADC_SARMUX_PIN_MAX) {
				LOG_ERR("Invalid SARMUX pin number: %d", ch_cfg->vplus);
				return;
			}
			vplus_addr = (cy_en_sar_chan_config_port_pin_addr_t)(CY_SAR_ADDR_SARMUX_0 +
									     ch_cfg->vplus);
		} else if (ch_cfg->vplus_routing == PSOC4_ADC_ROUTE_AMUXBUS_A) {
			vplus_addr = CY_SAR_ADDR_SARMUX_AMUXBUS_A;
		} else if (ch_cfg->vplus_routing == PSOC4_ADC_ROUTE_AMUXBUS_B) {
			vplus_addr = CY_SAR_ADDR_SARMUX_AMUXBUS_B;
		} else {
			LOG_ERR("Unsupported positive input routing type: %d",
				ch_cfg->vplus_routing);
			return;
		}

		/* Calculate SAR address for negative input (differential only) */
#if (defined(CY_IP_M0S8PASS4A_SAR_VERSION) && (4U <= CY_IP_M0S8PASS4A_SAR_VERSION))
		cy_en_sar_chan_config_neg_port_pin_addr_t vminus_addr = 0;
		bool vminus_addr_en = false;

		if (ch_cfg->differential) {
			if (ch_cfg->vminus_routing == PSOC4_ADC_ROUTE_SARMUX) {
				if (ch_cfg->vminus > PSOC4_ADC_SARMUX_PIN_MAX) {
					LOG_ERR("Invalid SARMUX negative input pin number: %d",
						ch_cfg->vminus);
					return;
				}
				uint32_t temp_addr = CY_SAR_NEG_ADDR_SARMUX_0 + ch_cfg->vminus;

				vminus_addr = (cy_en_sar_chan_config_neg_port_pin_addr_t)temp_addr;
				vminus_addr_en = true;
			} else if (ch_cfg->vminus_routing == PSOC4_ADC_ROUTE_AMUXBUS_A) {
				vminus_addr = CY_SAR_NEG_ADDR_SARMUX_AMUXBUS_A;
				vminus_addr_en = true;
			} else if (ch_cfg->vminus_routing == PSOC4_ADC_ROUTE_AMUXBUS_B) {
				vminus_addr = CY_SAR_NEG_ADDR_SARMUX_AMUXBUS_B;
				vminus_addr_en = true;
			} else {
				LOG_ERR("Unsupported negative input routing type: %d",
					ch_cfg->vminus_routing);
				return;
			}
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
	bool has_differential = false;

	/* Calculate total switch mask and check for differential channels */
	for (int ch = 0; ch < PSOC4_ADC_NUM_CHANNELS; ch++) {
		if (sequence->channels & BIT(ch)) {
			total_sw_mask |= data->channel_cfg[ch].sw_mask;
			if (data->channel_cfg[ch].differential) {
				has_differential = true;
			}
		}
	}

	/* Define all possible analog switches */
	uint32_t all_switches =
		CY_SAR_MUX_FW_P0_VPLUS | CY_SAR_MUX_FW_P1_VPLUS | CY_SAR_MUX_FW_P2_VPLUS |
		CY_SAR_MUX_FW_P3_VPLUS | CY_SAR_MUX_FW_P4_VPLUS | CY_SAR_MUX_FW_P5_VPLUS |
		CY_SAR_MUX_FW_P6_VPLUS | CY_SAR_MUX_FW_P7_VPLUS | CY_SAR_MUX_FW_AMUXBUSA_VPLUS |
		CY_SAR_MUX_FW_AMUXBUSB_VPLUS | CY_SAR_MUX_FW_P0_VMINUS | CY_SAR_MUX_FW_P1_VMINUS |
		CY_SAR_MUX_FW_P2_VMINUS | CY_SAR_MUX_FW_P3_VMINUS | CY_SAR_MUX_FW_P4_VMINUS |
		CY_SAR_MUX_FW_P5_VMINUS | CY_SAR_MUX_FW_P6_VMINUS | CY_SAR_MUX_FW_P7_VMINUS |
		CY_SAR_MUX_FW_AMUXBUSA_VMINUS | CY_SAR_MUX_FW_AMUXBUSB_VMINUS;

	/* Turn off all switches first */
	Cy_SAR_SetAnalogSwitch(cfg->base, all_switches, false);

	/* Turn on only the needed switches */
	if (total_sw_mask != 0) {
		Cy_SAR_SetAnalogSwitch(cfg->base, total_sw_mask, true);
	}

	/* Configure VSSA_VMINUS switch based on differential mode */
	if (has_differential) {
		Cy_SAR_SetAnalogSwitch(cfg->base, SAR_MUX_SWITCH0_MUX_FW_VSSA_VMINUS_Msk, false);
	} else {
		Cy_SAR_SetAnalogSwitch(cfg->base, SAR_MUX_SWITCH0_MUX_FW_VSSA_VMINUS_Msk, true);
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
	sar_status = Cy_SAR_Init(cfg->base, &data->pdl_sar_cfg);
	if (sar_status != CY_SAR_SUCCESS) {
		LOG_ERR("Failed to initialize SAR ADC: %d", sar_status);
		return -EIO;
	}

	Cy_SAR_Enable(cfg->base);

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

	for (uint8_t ch = 0; ch < PSOC4_ADC_NUM_CHANNELS; ch++) {
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

	/* Configure clock for SAR ADC peripheral */
	Cy_SysClk_PeriphAssignDivider(PCLK_PASS0_CLOCK_SAR, CY_SYSCLK_DIV_16_BIT, 0U);
	Cy_SysClk_PeriphEnableDivider(CY_SYSCLK_DIV_16_BIT, 0U);

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
		.irq_func = psoc4_adc_config_func_##n,                                             \
		.vref_src = DT_INST_ENUM_IDX(n, vref_src),                                         \
		.vref_mv = DT_INST_PROP(n, vref_mv),                                               \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, psoc4_adc_init, NULL, &psoc4_adc_data_##n, &psoc4_adc_cfg_##n,    \
			      POST_KERNEL, CONFIG_ADC_INIT_PRIORITY, &adc_psoc4_driver_api);

DT_INST_FOREACH_STATUS_OKAY(INFINEON_PSOC4_ADC_INIT)

/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_sar_adc

#include <zephyr/drivers/adc.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/clock_control/clock_control_ifx_cat1.h>
#include <infineon_kconfig.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#include "cy_sar.h"
#include "cy_pdl.h"
#include "cy_gpio.h"
#include "cy_device.h"
#include "cyip_hsiom.h"

LOG_MODULE_REGISTER(ifx_sar_adc, CONFIG_ADC_LOG_LEVEL);

#define IFX_SAR_REF_INTERNAL_MV 1200U

/* SAR ADC sample time limits (hardware defined) */
#define IFX_SAR_MIN_ACQ_TICKS 3U
#define IFX_SAR_MAX_ACQ_TICKS 1023U

/* Number of hardware sample time registers in the SAR ADC */
#define IFX_SAR_NUM_SAMPLE_TIMES 4U

/* Invalid sample time index */
#define IFX_SAR_INVALID_SAMPLE_TIME 0xFFU

/* SAR ADC supported resolutions (bits) */
#define IFX_SAR_RESOLUTION_8BIT  8U
#define IFX_SAR_RESOLUTION_10BIT 10U
#define IFX_SAR_RESOLUTION_12BIT 12U

/* SAR ADC resolution bit masks */
#define IFX_SAR_12BIT_MASK 0x0FFFU  /* 4095 - maximum 12-bit value */
#define IFX_SAR_10BIT_MASK 0x03FFU  /* 1023 - maximum 10-bit value */
#define IFX_SAR_8BIT_MASK  0x00FFU  /* 255 - maximum 8-bit value */

/* SAR v2 hardware compensation factor for single-ended unsigned mode */
#define IFX_SAR_V2_COMPENSATION_FACTOR 2U

#define IFX_SAR_MAX_PIN_NUM        64U /* Board total pins 0-63 */
#define IFX_SAR_GPIO_PINS_PER_PORT 8U  /* 8 pins per port */
#define IFX_SAR_SARMUX_PIN_MAX     7U  /* SARMUX port 2, pins 0-7 */
#define IFX_SAR_SARMUX_GPIO_PORT   2U  /* SARMUX dedicated GPIO port */

/* SARBUS routing addresses (CTB OpAmp outputs)
 *
 * This driver supports SARBUS routing by enabling the appropriate SAR MUX switches
 * to connect SARBUS0/SARBUS1 to the ADC input. However, the CTB (Continuous Time
 * Block) OpAmp configuration must be handled separately by the user application.
 *
 * Address format: Upper nibble = CTB block, Lower nibble = OpAmp output index
 * Addresses defined in cy_sar.h: CY_SAR_ADDR_CTBx_OAy (e.g., 0x13 = CTB0 OA1)
 */
#define IFX_SAR_SARBUS_CTB_SHIFT   4U
#define IFX_SAR_SARBUS_CTB_MASK    0xF0U
#define IFX_SAR_SARBUS_OA_MASK     0x0FU
#define IFX_SAR_SARBUS_CTB_MIN     1U   /* CTB blocks start at index 1 (0 is invalid) */
#define IFX_SAR_SARBUS_CTB_MAX     4U
#define IFX_SAR_SARBUS_OA_MIN      2U
#define IFX_SAR_SARBUS_OA_MAX      3U
#define IFX_SAR_SARBUS0_OA_IDX     2U   /* OpAmp output index for SARBUS0 (OA0) */
#define IFX_SAR_SARBUS1_OA_IDX     3U   /* OpAmp output index for SARBUS1 (OA1) */

/* Helper macro to check if address is a valid SARBUS (CTB OpAmp) address */
#define IFX_SAR_IS_SARBUS_ADDR(pin) \
	(((pin) >= ((1U << IFX_SAR_SARBUS_CTB_SHIFT) | IFX_SAR_SARBUS_OA_MIN)) && \
	 (((pin) & IFX_SAR_SARBUS_OA_MASK) >= IFX_SAR_SARBUS_OA_MIN) && \
	 (((pin) & IFX_SAR_SARBUS_OA_MASK) <= IFX_SAR_SARBUS_OA_MAX) && \
	 ((((pin) >> IFX_SAR_SARBUS_CTB_SHIFT) & IFX_SAR_SARBUS_OA_MASK) <= \
	 IFX_SAR_SARBUS_CTB_MAX))

enum ifx_sar_vref_src {
	IFX_SAR_VREF_INTERNAL = 0,
	IFX_SAR_VREF_VDDA = 1,
	IFX_SAR_VREF_VDDA_DIV_2 = 2,
	IFX_SAR_VREF_EXT = 3,
};

struct ifx_sar_config {
	SAR_Type *base;
	enum ifx_sar_vref_src vref_src;
	uint32_t vref_mv;
	en_clk_dst_t clk_dst;
	void (*irq_func)(const struct device *dev);
};

enum ifx_sar_pin_routing {
	IFX_SAR_ROUTE_SARMUX,
	IFX_SAR_ROUTE_SARBUS,  /* CTB OpAmp outputs */
};

struct ifx_sar_channel_cfg {
	bool differential;
	uint8_t vplus;
	uint8_t vminus;
	uint8_t reference;
	uint8_t sample_time_idx;
	enum ifx_sar_pin_routing vplus_routing;
	enum ifx_sar_pin_routing vminus_routing;
	uint32_t sw_mask;
};

struct ifx_sar_data {
	struct adc_context ctx;
	const struct device *dev;
	struct ifx_cat1_clock clock;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	uint32_t channels;
	uint32_t channels_mask;
	uint32_t configured_sequence_mask;  /* Track which sequence is currently configured */
	uint32_t configured_switch0;        /* Configured MUX_SWITCH0 value */
	uint32_t configured_hw_ctrl;        /* Configured MUX_SWITCH_HW_CTRL value */
	uint8_t resolution[CY_SAR_SEQ_NUM_CHANNELS];
	struct ifx_sar_channel_cfg channel_cfg[CY_SAR_SEQ_NUM_CHANNELS];
	uint32_t sample_times[IFX_SAR_NUM_SAMPLE_TIMES];
	cy_stc_sar_config_t pdl_sar_cfg;
	cy_stc_sar_channel_config_t channel_configs[CY_SAR_SEQ_NUM_CHANNELS];
};

static const cy_stc_sar_config_t ifx_sar_pdl_cfg_default = {
	.vrefSel = CY_SAR_VREF_SEL_BGR,
	.vrefBypCapEn = true,
	.vrefMvValue = IFX_SAR_REF_INTERNAL_MV,
	.negSel = CY_SAR_NEG_SEL_VSSA_KELVIN,
	.negVref = CY_SAR_NEGVREF_HW,  /* Enable hardware control of Vref switch */
	.power = CY_SAR_HALF_PWR,
	.singleEndedSigned = false,
	.differentialSigned = false,
	.trigMode = CY_SAR_TRIGGER_MODE_FW_ONLY,
	.eosEn = true,
	.sampleTime0 = IFX_SAR_MIN_ACQ_TICKS,
	.sampleTime1 = IFX_SAR_MIN_ACQ_TICKS,
	.sampleTime2 = IFX_SAR_MIN_ACQ_TICKS,
	.sampleTime3 = IFX_SAR_MIN_ACQ_TICKS,
};

static const cy_stc_sar_channel_config_t ifx_sar_default_channel_cfg = {
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
	struct ifx_sar_data *data = CONTAINER_OF(ctx, struct ifx_sar_data, ctx);
	const struct ifx_sar_config *config = data->dev->config;

	data->repeat_buffer = data->buffer;
	Cy_SAR_StartConvert(config->base, CY_SAR_START_CONVERT_SINGLE_SHOT);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct ifx_sar_data *data = CONTAINER_OF(ctx, struct ifx_sar_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

/* Determine which routing method to use based on pin number */
static int ifx_sar_determine_routing(uint8_t pin, enum ifx_sar_pin_routing *routing)
{
	if (pin <= IFX_SAR_SARMUX_PIN_MAX) {
		*routing = IFX_SAR_ROUTE_SARMUX;
		return 0;
	} else if (IFX_SAR_IS_SARBUS_ADDR(pin)) {
		/* CTB OpAmp outputs (SARBUS routing) */
		*routing = IFX_SAR_ROUTE_SARBUS;
		return 0;
	}

	LOG_ERR("Invalid pin number %u for routing determination", pin);
	return -EINVAL;
}

/* Get GPIO port number based on pin number and routing method */
static uint32_t ifx_sar_get_gpio_port(uint8_t pin, enum ifx_sar_pin_routing routing)
{
	/* SARMUX pins are on port 2 */
	ARG_UNUSED(pin);
	ARG_UNUSED(routing);
	return IFX_SAR_SARMUX_GPIO_PORT;
}

/* Convert Zephyr ADC acquisition time to hardware clock ticks */
static int ifx_sar_calculate_acq_ticks(const struct device *dev,
						uint32_t acquisition_time,
						uint32_t *result)
{
	const struct ifx_sar_config *cfg = dev->config;
	struct ifx_sar_data *data = dev->data;
	uint32_t ticks;

	if (acquisition_time == ADC_ACQ_TIME_DEFAULT) {
		/* Use minimum acquisition time (3 ticks) for default */
		*result = IFX_SAR_MIN_ACQ_TICKS;
		return 0;
	}

	if (ADC_ACQ_TIME_UNIT(acquisition_time) == ADC_ACQ_TIME_TICKS) {
		ticks = ADC_ACQ_TIME_VALUE(acquisition_time);
	} else {
		/* Query the SAR peripheral clock frequency */
		uint32_t freq_hz = ifx_cat1_utils_peri_pclk_get_frequency(
			cfg->clk_dst, &data->clock);

		if (freq_hz == 0) {
			LOG_ERR("Failed to get SAR ADC clock frequency");
			return -EIO;
		}

		uint32_t ns_per_tick = NSEC_PER_SEC / freq_hz;

		/* Convert nanoseconds/microseconds to ADC clock ticks */
		uint32_t ns =
			ADC_ACQ_TIME_VALUE(acquisition_time) *
			((ADC_ACQ_TIME_UNIT(acquisition_time) == ADC_ACQ_TIME_MICROSECONDS) ? 1000U
											    : 1U);

		/*
		 * Calculate ticks with ceiling division: (ns + ns_per_tick - 1) / ns_per_tick
		 * Rounds up to guarantee the minimum requested acquisition time.
		 */
		ticks = (ns + ns_per_tick - 1U) / ns_per_tick;
	}

	/*
	 * Clamp ticks to valid hardware range.
	 * SAR ADC hardware supports 3-1023 clock ticks for sampling.
	 */
	*result = CLAMP(ticks, IFX_SAR_MIN_ACQ_TICKS, IFX_SAR_MAX_ACQ_TICKS);
	return 0;
}

/* Find an available sample time slot (0-3) */
static uint8_t ifx_sar_find_sample_time_idx(struct ifx_sar_data *data, uint32_t ticks)
{
	/*
	 * Find an available sample time slot (0-3).
	 * The SAR ADC has 4 hardware sample time registers that can be shared between channels.
	 * This allows different channels to have different acquisition times.
	 */
	for (int i = 0; i < IFX_SAR_NUM_SAMPLE_TIMES; i++) {
		if (data->sample_times[i] == ticks || data->sample_times[i] == 0U) {
			data->sample_times[i] = ticks;
			return i; /* Return slot index (0-3) */
		}
	}

	/* No available slots found */
	return IFX_SAR_INVALID_SAMPLE_TIME;
}

static int ifx_sar_channel_setup(const struct device *dev,
				   const struct adc_channel_cfg *channel_cfg)
{
	struct ifx_sar_data *data = dev->data;
	const struct ifx_sar_config *cfg = dev->config;
	uint8_t ch = channel_cfg->channel_id;
	uint32_t sw_mask = 0;

	/* Channel validation */
	if (ch >= CY_SAR_SEQ_NUM_CHANNELS) {
		LOG_ERR("Invalid channel ID: %u", ch);
		return -EINVAL;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("Invalid gain: %d", channel_cfg->gain);
		return -EINVAL;
	}

	/*
	 * Validate that the channel reference matches the ADC block's vref-src.
	 * The SAR ADC has a single reference source for the entire block,
	 * so per-channel reference must be consistent with the block setting.
	 */
	enum ifx_sar_vref_src expected_vref;

	switch (channel_cfg->reference) {
	case ADC_REF_INTERNAL:
		expected_vref = IFX_SAR_VREF_INTERNAL;
		break;
	case ADC_REF_VDD_1:
		expected_vref = IFX_SAR_VREF_VDDA;
		break;
	case ADC_REF_VDD_1_2:
		expected_vref = IFX_SAR_VREF_VDDA_DIV_2;
		break;
	case ADC_REF_EXTERNAL0:
		expected_vref = IFX_SAR_VREF_EXT;
		break;
	default:
		LOG_ERR("Channel %u: Unsupported reference: %d", ch, channel_cfg->reference);
		return -EINVAL;
	}

	if (expected_vref != cfg->vref_src) {
		LOG_ERR("Channel %u: Reference mismatch - channel requests %d but ADC block "
			"is configured for vref-src %d", ch, channel_cfg->reference, cfg->vref_src);
		return -EINVAL;
	}

	/* Input validation will be done per routing type below */

	data->channel_cfg[ch].differential = channel_cfg->differential;

	data->channel_cfg[ch].vplus = channel_cfg->input_positive;
	int ret = ifx_sar_determine_routing(channel_cfg->input_positive,
					  &data->channel_cfg[ch].vplus_routing);
	if (ret < 0) {
		return ret;
	}

	if (data->channel_cfg[ch].vplus_routing == IFX_SAR_ROUTE_SARMUX) {

		if (channel_cfg->input_positive > IFX_SAR_SARMUX_PIN_MAX) {
			LOG_ERR("Channel %u: SARMUX pin %u > max %u", ch,
				channel_cfg->input_positive, IFX_SAR_SARMUX_PIN_MAX);
			return -EINVAL;
		}

		uint32_t port =
			ifx_sar_get_gpio_port(channel_cfg->input_positive, IFX_SAR_ROUTE_SARMUX);
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

	} else if (data->channel_cfg[ch].vplus_routing == IFX_SAR_ROUTE_SARBUS) {
		/*
		 * SARBUS routing - CTB OpAmp outputs to SAR ADC.
		 *
		 * SUPPORTED BY THIS DRIVER:
		 *   - Enables SAR MUX switches (SARBUS0/SARBUS1) to route OpAmp output to ADC
		 *   - Validates CTB/OpAmp address format (e.g., 0x13 = CTB0 OA1)
		 *   - Configures hardware switch control for SARBUS paths
		 *
		 * NOT SUPPORTED (user must configure separately):
		 *   - CTB OpAmp initialization and power control
		 *   - OpAmp input/output switch configuration
		 *   - GPIO pin routing to OpAmp inputs
		 *   - OpAmp gain and mode settings
		 *
		 * The user application or device tree must configure the CTB block before
		 * using SARBUS channels for ADC measurements.
		 */

		/* Determine which SARBUS to use based on address */
		uint8_t ctb_idx = (channel_cfg->input_positive >> IFX_SAR_SARBUS_CTB_SHIFT) &
				  IFX_SAR_SARBUS_OA_MASK;
		uint8_t oa_idx = channel_cfg->input_positive & IFX_SAR_SARBUS_OA_MASK;

		/* Validate CTB and OpAmp indices */
		if (ctb_idx < IFX_SAR_SARBUS_CTB_MIN || ctb_idx > IFX_SAR_SARBUS_CTB_MAX ||
		    oa_idx < IFX_SAR_SARBUS_OA_MIN || oa_idx > IFX_SAR_SARBUS_OA_MAX) {
			LOG_ERR("Channel %u: Invalid CTB/OpAmp address 0x%02x", ch,
				channel_cfg->input_positive);
			return -EINVAL;
		}

		/*
		 * Enable appropriate SARBUS switch in SAR MUX.
		 * Hardware constraint: SARBUS0 can only connect to positive terminal.
		 * SARBUS0: CTB OA with lower nibble 2 (OA0 output)
		 * SARBUS1: Reserved for negative terminal in differential mode
		 */
		if (oa_idx == IFX_SAR_SARBUS1_OA_IDX) {
			LOG_ERR("Channel %u: SARBUS1 can only connect to negative terminal in "
				"differential mode", ch);
			return -EINVAL;
		}

		if (oa_idx != IFX_SAR_SARBUS0_OA_IDX) {
			LOG_ERR("Channel %u: OpAmp index %u not supported", ch, oa_idx);
			return -EINVAL;
		}

		sw_mask |= CY_SAR_MUX_FW_SARBUS0_VPLUS;

	} else {
		LOG_ERR("Channel %u: Unknown routing type for positive input pin %u", ch,
			channel_cfg->input_positive);
		return -EINVAL;
	}

	/* configuration for Negative input (differential only) */
	if (channel_cfg->differential) {

		data->channel_cfg[ch].vminus = channel_cfg->input_negative;
		ret = ifx_sar_determine_routing(channel_cfg->input_negative,
					      &data->channel_cfg[ch].vminus_routing);
		if (ret < 0) {
			return ret;
		}

		/* Positive and negative inputs must use the same routing type */
		if (data->channel_cfg[ch].vplus_routing !=
			data->channel_cfg[ch].vminus_routing) {
			LOG_ERR("Channel %u: positive and negative routing mismatch", ch);
			return -EINVAL;
		}

		if (data->channel_cfg[ch].vminus_routing == IFX_SAR_ROUTE_SARMUX) {

			if (channel_cfg->input_negative > IFX_SAR_SARMUX_PIN_MAX) {
				LOG_ERR("Channel %u: SARMUX negative input pin %u > max %u", ch,
					channel_cfg->input_negative, IFX_SAR_SARMUX_PIN_MAX);
				return -EINVAL;
			}

			uint32_t port = ifx_sar_get_gpio_port(channel_cfg->input_negative,
							    IFX_SAR_ROUTE_SARMUX);
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

		} else if (data->channel_cfg[ch].vminus_routing == IFX_SAR_ROUTE_SARBUS) {

			/* Determine which SARBUS to use for negative input */
			uint8_t ctb_idx = (channel_cfg->input_negative >>
					  IFX_SAR_SARBUS_CTB_SHIFT) & IFX_SAR_SARBUS_OA_MASK;
			uint8_t oa_idx = channel_cfg->input_negative & IFX_SAR_SARBUS_OA_MASK;

			/* Validate CTB and OpAmp indices */
			if (ctb_idx < IFX_SAR_SARBUS_CTB_MIN ||
				ctb_idx > IFX_SAR_SARBUS_CTB_MAX ||
				oa_idx < IFX_SAR_SARBUS_OA_MIN ||
				oa_idx > IFX_SAR_SARBUS_OA_MAX) {
				LOG_ERR("Channel %u: Invalid CTB/OpAmp address for negative input "
					"0x%02x",
					ch, channel_cfg->input_negative);
				return -EINVAL;
			}

			/*
			 * Enable appropriate SARBUS switch for negative input.
			 * Hardware constraint: Only SARBUS1 can connect to negative terminal
			 * in differential mode. SARBUS0 is reserved for positive terminal.
			 */
			if (oa_idx == IFX_SAR_SARBUS0_OA_IDX) {
				LOG_ERR("Channel %u: SARBUS0 can only connect to positive terminal",
					ch);
				return -EINVAL;
			}

			if (oa_idx != IFX_SAR_SARBUS1_OA_IDX) {
				LOG_ERR("Channel %u: OpAmp index %u not supported for negative "
					"input",
					ch, oa_idx);
				return -EINVAL;
			}

			sw_mask |= CY_SAR_MUX_FW_SARBUS1_VMINUS;
		}
	}

	/* Check if any switches were configured */
	if (sw_mask == 0) {
		LOG_ERR("Channel %u: No SAR switches configured", ch);
		return -EINVAL;
	}

	data->channel_cfg[ch].sw_mask = sw_mask;
	data->channel_cfg[ch].reference = channel_cfg->reference;

	/* Configure acquisition time using hardware sample time registers */
	uint32_t ticks;
	int acq_ret = ifx_sar_calculate_acq_ticks(dev, channel_cfg->acquisition_time, &ticks);

	if (acq_ret < 0) {
		return acq_ret;
	}

	uint8_t sample_time_idx = ifx_sar_find_sample_time_idx(data, ticks);

	if (sample_time_idx == IFX_SAR_INVALID_SAMPLE_TIME) {
		LOG_ERR("Channel %u: No available sample time slots", ch);
		return -EINVAL;
	}

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

static void ifx_sar_map_oversampling(cy_stc_sar_config_t *cfg, uint8_t oversampling)
{
	if (oversampling == 0) {
		cfg->avgCnt = CY_SAR_AVG_CNT_2;

		/* No right shift of result */
		cfg->avgShift = false;
		return;
	}

	/* For oversampling > 0, enable result shifting (divide by N) */
	cfg->avgShift = true;

	/* Map Zephyr oversampling value to hardware averaging count */
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
static void ifx_sar_configure_reference(struct ifx_sar_data *data,
				      const struct ifx_sar_config *cfg)
{
	uint32_t vref_mv = cfg->vref_mv;

	switch (cfg->vref_src) {
	case IFX_SAR_VREF_INTERNAL:
		data->pdl_sar_cfg.vrefSel = CY_SAR_VREF_SEL_BGR;
		data->pdl_sar_cfg.vrefMvValue = IFX_SAR_REF_INTERNAL_MV;
		break;
	case IFX_SAR_VREF_VDDA:
		data->pdl_sar_cfg.vrefSel = CY_SAR_VREF_SEL_VDDA;
		data->pdl_sar_cfg.vrefMvValue = vref_mv;
		break;
	case IFX_SAR_VREF_VDDA_DIV_2:
		data->pdl_sar_cfg.vrefSel = CY_SAR_VREF_SEL_VDDA_DIV_2;
		data->pdl_sar_cfg.vrefMvValue = vref_mv / 2;
		break;
	case IFX_SAR_VREF_EXT:
		data->pdl_sar_cfg.vrefSel = CY_SAR_VREF_SEL_EXT;
		data->pdl_sar_cfg.vrefMvValue = vref_mv;
		break;
	default:
		LOG_ERR("Unsupported VREF source, using Internal");
		data->pdl_sar_cfg.vrefSel = CY_SAR_VREF_SEL_BGR;
		data->pdl_sar_cfg.vrefMvValue = IFX_SAR_REF_INTERNAL_MV;
		break;
	}
}

/* Configure ADC sample times */
static void ifx_sar_configure_sample_times(struct ifx_sar_data *data)
{
	data->pdl_sar_cfg.sampleTime0 = data->sample_times[0] ? data->sample_times[0] :
							 IFX_SAR_MIN_ACQ_TICKS;
	data->pdl_sar_cfg.sampleTime1 = data->sample_times[1] ? data->sample_times[1] :
							 IFX_SAR_MIN_ACQ_TICKS;
	data->pdl_sar_cfg.sampleTime2 = data->sample_times[2] ? data->sample_times[2] :
							 IFX_SAR_MIN_ACQ_TICKS;
	data->pdl_sar_cfg.sampleTime3 = data->sample_times[3] ? data->sample_times[3] :
							 IFX_SAR_MIN_ACQ_TICKS;
}

static int ifx_sar_configure_pdl(struct ifx_sar_data *data,
				   const struct adc_sequence *sequence,
				   const struct ifx_sar_config *cfg)
{
	bool has_differential = false;

	/* Initialize with defaults */
	data->pdl_sar_cfg = ifx_sar_pdl_cfg_default;

	/* Configure reference voltage */
	ifx_sar_configure_reference(data, cfg);

	/* Configure sample times */
	ifx_sar_configure_sample_times(data);

	/* Configure oversampling */
	ifx_sar_map_oversampling(&data->pdl_sar_cfg, sequence->oversampling);

	/* Configure channels */
	data->pdl_sar_cfg.chanEn = sequence->channels;

	for (int ch = 0; ch < CY_SAR_SEQ_NUM_CHANNELS; ch++) {
		if (!(sequence->channels & BIT(ch))) {
			data->pdl_sar_cfg.channelConfig[ch] = NULL;
			continue;
		}

		data->resolution[ch] = sequence->resolution;
		const struct ifx_sar_channel_cfg *ch_cfg = &data->channel_cfg[ch];

		/* Check for differential mode */
		if (ch_cfg->differential) {
			has_differential = true;
		}

		/* Calculate SAR address for positive input */
		cy_en_sar_chan_config_port_pin_addr_t vplus_addr;

		if (ch_cfg->vplus_routing == IFX_SAR_ROUTE_SARMUX) {
			if (ch_cfg->vplus > IFX_SAR_SARMUX_PIN_MAX) {
				LOG_ERR("Invalid SARMUX pin number: %d", ch_cfg->vplus);
				return -EINVAL;
			}
			vplus_addr = (cy_en_sar_chan_config_port_pin_addr_t)(CY_SAR_ADDR_SARMUX_0 +
									     ch_cfg->vplus);
		} else if (ch_cfg->vplus_routing == IFX_SAR_ROUTE_SARBUS) {
			/* SARBUS - use CTB OpAmp address directly */
			vplus_addr = (cy_en_sar_chan_config_port_pin_addr_t)ch_cfg->vplus;
		} else {
			LOG_ERR("Unsupported positive input routing type: %d",
				ch_cfg->vplus_routing);
			return -EINVAL;
		}

		/* Calculate SAR address for negative input (differential only) */
#if (defined(CY_IP_M0S8PASS4A_SAR_VERSION) && (4U <= CY_IP_M0S8PASS4A_SAR_VERSION))
		cy_en_sar_chan_config_neg_port_pin_addr_t vminus_addr = 0;
		bool vminus_addr_en = false;

		if (ch_cfg->differential) {
			if (ch_cfg->vminus_routing == IFX_SAR_ROUTE_SARMUX) {
				if (ch_cfg->vminus > IFX_SAR_SARMUX_PIN_MAX) {
					LOG_ERR("Invalid SARMUX negative input pin number: %d",
						ch_cfg->vminus);
					return -EINVAL;
				}
				uint32_t temp_addr = CY_SAR_NEG_ADDR_SARMUX_0 + ch_cfg->vminus;

				vminus_addr = (cy_en_sar_chan_config_neg_port_pin_addr_t)temp_addr;
				vminus_addr_en = true;
			} else {
				LOG_ERR("Unsupported negative input routing type: %d",
					ch_cfg->vminus_routing);
				return -EINVAL;
			}
		}
#endif

		/* Configure channel structure */
		data->channel_configs[ch] = ifx_sar_default_channel_cfg;
		data->channel_configs[ch].addr = vplus_addr;
		data->channel_configs[ch].differential = ch_cfg->differential;
		data->channel_configs[ch].avgEn = (sequence->oversampling > 0);
		data->channel_configs[ch].resolution =
			(sequence->resolution == IFX_SAR_RESOLUTION_12BIT) ?
				CY_SAR_MAX_RES : CY_SAR_SUB_RES;
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
	if (sequence->resolution == IFX_SAR_RESOLUTION_12BIT ||
		sequence->resolution == IFX_SAR_RESOLUTION_10BIT) {
		data->pdl_sar_cfg.subResolution = CY_SAR_SUB_RESOLUTION_10B;
	} else {
		data->pdl_sar_cfg.subResolution = CY_SAR_SUB_RESOLUTION_8B;
	}

	return 0;
}

static void ifx_sar_set_switches(struct ifx_sar_data *data, const struct adc_sequence *sequence,
				   const struct ifx_sar_config *cfg)
{
	/*
	 * DYNAMIC SWITCH CONFIGURATION
	 * Configure SAR MUX switches based on ALL channels that have been set up,
	 * not just the channels in the current sequence.
	 *
	 * This is critical because applications may read channels individually
	 * (sequence->channels = 0x1, then 0x2, etc.) rather than all at once.
	 * If we only enable switches for the current channel, residual charge from
	 * previous channels can bleed through, causing incorrect readings.
	 *
	 * Solution: Enable switches for ALL configured channels and keep them enabled.
	 * The hardware sequencer will properly isolate channels during sampling.
	 */

	/* Build switch mask for ALL configured channels (not just current sequence) */
	uint32_t all_channels_mask = data->channels_mask;  /* All channels that were set up */

	/* Check if switches are already configured for all channels */
	if (data->configured_sequence_mask == all_channels_mask) {
		/* Already configured - no need to reconfigure */
		return;
	}

	uint32_t combined_switch_mask = 0;
	uint32_t combined_hw_ctrl_mask = 0;

	/* Accumulate switch masks from ALL configured channels */
	for (int ch = 0; ch < CY_SAR_SEQ_NUM_CHANNELS; ch++) {
		if (all_channels_mask & BIT(ch)) {
			combined_switch_mask |= data->channel_cfg[ch].sw_mask;

			/* Add hardware control for this channel's switches */
			if (data->channel_cfg[ch].vplus_routing == IFX_SAR_ROUTE_SARMUX) {
				/* Enable hardware control for SARMUX pin */
				uint32_t pin = data->channel_cfg[ch].vplus;

				combined_hw_ctrl_mask |= BIT(pin);

				/* If differential, also enable hardware control for vminus */
				if (data->channel_cfg[ch].differential) {
					uint32_t vminus_pin = data->channel_cfg[ch].vminus;

					combined_hw_ctrl_mask |= BIT(vminus_pin);
				}
			} else if (data->channel_cfg[ch].vplus_routing == IFX_SAR_ROUTE_SARBUS) {
				/* Enable hardware control for SARBUS based on OpAmp output index */
				uint8_t oa_idx = data->channel_cfg[ch].vplus &
					IFX_SAR_SARBUS_OA_MASK;

				if (oa_idx == IFX_SAR_SARBUS0_OA_IDX) {
					combined_hw_ctrl_mask |=
						SAR_MUX_SWITCH_HW_CTRL_MUX_HW_CTRL_SARBUS0_Msk;
				} else if (oa_idx == IFX_SAR_SARBUS1_OA_IDX) {
					combined_hw_ctrl_mask |=
						SAR_MUX_SWITCH_HW_CTRL_MUX_HW_CTRL_SARBUS1_Msk;
				}
			}
		}
	}

	/* Always enable VSSA (ground reference) - required for all measurements */
	combined_switch_mask |= SAR_MUX_SWITCH0_MUX_FW_VSSA_VMINUS_Msk;
	combined_hw_ctrl_mask |= SAR_MUX_SWITCH_HW_CTRL_MUX_HW_CTRL_VSSA_Msk;

	/* Apply computed switch configuration */
	cfg->base->MUX_SWITCH0 = combined_switch_mask;
	cfg->base->MUX_SWITCH_HW_CTRL = combined_hw_ctrl_mask;

	/* Cache the configuration (using all_channels_mask, not sequence->channels) */
	data->configured_sequence_mask = all_channels_mask;
	data->configured_switch0 = combined_switch_mask;
	data->configured_hw_ctrl = combined_hw_ctrl_mask;
}

static int start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct ifx_sar_data *data = dev->data;
	const struct ifx_sar_config *cfg = dev->config;
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
	if (sequence->resolution != IFX_SAR_RESOLUTION_8BIT &&
		sequence->resolution != IFX_SAR_RESOLUTION_10BIT &&
		sequence->resolution != IFX_SAR_RESOLUTION_12BIT) {
		LOG_ERR("Invalid resolution: %d", sequence->resolution);
		return -EINVAL;
	}

	int buffer_check = validate_buffer_size(sequence);

	if (buffer_check < 0) {
		return buffer_check;
	}

	/* Configure PDL Structure */
	int pdl_ret = ifx_sar_configure_pdl(data, sequence, cfg);

	if (pdl_ret < 0) {
		return pdl_ret;
	}

	/*
	 * Configure analog switches before SAR initialization.
	 * The hardware sequencer requires switches to be set up before
	 * initializing channel configurations.
	 */
	ifx_sar_set_switches(data, sequence, cfg);

	/* Initialize SAR Hardware */
	Cy_SAR_Disable(cfg->base);
	sar_status = Cy_SAR_Init(cfg->base, &data->pdl_sar_cfg);
	if (sar_status != CY_SAR_SUCCESS) {
		LOG_ERR("Failed to initialize SAR ADC: %d", sar_status);
		return -EIO;
	}

	Cy_SAR_Enable(cfg->base);
	Cy_SAR_SetInterruptMask(cfg->base, CY_SAR_INTR_EOS);

	/* Start Conversion */
	data->channels = sequence->channels;
	data->buffer = sequence->buffer;

	adc_context_start_read(&data->ctx, sequence);
	return adc_context_wait_for_completion(&data->ctx);
}

static int ifx_sar_read(const struct device *dev, const struct adc_sequence *sequence)
{
	int ret;
	struct ifx_sar_data *data = dev->data;

	adc_context_lock(&data->ctx, false, NULL);
	ret = start_read(dev, sequence);
	adc_context_release(&data->ctx, ret);
	return ret;
}

#ifdef CONFIG_ADC_ASYNC
static int ifx_sar_read_async(const struct device *dev, const struct adc_sequence *sequence,
				struct k_poll_signal *async)
{
	int ret;
	struct ifx_sar_data *data = dev->data;

	adc_context_lock(&data->ctx, true, async);
	ret = start_read(dev, sequence);
	adc_context_release(&data->ctx, ret);

	return ret;
}
#endif

static void ifx_sar_isr(const struct device *dev)
{
	struct ifx_sar_data *data = dev->data;
	const struct ifx_sar_config *cfg = dev->config;
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
					uint16_t mask = (res_bits == IFX_SAR_RESOLUTION_12BIT)
								? IFX_SAR_12BIT_MASK
							: (res_bits == IFX_SAR_RESOLUTION_10BIT)
								? IFX_SAR_10BIT_MASK
								: IFX_SAR_8BIT_MASK;
					result = (uint16_t)raw & mask;
#if (defined(CY_IP_M0S8PASS4A_SAR_VERSION) && \
	 ((CY_IP_M0S8PASS4A_SAR_VERSION == 2U) || (CY_IP_M0S8PASS4A_SAR_VERSION == 3U)))
					/*
					 * SAR v2/v3 hardware limitation: single-ended channels
					 * always output signed 12-bit values (max 2047) regardless
					 * of the SINGLE_ENDED_SIGNED register bit setting. When
					 * configured for unsigned mode, compensate by doubling
					 * the raw value.
					 */
					if (!data->pdl_sar_cfg.singleEndedSigned) {
						result = result *
						IFX_SAR_V2_COMPENSATION_FACTOR;
					}
#endif
				}
			}
			*data->buffer++ = result;
		}
	}

	Cy_SAR_ClearInterrupt(cfg->base, CY_SAR_INTR_EOS);
	adc_context_on_sampling_done(&data->ctx, dev);
}

/* ADC device driver initialization function */
static int ifx_sar_init(const struct device *dev)
{
	struct ifx_sar_data *data = dev->data;
	const struct ifx_sar_config *cfg = dev->config;
	cy_en_sar_status_t sar_status;
	cy_en_sysclk_status_t clk_status;

	data->dev = dev;

	/* Assign peripheral clock divider to ADC */
	clk_status = ifx_cat1_utils_peri_pclk_assign_divider(cfg->clk_dst, &data->clock);
	if (clk_status != CY_SYSCLK_SUCCESS) {
		LOG_ERR("Failed to assign clock divider: %d", clk_status);
		return -EIO;
	}

	/* Initialize SAR configuration with defaults */
	data->pdl_sar_cfg = ifx_sar_pdl_cfg_default;

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

#define ADC_PERI_CLOCK_INIT(n)                                                                     \
	.clock = {                                                                                 \
		.block = DT_PROP(DT_INST_PHANDLE(n, clocks), div_type),                            \
		.channel = DT_PROP(DT_INST_PHANDLE(n, clocks), channel),                           \
	}

#define IFX_SAR_INIT(n)                                                                            \
	static void ifx_sar_config_func_##n(const struct device *dev)                              \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), ifx_sar_isr,                \
				DEVICE_DT_INST_GET(n), 0);                                         \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
	static struct ifx_sar_data ifx_sar_data_##n = {                                            \
		ADC_CONTEXT_INIT_TIMER(ifx_sar_data_##n, ctx),                                     \
		ADC_CONTEXT_INIT_LOCK(ifx_sar_data_##n, ctx),                                      \
		ADC_CONTEXT_INIT_SYNC(ifx_sar_data_##n, ctx),                                      \
		ADC_PERI_CLOCK_INIT(n)                                                             \
	};                                                                                         \
	static const struct ifx_sar_config ifx_sar_cfg_##n = {                                     \
		.base = (SAR_Type *)DT_INST_REG_ADDR(n),                                           \
		.irq_func = ifx_sar_config_func_##n,                                               \
		.vref_src = DT_INST_ENUM_IDX(n, vref_src),                                         \
		.vref_mv = DT_INST_PROP(n, vref_mv),                                               \
		.clk_dst = (en_clk_dst_t)DT_INST_PROP(n, clk_dst),                                 \
	};                                                                                         \
	static DEVICE_API(adc, ifx_sar_driver_api_##n) = {                                         \
		.channel_setup = ifx_sar_channel_setup,                                            \
		.read = ifx_sar_read,                                                              \
		COND_CODE_1(IS_ENABLED(CONFIG_ADC_ASYNC),                                          \
				(.read_async = ifx_sar_read_async,), ())                           \
		.ref_internal = COND_CODE_1(DT_INST_ENUM_HAS_VALUE(n, vref_src, vdda_by_2),        \
						(DT_INST_PROP(n, vref_mv) / 2),                    \
						(COND_CODE_1(DT_INST_ENUM_HAS_VALUE(n, vref_src,   \
											internal), \
							 (IFX_SAR_REF_INTERNAL_MV),                \
							 (DT_INST_PROP(n, vref_mv))))),            \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, ifx_sar_init, NULL, &ifx_sar_data_##n, &ifx_sar_cfg_##n,          \
				  POST_KERNEL, CONFIG_ADC_INIT_PRIORITY, &ifx_sar_driver_api_##n);

DT_INST_FOREACH_STATUS_OKAY(IFX_SAR_INIT)

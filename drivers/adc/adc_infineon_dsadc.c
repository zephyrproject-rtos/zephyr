/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_dsadc

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/clock_control/clock_control_ifx_cat1.h>
#include <zephyr/dt-bindings/adc/infineon-dsadc.h>
#include <infineon_kconfig.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#include "cy_dsadc.h"
#include "cy_device.h"

LOG_MODULE_REGISTER(ifx_dsadc, CONFIG_ADC_LOG_LEVEL);

#define IFX_DSADC_NUM_CHANNELS    PACSS_DCH_NR
#define IFX_DSADC_REF_INTERNAL_MV 1200U

/*
 * Decimator settings of the ModusToolbox PACSS current, voltage and external
 * temperature channels. The factory offset calibration is scaled for them.
 */
#define IFX_DSADC_DR1                  64U
#define IFX_DSADC_CURRENT_DR2          6U
#define IFX_DSADC_CURRENT_RR_SHIFTR    5U
#define IFX_DSADC_VOLTAGE_DR2          6U
#define IFX_DSADC_VOLTAGE_RR_SHIFTR    0U
#define IFX_DSADC_VOLTAGE_SHIFTL       3U
#define IFX_DSADC_TEMPERATURE_DR2      1U
#define IFX_DSADC_TEMPERATURE_RR_SHIFT 0U
#define IFX_DSADC_TEMPERATURE_SHIFTL   0U

#define IFX_DSADC_MIN_RESOLUTION 8U
#define IFX_DSADC_MAX_RESOLUTION 20U
#define IFX_DSADC_16BIT_MAX_RES  16U

/* Channel select delay applied by default, in modulator clocks */
#define IFX_DSADC_DEFAULT_CH_SEL_DLY 16U
#define IFX_DSADC_MAX_CH_SEL_DLY     255U

/* Factory calibration is evaluated at 25 degC (Q12.20 format) */
#define IFX_DSADC_CAL_TEMPERATURE (25 << CY_DSADC_TEMPERATURE_FRACTION_BITS)

/*
 * Gain correction is a fixed point number with 15 fractional bits. The default
 * value (8/7) is replaced by the factory calibration.
 */
#define IFX_DSADC_VALID_GCOR_BITS 15U
#define IFX_DSADC_DEFAULT_GCOR    0x924AU

/*
 * ACHAN0 calibration data is indexed by four times the analog gain, starting at 2X
 * (gain 1/2). ACHAN1 calibration data starts at 0.5X (gain 1/2).
 */
#define IFX_DSADC_ACHAN0_CAL_IDX_OFFSET 1U

/* The analog pump clock is sourced by the IMO */
#define IFX_DSADC_IMO_FREQ DT_PROP(DT_NODELABEL(clk_imo), clock_frequency)
BUILD_ASSERT((IFX_DSADC_IMO_FREQ == MHZ(24)) || (IFX_DSADC_IMO_FREQ == MHZ(48)),
	     "DSADC pump clock requires a 24 MHz or 48 MHz IMO");
#define IFX_DSADC_PUMP_CLOCK                                                                       \
	((IFX_DSADC_IMO_FREQ == MHZ(24)) ? CY_DSADC_ACHAN_PUMP_CLOCK_SOURCE_24MHZ                  \
					 : CY_DSADC_ACHAN_PUMP_CLOCK_SOURCE_48MHZ)

struct ifx_dsadc_config {
	PACSS_Type *base;
	en_clk_dst_t clk_dst;
	const struct pinctrl_dev_config *pcfg;
	void (*irq_func)(const struct device *dev);
	bool hpbgr_ext_cap;
};

struct ifx_dsadc_channel_cfg {
	cy_en_dsadc_dchan_achan_select_t achan;
	cy_en_dsadc_dchan_positive_pin_t pin_positive;
	cy_en_dsadc_dchan_negative_pin_t pin_negative;
	cy_en_dsadc_dchan_reference_vrefh_t vrefh;
	cy_en_dsadc_dchan_reference_vrefl_t vrefl;
	const struct ifx_dsadc_gain_level *gain_level;
	uint8_t cal_gain_idx;
	uint8_t ch_sel_dly;
	bool differential;
};

struct ifx_dsadc_data {
	struct adc_context ctx;
	const struct device *dev;
	struct ifx_cat1_clock clock;
	uint8_t *buffer;
	uint8_t *repeat_buffer;
	uint32_t channels;
	uint32_t pending;
	uint32_t configured_mask;
	bool sampling;
	uint8_t resolution;
	uint8_t sample_size;
	int32_t results[IFX_DSADC_NUM_CHANNELS];
	uint32_t max_count[IFX_DSADC_NUM_CHANNELS];
	struct ifx_dsadc_channel_cfg channel_cfg[IFX_DSADC_NUM_CHANNELS];
};

struct ifx_dsadc_gain_level {
	cy_en_dsadc_dchan_pga_gain_t pga_gain;
	uint8_t current_shiftl;
	cy_stc_dsadc_gain_mod_cap_config_t mod_cap;
};

/*
 * PGA and modulator gain split of each total gain (1/2 to 128 in powers of two),
 * and the current channel decimator left shift, as in the ModusToolbox PACSS
 * AGC gain levels.
 */
static const struct ifx_dsadc_gain_level ifx_dsadc_gain_levels[] = {
	{CY_DSADC_DCHAN_PGA_GAIN_1X, 8U, CY_DSADC_CONFIG_MOD_GAIN_0P5X},
	{CY_DSADC_DCHAN_PGA_GAIN_1X, 7U, CY_DSADC_CONFIG_MOD_GAIN_1X},
	{CY_DSADC_DCHAN_PGA_GAIN_1X, 6U, CY_DSADC_CONFIG_MOD_GAIN_2X},
	{CY_DSADC_DCHAN_PGA_GAIN_2X, 5U, CY_DSADC_CONFIG_MOD_GAIN_2X},
	{CY_DSADC_DCHAN_PGA_GAIN_4X, 4U, CY_DSADC_CONFIG_MOD_GAIN_2X},
	{CY_DSADC_DCHAN_PGA_GAIN_8X, 3U, CY_DSADC_CONFIG_MOD_GAIN_2X},
	{CY_DSADC_DCHAN_PGA_GAIN_16X, 2U, CY_DSADC_CONFIG_MOD_GAIN_2X},
	{CY_DSADC_DCHAN_PGA_GAIN_32X, 1U, CY_DSADC_CONFIG_MOD_GAIN_2X},
	{CY_DSADC_DCHAN_PGA_GAIN_32X, 0U, CY_DSADC_CONFIG_MOD_GAIN_4X},
};

/* Analog channel settings recommended by the ModusToolbox PACSS personality */
static const cy_stc_dsadc_achan_config_t ifx_dsadc_achan_cfg_default = {
	.aafCutoffFreq = CY_DSADC_ACHAN_AAF_CUTOFF_FREQ_30KHZ,
	.aafDisconnectCount = CY_DSADC_ACHAN_CIRCUIT_CHOPPING_CYCLE_2,
	.bufferPowerLevel = CY_DSADC_BUFFER_POWER_LEVEL_78_PERCENT,
	.compPower = CY_DSADC_ACHAN_MODULATOR_COMP_POWER_100,
	.datapathChoppingDivider = CY_DSADC_CHOPPING_CLOCK_DIVIDER_32,
	.demMode = CY_DSADC_DEM_MODE_DWA,
	.firstStagePower = CY_DSADC_ACHAN_MODULATOR_FIRST_STAGE_POWER_88,
	.modulatorChoppingDivider = CY_DSADC_CHOPPING_CLOCK_DIVIDER_32,
	.nonOverlapDelay = CY_DSADC_ACHAN_MODULATOR_NON_OVERLAP_DELAY_LOW,
	.pgaPowerLevel = CY_DSADC_PGA_POWER_LEVEL_58_PERCENT,
	.primaryTrigger = CY_DSADC_ACHAN_TRIGGER_FW,
	.secondaryTrigger = CY_DSADC_ACHAN_TRIGGER_FW,
	.secondThirdStagePower = CY_DSADC_ACHAN_MODULATOR_SECOND_THIRD_STAGE_POWER_100,
	.summerPower = CY_DSADC_ACHAN_MODULATOR_SUMMER_POWER_100,
	.syncPrimaryTrigger = CY_DSADC_ACHAN_TRIGGER_UNSYNCHRONIZED,
	.syncSecondaryTrigger = CY_DSADC_ACHAN_TRIGGER_UNSYNCHRONIZED,
	.pumpClock = IFX_DSADC_PUMP_CLOCK,
	.vcmPowerLevel = CY_DSADC_VCM_POWER_LEVEL_HIGH,
	.vrefPower = CY_DSADC_VREF_POWER_LEVEL_HIGH,
	.overloadDetectThr = 21U,
	.enableDem = true,
	.enableReset1 = false,
	.enableReset2 = false,
	.enableReset3 = true,
	.enableSecIntegrChopping = true,
	.useOverloadDetection = true,
};

/* Channel chopping of the current channel (ACHAN0), as in the ModusToolbox examples */
static const cy_stc_dsadc_achan_channel_chopping_config_t ifx_dsadc_achan0_chop_cfg = {
	.choppingType = CY_DSADC_ACHAN_CHOPPING_CROSS,
	.modbitSampleCount = 3U - 1U,
	.aafShortCount = 0U,
	.decimatorBlankCount = 0U,
};

static void ifx_dsadc_start_conversion(const struct device *dev)
{
	struct ifx_dsadc_data *data = dev->data;
	const struct ifx_dsadc_config *cfg = dev->config;
	uint32_t achan_mask = 0U;

	for (uint8_t ch = 0U; ch < IFX_DSADC_NUM_CHANNELS; ch++) {
		if (data->channels & BIT(ch)) {
			achan_mask |= BIT(data->channel_cfg[ch].achan);
		}
	}

	data->pending = data->channels;
	data->sampling = true;

	for (uint8_t achan = 0U; achan < PACSS_ACH_NR; achan++) {
		if (achan_mask & BIT(achan)) {
			Cy_DSADC_StartConversionAchan(&cfg->base->ACHAN[achan]);
		}
	}
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct ifx_dsadc_data *data = CONTAINER_OF(ctx, struct ifx_dsadc_data, ctx);

	data->repeat_buffer = data->buffer;
	ifx_dsadc_start_conversion(data->dev);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct ifx_dsadc_data *data = CONTAINER_OF(ctx, struct ifx_dsadc_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static bool ifx_dsadc_is_valid_positive(uint32_t pin)
{
	switch (pin) {
	case IFX_DSADC_POS_HVDIV0:
	case IFX_DSADC_POS_HVDIV1:
	case IFX_DSADC_POS_VTS:
	case IFX_DSADC_POS_VINT:
	case IFX_DSADC_POS_VSSA:
	case IFX_DSADC_POS_VSSD:
	case IFX_DSADC_POS_VSSL:
	case IFX_DSADC_POS_VREFL:
	case IFX_DSADC_POS_VTS_RET_K:
	case IFX_DSADC_POS_VINT_RET:
	case IFX_DSADC_POS_VSUB:
	case IFX_DSADC_POS_VCCD_DIV_2:
	case IFX_DSADC_POS_VCCHIB_DIV_2:
	case IFX_DSADC_POS_VDDA_DIV_4:
	case IFX_DSADC_POS_VDDD_DIV_4:
	case IFX_DSADC_POS_VREFH_1P2_HPBGR:
	case IFX_DSADC_POS_VREFH_0P8_HPBGR:
	case IFX_DSADC_POS_VREFH_0P7_HPBGR:
	case IFX_DSADC_POS_VREFH_1P2_SRSS:
	case IFX_DSADC_POS_GPIO0:
	case IFX_DSADC_POS_GPIO1:
	case IFX_DSADC_POS_GPIO2:
	case IFX_DSADC_POS_GPIO3:
	case IFX_DSADC_POS_GPIO4:
	case IFX_DSADC_POS_GPIO5:
	case IFX_DSADC_POS_GPIO6:
	case IFX_DSADC_POS_GPIO7:
	case IFX_DSADC_POS_RSH_0:
	case IFX_DSADC_POS_RSH_1:
	case IFX_DSADC_POS_AMUXA:
	case IFX_DSADC_POS_AMUXB:
		return true;
	default:
		return false;
	}
}

static bool ifx_dsadc_is_valid_negative(uint32_t pin)
{
	switch (pin) {
	case IFX_DSADC_NEG_VSSA:
	case IFX_DSADC_NEG_VREFL:
	case IFX_DSADC_NEG_SRSS_VSS:
	case IFX_DSADC_NEG_VDIV_RET:
	case IFX_DSADC_NEG_VTS_RET:
	case IFX_DSADC_NEG_VINT_RET:
	case IFX_DSADC_NEG_GPIO0:
	case IFX_DSADC_NEG_GPIO2:
	case IFX_DSADC_NEG_RSL_0:
	case IFX_DSADC_NEG_RSL_1:
	case IFX_DSADC_NEG_AMUXA:
	case IFX_DSADC_NEG_AMUXB:
		return true;
	default:
		return false;
	}
}

static bool ifx_dsadc_is_shunt_input(const struct ifx_dsadc_channel_cfg *ch)
{
	return (ch->pin_positive == (cy_en_dsadc_dchan_positive_pin_t)IFX_DSADC_POS_RSH_0) ||
	       (ch->pin_positive == (cy_en_dsadc_dchan_positive_pin_t)IFX_DSADC_POS_RSH_1) ||
	       (ch->pin_negative == (cy_en_dsadc_dchan_negative_pin_t)IFX_DSADC_NEG_RSL_0) ||
	       (ch->pin_negative == (cy_en_dsadc_dchan_negative_pin_t)IFX_DSADC_NEG_RSL_1);
}

static int ifx_dsadc_gain_to_idx(enum adc_gain gain, uint8_t *idx)
{
	switch (gain) {
	case ADC_GAIN_1_2:
		*idx = 0U;
		break;
	case ADC_GAIN_1:
		*idx = 1U;
		break;
	case ADC_GAIN_2:
		*idx = 2U;
		break;
	case ADC_GAIN_4:
		*idx = 3U;
		break;
	case ADC_GAIN_8:
		*idx = 4U;
		break;
	case ADC_GAIN_16:
		*idx = 5U;
		break;
	case ADC_GAIN_32:
		*idx = 6U;
		break;
	case ADC_GAIN_64:
		*idx = 7U;
		break;
	case ADC_GAIN_128:
		*idx = 8U;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/* The total gain is the PGA gain (1 to 32) multiplied by the modulator gain (0.5 to 4) */
static int ifx_dsadc_configure_gain(struct ifx_dsadc_channel_cfg *ch, enum adc_gain gain)
{
	uint8_t idx;

	if (ifx_dsadc_gain_to_idx(gain, &idx) != 0) {
		LOG_ERR("Unsupported gain %d", gain);
		return -EINVAL;
	}

	ch->gain_level = &ifx_dsadc_gain_levels[idx];

	if (ch->achan == CY_DSADC_DCHAN_SELECT_ACHAN0) {
		ch->cal_gain_idx = idx + IFX_DSADC_ACHAN0_CAL_IDX_OFFSET;
	} else {
		ch->cal_gain_idx = idx;
	}

	return 0;
}

static int ifx_dsadc_configure_reference(struct ifx_dsadc_channel_cfg *ch,
					 enum adc_reference reference)
{
	ch->vrefl = CY_DSADC_DCHAN_REFERENCE_VREFL_VREFL;

	switch (reference) {
	case ADC_REF_INTERNAL:
		ch->vrefh = CY_DSADC_DCHAN_REFERENCE_VREFH_DIRECT;
		break;
	case ADC_REF_VDD_1_3:
		ch->vrefh = CY_DSADC_DCHAN_REFERENCE_VREFH_VDDA_DIV_3;
		break;
	case ADC_REF_VDD_1_4:
		ch->vrefh = CY_DSADC_DCHAN_REFERENCE_VREFH_VDDA_DIV_4;
		break;
	case ADC_REF_EXTERNAL0:
		/* Ratiometric external temperature sensor reference (VTS_REF - VTS_RET) */
		if (ch->achan != CY_DSADC_DCHAN_SELECT_ACHAN1) {
			LOG_ERR("External reference is not supported on shunt inputs");
			return -EINVAL;
		}
		ch->vrefh = CY_DSADC_DCHAN_REFERENCE_VREFH_VTS_REF;
		ch->vrefl = CY_DSADC_DCHAN_REFERENCE_VREFL_VTS_RET;
		break;
	default:
		LOG_ERR("Unsupported reference %d", reference);
		return -EINVAL;
	}

	return 0;
}

static int ifx_dsadc_configure_acq_time(struct ifx_dsadc_channel_cfg *ch, uint16_t acq_time)
{
	if (acq_time == ADC_ACQ_TIME_DEFAULT) {
		ch->ch_sel_dly = IFX_DSADC_DEFAULT_CH_SEL_DLY;
		return 0;
	}

	if ((ADC_ACQ_TIME_UNIT(acq_time) != ADC_ACQ_TIME_TICKS) ||
	    (ADC_ACQ_TIME_VALUE(acq_time) > IFX_DSADC_MAX_CH_SEL_DLY)) {
		LOG_ERR("Unsupported acquisition time 0x%04x", acq_time);
		return -EINVAL;
	}

	ch->ch_sel_dly = (uint8_t)ADC_ACQ_TIME_VALUE(acq_time);

	return 0;
}

static int ifx_dsadc_channel_setup(const struct device *dev,
				   const struct adc_channel_cfg *channel_cfg)
{
	struct ifx_dsadc_data *data = dev->data;
	struct ifx_dsadc_channel_cfg ch = {0};
	uint8_t id = channel_cfg->channel_id;
	int ret;

	if (id >= IFX_DSADC_NUM_CHANNELS) {
		LOG_ERR("Invalid channel %u", id);
		return -EINVAL;
	}

	if (!ifx_dsadc_is_valid_positive(channel_cfg->input_positive)) {
		LOG_ERR("Invalid positive input 0x%02x", channel_cfg->input_positive);
		return -EINVAL;
	}

	ch.differential = channel_cfg->differential;
	ch.pin_positive = (cy_en_dsadc_dchan_positive_pin_t)channel_cfg->input_positive;

	if (ch.differential) {
		if (!ifx_dsadc_is_valid_negative(channel_cfg->input_negative)) {
			LOG_ERR("Invalid negative input 0x%02x", channel_cfg->input_negative);
			return -EINVAL;
		}
		ch.pin_negative = (cy_en_dsadc_dchan_negative_pin_t)channel_cfg->input_negative;
	} else {
		ch.pin_negative = CY_DSADC_DCHAN_NEGATIVE_PIN_VSSA;
	}

	ch.achan = ifx_dsadc_is_shunt_input(&ch) ? CY_DSADC_DCHAN_SELECT_ACHAN0
						 : CY_DSADC_DCHAN_SELECT_ACHAN1;

	ret = ifx_dsadc_configure_gain(&ch, channel_cfg->gain);
	if (ret < 0) {
		return ret;
	}

	ret = ifx_dsadc_configure_reference(&ch, channel_cfg->reference);
	if (ret < 0) {
		return ret;
	}

	ret = ifx_dsadc_configure_acq_time(&ch, channel_cfg->acquisition_time);
	if (ret < 0) {
		return ret;
	}

	data->channel_cfg[id] = ch;
	data->configured_mask |= BIT(id);

	LOG_DBG("Channel %u: ACHAN%u, pos 0x%02x, neg 0x%02x, %s", id, ch.achan, ch.pin_positive,
		ch.pin_negative, ch.differential ? "differential" : "single-ended");

	return 0;
}

static void ifx_dsadc_apply_calibration(PACSS_DCHAN_Type *dchan,
					const struct ifx_dsadc_channel_cfg *ch)
{
	if (ch->achan == CY_DSADC_DCHAN_SELECT_ACHAN0) {
		Cy_DSADC_CalibrateAchan0_2TempTrim(
			dchan, (cy_en_dsadc_achan0_2temp_trim_gain_type_t)ch->cal_gain_idx,
			IFX_DSADC_CAL_TEMPERATURE);
	} else if (ch->vrefh == CY_DSADC_DCHAN_REFERENCE_VREFH_VTS_REF) {
		Cy_DSADC_CalibrateAchan1_ExtTempTrim(dchan, IFX_DSADC_CAL_TEMPERATURE);
	} else {
		Cy_DSADC_CalibrateAchan1_2TempTrim(
			dchan, (cy_en_dsadc_achan1_2temp_trim_gain_type_t)ch->cal_gain_idx,
			IFX_DSADC_CAL_TEMPERATURE);
	}
}

/*
 * As in the ModusToolbox examples, the current channel (ACHAN0) keeps the anti-alias
 * filter connected and the voltage channels (ACHAN1) pre-charge it during the
 * channel select delay.
 */
static cy_en_dsadc_aaf_mode_t ifx_dsadc_aaf_mode(const struct ifx_dsadc_channel_cfg *ch)
{
	return (ch->achan == CY_DSADC_DCHAN_SELECT_ACHAN0) ? CY_DSADC_AAF_ENABLE
							   : CY_DSADC_AAF_SHORT_ON_SELECT;
}

static void ifx_dsadc_configure_dchan(const struct device *dev, uint8_t id)
{
	struct ifx_dsadc_data *data = dev->data;
	const struct ifx_dsadc_config *cfg = dev->config;
	const struct ifx_dsadc_channel_cfg *ch = &data->channel_cfg[id];
	PACSS_DCHAN_Type *dchan = &cfg->base->DCHAN[id];
	uint8_t dr2;
	uint8_t rr_shiftr;
	uint8_t shiftl;

	if (ch->achan == CY_DSADC_DCHAN_SELECT_ACHAN0) {
		dr2 = IFX_DSADC_CURRENT_DR2;
		rr_shiftr = IFX_DSADC_CURRENT_RR_SHIFTR;
		shiftl = ch->gain_level->current_shiftl;
	} else if (ch->vrefh == CY_DSADC_DCHAN_REFERENCE_VREFH_VTS_REF) {
		dr2 = IFX_DSADC_TEMPERATURE_DR2;
		rr_shiftr = IFX_DSADC_TEMPERATURE_RR_SHIFT;
		shiftl = IFX_DSADC_TEMPERATURE_SHIFTL;
	} else {
		dr2 = IFX_DSADC_VOLTAGE_DR2;
		rr_shiftr = IFX_DSADC_VOLTAGE_RR_SHIFTR;
		shiftl = IFX_DSADC_VOLTAGE_SHIFTL;
	}

	const cy_stc_dsadc_gain_level_config_t gain = {
		.offsetCorrection = 0,
		.gainCorrection = IFX_DSADC_DEFAULT_GCOR,
		.pgaGain = ch->gain_level->pga_gain,
		.leftShift = shiftl,
		.modCapConfig = ch->gain_level->mod_cap,
	};
	const cy_stc_dsadc_dchan_config_t dchan_cfg = {
		.gain = &gain,
		.fir = NULL,
		.rangeLowerThreshold = 0,
		.rangeUpperThreshold = 0,
		.achan = ch->achan,
		.conversionMode = CY_DSADC_DCHAN_SAMPLE_MODE_SINGLE_SHOT,
		.priority = CY_DSADC_DCHAN_PRIORITY_PRIMARY,
		.vrefhSelect = ch->vrefh,
		.vreflSelect = ch->vrefl,
		.pinPositive = ch->pin_positive,
		.pinNegative = ch->pin_negative,
		.aafMode = ifx_dsadc_aaf_mode(ch),
		.averagingMode = CY_DSADC_AVERAGING_MODE_AVERAGE,
		.averagingSamples = CY_DSADC_AVERAGING_SAMPLES_NONE,
		.rangeMode = CY_DSADC_DCHAN_RANGE_MODE_DISABLE,
		.rangeConditional = CY_DSADC_DCHAN_RANGE_CONDITIONAL_LESS_THAN,
		.sincMode = CY_DSADC_DCHAN_DECIMATOR_SINC_MODE_SINC3,
		.ppRightShift = 0U,
		.stage1DecimationRatio = IFX_DSADC_DR1 - 1U,
		.stage2DecimationRatio = dr2 - 1U,
		.rightShift = 0U,
		.rateReducerRightShift = rr_shiftr,
		.channelSelectDelay = ch->ch_sel_dly,
		.validGcorBits = IFX_DSADC_VALID_GCOR_BITS,
		.rangeCount = 0U,
		.useAgc = false,
		.enableGainCorrection = true,
		.enableOffsetCorrection = true,
	};

	data->max_count[id] = Cy_DSADC_CalcMaxCount(&dchan_cfg);

	Cy_DSADC_DisableDchan(dchan);
	Cy_DSADC_InitDchan(dchan, &dchan_cfg);
	ifx_dsadc_apply_calibration(dchan, ch);
	Cy_DSADC_ClearInterrupt(dchan, CY_DSADC_INTR_MASK);
	Cy_DSADC_SetInterruptMask(dchan, CY_DSADC_DCHAN_INTR_DATA_VAL);
	Cy_DSADC_EnableDchan(dchan);
}

static void ifx_dsadc_disable_dchan(const struct device *dev, uint8_t id)
{
	const struct ifx_dsadc_config *cfg = dev->config;
	PACSS_DCHAN_Type *dchan = &cfg->base->DCHAN[id];

	Cy_DSADC_DisableDchan(dchan);
	Cy_DSADC_SetInterruptMask(dchan, 0U);
	Cy_DSADC_ClearInterrupt(dchan, CY_DSADC_INTR_MASK);
}

static int validate_buffer_size(const struct adc_sequence *sequence, uint8_t sample_size)
{
	size_t required_buffer_size = POPCOUNT(sequence->channels) * sample_size;

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

static int start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct ifx_dsadc_data *data = dev->data;
	uint32_t unconfigured;
	uint8_t sample_size;
	int ret;

	if ((sequence->channels == 0U) ||
	    ((sequence->channels & ~BIT_MASK(IFX_DSADC_NUM_CHANNELS)) != 0U)) {
		LOG_ERR("Invalid channel selection 0x%08x", sequence->channels);
		return -EINVAL;
	}

	unconfigured = sequence->channels & ~data->configured_mask;
	if (unconfigured != 0U) {
		LOG_ERR("Channel(s) 0x%08x not configured", unconfigured);
		return -EINVAL;
	}

	if ((sequence->resolution < IFX_DSADC_MIN_RESOLUTION) ||
	    (sequence->resolution > IFX_DSADC_MAX_RESOLUTION)) {
		LOG_ERR("Invalid resolution: %d", sequence->resolution);
		return -EINVAL;
	}

	if (sequence->oversampling != 0U) {
		LOG_ERR("Invalid oversampling: %d", sequence->oversampling);
		return -EINVAL;
	}

	sample_size = (sequence->resolution > IFX_DSADC_16BIT_MAX_RES) ? sizeof(int32_t)
								       : sizeof(int16_t);

	ret = validate_buffer_size(sequence, sample_size);
	if (ret < 0) {
		return ret;
	}

	for (uint8_t ch = 0U; ch < IFX_DSADC_NUM_CHANNELS; ch++) {
		if (sequence->channels & BIT(ch)) {
			ifx_dsadc_configure_dchan(dev, ch);
		} else {
			ifx_dsadc_disable_dchan(dev, ch);
		}
	}

	data->channels = sequence->channels;
	data->resolution = sequence->resolution;
	data->sample_size = sample_size;
	data->buffer = sequence->buffer;

	adc_context_start_read(&data->ctx, sequence);
	return adc_context_wait_for_completion(&data->ctx);
}

static int ifx_dsadc_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct ifx_dsadc_data *data = dev->data;
	int ret;

	adc_context_lock(&data->ctx, false, NULL);
	ret = start_read(dev, sequence);
	adc_context_release(&data->ctx, ret);

	return ret;
}

#ifdef CONFIG_ADC_ASYNC
static int ifx_dsadc_read_async(const struct device *dev, const struct adc_sequence *sequence,
				struct k_poll_signal *async)
{
	struct ifx_dsadc_data *data = dev->data;
	int ret;

	adc_context_lock(&data->ctx, true, async);
	ret = start_read(dev, sequence);
	adc_context_release(&data->ctx, ret);

	return ret;
}
#endif

/*
 * Scale the result from the decimator full scale (Cy_DSADC_CalcMaxCount()) to the
 * requested resolution.
 */
static void ifx_dsadc_store_result(struct ifx_dsadc_data *data, uint8_t ch)
{
	uint8_t bits =
		data->channel_cfg[ch].differential ? (data->resolution - 1U) : data->resolution;
	int64_t scaled =
		((int64_t)data->results[ch] * (int64_t)BIT64(bits)) / (int64_t)data->max_count[ch];
	int32_t min = data->channel_cfg[ch].differential ? -(int32_t)BIT(bits) : 0;
	int32_t max = (int32_t)BIT(bits) - 1;
	int32_t result = (int32_t)CLAMP(scaled, min, max);

	if (data->sample_size == sizeof(int32_t)) {
		*(int32_t *)data->buffer = result;
	} else {
		*(uint16_t *)data->buffer = (uint16_t)result;
	}

	data->buffer += data->sample_size;
}

static void ifx_dsadc_isr(const struct device *dev)
{
	struct ifx_dsadc_data *data = dev->data;
	const struct ifx_dsadc_config *cfg = dev->config;

	for (uint8_t ch = 0U; ch < IFX_DSADC_NUM_CHANNELS; ch++) {
		PACSS_DCHAN_Type *dchan = &cfg->base->DCHAN[ch];
		uint32_t status = Cy_DSADC_GetInterruptStatusMasked(dchan);

		if (status == 0U) {
			continue;
		}

		Cy_DSADC_ClearInterrupt(dchan, status);

		if ((status & CY_DSADC_DCHAN_INTR_DATA_VAL) && (data->pending & BIT(ch))) {
			data->results[ch] = Cy_DSADC_GetResult(dchan);
			data->pending &= ~BIT(ch);
		}
	}

	if (!data->sampling || (data->pending != 0U)) {
		return;
	}

	data->sampling = false;

	for (uint8_t ch = 0U; ch < IFX_DSADC_NUM_CHANNELS; ch++) {
		if (data->channels & BIT(ch)) {
			ifx_dsadc_store_result(data, ch);
		}
	}

	adc_context_on_sampling_done(&data->ctx, dev);
}

static int ifx_dsadc_init(const struct device *dev)
{
	struct ifx_dsadc_data *data = dev->data;
	const struct ifx_dsadc_config *cfg = dev->config;
	const cy_stc_dsadc_hpbgr_config_t hpbgr_cfg = {
		.hpbgrChoppingDivider = CY_DSADC_CHOPPING_CLOCK_DIVIDER_4,
		.hpbgrChoppingPhase = CY_DSADC_HPBGR_CHOPPING_PHASE_NORMAL,
		.enableExternalCapacitor = cfg->hpbgr_ext_cap
						   ? CY_DSADC_HPBGR_EXTERNAL_CAPACITOR_PRESENT
						   : CY_DSADC_HPBGR_EXTERNAL_CAPACITOR_ABSENT,
	};
	cy_stc_dsadc_config_t pdl_cfg = {
		.ACHAN = {&ifx_dsadc_achan_cfg_default, &ifx_dsadc_achan_cfg_default},
		.ACHAN_CHOPPING = {&ifx_dsadc_achan0_chop_cfg, NULL},
		.HPBGR = &hpbgr_cfg,
	};
	cy_en_sysclk_status_t clk_status;
	int ret;

	data->dev = dev;

	if (cfg->pcfg != NULL) {
		ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			LOG_ERR("Failed to apply pinctrl state: %d", ret);
			return ret;
		}
	}

	clk_status = ifx_cat1_utils_peri_pclk_assign_divider(cfg->clk_dst, &data->clock);
	if (clk_status != CY_SYSCLK_SUCCESS) {
		LOG_ERR("Failed to assign clock divider: %d", clk_status);
		return -EIO;
	}

	for (uint8_t ch = 0U; ch < IFX_DSADC_NUM_CHANNELS; ch++) {
		ifx_dsadc_disable_dchan(dev, ch);
	}

	/* Enables the sequencer, HPBGR and both analog channels */
	Cy_DSADC_Init(cfg->base, &pdl_cfg);

	cfg->irq_func(dev);
	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#define ADC_PERI_CLOCK_INIT(n)                                                                     \
	.clock = {                                                                                 \
		.block = DT_PROP(DT_INST_PHANDLE(n, clocks), div_type),                            \
		.channel = DT_PROP(DT_INST_PHANDLE(n, clocks), channel),                           \
	}

#define IFX_DSADC_IRQ_CONNECT(idx, n)                                                              \
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, idx, irq), DT_INST_IRQ_BY_IDX(n, idx, priority),        \
		    ifx_dsadc_isr, DEVICE_DT_INST_GET(n), 0);                                      \
	irq_enable(DT_INST_IRQ_BY_IDX(n, idx, irq));

#define IFX_DSADC_INIT(n)                                                                          \
	IF_ENABLED(DT_INST_PINCTRL_HAS_IDX(n, 0), (PINCTRL_DT_INST_DEFINE(n);))                    \
	BUILD_ASSERT(DT_INST_NUM_IRQS(n) == IFX_DSADC_NUM_CHANNELS,                                \
		     "One interrupt per digital channel is required");                             \
	static void ifx_dsadc_config_func_##n(const struct device *dev)                            \
	{                                                                                          \
		LISTIFY(DT_INST_NUM_IRQS(n), IFX_DSADC_IRQ_CONNECT, (), n)                         \
	}                                                                                          \
	static struct ifx_dsadc_data ifx_dsadc_data_##n = {                                        \
		ADC_CONTEXT_INIT_TIMER(ifx_dsadc_data_##n, ctx),                                   \
		ADC_CONTEXT_INIT_LOCK(ifx_dsadc_data_##n, ctx),                                    \
		ADC_CONTEXT_INIT_SYNC(ifx_dsadc_data_##n, ctx),                                    \
		ADC_PERI_CLOCK_INIT(n)                                                             \
	};                                                                                         \
	static const struct ifx_dsadc_config ifx_dsadc_cfg_##n = {                                 \
		.base = (PACSS_Type *)DT_INST_REG_ADDR(n),                                         \
		.clk_dst = (en_clk_dst_t)DT_INST_PROP(n, clk_dst),                                 \
		.pcfg = COND_CODE_1(DT_INST_PINCTRL_HAS_IDX(n, 0),                                 \
				    (PINCTRL_DT_INST_DEV_CONFIG_GET(n)), (NULL)),                  \
		.irq_func = ifx_dsadc_config_func_##n,                                             \
		.hpbgr_ext_cap = DT_INST_PROP(n, hpbgr_external_capacitor),                        \
	};                                                                                         \
	static DEVICE_API(adc, ifx_dsadc_driver_api_##n) = {                                       \
		.channel_setup = ifx_dsadc_channel_setup,                                          \
		.read = ifx_dsadc_read,                                                            \
		COND_CODE_1(IS_ENABLED(CONFIG_ADC_ASYNC),                                          \
			    (.read_async = ifx_dsadc_read_async,), ())                             \
		.ref_internal = IFX_DSADC_REF_INTERNAL_MV,                                         \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, ifx_dsadc_init, NULL, &ifx_dsadc_data_##n, &ifx_dsadc_cfg_##n,    \
			      POST_KERNEL, CONFIG_ADC_INIT_PRIORITY, &ifx_dsadc_driver_api_##n);

DT_INST_FOREACH_STATUS_OKAY(IFX_DSADC_INIT)

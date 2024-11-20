
/*
 * Copyright (c) 2021-2024 Atmosic
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmosic_atmx3_adc

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adc_atm, CONFIG_ADC_LOG_LEVEL);

#include <zephyr/drivers/adc.h>
#include <soc.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#ifdef CONFIG_PM
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>
#endif

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#include "arch.h"
#include "at_wrpr.h"
#include "at_apb_pseq_regs_core_macro.h"
#include "at_apb_gadc_regs_core_macro.h"
#include "calibration.h"
#include "ll.h"
#include "pmu.h"
#include "pmu_top_regs_core_macro.h"
#include "pmu_swreg_regs_core_macro.h"
#include "pmu_gadc_regs_core_macro.h"
#include "timer.h"

#define ATM_GADC_RESOLUTION 11

/* Reference voltage values */
#define VOLT_3_3 3300
#define VOLT_1_8 1800

/* GADC internal reference voltage (Unit:mV) */
#define ATM_GADC_VREF_VOL VOLT_3_3

#define GADC_AVERAGE_AMOUNT 8

/** List of GADC channels */
typedef enum {
	UNUSED = 0,
	/** VBAT channel */
	VBATT = 1,
	/** VSTORE channel */
	VSTORE = 2,
	/** VDD1A channel */
	CORE = 3,
	/** Temperature channel */
	TEMP = 4,
	/// P4/P5 differential channel.
	PORT1_DIFFERENTIAL = 5,
	/// P6/P7 differential channel.
	PORT0_DIFFERENTIAL = 6,
	/// P4 single-ended channel.
	PORT1_SINGLE_ENDED_0 = 7,
	/// P5 single-ended channel.
	PORT1_SINGLE_ENDED_1 = 8,
	/// P6 single-ended channel.
	PORT0_SINGLE_ENDED_0 = 9,
	/// P7 single-ended channel.
	PORT0_SINGLE_ENDED_1 = 10,
	/// Li-ion channel.
	LI_ION_BATT = 11,
	/** Max channels */
	CHANNEL_NUM_MAX,
} GADC_CHANNEL_ID;

typedef enum {
	CH_TYPE_SINGLE_ENDED,
	CH_TYPE_DIFFERENTIAL,
	CH_TYPE_LI_ION,
	CH_TYPE_TEMPERATURE,
	CH_TYPE_MAX,
	CH_TYPE_INVALID,
} gadc_ch_type_t;

static gadc_ch_type_t const chmap[CHANNEL_NUM_MAX] = {
	CH_TYPE_INVALID, // unused, invalid channel
	CH_TYPE_SINGLE_ENDED, CH_TYPE_SINGLE_ENDED, CH_TYPE_SINGLE_ENDED, CH_TYPE_TEMPERATURE,
	CH_TYPE_DIFFERENTIAL, CH_TYPE_DIFFERENTIAL, CH_TYPE_SINGLE_ENDED, CH_TYPE_SINGLE_ENDED,
	CH_TYPE_SINGLE_ENDED, CH_TYPE_SINGLE_ENDED, CH_TYPE_LI_ION,
};

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
/** Calibration data type */
struct gadc_cal_s {
	union {
		/** 32 bits data which combine offset and gain */
		uint32_t value;
		/** Decomposition structure */
		struct {
			/** Double value of offset */
			int c0_x2 : 14;
			/** Mantissa part of gain */
			unsigned int c1_mantissa : 11;
			/** Exponent part of gain */
			int c1_exponent : 6;
			/** Sign part of gain */
			unsigned int c1_sign : 1;
		};
	};
};

/// FIFO data type
struct gadc_fifo_s {
	union {
		/// 32 bits data which represents full FIFO value
		uint32_t value;
		/// Decomposition structure.
		struct {
			/// Exponent part of FIFO data.
			int exponent : 5;
			/// Mantissa part of FIFO data.
			int sample_x2 : 12;
			/// Channel used for the FIFO
			unsigned int channel : 4;
			// Remaining 11 bits are zero from mask before assignment
		};
	};
};

BUILD_ASSERT(sizeof(struct gadc_cal_s) == 4, "wrong size");
/** Float data type */
typedef union {
	/** C float value */
	float value;
	/** Decomposition structure */
	struct {
		/** Mantissa part of float */
		unsigned int fraction : 23;
		/** Exponent part of float */
		int exponent : 8;
		/** Sign part of float */
		unsigned int sign : 1;
	} number;
} __ieee_float_shape_type;
#else
#error "Unsupported floating point endian"
#endif

struct gadc_atm_data {
	struct device const *dev;
	struct adc_context ctx;
	/** Current channel */
	uint32_t ch;
	/** Pending mask */
	uint32_t chmask;
	/** Active channels */
	size_t active_channels;
	/** Current results */
	uint16_t *buffer;
	/** Offset for the active channels */
	uint8_t offset[CHANNEL_NUM_MAX];
};
#define DEV_DATA(dev) ((struct gadc_atm_data *)(dev)->data)

static uint32_t chan_setup_mask;

typedef enum {
	GAIN_EXT_QUARTER,
	GAIN_EXT_HALF,
	GAIN_EXT_X1,
	GAIN_EXT_X2,
	GAIN_EXT_END,
	GAIN_EXT_MAX,
} gadc_gain_ext_t;

static gadc_gain_ext_t gext;
static gadc_gain_ext_t const gextmap[CHANNEL_NUM_MAX][GAIN_EXT_MAX] = {
	{GAIN_EXT_END}, // unused, invalid channel
	{GAIN_EXT_QUARTER, GAIN_EXT_HALF, GAIN_EXT_END},
	{GAIN_EXT_QUARTER, GAIN_EXT_HALF, GAIN_EXT_END},
	{GAIN_EXT_HALF, GAIN_EXT_END},
	{GAIN_EXT_X1, GAIN_EXT_END},
	{GAIN_EXT_QUARTER, GAIN_EXT_HALF, GAIN_EXT_X1, GAIN_EXT_X2, GAIN_EXT_END},
	{GAIN_EXT_QUARTER, GAIN_EXT_HALF, GAIN_EXT_X1, GAIN_EXT_X2, GAIN_EXT_END},
	{GAIN_EXT_QUARTER, GAIN_EXT_HALF, GAIN_EXT_END},
	{GAIN_EXT_QUARTER, GAIN_EXT_HALF, GAIN_EXT_END},
	{GAIN_EXT_QUARTER, GAIN_EXT_HALF, GAIN_EXT_END},
	{GAIN_EXT_QUARTER, GAIN_EXT_HALF, GAIN_EXT_END},
	{GAIN_EXT_X1, GAIN_EXT_END},
};
static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat)
{
	struct gadc_atm_data *data = CONTAINER_OF(ctx, struct gadc_atm_data, ctx);

	if (!repeat) {
		data->buffer += data->active_channels;
	}
}

/* Read GADC FIFO  and return channel measurement data */
static struct gadc_fifo_s gadc_read_ch_data(void)
{
	uint32_t data_output = CMSDK_GADC->DATAPATH_OUTPUT;
	struct gadc_fifo_s gadc_data = {
		.value = DGADC_DATAPATH_OUTPUT__DATA__READ(data_output),
	};

	return gadc_data;
}

/* Enable/Disable GADC analog side */
__INLINE void gadc_analog_control(bool enable)
{
	WRPR_CTRL_PUSH(CMSDK_PSEQ, WRPR_CTRL__CLK_ENABLE)
	{
		CMSDK_PSEQ->GADC_CONFIG = PSEQ_GADC_CONFIG__GADC_CUTVDD_B__MASK;
		if (enable) {
			/* Turn on GADC analog side */
			CMSDK_PSEQ->GADC_CONFIG = PSEQ_GADC_CONFIG__WRITE;
			//This delay was suggested by analog
			atm_timer_lpc_delay(2);
		} else {
			/* Turn off GADC analog side */
			CMSDK_PSEQ->GADC_CONFIG = 0;
		}
	}
	WRPR_CTRL_POP();
}

static void gadc_start_measurement(struct device const *dev, GADC_CHANNEL_ID ch)
{
	WRPR_CTRL_SET(CMSDK_GADC, WRPR_CTRL__CLK_ENABLE | WRPR_CTRL__CLK_SEL);

	gadc_analog_control(true);

	NVIC_EnableIRQ(DT_INST_IRQN(0));

	CMSDK_GADC->INTERRUPT_MASK = 0;
	CMSDK_GADC->INTERRUPT_CLEAR = DGADC_INTERRUPT_CLEAR__WRITE;
	CMSDK_GADC->INTERRUPT_CLEAR = 0;

	switch (ch) {
	case VBATT: {
		DGADC_GAIN_CONFIG0__CH1_GAIN_SEL__MODIFY(CMSDK_GADC->GAIN_CONFIG0, gext);
	} break;
	case VSTORE: {
		DGADC_GAIN_CONFIG0__CH2_GAIN_SEL__MODIFY(CMSDK_GADC->GAIN_CONFIG0, gext);
	} break;
	case CORE: {
		DGADC_GAIN_CONFIG0__CH3_GAIN_SEL__MODIFY(CMSDK_GADC->GAIN_CONFIG0, gext);
	} break;
	case TEMP: {
		DGADC_GAIN_CONFIG0__CH4_GAIN_SEL__MODIFY(CMSDK_GADC->GAIN_CONFIG0, gext);
	} break;
	case PORT1_DIFFERENTIAL: {
		DGADC_GAIN_CONFIG0__CH5_GAIN_SEL__MODIFY(CMSDK_GADC->GAIN_CONFIG0, gext);
	} break;
	case PORT0_DIFFERENTIAL: {
		DGADC_GAIN_CONFIG0__CH6_GAIN_SEL__MODIFY(CMSDK_GADC->GAIN_CONFIG0, gext);
	} break;
	case PORT1_SINGLE_ENDED_0: {
		DGADC_GAIN_CONFIG0__CH7_GAIN_SEL__MODIFY(CMSDK_GADC->GAIN_CONFIG0, gext);
	} break;
	case PORT1_SINGLE_ENDED_1: {
		DGADC_GAIN_CONFIG0__CH8_GAIN_SEL__MODIFY(CMSDK_GADC->GAIN_CONFIG0, gext);
	} break;
	case PORT0_SINGLE_ENDED_0: {
		DGADC_GAIN_CONFIG0__CH9_GAIN_SEL__MODIFY(CMSDK_GADC->GAIN_CONFIG0, gext);
	} break;
	case PORT0_SINGLE_ENDED_1: {
		DGADC_GAIN_CONFIG0__CH10_GAIN_SEL__MODIFY(CMSDK_GADC->GAIN_CONFIG0, gext);
	} break;
	case LI_ION_BATT: {
		DGADC_GAIN_CONFIG1__CH11_GAIN_SEL__MODIFY(CMSDK_GADC->GAIN_CONFIG1, gext);
		pmu_set_liion_measurement(true);
	} break;
	case UNUSED:
	case CHANNEL_NUM_MAX:
	default: {
		LOG_INF("Invalid channel: %d", ch);
		ASSERT_ERR(0);
	} break;
	}

	CMSDK_GADC->CTRL = DGADC_CTRL__WATCH_CHANNELS__WRITE(1 << ch) |
			   DGADC_CTRL__AVERAGING_AMOUNT__WRITE(
				   GADC_AVERAGE_AMOUNT) | /* (2^4 = 16 + 2 = 18 cycles) */
			   DGADC_CTRL__WAIT_AMOUNT__WRITE(0) |
			   DGADC_CTRL__MODE__WRITE(0); /* Continuous Mode */

	// Flush FIFO
	while (!(CMSDK_GADC->DATAPATH_OUTPUT & DGADC_DATAPATH_OUTPUT__EMPTY__MASK)) {
		YIELD();
	}

	/* Interrupt when complete (fifo overrun) */
	CMSDK_GADC->INTERRUPT_MASK = DGADC_INTERRUPT_MASK__MASK_INTRPT1__MASK;

	// Need to wait for analog side to settle before enabling digital datapath
	atm_timer_lpc_delay(1);

	DGADC_CTRL__ENABLE_DP__SET(CMSDK_GADC->CTRL);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct gadc_atm_data *data = CONTAINER_OF(ctx, struct gadc_atm_data, ctx);

	data->chmask = ctx->sequence.channels;
	data->ch = __builtin_ffs(data->chmask) - 1;
#ifdef CONFIG_PM
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
#endif
	/* Enable GADC analog side */
	gadc_analog_control(true);
	gadc_start_measurement(data->dev, data->ch);
}

static int gadc_atm_read_async(struct device const *dev, struct adc_sequence const *sequence,
			       struct k_poll_signal *async)
{
	struct gadc_atm_data *data = DEV_DATA(dev);

	if (!chan_setup_mask || chan_setup_mask & ~BIT_MASK(CHANNEL_NUM_MAX)) {
		LOG_ERR("Invalid selection of channels. Received: %#x", sequence->channels);
		return -EINVAL;
	}

	if (sequence->resolution != ATM_GADC_RESOLUTION) {
		LOG_ERR("Only %d bit resolution is supported. Received: %d", ATM_GADC_RESOLUTION,
			sequence->resolution);
		return -EINVAL;
	}

	data->active_channels = 0;
	for (int i = 0; i < CHANNEL_NUM_MAX; ++i) {
		if (sequence->channels & BIT(i)) {
			data->offset[i] = data->active_channels++;
		}
	}

	size_t exp_size = data->active_channels * sizeof(uint16_t);
	if (sequence->options) {
		exp_size *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < exp_size) {
		LOG_ERR("Required buffer size is %u. Received: %u", exp_size,
			sequence->buffer_size);
		return -ENOMEM;
	}

	data->buffer = sequence->buffer;

	if (async) {
		adc_context_lock(&data->ctx, true, async);
	} else {
		adc_context_lock(&data->ctx, false, async);
	}
	adc_context_start_read(&data->ctx, sequence);
	int ret = adc_context_wait_for_completion(&data->ctx);
	adc_context_release(&data->ctx, ret);

	return ret;
}

static int gadc_atm_read(struct device const *dev, struct adc_sequence const *sequence)
{
	return gadc_atm_read_async(dev, sequence, NULL);
}

static bool gadc_ext_valid(GADC_CHANNEL_ID ch, gadc_gain_ext_t gainext)
{
	for (int i = 0; (i < GAIN_EXT_MAX) && (gextmap[ch][i] != GAIN_EXT_END); i++) {
		if (gainext == gextmap[ch][i]) {
			return true;
		}
	}

	return false;
}

static int gadc_atm_channel_setup(struct device const *dev,
				  struct adc_channel_cfg const *channel_cfg)
{
	ARG_UNUSED(dev);

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("Selected GADC acquisition time is not valid");
		return -EINVAL;
	}

	if (channel_cfg->channel_id >= CHANNEL_NUM_MAX) {
		LOG_ERR("Channel %d is not valid", channel_cfg->channel_id);
		return -EINVAL;
	}

	switch (channel_cfg->gain) {
	case ADC_GAIN_1_4:
		gext = GAIN_EXT_QUARTER;
		break;
	case ADC_GAIN_1_2:
		gext = GAIN_EXT_HALF;
		break;
	case ADC_GAIN_1:
		gext = GAIN_EXT_X1;
		break;
	case ADC_GAIN_2:
		gext = GAIN_EXT_X2;
		break;
	default:
		LOG_ERR("Invalid channel gain");
		return -EINVAL;
	}

	if (!gadc_ext_valid(channel_cfg->channel_id, gext)) {
		LOG_ERR("Invalid gext (%d) for channel (%d)", gext, channel_cfg->channel_id);
		return -EINVAL;
	}

	if (channel_cfg->reference != ADC_REF_INTERNAL) {
		LOG_ERR("Invalid channel reference");
		return -EINVAL;
	}

	chan_setup_mask |= (1 << channel_cfg->channel_id);

	LOG_DBG("Channel (%#x) setup succeeded!", chan_setup_mask);
	return 0;
}

static struct adc_driver_api const api_atm_driver_api = {
	.channel_setup = gadc_atm_channel_setup,
	.read = gadc_atm_read,
	.read_async = gadc_atm_read_async,
	.ref_internal = ATM_GADC_VREF_VOL,
};

static uint16_t gadc_process_samples(struct device const *dev, GADC_CHANNEL_ID ch)
{
	CMSDK_GADC->CTRL = 0;

	// 4 bit channel + 12 bit number + 5 bit exponent = 21 bits
	struct gadc_fifo_s raw_fifo = gadc_read_ch_data();

	// Disable clocks between samples
	gadc_analog_control(false);

	int16_t sample_x2_signed = raw_fifo.sample_x2;
	int8_t sample_exponent = raw_fifo.exponent;

	float sample_scaling;
	if (sample_exponent >= 0) {
		sample_scaling = 1 << sample_exponent;
	} else {
		// Need to invert exponent sign in order to shift
		sample_scaling = 1.0f / (1 << (-sample_exponent));
	}

	// gadc channel output is a ramp that goes from
	// 3.0V (digital code: 309lsb) to 1.5V (digital-code: 629lsb)
	// in a sawtooth form.
	float result;
	ASSERT_ERR(ch && (ch < CHANNEL_NUM_MAX));

	if (ch == LI_ION_BATT) {
		// Disable for next sample. Is set in gadc_start_channel_measurement
		pmu_set_liion_measurement(false);
	}

	ASSERT_ERR(gext < GAIN_EXT_MAX);

	if (CAL_PRESENT(misc_cal, GADC_GAIN_OFFSET[chmap[ch]])) {
		struct gadc_cal_s const *gadc_cal =
			(struct gadc_cal_s *)&misc_cal.GADC_GAIN_OFFSET[chmap[ch]];
		int16_t offset_x2 = gadc_cal->c0_x2;
		unsigned int sign = gadc_cal->c1_sign;

		// Need to set sign as this is the negative portion of diff and gain
		// stored as positive for all single-ended channels
		if (ch == PORT1_SINGLE_ENDED_1 || ch == PORT0_SINGLE_ENDED_1) {
			sign = 1;
		}

		__ieee_float_shape_type gain = { .number.fraction = gadc_cal->c1_mantissa
								    << (23 - 11),
						 .number.exponent =
							 gadc_cal->c1_exponent + (127 - 31),
						 .number.sign = sign };

		LOG_INF("Found cal for gext %u, channel %u, offset_x2 %d, gain %f", gext, ch,
			offset_x2, (double)gain.value);

		float c1;
		// Divide by 2 for sample_x2_signed and offset_x2
		if (ch == TEMP || ch == LI_ION_BATT) {
			// For Li-Ion and Temp channels the Cal-data already accounts for
			// any gain multipliers
			c1 = gain.value / 2;
		} else {
			// Single and Diff channels need gext multiplier applied
			if (misc_cal.version <= 16) {
				// Previously calibrated with 1/4
				c1 = gain.value / (2 << gext);
			} else {
				// Now calibrated with x1
				c1 = (2 * gain.value) / (1 << gext);
			}
		}
		// result = C1*((D*E) + C0)
		//  D = Digital Output
		//  E = Exponent
		//  C0 = cal offset
		//  C1 = cal gain
		//  sample_x2_signed and offset_x2 represent twice the values
		result = c1 * ((sample_x2_signed * sample_scaling) + offset_x2);

	} else {
		// result = C1*((D*E) + C0)
		// C0 nominal = 0, C1 nominal = 1.0
		// Divide by 2 for sample_x2_signed and offset_x2
		float c1_nominal;
		switch (ch) {
		case TEMP: {
			c1_nominal = 0.39879f / 2;
		} break;
		case LI_ION_BATT: {
			c1_nominal = 0.00586f / 2;
		} break;
		case PORT1_SINGLE_ENDED_1: // fall-through
		case PORT0_SINGLE_ENDED_1: {
			// Need to invert sign
			c1_nominal = -0.00391f / (2 << gext);
		} break;
		case CORE: // fall-through
		case VBATT: // fall-through
		case VSTORE: // fall-through
		case PORT1_DIFFERENTIAL: // fall-through
		case PORT0_DIFFERENTIAL: // fall-through
		case PORT1_SINGLE_ENDED_0: // fall-through
		case PORT0_SINGLE_ENDED_0: // fall-through
		case CHANNEL_NUM_MAX: // fall-through
		default: {
			// Extra divide by 2 for sample_x2_signed
			c1_nominal = 0.00391f / (2 << gext);
		} break;
		}

		result = c1_nominal * (sample_x2_signed * sample_scaling);
	}
	LOG_INF("raw: %x, samplie_x2: %u, result: %f V", raw_fifo.value, sample_x2_signed,
		(double)result);

	return (uint16_t)(result * 1000.0f);
}

static void gadc_atm_isr(void const *arg)
{
	struct device *dev = (struct device *)arg;
	struct gadc_atm_data *data = DEV_DATA(dev);

	CMSDK_GADC->INTERRUPT_CLEAR = DGADC_INTERRUPT_CLEAR__WRITE;
	CMSDK_GADC->INTERRUPT_CLEAR = 0;

	NVIC_DisableIRQ(DT_INST_IRQN(0));

	*(data->buffer + data->offset[data->ch]) = gadc_process_samples(dev, data->ch);
	data->chmask &= ~BIT(data->ch);

	WRPR_CTRL_SET(CMSDK_GADC, WRPR_CTRL__CLK_DISABLE);
	if (data->chmask) {
		data->ch = __builtin_ffs(data->chmask) - 1;
		gadc_start_measurement(dev, data->ch);
	} else {
		adc_context_on_sampling_done(&data->ctx, dev);
		gadc_analog_control(false);
#ifdef CONFIG_PM
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
#endif
	}
}

static int gadc_atm_init(struct device const *dev)
{
	struct gadc_atm_data *data = DEV_DATA(dev);
	data->dev = dev;

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), gadc_atm_isr, DEVICE_DT_INST_GET(0),
		    0);

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static struct gadc_atm_data gadc_atm_data_0 = {
	ADC_CONTEXT_INIT_TIMER(gadc_atm_data_0, ctx),
	ADC_CONTEXT_INIT_LOCK(gadc_atm_data_0, ctx),
	ADC_CONTEXT_INIT_SYNC(gadc_atm_data_0, ctx),
};
DEVICE_DT_INST_DEFINE(0, gadc_atm_init, NULL, &gadc_atm_data_0, NULL, POST_KERNEL,
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &api_atm_driver_api);
BUILD_ASSERT(CMSDK_GADC == (CMSDK_AT_APB_GADC_TypeDef *)DT_REG_ADDR(DT_NODELABEL(adc)),
	     "INVALID CMSDK CONFIGURATION");

/*
 * Copyright (c) 2021-2025 Atmosic
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmosic_atmx2_adc

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adc_atm, CONFIG_ADC_LOG_LEVEL);

#include <zephyr/drivers/adc.h>
#include <soc.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <intisr.h>
#ifdef CONFIG_PM
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>
#endif

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#include "at_wrpr.h"
#include "at_apb_pseq_regs_core_macro.h"
#include "at_apb_gadc_regs_core_macro.h"
#include "at_apb_nvm_regs_core_macro.h"
#include "calibration.h"
#include "ll.h"

#define ATM_GADC_RESOLUTION 10

/* Reference voltage values */
#define VOLT_3_3 3300
#define VOLT_1_8 1800

/* GADC internal reference voltage (Unit:mV) */
#define ATM_GADC_VREF_VOL VOLT_3_3

/** List of GADC channels */
typedef enum {
	/** VBAT channel */
	VBATT = 0,
	/** VSTORE channel */
	VSTORE = 1,
	/** VDD1A channel */
	CORE = 2,
	/** Temperature channel */
	TEMP = 3,
	/** P10/P11 differential channel */
	PORT0_DIFFERENTIAL = 4,
	/** P10 single-ended channel */
	PORT0_SINGLE_ENDED_0 = 6,
	/** P11 single-ended channel */
	PORT0_SINGLE_ENDED_1 = 7,
	/** For GADC driver use only */
	ZV_PORT = 8,
	/** P9 single-ended channel */
	PORT1_SINGLE_ENDED_1 = 9,
	/** Max channels */
	GADC_CHANNEL_MAX,
} GADC_CHANNEL_ID;

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
/** Calibration data type */
struct gadc_cal_s {
	union {
		/** 32 bits data which combine offset and gain */
		uint32_t value;
		/** Decomposition structure */
		struct {
			/** Mantissa part of gain */
			unsigned int c1_mantissa : 12;
			/** Exponent part of gain */
			int c1_exponent : 6;
			/** Sign part of gain */
			unsigned int c1_sign : 1;
			/** Double value of offset */
			int c0_x2 : 13;
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
	uint8_t offset[GADC_CHANNEL_MAX];
};
#define DEV_DATA(dev) ((struct gadc_atm_data *)(dev)->data)

static uint32_t chan_setup_mask;
static uint16_t gadc_zerovolt_meas_x4; /* Zero volt gadc measurement */

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat)
{
	struct gadc_atm_data *data = CONTAINER_OF(ctx, struct gadc_atm_data, ctx);

	if (!repeat) {
		data->buffer += data->active_channels;
	}
}

/* Read GADC FIFO  and return channel measurement data */
static uint16_t gadc_read_ch_data(void)
{
	uint32_t gadc_data = 0;

	/* Read 8 samples from fifo. Drop first 4 samples */
	for (int i = 0; i < 8; i++) {
		uint32_t data_output = CMSDK_GADC->DATAPATH_OUTPUT;
		uint16_t chdata = DGADC_DATAPATH_OUTPUT__DATA__READ(data_output);
		if (i > 3) {
			gadc_data += ((chdata & 0xfff) ^ 0x800);
		}
	}

	/* Avg last 4 samples */
	gadc_data = gadc_data >> 2;

	/* Flush FIFO */
	while (!(CMSDK_GADC->DATAPATH_OUTPUT & DGADC_DATAPATH_OUTPUT__EMPTY__MASK)) {
		YIELD();
	}

	return ((uint16_t)gadc_data);
}

static void gadc_measure_zerovolt_channel(void)
{
	CMSDK_GADC->INTERRUPT_MASK = 0;
	CMSDK_GADC->INTERRUPT_CLEAR = DGADC_INTERRUPT_CLEAR__WRITE;
	CMSDK_GADC->INTERRUPT_CLEAR = 0;

	uint8_t ch = VSTORE;

	bool non_harv = NVM_EFUSE_AUTOREAD__OTP_HARV_DISABLED__READ(CMSDK_NVM->EFUSE_AUTOREAD);
	if (non_harv) {
		ch = VSTORE;
	} else {
		ch = ZV_PORT;
	}

	CMSDK_GADC->CTRL = DGADC_CTRL__WATCH_CHANNELS__WRITE(1 << ch) |
			   DGADC_CTRL__AVERAGING_AMOUNT__WRITE(4) | /* (2^4 = 16 + 2 = 18 cycles) */
			   DGADC_CTRL__WAIT_AMOUNT__WRITE(0) |
			   DGADC_CTRL__MODE__WRITE(0) | /* Continuous Mode */
			   DGADC_CTRL__ENABLE_DP__MASK;

	/* Wait for fifo overrun to be set */
	while (!DGADC_INTERRUPTS__INTRPT1__READ(CMSDK_GADC->INTERRUPTS)) {
		YIELD();
	}

	CMSDK_GADC->CTRL = 0;
	gadc_zerovolt_meas_x4 = gadc_read_ch_data();
}

static void gadc_start_measurement(struct device const *dev, GADC_CHANNEL_ID ch)
{
	NVIC_EnableIRQ(DT_INST_IRQN(0));

	WRPR_CTRL_SET(CMSDK_GADC, WRPR_CTRL__CLK_ENABLE);

	/* Handle temp dependency for single ended channels */
	if ((ch != PORT0_DIFFERENTIAL) && (ch != TEMP)) {
		gadc_measure_zerovolt_channel();
	}

	CMSDK_GADC->INTERRUPT_MASK = 0;
	CMSDK_GADC->INTERRUPT_CLEAR = DGADC_INTERRUPT_CLEAR__WRITE;
	CMSDK_GADC->INTERRUPT_CLEAR = 0;

	uint16_t gext = DGADC_FINAL_INVERSION__GEXT__READ(CMSDK_GADC->FINAL_INVERSION);
	if (adc_ref_internal(dev) == VOLT_3_3) {
		gext &= ~(1 << ch);
	} else {
		gext |= (1 << ch);
	}
	CMSDK_GADC->FINAL_INVERSION = DGADC_FINAL_INVERSION__GEXT__WRITE(gext);

	CMSDK_GADC->CTRL = DGADC_CTRL__WATCH_CHANNELS__WRITE(1 << ch) |
			   DGADC_CTRL__AVERAGING_AMOUNT__WRITE(4) | /* (2^4 = 16 + 2 = 18 cycles) */
			   DGADC_CTRL__WAIT_AMOUNT__WRITE(0) |
			   DGADC_CTRL__MODE__WRITE(0) | /* Continuous Mode */
			   DGADC_CTRL__ENABLE_DP__MASK;

	/* Interrupt when complete (fifo overrun) */
	CMSDK_GADC->INTERRUPT_MASK = DGADC_INTERRUPT_MASK__MASK_INTRPT1__MASK;
}

/* Enable/Disable GADC analog side */
__INLINE void gadc_analog_control(bool enable)
{
	WRPR_CTRL_PUSH(CMSDK_PSEQ, WRPR_CTRL__CLK_ENABLE) {
		CMSDK_PSEQ->GADC_CONFIG = PSEQ_GADC_CONFIG__GADC_CUTVDD_B__MASK;
		if (enable) {
			/* Turn on GADC analog side */
			CMSDK_PSEQ->GADC_CONFIG = PSEQ_GADC_CONFIG__WRITE;
		} else {
			/* Turn off GADC analog side */
			CMSDK_PSEQ->GADC_CONFIG = 0;
		}
	} WRPR_CTRL_POP();
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

	if (!chan_setup_mask || chan_setup_mask & ~BIT_MASK(GADC_CHANNEL_MAX)) {
		LOG_ERR("Invalid selection of channels. Received: %#x", sequence->channels);
		return -EINVAL;
	}

	if (sequence->resolution != ATM_GADC_RESOLUTION) {
		LOG_ERR("Only %d bit resolution is supported. Received: %d", ATM_GADC_RESOLUTION,
			sequence->resolution);
		return -EINVAL;
	}

	data->active_channels = 0;
	for (int i = 0; i < GADC_CHANNEL_MAX; ++i) {
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

static int gadc_atm_channel_setup(struct device const *dev,
				  struct adc_channel_cfg const *channel_cfg)
{
	ARG_UNUSED(dev);

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("Selected GADC acquisition time is not valid");
		return -EINVAL;
	}

	if (channel_cfg->channel_id >= GADC_CHANNEL_MAX) {
		LOG_ERR("Channel %d is not valid", channel_cfg->channel_id);
		return -EINVAL;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("Invalid channel gain");
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
	static float vbatt; /* Measured value using GADC */

	CMSDK_GADC->CTRL = 0;

	uint16_t sample_x4 = gadc_read_ch_data();

	/* gadc channel output is a ramp that goes from 3.0V
           (digital code: 309lsb) to 1.5V (digital-code: 629lsb)
           in a sawtooth form */
	float result;

	uint8_t gext = (adc_ref_internal(dev) == VOLT_3_3) ? 0 : 1;
	if (CAL_PRESENT(misc_cal, GADC_GAIN_OFFSET[ch][gext])) {
		struct gadc_cal_s gadc_cal = { .value = misc_cal.GADC_GAIN_OFFSET[ch][gext] };
		int16_t offset_x2 = gadc_cal.c0_x2;
		__ieee_float_shape_type gain = {
			.number.fraction = gadc_cal.c1_mantissa << (23 - 12),
			.number.exponent = gadc_cal.c1_exponent + (127 - 31),
			.number.sign = gadc_cal.c1_sign,
		};

		LOG_DBG("Found cal for gext %u, channel %u, offset_x2 %d, gain %f", gext, ch,
			offset_x2, (double)gain.value);

		int16_t sample_x4_signed = sample_x4;

		if ((ch != PORT0_DIFFERENTIAL) && (ch != TEMP)) {
			if (gadc_cal.c1_sign) {
				sample_x4_signed = sample_x4 + gadc_zerovolt_meas_x4;
			} else {
				sample_x4_signed = sample_x4 - gadc_zerovolt_meas_x4;
			}
		}
		/*
		result = C1*(D + C0)
		    D = Digital Output
		    C0 = cal offset
		    C1 = cal gain

		result = C1*(D_x4 + C0_x2*2)/4 */
		result = 0.25f * (gain.value * (sample_x4_signed + (offset_x2 * 2)));

		if (ch != PORT0_DIFFERENTIAL) {
			/* Apply correction for single ended channels */
			result -= (0.012f * result);
		}

	} else {
		/*
		result = -1.2*(C1*(D + C0) - 5*(Vcm - Bias))
		    D = Digital Output
		    C0 = -512
		    C1 = 1/256
		    Vcm = 0.675v
		    Bias = 0.333v

		result = -1.2*((1/256)*(D - 512) - 5*(0.675v - 0.333v))
		result = (-1.2/256)*((D - 512) - 256*5*0.342)
		result = -0.0046875*((D - 512) - 437.76)
		result = 0.25 * -0.0046875*(D_x4 - (949.76*4)) */
		result = -0.001171875f * (sample_x4 - 3799.04f);
	}

	if (ch == VBATT) {
		vbatt = result;
	}

	LOG_DBG("channel: %d, sample_x4: %u, zerovolt: %u, result: %f V", ch, sample_x4,
		gadc_zerovolt_meas_x4, (double)result);

	return (uint16_t)((result * 1000.0f) + 0.5f);
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

	CMSDK_WRPR->INTRPT_CFG_4 = INTISR_SRC_GADC;
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
BUILD_ASSERT(CMSDK_GADC == (CMSDK_AT_APB_GADC_TypeDef *)DT_REG_ADDR(DT_NODELABEL(adc)));

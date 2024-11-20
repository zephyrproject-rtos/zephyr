
/*
 * Copyright (c) 2023-2024 Atmosic
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmosic_atm34_adc

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
#include "spi.h"
#include "pmu_spi.h"
#include "pmu_top_regs_core_macro.h"
#include "pmu_swreg_regs_core_macro.h"
#include "pmu_gadc_regs_core_macro.h"
#include "timer.h"
#include "sec_jrnl.h"

/* Reference voltage values */
#define VOLT_3_3 3300
#define VOLT_1_8 1800

/* GADC internal reference voltage (Unit:mV) */
#define ATM_GADC_VREF_VOL VOLT_3_3

// Simulation defined numbers for temperature calculation
#define VPTAT_at_25 374.84f // mV
#define SLOPE_T     1.24f   // mV/C

#define GADC_MOD_SELECT    0
#define GADC_WARMUP_CYCLES 3
#define GADC_WAIT_AMOUNT   40

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
	/// Reserved,
	GROUND,
	/// Calibration
	CALIBRATION,
	/** Max channels */
	CHANNEL_NUM_MAX,
} GADC_CHANNEL_ID;
#define CHANNEL_NUM_MAX_USER (CHANNEL_NUM_MAX - 2)

typedef enum {
	CH_TYPE_SINGLE_ENDED,
	CH_TYPE_DIFFERENTIAL,
	CH_TYPE_LI_ION,
	CH_TYPE_TEMPERATURE,
	CH_TYPE_MAX,
	CH_TYPE_INVALID,
} gadc_ch_type_t;

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
/// FIFO data type
struct gadc_fifo_s {
	union {
		/// 32 bits data which represents full FIFO value
		uint32_t value;
		/// Decomposition structure.
		struct {
			/// Sample part of FIFO data
			signed int sample: 16;
			/// Channel used for the FIFO
			unsigned int channel: 4;
			/// Remaining 12 bits are zero from mask before assignment
		};
	};
};
#else
#error "Unsupported endianness"
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
	uint8_t offset[CHANNEL_NUM_MAX_USER];
};
#define DEV_DATA(dev) ((struct gadc_atm_data *)(dev)->data)

static uint32_t chan_setup_mask;

typedef enum {
	GAIN_EXT_X1,
	GAIN_EXT_HALF,
	GAIN_EXT_QUARTER,
	GAIN_EXT_EIGHTH,
	GAIN_EXT_END,
	GAIN_EXT_MAX,
} gadc_gain_ext_t;

static gadc_gain_ext_t gext[CHANNEL_NUM_MAX];
static gadc_gain_ext_t const gextmap[CHANNEL_NUM_MAX][GAIN_EXT_MAX] = {
	{GAIN_EXT_END}, // unused, invalid channel
	{GAIN_EXT_EIGHTH, GAIN_EXT_QUARTER, GAIN_EXT_END},
	{GAIN_EXT_EIGHTH, GAIN_EXT_QUARTER, GAIN_EXT_END},
	{GAIN_EXT_HALF, GAIN_EXT_END},
	{GAIN_EXT_X1, GAIN_EXT_END},
	{GAIN_EXT_QUARTER, GAIN_EXT_HALF, GAIN_EXT_X1, GAIN_EXT_END},
	{GAIN_EXT_QUARTER, GAIN_EXT_HALF, GAIN_EXT_X1, GAIN_EXT_END},
	{GAIN_EXT_QUARTER, GAIN_EXT_HALF, GAIN_EXT_END},
	{GAIN_EXT_QUARTER, GAIN_EXT_HALF, GAIN_EXT_END},
	{GAIN_EXT_QUARTER, GAIN_EXT_HALF, GAIN_EXT_END},
	{GAIN_EXT_QUARTER, GAIN_EXT_HALF, GAIN_EXT_END},
	{GAIN_EXT_HALF, GAIN_EXT_X1, GAIN_EXT_END},
	{GAIN_EXT_EIGHTH, GAIN_EXT_QUARTER, GAIN_EXT_HALF, GAIN_EXT_X1, GAIN_EXT_END},
	{GAIN_EXT_EIGHTH, GAIN_EXT_QUARTER, GAIN_EXT_HALF, GAIN_EXT_X1, GAIN_EXT_END},
};

static struct gadc_cal_s gcal;
static uint16_t gcal_len;

static uint32_t calts[GAIN_EXT_END];
static bool firstcal[GAIN_EXT_END];

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

static void gadc_apply_calibration(void)
{
	if (CAL_PRESENT(gcal, offset_comp3)) {
		CMSDK_GADC->CTRL1 = gcal.ctrl1;
		CMSDK_GADC->GAIN_COMP0 = gcal.gain_comp0;
		CMSDK_GADC->GAIN_COMP1 = gcal.gain_comp1;
		CMSDK_GADC->GAIN_COMP2 = gcal.gain_comp2;
		CMSDK_GADC->GAIN_COMP3 = gcal.gain_comp3;
		CMSDK_GADC->GAIN_COMP4 = gcal.gain_comp4;
		CMSDK_GADC->GAIN_COMP5 = gcal.gain_comp5;
		CMSDK_GADC->GAIN_COMP6 = gcal.gain_comp6;
		CMSDK_GADC->GAIN_COMP7 = gcal.gain_comp7;
		CMSDK_GADC->OFFSET_COMP0 = gcal.offset_comp0;
		CMSDK_GADC->OFFSET_COMP1 = gcal.offset_comp1;
		CMSDK_GADC->OFFSET_COMP2 = gcal.offset_comp2;
		CMSDK_GADC->OFFSET_COMP3 = gcal.offset_comp3;
	}
}

static void gadc_start_measurement(struct device const *dev, GADC_CHANNEL_ID ch)
{
	WRPR_CTRL_SET(CMSDK_GADC, WRPR_CTRL__CLK_ENABLE | WRPR_CTRL__CLK_SEL);
	gadc_apply_calibration();

	gadc_analog_control(true);

	NVIC_EnableIRQ(DT_INST_IRQN(0));

	CMSDK_GADC->INTERRUPT_MASK = 0;
	CMSDK_GADC->INTERRUPT_CLEAR = DGADC_INTERRUPT_CLEAR__WRITE;
	CMSDK_GADC->INTERRUPT_CLEAR = 0;

	switch (ch) {
	case VBATT: {
		DGADC_GAIN_CONFIG0__CH1_GAIN_SEL__MODIFY(CMSDK_GADC->GAIN_CONFIG0, gext[ch]);
	} break;
	case VSTORE: {
		DGADC_GAIN_CONFIG0__CH2_GAIN_SEL__MODIFY(CMSDK_GADC->GAIN_CONFIG0, gext[ch]);
	} break;
	case CORE: {
		DGADC_GAIN_CONFIG0__CH3_GAIN_SEL__MODIFY(CMSDK_GADC->GAIN_CONFIG0, gext[ch]);
	} break;
	case TEMP: {
		DGADC_GAIN_CONFIG0__CH4_GAIN_SEL__MODIFY(CMSDK_GADC->GAIN_CONFIG0, gext[ch]);
	} break;
	case PORT1_DIFFERENTIAL: {
		DGADC_GAIN_CONFIG0__CH5_GAIN_SEL__MODIFY(CMSDK_GADC->GAIN_CONFIG0, gext[ch]);
	} break;
	case PORT0_DIFFERENTIAL: {
		DGADC_GAIN_CONFIG0__CH6_GAIN_SEL__MODIFY(CMSDK_GADC->GAIN_CONFIG0, gext[ch]);
	} break;
	case PORT1_SINGLE_ENDED_0: {
		DGADC_GAIN_CONFIG0__CH7_GAIN_SEL__MODIFY(CMSDK_GADC->GAIN_CONFIG0, gext[ch]);
	} break;
	case PORT1_SINGLE_ENDED_1: {
		DGADC_GAIN_CONFIG0__CH8_GAIN_SEL__MODIFY(CMSDK_GADC->GAIN_CONFIG0, gext[ch]);
	} break;
	case PORT0_SINGLE_ENDED_0: {
		DGADC_GAIN_CONFIG0__CH9_GAIN_SEL__MODIFY(CMSDK_GADC->GAIN_CONFIG0, gext[ch]);
	} break;
	case PORT0_SINGLE_ENDED_1: {
		DGADC_GAIN_CONFIG0__CH10_GAIN_SEL__MODIFY(CMSDK_GADC->GAIN_CONFIG0, gext[ch]);
	} break;
	case LI_ION_BATT: {
		DGADC_GAIN_CONFIG1__CH11_GAIN_SEL__MODIFY(CMSDK_GADC->GAIN_CONFIG1, gext[ch]);
		WRPR_CTRL_PUSH(CMSDK_PMU, WRPR_CTRL__CLK_ENABLE)
		{
			uint32_t gadc_ctrl = PMU_READ(GADC, GADC_CTRL_REG_ADDR);
			GADC_GADC_CTRL__LI_EN__SET(gadc_ctrl);
			PMU_WRITE(GADC, GADC_CTRL_REG_ADDR, gadc_ctrl);
		}
		WRPR_CTRL_POP();
	} break;
	case CALIBRATION: {
		DGADC_GAIN_CONFIG1__CH12_GAIN_SEL__MODIFY(CMSDK_GADC->GAIN_CONFIG1, gext[ch]);
	} break;
	case UNUSED:
	case CHANNEL_NUM_MAX:
	default: {
		LOG_INF("Invalid channel: %d", ch);
		ASSERT_ERR(0);
	} break;
	}

	uint8_t clkdiv = DT_PROP(DT_NODELABEL(adc), clock_freq);
	uint8_t savg = DT_PROP(DT_NODELABEL(adc), sample_avg);
	CMSDK_GADC->CTRL = DGADC_CTRL__WATCH_CHANNELS__WRITE(1 << ch) |
			   DGADC_CTRL__AVERAGING_AMOUNT__WRITE(savg) |
			   DGADC_CTRL__WAIT_AMOUNT__WRITE(GADC_WAIT_AMOUNT) |
			   DGADC_CTRL__CLKDIV__WRITE(clkdiv) |
			   DGADC_CTRL__WARMUP__WRITE(GADC_WARMUP_CYCLES) |
			   DGADC_CTRL__MODE__WRITE(1); // One Shot Mode

	uint8_t osrsel = DT_PROP(DT_NODELABEL(adc), osr_select);
	DGADC_CTRL1__OSR_SEL__MODIFY(CMSDK_GADC->CTRL1, osrsel);
	DGADC_CTRL1__MOD_SEL__MODIFY(CMSDK_GADC->CTRL1, GADC_MOD_SELECT);

	// Flush old FIFO values
	while (!(CMSDK_GADC->DATAPATH_OUTPUT & DGADC_DATAPATH_OUTPUT__EMPTY__MASK)) {
		YIELD();
	}

	DGADC_CTRL__ENABLE_DP__SET(CMSDK_GADC->CTRL);

	// Interrupt when complete (fifo overrun)
	CMSDK_GADC->INTERRUPT_MASK = DGADC_INTERRUPT_MASK__MASK_INTRPT2__MASK;
}

static void gadc_calibrate_offset(gadc_gain_ext_t gainext, int16_t sample)
{
	int16_t result = -sample;
	LOG_INF("%s: %u %d", __func__, gainext, result);
	switch (gainext) {
	case GAIN_EXT_X1:
		gcal.offset_comp0 = DGADC_OFFSET_COMP0__OFFSET__WRITE(result);
		break;
	case GAIN_EXT_HALF:
		gcal.offset_comp1 = DGADC_OFFSET_COMP1__OFFSET__WRITE(result);
		break;
	case GAIN_EXT_QUARTER:
		gcal.offset_comp2 = DGADC_OFFSET_COMP2__OFFSET__WRITE(result);
		break;
	case GAIN_EXT_EIGHTH:
		gcal.offset_comp3 = DGADC_OFFSET_COMP3__OFFSET__WRITE(result);
		break;
	default:
		LOG_ERR("Invalid gext: %d", gainext);
		break;
	}
}

static void gadc_measure_or_calibrate(struct gadc_atm_data *data)
{
	uint32_t curts = atm_lpc_to_ms(atm_get_sys_time());
	if (firstcal[gext[data->ch]] ||
	    ((curts - calts[gext[data->ch]]) > CONFIG_ADC_CAL_REFRESH_INTERVAL)) {
		calts[gext[data->ch]] = curts;
		firstcal[gext[data->ch]] = false;
		gadc_calibrate_offset(gext[data->ch], 0);

		// Set up the calibration channel for measurement
		gext[CALIBRATION] = gext[data->ch];
		data->chmask |= BIT(CALIBRATION);
		data->ch = CALIBRATION;
	}

	gadc_start_measurement(data->dev, data->ch);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct gadc_atm_data *data = CONTAINER_OF(ctx, struct gadc_atm_data, ctx);

	data->chmask = ctx->sequence.channels;
	data->ch = __builtin_ffs(data->chmask) - 1;
#ifdef CONFIG_PM
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
#endif
	gadc_measure_or_calibrate(data);
}

static int gadc_atm_read_async(struct device const *dev, struct adc_sequence const *sequence,
			       struct k_poll_signal *async)
{
	struct gadc_atm_data *data = DEV_DATA(dev);

	if (!chan_setup_mask || chan_setup_mask & ~BIT_MASK(CHANNEL_NUM_MAX_USER)) {
		LOG_ERR("Invalid selection of channels. Received: %#x", sequence->channels);
		return -EINVAL;
	}

	uint8_t resolution = DT_PROP(DT_NODELABEL(adc), resolution);
	if (sequence->resolution != resolution) {
		LOG_ERR("Only %d bit resolution is supported. Received: %d", resolution,
			sequence->resolution);
		return -EINVAL;
	}

	data->active_channels = 0;
	for (int i = 0; i < CHANNEL_NUM_MAX_USER; ++i) {
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

	if (channel_cfg->channel_id >= CHANNEL_NUM_MAX_USER) {
		LOG_ERR("Channel %d is not valid", channel_cfg->channel_id);
		return -EINVAL;
	}

	gadc_gain_ext_t gainext;
	switch (channel_cfg->gain) {
	case ADC_GAIN_1_8:
		gainext = GAIN_EXT_EIGHTH;
		break;
	case ADC_GAIN_1_4:
		gainext = GAIN_EXT_QUARTER;
		break;
	case ADC_GAIN_1_2:
		gainext = GAIN_EXT_HALF;
		break;
	case ADC_GAIN_1:
		gainext = GAIN_EXT_X1;
		break;
	default:
		LOG_ERR("Invalid channel gain");
		return -EINVAL;
	}

	if (!gadc_ext_valid(channel_cfg->channel_id, gainext)) {
		LOG_ERR("Invalid gext (%d) for channel (%d)", gainext, channel_cfg->channel_id);
		return -EINVAL;
	}
	gext[channel_cfg->channel_id] = gainext;

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
#ifdef CONFIG_ADC_ASYNC
	.read_async = gadc_atm_read_async,
#endif
	.ref_internal = ATM_GADC_VREF_VOL,
};

static uint16_t gadc_process_samples(struct device const *dev, GADC_CHANNEL_ID ch,
				     int16_t *sample_raw)
{
	ASSERT_ERR(ch && (ch < CHANNEL_NUM_MAX));
	CMSDK_GADC->CTRL = 0;

	struct gadc_fifo_s raw_fifo = gadc_read_ch_data();

	// Disable clocks between samples
	gadc_analog_control(false);
	WRPR_CTRL_SET(CMSDK_GADC, WRPR_CTRL__SRESET);

	// raw_fifo:  4 bit channel + 16 bit data = 20 bits
	int16_t sample_signed = raw_fifo.sample;
	*sample_raw = sample_signed;

	float sample_scaling = (32767.0f / 0.6f) / (1 << gext[ch]);
	float result = sample_signed / sample_scaling;
	if (ch == LI_ION_BATT) {
		WRPR_CTRL_PUSH(CMSDK_PMU, WRPR_CTRL__CLK_ENABLE)
		{
			uint32_t gadc_ctrl = PMU_READ(GADC, GADC_CTRL_REG_ADDR);
			GADC_GADC_CTRL__LI_EN__CLR(gadc_ctrl);
			PMU_WRITE(GADC, GADC_CTRL_REG_ADDR, gadc_ctrl);
		}
		WRPR_CTRL_POP();
		result *= 6;
	} else if ((ch == PORT0_SINGLE_ENDED_1) || (ch == PORT1_SINGLE_ENDED_1)) {
		// need to invert sign
		result *= -1;
	} else if (ch == TEMP) {
		result = (((result * 1000) - VPTAT_at_25) / SLOPE_T) + 25;
	}

	return (uint16_t)(result * 1000.0f);
}

static void gadc_atm_isr(void const *arg)
{
	struct device *dev = (struct device *)arg;
	struct gadc_atm_data *data = DEV_DATA(dev);

	CMSDK_GADC->INTERRUPT_CLEAR = DGADC_INTERRUPT_CLEAR__WRITE;
	CMSDK_GADC->INTERRUPT_CLEAR = 0;

	NVIC_DisableIRQ(DT_INST_IRQN(0));

	int16_t sample_raw;
	uint16_t sample = gadc_process_samples(dev, data->ch, &sample_raw);
	data->chmask &= ~BIT(data->ch);
	if (data->ch == CALIBRATION) {
		data->ch = __builtin_ffs(data->chmask) - 1;
		gadc_calibrate_offset(gext[data->ch], sample_raw);
		gadc_start_measurement(dev, data->ch);
		return;
	}

	*(data->buffer + data->offset[data->ch]) = sample;
	if (data->chmask) {
		data->ch = __builtin_ffs(data->chmask) - 1;
		gadc_measure_or_calibrate(data);
		return;
	}

	adc_context_on_sampling_done(&data->ctx, dev);
#ifdef CONFIG_PM
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
#endif
}

static int gadc_atm_init(struct device const *dev)
{
	struct gadc_atm_data *data = DEV_DATA(dev);
	data->dev = dev;

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), gadc_atm_isr, DEVICE_DT_INST_GET(0),
		    0);

	// Fetch GADC calibration
	gcal_len = sizeof(gcal);
	sec_jrnl_ret_status_t status =
		nsc_sec_jrnl_get(ATM_TAG_GADC_CAL, &gcal_len, (uint8_t *)&gcal);
	if (status != SEC_JRNL_OK) {
		LOG_INF("GADC_CAL tag not found: %#x", status);
		gcal_len = 0;
	}

	uint32_t ts = atm_lpc_to_ms(atm_get_sys_time());
	for (int i = 0; i < ARRAY_SIZE(calts); i++) {
		calts[i] = ts;
		firstcal[i] = true;
	}

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

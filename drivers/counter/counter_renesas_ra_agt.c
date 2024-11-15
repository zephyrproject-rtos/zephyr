/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_agt_counter

#include <zephyr/devicetree.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/irq.h>
#include <r_agt.h>
#include <zephyr/logging/log.h>

#define FSUB_FREQUENCY_HZ (32768U)
typedef volatile R_AGTX0_AGT16_CTRL_Type agt_reg_ctrl_t;
#define AGT_AGTCR_STOP_TIMER  0xF0U /* 1111 0000 */
#define AGT_AGTCR_START_TIMER 0xF1U /* 1111 0001 */
#define AGT_AGTCR_FLAGS_MASK  0xF0U /* 1111 0000 */

#define AGT_PRV_MIN_CLOCK_FREQ      (0U)
#define AGT_SOURCE_CLOCK_PCLKB_BITS (0x3U)

LOG_MODULE_REGISTER(ra_agt_counter, CONFIG_COUNTER_LOG_LEVEL);

struct counter_ra_agt_config {
	/* info must be first element */
	struct counter_config_info info; /* Counter Config info struct */
	uint32_t channel;                /* Channel no */
	uint32_t cycle_end_irq;          /* Underflow interrupt*/
	uint32_t cycle_end_ipl;          /* Underflow interrupt priority */
	uint32_t channel_ipl;            /* IPL channel no. */
	uint32_t channel_irq;            /* IRQ channel no. */
	/* Device tree data */
	timer_source_div_t source_div;   /* Clock source divider */
	agt_agtio_filter_t agtio_filter; /* Input filter for AGTIO */
	agt_measure_t measurement_mode;  /* Measurement mode */
	agt_clock_t count_source;        /* AGT channel clock source */
	uint32_t resolution;             /* AGT node resolution */
	uint32_t dt_reg;                 /* Reg address from device tree */
};

struct counter_ra_agt_alarm {
	counter_alarm_callback_t callback;
	void *data;
};

struct counter_ra_agt_data {
	/* Common data */
	R_AGTX0_Type *agt_reg;   /* AGT register base address */
	uint32_t period;         /* Current timer period (counts) */
	uint32_t period_counts;  /* Period in raw timer counts */
	uint32_t cycle_end_ipl;  /* Cycle end interrupt priority */
	IRQn_Type cycle_end_irq; /* Cycle end interrupt */
	/* Alarm-related data */
	struct counter_ra_agt_alarm alarm; /* Counter alarm config struct */
	counter_top_callback_t top_cb;     /* Top level callback */
	void *top_cb_data;                 /* Top level callback data */
	uint32_t guard_period;             /* Absolute counter alarm's guard period */
};

static void r_agt_hardware_cfg(const struct device *dev);
static void r_agt_period_register_set(const struct device *dev, uint32_t period_counts);
static uint32_t r_agt_clock_frequency_get(agt_reg_ctrl_t *p_reg);
static fsp_err_t r_agt_common_preamble(agt_reg_ctrl_t *p_reg);
static agt_reg_ctrl_t *r_agt_reg_ctrl_get(const struct device *dev);
static uint32_t r_agt_ticks_sub(uint32_t val, uint32_t old, uint32_t top);

static int counter_ra_agt_start(const struct device *dev)
{
	agt_reg_ctrl_t *const reg = r_agt_reg_ctrl_get(dev);
	uint32_t timeout = UINT32_MAX;

	reg->AGTCR = AGT_AGTCR_START_TIMER;

	while (!(reg->AGTCR & BIT(R_AGTX0_AGT16_CTRL_AGTCR_TCSTF_Pos)) && likely(--timeout))
		;

	return timeout > 0 ? 0 : -EIO;
}

static int counter_ra_agt_stop(const struct device *dev)
{
	agt_reg_ctrl_t *const reg = r_agt_reg_ctrl_get(dev);
	uint32_t timeout = UINT32_MAX;

	reg->AGTCR = AGT_AGTCR_STOP_TIMER;

	while ((reg->AGTCR & BIT(R_AGTX0_AGT16_CTRL_AGTCR_TCSTF_Pos)) && likely(--timeout))
		;

	return timeout > 0 ? 0 : -EIO;
}

static inline int counter_ra_agt_read(const struct device *dev)
{
	struct counter_ra_agt_data *data = dev->data;

	return data->agt_reg->AGT16.AGT;
}
static int counter_ra_agt_get_value(const struct device *dev, uint32_t *ticks)
{
	*ticks = counter_ra_agt_read(dev);
	return 0;
}

static uint32_t counter_ra_agt_get_top_value(const struct device *dev)
{
	const struct counter_ra_agt_config *config = dev->config;

	return config->info.max_top_value;
}

static int counter_ra_agt_set_top_value(const struct device *dev, const struct counter_top_cfg *cfg)
{
	const struct counter_ra_agt_config *config = dev->config;
	struct counter_ra_agt_data *data = dev->data;
	agt_reg_ctrl_t *p_reg_ctrl = r_agt_reg_ctrl_get(dev);

	if (cfg->ticks != config->info.max_top_value) {
		return -ENOTSUP;
	}

	if (cfg->callback == NULL) {
		/* Disable Match Register A if callback is NULL */
		p_reg_ctrl->AGTCR_b.TCMAF = 0;
	} else {
		/* Enable Match Register A */
		p_reg_ctrl->AGTCR_b.TCMAF = 1;
	}

	data->top_cb = cfg->callback;
	data->top_cb_data = cfg->user_data;

	return 0;
}

static inline void counter_ra_agt_set_compare_value(const struct device *dev, uint8_t chan,
						    uint32_t value)
{
	struct counter_ra_agt_data *data = dev->data;

	data->agt_reg->AGT16.AGTCMA = value;
}

static inline void counter_ra_agt_enable_channel_irq(const struct device *dev)
{
	const struct counter_ra_agt_config *config = dev->config;
	agt_reg_ctrl_t *p_reg_ctrl = r_agt_reg_ctrl_get(dev);

	/* Enable AGT compare match A */
	p_reg_ctrl->AGTCMSR |= BIT(R_AGTX0_AGT16_CTRL_AGTCMSR_TCMEA_Pos);
	irq_enable(config->channel_irq);
}

static inline void counter_ra_agt_clear_channel_irq(const struct device *dev)
{
	const struct counter_ra_agt_config *config = dev->config;
	agt_reg_ctrl_t *const reg_ctrl = r_agt_reg_ctrl_get(dev);

	reg_ctrl->AGTCR_b.TCMAF = 0;
	R_BSP_IrqStatusClear(config->channel_irq);
	NVIC_ClearPendingIRQ(config->channel_irq);
}

static int counter_ra_agt_set_alarm(const struct device *dev, uint8_t chan,
				    const struct counter_alarm_cfg *alarm_cfg)
{
	struct counter_ra_agt_data *data = dev->data;
	const struct counter_ra_agt_config *config = dev->config;

	const bool absolute = alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE;
	const uint32_t top = counter_ra_agt_get_top_value(dev);
	struct counter_ra_agt_alarm *const alarm = &data->alarm;
	uint16_t val = alarm_cfg->ticks;
	uint32_t now;
	uint32_t max_rel_val;
	bool irq_on_late;
	int32_t diff = 0;
	int err = 0;

	if (alarm_cfg->ticks > counter_ra_agt_get_top_value(dev)) {
		LOG_ERR("%s: alarm ticks is larger than top value", __func__);
		return -EINVAL;
	}

	if (alarm->callback) {
		LOG_ERR("%s: Device busy. Callback is still available.", __func__);
		return -EBUSY;
	}
	alarm->callback = alarm_cfg->callback;
	alarm->data = alarm_cfg->user_data;

	/*
	 * stop AGT for writing the match register directly instead of sync when underflow event
	 * occur reference: RA6M5 User manual - 22.3.2 Reload Register and AGT Compare Match A/B
	 * Register Rewrite Operation
	 */
	counter_ra_agt_stop(dev);
	now = counter_ra_agt_read(dev);

	if (absolute) {
		/* Acceptable alarm value range, counting from current tick (now) */
		max_rel_val = top - data->guard_period;
		irq_on_late = alarm_cfg->flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE;
	} else {
		/* If relative value is smaller than half of the counter range
		 * it is assumed that there is a risk of setting value too late
		 * and late detection algorithm must be applied. When late
		 * setting is detected, interrupt shall be triggered for
		 * immediate expiration of the timer. Detection is performed
		 * by limiting relative distance between CC and counter.
		 *
		 * Note that half of counter range is an arbitrary value.
		 */
		irq_on_late = (val < (top / 2U));
		/* Limit max to detect short relative being set too late */
		max_rel_val = irq_on_late ? top / 2U : top;
		/* Recalculate alarm tick timestamp based on current tick (now) */
		val = r_agt_ticks_sub(now, val, top);
	}

	counter_ra_agt_set_compare_value(dev, chan, val);

	/* Decrement value to detect also case when val == counter_ra_agt_read(dev).
	 * Otherwise, condition would need to include comparing diff against 0.
	 */
	diff = r_agt_ticks_sub(now, val - 1, top);

	if (diff > max_rel_val) {
		if (absolute) {
			err = -ETIME;
		}

		/* Interrupt is triggered always for relative alarm and
		 * for absolute depending on the flag.
		 */
		if (irq_on_late) {
			/* Set pending IRQ */
			counter_ra_agt_enable_channel_irq(dev);
			NVIC_SetPendingIRQ(config->channel_irq);
		} else {
			alarm->callback = NULL;
			alarm->data = NULL;
		}
	}

	counter_ra_agt_clear_channel_irq(dev);
	counter_ra_agt_enable_channel_irq(dev);
	counter_ra_agt_start(dev);

	return err;
}

static int counter_ra_agt_cancel_alarm(const struct device *dev, uint8_t chan)
{
	struct counter_ra_agt_data *data = dev->data;

	agt_reg_ctrl_t *p_reg_ctrl = r_agt_reg_ctrl_get(dev);

	/* Disable AGT compare match A */
	p_reg_ctrl->AGTCMSR &= ~BIT(R_AGTX0_AGT16_CTRL_AGTCMSR_TCMEA_Pos);

	data->alarm.callback = NULL;
	data->alarm.data = NULL;

	return 0;
}

static uint32_t counter_ra_agt_get_pending_int(const struct device *dev)
{
	agt_reg_ctrl_t *const reg = r_agt_reg_ctrl_get(dev);

	return (reg->AGTCR & AGT_AGTCR_FLAGS_MASK) != 0;
}

static uint32_t counter_ra_agt_get_freq(const struct device *dev)
{
	struct counter_ra_agt_data *data = dev->data;
	timer_info_t info;

	agt_reg_ctrl_t *p_reg = r_agt_reg_ctrl_get(dev);

	r_agt_common_preamble(p_reg);

	/* Get and store period */
	info.period_counts = data->period;
	info.clock_frequency = r_agt_clock_frequency_get(p_reg);

	/* AGT supports only counting down direction */
	info.count_direction = TIMER_DIRECTION_DOWN;

	return info.clock_frequency;
}

static void counter_ra_agt_agti_isr(const struct device *dev)
{
	const struct counter_ra_agt_config *config = dev->config;
	struct counter_ra_agt_data *data = dev->data;
	agt_reg_ctrl_t *const reg_ctrl = r_agt_reg_ctrl_get(dev);

	R_BSP_IrqStatusClear(config->cycle_end_irq);

	const uint32_t agtcr = reg_ctrl->AGTCR;

	if (agtcr & BIT(R_AGTX0_AGT16_CTRL_AGTCR_TUNDF_Pos)) {
		if (data->top_cb) {
			data->top_cb(dev, data->top_cb_data);
		}
	}

	reg_ctrl->AGTCR = (uint8_t)(agtcr & ~(BIT(R_AGTX0_AGT16_CTRL_AGTCR_TUNDF_Pos) |
					      BIT(R_AGTX0_AGT16_CTRL_AGTCR_TEDGF_Pos)));
}

static void counter_ra_agt_agtcmai_isr(const struct device *dev)
{
	const struct counter_ra_agt_config *config = dev->config;
	struct counter_ra_agt_data *data = dev->data;
	agt_reg_ctrl_t *const reg_ctrl = r_agt_reg_ctrl_get(dev);
	struct counter_ra_agt_alarm *const alarm = &data->alarm;

	const uint32_t val = data->agt_reg->AGT16.AGTCMA;

	const counter_alarm_callback_t cb = alarm->callback;
	void *cb_data = alarm->data;

	alarm->callback = NULL;
	alarm->data = NULL;

	/* Disable AGT compare match A */
	reg_ctrl->AGTCMSR &= ~BIT(R_AGTX0_AGT16_CTRL_AGTCMSR_TCMEA_Pos);

	R_BSP_IrqStatusClear(config->channel_irq);

	if (cb) {
		cb(dev, config->channel, val, cb_data);
	}

	reg_ctrl->AGTCR_b.TCMAF = 0;
}

static uint32_t counter_ra_agt_get_guard_period(const struct device *dev, uint32_t flags)
{
	struct counter_ra_agt_data *data = dev->data;

	return data->guard_period;
}

static int counter_ra_agt_set_guard_period(const struct device *dev, uint32_t guard, uint32_t flags)
{
	struct counter_ra_agt_data *data = dev->data;

	if (counter_ra_agt_get_top_value(dev) < guard) {
		LOG_ERR("%s: Invalid guard rate", __func__);
		return -EINVAL;
	}

	data->guard_period = guard;

	return 0;
}

static int counter_ra_agt_init(const struct device *dev)
{
	const struct counter_ra_agt_config *config = dev->config;
	struct counter_ra_agt_data *data = dev->data;

	data->agt_reg = (R_AGTX0_Type *)config->dt_reg;

	agt_reg_ctrl_t *p_reg_ctrl = r_agt_reg_ctrl_get(dev);

	/* Power on the AGT channel */
	R_BSP_MODULE_START(FSP_IP_AGT, config->channel);

	/* Clear AGTCR. This stops the timer if it is running and clears the flags */
	p_reg_ctrl->AGTCR = 0U;

	/* The timer is stopped in sync with the count clock, or in sync with PCLK in event and
	 * external count modes.
	 */
	FSP_HARDWARE_REGISTER_WAIT(0U, p_reg_ctrl->AGTCR_b.TCSTF);

	/* Clear AGTMR2 before AGTMR1 is set. Reference Note 3 in section 25.2.6 "AGT Mode Register
	 * 2 (AGTMR2)" of the RA6M3 manual R01UH0886EJ0100.
	 */
	p_reg_ctrl->AGTMR2 = 0U;

	/* Set count source and divider and configure pins */
	r_agt_hardware_cfg(dev);

	/* Set period register and update duty cycle if output mode is used for one-shot or periodic
	 * mode.
	 */
	r_agt_period_register_set(dev, data->period_counts);

	/* 22.3.1 Reload Register and Counter Rewrite Operation */
	p_reg_ctrl->AGTCMSR |= BIT(R_AGTX0_AGT16_CTRL_AGTCMSR_TCMEA_Pos);

	return 0;
}

static agt_reg_ctrl_t *r_agt_reg_ctrl_get(const struct device *dev)
{
	struct counter_ra_agt_data *data = dev->data;
	agt_reg_ctrl_t *p_reg_ctrl = &data->agt_reg->AGT16.CTRL;

	return p_reg_ctrl;
}

static void r_agt_hardware_cfg(const struct device *dev)
{
	const struct counter_ra_agt_config *config = dev->config;

	/* Update the divider for PCLKB */
	agt_reg_ctrl_t *p_reg_ctrl = r_agt_reg_ctrl_get(dev);
	uint32_t count_source_int = (uint32_t)config->count_source;
	uint32_t agtmr2 = 0U;
	uint32_t agtcmsr = 0U;
	uint32_t tedgsel = 0U;
	uint32_t agtioc = config->agtio_filter;
	uint32_t mode = config->measurement_mode & R_AGTX0_AGT16_CTRL_AGTMR1_TMOD_Msk;
	uint32_t edge = 0U;

	if (AGT_CLOCK_PCLKB == config->count_source) {
		if (TIMER_SOURCE_DIV_1 != config->source_div) {
			/* Toggle the second bit if the count_source_int is not 0 to map PCLKB / 8
			 * to 1 and PCLKB / 2 to 3.
			 */
			count_source_int = config->source_div ^ 2U;
			count_source_int <<= R_AGTX0_AGT16_CTRL_AGTMR1_TCK_Pos;
		}
	}

	else if (AGT_CLOCK_AGT_UNDERFLOW != config->count_source) {
		/* Update the divider for LOCO/subclock */
		agtmr2 = config->source_div;
	} else {
		/* No divider can be used when count source is AGT_CLOCK_AGT_UNDERFLOW */
	}

	uint32_t agtmr1 = (count_source_int | edge) | mode;

	/* Configure output settings */

	agtioc |= tedgsel;

	p_reg_ctrl->AGTIOC = (uint8_t)agtioc;
	p_reg_ctrl->AGTCMSR = (uint8_t)agtcmsr;
	p_reg_ctrl->AGTMR1 = (uint8_t)agtmr1;
	p_reg_ctrl->AGTMR2 = (uint8_t)agtmr2;
}

static void r_agt_period_register_set(const struct device *dev, uint32_t period_counts)
{
	struct counter_ra_agt_data *data = dev->data;

	/* Store the period value so it can be retrieved later */
	data->period = period_counts;

	/* Set counter to period minus one */
	uint32_t period_reg = (period_counts - 1U);

	data->agt_reg->AGT16.AGT = (uint16_t)period_reg;
}

static uint32_t r_agt_clock_frequency_get(agt_reg_ctrl_t *p_reg)
{
	uint32_t clock_freq_hz = 0U;
	uint8_t count_source_int = p_reg->AGTMR1_b.TCK;
	timer_source_div_t divider = TIMER_SOURCE_DIV_1;

	if (0U == (count_source_int & (~AGT_SOURCE_CLOCK_PCLKB_BITS))) {
		/* Call CGC function to obtain current PCLKB clock frequency */
		clock_freq_hz = R_FSP_SystemClockHzGet(FSP_PRIV_CLOCK_PCLKB);

		/* If Clock source is PCLKB or derived from PCLKB */
		divider = (timer_source_div_t)count_source_int;

		if (divider != 0U) {
			/* Set divider to 3 to divide by 8 when AGTMR1.TCK is 1 (PCLKB / 8). Set
			 * divider to 1 to divide by 2 when AGTMR1.TCK is 3 (PCLKB / 2). XOR with 2
			 * to convert 1 to 3 and 3 to 1.
			 */
			divider ^= 2U;
		}
	} else {
		/* Else either fSUB clock or LOCO clock is used. The frequency is set to 32Khz
		 * (32768). This function does not support AGT0 underflow as count source.
		 */
		clock_freq_hz = FSUB_FREQUENCY_HZ;
		divider = (timer_source_div_t)p_reg->AGTMR2_b.CKS;
	}

	clock_freq_hz >>= divider;

	return clock_freq_hz;
}

static fsp_err_t r_agt_common_preamble(agt_reg_ctrl_t *p_reg)
{
	/* Ensure timer state reflects expected status. Reference section 25.4.1 "Count Operation
	 * Start and Stop Control" in the RA6M3 manual R01UH0886EJ0100.
	 */
	uint32_t agtcr_tstart = p_reg->AGTCR_b.TSTART;

	FSP_HARDWARE_REGISTER_WAIT(agtcr_tstart, p_reg->AGTCR_b.TCSTF);

	return FSP_SUCCESS;
}

static uint32_t r_agt_ticks_sub(uint32_t val, uint32_t old, uint32_t top)
{
	if (likely(IS_BIT_MASK(top))) {
		return (val - old) & top;
	}

	/* if top is not 2^n-1 */
	return (val >= old) ? (val - old) : val + top + 1 - old;
}

static const struct counter_driver_api ra_agt_driver_api = {
	.start = counter_ra_agt_start,
	.stop = counter_ra_agt_stop,
	.get_value = counter_ra_agt_get_value,
	.set_alarm = counter_ra_agt_set_alarm,
	.cancel_alarm = counter_ra_agt_cancel_alarm,
	.set_top_value = counter_ra_agt_set_top_value,
	.get_pending_int = counter_ra_agt_get_pending_int,
	.get_top_value = counter_ra_agt_get_top_value,
	.get_freq = counter_ra_agt_get_freq,
	.get_guard_period = counter_ra_agt_get_guard_period,
	.set_guard_period = counter_ra_agt_set_guard_period,
};

#define TIMER(idx) DT_INST_PARENT(idx)

#define _ELC_EVENT_AGT_INT(channel)       ELC_EVENT_AGT##channel##_INT
#define _ELC_EVENT_AGT_COMPARE_A(channel) ELC_EVENT_AGT##channel##_COMPARE_A

#define ELC_EVENT_AGT_INT(channel)       _ELC_EVENT_AGT_INT(channel)
#define ELC_EVENT_AGT_COMPARE_A(channel) _ELC_EVENT_AGT_COMPARE_A(channel)

#define AGT_DEVICE_INIT_RA(n)                                                                      \
	static const struct counter_ra_agt_config ra_agt_config_##n = {                            \
		.info =                                                                            \
			{                                                                          \
				.max_top_value = UINT16_MAX,                                       \
				.freq = 0,                                                         \
				.channels = 1,                                                     \
				.flags = 0,                                                        \
			},                                                                         \
		.agtio_filter = AGT_AGTIO_FILTER_NONE,                                             \
		.measurement_mode = 0U,                                                            \
		.source_div = DT_PROP(TIMER(n), renesas_prescaler),                                \
		.count_source = DT_STRING_TOKEN(TIMER(n), renesas_count_source),                   \
		.channel = DT_PROP(TIMER(n), channel),                                             \
		.channel_irq = DT_IRQ_BY_NAME(TIMER(n), agtcmai, irq),                             \
		.channel_ipl = DT_IRQ_BY_NAME(TIMER(n), agtcmai, priority),                        \
		.cycle_end_irq = DT_IRQ_BY_NAME(TIMER(n), agti, irq),                              \
		.cycle_end_ipl = DT_IRQ_BY_NAME(TIMER(n), agti, priority),                         \
		.resolution = DT_PROP(TIMER(n), renesas_resolution),                               \
		.dt_reg = DT_REG_ADDR(TIMER(n)),                                                   \
	};                                                                                         \
                                                                                                   \
	static struct counter_ra_agt_data counter_ra_agt_data_##n = {                              \
		.period_counts = 0,                                                                \
	};                                                                                         \
                                                                                                   \
	static int counter_ra_agt_##n##_init(const struct device *dev)                             \
	{                                                                                          \
		R_ICU->IELSR[DT_IRQ_BY_NAME(TIMER(n), agti, irq)] =                                \
			ELC_EVENT_AGT_INT(DT_PROP(TIMER(n), channel));                             \
		IRQ_CONNECT(DT_IRQ_BY_NAME(TIMER(n), agti, irq),                                   \
			    DT_IRQ_BY_NAME(TIMER(n), agti, priority), counter_ra_agt_agti_isr,     \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_IRQ_BY_NAME(TIMER(n), agti, irq));                                   \
                                                                                                   \
		R_ICU->IELSR[DT_IRQ_BY_NAME(TIMER(n), agtcmai, irq)] =                             \
			ELC_EVENT_AGT_COMPARE_A(DT_PROP(TIMER(n), channel));                       \
		IRQ_CONNECT(DT_IRQ_BY_NAME(TIMER(n), agtcmai, irq),                                \
			    DT_IRQ_BY_NAME(TIMER(n), agtcmai, priority),                           \
			    counter_ra_agt_agtcmai_isr, DEVICE_DT_INST_GET(n), 0);                 \
		irq_disable(DT_IRQ_BY_NAME(TIMER(n), agtcmai, irq));                               \
                                                                                                   \
		return counter_ra_agt_init(dev);                                                   \
	}                                                                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, counter_ra_agt_##n##_init, NULL, &counter_ra_agt_data_##n,        \
			      &ra_agt_config_##n, POST_KERNEL, CONFIG_COUNTER_INIT_PRIORITY,       \
			      &ra_agt_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AGT_DEVICE_INIT_RA)

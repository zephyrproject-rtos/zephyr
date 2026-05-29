/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <soc.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/clock_control/mchp_clock_control.h>

#define DT_DRV_COMPAT microchip_tcc_g1_counter

LOG_MODULE_REGISTER(counter_mchp_tcc_g1, CONFIG_COUNTER_LOG_LEVEL);

#define ALL_TCC_SYNC_BITS                 ((uint32_t)UINT32_MAX)
#define COUNTER_RET_FAILED                (-1)
#define COUNTER_RET_PASSED                (0)
#define TCC_SYNCHRONIZATION_TIMEOUT_IN_US (5U)
#define TCC_CTRLB_TIMEOUT_IN_US           (5U)
#define DELAY_US                          (1U)
#define COMPARE_IRQ_LINE_MAX              (6U)

struct mchp_counter_clock {
	const struct device *clock_dev;
	clock_control_subsys_t host_core_sync_clk;
	clock_control_subsys_t periph_async_clk;
};

struct tcc_counter_irq_map {
	const uint32_t ovf_irq_line;                        /* Overflow interrupt number */
	const uint32_t comp_irq_line[COMPARE_IRQ_LINE_MAX]; /* Channel interrupt number */
};

/*Structure to hold channel-specific data for the counter.*/
struct counter_mchp_ch_data {
	counter_alarm_callback_t callback;
	uint32_t compare_value;
	void *user_data;
};

struct counter_mchp_dev_data {
	counter_top_callback_t top_cb;
	void *top_user_data;
	struct counter_mchp_ch_data *channel_data;
	uint32_t guard_period;
	bool late_alarm_flag;
	uint8_t late_alarm_channel;
};

struct counter_mchp_dev_config {
	struct counter_config_info info; /* General counter configuration information */
	tcc_registers_t *regs;
	struct mchp_counter_clock counter_clock;
	struct tcc_counter_irq_map *channel_irq_map;
	uint32_t max_bit_width;
	uint16_t prescaler;
	uint8_t max_channels;
	void (*irq_config_func)(const struct device *dev);
};

typedef enum {
	/* Reload or reset the counter on the next generic clock */
	TCC_GCLK_RESET_ON_GENERIC_CLOCK = 0x0,
	/* Reload or reset the counter on the next prescaler clock */
	TCC_PRESC_RESET_ON_PRESCALER_CLOCK = 0x1,
	/* Reload or reset the counter on the next generic clock, and reset the prescaler
	 * counter
	 */
	TCC_RESYNC_RESET_ON_GENERIC_CLOCK = 0x2,
} tcc_counter_prescaler_sync_mode_t;

/* find the prescale index based on the value present in the device tree */
static uint8_t get_tcc_prescale_index(uint16_t prescaler)
{
	uint8_t prescale_index = 0u;

	switch (prescaler) {
	case 64:
		prescale_index = TCC_CTRLA_PRESCALER_DIV64_Val;
		break;
	case 256:
		prescale_index = TCC_CTRLA_PRESCALER_DIV256_Val;
		break;
	case 1024:
		prescale_index = TCC_CTRLA_PRESCALER_DIV1024_Val;
		break;
	default:
		if ((prescaler >= 1) && (prescaler <= 16)) {
			prescale_index = __builtin_ctz(prescaler);
		}
		break;
	}

	return prescale_index;
}

static void tcc_counter_wait_sync(const volatile uint32_t *sync_reg_addr, uint32_t bit_mask)
{
	if (false == WAIT_FOR((((*sync_reg_addr) & bit_mask) == 0u),
			      TCC_SYNCHRONIZATION_TIMEOUT_IN_US, k_busy_wait(DELAY_US))) {
		LOG_ERR("%s : Synchronization time-out occurred", __func__);
	}
}

static void tcc_counter_ctrlbset_sync(tcc_registers_t *const p_regs, uint32_t bit_mask)
{
	if (false == WAIT_FOR((((p_regs->TCC_CTRLBSET) & bit_mask) == 0u), TCC_CTRLB_TIMEOUT_IN_US,
			      k_busy_wait(DELAY_US))) {
		LOG_ERR("%s : CTRLBSET time-out occurred", __func__);
	}
}

static void tcc_counter_init(tcc_registers_t *const p_regs, uint32_t prescaler,
			     uint8_t max_channels, const uint32_t max_bit_width)
{
	uint32_t max_counter_value;

	p_regs->TCC_CTRLA |= TCC_CTRLA_SWRST_Msk;
	tcc_counter_wait_sync(&p_regs->TCC_SYNCBUSY, TCC_SYNCBUSY_SWRST_Msk);

	/* Configure counter mode & prescaler */
	p_regs->TCC_CTRLA = TCC_CTRLA_CPTEN0(0U) | TCC_CTRLA_CPTEN1(0U) | TCC_CTRLA_CPTEN2(0U) |
			    TCC_CTRLA_CPTEN3(0U) | TCC_CTRLA_CPTEN4(0U) | TCC_CTRLA_CPTEN5(0U) |
			    TCC_CTRLA_MSYNC(0U) |
			    TCC_CTRLA_PRESCALER(get_tcc_prescale_index(prescaler)) |
			    TCC_CTRLA_PRESCSYNC(TCC_GCLK_RESET_ON_GENERIC_CLOCK) |
			    TCC_CTRLA_RUNSTDBY(0U) | TCC_CTRLA_RESOLUTION(0U);

	/* Configure waveform generation mode */
	p_regs->TCC_WAVE = TCC_WAVE_WAVEGEN_NFRQ;

	/* Configure timer one shot mode & direction */
	p_regs->TCC_CTRLBSET = TCC_CTRLBCLR_ONESHOT(0U) | TCC_CTRLBCLR_DIR(0U);

	/* Configure drive control register */
	p_regs->TCC_DRVCTRL = TCC_DRVCTRL_INVEN(0U);

	/* Set the period register to maximum value */
	max_counter_value = (uint32_t)(BIT64(max_bit_width) - 1);

	p_regs->TCC_PER = max_counter_value;

	for (uint8_t index = 0u; index < max_channels; index++) {
		p_regs->TCC_CC[index] = max_counter_value;
	}

	/* Clear all interrupt flags */
	p_regs->TCC_INTFLAG = TCC_INTFLAG_Msk;

	/* Event control register */
	p_regs->TCC_EVCTRL = TCC_EVCTRL_EVACT0(0U) | TCC_EVCTRL_EVACT1(0U) | TCC_EVCTRL_TCINV(0U) |
			     TCC_EVCTRL_TCEI(0U) | TCC_EVCTRL_OVFEO(0U) | TCC_EVCTRL_MCEO0(0U) |
			     TCC_EVCTRL_MCEO1(0U) | TCC_EVCTRL_MCEO2(0U) | TCC_EVCTRL_MCEO3(0U) |
			     TCC_EVCTRL_MCEO4(0U) | TCC_EVCTRL_MCEO5(0U);

	tcc_counter_wait_sync(&p_regs->TCC_SYNCBUSY, ALL_TCC_SYNC_BITS);
}

static void tcc_counter_retrigger(tcc_registers_t *const p_regs)
{
	/* Write command to force COUNT register read synchronization */
	p_regs->TCC_CTRLBSET |= TCC_CTRLBSET_CMD_RETRIGGER;
	tcc_counter_wait_sync(&p_regs->TCC_SYNCBUSY, TCC_SYNCBUSY_CTRLB_Msk);

	tcc_counter_ctrlbset_sync(p_regs, TCC_CTRLBSET_CMD_Msk);
}

static int32_t tcc_counter_get_count(tcc_registers_t *const p_regs, uint32_t *const counter_value)
{
	/* Write command to force COUNT register read synchronization */
	p_regs->TCC_CTRLBSET |= TCC_CTRLBSET_CMD_READSYNC;
	tcc_counter_wait_sync(&p_regs->TCC_SYNCBUSY, TCC_SYNCBUSY_CTRLB_Msk);

	/* Wait for CMD to become zero */
	tcc_counter_ctrlbset_sync(p_regs, TCC_CTRLBSET_CMD_Msk);
	*counter_value = p_regs->TCC_COUNT;

	return COUNTER_RET_PASSED;
}
static void tcc_counter_set_period(tcc_registers_t *const p_regs, const uint32_t period)
{
	p_regs->TCC_PER = period;
	tcc_counter_wait_sync(&p_regs->TCC_SYNCBUSY, TCC_SYNCBUSY_PER_Msk);
}

static inline uint32_t tcc_counter_get_period(tcc_registers_t *const p_regs)
{
	return p_regs->TCC_PER;
}

static void tcc_counter_set_compare(tcc_registers_t *const p_regs, const uint32_t chan_id,
				    const uint32_t compare_value)
{
	/* Set the period value */
	p_regs->TCC_CC[chan_id] = compare_value;
	tcc_counter_wait_sync(&p_regs->TCC_SYNCBUSY, ALL_TCC_SYNC_BITS);
}

static int32_t tcc_counter_alarm_irq_enable(tcc_registers_t *const p_regs, uint8_t max_channels,
					    const uint32_t channel_id)
{
	int32_t ret_val;

	if (channel_id < max_channels) {
		p_regs->TCC_INTENSET = TCC_INTFLAG_MC0_Msk << channel_id;
		ret_val = COUNTER_RET_PASSED;
	} else {
		LOG_ERR("channel id is greater than max channel %s", __func__);
		ret_val = COUNTER_RET_FAILED;
	}

	return ret_val;
}

static int32_t tcc_counter_alarm_irq_disable(tcc_registers_t *const p_regs, uint8_t max_channels,
					     const uint32_t channel_id)
{
	int32_t ret_val;

	if (channel_id < max_channels) {
		p_regs->TCC_INTENCLR = TCC_INTFLAG_MC0_Msk << channel_id;
		ret_val = COUNTER_RET_PASSED;
	} else {
		LOG_ERR("channel id is greater than max channel %s", __func__);
		ret_val = COUNTER_RET_FAILED;
	}

	return ret_val;
}

static int32_t tcc_counter_alarm_irq_clear(tcc_registers_t *const p_regs, uint8_t max_channels,
					   const uint32_t channel_id)
{
	int32_t ret_val;

	if (channel_id < max_channels) {
		p_regs->TCC_INTFLAG = TCC_INTFLAG_MC0_Msk << channel_id;
		ret_val = COUNTER_RET_PASSED;
	} else {
		LOG_ERR("channel id is greater than max channel %s", __func__);
		ret_val = COUNTER_RET_FAILED;
	}

	return ret_val;
}

static inline void tcc_counter_top_irq_enable(tcc_registers_t *const p_regs)
{
	/* Enable overflow interrupt */
	p_regs->TCC_INTENSET = TCC_INTFLAG_OVF_Msk;
}

static inline void tcc_counter_top_irq_disable(tcc_registers_t *const p_regs)
{
	/* Disable overflow interrupt */
	p_regs->TCC_INTENCLR = TCC_INTFLAG_OVF_Msk;
}

static inline void tcc_counter_top_irq_clear(tcc_registers_t *const p_regs)
{
	/* Clear overflow interrupt */
	p_regs->TCC_INTFLAG = TCC_INTFLAG_OVF_Msk;
}

static uint32_t tcc_counter_ticks_sub(uint32_t val, uint32_t old, uint32_t top)
{
	uint32_t ret_val;

	if (true == likely(IS_BIT_MASK(top))) {
		ret_val = (val - old) & top;
	} else {
		/* If top is not 2^n - 1, handle general wraparound case */
		ret_val = (val >= old) ? (val - old) : (val + top + 1U - old);
	}
	/*Difference (val - old) modulo (top + 1).*/
	return ret_val;
}

static uint32_t tcc_counter_ticks_add(uint32_t val1, uint32_t val2, uint32_t top)
{
	uint32_t to_top;
	uint32_t ret_val;

	if (true == likely(IS_BIT_MASK(top))) {
		ret_val = (val1 + val2) & top;
	} else {
		to_top = top - val1;
		ret_val = (val2 <= to_top) ? val1 + val2 : val2 - to_top - 1U;
	}
	/*Sum (val1 + val2) modulo (top + 1).*/
	return ret_val;
}

/* Computes the absolute difference between two counter values with wraparound.*/
static uint32_t tcc_counter_ticks_diff(uint32_t cnt_val_1, uint32_t cnt_val_2, uint32_t top)
{
	uint32_t diff = (cnt_val_1 > cnt_val_2) ? (cnt_val_1 - cnt_val_2) : (cnt_val_2 - cnt_val_1);
	uint32_t wrap_diff = top - diff;
	/*Absolute difference in ticks.*/
	return (diff < wrap_diff) ? diff : wrap_diff;
}

static int32_t counter_mchp_start(const struct device *const dev)
{
	const struct counter_mchp_dev_config *const cfg = dev->config;
	tcc_registers_t *const p_regs = cfg->regs;

	/* Set enable bit */
	p_regs->TCC_CTRLA |= TCC_CTRLA_ENABLE_Msk;
	tcc_counter_wait_sync(&p_regs->TCC_SYNCBUSY, TCC_SYNCBUSY_ENABLE_Msk);

	/* Write command to force COUNT register read synchronization */
	p_regs->TCC_CTRLBSET |= TCC_CTRLBSET_CMD_RETRIGGER;
	tcc_counter_wait_sync(&p_regs->TCC_SYNCBUSY, TCC_SYNCBUSY_CTRLB_Msk);

	/* Wait for CMD to become zero */
	tcc_counter_ctrlbset_sync(p_regs, TCC_CTRLBSET_CMD_Msk);
	return 0;
}

static int32_t counter_mchp_stop(const struct device *const dev)
{
	const struct counter_mchp_dev_config *const cfg = dev->config;
	tcc_registers_t *const p_regs = cfg->regs;

	/* Write command to force COUNT register read synchronization */
	p_regs->TCC_CTRLBSET |= TCC_CTRLBSET_CMD_STOP;
	tcc_counter_wait_sync(&p_regs->TCC_SYNCBUSY, TCC_SYNCBUSY_CTRLB_Msk);

	/* Wait for CMD to become zero */
	tcc_counter_ctrlbset_sync(p_regs, TCC_CTRLBSET_CMD_Msk);
	return COUNTER_RET_PASSED;
}

static int32_t counter_mchp_get_value(const struct device *const dev, uint32_t *const ticks)
{
	const struct counter_mchp_dev_config *const cfg = dev->config;

	return tcc_counter_get_count(cfg->regs, ticks);
}

static int32_t counter_mchp_set_alarm(const struct device *const dev, const uint8_t chan_id,
				      const struct counter_alarm_cfg *const alarm_cfg)
{
	bool irq_on_late;
	uint32_t absolute;
	int32_t ret_val = 0;
	uint32_t count_value = 0u;
	uint32_t furthest_count_value = 0u;
	int32_t count_diff = 0u;
	uint32_t top_value = 0u;
	uint32_t ticks = alarm_cfg->ticks;

	struct counter_mchp_dev_data *const data = dev->data;
	const struct counter_mchp_dev_config *const cfg = dev->config;
	uint8_t max_channels = cfg->max_channels;

	__ASSERT(chan_id < counter_get_num_of_channels(dev), "Invalid channel ID: %u (max %u)",
		 chan_id, counter_get_num_of_channels(dev));

	top_value = tcc_counter_get_period(cfg->regs);
	__ASSERT_NO_MSG(data->guard_period < top_value);

	if (ticks > top_value) {
		LOG_ERR("requested tick value is less than top period");
		return -EINVAL;
	}

	if (NULL != data->channel_data[chan_id].callback) {
		LOG_ERR("Counter Alarm is already set");
		return -EBUSY;
	}

	/* First take care of a risk of an event coming from CC being set to
	 * next tick. Reconfigure CC to future ( current counter value minus guard period
	 * is the furthestfuture).
	 */
	(void)tcc_counter_get_count(cfg->regs, &count_value);
	furthest_count_value = tcc_counter_ticks_sub(count_value, data->guard_period, top_value);

	tcc_counter_set_compare(cfg->regs, chan_id, furthest_count_value);
	tcc_counter_alarm_irq_clear(cfg->regs, max_channels, chan_id);

	/* Update new callback functions */
	data->channel_data[chan_id].callback = alarm_cfg->callback;
	data->channel_data[chan_id].user_data = alarm_cfg->user_data;

	/* Check if "Absolute Alarm" flag is set */
	absolute = alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE;

	/* Check if counter has exceeded the alarm count in absolute alarm configuration */
	if (absolute != 0) {
		count_diff = tcc_counter_ticks_diff(count_value, ticks, top_value);

		if (count_diff <= data->guard_period) {
			ret_val = -ETIME;
			irq_on_late = alarm_cfg->flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE;

			if (irq_on_late != 0) {
				data->late_alarm_flag = true;
				data->late_alarm_channel = chan_id;

				/* Update compare value*/
				data->channel_data[chan_id].compare_value = ticks;

				/* Enable interrupt and trigger immediately */
				NVIC_SetPendingIRQ(cfg->channel_irq_map->comp_irq_line[chan_id]);
			} else {
				data->channel_data[chan_id].callback = NULL;
				data->channel_data[chan_id].user_data = NULL;
			}
		} else {
			/* Update compare value*/
			data->channel_data[chan_id].compare_value = ticks;
			tcc_counter_set_compare(cfg->regs, chan_id, ticks);

			/* Enable interrupt at compare match */
			tcc_counter_alarm_irq_enable(cfg->regs, max_channels, chan_id);
		}
	} else {
		ticks = tcc_counter_ticks_add(count_value, ticks, top_value);

		/* Update compare value*/
		data->channel_data[chan_id].compare_value = ticks;
		tcc_counter_set_compare(cfg->regs, chan_id, ticks);

		/* Enable interrupt at compare match */
		tcc_counter_alarm_irq_enable(cfg->regs, max_channels, chan_id);
	}

	return ret_val;
}

static int32_t counter_mchp_cancel_alarm(const struct device *const dev, uint8_t chan_id)
{
	struct counter_mchp_dev_data *const data = dev->data;
	const struct counter_mchp_dev_config *const cfg = dev->config;
	uint8_t max_channels = cfg->max_channels;
	/* Check for valid channel  */
	__ASSERT(chan_id < counter_get_num_of_channels(dev), "Invalid channel ID: %u (max %u)",
		 chan_id, counter_get_num_of_channels(dev));

	/* Set the callback function to NULL */
	data->channel_data[chan_id].callback = NULL;

	/* Clear, and disable interrupt flags */
	tcc_counter_alarm_irq_disable(cfg->regs, max_channels, chan_id);
	tcc_counter_alarm_irq_clear(cfg->regs, max_channels, chan_id);

	/* Clear NVIC Flag to avoid retrigger */
	NVIC_ClearPendingIRQ(cfg->channel_irq_map->comp_irq_line[chan_id]);

	return 0;
}

static int32_t counter_mchp_set_top_value(const struct device *const dev,
					  const struct counter_top_cfg *const top_cfg)
{
	int32_t ret_val = 0;
	struct counter_mchp_dev_data *const data = dev->data;
	const struct counter_mchp_dev_config *const cfg = dev->config;

	/* Check if alarm callback function is already in progress */
	for (uint8_t i = 0; i < counter_get_num_of_channels(dev); i++) {
		if (NULL != data->channel_data[i].callback) {
			LOG_ERR("callback already specified %s", __func__);
			return -EBUSY;
		}
	}

	/* Check if top callback function is already in progress */
	tcc_counter_top_irq_disable(cfg->regs);
	tcc_counter_top_irq_clear(cfg->regs);

	/* Update callback functions */
	data->top_cb = top_cfg->callback;
	data->top_user_data = top_cfg->user_data;

	/* Update the counter period based on top configuration data */
	tcc_counter_set_period(cfg->regs, top_cfg->ticks);

	if ((top_cfg->flags & COUNTER_TOP_CFG_DONT_RESET) != 0) {
		/*
		 * Top trigger is on equality of the rising edge only, so
		 * manually reset it if the counter has missed the new top.
		 */
		uint32_t counter_value = 0u;

		tcc_counter_get_count(cfg->regs, &counter_value);

		if (counter_value >= top_cfg->ticks) {
			ret_val = -ETIME;
			if ((top_cfg->flags & COUNTER_TOP_CFG_RESET_WHEN_LATE) ==
			    COUNTER_TOP_CFG_RESET_WHEN_LATE) {
				tcc_counter_retrigger(cfg->regs);
			}
		}
	} else {
		tcc_counter_retrigger(cfg->regs);
	}

	/* Enable top IRQ */
	if (NULL != top_cfg->callback) {
		tcc_counter_top_irq_enable(cfg->regs);
	}

	return ret_val;
}

static uint32_t counter_mchp_get_pending_int(const struct device *const dev)
{
	const struct counter_mchp_dev_config *const cfg = dev->config;
	tcc_registers_t *const p_regs = cfg->regs;

	return p_regs->TCC_INTFLAG;
}

static uint32_t counter_mchp_get_top_value(const struct device *const dev)
{
	const struct counter_mchp_dev_config *const cfg = dev->config;

	return tcc_counter_get_period(cfg->regs);
}

static uint32_t counter_mchp_get_guard_period(const struct device *dev, uint32_t flags)
{
	struct counter_mchp_dev_data *const data = dev->data;

	return data->guard_period;
}

static int32_t counter_mchp_set_guard_period(const struct device *dev, uint32_t guard,
					     uint32_t flags)
{
	struct counter_mchp_dev_data *const data = dev->data;
	const struct counter_mchp_dev_config *const cfg = dev->config;

	if (guard > tcc_counter_get_period(cfg->regs)) {
		LOG_ERR("guard period set is invalid");
		return -EINVAL;
	}
	data->guard_period = guard;

	return 0;
}

/* Retrieves the source clock frequency and calculates the counter frequency
 * based on the device's prescaler.
 */
static uint32_t counter_mchp_get_frequency(const struct device *const dev)
{
	uint32_t source_clk_freq = 0u;
	const struct counter_mchp_dev_config *const cfg = dev->config;
	const struct mchp_counter_clock *const clk = &cfg->counter_clock;

	/* Get the clock source */
	clock_control_get_rate(clk->clock_dev, clk->periph_async_clk, &source_clk_freq);

	/* Update the info field for clock frequency based on clock rate */
	source_clk_freq = source_clk_freq / cfg->prescaler;

	return source_clk_freq;
}

static int32_t counter_mchp_init(const struct device *const dev)
{
	int32_t ret_val;
	const struct counter_mchp_dev_config *const cfg = dev->config;
	const struct mchp_counter_clock *const clk = &cfg->counter_clock;

	/* Enable host synchronous core clock */
	ret_val = clock_control_on(clk->clock_dev, clk->host_core_sync_clk);

	if ((ret_val < 0) && (ret_val != -EALREADY)) {
		LOG_ERR("%s : Unable to initialize host clock", __func__);
		return ret_val;
	}

	/* Enable peripheral asynchronous clock */
	ret_val = clock_control_on(clk->clock_dev, clk->periph_async_clk);

	if ((ret_val < 0) && (ret_val != -EALREADY)) {
		LOG_ERR("%s : Unable to initialize peripheral clock", __func__);
		return ret_val;
	}

	/* Initialize counter peripheral */
	tcc_counter_init(cfg->regs, cfg->prescaler, cfg->max_channels, cfg->max_bit_width);
	cfg->irq_config_func(dev);

	return ret_val;
}

static inline void counter_mchp_irq_0_handle(const struct device *const dev)
{
	struct counter_mchp_dev_data *const data = dev->data;
	const struct counter_mchp_dev_config *const cfg = dev->config;

	NVIC_ClearPendingIRQ(cfg->channel_irq_map->ovf_irq_line);
	tcc_counter_top_irq_clear(cfg->regs);

	if (data->top_cb != NULL) {
		data->top_cb(dev, data->top_user_data);
	}
}

static inline void counter_mchp_channel_irq_handle(const struct device *const dev, uint8_t channel)
{
	uint32_t cc_value = 0u;
	struct counter_mchp_dev_data *const data = dev->data;
	const struct counter_mchp_dev_config *const cfg = dev->config;
	counter_alarm_callback_t cb = data->channel_data[channel].callback;

	/* Immediate interrupt trigger */
	if ((data->late_alarm_flag == true) && (data->late_alarm_channel == channel)) {
		data->late_alarm_flag = false;
	} else {
		tcc_counter_alarm_irq_clear(cfg->regs, cfg->max_channels, channel);
	}

	data->channel_data[channel].callback = NULL;
	cc_value = data->channel_data[channel].compare_value;

	if (cb != NULL) {
		cb(dev, channel, cc_value, data->channel_data[channel].user_data);
	}
}

/* Wrapper for channel 0 compare (alarm) interrupt.
 * This is mapped to irq-1 (channel 0 IRQ).
 */
static inline void counter_mchp_irq_1_handle(const struct device *const dev)
{
	counter_mchp_channel_irq_handle(dev, 0);
}

/* Wrapper for channel 1 compare (alarm) interrupt.
 * This is mapped to irq-2 (channel 1 IRQ).
 */
static inline void counter_mchp_irq_2_handle(const struct device *const dev)
{
	counter_mchp_channel_irq_handle(dev, 1);
}

/* Wrapper for channel 2 compare (alarm) interrupt.
 * This is mapped to irq-3 (channel 2 IRQ).
 */
static inline void counter_mchp_irq_3_handle(const struct device *const dev)
{
	counter_mchp_channel_irq_handle(dev, 2);
}

/* Wrapper for channel 3 compare (alarm) interrupt.
 * This is mapped to irq-4 (channel 3 IRQ).
 */
static inline void counter_mchp_irq_4_handle(const struct device *const dev)
{
	counter_mchp_channel_irq_handle(dev, 3);
}

/* Wrapper for channel 4 compare (alarm) interrupt.
 * This is mapped to irq-5 (channel 4 IRQ).
 */
static inline void counter_mchp_irq_5_handle(const struct device *const dev)
{
	counter_mchp_channel_irq_handle(dev, 4);
}

/* Wrapper for channel 5 compare (alarm) interrupt.
 * This is mapped to irq-6 (channel 5 IRQ).
 */
static inline void counter_mchp_irq_6_handle(const struct device *const dev)
{
	counter_mchp_channel_irq_handle(dev, 5);
}

static DEVICE_API(counter, counter_mchp_api) = {
	.start = counter_mchp_start,
	.stop = counter_mchp_stop,
	.get_freq = counter_mchp_get_frequency,
	.get_value = counter_mchp_get_value,
	.set_alarm = counter_mchp_set_alarm,
	.cancel_alarm = counter_mchp_cancel_alarm,
	.set_top_value = counter_mchp_set_top_value,
	.get_pending_int = counter_mchp_get_pending_int,
	.get_top_value = counter_mchp_get_top_value,
	.get_guard_period = counter_mchp_get_guard_period,
	.set_guard_period = counter_mchp_set_guard_period,
};

/* clang-format off */
#define COUNTER_MCHP_CLOCK_ASSIGN(n)						\
	.counter_clock.clock_dev = DEVICE_DT_GET(DT_NODELABEL(clock)),		\
	.counter_clock.host_core_sync_clk =					\
		(void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, subsystem)),	\
	.counter_clock.periph_async_clk =					\
		(void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, gclk, subsystem)),


#define COUNTER_MCHP_IRQ_HANDLER(n)							\
	static void counter_mchp_config_##n(const struct device *dev)			\
	{										\
		 LISTIFY(DT_NUM_IRQS(DT_DRV_INST(n)), MCHP_COUNTER_IRQ_CONNECT, (;), n)	\
	}

/* This macro calculates the number of IRQs for the given instance of the Microchip TCC g1
 * counter by subtracting 1 from the total number of IRQs ( First IRQ is for OVF).
 */
#define COUNTER_MCHP_CC_NUMS(n) (DT_NUM_IRQS(DT_DRV_INST(n)) - 1u)

#define COUNTER_MCHP_MAX_BIT_WIDTH(n) (DT_INST_PROP(n, max_bit_width))
/*UTIL_INC is used here because the channel irqs are starting from the index 1*/
#define COUNTER_MCHP_COMP_IRQ_IDX(idx, n) DT_INST_IRQ_BY_IDX(n, UTIL_INC(idx), irq)

/* This macro declares a static IRQ mapping structure for the specified instance
 * of a Microchip counter device. The structure maps IRQs to channels based on
 * the number of compare/capture channels (`CC_NUMS`) for the device instance.
 */
#define COUNTER_MCHP_IRQ_MAP_VAR(n)						\
	static struct tcc_counter_irq_map counter_mchp_irq_map_##n = {		\
		.ovf_irq_line = DT_INST_IRQ_BY_IDX(n, 0, irq),			\
		.comp_irq_line = {						\
			LISTIFY(UTIL_DEC(DT_NUM_IRQS(DT_DRV_INST(n))),		\
			COUNTER_MCHP_COMP_IRQ_IDX,				\
			(,), n) }}
/* UTIL_DEC is used above so that the total number of the irqs is reduced
 * as the first one is for the overflow
 */
#define COUNTER_MCHP_CONFIG_VAR(n)                                                              \
	static const struct counter_mchp_dev_config counter_mchp_dev_config_##n = {		\
		.info = {.max_top_value =                                                       \
				 ((uint32_t)((1ULL << COUNTER_MCHP_MAX_BIT_WIDTH(n)) - 1)),     \
			 .freq = 0u,                                                            \
			 .flags = COUNTER_CONFIG_INFO_COUNT_UP,                                 \
			 .channels = COUNTER_MCHP_CC_NUMS(n)},                                  \
		.regs = (tcc_registers_t *)DT_INST_REG_ADDR(n),                                 \
		COUNTER_MCHP_CLOCK_ASSIGN(n)							\
		.channel_irq_map = &counter_mchp_irq_map_##n,					\
		.max_bit_width = COUNTER_MCHP_MAX_BIT_WIDTH(n),                                 \
		.prescaler = DT_INST_PROP_OR(n, prescaler, 1),                                  \
		.max_channels = DT_INST_PROP_OR(n, channels, 0),				\
		.irq_config_func = &counter_mchp_config_##n,                                    \
	};

/*
 * This macro creates a static array of channel data structures for the specified
 * instance of a Microchip counter device. The size of the array is determined
 * by the number of compare/capture channels (`CC_NUMS`) for the device instance.
 */
#define COUNTER_MCHP_CHANNEL_DATA_VAR(n)                                                           \
	static struct counter_mchp_ch_data counter_mchp_channel_data_##n[COUNTER_MCHP_CC_NUMS(n)];

#define COUNTER_MCHP_DEV_DATA_VAR(n)                                            \
	static struct counter_mchp_dev_data counter_mchp_dev_data_##n = {	\
		.channel_data = counter_mchp_channel_data_##n};

/*
 * This macro connects an IRQ handler to the specified IRQ line for a Microchip
 * counter device instance and enables the IRQ. The IRQ number and priority are
 * retrieved from the device tree.
 */
#define MCHP_COUNTER_IRQ_CONNECT(m, n)								\
	COND_CODE_1(DT_IRQ_HAS_IDX(DT_DRV_INST(n), m),						\
	(											\
		do {										\
			IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, m, irq),				\
				DT_INST_IRQ_BY_IDX(n, m, priority),				\
				counter_mchp_irq_##m##_handle, DEVICE_DT_INST_GET(n), 0);	\
			irq_enable(DT_INST_IRQ_BY_IDX(n, m, irq));				\
		} while (false);								\
	), ())

#define COUNTER_MCHP_DEVICE_INIT_FUNC(n)	\
	COUNTER_MCHP_IRQ_MAP_VAR(n);            \
	COUNTER_MCHP_IRQ_HANDLER(n)             \
	COUNTER_MCHP_CONFIG_VAR(n);             \
	COUNTER_MCHP_CHANNEL_DATA_VAR(n)        \
	COUNTER_MCHP_DEV_DATA_VAR(n)

#define COUNTER_MCHP_DEVICE_INIT(n)							\
	COUNTER_MCHP_DEVICE_INIT_FUNC(n)						\
	DEVICE_DT_INST_DEFINE(n, counter_mchp_init, NULL, &counter_mchp_dev_data_##n,	\
			      &counter_mchp_dev_config_##n, POST_KERNEL,		\
			      CONFIG_COUNTER_INIT_PRIORITY, &counter_mchp_api);
/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(COUNTER_MCHP_DEVICE_INIT)

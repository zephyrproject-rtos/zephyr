/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/clock_control/mchp_clock_control.h>

#define DT_DRV_COMPAT microchip_rtc_g1_counter

LOG_MODULE_REGISTER(counter_mchp_rtc_g1, CONFIG_COUNTER_LOG_LEVEL);

/* All timer/counter synchronization bits set. */
#define ALL_RTC_SYNC_BITS                 ((uint32_t)UINT32_MAX)
#define COUNTER_RET_FAILED                (-1)
#define COUNTER_RET_PASSED                (0)
#define RTC_SYNCHRONIZATION_TIMEOUT_IN_US (5000U)
#define DELAY_US                          (10U)

struct mchp_counter_clock {
	const struct device *clock_dev;
	clock_control_subsys_t host_core_sync_clk;
	clock_control_subsys_t periph_async_clk;
};

struct counter_mchp_ch_data {
	counter_alarm_callback_t callback;
	uint32_t compare_value;
	void *user_data;
};

struct counter_mchp_dev_data {
	counter_top_callback_t top_cb;
	void *top_user_data;
	uint32_t guard_period;
	struct counter_mchp_ch_data *channel_data;
	bool late_alarm_flag;
	uint8_t late_alarm_channel;
};

struct counter_mchp_dev_config {
	struct counter_config_info info;
	void *regs;
	const struct pinctrl_dev_config *pcfg;
	struct mchp_counter_clock counter_clock;
	uint32_t irq_line;
	uint32_t max_bit_width;
	uint16_t prescaler;
	void (*irq_config_func)(const struct device *dev);
};

typedef enum {
	COUNTER_BIT_MODE_8 = 8,
	COUNTER_BIT_MODE_16 = 16,
	COUNTER_BIT_MODE_32 = 32,
} rtc_counter_modes_t;

/* this function will ge the value of the prescale index from prescaler val in dts */
static inline uint8_t get_rtc_prescale_index(uint16_t prescaler)
{
	return __builtin_ctz(prescaler) + 1;
}

static void rtc_counter_wait_sync(const volatile uint32_t *sync_reg_addr, uint32_t bit_mask)
{
	/* WAIT_FOR returns true if condition met before timeout, false otherwise */
	bool success = WAIT_FOR((((*sync_reg_addr) & bit_mask) == 0U),
				RTC_SYNCHRONIZATION_TIMEOUT_IN_US, k_busy_wait(DELAY_US));

	if (!success) {
		LOG_ERR("%s : Synchronization time-out occurred", __func__);
	}
}

/*
 * Wait until the counter value differs from the RTC_COUNT register, with 5ms timeout.
 *
 * This function polls the RTC_COUNT register and waits until its value differs
 * from the value pointed to by @p counter_value. If the value does not change
 * within 5 milliseconds, the function returns a timeout error.
 *
 */
static void rtc_counter_wait_count_change(const void *regs, const uint32_t *counter_value,
					  const uint32_t max_bit_width)
{
	switch (max_bit_width) {
	case COUNTER_BIT_MODE_32: {
		rtc_mode0_registers_t *const p_regs =
			(rtc_mode0_registers_t *const)(&(((rtc_registers_t *)regs)->MODE0));

		if ((p_regs->RTC_CTRLA & RTC_MODE0_CTRLA_ENABLE_Msk) != 0) {
			bool success =
				WAIT_FOR((*counter_value != p_regs->RTC_COUNT),
					 RTC_SYNCHRONIZATION_TIMEOUT_IN_US, k_busy_wait(DELAY_US));
			if (!success) {
				LOG_ERR("%s : Synchronization time-out occurred %d", __func__,
					max_bit_width);
			}
		}
		break;
	}
	case COUNTER_BIT_MODE_16: {
		rtc_mode1_registers_t *const p_regs =
			(rtc_mode1_registers_t *const)(&(((rtc_registers_t *)regs)->MODE1));

		if ((p_regs->RTC_CTRLA & RTC_MODE0_CTRLA_ENABLE_Msk) != 0) {
			bool success =
				WAIT_FOR((*counter_value != p_regs->RTC_COUNT),
					 RTC_SYNCHRONIZATION_TIMEOUT_IN_US, k_busy_wait(DELAY_US));
			if (!success) {
				LOG_ERR("%s : Synchronization time-out occurred %d", __func__,
					max_bit_width);
			}
		}
		break;
	}
	default:
		LOG_ERR("%s : Unsupported Counter mode %d", __func__, max_bit_width);
		break;
	}
}

/*
 * It performs the following steps:
 *	- Disables and resets the counter.
 *	- Configures the counter for either 16-bit or 32-bit mode, based on max_bit_width.
 *	- Sets the counter to count up in free-running mode.
 *	- Sets the period and compare values to their maximum.
 *	- Sets CTRLA.MATCHCLR to use channel 0 to control top value.
 *		(This configures the RTC so that when the counter value matches the value
 *		in compare channel 0, the counter is automatically cleared to zero. This
 *		allows for set and control top value)
 */
static int32_t rtc_counter_init(const void *regs, uint32_t prescaler, const uint32_t max_bit_width)
{
	int32_t return_value = COUNTER_RET_PASSED;

	switch (max_bit_width) {
	case COUNTER_BIT_MODE_32: {
		rtc_mode0_registers_t *const p_regs =
			(rtc_mode0_registers_t *const)(&(((rtc_registers_t *)regs)->MODE0));

		p_regs->RTC_CTRLA &= ~RTC_MODE0_CTRLA_ENABLE_Msk;
		rtc_counter_wait_sync(&p_regs->RTC_SYNCBUSY, RTC_MODE0_SYNCBUSY_ENABLE_Msk);

		p_regs->RTC_CTRLA = RTC_MODE0_CTRLA_SWRST_Msk;
		rtc_counter_wait_sync(&p_regs->RTC_SYNCBUSY, RTC_MODE0_SYNCBUSY_SWRST_Msk);

		p_regs->RTC_CTRLA = RTC_MODE0_CTRLA_MODE(0U) | RTC_MODE0_CTRLA_MATCHCLR(1U) |
				    RTC_MODE0_CTRLA_COUNTSYNC(1U) |
				    RTC_MODE0_CTRLA_PRESCALER(get_rtc_prescale_index(prescaler));

		p_regs->RTC_COMP[0U] = UINT32_MAX;
		p_regs->RTC_COMP[1U] = UINT32_MAX;
		p_regs->RTC_INTFLAG = RTC_MODE0_INTFLAG_Msk;
		rtc_counter_wait_sync(&p_regs->RTC_SYNCBUSY, ALL_RTC_SYNC_BITS);
		break;
	}
	case COUNTER_BIT_MODE_16: {
		rtc_mode1_registers_t *const p_regs =
			(rtc_mode1_registers_t *const)(&(((rtc_registers_t *)regs)->MODE1));

		p_regs->RTC_CTRLA &= ~RTC_MODE1_CTRLA_ENABLE_Msk;
		rtc_counter_wait_sync(&p_regs->RTC_SYNCBUSY, RTC_MODE1_SYNCBUSY_ENABLE_Msk);

		p_regs->RTC_CTRLA = RTC_MODE1_CTRLA_SWRST_Msk;
		rtc_counter_wait_sync(&p_regs->RTC_SYNCBUSY, RTC_MODE1_SYNCBUSY_SWRST_Msk);

		p_regs->RTC_CTRLA = RTC_MODE1_CTRLA_MODE(1U) | RTC_MODE1_CTRLA_COUNTSYNC(1U) |
				    RTC_MODE1_CTRLA_PRESCALER(get_rtc_prescale_index(prescaler));

		p_regs->RTC_PER = UINT16_MAX;
		p_regs->RTC_COMP[0U] = UINT16_MAX;
		p_regs->RTC_COMP[1U] = UINT16_MAX;
		p_regs->RTC_COMP[2U] = UINT16_MAX;
		p_regs->RTC_COMP[3U] = UINT16_MAX;
		p_regs->RTC_INTFLAG = RTC_MODE1_INTFLAG_Msk;
		rtc_counter_wait_sync(&p_regs->RTC_SYNCBUSY, ALL_RTC_SYNC_BITS);
		break;
	}
	default:
		LOG_ERR("%s : Unsupported Counter mode %d", __func__, max_bit_width);
		return_value = COUNTER_RET_FAILED;
		break;
	}

	return return_value;
}

static int32_t rtc_counter_disable(const void *regs, const uint32_t max_bit_width)
{
	int32_t return_value = COUNTER_RET_PASSED;

	switch (max_bit_width) {
	case COUNTER_BIT_MODE_32: {
		rtc_mode0_registers_t *const p_regs =
			(rtc_mode0_registers_t *const)(&(((rtc_registers_t *)regs)->MODE0));
		p_regs->RTC_CTRLA &= ~RTC_MODE0_CTRLA_ENABLE_Msk;
		rtc_counter_wait_sync(&p_regs->RTC_SYNCBUSY, RTC_MODE0_SYNCBUSY_ENABLE_Msk);
		break;
	}
	case COUNTER_BIT_MODE_16: {
		rtc_mode1_registers_t *const p_regs =
			(rtc_mode1_registers_t *const)(&(((rtc_registers_t *)regs)->MODE1));
		p_regs->RTC_CTRLA &= ~RTC_MODE1_CTRLA_ENABLE_Msk;
		rtc_counter_wait_sync(&p_regs->RTC_SYNCBUSY, RTC_MODE1_SYNCBUSY_ENABLE_Msk);
		break;
	}
	default:
		LOG_ERR("%s : Unsupported Counter mode %d", __func__, max_bit_width);
		return_value = COUNTER_RET_FAILED;
		break;
	}

	return return_value;
}

static int32_t rtc_counter_start(const void *regs, const uint32_t max_bit_width)
{
	int32_t return_value = COUNTER_RET_PASSED;

	switch (max_bit_width) {
	case COUNTER_BIT_MODE_32: {
		rtc_mode0_registers_t *const p_regs =
			(rtc_mode0_registers_t *const)(&(((rtc_registers_t *)regs)->MODE0));
		p_regs->RTC_CTRLA |= RTC_MODE0_CTRLA_ENABLE_Msk;
		rtc_counter_wait_sync(&p_regs->RTC_SYNCBUSY, RTC_MODE0_SYNCBUSY_ENABLE_Msk);
	}
	case COUNTER_BIT_MODE_16: {
		rtc_mode1_registers_t *const p_regs =
			(rtc_mode1_registers_t *const)(&(((rtc_registers_t *)regs)->MODE1));
		p_regs->RTC_CTRLA |= RTC_MODE1_CTRLA_ENABLE_Msk;
		rtc_counter_wait_sync(&p_regs->RTC_SYNCBUSY, RTC_MODE1_SYNCBUSY_ENABLE_Msk);
		break;
	}
	default:
		LOG_ERR("%s : Unsupported Counter mode %d", __func__, max_bit_width);
		return_value = COUNTER_RET_FAILED;
		break;
	}

	return return_value;
}

static inline int32_t rtc_counter_stop(const void *regs, const uint32_t max_bit_width)
{
	return rtc_counter_disable(regs, max_bit_width);
}

static int32_t rtc_counter_retrigger(const void *regs, const uint32_t max_bit_width)
{
	int32_t return_value = COUNTER_RET_PASSED;

	switch (max_bit_width) {
	case COUNTER_BIT_MODE_32: {
		rtc_mode0_registers_t *const p_regs =
			(rtc_mode0_registers_t *const)(&(((rtc_registers_t *)regs)->MODE0));
		p_regs->RTC_COUNT = 0u;
		p_regs->RTC_CTRLA |= RTC_MODE0_CTRLA_ENABLE_Msk;
		rtc_counter_wait_sync(&p_regs->RTC_SYNCBUSY, ALL_RTC_SYNC_BITS);
		break;
	}
	case COUNTER_BIT_MODE_16: {
		rtc_mode1_registers_t *const p_regs =
			(rtc_mode1_registers_t *const)(&(((rtc_registers_t *)regs)->MODE1));
		p_regs->RTC_COUNT = 0u;
		p_regs->RTC_CTRLA |= RTC_MODE1_CTRLA_ENABLE_Msk;
		rtc_counter_wait_sync(&p_regs->RTC_SYNCBUSY, ALL_RTC_SYNC_BITS);
		break;
	}
	default:
		LOG_ERR("%s : Unsupported Counter mode %d", __func__, max_bit_width);
		return_value = COUNTER_RET_FAILED;
		break;
	}

	return return_value;
}

static uint32_t rtc_counter_get_count(const void *regs, uint32_t *const counter_value,
				      const uint32_t max_bit_width)
{
	uint32_t return_value = COUNTER_RET_PASSED;

	switch (max_bit_width) {
	case COUNTER_BIT_MODE_32: {
		rtc_mode0_registers_t *const p_regs =
			(rtc_mode0_registers_t *const)(&(((rtc_registers_t *)regs)->MODE0));

		if ((p_regs->RTC_CTRLA & RTC_MODE0_CTRLA_COUNTSYNC_Msk) == 0u) {
			p_regs->RTC_CTRLA |= RTC_MODE0_CTRLA_COUNTSYNC_Msk;
			rtc_counter_wait_sync(&p_regs->RTC_SYNCBUSY,
					      RTC_MODE0_SYNCBUSY_COUNTSYNC_Msk);
		}
		rtc_counter_wait_sync(&p_regs->RTC_SYNCBUSY, RTC_MODE0_SYNCBUSY_COUNT_Msk);

		*counter_value = p_regs->RTC_COUNT;
		/* RTC Errata: TMR102-19
		 *   When COUNTSYNC is enabled, the first COUNT value is not correctly
		 *   synchronized and thus it is a wrong value.
		 * Workaround
		 *   After enabling COUNTSYNC, read the COUNT register until its
		 *   value is changed when compared to its first value read. After this,
		 *   all consequent value read from the COUNT register is valid.
		 */
		rtc_counter_wait_count_change(regs, counter_value, max_bit_width);
		*counter_value = p_regs->RTC_COUNT;

		break;
	}
	case COUNTER_BIT_MODE_16: {
		rtc_mode1_registers_t *const p_regs =
			(rtc_mode1_registers_t *const)(&(((rtc_registers_t *)regs)->MODE1));
		*counter_value = p_regs->RTC_COUNT;

		/* RTC Errata: TMR102-19
		 *   When COUNTSYNC is enabled, the first COUNT value is not correctly
		 *   synchronized and thus it is a wrong value.
		 * Workaround
		 *   After enabling COUNTSYNC, read the COUNT register until its
		 *   value is changed when compared to its first value read. After this,
		 *   all consequent value read from the COUNT register is valid.
		 */
		rtc_counter_wait_count_change(regs, counter_value, max_bit_width);
		*counter_value = p_regs->RTC_COUNT;

		break;
	}
	default:
		LOG_ERR("%s : Unsupported Counter mode %d", __func__, max_bit_width);
		return_value = COUNTER_RET_FAILED;
		break;
	}

	return return_value;
}

static int32_t rtc_counter_set_period(const void *regs, const uint32_t period,
				      const uint32_t max_bit_width)
{
	int32_t return_value = COUNTER_RET_PASSED;

	switch (max_bit_width) {
	case COUNTER_BIT_MODE_32: {
		rtc_mode0_registers_t *const p_regs =
			(rtc_mode0_registers_t *const)(&(((rtc_registers_t *)regs)->MODE0));
		p_regs->RTC_COMP[0] = period;
		rtc_counter_wait_sync(&p_regs->RTC_SYNCBUSY, RTC_MODE0_SYNCBUSY_COMP0_Msk);
		break;
	}
	case COUNTER_BIT_MODE_16: {
		rtc_mode1_registers_t *const p_regs =
			(rtc_mode1_registers_t *const)(&(((rtc_registers_t *)regs)->MODE1));
		p_regs->RTC_PER = period;
		rtc_counter_wait_sync(&p_regs->RTC_SYNCBUSY, RTC_MODE1_SYNCBUSY_PER_Msk);
		break;
	}
	default:
		LOG_ERR("%s : Unsupported Counter mode %d", __func__, max_bit_width);
		return_value = COUNTER_RET_FAILED;
		break;
	}

	return return_value;
}

static int32_t rtc_counter_get_period(const void *regs, uint32_t *period,
				      const uint32_t max_bit_width)
{
	uint32_t return_value = COUNTER_RET_PASSED;

	switch (max_bit_width) {
	case COUNTER_BIT_MODE_32: {
		rtc_mode0_registers_t *const p_regs =
			(rtc_mode0_registers_t *const)(&(((rtc_registers_t *)regs)->MODE0));
		*period = p_regs->RTC_COMP[0];
		break;
	}
	case COUNTER_BIT_MODE_16: {
		rtc_mode1_registers_t *const p_regs =
			(rtc_mode1_registers_t *const)(&(((rtc_registers_t *)regs)->MODE1));
		*period = p_regs->RTC_PER;
		break;
	}
	default:
		LOG_ERR("%s : Unsupported Counter mode %d", __func__, max_bit_width);
		return_value = COUNTER_RET_FAILED;
		break;
	}

	return return_value;
}

static int32_t rtc_counter_set_compare(const void *regs, const uint32_t chan_id,
				       const uint32_t compare_value, const uint32_t max_bit_width)
{
	int32_t return_value = COUNTER_RET_PASSED;

	switch (max_bit_width) {
	case COUNTER_BIT_MODE_32: {
		rtc_mode0_registers_t *const p_regs =
			(rtc_mode0_registers_t *const)(&(((rtc_registers_t *)regs)->MODE0));

		p_regs->RTC_COMP[1u] = compare_value;
		rtc_counter_wait_sync(&p_regs->RTC_SYNCBUSY, ALL_RTC_SYNC_BITS);
		break;
	}
	case COUNTER_BIT_MODE_16: {
		rtc_mode1_registers_t *const p_regs =
			(rtc_mode1_registers_t *const)(&(((rtc_registers_t *)regs)->MODE1));

		p_regs->RTC_COMP[chan_id] = compare_value;
		rtc_counter_wait_sync(&p_regs->RTC_SYNCBUSY, ALL_RTC_SYNC_BITS);
		break;
	}
	default:
		LOG_ERR("%s : Unsupported Counter mode %d", __func__, max_bit_width);
		return_value = COUNTER_RET_FAILED;
		break;
	}

	return return_value;
}

static int32_t rtc_counter_get_pending_irqs(const void *regs, const uint32_t max_bit_width)
{
	int32_t ret_status = 0;

	switch (max_bit_width) {
	case COUNTER_BIT_MODE_32: {
		rtc_mode0_registers_t *const p_regs =
			(rtc_mode0_registers_t *const)(&(((rtc_registers_t *)regs)->MODE0));

		ret_status = p_regs->RTC_INTFLAG;
		break;
	}
	case COUNTER_BIT_MODE_16: {
		rtc_mode1_registers_t *const p_regs =
			(rtc_mode1_registers_t *const)(&(((rtc_registers_t *)regs)->MODE1));

		ret_status = p_regs->RTC_INTFLAG;
		break;
	}
	default:
		LOG_ERR("%s : Unsupported Counter mode %d", __func__, max_bit_width);
		break;
	}

	return ret_status;
}

static int32_t rtc_counter_alarm_irq_enable(const void *regs, const uint32_t channel_id,
					    const uint32_t max_bit_width)
{
	int32_t ret_status = COUNTER_RET_PASSED;

	switch (max_bit_width) {
	case COUNTER_BIT_MODE_32: {
		rtc_mode0_registers_t *const p_regs =
			(rtc_mode0_registers_t *const)(&(((rtc_registers_t *)regs)->MODE0));

		if (channel_id == 0u) {
			p_regs->RTC_INTENSET = RTC_MODE0_INTFLAG_CMP1_Msk;
		} else {
			ret_status = COUNTER_RET_FAILED;
		}
		break;
	}
	case COUNTER_BIT_MODE_16: {
		rtc_mode1_registers_t *const p_regs =
			(rtc_mode1_registers_t *const)(&(((rtc_registers_t *)regs)->MODE1));

		p_regs->RTC_INTENSET = RTC_MODE0_INTFLAG_CMP0_Msk << channel_id;

		break;
	}
	default:
		LOG_ERR("%s : Unsupported Counter mode %d", __func__, max_bit_width);
		ret_status = COUNTER_RET_FAILED;
		break;
	}

	return ret_status;
}

static int32_t rtc_counter_alarm_irq_disable(const void *regs, const uint32_t channel_id,
					     const uint32_t max_bit_width)
{
	int32_t ret_status = COUNTER_RET_PASSED;

	switch (max_bit_width) {
	case COUNTER_BIT_MODE_32: {
		rtc_mode0_registers_t *const p_regs =
			(rtc_mode0_registers_t *const)(&(((rtc_registers_t *)regs)->MODE0));

		if (channel_id == 0u) {
			p_regs->RTC_INTENCLR = RTC_MODE0_INTFLAG_CMP1_Msk;
		} else {
			ret_status = COUNTER_RET_FAILED;
		}
		break;
	}
	case COUNTER_BIT_MODE_16: {
		rtc_mode1_registers_t *const p_regs =
			(rtc_mode1_registers_t *const)(&(((rtc_registers_t *)regs)->MODE1));

		p_regs->RTC_INTENCLR = 1U << ((uint32_t)RTC_MODE1_INTFLAG_CMP0_Msk + channel_id);
	}
	default:
		LOG_ERR("%s : Unsupported Counter mode %d", __func__, max_bit_width);
		ret_status = COUNTER_RET_FAILED;
		break;
	}

	return ret_status;
}

static int32_t rtc_counter_alarm_irq_clear(const void *regs, const uint32_t channel_id,
					   const uint32_t max_bit_width)
{
	int32_t ret_status = COUNTER_RET_PASSED;

	switch (max_bit_width) {
	case COUNTER_BIT_MODE_32: {
		rtc_mode0_registers_t *const p_regs =
			(rtc_mode0_registers_t *const)(&(((rtc_registers_t *)regs)->MODE0));

		if (channel_id == 0u) {
			p_regs->RTC_INTFLAG = RTC_MODE0_INTFLAG_CMP1_Msk;
		} else {
			ret_status = COUNTER_RET_FAILED;
		}
		break;
	}
	case COUNTER_BIT_MODE_16: {
		rtc_mode1_registers_t *const p_regs =
			(rtc_mode1_registers_t *const)(&(((rtc_registers_t *)regs)->MODE1));

		p_regs->RTC_INTFLAG = RTC_MODE0_INTFLAG_CMP0_Msk << channel_id;
		break;
	}
	default:
		LOG_ERR("%s : Unsupported Counter mode %d", __func__, max_bit_width);
		ret_status = COUNTER_RET_FAILED;
		break;
	}

	return ret_status;
}

static bool rtc_counter_alarm_irq_status(const uint32_t pending_irq_status,
					 const uint32_t channel_id, const uint32_t max_bit_width)
{
	int32_t ret_status = true;

	switch (max_bit_width) {
	case COUNTER_BIT_MODE_32: {
		if (channel_id == 0u) {
			ret_status = (bool)((pending_irq_status & RTC_MODE0_INTFLAG_CMP1_Msk) ==
					    RTC_MODE0_INTFLAG_CMP1_Msk);
		} else {
			ret_status = false;
		}
		break;
	}
	case COUNTER_BIT_MODE_16: {
		uint32_t channel_mask = RTC_MODE1_INTFLAG_CMP0_Msk << channel_id;

		ret_status =
			(bool)((pending_irq_status & channel_mask) == RTC_MODE1_INTFLAG_CMP0_Msk);
		break;
	}
	default:
		LOG_ERR("%s : Unsupported Counter mode %d", __func__, max_bit_width);
		ret_status = false;
		break;
	}

	return ret_status;
}

static int32_t rtc_counter_top_irq_enable(const void *regs, const uint32_t max_bit_width)
{
	int32_t return_value = COUNTER_RET_PASSED;

	switch (max_bit_width) {
	case COUNTER_BIT_MODE_32: {
		rtc_mode0_registers_t *const p_regs =
			(rtc_mode0_registers_t *const)(&(((rtc_registers_t *)regs)->MODE0));
		p_regs->RTC_INTENSET = RTC_MODE0_INTFLAG_CMP0_Msk;
		break;
	}
	case COUNTER_BIT_MODE_16: {
		rtc_mode1_registers_t *const p_regs =
			(rtc_mode1_registers_t *const)(&(((rtc_registers_t *)regs)->MODE1));
		p_regs->RTC_INTENSET = RTC_MODE1_INTFLAG_OVF_Msk;
		break;
	}
	default:
		LOG_ERR("%s : Unsupported Counter mode %d", __func__, max_bit_width);
		return_value = COUNTER_RET_FAILED;
		break;
	}

	return return_value;
}

static int32_t rtc_counter_top_irq_disable(const void *regs, const uint32_t max_bit_width)
{
	int32_t return_value = COUNTER_RET_PASSED;

	switch (max_bit_width) {
	case COUNTER_BIT_MODE_32: {
		rtc_mode0_registers_t *const p_regs =
			(rtc_mode0_registers_t *const)(&(((rtc_registers_t *)regs)->MODE0));

		p_regs->RTC_INTENCLR = RTC_MODE0_INTFLAG_CMP0_Msk;
		break;
	}
	case COUNTER_BIT_MODE_16: {
		rtc_mode1_registers_t *const p_regs =
			(rtc_mode1_registers_t *const)(&(((rtc_registers_t *)regs)->MODE1));

		p_regs->RTC_INTENCLR = RTC_MODE1_INTFLAG_OVF_Msk;
		break;
	}
	default:
		LOG_ERR("%s : Unsupported Counter mode %d", __func__, max_bit_width);
		return_value = COUNTER_RET_FAILED;
		break;
	}

	return return_value;
}

static int32_t rtc_counter_top_irq_clear(const void *regs, const uint32_t max_bit_width)
{
	int32_t return_value = COUNTER_RET_PASSED;

	switch (max_bit_width) {
	case COUNTER_BIT_MODE_32: {
		rtc_mode0_registers_t *const p_regs =
			(rtc_mode0_registers_t *const)(&(((rtc_registers_t *)regs)->MODE0));

		p_regs->RTC_INTFLAG = RTC_MODE0_INTFLAG_CMP0_Msk;
		break;
	}
	case COUNTER_BIT_MODE_16: {
		rtc_mode1_registers_t *const p_regs =
			(rtc_mode1_registers_t *const)(&(((rtc_registers_t *)regs)->MODE1));

		p_regs->RTC_INTFLAG = RTC_MODE1_INTFLAG_OVF_Msk;
		break;
	}
	default:
		LOG_ERR("%s : Unsupported Counter mode %d", __func__, max_bit_width);
		return_value = COUNTER_RET_FAILED;
		break;
	}

	return return_value;
}

static bool rtc_counter_top_irq_status(const uint32_t pending_irq_status,
				       const uint32_t max_bit_width)
{
	bool return_value = COUNTER_RET_PASSED;

	switch (max_bit_width) {
	case COUNTER_BIT_MODE_32: {
		return_value = (bool)((pending_irq_status & RTC_MODE0_INTFLAG_CMP0_Msk) ==
				      RTC_MODE0_INTFLAG_CMP0_Msk);
		break;
	}
	case COUNTER_BIT_MODE_16: {
		return_value = (bool)((pending_irq_status & RTC_MODE1_INTFLAG_OVF_Msk) ==
				      RTC_MODE1_INTFLAG_OVF_Msk);
		break;
	}
	default:
		LOG_ERR("%s : Unsupported Counter mode %d", __func__, max_bit_width);
		return_value = false;
		break;
	}

	return return_value;
}

/* Computes the difference between two tick values considering wraparound */
static uint32_t rtc_counter_ticks_sub(uint32_t val, uint32_t old, uint32_t top)
{
	uint32_t ret_status = 0;

	if (true == likely(IS_BIT_MASK(top))) {
		ret_status = (val - old) & top;
	} else {
		/* If top is not 2^n - 1, handle general wraparound case */
		ret_status = (val >= old) ? (val - old) : (val + top + 1U - old);
	}
	return ret_status;
}

/* Adds two tick values considering counter wraparound */
static uint32_t rtc_counter_ticks_add(uint32_t val1, uint32_t val2, uint32_t top)
{
	uint32_t ret_val = 0;
	uint64_t sum = (uint64_t)(val1 + val2);

	if (true == likely(IS_BIT_MASK(top))) {
		ret_val = sum & top;
	} else {
		ret_val = (uint32_t)(sum % top);
	}

	return ret_val;
}

/* Computes absolute difference between two counter values with wraparound */
static uint32_t rtc_counter_ticks_diff(uint32_t cnt_val_1, uint32_t cnt_val_2, uint32_t top)
{
	uint32_t diff = (cnt_val_1 > cnt_val_2) ? (cnt_val_1 - cnt_val_2) : (cnt_val_2 - cnt_val_1);
	uint32_t wrap_diff = top - diff;

	return (diff < wrap_diff) ? diff : wrap_diff;
}

static int32_t counter_mchp_start(const struct device *const dev)
{
	const struct counter_mchp_dev_config *const cfg = dev->config;

	rtc_counter_start(cfg->regs, cfg->max_bit_width);

	return 0;
}

static int32_t counter_mchp_stop(const struct device *const dev)
{
	const struct counter_mchp_dev_config *const cfg = dev->config;

	rtc_counter_stop(cfg->regs, cfg->max_bit_width);

	return 0;
}

static int32_t counter_mchp_get_value(const struct device *const dev, uint32_t *const ticks)
{
	const struct counter_mchp_dev_config *const cfg = dev->config;

	return rtc_counter_get_count(cfg->regs, ticks, cfg->max_bit_width);
}

static int32_t counter_mchp_set_alarm(const struct device *const dev, const uint8_t chan_id,
				      const struct counter_alarm_cfg *const alarm_cfg)
{
	bool irq_on_late;
	uint32_t absolute;
	int32_t ret_status = 0;
	uint32_t count_value = 0u;
	uint32_t furthest_count_value = 0u;
	int32_t count_diff = 0u;
	uint32_t top_value = 0u;
	uint32_t ticks = alarm_cfg->ticks;

	struct counter_mchp_dev_data *const data = dev->data;
	const struct counter_mchp_dev_config *const cfg = dev->config;

	/* Check for valid channel  */
	__ASSERT(chan_id < counter_get_num_of_channels(dev), "Invalid channel ID: %u (max %u)",
		 chan_id, counter_get_num_of_channels(dev));
	/* Get top value */
	rtc_counter_get_period(cfg->regs, &top_value, cfg->max_bit_width);
	__ASSERT_NO_MSG(data->guard_period < top_value);

	/* Check if the requested tick value is less than top (period) value */
	if (ticks > top_value) {
		LOG_ERR("tick value is greater than top value");
		return -EINVAL;
	}

	if (NULL != data->channel_data[chan_id].callback) {
		LOG_ERR("alarm callback already set");
		return -EBUSY;
	}

	/* First take care of a risk of an event coming from CC being set to
	 * next tick. Reconfigure CC to future ( current counter value minus guard period
	 * is the furthestfuture).
	 */
	(void)rtc_counter_get_count(cfg->regs, &count_value, cfg->max_bit_width);
	furthest_count_value = rtc_counter_ticks_sub(count_value, data->guard_period, top_value);

	rtc_counter_set_compare(cfg->regs, chan_id, furthest_count_value, cfg->max_bit_width);
	rtc_counter_alarm_irq_clear(cfg->regs, chan_id, cfg->max_bit_width);

	/* Update new callback functions */
	data->channel_data[chan_id].callback = alarm_cfg->callback;
	data->channel_data[chan_id].user_data = alarm_cfg->user_data;

	/* Check if "Absolute Alarm" flag is set */
	absolute = alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE;

	/* Check if counter has exceeded the alarm count in absolute alarm configuration */
	if (absolute != 0) {
		count_diff = rtc_counter_ticks_diff(count_value, ticks, top_value);
		if (count_diff <= data->guard_period) {
			ret_status = -ETIME;
			irq_on_late = alarm_cfg->flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE;
			if (irq_on_late != 0) {
				data->late_alarm_flag = true;
				data->late_alarm_channel = chan_id;

				/* Update compare value*/
				data->channel_data[chan_id].compare_value = ticks;

				/* Enable interrupt and trigger immediately */
				NVIC_SetPendingIRQ(cfg->irq_line);
			} else {
				data->channel_data[chan_id].callback = NULL;
				data->channel_data[chan_id].user_data = NULL;
			}
		} else {
			/* Enable interrupt at compare match */
			rtc_counter_alarm_irq_enable(cfg->regs, chan_id, cfg->max_bit_width);

			/* Update compare value*/
			data->channel_data[chan_id].compare_value = ticks;
			rtc_counter_set_compare(cfg->regs, chan_id, ticks, cfg->max_bit_width);
		}
	} else {
		ticks = rtc_counter_ticks_add(count_value, ticks, top_value);

		/* Enable interrupt at compare match */
		rtc_counter_alarm_irq_enable(cfg->regs, chan_id, cfg->max_bit_width);

		/* Update compare value*/
		data->channel_data[chan_id].compare_value = ticks;
		rtc_counter_set_compare(cfg->regs, chan_id, ticks, cfg->max_bit_width);
	}

	return ret_status;
}

static int32_t counter_mchp_cancel_alarm(const struct device *const dev, uint8_t chan_id)
{
	struct counter_mchp_dev_data *const data = dev->data;
	const struct counter_mchp_dev_config *const cfg = dev->config;

	/* Check for valid channel  */
	__ASSERT(chan_id < counter_get_num_of_channels(dev), "Invalid channel ID: %u (max %u)",
		 chan_id, counter_get_num_of_channels(dev));

	/* Clear, and disable interrupt flags */
	rtc_counter_alarm_irq_disable(cfg->regs, chan_id, cfg->max_bit_width);
	rtc_counter_alarm_irq_clear(cfg->regs, chan_id, cfg->max_bit_width);

	/* Set the callback function to NULL */
	data->channel_data[chan_id].callback = NULL;

	return 0;
}

static int32_t counter_mchp_set_top_value(const struct device *const dev,
					  const struct counter_top_cfg *const top_cfg)
{
	int32_t ret_status = 0;
	struct counter_mchp_dev_data *const data = dev->data;
	const struct counter_mchp_dev_config *const cfg = dev->config;

	/* Check if alarm callback function is already in progress */
	for (uint8_t i = 0; i < counter_get_num_of_channels(dev); i++) {
		if (NULL != data->channel_data[i].callback) {
			return -EBUSY;
		}
	}

	/* Check if top callback function is already in progress */
	rtc_counter_top_irq_disable(cfg->regs, cfg->max_bit_width);
	rtc_counter_top_irq_clear(cfg->regs, cfg->max_bit_width);

	/* Update callback functions */
	data->top_cb = top_cfg->callback;
	data->top_user_data = top_cfg->user_data;

	/* Update the counter period based on top configuration data */
	rtc_counter_set_period(cfg->regs, top_cfg->ticks, cfg->max_bit_width);

	if (top_cfg->flags & COUNTER_TOP_CFG_DONT_RESET) {
		/*
		 * Top trigger is on equality of the rising edge only, so
		 * manually reset it if the counter has missed the new top.
		 */
		uint32_t count_value = 0;
		(void)rtc_counter_get_count(cfg->regs, &count_value, cfg->max_bit_width);
		if (count_value >= top_cfg->ticks) {
			ret_status = -ETIME;
			if ((top_cfg->flags & COUNTER_TOP_CFG_RESET_WHEN_LATE) ==
			    COUNTER_TOP_CFG_RESET_WHEN_LATE) {
				rtc_counter_retrigger(cfg->regs, cfg->max_bit_width);
			}
		}
	} else {
		rtc_counter_retrigger(cfg->regs, cfg->max_bit_width);
	}

	/* Enable top IRQ */
	if (NULL != top_cfg->callback) {
		rtc_counter_top_irq_enable(cfg->regs, cfg->max_bit_width);
	}

	return ret_status;
}

static uint32_t counter_mchp_get_pending_int(const struct device *const dev)
{
	const struct counter_mchp_dev_config *const cfg = dev->config;

	rtc_counter_get_pending_irqs(cfg->regs, cfg->max_bit_width);

	return 0;
}

static uint32_t counter_mchp_get_top_value(const struct device *const dev)
{
	uint32_t period_value = 0u;
	const struct counter_mchp_dev_config *const cfg = dev->config;

	rtc_counter_get_period(cfg->regs, &period_value, cfg->max_bit_width);

	return period_value;
}

static uint32_t counter_mchp_get_guard_period(const struct device *dev, uint32_t flags)
{
	struct counter_mchp_dev_data *const data = dev->data;

	return data->guard_period;
}

static int32_t counter_mchp_set_guard_period(const struct device *dev, uint32_t guard,
					     uint32_t flags)
{
	uint32_t period_value = 0u;
	int32_t ret_status = -EINVAL;
	struct counter_mchp_dev_data *const data = dev->data;
	const struct counter_mchp_dev_config *const cfg = dev->config;

	/* Get period value */
	rtc_counter_get_period(cfg->regs, &period_value, cfg->max_bit_width);
	if (guard < period_value) {
		data->guard_period = guard;
		ret_status = 0;
	}

	return ret_status;
}

static uint32_t counter_mchp_get_frequency(const struct device *const dev)
{
	uint32_t source_clk_freq = 0u;
	const struct counter_mchp_dev_config *const cfg = dev->config;
	const struct mchp_counter_clock *clk =
		(const struct mchp_counter_clock *)&cfg->counter_clock;

	clock_control_get_rate(clk->clock_dev, clk->periph_async_clk, &source_clk_freq);
	/* Update the info field for clock frequency based on clock rate */
	source_clk_freq = source_clk_freq / cfg->prescaler;

	return source_clk_freq;
}

static int32_t counter_mchp_init(const struct device *const dev)
{
	int32_t ret_status = 0;
	const struct counter_mchp_dev_config *const cfg = dev->config;
	const struct mchp_counter_clock *clk =
		(const struct mchp_counter_clock *)&cfg->counter_clock;
	uint32_t max_counter_val = 0u;

	max_counter_val = (uint32_t)((1ULL << cfg->max_bit_width) - 1u);
	if (max_counter_val != cfg->info.max_top_value) {
		LOG_ERR("%s : Maximum bit width not allowed", __func__);
		return -EINVAL;
	}

	ret_status = clock_control_on(clk->clock_dev, clk->host_core_sync_clk);
	if ((ret_status < 0) && (ret_status != -EALREADY)) {
		LOG_ERR("%s : Unable to initialize host clock", __func__);
		return ret_status;
	}

	ret_status = clock_control_on(clk->clock_dev, clk->periph_async_clk);
	if ((ret_status < 0) && (ret_status != -EALREADY)) {
		LOG_ERR("%s : Unable to initialize peripheral clock", __func__);
		return ret_status;
	}

	ret_status = rtc_counter_init(cfg->regs, cfg->prescaler, cfg->max_bit_width);
	if (ret_status < 0) {
		LOG_ERR("%s : Counter failed to initialize", __func__);
		return ret_status;
	}
	cfg->irq_config_func(dev);

	return ret_status;
}

static void counter_mchp_alarm_irq_handler(const struct device *const dev)
{
	uint32_t cc_value = 0u;
	uint32_t alarm_status = 0u;
	uint32_t pending_irq_status = 0u;
	struct counter_mchp_dev_data *const data = dev->data;
	const struct counter_mchp_dev_config *const cfg = dev->config;

	NVIC_ClearPendingIRQ(cfg->irq_line);
	pending_irq_status = rtc_counter_get_pending_irqs(cfg->regs, cfg->max_bit_width);

	/* Check for immediate interrupt trigger */
	if (true == data->late_alarm_flag) {
		data->late_alarm_flag = false;
		counter_alarm_callback_t cb = NULL;

		cb = data->channel_data[data->late_alarm_channel].callback;
		if (cb != NULL) {
			data->channel_data[data->late_alarm_channel].callback = NULL;
			cc_value = data->channel_data[data->late_alarm_channel].compare_value;

			/* Execute callback function  */
			cb(dev, data->late_alarm_channel, cc_value,
			   data->channel_data[data->late_alarm_channel].user_data);
		}
		return;
	}

	for (uint8_t chan_id = 0; chan_id < counter_get_num_of_channels(dev); chan_id++) {
		alarm_status = rtc_counter_alarm_irq_status(pending_irq_status, chan_id,
							    cfg->max_bit_width);
		if (false != alarm_status) {
			/* Clear NVIC Flag to avoid retrigger */
			rtc_counter_alarm_irq_clear(cfg->regs, chan_id, cfg->max_bit_width);

			if (NULL != data->channel_data[chan_id].callback) {
				counter_alarm_callback_t cb = data->channel_data[chan_id].callback;

				data->channel_data[chan_id].callback = NULL;

				/* Execute callback function  */
				cc_value = data->channel_data[chan_id].compare_value;
				cb(dev, chan_id, cc_value, data->channel_data[chan_id].user_data);
			}
		}
	}
}

static void counter_mchp_top_irq_handler(const struct device *const dev)
{
	struct counter_mchp_dev_data *const data = dev->data;
	const struct counter_mchp_dev_config *const cfg = dev->config;
	uint32_t pending_irq_status = rtc_counter_get_pending_irqs(cfg->regs, cfg->max_bit_width);

	NVIC_ClearPendingIRQ(cfg->irq_line);
	if (false != rtc_counter_top_irq_status(pending_irq_status, cfg->max_bit_width)) {
		rtc_counter_top_irq_clear(cfg->regs, cfg->max_bit_width);

		if (NULL != data->top_cb) {
			data->top_cb(dev, data->top_user_data);
		}
	}
}

static void counter_mchp_interrupt_handler(const struct device *const dev)
{
	counter_mchp_alarm_irq_handler(dev);
	counter_mchp_top_irq_handler(dev);
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

#define COUNTER_MCHP_CC_NUMS(n)       ((DT_INST_PROP(n, max_bit_width) == 32) ? 1 : 4)
#define COUNTER_MCHP_MAX_BIT_WIDTH(n) (DT_INST_PROP(n, max_bit_width))
#define COUNTER_MCHP_PRESCALER(n)     DT_INST_PROP_OR(n, prescaler, 1)

/* clang-format off */
#define COUNTER_MCHP_CLOCK_ASSIGN(n)							\
	.counter_clock.clock_dev = DEVICE_DT_GET(DT_NODELABEL(clock)),			\
	.counter_clock.host_core_sync_clk =						\
		(void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, subsystem)),		\
	COND_CODE_1(									\
		DT_NODE_EXISTS(DT_INST_CLOCKS_CTLR_BY_NAME(n, rtcclk)),			\
		(.counter_clock.periph_async_clk =					\
			(void *)DT_INST_CLOCKS_CELL_BY_NAME(n, rtcclk, subsystem),),	\
		()									\
	)

#define COUNTER_MCHP_CONFIG_VAR(n)								\
	static const  struct counter_mchp_dev_config  counter_mchp_dev_config_##n = {		\
		.info = {									\
			.max_top_value =							\
				((uint32_t)((1ULL << COUNTER_MCHP_MAX_BIT_WIDTH(n)) -  1)),	\
			.freq = 0u,								\
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,					\
			.channels = COUNTER_MCHP_CC_NUMS(n) },					\
		.regs = (void *)DT_INST_REG_ADDR(n),						\
		COUNTER_MCHP_CLOCK_ASSIGN(n)							\
		.irq_line =	DT_INST_IRQ_BY_IDX(n, 0, irq),					\
		.max_bit_width = DT_INST_PROP(n, max_bit_width),				\
		.prescaler = COUNTER_MCHP_PRESCALER(n),						\
		.irq_config_func = &counter_mchp_config_##n,					\
	};

#define COUNTER_MCHP_IRQ_HANDLER(n)					\
	static void counter_mchp_config_##n(const struct device *dev)	\
	{								\
		MCHP_COUNTER_IRQ_CONNECT(n, 0);				\
	}

#define COUNTER_MCHP_CHANNEL_DATA_VAR(n)							\
	static struct counter_mchp_ch_data counter_mchp_channel_data_##n[COUNTER_MCHP_CC_NUMS(n)];

#define COUNTER_MCHP_TOP_DATA_VAR(n)						\
	static struct counter_mchp_dev_data counter_mchp_dev_data_##n = {	\
		.channel_data = counter_mchp_channel_data_##n};

#define MCHP_COUNTER_IRQ_CONNECT(n, m)								\
	COND_CODE_1(DT_IRQ_HAS_IDX(DT_DRV_INST(n), m),						\
	(											\
		do {										\
			IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, m, irq),				\
				DT_INST_IRQ_BY_IDX(n, m, priority),				\
				counter_mchp_interrupt_handler, DEVICE_DT_INST_GET(n), 0);	\
			irq_enable(DT_INST_IRQ_BY_IDX(n, m, irq));				\
		} while (false);								\
	),											\
	()											\
	)

#define COUNTER_MCHP_DEVICE_INIT_FUNC(n)	\
	COUNTER_MCHP_IRQ_HANDLER(n)		\
	COUNTER_MCHP_CONFIG_VAR(n);		\
	COUNTER_MCHP_CHANNEL_DATA_VAR(n)	\
	COUNTER_MCHP_TOP_DATA_VAR(n)

#define COUNTER_MCHP_DEVICE_INIT(n)                                                                \
	COUNTER_MCHP_DEVICE_INIT_FUNC(n)                                                           \
	DEVICE_DT_INST_DEFINE(n, counter_mchp_init, NULL, &counter_mchp_dev_data_##n,              \
			      &counter_mchp_dev_config_##n, POST_KERNEL,                           \
			      CONFIG_COUNTER_INIT_PRIORITY, &counter_mchp_api);
/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(COUNTER_MCHP_DEVICE_INIT)

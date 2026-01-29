/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/clock_control/mchp_clock_control.h>

#define DT_DRV_COMPAT microchip_tc_g1_counter

LOG_MODULE_REGISTER(counter_mchp_tc_g1, CONFIG_COUNTER_LOG_LEVEL);
#define ALL_TC_SYNC_BITS                 ((uint32_t)UINT32_MAX)
#define TC_SYNCHRONIZATION_TIMEOUT_IN_US (5U)
#define DELAY_US                         (2U)

struct mchp_counter_clock {
	const struct device *clock_dev;
	clock_control_subsys_t host_mclk;
	clock_control_subsys_t client_mclk;
	clock_control_subsys_t host_gclk;
};

struct counter_mchp_ch_data {
	counter_alarm_callback_t callback;
	uint32_t compare_value;
	void *user_data;
};

struct counter_mchp_dev_data {
	counter_top_callback_t top_cb;
	void *top_user_data;
	bool late_alarm_flag;
	uint8_t late_alarm_channel;
	uint32_t guard_period;
	struct counter_mchp_ch_data *channel_data;
};

struct counter_mchp_dev_config {
	struct counter_config_info info;
	void *regs;
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
} tc_counter_modes_t;

typedef enum {
	/* Reload or reset the counter on the next generic clock */
	TC_GCLK_RESET_ON_GENERIC_CLOCK = 0x0,
	/* Reload or reset the counter on the next prescaler clock */
	TC_PRESC_RESET_ON_PRESCALER_CLOCK = 0x1,
	/* Reload or reset the counter on the next generic clock,
	 * and reset the prescaler counter
	 */
	TC_RESYNC_RESET_ON_GENERIC_CLOCK = 0x2
} tc_counter_tc_prescaler_sync_mode_t;

typedef enum {
	TC_EVENT_CONTROL_MODE_OFF = 0x0,   /* Event action disabled. */
	TC_EVENT_CONTROL_MODE_COUNT = 0x2, /* Count on event. */
} tc_counter_tc_evt_control_mode_t;

/* find the prescale index based on the value present in the device tree */
static uint8_t get_tc_prescale_index(uint16_t prescaler)
{
	uint8_t prescale_index = TC_CTRLA_PRESCALER_DIV1_Val;

	switch (prescaler) {
	case 64:
		prescale_index = TC_CTRLA_PRESCALER_DIV64_Val;
		break;
	case 256:
		prescale_index = TC_CTRLA_PRESCALER_DIV256_Val;
		break;
	case 1024:
		prescale_index = TC_CTRLA_PRESCALER_DIV1024_Val;
		break;
	default:
		if ((prescaler >= 1) && (prescaler <= 16)) {
			prescale_index = __builtin_ctz(prescaler);
		}
		break;
	}
	return prescale_index;
}

static int tc_counter_wait_sync(const volatile uint32_t *sync_reg_addr, uint32_t bit_mask)
{
	int success = WAIT_FOR((((*sync_reg_addr) & bit_mask) == 0u),
			       TC_SYNCHRONIZATION_TIMEOUT_IN_US, k_busy_wait(DELAY_US));

	if (success == 0) {
		LOG_ERR("%s : Synchronization time-out occurred", __func__);
		return -ETIMEDOUT;
	}
	return 0;
}

static int tc_counter_ctrlb_wait_sync(const volatile uint8_t *sync_reg_addr, uint32_t bit_mask)
{
	int success = WAIT_FOR((((*sync_reg_addr) & bit_mask) == 0u),
			       TC_SYNCHRONIZATION_TIMEOUT_IN_US, k_busy_wait(DELAY_US));

	if (success == 0) {
		LOG_ERR("%s : Synchronization time-out occurred", __func__);
		return -ETIMEDOUT;
	}
	return 0;
}

static int tc_counter_init(const void *tc_regs, uint32_t prescaler, const uint32_t max_bit_width)
{
	int ret_val;

	uint32_t ctrla_reg_value = TC_CTRLA_CAPTEN0(0u) | TC_CTRLA_CAPTEN1(0u) |
				   TC_CTRLA_COPEN0(0u) | TC_CTRLA_COPEN1(0u) |
				   TC_CTRLA_PRESCALER(get_tc_prescale_index(prescaler)) |
				   TC_CTRLA_PRESCSYNC(TC_GCLK_RESET_ON_GENERIC_CLOCK) |
				   TC_CTRLA_ONDEMAND(0u) | TC_CTRLA_RUNSTDBY(0u);

	uint32_t evctrl_reg_value = TC_EVCTRL_EVACT(TC_EVENT_CONTROL_MODE_OFF) |
				    (uint16_t)TC_EVCTRL_TCINV(0u) | (uint16_t)TC_EVCTRL_TCEI(0u) |
				    (uint16_t)TC_EVCTRL_OVFEO(0u) | (uint16_t)TC_EVCTRL_MCEO0(0u) |
				    (uint16_t)TC_EVCTRL_MCEO1(0u);

	switch (max_bit_width) {
	case COUNTER_BIT_MODE_32: {
		tc_count32_registers_t *const p_regs = &(((tc_registers_t *)tc_regs)->COUNT32);

		if (TC_STATUS_Msk != (p_regs->TC_STATUS & TC_STATUS_Msk)) {
			p_regs->TC_CTRLA &= ~TC_CTRLA_ENABLE_Msk;
			ret_val =
				tc_counter_wait_sync(&p_regs->TC_SYNCBUSY, TC_SYNCBUSY_ENABLE_Msk);
			if (ret_val < 0) {
				LOG_ERR("ret_val = %d", ret_val);
				break;
			}
			p_regs->TC_CTRLA |= TC_CTRLA_SWRST_Msk;
			ret_val = tc_counter_wait_sync(&p_regs->TC_SYNCBUSY, TC_SYNCBUSY_SWRST_Msk);
			if (ret_val < 0) {
				LOG_ERR("ret_val = %d", ret_val);
				break;
			}
			p_regs->TC_CTRLA = ctrla_reg_value | TC_CTRLA_MODE(2u);
			p_regs->TC_WAVE = TC_WAVE_WAVEGEN_MFRQ;
			p_regs->TC_CTRLBSET = TC_CTRLBCLR_ONESHOT(0u) | TC_CTRLBCLR_DIR(0u);
			p_regs->TC_DRVCTRL = TC_DRVCTRL_INVEN(0u);
			p_regs->TC_CC[0u] = UINT32_MAX;
			p_regs->TC_CC[1u] = UINT32_MAX;
			p_regs->TC_INTFLAG = TC_INTFLAG_Msk;
			p_regs->TC_EVCTRL = evctrl_reg_value;
			ret_val = tc_counter_wait_sync(&p_regs->TC_SYNCBUSY, ALL_TC_SYNC_BITS);
			if (ret_val < 0) {
				LOG_ERR("ret_val = %d", ret_val);
				break;
			}
			ret_val = 0;
		} else {
			ret_val = -EBUSY;
		}
		break;
	}
	case COUNTER_BIT_MODE_16: {
		tc_count16_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT16);

		if (TC_STATUS_Msk != (p_regs->TC_STATUS & TC_STATUS_Msk)) {
			p_regs->TC_CTRLA &= ~TC_CTRLA_ENABLE_Msk;
			ret_val =
				tc_counter_wait_sync(&p_regs->TC_SYNCBUSY, TC_SYNCBUSY_ENABLE_Msk);
			if (ret_val < 0) {
				LOG_ERR("ret_val = %d", ret_val);
				break;
			}
			p_regs->TC_CTRLA |= TC_CTRLA_SWRST_Msk;
			ret_val = tc_counter_wait_sync(&p_regs->TC_SYNCBUSY, TC_SYNCBUSY_SWRST_Msk);
			if (ret_val < 0) {
				LOG_ERR("ret_val = %d", ret_val);
				break;
			}
			p_regs->TC_CTRLA = ctrla_reg_value;
			p_regs->TC_WAVE = TC_WAVE_WAVEGEN_MFRQ;
			p_regs->TC_CTRLBSET = TC_CTRLBCLR_ONESHOT(0u) | TC_CTRLBCLR_DIR(0u);
			p_regs->TC_DRVCTRL = TC_DRVCTRL_INVEN(0u);
			p_regs->TC_CC[0u] = UINT16_MAX;
			p_regs->TC_CC[1u] = UINT16_MAX;
			p_regs->TC_INTFLAG = TC_INTFLAG_Msk;
			p_regs->TC_EVCTRL = evctrl_reg_value;
			ret_val = tc_counter_wait_sync(&p_regs->TC_SYNCBUSY, ALL_TC_SYNC_BITS);
			if (ret_val < 0) {
				LOG_ERR("ret_val = %d", ret_val);
				break;
			}
			ret_val = 0;
		} else {
			ret_val = -EBUSY;
		}
		break;
	}
	case COUNTER_BIT_MODE_8: {
		tc_count8_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT8);

		if (TC_STATUS_Msk != (p_regs->TC_STATUS & TC_STATUS_Msk)) {
			p_regs->TC_CTRLA &= ~TC_CTRLA_ENABLE_Msk;
			ret_val =
				tc_counter_wait_sync(&p_regs->TC_SYNCBUSY, TC_SYNCBUSY_ENABLE_Msk);
			if (ret_val < 0) {
				LOG_ERR("ret_val = %d", ret_val);
				break;
			}
			p_regs->TC_CTRLA |= TC_CTRLA_SWRST_Msk;
			ret_val = tc_counter_wait_sync(&p_regs->TC_SYNCBUSY, TC_SYNCBUSY_SWRST_Msk);
			if (ret_val < 0) {
				LOG_ERR("ret_val = %d", ret_val);
				break;
			}
			p_regs->TC_CTRLA = ctrla_reg_value | TC_CTRLA_MODE(1u);
			p_regs->TC_WAVE = TC_WAVE_WAVEGEN_NFRQ;
			p_regs->TC_CTRLBSET = TC_CTRLBCLR_ONESHOT(0u) | TC_CTRLBCLR_DIR(0u);
			p_regs->TC_DRVCTRL = TC_DRVCTRL_INVEN(0u);
			p_regs->TC_CC[0u] = UINT8_MAX;
			p_regs->TC_CC[1u] = UINT8_MAX;
			p_regs->TC_PER = UINT8_MAX;
			p_regs->TC_INTFLAG = TC_INTFLAG_Msk;
			p_regs->TC_EVCTRL = evctrl_reg_value;
			ret_val = tc_counter_wait_sync(&p_regs->TC_SYNCBUSY, ALL_TC_SYNC_BITS);
			if (ret_val < 0) {
				LOG_ERR("ret_val = %d", ret_val);
				break;
			}
			ret_val = 0;
		} else {
			ret_val = -EBUSY;
		}
		break;
	}
	default:
		LOG_ERR("%s : Unsupported Counter mode %d", __func__, max_bit_width);
		ret_val = -ENOTSUP;
		break;
	}

	return ret_val;
}

static int32_t tc_counter_start(const void *tc_regs, const uint32_t max_bit_width)
{
	int32_t ret_val;

	switch (max_bit_width) {
	case COUNTER_BIT_MODE_32: {
		tc_count32_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT32);

		p_regs->TC_CTRLA |= TC_CTRLA_ENABLE_Msk;
		ret_val = tc_counter_wait_sync(&p_regs->TC_SYNCBUSY, TC_SYNCBUSY_ENABLE_Msk);
		if (ret_val < 0) {
			LOG_ERR("%s : ret_val = %d", __func__, ret_val);
			break;
		}
		ret_val = tc_counter_wait_sync(&p_regs->TC_SYNCBUSY, TC_SYNCBUSY_CTRLB_Msk);
		if (ret_val < 0) {
			LOG_ERR("%s : ret_val = %d", __func__, ret_val);
			break;
		}

		p_regs->TC_CTRLBSET |= TC_CTRLBSET_CMD_RETRIGGER;
		ret_val = tc_counter_wait_sync(&p_regs->TC_SYNCBUSY, TC_SYNCBUSY_CTRLB_Msk);
		if (ret_val < 0) {
			LOG_ERR("%s : ret_val = %d", __func__, ret_val);
			break;
		}
		ret_val = tc_counter_ctrlb_wait_sync(&p_regs->TC_CTRLBSET, TC_CTRLBSET_CMD_Msk);
		if (ret_val < 0) {
			LOG_ERR("%s : ret_val = %d", __func__, ret_val);
			break;
		}
		ret_val = 0;
		break;
	}
	case COUNTER_BIT_MODE_16: {
		tc_count16_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT16);

		p_regs->TC_CTRLA |= TC_CTRLA_ENABLE_Msk;
		ret_val = tc_counter_wait_sync(&p_regs->TC_SYNCBUSY, TC_SYNCBUSY_ENABLE_Msk);
		if (ret_val < 0) {
			LOG_ERR("%s : ret_val = %d", __func__, ret_val);
			break;
		}
		p_regs->TC_CTRLBSET |= TC_CTRLBSET_CMD_RETRIGGER;
		ret_val = tc_counter_wait_sync(&p_regs->TC_SYNCBUSY, TC_SYNCBUSY_CTRLB_Msk);
		if (ret_val < 0) {
			LOG_ERR("%s : ret_val = %d", __func__, ret_val);
			break;
		}
		ret_val = tc_counter_ctrlb_wait_sync(&p_regs->TC_CTRLBSET, TC_CTRLBSET_CMD_Msk);
		if (ret_val < 0) {
			LOG_ERR("%s : ret_val = %d", __func__, ret_val);
			break;
		}
		ret_val = 0;
		break;
	}
	case COUNTER_BIT_MODE_8: {
		tc_count8_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT8);

		p_regs->TC_CTRLA |= TC_CTRLA_ENABLE_Msk;
		ret_val = tc_counter_wait_sync(&p_regs->TC_SYNCBUSY, TC_SYNCBUSY_ENABLE_Msk);

		p_regs->TC_CTRLBSET |= TC_CTRLBSET_CMD_RETRIGGER;
		ret_val = tc_counter_wait_sync(&p_regs->TC_SYNCBUSY, TC_SYNCBUSY_CTRLB_Msk);
		if (ret_val < 0) {
			LOG_ERR("%s : ret_val = %d", __func__, ret_val);
			break;
		}
		ret_val = tc_counter_ctrlb_wait_sync(&p_regs->TC_CTRLBSET, TC_CTRLBSET_CMD_Msk);
		if (ret_val < 0) {
			LOG_ERR("%s : ret_val = %d", __func__, ret_val);
			break;
		}
		ret_val = 0;
		break;
	}
	default:
		LOG_ERR("%s : Unsupported Counter mode %d", __func__, max_bit_width);
		ret_val = -ENOTSUP;
		break;
	}

	return ret_val;
}

static int32_t tc_counter_stop(const void *tc_regs, const uint32_t max_bit_width)
{
	int32_t ret_val;

	switch (max_bit_width) {
	case COUNTER_BIT_MODE_32: {
		tc_count32_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT32);

		p_regs->TC_CTRLBSET |= TC_CTRLBSET_CMD_STOP;
		ret_val = tc_counter_wait_sync(&p_regs->TC_SYNCBUSY, TC_SYNCBUSY_CTRLB_Msk);
		if (ret_val < 0) {
			LOG_ERR("%s : ret_val = %d", __func__, ret_val);
			break;
		}
		ret_val = tc_counter_ctrlb_wait_sync(&p_regs->TC_CTRLBSET, TC_CTRLBSET_CMD_Msk);
		if (ret_val < 0) {
			LOG_ERR("%s : ret_val = %d", __func__, ret_val);
			break;
		}
		ret_val = 0;
		break;
	}
	case COUNTER_BIT_MODE_16: {
		tc_count16_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT16);

		p_regs->TC_CTRLBSET |= TC_CTRLBSET_CMD_STOP;
		ret_val = tc_counter_wait_sync(&p_regs->TC_SYNCBUSY, TC_SYNCBUSY_CTRLB_Msk);
		if (ret_val < 0) {
			LOG_ERR("%s : ret_val = %d", __func__, ret_val);
			break;
		}
		ret_val = tc_counter_ctrlb_wait_sync(&p_regs->TC_CTRLBSET, TC_CTRLBSET_CMD_Msk);
		if (ret_val < 0) {
			LOG_ERR("%s : ret_val = %d", __func__, ret_val);
			break;
		}
		ret_val = 0;
		break;
	}
	case COUNTER_BIT_MODE_8: {
		tc_count8_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT8);

		p_regs->TC_CTRLBSET |= TC_CTRLBSET_CMD_STOP;
		ret_val = tc_counter_wait_sync(&p_regs->TC_SYNCBUSY, TC_SYNCBUSY_CTRLB_Msk);
		if (ret_val < 0) {
			LOG_ERR("%s : ret_val = %d", __func__, ret_val);
			break;
		}
		ret_val = tc_counter_ctrlb_wait_sync(&p_regs->TC_CTRLBSET, TC_CTRLBSET_CMD_Msk);
		if (ret_val < 0) {
			LOG_ERR("%s : ret_val = %d", __func__, ret_val);
			break;
		}
		ret_val = 0;
		break;
	}
	default:
		LOG_ERR("%s : Unsupported Counter mode %d", __func__, max_bit_width);
		ret_val = -ENOTSUP;
		break;
	}

	return ret_val;
}

static int32_t tc_counter_retrigger(const void *tc_regs, const uint32_t max_bit_width)
{
	int32_t ret_val;

	switch (max_bit_width) {
	case COUNTER_BIT_MODE_32: {
		tc_count32_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT32);

		p_regs->TC_CTRLBSET |= TC_CTRLBSET_CMD_RETRIGGER;
		ret_val = tc_counter_wait_sync(&p_regs->TC_SYNCBUSY, TC_SYNCBUSY_CTRLB_Msk);
		if (ret_val < 0) {
			LOG_ERR("%s : ret_val = %d", __func__, ret_val);
			break;
		}
		ret_val = tc_counter_ctrlb_wait_sync(&p_regs->TC_CTRLBSET, TC_CTRLBSET_CMD_Msk);
		if (ret_val < 0) {
			LOG_ERR("%s : ret_val = %d", __func__, ret_val);
			break;
		}
		ret_val = 0;
		break;
	}
	case COUNTER_BIT_MODE_16: {
		tc_count16_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT16);

		p_regs->TC_CTRLBSET |= TC_CTRLBSET_CMD_RETRIGGER;
		ret_val = tc_counter_wait_sync(&p_regs->TC_SYNCBUSY, TC_SYNCBUSY_CTRLB_Msk);
		if (ret_val < 0) {
			LOG_ERR("%s : ret_val = %d", __func__, ret_val);
			break;
		}
		ret_val = tc_counter_ctrlb_wait_sync(&p_regs->TC_CTRLBSET, TC_CTRLBSET_CMD_Msk);
		if (ret_val < 0) {
			LOG_ERR("%s : ret_val = %d", __func__, ret_val);
			break;
		}
		break;
	}
	case COUNTER_BIT_MODE_8: {
		tc_count8_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT8);

		p_regs->TC_CTRLBSET |= TC_CTRLBSET_CMD_RETRIGGER;
		ret_val = tc_counter_wait_sync(&p_regs->TC_SYNCBUSY, TC_SYNCBUSY_CTRLB_Msk);
		if (ret_val < 0) {
			LOG_ERR("%s : ret_val = %d", __func__, ret_val);
			break;
		}
		ret_val = tc_counter_ctrlb_wait_sync(&p_regs->TC_CTRLBSET, TC_CTRLBSET_CMD_Msk);
		if (ret_val < 0) {
			LOG_ERR("%s : ret_val = %d", __func__, ret_val);
			break;
		}
		ret_val = 0;
		break;
	}
	default:
		LOG_ERR("%s : Unsupported Counter mode %d", __func__, max_bit_width);
		ret_val = -ENOTSUP;
		break;
	}

	return ret_val;
}

static inline int32_t tc_counter_get_count(const void *tc_regs, uint32_t *const counter_value,
					   const uint32_t max_bit_width)
{
	int32_t ret_val = 0;

	switch (max_bit_width) {
	case COUNTER_BIT_MODE_32: {
		tc_count32_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT32);

		p_regs->TC_CTRLBSET |= TC_CTRLBSET_CMD_READSYNC;
		ret_val = tc_counter_wait_sync(&p_regs->TC_SYNCBUSY, TC_SYNCBUSY_CTRLB_Msk);
		if (ret_val < 0) {
			LOG_ERR("%s : ret_val = %d", __func__, ret_val);
			break;
		}
		ret_val = tc_counter_ctrlb_wait_sync(&p_regs->TC_CTRLBSET, TC_CTRLBSET_CMD_Msk);
		if (ret_val < 0) {
			LOG_ERR("%s : ret_val = %d", __func__, ret_val);
			break;
		}
		*counter_value = p_regs->TC_COUNT;
		ret_val = 0;
		break;
	}
	case COUNTER_BIT_MODE_16: {
		tc_count16_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT16);

		p_regs->TC_CTRLBSET |= TC_CTRLBSET_CMD_READSYNC;
		ret_val = tc_counter_wait_sync(&p_regs->TC_SYNCBUSY, TC_SYNCBUSY_CTRLB_Msk);
		if (ret_val < 0) {
			LOG_ERR("%s : ret_val = %d", __func__, ret_val);
			break;
		}
		ret_val = tc_counter_ctrlb_wait_sync(&p_regs->TC_CTRLBSET, TC_CTRLBSET_CMD_Msk);
		if (ret_val < 0) {
			LOG_ERR("%s : ret_val = %d", __func__, ret_val);
			break;
		}
		*counter_value = p_regs->TC_COUNT;
		ret_val = 0;
		break;
	}
	case COUNTER_BIT_MODE_8: {
		tc_count8_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT8);

		p_regs->TC_CTRLBSET |= TC_CTRLBSET_CMD_READSYNC;
		ret_val = tc_counter_wait_sync(&p_regs->TC_SYNCBUSY, TC_SYNCBUSY_CTRLB_Msk);
		if (ret_val < 0) {
			LOG_ERR("%s : ret_val = %d", __func__, ret_val);
			break;
		}
		ret_val = tc_counter_ctrlb_wait_sync(&p_regs->TC_CTRLBSET, TC_CTRLBSET_CMD_Msk);
		if (ret_val < 0) {
			LOG_ERR("%s : ret_val = %d", __func__, ret_val);
			break;
		}
		*counter_value = p_regs->TC_COUNT;
		ret_val = 0;
		break;
	}
	default:
		LOG_ERR("%s : Unsupported Counter mode %d", __func__, max_bit_width);
		ret_val = -ENOTSUP;
		break;
	}
	if (ret_val < 0) {
		LOG_ERR("%s : ret_val = %d", __func__, ret_val);
	}
	return ret_val;
}

static int32_t tc_counter_set_period(const void *tc_regs, const uint32_t period,
				     const uint32_t max_bit_width)
{
	int32_t ret_val = 0;

	switch (max_bit_width) {
	case COUNTER_BIT_MODE_32: {
		tc_count32_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT32);

		p_regs->TC_CC[0u] = period;
		ret_val = tc_counter_wait_sync(&p_regs->TC_SYNCBUSY, TC_SYNCBUSY_CC0_Msk);
		break;
	}
	case COUNTER_BIT_MODE_16: {
		tc_count16_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT16);

		p_regs->TC_CC[0u] = period;
		ret_val = tc_counter_wait_sync(&p_regs->TC_SYNCBUSY, TC_SYNCBUSY_CC0_Msk);
		break;
	}
	case COUNTER_BIT_MODE_8: {
		tc_count8_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT8);

		p_regs->TC_PER = period;
		ret_val = tc_counter_wait_sync(&p_regs->TC_SYNCBUSY, TC_SYNCBUSY_CC0_Msk);
		break;
	}
	default:
		LOG_ERR("%s : Unsupported Counter mode %d", __func__, max_bit_width);
		ret_val = -ENOTSUP;
		break;
	}
	if (ret_val < 0) {
		LOG_ERR("%s : ret_val = %d", __func__, ret_val);
	}

	return ret_val;
}

static int32_t tc_counter_get_period(const void *tc_regs, uint32_t *const period,
				     const uint32_t max_bit_width)
{
	uint32_t ret_val = 0;

	switch (max_bit_width) {
	case COUNTER_BIT_MODE_32: {
		tc_count32_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT32);

		*period = p_regs->TC_CC[0u];
		break;
	}
	case COUNTER_BIT_MODE_16: {
		tc_count16_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT16);

		*period = p_regs->TC_CC[0u];
		break;
	}
	case COUNTER_BIT_MODE_8: {
		tc_count8_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT8);

		*period = p_regs->TC_PER;
		break;
	}
	default:
		LOG_ERR("%s : Unsupported Counter mode %d", __func__, max_bit_width);
		ret_val = -ENOTSUP;
		break;
	}

	return ret_val;
}

static int32_t tc_counter_set_compare(const void *tc_regs, const uint32_t chan_id,
				      const uint32_t compare_value, const uint32_t max_bit_width)
{
	int32_t ret_val = 0;

	switch (max_bit_width) {
	case COUNTER_BIT_MODE_32: {
		(void)chan_id;
		tc_count32_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT32);

		p_regs->TC_CC[1u] = compare_value;
		ret_val = tc_counter_wait_sync(&p_regs->TC_SYNCBUSY, TC_SYNCBUSY_CC1_Msk);
		break;
	}
	case COUNTER_BIT_MODE_16: {
		(void)chan_id;
		tc_count16_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT16);

		p_regs->TC_CC[1u] = compare_value;
		ret_val = tc_counter_wait_sync(&p_regs->TC_SYNCBUSY, TC_SYNCBUSY_CC1_Msk);
		break;
	}
	case COUNTER_BIT_MODE_8: {
		tc_count8_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT8);

		p_regs->TC_CC[chan_id] = compare_value;
		if (chan_id == 0u) {
			ret_val = tc_counter_wait_sync(&p_regs->TC_SYNCBUSY, TC_SYNCBUSY_CC0_Msk);
		} else {
			ret_val = tc_counter_wait_sync(&p_regs->TC_SYNCBUSY, TC_SYNCBUSY_CC1_Msk);
		}
		break;
	}
	default:
		LOG_ERR("%s : Unsupported Counter mode %d", __func__, max_bit_width);
		ret_val = -EBUSY;
		break;
	}
	if (ret_val < 0) {
		LOG_ERR("%s : ret_val = %d", __func__, ret_val);
	}
	return ret_val;
}

static int tc_counter_get_pending_irqs(const void *tc_regs, const uint32_t max_bit_width)
{
	int ret_status = 0;

	switch (max_bit_width) {
	case COUNTER_BIT_MODE_32: {
		tc_count32_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT32);

		ret_status = p_regs->TC_INTFLAG;
	} break;
	case COUNTER_BIT_MODE_16: {
		tc_count16_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT16);

		ret_status = p_regs->TC_INTFLAG;
	} break;
	case COUNTER_BIT_MODE_8: {
		tc_count8_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT8);

		ret_status = p_regs->TC_INTFLAG;
	} break;
	default:
		LOG_ERR("%s : Unsupported Counter mode %d", __func__, max_bit_width);
		break;
	}

	return ret_status;
}

static int32_t tc_counter_alarm_irq_enable(const void *tc_regs, const uint32_t channel_id,
					   const uint32_t max_bit_width)
{
	int32_t ret_val = 0;

	switch (max_bit_width) {
	case COUNTER_BIT_MODE_32: {
		tc_count32_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT32);

		if (channel_id == 0u) {
			p_regs->TC_INTENSET = TC_INTFLAG_MC1_Msk;
		} else {
			ret_val = -ENOTSUP;
		}
		break;
	}
	case COUNTER_BIT_MODE_16: {
		tc_count16_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT16);

		if (channel_id == 0u) {
			p_regs->TC_INTENSET = TC_INTFLAG_MC1_Msk;
		} else {
			ret_val = -ENOTSUP;
		}
		break;
	}
	case COUNTER_BIT_MODE_8: {
		tc_count8_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT8);

		if (channel_id == 0u) {
			p_regs->TC_INTENSET = TC_INTFLAG_MC0_Msk;
		} else if (channel_id == 1u) {
			p_regs->TC_INTENSET = TC_INTFLAG_MC1_Msk;
		} else {
			ret_val = -ENOTSUP;
		}
		break;
	}
	default:
		LOG_ERR("%s : Unsupported Counter mode %d", __func__, max_bit_width);
		ret_val = -ENOTSUP;
		break;
	}

	return ret_val;
}

static int32_t tc_counter_alarm_irq_disable(const void *tc_regs, const uint32_t channel_id,
					    const uint32_t max_bit_width)
{
	int32_t ret_val = 0;

	switch (max_bit_width) {
	case COUNTER_BIT_MODE_32: {
		tc_count32_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT32);

		if (channel_id == 0u) {
			p_regs->TC_INTENCLR = TC_INTFLAG_MC1_Msk;
		} else {
			ret_val = -ENOTSUP;
		}
		break;
	}
	case COUNTER_BIT_MODE_16: {
		tc_count16_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT16);

		if (channel_id == 0u) {
			p_regs->TC_INTENCLR = TC_INTFLAG_MC1_Msk;
		} else {
			ret_val = -ENOTSUP;
		}
		break;
	}
	case COUNTER_BIT_MODE_8: {
		tc_count8_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT8);

		if (channel_id == 0u) {
			p_regs->TC_INTENCLR = TC_INTFLAG_MC0_Msk;
		} else if (channel_id == 1u) {
			p_regs->TC_INTENCLR = TC_INTFLAG_MC1_Msk;
		} else {
			ret_val = -ENOTSUP;
		}
		break;
	}
	default:
		LOG_ERR("%s : Unsupported Counter mode %d", __func__, max_bit_width);
		ret_val = -ENOTSUP;
		break;
	}

	return ret_val;
}

static int32_t tc_counter_alarm_irq_clear(const void *tc_regs, const uint32_t channel_id,
					  const uint32_t max_bit_width)
{
	int32_t ret_val = 0;

	switch (max_bit_width) {
	case COUNTER_BIT_MODE_32: {
		tc_count32_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT32);

		if (channel_id == 0u) {
			p_regs->TC_INTFLAG = TC_INTFLAG_MC1_Msk;
		} else {
			ret_val = -ENOTSUP;
		}
		break;
	}
	case COUNTER_BIT_MODE_16: {
		tc_count16_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT16);

		if (channel_id == 0u) {
			p_regs->TC_INTFLAG = TC_INTFLAG_MC1_Msk;
		} else {
			ret_val = -ENOTSUP;
		}
		break;
	}
	case COUNTER_BIT_MODE_8: {
		tc_count8_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT8);

		if (channel_id == 0u) {
			p_regs->TC_INTFLAG = TC_INTFLAG_MC0_Msk;
		} else if (channel_id == 1u) {
			p_regs->TC_INTFLAG = TC_INTFLAG_MC1_Msk;
		} else {
			ret_val = -ENOTSUP;
		}
		break;
	}
	default:
		LOG_ERR("%s : Unsupported Counter mode %d", __func__, max_bit_width);
		ret_val = -ENOTSUP;
		break;
	}

	return ret_val;
}

static bool tc_counter_alarm_irq_status(const uint32_t pending_irq_status,
					const uint32_t channel_id, const uint32_t max_bit_width)
{
	bool ret_status = false;

	switch (max_bit_width) {
	case COUNTER_BIT_MODE_32: {
		if (channel_id == 0u) {
			ret_status = (bool)((pending_irq_status & (uint8_t)TC_INTFLAG_MC1_Msk) ==
					    TC_INTFLAG_MC1_Msk);
		} else {
			ret_status = false;
		}
		break;
	}
	case COUNTER_BIT_MODE_16: {
		if (channel_id == 0u) {
			ret_status = (bool)((pending_irq_status & (uint8_t)TC_INTFLAG_MC1_Msk) ==
					    TC_INTFLAG_MC1_Msk);
		} else {
			ret_status = false;
		}
		break;
	}
	case COUNTER_BIT_MODE_8: {
		if (channel_id == 0u) {
			ret_status = (bool)((pending_irq_status & (uint8_t)TC_INTFLAG_MC0_Msk) ==
					    TC_INTFLAG_MC0_Msk);
		} else if (channel_id == 1u) {
			ret_status = (bool)((pending_irq_status & (uint8_t)TC_INTFLAG_MC1_Msk) ==
					    TC_INTFLAG_MC1_Msk);
		} else {
			ret_status = false;
		}
		break;
	}
	default:
		LOG_ERR("%s : Unsupported Counter mode %d", __func__, max_bit_width);
		break;
	}

	return ret_status;
}

static int32_t tc_counter_top_irq_enable(const void *tc_regs, const uint32_t max_bit_width)
{
	int32_t ret_val = 0;

	switch (max_bit_width) {
	case COUNTER_BIT_MODE_32: {
		tc_count32_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT32);

		p_regs->TC_INTENSET = TC_INTFLAG_MC0_Msk;
		break;
	}
	case COUNTER_BIT_MODE_16: {
		tc_count16_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT16);

		p_regs->TC_INTENSET = TC_INTFLAG_MC0_Msk;
		break;
	}
	case COUNTER_BIT_MODE_8: {
		tc_count8_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT8);

		p_regs->TC_INTENSET = TC_INTFLAG_OVF_Msk;
		break;
	}
	default:
		LOG_ERR("%s : Unsupported Counter mode %d", __func__, max_bit_width);
		ret_val = -ENOTSUP;
		break;
	}

	return ret_val;
}

static int32_t tc_counter_top_irq_disable(const void *tc_regs, const uint32_t max_bit_width)
{
	int32_t ret_val = 0;

	switch (max_bit_width) {
	case COUNTER_BIT_MODE_32: {
		tc_count32_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT32);

		p_regs->TC_INTENCLR = TC_INTFLAG_MC0_Msk;
		break;
	}
	case COUNTER_BIT_MODE_16: {
		tc_count16_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT16);

		p_regs->TC_INTENCLR = TC_INTFLAG_MC0_Msk;
		break;
	}
	case COUNTER_BIT_MODE_8: {
		tc_count8_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT8);

		p_regs->TC_INTENCLR = TC_INTFLAG_OVF_Msk;
		break;
	}
	default:
		LOG_ERR("%s : Unsupported Counter mode %d", __func__, max_bit_width);
		ret_val = -ENOTSUP;
		break;
	}

	return ret_val;
}

static int32_t tc_counter_top_irq_clear(const void *tc_regs, const uint32_t max_bit_width)
{
	int32_t ret_val = 0;

	switch (max_bit_width) {
	case COUNTER_BIT_MODE_32: {
		tc_count32_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT32);

		p_regs->TC_INTFLAG = TC_INTFLAG_MC0_Msk;
		break;
	}
	case COUNTER_BIT_MODE_16: {
		tc_count16_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT16);

		p_regs->TC_INTFLAG = TC_INTFLAG_MC0_Msk;
		break;
	}
	case COUNTER_BIT_MODE_8: {
		tc_count8_registers_t *p_regs = &(((tc_registers_t *)tc_regs)->COUNT8);

		p_regs->TC_INTFLAG = TC_INTFLAG_OVF_Msk;
		break;
	}
	default:
		LOG_ERR("%s : Unsupported Counter mode %d", __func__, max_bit_width);
		ret_val = -ENOTSUP;
		break;
	}

	return ret_val;
}

static int32_t tc_counter_top_irq_status(const uint32_t pending_irq_status,
					 const uint32_t max_bit_width)
{
	int32_t ret_val = 0;

	switch (max_bit_width) {
	case COUNTER_BIT_MODE_32: {
		ret_val =
			((pending_irq_status & (uint8_t)TC_INTFLAG_MC0_Msk) == TC_INTFLAG_MC0_Msk);
		break;
	}
	case COUNTER_BIT_MODE_16: {
		ret_val =
			((pending_irq_status & (uint8_t)TC_INTFLAG_MC0_Msk) == TC_INTFLAG_MC0_Msk);
		break;
	}
	case COUNTER_BIT_MODE_8: {
		ret_val =
			((pending_irq_status & (uint8_t)TC_INTFLAG_OVF_Msk) == TC_INTFLAG_OVF_Msk);
		break;
	}
	default:
		LOG_ERR("%s : Unsupported Counter mode %d", __func__, max_bit_width);
		ret_val = -ENOTSUP;
		break;
	}

	return ret_val;
}

static uint32_t tc_counter_ticks_sub(uint32_t val, uint32_t old, uint32_t top)
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

static uint32_t tc_counter_ticks_add(uint32_t val1, uint32_t val2, uint32_t top)
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

static uint32_t tc_counter_ticks_diff(uint32_t cntr_val_1, uint32_t cntr_val_2, uint32_t top)
{
	uint32_t diff =
		(cntr_val_1 > cntr_val_2) ? (cntr_val_1 - cntr_val_2) : (cntr_val_2 - cntr_val_1);
	uint32_t wrap_diff = top - diff;

	return (diff < wrap_diff) ? diff : wrap_diff;
}

static int32_t counter_mchp_start(const struct device *const dev)
{
	const struct counter_mchp_dev_config *const cfg = dev->config;

	return tc_counter_start(cfg->regs, cfg->max_bit_width);
}

static int32_t counter_mchp_stop(const struct device *const dev)
{
	const struct counter_mchp_dev_config *const cfg = dev->config;

	return tc_counter_stop(cfg->regs, cfg->max_bit_width);
}

static int32_t counter_mchp_get_value(const struct device *const dev, uint32_t *const ticks)
{
	const struct counter_mchp_dev_config *const cfg = dev->config;

	return tc_counter_get_count(cfg->regs, ticks, cfg->max_bit_width);
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

	tc_counter_get_period(cfg->regs, &top_value, cfg->max_bit_width);
	__ASSERT_NO_MSG(data->guard_period < top_value);

	/* Check if the requested tick value is less than top (period) value */
	if (ticks > top_value) {
		LOG_ERR("%s : invalid value requested", __func__);
		return -EINVAL;
	}

	if (NULL != data->channel_data[chan_id].callback) {
		LOG_ERR("alarm already set");
		return -EBUSY;
	}

	/* First take care of a risk of an event coming from CC being set to
	 * next tick. Reconfigure CC to future ( current counter value minus guard period
	 * is the furthestfuture).
	 */
	ret_status = tc_counter_get_count(cfg->regs, &count_value, cfg->max_bit_width);
	if (ret_status < 0) {
		LOG_ERR("%s : ret_val = %d", __func__, ret_status);
		return ret_status;
	}
	furthest_count_value = tc_counter_ticks_sub(count_value, data->guard_period, top_value);

	ret_status = tc_counter_set_compare(cfg->regs, chan_id, furthest_count_value,
					    cfg->max_bit_width);
	if (ret_status < 0) {
		LOG_ERR("%s : ret_val = %d", __func__, ret_status);
		return ret_status;
	}
	ret_status = tc_counter_alarm_irq_clear(cfg->regs, chan_id, cfg->max_bit_width);
	if (ret_status < 0) {
		LOG_ERR("%s : ret_val = %d", __func__, ret_status);
		return ret_status;
	}
	/* Update new callback functions */
	data->channel_data[chan_id].callback = alarm_cfg->callback;
	data->channel_data[chan_id].user_data = alarm_cfg->user_data;

	absolute = alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE;

	/* Check if counter has exceeded the alarm count in absolute alarm configuration */
	if (absolute != 0) {
		count_diff = tc_counter_ticks_diff(count_value, ticks, top_value);
		if (count_diff <= data->guard_period) {
			ret_status = -ETIME;
			irq_on_late = alarm_cfg->flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE;
			if (0 != irq_on_late) {
				data->late_alarm_flag = true;
				data->late_alarm_channel = chan_id;
				data->channel_data[chan_id].compare_value = ticks;
				NVIC_SetPendingIRQ(cfg->irq_line);
			} else {
				data->channel_data[chan_id].callback = NULL;
				data->channel_data[chan_id].user_data = NULL;
			}
		} else {
			data->channel_data[chan_id].compare_value = ticks;
			ret_status = tc_counter_set_compare(cfg->regs, chan_id, ticks,
							    cfg->max_bit_width);
			if (ret_status < 0) {
				LOG_ERR("%s : ret_val = %d", __func__, ret_status);
				return ret_status;
			}
			/* Enable interrupt at compare match */
			ret_status =
				tc_counter_alarm_irq_enable(cfg->regs, chan_id, cfg->max_bit_width);
			if (ret_status < 0) {
				LOG_ERR("%s : ret_val = %d", __func__, ret_status);
				return ret_status;
			}
		}
	} else {
		ticks = tc_counter_ticks_add(count_value, ticks, top_value);
		data->channel_data[chan_id].compare_value = ticks;
		tc_counter_set_compare(cfg->regs, chan_id, ticks, cfg->max_bit_width);

		/* Enable interrupt at compare match */
		tc_counter_alarm_irq_enable(cfg->regs, chan_id, cfg->max_bit_width);
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
	tc_counter_alarm_irq_disable(cfg->regs, chan_id, cfg->max_bit_width);
	tc_counter_alarm_irq_clear(cfg->regs, chan_id, cfg->max_bit_width);

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
	tc_counter_top_irq_disable(cfg->regs, cfg->max_bit_width);
	tc_counter_top_irq_clear(cfg->regs, cfg->max_bit_width);

	data->top_cb = top_cfg->callback;
	data->top_user_data = top_cfg->user_data;

	/* Update the counter period based on top configuration data */
	tc_counter_set_period(cfg->regs, top_cfg->ticks, cfg->max_bit_width);

	if (top_cfg->flags & COUNTER_TOP_CFG_DONT_RESET) {
		/*
		 * Top trigger is on equality of the rising edge only, so
		 * manually reset it if the counter has missed the new top.
		 */
		uint32_t counter_value = 0u;

		tc_counter_get_count(cfg->regs, &counter_value, cfg->max_bit_width);
		if (counter_value >= top_cfg->ticks) {
			ret_status = -ETIME;
			if ((top_cfg->flags & COUNTER_TOP_CFG_RESET_WHEN_LATE) ==
			    COUNTER_TOP_CFG_RESET_WHEN_LATE) {
				tc_counter_retrigger(cfg->regs, cfg->max_bit_width);
			}
		}
	} else {
		tc_counter_retrigger(cfg->regs, cfg->max_bit_width);
	}

	if (NULL != top_cfg->callback) {
		tc_counter_top_irq_enable(cfg->regs, cfg->max_bit_width);
	}

	return ret_status;
}

static uint32_t counter_mchp_get_pending_int(const struct device *const dev)
{
	const struct counter_mchp_dev_config *const cfg = dev->config;
	uint32_t ret_val;

	ret_val = tc_counter_get_pending_irqs(cfg->regs, cfg->max_bit_width);
	ret_val = (ret_val != 0) ? 1 : 0;
	return ret_val;
}

static uint32_t counter_mchp_get_top_value(const struct device *const dev)
{
	uint32_t period_value = 0u;
	const struct counter_mchp_dev_config *const cfg = dev->config;

	(void)tc_counter_get_period(cfg->regs, &period_value, cfg->max_bit_width);

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

	tc_counter_get_period(cfg->regs, &period_value, cfg->max_bit_width);
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
	const struct mchp_counter_clock *const clk = &cfg->counter_clock;

	clock_control_get_rate(clk->clock_dev, clk->host_gclk, &source_clk_freq);
	source_clk_freq = source_clk_freq / cfg->prescaler;

	return source_clk_freq;
}

static int32_t counter_init(const struct device *const dev)
{
	int32_t ret_status;
	const struct counter_mchp_dev_config *const cfg = dev->config;
	uint32_t max_counter_val;

	max_counter_val = (uint32_t)((1ULL << cfg->max_bit_width) - 1u);
	if (max_counter_val != cfg->info.max_top_value) {
		LOG_ERR("%s : Maximum bit width not allowed", __func__);
		return -EINVAL;
	}

	ret_status = tc_counter_init(cfg->regs, cfg->prescaler, cfg->max_bit_width);
	if (ret_status < 0) {
		LOG_ERR("%s : Counter failed to initialize", __func__);
		return ret_status;
	}
	cfg->irq_config_func(dev);

	return 0;
}

static void counter_mchp_alarm_irq_handler(const struct device *const dev)
{
	uint32_t cc_value = 0u;
	struct counter_mchp_dev_data *const data = dev->data;
	const struct counter_mchp_dev_config *const cfg = dev->config;
	uint32_t pending_irq_status = 0u;

	NVIC_ClearPendingIRQ(cfg->irq_line);
	pending_irq_status = tc_counter_get_pending_irqs(cfg->regs, cfg->max_bit_width);

	/* Check for immediate interrupt trigger */
	if (true == data->late_alarm_flag) {
		data->late_alarm_flag = false;
		if (NULL != data->channel_data[data->late_alarm_channel].callback) {
			counter_alarm_callback_t cb = NULL;

			cb = data->channel_data[data->late_alarm_channel].callback;
			if (cb != NULL) {
				data->channel_data[data->late_alarm_channel].callback = NULL;
				cc_value =
					data->channel_data[data->late_alarm_channel].compare_value;

				/* Execute callback function  */
				cb(dev, data->late_alarm_channel, cc_value,
				   data->channel_data[data->late_alarm_channel].user_data);
			}
		}
		return;
	}

	for (uint8_t chan_id = 0; chan_id < counter_get_num_of_channels(dev); chan_id++) {
		if (false !=
		    tc_counter_alarm_irq_status(pending_irq_status, chan_id, cfg->max_bit_width)) {
			tc_counter_alarm_irq_clear(cfg->regs, chan_id, cfg->max_bit_width);

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
	uint32_t pending_irq_status = 0u;

	NVIC_ClearPendingIRQ(cfg->irq_line);
	pending_irq_status = tc_counter_get_pending_irqs(cfg->regs, cfg->max_bit_width);

	if (0 != tc_counter_top_irq_status(pending_irq_status, cfg->max_bit_width)) {
		tc_counter_top_irq_clear(cfg->regs, cfg->max_bit_width);

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

static int32_t counter_mchp_init(const struct device *dev)
{
	int32_t ret_status;

	/*< Enable the clock for the device instance. */
	const struct counter_mchp_dev_config *cfg =
		(const struct counter_mchp_dev_config *)(dev->config);
	const struct mchp_counter_clock *const clk =
		(const struct mchp_counter_clock *const)&cfg->counter_clock;

	/* Enable host synchronous core clock */
	ret_status = clock_control_on(clk->clock_dev, clk->host_mclk);
	if ((ret_status < 0) && (ret_status != -EALREADY)) {
		LOG_ERR("%s : Unable to initialize host clock", __func__);
		return ret_status;
	}

	/* Conditionally enable client synchronous core clock */
	if (cfg->max_bit_width == 32) {
		if (clk->client_mclk != NULL) {
			ret_status = clock_control_on(clk->clock_dev, clk->client_mclk);
			if ((ret_status < 0) && (ret_status != -EALREADY)) {
				LOG_ERR("%s : Unable to initialize client clock", __func__);
				return ret_status;
			}
		} else {
			LOG_ERR("Peripheral does not support 32 bit mode");
			return -EINVAL;
		}
	}

	/* Enable peripheral asynchronous clock */
	ret_status = clock_control_on(clk->clock_dev, clk->host_gclk);
	if ((ret_status < 0) && (ret_status != -EALREADY)) {
		LOG_ERR("%s : Unable to initialize peripheral clock", __func__);
		return ret_status;
	}
	ret_status = counter_init(dev);

	return ret_status;
}

#define COUNTER_MCHP_CC_NUMS(n) ((DT_INST_PROP(n, max_bit_width) >= 16) ? 1 : 2)

#define COUNTER_MCHP_MAX_BIT_WIDTH(n) (DT_INST_PROP(n, max_bit_width))
#define COUNTER_MCHP_PRESCALER(n)                                                                  \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, prescaler),	\
						(DT_INST_PROP(n, prescaler)), (1))

#define GET_THE_CLIENT_MCLOCK_IF_AVAILABLE(n)                                                      \
	COND_CODE_1(DT_INST_CLOCKS_HAS_NAME(n, client_mclk),				\
	((void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, client_mclk, subsystem))),		\
	NULL)

#define COUNTER_MCHP_CLOCK_ASSIGN(n)                                                               \
	.counter_clock.clock_dev = DEVICE_DT_GET(DT_NODELABEL(clock)),                             \
	.counter_clock.host_mclk = (void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, subsystem)),      \
	.counter_clock.host_gclk = (void *)DT_INST_CLOCKS_CELL_BY_NAME(n, gclk, subsystem),        \
	.counter_clock.client_mclk = GET_THE_CLIENT_MCLOCK_IF_AVAILABLE(n)

/* clang-format off */
#define COUNTER_MCHP_CONFIG_VAR(n)								\
	static const  struct counter_mchp_dev_config counter_mchp_dev_config_##n = {		\
		.info = {									\
			.max_top_value =							\
				((uint32_t)((1ULL << COUNTER_MCHP_MAX_BIT_WIDTH(n)) -  1)),	\
			.freq = 0u,								\
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,					\
			.channels = COUNTER_MCHP_CC_NUMS(n) },					\
		.regs = (void *)DT_INST_REG_ADDR(n),						\
		COUNTER_MCHP_CLOCK_ASSIGN(n),							\
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

#define COUNTER_MCHP_CHANNEL_DATA_VAR(n)				\
	static struct counter_mchp_ch_data				\
		counter_mchp_channel_data_##n[COUNTER_MCHP_CC_NUMS(n)];

#define COUNTER_MCHP_TOP_DATA_VAR(n)						\
	static  struct counter_mchp_dev_data counter_mchp_dev_data_##n = {	\
		.channel_data = counter_mchp_channel_data_##n};

#define MCHP_COUNTER_IRQ_CONNECT(n, m)							\
	COND_CODE_1(DT_IRQ_HAS_IDX(DT_DRV_INST(n), m),					\
	(										\
		do {									\
			IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, m, irq),			\
			    DT_INST_IRQ_BY_IDX(n, m, priority),				\
			    counter_mchp_interrupt_handler, DEVICE_DT_INST_GET(n), 0);	\
			irq_enable(DT_INST_IRQ_BY_IDX(n, m, irq));			\
		} while (false);							\
	),										\
	()										\
	)

#define COUNTER_MCHP_DEVICE_INIT_FUNC(n)					\
	COUNTER_MCHP_IRQ_HANDLER(n)						\
	COUNTER_MCHP_CONFIG_VAR(n);						\
	COUNTER_MCHP_CHANNEL_DATA_VAR(n)					\
	COUNTER_MCHP_TOP_DATA_VAR(n)

#define COUNTER_MCHP_DEVICE_INIT(n)							\
	COUNTER_MCHP_DEVICE_INIT_FUNC(n)						\
	DEVICE_DT_INST_DEFINE(n, counter_mchp_init, NULL,				\
				  &counter_mchp_dev_data_##n,				\
				  &counter_mchp_dev_config_##n, POST_KERNEL,		\
				  CONFIG_COUNTER_INIT_PRIORITY, &counter_mchp_api);
/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(COUNTER_MCHP_DEVICE_INIT)

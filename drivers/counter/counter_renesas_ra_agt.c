/*
 * Copyright (c) 2024-2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_agt_counter

#include <soc.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/counter.h>

#include "r_agt.h"
#include "rp_agt.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(counter_renesas_ra_agt, CONFIG_COUNTER_LOG_LEVEL);

#define AGT_SUPPORTED_CHANNEL_COUNT 2

#define AGT_NUM_IRQ_ENABLED_CHANNELS(inst) \
	(DT_IRQ_HAS_NAME(TIMER(inst), agtcmai) + \
	 DT_IRQ_HAS_NAME(TIMER(inst), agtcmbi))

struct counter_renesas_ra_agt_config {
	struct counter_config_info info;
	void (*irq_config_func)(void);
};

struct counter_renesas_ra_agt_data {
	struct st_agt_instance_ctrl agt_ctrl;
	struct st_timer_cfg agt_cfg;
	struct st_agt_extended_cfg agt_extend_cfg;
	IRQn_Type agt_irq[AGT_SUPPORTED_CHANNEL_COUNT];
	uint8_t agt_ipl[AGT_SUPPORTED_CHANNEL_COUNT];
	uint32_t guard_period;
	counter_alarm_callback_t alarm_cb[AGT_SUPPORTED_CHANNEL_COUNT];
	counter_top_callback_t top_cb;
	void *alarm_data[AGT_SUPPORTED_CHANNEL_COUNT];
	void *top_data;
	struct k_spinlock lock;
};

extern void agt_int_isr(void);
extern void agtcmai_isr(void);
extern void agtcmbi_isr(void);

static inline bool renesas_ra_agt_is_running(const struct device *dev)
{
	struct counter_renesas_ra_agt_data *data = dev->data;

	return data->agt_ctrl.is_agtw ? (data->agt_ctrl.p_reg->AGT32.CTRL.AGTCR_b.TSTART == 1)
			: (data->agt_ctrl.p_reg->AGT16.CTRL.AGTCR_b.TSTART == 1);
}

static inline int renesas_ra_agt_period_set(const struct device *dev, uint32_t period)
{
	struct counter_renesas_ra_agt_data *data = dev->data;
	const bool counting = renesas_ra_agt_is_running(dev);
	fsp_err_t err;

	if (counting) {
		err = R_AGT_Stop(&data->agt_ctrl);
		if (err != FSP_SUCCESS) {
			return -EIO;
		}
	}

	err = R_AGT_PeriodSet(&data->agt_ctrl, period);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	if (counting) {
		err = R_AGT_Start(&data->agt_ctrl);
		if (err != FSP_SUCCESS) {
			return -EIO;
		}
	}

	return 0;
}

static inline int renesas_ra_agt_compare_match_set(const struct device *dev,
	uint32_t val, timer_compare_match_t channel)
{
	struct counter_renesas_ra_agt_data *data = dev->data;
	const bool counting = renesas_ra_agt_is_running(dev);
	fsp_err_t err;

	if (counting) {
		err = R_AGT_Stop(&data->agt_ctrl);
		if (err != FSP_SUCCESS) {
			return -EIO;
		}
	}

	err = RP_AGT_CompareMatchSet(&data->agt_ctrl, val, channel);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	if (counting) {
		err = R_AGT_Start(&data->agt_ctrl);
		if (err != FSP_SUCCESS) {
			return -EIO;
		}
	}

	return 0;
}

static inline k_spinlock_key_t counter_renesas_ra_agt_lock(const struct device *dev)
{
	struct counter_renesas_ra_agt_data *data = dev->data;

	return k_spin_lock(&data->lock);
}

static inline void counter_renesas_ra_agt_unlock(const struct device *dev, k_spinlock_key_t key)
{
	struct counter_renesas_ra_agt_data *data = dev->data;

	k_spin_unlock(&data->lock, key);
}

static int counter_renesas_ra_agt_start(const struct device *dev)
{
	struct counter_renesas_ra_agt_data *data = dev->data;
	fsp_err_t err;

	err = R_AGT_Start(&data->agt_ctrl);
	if (err != FSP_SUCCESS) {
		LOG_DBG("Counter start failed");
		return -EIO;
	}

	return 0;
}

static int counter_renesas_ra_agt_stop(const struct device *dev)
{
	struct counter_renesas_ra_agt_data *data = dev->data;
	fsp_err_t err;

	err = R_AGT_Stop(&data->agt_ctrl);
	if (err != FSP_SUCCESS) {
		LOG_DBG("Counter stop failed");
		return -EIO;
	}

	return 0;
}

static int counter_renesas_ra_agt_get_value(const struct device *dev, uint32_t *ticks)
{
	struct counter_renesas_ra_agt_data *data = dev->data;
	timer_status_t status;
	fsp_err_t err;

	err = R_AGT_StatusGet(&data->agt_ctrl, &status);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	*ticks = status.counter;

	return 0;
}

static uint32_t counter_renesas_ra_agt_get_top_value(const struct device *dev)
{
	struct counter_renesas_ra_agt_data *data = dev->data;

	return data->agt_ctrl.period;
}

static int counter_renesas_ra_agt_set_top_value(const struct device *dev,
						const struct counter_top_cfg *cfg)
{
	const struct counter_renesas_ra_agt_config *config = dev->config;
	struct counter_renesas_ra_agt_data *data = dev->data;
	k_spinlock_key_t key = counter_renesas_ra_agt_lock(dev);
	bool reset = false;
	uint32_t now = 0;
	fsp_err_t err;
	int ret = 0;

	if (cfg->ticks > config->info.max_top_value) {
		LOG_DBG("Top value exceed maximum value");
		ret = -EINVAL;
		goto out;
	}

	ret = renesas_ra_agt_period_set(dev, cfg->ticks);
	if (ret != 0) {
		LOG_DBG("Counter period set failed");
		goto out;
	}

	if (cfg->callback != NULL) {
		if (data->agt_cfg.cycle_end_irq == BSP_IRQ_DISABLED) {
			ret = -ENOTSUP;
			goto out;
		}

		irq_enable(data->agt_cfg.cycle_end_irq);
	} else {
		if (data->agt_cfg.cycle_end_ipl != BSP_IRQ_DISABLED) {
			irq_disable(data->agt_cfg.cycle_end_irq);
		}
	}

	data->top_cb = cfg->callback;
	data->top_data = cfg->user_data;

	if ((cfg->flags & COUNTER_TOP_CFG_DONT_RESET) == 0) {
		reset = true;
	} else if ((cfg->flags & COUNTER_TOP_CFG_RESET_WHEN_LATE) != 0) {
		ret = counter_renesas_ra_agt_get_value(dev, &now);
		if (ret != 0) {
			goto out;
		}

		reset = cfg->ticks < now;
	} else {
		;
	}

	if (reset) {
		err = R_AGT_Reset(&data->agt_ctrl);
		if (err != FSP_SUCCESS) {
			LOG_DBG("Counter reset failed");
			ret = -EIO;
			goto out;
		}
	}
out:
	counter_renesas_ra_agt_unlock(dev, key);
	return ret;
}

static inline uint32_t ticks_sub(uint32_t val, uint32_t old, uint32_t top)
{
	if (likely(IS_BIT_MASK(top))) {
		return (val - old) & top;
	}

	/* if top is not 2^n-1 */
	return (val >= old) ? (val - old) : val + top + 1 - old;
}

static int renesas_ra_agt_abs_alarm_set(const struct device *dev, uint32_t val, uint32_t top,
					bool irq_on_late, timer_compare_match_t channel)
{
	struct counter_renesas_ra_agt_data *data = dev->data;
	uint32_t max_val;
	uint32_t read_again;
	fsp_err_t err;
	uint8_t chan = (uint8_t)channel;
	int ret;

	ret = renesas_ra_agt_compare_match_set(dev, val, channel);
	if (ret != 0) {
		return ret;
	}

	ret = counter_renesas_ra_agt_get_value(dev, &read_again);
	if (ret != 0) {
		return ret;
	}

	max_val = ticks_sub(read_again + top, data->guard_period, top);
	if (val > max_val) {
		if (irq_on_late) {
			NVIC_SetPendingIRQ(data->agt_irq[chan]);
		} else {
			data->alarm_cb[chan] = NULL;
		}

		ret = -ETIME;
	}

	err = RP_AGT_EventSet(&data->agt_ctrl,
					(channel == TIMER_COMPARE_MATCH_A ? TIMER_AGT_AGTCMAI
									: TIMER_AGT_AGTCMBI), true);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	irq_enable(data->agt_irq[chan]);

	return ret;
}

static int renesas_ra_agt_rel_alarm_set(const struct device *dev,
	uint32_t val, uint32_t top, bool irq_on_late, timer_compare_match_t channel)
{
	struct counter_renesas_ra_agt_data *data = dev->data;
	uint32_t max_rel_val = irq_on_late ? (top / 2) : top;
	uint32_t diff;
	uint32_t now;
	fsp_err_t err;
	uint8_t chan = (uint8_t)channel;
	int ret;

	ret = counter_renesas_ra_agt_get_value(dev, &now);
	if (ret != 0) {
		return ret;
	}

	val = ticks_sub(now, val, top);

	ret = renesas_ra_agt_compare_match_set(dev, val, channel);
	if (ret != 0) {
		return ret;
	}

	ret = counter_renesas_ra_agt_get_value(dev, &now);
	if (ret != 0) {
		return ret;
	}

	diff = ticks_sub(now, val, top);

	if (diff > max_rel_val || diff == 0) {
		if (irq_on_late) {
			NVIC_SetPendingIRQ(data->agt_irq[chan]);
		} else {
			data->alarm_cb[chan] = NULL;
		}
	}

	err = RP_AGT_EventSet(&data->agt_ctrl,
					(channel == TIMER_COMPARE_MATCH_A ? TIMER_AGT_AGTCMAI
									: TIMER_AGT_AGTCMBI), true);

	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	irq_enable(data->agt_irq[chan]);

	return 0;
}

static int counter_renesas_ra_agt_set_alarm(const struct device *dev, uint8_t chan,
					    const struct counter_alarm_cfg *alarm_cfg)
{
	struct counter_renesas_ra_agt_data *data = dev->data;
	k_spinlock_key_t key = counter_renesas_ra_agt_lock(dev);
	const uint32_t top = counter_renesas_ra_agt_get_top_value(dev);
	const bool absolute = (alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) != 0;
	const bool irq_on_late =
		absolute ? (alarm_cfg->flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE) != 0
			 : alarm_cfg->ticks < (top / 2);
	timer_compare_match_t channel = (timer_compare_match_t)chan;
	int ret = 0;

	if (chan >= AGT_SUPPORTED_CHANNEL_COUNT) {
		ret = -EINVAL;
		goto out;
	}

	if (alarm_cfg->ticks > top) {
		ret = -EINVAL;
		goto out;
	}

	if (data->alarm_cb[chan] != NULL) {
		ret = -EBUSY;
		goto out;
	}

	if (data->agt_irq[chan] == BSP_IRQ_DISABLED) {
		ret = -ENOTSUP;
		goto out;
	}

	data->alarm_cb[chan] = alarm_cfg->callback;
	data->alarm_data[chan] = alarm_cfg->user_data;

	if (absolute) {
		ret = renesas_ra_agt_abs_alarm_set(dev, alarm_cfg->ticks, top,
						irq_on_late, channel);
	} else {
		ret = renesas_ra_agt_rel_alarm_set(dev, alarm_cfg->ticks, top,
						irq_on_late, channel);
	}
out:
	counter_renesas_ra_agt_unlock(dev, key);
	return ret;
}

static int counter_renesas_ra_agt_cancel_alarm(const struct device *dev, uint8_t chan)
{
	struct counter_renesas_ra_agt_data *data = dev->data;
	k_spinlock_key_t key = counter_renesas_ra_agt_lock(dev);
	fsp_err_t err;
	timer_compare_match_t channel = (timer_compare_match_t)chan;
	int ret = 0;

	if (chan >= AGT_SUPPORTED_CHANNEL_COUNT) {
		ret = -ENOTSUP;
		goto out;
	}

	if (data->agt_irq[chan] == BSP_IRQ_DISABLED) {
		ret = -ENOTSUP;
		goto out;
	}
	err = RP_AGT_EventSet(&data->agt_ctrl,
				(channel == TIMER_COMPARE_MATCH_A ? TIMER_AGT_AGTCMAI
								: TIMER_AGT_AGTCMBI), false);
	if (err != FSP_SUCCESS) {
		ret = -EIO;
		goto out;
	}

	irq_disable(data->agt_irq[chan]);
	R_BSP_IrqStatusClear(data->agt_irq[chan]);
	NVIC_ClearPendingIRQ(data->agt_irq[chan]);
	data->alarm_cb[chan] = NULL;
	data->alarm_data[chan] = NULL;
out:
	counter_renesas_ra_agt_unlock(dev, key);
	return ret;
}

static uint32_t counter_renesas_ra_agt_get_guard_period(const struct device *dev, uint32_t flags)
{
	struct counter_renesas_ra_agt_data *data = dev->data;

	return data->guard_period;
}

static int counter_renesas_ra_agt_set_guard_period(const struct device *dev, uint32_t guard,
						   uint32_t flags)
{
	struct counter_renesas_ra_agt_data *data = dev->data;
	k_spinlock_key_t key = counter_renesas_ra_agt_lock(dev);
	int ret = 0;

	if (counter_renesas_ra_agt_get_top_value(dev) < guard) {
		LOG_DBG("Invalid guard rate");
		ret = -EINVAL;
		goto out;
	}

	data->guard_period = guard;
out:
	counter_renesas_ra_agt_unlock(dev, key);
	return 0;
}

static uint32_t counter_renesas_ra_agt_get_pending_int(const struct device *dev)
{
	struct counter_renesas_ra_agt_data *data = dev->data;
	uint8_t event = 0;
	fsp_err_t err;

	err = RP_AGT_EventGet(&data->agt_ctrl, &event);
	if (err != FSP_SUCCESS) {
		LOG_DBG("Counter get status failed");
		return 0;
	}

	return (uint32_t)!!(
		event & (R_AGTX0_AGT16_CTRL_AGTCR_TCMAF_Msk |
			 R_AGTX0_AGT16_CTRL_AGTCR_TUNDF_Msk |
			  R_AGTX0_AGT16_CTRL_AGTCR_TCMBF_Msk));
}

static uint32_t counter_renesas_ra_agt_get_freq(const struct device *dev)
{
	struct counter_renesas_ra_agt_data *data = dev->data;
	timer_info_t info;
	fsp_err_t err;

	err = R_AGT_InfoGet(&data->agt_ctrl, &info);
	if (err != FSP_SUCCESS) {
		LOG_DBG("Counter get freq failed");
		return -EIO;
	}

	return info.clock_frequency;
}

static int counter_renesas_ra_agt_init(const struct device *dev)
{
	const struct counter_renesas_ra_agt_config *cfg = dev->config;
	struct counter_renesas_ra_agt_data *data = dev->data;
	fsp_err_t err;

	err = R_AGT_Open(&data->agt_ctrl, &data->agt_cfg);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	for (int i = 0; i < AGT_SUPPORTED_CHANNEL_COUNT; i++) {
		if (data->agt_irq[i] != BSP_IRQ_DISABLED) {
			R_FSP_IsrContextSet(data->agt_irq[i], &data->agt_ctrl);
		}
	}

	cfg->irq_config_func();

	return 0;
}

static void counter_renesas_ra_agt_agti_isr(const struct device *dev)
{
	struct counter_renesas_ra_agt_data *data = dev->data;
	counter_top_callback_t cb = data->top_cb;
	void *usr_data = data->top_data;

	if (cb != NULL) {
		cb(dev, usr_data);
	}

	agt_int_isr();
}

static void counter_renesas_ra_agt_alarm_isr(const struct device *dev, uint8_t chan)
{
	struct counter_renesas_ra_agt_data *data = dev->data;
	counter_alarm_callback_t cb = data->alarm_cb[chan];
	void *usr_data = data->alarm_data[chan];
	uint32_t now;

	if (cb != NULL) {
		data->alarm_cb[chan] = NULL;
		data->alarm_data[chan] = NULL;

		if (counter_renesas_ra_agt_get_value(dev, &now) != 0) {
			LOG_DBG("Error in counter alarm");
			return;
		}

		cb(dev, chan, now, usr_data);
	}
}

static __unused void counter_renesas_ra_agt_channel_a(const struct device *dev)
{
	counter_renesas_ra_agt_alarm_isr(dev, (uint8_t)TIMER_COMPARE_MATCH_A);
	agtcmai_isr();
}

static __unused void counter_renesas_ra_agt_channel_b(const struct device *dev)
{
	counter_renesas_ra_agt_alarm_isr(dev, (uint8_t)TIMER_COMPARE_MATCH_B);
	agtcmbi_isr();
}

static DEVICE_API(counter, agt_renesas_ra_driver_api) = {
	.start = counter_renesas_ra_agt_start,
	.stop = counter_renesas_ra_agt_stop,
	.get_value = counter_renesas_ra_agt_get_value,
	.set_alarm = counter_renesas_ra_agt_set_alarm,
	.cancel_alarm = counter_renesas_ra_agt_cancel_alarm,
	.set_top_value = counter_renesas_ra_agt_set_top_value,
	.get_pending_int = counter_renesas_ra_agt_get_pending_int,
	.get_top_value = counter_renesas_ra_agt_get_top_value,
	.get_freq = counter_renesas_ra_agt_get_freq,
	.get_guard_period = counter_renesas_ra_agt_get_guard_period,
	.set_guard_period = counter_renesas_ra_agt_set_guard_period,
};

#define TIMER(idx) DT_INST_PARENT(idx)

#define EVENT_AGT_INT(channel)       BSP_PRV_IELS_ENUM(CONCAT(EVENT_AGT, channel, _INT))
#define EVENT_AGT_COMPARE_A(channel) BSP_PRV_IELS_ENUM(CONCAT(EVENT_AGT, channel, _COMPARE_A))
#define EVENT_AGT_COMPARE_B(channel) BSP_PRV_IELS_ENUM(CONCAT(EVENT_AGT, channel, _COMPARE_B))

#define AGT_IRQ_GET_BY_NAME(inst, name, cell)                                                      \
	COND_CODE_1(DT_IRQ_HAS_NAME(TIMER(inst), name), (DT_IRQ_BY_NAME(TIMER(inst), name, cell)), \
		    ((IRQn_Type) BSP_IRQ_DISABLED))

#define AGT_IRQ_CONFIG(inst, name, event, isr)                                                     \
	IF_ENABLED(DT_IRQ_HAS_NAME(TIMER(inst), name),                                             \
		   (R_ICU->IELSR[DT_IRQ_BY_NAME(TIMER(inst), name, irq)] = event;                  \
		    BSP_ASSIGN_EVENT_TO_CURRENT_CORE(event);                                       \
		    IRQ_CONNECT(DT_IRQ_BY_NAME(TIMER(inst), name, irq),                            \
				DT_IRQ_BY_NAME(TIMER(inst), name, priority),                       \
				isr, DEVICE_DT_INST_GET(inst), 0);                                 \
				irq_disable(DT_IRQ_BY_NAME(TIMER(inst), name, irq));))

#define COUNTER_AGT_DEVICE_INIT(inst)                                                              \
	BUILD_ASSERT(!DT_IRQ_HAS_NAME(TIMER(inst), agtcmbi) ||                                     \
		DT_IRQ_HAS_NAME(TIMER(inst), agtcmai),                                             \
		"AGT instance " STRINGIFY(inst) ": agtcmai must be enabled to use agtcmbi; "       \
		"counter alarm comparison channels must be contiguous from earliest first");       \
                                                                                                   \
	static void counter_renesas_ra_agt##inst##_irq_config_func(void)                           \
	{                                                                                          \
		AGT_IRQ_CONFIG(inst, agti, EVENT_AGT_INT(DT_PROP(TIMER(inst), channel)),           \
			       counter_renesas_ra_agt_agti_isr);                                   \
		AGT_IRQ_CONFIG(inst, agtcmai, EVENT_AGT_COMPARE_A(DT_PROP(TIMER(inst), channel)),  \
			       counter_renesas_ra_agt_channel_a);                                  \
		AGT_IRQ_CONFIG(inst, agtcmbi, EVENT_AGT_COMPARE_B(DT_PROP(TIMER(inst), channel)),  \
			       counter_renesas_ra_agt_channel_b);                                  \
	}                                                                                          \
                                                                                                   \
	struct counter_renesas_ra_agt_config counter_renesas_ra_agt_config##inst = {               \
		.info.max_top_value =                                                              \
			DT_PROP(TIMER(inst), renesas_resolution) >= 32                             \
				? UINT32_MAX                                                       \
				: BIT_MASK(DT_PROP(TIMER(inst), renesas_resolution)),              \
		.info.flags = (uint8_t)0U,                                                         \
		.info.channels = AGT_NUM_IRQ_ENABLED_CHANNELS(inst),                               \
		.irq_config_func = counter_renesas_ra_agt##inst##_irq_config_func,                 \
	};                                                                                         \
                                                                                                   \
	static struct counter_renesas_ra_agt_data counter_renesas_ra_agt_data##inst = {            \
		.agt_cfg =                                                                         \
			{                                                                          \
				.mode = TIMER_MODE_PERIODIC,                                       \
				.period_counts = DT_PROP(TIMER(inst), renesas_resolution) >= 32    \
							 ? UINT32_MAX                              \
							 : BIT_MASK(DT_PROP(TIMER(inst),           \
									    renesas_resolution)),  \
				.source_div = DT_PROP(TIMER(inst), renesas_prescaler),             \
				.channel = DT_PROP(TIMER(inst), channel),                          \
				.cycle_end_irq = AGT_IRQ_GET_BY_NAME(inst, agti, irq),             \
				.cycle_end_ipl = AGT_IRQ_GET_BY_NAME(inst, agti, priority),        \
				.p_extend = &counter_renesas_ra_agt_data##inst.agt_extend_cfg,     \
			},                                                                         \
		.agt_extend_cfg =                                                                  \
			{                                                                          \
				.count_source = DT_STRING_TOKEN_OR(                                \
					TIMER(inst), renesas_count_source, AGT_CLOCK_LOCO),        \
				.agtoab_settings_b.agtoa = AGT_PIN_CFG_DISABLED,                   \
				.agtoab_settings_b.agtob = AGT_PIN_CFG_DISABLED,                   \
				.agto = AGT_PIN_CFG_DISABLED,                                      \
				.measurement_mode = AGT_MEASURE_DISABLED,                          \
				.agtio_filter = AGT_AGTIO_FILTER_NONE,                             \
				.enable_pin = AGT_ENABLE_PIN_NOT_USED,                             \
				.trigger_edge = AGT_TRIGGER_EDGE_RISING,                           \
			},                                                                         \
		.agt_irq[0] = AGT_IRQ_GET_BY_NAME(inst, agtcmai, irq),                             \
		.agt_ipl[0] = AGT_IRQ_GET_BY_NAME(inst, agtcmai, priority),                        \
		.agt_irq[1] = AGT_IRQ_GET_BY_NAME(inst, agtcmbi, irq),                             \
		.agt_ipl[1] = AGT_IRQ_GET_BY_NAME(inst, agtcmbi, priority),                        \
		.guard_period = 0,                                                                 \
	};                                                                                         \
												   \
	DEVICE_DT_INST_DEFINE(inst, counter_renesas_ra_agt_init, NULL,                             \
			      &counter_renesas_ra_agt_data##inst,                                  \
			      &counter_renesas_ra_agt_config##inst, POST_KERNEL,                   \
			      CONFIG_COUNTER_INIT_PRIORITY, &agt_renesas_ra_driver_api);

DT_INST_FOREACH_STATUS_OKAY(COUNTER_AGT_DEVICE_INIT)

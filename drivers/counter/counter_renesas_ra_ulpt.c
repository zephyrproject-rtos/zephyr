/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_ulpt_counter

/* Zephyr Counter Driver for Renesas RA Ultra-Low Power Timer.
 *
 * The ULPT is a down-counter counting from the reload value to 0.
 * The FSP reload value is (period - 1), mapping directly to Zephyr top_value.
 *
 * Compare match registers latch a new value only on underflow or when stopped.
 * Writing a new reload value while running takes effect on the next underflow
 * when compare match is enabled.
 *
 * Interrupt routing:
 *  ulpti    : Underflow interrupt, triggers the Top Callback at count 0.
 *  ulptcmai : Compare Match A interrupt, mapped to Alarm Channel 0.
 *  ulptcmbi : Compare Match B interrupt, mapped to Alarm Channel 1.
 */
#include <soc.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/logging/log.h>

#include "r_ulpt.h"
#include "rp_ulpt.h"

LOG_MODULE_REGISTER(counter_renesas_ra_ulpt, CONFIG_COUNTER_LOG_LEVEL);

#define RENESAS_RA_ULPT_COMPARE_CHANNEL_MAX 2U

/* Cycles required for register write synchronization with count source */
#define RENESAS_RA_ULPT_SYNC_CYCLES 5U

/* Absolute minimum guard to guarantee hardware compare latching */
#define RENESAS_RA_ULPT_GUARD_PERIOD_MIN (RENESAS_RA_ULPT_SYNC_CYCLES)

struct counter_renesas_ra_ulpt_config {
	struct counter_config_info info;
	void (*irq_config_func)(void);
	IRQn_Type alarm_irq[RENESAS_RA_ULPT_COMPARE_CHANNEL_MAX];
	IRQn_Type top_irq;
};

struct counter_renesas_ra_ulpt_alarm_data {
	counter_alarm_callback_t callback;
	void *user_data;
};

struct counter_renesas_ra_ulpt_data {
	struct st_ulpt_instance_ctrl fsp_ctrl;
	struct st_timer_cfg fsp_cfg;
	struct st_ulpt_extended_cfg fsp_extended_cfg;
	struct counter_renesas_ra_ulpt_alarm_data alarm[RENESAS_RA_ULPT_COMPARE_CHANNEL_MAX];
	counter_top_callback_t top_callback;
	void *top_user_data;
	struct k_spinlock lock;
	uint32_t guard_period;
	uint32_t top_value;
	atomic_t int_pending;
};

extern void ulpt_int_isr(void);

static inline void renesas_ra_ulpt_top_callback(const struct device *dev)
{
	struct counter_renesas_ra_ulpt_data *data = dev->data;
	counter_top_callback_t callback = data->top_callback;
	void *user_data = data->top_user_data;

	if (likely(callback)) {
		callback(dev, user_data);
	}
}

static void renesas_ra_ulpt_alarm_callback(const struct device *dev, uint8_t chan_id)
{
	struct counter_renesas_ra_ulpt_data *data = dev->data;
	counter_alarm_callback_t callback = data->alarm[chan_id].callback;
	void *user_data = data->alarm[chan_id].user_data;
	timer_status_t status;

	if (unlikely(!callback)) {
		return;
	}

	/* Clear before calling to allow re-arming */
	data->alarm[chan_id].callback = NULL;
	data->alarm[chan_id].user_data = NULL;

	RP_ULPT_CompareMatchDisable(&data->fsp_ctrl, chan_id);

	R_ULPT_StatusGet(&data->fsp_ctrl, &status);

	callback(dev, chan_id, status.counter, user_data);
}

static void renesas_ra_ulpt_callback(timer_callback_args_t *p_args)
{
	const struct device *dev = p_args->p_context;
	struct counter_renesas_ra_ulpt_data *data = dev->data;
	uint32_t int_pending = atomic_clear(&data->int_pending);
	uint8_t event;

	RP_ULPT_EventGet(&data->fsp_ctrl, &event);

	if (event & R_ULPT0_ULPTCR_TUNDF_Msk) {
		renesas_ra_ulpt_top_callback(dev);
	}

	if ((event & R_ULPT0_ULPTCR_TCMAF_Msk) || (int_pending & BIT(TIMER_COMPARE_MATCH_A))) {
		renesas_ra_ulpt_alarm_callback(dev, TIMER_COMPARE_MATCH_A);
	}

	if ((event & R_ULPT0_ULPTCR_TCMBF_Msk) || (int_pending & BIT(TIMER_COMPARE_MATCH_B))) {
		renesas_ra_ulpt_alarm_callback(dev, TIMER_COMPARE_MATCH_B);
	}
}

static int counter_renesas_ra_ulpt_start(const struct device *dev)
{
	struct counter_renesas_ra_ulpt_data *data = dev->data;

	if (R_ULPT_Start(&data->fsp_ctrl) != FSP_SUCCESS) {
		LOG_DBG("Failed to start ULPT");
		return -EIO;
	}

	return 0;
}

static int counter_renesas_ra_ulpt_stop(const struct device *dev)
{
	struct counter_renesas_ra_ulpt_data *data = dev->data;

	if (R_ULPT_Stop(&data->fsp_ctrl) != FSP_SUCCESS) {
		LOG_DBG("Failed to stop ULPT");
		return -EIO;
	}

	return 0;
}

static int counter_renesas_ra_ulpt_reset(const struct device *dev)
{
	struct counter_renesas_ra_ulpt_data *data = dev->data;

	if (R_ULPT_Reset(&data->fsp_ctrl) != FSP_SUCCESS) {
		LOG_DBG("Failed to reset ULPT");
		return -EIO;
	}

	return 0;
}

static int counter_renesas_ra_ulpt_get_value(const struct device *dev, uint32_t *ticks)
{
	struct counter_renesas_ra_ulpt_data *data = dev->data;
	timer_status_t status;

	if (R_ULPT_StatusGet(&data->fsp_ctrl, &status) != FSP_SUCCESS) {
		LOG_DBG("Failed to get ULPT status");
		return -EIO;
	}

	*ticks = status.counter;

	return 0;
}

static uint32_t counter_renesas_ra_ulpt_get_top_value(const struct device *dev)
{
	struct counter_renesas_ra_ulpt_data *data = dev->data;

	return data->top_value;
}

static inline uint32_t renesas_ra_ulpt_ticks_sub_mod(uint32_t now, uint32_t sub, uint32_t top)
{
	/* Computes (now - sub) mod (top + 1) on a down-counter.
	 *
	 * sub as a target position: result is the forward distance from now to sub.
	 * sub as a tick count:      result is the counter position sub ticks ahead of now.
	 */
	return (now >= sub) ? (now - sub) : ((top - sub) + 1U + now);
}

static inline bool renesas_ra_ulpt_is_alarm_late(uint32_t now, uint32_t compare, uint32_t top,
						 uint32_t guard)
{
	uint32_t diff = renesas_ra_ulpt_ticks_sub_mod(now, compare, top);

	/* Late conditions on the down-counter:
	 * diff <= guard       : Target is too close ahead to latch in time.
	 * diff >= top - guard : Target has already been passed.
	 */
	return (diff <= guard) || (diff >= (top - guard));
}

static int counter_renesas_ra_ulpt_set_alarm(const struct device *dev, uint8_t chan_id,
					     const struct counter_alarm_cfg *alarm_cfg)
{
	struct counter_renesas_ra_ulpt_data *data = dev->data;
	const struct counter_renesas_ra_ulpt_config *config = dev->config;
	const uint32_t ticks = alarm_cfg->ticks;
	const bool absolute = alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE;
	const bool expire_when_late = alarm_cfg->flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE;
	k_spinlock_key_t key;
	timer_status_t status;
	uint32_t top;
	uint32_t guard;
	uint32_t compare;
	uint32_t now;
	bool need_restart;

	if (chan_id >= RENESAS_RA_ULPT_COMPARE_CHANNEL_MAX) {
		return -ENOTSUP;
	}

	if (config->alarm_irq[chan_id] == FSP_INVALID_VECTOR) {
		return -ENOTSUP;
	}

	/* Claim the channel before touching hardware.
	 * Setting the callback marks it busy for other callers.
	 */
	key = k_spin_lock(&data->lock);

	/* Read period-dependent state after locking */
	top = data->top_value;
	guard = data->guard_period;

	if (ticks > top) {
		k_spin_unlock(&data->lock, key);
		return -EINVAL;
	}

	if (data->alarm[chan_id].callback != NULL) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}
	data->alarm[chan_id].callback = alarm_cfg->callback;
	data->alarm[chan_id].user_data = alarm_cfg->user_data;

	/* Capture current counter as baseline for relative alarms */
	if (R_ULPT_StatusGet(&data->fsp_ctrl, &status) != FSP_SUCCESS) {
		goto error;
	}

	now = status.counter;
	compare = absolute ? ticks : renesas_ra_ulpt_ticks_sub_mod(now, ticks, top);

	/* Check if the target is already late before hardware access */
	if (unlikely(renesas_ra_ulpt_is_alarm_late(now, compare, top, guard))) {
		goto late_alarm;
	}

	/* A compare value only latches on underflow or while stopped.
	 * Force an immediate latch by stopping and restarting if:
	 * now >= compare                     : Target reached before next underflow.
	 * now <= RENESAS_RA_ULPT_SYNC_CYCLES : Too close to underflow to sync in time.
	 */
	need_restart = (status.state == TIMER_STATE_COUNTING) &&
		       (now >= compare || now <= RENESAS_RA_ULPT_SYNC_CYCLES);

	if (need_restart) {
		if (R_ULPT_Stop(&data->fsp_ctrl) != FSP_SUCCESS) {
			goto error;
		}
	}

	if (RP_ULPT_CompareMatchSet(&data->fsp_ctrl, compare, chan_id) != FSP_SUCCESS ||
	    RP_ULPT_CompareMatchEnable(&data->fsp_ctrl, chan_id) != FSP_SUCCESS) {
		goto error;
	}

	/* Re-read after write: preemption or overhead may have advanced the counter */
	if (R_ULPT_StatusGet(&data->fsp_ctrl, &status) != FSP_SUCCESS) {
		goto error;
	}
	now = status.counter;

	if (need_restart) {
		if (R_ULPT_Start(&data->fsp_ctrl) != FSP_SUCCESS) {
			goto error;
		}
	}

	/* Re-check if accumulated latency pushed now into the late zone */
	if (unlikely(renesas_ra_ulpt_is_alarm_late(now, compare, top, guard))) {
		RP_ULPT_CompareMatchDisable(&data->fsp_ctrl, chan_id);
		goto late_alarm;
	}

	irq_enable(config->alarm_irq[chan_id]);
	k_spin_unlock(&data->lock, key);

	return 0;

late_alarm:
	/* Discard late absolute alarm when expiration is not requested */
	if (absolute && !expire_when_late) {
		data->alarm[chan_id].callback = NULL;
		data->alarm[chan_id].user_data = NULL;
		k_spin_unlock(&data->lock, key);

		NVIC_ClearPendingIRQ(config->alarm_irq[chan_id]);
		irq_disable(config->alarm_irq[chan_id]);

		return -ETIME;
	}

	/* Fire the alarm immediately via software IRQ */
	atomic_or(&data->int_pending, BIT(chan_id));
	k_spin_unlock(&data->lock, key);
	NVIC_SetPendingIRQ(config->alarm_irq[chan_id]);
	irq_enable(config->alarm_irq[chan_id]);

	return absolute ? -ETIME : 0;

error:
	/* Release the claim so the channel is not left busy on error */
	data->alarm[chan_id].callback = NULL;
	data->alarm[chan_id].user_data = NULL;
	k_spin_unlock(&data->lock, key);

	return -EIO;
}

static int counter_renesas_ra_ulpt_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	struct counter_renesas_ra_ulpt_data *data = dev->data;
	const struct counter_renesas_ra_ulpt_config *config = dev->config;
	k_spinlock_key_t key;

	if (chan_id >= RENESAS_RA_ULPT_COMPARE_CHANNEL_MAX) {
		return -ENOTSUP;
	}

	if (config->alarm_irq[chan_id] == FSP_INVALID_VECTOR) {
		return -ENOTSUP;
	}

	/* Hold the lock across the hardware teardown */
	key = k_spin_lock(&data->lock);

	RP_ULPT_CompareMatchDisable(&data->fsp_ctrl, chan_id);
	atomic_and(&data->int_pending, ~BIT(chan_id));
	irq_disable(config->alarm_irq[chan_id]);
	NVIC_ClearPendingIRQ(config->alarm_irq[chan_id]);

	data->alarm[chan_id].callback = NULL;
	data->alarm[chan_id].user_data = NULL;
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int counter_renesas_ra_ulpt_set_top_value(const struct device *dev,
						 const struct counter_top_cfg *cfg)
{
	struct counter_renesas_ra_ulpt_data *data = dev->data;
	const struct counter_renesas_ra_ulpt_config *config = dev->config;
	k_spinlock_key_t key;
	timer_status_t status;
	uint32_t period;

	if (cfg->callback != NULL && config->top_irq == FSP_INVALID_VECTOR) {
		return -ENOTSUP;
	}

	/* ULPT hardware does not support changing period without reset */
	if (cfg->flags & COUNTER_TOP_CFG_DONT_RESET) {
		return -ENOTSUP;
	}

	if (cfg->ticks > config->info.max_top_value || cfg->ticks == 0) {
		return -EINVAL;
	}

	/* Top value can only be changed when no alarm is active */
	key = k_spin_lock(&data->lock);
	if (data->alarm[TIMER_COMPARE_MATCH_A].callback != NULL ||
	    data->alarm[TIMER_COMPARE_MATCH_B].callback != NULL) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}

	/* FSP expects the period count, which is top_value + 1 */
	period = cfg->ticks + 1U;

	/* Reload values latch immediately if stopped or compare match is disabled.
	 * Otherwise, they defer to the next underflow.
	 *
	 * Stopped:
	 * Write latches immediately.
	 *
	 * Close to underflow:
	 * Force immediate latch by stopping and restarting if now <= RENESAS_RA_ULPT_SYNC_CYCLES.
	 *
	 * Far from underflow:
	 * Disable compare matches temporarily to force immediate latching.
	 */
	if (R_ULPT_StatusGet(&data->fsp_ctrl, &status) != FSP_SUCCESS) {
		goto error;
	}

	if (unlikely(status.state != TIMER_STATE_COUNTING)) {
		if (R_ULPT_PeriodSet(&data->fsp_ctrl, period) != FSP_SUCCESS) {
			goto error;
		}
	} else if (unlikely(status.counter <= RENESAS_RA_ULPT_SYNC_CYCLES)) {
		if (R_ULPT_Stop(&data->fsp_ctrl) != FSP_SUCCESS ||
		    R_ULPT_PeriodSet(&data->fsp_ctrl, period) != FSP_SUCCESS ||
		    R_ULPT_Start(&data->fsp_ctrl) != FSP_SUCCESS) {
			goto error;
		}
	} else {
		if (RP_ULPT_CompareMatchDisable(&data->fsp_ctrl,
					TIMER_COMPARE_MATCH_A) != FSP_SUCCESS ||
		    RP_ULPT_CompareMatchDisable(&data->fsp_ctrl,
					TIMER_COMPARE_MATCH_B) != FSP_SUCCESS ||
		    R_ULPT_PeriodSet(&data->fsp_ctrl, period) != FSP_SUCCESS) {
			goto error;
		}
	}

	data->top_value = cfg->ticks;
	data->top_callback = cfg->callback;
	data->top_user_data = cfg->user_data;
	k_spin_unlock(&data->lock, key);

	if (config->top_irq == FSP_INVALID_VECTOR) {
		return 0;
	}

	/* Enable underflow IRQ only when a callback is registered */
	if (cfg->callback) {
		irq_enable(config->top_irq);
	} else {
		irq_disable(config->top_irq);
	}

	return 0;
error:
	k_spin_unlock(&data->lock, key);
	return -EIO;
}

static uint32_t counter_renesas_ra_ulpt_get_pending_int(const struct device *dev)
{
	struct counter_renesas_ra_ulpt_data *data = dev->data;
	uint8_t event;

	RP_ULPT_EventGet(&data->fsp_ctrl, &event);

	return (uint32_t)(event != 0);
}

static uint32_t counter_renesas_ra_ulpt_get_guard_period(const struct device *dev, uint32_t flags)
{
	const struct counter_renesas_ra_ulpt_data *data = dev->data;

	ARG_UNUSED(flags);

	return data->guard_period;
}

static int counter_renesas_ra_ulpt_set_guard_period(const struct device *dev, uint32_t ticks,
						    uint32_t flags)
{
	struct counter_renesas_ra_ulpt_data *data = dev->data;
	k_spinlock_key_t key;

	ARG_UNUSED(flags);

	key = k_spin_lock(&data->lock);

	if (ticks > data->top_value) {
		k_spin_unlock(&data->lock, key);
		return -EINVAL;
	}

	/* Enforce minimum guard period for hardware safe latching */
	data->guard_period = MAX(ticks, RENESAS_RA_ULPT_GUARD_PERIOD_MIN);

	k_spin_unlock(&data->lock, key);

	return 0;
}

static uint32_t counter_renesas_ra_ulpt_get_freq(const struct device *dev)
{
	struct counter_renesas_ra_ulpt_data *data = dev->data;
	timer_info_t info;

	if (R_ULPT_InfoGet(&data->fsp_ctrl, &info) != FSP_SUCCESS) {
		LOG_DBG("Failed to get ULPT info");
		return 0;
	}

	return info.clock_frequency;
}

static int counter_renesas_ra_ulpt_init(const struct device *dev)
{
	const struct counter_renesas_ra_ulpt_config *config = dev->config;
	struct counter_renesas_ra_ulpt_data *data = dev->data;
	fsp_err_t err;

	err = R_ULPT_Open(&data->fsp_ctrl, &data->fsp_cfg);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	/* Bind each IRQ to this FSP instance so ulpt_int_isr can resolve
	 * the correct control block via R_FSP_IsrContextGet.
	 */
	if (config->top_irq != FSP_INVALID_VECTOR) {
		R_FSP_IsrContextSet(config->top_irq, &data->fsp_ctrl);
	}

	if (config->alarm_irq[TIMER_COMPARE_MATCH_A] != FSP_INVALID_VECTOR) {
		R_FSP_IsrContextSet(config->alarm_irq[TIMER_COMPARE_MATCH_A], &data->fsp_ctrl);
	}

	if (config->alarm_irq[TIMER_COMPARE_MATCH_B] != FSP_INVALID_VECTOR) {
		R_FSP_IsrContextSet(config->alarm_irq[TIMER_COMPARE_MATCH_B], &data->fsp_ctrl);
	}

	config->irq_config_func();

	return 0;
}

static DEVICE_API(counter, counter_renesas_ra_ulpt_driver_api) = {
	.start = counter_renesas_ra_ulpt_start,
	.stop = counter_renesas_ra_ulpt_stop,
	.get_value = counter_renesas_ra_ulpt_get_value,
	.reset = counter_renesas_ra_ulpt_reset,
	.set_alarm = counter_renesas_ra_ulpt_set_alarm,
	.cancel_alarm = counter_renesas_ra_ulpt_cancel_alarm,
	.set_top_value = counter_renesas_ra_ulpt_set_top_value,
	.get_pending_int = counter_renesas_ra_ulpt_get_pending_int,
	.get_top_value = counter_renesas_ra_ulpt_get_top_value,
	.get_guard_period = counter_renesas_ra_ulpt_get_guard_period,
	.set_guard_period = counter_renesas_ra_ulpt_set_guard_period,
	.get_freq = counter_renesas_ra_ulpt_get_freq,
};

/* BSP event enum for the underflow interrupt */
#define RENESAS_RA_ULPTI_EVENT(inst)                                                               \
	BSP_PRV_IELS_ENUM(CONCAT(EVENT_ULPT, DT_PROP(DT_INST_PARENT(inst), channel), _INT))

/* BSP event enum for the compare match A interrupt */
#define RENESAS_RA_ULPTCMA_EVENT(inst)                                                             \
	BSP_PRV_IELS_ENUM(CONCAT(EVENT_ULPT, DT_PROP(DT_INST_PARENT(inst), channel), _COMPARE_A))

/* BSP event enum for the compare match B interrupt */
#define RENESAS_RA_ULPTCMB_EVENT(inst)                                                             \
	BSP_PRV_IELS_ENUM(CONCAT(EVENT_ULPT, DT_PROP(DT_INST_PARENT(inst), channel), _COMPARE_B))

/* IRQ number for a named interrupt, or FSP_INVALID_VECTOR if absent */
#define RENESAS_RA_ULPT_IRQ_NUM(inst, name)                                                        \
	COND_CODE_1(DT_IRQ_HAS_NAME(DT_INST_PARENT(inst), name),                                   \
		   (DT_IRQ_BY_NAME(DT_INST_PARENT(inst), name, irq)),                              \
		   (FSP_INVALID_VECTOR))

/* IRQ priority for a named interrupt, or BSP_IRQ_DISABLED if absent */
#define RENESAS_RA_ULPT_IRQ_PRIO(inst, name)                                                       \
	COND_CODE_1(DT_IRQ_HAS_NAME(DT_INST_PARENT(inst), name),                                   \
		   (DT_IRQ_BY_NAME(DT_INST_PARENT(inst), name, priority)),                         \
		   (BSP_IRQ_DISABLED))

/* Connect a named IRQ if present in the parent devicetree node */
#define RENESAS_RA_ULPT_IRQ_CONNECT(inst, name, event, isr)                                        \
	IF_ENABLED(DT_IRQ_HAS_NAME(DT_INST_PARENT(inst), name), (                                  \
		   R_ICU->IELSR[RENESAS_RA_ULPT_IRQ_NUM(inst, name)] = event;                      \
		   BSP_ASSIGN_EVENT_TO_CURRENT_CORE(event);                                        \
		   IRQ_CONNECT(RENESAS_RA_ULPT_IRQ_NUM(inst, name),                                \
			   RENESAS_RA_ULPT_IRQ_PRIO(inst, name),                                   \
			   isr, NULL, 0);                                                          \
	   ))

#define COUNTER_RENESAS_RA_ULPT_INIT(inst)                                                         \
	static void counter_renesas_ra_ulpt_irq_config_func_##inst(void)                           \
	{                                                                                          \
		RENESAS_RA_ULPT_IRQ_CONNECT(inst, ulpti, RENESAS_RA_ULPTI_EVENT(inst),             \
					    ulpt_int_isr);                                         \
		RENESAS_RA_ULPT_IRQ_CONNECT(inst, ulptcmai, RENESAS_RA_ULPTCMA_EVENT(inst),        \
					    ulpt_int_isr);                                         \
		RENESAS_RA_ULPT_IRQ_CONNECT(inst, ulptcmbi, RENESAS_RA_ULPTCMB_EVENT(inst),        \
					    ulpt_int_isr);                                         \
	}                                                                                          \
                                                                                                   \
	static const struct counter_renesas_ra_ulpt_config counter_renesas_ra_ulpt_cfg##inst = {   \
		.info =                                                                            \
			{                                                                          \
				.freq = 0,                                                         \
				.max_top_value = UINT32_MAX - 1U,                                  \
				.flags = (uint8_t)0U,                                              \
				.channels = RENESAS_RA_ULPT_COMPARE_CHANNEL_MAX,                   \
			},                                                                         \
		.alarm_irq =                                                                       \
			{                                                                          \
				[TIMER_COMPARE_MATCH_A] = RENESAS_RA_ULPT_IRQ_NUM(inst, ulptcmai), \
				[TIMER_COMPARE_MATCH_B] = RENESAS_RA_ULPT_IRQ_NUM(inst, ulptcmbi), \
			},                                                                         \
		.irq_config_func = counter_renesas_ra_ulpt_irq_config_func_##inst,                 \
		.top_irq = RENESAS_RA_ULPT_IRQ_NUM(inst, ulpti),                                   \
	};                                                                                         \
                                                                                                   \
	static struct counter_renesas_ra_ulpt_data counter_renesas_ra_ulpt_data##inst = {          \
		.fsp_cfg =                                                                         \
			{                                                                          \
				.mode = TIMER_MODE_PERIODIC,                                       \
				.period_counts = UINT32_MAX,                                       \
				.duty_cycle_counts = 0U,                                           \
				.source_div = DT_PROP(DT_INST_PARENT(inst), prescaler),            \
				.channel = DT_PROP(DT_INST_PARENT(inst), channel),                 \
				.p_callback = renesas_ra_ulpt_callback,                            \
				.cycle_end_irq = FSP_INVALID_VECTOR,                               \
				.cycle_end_ipl = BSP_IRQ_DISABLED,                                 \
				.p_context = (void *)DEVICE_DT_INST_GET(inst),                     \
				.p_extend = &counter_renesas_ra_ulpt_data##inst.fsp_extended_cfg,  \
			},                                                                         \
                                                                                                   \
		.fsp_extended_cfg =                                                                \
			{                                                                          \
				.count_source =                                                    \
					DT_STRING_TOKEN(DT_INST_PARENT(inst), count_source),       \
				.ulpto = ULPT_PULSE_PIN_CFG_DISABLED,                              \
				.ulptoab_settings_b.ulptoa = ULPT_MATCH_PIN_CFG_DISABLED,          \
				.ulptoab_settings_b.ulptob = ULPT_MATCH_PIN_CFG_DISABLED,          \
				.ulptevi_filter = ULPT_ULPTEVI_FILTER_NONE,                        \
				.enable_function = ULPT_ENABLE_FUNCTION_IGNORED,                   \
				.trigger_edge = ULPT_TRIGGER_EDGE_RISING,                          \
				.event_pin = ULPT_EVENT_PIN_RISING,                                \
			},                                                                         \
		.guard_period = RENESAS_RA_ULPT_GUARD_PERIOD_MIN,                                  \
		.top_value = UINT32_MAX - 1U,                                                      \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, counter_renesas_ra_ulpt_init, NULL,                            \
			      &counter_renesas_ra_ulpt_data##inst,                                 \
			      &counter_renesas_ra_ulpt_cfg##inst, POST_KERNEL,                     \
			      CONFIG_COUNTER_INIT_PRIORITY, &counter_renesas_ra_ulpt_driver_api);

DT_INST_FOREACH_STATUS_OKAY(COUNTER_RENESAS_RA_ULPT_INIT)

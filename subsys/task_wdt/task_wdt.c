/*
 * Copyright (c) 2020 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/task_wdt/task_wdt.h>

#include <zephyr/debug/coredump.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/device.h>
#include <errno.h>

#define LOG_LEVEL CONFIG_WDT_LOG_LEVEL
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
LOG_MODULE_REGISTER(task_wdt);

#include "task_wdt_reset_info.h"

/*
 * This dummy channel is used to continue feeding the hardware watchdog if the
 * task watchdog timeouts are too long for regular updates
 */
#define TASK_WDT_BACKGROUND_CHANNEL UINTPTR_MAX

/*
 * Task watchdog channel data
 */
struct task_wdt_channel {
	/* period in milliseconds used to reset the timeout, set to 0 to
	 * indicate that the channel is available
	 */
	uint32_t reload_period;
	/* abs. ticks when this channel expires (updated by task_wdt_feed) */
	int64_t timeout_abs_ticks;
	/* user data passed to the callback function */
	void *user_data;
	/* function to be called when watchdog timer expired */
	task_wdt_callback_t callback;
#ifdef CONFIG_TASK_WDT_FAULT_REASONS
	/* function to be called with the fault reason, takes precedence over callback */
	task_wdt_fault_callback_t fault_callback;
	/* minimum interval in milliseconds between two feeds, 0 if not checked */
	uint32_t min_period;
	/* abs. ticks of the last task_wdt_feed() call */
	int64_t last_feed_ticks;
#endif
#ifdef CONFIG_TASK_WDT_THREAD_INFO
	/* thread owning the channel, NULL if not known */
	k_tid_t thread;
	/* owner set with task_wdt_channel_thread_set(), not updated on feed */
	bool thread_fixed;
#endif
};

/* array of all task watchdog channels */
static struct task_wdt_channel channels[CONFIG_TASK_WDT_CHANNELS];
static struct k_spinlock channels_lock;

/* timer used for watchdog handling */
static struct k_timer timer;

/* Tell whether the Task Watchdog has been fully initialized. */
static bool task_wdt_initialized;

#ifdef CONFIG_TASK_WDT_HW_FALLBACK
/* pointer to the hardware watchdog used as a fallback */
static const struct device *hw_wdt_dev;
static int hw_wdt_channel;
static bool hw_wdt_started;
#endif

static void schedule_next_timeout(int64_t current_ticks)
{
	uintptr_t next_channel_id;	/* channel which will time out next */
	int64_t next_timeout;		/* timeout in absolute ticks of this channel */

#ifdef CONFIG_TASK_WDT_HW_FALLBACK
	next_channel_id = TASK_WDT_BACKGROUND_CHANNEL;
	next_timeout = current_ticks +
		k_ms_to_ticks_ceil64(CONFIG_TASK_WDT_MIN_TIMEOUT);
#else
	next_channel_id = 0;
	next_timeout = INT64_MAX;
#endif

	/* find minimum timeout of all channels */
	for (int id = 0; id < ARRAY_SIZE(channels); id++) {
		if (channels[id].reload_period != 0 &&
		    channels[id].timeout_abs_ticks < next_timeout) {
			next_channel_id = id;
			next_timeout = channels[id].timeout_abs_ticks;
		}
	}

	/* update task wdt kernel timer */
	k_timer_user_data_set(&timer, (void *)next_channel_id);
	k_timer_start(&timer, K_TIMEOUT_ABS_TICKS(next_timeout), K_FOREVER);

#ifdef CONFIG_TASK_WDT_HW_FALLBACK
	if (hw_wdt_started) {
		wdt_feed(hw_wdt_dev, hw_wdt_channel);
	}
#endif
}

#ifdef CONFIG_TASK_WDT_THREAD_INFO
static void task_wdt_report(int channel_id, enum task_wdt_fault_reason reason)
{
	struct task_wdt_channel *channel = &channels[channel_id];
	k_tid_t thread = channel->thread;
	int64_t last_feed_ticks =
		channel->timeout_abs_ticks - k_ms_to_ticks_ceil64(channel->reload_period);
	char state[32];
	const char *name;

	if (reason == TASK_WDT_FAULT_TOO_EARLY) {
		LOG_ERR("Channel %d fed too early", channel_id);
	} else {
		LOG_ERR("Channel %d timed out, last fed %u ms ago", channel_id,
			(uint32_t)k_ticks_to_ms_floor64(sys_clock_tick_get() - last_feed_ticks));
	}

	if (thread == NULL) {
		LOG_ERR("Channel %d has no owner thread", channel_id);
		return;
	}

	name = k_thread_name_get(thread);
	LOG_ERR("Owner thread %p (%s): %s, priority %d", (void *)thread, name != NULL ? name : "",
		k_thread_state_str(thread, state, sizeof(state)), k_thread_priority_get(thread));

#if defined(CONFIG_INIT_STACKS) && defined(CONFIG_THREAD_STACK_INFO)
	size_t unused;

	/* Some architectures fault when inspecting the stack in use */
	if ((!IS_ENABLED(CONFIG_NO_UNUSED_STACK_INSPECTION) || thread != k_current_get()) &&
	    k_thread_stack_space_get(thread, &unused) == 0) {
		LOG_ERR("Owner thread %p: %zu bytes of stack unused", (void *)thread, unused);
	}
#endif
}
#endif /* CONFIG_TASK_WDT_THREAD_INFO */

static void task_wdt_fault(int channel_id, enum task_wdt_fault_reason reason)
{
	struct task_wdt_fault fault = {
		.channel_id = channel_id,
		.reason = reason,
#ifdef CONFIG_TASK_WDT_THREAD_INFO
		.thread = channels[channel_id].thread,
#endif
	};
	bool handled = false;

	/* only used with the reset information or fault reasons */
	ARG_UNUSED(fault);

#ifdef CONFIG_TASK_WDT_THREAD_INFO
	task_wdt_report(channel_id, reason);
#endif

#ifdef CONFIG_TASK_WDT_RESET_INFO
	/* the callback may reset the system, so store the record beforehand */
	task_wdt_reset_info_save(&fault);
#endif

#ifdef CONFIG_TASK_WDT_FAULT_REASONS
	if (channels[channel_id].fault_callback) {
		channels[channel_id].fault_callback(&fault, channels[channel_id].user_data);
		handled = true;
	}
#endif
	if (!handled && channels[channel_id].callback) {
		channels[channel_id].callback(channel_id, channels[channel_id].user_data);
		handled = true;
	}

	if (handled) {
#ifdef CONFIG_TASK_WDT_RESET_INFO
		/* the callback handled the fault without a reset */
		task_wdt_reset_info_cancel();
#endif
	} else {
#ifdef CONFIG_TASK_WDT_COREDUMP
		if (channels[channel_id].thread != NULL) {
			coredump(K_ERR_KERNEL_PANIC, NULL, channels[channel_id].thread);
		}
#endif
#ifdef CONFIG_TASK_WDT_THREAD_INFO
		/* flush the report before the reset */
		LOG_PANIC();
#endif
		sys_reboot(SYS_REBOOT_COLD);
	}
}

/**
 * @brief Task watchdog timer callback.
 *
 * If the device operates as intended, this function will never be called,
 * as the timer is continuously restarted with the next due timeout in the
 * task_wdt_feed() function.
 *
 * If all task watchdogs have longer timeouts than the hardware watchdog,
 * this function is called regularly (via the background channel). This
 * should be avoided by setting CONFIG_TASK_WDT_MIN_TIMEOUT to the minimum
 * task watchdog timeout used in the application.
 *
 * @param timer_id Pointer to the timer which called the function
 */
static void task_wdt_trigger(struct k_timer *timer_id)
{
	uintptr_t channel_id = (uintptr_t)k_timer_user_data_get(timer_id);
	bool bg_channel = IS_ENABLED(CONFIG_TASK_WDT_HW_FALLBACK) &&
			  (channel_id == TASK_WDT_BACKGROUND_CHANNEL);

	/* If the timeout expired for the background channel (so the hardware
	 * watchdog needs to be fed) or for a channel that has been deleted,
	 * only schedule a new timeout (the hardware watchdog, if used, will be
	 * fed right after that new timeout is scheduled).
	 */
	if (bg_channel || channels[channel_id].reload_period == 0) {
		schedule_next_timeout(sys_clock_tick_get());
		return;
	}

	task_wdt_fault(channel_id, TASK_WDT_FAULT_TIMEOUT);
}

int task_wdt_init(const struct device *hw_wdt)
{
	if (hw_wdt) {
#ifdef CONFIG_TASK_WDT_HW_FALLBACK
		struct wdt_timeout_cfg wdt_config;

		wdt_config.flags = WDT_FLAG_RESET_SOC;
		wdt_config.window.min = 0U;
		wdt_config.window.max = CONFIG_TASK_WDT_MIN_TIMEOUT +
			CONFIG_TASK_WDT_HW_FALLBACK_DELAY;
		wdt_config.callback = NULL;

		hw_wdt_dev = hw_wdt;
		hw_wdt_channel = wdt_install_timeout(hw_wdt_dev, &wdt_config);
		if (hw_wdt_channel < 0) {
			LOG_ERR("hw_wdt install timeout failed: %d", hw_wdt_channel);
			return hw_wdt_channel;
		}
#else
		return -ENOTSUP;
#endif
	}

	k_timer_init(&timer, task_wdt_trigger, NULL);
	schedule_next_timeout(sys_clock_tick_get());

	task_wdt_initialized = true;

	return 0;
}

static int channel_add(uint32_t reload_period, task_wdt_callback_t callback,
		       task_wdt_fault_callback_t fault_callback, void *user_data)
{
	k_spinlock_key_t key;

	if (reload_period == 0) {
		return -EINVAL;
	}

	/*
	 * k_spin_lock instead of k_sched_lock required here to avoid being interrupted by a
	 * triggering other task watchdog channel (executed in ISR context).
	 */
	key = k_spin_lock(&channels_lock);

	/* look for unused channel (reload_period set to 0) */
	for (int id = 0; id < ARRAY_SIZE(channels); id++) {
		if (channels[id].reload_period == 0) {
			channels[id].reload_period = reload_period;
			channels[id].user_data = user_data;
			channels[id].timeout_abs_ticks = K_TICKS_FOREVER;
			channels[id].callback = callback;
#ifdef CONFIG_TASK_WDT_FAULT_REASONS
			channels[id].fault_callback = fault_callback;
			channels[id].min_period = 0U;
#else
			ARG_UNUSED(fault_callback);
#endif
#ifdef CONFIG_TASK_WDT_THREAD_INFO
			channels[id].thread = NULL;
			channels[id].thread_fixed = false;
#endif

#ifdef CONFIG_TASK_WDT_HW_FALLBACK
			if (!hw_wdt_started && hw_wdt_dev) {
				/* also start fallback hw wdt */
				wdt_setup(hw_wdt_dev,
					0
#ifdef CONFIG_TASK_WDT_HW_FALLBACK_PAUSE_HALTED_BY_DBG
					| WDT_OPT_PAUSE_HALTED_BY_DBG
#endif
#ifdef CONFIG_TASK_WDT_HW_FALLBACK_PAUSE_IN_SLEEP
					| WDT_OPT_PAUSE_IN_SLEEP
#endif
				);
				hw_wdt_started = true;
			}
#endif
			/* must be called after hw wdt has been started */
			task_wdt_feed(id);

			k_spin_unlock(&channels_lock, key);

			return id;
		}
	}

	k_spin_unlock(&channels_lock, key);

	return -ENOMEM;
}

int task_wdt_add(uint32_t reload_period, task_wdt_callback_t callback, void *user_data)
{
	return channel_add(reload_period, callback, NULL, user_data);
}

int task_wdt_add_ext(uint32_t reload_period, task_wdt_fault_callback_t callback, void *user_data)
{
#ifdef CONFIG_TASK_WDT_FAULT_REASONS
	return channel_add(reload_period, NULL, callback, user_data);
#else
	ARG_UNUSED(reload_period);
	ARG_UNUSED(callback);
	ARG_UNUSED(user_data);

	return -ENOTSUP;
#endif
}

int task_wdt_delete(int channel_id)
{
	k_spinlock_key_t key;

	if (channel_id < 0 || channel_id >= ARRAY_SIZE(channels)) {
		return -EINVAL;
	}

	key = k_spin_lock(&channels_lock);

	channels[channel_id].reload_period = 0;

	k_spin_unlock(&channels_lock, key);

	return 0;
}

int task_wdt_feed(int channel_id)
{
	int64_t current_ticks;
	bool too_early = false;

	if (channel_id < 0 || channel_id >= ARRAY_SIZE(channels) ||
	    channels[channel_id].reload_period == 0) {
		return -EINVAL;
	}

	/*
	 * We need a critical section instead of a mutex while updating the
	 * channels array in order to prevent priority inversion. Otherwise,
	 * a low priority thread could be preempted before releasing the mutex
	 * and block a high priority thread that wants to feed its task wdt.
	 */
	k_sched_lock();

	current_ticks = sys_clock_tick_get();

#ifdef CONFIG_TASK_WDT_FAULT_REASONS
	too_early = channels[channel_id].min_period != 0U &&
		    current_ticks - channels[channel_id].last_feed_ticks <
			    k_ms_to_ticks_ceil64(channels[channel_id].min_period);
	channels[channel_id].last_feed_ticks = current_ticks;
#endif

	/* feed the specified channel */
	channels[channel_id].timeout_abs_ticks = current_ticks +
		k_ms_to_ticks_ceil64(channels[channel_id].reload_period);

#ifdef CONFIG_TASK_WDT_THREAD_INFO
	/* the owner is the thread which fed the channel last */
	if (!channels[channel_id].thread_fixed && !k_is_in_isr()) {
		channels[channel_id].thread = k_current_get();
	}
#endif

	schedule_next_timeout(current_ticks);

	k_sched_unlock();

	if (too_early) {
		task_wdt_fault(channel_id, TASK_WDT_FAULT_TOO_EARLY);
	}

	return 0;
}

int task_wdt_window_set(int channel_id, uint32_t min_period)
{
#ifdef CONFIG_TASK_WDT_FAULT_REASONS
	k_spinlock_key_t key;
	int ret = 0;

	if (channel_id < 0 || channel_id >= ARRAY_SIZE(channels)) {
		return -EINVAL;
	}

	key = k_spin_lock(&channels_lock);

	if (channels[channel_id].reload_period == 0 ||
	    min_period >= channels[channel_id].reload_period) {
		ret = -EINVAL;
	} else {
		channels[channel_id].min_period = min_period;
	}

	k_spin_unlock(&channels_lock, key);

	return ret;
#else
	ARG_UNUSED(channel_id);
	ARG_UNUSED(min_period);

	return -ENOTSUP;
#endif
}

int task_wdt_channel_thread_set(int channel_id, k_tid_t thread)
{
#ifdef CONFIG_TASK_WDT_THREAD_INFO
	k_spinlock_key_t key;
	int ret = 0;

	if (channel_id < 0 || channel_id >= ARRAY_SIZE(channels)) {
		return -EINVAL;
	}

	key = k_spin_lock(&channels_lock);

	if (channels[channel_id].reload_period == 0) {
		ret = -EINVAL;
	} else {
		channels[channel_id].thread = thread;
		channels[channel_id].thread_fixed = (thread != NULL);
	}

	k_spin_unlock(&channels_lock, key);

	return ret;
#else
	ARG_UNUSED(channel_id);
	ARG_UNUSED(thread);

	return -ENOTSUP;
#endif
}

int task_wdt_channel_info_get(int channel_id, struct task_wdt_channel_info *info)
{
	k_spinlock_key_t key;
	int64_t ticks_left;
	int ret = 0;

	if (channel_id < 0 || channel_id >= ARRAY_SIZE(channels) || info == NULL) {
		return -EINVAL;
	}

	key = k_spin_lock(&channels_lock);

	if (channels[channel_id].reload_period == 0) {
		ret = -EINVAL;
	} else {
		ticks_left = channels[channel_id].timeout_abs_ticks - sys_clock_tick_get();

		info->reload_period = channels[channel_id].reload_period;
		info->time_left = ticks_left > 0 ? (uint32_t)k_ticks_to_ms_floor64(ticks_left) : 0;
#ifdef CONFIG_TASK_WDT_THREAD_INFO
		info->thread = channels[channel_id].thread;
#else
		info->thread = NULL;
#endif
	}

	k_spin_unlock(&channels_lock, key);

	return ret;
}

#ifndef CONFIG_TASK_WDT_RESET_INFO
int task_wdt_reset_info_get(struct task_wdt_reset_info *info)
{
	ARG_UNUSED(info);

	return -ENOTSUP;
}

int task_wdt_reset_info_clear(void)
{
	return -ENOTSUP;
}
#endif

void task_wdt_suspend(void)
{
	k_spinlock_key_t key;

	/*
	 * Allow the function to be called from a custom PM policy callback, even when
	 * the Task Watchdog was not initialized yet.
	 */
	if (!task_wdt_initialized) {
		return;
	}

	/*
	 * Prevent all task watchdog channels from triggering.
	 * Protect the timer access with the spinlock to avoid the timer being started
	 * concurrently by a call to schedule_next_timeout().
	 */
	key = k_spin_lock(&channels_lock);
	k_timer_stop(&timer);
	k_spin_unlock(&channels_lock, key);

#ifdef CONFIG_TASK_WDT_HW_FALLBACK
	/*
	 * Give a whole hardware watchdog timer period of time to the application to put
	 * the system in a suspend mode that will pause the hardware watchdog.
	 */
	if (hw_wdt_started) {
		wdt_feed(hw_wdt_dev, hw_wdt_channel);
	}
#endif
}

void task_wdt_resume(void)
{
	k_spinlock_key_t key;
	int64_t current_ticks;

	/*
	 * Allow the function to be called from a custom PM policy callback, even when
	 * the Task Watchdog was not initialized yet.
	 */
	if (!task_wdt_initialized) {
		return;
	}

	key = k_spin_lock(&channels_lock);

	/*
	 * Feed all enabled channels, so the application threads have time to resume
	 * feeding the channels by themselves.
	 */
	current_ticks = sys_clock_tick_get();
	for (size_t id = 0; id < ARRAY_SIZE(channels); id++) {
		if (channels[id].reload_period != 0) {
			channels[id].timeout_abs_ticks = current_ticks +
				k_ms_to_ticks_ceil64(channels[id].reload_period);
		}
	}

	/* Restart the Task Watchdog timer */
	schedule_next_timeout(current_ticks);

	k_spin_unlock(&channels_lock, key);
}

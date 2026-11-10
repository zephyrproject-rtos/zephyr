/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME net_openthread_power
#define LOG_LEVEL       CONFIG_OPENTHREAD_PLATFORM_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr/kernel.h>
#include <zephyr/pm/policy.h>
#include <zephyr/pm/state.h>

#include <openthread/platform/alarm-micro.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/tasklet.h>

#include "platform-zephyr.h"
#include "power.h"

#define PLATFORM_POWER_MAX_LOCKED_STATES 8

struct platform_power_state {
	bool initialized;
	bool deep_sleep_locked;
	bool milli_armed;
	bool milli_fired;
	bool micro_armed;
	bool micro_fired;
	uint32_t next_wake_ms;
	uint32_t next_wake_us;
	uint32_t pending_signals;
	uint32_t processed_signals;
	uint32_t processing_signals;
};

struct platform_power_locked_state {
	enum pm_state state;
	uint8_t substate_id;
};

static struct platform_power_state power_state;
static struct k_spinlock power_lock;
static struct platform_power_locked_state locked_states[PLATFORM_POWER_MAX_LOCKED_STATES];
static uint8_t locked_state_count;

static void platform_power_cache_locked_states(void)
{
	const struct pm_state_info *states;
	uint8_t num_states = pm_state_cpu_get_all(0U, &states);

	locked_state_count = 0U;

	for (uint8_t i = 0U; i < num_states; i++) {
		if (states[i].state <= PM_STATE_RUNTIME_IDLE) {
			continue;
		}

		if (locked_state_count >= PLATFORM_POWER_MAX_LOCKED_STATES) {
			LOG_WRN("Too many deep PM states to lock; truncating");
			break;
		}

		locked_states[locked_state_count].state = states[i].state;
		locked_states[locked_state_count].substate_id = states[i].substate_id;
		locked_state_count++;
	}
}

static bool alarm_is_imminent_32(bool armed, uint32_t now, uint32_t deadline, uint32_t threshold)
{
	if (!armed) {
		return false;
	}

	return (int32_t)(deadline - now) < (int32_t)threshold;
}

static bool platform_power_platform_event_pending(void)
{
	if (power_state.milli_fired || power_state.micro_fired || platformRadioIsPending()) {
		return true;
	}

	if (IS_ENABLED(CONFIG_OPENTHREAD_COPROCESSOR) && platformUartIsPending()) {
		return true;
	}

	return false;
}

static bool platform_power_driver_pass_pending(void)
{
	return power_state.pending_signals != power_state.processed_signals ||
	       platform_power_platform_event_pending();
}

static bool platform_power_has_pending_events_locked(otInstance *instance)
{
	return platform_power_driver_pass_pending() ||
	       (instance != NULL && otTaskletsArePending(instance));
}

bool platformPowerHasPendingEvents(otInstance *instance)
{
	k_spinlock_key_t key;
	bool pending;

	if (!power_state.initialized) {
		return false;
	}

	key = k_spin_lock(&power_lock);
	pending = platform_power_has_pending_events_locked(instance);
	k_spin_unlock(&power_lock, key);

	return pending;
}

static bool platform_power_should_lock_deep_sleep(otInstance *instance)
{
	const uint32_t min_sleep_ms = CONFIG_OPENTHREAD_PLATFORM_DEEP_SLEEP_MIN_MS;
	const uint32_t min_sleep_us = min_sleep_ms * USEC_PER_MSEC;

	if (platform_power_has_pending_events_locked(instance)) {
		return true;
	}

	if (alarm_is_imminent_32(power_state.milli_armed, otPlatAlarmMilliGetNow(),
				 power_state.next_wake_ms, min_sleep_ms)) {
		return true;
	}

	if (IS_ENABLED(CONFIG_OPENTHREAD_PLATFORM_USEC_TIMER) &&
	    alarm_is_imminent_32(power_state.micro_armed, otPlatAlarmMicroGetNow(),
				 power_state.next_wake_us, min_sleep_us)) {
		return true;
	}

	return false;
}

static void platform_power_set_deep_sleep_lock(bool lock)
{
	if (lock == power_state.deep_sleep_locked) {
		return;
	}

	for (uint8_t i = 0U; i < locked_state_count; i++) {
		if (lock) {
			pm_policy_state_lock_get(locked_states[i].state,
						 locked_states[i].substate_id);
		} else {
			pm_policy_state_lock_put(locked_states[i].state,
						 locked_states[i].substate_id);
		}
	}

	power_state.deep_sleep_locked = lock;
}

void platformPowerInit(void)
{
	k_spinlock_key_t key;

	platform_power_cache_locked_states();

	key = k_spin_lock(&power_lock);

	power_state = (struct platform_power_state){
		.initialized = true,
		/*
		 * OpenThread startup submits initial work outside of
		 * otSysEventSignalPending(). Start conservatively and release
		 * the deep-sleep lock after the first process pass.
		 */
		.pending_signals = 1U,
	};
	platform_power_set_deep_sleep_lock(platform_power_should_lock_deep_sleep(NULL));

	k_spin_unlock(&power_lock, key);
}

void platformPowerProcessBegin(void)
{
	k_spinlock_key_t key;

	if (!power_state.initialized) {
		return;
	}

	key = k_spin_lock(&power_lock);
	power_state.processing_signals = power_state.pending_signals;
	k_spin_unlock(&power_lock, key);
}

void platformPowerProcess(otInstance *instance)
{
	k_spinlock_key_t key;

	if (!power_state.initialized || instance == NULL) {
		return;
	}

	key = k_spin_lock(&power_lock);

	if (power_state.pending_signals == power_state.processing_signals) {
		power_state.processed_signals = power_state.pending_signals;
		power_state.milli_fired = false;
		power_state.micro_fired = false;
	}

	platform_power_set_deep_sleep_lock(platform_power_should_lock_deep_sleep(instance));
	k_spin_unlock(&power_lock, key);
}

void platformPowerSignalPending(void)
{
	k_spinlock_key_t key;

	if (!power_state.initialized) {
		return;
	}

	key = k_spin_lock(&power_lock);
	power_state.pending_signals++;
	platform_power_set_deep_sleep_lock(platform_power_should_lock_deep_sleep(NULL));
	k_spin_unlock(&power_lock, key);
}

void platformPowerNotifyAlarmMilliStart(uint32_t fire_at_ms)
{
	k_spinlock_key_t key = k_spin_lock(&power_lock);

	power_state.milli_armed = true;
	power_state.milli_fired = false;
	power_state.next_wake_ms = fire_at_ms;
	platform_power_set_deep_sleep_lock(platform_power_should_lock_deep_sleep(NULL));

	k_spin_unlock(&power_lock, key);
}

void platformPowerNotifyAlarmMilliStop(void)
{
	k_spinlock_key_t key = k_spin_lock(&power_lock);

	power_state.milli_armed = false;
	power_state.milli_fired = false;
	platform_power_set_deep_sleep_lock(platform_power_should_lock_deep_sleep(NULL));

	k_spin_unlock(&power_lock, key);
}

void platformPowerNotifyAlarmMilliFired(void)
{
	k_spinlock_key_t key = k_spin_lock(&power_lock);

	power_state.milli_armed = false;
	power_state.milli_fired = true;
	platform_power_set_deep_sleep_lock(platform_power_should_lock_deep_sleep(NULL));

	k_spin_unlock(&power_lock, key);
}

void platformPowerNotifyAlarmMicroStart(uint32_t fire_at_us)
{
	k_spinlock_key_t key = k_spin_lock(&power_lock);

	power_state.micro_armed = true;
	power_state.micro_fired = false;
	power_state.next_wake_us = fire_at_us;
	platform_power_set_deep_sleep_lock(platform_power_should_lock_deep_sleep(NULL));

	k_spin_unlock(&power_lock, key);
}

void platformPowerNotifyAlarmMicroStop(void)
{
	k_spinlock_key_t key = k_spin_lock(&power_lock);

	power_state.micro_armed = false;
	power_state.micro_fired = false;
	platform_power_set_deep_sleep_lock(platform_power_should_lock_deep_sleep(NULL));

	k_spin_unlock(&power_lock, key);
}

void platformPowerNotifyAlarmMicroFired(void)
{
	k_spinlock_key_t key = k_spin_lock(&power_lock);

	power_state.micro_armed = false;
	power_state.micro_fired = true;
	platform_power_set_deep_sleep_lock(platform_power_should_lock_deep_sleep(NULL));

	k_spin_unlock(&power_lock, key);
}

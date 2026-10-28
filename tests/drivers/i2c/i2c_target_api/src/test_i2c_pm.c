/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c/target/eeprom.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>

#if defined(CONFIG_SMP)
#include <zephyr/arch/cpu.h>
#endif

#define NODE_EP0 DT_NODELABEL(eeprom0)
#define NODE_EP1 DT_NODELABEL(eeprom1)

#define I2C_PM_LEN 16
#define I2C_PM_MIN_IDLE_MS 100
#define I2C_PM_IDLE_MARGIN_MS 10

static atomic_t i2c_pm_entries[PM_STATE_COUNT];
static bool i2c_pm_registered;
static bool i2c_pm_notifier_on;

static void count_state_entry(enum pm_state state)
{
	if (state < PM_STATE_COUNT) {
		atomic_inc(&i2c_pm_entries[state]);
	}
}

static struct pm_notifier i2c_pm_notifier = {
	.state_entry = count_state_entry,
};

static void reset_state_counts(void)
{
	for (int state = 0; state < PM_STATE_COUNT; state++) {
		atomic_clear(&i2c_pm_entries[state]);
	}
}

static const char *get_state_name(enum pm_state state)
{
	switch (state) {
	case PM_STATE_ACTIVE:
		return "PM_STATE_ACTIVE";
	case PM_STATE_RUNTIME_IDLE:
		return "PM_STATE_RUNTIME_IDLE";
	case PM_STATE_SUSPEND_TO_IDLE:
		return "PM_STATE_SUSPEND_TO_IDLE";
	case PM_STATE_STANDBY:
		return "PM_STATE_STANDBY";
	case PM_STATE_SUSPEND_TO_RAM:
		return "PM_STATE_SUSPEND_TO_RAM";
	case PM_STATE_SUSPEND_TO_DISK:
		return "PM_STATE_SUSPEND_TO_DISK";
	case PM_STATE_SOFT_OFF:
		return "PM_STATE_SOFT_OFF";
	default:
		return "unknown";
	}
}

static bool report_state_counts(void)
{
	bool entered = false;

	for (int state = 0; state < PM_STATE_COUNT; state++) {
		atomic_val_t count = atomic_get(&i2c_pm_entries[state]);

		if (count > 0) {
			printk("entered %s, count %ld\n",
			       get_state_name((enum pm_state)state), (long)count);
			entered = true;
		}
	}

	return entered;
}

static uint8_t get_cpu_id(void)
{
#if defined(CONFIG_SMP)
	return arch_curr_cpu()->id;
#else
	return 0U;
#endif
}

static uint32_t get_sleep_time_ms(const struct pm_state_info *state)
{
	uint64_t need_us = (uint64_t)state->min_residency_us + state->exit_latency_us;
	uint32_t need_ms = (uint32_t)DIV_ROUND_UP(need_us, 1000U);

	if (need_ms < I2C_PM_MIN_IDLE_MS) {
		return I2C_PM_MIN_IDLE_MS;
	}

	return need_ms + I2C_PM_IDLE_MARGIN_MS;
}

static void lock_other_states(const struct pm_state_info *states, uint8_t count,
			      const struct pm_state_info *keep, bool lock)
{
	for (uint8_t i = 0; i < count; i++) {
		if (&states[i] == keep) {
			continue;
		}

		if (lock) {
			pm_policy_state_lock_get(states[i].state, states[i].substate_id);
		} else {
			pm_policy_state_lock_put(states[i].state, states[i].substate_id);
		}
	}
}

static void check_transfer(const struct device *i2c, uint16_t addr)
{
	uint8_t tx[1 + I2C_PM_LEN];
	uint8_t rx[I2C_PM_LEN];
	int ret;

	tx[0] = 0;
	for (int i = 0; i < I2C_PM_LEN; i++) {
		tx[i + 1] = (uint8_t)i;
	}

	memset(rx, 0, sizeof(rx));

	ret = i2c_write(i2c, tx, sizeof(tx), addr);
	zassert_ok(ret, "I2C write failed, code %d", ret);

	ret = i2c_write_read(i2c, addr, tx, 1, rx, sizeof(rx));
	zassert_ok(ret, "I2C read failed, code %d", ret);

	zassert_equal(memcmp(&tx[1], rx, sizeof(rx)), 0, "RX buffer does not match TX");
}

/* The target holds a PM lock while it is registered, so the idle runs detached.
 * The second pass repeats that after a finished transfer.
 */
static void check_target_after_idle(const struct pm_state_info *state, uint32_t idle_ms)
{
	const struct device *const eeprom = DEVICE_DT_GET(NODE_EP0);
	const struct device *const i2c = DEVICE_DT_GET(DT_BUS(NODE_EP1));
	uint16_t addr = DT_REG_ADDR(NODE_EP0);

	for (int cycle = 0; cycle < 2; cycle++) {
		int ret;

		reset_state_counts();
		k_msleep(idle_ms);
		(void)report_state_counts();
		zassert_true(atomic_get(&i2c_pm_entries[state->state]) > 0,
			     "power state %s was not entered", get_state_name(state->state));

		ret = i2c_target_driver_register(eeprom);
		zassert_ok(ret, "Failed to register EEPROM target, code %d", ret);
		i2c_pm_registered = true;

		check_transfer(i2c, addr);

		ret = i2c_target_driver_unregister(eeprom);
		i2c_pm_registered = false;
		zassert_ok(ret, "Failed to unregister EEPROM target, code %d", ret);
	}
}

static void run_in_each_state(void (*fn)(const struct pm_state_info *state, uint32_t idle_ms))
{
	const struct pm_state_info *states;
	uint8_t count = pm_state_cpu_get_all(get_cpu_id(), &states);
	bool tested = false;

	for (uint8_t i = 0; i < count; i++) {
		const struct pm_state_info *state = &states[i];
		uint32_t sleep_ms;

		if (state->state == PM_STATE_SOFT_OFF) {
			continue;
		}

		sleep_ms = get_sleep_time_ms(state);
		tested = true;
		if (state->substate_id == 0U) {
			printk("testing %s for %u ms\n", get_state_name(state->state), sleep_ms);
		} else {
			printk("testing %s substate %u for %u ms\n", get_state_name(state->state),
			       state->substate_id, sleep_ms);
		}

		lock_other_states(states, count, state, true);
		fn(state, sleep_ms);
		lock_other_states(states, count, state, false);
	}

	zassert_true(tested, "no idle power state is enabled");
}

static void i2c_pm_teardown(void *fixture)
{
	const struct device *const eeprom = DEVICE_DT_GET(NODE_EP0);

	ARG_UNUSED(fixture);

	if (i2c_pm_registered) {
		int ret;

		ret = i2c_target_driver_unregister(eeprom);
		i2c_pm_registered = false;
		zassert_ok(ret, "Failed to unregister EEPROM target, code %d", ret);
	}

	if (i2c_pm_notifier_on) {
		int ret;

		ret = pm_notifier_unregister(&i2c_pm_notifier);
		i2c_pm_notifier_on = false;
		zassert_ok(ret, "Failed to unregister PM notifier, code %d", ret);
	}
}

ZTEST(i2c_pm, test_i2c)
{
	const struct device *const eeprom = DEVICE_DT_GET(NODE_EP0);
	const struct device *const i2c = DEVICE_DT_GET(DT_BUS(NODE_EP1));

	zassert_equal(DT_PROP_OR(NODE_EP0, address_width, 8), 8, "EEPROM address width is not 8");
	zassert_true(device_is_ready(eeprom), "EEPROM target is not ready");
	zassert_true(device_is_ready(i2c), "I2C controller is not ready");

	pm_notifier_register(&i2c_pm_notifier);
	i2c_pm_notifier_on = true;

	run_in_each_state(check_target_after_idle);
}

ZTEST_SUITE(i2c_pm, NULL, NULL, NULL, i2c_pm_teardown, NULL);

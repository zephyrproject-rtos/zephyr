/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TEST_PWM_PM_H__
#define __TEST_PWM_PM_H__

#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/state.h>
#include <zephyr/ztest.h>

#define PWM_PM_NODE DT_INST(0, test_pwm_loopback)

/* pwms[0] generates the waveform, pwms[1] captures it */
#define PWM_PM_OUT_CTLR DT_PWMS_CTLR_BY_IDX(PWM_PM_NODE, 0)
#define PWM_PM_IN_CTLR  DT_PWMS_CTLR_BY_IDX(PWM_PM_NODE, 1)

/*
 * The states a node lists in zephyr,disabling-power-states are the ones that
 * stop it, and so the ones its driver locks while it is in use.
 */
#define DISABLING_STATE(idx, node_id)                                                    \
	PM_STATE_CONSTRAINT_INIT(DT_PHANDLE_BY_IDX(node_id, zephyr_disabling_power_states, idx))

#define DISABLING_STATES(node_id)                                                        \
	{LISTIFY(DT_PROP_LEN_OR(node_id, zephyr_disabling_power_states, 0), DISABLING_STATE, \
		 (,), node_id)}

#define HAS_DISABLING_STATES(node_id) DT_NODE_HAS_PROP(node_id, zephyr_disabling_power_states)

/* What the capture side read off the line. A field the capture device cannot
 * measure stays at zero and is not compared.
 */
struct pwm_pm_reading {
	uint32_t period;
	uint32_t pulse;
};

extern const struct pwm_dt_spec pwm_pm_out;
extern const struct pwm_dt_spec pwm_pm_in;

/* Start a 10 ms, 25% waveform on the output. */
void pwm_pm_start_output(void);

/* Capture period and pulse; fails the test if the capture side sees nothing. */
void pwm_pm_capture(struct pwm_pm_reading *reading);

/* Run a PM action and check the state it should leave the device in. */
void pwm_pm_action(const struct device *dev, enum pm_device_action action);

/* Suite predicate: both devices ready and the output one has device PM. */
bool pwm_pm_supported(const void *global_state);

/* Suite after hook: bring both devices back to ACTIVE, capture disabled. */
void pwm_pm_after(void *fixture);

#endif /* __TEST_PWM_PM_H__ */

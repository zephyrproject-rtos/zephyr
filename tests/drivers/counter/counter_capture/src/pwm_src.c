/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * PWM trigger-source backend: the capture input is driven by a PWM channel
 * routed to the same pad it captures on. The PWM is set to a static duty
 * (fully low or fully high) so each level change produces a single, clean,
 * peripheral-generated edge. The board loops the PWM output back to the
 * capture input, so no external wire is needed.
 *
 * Any board can use this backend by declaring a "pwms" property on the
 * "/zephyr,user" node, one entry per capture channel.
 */

#include <zephyr/drivers/pwm.h>
#include <zephyr/sys/util.h>
#include <zephyr/devicetree.h>

#include "trigger_src.h"

#define CAPTURE_COUNT DT_PROP_LEN(DT_NODELABEL(counter_loopback_0), test_counter_captures)

/* Arbitrary period; only fully-off (0) and fully-on (= period) duty are used. */
#define TRIGGER_PWM_PERIOD PWM_USEC(100)

#define PWM_SPEC_BY_IDX(node_id, prop, idx) PWM_DT_SPEC_GET_BY_IDX(node_id, idx)

/* clang-format off */
static const struct pwm_dt_spec capture_tester_pwms[] = {
	DT_FOREACH_PROP_ELEM_SEP(DT_PATH(zephyr_user), pwms, PWM_SPEC_BY_IDX, (,))
};
/* clang-format on */

BUILD_ASSERT(ARRAY_SIZE(capture_tester_pwms) == CAPTURE_COUNT,
	     "capture_tester_pwms and test-counter-captures size mismatch");

/* PWM has no readback, so the last driven level is cached per channel. */
static int driven_level[CAPTURE_COUNT];

int trigger_src_setup(size_t idx)
{
	if (!pwm_is_ready_dt(&capture_tester_pwms[idx])) {
		return -ENODEV;
	}

	driven_level[idx] = 0;

	return pwm_set_dt(&capture_tester_pwms[idx], TRIGGER_PWM_PERIOD, 0);
}

int trigger_src_get(size_t idx)
{
	return driven_level[idx];
}

int trigger_src_set(size_t idx, int level)
{
	int ret;

	ret = pwm_set_dt(&capture_tester_pwms[idx], TRIGGER_PWM_PERIOD,
			 level ? TRIGGER_PWM_PERIOD : 0);
	if (ret != 0) {
		return ret;
	}

	driven_level[idx] = level ? 1 : 0;

	return 0;
}

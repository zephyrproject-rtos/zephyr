/*
 * Copyright (c) 2020-2021 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TEST_PWM_LOOPBACK_H__
#define __TEST_PWM_LOOPBACK_H__

#include <zephyr.h>
#include <drivers/pwm.h>
#include <ztest.h>

#define PWM_LOOPBACK_OUT_IDX 0
#define PWM_LOOPBACK_IN_IDX  1

#define PWM_LOOPBACK_NODE DT_INST(0, test_pwm_loopback)

#define PWM_LOOPBACK_OUT_LABEL \
	DT_PWMS_LABEL_BY_IDX(PWM_LOOPBACK_NODE, PWM_LOOPBACK_OUT_IDX)
#define PWM_LOOPBACK_OUT_CHANNEL \
	DT_PWMS_CHANNEL_BY_IDX(PWM_LOOPBACK_NODE, PWM_LOOPBACK_OUT_IDX)
#define PWM_LOOPBACK_OUT_FLAGS \
	DT_PWMS_FLAGS_BY_IDX(PWM_LOOPBACK_NODE, PWM_LOOPBACK_OUT_IDX)

#define PWM_LOOPBACK_IN_LABEL \
	DT_PWMS_LABEL_BY_IDX(PWM_LOOPBACK_NODE, PWM_LOOPBACK_IN_IDX)
#define PWM_LOOPBACK_IN_CHANNEL \
	DT_PWMS_CHANNEL_BY_IDX(PWM_LOOPBACK_NODE, PWM_LOOPBACK_IN_IDX)
#define PWM_LOOPBACK_IN_FLAGS \
	DT_PWMS_FLAGS_BY_IDX(PWM_LOOPBACK_NODE, PWM_LOOPBACK_IN_IDX)

struct test_pwm {
	const struct device *dev;
	uint32_t pwm;
	pwm_flags_t flags;
};

struct test_pwm_callback_data {
	uint32_t *buffer;
	size_t buffer_len;
	size_t count;
	int status;
	struct k_sem sem;
	bool pulse_capture;
};

void get_test_pwms(struct test_pwm *out, struct test_pwm *in);

void test_pulse_capture(void);

void test_pulse_capture_inverted(void);

void test_period_capture(void);

void test_period_capture_inverted(void);

void test_pulse_and_period_capture(void);

void test_capture_timeout(void);

void test_continuous_capture(void);

void test_capture_busy(void);

#endif /* __TEST_PWM_LOOPBACK_H__ */

/*
 * Copyright 2024 Navimatix GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/drivers/gpio.h"
#include "zephyr/kernel.h"
#include "zephyr/ztest_assert.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/stepper.h>

#define ACCELERATION 50
#define LOW_SPD_I    40000000
#define MED_SPD_I    20000000
#define HIGH_SPD_I   10000000

struct stepper_acceleration_fixture {
	const struct device *dev;
	stepper_event_callback_t callback;
};

struct k_poll_signal stepper_signal;
struct k_poll_event stepper_event;

#define POLL_AND_CHECK_SIGNAL(signal, event, expected_event, timeout)                              \
	({                                                                                         \
		(void)k_poll(&(event), 1, timeout);                                                \
		unsigned int signaled;                                                             \
		int result;                                                                        \
		k_poll_signal_check(&(signal), &signaled, &result);                                \
		zassert_equal(signaled, 1, "Signal not set");                                      \
		zassert_equal(result, (expected_event), "Expected Event %d but got %d",            \
			      (expected_event), result);                                           \
	})

static void stepper_acceleration_print_event_callback(const struct device *dev,
						      enum stepper_event event, void *dummy)
{
	switch (event) {
	case STEPPER_EVENT_STEPS_COMPLETED:
		k_poll_signal_raise(&stepper_signal, STEPPER_EVENT_STEPS_COMPLETED);
		break;
	case STEPPER_EVENT_LEFT_END_STOP_DETECTED:
		k_poll_signal_raise(&stepper_signal, STEPPER_EVENT_LEFT_END_STOP_DETECTED);
		break;
	case STEPPER_EVENT_RIGHT_END_STOP_DETECTED:
		k_poll_signal_raise(&stepper_signal, STEPPER_EVENT_RIGHT_END_STOP_DETECTED);
		break;
	case STEPPER_EVENT_STALL_DETECTED:
		k_poll_signal_raise(&stepper_signal, STEPPER_EVENT_STALL_DETECTED);
		break;
	case STEPPER_EVENT_STOPPED:
		k_poll_signal_raise(&stepper_signal, STEPPER_EVENT_STOPPED);
		break;
	default:
		break;
	}
}

static void *stepper_acceleration_setup(void)
{
	static struct stepper_acceleration_fixture fixture = {
		.dev = DEVICE_DT_GET(DT_NODELABEL(stepper_motor)),
		.callback = stepper_acceleration_print_event_callback,
	};

	k_poll_signal_init(&stepper_signal);
	k_poll_event_init(&stepper_event, K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY,
			  &stepper_signal);

	zassert_not_null(fixture.dev);
	return &fixture;
}

static void stepper_acceleration_before(void *f)
{
	struct stepper_acceleration_fixture *fixture = f;
	(void)stepper_disable(fixture->dev);
	(void)stepper_enable(fixture->dev);
	(void)stepper_set_reference_position(fixture->dev, 0);
	k_poll_signal_reset(&stepper_signal);
}

static void stepper_acceleration_after(void *f)
{
	struct stepper_acceleration_fixture *fixture = f;
	(void)stepper_disable(fixture->dev);
	(void)stepper_enable(fixture->dev);
}

#define TEST_MOVE_BY_DIFFERENT_SPEEDS(fixture, velocity_start, velocity_test, steps, pos_target,   \
				      test_time_us)                                                \
	({                                                                                         \
		int direction = STEPPER_DIRECTION_POSITIVE;                                        \
		int32_t pos;                                                                       \
		uint32_t test_time;                                                                \
		uint64_t interval_start = 0;                                                       \
		uint64_t interval_test = 0;                                                        \
		volatile uint32_t temp_start_velocity = velocity_start;                            \
		volatile uint32_t temp_test_velocity = velocity_test;                              \
                                                                                                   \
		if (steps < 0) {                                                                   \
			direction = STEPPER_DIRECTION_NEGATIVE;                                    \
		}                                                                                  \
                                                                                                   \
		if (velocity_start != 0) {                                                         \
			interval_start = NSEC_PER_SEC / temp_start_velocity;                       \
		}                                                                                  \
                                                                                                   \
		if (velocity_test != 0) {                                                          \
			interval_test = NSEC_PER_SEC / temp_test_velocity;                         \
		}                                                                                  \
                                                                                                   \
		/* Add one interval time of both speeds to test_time to catch delays and */        \
		/* algorithm inaccuracies */                                                       \
		test_time = test_time_us + (interval_start / NSEC_PER_USEC) +                      \
			    (interval_test / NSEC_PER_USEC);                                       \
                                                                                                   \
		(void)stepper_enable(fixture->dev);                                                \
		(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);           \
		if (interval_start != 0) {                                                         \
			(void)stepper_set_microstep_interval(fixture->dev, interval_start);        \
			(void)stepper_run(fixture->dev, direction);                                \
		}                                                                                  \
		/* Acceleration time + 1/10 second + first step delay*/                            \
		k_usleep(1000000 * (uint32_t)ceil(velocity_start * 1.0 / ACCELERATION) + 100000 +  \
			 interval_start / NSEC_PER_USEC);                                          \
		(void)stepper_set_microstep_interval(fixture->dev, interval_test);                 \
		(void)stepper_move_by(fixture->dev, steps);                                        \
		POLL_AND_CHECK_SIGNAL(stepper_signal, stepper_event,                               \
				      STEPPER_EVENT_STEPS_COMPLETED, K_USEC(test_time));           \
		(void)stepper_get_actual_position(fixture->dev, &pos);                             \
                                                                                                   \
		zassert_true(IN_RANGE(pos, pos_target - 1, pos_target + 1),                        \
			     "Current position should be between %d and %d but is %d",             \
			     pos_target - 1, pos_target + 1, pos);                                 \
	})

#define TEST_MOVE_TO_DIFFERENT_SPEEDS(fixture, velocity_start, velocity_test, pos_target,          \
				      test_time_us)                                                \
	({                                                                                         \
		int direction = STEPPER_DIRECTION_POSITIVE;                                        \
		int32_t pos;                                                                       \
		uint32_t test_time;                                                                \
		uint64_t interval_start = 0;                                                       \
		uint64_t interval_test = 0;                                                        \
		volatile uint32_t temp_start_velocity = velocity_start;                            \
		volatile uint32_t temp_test_velocity = velocity_test;                              \
                                                                                                   \
		if (pos_target < 0) {                                                              \
			direction = STEPPER_DIRECTION_NEGATIVE;                                    \
		}                                                                                  \
                                                                                                   \
		if (velocity_start != 0) {                                                         \
			interval_start = NSEC_PER_SEC / temp_start_velocity;                       \
		}                                                                                  \
                                                                                                   \
		if (velocity_test != 0) {                                                          \
			interval_test = NSEC_PER_SEC / temp_test_velocity;                         \
		}                                                                                  \
                                                                                                   \
		/* Add one interval time of both speeds to test_time to catch delays and */        \
		/* algorithm inaccuracies */                                                       \
		test_time = test_time_us + (interval_start / NSEC_PER_USEC) +                      \
			    (interval_test / NSEC_PER_USEC);                                       \
                                                                                                   \
		(void)stepper_enable(fixture->dev);                                                \
		(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);           \
		if (interval_start != 0) {                                                         \
			(void)stepper_set_microstep_interval(fixture->dev, interval_start);        \
			(void)stepper_run(fixture->dev, direction);                                \
		}                                                                                  \
		/* Acceleration time + 1/10 second + first step delay*/                            \
		k_usleep(1000000 * (uint32_t)ceil(velocity_start * 1.0 / ACCELERATION) + 100000 +  \
			 interval_start / NSEC_PER_USEC);                                          \
		(void)stepper_set_microstep_interval(fixture->dev, interval_test);                 \
		(void)stepper_move_to(fixture->dev, pos_target);                                   \
		POLL_AND_CHECK_SIGNAL(stepper_signal, stepper_event,                               \
				      STEPPER_EVENT_STEPS_COMPLETED, K_USEC(test_time));           \
		(void)stepper_get_actual_position(fixture->dev, &pos);                             \
		zassert_equal(pos, pos_target, "Target position should be %d but is %d",           \
			      pos_target, pos);                                                    \
	})

#define TEST_RUN_DIFFERENT_SPEEDS(fixture, velocity_start, velocity_test, pos_target, t_start,     \
				  t_test, direction)                                               \
	({                                                                                         \
		int32_t pos;                                                                       \
		uint64_t interval_start = 0;                                                       \
		uint64_t interval_test = 0;                                                        \
		volatile uint32_t temp_start_velocity = velocity_start;                            \
		volatile uint32_t temp_test_velocity = velocity_test;                              \
                                                                                                   \
		if (velocity_start != 0) {                                                         \
			interval_start = NSEC_PER_SEC / temp_start_velocity;                       \
		}                                                                                  \
                                                                                                   \
		if (velocity_test != 0) {                                                          \
			interval_test = NSEC_PER_SEC / temp_test_velocity;                         \
		}                                                                                  \
                                                                                                   \
		(void)stepper_enable(fixture->dev);                                                \
		(void)stepper_set_microstep_interval(fixture->dev, interval_start);                \
		(void)stepper_run(fixture->dev, direction);                                        \
		k_usleep(t_start);                                                                 \
		if (interval_test != 0) {                                                          \
			(void)stepper_set_microstep_interval(fixture->dev, interval_test);         \
			(void)stepper_run(fixture->dev, direction);                                \
		} else {                                                                           \
			(void)stepper_stop(fixture->dev);                                          \
		}                                                                                  \
                                                                                                   \
		k_usleep(t_test);                                                                  \
                                                                                                   \
		(void)stepper_get_actual_position(fixture->dev, &pos);                             \
		zassert_true(IN_RANGE(pos, pos_target - 2, pos_target + 2),                        \
			     "Current position should be between %d and %d but is %d",             \
			     pos_target - 2, pos_target + 2, pos);                                 \
	})

ZTEST_SUITE(stepper_acceleration, NULL, stepper_acceleration_setup, stepper_acceleration_before,
	    stepper_acceleration_after, NULL);

ZTEST_F(stepper_acceleration, test_run_positive_direction_correct_position_from_zero_speed)
{
	TEST_RUN_DIFFERENT_SPEEDS(fixture, 0, 50, 30, 0, 1100000, STEPPER_DIRECTION_POSITIVE);
}

ZTEST_F(stepper_acceleration, test_run_negative_direction_correct_position_from_zero_speed)
{
	TEST_RUN_DIFFERENT_SPEEDS(fixture, 0, 50, -30, 0, 1100000, STEPPER_DIRECTION_NEGATIVE);
}

ZTEST_F(stepper_acceleration, test_run_positive_direction_correct_position_stopping)
{
	TEST_RUN_DIFFERENT_SPEEDS(fixture, 50, 0, 50, 1000000, 1100000, STEPPER_DIRECTION_POSITIVE);
}

ZTEST_F(stepper_acceleration, test_run_negative_direction_correct_position_stopping)
{
	TEST_RUN_DIFFERENT_SPEEDS(fixture, 50, 0, -50, 1000000, 1100000,
				  STEPPER_DIRECTION_NEGATIVE);
}

ZTEST_F(stepper_acceleration, test_run_positive_direction_stopping_signals)
{
	(void)stepper_enable(fixture->dev);
	(void)stepper_set_microstep_interval(fixture->dev, MED_SPD_I);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);
	(void)stepper_run(fixture->dev, STEPPER_DIRECTION_POSITIVE);
	k_usleep(1100000);
	(void)stepper_stop(fixture->dev);
	POLL_AND_CHECK_SIGNAL(stepper_signal, stepper_event, STEPPER_EVENT_STOPPED, K_MSEC(1040));
}

ZTEST_F(stepper_acceleration, test_run_negative_direction_stopping_signals)
{
	(void)stepper_enable(fixture->dev);
	(void)stepper_set_microstep_interval(fixture->dev, MED_SPD_I);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);
	(void)stepper_run(fixture->dev, STEPPER_DIRECTION_NEGATIVE);
	k_usleep(1100000);
	(void)stepper_stop(fixture->dev);
	POLL_AND_CHECK_SIGNAL(stepper_signal, stepper_event, STEPPER_EVENT_STOPPED, K_MSEC(1040));
}

ZTEST_F(stepper_acceleration, test_run_positive_direction_correct_position_from_lower_speed)
{
	TEST_RUN_DIFFERENT_SPEEDS(fixture, 50, 100, 110, 1000000, 1100000,
				  STEPPER_DIRECTION_POSITIVE);
}

ZTEST_F(stepper_acceleration, test_run_negative_direction_correct_position_from_lower_speed)
{
	TEST_RUN_DIFFERENT_SPEEDS(fixture, 50, 100, -110, 1000000, 1100000,
				  STEPPER_DIRECTION_NEGATIVE);
}
ZTEST_F(stepper_acceleration, test_run_positive_direction_correct_position_from_higher_speed)
{
	TEST_RUN_DIFFERENT_SPEEDS(fixture, 100, 50, 190, 2100000, 1100000,
				  STEPPER_DIRECTION_POSITIVE);
}

ZTEST_F(stepper_acceleration, test_run_negative_direction_correct_position_from_higher_speed)
{
	TEST_RUN_DIFFERENT_SPEEDS(fixture, 100, 50, -190, 2100000, 1100000,
				  STEPPER_DIRECTION_NEGATIVE);
}

ZTEST_F(stepper_acceleration, test_run_positive_direction_correct_position_from_same_speed)
{
	TEST_RUN_DIFFERENT_SPEEDS(fixture, 50, 50, 75, 1000000, 1000000,
				  STEPPER_DIRECTION_POSITIVE);
}

ZTEST_F(stepper_acceleration, test_run_negative_direction_correct_position_from_same_speed)
{
	TEST_RUN_DIFFERENT_SPEEDS(fixture, 50, 50, -75, 1000000, 1000000,
				  STEPPER_DIRECTION_NEGATIVE);
}

ZTEST_F(stepper_acceleration, test_move_to_positive_direction_movement_from_zero_speed)
{
	TEST_MOVE_TO_DIFFERENT_SPEEDS(fixture, 0, 50, 50, 2000000);
}

ZTEST_F(stepper_acceleration, test_move_to_negative_direction_movement_from_zero_speed)
{
	TEST_MOVE_TO_DIFFERENT_SPEEDS(fixture, 0, 50, -50, 2000000);
}

ZTEST_F(stepper_acceleration, test_move_to_positive_direction_movement_from_same_speed)
{
	TEST_MOVE_TO_DIFFERENT_SPEEDS(fixture, 50, 50, 80, 1500000);
}

ZTEST_F(stepper_acceleration, test_move_to_negative_direction_movement_from_same_speed)
{
	TEST_MOVE_TO_DIFFERENT_SPEEDS(fixture, 50, 50, -80, 1500000);
}

ZTEST_F(stepper_acceleration, test_move_to_positive_direction_movement_from_lower_speed)
{
	TEST_MOVE_TO_DIFFERENT_SPEEDS(fixture, 50, 100, 230, 3250000);
}

ZTEST_F(stepper_acceleration, test_move_to_negative_direction_movement_from_lower_speed)
{
	TEST_MOVE_TO_DIFFERENT_SPEEDS(fixture, 50, 100, -230, 3250000);
}

ZTEST_F(stepper_acceleration, test_move_to_positive_direction_movement_from_higher_speed)
{
	TEST_MOVE_TO_DIFFERENT_SPEEDS(fixture, 100, 50, 230, 2400000);
}

ZTEST_F(stepper_acceleration, test_move_to_negative_direction_movement_from_higher_speed)
{
	TEST_MOVE_TO_DIFFERENT_SPEEDS(fixture, 100, 50, -230, 2400000);
}

ZTEST_F(stepper_acceleration, test_move_by_positive_direction_movement_from_zero_speed)
{
	TEST_MOVE_BY_DIFFERENT_SPEEDS(fixture, 0, 50, 50, 50, 2000000);
}

ZTEST_F(stepper_acceleration, test_move_by_negative_direction_movement_from_zero_speed)
{
	TEST_MOVE_BY_DIFFERENT_SPEEDS(fixture, 0, 50, -50, -50, 2000000);
}

ZTEST_F(stepper_acceleration, test_move_by_positive_direction_movement_from_same_speed)
{
	TEST_MOVE_BY_DIFFERENT_SPEEDS(fixture, 50, 50, 50, 80, 1500000);
}

ZTEST_F(stepper_acceleration, test_move_by_negative_direction_movement_from_same_speed)
{
	TEST_MOVE_BY_DIFFERENT_SPEEDS(fixture, 50, 50, -50, -80, 1500000);
}

ZTEST_F(stepper_acceleration, test_move_by_positive_direction_movement_from_lower_speed)
{
	TEST_MOVE_BY_DIFFERENT_SPEEDS(fixture, 50, 100, 200, 230, 3250000);
}

ZTEST_F(stepper_acceleration, test_move_by_negative_direction_movement_from_lower_speed)
{
	TEST_MOVE_BY_DIFFERENT_SPEEDS(fixture, 50, 100, -200, -230, 3250000);
}

ZTEST_F(stepper_acceleration, test_move_by_positive_direction_movement_from_higher_speed)
{
	TEST_MOVE_BY_DIFFERENT_SPEEDS(fixture, 100, 50, 100, 210, 2000000);
}

ZTEST_F(stepper_acceleration, test_move_by_negative_direction_movement_from_higher_speed)
{
	TEST_MOVE_BY_DIFFERENT_SPEEDS(fixture, 100, 50, -100, -210, 2000000);
}

ZTEST_F(stepper_acceleration, test_run_negative_to_posive_direction_change)
{
	int ret;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_microstep_interval(fixture->dev, MED_SPD_I);
	(void)stepper_run(fixture->dev, STEPPER_DIRECTION_NEGATIVE);
	k_usleep(100000);
	(void)stepper_set_microstep_interval(fixture->dev, HIGH_SPD_I);
	ret = stepper_run(fixture->dev, STEPPER_DIRECTION_POSITIVE);
	zassert_equal(ret, -ENOTSUP, "Should return error code %d but returned %d", -ENOTSUP, ret);
}

ZTEST_F(stepper_acceleration, test_run_positive_to_negative_direction_change)
{
	int ret;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_microstep_interval(fixture->dev, MED_SPD_I);
	(void)stepper_run(fixture->dev, STEPPER_DIRECTION_POSITIVE);
	k_usleep(100000);
	(void)stepper_set_microstep_interval(fixture->dev, HIGH_SPD_I);
	ret = stepper_run(fixture->dev, STEPPER_DIRECTION_NEGATIVE);
	zassert_equal(ret, -ENOTSUP, "Should return error code %d but returned %d", -ENOTSUP, ret);
}

ZTEST_F(stepper_acceleration, test_move_to_negative_to_posive_direction_change)
{
	int ret;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_microstep_interval(fixture->dev, MED_SPD_I);
	(void)stepper_move_to(fixture->dev, -50);
	k_usleep(100000);
	ret = stepper_move_to(fixture->dev, 50);
	zassert_equal(ret, -ENOTSUP, "Should return error code %d but returned %d", -ENOTSUP, ret);
}

ZTEST_F(stepper_acceleration, test_move_to_positive_to_negative_direction_change)
{
	int ret;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_microstep_interval(fixture->dev, MED_SPD_I);
	(void)stepper_move_to(fixture->dev, 50);
	k_usleep(100000);
	ret = stepper_move_to(fixture->dev, -50);
	zassert_equal(ret, -ENOTSUP, "Should return error code %d but returned %d", -ENOTSUP, ret);
}

ZTEST_F(stepper_acceleration, test_move_by_negative_to_posive_direction_change)
{
	int ret;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_microstep_interval(fixture->dev, MED_SPD_I);
	(void)stepper_move_by(fixture->dev, -50);
	k_usleep(100000);
	ret = stepper_move_by(fixture->dev, 50);
	zassert_equal(ret, -ENOTSUP, "Should return error code %d but returned %d", -ENOTSUP, ret);
}

ZTEST_F(stepper_acceleration, test_move_by_positive_to_negative_direction_change)
{
	int ret;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_microstep_interval(fixture->dev, MED_SPD_I);
	(void)stepper_move_by(fixture->dev, 50);
	k_usleep(100000);
	ret = stepper_move_by(fixture->dev, -50);
	zassert_equal(ret, -ENOTSUP, "Should return error code %d but returned %d", -ENOTSUP, ret);
}

ZTEST_F(stepper_acceleration, test_move_to_positive_direction_too_small_position_difference)
{
	int ret;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);
	(void)stepper_set_microstep_interval(fixture->dev, MED_SPD_I);
	(void)stepper_run(fixture->dev, STEPPER_DIRECTION_POSITIVE);
	k_usleep(1100000);
	(void)stepper_set_microstep_interval(fixture->dev, HIGH_SPD_I);
	ret = stepper_move_to(fixture->dev, 40);
	zassert_equal(ret, -EINVAL,
		      "Should return error code %d but returned %d for move_to from lower to "
		      "higher velocity",
		      -EINVAL, ret);
	(void)stepper_set_microstep_interval(fixture->dev, MED_SPD_I);
	ret = stepper_move_to(fixture->dev, 40);
	zassert_equal(ret, -EINVAL,
		      "Should return error code %d but returned %d for move_to from existing "
		      "velocity with same velocity",
		      -EINVAL, ret);
	(void)stepper_set_microstep_interval(fixture->dev, LOW_SPD_I);
	ret = stepper_move_to(fixture->dev, 40);
	zassert_equal(ret, -EINVAL,
		      "Should return error code %d but returned %d for move_to from higher to "
		      "lower velocity",
		      -EINVAL, ret);
}

ZTEST_F(stepper_acceleration, test_move_to_negative_direction_too_small_position_difference)
{
	int ret;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);
	(void)stepper_set_microstep_interval(fixture->dev, MED_SPD_I);
	(void)stepper_run(fixture->dev, STEPPER_DIRECTION_NEGATIVE);
	k_usleep(1100000);
	(void)stepper_set_microstep_interval(fixture->dev, HIGH_SPD_I);
	ret = stepper_move_to(fixture->dev, -40);
	zassert_equal(ret, -EINVAL,
		      "Should return error code %d but returned %d for move_to from lower to "
		      "higher velocity",
		      -EINVAL, ret);
	(void)stepper_set_microstep_interval(fixture->dev, MED_SPD_I);
	ret = stepper_move_to(fixture->dev, -40);
	zassert_equal(ret, -EINVAL,
		      "Should return error code %d but returned %d for move_to from existing "
		      "velocity with same velocity",
		      -EINVAL, ret);
	(void)stepper_set_microstep_interval(fixture->dev, LOW_SPD_I);
	ret = stepper_move_to(fixture->dev, -40);
	zassert_equal(ret, -EINVAL,
		      "Should return error code %d but returned %d for move_to from higher to "
		      "lower velocity",
		      -EINVAL, ret);
}

ZTEST_F(stepper_acceleration, test_move_by_positive_direction_too_small_position_difference)
{
	int ret;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);
	(void)stepper_set_microstep_interval(fixture->dev, MED_SPD_I);
	(void)stepper_run(fixture->dev, STEPPER_DIRECTION_POSITIVE);
	k_usleep(1100000);
	(void)stepper_set_microstep_interval(fixture->dev, HIGH_SPD_I);
	ret = stepper_move_by(fixture->dev, 10);
	zassert_equal(ret, -EINVAL,
		      "Should return error code %d but returned %d for move_to from lower to "
		      "higher velocity",
		      -EINVAL, ret);
	(void)stepper_set_microstep_interval(fixture->dev, MED_SPD_I);
	ret = stepper_move_by(fixture->dev, 10);
	zassert_equal(ret, -EINVAL,
		      "Should return error code %d but returned %d for move_to from existing "
		      "velocity with same velocity",
		      -EINVAL, ret);
	(void)stepper_set_microstep_interval(fixture->dev, LOW_SPD_I);
	ret = stepper_move_by(fixture->dev, 10);
	zassert_equal(ret, -EINVAL,
		      "Should return error code %d but returned %d for move_to from higher to "
		      "lower velocity",
		      -EINVAL, ret);
}

ZTEST_F(stepper_acceleration, test_move_by_negative_direction_too_small_position_difference)
{
	int ret;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);
	(void)stepper_set_microstep_interval(fixture->dev, MED_SPD_I);
	(void)stepper_run(fixture->dev, STEPPER_DIRECTION_NEGATIVE);
	k_usleep(1100000);
	(void)stepper_set_microstep_interval(fixture->dev, HIGH_SPD_I);
	ret = stepper_move_by(fixture->dev, -10);
	zassert_equal(ret, -EINVAL,
		      "Should return error code %d but returned %d for move_to from lower to "
		      "higher velocity",
		      -EINVAL, ret);
	(void)stepper_set_microstep_interval(fixture->dev, MED_SPD_I);
	ret = stepper_move_by(fixture->dev, -10);
	zassert_equal(ret, -EINVAL,
		      "Should return error code %d but returned %d for move_to from existing "
		      "velocity with same velocity",
		      -EINVAL, ret);
	(void)stepper_set_microstep_interval(fixture->dev, LOW_SPD_I);
	ret = stepper_move_by(fixture->dev, -10);
	zassert_equal(ret, -EINVAL,
		      "Should return error code %d but returned %d for move_to from higher to "
		      "lower velocity",
		      -EINVAL, ret);
}

ZTEST_F(stepper_acceleration, test_move_by_negative_to_positive_direction_change_when_stopped)
{
	int pos;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);
	(void)stepper_set_microstep_interval(fixture->dev, MED_SPD_I);
	(void)stepper_move_by(fixture->dev, -50);
	POLL_AND_CHECK_SIGNAL(stepper_signal, stepper_event, STEPPER_EVENT_STEPS_COMPLETED,
			      K_SECONDS(3));

	k_poll_signal_reset(&stepper_signal);
	(void)stepper_move_by(fixture->dev, 50);
	POLL_AND_CHECK_SIGNAL(stepper_signal, stepper_event, STEPPER_EVENT_STEPS_COMPLETED,
			      K_SECONDS(3));
	(void)stepper_get_actual_position(fixture->dev, &pos);

	zassert_true(IN_RANGE(pos, -1, 1), "Current position should be between %d and %d but is %d",
		     -1, 1, pos);
}

ZTEST_F(stepper_acceleration, test_move_by_positive_to_negative_direction_change_when_stopped)
{
	int pos;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);
	(void)stepper_set_microstep_interval(fixture->dev, MED_SPD_I);
	(void)stepper_move_by(fixture->dev, 50);
	POLL_AND_CHECK_SIGNAL(stepper_signal, stepper_event, STEPPER_EVENT_STEPS_COMPLETED,
			      K_SECONDS(3));

	k_poll_signal_reset(&stepper_signal);
	(void)stepper_move_by(fixture->dev, -50);
	POLL_AND_CHECK_SIGNAL(stepper_signal, stepper_event, STEPPER_EVENT_STEPS_COMPLETED,
			      K_SECONDS(3));
	(void)stepper_get_actual_position(fixture->dev, &pos);

	zassert_true(IN_RANGE(pos, -1, 1), "Current position should be between %d and %d but is %d",
		     -1, 1, pos);
}

ZTEST_F(stepper_acceleration, test_move_to_negative_to_positive_direction_change_when_stopped)
{
	int pos;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);
	(void)stepper_set_microstep_interval(fixture->dev, MED_SPD_I);
	(void)stepper_move_to(fixture->dev, -50);
	POLL_AND_CHECK_SIGNAL(stepper_signal, stepper_event, STEPPER_EVENT_STEPS_COMPLETED,
			      K_SECONDS(3));

	k_poll_signal_reset(&stepper_signal);
	(void)stepper_move_to(fixture->dev, 0);
	POLL_AND_CHECK_SIGNAL(stepper_signal, stepper_event, STEPPER_EVENT_STEPS_COMPLETED,
			      K_SECONDS(3));
	(void)stepper_get_actual_position(fixture->dev, &pos);

	zassert_equal(0, pos, "Position should be 0 but is %d", pos);
}

ZTEST_F(stepper_acceleration, test_move_to_positive_to_negative_direction_change_when_stopped)
{
	int pos;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);
	(void)stepper_set_microstep_interval(fixture->dev, MED_SPD_I);
	(void)stepper_move_to(fixture->dev, 50);
	POLL_AND_CHECK_SIGNAL(stepper_signal, stepper_event, STEPPER_EVENT_STEPS_COMPLETED,
			      K_SECONDS(3));

	k_poll_signal_reset(&stepper_signal);
	(void)stepper_move_to(fixture->dev, 0);
	POLL_AND_CHECK_SIGNAL(stepper_signal, stepper_event, STEPPER_EVENT_STEPS_COMPLETED,
			      K_SECONDS(3));
	(void)stepper_get_actual_position(fixture->dev, &pos);

	zassert_equal(0, pos, "Position should be 0 but is %d", pos);
}

ZTEST_F(stepper_acceleration, test_run_negative_to_positive_direction_change_when_stopped)
{
	int pos;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);
	(void)stepper_set_microstep_interval(fixture->dev, MED_SPD_I);
	(void)stepper_run(fixture->dev, STEPPER_DIRECTION_NEGATIVE);
	k_msleep(1000);
	(void)stepper_stop(fixture->dev);
	POLL_AND_CHECK_SIGNAL(stepper_signal, stepper_event, STEPPER_EVENT_STOPPED, K_SECONDS(3));

	k_poll_signal_reset(&stepper_signal);
	(void)stepper_run(fixture->dev, STEPPER_DIRECTION_POSITIVE);
	k_msleep(1000);
	(void)stepper_stop(fixture->dev);
	POLL_AND_CHECK_SIGNAL(stepper_signal, stepper_event, STEPPER_EVENT_STOPPED, K_SECONDS(3));
	(void)stepper_get_actual_position(fixture->dev, &pos);

	zassert_true(IN_RANGE(pos, -2, 2), "Current position should be between %d and %d but is %d",
		     -2, 2, pos);
}

ZTEST_F(stepper_acceleration, test_run_positive_to_negative_direction_change_when_stopped)
{
	int pos;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);
	(void)stepper_set_microstep_interval(fixture->dev, MED_SPD_I);
	(void)stepper_run(fixture->dev, STEPPER_DIRECTION_POSITIVE);
	k_msleep(1000);
	(void)stepper_stop(fixture->dev);
	POLL_AND_CHECK_SIGNAL(stepper_signal, stepper_event, STEPPER_EVENT_STOPPED, K_SECONDS(3));

	k_poll_signal_reset(&stepper_signal);
	(void)stepper_run(fixture->dev, STEPPER_DIRECTION_NEGATIVE);
	k_msleep(1000);
	(void)stepper_stop(fixture->dev);
	POLL_AND_CHECK_SIGNAL(stepper_signal, stepper_event, STEPPER_EVENT_STOPPED, K_SECONDS(3));
	(void)stepper_get_actual_position(fixture->dev, &pos);

	zassert_true(IN_RANGE(pos, -2, 2), "Current position should be between %d and %d but is %d",
		     -2, 2, pos);
}

ZTEST_F(stepper_acceleration, test_move_by_positive_direction_correct_deceleration_time)
{
	int pos;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);
	(void)stepper_set_microstep_interval(fixture->dev, MED_SPD_I);
	(void)stepper_move_by(fixture->dev, 60);
	(void)k_msleep(1700);
	(void)stepper_get_actual_position(fixture->dev, &pos);
	zassert_equal(pos, 54, "Current position should be 54, but is %d", pos);
}

ZTEST_F(stepper_acceleration, test_move_by_negative_direction_correct_deceleration_time)
{
	int pos;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);
	(void)stepper_set_microstep_interval(fixture->dev, MED_SPD_I);
	(void)stepper_move_by(fixture->dev, -60);
	(void)k_msleep(1700);
	(void)stepper_get_actual_position(fixture->dev, &pos);
	zassert_equal(pos, -54, "Current position should be -54, but is %d", pos);
}

ZTEST_F(stepper_acceleration, test_move_to_positive_direction_correct_deceleration_time)
{
	int pos;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);
	(void)stepper_set_microstep_interval(fixture->dev, MED_SPD_I);
	(void)stepper_move_to(fixture->dev, 60);
	(void)k_msleep(1700);
	(void)stepper_get_actual_position(fixture->dev, &pos);
	zassert_equal(pos, 54, "Current position should be 54, but is %d", pos);
}

ZTEST_F(stepper_acceleration, test_move_to_negative_direction_correct_deceleration_time)
{
	int pos;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);
	(void)stepper_set_microstep_interval(fixture->dev, MED_SPD_I);
	(void)stepper_move_to(fixture->dev, -60);
	(void)k_msleep(1700);
	(void)stepper_get_actual_position(fixture->dev, &pos);
	zassert_equal(pos, -54, "Current position should be -54, but is %d", pos);
}

ZTEST_F(stepper_acceleration, test_stop_is_moving_false_when_stopped)
{
	bool moving;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);
	(void)stepper_set_microstep_interval(fixture->dev, MED_SPD_I);
	(void)stepper_run(fixture->dev, STEPPER_DIRECTION_POSITIVE);
	k_msleep(1100);
	(void)stepper_stop(fixture->dev);
	POLL_AND_CHECK_SIGNAL(stepper_signal, stepper_event, STEPPER_EVENT_STOPPED, K_MSEC(1100));

	(void)stepper_is_moving(fixture->dev, &moving);
	zassert_false(moving, "Driver should not be in state is_moving after stopping");
}

ZTEST_F(stepper_acceleration, test_stop_no_movement_when_stopped)
{
	int pos_1;
	int pos_2;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);
	(void)stepper_set_microstep_interval(fixture->dev, MED_SPD_I);
	(void)stepper_run(fixture->dev, STEPPER_DIRECTION_POSITIVE);
	k_msleep(1100);
	(void)stepper_stop(fixture->dev);
	POLL_AND_CHECK_SIGNAL(stepper_signal, stepper_event, STEPPER_EVENT_STOPPED, K_SECONDS(3));

	(void)stepper_get_actual_position(fixture->dev, &pos_1);
	k_msleep(500);
	(void)stepper_get_actual_position(fixture->dev, &pos_2);
	zassert_equal(pos_2, pos_1, "Current position should not have changed from %d but is %d",
		      pos_1, pos_2);
}

ZTEST_F(stepper_acceleration, test_step_interval_run_immediate_effect)
{
	int pos;
	int pos_target = 115;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);
	(void)stepper_set_microstep_interval(fixture->dev, MED_SPD_I);
	(void)stepper_run(fixture->dev, STEPPER_DIRECTION_POSITIVE);
	k_msleep(1100);
	(void)stepper_set_microstep_interval(fixture->dev, HIGH_SPD_I);
	k_msleep(1100);

	(void)stepper_get_actual_position(fixture->dev, &pos);
	zassert_true(IN_RANGE(pos, pos_target - 2, pos_target + 2),
		     "Current position should be between %d and %d but is %d", pos_target - 2,
		     pos_target + 2, pos);
}

ZTEST_F(stepper_acceleration, test_step_interval_move_by_immediate_effect)
{
	int pos;
	int steps = 255;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);
	(void)stepper_set_microstep_interval(fixture->dev, MED_SPD_I);
	(void)stepper_move_by(fixture->dev, steps);
	k_msleep(1100);
	(void)stepper_set_microstep_interval(fixture->dev, HIGH_SPD_I);
	POLL_AND_CHECK_SIGNAL(stepper_signal, stepper_event, STEPPER_EVENT_STEPS_COMPLETED,
			      K_MSEC(3500));

	(void)stepper_get_actual_position(fixture->dev, &pos);
	zassert_equal(pos, steps, "Target position should be %d but is %d", steps, pos);
}

ZTEST_F(stepper_acceleration, test_step_interval_move_to_immediate_effect)
{
	int pos;
	int steps = 255;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);
	(void)stepper_set_microstep_interval(fixture->dev, MED_SPD_I);
	(void)stepper_move_to(fixture->dev, steps);
	k_msleep(1100);
	(void)stepper_set_microstep_interval(fixture->dev, HIGH_SPD_I);
	POLL_AND_CHECK_SIGNAL(stepper_signal, stepper_event, STEPPER_EVENT_STEPS_COMPLETED,
			      K_MSEC(3500));

	(void)stepper_get_actual_position(fixture->dev, &pos);
	zassert_equal(pos, steps, "Target position should be %d but is %d", steps, pos);
}

/*
 * Copyright (c) 2024 Fabian Blatz <fabianblatz@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/drivers/stepper.h>
#include <zephyr/drivers/stepper/stepper_fake.h>
#include <zephyr/fff.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/util.h>

#define FAKE_STEPPER_NAME DEVICE_DT_NAME(DT_NODELABEL(fake_stepper))

/* Global variables */
static const struct device *const fake_stepper_dev = DEVICE_DT_GET(DT_NODELABEL(fake_stepper));

DEFINE_FFF_GLOBALS;

#define ASSERT_STEPPER_FUNC_CALLED(stepper_fake_func, retval)                                      \
	zassert_ok(retval, "failed to execute shell command (err %d)", retval);                    \
	zassert_equal(stepper_fake_func.call_count, 1,                                             \
		      STRINGIFY(stepper_fake_func) " function not called");                        \
	zassert_equal(stepper_fake_func.arg0_val, fake_stepper_dev, "wrong device pointer")

static void *stepper_shell_setup(void)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();

	/* Wait for the initialization of the shell dummy backend. */
	WAIT_FOR(shell_ready(sh), 20000, k_msleep(1));
	zassert_true(shell_ready(sh), "timed out waiting for dummy shell backend");

	return NULL;
}

ZTEST_SUITE(stepper_shell, NULL, stepper_shell_setup, NULL, NULL, NULL);

ZTEST(stepper_shell, test_stepper_enable)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err = shell_execute_cmd(sh, "stepper enable " FAKE_STEPPER_NAME " on");

	ASSERT_STEPPER_FUNC_CALLED(fake_stepper_enable_fake, err);
	zassert_equal(fake_stepper_enable_fake.arg1_val, true, "wrong enable value");

	RESET_FAKE(fake_stepper_enable);

	err = shell_execute_cmd(sh, "stepper enable " FAKE_STEPPER_NAME " off");
	ASSERT_STEPPER_FUNC_CALLED(fake_stepper_enable_fake, err);
	zassert_equal(fake_stepper_enable_fake.arg1_val, false, "wrong enable value");
}

ZTEST(stepper_shell, test_stepper_move)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err = shell_execute_cmd(sh, "stepper move " FAKE_STEPPER_NAME " 1000");

	ASSERT_STEPPER_FUNC_CALLED(fake_stepper_move_fake, err);
	zassert_equal(fake_stepper_move_fake.arg1_val, 1000, "wrong microsteps value");
}

ZTEST(stepper_shell, test_stepper_set_max_velocity)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err = shell_execute_cmd(sh, "stepper set_max_velocity " FAKE_STEPPER_NAME " 200");

	ASSERT_STEPPER_FUNC_CALLED(fake_stepper_set_max_velocity_fake, err);
	zassert_equal(fake_stepper_set_max_velocity_fake.arg1_val, 200, "wrong velocity value");
}

ZTEST(stepper_shell, test_stepper_set_micro_step_res)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err = shell_execute_cmd(sh, "stepper set_micro_step_res " FAKE_STEPPER_NAME " 64");

	ASSERT_STEPPER_FUNC_CALLED(fake_stepper_set_micro_step_res_fake, err);
	zassert_equal(fake_stepper_set_micro_step_res_fake.arg1_val, 64,
		      "wrong micro steps resolution value");
}

ZTEST(stepper_shell, test_stepper_set_micro_step_res_invalid_value)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err = shell_execute_cmd(sh, "stepper set_micro_step_res " FAKE_STEPPER_NAME " 111");

	zassert_not_equal(err, 0, " executed set_micro_step_res with invalid micro steps value");
}

ZTEST(stepper_shell, test_stepper_get_micro_step_res)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err = shell_execute_cmd(sh, "stepper get_micro_step_res " FAKE_STEPPER_NAME);

	ASSERT_STEPPER_FUNC_CALLED(fake_stepper_get_micro_step_res_fake, err);
}

ZTEST(stepper_shell, test_stepper_set_reference_position)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err = shell_execute_cmd(sh, "stepper set_reference_position " FAKE_STEPPER_NAME " 100");

	ASSERT_STEPPER_FUNC_CALLED(fake_stepper_set_reference_position_fake, err);
	zassert_equal(fake_stepper_set_reference_position_fake.arg1_val, 100,
		      "wrong actual position value");
}

ZTEST(stepper_shell, test_stepper_get_actual_position)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err = shell_execute_cmd(sh, "stepper get_actual_position " FAKE_STEPPER_NAME);

	ASSERT_STEPPER_FUNC_CALLED(fake_stepper_get_actual_position_fake, err);
}

ZTEST(stepper_shell, test_stepper_set_target_position)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err = shell_execute_cmd(sh, "stepper set_target_position " FAKE_STEPPER_NAME " 200");

	ASSERT_STEPPER_FUNC_CALLED(fake_stepper_set_target_position_fake, err);
	zassert_equal(fake_stepper_set_target_position_fake.arg1_val, 200,
		      "wrong target position value");
}

ZTEST(stepper_shell, test_stepper_enable_constant_velocity_mode)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err = shell_execute_cmd(sh, "stepper enable_constant_velocity_mode " FAKE_STEPPER_NAME
					" positive 200");

	ASSERT_STEPPER_FUNC_CALLED(fake_stepper_enable_constant_velocity_mode_fake, err);
	zassert_equal(fake_stepper_enable_constant_velocity_mode_fake.arg1_val,
		      STEPPER_DIRECTION_POSITIVE, "wrong direction value");
	zassert_equal(fake_stepper_enable_constant_velocity_mode_fake.arg2_val, 200,
		      "wrong velocity value");
}

ZTEST(stepper_shell, test_stepper_enable_constant_velocity_mode_invalid_direction)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err = shell_execute_cmd(sh, "stepper enable_constant_velocity_mode " FAKE_STEPPER_NAME
					" foo 200");

	zassert_not_equal(err, 0,
			  " executed enable_constant_velocity_mode with invalid direction value");
}

ZTEST(stepper_shell, test_stepper_info)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err = shell_execute_cmd(sh, "stepper info " FAKE_STEPPER_NAME);

	zassert_ok(err, "failed to execute shell command (err %d)", err);

	zassert_equal(fake_stepper_is_moving_fake.call_count, 1, "is_moving function not called");
	zassert_equal(fake_stepper_get_actual_position_fake.call_count, 1,
		      "get_actual_position function not called");
	zassert_equal(fake_stepper_get_micro_step_res_fake.call_count, 1,
		      "get_micro_step_res function not called");
}

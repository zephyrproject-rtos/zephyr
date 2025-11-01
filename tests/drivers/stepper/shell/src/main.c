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

#define FAKE_STEPPER_CONTROLLER DEVICE_DT_NAME(DT_NODELABEL(fake_stepper_controller))
#define FAKE_STEPPER_DRIVER     DEVICE_DT_NAME(DT_NODELABEL(fake_stepper_driver))

/* Global variables */
static const struct device *const fake_stepper_driver_dev =
	DEVICE_DT_GET(DT_NODELABEL(fake_stepper_driver));
static const struct device *const fake_stepper_controller_dev =
	DEVICE_DT_GET(DT_NODELABEL(fake_stepper_controller));

DEFINE_FFF_GLOBALS;

#define ASSERT_STEPPER_FUNC_CALLED(stepper_fake_func, stepper_fake_dev, retval)                    \
	zassert_ok(retval, "failed to execute shell command (err %d)", retval);                    \
	zassert_equal(stepper_fake_func.call_count, 1,                                             \
		      STRINGIFY(stepper_fake_func) " function not called");                        \
	zassert_equal(stepper_fake_func.arg0_val, stepper_fake_dev, "wrong device pointer")

static void *stepper_shell_setup(void)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();

	/* Wait for the initialization of the shell dummy backend. */
	WAIT_FOR(shell_ready(sh), 20000, k_msleep(1));
	zassert_true(shell_ready(sh), "timed out waiting for dummy shell backend");

	return NULL;
}

ZTEST_SUITE(stepper_shell, NULL, stepper_shell_setup, NULL, NULL, NULL);

ZTEST(stepper_shell, test_stepper_drv_enable)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err = shell_execute_cmd(sh, "stepper enable " FAKE_STEPPER_DRIVER);

	ASSERT_STEPPER_FUNC_CALLED(fake_stepper_drv_enable_fake, fake_stepper_driver_dev, err);
	zassert_equal(err, 0, "stepper enable could not be executed");
}

ZTEST(stepper_shell, test_stepper_drv_disable)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err = shell_execute_cmd(sh, "stepper disable " FAKE_STEPPER_DRIVER);

	ASSERT_STEPPER_FUNC_CALLED(fake_stepper_drv_disable_fake, fake_stepper_driver_dev, err);
	zassert_equal(err, 0, "stepper disable could not be executed");
}

ZTEST(stepper_shell, test_stepper_move_by)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err = shell_execute_cmd(sh, "stepper move_by " FAKE_STEPPER_CONTROLLER " 1000");

	ASSERT_STEPPER_FUNC_CALLED(fake_stepper_move_by_fake, fake_stepper_controller_dev, err);
	zassert_equal(fake_stepper_move_by_fake.arg1_val, 1000, "wrong microsteps value");
}

ZTEST(stepper_shell, test_stepper_set_microstep_interval)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err = shell_execute_cmd(sh, "stepper set_microstep_interval " FAKE_STEPPER_CONTROLLER
					" 200");

	ASSERT_STEPPER_FUNC_CALLED(fake_stepper_set_microstep_interval_fake,
				   fake_stepper_controller_dev, err);
	zassert_equal(fake_stepper_set_microstep_interval_fake.arg1_val, 200,
		      "wrong step_interval value");
}

ZTEST(stepper_shell, test_stepper_drv_set_micro_step_res)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err = shell_execute_cmd(sh, "stepper set_micro_step_res " FAKE_STEPPER_DRIVER " 64");

	ASSERT_STEPPER_FUNC_CALLED(fake_stepper_drv_set_micro_step_res_fake,
				   fake_stepper_driver_dev, err);
	zassert_equal(fake_stepper_drv_set_micro_step_res_fake.arg1_val, 64,
		      "wrong micro steps resolution value");
}

ZTEST(stepper_shell, test_stepper_drv_set_micro_step_res_invalid_value)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err = shell_execute_cmd(sh, "stepper set_micro_step_res " FAKE_STEPPER_DRIVER " 111");

	zassert_not_equal(err, 0, " executed set_micro_step_res with invalid micro steps value");
}

ZTEST(stepper_shell, test_stepper_drv_get_micro_step_res)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err = shell_execute_cmd(sh, "stepper get_micro_step_res " FAKE_STEPPER_DRIVER);

	ASSERT_STEPPER_FUNC_CALLED(fake_stepper_drv_get_micro_step_res_fake,
				   fake_stepper_driver_dev, err);
}

ZTEST(stepper_shell, test_stepper_set_reference_position)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err = shell_execute_cmd(sh, "stepper set_reference_position " FAKE_STEPPER_CONTROLLER
					" 100");

	ASSERT_STEPPER_FUNC_CALLED(fake_stepper_set_reference_position_fake,
				   fake_stepper_controller_dev, err);
	zassert_equal(fake_stepper_set_reference_position_fake.arg1_val, 100,
		      "wrong actual position value");
}

ZTEST(stepper_shell, test_stepper_get_actual_position)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err = shell_execute_cmd(sh, "stepper get_actual_position " FAKE_STEPPER_CONTROLLER);

	ASSERT_STEPPER_FUNC_CALLED(fake_stepper_get_actual_position_fake,
				   fake_stepper_controller_dev, err);
}

ZTEST(stepper_shell, test_stepper_move_to)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err = shell_execute_cmd(sh, "stepper move_to " FAKE_STEPPER_CONTROLLER " 200");

	ASSERT_STEPPER_FUNC_CALLED(fake_stepper_move_to_fake, fake_stepper_controller_dev, err);
	zassert_equal(fake_stepper_move_to_fake.arg1_val, 200, "wrong target position value");
}

ZTEST(stepper_shell, test_stepper_run)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err = shell_execute_cmd(sh, "stepper run " FAKE_STEPPER_CONTROLLER " positive");

	ASSERT_STEPPER_FUNC_CALLED(fake_stepper_run_fake, fake_stepper_controller_dev, err);
	zassert_equal(fake_stepper_run_fake.arg1_val, STEPPER_DIRECTION_POSITIVE,
		      "wrong direction value");
}

ZTEST(stepper_shell, test_stepper_run_invalid_direction)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err = shell_execute_cmd(sh, "stepper run " FAKE_STEPPER_CONTROLLER " foo");

	zassert_not_equal(err, 0, " executed run with invalid direction value");
}

ZTEST(stepper_shell, test_stepper_stop)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err = shell_execute_cmd(sh, "stepper stop " FAKE_STEPPER_CONTROLLER);

	ASSERT_STEPPER_FUNC_CALLED(fake_stepper_stop_fake, fake_stepper_controller_dev, err);
	zassert_equal(err, 0, "stepper stop could not be executed");
}

ZTEST(stepper_shell, test_stepper_controller_info)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err = shell_execute_cmd(sh, "stepper control_info " FAKE_STEPPER_CONTROLLER);

	zassert_ok(err, "failed to execute shell command (err %d)", err);

	zassert_equal(fake_stepper_is_moving_fake.call_count, 1, "is_moving function not called");
	zassert_equal(fake_stepper_get_actual_position_fake.call_count, 1,
		      "get_actual_position function not called");
}

ZTEST(stepper_shell, test_stepper_info)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err = shell_execute_cmd(sh, "stepper info " FAKE_STEPPER_DRIVER);

	zassert_ok(err, "failed to execute shell command (err %d)", err);

	zassert_equal(fake_stepper_drv_get_micro_step_res_fake.call_count, 1,
		      "get_micro_step_res function not called");
}

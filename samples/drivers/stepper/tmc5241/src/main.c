/*
 * SPDX-FileCopyrightText: Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Exercises the tmc5241 driver end-to-end for Level 2 functional testing.
 * Each block below is commented with the stp.md test ID(s) it covers.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/stepper/stepper.h>
#include <zephyr/drivers/stepper/stepper_ctrl.h>
#include <zephyr/drivers/stepper/stepper_trinamic.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(stepper_tmc5241, CONFIG_STEPPER_LOG_LEVEL);

static const struct device *stepper_ctrl = DEVICE_DT_GET(DT_ALIAS(stepper_ctrl));
static const struct device *stepper_driver = DEVICE_DT_GET(DT_ALIAS(stepper_driver));

int32_t ping_pong_target_position = CONFIG_STEPS_PER_REV * CONFIG_PING_PONG_N_REV *
				     DT_PROP(DT_ALIAS(stepper_driver), micro_step_res);

K_SEM_DEFINE(steps_completed_sem, 0, 1);

/* L2-004 (STALL_DETECTED), L2-005 (FAULT_DETECTED) - these fire from the
 * stepper.h child (SRS-DRV-006), not stepper_ctrl.h.
 */
void stepper_driver_callback(const struct device *dev, const enum stepper_event event,
			     void *user_data)
{
	ARG_UNUSED(user_data);
	switch (event) {
	case STEPPER_EVENT_STALL_DETECTED:
		LOG_WRN("event: stall detected");
		break;
	case STEPPER_EVENT_FAULT_DETECTED:
		LOG_ERR("event: fault detected");
		break;
	default:
		break;
	}
}

/* L2-007 (STEPS_COMPLETED), L2-014 (RIGHT/LEFT_END_STOP_DETECTED) - fire from
 * the stepper_ctrl.h child (SRS-CTRL-010), interrupt-driven or polled
 * depending on whether diag0-gpios is wired (L2-015, SRS-CTRL-011).
 */
void stepper_ctrl_callback(const struct device *dev, const enum stepper_ctrl_event event,
			   void *user_data)
{
	ARG_UNUSED(user_data);
	switch (event) {
	case STEPPER_CTRL_EVENT_STEPS_COMPLETED:
		LOG_INF("event: steps completed");
		k_sem_give(&steps_completed_sem);
		break;
	case STEPPER_CTRL_EVENT_RIGHT_END_STOP_DETECTED:
		LOG_INF("event: right end-stop detected");
		break;
	case STEPPER_CTRL_EVENT_LEFT_END_STOP_DETECTED:
		LOG_INF("event: left end-stop detected");
		break;
	default:
		break;
	}
}

int main(void)
{
	int ret;

	LOG_INF("Starting tmc5241 stepper sample");
	if (!device_is_ready(stepper_ctrl) || !device_is_ready(stepper_driver)) {
		LOG_ERR("stepper_ctrl/stepper_driver device not ready");
		return -ENODEV;
	}

	/* L2-016: settings only take effect on the next move_to()/run() - the driver
	 * arms/disarms StallGuard-stop internally (SRS-CTRL-001), no manual enable call
	 * is needed or exposed as public API (mirrors tmc50xx/tmc51xx samples).
	 */
	struct tmc_stallguard_settings sg_settings = {
		.is_sg_enabled = true,
		.sg_velocity_check_interval_ms = 100,
		.sg_threshold_velocity = CONFIG_STALLGUARD_THRESHOLD_VELOCITY,
	};
	tmc5241_stepper_ctrl_configure_stallguard(stepper_ctrl, &sg_settings);

	stepper_set_event_cb(stepper_driver, stepper_driver_callback, NULL);
	stepper_ctrl_set_event_cb(stepper_ctrl, stepper_ctrl_callback, NULL);

	/* L2-001: stepper_enable(); read back CHOPCONF.TOFF externally to verify. */
	stepper_enable(stepper_driver);

	/* L2-002/L2-003: round-trip the DT-configured microstep resolution, then
	 * demonstrate the invalid-value rejection path.
	 */
	enum stepper_micro_step_resolution micro_step_res;

	stepper_get_micro_step_res(stepper_driver, &micro_step_res);
	LOG_INF("Microstep resolution is %d", micro_step_res);

	ret = stepper_set_micro_step_res(stepper_driver, (enum stepper_micro_step_resolution)3);
	LOG_INF("set_micro_step_res(3) [invalid] returned %d (expect -EINVAL)", ret);

	/* L2-013: TMC5241 uses the ramp generator, not a fixed step interval. */
	ret = stepper_ctrl_set_microstep_interval(stepper_ctrl, 1000);
	LOG_INF("set_microstep_interval() returned %d (expect -ENOSYS)", ret);

	/* L2-011: set a known reference position before moving. */
	stepper_ctrl_set_reference_position(stepper_ctrl, 0);

	/* L2-009: brief run()/stop() velocity-mode exercise before switching to
	 * position-mode ping-pong below.
	 */
	stepper_ctrl_run(stepper_ctrl, STEPPER_CTRL_DIRECTION_POSITIVE);
	k_sleep(K_MSEC(500));
	stepper_ctrl_stop(stepper_ctrl);

	/* L2-007/L2-008: absolute/relative moves. */
	stepper_ctrl_move_by(stepper_ctrl, ping_pong_target_position);

	/* L2-012: change VMAX at runtime via the Trinamic-specific public API. */
	int32_t tmc_velocity =
		DT_PROP(DT_ALIAS(stepper_ctrl), vmax) * CONFIG_MAX_VELOCITY_MULTIPLIER;
	(void)tmc5241_stepper_ctrl_set_max_velocity(stepper_ctrl, tmc_velocity);

	for (;;) {
		if (k_sem_take(&steps_completed_sem, K_FOREVER) == 0) {
			/* L2-010: is_moving() comes from DRVSTATUS.STST, which is only set
			 * 2^20 fCLK cycles (65 ms at 16 MHz) after the last step. It still
			 * reads true right after the event and false once that time passed.
			 */
			bool is_moving;

			stepper_ctrl_is_moving(stepper_ctrl, &is_moving);
			LOG_INF("is_moving right after the event: %d", is_moving);
			k_msleep(100);
			stepper_ctrl_is_moving(stepper_ctrl, &is_moving);
			LOG_INF("is_moving 100 ms later: %d", is_moving);

			ping_pong_target_position *= -1;
			stepper_ctrl_move_by(stepper_ctrl, ping_pong_target_position);

			int32_t actual_position;

			ret = stepper_ctrl_get_actual_position(stepper_ctrl, &actual_position);
			if (ret == 0) {
				LOG_INF("Actual position: %d", actual_position);
			} else {
				LOG_ERR("Failed to get actual position");
			}
		}
	}
	return 0;
}

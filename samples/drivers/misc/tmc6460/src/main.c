/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Cherrence Sarip <cherrence.sarip@analog.com>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file main.c
 * @brief TMC6460 open-loop voltage mode BLDC motor control sample.
 *
 * Direct port of pytrinamic bldc_openloop_voltage_mode.py with
 * explicit hardware initialization that the eval board firmware does.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include <zephyr/drivers/misc/tmc6460/tmc6460.h>

LOG_MODULE_REGISTER(tmc6460_voltage_mode, LOG_LEVEL_INF);

/* Board GPIOs */
#define DRV_EN_NODE DT_ALIAS(tmc6460_drv_en)
static const struct gpio_dt_spec drv_en = GPIO_DT_SPEC_GET(DRV_EN_NODE, gpios);

#define SLEEPN_NODE DT_ALIAS(tmc6460_sleepn)
static const struct gpio_dt_spec sleepn = GPIO_DT_SPEC_GET(SLEEPN_NODE, gpios);

/*
 * Motor Parameters. The physical pole-pair count is a hardware trait and is
 * read from devicetree; the remaining values are open-loop demonstration
 * setpoints (matching the reference pytrinamic script).
 */
#define TMC6460_NODE      DT_NODELABEL(tmc6460)
#define N_POLE_PAIRS      DT_PROP(TMC6460_NODE, pole_pairs)
#define OPENLOOP_VELOCITY 2000
#define OPENLOOP_VOLTAGE  1500
#define RUN_TIME_MS       5000

/*
 * Ramper velocity limit. The reference pytrinamic script never writes V_MAX
 * because the eval-board firmware pre-initialises it. On a bare chip V_MAX
 * powers up as 0, which clamps the velocity ramper (V_ACTUAL stays 0, so
 * PHI_E never rotates and the motor cannot spin). Set it to the target
 * open-loop velocity so the ramper can actually ramp.
 */
#define RAMPER_V_MAX_VAL  2000U

/*
 * Ramper acceleration profile (units per pytrinamic bldc_openloop_voltage_mode.py).
 * A1/A2 are the acceleration values for the two ramp phases and A_MAX caps the
 * peak acceleration.
 */
#define RAMPER_A1_VAL    100U
#define RAMPER_A2_VAL    200U
#define RAMPER_A_MAX_VAL 100U

/*
 * FOC voltage/torque/flux limiters. A genuine cold-VS power-on leaves all of
 * these at sane defaults (U_S_MAX/UQ_UD_LIMITS = 0x7fff, TORQUE_FLUX_LIMITS =
 * 0x7fff7fff, VELOCITY_LIMIT = 0x7fffffff). A warm nSLEEP-only reset does NOT
 * restore the full POR state, so after a prior corrupting run any single one of
 * them can read 0 - and one zero limiter silently forces FOC_UQ_UD_LIMITED to 0
 * even when the others are wide open. FOC_UQ_UD then shows the request but the
 * PWM stays at center and the motor gets no drive. The reference script never
 * writes the torque/flux or velocity limiter (it relies on a fresh POR), so
 * write every limiter explicitly to make the port independent of reset depth.
 */
#define FOC_U_S_MAX_VAL            0x7FFFU
#define FOC_UQ_UD_LIMIT_VAL        0x7FFFU
#define FOC_TORQUE_FLUX_LIMITS_VAL 0x7FFF7FFFU
#define FOC_VELOCITY_LIMIT_VAL     0x7FFFFFFFU

/*
 * Register field value choices (values per TMC6460 datasheet register map)
 */
#define MOTOR_TYPE_BLDC           3U
#define MOTION_MODE_PWM_OFF       0U
#define MOTION_MODE_VOLTAGE_EXT   11U
#define RAMP_MODE_VELOCITY        1U
#define CSA_GAIN_X1               0U
#define SLEW_RATE_SR_400_V_PER_US 2U
#define LS_RES_55_MOHM            3U
#define PWM_CHOP_CENTERED         7U
#define PWM_SV_MODE_HARMONIC      1U

/*
 * Field masks/shifts
 */

/* MCC_CONFIG_MOTOR_MOTION (0x0100) */
#define TMC6460_MCC_CONFIG_MOTOR_MOTION_RAMP_USE_PHI_E_MASK  0x00008000U
#define TMC6460_MCC_CONFIG_MOTOR_MOTION_RAMP_USE_PHI_E_SHIFT 15

/* MCC_ADC_CSA_GAIN (0x00C3) */
#define TMC6460_MCC_ADC_CSA_GAIN_CSA_GAIN_MASK  0x00000003U
#define TMC6460_MCC_ADC_CSA_GAIN_CSA_GAIN_SHIFT 0

/* MCC_CONFIG_GDRV (0x0101) */
#define TMC6460_MCC_CONFIG_GDRV_USE_INTERNAL_R_REF_MASK  0x01000000U
#define TMC6460_MCC_CONFIG_GDRV_USE_INTERNAL_R_REF_SHIFT 24
#define TMC6460_MCC_CONFIG_GDRV_SLEW_RATE_MASK           0x00000003U
#define TMC6460_MCC_CONFIG_GDRV_SLEW_RATE_SHIFT          0
#define TMC6460_MCC_CONFIG_GDRV_LS_RES_ON_MASK           0x00000030U
#define TMC6460_MCC_CONFIG_GDRV_LS_RES_ON_SHIFT          4

/* MCC_CONFIG_PWM (0x0102) */
#define TMC6460_MCC_CONFIG_PWM_CHOP_MASK     0x00000007U
#define TMC6460_MCC_CONFIG_PWM_CHOP_SHIFT    0
#define TMC6460_MCC_CONFIG_PWM_SV_MODE_MASK  0x00000030U
#define TMC6460_MCC_CONFIG_PWM_SV_MODE_SHIFT 4

/* MCC_CONFIG_PWM_PERIOD (0x0103) */
#define TMC6460_MCC_CONFIG_PWM_PERIOD_MAX_COUNT_MASK  0x0000FFFFU
#define TMC6460_MCC_CONFIG_PWM_PERIOD_MAX_COUNT_SHIFT 0

/* EXT_CTRL_VOLTAGE (0x0200) */
#define TMC6460_EXT_CTRL_VOLTAGE_UD_MASK  0x0000FFFFU
#define TMC6460_EXT_CTRL_VOLTAGE_UD_SHIFT 0

/* CLK_CTRL_CONFIG (0x0040) fields */
#define TMC6460_CLK_CTRL_CONFIG_COMMIT_MASK      0x00000001U
#define TMC6460_CLK_CTRL_CONFIG_COMMIT_SHIFT     0
#define TMC6460_CLK_CTRL_CONFIG_PLL_SRC_MASK     0x00000002U
#define TMC6460_CLK_CTRL_CONFIG_PLL_SRC_SHIFT    1
#define TMC6460_CLK_CTRL_CONFIG_PLL_EN_MASK      0x00000004U
#define TMC6460_CLK_CTRL_CONFIG_PLL_EN_SHIFT     2
#define TMC6460_CLK_CTRL_CONFIG_ADC_CLK_EN_MASK  0x00000008U
#define TMC6460_CLK_CTRL_CONFIG_ADC_CLK_EN_SHIFT 3
#define TMC6460_CLK_CTRL_CONFIG_PWM_CLK_EN_MASK  0x00000010U
#define TMC6460_CLK_CTRL_CONFIG_PWM_CLK_EN_SHIFT 4
#define TMC6460_CLK_CTRL_CONFIG_CLK_FSM_EN_MASK  0x00000020U
#define TMC6460_CLK_CTRL_CONFIG_CLK_FSM_EN_SHIFT 5
#define TMC6460_CLK_CTRL_CONFIG_CLOCK_DIVIDER_MASK  0x00001F00U
#define TMC6460_CLK_CTRL_CONFIG_CLOCK_DIVIDER_SHIFT 8

/*
 * ADC_CONFIG (0x0080) - authoritative field map (pytrinamic TMC6460map.py).
 * Only three fields exist in this register:
 *   NRST_ADC_0     bit 2      active-low reset for current-sense ADC 0
 *   NRST_ADC_1     bit 3      active-low reset for current-sense ADC 1
 *   CSA_AZ_FLT_EXP bits 8-11  current-sense-amplifier auto-zero filter exponent
 * Bits 0 and 1 are NOT defined. Every ADC reset must therefore be a
 * read-modify-write that changes only NRST (bits 2,3) and preserves
 * CSA_AZ_FLT_EXP - a blind write that zeroes the auto-zero filter stops the
 * delta-sigma modulators from ever reaching READY, which keeps ADC_FAIL
 * asserted and blocks the gate driver.
 */
#define TMC6460_ADC_CONFIG_NRST_ADC_0_MASK  0x00000004U
#define TMC6460_ADC_CONFIG_NRST_ADC_0_SHIFT 2
#define TMC6460_ADC_CONFIG_NRST_ADC_1_MASK  0x00000008U
#define TMC6460_ADC_CONFIG_NRST_ADC_1_SHIFT 3
#define TMC6460_ADC_CONFIG_NRST_BOTH_MASK \
	(TMC6460_ADC_CONFIG_NRST_ADC_0_MASK | TMC6460_ADC_CONFIG_NRST_ADC_1_MASK)
#define TMC6460_ADC_CONFIG_CSA_AZ_FLT_EXP_MASK  0x00000F00U
#define TMC6460_ADC_CONFIG_CSA_AZ_FLT_EXP_SHIFT 8

/* ADC_STATUS (0x008A) - ADC_0_READY & ADC_1_READY drive ADC_FAIL_STATUS */
#define TMC6460_ADC_STATUS_ADC_0_READY_MASK 0x00000001U
#define TMC6460_ADC_STATUS_ADC_1_READY_MASK 0x00000002U

/*
 * Convenience field descriptors
 */
#define FIELD_MOTOR_TYPE                                                    \
	TMC6460_FIELD(TMC6460_MCC_CONFIG_MOTOR_MOTION,                     \
		      TMC6460_MCC_CONFIG_MOTOR_MOTION_MOTOR_TYPE_MASK,     \
		      TMC6460_MCC_CONFIG_MOTOR_MOTION_MOTOR_TYPE_SHIFT, false)

#define FIELD_N_POLE_PAIRS                                                  \
	TMC6460_FIELD(TMC6460_MCC_CONFIG_MOTOR_MOTION,                     \
		      TMC6460_MCC_CONFIG_MOTOR_MOTION_N_POLE_PAIRS_MASK,   \
		      TMC6460_MCC_CONFIG_MOTOR_MOTION_N_POLE_PAIRS_SHIFT, false)

#define FIELD_MOTION_MODE                                                   \
	TMC6460_FIELD(TMC6460_MCC_CONFIG_MOTOR_MOTION,                     \
		      TMC6460_MCC_CONFIG_MOTOR_MOTION_MOTION_MODE_MASK,    \
		      TMC6460_MCC_CONFIG_MOTOR_MOTION_MOTION_MODE_SHIFT, false)

#define FIELD_RAMP_MODE                                                     \
	TMC6460_FIELD(TMC6460_MCC_CONFIG_MOTOR_MOTION,                     \
		      TMC6460_MCC_CONFIG_MOTOR_MOTION_RAMP_MODE_MASK,      \
		      TMC6460_MCC_CONFIG_MOTOR_MOTION_RAMP_MODE_SHIFT, false)

#define FIELD_RAMP_EN                                                       \
	TMC6460_FIELD(TMC6460_MCC_CONFIG_MOTOR_MOTION,                     \
		      TMC6460_MCC_CONFIG_MOTOR_MOTION_RAMP_EN_MASK,        \
		      TMC6460_MCC_CONFIG_MOTOR_MOTION_RAMP_EN_SHIFT, false)

#define FIELD_RAMP_USE_PHI_E                                                \
	TMC6460_FIELD(TMC6460_MCC_CONFIG_MOTOR_MOTION,                     \
		      TMC6460_MCC_CONFIG_MOTOR_MOTION_RAMP_USE_PHI_E_MASK, \
		      TMC6460_MCC_CONFIG_MOTOR_MOTION_RAMP_USE_PHI_E_SHIFT, false)

#define FIELD_CHARGE_PUMP_EN                                                \
	TMC6460_FIELD(TMC6460_MCC_CONFIG_GDRV,                             \
		      TMC6460_MCC_CONFIG_GDRV_CHARGE_PUMP_EN_MASK,         \
		      TMC6460_MCC_CONFIG_GDRV_CHARGE_PUMP_EN_SHIFT, false)

/*
 * Clock source selection.
 *
 * The TMC6460 generates its internal 1 MHz PLL reference by dividing the
 * selected source clock with CLOCK_DIVIDER. The eval board firmware uses:
 *   - internal 15 MHz oscillator -> CLOCK_DIVIDER = 15 - 1 = 14
 *   - external 16 MHz clock (CLK_IN pin) -> CLOCK_DIVIDER = 16 - 1 = 15
 *
 * A bare Nucleo board provides no external clock, so use the internal
 * oscillator. The chip's power-on default leaves CLOCK_DIVIDER at 0, so the
 * 1 MHz reference never appears and the PLL cannot lock (CLK_1M0_TIMEOUT and
 * CLK_STUCK get set). Writing a valid divider is what makes the motor run.
 */
#define TMC6460_USE_EXTERNAL_CLOCK 0

#if TMC6460_USE_EXTERNAL_CLOCK
#define TMC6460_CLOCK_DIVIDER_VAL 15U /* 16 MHz / 16 = 1 MHz */
#define TMC6460_PLL_SRC_VAL       1U  /* EXT_CLK */
#else
#define TMC6460_CLOCK_DIVIDER_VAL 14U /* 15 MHz / 15 = 1 MHz */
#define TMC6460_PLL_SRC_VAL       0U  /* INT_CLK */
#endif

/**
 * @brief Initialize the TMC6460 clock system.
 *
 * The CLK_CTRL_CONFIG.COMMIT bit latches the clock configuration. At power-on
 * the chip already has COMMIT=1 with CLOCK_DIVIDER=0, so it committed an invalid
 * divide-by-zero config and the clock FSM is stuck (CLK_STUCK / CLK_1M0_TIMEOUT).
 * Simply writing a new value while COMMIT stays high does not re-latch it.
 *
 * The sequence below therefore:
 *   1. Disables the clock FSM and clears COMMIT (write 0) to reset the FSM.
 *   2. Loads CLOCK_DIVIDER + clock enables with COMMIT still low.
 *   3. Sets COMMIT (0 -> 1 edge) to latch the new configuration.
 * Each step reads the register back so we can confirm the divider actually
 * stored and the commit took effect.
 */
static int tmc6460_clock_init(const struct device *dev)
{
	uint32_t cfg;
	uint32_t val;
	int ret;
	int retry;

	/* Read current CLK_CTRL_CONFIG to see the power-on defaults */
	ret = tmc6460_read(dev, TMC6460_CLK_CTRL_CONFIG, &val);
	if (ret != 0) {
		return ret;
	}
	LOG_INF("CLK_CTRL_CONFIG default = 0x%08x", val);

	/* Step 1: Disable the FSM and clear COMMIT to reset the stuck clock FSM */
	ret = tmc6460_write(dev, TMC6460_CLK_CTRL_CONFIG, 0U);
	if (ret != 0) {
		return ret;
	}
	k_msleep(5);
	ret = tmc6460_read(dev, TMC6460_CLK_CTRL_CONFIG, &val);
	if (ret != 0) {
		return ret;
	}
	LOG_INF("CLK_CTRL_CONFIG after disable = 0x%08x", val);

	/* Step 2: Load divider + clock enables + PLL source, COMMIT still low */
	cfg = ((uint32_t)TMC6460_CLOCK_DIVIDER_VAL
			<< TMC6460_CLK_CTRL_CONFIG_CLOCK_DIVIDER_SHIFT) |
	      TMC6460_CLK_CTRL_CONFIG_CLK_FSM_EN_MASK |
	      TMC6460_CLK_CTRL_CONFIG_PWM_CLK_EN_MASK |
	      TMC6460_CLK_CTRL_CONFIG_ADC_CLK_EN_MASK |
	      TMC6460_CLK_CTRL_CONFIG_PLL_EN_MASK |
	      ((uint32_t)TMC6460_PLL_SRC_VAL
			<< TMC6460_CLK_CTRL_CONFIG_PLL_SRC_SHIFT);

	ret = tmc6460_write(dev, TMC6460_CLK_CTRL_CONFIG, cfg);
	if (ret != 0) {
		return ret;
	}
	k_msleep(1);
	ret = tmc6460_read(dev, TMC6460_CLK_CTRL_CONFIG, &val);
	if (ret != 0) {
		return ret;
	}
	LOG_INF("CLK_CTRL_CONFIG loaded (no commit) = 0x%08x", val);
	LOG_INF("  -> CLOCK_DIVIDER read back = %u (want %u)",
		(val & TMC6460_CLK_CTRL_CONFIG_CLOCK_DIVIDER_MASK)
			>> TMC6460_CLK_CTRL_CONFIG_CLOCK_DIVIDER_SHIFT,
		(uint32_t)TMC6460_CLOCK_DIVIDER_VAL);

	/*
	 * Step 3: Set COMMIT (0 -> 1 edge) to latch the configuration. This is a
	 * fire-and-forget write, so it is not read back immediately.
	 *
	 * Committing switches the chip's clock domain (PLL enable + new divider).
	 * On the UART transport this momentarily changes the reference the chip
	 * uses to auto-baud off the sync byte, so a register read issued *during*
	 * the switch comes back with corrupted framing (UART RX timeout). The
	 * reference recovers once the PLL has locked and the clock is stable, so
	 * let the switch settle before polling and treat transient read failures
	 * during the transition as "not ready yet" instead of a hard error. On
	 * SPI these reads never fail, so the same path is safe for both buses.
	 */
	ret = tmc6460_write(dev, TMC6460_CLK_CTRL_CONFIG,
			    cfg | TMC6460_CLK_CTRL_CONFIG_COMMIT_MASK);
	if (ret != 0) {
		return ret;
	}

	/* Let the clock-domain switch settle before resuming reads */
	k_msleep(10);

	/*
	 * Poll CLK_CTRL_STATUS for PLL lock (up to ~500 ms), tolerating transient
	 * UART framing errors while the clock is still switching.
	 */
	for (retry = 0; retry < 50; retry++) {
		k_msleep(10);
		ret = tmc6460_read(dev, TMC6460_CLK_CTRL_STATUS, &val);
		if (ret != 0) {
			/*
			 * Transient read error during the clock switch (UART
			 * reference momentarily off). Keep polling; the loop
			 * bound still limits the total wait.
			 */
			continue;
		}

		if (val & TMC6460_CLK_CTRL_STATUS_PLL_READY_MASK) {
			LOG_INF("PLL locked after %d ms, STATUS=0x%08x",
				(retry + 1) * 10, val);
			return 0;
		}

		/* Check for PLL error */
		if (val & TMC6460_CLK_CTRL_STATUS_PLL_ERR_MASK) {
			LOG_ERR("PLL error! STATUS=0x%08x", val);
			return -EIO;
		}
	}

	LOG_WRN("PLL did not lock after 500ms, STATUS=0x%08x", val);
	LOG_WRN("  CLK_1M0_OK=%u PLL_ERR=%u PLL_LOCK=%u PLL_READY=%u",
		(val & 0x01U) ? 1U : 0U,
		(val & 0x10U) ? 1U : 0U,
		(val & 0x20U) ? 1U : 0U,
		(val & 0x40U) ? 1U : 0U);

	return 0;
}

/**
 * @brief Reset the on-chip ADCs so they re-initialize with a valid clock.
 *
 * When the TMC6460 powers up without a valid PLL/1 MHz reference, the ADCs
 * fail to initialize and latch ADC_FAIL, which blocks SYS_READY (and therefore
 * the gate driver). Toggling the active-low NRST_ADC_0/1 bits (assert then
 * release) forces the ADC state machines to restart now that the clock runs.
 * CHIP_EVENTS is then cleared (write-1-to-clear) to drop the latched failures.
 */
static int tmc6460_adc_reset(const struct device *dev)
{
	uint32_t cfg;
	uint32_t val;
	int ret;

	ret = tmc6460_read(dev, TMC6460_ADC_CONFIG, &cfg);
	if (ret != 0) {
		return ret;
	}
	LOG_INF("ADC_CONFIG before reset = 0x%08x (CSA_AZ_FLT_EXP=0x%x)",
		cfg,
		(cfg & TMC6460_ADC_CONFIG_CSA_AZ_FLT_EXP_MASK) >>
			TMC6460_ADC_CONFIG_CSA_AZ_FLT_EXP_SHIFT);

	/*
	 * If the current-sense ADCs are already ready (for example inherited
	 * from a prior cold power-on), leave them untouched - a needless NRST
	 * cycle can only lose that state.
	 */
	ret = tmc6460_read(dev, TMC6460_ADC_STATUS, &val);
	if (ret != 0) {
		return ret;
	}
	if ((val & TMC6460_ADC_STATUS_ADC_0_READY_MASK) &&
	    (val & TMC6460_ADC_STATUS_ADC_1_READY_MASK)) {
		LOG_INF("ADCs already ready (ADC_STATUS=0x%08x); leaving untouched",
			val);
		return 0;
	}

	/*
	 * Assert reset by clearing ONLY the NRST bits (2,3). Every other field,
	 * crucially CSA_AZ_FLT_EXP, is preserved via read-modify-write: zeroing
	 * the auto-zero filter stops the modulators from ever reaching READY.
	 */
	cfg &= ~TMC6460_ADC_CONFIG_NRST_BOTH_MASK;
	ret = tmc6460_write(dev, TMC6460_ADC_CONFIG, cfg);
	if (ret != 0) {
		return ret;
	}
	/*
	 * Hold the current-sense delta-sigma modulators in reset long enough to
	 * drain their internal integrator state before releasing them.
	 */
	k_msleep(150);

	/* Release reset by setting the NRST bits, still preserving CSA_AZ_FLT_EXP */
	cfg |= TMC6460_ADC_CONFIG_NRST_BOTH_MASK;
	ret = tmc6460_write(dev, TMC6460_ADC_CONFIG, cfg);
	if (ret != 0) {
		return ret;
	}

	/* Read back the real value the chip holds after release */
	ret = tmc6460_read(dev, TMC6460_ADC_CONFIG, &val);
	if (ret != 0) {
		return ret;
	}
	LOG_INF("ADC_CONFIG after reset  = 0x%08x (CSA_AZ_FLT_EXP=0x%x)",
		val,
		(val & TMC6460_ADC_CONFIG_CSA_AZ_FLT_EXP_MASK) >>
			TMC6460_ADC_CONFIG_CSA_AZ_FLT_EXP_SHIFT);

	/* Clear latched fail events (write 1s to clear) */
	ret = tmc6460_write(dev, TMC6460_CHIP_EVENTS, 0xFFFFFFFFU);
	if (ret != 0) {
		return ret;
	}

	/* Give the modulators time to settle before the first poll */
	k_msleep(50);

	/* Wait for both ADCs to report ready (up to ~1.5s) */
	ret = tmc6460_poll_flag(dev, TMC6460_ADC_STATUS,
				TMC6460_ADC_STATUS_ADC_0_READY_MASK |
					TMC6460_ADC_STATUS_ADC_1_READY_MASK,
				10U, 1500U);
	if (ret == 0) {
		LOG_INF("ADCs ready");
		return 0;
	}

	/*
	 * The current-sense ADCs did not reach READY. This is the genuine fault:
	 * ADC_FAIL_STATUS stays asserted and the gate driver will not turn on.
	 * No override is applied - the failure is reported honestly. If the
	 * CSA_AZ_FLT_EXP logged above reads 0, a prior blind write wiped the
	 * auto-zero filter and a cold VS power-cycle is required to restore the
	 * POR default before this (now non-destructive) firmware can succeed.
	 */
	(void)tmc6460_read(dev, TMC6460_ADC_STATUS, &val);
	LOG_ERR("ADC_0/1 never reached READY (ADC_STATUS=0x%08x, ADC_0_READY=%u ADC_1_READY=%u)",
		val,
		(val & TMC6460_ADC_STATUS_ADC_0_READY_MASK) ? 1U : 0U,
		(val & TMC6460_ADC_STATUS_ADC_1_READY_MASK) ? 1U : 0U);
	LOG_ERR("ADC_FAIL is real; the motor will not spin. Try a cold VS power-cycle.");
	return -EIO;
}

int main(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(tmc6460));
	uint32_t val;
	int ret;

	LOG_INF("TMC6460 Voltage Mode BLDC Sample");

	/* --- Board-level GPIO init --- */

	if (!gpio_is_ready_dt(&sleepn)) {
		LOG_ERR("SLEEPN GPIO not ready");
		return -ENODEV;
	}
	/*
	 * Perform an explicit nSLEEP reset pulse (low -> high) like the eval
	 * firmware does. This guarantees the TMC6460 starts from a clean
	 * power-on state before we reconfigure its clock system.
	 */
	ret = gpio_pin_configure_dt(&sleepn, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure SLEEPN: %d", ret);
		return ret;
	}
	LOG_INF("SLEEPN asserted (LOW) - resetting chip");
	/*
	 * Hold nSLEEP low long enough to reset the analog front end. nSLEEP is the
	 * only line that can clear analog state (the current-sense ADCs); a short
	 * pulse resets the digital core but can leave the modulators latched.
	 */
	k_msleep(50);
	gpio_pin_set_dt(&sleepn, 1);
	LOG_INF("SLEEPN deasserted (HIGH)");
	k_msleep(10);

	if (!gpio_is_ready_dt(&drv_en)) {
		LOG_ERR("DRV_EN GPIO not ready");
		return -ENODEV;
	}
	ret = gpio_pin_configure_dt(&drv_en, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure DRV_EN: %d", ret);
		return ret;
	}
	LOG_INF("DRV_EN asserted (HIGH)");

	/* Wait for TMC6460 to wake up */
	k_msleep(200);

	if (!device_is_ready(dev)) {
		LOG_ERR("TMC6460 device not ready");
		return -ENODEV;
	}

	/* Verify SPI communication */
	ret = tmc6460_read(dev, TMC6460_CHIP_ID, &val);
	if (ret != 0) {
		LOG_ERR("Failed to read CHIP_ID: %d", ret);
		return ret;
	}
	LOG_INF("CHIP_ID = 0x%08x", val);

	/* Check initial status */
	ret = tmc6460_read(dev, TMC6460_CHIP_STATUS_FLAGS, &val);
	if (ret == 0) {
		LOG_INF("Initial STATUS_FLAGS = 0x%08x (SYS_READY=%u, GDRV_ON=%u)",
			val,
			(val >> 30) & 1U,
			(val >> 31) & 1U);
	}

	/*
	 * Hold the ADCs in reset BEFORE touching the clock.
	 * Tearing down and re-locking the PLL while the current-sense ADCs are
	 * live can make them latch a failure. Assert NRST now so the ADCs stay in
	 * reset across the whole clock bring-up, then release them in
	 * tmc6460_adc_reset() once the PLL is stable. This is a read-modify-write
	 * that clears ONLY the NRST bits (2,3) and preserves CSA_AZ_FLT_EXP - a
	 * blind write would wipe the auto-zero filter and prevent the ADCs from
	 * ever reaching READY.
	 */
	ret = tmc6460_read(dev, TMC6460_ADC_CONFIG, &val);
	if (ret != 0) {
		LOG_ERR("Failed to read ADC_CONFIG: %d", ret);
		return ret;
	}
	val &= ~TMC6460_ADC_CONFIG_NRST_BOTH_MASK;
	ret = tmc6460_write(dev, TMC6460_ADC_CONFIG, val);
	if (ret != 0) {
		LOG_ERR("Failed to assert ADC reset: %d", ret);
		return ret;
	}
	LOG_INF("ADC held in reset for clock bring-up (ADC_CONFIG=0x%08x)", val);

	/*
	 * Clock initialization.
	 * Uses read-modify-write to preserve CLOCK_DIVIDER and other defaults.
	 */
	ret = tmc6460_clock_init(dev);
	if (ret != 0) {
		LOG_ERR("Clock init failed: %d", ret);
		return ret;
	}

	/* Check if system is ready now */
	ret = tmc6460_read(dev, TMC6460_CHIP_STATUS_FLAGS, &val);
	if (ret == 0) {
		LOG_INF("Post-clock STATUS_FLAGS = 0x%08x (SYS_READY=%u, GDRV_ON=%u)",
			val,
			(val >> 30) & 1U,
			(val >> 31) & 1U);
	}

	/*
	 * Reset the ADCs.
	 * The ADCs latched a failure while there was no valid clock at power-on.
	 * Toggle their NRST now that the PLL is locked so ADC_FAIL clears and the
	 * chip can reach SYS_READY (required for the gate driver to turn on).
	 */
	ret = tmc6460_adc_reset(dev);
	if (ret != 0) {
		LOG_ERR("ADC reset failed: %d", ret);
		return ret;
	}

	/*
	 * Gate driver + charge pump.
	 * Use read-modify-write to preserve other GDRV fields.
	 */

	/* Enable charge pump first */
	ret = tmc6460_field_write(dev, FIELD_CHARGE_PUMP_EN, 1U);
	if (ret != 0) {
		LOG_ERR("Failed to enable charge pump: %d", ret);
		return ret;
	}

	/* Wait for charge pump */
	k_msleep(50);

	/* Gate driver config */
	ret = tmc6460_field_write(dev,
			TMC6460_FIELD(TMC6460_MCC_CONFIG_GDRV,
				      TMC6460_MCC_CONFIG_GDRV_USE_INTERNAL_R_REF_MASK,
				      TMC6460_MCC_CONFIG_GDRV_USE_INTERNAL_R_REF_SHIFT, false),
			0U);
	if (ret != 0) {
		LOG_ERR("Failed to set R_REF: %d", ret);
		return ret;
	}

	ret = tmc6460_field_write(dev,
			TMC6460_FIELD(TMC6460_MCC_CONFIG_GDRV,
				      TMC6460_MCC_CONFIG_GDRV_SLEW_RATE_MASK,
				      TMC6460_MCC_CONFIG_GDRV_SLEW_RATE_SHIFT, false),
			SLEW_RATE_SR_400_V_PER_US);
	if (ret != 0) {
		LOG_ERR("Failed to set SLEW_RATE: %d", ret);
		return ret;
	}

	ret = tmc6460_field_write(dev,
			TMC6460_FIELD(TMC6460_MCC_CONFIG_GDRV,
				      TMC6460_MCC_CONFIG_GDRV_LS_RES_ON_MASK,
				      TMC6460_MCC_CONFIG_GDRV_LS_RES_ON_SHIFT, false),
			LS_RES_55_MOHM);
	if (ret != 0) {
		LOG_ERR("Failed to set LS_RES: %d", ret);
		return ret;
	}

	/* Enable gate driver */
	ret = tmc6460_enable(dev);
	if (ret != 0) {
		LOG_ERR("Failed to enable gate driver: %d", ret);
		return ret;
	}

	/* Wait and check status */
	k_msleep(50);
	ret = tmc6460_read(dev, TMC6460_CHIP_STATUS_FLAGS, &val);
	if (ret == 0) {
		LOG_INF("Post-GDRV STATUS_FLAGS = 0x%08x (SYS_READY=%u, GDRV_ON=%u)",
			val,
			(val >> 30) & 1U,
			(val >> 31) & 1U);
	}

	ret = tmc6460_read(dev, TMC6460_MCC_CONFIG_GDRV, &val);
	if (ret == 0) {
		LOG_INF("GDRV = 0x%08x", val);
	}

	/*
	 * Motor configuration (matches pytrinamic script exactly).
	 */
	LOG_INF("Configuring motor settings...");

	ret = tmc6460_field_write(dev, FIELD_MOTOR_TYPE, MOTOR_TYPE_BLDC);
	if (ret != 0) {
		LOG_ERR("Failed to set MOTOR_TYPE: %d", ret);
		return ret;
	}

	ret = tmc6460_field_write(dev, FIELD_N_POLE_PAIRS, N_POLE_PAIRS);
	if (ret != 0) {
		LOG_ERR("Failed to set N_POLE_PAIRS: %d", ret);
		return ret;
	}

	/* ADC and CSA setup */
	ret = tmc6460_field_write(dev,
			TMC6460_FIELD(TMC6460_MCC_ADC_CSA_GAIN,
				      TMC6460_MCC_ADC_CSA_GAIN_CSA_GAIN_MASK,
				      TMC6460_MCC_ADC_CSA_GAIN_CSA_GAIN_SHIFT, false),
			CSA_GAIN_X1);
	if (ret != 0) {
		LOG_ERR("Failed to set CSA_GAIN: %d", ret);
		return ret;
	}

	/* PWM setup */
	ret = tmc6460_field_write(dev,
			TMC6460_FIELD(TMC6460_MCC_CONFIG_PWM_PERIOD,
				      TMC6460_MCC_CONFIG_PWM_PERIOD_MAX_COUNT_MASK,
				      TMC6460_MCC_CONFIG_PWM_PERIOD_MAX_COUNT_SHIFT, false),
			4800U);
	if (ret != 0) {
		LOG_ERR("Failed to set PWM_PERIOD: %d", ret);
		return ret;
	}

	ret = tmc6460_field_write(dev,
			TMC6460_FIELD(TMC6460_MCC_CONFIG_PWM,
				      TMC6460_MCC_CONFIG_PWM_SV_MODE_MASK,
				      TMC6460_MCC_CONFIG_PWM_SV_MODE_SHIFT, false),
			PWM_SV_MODE_HARMONIC);
	if (ret != 0) {
		LOG_ERR("Failed to set SV_MODE: %d", ret);
		return ret;
	}

	ret = tmc6460_field_write(dev,
			TMC6460_FIELD(TMC6460_MCC_CONFIG_PWM,
				      TMC6460_MCC_CONFIG_PWM_CHOP_MASK,
				      TMC6460_MCC_CONFIG_PWM_CHOP_SHIFT, false),
			PWM_CHOP_CENTERED);
	if (ret != 0) {
		LOG_ERR("Failed to set CHOP: %d", ret);
		return ret;
	}

	/* Ramper setup */
	ret = tmc6460_field_write(dev, FIELD_RAMP_MODE, RAMP_MODE_VELOCITY);
	if (ret != 0) {
		LOG_ERR("Failed to set RAMP_MODE: %d", ret);
		return ret;
	}

	ret = tmc6460_field_write(dev, FIELD_RAMP_USE_PHI_E, 1U);
	if (ret != 0) {
		LOG_ERR("Failed to set RAMP_USE_PHI_E: %d", ret);
		return ret;
	}

	ret = tmc6460_field_write(dev, FIELD_RAMP_EN, 1U);
	if (ret != 0) {
		LOG_ERR("Failed to set RAMP_EN: %d", ret);
		return ret;
	}

	ret = tmc6460_write(dev, TMC6460_RAMPER_A1, RAMPER_A1_VAL);
	if (ret != 0) {
		LOG_ERR("Failed to set A1: %d", ret);
		return ret;
	}

	ret = tmc6460_write(dev, TMC6460_RAMPER_A2, RAMPER_A2_VAL);
	if (ret != 0) {
		LOG_ERR("Failed to set A2: %d", ret);
		return ret;
	}

	ret = tmc6460_write(dev, TMC6460_RAMPER_A_MAX, RAMPER_A_MAX_VAL);
	if (ret != 0) {
		LOG_ERR("Failed to set A_MAX: %d", ret);
		return ret;
	}

	/*
	 * The reference pytrinamic script never writes V_MAX because the eval
	 * firmware pre-initialises it; on a bare chip it can be 0, which clamps the
	 * velocity ramper to a standstill. Write it explicitly.
	 */
	ret = tmc6460_write(dev, TMC6460_RAMPER_V_MAX, RAMPER_V_MAX_VAL);
	if (ret != 0) {
		LOG_ERR("Failed to set V_MAX: %d", ret);
		return ret;
	}

	/*
	 * FOC voltage/torque/flux limiters. The reference script never writes the
	 * torque/flux or velocity limiter (it relies on a fresh power-on default),
	 * so write every limiter explicitly to make the sample independent of reset
	 * depth: one limiter left at 0 by a warm nSLEEP-only reset would silently
	 * force FOC_UQ_UD_LIMITED to 0 and the motor would get no drive.
	 */
	ret = tmc6460_write(dev, TMC6460_FOC_PID_U_S_MAX, FOC_U_S_MAX_VAL);
	if (ret != 0) {
		LOG_ERR("Failed to set U_S_MAX: %d", ret);
		return ret;
	}

	ret = tmc6460_write(dev, TMC6460_FOC_PID_UQ_UD_LIMITS, FOC_UQ_UD_LIMIT_VAL);
	if (ret != 0) {
		LOG_ERR("Failed to set UQ_UD_LIMITS: %d", ret);
		return ret;
	}

	ret = tmc6460_write(dev, TMC6460_FOC_PID_TORQUE_FLUX_LIMITS,
			    FOC_TORQUE_FLUX_LIMITS_VAL);
	if (ret != 0) {
		LOG_ERR("Failed to set TORQUE_FLUX_LIMITS: %d", ret);
		return ret;
	}

	ret = tmc6460_write(dev, TMC6460_FOC_PID_VELOCITY_LIMIT,
			    FOC_VELOCITY_LIMIT_VAL);
	if (ret != 0) {
		LOG_ERR("Failed to set VELOCITY_LIMIT: %d", ret);
		return ret;
	}

	/* Enable voltage control mode */
	ret = tmc6460_write(dev, TMC6460_EXT_CTRL_VOLTAGE, 0U);
	if (ret != 0) {
		LOG_ERR("Failed to clear VOLTAGE: %d", ret);
		return ret;
	}

	ret = tmc6460_field_write(dev, FIELD_MOTION_MODE, MOTION_MODE_VOLTAGE_EXT);
	if (ret != 0) {
		LOG_ERR("Failed to set MOTION_MODE: %d", ret);
		return ret;
	}

	/*
	 * Clear any latched bring-up events (ADC/CP/UV fire while the clock and
	 * ADCs are initialising) so the register starts clean and any new fault
	 * that trips during motion is unambiguous.
	 */
	ret = tmc6460_write(dev, TMC6460_CHIP_EVENTS, 0xFFFFFFFFU);
	if (ret != 0) {
		LOG_ERR("Failed to clear events: %d", ret);
		return ret;
	}

	/* Start motor */
	LOG_INF("Turning motor...");
	ret = tmc6460_field_write(dev,
			TMC6460_FIELD(TMC6460_EXT_CTRL_VOLTAGE,
				      TMC6460_EXT_CTRL_VOLTAGE_UD_MASK,
				      TMC6460_EXT_CTRL_VOLTAGE_UD_SHIFT, true),
			(uint32_t)(uint16_t)OPENLOOP_VOLTAGE);
	if (ret != 0) {
		LOG_ERR("Failed to set VOLTAGE UD: %d", ret);
		return ret;
	}

	ret = tmc6460_write(dev, TMC6460_FOC_PID_VELOCITY_TARGET,
			    (uint32_t)OPENLOOP_VELOCITY);
	if (ret != 0) {
		LOG_ERR("Failed to set PID_VELOCITY_TARGET: %d", ret);
		return ret;
	}

	/* Let the motor run */
	k_msleep(RUN_TIME_MS);

	ret = tmc6460_read(dev, TMC6460_RAMPER_V_ACTUAL, &val);
	if (ret == 0) {
		LOG_INF("RAMPER_V_ACTUAL = %d", (int32_t)val);
	}

	/* Stop motor */
	LOG_INF("Stopping motor...");
	ret = tmc6460_write(dev, TMC6460_FOC_PID_VELOCITY_TARGET, 0U);
	if (ret != 0) {
		LOG_ERR("Failed to clear VELOCITY: %d", ret);
		return ret;
	}

	ret = tmc6460_write(dev, TMC6460_EXT_CTRL_VOLTAGE, 0U);
	if (ret != 0) {
		LOG_ERR("Failed to clear VOLTAGE: %d", ret);
		return ret;
	}

	k_msleep(1000);

	/* Turn system off */
	LOG_INF("Turning system off...");
	ret = tmc6460_field_write(dev, FIELD_MOTION_MODE, MOTION_MODE_PWM_OFF);
	if (ret != 0) {
		LOG_ERR("Failed to set PWM_OFF: %d", ret);
		return ret;
	}

	ret = tmc6460_disable(dev);
	if (ret != 0) {
		LOG_ERR("Failed to disable gate driver: %d", ret);
		return ret;
	}

	LOG_INF("Done.");
	return 0;
}

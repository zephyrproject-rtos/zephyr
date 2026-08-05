/*
 * Copyright (c) 2026 Gabriel Germano
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rp2350_powman.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <hardware/structs/powman.h>
#include <hardware/powman.h>
#include <hardware/regs/powman.h>
#include <hardware/platform_defs.h>
#include <hardware/clocks.h>
#include <hardware/regs/clocks.h>
#include <hardware/regs/otp_data.h>

#include <pico/bootrom.h>
#include <boot/bootrom_constants.h>

LOG_MODULE_REGISTER(rp2350_powman, CONFIG_SOC_LOG_LEVEL);

/* POWMAN_LAST_SWCORE_PWRUP is a 0-6 enum (1=pwrup0, 6=alarm_pwrup), not a bitmask.
 * https://github.com/zephyrproject-rtos/hal_rpi_pico/blob/266394b0f23dff8f77c54f67f89d62cee246f21d/src/rp2350/hardware_regs/include/hardware/regs/powman.h#L1842-L1859
 */
#define RP2350_LAST_SWCORE_PWRUP_GPIO0 1u
#define RP2350_LAST_SWCORE_PWRUP_ALARM 6u

#define RP2350_LPOSC_NOMINAL_HZ 32768u

/* Cached LPOSC frequency; 0 = not yet measured. */
static uint32_t lposc_freq_hz;

/* Factory-trimmed LPOSC frequency (Hz), read from OTP; 0 if unavailable.
 * Cheaper than frequency_count_khz(), but reflects room-temperature
 * manufacturing conditions rather than the current ones.
 */
static uint32_t rp2350_lposc_freq_from_otp(void)
{
	uint16_t otp_val = 0;
	otp_cmd_t cmd = {.flags = OTP_DATA_LPOSC_CALIB_ROW | OTP_CMD_ECC_BITS};

	if (rom_func_otp_access((uint8_t *)&otp_val, sizeof(otp_val), cmd) != BOOTROM_OK) {
		return 0;
	}

	return otp_val;
}

void rp2350_powman_timer_init(void)
{
	if (lposc_freq_hz == 0) {
		uint32_t khz = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_LPOSC_CLKSRC);

		if (khz != 0) {
			lposc_freq_hz = khz * 1000u;
			LOG_INF("LPOSC calibrated: %u Hz (%u kHz)", lposc_freq_hz, khz);
		} else {
			lposc_freq_hz = rp2350_lposc_freq_from_otp();
			if (lposc_freq_hz != 0) {
				LOG_WRN("LPOSC frequency counter failed; using OTP "
					"factory calibration: %u Hz", lposc_freq_hz);
			} else {
				LOG_ERR("LPOSC frequency measured as 0 and OTP calibration "
					"unavailable; using %u Hz nominal",
					RP2350_LPOSC_NOMINAL_HZ);
				lposc_freq_hz = RP2350_LPOSC_NOMINAL_HZ;
			}
		}
	}

	/* Only reprogram the tick source if it isn't already LPOSC, so repeated
	 * calls don't disturb a running timer.
	 */
	if (!(powman_hw->timer & POWMAN_TIMER_USING_LPOSC_BITS)) {
		powman_timer_set_1khz_tick_source_lposc_with_hz(lposc_freq_hz);
	}

	if (!powman_timer_is_running()) {
		powman_timer_start();
	}

	WAIT_FOR(powman_hw->timer & POWMAN_TIMER_USING_LPOSC_BITS, 10000, k_usleep(100));

	if (!(powman_hw->timer & POWMAN_TIMER_USING_LPOSC_BITS)) {
		LOG_ERR("Timeout waiting for POWMAN LPOSC tick source, proceeding anyway");
	}
}

void rp2350_powman_arm_alarm(uint32_t seconds)
{
	rp2350_powman_timer_init();

	uint64_t wake_at_ms = powman_timer_get_ms() + ((uint64_t)seconds * MSEC_PER_SEC);

	powman_enable_alarm_wakeup_at_ms(wake_at_ms);
}

bool rp2350_powman_arm_gpio_wakeup(uint32_t gpio, bool wake_on_high)
{
	if (gpio >= NUM_BANK0_GPIOS) {
		return false;
	}

	/* Edge-detect: level mode blocks the power-down if the line rests at the wake level. */
	powman_enable_gpio_wakeup(0, gpio, true, wake_on_high);

	return true;
}

void rp2350_powman_disarm_wakeups(void)
{
	powman_disable_all_wakeups();
}

enum rp2350_pm_wakeup_source rp2350_powman_wakeup_source(void)
{
	uint32_t pwrup = powman_hw->last_swcore_pwrup;

	if (pwrup == RP2350_LAST_SWCORE_PWRUP_GPIO0) {
		return RP2350_PM_WAKEUP_GPIO;
	}
	if (pwrup == RP2350_LAST_SWCORE_PWRUP_ALARM) {
		return RP2350_PM_WAKEUP_ALARM;
	}

	return RP2350_PM_WAKEUP_NONE;
}

bool rp2350_powman_had_powerdown(void)
{
	return (powman_hw->chip_reset & POWMAN_CHIP_RESET_HAD_SWCORE_PD_BITS) != 0;
}

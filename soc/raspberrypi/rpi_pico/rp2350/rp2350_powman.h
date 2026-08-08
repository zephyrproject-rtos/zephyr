/*
 * Copyright (c) 2026 Gabriel Germano
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_RASPBERRYPI_RPI_PICO_RP2350_POWMAN_H_
#define SOC_RASPBERRYPI_RPI_PICO_RP2350_POWMAN_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum rp2350_pm_wakeup_source {
	RP2350_PM_WAKEUP_NONE = 0,
	RP2350_PM_WAKEUP_ALARM,
	RP2350_PM_WAKEUP_GPIO,
};

/* The RP2350 LPOSC is nominally ~32 kHz but well off in practice; this
 * measures its real frequency once and calibrates the POWMAN timer to it.
 */
void rp2350_powman_timer_init(void);

void rp2350_powman_arm_alarm(uint32_t seconds);

/* Returns false if gpio is out of range for this chip; nothing armed. */
bool rp2350_powman_arm_gpio_wakeup(uint32_t gpio, bool wake_on_high);

void rp2350_powman_disarm_wakeups(void);

enum rp2350_pm_wakeup_source rp2350_powman_wakeup_source(void);

bool rp2350_powman_had_powerdown(void);

#ifdef __cplusplus
}
#endif

#endif /* SOC_RASPBERRYPI_RPI_PICO_RP2350_POWMAN_H_ */

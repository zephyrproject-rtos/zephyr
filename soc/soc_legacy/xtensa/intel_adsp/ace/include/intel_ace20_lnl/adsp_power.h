/*
 * Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_INTEL_ADSP_POWER_H_
#define ZEPHYR_SOC_INTEL_ADSP_POWER_H_

#include <stdint.h>
#include <zephyr/toolchain.h>
#include <zephyr/sys/util_macro.h>

/* Value used as delay when waiting for hw register state change. */
#define HW_STATE_CHECK_DELAY  64


struct ace_pwrctl {
	uint16_t wpdsphpxpg : 5;
	uint16_t wphstpg    : 1;
	uint16_t wphubhppg  : 1;
	uint16_t wpdspulppg : 1;
	uint16_t wpioxpg    : 2;
	uint16_t rsvd11     : 2;
	uint16_t wpmlpg     : 1;
	uint16_t rsvd14     : 2;
	uint16_t phubulppg  : 1;
};

/* Power control */
#define PWRCTL_REG 0x71b90

#define ACE_PWRCTL ((volatile struct ace_pwrctl *)PWRCTL_REG)

#define PWRSTS_REG 0x71b92

struct ace_pwrsts {
	uint16_t dsphpxpgs : 5;
	uint16_t hstpgs    : 1;
	uint16_t hubhppgs  : 1;
	uint16_t dspulppgs : 1;
	uint16_t ioxpgs    : 4;
	uint16_t mlpgs     : 2;
	uint16_t rsvd14    : 1;
	uint16_t hubulppgs : 1;
};

#define ACE_PWRSTS ((volatile struct ace_pwrsts *)PWRSTS_REG)

/**
 * @brief Power up a specific CPU.
 *
 * This sets the "not power gating" bit in the power control
 * register to disable power gating to CPU, thus powering up
 * the CPU.
 *
 * @param cpu_num CPU to be powered up.
 */
static ALWAYS_INLINE void soc_cpu_power_up(int cpu_num)
{
	ACE_PWRCTL->wpdsphpxpg |= BIT(cpu_num);
}

/**
 * @brief Power down a specific CPU.
 *
 * This clears the "not power gating" bit in the power control
 * register to enable power gating to CPU, thus powering down
 * the CPU.
 *
 * @param cpu_num CPU to be powered down.
 */
static ALWAYS_INLINE void soc_cpu_power_down(int cpu_num)
{
	ACE_PWRCTL->wpdsphpxpg &= ~BIT(cpu_num);
}

/**
 * @brief Test if a CPU is currently powered.
 *
 * This queries the power status register to see if the CPU
 * is currently powered.
 *
 * @param cpu_num CPU to be queried.
 * @return True if CPU is powered, false if now.
 */
static ALWAYS_INLINE bool soc_cpu_is_powered(int cpu_num)
{
	return (ACE_PWRSTS->dsphpxpgs & BIT(cpu_num)) == BIT(cpu_num);
}

/**
 * @brief Retrieve node identifier for Intel ADSP HOST power domain.
 */
#define INTEL_ADSP_HST_DOMAIN_DTNODE DT_NODELABEL(hst_domain)

/**
 * @brief Intel ADSP HOST power domain pointer.
 */
#define INTEL_ADSP_HST_DOMAIN_DEV DEVICE_DT_GET(INTEL_ADSP_HST_DOMAIN_DTNODE)

#endif /* ZEPHYR_SOC_INTEL_ADSP_POWER_H_ */

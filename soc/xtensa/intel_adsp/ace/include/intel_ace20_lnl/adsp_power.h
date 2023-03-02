/*
 * Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_INTEL_ADSP_POWER_H_
#define ZEPHYR_SOC_INTEL_ADSP_POWER_H_

/* Value used as delay when waiting for hw register state change. */
#define HW_STATE_CHECK_DELAY 256


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

#endif /* ZEPHYR_SOC_INTEL_ADSP_POWER_H_ */

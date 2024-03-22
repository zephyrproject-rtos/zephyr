/*
 * Copyright (c) 2021 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_ARM_NXP_IMX_RT_POWER_RT10XX_H_
#define _SOC_ARM_NXP_IMX_RT_POWER_RT10XX_H_

struct clock_callbacks {
	/*
	 * The clock_set_run function should switch all clocks to their
	 * default running configuration, and power up required PLLs
	 * The system will be transitioning out of sleep when calling this function.
	 */
	void (*clock_set_run)(void);
	 /*
	  * The clock_set_low_power function should switch all clocks to
	  * the lowest power mode possible, and disable all possible PLLs.
	  * Note that the function should, at minimum, switch the arm core to the
	  * 24 MHz oscillator.
	  * The system will transition to sleep after calling this function.
	  */
	void (*clock_set_low_power)(void);
	/*
	 * The clock_init function should perform any one time initialization that
	 * clocks used in low power mode require.
	 */
	void (*clock_lpm_init)(void);
};

void imxrt_clock_pm_callbacks_register(struct clock_callbacks *callbacks);

#endif /* _SOC_ARM_NXP_IMX_RT_POWER_RT10XX_H_ */

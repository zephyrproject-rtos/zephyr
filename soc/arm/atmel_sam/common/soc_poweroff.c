/*
 * Copyright (c) 2023 Bjarki Arge Andreasen
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/poweroff.h>
#include <soc.h>

/*
 * Poweroff will make the chip enter the backup low-power mode, which
 * achieves the lowest possible power consumption. Wakeup from this mode
 * requires enabling a wakeup source or input, or power cycling the device.
 */

static void soc_core_sleepdeep_enable(void)
{
	SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
}

static void soc_core_sleepdeep_wait(void)
{
	__WFE();
	__WFI();
}

void z_sys_poweroff(void)
{
	soc_core_sleepdeep_enable();
	soc_supc_core_voltage_regulator_off();
	soc_core_sleepdeep_wait();

	CODE_UNREACHABLE;
}

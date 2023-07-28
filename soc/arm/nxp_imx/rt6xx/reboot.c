/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/reboot.h>
#include <zephyr/arch/arm/aarch32/cortex_m/cmsis.h>
#include <zephyr/toolchain.h>

#include <fsl_common.h>

void z_sys_reboot(enum sys_reboot_mode mode)
{
	ARG_UNUSED(mode);

	/*
	 * On the RT685, the warm reset will not complete correctly unless the
	 * ROM toggles the flash reset pin. We can control this behavior using
	 * the OTP shadow register for OPT word BOOT_CFG1
	 *
	 * Set FLEXSPI_RESET_PIN_ENABLE=1, FLEXSPI_RESET_PIN= PIO2_12
	 */
	OCOTP->OTP_SHADOW[97] = 0x314000;

	NVIC_SystemReset();

	CODE_UNREACHABLE;
}

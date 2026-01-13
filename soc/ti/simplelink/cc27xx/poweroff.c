/*
 * Copyright (c) 2024 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/poweroff.h>
#include <zephyr/toolchain.h>

#include <ti/drivers/Power.h>

FUNC_NORETURN void z_sys_poweroff(void)
{
	Power_shutdown(0, 0);

	CODE_UNREACHABLE;
}

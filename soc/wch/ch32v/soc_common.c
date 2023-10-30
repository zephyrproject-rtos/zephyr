/*
 * Copyright (c) 2023-2024 Chen Xingyu <hi@xingrz.me>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <soc.h>

/* CH32V_SYS_R8_SAFE_ACCESS_SIG_REG */
#define SAFE_ACCESS_SIG_KEY_1 (0x57)
#define SAFE_ACCESS_SIG_KEY_2 (0xA8)

void ch32v_sys_unlock(void)
{
	sys_write8(SAFE_ACCESS_SIG_KEY_1, CH32V_SYS_R8_SAFE_ACCESS_SIG_REG);
	sys_write8(SAFE_ACCESS_SIG_KEY_2, CH32V_SYS_R8_SAFE_ACCESS_SIG_REG);
}

void ch32v_sys_relock(void)
{
	sys_write8(0x00, CH32V_SYS_R8_SAFE_ACCESS_SIG_REG);
}

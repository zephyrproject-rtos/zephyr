/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * KM0 replacements for vendor chipinfo/rom-patch routines. lib_chipinfo.a is
 * not linked: its rom-patch object drags in many unrelated symbols.
 */

#include <stdarg.h>
#include <ameba_soc.h>
#include <zephyr/sys/printk.h>

int DiagVprintf(const char *fmt, va_list ap)
{
	vprintk(fmt, ap);

	return 0;
}

int DiagVprintfNano(const char *fmt, va_list ap)
{
	vprintk(fmt, ap);

	return 0;
}

u32 DiagPrintfNano(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintk(fmt, ap);
	va_end(ap);

	return 0;
}

/* Chip cut (0 = A-cut). RL_VER reads 0 unless CHIP_INFO_EN holds key 0xA. */
u32 SYSCFG_RLVersion(void)
{
	static u8 rl_ver = 0xFF;

	if (rl_ver == 0xFF) {
		u32 reg = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_LSYS_SCAN_CTRL);
		u32 saved = reg & ~LSYS_MASK_CHIP_INFO_EN;

		HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_LSYS_SCAN_CTRL,
			    saved | LSYS_CHIP_INFO_EN(0xA));
		rl_ver = LSYS_GET_RL_VER(HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_LSYS_SCAN_CTRL));
		HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_LSYS_SCAN_CTRL, saved);
	}

	return rl_ver;
}

u8 EFUSE_GetChipVersion(void)
{
	static u8 chip_ver = 0xFF;

	if (chip_ver == 0xFF) {
		OTP_Read8(OTP_CHIPVER, &chip_ver);
		chip_ver &= 0x1F;
	}

	return chip_ver;
}

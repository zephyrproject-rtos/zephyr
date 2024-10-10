/*
 * ofs_regs_ra.h
 *
 * Definitions of RA2 Option Function Select Registers located at 0x400/0x2400
 *
 * Copyright (c) 2023-2024 MUNIC SA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef __SOC_RENESAS_RA_COMMON_OFS_REG_RA_H__
#define __SOC_RENESAS_RA_COMMON_OFS_REG_RA_H__

#define DT_OFS_FLAG(node, bool_prop)	\
	COND_CODE_1(DT_NODE_HAS_PROP(node, bool_prop), \
			(~DT_PROP(node, bool_prop) & 1), (1))

/* Enable high speed on-chip oscillator after reset
 * 1 -> Enable
 * 0 -> Disable
 */
#define EARLY_BOOT_HOCO_EN	COND_CODE_1( \
		UTIL_AND(DT_NODE_HAS_STATUS(DT_NODELABEL(hoco), okay),	       \
			DT_PROP(DT_NODELABEL(hoco), enable_after_reset)),      \
		(1), (0))

union ra_ofs_watchdog {
	struct {
		uint16_t __r0:1;
		uint16_t WDTSTRT:1;
		uint16_t WDTTOPS:2;
		uint16_t WDTCKS	:4;
		uint16_t WDTRPES:2;
		uint16_t WDTRPSS:2;
		uint16_t WDTRSTIRQS:1;
		uint16_t __r1:1;
		uint16_t WDTSTPCTL:1;
	};
	uint16_t reg;
};

struct ofs_regs {
	/* OFS0 : Option Function Select Register 0 */
	union {
		struct {
			union ra_ofs_watchdog iwdt;
			union ra_ofs_watchdog wdt;
		};
		uint16_t wd[2];
		uint32_t ofs0;
	};
	/* OFS1 : Option Function Select Register 1 */
	union {
		struct {
			uint32_t __r0:2;
			uint32_t LVDAS:1;
			uint32_t VDSEL:3;
			uint32_t __r1:2;
			uint32_t HOCOEN:1;
			uint32_t __r2:3;
			uint32_t HOCOFRQ:3;
			uint32_t __r3:16;
			uint32_t ICSATS:1;
		};
		uint32_t ofs1;
	};
} __packed;

extern const uint8_t __ofs_regs[];
#define _OFS_REGS	(*((const struct ofs_regs *)__ofs_regs))

#endif /* __SOC_RENESAS_RA_COMMON_OFS_REG_RA_H__ */

/*
 * Copyright (c) 2018 Foundries.io Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/sections.h>
#include <zephyr/toolchain.h>
#include <zephyr/arch/cpu.h>

/* Exports */
GTEXT(_WdogInit)

/* WDOG instance base address. */
#ifdef CONFIG_SOC_OPENISA_RV32M1_RI5CY
#define WDOG_BASE 0x4002A000	/* WDOG0 */
#else
#define WDOG_BASE 0x41026000	/* WDOG1 */
#endif

/* Register offsets. */
#define WDOG_CS_OFFSET 0x00
#define WDOG_CNT_OFFSET 0x04
#define WDOG_TOVAL_OFFSET 0x08

/* Watchdog unlock key. */
#define WDOG_CNT_UNLOCK 0xD928C520

/* TOVAL must be set when disabling watchdog after reset. */
#define WDOG_TOVAL_SET 0xFFFF

/* Disable the WDOG_CS[EN] bit. */
#define WDOG_CS_EN_DISABLED ~0x80
/* Set WDOG_CS[UPDATE] bit. */
#define WDOG_CS_UPDATE_ENABLED 0x20

/*
 * Unlock and disable the watchdog, which is enabled by default.
 */
SECTION_FUNC(TEXT, _WdogInit)
	/* Disable interrupts if they're on. This is timing-sensitive code. */
	csrrc t0, mstatus, MSTATUS_IEN

	/* Get base address. */
	li t1, WDOG_BASE

	/* Unlock the watchdog. */
	li t2, WDOG_CNT_UNLOCK
	sw t2, WDOG_CNT_OFFSET(t1)

	/* Disable the watchdog. Allow updates later. */
	lw t2, WDOG_CS_OFFSET(t1)
	andi t2, t2, WDOG_CS_EN_DISABLED
	ori t2, t2, WDOG_CS_UPDATE_ENABLED
	sw t2, WDOG_CS_OFFSET(t1)

	/* This must also be done per reference manual. */
	li t2, WDOG_TOVAL_SET
	sw t2, WDOG_TOVAL_OFFSET(t1)

	/* Re-enable interrupts if they were disabled. */
	csrrs x0, mstatus, t0
	ret

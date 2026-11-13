/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief AURIX ENDINIT protection helpers
 *
 * On TC3x, writes to CPU ENDINIT protected registers (CEy, and E registers
 * such as module CLC) need the CPU watchdog ENDINIT bit of the running core
 * cleared, and writes to Safety ENDINIT protected registers (SE, such as the
 * CCU) need the safety watchdog ENDINIT bit cleared. Both are changed with a
 * password access followed by a modify access to WDTxCON0 (TC3xx UM, SCU,
 * "Password Access to WDTxCON0", "Modify Access to WDTxCON0").
 *
 * On TC4x the ENDINIT protection is replaced by PROT registers. The
 * registers written by the in-tree drivers are writable while their PROT is
 * in the reset state Init, which Zephyr does not change, so the helpers do
 * nothing there.
 *
 * Usage:
 *
 *	unsigned int key = aurix_endinit_safety_unlock();
 *	... write Safety ENDINIT protected registers ...
 *	aurix_endinit_safety_lock(key);
 */

#ifndef ZEPHYR_SOC_INFINEON_AURIX_COMMON_AURIX_ENDINIT_H_
#define ZEPHYR_SOC_INFINEON_AURIX_COMMON_AURIX_ENDINIT_H_

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/irq.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_SOC_SERIES_TC3X)

#define AURIX_SCU_BASE          0xF0036000UL
#define AURIX_SCU_WDTCPU0CON0   0x24CUL
#define AURIX_SCU_WDTCPU_STRIDE 12UL
#define AURIX_SCU_WDTSCON0      0x2A8UL

#define AURIX_WDTCON0_ENDINIT BIT(0)
#define AURIX_WDTCON0_LCK     BIT(1)
/* Bits 7:2 of PW read back inverted */
#define AURIX_WDTCON0_PW_INV  GENMASK(7, 2)

/** @brief WDTCPUyCON0 of the running core (CPU5 has CORE_ID 6 on TC3x) */
static inline mem_addr_t aurix_wdtcpu_con0(void)
{
	uint32_t core = cr_read(TRICORE_CORE_ID) & 0x7U;

	if (core == 6U) {
		core = 5U;
	}

	return AURIX_SCU_BASE + AURIX_SCU_WDTCPU0CON0 + core * AURIX_SCU_WDTCPU_STRIDE;
}

/** @brief Set or clear ENDINIT of a WDTxCON0 register */
static inline void aurix_wdtcon0_set_endinit(mem_addr_t con0, bool endinit)
{
	uint32_t val = sys_read32(con0);
	/* REL and the stored password: re-invert PW bits 7:2 */
	uint32_t rel_pw = (val & ~(AURIX_WDTCON0_ENDINIT | AURIX_WDTCON0_LCK)) ^
			  AURIX_WDTCON0_PW_INV;

	/* Password access: bits 1:0 = 01b */
	sys_write32(rel_pw | AURIX_WDTCON0_ENDINIT, con0);
	/* Modify access: LCK = 1 and the requested ENDINIT */
	sys_write32(rel_pw | AURIX_WDTCON0_LCK | (endinit ? AURIX_WDTCON0_ENDINIT : 0U), con0);

	/* Read back so the new state is in effect before the next access */
	while (((sys_read32(con0) & AURIX_WDTCON0_ENDINIT) != 0U) != endinit) {
	}
}

static inline unsigned int aurix_endinit_cpu_unlock(void)
{
	unsigned int key = irq_lock();

	aurix_wdtcon0_set_endinit(aurix_wdtcpu_con0(), false);

	return key;
}

static inline void aurix_endinit_cpu_lock(unsigned int key)
{
	aurix_wdtcon0_set_endinit(aurix_wdtcpu_con0(), true);
	irq_unlock(key);
}

static inline unsigned int aurix_endinit_safety_unlock(void)
{
	unsigned int key = irq_lock();

	aurix_wdtcon0_set_endinit(AURIX_SCU_BASE + AURIX_SCU_WDTSCON0, false);

	return key;
}

static inline void aurix_endinit_safety_lock(unsigned int key)
{
	aurix_wdtcon0_set_endinit(AURIX_SCU_BASE + AURIX_SCU_WDTSCON0, true);
	irq_unlock(key);
}

#else /* TC4x: PROT registers stay in state Init */

static inline unsigned int aurix_endinit_cpu_unlock(void)
{
	return 0U;
}

static inline void aurix_endinit_cpu_lock(unsigned int key)
{
	ARG_UNUSED(key);
}

static inline unsigned int aurix_endinit_safety_unlock(void)
{
	return 0U;
}

static inline void aurix_endinit_safety_lock(unsigned int key)
{
	ARG_UNUSED(key);
}

#endif /* CONFIG_SOC_SERIES_TC3X */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_INFINEON_AURIX_COMMON_AURIX_ENDINIT_H_ */

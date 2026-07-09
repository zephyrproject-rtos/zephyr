/*
 * Copyright (c) 2026 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_INTEL_ADSP_FATAL_BREADCRUMB_H_
#define ZEPHYR_SOC_INTEL_ADSP_FATAL_BREADCRUMB_H_

#ifndef _ASMLANGUAGE

#ifdef CONFIG_XTENSA_ADSP_FATAL_BREADCRUMB
#include <mem_window.h>
#include <zephyr/cache.h>

/**
 * ADSP_RECORD_FATAL_BREADCRUMB - record a fatal exception into HP-SRAM window0.
 *
 * @bsa:   pointer to the _xtensa_irq_bsa_t base save area
 * @cause: EXCCAUSE code for the exception
 *
 * Writes the faulting PC, exception cause and faulting address to the
 * HP-SRAM window0 uncached alias. The host SOF driver prints window0[0]
 * as "Firmware state" and window0[1] as "status/error code" on an IPC
 * timeout, making the crash PC visible in dmesg even without console or
 * mtrace. Only the first fatal exception is latched; win0[3] counts all.
 */
#define ADSP_RECORD_FATAL_BREADCRUMB(bsa, cause)				\
	do {									\
		volatile uint32_t *_bc_win =					\
			(volatile uint32_t *)sys_cache_uncached_ptr_get(	\
				(void *)HP_SRAM_WIN0_BASE);			\
		if ((_bc_win[1] >> 28) != 0xeU) {				\
			_bc_win[0] = (uint32_t)(bsa)->pc;			\
			_bc_win[1] = 0xe0000000U |				\
				((uint32_t)(cause) & 0xffU);			\
			_bc_win[2] = (uint32_t)(bsa)->excvaddr;			\
			_bc_win[4] = *(volatile uint32_t *)			\
				((uint32_t)(bsa)->pc & ~3U);			\
		}								\
		_bc_win[3]++;							\
	} while (0)

#else
#define ADSP_RECORD_FATAL_BREADCRUMB(bsa, cause) do { } while (0)
#endif /* CONFIG_XTENSA_ADSP_FATAL_BREADCRUMB */

#endif /* !_ASMLANGUAGE */

#ifdef _ASMLANGUAGE

#if defined(CONFIG_XTENSA_ADSP_TRIPLE_FAULT_BREADCRUMB)
#if defined(CONFIG_SOC_SERIES_INTEL_ADSP_ACE)
#define _ADSP_BC_WIN0_BASE_UC 0x40024000
#else
#define _ADSP_BC_WIN0_BASE_UC 0x5e024000
#endif
#endif /* CONFIG_XTENSA_ADSP_TRIPLE_FAULT_BREADCRUMB */

/*
 * adsp_triple_fault_breadcrumb - record triple fault state into HP-SRAM window0.
 *
 * Writes EPC1, EXCCAUSE|0xdb1e0000 and DEPC to the HP-SRAM window0 uncached
 * alias so the host SOF driver can diagnose a triple fault from dmesg.
 * Clobbers a2/a3.  Called from _TripleFault with no stack available.
 */
.macro adsp_triple_fault_breadcrumb
#if defined(CONFIG_XTENSA_ADSP_TRIPLE_FAULT_BREADCRUMB)
	rsr.epc1 a2
	movi a3, _ADSP_BC_WIN0_BASE_UC
	s32i a2, a3, 0
	rsr.exccause a2
	movi a3, 0xdb1e0000
	or a2, a2, a3
	movi a3, _ADSP_BC_WIN0_BASE_UC
	s32i a2, a3, 4
	rsr.depc a2
	movi a3, _ADSP_BC_WIN0_BASE_UC
	s32i a2, a3, 8
	memw
#endif /* CONFIG_XTENSA_ADSP_TRIPLE_FAULT_BREADCRUMB */
.endm

#endif /* _ASMLANGUAGE */
#endif /* ZEPHYR_SOC_INTEL_ADSP_FATAL_BREADCRUMB_H_ */

/*
 * Copyright (c) 2026 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_INTEL_ADSP_FATAL_BREADCRUMB_H_
#define ZEPHYR_SOC_INTEL_ADSP_FATAL_BREADCRUMB_H_

#ifndef _ASMLANGUAGE

#ifdef CONFIG_XTENSA_ADSP_FATAL_BREADCRUMB
#include <adsp_memory.h>
#include <mem_window.h>
#include <zephyr/cache.h>

/* Select diagnostic data written to win0[1] (host "status/error code") */
#if defined(CONFIG_XTENSA_ADSP_FATAL_BREADCRUMB_DATA_VADDR)
#define _ADSP_BC_DATA1(bsa, cause)	((uint32_t)(bsa)->excvaddr)
#elif defined(CONFIG_XTENSA_ADSP_FATAL_BREADCRUMB_DATA_A0)
#define _ADSP_BC_DATA1(bsa, cause)	((uint32_t)(bsa)->a0)
#else
#define _ADSP_BC_DATA1(bsa, cause)	(0xe0000000U | ((uint32_t)(cause) & 0xffU))
#endif

/**
 * ADSP_RECORD_FATAL_BREADCRUMB - record a fatal exception into HP-SRAM window0.
 *
 * @bsa:   pointer to the _xtensa_irq_bsa_t base save area
 * @cause: EXCCAUSE code for the exception
 *
 * Writes the faulting PC, diagnostic data and faulting address to the
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
		if (_bc_win[0] == 0U) {						\
			_bc_win[0] = (uint32_t)(bsa)->pc;			\
			_bc_win[1] = _ADSP_BC_DATA1(bsa, cause);		\
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

/*
 * adsp_triple_fault_breadcrumb - record triple fault state into HP-SRAM window0.
 *
 * Writes EPC1, EXCCAUSE|0xdb1e0000 and DEPC to the HP-SRAM window0 uncached
 * alias so the host SOF driver can diagnose a triple fault from dmesg.
 * Constructs the base address using shifts to avoid literal generation under
 * Clang's Integrated Assembler. Clobbers a2/a3/a4.
 */
.macro adsp_triple_fault_breadcrumb
#if defined(CONFIG_XTENSA_ADSP_TRIPLE_FAULT_BREADCRUMB)
	/* Construct window0 uncached base without literals/l32r (Clang IAS safe) */
#if defined(CONFIG_SOC_SERIES_INTEL_ADSP_ACE)
	movi a3, 1
	slli a3, a3, 30
#else
	movi a3, 0x5e
	slli a3, a3, 24
#endif
	movi a4, 0x240
	slli a4, a4, 8
	add a3, a3, a4
	rsr.epc1 a2
	s32i a2, a3, 0
	/* Construct 0xdb1e0000 without literals/l32r */
	movi a2, 0xd8
	slli a2, a2, 8
	movi a4, 0x1e
	add a2, a2, a4
	slli a2, a2, 16
	rsr.exccause a4
	or a2, a2, a4
	s32i a2, a3, 4
	rsr.depc a2
	s32i a2, a3, 8
	memw
#endif /* CONFIG_XTENSA_ADSP_TRIPLE_FAULT_BREADCRUMB */
.endm

#endif /* _ASMLANGUAGE */
#endif /* ZEPHYR_SOC_INTEL_ADSP_FATAL_BREADCRUMB_H_ */

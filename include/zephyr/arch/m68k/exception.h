/*
 * Copyright (c) 2026 Dimitri Varpusvuori
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief M68K exception stack frame definitions
 */

#ifndef ZEPHYR_INCLUDE_ARCH_M68K_EXCEPTION_H_
#define ZEPHYR_INCLUDE_ARCH_M68K_EXCEPTION_H_

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Common exception frame; bus and address errors add CPU-specific data. */
/** Exception frame format field shift. */
#define M68K_ESF_FORMAT_SHIFT       12U
/** Exception frame format field mask. */
#define M68K_ESF_FORMAT_MASK        0xf000U
/** Exception frame vector offset field mask. */
#define M68K_ESF_VECTOR_OFFSET_MASK 0x0fffU

/** M68K exception stack frame. */
struct arch_esf {
	/* Saved by the dispatch entry. */
	/** Data register D0. */
	uint32_t d0;
	/** Data register D1. */
	uint32_t d1;
	/** Address register A0. */
	uint32_t a0;
	/** Address register A1. */
	uint32_t a1;

	/* Pushed by the vector stub. */
	/** Exception vector number. */
	uint32_t vector;

	/* Pushed by the CPU. */
	/** Status register. */
	uint16_t sr;
	/** Program counter. */
	uint32_t pc;
#if defined(CONFIG_M68K_EXCEPTION_FRAME_HAS_FORMAT_WORD)
	/* Avoid implementation-defined C bit-field layout. */
	/** Exception frame format and vector offset. */
	uint16_t format_vector;
#endif
} __packed __aligned(ARCH_STACK_PTR_ALIGN);

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_M68K_EXCEPTION_H_ */

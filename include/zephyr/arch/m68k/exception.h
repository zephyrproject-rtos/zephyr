/*
 * Copyright (c) 2026 Dimitri Varpusvuori
 *
 * SPDX-License-Identifier: Apache-2.0
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
#define M68K_ESF_FORMAT_SHIFT       12U
#define M68K_ESF_FORMAT_MASK        0xf000U
#define M68K_ESF_VECTOR_OFFSET_MASK 0x0fffU

struct arch_esf {
	/* Saved by the dispatch entry. */
	uint32_t d0, d1, a0, a1;

	/* Pushed by the vector stub. */
	uint32_t vector;

	/* Pushed by the CPU. */
	uint16_t sr;
	uint32_t pc;
#if defined(CONFIG_M68K_EXCEPTION_FRAME_HAS_FORMAT_WORD)
	/* Avoid implementation-defined C bit-field layout. */
	uint16_t format_vector;
#endif
} __packed __aligned(ARCH_STACK_PTR_ALIGN);

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_M68K_EXCEPTION_H_ */

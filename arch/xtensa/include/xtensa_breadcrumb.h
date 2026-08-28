/*
 * Copyright (c) 2026 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_XTENSA_INCLUDE_XTENSA_BREADCRUMB_H_
#define ZEPHYR_ARCH_XTENSA_INCLUDE_XTENSA_BREADCRUMB_H_

#ifndef _ASMLANGUAGE

#include <xtensa_asm2_context.h>

#if defined(CONFIG_XTENSA_FATAL_BREADCRUMB) || defined(__DOXYGEN__)

/**
 * @brief Record a fatal exception breadcrumb.
 *
 * Weakly defined with a no-op default; an SoC that supports
 * CONFIG_XTENSA_FATAL_BREADCRUMB provides a strong override that records
 * the breadcrumb into its own SoC-specific storage.
 *
 * @param bsa Pointer to the _xtensa_irq_bsa_t base save area.
 * @param cause EXCCAUSE code for the exception.
 */
void xtensa_fatal_breadcrumb(const _xtensa_irq_bsa_t *bsa, int cause);

#define XTENSA_RECORD_FATAL_BREADCRUMB(bsa, cause) xtensa_fatal_breadcrumb(bsa, cause)

#else

#define XTENSA_RECORD_FATAL_BREADCRUMB(bsa, cause) do { } while (0)

#endif /* CONFIG_XTENSA_FATAL_BREADCRUMB || __DOXYGEN__ */

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_XTENSA_INCLUDE_XTENSA_BREADCRUMB_H_ */

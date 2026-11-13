/*
 * Copyright (c) 2026 Dimitri Varpusvuori
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/exception.h>
#include <zephyr/fatal.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

#ifdef CONFIG_EXCEPTION_DEBUG
static void m68k_esf_dump(const struct arch_esf *esf)
{
	EXCEPTION_DUMP("vector: %u  pc: 0x%08x  sr: 0x%04x",
		       (unsigned int)esf->vector, (unsigned int)esf->pc,
		       (unsigned int)esf->sr);
#if defined(CONFIG_M68K_EXCEPTION_FRAME_HAS_FORMAT_WORD)
	EXCEPTION_DUMP("format/vector: 0x%04x", (unsigned int)esf->format_vector);
#endif
}
#endif

void z_m68k_fatal_error(unsigned int reason, const struct arch_esf *esf)
{
#ifdef CONFIG_EXCEPTION_DEBUG
	if (esf != NULL) {
		m68k_esf_dump(esf);
	}
#endif

	z_fatal_error(reason, esf);
}

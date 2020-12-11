/*
 * Copyright (c) 2019-2020 Cobham Gaisler AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

FUNC_NORETURN void z_sparc_fatal_error(unsigned int reason,
				       const z_arch_esf_t *esf)
{
	if (esf != NULL) {
		LOG_ERR(" pc: %08x", esf->pc);
		LOG_ERR("npc: %08x", esf->npc);
		LOG_ERR("psr: %08x", esf->psr);
		LOG_ERR("tbr: %08x", esf->tbr);
		LOG_ERR(" sp: %08x", esf->sp);
		LOG_ERR("  y: %08x", esf->y);
	}

	z_fatal_error(reason, esf);
	CODE_UNREACHABLE;
}

FUNC_NORETURN void _Fault(const z_arch_esf_t *esf)
{
	LOG_ERR("Trap tt=0x%02x", (esf->tbr >> 4) & 0xff);

	z_sparc_fatal_error(K_ERR_CPU_EXCEPTION, esf);
}

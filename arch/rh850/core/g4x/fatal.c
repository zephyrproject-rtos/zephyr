/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/exception.h>
#include <zephyr/kernel.h>
#include <kernel_arch_data.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

#ifdef CONFIG_EXCEPTION_DEBUG
static void dump_rh850_esf(const struct arch_esf *esf)
{
	EXCEPTION_DUMP(" CTPC:  0x%08x  CTPSW:  0x%08x", esf->ctpc, esf->ctpsw);
	EXCEPTION_DUMP(" EIWR/FEWR:  0x%08x  EIPC/FEPC:   0x%08x  EIPSW/FEPSW:  0x%08x", esf->eiwr,
		       esf->eipc, esf->eipsw);
	EXCEPTION_DUMP(" LP:    0x%08x", esf->lp);

#ifdef CONFIG_FXU
	EXCEPTION_DUMP(" FXXC:  0x%08x  FXSR:   0x%08x", esf->fxxc, esf->fxsr);
#endif

#ifdef CONFIG_FPU_SHARING
	EXCEPTION_DUMP(" FPEPC: 0x%08x  FPSR:   0x%08x", esf->fpepc, esf->fpsr);
#endif

	EXCEPTION_DUMP(" r30:   0x%08x  r29:    0x%08x  r28:    0x%08x", esf->r30, esf->r29,
		       esf->r28);
	EXCEPTION_DUMP(" r27:   0x%08x  r26:    0x%08x  r25:    0x%08x", esf->r27, esf->r26,
		       esf->r25);
	EXCEPTION_DUMP(" r24:   0x%08x  r23:    0x%08x  r22:    0x%08x", esf->r24, esf->r23,
		       esf->r22);
	EXCEPTION_DUMP(" r21:   0x%08x  r20:    0x%08x  r19:    0x%08x", esf->r21, esf->r20,
		       esf->r19);
	EXCEPTION_DUMP(" r18:   0x%08x  r17:    0x%08x  r16:    0x%08x", esf->r18, esf->r17,
		       esf->r16);
	EXCEPTION_DUMP(" r15:   0x%08x  r14:    0x%08x  r13:    0x%08x", esf->r15, esf->r14,
		       esf->r13);
	EXCEPTION_DUMP(" r12:   0x%08x  r11:    0x%08x  r10:    0x%08x", esf->r12, esf->r11,
		       esf->r10);
	EXCEPTION_DUMP(" r9:    0x%08x  r8:     0x%08x  r7:     0x%08x", esf->r9, esf->r8, esf->r7);
	EXCEPTION_DUMP(" r6:    0x%08x  r5:     0x%08x  r2:     0x%08x", esf->r6, esf->r5, esf->r2);
	EXCEPTION_DUMP(" r1:    0x%08x", esf->r1);
}
#endif

void z_rh850_fatal_error(const struct arch_esf *esf, unsigned int reason)
{
#ifdef CONFIG_EXCEPTION_DEBUG
	if (esf != NULL) {
		dump_rh850_esf(esf);
	}
#endif /* CONFIG_EXCEPTION_DEBUG */

	z_fatal_error(reason, esf);
}

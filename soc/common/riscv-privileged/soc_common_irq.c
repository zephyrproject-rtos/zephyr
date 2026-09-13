/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief interrupt management code for riscv SOCs supporting the riscv
	  privileged architecture specification
 */
#include <zephyr/irq.h>

#if defined(CONFIG_RISCV_SOC_INTERRUPT_INIT)
__weak void soc_interrupt_init(void)
{
	/* ensure that all interrupts are disabled */
	(void)arch_irq_lock();

#ifdef CONFIG_RISCV_S_MODE
	csr_write(sie, 0);
#if !defined(CONFIG_64BIT) && defined(CONFIG_RISCV_ISA_EXT_SSAIA)
	csr_write(sieh, 0);
#endif
	/* sip.STIP is read-only from S-mode; clearing sie is sufficient */
#else
	csr_write(mie, 0);
	csr_write(mip, 0);
#if !defined(CONFIG_64BIT) && defined(CONFIG_RISCV_ISA_EXT_SMAIA)
	/* mie/mip are 64 bits in AIA; upper 32 bits are accessed using mieh/miph CSR for RV32 */
	csr_write(mieh, 0);
	csr_write(miph, 0);
#endif
#endif
}
#endif

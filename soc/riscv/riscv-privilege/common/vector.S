/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 * Contributors: 2018 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>

/* exports */
GTEXT(__start)

/* imports */
GTEXT(__initialize)
GTEXT(__irq_wrapper)
GTEXT(__irq_wrapper_s)

SECTION_FUNC(vectors, __start)
	.option norvc;

	/*
	 * Set mtvec (Machine Trap-Vector Base-Address Register)
	 * to __irq_wrapper.
	 */
	la t0, __irq_wrapper
	csrw mtvec, t0
#if defined(CONFIG_BOARD_HART_TO_USE) && (CONFIG_BOARD_HART_TO_USE != 0)
    /* Set up dummy vector routine to catch supervisor traps in case of
     * problems.
     */
#if defined(CONFIG_SOC_MPFS)
    /*
     * For MPFS and FU540 only allow setting STVEC on U54 as E51 does not have
     * this register. On these SOCs, not setting STVEC can cause unexpected traps...
     */
    csrr t0, mhartid
    beqz t0, skip_svect
#endif

	//* Jump to __reset */
	//tail __reset
    skip_svect:
#endif
	/* Jump to __initialize */
	tail __initialize

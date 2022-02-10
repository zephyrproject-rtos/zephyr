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

#if CONFIG_SUPERVISOR_MODE_CAPABLE
    /* Set up dummy vector routine to catch supervisor traps in case of
     * problems.
     */
#if CONFIG_HAS_MONITOR_HART && !CONFIG_MONITOR_HART_SUPERVISOR
    /*
     * For MPFS and FU540 type systems only allow setting STVEC on application
     * harts as monitor hart does not have this register. On these SOCs, 
     * not setting STVEC can cause unexpected traps...
     */
    li t1, CONFIG_MONITOR_HART_ID
    csrr t0, mhartid
    beq t0, t1, skip_svect
#endif /* CONFIG_HAS_MONITOR_HART && !CONFIG_MONITOR_HART_SUPERVISOR */
	la t0, __irq_wrapper_s
	csrw stvec, t0

    skip_svect:
#endif /* CONFIG_SUPERVISOR_MODE_CAPABLE */
	/* Jump to __initialize */
	tail __initialize

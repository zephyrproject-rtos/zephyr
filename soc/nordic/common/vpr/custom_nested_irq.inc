/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * VPR preempts by CLIC level in hardware, so nesting only needs MIE, which
 * the trap handler masks with its first instruction.
 */
.macro __custom_nested_interrupts_enable
	csrsi mstatus, MSTATUS_IEN
.endm

.macro __custom_nested_interrupts_disable
	csrci mstatus, MSTATUS_IEN
.endm

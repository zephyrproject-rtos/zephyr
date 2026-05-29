/*
 * Copyright (c) 2017-2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef CONFIG_X86_64
/* Save SSP restore on current shadow stack. It takes care of aligning the
 * shadow stack token to 8 bytes boundary. First scratch will contain current
 * SSP after the macro finishes.
 * r0 - scratch register
 * r1 - scratch register
 */
.macro save_ssp_restore_token r0, r1

rdsspd \r0
movl \r0, \r1
subl $8, \r0
andl $~0x7, \r0
wrssd \r1, (\r0)
xorl \r1, \r1
wrssd \r1, 4(\r0)

.endm

/* Discard previous SSP token created by rstorssp. It takes care of aligning
 * the SSP in case there was a hole in the shadow stack. Needs to be
 * called right after rstorssp so it can use the flags set by rstorssp.
 * r0 - scratch register
 */
.macro discard_previous_ssp_token r0
movl $2, \r0
jnc 1f
incl \r0
1:
incsspd \r0
.endm

#endif /* CONFIG_X86_64 */

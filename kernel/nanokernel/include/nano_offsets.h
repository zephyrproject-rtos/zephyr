/* nano_offsets.h - nanokernel structure member offset definitions */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _NANO_OFFSETS__H_
#define _NANO_OFFSETS__H_

/*
 * The final link step uses the symbol _OffsetAbsSyms to force the linkage of
 * offsets.o into the ELF image.
 */

GEN_ABS_SYM_BEGIN(_OffsetAbsSyms)

/* arch-agnostic tNANO structure member offsets */

GEN_OFFSET_SYM(tNANO, fiber);
GEN_OFFSET_SYM(tNANO, task);
GEN_OFFSET_SYM(tNANO, current);

#if defined(CONFIG_THREAD_MONITOR)
GEN_OFFSET_SYM(tNANO, threads);
#endif

#ifdef CONFIG_FP_SHARING
GEN_OFFSET_SYM(tNANO, current_fp);
#endif

/* size of the entire tNANO structure */

GEN_ABSOLUTE_SYM(__tNANO_SIZEOF, sizeof(tNANO));

/* arch-agnostic struct tcs structure member offsets */

GEN_OFFSET_SYM(tTCS, link);
GEN_OFFSET_SYM(tTCS, prio);
GEN_OFFSET_SYM(tTCS, flags);
GEN_OFFSET_SYM(tTCS, coopReg);   /* start of coop register set */
GEN_OFFSET_SYM(tTCS, preempReg); /* start of prempt register set */

#if defined(CONFIG_THREAD_MONITOR)
GEN_OFFSET_SYM(tTCS, next_thread);
#endif


/* size of the entire struct tcs structure */

GEN_ABSOLUTE_SYM(__tTCS_SIZEOF, sizeof(tTCS));

#endif /* _NANO_OFFSETS__H_ */

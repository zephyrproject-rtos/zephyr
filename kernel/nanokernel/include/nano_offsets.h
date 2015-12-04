/* nano_offsets.h - nanokernel structure member offset definitions */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

#ifdef CONFIG_ERRNO
GEN_OFFSET_SYM(tTCS, errno_var);
#endif

/* size of the entire struct tcs structure */

GEN_ABSOLUTE_SYM(__tTCS_SIZEOF, sizeof(tTCS));

#endif /* _NANO_OFFSETS__H_ */

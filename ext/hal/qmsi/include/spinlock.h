/*
 * Copyright (c) 2016, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL CORPORATION OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

#include "qm_soc_regs.h"
/*
 * Single, shared spinlock which can be used for synchronization between the
 * Lakemont and ARC cores.
 * The Spinlock lock size and position in RAM must be same on both cores.
 */
#if (QUARK_SE)

typedef struct {
	volatile char flag[2];
	volatile char turn;
} spinlock_t;

extern spinlock_t __esram_lock_start;
void spinlock_lock(spinlock_t *lock);
void spinlock_unlock(spinlock_t *lock);

#define QM_SPINLOCK_LOCK() spinlock_lock(&__esram_lock_start)
#define QM_SPINLOCK_UNLOCK() spinlock_unlock(&__esram_lock_start)

#else

#define QM_SPINLOCK_LOCK()
#define QM_SPINLOCK_UNLOCK()

#endif /* defined(QM_QUARK_SE) */

#endif /* __SPINLOCK_H__ */

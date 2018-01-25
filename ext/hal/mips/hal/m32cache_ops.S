/*
 * Copyright 2014-2015, Imagination Technologies Limited and/or its
 *                      affiliated group companies.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
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

.set nomips16
#include "m32cache.h"

/*
 * void m32_flush_cache (void)
 *
 * Writeback and invalidate all caches
 */
LEAF(m32_flush_cache)
	SIZE_CACHE(a1,mips_dcache_size)

	/* writeback and invalidate primary caches individually */
	lw	a2,mips_dcache_linesize
	li	a0,KSEG0_BASE
	cacheop(a0,a1,a2,Index_Writeback_Inv_D)

9:	lw	a1,mips_icache_size
	lw	a2,mips_icache_linesize
	blez	a1,9f
	li	a0,KSEG0_BASE
	cacheop(a0,a1,a2,Index_Invalidate_I)

9:	lw	a1,mips_scache_size
	lw	a2,mips_scache_linesize
	blez	a1,9f
	sync
	li	a0,KSEG0_BASE
	cacheop(a0,a1,a2,Index_Writeback_Inv_S)

9:	sync
	jr.hb	ra
END(m32_flush_cache)

/*
 * void m32_flush_dcache (void)
 *
 * Writeback and invalidate data caches only
 */
LEAF(m32_flush_dcache)
	SIZE_CACHE(a1,mips_dcache_size)

	/* writeback and invalidate primary data cache */
	lw	a2,mips_dcache_linesize
	li	a0,KSEG0_BASE
	cacheop(a0,a1,a2,Index_Writeback_Inv_D)

9:	lw	a1,mips_scache_size
	lw	a2,mips_scache_linesize
	blez	a1,9f
	sync
	li	a0,KSEG0_BASE
	cacheop(a0,a1,a2,Index_Writeback_Inv_S)

9:	sync
	jr.hb	ra
END(m32_flush_dcache)

/*
 * void m32_flush_icache (void)
 *
 * Writeback and invalidate instruction cache only
 */
LEAF(m32_flush_icache)
	SIZE_CACHE(a1,mips_icache_size)

	/* writeback and invalidate primary instruction cache */
	lw	a2,mips_icache_linesize
	li	a0,KSEG0_BASE
	cacheop(a0,a1,a2,Index_Invalidate_I)

9:	lw	a1,mips_scache_size
	blez	a1,9f
	lw	a2,mips_scache_linesize
	li	a0,KSEG0_BASE
	cacheop(a0,a1,a2,Index_Writeback_Inv_S)

9:	sync
	jr.hb	ra
END(m32_flush_icache)

/*
 * void m32_clean_cache (unsigned kva, size_t n)
 *
 * Writeback and invalidate address range in all caches
 */
LEAF(m32_clean_cache)
	SIZE_CACHE(a2,mips_dcache_linesize)
	vcacheop(a0,a1,a2,Hit_Writeback_Inv_D)

9:	lw	a2,mips_icache_linesize
	blez	a2,9f
	vcacheop(a0,a1,a2,Hit_Invalidate_I)

9:	lw	a2,mips_scache_linesize
	blez	a2,9f

	sync
	vcacheop(a0,a1,a2,Hit_Writeback_Inv_S)

9:	sync
	jr.hb	ra
END(m32_clean_cache)

/*
 * void m32_sync_icache (unsigned kva, size_t n)
 *
 * Synchronise icache and dcache for virtual address range
 */
LEAF(m32_sync_icache)
	/* check for bad size */
	PTR_ADDU	maxaddr,a0,a1
	blez	a1,9f

	/* get synci step and skip if not required */
	rdhwr	a2,$1
	PTR_ADDU	maxaddr,-1
	beqz	a2,9f

	/* ensure stores complete */
	sync

	/* align to line boundaries */
	PTR_SUBU	mask,a2,1
	not	mask
	and	addr,a0,mask
	PTR_SUBU   	addr,a2
	and	maxaddr,mask

	/* the cacheop loop */
10:	PTR_ADDU   	addr,a2
	synci	0(addr)
	bne     addr,maxaddr,10b

9:	sync
	jr.hb	ra
END(m32_sync_icache)

/*
 * void m32_clean_dcache (unsigned kva, size_t n)
 *
 * Writeback and invalidate address range in data caches
 */
LEAF(m32_clean_dcache)
	SIZE_CACHE(a2,mips_dcache_linesize)
	vcacheop(a0,a1,a2,Hit_Writeback_Inv_D)

9:	lw	a2,mips_scache_linesize
	blez	a2,9f
	sync
	vcacheop(a0,a1,a2,Hit_Writeback_Inv_S)

9:	sync
	jr.hb	ra
END(m32_clean_dcache)

/*
 * void m32_clean_dcache_nowrite (unsigned kva, size_t n)
 *
 * Invalidate (but don't writeback) address range in data caches
 * XXX Only safe if region is totally cache-line aligned.
 */
LEAF(m32_clean_dcache_nowrite)
	SIZE_CACHE(a2,mips_dcache_linesize)
	vcacheop(a0,a1,a2,Hit_Invalidate_D)

9:	lw	a2,mips_scache_linesize
	blez	a2,9f
	vcacheop(a0,a1,a2,Hit_Invalidate_S)

9:	sync
	jr.hb	ra
END(m32_clean_dcache_nowrite)

/*
 * Cache locking
 *
 * The MIPS32 cache architecture does support per-line cache locking.
 *
 * WARNING: if you lock any cache lines, then don't call the
 * mips_flush_xcache routines, because these will flush the
 * locked data out of the cache too; use only mips_clean_xcache.
 */

/*
 * void m32_lock_dcache (void *data, size_t n)
 *
 * Load and lock a block of data into the d-cache
 */
LEAF(m32_lock_dcache)
	SIZE_CACHE(a2,mips_dcache_linesize)
	vcacheop(a0,a1,a2,Fetch_Lock_D)
	sync
9:	jr.hb	ra
END(m32_lock_dcache)

/*
 * void m32_lock_icache (void *code, size_t n)
 *
 * Load and lock a block of instructions into the i-cache
 */
LEAF(m32_lock_icache)
	SIZE_CACHE(a2,mips_icache_linesize)
	vcacheop(a0,a1,a2,Fetch_Lock_I)
	sync
9:	jr.hb	ra
END(m32_lock_icache)

/*
 * void m32_lock_scache (void * data, size_t n)
 *
 * Load and lock a block of data into the s-cache
 */
LEAF(m32_lock_scache)
	SIZE_CACHE(a2,mips_scache_linesize)
	vcacheop(a0,a1,a2,Fetch_Lock_S)
	sync
9:	jr.hb	ra
END(m32_lock_scache)

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
#include "cache.h"

/*
 * void mips_flush_cache (void)
 *
 * Writeback and invalidate all caches in the quickest way possible.
 */
SWCACHE(flush_cache)

/*
 * void mips_flush_dcache (void)
 *
 * Writeback and invalidate only the data caches.
 * Joint caches (i.e. combined I & D) will be flushed too.
 */
SWCACHE(flush_dcache)

/*
 * void mips_flush_icache (void)
 *
 * Writeback and invalidate only the instruction caches.
 * Joint caches (i.e. combined I & D) will be flushed too.
 */
SWCACHE(flush_icache)

/*
 * void mips_clean_cache (vaddr_t va, unsigned int size)
 *
 * Writeback and invalidate a virtual address range in all caches.
 */
SWCACHE(clean_cache)

/*
 * void mips_sync_icache (vaddr_t va, unsigned int size)
 *
 * Synchronise i-cache with d-cache in this virtual address range;
 * often same as mips_clean_cache.
 */
SWCACHE(sync_icache)

/*
 * void mips_clean_dcache (vaddr_t va, unsigned int size)
 *
 * Writeback and invalidate a virtual address range in data caches.
 * Joint caches (i.e. combined I & D) will be cleaned too.
 */
SWCACHE(clean_dcache)

/*
 * void mips_clean_dcache_nowrite (vaddr_t va, unsigned int size)
 *
 * Invalidate a virtual address range in data caches.
 * Joint caches (i.e. combined I & D) will be cleaned too.
 * XXX Only safe if region is totally cache line aligned,
 * i.e. it doesn't share a cache line with other data.
 */
SWCACHE(clean_dcache_nowrite)

/*
 * void mips_lock_dcache (vaddr_t va, unsigned int size)
 *
 * Load and lock a block of data into the primary data cache
 */
SWCACHE(lock_dcache)

/*
 * void mips_lock_icache (vaddr_t va, unsigned int size)
 *
 * Load and lock a block of instructions into the primary instruction cache
 */
SWCACHE(lock_icache)

/*
 * void mips_lock_scache (vaddr_t va, unsigned int size)
 *
 * Load and lock a block of memory into the secondary cache
 */
SWCACHE(lock_scache)

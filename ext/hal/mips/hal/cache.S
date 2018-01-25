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

DECL(mips_icache_size,-1)
DECL(mips_icache_linesize,-1)
DECL(mips_icache_ways,1)

DECL(mips_dcache_size,-1)
DECL(mips_dcache_linesize,-1)
DECL(mips_dcache_ways,1)

DECL(mips_scache_size,-1)
DECL(mips_scache_linesize,-1)
DECL(mips_scache_ways,1)

DECL(mips_tcache_size,-1)
DECL(mips_tcache_linesize,-1)
DECL(mips_tcache_ways,1)

/*
 * void mips_size_cache (void)
 *
 * Size caches without reinitialising and losing dirty cache lines.
 */
SWCACHE(size_cache)

/*
 * void mips_clean_icache (vaddr_t va, unsigned int size)
 *
 * Writeback and invalidate a virtual address range in instruction caches.
 * Joint caches (i.e. combined I & D) will be cleaned too.
 */
SWCACHE(clean_icache)

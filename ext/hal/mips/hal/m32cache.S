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
 * void m32_size_cache()
 *
 * Work out size of I, D & S caches (assume already initialised)
 */
LEAF(m32_size_cache)
	lw	t0, mips_icache_size
	move	tmp3, ra
	bgtz	t0, 8f				# already known?

	bal	_size_cache
	move	ra, tmp3

8:	# Return
	jr	ra
END(m32_size_cache)

/*
 * void m32_clean_icache (unsigned kva, size_t n)
 *
 * Writeback and invalidate address range in instruction caches
 */
LEAF(m32_clean_icache)
	SIZE_CACHE(a2, mips_icache_linesize)
	vcacheop(a0,a1,a2,Hit_Invalidate_I)

	lw	a2, mips_scache_linesize
	blez	a2, 9f
	vcacheop(a0,a1,a2,Hit_Writeback_Inv_S)
	sync

9:	jr.hb	ra
END(m32_clean_icache)

/*
 * static void _size_cache()
 *
 * Internal routine to determine cache sizes by looking at config
 * registers.  Sizes are returned in registers, as follows:
 *
 * Do not use tmp3 (reg a1) and tmp1 (reg v1) in this function.
 */
LEAF(_size_cache)
	# Read $config, 0 to check presence of $config, 1
	mfc0	cfg, C0_CONFIG

	# Read Configuration register, select 1
	mfc0	cfg, C0_CONFIG1

	# Get I-cache line size
	ext	tmp, cfg, CFG1_IL_SHIFT, CFG1_IL_BITS
	beqz	tmp, 8f		# No I-cache

	# Get number of I-cache ways
	ext	iways, cfg, CFG1_IA_SHIFT, CFG1_IA_BITS
	addiu	iways, iways, 1
	move	icachesize,iways

	# Total icache size = lines/way * linesize * ways
	li	ilinesize, 1
	addiu	tmp, tmp, 1
	sllv	ilinesize, ilinesize, tmp
	sllv	icachesize, icachesize, tmp

	# Get I-cache lines per way
	ext	tmp, cfg, CFG1_IS_SHIFT, CFG1_IS_BITS
	addiu	tmp, tmp, 1
	andi	tmp, tmp, 7
	addiu	tmp, tmp, 5
	sllv	icachesize, icachesize, tmp

	# Store icache config
	sw	icachesize, mips_icache_size
	sw	ilinesize, mips_icache_linesize
	sw	iways, mips_icache_ways

8:	# No I-cache, check for D-cache
	ext	tmp, cfg, CFG1_DL_SHIFT, CFG1_DL_BITS
	beqz	tmp, 9f					# No D-cache

	# Get number of dcache ways
	ext	dways, cfg, CFG1_DA_SHIFT, CFG1_DA_BITS
	addiu	dways, dways, 1
	move	dcachesize,dways

	# Total dcache size = lines/way * linesize * ways
	li	dlinesize, 1
	addiu	tmp, tmp, 1
	sllv	dlinesize, dlinesize, tmp
	sllv	dcachesize, dcachesize, tmp

	# Get dcache lines per way
	ext	tmp, cfg, CFG1_DS_SHIFT, CFG1_DS_BITS
	addiu	tmp, tmp, 1
	andi	tmp, tmp, 7
	addiu	tmp, tmp, 5
	sllv	dcachesize, dcachesize, tmp

	# Store dcache config
	sw	dcachesize, mips_dcache_size
	sw	dlinesize, mips_dcache_linesize
	sw	dways, mips_dcache_ways
9:
	LA	tmp, __cache_size_hook
	move	tmp4, ra
	jal	tmp
	move	ra, tmp4

	# Return
	jr	ra
END(_size_cache)

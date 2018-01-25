/*
 * Copyright 2015, Imagination Technologies Limited and/or its
 *                 affiliated group companies.
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
#include <mips/asm.h>
#include <mips/regdef.h>
#include <mips/cm3.h>
#include <mips/m32c0.h>

#define tmp		t0
#define cfg		t1
#define scachesize	t8
#define slinesize	t9
#define sways		v0
#define tmp1		v1
#define tmp2		a0
#define tmp3		a1
#define tmp4		a2
#define tmp5		a3
/*
 * static void __cache_size_hook()
 *
 * Routine for calculating L2 cache size from CM3 configuration
 * registers.  Sizing information is stored directly to memory.
 *
 * Do not use tmp3 (reg a1), tmp1 (reg v1) or tmp4 (a2) in this function.
 */

LEAF(__cache_size_hook)

	# Check if Coherency Manager memory-mapped
	# Global Configuration Register Space is implemented.
	mfc0	tmp, C0_CONFIG3
	ext	tmp, tmp, CFG3_M_SHIFT, 1
	beqz	tmp, 1f		# Fall back to config2 based L2

	mfc0	tmp, C0_CONFIG4
	ext	tmp, tmp, CFG4_M_SHIFT, 1
	beqz	tmp, 1f		# Fall back to config2 based L2

	# Do we have a memory mapped L2 cache config?
	mfc0	tmp, C0_CONFIG5
	ext	tmp, tmp, CFG5_L2C_SHIFT, 1
	bnez	tmp, 2f

1:
	# Jump to the standard Config2 based scache config
	j	__def_cache_size_hook

2:
	# Read CMGCRBase to find CMGCR_BASE_ADDR
	PTR_MFC0 tmp,C0_CMGCRBASE
	sll	tmp, tmp, 4
	lui	tmp2, 0xb000	 # Make it virtual
	or	tmp, tmp, tmp2

	# Read GCR_L2_CONFIG
	PTR_L	tmp, GCR_L2_CONFIG(tmp)

	# Extract line size
	ext	slinesize, tmp, GCR_L2_SL_SHIFT, GCR_L2_SL_BITS

	# Check for no cache
	beqz	slinesize, 3f
	li	tmp2, 2
	sllv	slinesize, tmp2, slinesize	# Now have true L2 line size

	# Extract sets/way
	ext	sways, tmp, GCR_L2_SS_SHIFT, GCR_L2_SS_BITS
	li	tmp2, 64
	sllv	sways, tmp2, sways		# Now we have true L2 sets/way

	# Extract L2 associativity
	ext	tmp, tmp, GCR_L2_SA_SHIFT, GCR_L2_SA_BITS
	addiu	tmp, tmp, 1
	mul	tmp, tmp, sways			# Get total number of sets
	mul	scachesize, slinesize, tmp	# L2 cache size

	sw	scachesize, mips_scache_size
	sw	slinesize, mips_scache_linesize
	sw	sways, mips_scache_ways

3:
	# Return
	jr	ra
END(__cache_size_hook)

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

#include <mips/asm.h>
#include <mips/regdef.h>
#include <mips/m32c0.h>

/*
 * MIPS32 cache operations.
 *
 * The _flush and _clean functions are complex composites that do whatever
 * is necessary to flush/clean ALL caches, in the quickest possible way.
 * The other functions are targetted explicitly at a particular cache
 * I or D; it is up to the user to call the correct set of functions
 * for a given system.
 */

IMPORT(mips_icache_size,4)
IMPORT(mips_icache_linesize,4)
IMPORT(mips_icache_ways,4)

IMPORT(mips_dcache_size,4)
IMPORT(mips_dcache_linesize,4)
IMPORT(mips_dcache_ways,4)

IMPORT(mips_scache_size,4)
IMPORT(mips_scache_linesize,4)
IMPORT(mips_scache_ways,4)

/*
 * Macros to automate cache operations
 */

#define addr	t0
#define maxaddr	t1
#define mask	t2

#define cacheop(kva, n, linesize, op)	\
	/* check for bad size */	\
	blez	n,11f ;			\
	PTR_ADDU maxaddr,kva,n ;	\
	/* align to line boundaries */	\
	PTR_SUBU mask,linesize,1 ;	\
	not	mask ;			\
	and	addr,kva,mask ;		\
	PTR_SUBU addr,linesize ;	\
	PTR_ADDU maxaddr,-1 ;		\
	and	maxaddr,mask ;		\
	/* the cacheop loop */		\
10:	PTR_ADDU addr,linesize ;	\
	cache	op,0(addr) ;	 	\
	bne	addr,maxaddr,10b ;	\
11:

/* virtual cache op: no limit on size of region */
#define vcacheop(kva, n, linesize, op)	\
	cacheop(kva, n, linesize, op)

/* indexed cache op: region limited to cache size */
#define icacheop(kva, n, linesize, size, op) \
	move	t3,n;			\
	bltu	n,size,12f ;		\
	move	t3,size ;		\
12:	cacheop(kva, t3, linesize, op)


/* caches may not have been sized yet */
#define SIZE_CACHE(reg,which)		\
	lw	reg,which;		\
	move	v1,ra;			\
	bgez	reg,9f;			\
	bal	m32_size_cache;		\
	lw	reg,which;		\
	move	ra,v1;			\
9:	blez	reg,9f;			\
	sync

#define tmp		t0
#define cfg		t1
#define icachesize	t2
#define ilinesize	t3
#define iways		ta0
#define dcachesize	ta1
#define dlinesize	ta2
#define dways		ta3
#define scachesize	t8
#define slinesize	t9
#define sways		v0
#define tmp1		v1
#define tmp2		a0
#define tmp3		a1
#define tmp4		a2
#define tmp5		a3

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

#ifndef _CM3_H_
#define _CM3_H_

#define CMGCR_BASE_ADDR_SHIFT	11

#define CMGCR_BASE_ADDR_LSHIFT	4
/* Offsets of memory-mapped registers */
#define GCR_L2_CONFIG		0x130
#define GCR_L2_RAM_CONFIG	0x240
#define GCR_TAG_ADDR		0x600
#define GCR_TAG_STATE		0x608
#define GCR_TAG_DATA		0x610
#define GCR_TAG_ECC		0x618

/* Contents of  L2 RAM field */
#define GCR_L2_RAM_HCID_SHIFT	30
#define GCR_L2_RAM_HCID_BITS	1
#define GCR_L2_RAM_HCIS_SHIFT	29
#define GCR_L2_RAM_HCIS_BITS	1

/* L2 Configuration Register */
#define GCR_L2_REG_EXISTS_MASK	0x80000000
#define GCR_L2_REG_EXISTS_SHIFT	31
#define GCR_L2_REG_EXISTS_BITS	1
#define GCR_L2_LRU_WE_MASK	(1<<GCR_L2_LRU_WE_SHIFT)
#define GCR_L2_LRU_WE_SHIFT	26
#define GCR_L2_LRU_WE_BITS	1
#define GCR_L2_TAG_WE_MASK	(1<<GCR_L2_TAG_WE_SHIFT)
#define GCR_L2_TAG_WE_SHIFT	25
#define GCR_L2_TAG_WE_BITS	1
#define GCR_L2_ECC_WE_MASK	(1<<GCR_L2_ECC_WE_SHIFT)
#define GCR_L2_ECC_WE_SHIFT	24
#define GCR_L2_ECC_WE_BITS	1
#define GCR_L2_BYPASS_MASK	(1<<GCR_L2_BYPASS_SHIFT)
#define GCR_L2_BYPASS_SHIFT	20
#define GCR_L2_BYPASS_BITS	1
#define GCR_L2_SS_MASK		0x0000F000
#define GCR_L2_SS_SHIFT		12
#define GCR_L2_SS_BITS		4
#define GCR_L2_SL_MASK		0x00000F00
#define GCR_L2_SL_SHIFT		8
#define GCR_L2_SL_BITS		4
#define GCR_L2_SA_MASK		0x000000FF
#define GCR_L2_SA_SHIFT		0
#define GCR_L2_SA_BITS		8

#endif /* _CM3_H_ */

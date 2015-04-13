/* Freescale K6x microprocessor MPU register definitions */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
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

/*
DESCRIPTION
This module defines the Memory Protection Unit (MPU) Registers for the
K6x Family of microprocessors.
NOTE: Not all the registers are currently defined here - only those that are
currently used.
*/

#ifndef _K6xMPU_H_
#define _K6xMPU_H_

#include <stdint.h>

#define MPU_VALID_MASK 0x01
#define MPU_SLV_PORT_ERR_MASK 0xF8

typedef union {
	uint32_t value;
	struct {
		uint8_t valid : 1
			__attribute__((packed)); /* MPU valid/enable */
		uint8_t res_1 : 7 __attribute__((packed)); /* RAZ/WI */
		uint8_t numRgnDescs : 4
			__attribute__((packed)); /* # of regions */
		uint8_t numSlvPorts : 4
			__attribute__((packed)); /* # of slave ports */
		uint8_t hwRevLvl : 4
			__attribute__((packed)); /* Hardware revision */
		uint8_t res_20 : 3 __attribute__((packed)); /* RAZ/WI */
		uint8_t res_23 : 1 __attribute__((packed)); /* RAO/WI */
		uint8_t res_24 : 3 __attribute__((packed)); /* RAZ/WI */
		uint8_t slvPortNErr : 5
			__attribute__((packed)); /* slave port N err */
	} field;
} CESR_t; /* 0x000 Control/Error Status Register */

#define MPU_NUM_SLV_PORTS 5
#define MPU_NUM_REGIONS 12
#define MPU_NUM_WORDS_PER_REGION 4

typedef volatile struct {
	CESR_t ctrlErrStatus; /* 0x0000 */
	uint32_t errAddr0;    /* 0x0010 */
	uint32_t errDetail0;  /* 0x0014 */
	uint32_t errAddr1;    /* 0x0018 */
	uint32_t errDetail1;  /* 0x001C */
	uint32_t errAddr2;    /* 0x0020 */
	uint32_t errDetail2;  /* 0x0024 */
	uint32_t errAddr3;    /* 0x0028 */
	uint32_t errDetail3;  /* 0x002C */
	uint32_t errAddr4;    /* 0x0030 */
	uint32_t errDetail4;  /* 0x0034 */
	uint32_t rgnDesc[MPU_NUM_REGIONS][MPU_NUM_WORDS_PER_REGION]; /* 0x0400
									*/
	uint32_t rgnDescAltAccCtrl[MPU_NUM_REGIONS]; /* 0x0800 */
} K6x_MPU_t; /* K6x Microntroller PMC module */

#endif /* _K6xMPU_H_ */

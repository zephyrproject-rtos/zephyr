/* Freescale K6x microprocessor PMC register definitions */

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
This module defines the Power Management Controller (PMC) registers for the
K6x Family of microprocessors.
NOTE: Not all the registers are currently defined here - only those that are
currently used.
*/

#ifndef _K6xPMC_H_
#define _K6xPMC_H_

#include <stdint.h>

#define PMC_REGSC_ACKISO_MASK 0x08 /* ack I/O isolation (write to clear) */

typedef union {
	uint8_t value;
	struct {
		uint8_t bandgapBufEn : 1
			__attribute__((packed)); /* bandgap buffering */
		uint8_t res_1 : 1 __attribute__((packed)); /* SBZ */
		uint8_t regOnStatus : 1
			__attribute__((packed)); /* regulator on, R/O */
		uint8_t ackIsolation : 1
			__attribute__((packed)); /* ack I/O isolation */
		uint8_t bandgapEn : 1
			__attribute__((packed)); /* bandgap enable */
		uint8_t res_5 : 1 __attribute__((packed));
		uint8_t res_6 : 2 __attribute__((packed)); /* RAZ/WI */
	} field;
} REGSC_t; /* 0x0002 Regulator Status/Control Register */

typedef volatile struct {
	uint8_t lvdsc1; /* 0x0000 */
	uint8_t lvdsc2; /* 0x0001 */
	REGSC_t regsc;  /* 0x0002 */
} K6x_PMC_t;		/* K6x Microntroller PMC module */

#endif /* _K6xPMC_H_ */

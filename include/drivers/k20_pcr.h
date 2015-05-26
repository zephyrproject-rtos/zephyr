/* Freescale K20 microprocessor Pin Control register definitions */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
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
This module defines the PCR (Port/Pin Control/Configuration Registers) for the
K20 Family of microprocessors
*/

#ifndef _K20PCR_H_
#define _K20PCR_H_

#include <stdint.h>

#define PCR_PORT_A 0
#define PCR_PORT_B 1
#define PCR_PORT_C 2
#define PCR_PORT_D 3
#define PCR_PORT_E 4

#define PCR_MUX_DISABLED 0
#define PCR_MUX_ALT1 1
#define PCR_MUX_ALT2 2
#define PCR_MUX_ALT3 3
#define PCR_MUX_ALT4 4
#define PCR_MUX_ALT5 5
#define PCR_MUX_ALT6 6
#define PCR_MUX_ALT7 7

typedef union {
	uint32_t value;
	struct {
		uint8_t ps : 1 __packed;
		uint8_t pe : 1 __packed;
		uint8_t sre : 1 __packed;
		uint8_t res_3 : 1 __packed;
		uint8_t pfe : 1 __packed;
		uint8_t ode : 1 __packed;
		uint8_t dse : 1 __packed;
		uint8_t res_7 : 1 __packed;
		uint8_t mux : 3 __packed;
		uint8_t res_11_14 : 4 __packed;
		uint8_t lk : 1 __packed;
		uint8_t irqc : 4 __packed;
		uint8_t res_20_23 : 4 __packed;
		uint8_t isf : 1 __packed;
		uint8_t res_25_31 : 7 __packed;
	} field;
} K20_PCR_t; /* Pin Control Register n, n= 0-31 */

typedef union {
	uint32_t value;
	struct {
		uint16_t gpwe : 16 __packed;
		uint16_t gpwd : 16 __packed;
	} field;
} K20_GPC_t; /* Global Pin Control Low/High Register */

/* K20 Microntroller PCR module register structure */

typedef volatile struct {
	struct {
		K20_PCR_t pcr[32] __packed; /* 0x00-07C */
		K20_GPC_t gpchr __packed;   /* 0x80 */
		K20_GPC_t gpclr __packed;   /* 0x84 */
		uint8_t res_88_9F[0xA0 - 0x88];	/* 0x88-0x9F Reserved */
		uint32_t isfr __packed; /* 0xA0 */
		uint8_t res_A4_FF[0x1000 - 0xA4];      /* 0xA4-0xFFF Reserved */
	} port[5];
} K20_PORT_PCR_t;

/* pin control register for port A..E on pin 0..31 */
#define K20_PCR(base, port, pin) \
	((K20_PCR_t *)(base + (0x1000 * port) + (pin * 4)))

#endif /* _K20PCR_H_ */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 * @brief Freescale K20 microprocessor Pin Control register definitions
 *
 * This module defines the PCR (Port/Pin Control/Configuration Registers) for
 * the K20 Family of microprocessors
 */

#ifndef _K20PCR_H_
#define _K20PCR_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

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

union K20_PCR {
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
}; /* Pin Control Register n, n= 0-31 */

union K20_GPC {
	uint32_t value;
	struct {
		uint16_t gpwe : 16 __packed;
		uint16_t gpwd : 16 __packed;
	} field;
}; /* Global Pin Control Low/High Register */

/* K20 Microntroller PCR module register structure */

struct K20_PORT_PCR {
	struct {
		union K20_PCR pcr[32] __packed; /* 0x00-07C */
		union K20_GPC gpchr __packed;   /* 0x80 */
		union K20_GPC gpclr __packed;   /* 0x84 */
		uint8_t res_88_9F[0xA0 - 0x88];	/* 0x88-0x9F Reserved */
		uint32_t isfr __packed; /* 0xA0 */
		uint8_t res_A4_FF[0x1000 - 0xA4];      /* 0xA4-0xFFF Reserved */
	} port[5];
};

/* pin control register for port A..E on pin 0..31 */
#define K20_PCR(base, port, pin) \
	((union K20_PCR *)(base + (0x1000 * port) + (pin * 4)))

#ifdef __cplusplus
}
#endif

#endif /* _K20PCR_H_ */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
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
 * @brief Freescale K6x microprocessor MPU register definitions
 *
 * This module defines the Memory Protection Unit (MPU) Registers for the K6x
 * Family of microprocessors.
 * NOTE: Not all the registers are currently defined here - only those that are
 * currently used.
 */

#ifndef _K6xMPU_H_
#define _K6xMPU_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MPU_VALID_MASK 0x01
#define MPU_SLV_PORT_ERR_MASK 0xF8

union CESR {
	uint32_t value;
	struct {
		uint8_t valid : 1 __packed; /* MPU valid/enable */
		uint8_t res_1 : 7 __packed; /* RAZ/WI */
		uint8_t numRgnDescs : 4 __packed; /* # of regions */
		uint8_t numSlvPorts : 4 __packed; /* # of slave ports */
		uint8_t hwRevLvl : 4 __packed; /* Hardware revision */
		uint8_t res_20 : 3 __packed; /* RAZ/WI */
		uint8_t res_23 : 1 __packed; /* RAO/WI */
		uint8_t res_24 : 3 __packed; /* RAZ/WI */
		uint8_t slvPortNErr : 5 __packed; /* slave port N err */
	} field;
}; /* 0x000 Control/Error Status Register */

#define MPU_NUM_SLV_PORTS 5
#define MPU_NUM_REGIONS 12
#define MPU_NUM_WORDS_PER_REGION 4

struct K6x_MPU {
	union CESR ctrlErrStatus; /* 0x0000 */
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
}; /* K6x Microntroller PMC module */

#ifdef __cplusplus
}
#endif

#endif /* _K6xMPU_H_ */

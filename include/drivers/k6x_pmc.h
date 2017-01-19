/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Freescale K6x microprocessor PMC register definitions
 *
 * This module defines the Power Management Controller (PMC) registers for the
 * K6x Family of microprocessors.
 * NOTE: Not all the registers are currently defined here - only those that are
 * currently used.
 */

#ifndef _K6xPMC_H_
#define _K6xPMC_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PMC_REGSC_ACKISO_MASK 0x08 /* ack I/O isolation (write to clear) */

union REGSC {
	uint8_t value;
	struct {
		uint8_t bandgapBufEn : 1 __packed; /* bandgap buffering */
		uint8_t res_1 : 1 __packed; /* SBZ */
		uint8_t regOnStatus : 1 __packed; /* regulator on, R/O */
		uint8_t ackIsolation : 1 __packed; /* ack I/O isolation */
		uint8_t bandgapEn : 1 __packed; /* bandgap enable */
		uint8_t res_5 : 1 __packed;
		uint8_t res_6 : 2 __packed; /* RAZ/WI */
	} field;
}; /* 0x0002 Regulator Status/Control Register */

struct K6x_PMC {
	uint8_t lvdsc1;		/* 0x0000 */
	uint8_t lvdsc2;		/* 0x0001 */
	union REGSC regsc;	/* 0x0002 */
};	/* K6x Microntroller PMC module */

#ifdef __cplusplus
}
#endif

#endif /* _K6xPMC_H_ */

/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * General header for the CC2650 System on Chip.
 */


#ifndef _CC2650_SOC_H_
#define _CC2650_SOC_H_

#include <sys/util.h>
#include "registers/ccfg.h"
#include "registers/gpio.h"
#include "registers/ioc.h"
#include "registers/prcm.h"


/* Helper functions and macros */

#define REG_ADDR(Base, Offset)	(u32_t)(Base + (u32_t)Offset)

int bit_is_set(u32_t reg, u8_t bit);


#endif /* _CC2650_SOC_H_ */

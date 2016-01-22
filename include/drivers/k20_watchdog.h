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
 * @brief Freescale K20 microprocessor Watch Dog registers
 *
 * This module defines Watch Dog Registers for the K20 Family of microprocessors
 */

#ifndef _K20WDOG_H_
#define _K20WDOG_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Sequence of writes within 20 bus cycles for action to take effect */
#define WDOG_REFRESH_1 0xA602
#define WDOG_REFRESH_2 0xB480
#define WDOG_UNLOCK_1 0xC520
#define WDOG_UNLOCK_2 0xD928

union WDOG_STCTRLH {
	uint16_t value; /* reset= 0x01D3 */
	struct {
		uint8_t wdogen : 1 __packed;
		uint8_t clksrc : 1 __packed;
		uint8_t irqrsten : 1 __packed;
		uint8_t winen : 1 __packed;
		uint8_t allowupdate : 1 __packed;
		uint8_t dbgen : 1 __packed;
		uint8_t stopen : 1 __packed;
		uint8_t waiten : 1 __packed;
		uint8_t res_8_9 : 2 __packed;
		uint8_t testwdog : 1 __packed;
		uint8_t testsel : 1 __packed;
		uint8_t bytesel : 2 __packed;
		uint8_t disestwdog : 1 __packed;
		uint8_t res_15 : 1 __packed;
	} field;
};

/* K20 Microntroller WDOG module register structure */

struct K20_WDOG {
	union WDOG_STCTRLH stctrlh; /* 0x00 */
	uint16_t stctrll;       /* 0x02 */
	uint16_t tovalh;	/* 0x04 */
	uint16_t tovall;	/* 0x06 */
	uint16_t winh;		/* 0x08 */
	uint16_t winl;		/* 0x0A */
	uint16_t refresh;       /* 0x0C */
	uint16_t unlock;	/* 0x0E */
	uint16_t tmrouth;       /* 0x10 */
	uint16_t tmroutl;       /* 0x12 */
	uint16_t rstcnt;	/* 0x14 */
	uint16_t presc;		/* 0x16 */
};

/**/
/**< Macro to enable all interrupts. */
#define EnableInterrupts __asm__(" CPSIE i");

/**< Macro to disable all interrupts. */
#define DisableInterrupts __asm__(" CPSID i");
/**/

/**
 *
 * @brief Watchdog timer unlock routine.
 *
 * This routine will unlock the watchdog timer registers for write access.
 * Writing 0xC520 followed by 0xD928 will unlock the write-once registers
 * in the WDOG so they are writable within the WCT period.
 *
 * @return N/A
 */
static ALWAYS_INLINE void wdog_unlock(volatile struct K20_WDOG *wdog_p)
{
	/*
	 * NOTE: DO NOT SINGLE STEP THROUGH THIS FUNCTION!!!
	 * There are timing requirements for the execution of the unlock
	 * process.
	 * Single stepping through the code you will cause the CPU to reset.
	 */

	/*
	 * This sequence must execute within 20 clock cycles, so disable
	 * interrupts to keep the code atomic and ensure the timing.
	 */
	DisableInterrupts;

	/* Write 2-word unlock sequence to unlock register */
	wdog_p->unlock = (uint16_t)WDOG_UNLOCK_1;
	wdog_p->unlock = (uint16_t)WDOG_UNLOCK_2;

	/* Re-enable interrupts now that we are done */
	EnableInterrupts;
}

/**
 *
 * @brief Watchdog timer disable routine
 *
 * This routine will disable the watchdog timer.
 *
 * @return N/A
 */
static ALWAYS_INLINE void wdog_disable(volatile struct K20_WDOG *wdog_p)
{
	union WDOG_STCTRLH_t stctrlh;

	/* First unlock the watchdog so that we can write to registers */
	wdog_unlock(wdog_p);

	/*
	 * Writes to control/configuration registers must execute within
	 * 256 clock cycles after unlocking, so interrupts may need to be
	 * disabled to ensure the timing.
	 */
	stctrlh.value = wdog_p->stctrlh.value;
	stctrlh.field.wdogen = 0;
	wdog_p->stctrlh.value = stctrlh.value;
}

#ifdef __cplusplus
}
#endif

#endif /* _K20WDOG_H_ */

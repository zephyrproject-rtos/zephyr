/* Freescale K20 microprocessor Watch Dog registers */

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
This module defines Watch Dog Registers for the K20 Family of microprocessors
*/

#ifndef _K20WDOG_H_
#define _K20WDOG_H_

#include <stdint.h>

/* Sequence of writes within 20 bus cycles for action to take effect */
#define WDOG_REFRESH_1 0xA602
#define WDOG_REFRESH_2 0xB480
#define WDOG_UNLOCK_1 0xC520
#define WDOG_UNLOCK_2 0xD928

typedef union {
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
} WDOG_STCTRLH_t;

/* K20 Microntroller WDOG module register structure */

typedef volatile struct {
	WDOG_STCTRLH_t stctrlh; /* 0x00 */
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
} K20_WDOG_t;

/***********************************************************************/
/*!< Macro to enable all interrupts. */
#define EnableInterrupts __asm__(" CPSIE i");

/*!< Macro to disable all interrupts. */
#define DisableInterrupts __asm__(" CPSID i");
/***********************************************************************/

/*******************************************************************************
 *
 * wdog_unlock - Watchdog timer unlock routine.
 *
 * This routine will unlock the watchdog timer registers for write access.
 * Writing 0xC520 followed by 0xD928 will unlock the write-once registers
 * in the WDOG so they are writable within the WCT period.
 *
 * RETURNS: N/A
 */
static ALWAYS_INLINE void wdog_unlock(K20_WDOG_t *wdog_p)
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

/*******************************************************************************
 *
 * wdog_disable - Watchdog timer disable routine
 *
 * This routine will disable the watchdog timer.
 *
 * RETURNS: N/A
 */
static ALWAYS_INLINE void wdog_disable(K20_WDOG_t *wdog_p)
{
	WDOG_STCTRLH_t stctrlh;

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

#endif /* _K20WDOG_H_ */

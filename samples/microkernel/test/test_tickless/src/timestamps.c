/* timestamps.c - timestamp support for tickless idle testing */

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
BSP-specific timestamp support for the tickless idle test.
*/

#include <tc_util.h>

#if defined(CONFIG_BSP_TI_LM3S6965_QEMU)
/*
 * TI LM3S6965EVM QEMU target - use a General Purpose Timer in
 * 32-bit periodic timer mode (down-counter)
 * (RTC mode's resolution of 1 second is insufficient.)
 */

#define _TIMESTAMP_NUM 0  /* set to timer # for use by timestamp (0-3) */

#define _CLKGATECTRL *((volatile uint32_t *)0x400FE104)
#define _CLKGATECTRL_TIMESTAMP_EN (1 << (16 + _TIMESTAMP_NUM))

#define _TIMESTAMP_BASE 0x40030000
#define _TIMESTAMP_OFFSET (0x1000 * _TIMESTAMP_NUM)
#define _TIMESTAMP_ADDR (_TIMESTAMP_BASE + _TIMESTAMP_OFFSET)

#define _TIMESTAMP_CFG *((volatile uint32_t *)(_TIMESTAMP_ADDR + 0))
#define _TIMESTAMP_CTRL *((volatile uint32_t *)(_TIMESTAMP_ADDR + 0xC))
#define _TIMESTAMP_MODE *((volatile uint32_t *)(_TIMESTAMP_ADDR + 0x4))
#define _TIMESTAMP_LOAD *((volatile uint32_t *)(_TIMESTAMP_ADDR + 0x28))
#define _TIMESTAMP_IMASK *((volatile uint32_t *)(_TIMESTAMP_ADDR + 0x18))
#define _TIMESTAMP_ISTATUS *((volatile uint32_t *)(_TIMESTAMP_ADDR + 0x1C))
#define _TIMESTAMP_ICLEAR *((volatile uint32_t *)(_TIMESTAMP_ADDR + 0x24))
#define _TIMESTAMP_VAL *((volatile uint32_t *)(_TIMESTAMP_ADDR + 0x48))

/*
 * Set the rollover value such that it leaves the most significant bit of
 * the returned timestamp value unused.  This allows room for extended values
 * when handling rollovers when converting to an up-counter value.
 */
#define _TIMESTAMP_MAX ((uint32_t)0x7FFFFFFF)
#define _TIMESTAMP_EXT ((uint32_t)0x80000000)

/*******************************************************************************
*
* _TimestampOpen - timestamp initialization
*
* This routine initializes the timestamp timer.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

void _TimestampOpen(void)
	{
    /* QEMU does not currently support the 32-bit timer modes of the GPTM */
	printk("WARNING! Timestamp is not supported for this target!\n");

    /* enable timer access */
	_CLKGATECTRL |= _CLKGATECTRL_TIMESTAMP_EN;

    /* minimum 3 clk delay is required before timer register access */
	task_sleep(3);

	_TIMESTAMP_CTRL = 0x0;  /* disable/reset timer */
	_TIMESTAMP_CFG = 0x0;  /* 32-bit timer */
	_TIMESTAMP_MODE = 0x2;  /* periodic mode */
	_TIMESTAMP_LOAD = _TIMESTAMP_MAX; /* maximum interval to reduce rollovers */
	_TIMESTAMP_IMASK = 0x70F;  /* mask all timer interrupts */
	_TIMESTAMP_ICLEAR = 0x70F;  /* clear all interrupt status */

	_TIMESTAMP_CTRL = 0x1;  /* enable timer */
	}

/*******************************************************************************
*
* _TimestampRead - timestamp timer read
*
* This routine returns the timestamp value.
*
* RETURNS: timestamp value
*
* \NOMANUAL
*/

uint32_t _TimestampRead(void)
	{
	static uint32_t lastTimerVal = 0;
	static uint32_t cnt = 0;
	uint32_t timerVal = _TIMESTAMP_VAL;

    /* handle rollover for every other read (end of sleep) */

	if ((cnt % 2) && (timerVal > lastTimerVal)) {
	lastTimerVal = timerVal;

	/* convert to extended up-counter value */
	timerVal = _TIMESTAMP_EXT + (_TIMESTAMP_MAX - timerVal);
	}
	else {
	lastTimerVal = timerVal;

	/* convert to up-counter value */
	timerVal = _TIMESTAMP_MAX - timerVal;
	}

	cnt++;

	return timerVal;
	}

/*******************************************************************************
*
* _TimestampClose - timestamp release
*
* This routine releases the timestamp timer.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

void _TimestampClose(void)
	{

    /* disable/reset timer */
	_TIMESTAMP_CTRL = 0x0;
	_TIMESTAMP_CFG = 0x0;

    /* disable timer access */
	_CLKGATECTRL &= ~_CLKGATECTRL_TIMESTAMP_EN;
	}

#elif defined(CONFIG_BSP_FSL_FRDM_K64F)
/* Freescale FRDM-K64F target - use RTC (prescale value) */

#define _COUNTDOWN_TIMER FALSE

#define _CLKGATECTRL *((volatile uint32_t *)0x4004803C)
#define _CLKGATECTRL_TIMESTAMP_EN (1 << 29)

#define _SYSOPTCTRL2 *((volatile uint32_t *)0x40048004)
#define _SYSOPTCTRL2_32KHZRTCCLK (1 << 4)

#define _TIMESTAMP_ADDR (0x4003D000)

#define _TIMESTAMP_ICLEAR *((volatile uint32_t *)(_TIMESTAMP_ADDR + 0x24))

#define _TIMESTAMP_VAL *((volatile uint32_t *)(_TIMESTAMP_ADDR + 0))
#define _TIMESTAMP_PRESCALE *((volatile uint32_t *)(_TIMESTAMP_ADDR + 0x4))
#define _TIMESTAMP_COMP *((volatile uint32_t *)(_TIMESTAMP_ADDR + 0xC))
#define _TIMESTAMP_CTRL *((volatile uint32_t *)(_TIMESTAMP_ADDR + 0x10))
#define _TIMESTAMP_STATUS *((volatile uint32_t *)(_TIMESTAMP_ADDR + 0x14))
#define _TIMESTAMP_LOCK *((volatile uint32_t *)(_TIMESTAMP_ADDR + 0x18))
#define _TIMESTAMP_IMASK *((volatile uint32_t *)(_TIMESTAMP_ADDR + 0x1C))
#define _TIMESTAMP_RACCESS *((volatile uint32_t *)(_TIMESTAMP_ADDR + 0x800))
#define _TIMESTAMP_WACCESS *((volatile uint32_t *)(_TIMESTAMP_ADDR + 0x804))

/*******************************************************************************
*
* _TimestampOpen - timestamp initialization
*
* This routine initializes the timestamp timer.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

void _TimestampOpen(void)
	{
    /* enable timer access */
	_CLKGATECTRL |= _CLKGATECTRL_TIMESTAMP_EN;

    /* set 32 KHz RTC clk */
	_SYSOPTCTRL2 |= _SYSOPTCTRL2_32KHZRTCCLK;

	_TIMESTAMP_STATUS = 0x0;  /* disable counter */
	_TIMESTAMP_CTRL = 0x100;  /* enable oscillator */

	_TIMESTAMP_LOCK = 0xFF;  /* unlock registers */
	_TIMESTAMP_PRESCALE = 0x0;  /* reset prescale value */
	_TIMESTAMP_COMP = 0x0;  /* reset compensation values */
	_TIMESTAMP_RACCESS = 0xFF;  /* allow register read access */
	_TIMESTAMP_WACCESS = 0xFF;  /* allow register write access */
	_TIMESTAMP_IMASK = 0x0;  /* mask all timer interrupts */

    /* minimum 0.3 sec delay required for oscillator stabilization */
	task_sleep(300000/sys_clock_us_per_tick);

	_TIMESTAMP_VAL = 0x0;  /* clear invalid time flag in status register */

	_TIMESTAMP_STATUS = 0x10;  /* enable counter */
	}

/*******************************************************************************
*
* _TimestampRead - timestamp timer read
*
* This routine returns the timestamp value.
*
* RETURNS: timestamp value
*
* \NOMANUAL
*/

uint32_t _TimestampRead(void)
	{
	static uint32_t lastPrescale = 0;
	static uint32_t cnt = 0;
	uint32_t prescale1 = _TIMESTAMP_PRESCALE;
	uint32_t prescale2 = _TIMESTAMP_PRESCALE;

    /* ensure a valid reading */

	while (prescale1 != prescale2) {
		prescale1 = _TIMESTAMP_PRESCALE;
		prescale2 = _TIMESTAMP_PRESCALE;
		}

    /* handle prescale rollover @ 0x8000 for every other read (end of sleep) */

	if ((cnt % 2) && (prescale1 < lastPrescale)) {
		prescale1 += 0x8000;
		}

	lastPrescale = prescale2;
	cnt++;

	return prescale1;
	}

/*******************************************************************************
*
* _TimestampClose - timestamp release
*
* This routine releases the timestamp timer.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

void _TimestampClose(void)
	{
	_TIMESTAMP_STATUS = 0x0;  /* disable counter */
	_TIMESTAMP_CTRL = 0x0;  /* disable oscillator */
	}

#else
#error "Unknown BSP"
#endif /* BSP */

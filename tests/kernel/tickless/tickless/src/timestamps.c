/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * Timestamp support for tickless idle testing
 *
 * Platform-specific timestamp support for the tickless idle test.
 */

#include <tc_util.h>
#include <stddef.h>
#include <misc/__assert.h>

#if defined(CONFIG_SOC_TI_LM3S6965_QEMU)
/*
 * @brief Use a General Purpose Timer in
 * 32-bit periodic timer mode (down-counter)
 * (RTC mode's resolution of 1 second is insufficient.)
 */

#define _TIMESTAMP_NUM 0  /* set to timer # for use by timestamp (0-3) */

#define _CLKGATECTRL (*((volatile u32_t *)0x400FE104))
#define _CLKGATECTRL_TIMESTAMP_EN (1 << (16 + _TIMESTAMP_NUM))

#define _TIMESTAMP_BASE 0x40030000
#define _TIMESTAMP_OFFSET (0x1000 * _TIMESTAMP_NUM)
#define _TIMESTAMP_ADDR (_TIMESTAMP_BASE + _TIMESTAMP_OFFSET)

#define _TIMESTAMP_CFG (*((volatile u32_t *)(_TIMESTAMP_ADDR + 0)))
#define _TIMESTAMP_CTRL (*((volatile u32_t *)(_TIMESTAMP_ADDR + 0xC)))
#define _TIMESTAMP_MODE (*((volatile u32_t *)(_TIMESTAMP_ADDR + 0x4)))
#define _TIMESTAMP_LOAD (*((volatile u32_t *)(_TIMESTAMP_ADDR + 0x28)))
#define _TIMESTAMP_IMASK (*((volatile u32_t *)(_TIMESTAMP_ADDR + 0x18)))
#define _TIMESTAMP_ISTATUS (*((volatile u32_t *)(_TIMESTAMP_ADDR + 0x1C)))
#define _TIMESTAMP_ICLEAR (*((volatile u32_t *)(_TIMESTAMP_ADDR + 0x24)))
#define _TIMESTAMP_VAL (*((volatile u32_t *)(_TIMESTAMP_ADDR + 0x48)))

/*
 * Set the rollover value such that it leaves the most significant bit of
 * the returned timestamp value unused.  This allows room for extended values
 * when handling rollovers when converting to an up-counter value.
 */
#define _TIMESTAMP_MAX ((u32_t)0x7FFFFFFF)
#define _TIMESTAMP_EXT ((u32_t)0x80000000)

#define TICKS_TO_MS  (MSEC_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

/**
 *
 * @brief Timestamp initialization
 *
 * This routine initializes the timestamp timer.
 *
 * @return N/A
 */
void _timestamp_open(void)
{
	/* QEMU does not currently support the 32-bit timer modes of the GPTM */
	printk("WARNING! Timestamp is not supported for this target!\n");

	/* enable timer access */
	_CLKGATECTRL |= _CLKGATECTRL_TIMESTAMP_EN;

	/* minimum 3 clk delay is required before timer register access */
	k_sleep(3 * TICKS_TO_MS);

	_TIMESTAMP_CTRL = 0x0;                  /* disable/reset timer */
	_TIMESTAMP_CFG = 0x0;                   /* 32-bit timer */
	_TIMESTAMP_MODE = 0x2;                  /* periodic mode */

	/* maximum interval to reduce rollovers */
	_TIMESTAMP_LOAD = _TIMESTAMP_MAX;
	_TIMESTAMP_IMASK = 0x70F;               /* mask all timer interrupts */
	_TIMESTAMP_ICLEAR = 0x70F;              /* clear all interrupt status */

	_TIMESTAMP_CTRL = 0x1;                  /* enable timer */
}

/**
 *
 * @brief Timestamp timer read
 *
 * This routine returns the timestamp value.
 *
 * @return timestamp value
 */
u32_t _timestamp_read(void)
{
	static u32_t last_timer_val;
	static u32_t cnt;
	u32_t timer_val = _TIMESTAMP_VAL;

	/* handle rollover for every other read (end of sleep) */

	if ((cnt % 2) && (timer_val > last_timer_val)) {
		last_timer_val = timer_val;

		/* convert to extended up-counter value */
		timer_val = _TIMESTAMP_EXT + (_TIMESTAMP_MAX - timer_val);
	} else {
		last_timer_val = timer_val;

		/* convert to up-counter value */
		timer_val = _TIMESTAMP_MAX - timer_val;
	}

	cnt++;

	return timer_val;
}

/**
 *
 * @brief Timestamp release
 *
 * This routine releases the timestamp timer.
 *
 * @return N/A
 */
void _timestamp_close(void)
{

	/* disable/reset timer */
	_TIMESTAMP_CTRL = 0x0;
	_TIMESTAMP_CFG = 0x0;

	/* disable timer access */
	_CLKGATECTRL &= ~_CLKGATECTRL_TIMESTAMP_EN;
}

#elif defined(CONFIG_SOC_MK64F12)
/* Freescale FRDM-K64F target - use RTC (prescale value) */

#define _COUNTDOWN_TIMER false

#define _CLKGATECTRL (*((volatile u32_t *)0x4004803C))
#define _CLKGATECTRL_TIMESTAMP_EN (1 << 29)

#define _SYSOPTCTRL2 (*((volatile u32_t *)0x40048004))
#define _SYSOPTCTRL2_32KHZRTCCLK (1 << 4)

#define _TIMESTAMP_ADDR (0x4003D000)

#define _TIMESTAMP_ICLEAR (*((volatile u32_t *)(_TIMESTAMP_ADDR + 0x24)))

#define _TIMESTAMP_VAL (*((volatile u32_t *)(_TIMESTAMP_ADDR + 0)))
#define _TIMESTAMP_PRESCALE (*((volatile u32_t *)(_TIMESTAMP_ADDR + 0x4)))
#define _TIMESTAMP_COMP (*((volatile u32_t *)(_TIMESTAMP_ADDR + 0xC)))
#define _TIMESTAMP_CTRL (*((volatile u32_t *)(_TIMESTAMP_ADDR + 0x10)))
#define _TIMESTAMP_STATUS (*((volatile u32_t *)(_TIMESTAMP_ADDR + 0x14)))
#define _TIMESTAMP_LOCK (*((volatile u32_t *)(_TIMESTAMP_ADDR + 0x18)))
#define _TIMESTAMP_IMASK (*((volatile u32_t *)(_TIMESTAMP_ADDR + 0x1C)))
#define _TIMESTAMP_RACCESS (*((volatile u32_t *)(_TIMESTAMP_ADDR + 0x800)))
#define _TIMESTAMP_WACCESS (*((volatile u32_t *)(_TIMESTAMP_ADDR + 0x804)))

/**
 *
 * @brief Timestamp initialization
 *
 * This routine initializes the timestamp timer.
 *
 * @return N/A
 */
void _timestamp_open(void)
{
	/* enable timer access */
	_CLKGATECTRL |= _CLKGATECTRL_TIMESTAMP_EN;

	/* set 32 KHz RTC clk */
	_SYSOPTCTRL2 |= _SYSOPTCTRL2_32KHZRTCCLK;

	_TIMESTAMP_STATUS = 0x0;        /* disable counter */
	_TIMESTAMP_CTRL = 0x100;        /* enable oscillator */

	_TIMESTAMP_LOCK = 0xFF;         /* unlock registers */
	_TIMESTAMP_PRESCALE = 0x0;      /* reset prescale value */
	_TIMESTAMP_COMP = 0x0;          /* reset compensation values */
	_TIMESTAMP_RACCESS = 0xFF;      /* allow register read access */
	_TIMESTAMP_WACCESS = 0xFF;      /* allow register write access */
	_TIMESTAMP_IMASK = 0x0;         /* mask all timer interrupts */

	/* minimum 0.3 sec delay required for oscillator stabilization */
	k_sleep(0.3 * MSEC_PER_SEC);

	/* clear invalid time flag in status register */
	_TIMESTAMP_VAL = 0x0;

	_TIMESTAMP_STATUS = 0x10;       /* enable counter */
}

/**
 *
 * @brief Timestamp timer read
 *
 * This routine returns the timestamp value.
 *
 * @return timestamp value
 */
u32_t _timestamp_read(void)
{
	static u32_t last_prescale;
	static u32_t cnt;
	u32_t prescale1 = _TIMESTAMP_PRESCALE;
	u32_t prescale2 = _TIMESTAMP_PRESCALE;

	/* ensure a valid reading */

	while (prescale1 != prescale2) {
		prescale1 = _TIMESTAMP_PRESCALE;
		prescale2 = _TIMESTAMP_PRESCALE;
	}

	/* handle prescale rollover @ 0x8000
	 * for every other read (end of sleep)
	 */

	if ((cnt % 2) && (prescale1 < last_prescale)) {
		prescale1 += 0x8000;
	}

	last_prescale = prescale2;
	cnt++;

	return prescale1;
}

/**
 *
 * @brief Timestamp release
 *
 * This routine releases the timestamp timer.
 *
 * @return N/A
 */
void _timestamp_close(void)
{
	_TIMESTAMP_STATUS = 0x0;        /* disable counter */
	_TIMESTAMP_CTRL = 0x0;          /* disable oscillator */
}

#elif defined(CONFIG_SOC_FAMILY_SAM)
/* Atmel SAM family processor - use RTT (Real-time Timer) */

#include <soc.h>

/**
 *
 * @brief Timestamp initialization
 *
 * This routine initializes the timestamp timer.
 *
 * @return N/A
 */
void _timestamp_open(void)
{
	/* enable RTT clock from PMC */
	soc_pmc_peripheral_enable(ID_RTT);

	/* Reset RTT and set prescaler to 3, minimum required by SAM E70 SoC */
	RTT->RTT_MR = RTT_MR_RTTRST | RTT_MR_RTPRES(3);
}

/**
 *
 * @brief Timestamp timer read
 *
 * This routine returns the timestamp value.
 *
 * @return timestamp value
 */
u32_t _timestamp_read(void)
{
	u32_t timer_val_0;
	u32_t timer_val_1;

	/* As RTT_VR can be updated asynchronously with the Master Clock, it
	 * must be read twice at the same value to ensure the read value is
	 * correct.
	 */
	do {
		timer_val_0 = RTT->RTT_VR;
		timer_val_1 = RTT->RTT_VR;
	} while (timer_val_0 != timer_val_1);

	return timer_val_0;
}

/**
 *
 * @brief Timestamp release
 *
 * This routine releases the timestamp timer.
 *
 * @return N/A
 */
void _timestamp_close(void)
{
	/* disable RTT clock from PMC */
	soc_pmc_peripheral_disable(ID_RTT);
}

#else
#error "Unknown platform"
#endif /* CONFIG_SOC_xxx */

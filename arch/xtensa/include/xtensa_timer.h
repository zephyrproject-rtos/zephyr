/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 *        XTENSA INFORMATION FOR RTOS TICK TIMER AND CLOCK FREQUENCY
 *
 * This header contains definitions and macros for use primarily by Xtensa
 * RTOS assembly coded source files. It includes and uses the Xtensa hardware
 * abstraction layer (HAL) to deal with config specifics. It may also be
 * included in C source files.
 *
 * User may edit to modify timer selection and to specify clock frequency and
 * tick duration to match timer interrupt to the real-time tick duration.
 *
 * If the RTOS has no timer interrupt, then there is no tick timer and the
 * clock frequency is irrelevant, so all of these macros are left undefined
 * and the Xtensa core configuration need not have a timer.
 */

#ifndef XTENSA_TIMER_H
#define XTENSA_TIMER_H

#ifdef __ASSEMBLER__
#include    <xtensa/coreasm.h>
#endif

#include    <xtensa/corebits.h>
#include    <xtensa/config/system.h>

#include    "xtensa_rtos.h"     /* in case this wasn't included directly */

#if CONFIG_XTENSA_INTERNAL_TIMER || (CONFIG_XTENSA_TIMER_IRQ < 0)
/*
 * Select timer to use for periodic tick, and determine its interrupt number
 * and priority. User may specify a timer by defining XT_TIMER_INDEX with -D,
 * in which case its validity is checked (it must exist in this core and must
 * not be on a high priority interrupt - an error will be reported in invalid).
 * Otherwise select the first low or medium priority interrupt timer available.
 */
#if XCHAL_NUM_TIMERS == 0
#error "This Xtensa configuration is unsupported, it has no timers."
#endif /* XCHAL_NUM_TIMERS */

#ifndef XT_TIMER_INDEX
	#if XCHAL_TIMER3_INTERRUPT != XTHAL_TIMER_UNCONFIGURED
	  #if XCHAL_INT_LEVEL(XCHAL_TIMER3_INTERRUPT) <= XCHAL_EXCM_LEVEL
	    #undef  XT_TIMER_INDEX
	    #define XT_TIMER_INDEX    3
	  #endif
	#endif
	#if XCHAL_TIMER2_INTERRUPT != XTHAL_TIMER_UNCONFIGURED
	  #if XCHAL_INT_LEVEL(XCHAL_TIMER2_INTERRUPT) <= XCHAL_EXCM_LEVEL
	    #undef  XT_TIMER_INDEX
	    #define XT_TIMER_INDEX    2
	  #endif
	#endif
	#if XCHAL_TIMER1_INTERRUPT != XTHAL_TIMER_UNCONFIGURED
	  #if XCHAL_INT_LEVEL(XCHAL_TIMER1_INTERRUPT) <= XCHAL_EXCM_LEVEL
	    #undef  XT_TIMER_INDEX
	    #define XT_TIMER_INDEX    1
	  #endif
	#endif
	#if XCHAL_TIMER0_INTERRUPT != XTHAL_TIMER_UNCONFIGURED
	  #if XCHAL_INT_LEVEL(XCHAL_TIMER0_INTERRUPT) <= XCHAL_EXCM_LEVEL
	    #undef  XT_TIMER_INDEX
	    #define XT_TIMER_INDEX    0
	  #endif
	#endif
#endif
#ifndef XT_TIMER_INDEX
#error "There is no suitable timer in this Xtensa configuration."
#endif

#define XT_CCOMPARE             ((CCOMPARE) + (XT_TIMER_INDEX))
#define XT_TIMER_INTNUM         XCHAL_TIMER_INTERRUPT(XT_TIMER_INDEX)
#if XT_TIMER_INTNUM == XTHAL_TIMER_UNCONFIGURED
#error "The timer selected by XT_TIMER_INDEX does not exist in this core."
#endif
#else /* Case of an external timer which is not emulated by internal timer */
#define XT_TIMER_INTNUM         CONFIG_XTENSA_TIMER_IRQ
#endif /* CONFIG_XTENSA_INTERNAL_TIMER || (CONFIG_XTENSA_TIMER_IRQ < 0) */

#if CONFIG_XTENSA_INTERNAL_TIMER
#define XT_TIMER_INTPRI         XCHAL_INT_LEVEL(XT_TIMER_INTNUM)
#else
#define XT_TIMER_INTPRI         CONFIG_XTENSA_TIMER_IRQ_PRIORITY
#endif /* CONFIG_XTENSA_INTERNAL_TIMER */

#if XT_TIMER_INTPRI > XCHAL_EXCM_LEVEL
#error "The timer interrupt cannot be high priority (use medium or low)."
#endif

#define XT_TIMER_INTEN          (1 << (XT_TIMER_INTNUM))

/*
 * Set processor clock frequency, used to determine clock divisor for timer
 * tick.  User should BE SURE TO ADJUST THIS for the Xtensa platform being
 * used.  If using a supported board via the board-independent API defined in
 * xtbsp.h, this may be left undefined and frequency and tick divisor will be
 * computed and cached during run-time initialization.
 *
 * NOTE ON SIMULATOR: Under the Xtensa instruction set simulator, the frequency
 * can only be estimated because it depends on the speed of the host and the
 * version of the simulator.  Also because it runs much slower than hardware,
 * it is not possible to achieve real-time performance for most applications
 * under the simulator. A frequency too low does not allow enough time between
 * timer interrupts, starving threads.  To obtain a more convenient but
 * non-real-time tick duration on the simulator, compile with xt-xcc option
 * "-DXT_SIMULATOR".  Adjust this frequency to taste (it's not real-time
 * anyway!).
 */
#if defined(XT_SIMULATOR) && !defined(XT_CLOCK_FREQ)
#define XT_CLOCK_FREQ       CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
#endif

#if !defined(XT_CLOCK_FREQ) && !defined(XT_BOARD)
#error "XT_CLOCK_FREQ must be defined for the target platform."
#endif

/*
 * Default number of timer "ticks" per second (default 100 for 10ms tick).
 * RTOS may define this in its own way (if applicable) in xtensa_rtos.h.
 * User may redefine this to an optimal value for the application, either by
 * editing this here or in xtensa_rtos.h, or compiling with xt-xcc option
 * "-DXT_TICK_PER_SEC=<value>" where <value> is a suitable number.
 */
#ifndef XT_TICK_PER_SEC
#if CONFIG_TICKLESS_KERNEL
#define XT_TICK_PER_SEC         1000	/* In tickless kernel 1TICK  = 1msec */
#else
#define XT_TICK_PER_SEC         CONFIG_SYS_CLOCK_TICKS_PER_SEC
#endif  /* CONFIG_TICKLESS_KERNEL */
#endif /* XT_TICK_PER_SEC */

/*
 * Derivation of clock divisor for timer tick and interrupt (one per tick).
 */
#ifdef XT_CLOCK_FREQ
#define XT_TICK_DIVISOR     (XT_CLOCK_FREQ / XT_TICK_PER_SEC)
#endif

#if CONFIG_XTENSA_INTERNAL_TIMER || (CONFIG_XTENSA_TIMER_IRQ < 0)
#ifndef __ASSEMBLER__
extern unsigned int _xt_tick_divisor;
extern void _xt_tick_divisor_init(void);
#endif

#endif // Internal/External timer

#endif  /* XTENSA_TIMER_H */


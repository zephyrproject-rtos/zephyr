/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <zephyr/types.h>
#include <system_timer.h>
#include <xtensa_rtos.h>

#include <xtensa_timer.h>
#include <kernel_structs.h>
#include "irq.h"

#include    "xtensa_rtos.h"

/*
 * This device driver can be also used with an extenal timer instead of
 * the internal one that may simply not exist.
 * The below macros are used to abstract the timer HW interface assuming that
 * it allows implementing them.
 * Of course depending on the HW specific requirements, part of the code may
 * need to changed. We tried to identify this code and hoghlight it to users.
 *
 * User shall track the TODO flags and follow the instruction to adapt the code
 * according to his HW.
 */

/* Map CCOMPAREn timer number to interrupt number.  Really this should
 * be a kconfig, but for legacy reasons we honor xtensa_rtos.h config
 * instead.
 */
#if XT_TIMER_INDEX == 0
#define TIMER_IRQ XCHAL_TIMER0_INTERRUPT
#elif XT_TIMER_INDEX == 1
#define TIMER_IRQ XCHAL_TIMER1_INTERRUPT
#elif XT_TIMER_INDEX == 2
#define TIMER_IRQ XCHAL_TIMER2_INTERRUPT
#else
#error Unrecognized/unset XT_TIMER_INDEX
#endif

#define MAX_TIMER_CYCLES 0xFFFFFFFF
/* Abstraction macros to access the timer fire time register */
#if CONFIG_XTENSA_INTERNAL_TIMER || (CONFIG_XTENSA_TIMER_IRQ < 0)
#define _XT_SR_CCOMPARE(op, idx) XT_##op##SR_CCOMPARE##idx
#define XT_SR_CCOMPARE(op, idx) _XT_SR_CCOMPARE(op, idx)
/* Use XT_TIMER_INDEX to select XT_CHAL macro to access CCOMPAREx register */
#define GET_TIMER_FIRE_TIME(void) XT_SR_CCOMPARE(R, XT_TIMER_INDEX)()
#define SET_TIMER_FIRE_TIME(time) XT_SR_CCOMPARE(W, XT_TIMER_INDEX)(time)
#define GET_TIMER_CURRENT_TIME(void) XT_RSR_CCOUNT()

#define XTENSA_RSR(sr) \
	({u32_t v; \
	 __asm__ volatile ("rsr." #sr " %0" : "=a"(v)); \
	 v; })
#define XTENSA_WSR(sr, v) \
	({__asm__ volatile ("wsr." #sr " %0" :: "a"(v)); })

#ifndef XT_RSR_CCOUNT
#define XT_RSR_CCOUNT() XTENSA_RSR(ccount)
#endif

#ifndef XT_RSR_CCOMPARE0
#define XT_RSR_CCOMPARE0() XTENSA_RSR(ccompare0)
#endif
#ifndef XT_RSR_CCOMPARE1
#define XT_RSR_CCOMPARE1() XTENSA_RSR(ccompare1)
#endif
#ifndef XT_RSR_CCOMPARE2
#define XT_RSR_CCOMPARE2() XTENSA_RSR(ccompare2)
#endif

#ifndef XT_WSR_CCOMPARE0
#define XT_WSR_CCOMPARE0(v) XTENSA_WSR(ccompare0, v)
#endif
#ifndef XT_WSR_CCOMPARE1
#define XT_WSR_CCOMPARE1(v) XTENSA_WSR(ccompare1, v)
#endif
#ifndef XT_WSR_CCOMPARE2
#define XT_WSR_CCOMPARE2(v) XTENSA_WSR(ccompare2, v)
#endif

/* Value underwich, don't program next tick but trigger it immediately. */
#define MIN_TIMER_PROG_DELAY 50
#else /* Case of an external timer which is not emulated by internal timer */
/* TODO: User who wants ot use and external timer should ensure that:
 *   - CONFIG_XTENSA_INTERNAL_TIMER is unset
 *   - CONFIG_XTENSA_TIMER_IRQ > 0
 *   - Macros below are correctly implemented
 */
#define GET_TIMER_FIRE_TIME(void) /* TODO: Implement this case */
#define SET_TIMER_FIRE_TIME(time) /* TODO: Implement this case */
#define GET_TIMER_CURRENT_TIME(void) /* TODO: Implement this case */
/* Value underwich, don't program next tick but trigger it immediately. */
#define MIN_TIMER_PROG_DELAY 50   /* TODO: Update this value */
#endif /* CONFIG_XTENSA_INTERNAL_TIMER || (CONFIG_XTENSA_TIMER_IRQ < 0) */

#ifdef CONFIG_TICKLESS_IDLE
#define TIMER_MODE_PERIODIC 0 /* normal running mode */
#define TIMER_MODE_ONE_SHOT 1 /* emulated, since sysTick has 1 mode */

#define IDLE_NOT_TICKLESS 0 /* non-tickless idle mode */
#define IDLE_TICKLESS 1     /* tickless idle  mode */

extern s32_t _sys_idle_elapsed_ticks;

static u32_t __noinit cycles_per_tick;
static u32_t __noinit max_system_ticks;
static u32_t idle_original_ticks;
static u32_t __noinit max_load_value;

#ifdef CONFIG_TICKLESS_KERNEL
static u32_t last_timer_value;
#else
static unsigned char timer_mode = TIMER_MODE_PERIODIC;
static unsigned char idle_mode = IDLE_NOT_TICKLESS;
#endif	/* CONFIG_TICKLESS_KERNEL */

#ifdef CONFIG_TICKLESS_KERNEL

 /* provides total programmed in tick count. */
u32_t _get_program_time(void)
{
	return idle_original_ticks;
}

 /* Timer Clock Ticks remaining for timer to expire. */
u32_t _get_remaining_program_time(void)
{
	u32_t c; /* Current time (time within this function execution) */
	u32_t f; /* Idle timer programmed fire time */
	u32_t r; /*remaining time to the timer to expire */

	if (!idle_original_ticks) {
		return 0;
	}

	f = GET_TIMER_FIRE_TIME();
	c = GET_TIMER_CURRENT_TIME();
	r = f > c ? (f - c) / cycles_per_tick : 0;
	return r;
}

 /* Total number of timer ticks passed since last Timer program. */
u32_t _get_elapsed_program_time(void)
{
	if (!idle_original_ticks) {
		return 0;
	}
	return (_get_program_time() - _get_remaining_program_time());
}

/* Returns number of clocks Cycles remaining for timer to overflow. */
static inline int32_t _get_max_clock_time(void)
{
	u32_t C;

	C = GET_TIMER_CURRENT_TIME();
	return (MAX_TIMER_CYCLES - C);
}

static inline void _set_max_clock_time(void)
{
	unsigned int key;

	key = irq_lock();
	_sys_clock_tick_count = _get_elapsed_clock_time();
	last_timer_value = GET_TIMER_CURRENT_TIME();
	irq_unlock(key);
	SET_TIMER_FIRE_TIME(MAX_TIMER_CYCLES); /* Program timer to max value */
}

/*
 * This Function does following:-
 * 1. Updates expected system ticks equal to time.
 * 2. Update kernel time book keeping for time passed since device bootup.
 * 3. Calls routine to set interrupt.
 */
void _set_time(u32_t time)
{
	u32_t C; /*  (current) time */
	u32_t F; /*  Time to program */
	unsigned int key;

	if (!time) {
		idle_original_ticks = 0;
		return;
	}
	key = irq_lock();
	/* Update System Level Ticks Time Keeping */
	_sys_clock_tick_count = _get_elapsed_clock_time();
	C = GET_TIMER_CURRENT_TIME();
	last_timer_value = C;
	irq_unlock(key);

	/* Track TICKs to program, this is required at next timer interrupt */
	idle_original_ticks = time;

	/* Track timer Overflow Case */
	if (idle_original_ticks >= (_get_max_clock_time() / cycles_per_tick)) {
		F = MAX_TIMER_CYCLES;
		idle_original_ticks = (F - C) / cycles_per_tick;
	}

	/* Calculate tiring time */
	F = (C + (idle_original_ticks * cycles_per_tick));
	/* Program firing timer */
	SET_TIMER_FIRE_TIME(F);
}

/*
 * This is used to program Timer clock to maximum Clock cycles in case Clock to
 * remain On.
 */
void _enable_sys_clock(void)
{
	if (!idle_original_ticks) {
		/* Program sys tick to maximum possible value */
		_set_time(_get_max_clock_time());
	}
}

/* Total number of ticks passed since device bootup. */
u64_t _get_elapsed_clock_time(void)
{
	u32_t C;
	unsigned int key;
	u64_t total;
	u32_t elapsed;

	key = irq_lock();
	C = GET_TIMER_CURRENT_TIME();
	elapsed = (last_timer_value <= C) ? (C - last_timer_value) :
				(MAX_TIMER_CYCLES - last_timer_value) + C;
	total = (_sys_clock_tick_count + (elapsed / cycles_per_tick));
	irq_unlock(key);

	return total;
}
#endif /* CONFIG_TICKLESS_KERNEL */

static ALWAYS_INLINE void tickless_idle_init(void)
{
	cycles_per_tick = sys_clock_hw_cycles_per_tick;
	/* calculate the max number of ticks with this 32-bit H/W counter */
	max_system_ticks = MAX_TIMER_CYCLES / cycles_per_tick;
	max_load_value = max_system_ticks * cycles_per_tick;
}

/*
 * @brief Place the system timer into idle state
 *
 * Re-program the timer to enter into the idle state for either the given
 * number of ticks or the maximum number of ticks that can be programmed
 * into hardware.
 *
 * @return N/A
 */

void _timer_idle_enter(s32_t ticks)
{
#ifdef CONFIG_TICKLESS_KERNEL
	if (idle_original_ticks != K_FOREVER) {
		/* Need to reprograme timer if current program is smaller */
		if (ticks > idle_original_ticks) {
			_set_time(ticks);
		}
	} else {
		idle_original_ticks = 0;
		/* Set time to largest possile Timer Tick */
		 _set_max_clock_time();
	}
#else
	u32_t P; /* Programming (current) time */
	u32_t F; /* Idle timer fire time */
	u32_t f; /* Last programmed timer fire time */

	if ((ticks == K_FOREVER) || (ticks > max_system_ticks)) {
		/*
		 * The number of cycles until the timer must fire next might
		 * not fit in the 32-bit counter register. To work around this,
		 * program the counter to fire in the maximum number of ticks.
		 */
		idle_original_ticks = max_system_ticks - 1;
	} else {
		/* Leave one tick margin time to react when coming back */
		idle_original_ticks = ticks - 1;
	}
	/* Set timer to virtual "one shot" mode. */
	timer_mode = TIMER_MODE_ONE_SHOT;
	idle_mode = IDLE_TICKLESS;
	/*
	 * We're being asked to have the timer fire in "ticks" from now. To
	 * maintain accuracy we must account for the remaining time left in the
	 * timer to the next tick to fire, so that the programmed fire time
	 * corresponds always on a tick bondary.
	 */
	P = GET_TIMER_CURRENT_TIME();
	f = GET_TIMER_FIRE_TIME();
	/*
	 * Get the time of last tick. As we are entring idle mode we are sure
	 * that |f - P| < cycles_per_tick.
	 * |-------f----P---|--------|--------|----F---|--------|--------|
	 * |-------|----P---f--------|--------|----F---|--------|--------|
	 *              P   f-----------s--------->F
	 */
	if (f < P) {
		f = f + cycles_per_tick;
	}
	F = f + idle_original_ticks * cycles_per_tick;
	/* Program the timer register to fire at the right time */
	SET_TIMER_FIRE_TIME(F);
#endif	/* CONFIG_TICKLESS_KERNEL */
}

/**
 *
 * @brief Handling of tickless idle when interrupted
 *
 * The routine, called by _sys_power_save_idle_exit, is responsible for taking
 * the timer out of idle mode and generating an interrupt at the next
 * tick interval.  It is expected that interrupts have been disabled.
 * Note that in this routine, _sys_idle_elapsed_ticks must be zero because the
 * ticker has done its work and consumed all the ticks. This has to be true
 * otherwise idle mode wouldn't have been entered in the first place.
 *
 * @return N/A
 */
void _timer_idle_exit(void)
{
#ifdef CONFIG_TICKLESS_KERNEL
	if (!idle_original_ticks) {
		_set_max_clock_time();
	}
#else
	u32_t C; /* Current time (time within this function execution) */
	u32_t F; /* Idle timer programmed fire time */
	u32_t s; /* Requested idle timer sleep time */
	u32_t e; /* elapsed "Cer time" */
	u32_t r; /*reamining time to the timer to expire */

	if (timer_mode == TIMER_MODE_PERIODIC) {
		/*
		 * The timer interrupt handler is handling a completed tickless
		 * idle
		 * or this has been called by mistake; there's nothing to do
		 * here.
		 */
		return;
	}

	/*
	 * This is a tricky logic where we use the particularity of unsigned
	 * integers computation and overflow/underflow to check for timer expiry
	 * In adddition to above defined variables, let's define following ones:
	 * P := Programming time (time within _timer_idle_enter execution)
	 * M := Maximum programmable value (0xFFFFFFFF)
	 *
	 * First case:
	 0----fired---->P-----not fired---->F---------------fired------------M
	 0              P<------------s-----F                                M
	 0              P        C<---r-----F                                M
	 0   C<---------P-------------r-----F                                M
	 0--------------P-------------r-----F              C<----------------M
	 *
	 * Second case:
	 0--not fired-->F-------fired------>P--------------not-fired---------M
	 0--------s-----F                   P<-------------------------------M
	 0--------r-----F        C<---------P--------------------------------M
	 0   C<---r-----F                   P                                M
	 0--------r-----F                   P              C<----------------M
	 *
	 * On both case, the timer fired when and only when r >= s.
	 */
	F = GET_TIMER_FIRE_TIME();
	s = idle_original_ticks * cycles_per_tick; /* also s = F - P; */
	C = GET_TIMER_CURRENT_TIME();
	r = F - C;
	/*
	 * Announce elapsed ticks to the kernel. Note we are  guaranteed
	 * that the timer ISR will execute before the tick event is serviced,
	 * so _sys_idle_elapsed_ticks is adjusted to account for it.
	 */
	e = s - r; /* also e = (C > P ? C - P : C - P + M); */
	_sys_idle_elapsed_ticks = e / cycles_per_tick;
	if (r >= s) {
		/*
		 * The timer expired. There is nothing to do for this use case.
		 * There is no need to reprogram the timer, the interrupt is
		 * being serviced, and the timer ISR will be called after this
		 * function returns.
		 */
	} else {
		/*
		 * System was interrupted before the timer fires.
		 * Reprogram to fire on tick edge: F := C + (r % cpt).
		 */
		F = C + (r - _sys_idle_elapsed_ticks * cycles_per_tick);
		C = GET_TIMER_CURRENT_TIME(); /* Update current time value */
		if (F - C < MIN_TIMER_PROG_DELAY) {
			/*
			 * We are too close to the next tick edge. Let's fire
			 * it manually and reprogram timer to fire on next one.
			 */
			F += cycles_per_tick;
			_sys_idle_elapsed_ticks += 1;
		}
		SET_TIMER_FIRE_TIME(F);
	}
	if (_sys_idle_elapsed_ticks) {
		_sys_clock_tick_announce();
	}

	/* Exit timer idle mode */
	idle_mode = IDLE_NOT_TICKLESS;
	timer_mode = TIMER_MODE_PERIODIC;
#endif	/* CONFIG_TICKLESS_KERNEL */
}
#endif /* CONFIG_TICKLESS_IDLE */

#if CONFIG_XTENSA_INTERNAL_TIMER || (CONFIG_XTENSA_TIMER_IRQ < 0)

// Internal timer
extern void _zxt_tick_timer_init(void);
unsigned int _xt_tick_divisor;  /* cached number of cycles per tick */

#ifdef CONFIG_XTENSA_ASM2
void _zxt_tick_timer_init(void)
{
	int val;

	__asm__ volatile("rsr.intenable %0" : "=r"(val));
	val |= 1 << TIMER_IRQ;
	__asm__ volatile("wsr.intenable %0" : : "r"(val));
	__asm__ volatile("rsync");

	SET_TIMER_FIRE_TIME(GET_TIMER_CURRENT_TIME() + _xt_tick_divisor);
}
#endif

#ifdef CONFIG_SMP
/**
 * @brief Timer initialization for SMP auxiliary CPUs
 *
 * Called on MP CPUs other than zero.  Some architectures appear to
 * generate spurious timer interrupts during initialization, so this
 * function must be called late in the SMP initialization sequence.
 */
void smp_timer_init(void)
{
	_zxt_tick_timer_init();
}
#endif

/*
 * Compute and initialize at run-time the tick divisor (the number of
 * processor clock cycles in an RTOS tick, used to set the tick timer).
 * Called when the processor clock frequency is not known at compile-time.
 */
void _xt_tick_divisor_init(void)
{
#ifdef XT_CLOCK_FREQ

	_xt_tick_divisor = (XT_CLOCK_FREQ / XT_TICK_PER_SEC);

#else
	#ifdef XT_BOARD
	_xt_tick_divisor = xtbsp_clock_freq_hz() / XT_TICK_PER_SEC;
#else
#error "No way to obtain processor clock frequency"
#endif  /* XT_BOARD */

#endif /* XT_CLOCK_FREQ */
}
#endif /* CONFIG_XTENSA_INTERNAL_TIMER || (CONFIG_XTENSA_TIMER_IRQ < 0) */


/**
 *
 * @brief System clock tick handler
 *
 * This routine handles the system clock periodic tick interrupt. It always
 * announces one tick by pushing a TICK_EVENT event onto the kernel stack.
 *
 * @return N/A
 */
void _timer_int_handler(void *params)
{
	ARG_UNUSED(params);

#ifdef CONFIG_EXECUTION_BENCHMARKING
	extern void read_timer_start_of_tick_handler(void);
	read_timer_start_of_tick_handler();
#endif

#ifdef CONFIG_XTENSA_ASM2
	/* FIXME: the legacy xtensa code did this in the assembly
	 * hook, and was a little more sophisticated.  We should track
	 * the delta from the last set time, not the current time.
	 * And the earlier code was even prepared to handle missed
	 * ticks by calling the handler function multiple times if the
	 * delta was more than one _xt_tick_divisor.
	 */
	SET_TIMER_FIRE_TIME(GET_TIMER_CURRENT_TIME() + _xt_tick_divisor);
#endif

	sys_trace_isr_enter();

#ifdef CONFIG_SMP
	/* The timer infractructure isn't prepared to handle
	 * asynchronous timeouts on multiple CPUs.  In SMP we use the
	 * timer interrupt on auxiliary CPUs only for scheduling.
	 * Don't muck up the timeout math.
	 */
	if (_arch_curr_cpu()->id) {
		return;
	}
#endif

#ifdef CONFIG_TICKLESS_KERNEL
	if (!idle_original_ticks) {
		_set_max_clock_time();
		return;
	}

	_sys_idle_elapsed_ticks = idle_original_ticks;
	idle_original_ticks = 0;
	/* Anounce elapsed of _sys_idle_elapsed_ticks systicks */
	_sys_clock_tick_announce();

	/* Program timer incase it is not Prgrammed */
	if (!idle_original_ticks) {
		_set_max_clock_time();
		return;
	}
#else
	/* Announce the tick event to the kernel. */
	_sys_clock_final_tick_announce();
#endif	/* CONFIG_TICKLESS_KERNEL */

#ifdef CONFIG_EXECUTION_BENCHMARKING
	extern void read_timer_end_of_tick_handler(void);
	read_timer_end_of_tick_handler();
#endif
}


/**
 *
 * @brief Initialize and enable the system clock
 *
 * This routine is used to program the systick to deliver interrupts at the
 * rate specified via the 'sys_clock_us_per_tick' global variable.
 *
 * @return 0
 */
int _sys_clock_driver_init(struct device *device)
{
	IRQ_CONNECT(TIMER_IRQ, 0, _timer_int_handler, 0, 0);

#if CONFIG_XTENSA_INTERNAL_TIMER || (CONFIG_XTENSA_TIMER_IRQ < 0)
	_xt_tick_divisor_init();
	/* Set up periodic tick timer (assume enough time to complete init). */
	_zxt_tick_timer_init();
#else /* Case of an external timer which is not emulated by internal timer */
	/*
	 * The code below is just an example code that is provided for Xtensa
	 * customers as an example of how to support external timers.
	 * The TODOs are here to tell customer what shall be re-implemented.
	 * This implementation is not fake, it works with an external timer that
	 * is provided as a systemC example and that could be plugged by using:
	 *    make run EMU_PLATFORM=xtsc-run.
	 *
	 *
	 * The address below is that of the systemC timer example, provided in
	 * ${ZEPHYR_BASE}/board/xt-sim/xtsc-models/external-irqs.
	 * Hopefully, this hard-coded address doesn't conflict with anything
	 * User needs for sure to rewrite this code to fit his timer.
	 * I do agree that this hope is unlikely to be satisfied, but users who
	 * don't have external timer will never hit here, and those who do, will
	 * for sure modify this code in order to initialize their HW.
	 */
	/* TODO: Implement this case: remove below code and write yours */
	volatile u32_t *p_mmio = (u32_t *) 0xC0000000; /* start HW reg */
	u32_t interrupt = 0x00000000;
	/* Start the timer: Trigger the interrupt source drivers */
	*p_mmio = MAX_TIMER_CYCLES;
	*p_mmio = interrupt;
	/*
	 * Code above is example code, it is kept here on purpose to let users
	 * find all code related to external timer support on the same file.
	 * They will have to rewrite this anyway.
	 *
	 * Code below (enabling timer IRQ) is likely to reamin as is.
	 */
	/* Enable the interrupt handler */
	irq_enable(CONFIG_XTENSA_TIMER_IRQ);
#endif /* CONFIG_XTENSA_INTERNAL_TIMER || (CONFIG_XTENSA_TIMER_IRQ < 0) */
#if CONFIG_TICKLESS_IDLE
	tickless_idle_init();
#endif	/* CONFIG_TICKLESS_KERNEL */
	return 0;
}


/**
 *
 * @brief Read the platform's timer hardware
 *
 * This routine returns the current time in terms of timer hardware clock
 * cycles.
 *
 * @return up counter of elapsed clock cycles
 */
u32_t _timer_cycle_get_32(void)
{
	return GET_TIMER_CURRENT_TIME();
}

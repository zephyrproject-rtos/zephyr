/*
 * Copyright (c) 2017, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>
#include <sys/printk.h>
#include <xtensa-asm2.h>

#ifdef CONFIG_MULTITHREADING
#error Disable multithreading for this unit test!
#endif

/* Just random numbers intended to whiten the register contents during
 * the spill test and make every bit of every register in every call
 * significant in an attempt to catch any mistakes/swaps/etc...
 */
int white[] = {
	       0x5fad484a,
	       0xc23e88f7,
	       0xfff301fb,
	       0xf1189ba7,
	       0x88bffad6,
	       0xaabb96fa,
	       0x629619d5,
	       0x246bee82
};

static inline unsigned int ccount(void)
{
	unsigned int cc;

	__asm__ volatile("rsr.ccount %0" : "=r"(cc));
	return cc;
}

/* We call spill_fn() through a pointer to prevent the compiler from
 * detecting and optimizing out the tail recursion in fn() and forcing
 * a real function call using CALLn instructions.
 */
int (*spill_fnp)(int level, int a, int b, int c);

/* WINDOWBASE/WINDOWSTART registers tested before and after the spill */
unsigned int spill_wb0, spill_ws0, spill_wb1, spill_ws1;

/* Test start/end values for CCOUNT */
unsigned int spill_start, spill_end;

/* Validated result for spill_fn() */
int spill_expect;

enum {
	NO_SPILL, HAL_SPILL, ZEPHYR_SPILL, NUM_MODES
} spill_mode;

static int spill_fn(int level, int a, int b, int c)
{
	/* Be very careful when debugging, note that a printk() call
	 * tends to push all the registers out of the windows on its
	 * own, leaving no frames for us to test against!
	 */
	if (level >= ARRAY_SIZE(white)) {
		__asm__ volatile ("rsr.WINDOWBASE %0" : "=r"(spill_wb0));
		__asm__ volatile ("rsr.WINDOWSTART %0" : "=r"(spill_ws0));

		spill_start = ccount();

		if (spill_mode == NO_SPILL) {
			/* Just here to test the cycle count overhead
			 * and get the baseline function result.
			 */
		} else if (spill_mode == ZEPHYR_SPILL) {
			/* FIXME: the a0_save hack should be needless.  It
			 * *should* be enough to list "a0" in the clobber list
			 * of the __asm__ statement (and let the compiler
			 * decide on how to save the value), but that's not
			 * working for me...
			 */
			int a0_save;

			__asm__ volatile
				("mov %0, a0"			"\n\t"
				 "call0 spill_reg_windows"	"\n\t"
				 "mov a0, %0"			"\n\t"
				 : "=r"(a0_save));
		} else if (spill_mode == HAL_SPILL) {
			/* Strictly there is a xthal_window_spill_nw
			 * routine that is called with special setup
			 * (use CALL0, spill A2/A3, clear WOE) and
			 * supposed to be faster, but I couldn't make
			 * that work.
			 */
			extern void xthal_window_spill(void);
			xthal_window_spill();
		}

		spill_end = ccount();
		__asm__ volatile ("rsr.WINDOWBASE %0" : "=r" (spill_wb1));
		__asm__ volatile ("rsr.WINDOWSTART %0" : "=r" (spill_ws1));

		return ((a + b) | c);
	}

	int val1 = (a - (b & c)) ^ white[level];
	int val2 = ((a | b) + c) ^ white[(level + 1) % ARRAY_SIZE(white)];
	int val3 = (a - (b - c)) ^ white[(level + 2) % ARRAY_SIZE(white)];

	int x = spill_fnp(level+1, val1, val2, val3);

	/* FIXME: as it happens, the compiler seems not to be
	 * optimizing components of this addition before the function
	 * call, which is what we want: the desire is that the
	 * individual values be held in registers across the call so
	 * that they can be checked to have been spilled/filled
	 * properly as we return up the stack.  But the compiler
	 * certainly COULD reorder this addition (it would actually be
	 * a good optimization: you could reduce the number of
	 * registers used before the tail return and use a smaller
	 * call frame).  For now, I'm happy enough simply having read
	 * the generated code, but long term this should be a more
	 * robust test if possible.  Maybe write the values to some
	 * extern volatile spots...
	 */
	return x + val1 + val2 + val3 + a + b + c;
}

int test_reg_spill(void)
{
	spill_fnp = spill_fn;

	int ok = 1;

	for (spill_mode = 0; spill_mode < NUM_MODES; spill_mode++) {
		printk("Testing %s\n",
		       spill_mode == NO_SPILL ? "NO_SPILL"
		       : (spill_mode == HAL_SPILL ? "HAL_SPILL"
			  : "ZEPHYR_SPILL"));

		int result = spill_fnp(0, 1, 2, 3);

		printk("  WINDOWBASE %d -> %d, WINDOWSTART 0x%x -> 0x%x (%d cycles)\n",
		       spill_wb0, spill_wb1, spill_ws0, spill_ws1,
		       spill_end - spill_start);

		if (spill_mode == NO_SPILL) {
			spill_expect = result;
			continue;
		}

		if (spill_ws1 != 1 << spill_wb1) {
			printk("WINDOWSTART should show exactly one frame at WINDOWBASE\n");
			ok = 0;
		}

		if (result != spill_expect) {
			printk("Unexpected fn(1, 2, 3) result, got %d want %d\n",
			       result, spill_expect);
			ok = 0;
		}
	}

	return ok;
}

int *test_highreg_handle;

/* Simple save locations for some context needed by the test assembly */
void *_test_highreg_sp_save;
void *_test_highreg_a0_save;

int test_highreg_stack[64];

int *test_highreg_sp_top = &test_highreg_stack[ARRAY_SIZE(test_highreg_stack)];

/* External function, defined in assembly */
void fill_window(void (*fn)(void));

/* Test rig for fill_window, maybe remove as a metatest */
int testfw_wb, testfw_ws;
void testfw(void);

/* Assembly-defined leaf functions for fill_window which poke the
 * specified number of high GPRs before calling xtensa_save_high_regs
 * to spill them into the test_highreg_stack area for inspection.
 */
void test_highreg_0(void);
void test_highreg_4(void);
void test_highreg_8(void);
void test_highreg_12(void);

typedef void (*test_fn_t)(void);
test_fn_t highreg_tests[] = {
	test_highreg_0,
	test_highreg_4,
	test_highreg_8,
	test_highreg_12,
};

int test_highreg_save(void)
{
	int ok = 1;

	fill_window(testfw);
	printk("testfw wb %d ws 0x%x\n", testfw_wb, testfw_ws);
	ok = ok && (testfw_ws == ((1 << (XCHAL_NUM_AREGS / 4)) - 1));

	for (int i = 0; i < ARRAY_SIZE(highreg_tests); i++) {
		printk("\nHighreg test %d\n", i);

		fill_window(highreg_tests[i]);

		ok = ok && (*test_highreg_handle == (int)test_highreg_sp_top);

		int spilled_words = test_highreg_sp_top - test_highreg_handle;

		for (int quad = 0; ok && quad < (spilled_words - 1)/4; quad++) {
			int *qbase = test_highreg_sp_top - (quad + 1) * 4;

			for (int ri = 0; ri < 4; ri++) {
				int reg = 4 + quad * 4 + ri;

				ok = ok && (qbase[ri] == reg);
				printk("  q %d reg %d qb[%d] %d\n",
				       quad, reg, ri, qbase[ri]);
			}
		}
	}

	return ok;
}

void *switch_handle0, *switch_handle1;

void xtensa_switch(void *handle, void **old_handle);

void test_switch_bounce(void);
__asm__("test_switch_bounce:"	"\n\t"
	"call4 test_switch_top"	"\n\t");

volatile int switch_count;

/* Sits in a loop switching back to handle0 (which is the main thread) */
void test_switch_top(void)
{
	int n = 1;

	while (1) {
		switch_count = n++;
		xtensa_switch(switch_handle0, &switch_handle1);
	}
}

int test_switch(void)
{
	static int stack2[512];

	printk("%s\n", __func__);

	(void)memset(stack2, 0, sizeof(stack2));

	int *sp = xtensa_init_stack(&stack2[ARRAY_SIZE(stack2)],
				    (void *)test_switch_bounce,
				    0, 0, 0);

#if 0
	/* DEBUG: dump the stack contents for manual inspection */
	for (int i = 0; i < 64; i++) {
		int idx = ARRAY_SIZE(stack2) - (i+1);
		int off = (i+1) * -4;
		int *addr = &stack2[idx];

		if (addr < sp) {
			break;
		}
		printk("%p (%d): 0x%x\n", addr, off, stack2[idx]);
	}
	printk("sp: %p\n", sp);
#endif

	switch_handle1 = sp;

	const int n_switch = 10;

	for (int i = 0; i < n_switch; i++) {
		xtensa_switch(switch_handle1, &switch_handle0);
		/* printk("switch %d count %d\n", i, switch_count); */
	}

	return switch_count == n_switch;
}

void rfi_jump(void);
void rfi_jump_c(void)
{
	int ps;

	__asm__ volatile ("rsr.PS %0" : "=r"(ps));
	printk("%s, PS = %xh\n", __func__, ps);
}

int xstack_ok;

#define XSTACK_SIZE 1024
#define XSTACK_CANARY 0x5a5aa5a5
static int xstack_stack2[XSTACK_SIZE + 1];

void do_xstack_call(void *new_stack); /* in asmhelp.S */

void xstack_bottom(void)
{
	xstack_ok = 1;
}

void xstack_top(void)
{
	int on_my_stack;

	printk("%s oms %p\n", __func__, &on_my_stack);

	/* Do this via fill_window() to be absolutely sure the whole
	 * call stack across both physical stacks got spilled and
	 * filled properly.
	 */
	fill_window(xstack_bottom);
}

int test_xstack(void)
{
	/* Make the stack one element big and put a canary above it to
	 * check nothing underflows
	 */

	int *new_stack = &xstack_stack2[XSTACK_SIZE];
	*new_stack = XSTACK_CANARY;

	printk("%s new_stack = %p\n", __func__, new_stack);

	do_xstack_call(new_stack);

	printk("xstack_ok %d stack2[%d] 0x%x\n",
	       xstack_ok, XSTACK_SIZE, xstack_stack2[XSTACK_SIZE]);

	return xstack_ok && xstack_stack2[XSTACK_SIZE] == XSTACK_CANARY;
}

#ifdef CONFIG_SOC_ESP32
#define TIMER_INT 16
#else
#define TIMER_INT 13
#endif

volatile int timer2_fired;

int excint_stack[8192];
void *excint_stack_top = &excint_stack[ARRAY_SIZE(excint_stack)];

static struct { int nest; void *stack_top; } excint_cpu;

volatile int int5_result;

void disable_timer(void)
{
	int ie;

	__asm__ volatile("rsr.intenable %0" : "=r"(ie));
	ie &= ~(1<<TIMER_INT);
	__asm__ volatile("wsr.intenable %0; rsync" : : "r"(ie));
}

void enable_timer(void)
{
	int ie;

	__asm__ volatile("rsr.intenable %0" : "=r"(ie));
	ie |= (1<<TIMER_INT);
	__asm__ volatile("wsr.intenable %0; rsync" : : "r"(ie));
}

void *handle_int5_c(void *handle)
{
	int5_result = spill_fnp(0, 3, 2, 1);

	int ccompare2_val = ccount() - 1;

	__asm__ volatile("wsr.ccompare2 %0; rsync" : : "r"(ccompare2_val));

	disable_timer();

	timer2_fired = 1;

	return handle;
}

int interrupt_test(void)
{
	int ok = 1;

	excint_cpu.nest = 0;
	excint_cpu.stack_top = &excint_stack[ARRAY_SIZE(excint_stack)];

	void *cpuptr = &excint_cpu;

	__asm__ volatile("wsr.MISC0 %0" : : "r"(cpuptr));

	/* We reuse the "spill_fn" logic from above to get a
	 * stack-sensitive, deeply-recursive computation going that
	 * will be sensitive to interrupt bugs
	 */
	spill_mode = NO_SPILL;

	unsigned int start = ccount();
	int expect = spill_fnp(0, 3, 2, 1);
	unsigned int spill_time = ccount() - start;

	/* Ten thousand iterations is still pretty quick */
	for (int i = 0; i < 10000; i++) {
		int nest = i & 1;

		excint_cpu.nest = nest;
		timer2_fired = 0;

		/* Vaguely random delay between 2-8 iterations of
		 * spill_fn().  Maybe improve with a real PRNG.
		 */
		const int max_reps = 8;
		int wh = white[i % ARRAY_SIZE(white)];
		int delay = spill_time * 2U
			+ ((wh * (i+1)) % (spill_time * (max_reps - 2)));

		int alarm = ccount() + delay;

		__asm__ volatile("wsr.ccompare2 %0; rsync" : : "r"(alarm));

		enable_timer();

#if 0
		/* This is what I want to test: run the spill_fn test
		 * repeatedly in the main thread so that it can be
		 * interrupted and restored, and validate that it
		 * returns the same result every time.  But this can't
		 * work, even in principle: the timer interrupt we are
		 * using is "high priority", which means that it can
		 * interrupt the window exceptions being thrown in the
		 * main thread.  And by design, Xtensa window
		 * exceptions CANNOT be made reentrant (they don't
		 * save the interrupted state, so can be interrupted
		 * again before they can mask off exceptions, which
		 * will then lose/clobber the OWB field in PS when the
		 * interrupt handler throws another window exception).
		 * So this doesn't work, in fact it fails every 2-10
		 * iterations as spill_fn spends a lot of its time
		 * spill/filling stack frames (by design, of course).
		 *
		 * This could be made to work if we could repurpose
		 * the existing medium priority timer interrupt (which
		 * is hard in a unit test: that's an important
		 * interrupt!) or use the low priority timer which
		 * delivers to the global exception handler (basically
		 * impossible in a unit test).  Frustrating.
		 */
		int reps = 0;

		while (!timer2_fired && reps < (max_reps+2)) {
			int result = spill_fnp(0, 3, 2, 1);

			reps++;
			if (result != expect) {
				ok = 0;
			}
		}
		if (reps >= max_reps+2) {
			printk("Interrupt didn't arrive\n");
			ok = 0;
		}
		if (int5_result != expect) {
			printk("Unexpected int spill_fn() result\n");
			ok = 0;
		}
		printk("INT test delay %d nest %d reps %d\n",
		       delay, nest, reps);
#else
		/* So this is what we do instead: just spin in the
		 * main thread calling functions that don't involve
		 * exceptions.  By experiment, calling spill_fn with a
		 * first (depth) argument of 6 or 7 results in a
		 * shallow call tree that won't throw exepctions.  At
		 * least we're executing real code which depends on
		 * its register state and validating that interrupts
		 * don't hurt.
		 */
		volatile int dummy = 1;

		while (!timer2_fired) {
			dummy = spill_fnp(6, dummy, 2, 3);
		}
		if (int5_result != expect) {
			printk("Unexpected int spill_fn() result\n");
			ok = 0;
		}
#endif
	}
	return ok;
}

void main(void)
{
	/* Turn off interrupts and leave disabled, otherwise the
	 * "userspace" context switching tests might not be reliable.
	 * Stack pointers can exist in indeterminate states here.
	 * (Note: the interrupt test below is using a high priority
	 * interrupt which is not masked by irq_lock(), so it doesn't
	 * care).
	 */
	int key = irq_lock();

	/* Strictly not a "test", we just want to know that the jump
	 * worked.  If the rest of the code runs, this must have
	 * "passed".
	 */
	rfi_jump();

	int ok = 1;

	ok = ok && test_reg_spill();

	ok = ok && test_highreg_save();

	ok = ok && test_switch();

	ok = ok && test_xstack();

	ok = ok && interrupt_test();

	irq_unlock(key);

	printk("%s\n", ok ? "OK" : "Failed");
}

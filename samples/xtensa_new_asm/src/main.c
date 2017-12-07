/*
 * Copyright (c) 2017, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>
#include <misc/printk.h>

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

/* We call spill_fn() through a pointer to prevent the compiler from
 * detecting and optimizing out the tail recursion in fn() and forcing
 * a real function call using CALLn instructions.
 */
int (*spill_fnp)(int level, int a, int b, int c);

/* WINDOWBASE/WINDOWSTART registers tested before and after the spill */
unsigned int spill_wb0, spill_ws0, spill_wb1, spill_ws1;

/* Test start/end values for CCOUNT */
unsigned int spill_start, spill_end;

enum {
	NO_SPILL, HAL_SPILL, ZEPHYR_SPILL, NUM_MODES
} spill_mode;

static int spill_fn(int level, int a, int b, int c)
{
	// Be very careful when debugging, note that a printk() call
	// tends to push all the registers out of the windows on its
	// own, leaving no frames for us to test against!
	if(level >= ARRAY_SIZE(white)) {
		__asm__ volatile ("rsr.WINDOWBASE %0" : "=r"(spill_wb0));
		__asm__ volatile ("rsr.WINDOWSTART %0" : "=r"(spill_ws0));

		__asm__ volatile ("rsr.CCOUNT %0" : "=r"(spill_start));

		if (spill_mode == NO_SPILL) {
			// Just here to test the cycle count overhead
			// and get the baseline function result.
		} else if (spill_mode == ZEPHYR_SPILL) {
			// FIXME: the a0_save hack should be needless.  It
			// *should* be enough to list "a0" in the clobber list
			// of the __asm__ statement (and let the compiler
			// decide on how to save the value), but that's not
			// working for me...
			int a0_save;
			__asm__ volatile
				("mov %0, a0" "\n\t"
				 "call0 spill_reg_windows" "\n\t"
				 "mov a0, %0" "\n\t"
				 : "=r"(a0_save));
		} else if (spill_mode == HAL_SPILL) {
			// Strictly there is a xthal_window_spill_nw
			// routine that is called with special setup
			// (use CALL0, spill A2/A3, clear WOE) and
			// supposed to be faster, but I couldn't make
			// that work.
			extern void xthal_window_spill(void);
			xthal_window_spill();
		}

		__asm__ volatile ("rsr.CCOUNT %0" : "=r" (spill_end) );
		__asm__ volatile ("rsr.WINDOWBASE %0" : "=r" (spill_wb1) );
		__asm__ volatile ("rsr.WINDOWSTART %0" : "=r" (spill_ws1));

		return ((a + b) | c);
	}

	int val1 = (a - (b & c)) ^ white[level];
	int val2 = ((a | b) + c) ^ white[(level + 1) % ARRAY_SIZE(white)];
	int val3 = (a - (b - c)) ^ white[(level + 2) % ARRAY_SIZE(white)];

	int x = spill_fnp(level+1, val1, val2, val3);

	// FIXME: as it happens, the compiler seems not to be
	// optimizing components of this addition before the function
	// call, which is what we want: the desire is that the
	// individual values be held in registers across the call so
	// that they can be checked to have been spilled/filled
	// properly as we return up the stack.  But the compiler
	// certainly COULD reorder this addition (it would actually be
	// a good optimization: you could reduce the number of
	// registers used before the tail return and use a smaller
	// call frame).  For now, I'm happy enough simply having read
	// the generated code, but long term this should be a more
	// robust test if possible.  Maybe write the values to some
	// extern volatile spots...
	return x + val1 + val2 + val3 + a + b + c;
}

int test_reg_spill(void)
{
	spill_fnp = spill_fn;

	int ok = 1;
	int expect = 0;

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
			expect = result;
			continue;
		}

		if (spill_ws1 != 1 << spill_wb1) {
			printk("WINDOWSTART should show exactly one frame at WINDOWBASE\n");
			ok = 0;
		}

		if (result != expect) {
			printk("Unexpected fn(1, 2, 3) result, got %d want %d\n",
			       result, expect);
			ok = 0;
		}
	}

	return ok;
}

int *test_highreg_handle;

// Simple save locations for some context needed by the test assembly
void *_test_highreg_sp_save;
void *_test_highreg_a0_save;

int test_highreg_stack[64];

int *test_highreg_sp_top = &test_highreg_stack[ARRAY_SIZE(test_highreg_stack)];

// External function, defined in assembly
void fill_window(void (*fn)(void));

// Test rig for fill_window, maybe remove as a metatest
int testfw_wb, testfw_ws;
void testfw(void);

// Assembly-defined leaf functions for fill_window which poke the
// specified number of high GPRs before calling xtensa_save_high_regs
// to spill them into the test_highreg_stack area for inspection.
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
				printk("  q %d reg %d qb[%d] %d\n", quad, reg, ri, qbase[ri]);
			}
		}
	}

	return ok;
}

#if XCHAL_HAVE_LOOPS
#define BASE_SAVE_AREA_SIZE 56
#else
#define BASE_SAVE_AREA_SIZE 44
#endif

void *switch_handle0, *switch_handle1;

void xtensa_switch(void *handle, void **old_handle);

void test_switch_bounce(void);
__asm__("test_switch_bounce:"	"\n\t"
	"call4 test_switch_top"	"\n\t");

void test_switch_top(void)
{
	//printk("switched stack!\n");
	xtensa_switch(switch_handle0, &switch_handle1);
	while(1) { /* never reached */ }
}

int test_switch(void)
{
	static int stack2[512];

	memset(stack2, 0, sizeof(stack2));

	int *sp = &stack2[ARRAY_SIZE(stack2)];

	int restore_ps, restore_pc;
	__asm__ volatile ("rsr.PS %0" : "=r"(restore_ps));
	restore_pc = (int)test_switch_bounce;

	*(--sp) = 0; // caller spill
	*(--sp) = 0;
	*(--sp) = 0;
	*(--sp) = 0;

	*(--sp) = 0; // a0
	*(--sp) = 0; // unused
	*(--sp) = 0; // a2
	*(--sp) = 0; // a3

	*(--sp) = restore_pc;
	*(--sp) = restore_ps;

	*(--sp) = 0; // SAR
#if XCHAL_HAVE_LOOPS
	*(--sp) = 0; // LBEG
	*(--sp) = 0; // LEND
	*(--sp) = 0; // LCOUNT
#endif

	// Finally push the intermediate stack pointer (which is
	// unchanged, since we don't use high registers)
	*(sp-1) = (int)sp;
	sp--;

#if 0
	for(int i=0; i<16; i++) {
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

	xtensa_switch(sp, &switch_handle0);

	printk("Switched back, handles 0 %p 1 %p\n", switch_handle0, switch_handle1);
	return switch_handle0 && switch_handle1;
}

void rfi_jump(void);
void rfi_jump_c(void)
{
	int ps;
	__asm__ volatile ("rsr.PS %0" : "=r"(ps));
	printk("rfi_jump_c, PS = %xh\n", ps);
}

int xstack_ok = 0;

#define XSTACK_SIZE 256
#define XSTACK_CANARY 0x5a5aa5a5
static int xstack_stack2[XSTACK_SIZE + 1];

void do_xstack_call(void *new_stack); // in asmhelp.S

void xstack_bottom(void)
{
	xstack_ok = 1;
}

void xstack_top(void)
{
	int on_my_stack;
	printk("xstack_top oms %p\n",&on_my_stack); 

	// Do this via fill_window() to be absolutely sure the whole
	// call stack across both physical stacks got spilled and
	// filled properly.
	fill_window(xstack_bottom);
}

int test_xstack(void)
{
	// Make the stack one element big and put a canary above it to
	// check nothing underflows

	int *new_stack = &xstack_stack2[XSTACK_SIZE];
	*new_stack = XSTACK_CANARY;

	printk("test_xstack new_stack = %p\n", new_stack);

	do_xstack_call(new_stack);

	printk("xstack_ok %d stack2[%d] 0x%x\n", xstack_ok, XSTACK_SIZE, xstack_stack2[XSTACK_SIZE]);

	return xstack_ok && xstack_stack2[XSTACK_SIZE] == XSTACK_CANARY;
}

void main(void)
{
	// Turn off interrupts and leave disabled, otherwise the
	// "userspace" context switching tests might not be reliable.
	// Stack pointers can exist in indeterminate states here.
	int key = irq_lock();

	// Strictly not a "test", we just want to know that the jump
	// worked.  If the rest of the code runs, this must have
	// "passed".
	rfi_jump();

	int ok = 1;

	ok = ok && test_reg_spill();

	ok = ok && test_highreg_save();

	ok = ok && test_switch();

	ok = ok && test_xstack();

	irq_unlock(key);

	printk("%s\n", ok ? "OK" : "Failed");
}

/*
 * Copyright (c) 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/*
 * THIS FILE WAS AUTOMATICALLY GENERATED.  DO NOT EDIT.
 *
 * Functions here are designed to produce efficient code to
 * search an Xtensa bitmask of interrupts, inspecting only those bits
 * declared to be associated with a given interrupt level.  Each
 * dispatcher will handle exactly one flagged interrupt, in numerical
 * order (low bits first) and will return a mask of that bit that can
 * then be cleared by the calling code.  Unrecognized bits for the
 * level will invoke an error handler.
 */

#include <xtensa/config/core-isa.h>
#include <zephyr/sys/util.h>
#include <zephyr/sw_isr_table.h>

#if !defined(XCHAL_INT0_LEVEL) || XCHAL_INT0_LEVEL != 5
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT1_LEVEL) || XCHAL_INT1_LEVEL != 2
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT2_LEVEL) || XCHAL_INT2_LEVEL != 2
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT16_LEVEL) || XCHAL_INT16_LEVEL != 2
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT17_LEVEL) || XCHAL_INT17_LEVEL != 2
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT18_LEVEL) || XCHAL_INT18_LEVEL != 2
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT19_LEVEL) || XCHAL_INT19_LEVEL != 2
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT20_LEVEL) || XCHAL_INT20_LEVEL != 2
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT21_LEVEL) || XCHAL_INT21_LEVEL != 2
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT22_LEVEL) || XCHAL_INT22_LEVEL != 2
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT23_LEVEL) || XCHAL_INT23_LEVEL != 2
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT3_LEVEL) || XCHAL_INT3_LEVEL != 3
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT4_LEVEL) || XCHAL_INT4_LEVEL != 3
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT5_LEVEL) || XCHAL_INT5_LEVEL != 3
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT24_LEVEL) || XCHAL_INT24_LEVEL != 3
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT25_LEVEL) || XCHAL_INT25_LEVEL != 3
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT26_LEVEL) || XCHAL_INT26_LEVEL != 3
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT27_LEVEL) || XCHAL_INT27_LEVEL != 3
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT28_LEVEL) || XCHAL_INT28_LEVEL != 3
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT29_LEVEL) || XCHAL_INT29_LEVEL != 3
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT30_LEVEL) || XCHAL_INT30_LEVEL != 3
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT31_LEVEL) || XCHAL_INT31_LEVEL != 3
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT6_LEVEL) || XCHAL_INT6_LEVEL != 1
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT7_LEVEL) || XCHAL_INT7_LEVEL != 1
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT8_LEVEL) || XCHAL_INT8_LEVEL != 1
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT9_LEVEL) || XCHAL_INT9_LEVEL != 1
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT10_LEVEL) || XCHAL_INT10_LEVEL != 1
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT11_LEVEL) || XCHAL_INT11_LEVEL != 1
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT12_LEVEL) || XCHAL_INT12_LEVEL != 1
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT13_LEVEL) || XCHAL_INT13_LEVEL != 1
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT14_LEVEL) || XCHAL_INT14_LEVEL != 1
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT15_LEVEL) || XCHAL_INT15_LEVEL != 1
#error core-isa.h interrupt level does not match dispatcher!
#endif

static inline int _xtensa_handle_one_int5(unsigned int mask)
{
	int irq;

	if (mask & BIT(0)) {
		mask = BIT(0);
		irq = 0;
		goto handle_irq;
	}
	return 0;
handle_irq:
	_sw_isr_table[irq].isr(_sw_isr_table[irq].arg);
	return mask;
}

static inline int _xtensa_handle_one_int2(unsigned int mask)
{
	int irq;

	if (mask & 0x70006) {
		if (mask & 0x6) {
			if (mask & BIT(1)) {
				mask = BIT(1);
				irq = 1;
				goto handle_irq;
			}
			if (mask & BIT(2)) {
				mask = BIT(2);
				irq = 2;
				goto handle_irq;
			}
		} else {
			if (mask & BIT(16)) {
				mask = BIT(16);
				irq = 16;
				goto handle_irq;
			}
			if (mask & BIT(17)) {
				mask = BIT(17);
				irq = 17;
				goto handle_irq;
			}
			if (mask & BIT(18)) {
				mask = BIT(18);
				irq = 18;
				goto handle_irq;
			}
		}
	} else {
		if (mask & 0x180000) {
			if (mask & BIT(19)) {
				mask = BIT(19);
				irq = 19;
				goto handle_irq;
			}
			if (mask & BIT(20)) {
				mask = BIT(20);
				irq = 20;
				goto handle_irq;
			}
		} else {
			if (mask & BIT(21)) {
				mask = BIT(21);
				irq = 21;
				goto handle_irq;
			}
			if (mask & BIT(22)) {
				mask = BIT(22);
				irq = 22;
				goto handle_irq;
			}
			if (mask & BIT(23)) {
				mask = BIT(23);
				irq = 23;
				goto handle_irq;
			}
		}
	}
	return 0;
handle_irq:
	_sw_isr_table[irq].isr(_sw_isr_table[irq].arg);
	return mask;
}

static inline int _xtensa_handle_one_int3(unsigned int mask)
{
	int irq;

	if (mask & 0x3000038) {
		if (mask & 0x18) {
			if (mask & BIT(3)) {
				mask = BIT(3);
				irq = 3;
				goto handle_irq;
			}
			if (mask & BIT(4)) {
				mask = BIT(4);
				irq = 4;
				goto handle_irq;
			}
		} else {
			if (mask & BIT(5)) {
				mask = BIT(5);
				irq = 5;
				goto handle_irq;
			}
			if (mask & BIT(24)) {
				mask = BIT(24);
				irq = 24;
				goto handle_irq;
			}
			if (mask & BIT(25)) {
				mask = BIT(25);
				irq = 25;
				goto handle_irq;
			}
		}
	} else {
		if (mask & 0x1c000000) {
			if (mask & BIT(26)) {
				mask = BIT(26);
				irq = 26;
				goto handle_irq;
			}
			if (mask & BIT(27)) {
				mask = BIT(27);
				irq = 27;
				goto handle_irq;
			}
			if (mask & BIT(28)) {
				mask = BIT(28);
				irq = 28;
				goto handle_irq;
			}
		} else {
			if (mask & BIT(29)) {
				mask = BIT(29);
				irq = 29;
				goto handle_irq;
			}
			if (mask & BIT(30)) {
				mask = BIT(30);
				irq = 30;
				goto handle_irq;
			}
			if (mask & BIT(31)) {
				mask = BIT(31);
				irq = 31;
				goto handle_irq;
			}
		}
	}
	return 0;
handle_irq:
	_sw_isr_table[irq].isr(_sw_isr_table[irq].arg);
	return mask;
}

static inline int _xtensa_handle_one_int1(unsigned int mask)
{
	int irq;

	if (mask & 0x7c0) {
		if (mask & 0xc0) {
			if (mask & BIT(6)) {
				mask = BIT(6);
				irq = 6;
				goto handle_irq;
			}
			if (mask & BIT(7)) {
				mask = BIT(7);
				irq = 7;
				goto handle_irq;
			}
		} else {
			if (mask & BIT(8)) {
				mask = BIT(8);
				irq = 8;
				goto handle_irq;
			}
			if (mask & BIT(9)) {
				mask = BIT(9);
				irq = 9;
				goto handle_irq;
			}
			if (mask & BIT(10)) {
				mask = BIT(10);
				irq = 10;
				goto handle_irq;
			}
		}
	} else {
		if (mask & 0x1800) {
			if (mask & BIT(11)) {
				mask = BIT(11);
				irq = 11;
				goto handle_irq;
			}
			if (mask & BIT(12)) {
				mask = BIT(12);
				irq = 12;
				goto handle_irq;
			}
		} else {
			if (mask & BIT(13)) {
				mask = BIT(13);
				irq = 13;
				goto handle_irq;
			}
			if (mask & BIT(14)) {
				mask = BIT(14);
				irq = 14;
				goto handle_irq;
			}
			if (mask & BIT(15)) {
				mask = BIT(15);
				irq = 15;
				goto handle_irq;
			}
		}
	}
	return 0;
handle_irq:
	_sw_isr_table[irq].isr(_sw_isr_table[irq].arg);
	return mask;
}

static inline int _xtensa_handle_one_int0(unsigned int mask)
{
	return 0;
}
static inline int _xtensa_handle_one_int4(unsigned int mask)
{
	return 0;
}
static inline int _xtensa_handle_one_int6(unsigned int mask)
{
	return 0;
}
static inline int _xtensa_handle_one_int7(unsigned int mask)
{
	return 0;
}
static inline int _xtensa_handle_one_int8(unsigned int mask)
{
	return 0;
}
static inline int _xtensa_handle_one_int9(unsigned int mask)
{
	return 0;
}
static inline int _xtensa_handle_one_int10(unsigned int mask)
{
	return 0;
}
static inline int _xtensa_handle_one_int11(unsigned int mask)
{
	return 0;
}
static inline int _xtensa_handle_one_int12(unsigned int mask)
{
	return 0;
}
static inline int _xtensa_handle_one_int13(unsigned int mask)
{
	return 0;
}
static inline int _xtensa_handle_one_int14(unsigned int mask)
{
	return 0;
}
static inline int _xtensa_handle_one_int15(unsigned int mask)
{
	return 0;
}

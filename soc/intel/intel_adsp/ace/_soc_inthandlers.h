/* Copyright (c) 2022 Intel Corporation
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

#if !defined(XCHAL_INT0_LEVEL) || XCHAL_INT0_LEVEL != 1
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT1_LEVEL) || XCHAL_INT1_LEVEL != 1
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT2_LEVEL) || XCHAL_INT2_LEVEL != 2
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT3_LEVEL) || XCHAL_INT3_LEVEL != 2
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT4_LEVEL) || XCHAL_INT4_LEVEL != 2
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT5_LEVEL) || XCHAL_INT5_LEVEL != 3
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT6_LEVEL) || XCHAL_INT6_LEVEL != 3
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT7_LEVEL) || XCHAL_INT7_LEVEL != 3
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT8_LEVEL) || XCHAL_INT8_LEVEL != 5
#error core-isa.h interrupt level does not match dispatcher!
#endif

static inline int _xtensa_handle_one_int1(unsigned int mask)
{
	int irq;

	if (mask & BIT(0)) {
		mask = BIT(0);
		irq = 0;
		goto handle_irq;
	}
	if (mask & BIT(1)) {
		mask = BIT(1);
		irq = 1;
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

	if (mask & BIT(2)) {
		mask = BIT(2);
		irq = 2;
		goto handle_irq;
	}
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
	return 0;
handle_irq:
	_sw_isr_table[irq].isr(_sw_isr_table[irq].arg);
	return mask;
}

static inline int _xtensa_handle_one_int3(unsigned int mask)
{
	int irq;

	if (mask & BIT(5)) {
		mask = BIT(5);
		irq = 5;
		goto handle_irq;
	}
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
	return 0;
handle_irq:
	_sw_isr_table[irq].isr(_sw_isr_table[irq].arg);
	return mask;
}

static inline int _xtensa_handle_one_int5(unsigned int mask)
{
	/* It is a Non-maskable interrupt handler.
	 * The non-maskable interrupt have no corresponding bit in INTERRUPT and INTENABLE registers
	 * so mask parameter is always 0.
	 */
	_sw_isr_table[8].isr(_sw_isr_table[8].arg);
	return 0;
}

static inline int _xtensa_handle_one_int0(unsigned int mask)
{
	return 0;
}
static inline int _xtensa_handle_one_int4(unsigned int mask)
{
	return 0;
}

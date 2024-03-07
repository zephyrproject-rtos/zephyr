/*
 * Copyright (c) 2021 NXP
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
#if !defined(XCHAL_INT1_LEVEL) || XCHAL_INT1_LEVEL != 3
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT2_LEVEL) || XCHAL_INT2_LEVEL != 2
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT3_LEVEL) || XCHAL_INT3_LEVEL != 3
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT4_LEVEL) || XCHAL_INT4_LEVEL != 2
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT5_LEVEL) || XCHAL_INT5_LEVEL != 2
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT6_LEVEL) || XCHAL_INT6_LEVEL != 2
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT7_LEVEL) || XCHAL_INT7_LEVEL != 2
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT8_LEVEL) || XCHAL_INT8_LEVEL != 1
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT9_LEVEL) || XCHAL_INT9_LEVEL != 2
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT10_LEVEL) || XCHAL_INT10_LEVEL != 2
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT11_LEVEL) || XCHAL_INT11_LEVEL != 2
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT12_LEVEL) || XCHAL_INT12_LEVEL != 2
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT13_LEVEL) || XCHAL_INT13_LEVEL != 2
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT14_LEVEL) || XCHAL_INT14_LEVEL != 2
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT15_LEVEL) || XCHAL_INT15_LEVEL != 2
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

static inline int _xtensa_handle_one_int1(unsigned int mask)
{
	int irq;

	if (mask & BIT(8)) {
		mask = BIT(8);
		irq = 8;
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
	int i = 0;

	mask &= XCHAL_INTLEVEL2_MASK;
	for (i = 0; i <= 31; i++)
		if (mask & BIT(i)) {
			mask = BIT(i);
			irq = i;
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

	if (mask & BIT(1)) {
		mask = BIT(1);
		irq = 1;
		goto handle_irq;
	}
	if (mask & BIT(3)) {
		mask = BIT(3);
		irq = 3;
		goto handle_irq;
	}
	if (mask & BIT(31)) {
		mask = BIT(31);
		irq = 31;
		goto handle_irq;
	}
	return 0;
handle_irq:
	_sw_isr_table[irq].isr(_sw_isr_table[irq].arg);
	return mask;
}

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

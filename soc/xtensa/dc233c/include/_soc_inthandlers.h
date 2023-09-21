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
#if !defined(XCHAL_INT2_LEVEL) || XCHAL_INT2_LEVEL != 1
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT3_LEVEL) || XCHAL_INT3_LEVEL != 1
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT4_LEVEL) || XCHAL_INT4_LEVEL != 1
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT5_LEVEL) || XCHAL_INT5_LEVEL != 1
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT6_LEVEL) || XCHAL_INT6_LEVEL != 1
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT7_LEVEL) || XCHAL_INT7_LEVEL != 1
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT15_LEVEL) || XCHAL_INT15_LEVEL != 1
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT16_LEVEL) || XCHAL_INT16_LEVEL != 1
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT17_LEVEL) || XCHAL_INT17_LEVEL != 1
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT18_LEVEL) || XCHAL_INT18_LEVEL != 1
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT19_LEVEL) || XCHAL_INT19_LEVEL != 1
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT20_LEVEL) || XCHAL_INT20_LEVEL != 1
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT8_LEVEL) || XCHAL_INT8_LEVEL != 2
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT9_LEVEL) || XCHAL_INT9_LEVEL != 3
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT10_LEVEL) || XCHAL_INT10_LEVEL != 3
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT11_LEVEL) || XCHAL_INT11_LEVEL != 3
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT21_LEVEL) || XCHAL_INT21_LEVEL != 3
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT12_LEVEL) || XCHAL_INT12_LEVEL != 4
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT13_LEVEL) || XCHAL_INT13_LEVEL != 5
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT14_LEVEL) || XCHAL_INT14_LEVEL != 7
#error core-isa.h interrupt level does not match dispatcher!
#endif

static inline int _xtensa_handle_one_int1(unsigned int mask)
{
	int irq;

	if (mask & 0x7f) {
		if (mask & 0x7) {
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
			if (mask & BIT(2)) {
				mask = BIT(2);
				irq = 2;
				goto handle_irq;
			}
		} else {
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
				if (mask & BIT(6)) {
					mask = BIT(6);
					irq = 6;
					goto handle_irq;
				}
			}
		}
	} else {
		if (mask & 0x18080) {
			if (mask & BIT(7)) {
				mask = BIT(7);
				irq = 7;
				goto handle_irq;
			}
			if (mask & BIT(15)) {
				mask = BIT(15);
				irq = 15;
				goto handle_irq;
			}
			if (mask & BIT(16)) {
				mask = BIT(16);
				irq = 16;
				goto handle_irq;
			}
		} else {
			if (mask & 0x60000) {
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
			} else {
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
			}
		}
	}
	return 0;
handle_irq:
	_sw_isr_table[irq].isr(_sw_isr_table[irq].arg);
	return mask;
}

static inline int _xtensa_handle_one_int2(unsigned int mask)
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

static inline int _xtensa_handle_one_int3(unsigned int mask)
{
	int irq;

	if (mask & 0x600) {
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
	} else {
		if (mask & BIT(11)) {
			mask = BIT(11);
			irq = 11;
			goto handle_irq;
		}
		if (mask & BIT(21)) {
			mask = BIT(21);
			irq = 21;
			goto handle_irq;
		}
	}
	return 0;
handle_irq:
	_sw_isr_table[irq].isr(_sw_isr_table[irq].arg);
	return mask;
}

static inline int _xtensa_handle_one_int4(unsigned int mask)
{
	int irq;

	if (mask & BIT(12)) {
		mask = BIT(12);
		irq = 12;
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

	if (mask & BIT(13)) {
		mask = BIT(13);
		irq = 13;
		goto handle_irq;
	}
	return 0;
handle_irq:
	_sw_isr_table[irq].isr(_sw_isr_table[irq].arg);
	return mask;
}

static inline int _xtensa_handle_one_int7(unsigned int mask)
{
	int irq;

	if (mask & BIT(14)) {
		mask = BIT(14);
		irq = 14;
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
static inline int _xtensa_handle_one_int6(unsigned int mask)
{
	return 0;
}

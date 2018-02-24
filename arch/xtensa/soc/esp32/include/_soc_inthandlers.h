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
#include <sw_isr_table.h>

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
#if !defined(XCHAL_INT8_LEVEL) || XCHAL_INT8_LEVEL != 1
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT9_LEVEL) || XCHAL_INT9_LEVEL != 1
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT10_LEVEL) || XCHAL_INT10_LEVEL != 1
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT12_LEVEL) || XCHAL_INT12_LEVEL != 1
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT13_LEVEL) || XCHAL_INT13_LEVEL != 1
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT17_LEVEL) || XCHAL_INT17_LEVEL != 1
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT18_LEVEL) || XCHAL_INT18_LEVEL != 1
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
#if !defined(XCHAL_INT11_LEVEL) || XCHAL_INT11_LEVEL != 3
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT15_LEVEL) || XCHAL_INT15_LEVEL != 3
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT22_LEVEL) || XCHAL_INT22_LEVEL != 3
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT23_LEVEL) || XCHAL_INT23_LEVEL != 3
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT27_LEVEL) || XCHAL_INT27_LEVEL != 3
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT29_LEVEL) || XCHAL_INT29_LEVEL != 3
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT24_LEVEL) || XCHAL_INT24_LEVEL != 4
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT25_LEVEL) || XCHAL_INT25_LEVEL != 4
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT28_LEVEL) || XCHAL_INT28_LEVEL != 4
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT30_LEVEL) || XCHAL_INT30_LEVEL != 4
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT16_LEVEL) || XCHAL_INT16_LEVEL != 5
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT26_LEVEL) || XCHAL_INT26_LEVEL != 5
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT31_LEVEL) || XCHAL_INT31_LEVEL != 5
#error core-isa.h interrupt level does not match dispatcher!
#endif
#if !defined(XCHAL_INT14_LEVEL) || XCHAL_INT14_LEVEL != 7
#error core-isa.h interrupt level does not match dispatcher!
#endif

static inline int _xtensa_handle_one_int0(unsigned int mask)
{
	return 0;
}

static inline int _xtensa_handle_one_int1(unsigned int mask)
{
	if (mask & 0x7f) {
		if (mask & 0x7) {
			if (mask & (1 << 0)) {
				struct _isr_table_entry *e = &_sw_isr_table[0];

				e->isr(e->arg);
				return 1 << 0;
			}
			if (mask & (1 << 1)) {
				struct _isr_table_entry *e = &_sw_isr_table[1];

				e->isr(e->arg);
				return 1 << 1;
			}
			if (mask & (1 << 2)) {
				struct _isr_table_entry *e = &_sw_isr_table[2];

				e->isr(e->arg);
				return 1 << 2;
			}
		} else {
			if (mask & 0x18) {
				if (mask & (1 << 3)) {
					struct _isr_table_entry *e = &_sw_isr_table[3];

					e->isr(e->arg);
					return 1 << 3;
				}
				if (mask & (1 << 4)) {
					struct _isr_table_entry *e = &_sw_isr_table[4];

					e->isr(e->arg);
					return 1 << 4;
				}
			} else {
				if (mask & (1 << 5)) {
					struct _isr_table_entry *e = &_sw_isr_table[5];

					e->isr(e->arg);
					return 1 << 5;
				}
				if (mask & (1 << 6)) {
					struct _isr_table_entry *e = &_sw_isr_table[6];

					e->isr(e->arg);
					return 1 << 6;
				}
			}
		}
	} else {
		if (mask & 0x780) {
			if (mask & 0x180) {
				if (mask & (1 << 7)) {
					struct _isr_table_entry *e = &_sw_isr_table[7];

					e->isr(e->arg);
					return 1 << 7;
				}
				if (mask & (1 << 8)) {
					struct _isr_table_entry *e = &_sw_isr_table[8];

					e->isr(e->arg);
					return 1 << 8;
				}
			} else {
				if (mask & (1 << 9)) {
					struct _isr_table_entry *e = &_sw_isr_table[9];

					e->isr(e->arg);
					return 1 << 9;
				}
				if (mask & (1 << 10)) {
					struct _isr_table_entry *e = &_sw_isr_table[10];

					e->isr(e->arg);
					return 1 << 10;
				}
			}
		} else {
			if (mask & 0x3000) {
				if (mask & (1 << 12)) {
					struct _isr_table_entry *e = &_sw_isr_table[12];

					e->isr(e->arg);
					return 1 << 12;
				}
				if (mask & (1 << 13)) {
					struct _isr_table_entry *e = &_sw_isr_table[13];

					e->isr(e->arg);
					return 1 << 13;
				}
			} else {
				if (mask & (1 << 17)) {
					struct _isr_table_entry *e = &_sw_isr_table[17];

					e->isr(e->arg);
					return 1 << 17;
				}
				if (mask & (1 << 18)) {
					struct _isr_table_entry *e = &_sw_isr_table[18];

					e->isr(e->arg);
					return 1 << 18;
				}
			}
		}
	}
	return 0;
}

static inline int _xtensa_handle_one_int2(unsigned int mask)
{
	if (mask & (1 << 19)) {
		struct _isr_table_entry *e = &_sw_isr_table[19];

		e->isr(e->arg);
		return 1 << 19;
	}
	if (mask & (1 << 20)) {
		struct _isr_table_entry *e = &_sw_isr_table[20];

		e->isr(e->arg);
		return 1 << 20;
	}
	if (mask & (1 << 21)) {
		struct _isr_table_entry *e = &_sw_isr_table[21];

		e->isr(e->arg);
		return 1 << 21;
	}
	return 0;
}

static inline int _xtensa_handle_one_int3(unsigned int mask)
{
	if (mask & 0x408800) {
		if (mask & (1 << 11)) {
			struct _isr_table_entry *e = &_sw_isr_table[11];

			e->isr(e->arg);
			return 1 << 11;
		}
		if (mask & (1 << 15)) {
			struct _isr_table_entry *e = &_sw_isr_table[15];

			e->isr(e->arg);
			return 1 << 15;
		}
		if (mask & (1 << 22)) {
			struct _isr_table_entry *e = &_sw_isr_table[22];

			e->isr(e->arg);
			return 1 << 22;
		}
	} else {
		if (mask & (1 << 23)) {
			struct _isr_table_entry *e = &_sw_isr_table[23];

			e->isr(e->arg);
			return 1 << 23;
		}
		if (mask & (1 << 27)) {
			struct _isr_table_entry *e = &_sw_isr_table[27];

			e->isr(e->arg);
			return 1 << 27;
		}
		if (mask & (1 << 29)) {
			struct _isr_table_entry *e = &_sw_isr_table[29];

			e->isr(e->arg);
			return 1 << 29;
		}
	}
	return 0;
}

static inline int _xtensa_handle_one_int4(unsigned int mask)
{
	if (mask & 0x3000000) {
		if (mask & (1 << 24)) {
			struct _isr_table_entry *e = &_sw_isr_table[24];

			e->isr(e->arg);
			return 1 << 24;
		}
		if (mask & (1 << 25)) {
			struct _isr_table_entry *e = &_sw_isr_table[25];

			e->isr(e->arg);
			return 1 << 25;
		}
	} else {
		if (mask & (1 << 28)) {
			struct _isr_table_entry *e = &_sw_isr_table[28];

			e->isr(e->arg);
			return 1 << 28;
		}
		if (mask & (1 << 30)) {
			struct _isr_table_entry *e = &_sw_isr_table[30];

			e->isr(e->arg);
			return 1 << 30;
		}
	}
	return 0;
}

static inline int _xtensa_handle_one_int5(unsigned int mask)
{
	if (mask & (1 << 16)) {
		struct _isr_table_entry *e = &_sw_isr_table[16];

		e->isr(e->arg);
		return 1 << 16;
	}
	if (mask & (1 << 26)) {
		struct _isr_table_entry *e = &_sw_isr_table[26];

		e->isr(e->arg);
		return 1 << 26;
	}
	if (mask & (1 << 31)) {
		struct _isr_table_entry *e = &_sw_isr_table[31];

		e->isr(e->arg);
		return 1 << 31;
	}
	return 0;
}

static inline int _xtensa_handle_one_int6(unsigned int mask)
{
	return 0;
}

static inline int _xtensa_handle_one_int7(unsigned int mask)
{
	if (mask & (1 << 14)) {
		struct _isr_table_entry *e = &_sw_isr_table[14];

		e->isr(e->arg);
		return 1 << 14;
	}
	return 0;
}


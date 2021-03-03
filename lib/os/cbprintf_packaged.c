/*
 * Copyright (c) 2021 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <sys/cbprintf.h>
#include <sys/types.h>
#include <sys/util.h>
#include <sys/__assert.h>


/**
 * @brief Check if address is in read only section.
 *
 * @param addr Address.
 *
 * @return True if address identified within read only section.
 */
static inline bool ptr_in_rodata(const char *addr)
{
#if defined(CBPRINTF_VIA_UNIT_TEST)
	/* Unit test is X86 (or other host) but not using Zephyr
	 * linker scripts.
	 */
#define RO_START 0
#define RO_END 0
#elif defined(CONFIG_ARC) || defined(CONFIG_ARM) || defined(CONFIG_X86) \
	|| defined(CONFIG_RISCV)
	extern char _image_rodata_start[];
	extern char _image_rodata_end[];
#define RO_START _image_rodata_start
#define RO_END _image_rodata_end
#elif defined(CONFIG_NIOS2) || defined(CONFIG_RISCV)
	extern char _image_rom_start[];
	extern char _image_rom_end[];
#define RO_START _image_rom_start
#define RO_END _image_rom_end
#elif defined(CONFIG_XTENSA)
	extern char _rodata_start[];
	extern char _rodata_end[];
#define RO_START _rodata_start
#define RO_END _rodata_end
#else
#define RO_START 0
#define RO_END 0
#endif

	return ((addr >= (const char *)RO_START) &&
		(addr < (const char *)RO_END));
}

/*
 * va_list creation
 */

#if defined(__aarch64__)
/*
 * Reference:
 *
 * Procedure Call Standard for the ARM 64-bit Architecture
 */

struct __va_list {
	void	*__stack;
	void	*__gr_top;
	void	*__vr_top;
	int	__gr_offs;
	int	__vr_offs;
};

BUILD_ASSERT(sizeof(va_list) == sizeof(struct __va_list),
	     "architecture specific support is wrong");

static int cbprintf_via_va_list(cbprintf_cb out, void *ctx,
				const char *fmt, void *buf)
{
	union {
		va_list ap;
		struct __va_list __ap;
	} u;

	/* create a valid va_list with our buffer */
	u.__ap.__stack = buf;
	u.__ap.__gr_top = NULL;
	u.__ap.__vr_top = NULL;
	u.__ap.__gr_offs = 0;
	u.__ap.__vr_offs = 0;

	return cbvprintf(out, ctx, fmt, u.ap);
}

#define VA_STACK_MIN_ALIGN	8

#elif defined(__x86_64__)
/*
 * Reference:
 *
 * System V Application Binary Interface
 * AMD64 Architecture Processor Supplement
 */

struct __va_list {
	unsigned int gp_offset;
	unsigned int fp_offset;
	void *overflow_arg_area;
	void *reg_save_area;
};

BUILD_ASSERT(sizeof(va_list) == sizeof(struct __va_list),
	     "architecture specific support is wrong");

static int cbprintf_via_va_list(cbprintf_cb out, void *ctx,
				const char *fmt, void *buf)
{
	union {
		va_list ap;
		struct __va_list __ap;
	} u;

	/* create a valid va_list with our buffer */
	u.__ap.overflow_arg_area = buf;
	u.__ap.reg_save_area = NULL;
	u.__ap.gp_offset = (6 * 8);
	u.__ap.fp_offset = (6 * 8 + 16 * 16);

	return cbvprintf(out, ctx, fmt, u.ap);
}

#define VA_STACK_MIN_ALIGN	8

#elif defined(__xtensa__)
/*
 * Reference:
 *
 * gcc source code (gcc/config/xtensa/xtensa.c)
 * xtensa_build_builtin_va_list(), xtensa_va_start(),
 * xtensa_gimplify_va_arg_expr()
 */

struct __va_list {
	void *__va_stk;
	void *__va_reg;
	int __va_ndx;
};

BUILD_ASSERT(sizeof(va_list) == sizeof(struct __va_list),
	     "architecture specific support is wrong");

static int cbprintf_via_va_list(cbprintf_cb out, void *ctx,
				const char *fmt, void *buf)
{
	union {
		va_list ap;
		struct __va_list __ap;
	} u;

	/* create a valid va_list with our buffer */
	u.__ap.__va_stk = (char *)buf - 32;
	u.__ap.__va_reg = NULL;
	u.__ap.__va_ndx = (6 + 2) * 4;

	return cbvprintf(out, ctx, fmt, u.ap);
}

#else
/*
 * Default implementation shared by many architectures like
 * 32-bit ARM and Intel.
 *
 * We assume va_list is a simple pointer.
 */

BUILD_ASSERT(sizeof(va_list) == sizeof(void *),
	     "architecture specific support is needed");

static int cbprintf_via_va_list(cbprintf_cb out, void *ctx,
				const char *fmt, void *buf)
{
	union {
		va_list ap;
		void *ptr;
	} u;

	u.ptr = buf;

	return cbvprintf(out, ctx, fmt, u.ap);
}

#endif

/*
 * Special alignment cases
 */

#if defined(__i386__)
/* there are no gaps on the stack */
#define VA_STACK_ALIGN(type)	1
#endif

#if defined(__riscv)
#define VA_STACK_MIN_ALIGN	(__riscv_xlen / 8)
#endif

#if defined(__sparc__)
/* no gaps on the stack even though the CPU can't do unaligned accesses */
#define VA_STACK_ALIGN(type)	1
#define VA_STACK_LL_DBL_MEMCPY	true
#endif

/*
 * Default alignment values if not specified by architecture config
 */

#ifndef VA_STACK_MIN_ALIGN
#define VA_STACK_MIN_ALIGN	1
#endif

#ifndef VA_STACK_ALIGN
#define VA_STACK_ALIGN(type)	MAX(VA_STACK_MIN_ALIGN, __alignof__(type))
#endif

#ifndef VA_STACK_LL_DBL_MEMCPY
#define VA_STACK_LL_DBL_MEMCPY	false
#endif

int cbvprintf_package(void *packaged, size_t len,
		      const char *fmt, va_list ap)
{
	char *buf = packaged, *buf0 = buf;
	unsigned int align, size, i, s_idx = 0;
	uint8_t str_ptr_pos[16];
	const char *s;
	bool parsing = false;

	/*
	 * Make room to store the arg list size and the number of
	 * appended strings. They both occupy 1 byte each.
	 *
	 * Given the next value to store is the format string pointer
	 * which is guaranteed to be at least 4 bytes, we just reserve
	 * a pointer size for the above to preserve alignment.
	 */
	buf += sizeof(char *);

	/*
	 * When buf0 is NULL we don't store anything.
	 * Instead we count the needed space to store the data.
	 */
	if (!buf0) {
		len = 0;
	}

	/*
	 * Otherwise we must ensure we can store at least
	 * thepointer to the format string itself.
	 */
	if (buf0 && buf - buf0 + sizeof(char *) > len) {
		return -ENOSPC;
	}

	/*
	 * Then process the format string itself.
	 * Here we branch directly into the code processing strings
	 * which is in the middle of the following while() loop. That's the
	 * reason for the post-decrement on fmt as it will be incremented
	 * prior to the next (actually first) round of that loop.
	 */
	s = fmt--;
	align = VA_STACK_ALIGN(char *);
	size = sizeof(char *);
	goto process_string;

	/* Scan the format string */
	while (*++fmt) {
		if (!parsing) {
			if (*fmt == '%') {
				parsing = true;
				align = VA_STACK_ALIGN(int);
				size = sizeof(int);
			}
			continue;
		}
		switch (*fmt) {
		case '%':
			parsing = false;
			continue;

		case '#':
		case '-':
		case '+':
		case ' ':
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case '.':
		case 'h':
		case 'l':
		case 'L':
			continue;

		case '*':
			break;

		case 'j':
			align = VA_STACK_ALIGN(intmax_t);
			size = sizeof(intmax_t);
			continue;

		case 'z':
			align = VA_STACK_ALIGN(size_t);
			size = sizeof(size_t);
			continue;

		case 't':
			align = VA_STACK_ALIGN(ptrdiff_t);
			size = sizeof(ptrdiff_t);
			continue;

		case 'c':
		case 'd':
		case 'i':
		case 'o':
		case 'u':
		case 'x':
		case 'X':
			if (fmt[-1] == 'l') {
				if (fmt[-2] == 'l') {
					align = VA_STACK_ALIGN(long long);
					size = sizeof(long long);
				} else {
					align = VA_STACK_ALIGN(long);
					size = sizeof(long);
				}
			}
			parsing = false;
			break;

		case 's':
		case 'p':
		case 'n':
			align = VA_STACK_ALIGN(void *);
			size = sizeof(void *);
			parsing = false;
			break;

		case 'a':
		case 'A':
		case 'e':
		case 'E':
		case 'f':
		case 'F':
		case 'g':
		case 'G': {
			/*
			 * Handle floats separately as they may be
			 * held in a different register set.
			 */
			union { double d; long double ld; } v;

			if (fmt[-1] == 'L') {
				v.ld = va_arg(ap, long double);
				align = VA_STACK_ALIGN(long double);
				size = sizeof(long double);
			} else {
				v.d = va_arg(ap, double);
				align = VA_STACK_ALIGN(double);
				size = sizeof(double);
			}
			/* align destination buffer location */
			buf = (void *) ROUND_UP(buf, align);
			if (buf0) {
				/* make sure it fits */
				if (buf - buf0 + size > len) {
					return -ENOSPC;
				}
				if (VA_STACK_LL_DBL_MEMCPY) {
					memcpy(buf, &v, size);
				} else if (fmt[-1] == 'L') {
					*(long double *)buf = v.ld;
				} else {
					*(double *)buf = v.d;
				}
			}
			buf += size;
			parsing = false;
			continue;
		}

		default:
			parsing = false;
			continue;
		}

		/* align destination buffer location */
		buf = (void *) ROUND_UP(buf, align);

		/* make sure the data fits */
		if (buf0 && buf - buf0 + size > len) {
			return -ENOSPC;
		}

		/* copy va_list data over to our buffer */
		if (*fmt == 's') {
			s = va_arg(ap, char *);
process_string:
			if (buf0) {
				*(const char **)buf = s;
			}
			if (ptr_in_rodata(s)) {
				/* do nothing special */
			} else if (buf0) {
				/*
				 * Remember string pointer location.
				 * We will append it later.
				 */
				if (s_idx >= ARRAY_SIZE(str_ptr_pos)) {
					__ASSERT(false, "str_ptr_pos[] too small");
					return -EINVAL;
				}
				/* Use same multiple as the arg list size. */
				str_ptr_pos[s_idx++] = (buf - buf0) / sizeof(int);
			} else {
				/*
				 * Add the string length, the final '\0'
				 * and size of the pointer position prefix.
				 */
				len += strlen(s) + 1 + 1;
			}
			buf += sizeof(char *);
		} else if (size == sizeof(int)) {
			int v = va_arg(ap, int);

			if (buf0) {
				*(int *)buf = v;
			}
			buf += sizeof(int);
		} else if (size == sizeof(long)) {
			long v = va_arg(ap, long);

			if (buf0) {
				*(long *)buf = v;
			}
			buf += sizeof(long);
		} else if (size == sizeof(long long)) {
			long long v = va_arg(ap, long long);

			if (buf0) {
				if (VA_STACK_LL_DBL_MEMCPY) {
					memcpy(buf, &v, sizeof(long long));
				} else {
					*(long long *)buf = v;
				}
			}
			buf += sizeof(long long);
		} else {
			__ASSERT(false, "unexpected size %u", size);
			return -EINVAL;
		}
	}

	/*
	 * We remember the size of the argument list as a multiple of
	 * sizeof(int) and limit it to a 8-bit field. That means 1020 bytes
	 * worth of va_list, or about 127 arguments on a 64-bit system
	 * (twice that on 32-bit systems). That ought to be good enough.
	 */
	if ((buf - buf0) / sizeof(int) > 255) {
		__ASSERT(false, "too many format args");
		return -EINVAL;
	}

	/*
	 * If all we wanted was to count required buffer size
	 * then we have it now.
	 */
	if (!buf0) {
		return len + buf - buf0;
	}

	/* Clear our buffer header. We made room for it initially. */
	*(char **)buf0 = 0;

	/* Record end of argument list and number of appended strings. */
	buf0[0] = (buf - buf0) / sizeof(int);
	buf0[1] = s_idx;

	/* Store strings prefixed by their pointer location. */
	for (i = 0; i < s_idx; i++) {
		/* retrieve the string pointer */
		s = *(char **)(buf0 + str_ptr_pos[i] * sizeof(int));
		/* clear the in-buffer pointer (less entropy if compressed) */
		*(char **)(buf0 + str_ptr_pos[i] * sizeof(int)) = NULL;
		/* find the string length including terminating '\0' */
		size = strlen(s) + 1;
		/* make sure it fits */
		if (buf - buf0 + 1 + size > len) {
			return -ENOSPC;
		}
		/* store the pointer position prefix */
		*buf++ = str_ptr_pos[i];
		/* copy the string with its terminating '\0' */
		memcpy(buf, s, size);
		buf += size;
	}

	/*
	 * TODO: remove pointers for appended strings since they're useless.
	 * TODO: explore leveraging same mechanism to remove alignment padding
	 */

	return buf - buf0;
}

int cbprintf_package(void *packaged, size_t len, const char *format, ...)
{
	va_list ap;
	int ret;

	va_start(ap, format);
	ret = cbvprintf_package(packaged, len, format, ap);
	va_end(ap);
	return ret;
}

int cbpprintf(cbprintf_cb out, void *ctx, void *packaged)
{
	char *buf = packaged, *fmt, *s, **ps;
	unsigned int i, args_size, s_nbr, s_idx;

	if (!buf) {
		return -EINVAL;
	}

	/* Retrieve the size of the arg list and number of strings. */
	args_size = ((uint8_t *)buf)[0] * sizeof(int);
	s_nbr     = ((uint8_t *)buf)[1];

	/* Locate the string table */
	s = buf + args_size;

	/*
	 * Patch in string pointers.
	 */
	for (i = 0; i < s_nbr; i++) {
		/* Locate pointer location for this string */
		s_idx = *(uint8_t *)s++;
		ps = (char **)(buf + s_idx * sizeof(int));
		/* update the pointer with current string location */
		*ps = s;
		/* move to next string */
		s += strlen(s) + 1;
	}

	/* Retrieve format string */
	fmt = ((char **)buf)[1];

	/* skip past format string pointer */
	buf += sizeof(char *) * 2;

	/* Turn this into a va_list and  print it */
	return cbprintf_via_va_list(out, ctx, fmt, buf);
}

/*
 * Copyright (c) 2021 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <linker/utils.h>
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
	return false;
#else
	return linker_is_in_rodata(addr);
#endif
}

/*
 * va_list creation
 */

#if defined(__CHECKER__)
static int cbprintf_via_va_list(cbprintf_cb out,
				cbvprintf_exteral_formatter_func formatter,
				void *ctx,
				const char *fmt, void *buf)
{
	return 0;
}
#elif defined(__aarch64__)
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

static int cbprintf_via_va_list(cbprintf_cb out,
				cbvprintf_exteral_formatter_func formatter,
				void *ctx,
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

	return formatter(out, ctx, fmt, u.ap);
}

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

static int cbprintf_via_va_list(cbprintf_cb out,
				cbvprintf_exteral_formatter_func formatter,
				void *ctx,
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

	return formatter(out, ctx, fmt, u.ap);
}

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

static int cbprintf_via_va_list(cbprintf_cb out,
				cbvprintf_exteral_formatter_func formatter,
				void *ctx,
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

	return formatter(out, ctx, fmt, u.ap);
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

static int cbprintf_via_va_list(cbprintf_cb out,
				cbvprintf_exteral_formatter_func formatter,
				void *ctx,
				const char *fmt, void *buf)
{
	union {
		va_list ap;
		void *ptr;
	} u;

	u.ptr = buf;

	return formatter(out, ctx, fmt, u.ap);
}

#endif

static int z_strncpy(char *dst, const char *src, size_t num)
{
	for (size_t i = 0; i < num; i++) {
		dst[i] = src[i];
		if (src[i] == '\0') {
			return i + 1;
		}
	}

	return -ENOSPC;
}

static size_t get_package_len(void *packaged)
{
	__ASSERT_NO_MSG(packaged != NULL);

	uint8_t *buf = packaged;
	uint8_t *start = buf;
	unsigned int args_size, s_nbr, ros_nbr;

	args_size = buf[0] * sizeof(int);
	s_nbr     = buf[1];
	ros_nbr   = buf[2];

	/* Move beyond args. */
	buf += args_size;

	/* Move beyond read-only string indexes array. */
	buf += ros_nbr;

	/* Move beyond strings appended to the package. */
	for (int i = 0; i < s_nbr; i++) {
		buf++;
		buf += strlen((const char *)buf) + 1;
	}

	return (size_t)(uintptr_t)(buf - start);
}

static int append_string(void *dst, size_t max, const char *str, uint16_t strl)
{
	char *buf = dst;

	if (dst == NULL) {
		return 1 + strlen(str);
	}

	if (strl) {
		memcpy(dst, str, strl);

		return strl;
	}

	return z_strncpy(buf, str, max);
}

int cbvprintf_package(void *packaged, size_t len, uint32_t flags,
		      const char *fmt, va_list ap)
{
/*
 * Internally, a byte is used to store location of a string argument within a
 * package. MSB bit is set if string is read-only so effectively 7 bits are
 * used for index, which should be enough.
 */
#define STR_POS_RO_FLAG BIT(7)
#define STR_POS_MASK BIT_MASK(7)

/* Buffer offset abstraction for better code clarity. */
#define BUF_OFFSET ((uintptr_t)buf - (uintptr_t)buf0)

	uint8_t *buf0 = packaged;  /* buffer start (may be NULL) */
	uint8_t *buf = buf0;       /* current buffer position */
	unsigned int size;         /* current argument's size */
	unsigned int align;        /* current argument's required alignment */
	uint8_t str_ptr_pos[16];   /* string pointer positions */
	unsigned int s_idx = 0;    /* index into str_ptr_pos[] */
	unsigned int s_rw_cnt = 0; /* number of rw strings */
	unsigned int s_ro_cnt = 0; /* number of ro strings */
	unsigned int i;
	const char *s;
	bool parsing = false;
	/* Flag indicates that rw strings are stored as array with positions,
	 * instead of appending them to the package.
	 */
	bool rws_pos_en = !!(flags & CBPRINTF_PACKAGE_ADD_RW_STR_POS);
	/* Get number of first read only strings present in the string.
	 * There is always at least 1 (fmt) but flags can indicate more, e.g
	 * fixed prefix appended to all strings.
	 */
	int fros_cnt = 1 + Z_CBPRINTF_PACKAGE_FIRST_RO_STR_CNT_GET(flags);

	/* Buffer must be aligned at least to size of a pointer. */
	if ((uintptr_t)packaged % sizeof(void *)) {
		return -EFAULT;
	}

#if defined(__xtensa__)
	/* Xtensa requires package to be 16 bytes aligned. */
	if ((uintptr_t)packaged % CBPRINTF_PACKAGE_ALIGNMENT) {
		return -EFAULT;
	}
#endif

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
	 * In this case, incoming len argument indicates the anticipated
	 * buffer "misalignment" offset.
	 */
	if (buf0 == NULL) {
		buf += len % CBPRINTF_PACKAGE_ALIGNMENT;
		/*
		 * The space to store the data is represented by both the
		 * buffer offset as well as the extra string data to be
		 * appended. When only figuring out the needed space, we
		 * don't append anything. Instead, we reuse the len variable
		 * to sum the size of that data.
		 *
		 * Also, we subtract any initial misalignment offset from
		 * the total as this won't be part of the buffer. To avoid
		 * going negative with an unsigned variable, we add an offset
		 * (CBPRINTF_PACKAGE_ALIGNMENT) that will be removed before
		 * returning.
		 */
		len = CBPRINTF_PACKAGE_ALIGNMENT - (len % CBPRINTF_PACKAGE_ALIGNMENT);
	}

	/*
	 * Otherwise we must ensure we can store at least
	 * the pointer to the format string itself.
	 */
	if (buf0 != NULL && BUF_OFFSET + sizeof(char *) > len) {
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
	while (*++fmt != '\0') {
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
			if (buf0 != NULL) {
				/* make sure it fits */
				if (BUF_OFFSET + size > len) {
					return -ENOSPC;
				}
				if (Z_CBPRINTF_VA_STACK_LL_DBL_MEMCPY) {
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
		if (buf0 != NULL && BUF_OFFSET + size > len) {
			return -ENOSPC;
		}

		/* copy va_list data over to our buffer */
		if (*fmt == 's') {
			s = va_arg(ap, char *);
process_string:
			if (buf0 != NULL) {
				*(const char **)buf = s;
			}

			bool is_ro = (fros_cnt-- > 0) ? true : ptr_in_rodata(s);
			bool do_ro = !!(flags & CBPRINTF_PACKAGE_ADD_RO_STR_POS);

			if (is_ro && !do_ro) {
				/* nothing to do */
			} else {
				uint32_t s_ptr_idx = BUF_OFFSET / sizeof(int);

				/*
				 * In the do_ro case we must consider
				 * room for possible STR_POS_RO_FLAG.
				 * Otherwise the index range is 8 bits
				 * and any overflow is caught later.
				 */
				if (do_ro && s_ptr_idx > STR_POS_MASK) {
					__ASSERT(false, "String with too many arguments");
					return -EINVAL;
				}

				if (s_idx >= ARRAY_SIZE(str_ptr_pos)) {
					__ASSERT(false, "str_ptr_pos[] too small");
					return -EINVAL;
				}

				if (buf0 != NULL) {
					/*
					 * Remember string pointer location.
					 * We will append non-ro strings later.
					 */
					str_ptr_pos[s_idx] = s_ptr_idx;
					if (is_ro) {
						/* flag read-only string. */
						str_ptr_pos[s_idx] |= STR_POS_RO_FLAG;
						s_ro_cnt++;
					} else {
						s_rw_cnt++;
					}
				} else if (is_ro || rws_pos_en) {
					/*
					 * Add only pointer position prefix
					 * when counting strings.
					 */
					len += 1;
				} else {
					/*
					 * Add the string length, the final '\0'
					 * and size of the pointer position prefix.
					 */
					len += strlen(s) + 1 + 1;
				}

				s_idx++;
			}
			buf += sizeof(char *);
		} else if (size == sizeof(int)) {
			int v = va_arg(ap, int);

			if (buf0 != NULL) {
				*(int *)buf = v;
			}
			buf += sizeof(int);
		} else if (size == sizeof(long)) {
			long v = va_arg(ap, long);

			if (buf0 != NULL) {
				*(long *)buf = v;
			}
			buf += sizeof(long);
		} else if (size == sizeof(long long)) {
			long long v = va_arg(ap, long long);

			if (buf0 != NULL) {
				if (Z_CBPRINTF_VA_STACK_LL_DBL_MEMCPY) {
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
	if (BUF_OFFSET / sizeof(int) > 255) {
		__ASSERT(false, "too many format args");
		return -EINVAL;
	}

	/*
	 * If all we wanted was to count required buffer size
	 * then we have it now.
	 */
	if (buf0 == NULL) {
		return BUF_OFFSET + len - CBPRINTF_PACKAGE_ALIGNMENT;
	}

	/* Clear our buffer header. We made room for it initially. */
	*(char **)buf0 = NULL;

	/* Record end of argument list. */
	buf0[0] = BUF_OFFSET / sizeof(int);

	if (rws_pos_en) {
		/* Strings are appended, update location counter. */
		buf0[1] = 0;
		buf0[3] = s_rw_cnt;
	} else {
		/* Strings are appended, update append counter. */
		buf0[1] = s_rw_cnt;
		buf0[3] = 0;
	}

	buf0[2] = s_ro_cnt;

	/* Store strings pointer locations of read only strings. */
	if (s_ro_cnt) {
		for (i = 0; i < s_idx; i++) {
			if (!(str_ptr_pos[i] & STR_POS_RO_FLAG)) {
				continue;
			}

			uint8_t pos = str_ptr_pos[i] & STR_POS_MASK;

			/* make sure it fits */
			if (BUF_OFFSET + 1 > len) {
				return -ENOSPC;
			}
			/* store the pointer position prefix */
			*buf++ = pos;
		}
	}

	/* Store strings prefixed by their pointer location. */
	for (i = 0; i < s_idx; i++) {
		/* Process only RW strings. */
		if (s_ro_cnt && str_ptr_pos[i] & STR_POS_RO_FLAG) {
			continue;
		}

		if (rws_pos_en) {
			size = 0;
		} else {
			/* retrieve the string pointer */
			s = *(char **)(buf0 + str_ptr_pos[i] * sizeof(int));
			/* clear the in-buffer pointer (less entropy if compressed) */
			*(char **)(buf0 + str_ptr_pos[i] * sizeof(int)) = NULL;
			/* find the string length including terminating '\0' */
			size = strlen(s) + 1;
		}

		/* make sure it fits */
		if (BUF_OFFSET + 1 + size > len) {
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

	return BUF_OFFSET;

#undef BUF_OFFSET
#undef STR_POS_RO_FLAG
#undef STR_POS_MASK
}

int cbprintf_package(void *packaged, size_t len, uint32_t flags,
		     const char *format, ...)
{
	va_list ap;
	int ret;

	va_start(ap, format);
	ret = cbvprintf_package(packaged, len, flags, format, ap);
	va_end(ap);
	return ret;
}

int cbpprintf_external(cbprintf_cb out,
		       cbvprintf_exteral_formatter_func formatter,
		       void *ctx, void *packaged)
{
	uint8_t *buf = packaged;
	char *fmt, *s, **ps;
	unsigned int i, args_size, s_nbr, ros_nbr, rws_nbr, s_idx;

	if (buf == NULL) {
		return -EINVAL;
	}

	/* Retrieve the size of the arg list and number of strings. */
	args_size = buf[0] * sizeof(int);
	s_nbr     = buf[1];
	ros_nbr   = buf[2];
	rws_nbr   = buf[3];

	/* Locate the string table */
	s = (char *)(buf + args_size + ros_nbr + rws_nbr);

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
	return cbprintf_via_va_list(out, formatter, ctx, fmt, buf);
}

int cbprintf_package_copy(void *in_packaged,
			  size_t in_len,
			  void *packaged,
			  size_t len,
			  uint32_t flags,
			  uint16_t *strl,
			  size_t strl_len)
{
	__ASSERT_NO_MSG(in_packaged != NULL);

	uint8_t *buf = in_packaged;
	uint32_t *buf32 = in_packaged;
	unsigned int args_size, ros_nbr, rws_nbr;
	bool rw_cpy;
	bool ro_cpy;

	in_len != 0 ? in_len : get_package_len(in_packaged);

	/* Get number of RO string indexes in the package and check if copying
	 * includes appending those strings.
	 */
	ros_nbr = buf[2];
	ro_cpy = ros_nbr &&
		(flags & CBPRINTF_PACKAGE_COPY_RO_STR) == CBPRINTF_PACKAGE_COPY_RO_STR;

	/* Get number of RW string indexes in the package and check if copying
	 * includes appending those strings.
	 */
	rws_nbr = buf[3];
	rw_cpy = rws_nbr > 0 &&
		 (flags & CBPRINTF_PACKAGE_COPY_RW_STR) == CBPRINTF_PACKAGE_COPY_RW_STR;


	/* If flags are not set or appending request without rw string indexes
	 * present is chosen, just do a simple copy (or length calculation).
	 * Assuming that it is the most common case.
	 */
	if (!rw_cpy && !ro_cpy) {
		if (packaged) {
			memcpy(packaged, in_packaged, in_len);
		}

		return in_len;
	}

	/* If we got here, it means that coping will be more complex and will be
	 * done with strings appending.
	 * Retrieve the size of the arg list.
	 */
	args_size = buf[0] * sizeof(int);

	size_t out_len = in_len;

	/* Pointer to array with string locations. Array starts with read-only
	 * string locations.
	 */
	uint8_t *str_pos = &buf[args_size];
	size_t strl_cnt = 0;

	/* If null destination, just calculate output length. */
	if (packaged == NULL) {
		if (ro_cpy) {
			for (int i = 0; i < ros_nbr; i++) {
				const char *str = *(const char **)&buf32[*str_pos];
				int len = append_string(NULL, 0, str, 0);

				/* If possible store calculated string length. */
				if (strl && strl_cnt < strl_len) {
					strl[strl_cnt++] = (uint16_t)len;
				}
				out_len += len;
				str_pos++;
			}
		} else {
			if (ros_nbr && flags & CBPRINTF_PACKAGE_COPY_KEEP_RO_STR) {
				str_pos += ros_nbr;
			}
		}

		bool drop_ro_str_pos = !(flags &
					(CBPRINTF_PACKAGE_COPY_KEEP_RO_STR |
					 CBPRINTF_PACKAGE_COPY_RO_STR));

		/* Handle RW strings. */
		for (int i = 0; i < rws_nbr; i++) {
			const char *str = *(const char **)&buf32[*str_pos];
			bool is_ro = ptr_in_rodata(str);

			if ((is_ro && flags & CBPRINTF_PACKAGE_COPY_RO_STR) ||
			    (!is_ro && flags & CBPRINTF_PACKAGE_COPY_RW_STR)) {
				int len = append_string(NULL, 0, str, 0);

				/* If possible store calculated string length. */
				if (strl && strl_cnt < strl_len) {
					strl[strl_cnt++] = (uint16_t)len;
				}
				out_len += len;
			}

			if (is_ro && drop_ro_str_pos) {
				/* If read-only string location is dropped decreased
				 * length.
				 */
				out_len--;
			}

			str_pos++;
		}

		return out_len;
	}

	uint8_t cpy_str_pos[16];
	uint8_t scpy_cnt;
	uint8_t *dst = packaged;
	uint8_t *dst_hdr = packaged;

	memcpy(dst, in_packaged, args_size);
	dst += args_size;

	/* Pointer to the beginning of string locations in the destination package. */
	uint8_t *dst_str_loc = dst;

	/* If read-only strings shall be appended to the output package copy
	 * their indexes to the local array, otherwise indicate that indexes
	 * shall remain in the output package.
	 */
	if (ro_cpy) {
		memcpy(cpy_str_pos, str_pos, ros_nbr);
		scpy_cnt = ros_nbr;
		/* Read only string indexes removed from package. */
		dst_hdr[2] = 0;
		str_pos += ros_nbr;
	} else {
		scpy_cnt = 0;
		if (ros_nbr && flags & CBPRINTF_PACKAGE_COPY_KEEP_RO_STR) {
			memcpy(dst, str_pos, ros_nbr);
			dst += ros_nbr;
			str_pos += ros_nbr;
		} else {
			dst_hdr[2] = 0;
		}
	}

	/* Go through read-write strings and identify which shall be appended.
	 * Note that there may be read-only strings there. Use address evaluation
	 * to determine if strings is read-only.
	 */
	for (int i = 0; i < rws_nbr; i++) {
		const char *str = *(const char **)&buf32[*str_pos];
		bool is_ro = ptr_in_rodata(str);

		if (is_ro) {
			if (flags & CBPRINTF_PACKAGE_COPY_RO_STR) {
				cpy_str_pos[scpy_cnt++] = *str_pos;
			} else if (flags & CBPRINTF_PACKAGE_COPY_KEEP_RO_STR) {
				*dst++ = *str_pos;
				/* Increment amount of ro locations. */
				dst_hdr[2]++;
			} else {
				/* Drop information about ro_str location. */
			}
		} else {
			if (flags & CBPRINTF_PACKAGE_COPY_RW_STR) {
				cpy_str_pos[scpy_cnt++] = *str_pos;
			} else {
				*dst++ = *str_pos;
			}
		}
		str_pos++;
	}

	/* Increment amount of strings appended to the package. */
	dst_hdr[1] += scpy_cnt;
	/* Update number of rw string locations in the package. */
	dst_hdr[3] = (uint8_t)(uintptr_t)(dst - dst_str_loc) - dst_hdr[2];

	/* Copy appended strings from source package to destination. */
	size_t strs_len = in_len - (args_size + ros_nbr + rws_nbr);

	memcpy(dst, str_pos, strs_len);

	dst += strs_len;

	if (scpy_cnt == 0) {
		return dst - dst_hdr;
	}

	/* Calculate remaining space in the buffer. */
	size_t rem = len - ((size_t)(uintptr_t)(dst - dst_hdr));

	if (rem <= scpy_cnt) {
		return -ENOSPC;
	}

	/* Append strings */
	for (int i = 0; i < scpy_cnt; i++) {
		uint8_t loc = cpy_str_pos[i];
		const char *str = *(const char **)&buf32[loc];
		int cpy_len;
		uint16_t str_len = strl ? strl[i] : 0;

		*dst = loc;
		rem--;
		dst++;
		cpy_len = append_string(dst, rem, str, str_len);

		if (cpy_len < 0) {
			return -ENOSPC;
		}

		rem -= cpy_len;
		dst += cpy_len;
	}

	return len - rem;
}

/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <offsets_short.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/syscall.h>
#include <zephyr/kernel_structs.h>

/* exports */
GTEXT(arch_user_string_nlen)
GTEXT(z_tricore_user_string_nlen_fault_start)
GTEXT(z_tricore_user_string_nlen_fault_end)
GTEXT(z_tricore_user_string_nlen_fixup)

/*
 * size_t arch_user_string_nlen(const char *s, size_t maxsize, int *err_arg)
 */
SECTION_FUNC(TEXT, arch_user_string_nlen)
	mov	%d2, 0
	st.w	[%a5], %d2    # Clear error argument

loop:
	addsc.a	%a15, %a4, %d2, 0
z_tricore_user_string_nlen_fault_start:
	ld.bu	%d14, [%a15]    	# Load byte from user string
z_tricore_user_string_nlen_fault_end:
	jz	%d14, exit		# Test string's end of line
	jnei	%d2, %d4, loop		# Check if max length is reached
	ret

z_tricore_user_string_nlen_fixup:
	mov	%d14, -1		# Put error to -1
	st.w	[%a5], %d14
	ret

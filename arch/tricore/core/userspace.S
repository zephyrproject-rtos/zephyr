/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
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

/* size_t arch_user_string_nlen(const char *s, size_t maxsize, int *err_arg) */
SECTION_FUNC(TEXT, arch_user_string_nlen)
	mov	%d2, 0
	st.w	[%a5], %d2    # Clear error argument
	jz	%d4, _arch_user_string_nlen_done	# Nothing to read for maxsize 0

loop:
	addsc.a	%a15, %a4, %d2, 0
z_tricore_user_string_nlen_fault_start:
	ld.bu	%d14, [%a15]    	# Load byte from user string
z_tricore_user_string_nlen_fault_end:
	jz	%d14, _arch_user_string_nlen_done	# Test string's end of line
	add	%d2, 1
	jne	%d2, %d4, loop		# Check if max length is reached
_arch_user_string_nlen_done:
	ret

z_tricore_user_string_nlen_fixup:
	mov	%d14, -1		# Put error to -1
	st.w	[%a5], %d14
	ret

/*
 * Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#ifdef TEST_RUNTIME_BUILTINS_RELOCATION

extern float __aeabi_fadd(float lhs, float rhs);
extern float __aeabi_fdiv(float lhs, float rhs);
extern float __aeabi_fmul(float lhs, float rhs);

__attribute__((noinline))
static float relocated_float_operation(float lhs, float rhs)
{
	return (lhs * rhs) / (lhs + rhs);
}

ZTEST(code_relocation, test_runtime_builtins_relocation)
{
	extern uintptr_t __sram2_text_reloc_start;
	extern uintptr_t __sram2_text_reloc_end;
	volatile float lhs = 6.0f;
	volatile float rhs = 4.0f;
	volatile float result;

	result = relocated_float_operation(lhs, rhs);
	zassert_equal(result, 2.4f);

	zassert_between_inclusive((uintptr_t)&relocated_float_operation,
		(uintptr_t)&__sram2_text_reloc_start,
		(uintptr_t)&__sram2_text_reloc_end,
		"floating-point caller was not relocated");

	/* Verify that dependencies from the runtime archive were also relocated. */
	zassert_between_inclusive((uintptr_t)&__aeabi_fadd,
		(uintptr_t)&__sram2_text_reloc_start,
		(uintptr_t)&__sram2_text_reloc_end,
		"__aeabi_fadd from the runtime archive was not relocated");
	zassert_between_inclusive((uintptr_t)&__aeabi_fmul,
		(uintptr_t)&__sram2_text_reloc_start,
		(uintptr_t)&__sram2_text_reloc_end,
		"__aeabi_fmul from the runtime archive was not relocated");
	zassert_between_inclusive((uintptr_t)&__aeabi_fdiv,
		(uintptr_t)&__sram2_text_reloc_start,
		(uintptr_t)&__sram2_text_reloc_end,
		"__aeabi_fdiv from the runtime archive was not relocated");
}

#endif /* TEST_RUNTIME_BUILTINS_RELOCATION */

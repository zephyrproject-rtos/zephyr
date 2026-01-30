/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_TRICORE_INCLUDE_KERNEL_ARCH_DATA_H_
#define ZEPHYR_ARCH_TRICORE_INCLUDE_KERNEL_ARCH_DATA_H_

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>

#ifndef _ASMLANGUAGE
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/sys/dlist.h>
#include <zephyr/sys/atomic.h>

#ifdef __cplusplus
extern "C" {
#endif

struct z_tricore_lower_context {
	uint32_t pcxi;
	uint32_t a11;
	uint32_t a2;
	uint32_t a3;
	uint32_t d0;
	uint32_t d1;
	uint32_t d2;
	uint32_t d3;
	uint32_t a4;
	uint32_t a5;
	uint32_t a6;
	uint32_t a7;
	uint32_t d4;
	uint32_t d5;
	uint32_t d6;
	uint32_t d7;
};
typedef struct z_tricore_lower_context z_tricore_lower_context_t;

struct z_tricore_upper_context {
	uint32_t pcxi;
	uint32_t psw;
	uint32_t a10;
	uint32_t a11;
	uint32_t d8;
	uint32_t d9;
	uint32_t d10;
	uint32_t d11;
	uint32_t a12;
	uint32_t a13;
	uint32_t a14;
	uint32_t a15;
	uint32_t d12;
	uint32_t d13;
	uint32_t d14;
	uint32_t d15;
};
typedef struct z_tricore_upper_context z_tricore_upper_context_t;

union z_tricore_context {
	struct z_tricore_lower_context lower;
	struct z_tricore_upper_context upper;
};

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_TRICORE_INCLUDE_KERNEL_ARCH_DATA_H_ */

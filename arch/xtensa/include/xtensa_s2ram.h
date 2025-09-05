/*
 * Copyright (c) 2025 Meta Platforms, Inc. and its affiliates.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_XTENSA_INCLUDE_XTENSA_S2SRAM_H_
#define ZEPHYR_ARCH_XTENSA_INCLUDE_XTENSA_S2SRAM_H_

#include <xtensa/hal-certified.h>
#include <xtensa/hal-core-state.h>
#include <xtensa/config/tie.h>

#ifdef XCHAL_TOTAL_SA_ALIGN
#define XTENSA_S2RAM_ALIGN (XCHAL_TOTAL_SA_ALIGN)
#else
#define XTENSA_S2RAM_ALIGN (16)
#endif

#define __S2RAM_ALIGN __aligned(XTENSA_S2RAM_ALIGN)
#define S2RAM_MAGIC   0x53325241

#define XTENSA_S2RAM_MAGIC_OFFSET      (0)
#define XTENSA_S2RAM_SYSTEM_OFF_OFFSET (4)
#define XTENSA_S2RAM_CORE_STATE_OFFSET (XTENSA_S2RAM_ALIGN)

#ifndef __ASSEMBLER__

#include <stdint.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

struct xtensa_s2ram_save_area {
	__S2RAM_ALIGN
	uint32_t magic;
	uintptr_t system_off;
	/* for future use*/
	uint32_t reserved[2];

	__S2RAM_ALIGN
	XthalCoreState core_state;
};

_Static_assert((offsetof(struct xtensa_s2ram_save_area, magic) == XTENSA_S2RAM_MAGIC_OFFSET),
	       "magic must be at XTENSA_S2RAM_MAGIC_OFFSET");
_Static_assert((offsetof(struct xtensa_s2ram_save_area, system_off) == 4),
	       "system_off must be at XTENSA_S2RAM_SYSTEM_OFF_OFFSET");
_Static_assert((offsetof(struct xtensa_s2ram_save_area, core_state) == XTENSA_S2RAM_ALIGN),
	       "core_state must be at XTENSA_S2RAM_CORE_STATE_OFFSET");

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __ASSEMBLER__ */

#endif /* ZEPHYR_ARCH_XTENSA_INCLUDE_XTENSA_S2SRAM_H_ */

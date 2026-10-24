/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Coprocessor whose registers are preserved across context switches
 *
 * Maps @kconfig{CONFIG_XTENSA_CP_SHARING_ID} to the save area and the
 * save and restore macros that the core configuration defines for that
 * coprocessor.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_XTENSA_CP_SHARING_H_
#define ZEPHYR_INCLUDE_ARCH_XTENSA_CP_SHARING_H_

#ifdef CONFIG_XTENSA_CP_SHARING

#include <zephyr/toolchain.h>
#include <xtensa/config/tie.h>

/** Coprocessor number, which is also its bit in CPENABLE */
#define XTENSA_CP_ID CONFIG_XTENSA_CP_SHARING_ID

/** Size of the register save area of the coprocessor */
#define XTENSA_CP_SA_SIZE _CONCAT(_CONCAT(XCHAL_CP, XTENSA_CP_ID), _SA_SIZE)

/** Alignment of the register save area of the coprocessor */
#define XTENSA_CP_SA_ALIGN _CONCAT(_CONCAT(XCHAL_CP, XTENSA_CP_ID), _SA_ALIGN)

#ifdef _ASMLANGUAGE
/* Save area helper macros used by the ones of tie-asm.h */
#include <xtensa/coreasm.h>
#include <xtensa/config/tie-asm.h>

/** Number of temporary registers used by XTENSA_CP_STORE and XTENSA_CP_LOAD */
#define XTENSA_CP_NUM_ATMPS _CONCAT(_CONCAT(XCHAL_CP, XTENSA_CP_ID), _NUM_ATMPS)

/** Assembler macro saving the coprocessor registers to a save area */
#define XTENSA_CP_STORE _CONCAT(_CONCAT(xchal_cp, XTENSA_CP_ID), _store)

/** Assembler macro loading the coprocessor registers from a save area */
#define XTENSA_CP_LOAD _CONCAT(_CONCAT(xchal_cp, XTENSA_CP_ID), _load)
#endif /* _ASMLANGUAGE */

#endif /* CONFIG_XTENSA_CP_SHARING */

#endif /* ZEPHYR_INCLUDE_ARCH_XTENSA_CP_SHARING_H_ */

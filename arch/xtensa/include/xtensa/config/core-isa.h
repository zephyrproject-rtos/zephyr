/*
 * Copyright (c) 2026 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_XTENSA_INCLUDE_XTENSA_CONFIG_CORE_ISA_H_
#define ZEPHYR_ARCH_XTENSA_INCLUDE_XTENSA_CONFIG_CORE_ISA_H_

#if defined(__XCC__) && defined(__XCC_CLANG__)
/*
 * Under native LLVM Clang, both __XCC__ and __XCC_CLANG__ are defined to enable
 * HiFi intrinsics. However, Cadence-generated SoC core-isa.h headers for NXP
 * and MediaTek include a guard "#error xcc should not use this header" if
 * __XCC__ is defined. Undefine it temporarily during traversal and restore it
 * afterwards to bypass the guard.
 */
#undef __XCC__
#include_next <xtensa/config/core-isa.h>
#define __XCC__ 1
#else
#include_next <xtensa/config/core-isa.h>
#endif

#endif /* ZEPHYR_ARCH_XTENSA_INCLUDE_XTENSA_CONFIG_CORE_ISA_H_ */

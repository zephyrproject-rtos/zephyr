/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_XTENSA_INCLUDE_XTENSA_EXC_H_
#define ZEPHYR_ARCH_XTENSA_INCLUDE_XTENSA_EXC_H_

/**
 * @ingroup xtensa_internal_apis
 * @{
 */

/** Custom EXCCAUSE code used to indicate Zephyr exception (e.g. assert) */
#define XTENSA_EXCCAUSE_CUSTOM_ZEPHYR_EXCEPTION (63)

/** Custom EXCCAUSE code used to indicate Kernel OOPS */
#define XTENSA_EXCCAUSE_CUSTOM_KERNEL_OOPS (64)

/**
 * @}
 */

#endif /* ZEPHYR_ARCH_XTENSA_INCLUDE_XTENSA_EXC_H_ */

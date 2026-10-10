/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_COMMON_XIP_H_
#define ZEPHYR_INCLUDE_ARCH_COMMON_XIP_H_

#ifndef _ASMLANGUAGE
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Copy sections whose load address differs from their virtual address
 *
 * With CONFIG_XIP this copies the data section and the other regions residing
 * in ROM. TCM regions are copied unconditionally, as they are separate
 * memories whose load address differs from their virtual address regardless
 * of CONFIG_XIP.
 */
void arch_data_copy(void);
#ifdef __cplusplus
}
#endif

#endif	/* _ASMLANGUAGE */
#endif /* ZEPHYR_INCLUDE_ARCH_COMMON_XIP_H_ */

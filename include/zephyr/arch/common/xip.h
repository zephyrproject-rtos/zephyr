/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_INCLUDE_XIP_H_
#define ZEPHYR_ARCH_INCLUDE_XIP_H_

#ifndef _ASMLANGUAGE
#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_XIP
void arch_data_copy(void);
#else
static inline void arch_data_copy(void)
{
	/* Do nothing */
}
#endif /* CONFIG_XIP */
#ifdef __cplusplus
}
#endif

#endif	/* _ASMLANGUAGE */
#endif /* ZEPHYR_ARCH_INCLUDE_XIP_H_ */

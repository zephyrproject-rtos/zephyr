/*
 * Copyright (c) 2021 KT-Elektronik, Klaucke und Partner GmbH
 * Copyright (c) 2024 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_RX_INCLUDE_KERNEL_ARCH_FUNC_H_
#define ZEPHYR_ARCH_RX_INCLUDE_KERNEL_ARCH_FUNC_H_

#ifndef _ASMLANGUAGE
#include <kernel_arch_data.h>

#ifdef __cplusplus
extern "C" {
#endif

static ALWAYS_INLINE void arch_kernel_init(void)
{
	/* check if: further device initialization functions must be called here */
}

static inline bool arch_is_in_isr(void)
{
	return arch_curr_cpu()->nested != 0U;
}

extern void z_rx_arch_switch(void *switch_to, void **switched_from);

static inline void arch_switch(void *switch_to, void **switched_from)
{
	z_rx_arch_switch(switch_to, switched_from);
}

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_RX_INCLUDE_KERNEL_ARCH_FUNC_H_ */

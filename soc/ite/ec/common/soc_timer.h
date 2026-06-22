/*
 * Copyright (c) 2025 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ITE_IT8XXX2_SOC_TIMER_H_
#define _ITE_IT8XXX2_SOC_TIMER_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_SOC_IT8XXX2
bool ite_ec_timer_block_idle(void);
#else
static inline bool ite_ec_timer_block_idle(void)
{
	return false;
}
#endif /* CONFIG_SOC_IT8XXX2 */

#ifdef __cplusplus
}
#endif

#endif /* _ITE_IT8XXX2_SOC_TIMER_H_ */

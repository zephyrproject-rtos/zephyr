/*
 * Copyright (c) 2020, Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_CORTEX_M_NVIC_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_CORTEX_M_NVIC_H_

#if defined(CONFIG_ARMV8_M_BASELINE) || defined(CONFIG_ARMV8_M_MAINLINE)
/* The order here is on purpose since ARMv8-M SoCs may define
 * CONFIG_ARMV6_M_ARMV8_M_BASELINE or CONFIG_ARMV7_M_ARMV8_M_MAINLINE
 * so we want to check for ARMv8-M first.
 */
#define NVIC_NODEID DT_INST(0, arm_v8m_nvic)
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
#define NVIC_NODEID DT_INST(0, arm_v7m_nvic)
#elif defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
#define NVIC_NODEID DT_INST(0, arm_v6m_nvic)
#endif

#define NUM_IRQ_PRIO_BITS DT_PROP(NVIC_NODEID, arm_num_irq_priority_bits)

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_CORTEX_M_NVIC_H_ */

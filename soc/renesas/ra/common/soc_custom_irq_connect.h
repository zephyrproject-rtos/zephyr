/*
 * Copyright (c) 2024 TOKITA Hiroshi <tokita.hiroshi@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_RENESAS_RA_RA4M1_SOC_CUSTOM_IRQ_CONNECT_H_
#define ZEPHYR_SOC_RENESAS_RA_RA4M1_SOC_CUSTOM_IRQ_CONNECT_H_

#define RA_INTR_ID_COUNT DT_PROP(DT_CHOSEN(zephyr_interrupt_controller), interrupt_id_count)

#if RA_INTR_ID_COUNT < UINT8_MAX
typedef uint8_t ra_intr_id_t;
#define RA_INVALID_INTR_ID UINT8_MAX
#else
typedef uint16_t ra_intr_id_t;
#define RA_INVALID_INTR_ID UINT16_MAX
#endif

#define RA_INTR_ID_TO_IRQ(id) (__intr_id_table[id])
extern const ra_intr_id_t __intr_id_table[RA_INTR_ID_COUNT];

#define ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p)                           \
	{                                                                                          \
		BUILD_ASSERT(IS_ENABLED(CONFIG_ZERO_LATENCY_IRQS) ||                               \
				     !(flags_p & IRQ_ZERO_LATENCY),                                \
			     "ZLI interrupt registered but feature is disabled");                  \
		_CHECK_PRIO(priority_p, flags_p)                                                   \
		Z_ISR_DECLARE(irq_p, 0, isr_p, isr_param_p);                                       \
		z_arm_irq_priority_set(RA_INTR_ID_TO_IRQ(irq_p), priority_p, flags_p);             \
	}

#define ARCH_IRQ_DIRECT_CONNECT(irq_p, priority_p, isr_p, flags_p)                                 \
	{                                                                                          \
		BUILD_ASSERT(IS_ENABLED(CONFIG_ZERO_LATENCY_IRQS) ||                               \
				     !(flags_p & IRQ_ZERO_LATENCY),                                \
			     "ZLI interrupt registered but feature is disabled");                  \
		_CHECK_PRIO(priority_p, flags_p)                                                   \
		Z_ISR_DECLARE_DIRECT(irq_p, ISR_FLAG_DIRECT, isr_p);                               \
		z_arm_irq_priority_set(RA_INTR_ID_TO_IRQ(irq_p), priority_p, flags_p);             \
	}

#endif /* ZEPHYR_SOC_RENESAS_RA_RA4M1_SOC_CUSTOM_IRQ_CONNECT_H_ */

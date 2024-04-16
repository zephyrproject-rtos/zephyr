/* Copyright (C) 2023 BeagleBoard.org Foundation
 * Copyright (C) 2023 S Prashanth
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_INTC_VIM_H_
#define ZEPHYR_DRIVERS_INTC_VIM_H_

#include <stdint.h>

#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/interrupt-controller/ti-vim.h>
#include <zephyr/sys/util_macro.h>

#define VIM_BASE_ADDR             DT_REG_ADDR(DT_INST(0, ti_vim))

#define VIM_MAX_IRQ_PER_GROUP     (32)
#define VIM_MAX_GROUP_NUM         ((uint32_t)(CONFIG_NUM_IRQS / VIM_MAX_IRQ_PER_GROUP))

#define VIM_GET_IRQ_GROUP_NUM(n)  ((uint32_t)((n) / VIM_MAX_IRQ_PER_GROUP))
#define VIM_GET_IRQ_BIT_NUM(n)    ((uint32_t)((n) % VIM_MAX_IRQ_PER_GROUP))

#define VIM_PRI_INT_MAX           (15)

#define VIM_PID             (VIM_BASE_ADDR + 0x0000)
#define VIM_INFO            (VIM_BASE_ADDR + 0x0004)
#define VIM_PRIIRQ          (VIM_BASE_ADDR + 0x0008)
#define VIM_PRIFIQ          (VIM_BASE_ADDR + 0x000C)
#define VIM_IRQGSTS         (VIM_BASE_ADDR + 0x0010)
#define VIM_FIQGSTS         (VIM_BASE_ADDR + 0x0014)
#define VIM_IRQVEC          (VIM_BASE_ADDR + 0x0018)
#define VIM_FIQVEC          (VIM_BASE_ADDR + 0x001C)
#define VIM_ACTIRQ          (VIM_BASE_ADDR + 0x0020)
#define VIM_ACTFIQ          (VIM_BASE_ADDR + 0x0024)
#define VIM_DEDVEC          (VIM_BASE_ADDR + 0x0030)

#define VIM_RAW(n)          (VIM_BASE_ADDR + (0x400) + ((n) * 0x20))
#define VIM_STS(n)          (VIM_BASE_ADDR + (0x404) + ((n) * 0x20))
#define VIM_INTR_EN_SET(n)  (VIM_BASE_ADDR + (0x408) + ((n) * 0x20))
#define VIM_INTR_EN_CLR(n)  (VIM_BASE_ADDR + (0x40c) + ((n) * 0x20))
#define VIM_IRQSTS(n)       (VIM_BASE_ADDR + (0x410) + ((n) * 0x20))
#define VIM_FIQSTS(n)       (VIM_BASE_ADDR + (0x414) + ((n) * 0x20))
#define VIM_INTMAP(n)       (VIM_BASE_ADDR + (0x418) + ((n) * 0x20))
#define VIM_INTTYPE(n)      (VIM_BASE_ADDR + (0x41c) + ((n) * 0x20))
#define VIM_PRI_INT(n)      (VIM_BASE_ADDR + (0x1000) + ((n) * 0x4))
#define VIM_VEC_INT(n)      (VIM_BASE_ADDR + (0x2000) + ((n) * 0x4))

/* RAW */

#define VIM_GRP_RAW_STS_MASK                 (BIT_MASK(32))
#define VIM_GRP_RAW_STS_SHIFT                (0x00000000U)
#define VIM_GRP_RAW_STS_RESETVAL             (0x00000000U)
#define VIM_GRP_RAW_STS_MAX                  (BIT_MASK(32))

#define VIM_GRP_RAW_RESETVAL                 (0x00000000U)

/* STS */

#define VIM_GRP_STS_MSK_MASK                 (BIT_MASK(32))
#define VIM_GRP_STS_MSK_SHIFT                (0x00000000U)
#define VIM_GRP_STS_MSK_RESETVAL             (0x00000000U)
#define VIM_GRP_STS_MSK_MAX                  (BIT_MASK(32))

#define VIM_GRP_STS_RESETVAL                 (0x00000000U)

/* INTR_EN_SET */

#define VIM_GRP_INTR_EN_SET_MSK_MASK         (BIT_MASK(32))
#define VIM_GRP_INTR_EN_SET_MSK_SHIFT        (0x00000000U)
#define VIM_GRP_INTR_EN_SET_MSK_RESETVAL     (0x00000000U)
#define VIM_GRP_INTR_EN_SET_MSK_MAX          (BIT_MASK(32))

#define VIM_GRP_INTR_EN_SET_RESETVAL         (0x00000000U)

/* INTR_EN_CLR */

#define VIM_GRP_INTR_EN_CLR_MSK_MASK         (BIT_MASK(32))
#define VIM_GRP_INTR_EN_CLR_MSK_SHIFT        (0x00000000U)
#define VIM_GRP_INTR_EN_CLR_MSK_RESETVAL     (0x00000000U)
#define VIM_GRP_INTR_EN_CLR_MSK_MAX          (BIT_MASK(32))

#define VIM_GRP_INTR_EN_CLR_RESETVAL         (0x00000000U)

/* IRQSTS */

#define VIM_GRP_IRQSTS_MSK_MASK              (BIT_MASK(32))
#define VIM_GRP_IRQSTS_MSK_SHIFT             (0x00000000U)
#define VIM_GRP_IRQSTS_MSK_RESETVAL          (0x00000000U)
#define VIM_GRP_IRQSTS_MSK_MAX               (BIT_MASK(32))

#define VIM_GRP_IRQSTS_RESETVAL              (0x00000000U)

/* FIQSTS */

#define VIM_GRP_FIQSTS_MSK_MASK              (BIT_MASK(32))
#define VIM_GRP_FIQSTS_MSK_SHIFT             (0x00000000U)
#define VIM_GRP_FIQSTS_MSK_RESETVAL          (0x00000000U)
#define VIM_GRP_FIQSTS_MSK_MAX               (BIT_MASK(32))

#define VIM_GRP_FIQSTS_RESETVAL              (0x00000000U)

/* INTMAP */

#define VIM_GRP_INTMAP_MSK_MASK              (BIT_MASK(32))
#define VIM_GRP_INTMAP_MSK_SHIFT             (0x00000000U)
#define VIM_GRP_INTMAP_MSK_RESETVAL          (0x00000000U)
#define VIM_GRP_INTMAP_MSK_MAX               (BIT_MASK(32))

#define VIM_GRP_INTMAP_RESETVAL              (0x00000000U)

/* INTTYPE */

#define VIM_GRP_INTTYPE_MSK_MASK             (BIT_MASK(32))
#define VIM_GRP_INTTYPE_MSK_SHIFT            (0x00000000U)
#define VIM_GRP_INTTYPE_MSK_RESETVAL         (0x00000000U)
#define VIM_GRP_INTTYPE_MSK_MAX              (BIT_MASK(32))

#define VIM_GRP_INTTYPE_RESETVAL             (0x00000000U)

/* INT */

#define VIM_PRI_INT_VAL_MASK                 (BIT_MASK(4))
#define VIM_PRI_INT_VAL_SHIFT                (0x00000000U)
#define VIM_PRI_INT_VAL_RESETVAL             (BIT_MASK(4))
#define VIM_PRI_INT_VAL_MAX                  (BIT_MASK(4))

#define VIM_PRI_INT_RESETVAL                 (BIT_MASK(4))

/* INT */

#define VIM_VEC_INT_VAL_MASK                 (0xFFFFFFFCU)
#define VIM_VEC_INT_VAL_SHIFT                (0x00000002U)
#define VIM_VEC_INT_VAL_RESETVAL             (0x00000000U)
#define VIM_VEC_INT_VAL_MAX                  (BIT_MASK(30))

#define VIM_VEC_INT_RESETVAL                 (0x00000000U)

/* INFO */

#define VIM_INFO_INTERRUPTS_MASK             (BIT_MASK(11))
#define VIM_INFO_INTERRUPTS_SHIFT            (0x00000000U)
#define VIM_INFO_INTERRUPTS_RESETVAL         (0x00000400U)
#define VIM_INFO_INTERRUPTS_MAX              (BIT_MASK(11))

#define VIM_INFO_RESETVAL                    (0x00000400U)

/* PRIIRQ */

#define VIM_PRIIRQ_VALID_MASK                (0x80000000U)
#define VIM_PRIIRQ_VALID_SHIFT               (BIT_MASK(5))
#define VIM_PRIIRQ_VALID_RESETVAL            (0x00000000U)
#define VIM_PRIIRQ_VALID_MAX                 (0x00000001U)

#define VIM_PRIIRQ_VALID_VAL_TRUE            (0x1U)
#define VIM_PRIIRQ_VALID_VAL_FALSE           (0x0U)

#define VIM_PRIIRQ_PRI_MASK                  (0x000F0000U)
#define VIM_PRIIRQ_PRI_SHIFT                 (0x00000010U)
#define VIM_PRIIRQ_PRI_RESETVAL              (0x00000000U)
#define VIM_PRIIRQ_PRI_MAX                   (BIT_MASK(4))

#define VIM_PRIIRQ_NUM_MASK                  (BIT_MASK(10))
#define VIM_PRIIRQ_NUM_SHIFT                 (0x00000000U)
#define VIM_PRIIRQ_NUM_RESETVAL              (0x00000000U)
#define VIM_PRIIRQ_NUM_MAX                   (BIT_MASK(10))

#define VIM_PRIIRQ_RESETVAL                  (0x00000000U)

/* PRIFIQ */

#define VIM_PRIFIQ_VALID_MASK                (0x80000000U)
#define VIM_PRIFIQ_VALID_SHIFT               (BIT_MASK(5))
#define VIM_PRIFIQ_VALID_RESETVAL            (0x00000000U)
#define VIM_PRIFIQ_VALID_MAX                 (0x00000001U)

#define VIM_PRIFIQ_VALID_VAL_TRUE            (0x1U)
#define VIM_PRIFIQ_VALID_VAL_FALSE           (0x0U)

#define VIM_PRIFIQ_PRI_MASK                  (0x000F0000U)
#define VIM_PRIFIQ_PRI_SHIFT                 (0x00000010U)
#define VIM_PRIFIQ_PRI_RESETVAL              (0x00000000U)
#define VIM_PRIFIQ_PRI_MAX                   (BIT_MASK(4))

#define VIM_PRIFIQ_NUM_MASK                  (BIT_MASK(10))
#define VIM_PRIFIQ_NUM_SHIFT                 (0x00000000U)
#define VIM_PRIFIQ_NUM_RESETVAL              (0x00000000U)
#define VIM_PRIFIQ_NUM_MAX                   (BIT_MASK(10))

#define VIM_PRIFIQ_RESETVAL                  (0x00000000U)

/* IRQGSTS */

#define VIM_IRQGSTS_STS_MASK                 (BIT_MASK(32))
#define VIM_IRQGSTS_STS_SHIFT                (0x00000000U)
#define VIM_IRQGSTS_STS_RESETVAL             (0x00000000U)
#define VIM_IRQGSTS_STS_MAX                  (BIT_MASK(32))

#define VIM_IRQGSTS_RESETVAL                 (0x00000000U)

/* FIQGSTS */

#define VIM_FIQGSTS_STS_MASK                 (BIT_MASK(32))
#define VIM_FIQGSTS_STS_SHIFT                (0x00000000U)
#define VIM_FIQGSTS_STS_RESETVAL             (0x00000000U)
#define VIM_FIQGSTS_STS_MAX                  (BIT_MASK(32))

#define VIM_FIQGSTS_RESETVAL                 (0x00000000U)

/* IRQVEC */

#define VIM_IRQVEC_ADDR_MASK                 (0xFFFFFFFCU)
#define VIM_IRQVEC_ADDR_SHIFT                (0x00000002U)
#define VIM_IRQVEC_ADDR_RESETVAL             (0x00000000U)
#define VIM_IRQVEC_ADDR_MAX                  (BIT_MASK(30))

#define VIM_IRQVEC_RESETVAL                  (0x00000000U)

/* FIQVEC */

#define VIM_FIQVEC_ADDR_MASK                 (0xFFFFFFFCU)
#define VIM_FIQVEC_ADDR_SHIFT                (0x00000002U)
#define VIM_FIQVEC_ADDR_RESETVAL             (0x00000000U)
#define VIM_FIQVEC_ADDR_MAX                  (BIT_MASK(30))

#define VIM_FIQVEC_RESETVAL                  (0x00000000U)

/* ACTIRQ */

#define VIM_ACTIRQ_VALID_MASK                (0x80000000U)
#define VIM_ACTIRQ_VALID_SHIFT               (BIT_MASK(5))
#define VIM_ACTIRQ_VALID_RESETVAL            (0x00000000U)
#define VIM_ACTIRQ_VALID_MAX                 (0x00000001U)

#define VIM_ACTIRQ_VALID_VAL_TRUE            (0x1U)
#define VIM_ACTIRQ_VALID_VAL_FALSE           (0x0U)

#define VIM_ACTIRQ_PRI_MASK                  (0x000F0000U)
#define VIM_ACTIRQ_PRI_SHIFT                 (0x00000010U)
#define VIM_ACTIRQ_PRI_RESETVAL              (0x00000000U)
#define VIM_ACTIRQ_PRI_MAX                   (BIT_MASK(4))

#define VIM_ACTIRQ_NUM_MASK                  (BIT_MASK(10))
#define VIM_ACTIRQ_NUM_SHIFT                 (0x00000000U)
#define VIM_ACTIRQ_NUM_RESETVAL              (0x00000000U)
#define VIM_ACTIRQ_NUM_MAX                   (BIT_MASK(10))

#define VIM_ACTIRQ_RESETVAL                  (0x00000000U)

/* ACTFIQ */

#define VIM_ACTFIQ_VALID_MASK                (0x80000000U)
#define VIM_ACTFIQ_VALID_SHIFT               (BIT_MASK(5))
#define VIM_ACTFIQ_VALID_RESETVAL            (0x00000000U)
#define VIM_ACTFIQ_VALID_MAX                 (0x00000001U)

#define VIM_ACTFIQ_VALID_VAL_TRUE            (0x1U)
#define VIM_ACTFIQ_VALID_VAL_FALSE           (0x0U)

#define VIM_ACTFIQ_PRI_MASK                  (0x000F0000U)
#define VIM_ACTFIQ_PRI_SHIFT                 (0x00000010U)
#define VIM_ACTFIQ_PRI_RESETVAL              (0x00000000U)
#define VIM_ACTFIQ_PRI_MAX                   (BIT_MASK(4))

#define VIM_ACTFIQ_NUM_MASK                  (BIT_MASK(10))
#define VIM_ACTFIQ_NUM_SHIFT                 (0x00000000U)
#define VIM_ACTFIQ_NUM_RESETVAL              (0x00000000U)
#define VIM_ACTFIQ_NUM_MAX                   (BIT_MASK(10))

#define VIM_ACTFIQ_RESETVAL                  (0x00000000U)

/* DEDVEC */

#define VIM_DEDVEC_ADDR_MASK                 (0xFFFFFFFCU)
#define VIM_DEDVEC_ADDR_SHIFT                (0x00000002U)
#define VIM_DEDVEC_ADDR_RESETVAL             (0x00000000U)
#define VIM_DEDVEC_ADDR_MAX                  (BIT_MASK(30))

#define VIM_DEDVEC_RESETVAL                  (0x00000000U)

/*
 * VIM Driver Interface Functions
 */

/**
 * @brief Get active interrupt ID.
 *
 * @return Returns the ID of an active interrupt.
 */
unsigned int z_vim_irq_get_active(void);

/**
 * @brief Signal end-of-interrupt.
 *
 * @param irq interrupt ID.
 */
void z_vim_irq_eoi(unsigned int irq);

/**
 * @brief Interrupt controller initialization.
 */
void z_vim_irq_init(void);

/**
 * @brief Configure priority, irq type for the interrupt ID.
 *
 * @param irq interrupt ID.
 * @param prio interrupt priority.
 * @param flags interrupt flags.
 */
void z_vim_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags);

/**
 * @brief Enable Interrupt.
 *
 * @param irq interrupt ID.
 */
void z_vim_irq_enable(unsigned int irq);

/**
 * @brief Disable Interrupt.
 *
 * @param irq interrupt ID.
 */
void z_vim_irq_disable(unsigned int irq);

/**
 * @brief Check if an interrupt is enabled.
 *
 * @param irq interrupt ID.
 *
 * @retval 0 If interrupt is disabled.
 * @retval 1 If interrupt is enabled.
 */
int z_vim_irq_is_enabled(unsigned int irq);

/**
 * @brief Raise a software interrupt.
 *
 * @param irq interrupt ID.
 */
void z_vim_arm_enter_irq(int irq);

#endif /* ZEPHYR_DRIVERS_INTC_VIM_H_ */

/*
 * Copyright (c) 2026 Jonathan Elliot Peace <jep@alphabetiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree IRQ-number bindings for the BCM2836 ARM-local
 *        interrupt controller (per-core timers, mailboxes, GPU cascade
 *        and PMU sources).
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_BCM2836_L1_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_BCM2836_L1_H_

/** @cond INTERNAL_HIDDEN */

/* Per-core sources on the BCM2836 ARM-local interrupt controller. */
#define BCM2836_LOCAL_IRQ_CNTPSIRQ   0  /* secure physical timer */
#define BCM2836_LOCAL_IRQ_CNTPNSIRQ  1  /* non-secure physical timer */
#define BCM2836_LOCAL_IRQ_CNTHPIRQ   2  /* hypervisor physical timer */
#define BCM2836_LOCAL_IRQ_CNTVIRQ    3  /* virtual timer */
#define BCM2836_LOCAL_IRQ_MAILBOX0   4
#define BCM2836_LOCAL_IRQ_MAILBOX1   5
#define BCM2836_LOCAL_IRQ_MAILBOX2   6
#define BCM2836_LOCAL_IRQ_MAILBOX3   7
#define BCM2836_LOCAL_IRQ_GPU        8  /* cascade from BCM2835 ARMC */
#define BCM2836_LOCAL_IRQ_PMU        9

/* IRQ-flag aliases mirroring arm-gic.h, so dts entries read alike. */
#ifndef IRQ_TYPE_LEVEL
#define IRQ_TYPE_LEVEL  2  /* BIT(1) */
#endif
#ifndef IRQ_TYPE_EDGE
#define IRQ_TYPE_EDGE   4  /* BIT(2) */
#endif

/** @endcond */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_BCM2836_L1_H_ */

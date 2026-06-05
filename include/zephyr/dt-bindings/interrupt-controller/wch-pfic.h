/*
 * Copyright (c) 2026 Colt Ma
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief WCH PFIC interrupt priority encoding macros for Device Tree.
 *
 * QingKe PFIC (Programmable Fast Interrupt Controller) supports two
 * priority grouping modes controlled by the INESTCTRL (CSR 0x804)
 * hardware register.
 *
 * When interrupt nesting is enabled (INESTCTRL bit1 = 1):
 *   IPRIOR[7]   = Preemption group  (0 or 1)
 *   IPRIOR[6:5] = Sub-priority       (0..3)
 *   IPRIOR[4:0] = Reserved
 *
 * When interrupt nesting is disabled (INESTCTRL bit1 = 0):
 *   IPRIOR[7:5] = Sub-priority       (0..7)
 *   IPRIOR[4:0] = Reserved
 *
 * The WCH_PFIC_PRIORITY() macro assumes nesting is enabled (the
 * default configuration for Zephyr on QingKe targets) and packs both
 * fields into the 8-bit IPRIOR format.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_WCH_PFIC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_WCH_PFIC_H_

/**
 * @brief Encode QingKe PFIC priority from preemption group and sub-priority.
 *
 * @param preempt  Preemption group: 0 (higher) or 1 (lower).
 *                 Group-0 ISRs can preempt group-1 ISRs.
 * @param sub      Sub-priority within the group: 0..3.
 *
 * @return 8-bit PFIC IPRIOR register value.
 */
#define WCH_PFIC_PRIORITY(preempt, sub) \
	((((preempt) & 0x1) << 7) | (((sub) & 0x3) << 5))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_WCH_PFIC_H_ */

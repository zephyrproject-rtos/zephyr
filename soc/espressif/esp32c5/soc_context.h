/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_RISCV_ESP32C5_SOC_CONTEXT_H
#define SOC_RISCV_ESP32C5_SOC_CONTEXT_H

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE

/* CLIC context: mcause for mil restore, mintthresh for preemption level */
#define SOC_ESF_MEMBERS                                                                            \
	unsigned long mcause;                                                                      \
	unsigned long mintthresh

#define SOC_ESF_INIT 0, 0

#endif /* CONFIG_RISCV_SOC_CONTEXT_SAVE */

#endif /* SOC_RISCV_ESP32C5_SOC_CONTEXT_H */

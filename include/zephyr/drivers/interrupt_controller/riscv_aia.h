/*
 * Copyright (c) 2025 Synopsys, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_RISCV_AIA_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_RISCV_AIA_H_

#include <zephyr/device.h>
#include <zephyr/types.h>
#include <zephyr/drivers/interrupt_controller/riscv_aplic.h>
#include <zephyr/drivers/interrupt_controller/riscv_imsic.h>

/* Unified AIA API - wraps APLIC and IMSIC */

/* IRQ control (EIID-based) */
void riscv_aia_irq_enable(uint32_t irq);
void riscv_aia_irq_disable(uint32_t irq);
int riscv_aia_irq_is_enabled(uint32_t irq);
void riscv_aia_set_priority(uint32_t irq, uint32_t prio);

/* Source configuration and routing */
const struct device *riscv_aia_get_dev(void);
void riscv_aia_config_source(unsigned int src, uint32_t mode);
void riscv_aia_route_to_hart(unsigned int src, uint32_t hart, uint32_t eiid);
void riscv_aia_enable_source(unsigned int src);
void riscv_aia_inject_msi(uint32_t hart, uint32_t eiid);

#endif

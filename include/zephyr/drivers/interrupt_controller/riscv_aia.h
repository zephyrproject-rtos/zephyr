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

/* Check if source is an APLIC source vs local interrupt */
bool riscv_aia_is_aplic_source(uint32_t src);

/* APLIC source control (1:1 EIID mapping) */
void riscv_aia_irq_enable(uint32_t src);
void riscv_aia_irq_disable(uint32_t src);
int riscv_aia_irq_is_enabled(uint32_t src);
void riscv_aia_set_priority(uint32_t src, uint32_t prio);

/* Source configuration and routing */
const struct device *riscv_aia_get_dev(void);
void riscv_aia_config_source(unsigned int src, uint32_t mode);
void riscv_aia_route_to_hart(unsigned int src, uint32_t hart, uint32_t eiid);
void riscv_aia_enable_source(unsigned int src);
void riscv_aia_inject_msi(uint32_t hart, uint32_t eiid);

#endif

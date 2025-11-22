/*
 * Copyright (c) 2025 Synopsys, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/* Unified AIA coordinator - wraps APLIC and IMSIC for Zephyr integration */

#include <zephyr/kernel.h>
#include <zephyr/drivers/interrupt_controller/riscv_aia.h>
#include <zephyr/drivers/interrupt_controller/riscv_imsic.h>
#include <zephyr/drivers/interrupt_controller/riscv_aplic.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(intc_riscv_aia, CONFIG_LOG_DEFAULT_LEVEL);

void riscv_aia_irq_enable(uint32_t irq)
{
	/*
	 * In AIA-MSI mode, IRQ number is an EIID.
	 * APLIC sources are configured separately via riscv_aplic_* APIs.
	 */
	riscv_imsic_enable_eiid(irq);
}

void riscv_aia_irq_disable(uint32_t irq)
{
	riscv_imsic_disable_eiid(irq);
}

int riscv_aia_irq_is_enabled(uint32_t irq)
{
	/* Check IMSIC enable state on CURRENT CPU */
	return riscv_imsic_is_enabled(irq);
}

void riscv_aia_set_priority(uint32_t irq, uint32_t prio)
{
	/* APLIC-MSI mode has no per-source priority registers.
	 * Priority in AIA is handled via IMSIC EITHRESHOLD (global threshold)
	 * or implicit EIID ordering (lower EIID = higher priority).
	 */
	if (prio != 0) {
		LOG_WRN("AIA-MSI: per-IRQ priority not supported (EIID %u, prio %u ignored)", irq,
			prio);
	}
}

/* Source configuration and routing wrappers */

const struct device *riscv_aia_get_dev(void)
{
	return riscv_aplic_get_dev();
}

void riscv_aia_config_source(unsigned int src, uint32_t mode)
{
	const struct device *aplic = riscv_aplic_get_dev();

	if (aplic) {
		riscv_aplic_msi_config_src(aplic, src, mode);
	}
}

void riscv_aia_route_to_hart(unsigned int src, uint32_t hart, uint32_t eiid)
{
	const struct device *aplic = riscv_aplic_get_dev();

	if (aplic) {
		riscv_aplic_msi_route(aplic, src, hart, eiid);
	}
}

void riscv_aia_enable_source(unsigned int src)
{
	riscv_aplic_enable_source(src);
}

void riscv_aia_inject_msi(uint32_t hart, uint32_t eiid)
{
	riscv_aplic_inject_genmsi(hart, eiid);
}

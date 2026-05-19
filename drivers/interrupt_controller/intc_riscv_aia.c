/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Unified AIA coordinator - wraps APLIC and IMSIC for Zephyr integration.
 *
 * This driver presents AIA (APLIC + IMSIC) as a single interrupt controller
 * to Zephyr, hiding the internal complexity. Like PLIC, AIA external interrupt
 * sources are represented as second-level Zephyr IRQs. The local second-level
 * IRQ number is used as both APLIC source and IMSIC EIID.
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/devicetree/interrupt_controller.h>
#include <zephyr/drivers/interrupt_controller/riscv_aia.h>
#include <zephyr/drivers/interrupt_controller/riscv_aplic.h>
#include <zephyr/drivers/interrupt_controller/riscv_imsic.h>
#include <zephyr/irq_multilevel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sw_isr_table.h>

LOG_MODULE_REGISTER(intc_riscv_aia, CONFIG_LOG_DEFAULT_LEVEL);

#define APLIC_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(riscv_aplic)
#define APLIC_ISR_TABLE_OFFSET INTC_BASE_ISR_TBL_OFFSET(APLIC_NODE)

IRQ_PARENT_ENTRY_DEFINE(riscv_aia, DEVICE_DT_GET(APLIC_NODE), DT_IRQN(APLIC_NODE),
			APLIC_ISR_TABLE_OFFSET, DT_INTC_GET_AGGREGATOR_LEVEL(APLIC_NODE));

static inline uint32_t riscv_aia_irq_to_src(uint32_t irq)
{
	return irq_from_level_2(irq);
}

static bool riscv_aia_src_is_valid(const struct device *aplic, uint32_t src)
{
	return src > 0 && src <= riscv_aplic_get_num_sources(aplic);
}

void riscv_aia_irq_enable(uint32_t irq)
{
	const struct device *aplic = riscv_aplic_get_dev();
	uint32_t src = riscv_aia_irq_to_src(irq);

	if (!riscv_aia_src_is_valid(aplic, src)) {
		return;
	}

	riscv_imsic_enable_eiid(src);

#ifdef CONFIG_RISCV_APLIC_MSI
	riscv_aplic_msi_route(aplic, src, 0, src);
#endif

	riscv_aplic_enable_src(aplic, src, true);
}

void riscv_aia_irq_disable(uint32_t irq)
{
	const struct device *aplic = riscv_aplic_get_dev();
	uint32_t src = riscv_aia_irq_to_src(irq);

	if (!riscv_aia_src_is_valid(aplic, src)) {
		return;
	}

	riscv_aplic_enable_src(aplic, src, false);
	riscv_imsic_disable_eiid(src);
}

int riscv_aia_irq_is_enabled(uint32_t irq)
{
	const struct device *aplic = riscv_aplic_get_dev();
	uint32_t src = riscv_aia_irq_to_src(irq);

	if (!riscv_aia_src_is_valid(aplic, src)) {
		return 0;
	}

	return riscv_imsic_is_enabled(src);
}

void riscv_aia_set_priority(uint32_t irq, uint32_t prio)
{
	uint32_t src = riscv_aia_irq_to_src(irq);

	/*
	 * APLIC-MSI mode has no per-source priority registers. Priority in AIA
	 * is handled via IMSIC EITHRESHOLD or implicit EIID ordering.
	 */
	if (prio != 0) {
		LOG_WRN("AIA-MSI: per-source priority not supported (src %u, prio %u ignored)", src,
			prio);
	}
}

void riscv_aia_config_source(uint32_t irq, uint32_t mode)
{
	const struct device *aplic = riscv_aplic_get_dev();
	uint32_t src = riscv_aia_irq_to_src(irq);

	if (!riscv_aia_src_is_valid(aplic, src)) {
		return;
	}

	riscv_aplic_config_src(aplic, src, mode);
}

#if defined(CONFIG_RISCV_APLIC_MSI)
void riscv_aia_route_to_hart(uint32_t irq, uint32_t hart, uint32_t eiid)
{
	const struct device *aplic = riscv_aplic_get_dev();
	uint32_t src = riscv_aia_irq_to_src(irq);

	if (!riscv_aia_src_is_valid(aplic, src)) {
		return;
	}

	riscv_aplic_msi_route(aplic, src, hart, eiid);
}

void riscv_aia_inject_msi(uint32_t hart, uint32_t eiid)
{
	riscv_aplic_msi_inject_genmsi(hart, eiid);
}
#endif /* CONFIG_RISCV_APLIC_MSI */

void riscv_aia_enable_source(uint32_t irq)
{
	riscv_aia_irq_enable(irq);
}

void riscv_aia_dispatch_eiid(uint32_t eiid)
{
	const struct device *aplic = riscv_aplic_get_dev();
	const struct _isr_table_entry *ite;

	if (!riscv_aia_src_is_valid(aplic, eiid)) {
		z_irq_spurious(NULL);
		return;
	}

	ite = &_sw_isr_table[APLIC_ISR_TABLE_OFFSET + eiid];
	ite->isr(ite->arg);
}

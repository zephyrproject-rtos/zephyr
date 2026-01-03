/*
 * Copyright (c) 2025 Synopsys, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Unified AIA coordinator - wraps APLIC and IMSIC for Zephyr integration
 *
 * This driver presents AIA (APLIC + IMSIC) as a single interrupt controller
 * to Zephyr, hiding the internal complexity. It determines which sources are
 * APLIC-managed vs local interrupts by checking the devicetree at compile
 * time - only devices whose interrupt-parent is the APLIC are marked as managed.
 *
 * Mapping: APLIC source N → EIID N (1:1 mapping)
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/interrupt_controller/riscv_aia.h>
#include <zephyr/drivers/interrupt_controller/riscv_imsic.h>
#include <zephyr/drivers/interrupt_controller/riscv_aplic.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(intc_riscv_aia, CONFIG_LOG_DEFAULT_LEVEL);

/*
 * Build the APLIC source bitmap at compile time from devicetree.
 *
 * We iterate through all nodes with status "okay" that have an interrupts
 * property, and check if their interrupt controller is the APLIC. If so,
 * we add their source number to the bitmap.
 *
 * This approach is correct because:
 * - Only devices that explicitly wire to APLIC in DT get marked
 * - Local interrupts (timer, software) wire to cpu-intc, not APLIC
 * - No hardcoded interrupt numbers needed
 */

/* Get the APLIC node if it exists */
#define APLIC_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(riscv_aplic_msi)

/*
 * Helper macro: Check if a node has interrupts property and its interrupt
 * controller is the APLIC. Returns the source bit to OR into bitmap, or 0.
 */
#define APLIC_SRC_BIT(node_id)                                                                     \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, interrupts),                       \
		(COND_CODE_1(DT_SAME_NODE(DT_IRQ_INTC(node_id), APLIC_NODE),      \
			((1ULL << DT_IRQ(node_id, irq))),                        \
			(0ULL))),                                                \
		(0ULL))

/*
 * Iterate through all status-okay nodes and build the bitmap.
 * Each node contributes its source bit if it uses APLIC as interrupt controller.
 */
#define BUILD_APLIC_BITMAP(node_id) | APLIC_SRC_BIT(node_id)

/*
 * Compile-time bitmap of APLIC sources.
 * Bit N set = source N is an APLIC source (device wired to APLIC in DT)
 * Bit N clear = not an APLIC source (local interrupt using mie CSR)
 */
static const uint64_t aplic_sources = 0ULL DT_FOREACH_STATUS_OKAY_NODE(BUILD_APLIC_BITMAP);

static int riscv_aia_init(void)
{
	LOG_DBG("AIA: APLIC sources bitmap = 0x%016llx (from DT)", aplic_sources);
	return 0;
}

/* Run after APLIC init (PRE_KERNEL_1) but before devices that use interrupts */
SYS_INIT(riscv_aia_init, PRE_KERNEL_2, 0);

/*
 * Check if a source is managed by APLIC.
 * Returns true if it should be routed through AIA (APLIC+IMSIC),
 * false if it's a local interrupt that uses mie CSR directly.
 */
bool riscv_aia_is_aplic_source(uint32_t src)
{
	if (src >= 64) {
		return false;
	}
	return (aplic_sources & (1ULL << src)) != 0;
}

void riscv_aia_irq_enable(uint32_t src)
{
	const struct device *aplic = riscv_aplic_get_dev();

	LOG_DBG("AIA enable: APLIC src %u -> EIID %u", src, src);

	/* Enable the EIID in IMSIC (1:1 mapping: EIID = source) */
	riscv_imsic_enable_eiid(src);

	/* Configure and enable the APLIC source */
	if (aplic && src > 0) {
		/* Route APLIC source to hart 0 with 1:1 EIID mapping */
		riscv_aplic_msi_route(aplic, src, 0, src);
		riscv_aplic_enable_src(aplic, src, true);
	}
}

void riscv_aia_irq_disable(uint32_t src)
{
	const struct device *aplic = riscv_aplic_get_dev();

	riscv_imsic_disable_eiid(src);

	if (aplic && src > 0) {
		riscv_aplic_enable_src(aplic, src, false);
	}
}

int riscv_aia_irq_is_enabled(uint32_t src)
{
	return riscv_imsic_is_enabled(src);
}

void riscv_aia_set_priority(uint32_t src, uint32_t prio)
{
	/*
	 * APLIC-MSI mode has no per-source priority registers.
	 * Priority in AIA is handled via IMSIC EITHRESHOLD (global threshold)
	 * or implicit EIID ordering (lower EIID = higher priority).
	 */
	if (prio != 0) {
		LOG_WRN("AIA-MSI: per-source priority not supported (src %u, prio %u ignored)", src,
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
		riscv_aplic_config_src(aplic, src, mode);
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
	riscv_aplic_msi_inject_genmsi(hart, eiid);
}

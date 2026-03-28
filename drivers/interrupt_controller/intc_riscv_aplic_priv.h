/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private header shared between APLIC core and MSI module.
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_RISCV_APLIC_PRIV_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_RISCV_APLIC_PRIV_H_

#include <zephyr/spinlock.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

struct aplic_cfg {
	uintptr_t base;
	uint32_t num_sources;
#ifdef CONFIG_RISCV_APLIC_MSI
	uint32_t imsic_addr;
#endif
};

struct aplic_data {
	struct k_spinlock lock;
};

static inline uint32_t rd32(uintptr_t base, uint32_t off)
{
	return sys_read32(base + off);
}

static inline void wr32(uintptr_t base, uint32_t off, uint32_t v)
{
	sys_write32(v, base + off);
}

#ifdef CONFIG_RISCV_APLIC_MSI
/**
 * @brief MSI-specific APLIC initialisation.
 *
 * Called from aplic_init() when CONFIG_RISCV_APLIC_MSI is enabled.
 * Configures MSI address registers and enables MSI delivery.
 *
 * @param dev APLIC device
 * @return 0 on success, negative error code on failure
 */
int aplic_msi_init(const struct device *dev);
#endif /* CONFIG_RISCV_APLIC_MSI */

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_RISCV_APLIC_PRIV_H_ */

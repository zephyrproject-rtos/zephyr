/*
 * Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/ztest.h>

DEVICE_DT_DEFINE(DT_NODELABEL(test_msi), NULL, NULL, NULL, NULL, POST_KERNEL, 0, NULL);
DEVICE_DT_DEFINE(DT_NODELABEL(test_pcie), NULL, NULL, NULL, NULL, POST_KERNEL, 0, NULL);

/* Inspect the real driver's register writes without a PCIe device. */
#include "intc_rp1.c"

void k_mem_map_phys_bare(uint8_t **virt_ptr, uintptr_t phys, size_t size, uint32_t flags)
{
	ARG_UNUSED(virt_ptr);
	ARG_UNUSED(phys);
	ARG_UNUSED(size);
	ARG_UNUSED(flags);
	zassert_unreachable("No PCIe memory mapping is expected");
}

ZTEST(rp1_intc, test_unready_pcie)
{
	struct device_state pcie_state = {0};
	const struct device pcie = {
		.state = &pcie_state,
	};
	const struct rp1_irq_config config = {
		.pcie = &pcie,
		.num_irqs = RP1_NUM_IRQS,
	};
	struct rp1_irq_data data = {0};
	const struct device dev = {
		.config = &config,
		.data = &data,
	};

	zassert_equal(rp1_irq_init(&dev), -ENODEV);
	zassert_false(data.intc.ready);
}

ZTEST(rp1_intc, test_source_trigger_types)
{
	uint32_t registers[0x1000U / sizeof(uint32_t)] = {0};
	struct rp1_irq_data data = {
		._mmio = (mm_reg_t)registers,
	};
	const struct device dev = {
		.data = &data,
	};

	rp1_intc_setup(&dev, RP1_NUM_IRQS);

	for (uint32_t line = 0; line < RP1_NUM_IRQS; line++) {
		bool edge = line >= RP1_IRQ_USBHOST0 && line <= RP1_IRQ_USBHOST1_3;
		uint32_t offset = RP1_MSIX_CFG(line);
		uint32_t cfg = registers[(RP1_MSIX_CFG_SET + offset) / sizeof(uint32_t)];

		zassert_equal(registers[(RP1_MSIX_CFG_CLR + offset) / sizeof(uint32_t)],
			      RP1_MSIX_CFG_ENABLE | RP1_MSIX_CFG_IACK_EN,
			      "Vector %u was not disabled and cleared", line);
		zassert_equal(cfg, edge ? 0U : RP1_MSIX_CFG_IACK_EN,
			      "Vector %u has the wrong IACK mode", line);

		/* Apply the SET write to the base register before testing EOI. */
		registers[offset / sizeof(uint32_t)] = cfg;
		registers[(RP1_MSIX_CFG_SET + offset) / sizeof(uint32_t)] = 0U;
		rp1_irq_eoi(&dev, line);
		zassert_equal(registers[(RP1_MSIX_CFG_SET + offset) / sizeof(uint32_t)],
			      edge ? 0U : RP1_MSIX_CFG_IACK, "Vector %u has the wrong EOI behavior",
			      line);
	}
}

ZTEST_SUITE(rp1_intc, NULL, NULL, NULL, NULL, NULL);

/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bus.h>

#if defined(CONFIG_ACPI)
int platform_acpi_init(const struct device *dev, platform_isr_t isr, int irq, int priority,
		       uint32_t flags, char *hid, char *uid)
{
	int ret;
	struct acpi_dev *acpi_chld;
	uint16_t irqs[CONFIG_ACPI_IRQ_VECTOR_MAX];
	struct acpi_reg_base reg_bases[CONFIG_ACPI_MMIO_ENTRIES_MAX];
	struct acpi_irq_resource irq_res;
	struct acpi_mmio_resource mmio_res;
	mm_reg_t *reg_base = DEVICE_MMIO_RAM_PTR(dev);

	acpi_chld = acpi_device_get(hid, uid);
	if (!acpi_chld) {
		return -ENODEV;
	}

	mmio_res.mmio_max = ARRAY_SIZE(reg_bases);
	mmio_res.reg_base = reg_bases;
	ret = acpi_device_mmio_get(acpi_chld, &mmio_res);
	if (ret) {
		return ret;
	}

	if (ACPI_RESOURCE_TYPE_GET(&mmio_res) == ACPI_RES_TYPE_IO) {
		*reg_base = (mm_reg_t)ACPI_IO_GET(&mmio_res);
	} else if (ACPI_RESOURCE_TYPE_GET(&mmio_res) == ACPI_RES_TYPE_MEM) {
#if defined(CONFIG_MMU)
		device_map(reg_base, ACPI_MMIO_GET(&mmio_res), ACPI_RESOURCE_SIZE_GET(&mmio_res),
			   K_MEM_CACHE_NONE);
#else
		*reg_base = ACPI_MMIO_GET(&mmio_res);
#endif
	} else {
		return -ENODEV;
	}

	if (isr) {
		irq_res.irq_vector_max = ARRAY_SIZE(irqs);
		irq_res.irqs = irqs;
		if (irq != ACPI_IRQ_DETECT) {
			irq_connect_dynamic(irq, priority,
					(void (*)(const void *))isr, dev, flags);
			irq_enable(irq);
		} else if (!acpi_device_irq_get(acpi_chld, &irq_res)) {
			irq_connect_dynamic(irq_res.irqs[0], priority,
					(void (*)(const void *))isr, dev,
					    irq_res.flags);
			irq_enable(irq_res.irqs[0]);
		} else {
			return -ENODEV;
		}
	}

	return 0;
}
#endif

#if defined(CONFIG_PCIE)
/* TODO: add support for Multi MSI interrupt. */
static int child_dev_config_irq(const struct device *dev, struct pci_conf *cfg, platform_isr_t isr)
{
	int irq;

	/* If found valid ISR for the device. */
	if (cfg->irq_num == PCIE_IRQ_DETECT) {
		irq = pcie_alloc_irq(cfg->pcie->bdf);
		if (irq == PCIE_CONF_INTR_IRQ_NONE) {
			return -EINVAL;
		}
	} else {
		irq = cfg->irq_num;
		pcie_conf_write(cfg->pcie->bdf, PCIE_CONF_INTR, irq);
	}

	pcie_connect_dynamic_irq(cfg->pcie->bdf, irq, cfg->irq_prio, (void (*)(const void *))isr,
				 dev, cfg->irq_flag);

	pcie_irq_enable(cfg->pcie->bdf, irq);

	return 0;
}

int platform_pci_init(const struct device *dev, struct pci_conf *cfg, platform_isr_t isr)
{
	int status = 0;
	struct pcie_bar mbar;

	if (!(cfg->pcie)) {
		return -EINVAL;
	}

	if (!pcie_probe_mbar(cfg->pcie->bdf, 0, &mbar)) {
		return -EINVAL;
	}

	/* TODO check IO mapped or not. */
	pcie_set_cmd(cfg->pcie->bdf, PCIE_CONF_CMDSTAT_MEM, true);

#if defined(CONFIG_MMU)
	/* TODO check IO mapped or not. */
	device_map(DEVICE_MMIO_RAM_PTR(dev), mbar.phys_addr, mbar.size, K_MEM_CACHE_NONE);
#else
	dev->mmio->mem = mbar.phys_addr;
#endif

	if (cfg->bus_master) {
		pcie_set_cmd(cfg->pcie->bdf, PCIE_CONF_CMDSTAT_MASTER, true);
	}

	if (isr) {
		status = child_dev_config_irq(dev, cfg, isr);
	}

	return status;
}
#endif

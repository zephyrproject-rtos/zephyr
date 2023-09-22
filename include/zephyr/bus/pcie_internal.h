/*
 * Copyright (c) 2024 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PCI_INTF_H_
#define ZEPHYR_INCLUDE_PCI_INTF_H_

#include <zephyr/drivers/pcie/pcie.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief PCI config structure (Internal use only).
 */
struct pci_conf {
	struct pcie_dev *pcie;
	bool bus_master;
	unsigned int irq_num;
	unsigned int irq_prio;
	unsigned int irq_flag;
};

/**
 * @brief Platform PCI init function (Internal use only).
 *
 */
int platform_pci_init(const struct device *dev, struct pci_conf *cfg, platform_isr_t isr);

/**
 * @def BUS_DEV_MMIO_ROM
 *
 * @brief Declare storage for MMIO data within a device's config struct
 *
 * @see DEVICE_MMIO_ROM()
 */
#define BUS_DEV_MMIO_ROM DEVICE_MMIO_ROM

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(pcie)
#define Z_BUS_DEV_PCI_DECLARE(n) COND_CODE_1(DT_INST_ON_BUS(n, pcie),\
	(DEVICE_PCIE_INST_DECLARE(n);), ())

DT_INST_FOREACH_STATUS_OKAY(Z_BUS_DEV_PCI_DECLARE)

/**
 * @def BUS_DEV_MMIO_RAM
 *
 * Declare storage for MMIO information within a device's dev_data struct.
 *
 * @see DEVICE_MMIO_RAM()
 */
#define BUS_DEV_MMIO_RAM                                                                      \
	DEVICE_MMIO_RAM;                                                               \
	struct pcie_dev *pcie
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(pcie) */

 /**
  * @def BUS_DEV_RAM_INIT
  *
@@ -70,11 +66,11 @@ DT_INST_FOREACH_STATUS_OKAY(Z_BUS_DEV_PCI_DECLARE)
  *
  * @see DEVICE_MMIO_RAM()
  */

#define BUS_DEV_RAM_INIT(node_id) COND_CODE_1(DT_ON_BUS(node_id, pcie),\
	(DEVICE_PCIE_INIT(node_id, pcie),), ())

#define BUS_DEV_PCI_CTX_GET(n) &Z_DEVICE_PCIE_NAME(n)

/**
 * @brief PCIe specific init function (Internal use only).
 *
 */
#define Z_BUS_DEV_PCI_INIT(n, init, isr)                                                      \
	static int __pci_init_##n(const struct device *dev)                                        \
	{                                                                                          \
		struct pci_conf pci_cfg = {                                                        \
			.pcie = BUS_DEV_PCI_CTX_GET(n),                                           \
			.bus_master = DT_PROP_OR(n, bus_master, 1),                                \
			.irq_num = DT_IRQN(n),                                                     \
			.irq_prio = DT_IRQ(n, priority),                                           \
			.irq_flag = Z_IRQ_FLAGS_GET(n),                                            \
		};                                                                                 \
		platform_isr_t isr_fn = (platform_isr_t)isr;                                       \
												   \
		return platform_pci_init(dev, &pci_cfg, isr_fn);                                   \
	}                                                                                          \
												   \
	static int Z_BUS_DEV_INIT_NAME(n)(const struct device *dev)                           \
	{                                                                                          \
		int status;                                                                        \
												   \
		status = __pci_init_##n(dev);                                                      \
		if (!status) {                                                                     \
			status = init(dev);                                                        \
		}                                                                                  \
		return status;                                                                     \
	}

#endif /* ZEPHYR_INCLUDE_PCI_INTF_H_ */

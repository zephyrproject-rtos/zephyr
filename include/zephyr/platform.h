/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PLATFORM_H_
#define ZEPHYR_INCLUDE_PLATFORM_H_

#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/linker/sections.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/util.h>
#if defined(CONFIG_PLATFORM_BUS)
#include <zephyr/sys/slist.h>
#endif

#if defined(CONFIG_PCIE)
#include <zephyr/drivers/pcie/pcie.h>
#endif

#if defined(CONFIG_ACPI)
#include <zephyr/dt-bindings/acpi/acpi.h>
#include <zephyr/acpi/acpi.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*platform_isr_t)(const struct device *dev);

/**
 * @brief PCI config structure (Internal use only).
 */
struct pci_conf {
	struct pcie_dev *pcie;
	uint8_t bus_master: 1;
	int irq_num;
	int irq_prio;
	int irq_flag;
};

/**
 * @brief Platform PCI init function (Internal use only).
 *
 */
int platform_pci_init(const struct device *dev, struct pci_conf *cfg, platform_isr_t isr);

/**
 * @brief Platform ACPI init function (Internal use only).
 *
 */
int platform_acpi_init(const struct device *dev, platform_isr_t isr, int irq, int priority,
		       uint32_t flags, char *hid, char *uid);

#define Z_PLATFORM_DEV_INIT_NAME(dev_id) _CONCAT(dev_init_, dev_id)

/**
 * @def PLATFORM_MMIO_ROM
 *
 * @brief Declare storage for MMIO data within a device's config struct
 *
 * @see DEVICE_MMIO_ROM()
 */
#define PLATFORM_MMIO_ROM DEVICE_MMIO_ROM

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(pcie)
#define PLATFORM_PCI_DECLARE_0(n)
#define PLATFORM_PCI_DECLARE_1(n) DEVICE_PCIE_INST_DECLARE(n);
#define Z_PLATFORM_PCI_DECLARE(n) _CONCAT(PLATFORM_PCI_DECLARE_, DT_INST_ON_BUS(n, pcie))(n)

DT_INST_FOREACH_STATUS_OKAY(Z_PLATFORM_PCI_DECLARE)

/**
 * @def PLATFORM_MMIO_RAM
 *
 * Declare storage for MMIO information within a device's dev_data struct.
 *
 * @see DEVICE_MMIO_RAM()
 */
#define PLATFORM_MMIO_RAM                                                                          \
	DEVICE_MMIO_RAM;                                                                           \
	struct pcie_dev *pcie
#else
#define PLATFORM_MMIO_RAM DEVICE_MMIO_RAM
#endif

#define PLATFORM_MMIO_RAM_PTR(dev) DEVICE_MMIO_RAM_PTR(dev)
#define PLATFORM_MMIO_GET(dev)     DEVICE_MMIO_GET(dev)

#define PLATFORM_RAM_INIT_0(node_id)
#define PLATFORM_RAM_INIT_1(node_id) DEVICE_PCIE_INIT(node_id, pcie),

/**
 * @def PLATFORM_RAM_INIT
 *
 * @brief Initialize a named struct member to point at a bus context such as pci
 *
 * @param node_id DTS node identifier
 *
 * @see DEVICE_MMIO_RAM()
 */
#define PLATFORM_RAM_INIT(node_id) _CONCAT(PLATFORM_RAM_INIT_, DT_ON_BUS(node_id, pcie))(node_id)

#define IS_IOMAPPED_0(n) 0
#define IS_IOMAPPED_1(n) 1

#define Z_IS_IO_MAPPED(n) _CONCAT(IS_IOMAPPED_, DT_PROP_OR(n, io_mapped, 0))(n)

#ifdef CONFIG_MMU
#define Z_PLATFORM_MMIO_MAP(dev, phy_add, size)                                                    \
	device_map(PLATFORM_MMIO_RAM_PTR(dev), phy_add, size, K_MEM_CACHE_NONE);
#else
#define Z_PLATFORM_MMIO_MAP(dev, phy_add, size) *PLATFORM_MMIO_RAM_PTR(dev) = (uintptr_t)phy_add
#endif

#define IRQ_FLAGS_SENSE0(n) 0
#define IRQ_FLAGS_SENSE1(n) DT_IRQ(n, sense)
#define Z_IRQ_FLAGS_GET(n)  _CONCAT(IRQ_FLAGS_SENSE, DT_IRQ_HAS_CELL(n, sense))(n)

#define PLATFORM_PCI_CTX_GET(n) &Z_DEVICE_PCIE_NAME(n)

#if !defined(PLATFORM_DISABLE_DEV_INTR)
#define IRQ_CONFIG_0(n)
#define IRQ_CONFIG_1(n)                                                                            \
	IRQ_CONNECT(DT_IRQN(n), DT_IRQ(n, priority), __platform_isr_##n, DEVICE_DT_GET(n),         \
		    Z_IRQ_FLAGS_GET(n));
#define Z_IRQ_CONFIG(n) _CONCAT(IRQ_CONFIG_, DT_IRQ_HAS_IDX(n, 0))(n)

#define IRQ_ENABLE_0(n)
#define IRQ_ENABLE_1(n) irq_enable(DT_IRQN(n))
#define Z_IRQ_ENABLE(n) _CONCAT(IRQ_ENABLE_, DT_IRQ_HAS_IDX(n, 0))(n)

#define Z_PLATFORM_ISR_INIT(n, isr)                                                                \
	static inline void __platform_isr_##n(const struct device *dev)                            \
	{                                                                                          \
		platform_isr_t isr_fn = (platform_isr_t)isr;                                       \
		if (isr_fn) {                                                                      \
			isr_fn(dev);                                                               \
		}                                                                                  \
	}                                                                                          \
	static inline void __config_irq_##n(const struct device *dev)                              \
	{                                                                                          \
		Z_IRQ_CONFIG(n);                                                                   \
	}                                                                                          \
	static inline void __enable_irq_##n(const struct device *dev)                              \
	{                                                                                          \
		Z_IRQ_ENABLE(n);                                                                   \
	}

#define Z_DEVICE_DT_INIT(n, init, isr)                                                             \
	static inline int __common_dev_init##n(const struct device *dev)                           \
	{                                                                                          \
		platform_isr_t isr_fn = (platform_isr_t)isr;                                       \
		int status;                                                                        \
                                                                                                   \
		if (isr_fn) {                                                                      \
			__config_irq_##n(dev);                                                     \
		}                                                                                  \
		status = init(dev);                                                                \
		if (status) {                                                                      \
			return status;                                                             \
		}                                                                                  \
		if (isr_fn) {                                                                      \
			__enable_irq_##n(dev);                                                     \
		}                                                                                  \
		return 0;                                                                          \
	}
#else
#define Z_PLATFORM_ISR_INIT(n, isr)
#define Z_DEVICE_DT_INIT(n, init, isr)                                                             \
	static inline int __common_dev_init##n(const struct device *dev)                           \
	{                                                                                          \
		return init(dev);                                                                  \
	}
#endif
/**
 * @brief Platform DTS based init function (Internal use only).
 *
 */
#ifdef DEVICE_MMIO_IS_IN_RAM
#define PLATFORM_ROM_INIT(node_id)

#define Z_PLATFORM_DTS_DEV_INIT(n, init, isr)                                                      \
	Z_PLATFORM_ISR_INIT(n, isr);                                                               \
	Z_DEVICE_DT_INIT(n, init, isr);                                                            \
	static int Z_PLATFORM_DEV_INIT_NAME(n)(const struct device *dev)                           \
	{                                                                                          \
		uintptr_t *reg = PLATFORM_MMIO_RAM_PTR(dev);                                       \
                                                                                                   \
		if (!Z_IS_IO_MAPPED(n)) {                                                          \
			Z_PLATFORM_MMIO_MAP(dev, DT_REG_ADDR(n), DT_REG_SIZE(n));                  \
		} else {                                                                           \
			*reg = DT_REG_ADDR(n);                                                     \
		}                                                                                  \
		return __common_dev_init##n(dev);                                                  \
	}
#else
/**
 * @def PLATFORM_ROM_INIT
 *
 * @brief Initialize a DEVICE_MMIO_ROM member
 *
 * @param node_id DTS node identifier
 *
 * @see DEVICE_MMIO_ROM_INIT()
 */
#define PLATFORM_ROM_INIT(node_id) DEVICE_MMIO_ROM_INIT(node_id),

#define Z_PLATFORM_DTS_DEV_INIT(n, init, isr)                                                      \
	Z_PLATFORM_ISR_INIT(n, isr);                                                               \
	Z_DEVICE_DT_INIT(n, init, isr);                                                            \
	static int Z_PLATFORM_DEV_INIT_NAME(n)(const struct device *dev)                           \
	{                                                                                          \
		return __common_dev_init##n(dev);                                                  \
	}
#endif

/**
 * @brief ACPI specific init function (Internal use only).
 *
 */
#define Z_PLATFORM_ACPI_DEV_INIT(n, init, isr)                                                     \
	static int __acpi_init_##n(const struct device *dev)                                       \
	{                                                                                          \
		return platform_acpi_init(dev, isr, DT_IRQN(n), DT_IRQ(n, priority),               \
					  Z_IRQ_FLAGS_GET(n), ACPI_DT_HID(n), ACPI_DT_UID(n));     \
	}                                                                                          \
                                                                                                   \
	static int Z_PLATFORM_DEV_INIT_NAME(n)(const struct device *dev)                           \
	{                                                                                          \
		int status;                                                                        \
                                                                                                   \
		status = __acpi_init_##n(dev);                                                     \
		if (!status) {                                                                     \
			status = init(dev);                                                        \
		}                                                                                  \
		return status;                                                                     \
	}

/**
 * @brief PCIe specific init function (Internal use only).
 *
 */
#define Z_PLATFORM_PCI_DEV_INIT(n, init, isr)                                                      \
	static int __pci_init_##n(const struct device *dev)                                        \
	{                                                                                          \
		struct pci_conf pci_cfg = {                                                        \
			.pcie = PLATFORM_PCI_CTX_GET(n),                                           \
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
	static int Z_PLATFORM_DEV_INIT_NAME(n)(const struct device *dev)                           \
	{                                                                                          \
		int status;                                                                        \
                                                                                                   \
		status = __pci_init_##n(dev);                                                      \
		if (!status) {                                                                     \
			status = init(dev);                                                        \
		}                                                                                  \
		return status;                                                                     \
	}

#define PLATFORM_ACPI_INIT_0(n, init, isr) Z_PLATFORM_DTS_DEV_INIT(n, init, isr)
#define PLATFORM_ACPI_INIT_1(n, init, isr) Z_PLATFORM_ACPI_DEV_INIT(n, init, isr)

#define Z_PLATFORM_ACPI_INIT(n, init, isr)                                                         \
	_CONCAT(PLATFORM_ACPI_INIT_, DT_NODE_HAS_PROP(n, acpi_uid))(n, init, isr)

#define PLATFORM_PCI_INIT_0(n, init, isr) Z_PLATFORM_ACPI_INIT(n, init, isr)
#define PLATFORM_PCI_INIT_1(n, init, isr) Z_PLATFORM_PCI_DEV_INIT(n, init, isr)

#define Z_PLATFORM_DEV_INIT(n, init, isr)                                                          \
	_CONCAT(PLATFORM_PCI_INIT_, DT_ON_BUS(n, pcie))(n, init, isr)

/**
 * @brief Create a device object from a devicetree node identifier and set it up
 * for boot time initialization. This macro is similar to DEVICE_DT_DEFINE but also provide
 * platform abstract interface for device drivers which are designed to work on multiple bus
 * topologies such as PCI, ACPI or just DTS base. Driver developers can use these macro instead
 * of DEVICE_DT_DEFINE in case they need to design drivers which works seamlessly on PCI bus,
 * ACPI and DTS base. Please note that MSI and multi vector interrupt support are still not
 * available. In case platform need different way of register ISR or no interrupt support then
 * they must define PLATFORM_DISABLE_DEV_INTR before include platform.h header. Please refer
 * DEVICE_DT_DEFINE documentation for details of all the parameters.
 *
 * @param node_id The devicetree node identifier.
 * @param init_fn Pointer to the device's initialization function, which will be
 * run by the kernel during system initialization. Can be `NULL`.
 * @param isr Pointer to the device's ISR function (optional).
 */
#define PLATFORM_DEV_DT_DEFINE(node_id, init_fn, isr, ...)                                         \
	Z_PLATFORM_DEV_INIT(node_id, init_fn, isr);                                                \
	DEVICE_DT_DEFINE(node_id, Z_PLATFORM_DEV_INIT_NAME(node_id), __VA_ARGS__)

#define PLATFORM_DEV_DT_INST_DEFINE(inst, init_fn, isr, ...)                                       \
	PLATFORM_DEV_DT_DEFINE(DT_DRV_INST(inst), init_fn, isr, __VA_ARGS__)
#endif

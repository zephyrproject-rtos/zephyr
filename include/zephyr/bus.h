/*
 * Copyright (c) 2024 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BUS_H_
#define ZEPHYR_INCLUDE_BUS_H_

#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/linker/sections.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>
#if defined(CONFIG_BUS_DEV)
#include <zephyr/sys/slist.h>
#endif /* CONFIG_BUS_DEV */

typedef void (*platform_isr_t)(const struct device *dev);

#if defined(CONFIG_PCIE)
#include <zephyr/bus/pcie_internal.h>
#endif

#if defined(CONFIG_ACPI)
#include <zephyr/bus/acpi_internal.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define Z_BUS_DEV_INIT_NAME(dev_id) _CONCAT(dev_init_, dev_id)

/**
 * @def BUS_DEV_MMIO_ROM
 *
 * @brief Declare storage for MMIO data within a device's config struct
 *
 * @see DEVICE_MMIO_ROM()
 */
#define BUS_DEV_MMIO_ROM DEVICE_MMIO_ROM

#if !DT_ANY_INST_ON_BUS_STATUS_OKAY(pcie)
#define BUS_DEV_MMIO_RAM DEVICE_MMIO_RAM
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(pcie) */

#ifndef BUS_DEV_RAM_INIT
#define BUS_DEV_RAM_INIT(node_id)
#endif

#define BUS_DEV_MMIO_RAM_PTR(dev) DEVICE_MMIO_RAM_PTR(dev)
#define BUS_DEV_MMIO_GET(dev)     DEVICE_MMIO_GET(dev)

#define Z_IS_IO_MAPPED(n) COND_CODE_1(DT_NODE_HAS_PROP(n, io_mapped), (1), (0))

#ifdef CONFIG_MMU
#define Z_BUS_DEV_MMIO_MAP(dev, phy_add, size)                                                    \
	device_map(BUS_DEV_MMIO_RAM_PTR(dev), phy_add, size, K_MEM_CACHE_NONE);
#else
#define Z_BUS_DEV_MMIO_MAP(dev, phy_add, size) *BUS_DEV_MMIO_RAM_PTR(dev) = (uintptr_t)phy_add
#endif

#define Z_IRQ_FLAGS_GET(n)  COND_CODE_1(DT_IRQ_HAS_CELL(n, sense), (DT_IRQ(n, sense)), (0))

#if !defined(BUS_DEV_DISABLE_INTR)
#define IRQ_CONFIG(n)                                                                            \
	IRQ_CONNECT(DT_IRQN(n), DT_IRQ(n, priority), __platform_isr_##n, DEVICE_DT_GET(n),         \
		    Z_IRQ_FLAGS_GET(n));
#define Z_IRQ_CONFIG(n) COND_CODE_1(DT_IRQ_HAS_IDX(n, 0), (IRQ_CONFIG(n)), ())

#define Z_IRQ_ENABLE(n) COND_CODE_1(DT_IRQ_HAS_IDX(n, 0), (irq_enable(DT_IRQN(n))), ())

#define Z_BUS_DEV_ISR_INIT(n, isr)                                                                \
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
#define Z_BUS_DEV_ISR_INIT(n, isr)
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
#define BUS_DEV_ROM_INIT(node_id)

#define Z_BUS_DEV_DTS_INIT(n, init, isr)                                                      \
	Z_BUS_DEV_ISR_INIT(n, isr);                                                               \
	Z_DEVICE_DT_INIT(n, init, isr);                                                            \
	static int Z_BUS_DEV_INIT_NAME(n)(const struct device *dev)                           \
	{                                                                                          \
		uintptr_t *reg = BUS_DEV_MMIO_RAM_PTR(dev);                                       \
                                                                                                   \
		if (!Z_IS_IO_MAPPED(n)) {                                                          \
			Z_BUS_DEV_MMIO_MAP(dev, DT_REG_ADDR(n), DT_REG_SIZE(n));                  \
		} else {                                                                           \
			*reg = DT_REG_ADDR(n);                                                     \
		}                                                                                  \
		return __common_dev_init##n(dev);                                                  \
	}
#else
/**
 * @def BUS_DEV_ROM_INIT
 *
 * @brief Initialize a DEVICE_MMIO_ROM member
 *
 * @param node_id DTS node identifier
 *
 * @see DEVICE_MMIO_ROM_INIT()
 */
#define BUS_DEV_ROM_INIT(node_id) DEVICE_MMIO_ROM_INIT(node_id),

#define Z_BUS_DEV_DTS_INIT(n, init, isr)                                                      \
	Z_BUS_DEV_ISR_INIT(n, isr);                                                               \
	Z_DEVICE_DT_INIT(n, init, isr);                                                            \
	static int Z_BUS_DEV_INIT_NAME(n)(const struct device *dev)                           \
	{                                                                                          \
		return __common_dev_init##n(dev);                                                  \
	}
#endif

#define BUS_DEV_ACPI_INIT(n, init, isr)                                                         \
	COND_CODE_1(DT_NODE_HAS_PROP(n, acpi_hid), (Z_BUS_DEV_ACPI_INIT(n, init, isr)), \
		(Z_BUS_DEV_DTS_INIT(n, init, isr)))

#define Z_BUS_DEV_INIT(n, init, isr)                                                          \
	COND_CODE_1(DT_ON_BUS(n, pcie), (Z_BUS_DEV_PCI_INIT(n, init, isr)),\
		(BUS_DEV_ACPI_INIT(n, init, isr)))

/**
 * @brief Create a device object from a devicetree node identifier and set it up
 * for boot time initialization. This macro is similar to DEVICE_DT_DEFINE but also provide
 * platform abstract interface for device drivers which are designed to work on multiple bus
 * topologies such as PCI, ACPI or just DTS base. Driver developers can use these macro instead
 * of DEVICE_DT_DEFINE in case they need to design drivers which works seamlessly on PCI bus,
 * ACPI and DTS base. Please note that MSI and multi vector interrupt support are still not
 * available. In case platform need different way of register ISR or no interrupt support then
 * they must define BUS_DEV_DISABLE_INTR before include platform.h header. Please refer
 * DEVICE_DT_DEFINE documentation for details of all the parameters.
 *
 * @param node_id The devicetree node identifier.
 * @param init_fn Pointer to the device's initialization function, which will be
 * run by the kernel during system initialization. Can be `NULL`.
 * @param isr Pointer to the device's ISR function (optional).
 */
#define BUS_DEV_DT_DEFINE(node_id, init_fn, isr, ...)                                         \
	Z_BUS_DEV_INIT(node_id, init_fn, isr);                                                \
	DEVICE_DT_DEFINE(node_id, Z_BUS_DEV_INIT_NAME(node_id), __VA_ARGS__)

#define BUS_DEV_DT_INST_DEFINE(inst, init_fn, isr, ...)                                       \
	BUS_DEV_DT_DEFINE(DT_DRV_INST(inst), init_fn, isr, __VA_ARGS__)
#endif /* ZEPHYR_INCLUDE_BUS_H_ */

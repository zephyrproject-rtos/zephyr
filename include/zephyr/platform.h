/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PLATFORM_H_
#define ZEPHYR_INCLUDE_PLATFORM_H_

#include <stdint.h>

#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/linker/sections.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/util.h>
#if defined(CONFIG_PLATFORM_ABST_SUPPORT)
#include <zephyr/sys/slist.h>
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(pcie)
BUILD_ASSERT(IS_ENABLED(CONFIG_PCIE), "DT need CONFIG_PCIE");
#include <zephyr/drivers/pcie/pcie.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Flags describe type of Device object and requried boot priority.
 *
 * These are various flag which describe boot priority requriements and type
 * of device object such as Root/sys devices or PCI device etc.
 */
#define BOOT_LEVEL_MASK	 (0x7) /* Mask for various boot option. */
#define BOOT_PRE_KERNEL	 (1 << 0) /* Device init will call before kernel init. */
#define BOOT_POST_KERNEL (1 << 1) /* Device init will call after kernel init. */
#define BOOT_APPLICATION (1 << 2) /* Device init will call after main thread invoked. */
#define BOOT_DTS_DEVICE (1 << 3) /* Root/sys device type which is not on any bus node.*/
#define BOOT_PCI_DEVICE	 (1 << 4) /* Device which is on PCI bus node.*/
#define DEV_FLAG_IOMAPPED (1 << 5) /*Device who's register address are IO mapped.*/
#define DEV_FLAG_BUS_ADD (1 << 6) /*Device who's address are bus specifc such as I2C devices*/

/**
 * @brief Get device boot priorty level.
 *
 * This will return device boot priorty level (notify_boot_level) based on device
 * object boot priorty flag.
 *
 * @param dev pointer to device object
 * @return notify_boot_level type
 */
#define BOOT_FLAG_LEVEL(dev)   ((dev->flag & BOOT_LEVEL_MASK) >> 1)

/**
 * @brief Check whether the device is defined using PLATFORM_DEV_DT_DEFINE or not.
 *
 * This macro will return true if device is new device object type
 *
 * @param dev pointer to device object
 * @return true if new device object type else false
 */
#define IS_PLATFORM_DEVICE(dev) (dev->flag & (BOOT_DTS_DEVICE | BOOT_PCI_DEVICE))

/**
 * @brief Check whether the device is IO mapped or or not.
 *
 * This macro will return true if device is IO mapped
 *
 * @param dev pointer to device object
 * @return true if IO mapped device else false
 */
#define IS_DEV_IOMAPPED(dev) ((dev->flag & DEV_FLAG_IOMAPPED) == DEV_FLAG_IOMAPPED)

/**
 * @brief Get memory mapped register based address.
 *
 * This will return mmio address of device's register base address.
 * If MMU enabled then this macro will return the virtual address else
 * will return physical address.
 *
 * @param dev pointer to device object
 * @return MMIO address of register base address
 */
#define PLATFORM_MMIO_GET(dev) dev->mmio->mem

/**
 * @brief Get IO mapped register based address.
 *
 * This will return IO/port address of device's register base address.
 *
 * @param dev pointer to device object
 * @return IO/port address of register base address
 */
#define PLATFORM_PORT_GET(dev) dev->mmio->port

/**
 * @brief Device's platform config info structure (in ROM) per driver instance
 */
struct platform_config {
	/* Hardware id for uniquely identify a device. */
	const char *hid;
	/* Instance id for uniquely identify a device instance. */
	uint8_t inst;
	/**
	 * Config function for the device.
	 *
	 * @param dev Device pointer.
	 *
	 * @retval 0 if success else system error code.
	 */
	int (*conf)(const struct device *dev);
	/**
	 * Isr function for the device.
	 *
	 * @param arg Device pointer, or any custom argu or can be NULL.
	 *
	 * @retval NIL
	 */
	void (*isr)(const void *arg);
};

/**
 * @brief device resource info
 *
 * Structure for passing down temp platform information from bus or dts to
 * various device and platform init functions.
 */
struct platform_resource {
	/* Hardware id for uniquely identify a device. */
	char *hw_id;
	/* Instance id for uniquely identify a device instance. */
	uint8_t inst;
	/* MMIO or port address. */
	uint32_t reg_size;
	union {
		uintptr_t reg_base_phy;
		uint32_t port;
	};
	/* io mapped vs memory mapped. */
	uint16_t io_mapped : 1;
	/* device IRQ number. */
	uint16_t irq_num : 8;
	/* device IRQ priority. */
	uint16_t irq_prio : 7;
	/* device IRQ flags. */
	uint32_t irq_flag;
};

/**
 * @brief Utility function internally used by framework for initialize device nodes.
 *
 * @param dev child node's device object.
 * @return NIL.
 */
void platform_node_init(const struct init_entry *entry);

/**
 * @brief Write 8bit value to a register
 *
 * @param dev the device object.
 * @param reg register offset.
 * @param val register value.
 * @return NIL.
 */
static inline void platform_write_reg_8(const struct device *dev, uint32_t reg, uint8_t val)
{
	if (IS_DEV_IOMAPPED(dev)) {
		sys_out8(val, PLATFORM_PORT_GET(dev) + reg);
	} else {
		sys_write8(val, PLATFORM_MMIO_GET(dev) + reg);
	}
}

/**
 * @brief Write 16bit value to a register
 *
 * @param dev the device object.
 * @param reg register offset.
 * @param val register value.
 * @return NIL.
 */
static inline void platform_write_reg_16(const struct device *dev, uint32_t reg, uint16_t val)
{
	if (IS_DEV_IOMAPPED(dev)) {
		sys_out16(val, PLATFORM_PORT_GET(dev) + reg);
	} else {
		sys_write16(val, PLATFORM_MMIO_GET(dev) + reg);
	}
}

/**
 * @brief Write 32bit value to a register
 *
 * @param dev the device object.
 * @param reg register offset.
 * @param val register value.
 * @return NIL.
 */
static inline void platform_write_reg_32(const struct device *dev, uint32_t reg, uint32_t val)
{
	if (IS_DEV_IOMAPPED(dev)) {
		sys_out32(val, PLATFORM_PORT_GET(dev) + reg);
	} else {
		sys_write32(val, PLATFORM_MMIO_GET(dev) + reg);
	}
}

/**
 * @brief Read 8bit value from a register
 *
 * @param dev the device object.
 * @param reg register offset.
 * @return register value.
 */
static inline uint8_t platform_read_reg_8(const struct device *dev, uint32_t reg)
{
	uint8_t val;

	if (IS_DEV_IOMAPPED(dev)) {
		val = sys_in8(PLATFORM_PORT_GET(dev) + reg);
	} else {
		val = sys_read8(PLATFORM_MMIO_GET(dev) + reg);
	}
	return val;
}

/**
 * @brief Read 16bit value from a register
 *
 * @param dev the device object.
 * @param reg register offset.
 * @return register value.
 */
static inline uint16_t platform_read_reg_16(const struct device *dev, uint32_t reg)
{
	uint16_t val;

	if (IS_DEV_IOMAPPED(dev)) {
		val = sys_in16(PLATFORM_PORT_GET(dev) + reg);
	} else {
		val = sys_read16(PLATFORM_MMIO_GET(dev) + reg);
	}
	return val;
}

/**
 * @brief Read 32bit value from a register
 *
 * @param dev the device object.
 * @param reg register offset.
 * @return register value.
 */
static inline uint32_t platform_read_reg_32(const struct device *dev, uint32_t reg)
{
	uint32_t val;

	if (IS_DEV_IOMAPPED(dev)) {
		val = sys_in32(PLATFORM_PORT_GET(dev) + reg);
	} else {
		val = sys_read32(PLATFORM_MMIO_GET(dev) + reg);
	}

	return val;
}

int platform_specific_dev_config(const struct device *dev, struct platform_resource *res);
int platform_specific_dev_update(const struct device *dev, struct platform_resource *res, int status);

#ifndef PLATFORM_LOG_DBG
#define PLATFORM_LOG_DBG(...)
#endif

#define DEV_FLAG_IOMAPPED_0   0
#define DEV_FLAG_IOMAPPED_1   DEV_FLAG_IOMAPPED
#define DEV_FLAG_IOMAP_GET(n) _CONCAT(DEV_FLAG_IOMAPPED_, DT_PROP_OR(n, io_mapped, 0))

#define DEV_FLAG_PCIE_0	     BOOT_DTS_DEVICE
#define DEV_FLAG_PCIE_1	     BOOT_PCI_DEVICE
#define DEV_FLAG_PCIE_GET(n) _CONCAT(DEV_FLAG_PCIE_, DT_ON_BUS(n, pcie))

/**
 * @brief Retrive info such as pci device, IO mapped etc.
 *
 */
#define DEV_FLAG(n) (DEV_FLAG_PCIE_GET(n) | DEV_FLAG_IOMAP_GET(n))

#define IRQ_FLAGS_SENSE0(n) 0
#define IRQ_FLAGS_SENSE1(n) DT_IRQ(n, sense)
#define IRQ_FLAGS_GET(n)    _CONCAT(IRQ_FLAGS_SENSE, DT_IRQ_HAS_CELL(n, sense))(n)

#define INIT_IRQ_INFO0(n)
#define INIT_IRQ_INFO1(n)                                                                          \
	.irq_num = DT_IRQ(n, irq), .irq_prio = DT_IRQ(n, priority), .irq_flag = IRQ_FLAGS_GET(n),

/**
 * @brief Retrive IRQ info such as irq number, priortiy and interrupt flags.
 *
 */
#define DEV_IRQ_INIT(n) _CONCAT(INIT_IRQ_INFO, DT_PROP_HAS_IDX(n, interrupts, 0))(n)

#define IRQ_REGISTER0(n)
#define IRQ_REGISTER1(n)                                                                           \
	IRQ_CONNECT(DT_IRQN(n), DT_IRQ(n, priority), platform_cfg->isr, DEVICE_DT_GET(n),               \
		    IRQ_FLAGS_GET(n));

/**
 * @brief Register ISR with platform's interrupt subsystem.
 *
 */
#define DEV_IRQ_REGISTER(n) _CONCAT(IRQ_REGISTER, DT_PROP_HAS_IDX(n, interrupts, 0))(n)

#define IRQ_ENABLE_0(n)
#define IRQ_ENABLE_1(n)                                                                           \
	irq_enable(DT_IRQN(n))

/**
 * @brief Enable device interrupt.
 *
 */
#define DEV_IRQ_ENABLE(n) _CONCAT(IRQ_ENABLE_, DT_PROP_HAS_IDX(n, interrupts, 0))(n)


#define DT_GET_REG_SIZE0(n) 0
#define DT_GET_REG_SIZE1(n) DT_REG_SIZE(n)

#define DT_REG_HAS_SIZE(node_id, idx) \
	IS_ENABLED(DT_CAT4(node_id, _REG_IDX_, idx, _VAL_SIZE))

#define GET_REG_SIZE_OR(n) _CONCAT(DT_GET_REG_SIZE, DT_REG_HAS_SIZE(n, 0))(n)

#define DT_GET_REG_ADDR0(n) 0
#define DT_GET_REG_ADDR1(n) .reg_base_phy = DT_REG_ADDR(n), .reg_size = GET_REG_SIZE_OR(n)

#define GET_REG_ADD_OR(n) _CONCAT(DT_GET_REG_ADDR, DT_PROP_HAS_IDX(n, reg, 0))(n)

#define DT_GET_MMIO_ADDR0(n) GET_REG_ADD_OR(n)
#define DT_GET_MMIO_ADDR1(n) .port = DT_REG_ADDR(n), .reg_size = GET_REG_SIZE_OR(n)

/**
 * @brief Retrive MMIO address.
 *
 */
#define GET_MMIO_ADD_OR(n) _CONCAT(DT_GET_MMIO_ADDR, DT_PROP_HAS_IDX(n, io_mapped, 0))(n)

/**
 * @brief Retrive platform specific information from DTS (Internal use only).
 *
 */
#define PLATFORM_GENERIC_CONFIG_DECLARE(n, isr_)                                                   \
	static const struct platform_config platform_cfg_##n = {                                          \
		.conf = platform_config_fn_##n,                                                      \
		.hid = DT_PROP_OR(n, hardware_id, 0),                                              \
		.isr = isr_,                                                                       \
	}


/**
 * @brief Retrive platform specific information from DTS for PCIe devices (Internal use only).
 *
 */
#define PLATFORM_PCIE_CONFIG_DECLARE(n, isr_)                                                      \
	DEVICE_PCIE_DECLARE(n);                                                                    \
	static const struct pcie_device_config platform_cfg_##n = {                                     \
		.conf = platform_config_fn_##n,                                                      \
		DEVICE_PCIE_INIT(n, pcie),                                                         \
		.isr = isr_,                                                                       \
	}

/**
 * @brief Platform specific configurations (Internal use only).
 *
 */
#define PLATFORM_SPECIFIC_DEV_CONFIG(n)                                                            \
	static int platform_config_fn_##n(const struct device *dev)                                  \
	{                                                                                          \
		struct platform_resource res = {                                                          \
			GET_MMIO_ADD_OR(n), .hw_id = DT_PROP_OR(n, hardware_id, 0),                \
			.io_mapped = DT_PROP_OR(n, io_mapped, 0), DEV_IRQ_INIT(n)};                \
		return platform_specific_dev_config(dev, &res);                                    \
	}

/**
 * @brief Platform specific configuration update (Internal use only).
 *
 */
#define PLATFORM_SPECIFIC_DEV_UPDATE(n)                                                            \
	static int platform_update_device_##n(const struct device *dev, int status)                    \
	{                                                                                          \
		struct platform_resource res = {                                                          \
			GET_MMIO_ADD_OR(n), .hw_id = DT_PROP_OR(n, hardware_id, 0),                \
			.io_mapped = DT_PROP_OR(n, io_mapped, 0), DEV_IRQ_INIT(n)};                \
                                                                                                   \
		return platform_specific_dev_update(dev, &res, status);                            \
	}

/**
 * @brief PCIe specific configuration update (Internal use only).
 *
 */
#define PLATFORM_PCIE_DEV_UPDATE(n)                                                                \
	int pcie_dev_update(const struct device *dev,\
				struct pci_platform_conf *pci_cfg, int status);\
	static int platform_update_device_##n(const struct device *dev, int status)                    \
	{                                                                                          \
		struct pci_platform_conf pci_cfg = {                                                 \
			.pci_id = PCIE_DT_ID(n),                                                   \
			.io_mapped = DT_PROP_OR(n, io_mapped, 0),                                  \
			.bus_master = DT_PROP_OR(n, bus_master, 1),                                \
			.irq_num = DT_IRQN(n),                                                     \
			.irq_prio = DT_IRQ(n, priority),                                           \
			.irq_flag = IRQ_FLAGS_GET(n),                                              \
		};                                                                                 \
		return pcie_dev_update(dev, &pci_cfg, status);                                     \
	}

/**
 * @brief PCIe specific configurations (Internal use only).
 *
 */
#define PLATFORM_PCIE_DEV_CONFIG(n)                                                                \
	int pcie_dev_config(const struct device *dev, struct pci_platform_conf *pci_cfg);            \
	static int platform_config_fn_##n(const struct device *dev)                                  \
	{                                                                                          \
		PLATFORM_LOG_DBG("%s\n", __func__);                                                \
		struct pci_platform_conf pci_cfg = {                                                 \
			.pci_id = PCIE_DT_ID(n),                                                   \
			.io_mapped = DT_PROP_OR(n, io_mapped, 0),                                  \
			.bus_master = DT_PROP_OR(n, bus_master, 1),                                \
			.irq_num = DT_IRQN(n),                                                     \
			.irq_prio = DT_IRQ(n, priority),                                           \
			.irq_flag = IRQ_FLAGS_GET(n),                                              \
		};                                                                                 \
		return pcie_dev_config(dev, &pci_cfg);                                             \
	}

/**
 * @brief Platform general configurations (Internal use only).
 *
 */
#define PLATFORM_GENERIC_DEV_CONFIG(n)                                                             \
	static int platform_config_fn_##n(const struct device *dev)                                  \
	{                                                                                          \
		const struct platform_config *platform_cfg = dev->platform_cfg;                                \
                                                                                                   \
		PLATFORM_LOG_DBG("%s\n", __func__);                                                \
                                                                                                   \
		if (DT_PROP_OR(n, io_mapped, 0)) {                                                 \
			dev->mmio->port = DT_REG_ADDR(n);                                          \
			PLATFORM_LOG_DBG("IO Mapped:%x\n", dev->mmio->port);                       \
		} else {                                                                           \
			dev->mmio->mem = DT_REG_ADDR(n);                                           \
			PLATFORM_LOG_DBG("memory mapped: %p\n", (void *)dev->mmio->mem);                   \
		}                                                                                  \
                                                                                                   \
		if (platform_cfg->isr) {                                                                \
			DEV_IRQ_REGISTER(n);                                                       \
		}                                                                                  \
		return 0;                                                                          \
	}

/**
 * @brief Platform general update (Internal use only).
 *
 */
#define PLATFORM_GENERIC_DEV_UPDATE(n)                                                             \
	static int platform_update_device_##n(const struct device *dev, int status)                    \
	{                                                                                          \
		const struct platform_config *platform_cfg = dev->platform_cfg;                                \
		if (status != 0) {\
			if (status < 0) {\
				status = -status;\
			} \
			if (status > UINT8_MAX) {\
				status = UINT8_MAX;\
			} \
			dev->state->init_res = status;\
		} else if (platform_cfg->isr) {\
			PLATFORM_LOG_DBG("enable irq\n");\
			DEV_IRQ_ENABLE(n);\
		} \
			\
		dev->state->initialized = true;\
		return 0;\
	}

#define PLATFORM_DEV_CONFIG_DECLARE_1(n, _isr) PLATFORM_PCIE_CONFIG_DECLARE(n, _isr)
#define PLATFORM_DEV_CONFIG_DECLARE_0(n, _isr) PLATFORM_GENERIC_CONFIG_DECLARE(n, _isr)

#define PLATFORM_DEV_CONFIG_DECLARE(n, _isr)                                                       \
	_CONCAT(PLATFORM_DEV_CONFIG_DECLARE_, DT_ON_BUS(n, pcie))(n, _isr)

#ifdef CONFIG_PLATFORM_SPECIFIC_DEV_INIT
#define PLATFORM_DEV_CONFIG_0(n) PLATFORM_SPECIFIC_DEV_CONFIG(n)
#define PLATFORM_DEV_UPDATE_0(n) PLATFORM_SPECIFIC_DEV_UPDATE(n)
#else
#define PLATFORM_DEV_CONFIG_0(n) PLATFORM_GENERIC_DEV_CONFIG(n)
#define PLATFORM_DEV_UPDATE_0(n) PLATFORM_GENERIC_DEV_UPDATE(n)
#endif

#define PLATFORM_DEV_CONFIG_1(n) PLATFORM_PCIE_DEV_CONFIG(n)
#define PLATFORM_DEV_UPDATE_1(n) PLATFORM_PCIE_DEV_UPDATE(n)

/**
 * @brief Platform Configuration macro (Internal use only).
 *
 */
#define PLATFORM_DEV_CONFIG(n) _CONCAT(PLATFORM_DEV_CONFIG_, DT_ON_BUS(n, pcie))(n)

/**
 * @brief Platform update macro (Internal use only).
 *
 */
#define PLATFORM_DEV_UPDATE(n) _CONCAT(PLATFORM_DEV_UPDATE_, DT_ON_BUS(n, pcie))(n)

/**
 * @brief Platform generic init function (Internal use only).
 *
 */
#define PLATFORM_GENERIC_DEV_INIT(n, init)                                                            \
	static int platform_dev_init_##n(const struct device *dev)                                     \
	{                                                                                          \
		int status;                                                                        \
		const struct platform_config *platform_cfg = dev->platform_cfg;                                \
                                                                                                   \
		PLATFORM_LOG_DBG("%s\n", __func__);                                      \
		status = platform_cfg->conf(dev);                                                       \
		if (status) {                                                                      \
			PLATFORM_LOG_DBG("device config failed:%s\n", platform_cfg->hid);               \
			goto exit;                                                                 \
		}                                                                                  \
			status = init(dev);                                                                \
exit:                                                                                      \
		platform_update_device_##n(dev, status);                                               \
		return status;                                                                     \
	}

/**
 * @brief PCIe specific init function (Internal use only).
 *
 */
#define PLATFORM_PCI_DEV_INIT(n, init)                                                                \
	static int platform_dev_init_##n(const struct device *dev)                                     \
	{                                                                                          \
		int status;                                                                        \
		const struct pcie_device_config *platform_cfg = dev->platform_cfg;                           \
                                                                                                   \
		PLATFORM_LOG_DBG("%x\n", platform_cfg->pcie->id);                    \
		status = platform_cfg->conf(dev);                                                       \
		if (status) {                                                                      \
			PLATFORM_LOG_DBG("device config failed:%x\n", platform_cfg->pcie->id);          \
			goto exit;                                                                 \
		}                                                                                  \
			status = init(dev);                                                                \
exit:                                                                                      \
		platform_update_device_##n(dev, status);                                               \
		return status;                                                                     \
	}

#define PLATFORM_DEV_INIT_0(n, init) PLATFORM_GENERIC_DEV_INIT(n, init)
#define PLATFORM_DEV_INIT_1(n, init) PLATFORM_PCI_DEV_INIT(n, init)

#define PLATFORM_DEV_INIT(n, init) _CONCAT(PLATFORM_DEV_INIT_, DT_ON_BUS(n, pcie))(n, init)

#define DECL_DEV_NODE(n)                                                                           \
	struct platform_resource res_##n = {GET_MMIO_ADD_OR(n),\
					.hw_id = DT_PROP_OR(n, hardware_id, 0),   \
				    .io_mapped = DT_PROP_OR(n, io_mapped, 0), DEV_IRQ_INIT(n)};


#define PROB_CHILD_NODE(n) platform_node_init(DEVICE_INIT_DT_GET(n))

/**
 * @brief Enumerate child devices for the given node
 *
 * Each device is placed in a custom section and Root/sys or any bus driver can
 * enumerate all their child node using this macro during boot time.
 *
 * @node_id node lable of bus driver or "root/sys" label if it is in the Root/sys node.
 *
 */
#define DEVICE_PROB_CHILD_NODE(node_id)                                               \
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(DT_NODELABEL(node_id), PROB_CHILD_NODE, (;))

/**
 * @brief Like PLATFORM_DEV_DT_DEFINE(), but uses an instance of a `DT_DRV_COMPAT`
 * compatible instead of a node identifier.
 *
 * @param inst Instance number. The `node_id` argument to PLATFORM_DEV_DT_DEFINE() is
 * set to `DT_DRV_INST(inst)`.
 * @param ... Other parameters as expected by PLATFORM_DEV_DT_DEFINE().
 */
#define PLATFORM_DEV_DT_INST_DEFINE(inst, ...)                                                     \
	PLATFORM_DEV_DT_DEFINE(DT_DRV_INST(inst), __VA_ARGS__)

/**
 * @brief Create a device object from a devicetree node identifier and set it up
 * for boot time initialization.
 *
 * This macro defines a @ref device that is automatically configured by the
 * kernel during system initialization. The global device object's name as a C
 * identifier is derived from the node's dependency ordinal. @ref device.name is
 * set to `DEVICE_DT_NAME(node_id)`.
 *
 * The device is declared with extern visibility, so a pointer to a global
 * device object can be obtained with `DEVICE_DT_GET(node_id)` from any source
 * file that includes `<zephyr/device.h>`. Before using the pointer, the
 * referenced object should be checked using device_is_ready().
 *
 * @param node_id The devicetree node identifier.
 * @param flag describe type of Device object and requried boot priority.
 * @param defaut_cfg The default device conifg such as baud rate, clk freq etc.
 * @param init_fn Pointer to the device's initialization function, which will be
 * run by the kernel during system initialization.
 * @param pm Pointer to the device's power management resources, a
 * @ref pm_device, which will be stored in @ref device.pm. Use `NULL` if the
 * device does not use PM.
 * @param data Pointer to the device's private mutable data, which will be
 * stored in @ref device.data.
 * @param config Pointer to the device's private constant data, which will be
 * stored in @ref device.config field.
 * @param api Pointer to the device's API structure. Can be `NULL`.
 * @param isr Pointer to the device's ISR function. Can be `NULL`.
 */
#define PLATFORM_DEV_DT_DEFINE(node_id, flag, defaut_cfg, init_fn, pm, data, config, api, isr,     \
			       ...)                                                                \
	Z_DEVICE_STATE_DEFINE(Z_DEVICE_DT_DEV_ID(node_id));                                        \
	Z_PLATFORM_DEV_DEFINE(node_id, Z_DEVICE_DT_DEV_ID(node_id), DEVICE_DT_NAME(node_id),       \
			      init_fn, pm, data, config, api, flag, defaut_cfg, isr,               \
			      &Z_DEVICE_STATE_NAME(Z_DEVICE_DT_DEV_ID(node_id)), __VA_ARGS__)

/**
 * @brief Initializer (V2) for @ref device.
 *
 * @param name_ Name of the device.
 * @param pm_ Reference to @ref pm_device (optional).
 * @param data_ Reference to device data.
 * @param config_ Reference to device config.
 * @param api_ Reference to device API ops.
 * @param state_ Reference to device state.
 * @param handles_ Reference to device handles.
 * @param res_ Device resource such as mmio address.
 * @param platform_cfg_ platform specific configuration info.
 * @param default_cfg_ The default device conifg such as baud rate, clk freq etc.
 * @param flag_ Flags describe type of Device object and requried boot priority.
 */
#define Z_PLATFORM_INIT(name_, pm_, data_, config_, api_, state_, handles_, res_, platform_cfg_,       \
			 default_cfg_, flag_)                                                      \
	{                                                                                          \
		.name = name_, .data = (data_), .default_cfg = default_cfg_, .config = (config_),  \
		.api = (api_), .state = (state_), .handles = (handles_),                           \
		IF_ENABLED(CONFIG_PM_DEVICE, (.pm = (pm_),)).mmio = res_, .platform_cfg = platform_cfg_,    \
		.flag = flag_,                                                                     \
	}

/**
 * @brief Define a @ref device
 *
 * @param node_id Devicetree node id for the device (DT_INVALID_NODE if a
 * software device).
 * @param dev_id Device identifier (used to name the defined @ref device).
 * @param name Name of the device.
 * @param pm Reference to @ref pm_device associated with the device.
 * (optional).
 * @param data Reference to device data.
 * @param config Reference to device config.
 * @param level Initialization level.
 * @param prio Initialization priority.
 * @param api Reference to device API.
 * @param ... Optional dependencies, manually specified.
 */

#define Z_PLATFORM_DEV_DEFINE(node_id, dev_id, name, init_fn, pm, data, config, api, flag,         \
			      default_cfg, isr_, state, ...)                                       \
	Z_DEVICE_NAME_CHECK(name);                                                                 \
	static union mmio_info dev_mmio_##node_id;                                                \
                                                                                                   \
	PLATFORM_DEV_CONFIG(node_id);                                                              \
	PLATFORM_DEV_CONFIG_DECLARE(node_id, isr_);                                                \
	PLATFORM_DEV_UPDATE(node_id);                                                              \
	PLATFORM_DEV_INIT(node_id, init_fn);                                                       \
                                                                                                   \
	Z_DEVICE_HANDLES_DEFINE(node_id, dev_id, __VA_ARGS__);                                     \
                                                                                                   \
	COND_CODE_1(DT_NODE_EXISTS(node_id), (), (static))                                         \
	const Z_DECL_ALIGN(struct device) DEVICE_NAME_GET(dev_id)                                  \
		Z_DEVICE_SECTION(PRE_KERNEL_2, 0) __used =                                    \
			Z_PLATFORM_INIT(name, pm, data, config, api, state,                       \
					 Z_DEVICE_HANDLES_NAME(dev_id), &dev_mmio_##node_id,       \
					 &platform_cfg_##node_id, default_cfg,                          \
					 flag | DEV_FLAG(node_id));                                \
                                                                                                   \
	Z_DEVICE_INIT_ENTRY_DEFINE(dev_id, platform_dev_init_##node_id, PRE_KERNEL_2, 0)

#endif

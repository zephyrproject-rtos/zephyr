/**
 * @file
 *
 * @brief Public APIs for the PCIe Controllers drivers.
 */

/*
 * Copyright (c) 2021 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_PCIE_CONTROLLERS_H_
#define ZEPHYR_INCLUDE_DRIVERS_PCIE_CONTROLLERS_H_

#include <zephyr/types.h>
#include <zephyr/device.h>

#ifdef CONFIG_PCIE_MSI
#include <zephyr/drivers/pcie/msi.h>
#endif

/**
 * @brief PCI Express Controller Interface
 * @defgroup pcie_controller_interface PCI Express Controller Interface
 * @ingroup io_interfaces
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Function called to read a 32-bit word from an endpoint's configuration space.
 *
 * Read a 32-bit word from an endpoint's configuration space with the PCI Express Controller
 * configuration space access method (I/O port, memory mapped or custom method)
 *
 * @param dev PCI Express Controller device pointer
 * @param bdf PCI(e) endpoint
 * @param reg the configuration word index (not address)
 * @return the word read (0xFFFFFFFFU if nonexistent endpoint or word)
 */
typedef uint32_t (*pcie_ctrl_conf_read_t)(const struct device *dev, pcie_bdf_t bdf,
					  unsigned int reg);

/**
 * @brief Function called to write a 32-bit word to an endpoint's configuration space.
 *
 * Write a 32-bit word to an endpoint's configuration space with the PCI Express Controller
 * configuration space access method (I/O port, memory mapped or custom method)
 *
 * @param dev PCI Express Controller device pointer
 * @param bdf PCI(e) endpoint
 * @param reg the configuration word index (not address)
 * @param data the value to write
 */
typedef void (*pcie_ctrl_conf_write_t)(const struct device *dev, pcie_bdf_t bdf,
					  unsigned int reg, uint32_t data);

/**
 * @brief Function called to allocate a memory region subset for an endpoint Base Address Register.
 *
 * When enumerating PCIe Endpoints, Type0 endpoints can require up to 6 memory zones
 * via the Base Address Registers from I/O or Memory types.
 *
 * This call allocates such zone in the PCI Express Controller memory regions if
 * such region is available and space is still available.
 *
 * @param dev PCI Express Controller device pointer
 * @param bdf PCI(e) endpoint
 * @param mem True if the BAR is of memory type
 * @param mem64 True if the BAR is of 64bit memory type
 * @param bar_size Size in bytes of the Base Address Register as returned by HW
 * @param bar_bus_addr bus-centric address allocated to be written in the BAR register
 * @return True if allocation was possible, False if allocation failed
 */
typedef bool (*pcie_ctrl_region_allocate_t)(const struct device *dev, pcie_bdf_t bdf,
					    bool mem, bool mem64, size_t bar_size,
					    uintptr_t *bar_bus_addr);

/**
 * @brief Function called to get the current allocation base of a memory region subset
 * for an endpoint Base Address Register.
 *
 * When enumerating PCIe Endpoints, Type1 bridge endpoints requires a range of memory
 * allocated by all endpoints in the bridged bus.
 *
 * @param dev PCI Express Controller device pointer
 * @param bdf PCI(e) endpoint
 * @param mem True if the BAR is of memory type
 * @param mem64 True if the BAR is of 64bit memory type
 * @param align size to take in account for alignment
 * @param bar_base_addr bus-centric address allocation base
 * @return True if allocation was possible, False if allocation failed
 */
typedef bool (*pcie_ctrl_region_get_allocate_base_t)(const struct device *dev, pcie_bdf_t bdf,
						     bool mem, bool mem64, size_t align,
						     uintptr_t *bar_base_addr);

/**
 * @brief Function called to translate an endpoint Base Address Register bus-centric address
 * into Physical address.
 *
 * When enumerating PCIe Endpoints, Type0 endpoints can require up to 6 memory zones
 * via the Base Address Registers from I/O or Memory types.
 *
 * The bus-centric address set in this BAR register is not necessarily accessible from the CPU,
 * thus must be translated by using the PCI Express Controller memory regions translation
 * ranges to permit mapping from the CPU.
 *
 * @param dev PCI Express Controller device pointer
 * @param bdf PCI(e) endpoint
 * @param mem True if the BAR is of memory type
 * @param mem64 True if the BAR is of 64bit memory type
 * @param bar_bus_addr bus-centric address written in the BAR register
 * @param bar_addr CPU-centric address translated from the bus-centric address
 * @return True if translation was possible, False if translation failed
 */
typedef bool (*pcie_ctrl_region_translate_t)(const struct device *dev, pcie_bdf_t bdf,
					     bool mem, bool mem64, uintptr_t bar_bus_addr,
					     uintptr_t *bar_addr);

#ifdef CONFIG_PCIE_MSI
typedef uint8_t (*pcie_ctrl_msi_device_setup_t)(const struct device *dev, unsigned int priority,
						msi_vector_t *vectors, uint8_t n_vector);
#endif

/**
 * @brief Read a 32-bit word from a Memory-Mapped endpoint's configuration space.
 *
 * Read a 32-bit word from an endpoint's configuration space from a Memory-Mapped
 * configuration space access method, known as PCI Control Access Method (CAM) or
 * PCIe Extended Control Access Method (ECAM).
 *
 * @param cfg_addr Logical address of Memory-Mapped configuration space
 * @param bdf PCI(e) endpoint
 * @param reg the configuration word index (not address)
 * @return the word read (0xFFFFFFFFU if nonexistent endpoint or word)
 */
uint32_t pcie_generic_ctrl_conf_read(mm_reg_t cfg_addr, pcie_bdf_t bdf, unsigned int reg);


/**
 * @brief Write a 32-bit word to a Memory-Mapped endpoint's configuration space.
 *
 * Write a 32-bit word to an endpoint's configuration space from a Memory-Mapped
 * configuration space access method, known as PCI Control Access Method (CAM) or
 * PCIe Extended Control Access Method (ECAM).
 *
 * @param cfg_addr Logical address of Memory-Mapped configuration space
 * @param bdf PCI(e) endpoint
 * @param reg the configuration word index (not address)
 * @param data the value to write
 */
void pcie_generic_ctrl_conf_write(mm_reg_t cfg_addr, pcie_bdf_t bdf,
				  unsigned int reg, uint32_t data);

/**
 * @brief Start PCIe Endpoints enumeration.
 *
 * Start a PCIe Endpoints enumeration from a Bus number.
 * When on non-x86 architecture or when firmware didn't setup the PCIe Bus hierarchy,
 * the PCIe bus complex must be enumerated to setup the Endpoints Base Address Registers.
 *
 * @param dev PCI Express Controller device pointer
 * @param bdf_start PCI(e) start endpoint (only bus & dev are used to start enumeration)
 */
void pcie_generic_ctrl_enumerate(const struct device *dev, pcie_bdf_t bdf_start);

/** @brief Structure providing callbacks to be implemented for devices
 * that supports the PCI Express Controller API
 */
__subsystem struct pcie_ctrl_driver_api {
	pcie_ctrl_conf_read_t conf_read;
	pcie_ctrl_conf_write_t conf_write;
	pcie_ctrl_region_allocate_t region_allocate;
	pcie_ctrl_region_get_allocate_base_t region_get_allocate_base;
	pcie_ctrl_region_translate_t region_translate;
#ifdef CONFIG_PCIE_MSI
	pcie_ctrl_msi_device_setup_t msi_device_setup;
#endif
};

/**
 * @brief Read a 32-bit word from an endpoint's configuration space.
 *
 * Read a 32-bit word from an endpoint's configuration space with the PCI Express Controller
 * configuration space access method (I/O port, memory mapped or custom method)
 *
 * @param dev PCI Express Controller device pointer
 * @param bdf PCI(e) endpoint
 * @param reg the configuration word index (not address)
 * @return the word read (0xFFFFFFFFU if nonexistent endpoint or word)
 */
static inline uint32_t pcie_ctrl_conf_read(const struct device *dev, pcie_bdf_t bdf,
					   unsigned int reg)
{
	const struct pcie_ctrl_driver_api *api =
		(const struct pcie_ctrl_driver_api *)dev->api;

	return api->conf_read(dev, bdf, reg);
}

/**
 * @brief Write a 32-bit word to an endpoint's configuration space.
 *
 * Write a 32-bit word to an endpoint's configuration space with the PCI Express Controller
 * configuration space access method (I/O port, memory mapped or custom method)
 *
 * @param dev PCI Express Controller device pointer
 * @param bdf PCI(e) endpoint
 * @param reg the configuration word index (not address)
 * @param data the value to write
 */
static inline void pcie_ctrl_conf_write(const struct device *dev, pcie_bdf_t bdf,
					unsigned int reg, uint32_t data)
{
	const struct pcie_ctrl_driver_api *api =
		(const struct pcie_ctrl_driver_api *)dev->api;

	api->conf_write(dev, bdf, reg, data);
}

/**
 * @brief Allocate a memory region subset for an endpoint Base Address Register.
 *
 * When enumerating PCIe Endpoints, Type0 endpoints can require up to 6 memory zones
 * via the Base Address Registers from I/O or Memory types.
 *
 * This call allocates such zone in the PCI Express Controller memory regions if
 * such region is available and space is still available.
 *
 * @param dev PCI Express Controller device pointer
 * @param bdf PCI(e) endpoint
 * @param mem True if the BAR is of memory type
 * @param mem64 True if the BAR is of 64bit memory type
 * @param bar_size Size in bytes of the Base Address Register as returned by HW
 * @param bar_bus_addr bus-centric address allocated to be written in the BAR register
 * @return True if allocation was possible, False if allocation failed
 */
static inline bool pcie_ctrl_region_allocate(const struct device *dev, pcie_bdf_t bdf,
					     bool mem, bool mem64, size_t bar_size,
					     uintptr_t *bar_bus_addr)
{
	const struct pcie_ctrl_driver_api *api =
		(const struct pcie_ctrl_driver_api *)dev->api;

	return api->region_allocate(dev, bdf, mem, mem64, bar_size, bar_bus_addr);
}

/**
 * @brief Function called to get the current allocation base of a memory region subset
 * for an endpoint Base Address Register.
 *
 * When enumerating PCIe Endpoints, Type1 bridge endpoints requires a range of memory
 * allocated by all endpoints in the bridged bus.
 *
 * @param dev PCI Express Controller device pointer
 * @param bdf PCI(e) endpoint
 * @param mem True if the BAR is of memory type
 * @param mem64 True if the BAR is of 64bit memory type
 * @param align size to take in account for alignment
 * @param bar_base_addr bus-centric address allocation base
 * @return True if allocation was possible, False if allocation failed
 */
static inline bool pcie_ctrl_region_get_allocate_base(const struct device *dev, pcie_bdf_t bdf,
						      bool mem, bool mem64, size_t align,
						      uintptr_t *bar_base_addr)
{
	const struct pcie_ctrl_driver_api *api =
		(const struct pcie_ctrl_driver_api *)dev->api;

	return api->region_get_allocate_base(dev, bdf, mem, mem64, align, bar_base_addr);
}

/**
 * @brief Translate an endpoint Base Address Register bus-centric address into Physical address.
 *
 * When enumerating PCIe Endpoints, Type0 endpoints can require up to 6 memory zones
 * via the Base Address Registers from I/O or Memory types.
 *
 * The bus-centric address set in this BAR register is not necessarily accessible from the CPU,
 * thus must be translated by using the PCI Express Controller memory regions translation
 * ranges to permit mapping from the CPU.
 *
 * @param dev PCI Express Controller device pointer
 * @param bdf PCI(e) endpoint
 * @param mem True if the BAR is of memory type
 * @param mem64 True if the BAR is of 64bit memory type
 * @param bar_bus_addr bus-centric address written in the BAR register
 * @param bar_addr CPU-centric address translated from the bus-centric address
 * @return True if translation was possible, False if translation failed
 */
static inline bool pcie_ctrl_region_translate(const struct device *dev, pcie_bdf_t bdf,
					  bool mem, bool mem64, uintptr_t bar_bus_addr,
					  uintptr_t *bar_addr)
{
	const struct pcie_ctrl_driver_api *api =
		(const struct pcie_ctrl_driver_api *)dev->api;

	if (!api->region_translate) {
		*bar_addr = bar_bus_addr;
		return true;
	} else {
		return api->region_translate(dev, bdf, mem, mem64, bar_bus_addr, bar_addr);
	}
}

#ifdef CONFIG_PCIE_MSI
static inline uint8_t pcie_ctrl_msi_device_setup(const struct device *dev, unsigned int priority,
						 msi_vector_t *vectors, uint8_t n_vector)
{
	const struct pcie_ctrl_driver_api *api =
		(const struct pcie_ctrl_driver_api *)dev->api;

	return api->msi_device_setup(dev, priority, vectors, n_vector);
}
#endif

/** @brief Structure describing a device that supports the PCI Express Controller API
 */
struct pcie_ctrl_config {
#ifdef CONFIG_PCIE_MSI
	const struct device *msi_parent;
#endif
	/* Configuration space physical address */
	uintptr_t cfg_addr;
	/* Configuration space physical size */
	size_t cfg_size;
	/* BAR regions translation ranges count */
	size_t ranges_count;
	/* BAR regions translation ranges table */
	struct {
		/* Flags as defined in the PCI Bus Binding to IEEE Std 1275-1994 */
		uint32_t flags;
		/* bus-centric offset from the start of the region */
		uintptr_t pcie_bus_addr;
		/* CPU-centric offset from the start of the region */
		uintptr_t host_map_addr;
		/* region size */
		size_t map_length;
	} ranges[];
};

/*
 * Fills the pcie_ctrl_config.ranges table from DT
 */
#define PCIE_RANGE_FORMAT(node_id, idx)							\
{											\
	.flags = DT_RANGES_CHILD_BUS_FLAGS_BY_IDX(node_id, idx),			\
	.pcie_bus_addr = DT_RANGES_CHILD_BUS_ADDRESS_BY_IDX(node_id, idx),		\
	.host_map_addr = DT_RANGES_PARENT_BUS_ADDRESS_BY_IDX(node_id, idx),		\
	.map_length = DT_RANGES_LENGTH_BY_IDX(node_id, idx),				\
},

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_PCIE_CONTROLLERS_H_ */

/*
 * Copyright (c) 2026 Process Mission
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Internal interface of the DesignWare PCIe host controller support
 *
 * The DesignWare PCIe glue drivers use this code for configuration space
 * access, iATU outbound window programming and BAR allocation.
 */

#ifndef ZEPHYR_DRIVERS_PCIE_CONTROLLER_PCIE_DW_COMMON_H_
#define ZEPHYR_DRIVERS_PCIE_CONTROLLER_PCIE_DW_COMMON_H_

#include <zephyr/drivers/pcie/controller.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/sys_io.h>

/** PCI address space regions handled by the outbound iATU windows. */
enum pcie_dw_region_type {
	/** I/O space range. */
	PCIE_DW_REGION_IO,
	/** 32-bit memory space range. */
	PCIE_DW_REGION_MEM,
	/** 64-bit memory space range. */
	PCIE_DW_REGION_MEM64,
	PCIE_DW_REGION_COUNT,
};

/** CPU to PCI translation range of one outbound iATU window. */
struct pcie_dw_region {
	/** CPU address of the first byte of the window. */
	uintptr_t phys_start;
	/** PCI bus address of the first byte of the window. */
	uintptr_t bus_start;
	/** Window size in bytes. */
	size_t size;
	/** Bytes of the window already handed out by the BAR allocator. */
	size_t allocation_offset;
};

/** Mutable state of one DesignWare PCIe root complex instance. */
struct pcie_dw_data {
	/** Mapped DBI window, which also holds the root complex configuration header. */
	DEVICE_MMIO_NAMED_RAM(pcie_dbi);
	/** Mapped outbound configuration space window used for CFG0/CFG1 accesses. */
	DEVICE_MMIO_NAMED_RAM(config);
	/** Root bus number. */
	uint8_t root_bus;
	/** Last bus number of the hierarchy. */
	uint8_t last_bus;
	/** The iATU registers are unrolled instead of accessed through a viewport. */
	bool atu_unroll;
	/** Number of available outbound iATU windows. */
	uint16_t num_windows;
	/** Serializes configuration space accesses and their iATU programming. */
	struct k_spinlock cfg_lock;
	/** Translation ranges of the outbound iATU windows. */
	struct pcie_dw_region regions[PCIE_DW_REGION_COUNT];
};

/** Static description of one DesignWare PCIe root complex instance. */
struct pcie_dw_config {
	/** Generic PCIe controller configuration from the devicetree. */
	const struct pcie_ctrl_config *common;
	/** DBI window, also holding the root complex configuration header. */
	DEVICE_MMIO_NAMED_ROM(pcie_dbi);
	/** Outbound configuration space window used for CFG0/CFG1 accesses. */
	DEVICE_MMIO_NAMED_ROM(config);
	/** Size of the DBI window in bytes. */
	size_t dbi_size;
	/** Root bus number. */
	uint8_t root_bus;
	/** Last bus number of the hierarchy. */
	uint8_t last_bus;
	/** Number of lanes of the link. */
	uint8_t num_lanes;
	/** Maximum link speed in PCIe generations. */
	uint8_t max_link_speed;
};

/**
 * @brief Map the DBI and configuration space windows and parse the ranges
 *
 * @param dev PCIe controller device
 * @retval 0 On success
 * @retval -EINVAL Invalid controller description
 * @retval -ERANGE A translation range overflows
 */
int pcie_dw_map(const struct device *dev);

/**
 * @brief Set up the root complex configuration header and detect the iATU
 *
 * Must be called after pcie_dw_map() and before any configuration space
 * access or outbound window programming.
 *
 * @param dev PCIe controller device
 * @retval 0 On success
 * @retval -EINVAL Invalid or unsupported controller configuration
 */
int pcie_dw_setup_root(const struct device *dev);

/**
 * @brief Program one outbound iATU window per translation range
 *
 * @param dev PCIe controller device
 * @retval 0 On success
 * @retval -ENOSPC More ranges than available iATU windows
 * @retval -ERANGE A range cannot be represented by an iATU window
 * @retval -EIO An iATU window could not be enabled
 */
int pcie_dw_setup_outbound_ranges(const struct device *dev);

/**
 * @brief Read a 32-bit word from an endpoint configuration space
 *
 * @param dev PCIe controller device
 * @param bdf Endpoint BDF
 * @param reg Configuration word index
 * @return The word read, or 0xffffffff if the endpoint or word does not exist
 */
uint32_t pcie_dw_conf_read(const struct device *dev, pcie_bdf_t bdf, unsigned int reg);

/**
 * @brief Write a 32-bit word to an endpoint configuration space
 *
 * @param dev PCIe controller device
 * @param bdf Endpoint BDF
 * @param reg Configuration word index
 * @param value Value to write
 */
void pcie_dw_conf_write(const struct device *dev, pcie_bdf_t bdf, unsigned int reg, uint32_t value);

/**
 * @brief Allocate a BAR range from the translation ranges
 *
 * @param dev PCIe controller device
 * @param bdf Endpoint BDF
 * @param mem True for a memory BAR, false for an I/O BAR
 * @param mem64 True for a 64-bit memory BAR
 * @param bar_size BAR size in bytes
 * @param bar_bus_addr Allocated PCI bus address
 * @return True on success, false if no range of the requested type fits
 */
bool pcie_dw_region_allocate(const struct device *dev, pcie_bdf_t bdf, bool mem, bool mem64,
			     size_t bar_size, uintptr_t *bar_bus_addr);

/**
 * @brief Get the current allocation base for a bridge window
 *
 * @param dev PCIe controller device
 * @param bdf Bridge BDF
 * @param mem True for a memory window, false for an I/O window
 * @param mem64 True for a 64-bit memory window
 * @param align Window alignment in bytes
 * @param bar_base_addr Bus address the window starts at
 * @return True on success, false if the aligned window does not fit
 */
bool pcie_dw_region_get_allocate_base(const struct device *dev, pcie_bdf_t bdf, bool mem,
				      bool mem64, size_t align, uintptr_t *bar_base_addr);

/**
 * @brief Translate a PCI bus address into a CPU address
 *
 * @param dev PCIe controller device
 * @param bdf Endpoint BDF
 * @param mem True for a memory BAR, false for an I/O BAR
 * @param mem64 True for a 64-bit memory BAR
 * @param bar_bus_addr PCI bus address to translate
 * @param bar_addr Translated CPU address
 * @return True on success, false if the address is outside the range
 */
bool pcie_dw_region_translate(const struct device *dev, pcie_bdf_t bdf, bool mem, bool mem64,
			      uintptr_t bar_bus_addr, uintptr_t *bar_addr);

#endif /* ZEPHYR_DRIVERS_PCIE_CONTROLLER_PCIE_DW_COMMON_H_ */

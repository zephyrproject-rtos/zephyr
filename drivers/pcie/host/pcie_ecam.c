/*
 * Copyright (c) 2021 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(pcie_ecam, LOG_LEVEL_ERR);

#include <kernel.h>
#include <device.h>
#include <drivers/pcie/pcie.h>
#include <drivers/pcie/controller.h>

#define DT_DRV_COMPAT pci_host_ecam_generic

/*
 * PCIe Controllers Regions
 *
 * TOFIX:
 * - handle prefetchable regions
 */
enum pcie_region_type {
	PCIE_REGION_IO = 0,
	PCIE_REGION_MEM,
	PCIE_REGION_MEM64,
	PCIE_REGION_MAX,
};

struct pcie_ecam_data {
	uintptr_t cfg_phys_addr;
	mm_reg_t cfg_addr;
	size_t cfg_size;
	struct {
		uintptr_t phys_start;
		uintptr_t bus_start;
		size_t size;
		size_t allocation_offset;
	} regions[PCIE_REGION_MAX];
};

static int pcie_ecam_init(const struct device *dev)
{
	const struct pcie_ctrl_config *cfg = (const struct pcie_ctrl_config *)dev->config;
	struct pcie_ecam_data *data = (struct pcie_ecam_data *)dev->data;
	int i;

	/*
	 * Flags defined in the PCI Bus Binding to IEEE Std 1275-1994 :
	 *           Bit# 33222222 22221111 11111100 00000000
	 *                10987654 32109876 54321098 76543210
	 *
	 * phys.hi cell:  npt000ss bbbbbbbb dddddfff rrrrrrrr
	 * phys.mid cell: hhhhhhhh hhhhhhhh hhhhhhhh hhhhhhhh
	 * phys.lo cell:  llllllll llllllll llllllll llllllll
	 *
	 * where:
	 *
	 * n	is 0 if the address is relocatable, 1 otherwise
	 * p	is 1 if the addressable region is "prefetchable", 0 otherwise
	 * t	is 1 if the address is aliased (for non-relocatable I/O), below 1 MB (for Memory),
	 *	or below 64 KB (for relocatable I/O).
	 * ss	is the space code, denoting the address space
	 *	00 denotes Configuration Space
	 *	01 denotes I/O Space
	 *	10 denotes 32-bit-address Memory Space
	 *	11 denotes 64-bit-address Memory Space
	 * bbbbbbbb	is the 8-bit Bus Number
	 * ddddd	is the 5-bit Device Number
	 * fff	is the 3-bit Function Number
	 * rrrrrrrr	is the 8-bit Register Number
	 * hh...hh	is a 32-bit unsigned number
	 * ll...ll	is a 32-bit unsigned number
	 *	for I/O Space is the 32-bit offset from the start of the region
	 *	for 32-bit-address Memory Space is the 32-bit offset from the start of the region
	 *	for 64-bit-address Memory Space is the 64-bit offset from the start of the region
	 *
	 * Here we only handle the p, ss, hh and ll fields.
	 *
	 * TOFIX:
	 * - handle prefetchable bit
	 */
	for (i = 0 ; i < cfg->ranges_count ; ++i) {
		switch ((cfg->ranges[i].flags >> 24) & 0x03) {
		case 0x01:
			data->regions[PCIE_REGION_IO].bus_start = cfg->ranges[i].pcie_bus_addr;
			data->regions[PCIE_REGION_IO].phys_start = cfg->ranges[i].host_map_addr;
			data->regions[PCIE_REGION_IO].size = cfg->ranges[i].map_length;
			/* Linux & U-Boot avoids allocating PCI resources from address 0 */
			if (data->regions[PCIE_REGION_IO].bus_start < 0x1000)
				data->regions[PCIE_REGION_IO].allocation_offset = 0x1000;
			break;
		case 0x02:
			data->regions[PCIE_REGION_MEM].bus_start = cfg->ranges[i].pcie_bus_addr;
			data->regions[PCIE_REGION_MEM].phys_start = cfg->ranges[i].host_map_addr;
			data->regions[PCIE_REGION_MEM].size = cfg->ranges[i].map_length;
			/* Linux & U-Boot avoids allocating PCI resources from address 0 */
			if (data->regions[PCIE_REGION_MEM].bus_start < 0x1000)
				data->regions[PCIE_REGION_MEM].allocation_offset = 0x1000;
			break;
		case 0x03:
			data->regions[PCIE_REGION_MEM64].bus_start = cfg->ranges[i].pcie_bus_addr;
			data->regions[PCIE_REGION_MEM64].phys_start = cfg->ranges[i].host_map_addr;
			data->regions[PCIE_REGION_MEM64].size = cfg->ranges[i].map_length;
			/* Linux & U-Boot avoids allocating PCI resources from address 0 */
			if (data->regions[PCIE_REGION_MEM64].bus_start < 0x1000)
				data->regions[PCIE_REGION_MEM64].allocation_offset = 0x1000;
			break;
		}
	}

	if (!data->regions[PCIE_REGION_IO].size &&
	    !data->regions[PCIE_REGION_MEM].size &&
	    !data->regions[PCIE_REGION_MEM64].size) {
		LOG_ERR("No regions defined");
		return -EINVAL;
	}

	/* Get Config address space physical address & size */
	data->cfg_phys_addr = cfg->cfg_addr;
	data->cfg_size = cfg->cfg_size;

	if (data->regions[PCIE_REGION_IO].size) {
		LOG_DBG("IO bus [0x%lx - 0x%lx, size 0x%lx]",
			data->regions[PCIE_REGION_IO].bus_start,
			(data->regions[PCIE_REGION_IO].bus_start +
			 data->regions[PCIE_REGION_IO].size - 1),
			data->regions[PCIE_REGION_IO].size);
		LOG_DBG("IO space [0x%lx - 0x%lx, size 0x%lx]",
			data->regions[PCIE_REGION_IO].phys_start,
			(data->regions[PCIE_REGION_IO].phys_start +
			 data->regions[PCIE_REGION_IO].size - 1),
			data->regions[PCIE_REGION_IO].size);
	}
	if (data->regions[PCIE_REGION_MEM].size) {
		LOG_DBG("MEM bus [0x%lx - 0x%lx, size 0x%lx]",
			data->regions[PCIE_REGION_MEM].bus_start,
			(data->regions[PCIE_REGION_MEM].bus_start +
			 data->regions[PCIE_REGION_MEM].size - 1),
			data->regions[PCIE_REGION_MEM].size);
		LOG_DBG("MEM space [0x%lx - 0x%lx, size 0x%lx]",
			data->regions[PCIE_REGION_MEM].phys_start,
			(data->regions[PCIE_REGION_MEM].phys_start +
			 data->regions[PCIE_REGION_MEM].size - 1),
			data->regions[PCIE_REGION_MEM].size);
	}
	if (data->regions[PCIE_REGION_MEM64].size) {
		LOG_DBG("MEM64 bus [0x%lx - 0x%lx, size 0x%lx]",
			data->regions[PCIE_REGION_MEM64].bus_start,
			(data->regions[PCIE_REGION_MEM64].bus_start +
			 data->regions[PCIE_REGION_MEM64].size - 1),
			data->regions[PCIE_REGION_MEM64].size);
		LOG_DBG("MEM64 space [0x%lx - 0x%lx, size 0x%lx]",
			data->regions[PCIE_REGION_MEM64].phys_start,
			(data->regions[PCIE_REGION_MEM64].phys_start +
			 data->regions[PCIE_REGION_MEM64].size - 1),
			data->regions[PCIE_REGION_MEM64].size);
	}

	/* Map config space to be used by the generic_pcie_ctrl_conf_read/write callbacks */
	device_map(&data->cfg_addr, data->cfg_phys_addr, data->cfg_size, K_MEM_CACHE_NONE);

	LOG_DBG("Config space [0x%lx - 0x%lx, size 0x%lx]",
		data->cfg_phys_addr, (data->cfg_phys_addr + data->cfg_size - 1), data->cfg_size);
	LOG_DBG("Config mapped [0x%lx - 0x%lx, size 0x%lx]",
		data->cfg_addr, (data->cfg_addr + data->cfg_size - 1), data->cfg_size);

	generic_pcie_ctrl_enumerate(dev, PCIE_BDF(0, 0, 0));

	return 0;
}

static uint32_t pcie_ecam_ctrl_conf_read(const struct device *dev, pcie_bdf_t bdf, unsigned int reg)
{
	struct pcie_ecam_data *data = (struct pcie_ecam_data *)dev->data;

	return generic_pcie_ctrl_conf_read(data->cfg_addr, bdf, reg);
}

static void pcie_ecam_ctrl_conf_write(const struct device *dev, pcie_bdf_t bdf, unsigned int reg,
				      uint32_t reg_data)
{
	struct pcie_ecam_data *data = (struct pcie_ecam_data *)dev->data;

	generic_pcie_ctrl_conf_write(data->cfg_addr, bdf, reg, reg_data);
}

static bool pcie_ecam_region_allocate_type(struct pcie_ecam_data *data, pcie_bdf_t bdf,
					   size_t bar_size, uintptr_t *bar_bus_addr,
					   enum pcie_region_type type)
{
	uintptr_t addr;

	addr = (((data->regions[type].bus_start + data->regions[type].allocation_offset) - 1) |
		((bar_size) - 1)) + 1;

	if (addr - data->regions[type].bus_start + bar_size > data->regions[type].size)
		return false;

	*bar_bus_addr = addr;
	data->regions[type].allocation_offset = addr - data->regions[type].bus_start + bar_size;

	return true;
}

static bool pcie_ecam_region_allocate(const struct device *dev, pcie_bdf_t bdf,
				      bool mem, bool mem64, size_t bar_size,
				      uintptr_t *bar_bus_addr)
{
	struct pcie_ecam_data *data = (struct pcie_ecam_data *)dev->data;
	enum pcie_region_type type;

	if (mem && !data->regions[PCIE_REGION_MEM64].size &&
	    !data->regions[PCIE_REGION_MEM].size) {
		LOG_DBG("bdf %x no mem region defined for allocation", bdf);
		return false;
	}

	if (!mem && !data->regions[PCIE_REGION_IO].size) {
		LOG_DBG("bdf %x no io region defined for allocation", bdf);
		return false;
	}

	/*
	 * Allocate into mem64 region if available or is the only available
	 *
	 * TOFIX:
	 * - handle allocation from/to mem/mem64 when a region is full
	 */
	if (mem && ((mem64 && data->regions[PCIE_REGION_MEM64].size) ||
		    (data->regions[PCIE_REGION_MEM64].size &&
		     !data->regions[PCIE_REGION_MEM].size))) {
		type = PCIE_REGION_MEM64;
	} else if (mem) {
		type = PCIE_REGION_MEM;
	} else {
		type = PCIE_REGION_IO;
	}

	return pcie_ecam_region_allocate_type(data, bdf, bar_size, bar_bus_addr, type);
}

static bool pcie_ecam_region_xlate(const struct device *dev, pcie_bdf_t bdf,
				   bool mem, bool mem64, uintptr_t bar_bus_addr,
				   uintptr_t *bar_addr)
{
	struct pcie_ecam_data *data = (struct pcie_ecam_data *)dev->data;
	enum pcie_region_type type;

	/* Means it hasn't been allocated */
	if (!bar_bus_addr)
		return false;

	if (mem && ((mem64 && data->regions[PCIE_REGION_MEM64].size) ||
		    (data->regions[PCIE_REGION_MEM64].size &&
		     !data->regions[PCIE_REGION_MEM].size))) {
		type = PCIE_REGION_MEM64;
	} else if (mem) {
		type = PCIE_REGION_MEM;
	} else {
		type = PCIE_REGION_IO;
	}

	*bar_addr = data->regions[type].phys_start + (bar_bus_addr - data->regions[type].bus_start);

	return true;
}

static const struct pcie_ctrl_driver_api pcie_ecam_api = {
	.conf_read = pcie_ecam_ctrl_conf_read,
	.conf_write = pcie_ecam_ctrl_conf_write,
	.region_allocate = pcie_ecam_region_allocate,
	.region_xlate = pcie_ecam_region_xlate,
};

#define PCIE_ECAM_INIT(n)							\
	static struct pcie_ecam_data pcie_ecam_data##n;				\
	static const struct pcie_ctrl_config pcie_ecam_config##n = {		\
		.cfg_addr = DT_INST_REG_ADDR(n),				\
		.cfg_size = DT_INST_REG_SIZE(n),				\
		.ranges_count = DT_NUM_RANGES(DT_DRV_INST(n)),		\
		.ranges = {							\
			DT_FOREACH_RANGE(DT_DRV_INST(n), PCIE_RANGE_FORMAT)	\
		},								\
	};									\
	DEVICE_DT_INST_DEFINE(n, &pcie_ecam_init, NULL,				\
			      &pcie_ecam_data##n,				\
			      &pcie_ecam_config##n,				\
			      PRE_KERNEL_1,					\
			      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,		\
			      &pcie_ecam_api);

DT_INST_FOREACH_STATUS_OKAY(PCIE_ECAM_INIT)

/*
 * Copyright (c) 2026 Process Mission
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/logging/log.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/util.h>

#include <zephyr/drivers/pcie/cap.h>
#include <zephyr/drivers/pcie/pcie.h>

#include "pcie_dw_common.h"

#define DEV_CFG(dev)  ((const struct pcie_dw_config *)(dev)->config)
#define DEV_DATA(dev) ((struct pcie_dw_data *)(dev)->data)

LOG_MODULE_REGISTER(pcie_dw, CONFIG_PCIE_LOG_LEVEL);

/* DesignWare PCIe Port Logic registers. */
#define PCIE_DW_PORT_LINK_CONTROL        0x710
#define PCIE_DW_PORT_LINK_DLL_ENABLE     BIT(5)
#define PCIE_DW_PORT_LINK_FAST_MODE      BIT(7)
#define PCIE_DW_PORT_LINK_MODE_MASK      GENMASK(21, 16)
#define PCIE_DW_PORT_LINK_MODE_SHIFT     16
/* Link mode: one bit per configured lane (x1 = 0x1 ... x8 = 0xf). */
#define PCIE_DW_PORT_LINK_MODE_X1        0x1U
#define PCIE_DW_PORT_LINK_MODE_X2        0x3U
#define PCIE_DW_PORT_LINK_MODE_X4        0x7U
#define PCIE_DW_PORT_LINK_MODE_X8        0xfU
#define PCIE_DW_LINK_WIDTH_SPEED_CONTROL 0x80c
#define PCIE_DW_LINK_WIDTH_MASK          GENMASK(12, 8)
#define PCIE_DW_LINK_SPEED_CHANGE        BIT(17)
#define PCIE_DW_MISC_CONTROL_1           0x8bc
#define PCIE_DW_DBI_RO_WR_ENABLE         BIT(0)

/* Viewport iATU registers. */
#define PCIE_DW_ATU_VIEWPORT              0x900
#define PCIE_DW_ATU_VIEWPORT_CR1          0x904
#define PCIE_DW_ATU_VIEWPORT_CR2          0x908
#define PCIE_DW_ATU_VIEWPORT_LOWER_BASE   0x90c
#define PCIE_DW_ATU_VIEWPORT_UPPER_BASE   0x910
#define PCIE_DW_ATU_VIEWPORT_LIMIT        0x914
#define PCIE_DW_ATU_VIEWPORT_LOWER_TARGET 0x918
#define PCIE_DW_ATU_VIEWPORT_UPPER_TARGET 0x91c

/* Unrolled outbound iATU registers. */
#define PCIE_DW_ATU_UNROLL_BASE         0x300000
#define PCIE_DW_ATU_UNROLL_REGION_SIZE  0x200
#define PCIE_DW_ATU_UNROLL_CR1          0x00
#define PCIE_DW_ATU_UNROLL_CR2          0x04
#define PCIE_DW_ATU_UNROLL_LOWER_BASE   0x08
#define PCIE_DW_ATU_UNROLL_UPPER_BASE   0x0c
#define PCIE_DW_ATU_UNROLL_LIMIT        0x10
#define PCIE_DW_ATU_UNROLL_LOWER_TARGET 0x14
#define PCIE_DW_ATU_UNROLL_UPPER_TARGET 0x18

/* Number of outbound windows covered by the unrolled iATU aperture. */
#define PCIE_DW_ATU_MAX_WINDOWS 8U

#define PCIE_DW_ATU_ENABLE         BIT(31)
#define PCIE_DW_ATU_TYPE_MEM       0x0
#define PCIE_DW_ATU_TYPE_IO        0x2
#define PCIE_DW_ATU_TYPE_CFG0      0x4
#define PCIE_DW_ATU_TYPE_CFG1      0x5
#define PCIE_DW_ATU_CFG_BUS(bus)   ((uint32_t)(bus) << 24)
#define PCIE_DW_ATU_CFG_DEV(dev)   ((uint32_t)(dev) << 19)
#define PCIE_DW_ATU_CFG_FUNC(func) ((uint32_t)(func) << 16)

#define PCIE_DW_ATU_ENABLE_RETRIES 5
#define PCIE_DW_DBI2_OFFSET        0x100000

/* PCI configuration space is 4 KiB, or 4 KiB / sizeof(uint32_t) words. */
#define PCIE_DW_CFG_SPACE_SIZE   4096U
#define PCIE_DW_CONF_OFFSET(reg) ((reg) * sizeof(uint32_t))

/* PCI Express capability registers, relative to the capability header. */
#define PCIE_DW_PCI_EXP_LINK_CAPABILITY 0x0c
#define PCIE_DW_PCI_EXP_LINK_CONTROL2   0x30
#define PCIE_DW_PCI_EXP_LINK_SPEED_MASK GENMASK(3, 0)

/* Class code of a PCI-to-PCI bridge (base class 0x06, subclass 0x04). */
#define PCIE_DW_CLASS_BRIDGE_PCI 0x060400U

static inline uint32_t lower_32_bits(uint64_t value)
{
	return (uint32_t)value;
}

static inline uint32_t upper_32_bits(uint64_t value)
{
	return (uint32_t)(value >> 32);
}

/* Add a byte offset to an address, returning false if the sum overflows. */
static inline bool pcie_dw_addr_add(uintptr_t base, size_t offset, uintptr_t *result)
{
	if (offset > (UINTPTR_MAX - base)) {
		return false;
	}

	*result = base + offset;
	return true;
}

static int pcie_dw_parse_ranges(struct pcie_dw_data *data, const struct pcie_ctrl_config *config)
{
	for (size_t i = 0; i < config->ranges_count; i++) {
		/* Invalid default; unsupported range types are skipped below. */
		enum pcie_dw_region_type type = PCIE_DW_REGION_COUNT;
		uintptr_t range_end;

		/* The range type is encoded in the flags of the OF PCI bus binding. */
		switch ((config->ranges[i].flags >> 24) & 0x3U) {
		case 0x1U:
			type = PCIE_DW_REGION_IO;
			break;
		case 0x2U:
			type = PCIE_DW_REGION_MEM;
			break;
		case 0x3U:
			type = PCIE_DW_REGION_MEM64;
			break;
		default:
			continue;
		}

		if (data->regions[type].size != 0U) {
			LOG_ERR("duplicate PCIe range type %u", type);
			return -EINVAL;
		}

		data->regions[type].bus_start = config->ranges[i].pcie_bus_addr;
		data->regions[type].phys_start = config->ranges[i].host_map_addr;
		data->regions[type].size = config->ranges[i].map_length;
		if (data->regions[type].size == 0U ||
		    !pcie_dw_addr_add(data->regions[type].bus_start, data->regions[type].size - 1U,
				      &range_end) ||
		    !pcie_dw_addr_add(data->regions[type].phys_start, data->regions[type].size - 1U,
				      &range_end)) {
			LOG_ERR("invalid PCIe range type %u", type);
			return -ERANGE;
		}

		/* Keep address zero unavailable, matching common host-firmware practice. */
		if (data->regions[type].bus_start < 0x1000U) {
			data->regions[type].allocation_offset = 0x1000U;
		}
	}

	if (data->regions[PCIE_DW_REGION_IO].size == 0U &&
	    data->regions[PCIE_DW_REGION_MEM].size == 0U &&
	    data->regions[PCIE_DW_REGION_MEM64].size == 0U) {
		LOG_ERR("no usable PCIe outbound ranges");
		return -EINVAL;
	}

	return 0;
}

int pcie_dw_map(const struct device *dev)
{
	const struct pcie_dw_config *config = DEV_CFG(dev);
	struct pcie_dw_data *data = DEV_DATA(dev);
	int ret;

	/* The window must hold at least the 4 KiB config space of one function. */
	if (config->dbi_size <= PCIE_DW_ATU_VIEWPORT_UPPER_TARGET ||
	    config->common->cfg_size < PCIE_DW_CFG_SPACE_SIZE) {
		return -EINVAL;
	}

	ret = pcie_dw_parse_ranges(data, config->common);
	if (ret != 0) {
		return ret;
	}

	data->root_bus = config->root_bus;
	data->last_bus = config->last_bus;
	/* A safe default until pcie_dw_setup_root() detects the real count. */
	data->num_windows = PCIE_DW_ATU_MAX_WINDOWS;

	DEVICE_MMIO_NAMED_MAP(dev, pcie_dbi, K_MEM_CACHE_NONE);
	DEVICE_MMIO_NAMED_MAP(dev, config, K_MEM_CACHE_NONE);

	return 0;
}

static mm_reg_t pcie_dw_atu_reg(const struct device *dev, uint8_t index, uint32_t viewport_offset,
				uint32_t unroll_offset)
{
	const struct pcie_dw_data *data = DEV_DATA(dev);
	mm_reg_t dbi = DEVICE_MMIO_NAMED_GET(dev, pcie_dbi);

	if (data->atu_unroll) {
		return dbi + PCIE_DW_ATU_UNROLL_BASE + (index * PCIE_DW_ATU_UNROLL_REGION_SIZE) +
		       unroll_offset;
	}

	return dbi + viewport_offset;
}

static int pcie_dw_program_outbound_atu(const struct device *dev, uint8_t index, uint32_t type,
					uint64_t cpu_phys, uint64_t pci_bus_addr, size_t size)
{
	struct pcie_dw_data *data = DEV_DATA(dev);
	mm_reg_t cr1;
	mm_reg_t cr2;
	uint64_t limit_phys;
	uint64_t limit_bus;
	size_t atu_end;

	if (size == 0U || u64_add_overflow(cpu_phys, size - 1U, &limit_phys) ||
	    u64_add_overflow(pci_bus_addr, size - 1U, &limit_bus)) {
		return -EINVAL;
	}

	if (index >= data->num_windows) {
		LOG_ERR("outbound iATU window %u is not available", index);
		return -ENOSPC;
	}

	if (data->atu_unroll) {
		atu_end = PCIE_DW_ATU_UNROLL_BASE + (index * PCIE_DW_ATU_UNROLL_REGION_SIZE) +
			  PCIE_DW_ATU_UNROLL_UPPER_TARGET + sizeof(uint32_t);
		if (atu_end > DEV_CFG(dev)->dbi_size) {
			LOG_ERR("DBI window is too small for unrolled iATU%u", index);
			return -ERANGE;
		}
	}

	if (upper_32_bits(cpu_phys) != upper_32_bits(limit_phys)) {
		LOG_ERR("iATU window %u crosses a 4-GiB boundary", index);
		return -ERANGE;
	}

	if (!data->atu_unroll) {
		sys_write32((uint32_t)index,
			    DEVICE_MMIO_NAMED_GET(dev, pcie_dbi) + PCIE_DW_ATU_VIEWPORT);
	}

	cr1 = pcie_dw_atu_reg(dev, index, PCIE_DW_ATU_VIEWPORT_CR1, PCIE_DW_ATU_UNROLL_CR1);
	cr2 = pcie_dw_atu_reg(dev, index, PCIE_DW_ATU_VIEWPORT_CR2, PCIE_DW_ATU_UNROLL_CR2);

	sys_write32(lower_32_bits(cpu_phys),
		    pcie_dw_atu_reg(dev, index, PCIE_DW_ATU_VIEWPORT_LOWER_BASE,
				    PCIE_DW_ATU_UNROLL_LOWER_BASE));
	sys_write32(upper_32_bits(cpu_phys),
		    pcie_dw_atu_reg(dev, index, PCIE_DW_ATU_VIEWPORT_UPPER_BASE,
				    PCIE_DW_ATU_UNROLL_UPPER_BASE));
	sys_write32(
		lower_32_bits(limit_phys),
		pcie_dw_atu_reg(dev, index, PCIE_DW_ATU_VIEWPORT_LIMIT, PCIE_DW_ATU_UNROLL_LIMIT));
	sys_write32(lower_32_bits(pci_bus_addr),
		    pcie_dw_atu_reg(dev, index, PCIE_DW_ATU_VIEWPORT_LOWER_TARGET,
				    PCIE_DW_ATU_UNROLL_LOWER_TARGET));
	sys_write32(upper_32_bits(pci_bus_addr),
		    pcie_dw_atu_reg(dev, index, PCIE_DW_ATU_VIEWPORT_UPPER_TARGET,
				    PCIE_DW_ATU_UNROLL_UPPER_TARGET));
	sys_write32(type, cr1);
	sys_write32(PCIE_DW_ATU_ENABLE, cr2);

	for (int retry = 0; retry < PCIE_DW_ATU_ENABLE_RETRIES; retry++) {
		if ((sys_read32(cr2) & PCIE_DW_ATU_ENABLE) != 0U) {
			return 0;
		}
		k_busy_wait(10);
	}

	LOG_ERR("failed to enable outbound iATU window %u", index);
	return -EIO;
}

static uint8_t pcie_dw_find_pcie_capability(const struct device *dev)
{
	mm_reg_t dbi = DEVICE_MMIO_NAMED_GET(dev, pcie_dbi);
	uint32_t command_status = sys_read32(dbi + PCIE_DW_CONF_OFFSET(PCIE_CONF_CMDSTAT));
	uint8_t offset;

	if ((command_status & PCIE_CONF_CMDSTAT_CAPS) == 0U) {
		return 0U;
	}

	offset = sys_read32(DEVICE_MMIO_NAMED_GET(dev, pcie_dbi) +
			    PCIE_DW_CONF_OFFSET(PCIE_CONF_CAPPTR)) &
		 0xfcU;
	for (int count = 0; (offset >= 0x40U) && (count < 48); count++) {
		uint32_t header = sys_read32(DEVICE_MMIO_NAMED_GET(dev, pcie_dbi) + offset);

		if ((header & 0xffU) == PCI_CAP_ID_EXP) {
			return offset;
		}
		offset = (header >> 8) & 0xfcU;
	}

	return 0U;
}

static uint32_t pcie_dw_lane_mode(uint8_t lanes)
{
	switch (lanes) {
	case 1:
		return PCIE_DW_PORT_LINK_MODE_X1;
	case 2:
		return PCIE_DW_PORT_LINK_MODE_X2;
	case 4:
		return PCIE_DW_PORT_LINK_MODE_X4;
	case 8:
		return PCIE_DW_PORT_LINK_MODE_X8;
	default:
		return 0U;
	}
}

int pcie_dw_setup_root(const struct device *dev)
{
	const struct pcie_dw_config *config = DEV_CFG(dev);
	struct pcie_dw_data *data = DEV_DATA(dev);
	uint32_t value;
	uint32_t lane_mode = pcie_dw_lane_mode(config->num_lanes);
	uint8_t pcie_cap;

	if (lane_mode == 0U) {
		LOG_ERR("unsupported lane count %u", config->num_lanes);
		return -EINVAL;
	}
	if (config->root_bus >= config->last_bus) {
		LOG_ERR("root bus has no available secondary bus");
		return -EINVAL;
	}
	if (config->max_link_speed > 4U) {
		LOG_ERR("unsupported maximum link speed Gen%u", config->max_link_speed);
		return -EINVAL;
	}

	/*
	 * A read of 0xffffffff from the viewport register tells a viewport based
	 * iATU from an unrolled one. The unrolled iATU aperture covers up to
	 * PCIE_DW_ATU_MAX_WINDOWS windows; a viewport based iATU reports the index
	 * of the last window when 0xff is written to the viewport register.
	 */
	data->atu_unroll = (sys_read32(DEVICE_MMIO_NAMED_GET(dev, pcie_dbi) +
				       PCIE_DW_ATU_VIEWPORT) == UINT32_MAX);
	if (data->atu_unroll) {
		size_t windows;

		if (DEV_CFG(dev)->dbi_size <
		    (PCIE_DW_ATU_UNROLL_BASE + PCIE_DW_ATU_UNROLL_REGION_SIZE)) {
			LOG_ERR("DBI window is too small for unrolled iATU");
			return -EINVAL;
		}

		windows = (DEV_CFG(dev)->dbi_size - PCIE_DW_ATU_UNROLL_BASE) /
			  PCIE_DW_ATU_UNROLL_REGION_SIZE;
		data->num_windows = (uint16_t)MIN(windows, PCIE_DW_ATU_MAX_WINDOWS);
	} else {
		sys_write32(0xffU, DEVICE_MMIO_NAMED_GET(dev, pcie_dbi) + PCIE_DW_ATU_VIEWPORT);
		data->num_windows = (uint16_t)(sys_read32(DEVICE_MMIO_NAMED_GET(dev, pcie_dbi) +
							  PCIE_DW_ATU_VIEWPORT) +
					       1U);
		sys_write32(0U, DEVICE_MMIO_NAMED_GET(dev, pcie_dbi) + PCIE_DW_ATU_VIEWPORT);
	}

	if (data->num_windows <= 1U) {
		LOG_ERR("no outbound iATU window is available");
		return -EINVAL;
	}

	value = sys_read32(DEVICE_MMIO_NAMED_GET(dev, pcie_dbi) + PCIE_DW_MISC_CONTROL_1);
	sys_write32(value | PCIE_DW_DBI_RO_WR_ENABLE,
		    DEVICE_MMIO_NAMED_GET(dev, pcie_dbi) + PCIE_DW_MISC_CONTROL_1);

	value = sys_read32(DEVICE_MMIO_NAMED_GET(dev, pcie_dbi) + PCIE_DW_PORT_LINK_CONTROL);
	value &= ~(PCIE_DW_PORT_LINK_MODE_MASK | PCIE_DW_PORT_LINK_FAST_MODE);
	value |= PCIE_DW_PORT_LINK_DLL_ENABLE | (lane_mode << PCIE_DW_PORT_LINK_MODE_SHIFT);
	sys_write32(value, DEVICE_MMIO_NAMED_GET(dev, pcie_dbi) + PCIE_DW_PORT_LINK_CONTROL);

	value = sys_read32(DEVICE_MMIO_NAMED_GET(dev, pcie_dbi) + PCIE_DW_LINK_WIDTH_SPEED_CONTROL);
	value &= ~PCIE_DW_LINK_WIDTH_MASK;
	value |= ((uint32_t)config->num_lanes << 8) | PCIE_DW_LINK_SPEED_CHANGE;
	sys_write32(value, DEVICE_MMIO_NAMED_GET(dev, pcie_dbi) + PCIE_DW_LINK_WIDTH_SPEED_CONTROL);

	pcie_cap = pcie_dw_find_pcie_capability(dev);
	if (pcie_cap != 0U && config->max_link_speed != 0U) {
		value = sys_read32(DEVICE_MMIO_NAMED_GET(dev, pcie_dbi) + pcie_cap +
				   PCIE_DW_PCI_EXP_LINK_CAPABILITY);
		value = (value & ~PCIE_DW_PCI_EXP_LINK_SPEED_MASK) | config->max_link_speed;
		sys_write32(value, DEVICE_MMIO_NAMED_GET(dev, pcie_dbi) + pcie_cap +
					   PCIE_DW_PCI_EXP_LINK_CAPABILITY);

		value = sys_read32(DEVICE_MMIO_NAMED_GET(dev, pcie_dbi) + pcie_cap +
				   PCIE_DW_PCI_EXP_LINK_CONTROL2);
		value = (value & ~PCIE_DW_PCI_EXP_LINK_SPEED_MASK) | config->max_link_speed;
		sys_write32(value, DEVICE_MMIO_NAMED_GET(dev, pcie_dbi) + pcie_cap +
					   PCIE_DW_PCI_EXP_LINK_CONTROL2);
	}

	/*
	 * Disable the Root Complex BAR apertures before generic enumeration:
	 * program BAR0 as an empty 64-bit memory BAR, then clear it again once
	 * the rest of the header has been set up.
	 */
	sys_write32(0x4U,
		    DEVICE_MMIO_NAMED_GET(dev, pcie_dbi) + PCIE_DW_CONF_OFFSET(PCIE_CONF_BAR0));
	sys_write32(0U, DEVICE_MMIO_NAMED_GET(dev, pcie_dbi) + PCIE_DW_CONF_OFFSET(PCIE_CONF_BAR1));
	if (DEV_CFG(dev)->dbi_size >=
	    (PCIE_DW_DBI2_OFFSET + PCIE_DW_CONF_OFFSET(PCIE_CONF_BAR1) + sizeof(uint32_t))) {
		sys_write32(0U, DEVICE_MMIO_NAMED_GET(dev, pcie_dbi) + PCIE_DW_DBI2_OFFSET +
					PCIE_DW_CONF_OFFSET(PCIE_CONF_BAR0));
		sys_write32(0U, DEVICE_MMIO_NAMED_GET(dev, pcie_dbi) + PCIE_DW_DBI2_OFFSET +
					PCIE_DW_CONF_OFFSET(PCIE_CONF_BAR1));
	}

	value = sys_read32(DEVICE_MMIO_NAMED_GET(dev, pcie_dbi) +
			   PCIE_DW_CONF_OFFSET(PCIE_CONF_INTR));
	value = (value & ~GENMASK(15, 8)) | BIT(8);
	sys_write32(value,
		    DEVICE_MMIO_NAMED_GET(dev, pcie_dbi) + PCIE_DW_CONF_OFFSET(PCIE_CONF_INTR));

	sys_write32(((uint32_t)data->root_bus) | ((uint32_t)(data->root_bus + 1U) << 8) |
			    ((uint32_t)data->last_bus << 16),
		    DEVICE_MMIO_NAMED_GET(dev, pcie_dbi) + PCIE_DW_CONF_OFFSET(PCIE_BUS_NUMBER));

	value = sys_read32(DEVICE_MMIO_NAMED_GET(dev, pcie_dbi) +
			   PCIE_DW_CONF_OFFSET(PCIE_CONF_CMDSTAT));
	value = (value & 0xffff0000U) | PCIE_CONF_CMDSTAT_IO | PCIE_CONF_CMDSTAT_MEM |
		PCIE_CONF_CMDSTAT_MASTER | PCIE_CONF_CMDSTAT_SERR;
	sys_write32(value,
		    DEVICE_MMIO_NAMED_GET(dev, pcie_dbi) + PCIE_DW_CONF_OFFSET(PCIE_CONF_CMDSTAT));

	value = sys_read32(DEVICE_MMIO_NAMED_GET(dev, pcie_dbi) +
			   PCIE_DW_CONF_OFFSET(PCIE_CONF_CLASSREV));
	value = (value & 0xffU) | (PCIE_DW_CLASS_BRIDGE_PCI << 8);
	sys_write32(value,
		    DEVICE_MMIO_NAMED_GET(dev, pcie_dbi) + PCIE_DW_CONF_OFFSET(PCIE_CONF_CLASSREV));
	sys_write32(0U, DEVICE_MMIO_NAMED_GET(dev, pcie_dbi) + PCIE_DW_CONF_OFFSET(PCIE_CONF_BAR0));

	value = sys_read32(DEVICE_MMIO_NAMED_GET(dev, pcie_dbi) + PCIE_DW_MISC_CONTROL_1);
	sys_write32(value & ~PCIE_DW_DBI_RO_WR_ENABLE,
		    DEVICE_MMIO_NAMED_GET(dev, pcie_dbi) + PCIE_DW_MISC_CONTROL_1);

	LOG_DBG("DWC root bus %u, %u lane(s), %s iATU", data->root_bus, config->num_lanes,
		data->atu_unroll ? "unrolled" : "viewport");
	return 0;
}

int pcie_dw_setup_outbound_ranges(const struct device *dev)
{
	struct pcie_dw_data *data = DEV_DATA(dev);
	uint8_t window = 1U;

	for (enum pcie_dw_region_type type = PCIE_DW_REGION_IO; type < PCIE_DW_REGION_COUNT;
	     type++) {
		struct pcie_dw_region *region = &data->regions[type];
		uint32_t atu_type;
		int ret;

		if (region->size == 0U) {
			continue;
		}

		atu_type = (type == PCIE_DW_REGION_IO) ? PCIE_DW_ATU_TYPE_IO : PCIE_DW_ATU_TYPE_MEM;
		ret = pcie_dw_program_outbound_atu(dev, window, atu_type, region->phys_start,
						   region->bus_start, region->size);
		if (ret != 0) {
			return ret;
		}

		LOG_DBG("iATU%u CPU %#lx -> PCI %#lx size %#lx", window, region->phys_start,
			region->bus_start, region->size);
		window++;
	}

	return 0;
}

static int pcie_dw_map_bus(const struct device *dev, pcie_bdf_t bdf, unsigned int reg,
			   mm_reg_t *address)
{
	struct pcie_dw_data *data = DEV_DATA(dev);
	uint8_t bus = PCIE_BDF_TO_BUS(bdf);
	uint8_t pci_dev = PCIE_BDF_TO_DEV(bdf);
	uint8_t func = PCIE_BDF_TO_FUNC(bdf);
	uint32_t target;
	uint32_t type;
	size_t offset;

	if (reg >= (PCIE_DW_CFG_SPACE_SIZE / sizeof(uint32_t))) {
		return -EINVAL;
	}
	offset = reg * sizeof(uint32_t);

	if (bus == data->root_bus) {
		if (pci_dev != 0U || func != 0U) {
			return -ENODEV;
		}
		if ((offset + sizeof(uint32_t)) > DEV_CFG(dev)->dbi_size) {
			return -EINVAL;
		}
		*address = DEVICE_MMIO_NAMED_GET(dev, pcie_dbi) + offset;
		return 0;
	}

	if (bus <= data->root_bus || bus > data->last_bus) {
		return -ENODEV;
	}

	/* A Root Port's secondary bus is a point-to-point PCIe link. */
	if (bus == (data->root_bus + 1U) && pci_dev != 0U) {
		return -ENODEV;
	}

	if ((offset + sizeof(uint32_t)) > DEV_CFG(dev)->common->cfg_size) {
		return -EINVAL;
	}

	target = PCIE_DW_ATU_CFG_BUS(bus) | PCIE_DW_ATU_CFG_DEV(pci_dev) |
		 PCIE_DW_ATU_CFG_FUNC(func);
	type = (bus == (data->root_bus + 1U)) ? PCIE_DW_ATU_TYPE_CFG0 : PCIE_DW_ATU_TYPE_CFG1;

	if (pcie_dw_program_outbound_atu(dev, 0U, type, DEV_CFG(dev)->common->cfg_addr, target,
					 DEV_CFG(dev)->common->cfg_size) != 0) {
		return -EIO;
	}

	*address = DEVICE_MMIO_NAMED_GET(dev, config) + offset;
	return 0;
}

uint32_t pcie_dw_conf_read(const struct device *dev, pcie_bdf_t bdf, unsigned int reg)
{
	struct pcie_dw_data *data = DEV_DATA(dev);
	k_spinlock_key_t key;
	mm_reg_t address;
	uint32_t value = UINT32_MAX;

	key = k_spin_lock(&data->cfg_lock);
	if (pcie_dw_map_bus(dev, bdf, reg, &address) == 0) {
		value = sys_read32(address);
	}
	k_spin_unlock(&data->cfg_lock, key);

	return value;
}

void pcie_dw_conf_write(const struct device *dev, pcie_bdf_t bdf, unsigned int reg, uint32_t value)
{
	struct pcie_dw_data *data = DEV_DATA(dev);
	k_spinlock_key_t key;
	mm_reg_t address;

	key = k_spin_lock(&data->cfg_lock);
	if (pcie_dw_map_bus(dev, bdf, reg, &address) == 0) {
		sys_write32(value, address);
	}
	k_spin_unlock(&data->cfg_lock, key);
}

static enum pcie_dw_region_type pcie_dw_select_region(const struct pcie_dw_data *data, bool mem,
						      bool mem64)
{
	if (!mem) {
		return PCIE_DW_REGION_IO;
	}

	if ((mem64 && data->regions[PCIE_DW_REGION_MEM64].size != 0U) ||
	    (data->regions[PCIE_DW_REGION_MEM].size == 0U &&
	     data->regions[PCIE_DW_REGION_MEM64].size != 0U)) {
		return PCIE_DW_REGION_MEM64;
	}

	return PCIE_DW_REGION_MEM;
}

static bool pcie_dw_region_is_available(const struct pcie_dw_data *data,
					enum pcie_dw_region_type type)
{
	return (type < PCIE_DW_REGION_COUNT) && (data->regions[type].size != 0U);
}

/*
 * The generic enumerator programs only the non-prefetchable memory window of
 * a bridge, so a 64-bit BAR behind a bridge has to be placed in the 32-bit
 * memory range to be forwarded by that window.
 */
static enum pcie_dw_region_type pcie_dw_select_region_for_bdf(const struct pcie_dw_data *data,
							      pcie_bdf_t bdf, bool mem, bool mem64)
{
	enum pcie_dw_region_type type = pcie_dw_select_region(data, mem, mem64);

	if (mem64 && type == PCIE_DW_REGION_MEM64 && PCIE_BDF_TO_BUS(bdf) > data->root_bus + 1U &&
	    data->regions[PCIE_DW_REGION_MEM].size != 0U) {
		type = PCIE_DW_REGION_MEM;
	}

	return type;
}

bool pcie_dw_region_allocate(const struct device *dev, pcie_bdf_t bdf, bool mem, bool mem64,
			     size_t bar_size, uintptr_t *bar_bus_addr)
{
	struct pcie_dw_data *data = dev->data;
	enum pcie_dw_region_type type = pcie_dw_select_region_for_bdf(data, bdf, mem, mem64);
	struct pcie_dw_region *region;
	uintptr_t current;
	uintptr_t address;
	uint64_t bar_end;
	size_t used;

	if ((!mem && mem64) || bar_bus_addr == NULL || !IS_POWER_OF_TWO(bar_size) ||
	    !pcie_dw_region_is_available(data, type)) {
		return false;
	}

	region = &data->regions[type];
	if (!pcie_dw_addr_add(region->bus_start, region->allocation_offset, &current)) {
		return false;
	}
	address = ROUND_UP(current, bar_size);
	if (address < current) {
		return false;
	}

	used = address - region->bus_start;
	if (used > region->size || bar_size > (region->size - used)) {
		return false;
	}

	/* A 32-bit BAR cannot be programmed with an address above 4 GiB. */
	if (!mem64 || type == PCIE_DW_REGION_MEM) {
		bar_end = (uint64_t)address + bar_size;
		if (bar_end > (uint64_t)UINT32_MAX + 1U) {
			return false;
		}
	}

	*bar_bus_addr = address;
	region->allocation_offset = used + bar_size;
	return true;
}

bool pcie_dw_region_get_allocate_base(const struct device *dev, pcie_bdf_t bdf, bool mem,
				      bool mem64, size_t align, uintptr_t *bar_base_addr)
{
	struct pcie_dw_data *data = dev->data;
	enum pcie_dw_region_type type = pcie_dw_select_region_for_bdf(data, bdf, mem, mem64);
	struct pcie_dw_region *region;
	uintptr_t current;
	size_t pad;

	if ((!mem && mem64) || bar_base_addr == NULL || !IS_POWER_OF_TWO(align) ||
	    !pcie_dw_region_is_available(data, type)) {
		return false;
	}

	region = &data->regions[type];
	if (region->allocation_offset >= region->size) {
		return false;
	}
	if (!pcie_dw_addr_add(region->bus_start, region->allocation_offset, &current)) {
		return false;
	}

	/*
	 * Align the base without computing the exclusive end of the range: the
	 * end can be one past UINTPTR_MAX for ranges that stop at the top of
	 * the bus address space.
	 */
	pad = (align - (current & (align - 1U))) & (align - 1U);
	if (!pcie_dw_addr_add(current, pad, &current)) {
		return false;
	}
	if (current < region->bus_start || (size_t)(current - region->bus_start) >= region->size) {
		return false;
	}

	/*
	 * Commit the aligned base so that later allocations start at the bridge
	 * window base instead of overlapping the unaligned remainder.
	 */
	region->allocation_offset = (size_t)(current - region->bus_start);
	*bar_base_addr = current;

	return true;
}

bool pcie_dw_region_translate(const struct device *dev, pcie_bdf_t bdf, bool mem, bool mem64,
			      uintptr_t bar_bus_addr, uintptr_t *bar_addr)
{
	struct pcie_dw_data *data = dev->data;
	enum pcie_dw_region_type type = pcie_dw_select_region_for_bdf(data, bdf, mem, mem64);
	const struct pcie_dw_region *region;
	uintptr_t offset;

	if ((!mem && mem64) || bar_addr == NULL || !pcie_dw_region_is_available(data, type)) {
		return false;
	}

	region = &data->regions[type];
	if (bar_bus_addr < region->bus_start ||
	    ((bar_bus_addr - region->bus_start) >= region->size)) {
		return false;
	}

	offset = bar_bus_addr - region->bus_start;
	if (!pcie_dw_addr_add(region->phys_start, offset, bar_addr)) {
		return false;
	}

	return true;
}

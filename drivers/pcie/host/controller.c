/*
 * Copyright (c) 2021 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(pcie_core, LOG_LEVEL_INF);

#include <kernel.h>
#include <drivers/pcie/pcie.h>
#include <drivers/pcie/controller.h>

#if CONFIG_PCIE_MSI
#include <drivers/pcie/msi.h>
#endif

/* arch agnostic PCIe API implementation */

uint32_t pcie_conf_read(pcie_bdf_t bdf, unsigned int reg)
{
	const struct device *dev;

	dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_pcie_controller));
	if (!dev) {
		LOG_ERR("Failed to get PCIe root complex");
		return 0xffffffff;
	}

	return pcie_ctrl_conf_read(dev, bdf, reg);
}

void pcie_conf_write(pcie_bdf_t bdf, unsigned int reg, uint32_t data)
{
	const struct device *dev;

	dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_pcie_controller));
	if (!dev) {
		LOG_ERR("Failed to get PCIe root complex");
		return;
	}

	pcie_ctrl_conf_write(dev, bdf, reg, data);
}

uint32_t generic_pcie_ctrl_conf_read(mm_reg_t cfg_addr, pcie_bdf_t bdf, unsigned int reg)
{
	volatile uint32_t *bdf_cfg_mem = (volatile uint32_t *)((uintptr_t)cfg_addr + (bdf << 4));

	if (!cfg_addr) {
		return 0xffffffff;
	}

	return bdf_cfg_mem[reg];
}

void generic_pcie_ctrl_conf_write(mm_reg_t cfg_addr, pcie_bdf_t bdf,
				 unsigned int reg, uint32_t data)
{
	volatile uint32_t *bdf_cfg_mem = (volatile uint32_t *)((uintptr_t)cfg_addr + (bdf << 4));

	if (!cfg_addr) {
		return;
	}

	bdf_cfg_mem[reg] = data;
}

static void generic_pcie_ctrl_enumerate_type1(const struct device *ctrl_dev, pcie_bdf_t bdf)
{
	/* Not yet supported */
}

static void generic_pcie_ctrl_type0_enumerate_bars(const struct device *ctrl_dev, pcie_bdf_t bdf)
{
	unsigned int bar, reg, data;
	uintptr_t scratch, bar_bus_addr;
	size_t size, bar_size;

	for (bar = 0, reg = PCIE_CONF_BAR0; reg <= PCIE_CONF_BAR5; reg ++, bar++) {
		bool found_mem64 = false;
		bool found_mem = false;

		data = scratch = pcie_conf_read(bdf, reg);

		if (PCIE_CONF_BAR_INVAL_FLAGS(data)) {
			continue;
		}

		if (PCIE_CONF_BAR_MEM(data)) {
			found_mem = true;
			if (PCIE_CONF_BAR_64(data)) {
				found_mem64 = true;
				scratch |= ((uint64_t)pcie_conf_read(bdf, reg + 1)) << 32;
				if (PCIE_CONF_BAR_ADDR(scratch) == PCIE_CONF_BAR_INVAL64) {
					continue;
				}
			} else {
				if (PCIE_CONF_BAR_ADDR(scratch) == PCIE_CONF_BAR_INVAL) {
					continue;
				}
			}
		}

		pcie_conf_write(bdf, reg, 0xFFFFFFFF);
		size = pcie_conf_read(bdf, reg);
		pcie_conf_write(bdf, reg, scratch & 0xFFFFFFFF);

		if (found_mem64) {
			pcie_conf_write(bdf, reg + 1, 0xFFFFFFFF);
			size |= ((uint64_t)pcie_conf_read(bdf, reg + 1)) << 32;
			pcie_conf_write(bdf, reg + 1, scratch >> 32);
		}

		if (!PCIE_CONF_BAR_ADDR(size)) {
			if (found_mem64) {
				reg++;
			}
			continue;
		}

		if (found_mem) {
			if (found_mem64) {
				bar_size = (uint64_t)~PCIE_CONF_BAR_ADDR(size) + 1;
			} else {
				bar_size = (uint32_t)~PCIE_CONF_BAR_ADDR(size) + 1;
			}
		} else {
			bar_size = (uint32_t)~PCIE_CONF_BAR_IO_ADDR(size) + 1;
		}

		if (pcie_ctrl_region_allocate(ctrl_dev, bdf, found_mem,
					      found_mem64, bar_size, &bar_bus_addr)) {
			uintptr_t bar_phys_addr;

			pcie_ctrl_region_xlate(ctrl_dev, bdf, found_mem,
					      found_mem64, bar_bus_addr, &bar_phys_addr);

			LOG_INF("[%02x:%02x.%x] BAR%d size 0x%lx "
				"assigned [%s 0x%lx-0x%lx -> 0x%lx-0x%lx]",
				PCIE_BDF_TO_BUS(bdf), PCIE_BDF_TO_DEV(bdf), PCIE_BDF_TO_FUNC(bdf),
				bar, bar_size,
				found_mem ? (found_mem64 ? "mem64" : "mem") : "io",
				bar_bus_addr, bar_bus_addr + bar_size - 1,
				bar_phys_addr, bar_phys_addr + bar_size - 1);

			pcie_conf_write(bdf, reg, bar_bus_addr & 0xFFFFFFFF);
			if (found_mem64) {
				pcie_conf_write(bdf, reg + 1, bar_bus_addr >> 32);
			}
		} else {
			LOG_INF("[%02x:%02x.%x] BAR%d size 0x%lx Failed memory allocation.",
				PCIE_BDF_TO_BUS(bdf), PCIE_BDF_TO_DEV(bdf), PCIE_BDF_TO_FUNC(bdf),
				bar, bar_size);
		}

		if (found_mem64) {
			reg++;
		}
	}
}

static void generic_pcie_ctrl_enumerate_type0(const struct device *ctrl_dev, pcie_bdf_t bdf)
{
	/* Setup Type0 BARs */
	generic_pcie_ctrl_type0_enumerate_bars(ctrl_dev, bdf);
}

void generic_pcie_ctrl_enumerate(const struct device *ctrl_dev, pcie_bdf_t bdf_start)
{
	uint32_t data, class, id;
	unsigned int dev = PCIE_BDF_TO_DEV(bdf_start),
		     func = 0,
		     bus = PCIE_BDF_TO_BUS(bdf_start);

	for (; dev <= PCIE_MAX_DEV; dev++) {
		func = 0;
		for (; func <= PCIE_MAX_FUNC; func++) {
			pcie_bdf_t bdf = PCIE_BDF(bus, dev, func);
			bool multifunction_device = false;
			bool layout_type_1 = false;

			id = pcie_conf_read(bdf, PCIE_CONF_ID);
			if (id == PCIE_ID_NONE) {
				continue;
			}

			class = pcie_conf_read(bdf, PCIE_CONF_CLASSREV);
			data = pcie_conf_read(bdf, PCIE_CONF_TYPE);

			multifunction_device = PCIE_CONF_MULTIFUNCTION(data);
			layout_type_1 = PCIE_CONF_TYPE_BRIDGE(data);

			LOG_INF("[%02x:%02x.%x] %04x:%04x class %x subclass %x progif %x "
				"rev %x Type%x multifunction %s",
				PCIE_BDF_TO_BUS(bdf), PCIE_BDF_TO_DEV(bdf), PCIE_BDF_TO_FUNC(bdf),
				id & 0xffff, id >> 16,
				PCIE_CONF_CLASSREV_CLASS(class),
				PCIE_CONF_CLASSREV_SUBCLASS(class),
				PCIE_CONF_CLASSREV_PROGIF(class),
				PCIE_CONF_CLASSREV_REV(class),
				layout_type_1 ? 1 : 0,
				multifunction_device ? "true" : "false");

			if (layout_type_1) {
				generic_pcie_ctrl_enumerate_type1(ctrl_dev, bdf);
			} else {
				generic_pcie_ctrl_enumerate_type0(ctrl_dev, bdf);
			}

			/* Do not enumerate sub-functions if not a multifunction device */
			if (PCIE_BDF_TO_FUNC(bdf) == 0 && !multifunction_device) {
				break;
			}
		}
	}
}

/*
 * Copyright (c) 2020 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT intel_vt_d

#include <errno.h>

#include <kernel.h>
#include <arch/cpu.h>

#include <soc.h>
#include <device.h>
#include <init.h>
#include <string.h>

#include <zephyr.h>

#include <arch/x86/intel_vtd.h>
#include <drivers/interrupt_controller/intel_vtd.h>

#include "intc_intel_vtd.h"

static void vtd_write_reg64(const struct device *dev,
			    uint16_t reg, uint64_t value)
{
	uintptr_t base_address = DEVICE_MMIO_GET(dev);

	sys_write64(value, (base_address + reg));
}

static uint32_t vtd_read_reg(const struct device *dev, uint16_t reg)
{
	uintptr_t base_address = DEVICE_MMIO_GET(dev);

	return sys_read32(base_address + reg);
}

static void vtd_send_cmd(const struct device *dev,
			 uint16_t cmd_bit, uint16_t status_bit)
{
	uintptr_t base_address = DEVICE_MMIO_GET(dev);

	sys_set_bit((base_address + VTD_GCMD_REG), cmd_bit);

	while (!sys_test_bit((base_address + VTD_GSTS_REG),
			     status_bit)) {
		/* Do nothing */
	}

}

static int vtd_ictl_allocate_entries(const struct device *dev,
				     uint8_t n_entries)
{
	struct vtd_ictl_data *data = dev->data;
	int irte_idx_start;

	if ((data->irte_num_used + n_entries) > IRTE_NUM) {
		return -EBUSY;
	}

	irte_idx_start = data->irte_num_used;
	data->irte_num_used += n_entries;

	return irte_idx_start;
}

static uint32_t vtd_ictl_remap_msi(const struct device *dev,
				   msi_vector_t *vector)
{
	return VTD_MSI_MAP(vector->arch.irte);
}

static int vtd_ictl_remap(const struct device *dev,
			  msi_vector_t *vector)
{
	struct vtd_ictl_data *data = dev->data;
	uint8_t irte_idx = vector->arch.irte;

	memset(&data->irte[irte_idx], 0, sizeof(struct vtd_irte));

	data->irte[irte_idx].l.vector = vector->arch.vector;
	data->irte[irte_idx].l.dst_id = arch_curr_cpu()->id;
	data->irte[irte_idx].l.present = 1;

	return 0;
}

static int vtd_ictl_init(const struct device *dev)
{
	struct vtd_ictl_data *data = dev->data;
	unsigned int key = irq_lock();
	uint64_t eime = 0;
	uint64_t irta;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	if (IS_ENABLED(CONFIG_X2APIC)) {
		eime = VTD_IRTA_EIME;
	}

	irta = VTD_IRTA_REG_GEN_CONTENT((uintptr_t)data->irte,
					IRTA_SIZE, eime);

	vtd_write_reg64(dev, VTD_IRTA_REG, irta);

	vtd_send_cmd(dev, VTD_GCMD_SIRTP, VTD_GSTS_SIRTPS);
	vtd_send_cmd(dev, VTD_GCMD_IRE, VTD_GSTS_IRES);

	printk("Intel VT-D up and running (status 0x%x)\n",
	       vtd_read_reg(dev, VTD_GSTS_REG));

	irq_unlock(key);

	return 0;
}

static const struct vtd_driver_api vtd_api = {
	.allocate_entries = vtd_ictl_allocate_entries,
	.remap_msi = vtd_ictl_remap_msi,
	.remap = vtd_ictl_remap,
};

static struct vtd_ictl_data vtd_ictl_data_0;

static const struct vtd_ictl_cfg vtd_ictl_cfg_0 = {
	DEVICE_MMIO_ROM_INIT(DT_DRV_INST(0)),
};

DEVICE_DT_INST_DEFINE(0,
	      vtd_ictl_init, device_pm_control_nop,
	      &vtd_ictl_data_0, &vtd_ictl_cfg_0,
	      PRE_KERNEL_1, CONFIG_INTEL_VTD_ICTL_INIT_PRIORITY, &vtd_api);

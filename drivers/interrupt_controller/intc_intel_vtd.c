/*
 * Copyright (c) 2020 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT intel_vt_d

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>

#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <string.h>


#include <zephyr/cache.h>

#include <zephyr/arch/x86/intel_vtd.h>
#include <zephyr/drivers/interrupt_controller/intel_vtd.h>
#include <zephyr/drivers/interrupt_controller/ioapic.h>
#include <zephyr/drivers/interrupt_controller/loapic.h>
#include <zephyr/drivers/pcie/msi.h>

#include <kernel_arch_func.h>

#include "intc_intel_vtd.h"

static inline void vtd_pause_cpu(void)
{
	__asm__ volatile("pause" ::: "memory");
}

static void vtd_write_reg32(const struct device *dev,
			    uint16_t reg, uint32_t value)
{
	uintptr_t base_address = DEVICE_MMIO_GET(dev);

	sys_write32(value, (base_address + reg));
}

static uint32_t vtd_read_reg32(const struct device *dev, uint16_t reg)
{
	uintptr_t base_address = DEVICE_MMIO_GET(dev);

	return sys_read32(base_address + reg);
}

static void vtd_write_reg64(const struct device *dev,
			    uint16_t reg, uint64_t value)
{
	uintptr_t base_address = DEVICE_MMIO_GET(dev);

	sys_write64(value, (base_address + reg));
}

static uint64_t vtd_read_reg64(const struct device *dev, uint16_t reg)
{
	uintptr_t base_address = DEVICE_MMIO_GET(dev);

	return sys_read64(base_address + reg);
}

static void vtd_send_cmd(const struct device *dev,
			 uint16_t cmd_bit, uint16_t status_bit)
{
	uintptr_t base_address = DEVICE_MMIO_GET(dev);
	uint32_t value;

	value = vtd_read_reg32(dev, VTD_GSTS_REG);
	value |= BIT(cmd_bit);

	vtd_write_reg32(dev, VTD_GCMD_REG, value);

	while (!sys_test_bit((base_address + VTD_GSTS_REG),
			     status_bit)) {
		/* Do nothing */
	}
}

static void vtd_flush_irte_from_cache(const struct device *dev,
				      uint8_t irte_idx)
{
	struct vtd_ictl_data *data = dev->data;

	if (!data->pwc) {
		cache_data_flush_range(&data->irte[irte_idx],
				       sizeof(union vtd_irte));
	}
}

static void vtd_qi_init(const struct device *dev)
{
	struct vtd_ictl_data *data = dev->data;
	uint64_t value;

	vtd_write_reg64(dev, VTD_IQT_REG, 0);
	data->qi_tail = 0;

	value = VTD_IQA_REG_GEN_CONTENT((uintptr_t)data->qi,
					VTD_IQA_WIDTH_128_BIT, QI_SIZE);
	vtd_write_reg64(dev, VTD_IQA_REG, value);

	vtd_send_cmd(dev, VTD_GCMD_QIE, VTD_GSTS_QIES);
}

static inline void vtd_qi_tail_inc(const struct device *dev)
{
	struct vtd_ictl_data *data = dev->data;

	data->qi_tail += sizeof(struct qi_descriptor);
	data->qi_tail %= (QI_NUM * sizeof(struct qi_descriptor));
}

static int vtd_qi_send(const struct device *dev,
		       struct qi_descriptor *descriptor)
{
	struct vtd_ictl_data *data = dev->data;
	union qi_wait_descriptor wait_desc = { 0 };
	struct qi_descriptor *desc;
	uint32_t wait_status;
	uint32_t wait_count;

	desc = (struct qi_descriptor *)((uintptr_t)data->qi + data->qi_tail);

	desc->low = descriptor->low;
	desc->high = descriptor->high;

	vtd_qi_tail_inc(dev);

	desc++;

	wait_status = QI_WAIT_STATUS_INCOMPLETE;

	wait_desc.wait.type = QI_TYPE_WAIT;
	wait_desc.wait.status_write = 1;
	wait_desc.wait.status_data = QI_WAIT_STATUS_COMPLETE;
	wait_desc.wait.address = ((uintptr_t)&wait_status) >> 2;

	desc->low = wait_desc.desc.low;
	desc->high = wait_desc.desc.high;

	vtd_qi_tail_inc(dev);

	vtd_write_reg64(dev, VTD_IQT_REG, data->qi_tail);

	wait_count = 0;

	while (wait_status != QI_WAIT_STATUS_COMPLETE) {
		/* We cannot use timeout here, this function being called
		 * at init time, it might result that the system clock
		 * is not initialized yet since VT-D init comes first.
		 */
		if (wait_count > QI_WAIT_COUNT_LIMIT) {
			printk("QI timeout\n");
			return -ETIME;
		}

		if (vtd_read_reg32(dev, VTD_FSTS_REG) & VTD_FSTS_IQE) {
			printk("QI error\n");
			return -EIO;
		}

		vtd_pause_cpu();
		wait_count++;
	}

	return 0;
}

static int vtd_global_cc_invalidate(const struct device *dev)
{
	union qi_icc_descriptor iec_desc = { 0 };

	iec_desc.icc.type = QI_TYPE_ICC;
	iec_desc.icc.granularity = 1; /* Global Invalidation requested */

	return vtd_qi_send(dev, &iec_desc.desc);
}

static int vtd_global_iec_invalidate(const struct device *dev)
{
	union qi_iec_descriptor iec_desc = { 0 };

	iec_desc.iec.type = QI_TYPE_IEC;
	iec_desc.iec.granularity = 0; /* Global Invalidation requested */

	return vtd_qi_send(dev, &iec_desc.desc);
}

static int vtd_index_iec_invalidate(const struct device *dev, uint8_t irte_idx)
{
	union qi_iec_descriptor iec_desc = { 0 };

	iec_desc.iec.type = QI_TYPE_IEC;
	iec_desc.iec.granularity = 1; /* Index based invalidation requested */

	iec_desc.iec.interrupt_index = irte_idx;
	iec_desc.iec.index_mask = 0;

	return vtd_qi_send(dev, &iec_desc.desc);
}

static void fault_status_description(uint32_t status)
{
	if (status & VTD_FSTS_PFO) {
		printk("Primary Fault Overflow (PFO)\n");
	}

	if (status & VTD_FSTS_AFO) {
		printk("Advanced Fault Overflow (AFO)\n");
	}

	if (status & VTD_FSTS_APF) {
		printk("Advanced Primary Fault (APF)\n");
	}

	if (status & VTD_FSTS_IQE) {
		printk("Invalidation Queue Error (IQE)\n");
	}

	if (status & VTD_FSTS_ICE) {
		printk("Invalidation Completion Error (ICE)\n");
	}

	if (status & VTD_FSTS_ITE) {
		printk("Invalidation Timeout Error\n");
	}

	if (status & VTD_FSTS_PPF) {
		printk("Primary Pending Fault (PPF) %u\n",
		       VTD_FSTS_FRI(status));
	}
}

static void fault_record_description(uint64_t low, uint64_t high)
{
	printk("Fault %s request: Reason 0x%x info 0x%llx src 0x%x\n",
	       (high & VTD_FRCD_T) ? "Read/Atomic" : "Write/Page",
	       VTD_FRCD_FR(high), VTD_FRCD_FI(low), VTD_FRCD_SID(high));
}

static void fault_event_isr(const void *arg)
{
	const struct device *dev = arg;
	struct vtd_ictl_data *data = dev->data;
	uint32_t status;
	uint8_t f_idx;

	status = vtd_read_reg32(dev, VTD_FSTS_REG);
	fault_status_description(status);

	if (!(status & VTD_FSTS_PPF)) {
		goto out;
	}

	f_idx = VTD_FSTS_FRI(status);
	while (f_idx < data->fault_record_num) {
		uint64_t fault_l, fault_h;

		/* Reading fault's 64 lowest bits */
		fault_l = vtd_read_reg64(dev, data->fault_record_reg +
					 (VTD_FRCD_REG_SIZE * f_idx));
		/* Reading fault's 64 highest bits */
		fault_h = vtd_read_reg64(dev, data->fault_record_reg +
				       (VTD_FRCD_REG_SIZE * f_idx) + 8);

		if (fault_h & VTD_FRCD_F) {
			fault_record_description(fault_l, fault_h);
		}

		/* Clearing the fault */
		vtd_write_reg64(dev, data->fault_record_reg +
				(VTD_FRCD_REG_SIZE * f_idx), fault_l);
		vtd_write_reg64(dev, data->fault_record_reg +
				(VTD_FRCD_REG_SIZE * f_idx) + 8, fault_h);
		f_idx++;
	}
out:
	/* Clearing fault status */
	vtd_write_reg32(dev, VTD_FSTS_REG, VTD_FSTS_CLEAR(status));
}

static void vtd_fault_event_init(const struct device *dev)
{
	struct vtd_ictl_data *data = dev->data;
	uint64_t value;
	uint32_t reg;

	value = vtd_read_reg64(dev, VTD_CAP_REG);
	data->fault_record_num = VTD_CAP_NFR(value) + 1;
	data->fault_record_reg = DEVICE_MMIO_GET(dev) +
		(uintptr_t)(16 * VTD_CAP_FRO(value));

	/* Allocating IRQ & vector and connecting the ISR handler,
	 * by-passing remapping by using x86 functions directly.
	 */
	data->fault_irq = arch_irq_allocate();
	data->fault_vector = z_x86_allocate_vector(0, -1);

	vtd_write_reg32(dev, VTD_FEDATA_REG, data->fault_vector);
	vtd_write_reg32(dev, VTD_FEADDR_REG,
			pcie_msi_map(data->fault_irq, NULL, 0));
	vtd_write_reg32(dev, VTD_FEUADDR_REG, 0);

	z_x86_irq_connect_on_vector(data->fault_irq, data->fault_vector,
				    fault_event_isr, dev);

	vtd_write_reg32(dev, VTD_FSTS_REG,
			VTD_FSTS_CLEAR(vtd_read_reg32(dev, VTD_FSTS_REG)));

	/* Unmasking interrupts */
	reg = vtd_read_reg32(dev, VTD_FECTL_REG);
	reg &= ~BIT(VTD_FECTL_REG_IM);
	vtd_write_reg32(dev, VTD_FECTL_REG, reg);
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
				   msi_vector_t *vector,
				   uint8_t n_vector)
{
	uint32_t shv = (n_vector > 1) ? VTD_INT_SHV : 0;

	return VTD_MSI_MAP(vector->arch.irte, shv);
}

static int vtd_ictl_remap(const struct device *dev,
			  uint8_t irte_idx,
			  uint16_t vector,
			  uint32_t flags,
			  uint16_t src_id)
{
	struct vtd_ictl_data *data = dev->data;
	union vtd_irte irte = { 0 };
	uint32_t delivery_mode;

	irte.bits.vector = vector;

	if (IS_ENABLED(CONFIG_X2APIC)) {
		/* Getting the logical APIC ID */
		irte.bits.dst_id = x86_read_loapic(LOAPIC_LDR);
	} else {
		/* As for IOAPIC: let's mask all possible IDs */
		irte.bits.dst_id = 0xFF << 8;
	}

	if (src_id != USHRT_MAX &&
	    !IS_ENABLED(CONFIG_INTEL_VTD_ICTL_NO_SRC_ID_CHECK)) {
		irte.bits.src_validation_type = 1;
		irte.bits.src_id = src_id;
	}

	delivery_mode = (flags & IOAPIC_DELIVERY_MODE_MASK);
	if ((delivery_mode != IOAPIC_FIXED) ||
	    (delivery_mode != IOAPIC_LOW)) {
		delivery_mode = IOAPIC_LOW;
	}

	irte.bits.trigger_mode = (flags & IOAPIC_TRIGGER_MASK) >> 15;
	irte.bits.delivery_mode = delivery_mode >> 8;
	irte.bits.redirection_hint = 1;
	irte.bits.dst_mode = 1; /* Always logical */
	irte.bits.present = 1;

	data->irte[irte_idx].parts.low = irte.parts.low;
	data->irte[irte_idx].parts.high = irte.parts.high;

	vtd_index_iec_invalidate(dev, irte_idx);

	vtd_flush_irte_from_cache(dev, irte_idx);

	return 0;
}

static int vtd_ictl_set_irte_vector(const struct device *dev,
				    uint8_t irte_idx,
				    uint16_t vector)
{
	struct vtd_ictl_data *data = dev->data;

	data->vectors[irte_idx] = vector;

	return 0;
}

static int vtd_ictl_get_irte_by_vector(const struct device *dev,
				       uint16_t vector)
{
	struct vtd_ictl_data *data = dev->data;
	int irte_idx;

	for (irte_idx = 0; irte_idx < IRTE_NUM; irte_idx++) {
		if (data->vectors[irte_idx] == vector) {
			return irte_idx;
		}
	}

	return -EINVAL;
}

static uint16_t vtd_ictl_get_irte_vector(const struct device *dev,
					 uint8_t irte_idx)
{
	struct vtd_ictl_data *data = dev->data;

	return data->vectors[irte_idx];
}

static int vtd_ictl_set_irte_irq(const struct device *dev,
				 uint8_t irte_idx,
				 unsigned int irq)
{
	struct vtd_ictl_data *data = dev->data;

	data->irqs[irte_idx] = irq;

	return 0;
}

static int vtd_ictl_get_irte_by_irq(const struct device *dev,
				    unsigned int irq)
{
	struct vtd_ictl_data *data = dev->data;
	int irte_idx;

	for (irte_idx = 0; irte_idx < IRTE_NUM; irte_idx++) {
		if (data->irqs[irte_idx] == irq) {
			return irte_idx;
		}
	}

	return -EINVAL;
}

static void vtd_ictl_set_irte_msi(const struct device *dev,
				  uint8_t irte_idx, bool msi)
{
	struct vtd_ictl_data *data = dev->data;

	data->msi[irte_idx] = msi;
}

static bool vtd_ictl_irte_is_msi(const struct device *dev,
				 uint8_t irte_idx)
{
	struct vtd_ictl_data *data = dev->data;

	return data->msi[irte_idx];
}

static int vtd_ictl_init(const struct device *dev)
{
	struct vtd_ictl_data *data = dev->data;
	unsigned int key = irq_lock();
	uint64_t eime = 0;
	uint64_t value;
	int ret = 0;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	if (vtd_read_reg64(dev, VTD_ECAP_REG) & VTD_ECAP_C) {
		printk("Page walk coherency supported\n");
		data->pwc = true;
	}

	vtd_fault_event_init(dev);

	vtd_qi_init(dev);

	if (vtd_global_cc_invalidate(dev) != 0) {
		printk("Could not perform ICC invalidation\n");
		ret = -EIO;
		goto out;
	}

	if (IS_ENABLED(CONFIG_X2APIC)) {
		eime = VTD_IRTA_EIME;
	}

	value = VTD_IRTA_REG_GEN_CONTENT((uintptr_t)data->irte,
					 IRTA_SIZE, eime);

	vtd_write_reg64(dev, VTD_IRTA_REG, value);

	if (vtd_global_iec_invalidate(dev) != 0) {
		printk("Could not perform IEC invalidation\n");
		ret = -EIO;
		goto out;
	}

	if (!IS_ENABLED(CONFIG_X2APIC) &&
	    IS_ENABLED(CONFIG_INTEL_VTD_ICTL_XAPIC_PASSTHROUGH)) {
		vtd_send_cmd(dev, VTD_GCMD_CFI, VTD_GSTS_CFIS);
	}

	vtd_send_cmd(dev, VTD_GCMD_SIRTP, VTD_GSTS_SIRTPS);
	vtd_send_cmd(dev, VTD_GCMD_IRE, VTD_GSTS_IRES);

	printk("Intel VT-D up and running (status 0x%x)\n",
	       vtd_read_reg32(dev, VTD_GSTS_REG));

out:
	irq_unlock(key);

	return ret;
}

static DEVICE_API(vtd, vtd_api) = {
	.allocate_entries = vtd_ictl_allocate_entries,
	.remap_msi = vtd_ictl_remap_msi,
	.remap = vtd_ictl_remap,
	.set_irte_vector = vtd_ictl_set_irte_vector,
	.get_irte_by_vector = vtd_ictl_get_irte_by_vector,
	.get_irte_vector = vtd_ictl_get_irte_vector,
	.set_irte_irq = vtd_ictl_set_irte_irq,
	.get_irte_by_irq = vtd_ictl_get_irte_by_irq,
	.set_irte_msi = vtd_ictl_set_irte_msi,
	.irte_is_msi = vtd_ictl_irte_is_msi
};

static struct vtd_ictl_data vtd_ictl_data_0 = {
	.irqs = { -EINVAL },
	.vectors = { -EINVAL },
};

static const struct vtd_ictl_cfg vtd_ictl_cfg_0 = {
	DEVICE_MMIO_ROM_INIT(DT_DRV_INST(0)),
};

DEVICE_DT_INST_DEFINE(0,
	      vtd_ictl_init, NULL,
	      &vtd_ictl_data_0, &vtd_ictl_cfg_0,
	      PRE_KERNEL_1, CONFIG_INTEL_VTD_ICTL_INIT_PRIORITY, &vtd_api);

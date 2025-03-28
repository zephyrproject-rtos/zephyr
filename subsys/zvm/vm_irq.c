/*
 * Copyright 2024-2025 HNU-ESNL: Guoqi Xie, Chenglai Xiong, Xingyu Hu and etc.
 * Copyright 2024-2025 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/dt-bindings/interrupt-controller/arm-gic.h>
#include <zephyr/zvm/vdev/vgic_common.h>
#include <zephyr/zvm/vdev/vgic_v3.h>
#include <zephyr/zvm/vm_irq.h>
#include <zephyr/zvm/zvm.h>

LOG_MODULE_DECLARE(ZVM_MODULE_NAME);

#define VWFI_YIELD_THRESHOLD	100

bool vcpu_irq_exist(struct z_vcpu *vcpu)
{
	bool pend, active;
	struct vcpu_virt_irq_block *vb = &vcpu->virq_block;

	pend = sys_dlist_is_empty(&vb->pending_irqs);
	active = sys_dlist_is_empty(&vb->active_irqs);

	if ((!(pend && active)) || virt_irq_ispending(vcpu)) {
		return true;
	}

	return false;
}

int vcpu_wait_for_irq(struct z_vcpu *vcpu)
{
	bool irq_exist, vcpu_will_yeild = false, vcpu_will_pause = false;
	k_spinlock_key_t key;

	/* judge whether the vcpu has pending or active irq */
	irq_exist = vcpu_irq_exist(vcpu);
	key = k_spin_lock(&vcpu->virq_block.vwfi.wfi_lock);

	if (irq_exist) {
		vcpu->virq_block.vwfi.yeild_count = 0;
		goto done;
	} else if (vcpu->virq_block.vwfi.yeild_count < VWFI_YIELD_THRESHOLD) {
		vcpu->virq_block.vwfi.yeild_count++;
		vcpu_will_yeild = true;
		goto done;
	}

	if (!vcpu->virq_block.vwfi.state) {
		vcpu_will_pause = true;
		vcpu->virq_block.vwfi.state = true;
		/*start wfi timeout*/
	}

done:
	k_spin_unlock(&vcpu->virq_block.vwfi.wfi_lock, key);

	if (vcpu_will_yeild) {
		/*yeild this thread*/
	}

	if (vcpu_will_pause) {
		irq_exist = vcpu_irq_exist(vcpu);
		if (irq_exist) {
			key = k_spin_lock(&vcpu->virq_block.vwfi.wfi_lock);
			vcpu->virq_block.vwfi.yeild_count = 0;
			vcpu->virq_block.vwfi.state = false;
			/*end wfi timeout*/
			k_spin_unlock(&vcpu->virq_block.vwfi.wfi_lock, key);
		}
	}

	return 0;

}

/**
 * @brief Init call for creating interrupt control block for vm.
 */
static int vm_irq_ctrlblock_create(struct device *unused, struct z_vm *vm)
{
	ARG_UNUSED(unused);
	struct vm_virt_irq_block *vvi_block = &vm->vm_irq_block;

	if (VGIC_TYPER_LR_NUM != 0) {
		vvi_block->flags = 0;
		vvi_block->flags |= VIRQ_HW_SUPPORT;
	} else {
		ZVM_LOG_ERR("Init gicv3 failed, the hardware do not supporte it.\n");
		return -ENODEV;
	}

	vvi_block->enabled = false;
	vvi_block->cpu_num = vm->vcpu_num;
	vvi_block->irq_num = VM_GLOBAL_VIRQ_NR;
	memset(
		vvi_block->ipi_vcpu_source, 0,
		sizeof(uint32_t)*CONFIG_MP_MAX_NUM_CPUS*VM_SGI_VIRQ_NR);
	memset(vvi_block->irq_bitmap, 0, VM_GLOBAL_VIRQ_NR/0x08);

	return 0;
}

/**
 * @brief Init virq descs for each vm. For It obtains some
 * device irq which is shared by all cores,
 * this type of interrupt is inited in this routine.
 */
static int vm_virq_desc_init(struct z_vm *vm)
{
	int i;
	struct virt_irq_desc *desc;

	for (i = 0; i < VM_SPI_VIRQ_NR; i++) {
		desc = &vm->vm_irq_block.vm_virt_irq_desc[i];

		desc->virq_flags = 0;
		/* For shared irq, it shared with all cores */
		desc->vcpu_id =  DEFAULT_VCPU;
		desc->vm_id = vm->vmid;
		desc->vdev_trigger = 0;
		desc->virq_num = i;
		desc->pirq_num = i;
		desc->id = VM_INVALID_DESC_ID;
		desc->virq_states = VIRQ_STATE_INVALID;
		desc->type = 0;

		sys_dnode_init(&(desc->desc_node));
	}

	return 0;
}

void vm_device_irq_init(struct z_vm *vm, struct z_virt_dev *vm_dev)
{
	bool *bit_map;
	struct virt_irq_desc *desc;

	desc = get_virt_irq_desc(vm->vcpus[DEFAULT_VCPU], vm_dev->virq);
	if (vm_dev->dev_pt_flag) {
		desc->virq_flags |= VIRQ_HW_FLAG;
		ZVM_LOG_INFO(
			"Add hardware interrupt support for %s device !\n",
			vm_dev->name);
	} else {
		ZVM_LOG_INFO(
			"Add software interrupt support for %s device !\n",
			vm_dev->name);
	}
	desc->id = desc->virq_num;
	desc->pirq_num = vm_dev->hirq;
	desc->virq_num = vm_dev->virq;
	/* For passthrough device, using fast irq path. */
	if (vm_dev->dev_pt_flag) {
		bit_map = vm->vm_irq_block.irq_bitmap;
		bit_map[vm_dev->hirq] = true;
	}
}

int vm_irq_block_init(struct z_vm *vm)
{
	int ret = 0;

	ret = vm_irq_ctrlblock_create(NULL, vm);
	if (ret) {
		return ret;
	}
	ret = vm_virq_desc_init(vm);

	return ret;
}

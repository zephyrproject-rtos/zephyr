/*
 * Copyright 2024-2025 HNU-ESNL: Guoqi Xie, Chenglai Xiong, Xingyu Hu and etc.
 * Copyright 2024-2025 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/arch/arm64/lib_helpers.h>
#include <zephyr/dt-bindings/interrupt-controller/arm-gic.h>
#include <../drivers/interrupt_controller/intc_gicv3_priv.h>
#include <../kernel/include/ksched.h>
#include <zephyr/zvm/arm/cpu.h>
#include <zephyr/zvm/vdev/vgic_common.h>
#include <zephyr/zvm/vdev/vgic_v3.h>
#include <zephyr/zvm/vm_irq.h>

LOG_MODULE_DECLARE(ZVM_MODULE_NAME);

static int virt_irq_set_type(struct z_vcpu *vcpu, uint32_t offset, uint32_t *value)
{
	uint8_t lowbit_value;
	int i, irq, idx_base;
	uint32_t reg_val;
	mem_addr_t base;
	struct virt_irq_desc *desc;

	idx_base = (offset - GICD_ICFGRn) / 4;
	irq = 16 * idx_base;
	base = GIC_DIST_BASE;

	/**
	 * Per-register control 16 interrupt signals.
	 * TODO: This may be more simple for reduce
	 * time.
	 */
	for (i = 0; i < 16; i++, irq++) {
		desc = vgic_get_virt_irq_desc(vcpu, irq);
		if (!desc) {
			return -ENOENT;
		}
		lowbit_value = (*value>>2*i) & GICD_ICFGR_MASK;
		if (desc->type != lowbit_value) {
			desc->type = lowbit_value;
			/* If it is a hardware device interrupt */
			if (desc->virq_flags & VIRQ_HW_FLAG) {
				reg_val = sys_read32(GICD_ICFGRn + (idx_base * 4));
				reg_val &= ~(GICD_ICFGR_MASK << 2*i);
				if (lowbit_value) {
					reg_val |= (GICD_ICFGR_TYPE << 2*i);
				}
				/* clear the enabled flag of interrupt */
				irq_disable(irq);
				sys_write32(reg_val, GICD_ICFGRn + (idx_base*4));
			}
		}
	}
	return 0;
}

/**
 * @breif: this type value is got from desc.
 * TODO: may be direct read from vgic register.
 */
static int virt_irq_get_type(struct z_vcpu *vcpu, uint32_t offset, uint32_t *value)
{
	int i, irq, idx_base;
	struct virt_irq_desc *desc;

	idx_base = (offset-GICD_ICFGRn) / 4;
	irq = 16 * idx_base;

	/*Per-register control 16 interrupt signals.*/
	for (i = 0; i < 16; i++, irq++) {
		desc = vgic_get_virt_irq_desc(vcpu, irq);
		if (!desc) {
			continue;
		}
		*value = *value | (desc->type << i * 2);
	}
	return 0;
}

/*
 * @brief Set priority for specific virtual interrupt requests
 */
static int vgic_virq_set_priority(struct z_vcpu *vcpu, uint32_t virt_irq, int prio)
{
	struct virt_irq_desc *desc;

	desc = vgic_get_virt_irq_desc(vcpu, virt_irq);
	if (!desc) {
		return -ENOENT;
	}
	desc->prio = prio;

	return 0;
}

static int vgic_set_virq(struct z_vcpu *vcpu, struct virt_irq_desc *desc)
{
	uint8_t lr_state;
	k_spinlock_key_t key;
	struct vcpu_virt_irq_block *vb = &vcpu->virq_block;

	if (!is_vm_irq_valid(vcpu->vm, desc->virq_flags)) {
		ZVM_LOG_WARN("VM can not receive virq signal, VM's name: %s.", vcpu->vm->vm_name);
		return -ESRCH;
	}

	key = k_spin_lock(&vb->spinlock);
	lr_state = desc->virq_states;

	switch (lr_state) {
	case VIRQ_STATE_INVALID:
		desc->virq_flags |= VIRQ_PENDING_FLAG;
		if (!sys_dnode_is_linked(&desc->desc_node)) {
			sys_dlist_append(&vb->pending_irqs, &desc->desc_node);
			vb->virq_pending_counts++;
		}
		if (desc->virq_num < VM_LOCAL_VIRQ_NR) {
			/*** bug: thread may be switch to host, and vcpu is NULL. ***/
		}
		break;
	case VIRQ_STATE_ACTIVE:
		desc->virq_flags |= VIRQ_ACTIVATED_FLAG;
		/* if vm interrupt is not in active list */
		if (!sys_dnode_is_linked(&desc->desc_node)) {
			sys_dlist_append(&vb->pending_irqs, &desc->desc_node);
			vb->virq_pending_counts++;
		}
		break;
	case VIRQ_STATE_PENDING:
		break;
	case VIRQ_STATE_ACTIVE_AND_PENDING:
		break;
	}
	k_spin_unlock(&vb->spinlock, key);
	/**
	 * @Bug: Occur bug here: without judgement, wakeup_target_vcpu
	 * will introduce a pause vm error !
	 * When vcpu is not bind to current cpu, we should inform the
	 * dest pcpu. In this situation, vCPU may run on the other pcpus
	 * or it is in a idle states.
	 */
	if (vcpu->work->vcpu_thread != _current) {
		if (is_thread_active_elsewhere(vcpu->work->vcpu_thread)) {
#if defined(CONFIG_SMP) &&  defined(CONFIG_SCHED_IPI_SUPPORTED)
			arch_sched_broadcast_ipi();
#endif
		} else {
			wakeup_target_vcpu(vcpu, desc);
		}
	}

	return 0;
}

static int vgic_unset_virq(struct z_vcpu *vcpu, struct virt_irq_desc *desc)
{
	uint8_t lr_state;
	k_spinlock_key_t key;
	struct vcpu_virt_irq_block *vb = &vcpu->virq_block;

	if (!is_vm_irq_valid(vcpu->vm, desc->virq_flags)) {
		ZVM_LOG_WARN(
			"VM can not receive virq signal, VM's name: %s.",
			vcpu->vm->vm_name);
		return -ESRCH;
	}

	key = k_spin_lock(&vb->spinlock);
	lr_state = desc->virq_states;

	desc->virq_flags &= ~VIRQ_PENDING_FLAG;
	desc->virq_flags &= ~VIRQ_ACTIVATED_FLAG;

	if (sys_dnode_is_linked(&desc->desc_node)) {
		sys_dlist_remove(&desc->desc_node);
		vb->virq_pending_counts--;
	}

	k_spin_unlock(&vb->spinlock, key);

	return 0;
}

/**
 * @brief set sgi interrupt to vm, which usually used on vcpu
 * communication.
 */
static bool vgic_set_sgi2vcpu(struct z_vcpu *vcpu, struct virt_irq_desc *desc)
{
	return true;
}

static int vgic_gicd_mem_read(struct z_vcpu *vcpu, struct virt_gic_gicd *gicd,
								uint32_t offset, uint64_t *v)
{
	uint32_t *value = (uint32_t *)v;

	offset += GIC_DIST_BASE;
	switch (offset) {
	case GICD_CTLR:
		*value = vgic_sysreg_read32(gicd->gicd_regs_base, VGICD_CTLR) & ~(1 << 31);
		break;
	case GICD_TYPER:
		*value = vgic_sysreg_read32(gicd->gicd_regs_base, VGICD_TYPER);
		break;
	case GICD_IIDR:
		*value = vgic_sysreg_read32(gicd->gicd_regs_base, offset-GIC_DIST_BASE);
	case GICD_STATUSR:
		*value = 0;
		break;
	case GICD_ISENABLERn...(GICD_ICENABLERn - 1):
		*value = 0;
		break;
	case GICD_ICENABLERn...(GICD_ISPENDRn - 1):
		*value = 0;
		break;
	case (GIC_DIST_BASE+VGIC_RESERVED)...(GIC_DIST_BASE+VGIC_INMIRn - 1):
		*value = vgic_sysreg_read32(gicd->gicd_base, offset-GIC_DIST_BASE);
		break;
	case GICD_ICFGRn...(GIC_DIST_BASE + 0x0cfc - 1):
		virt_irq_get_type(vcpu, offset, value);
		break;
	case (GIC_DIST_BASE + VGICD_PIDR2):
		*value = vgic_sysreg_read32(gicd->gicd_regs_base, VGICD_PIDR2);
		break;
	default:
		*value = 0;
		break;
	}
	return 0;
}

static int vgic_gicd_mem_write(struct z_vcpu *vcpu, struct virt_gic_gicd *gicd,
								uint32_t offset, uint64_t *v)
{
	uint32_t x, y, bit, t;
	uint32_t *value = (uint32_t *)v;
	k_spinlock_key_t key;

	key = k_spin_lock(&gicd->gicd_lock);
	offset += GIC_DIST_BASE;
	switch (offset) {
	case GICD_CTLR:
		vgic_sysreg_write32(*value, gicd->gicd_regs_base, VGICD_CTLR);
		break;
	case GICD_TYPER:
		break;
	case GICD_STATUSR:
		break;
	case GICD_ISENABLERn...(GICD_ICENABLERn - 1):
		x = (offset - GICD_ISENABLERn) / 4;
		y = x * 32;
		vgic_test_and_set_enable_bit(vcpu, y, value, 32, 1, gicd);
		break;
	case GICD_ICENABLERn...(GICD_ISPENDRn - 1):
		x = (offset - GICD_ICENABLERn) / 4;
		y = x * 32;
		vgic_test_and_set_enable_bit(vcpu, y, value, 32, 0, gicd);
		break;
	case GICD_ISPENDRn...(GICD_ICPENDRn - 1):
		/* Set virt irq to vm. */
		x = (offset - GICD_ISPENDRn) / 4;
		y = x * 32;
		vgic_test_and_set_pending_bit(vcpu, y, value, 32, 1, gicd);
		break;
	case GICD_ICPENDRn...(GICD_ISACTIVERn - 1):
		/* Unset virt irq to vm. */
		x = (offset - GICD_ICPENDRn) / 4;
		y = x * 32;
		vgic_test_and_set_pending_bit(vcpu, y, value, 32, 0, gicd);
		break;
	case GICD_IPRIORITYRn...(GIC_DIST_BASE + 0x07f8 - 1):
		t = *value;
		x = (offset - GICD_IPRIORITYRn) / 4;
		y = x * 4 - 1;
		bit = (t & 0x000000ff);
		vgic_virq_set_priority(vcpu, y + 1, bit);
		bit = (t & 0x0000ff00) >> 8;
		vgic_virq_set_priority(vcpu, y + 2, bit);
		bit = (t & 0x00ff0000) >> 16;
		vgic_virq_set_priority(vcpu, y + 3, bit);
		bit = (t & 0xff000000) >> 24;
		vgic_virq_set_priority(vcpu, y + 4, bit);
		break;
	case GICD_ICFGRn...(GIC_DIST_BASE + 0x0cfc - 1):
		virt_irq_set_type(vcpu, offset, value);
		break;
	case (GIC_DIST_BASE+VGIC_RESERVED)...(GIC_DIST_BASE+VGIC_INMIRn - 1):
		vgic_sysreg_write32(*value, gicd->gicd_base, offset-GIC_DIST_BASE);
		break;
	default:
		break;
	}
	k_spin_unlock(&gicd->gicd_lock, key);

	return 0;
}

void arch_vdev_irq_enable(struct z_vcpu *vcpu)
{
	uint32_t irq;
	struct z_vm *vm = vcpu->vm;
	struct z_virt_dev *vdev;
	struct  _dnode *d_node, *ds_node;

	SYS_DLIST_FOR_EACH_NODE_SAFE(&vm->vdev_list, d_node, ds_node) {
		vdev = CONTAINER_OF(d_node, struct z_virt_dev, vdev_node);
		if (vdev->dev_pt_flag && vcpu->vcpu_id == 0) {
			/* enable spi interrupt */
			irq = vdev->hirq;
			if (irq > CONFIG_NUM_IRQS) {
				continue;
			}
			arm_gic_irq_enable(irq);
		}
	}
}

void arch_vdev_irq_disable(struct z_vcpu *vcpu)
{
	uint32_t irq;
	struct z_vm *vm = vcpu->vm;
	struct z_virt_dev *vdev;
	struct  _dnode *d_node, *ds_node;

	SYS_DLIST_FOR_EACH_NODE_SAFE(&vm->vdev_list, d_node, ds_node) {
		vdev = CONTAINER_OF(d_node, struct z_virt_dev, vdev_node);
		if (vdev->dev_pt_flag && vcpu->vcpu_id == 0) {
			/* disable spi interrupt */
			irq = vdev->hirq;
			if (irq > CONFIG_NUM_IRQS) {
				continue;
			}
			arm_gic_irq_disable(irq);
		}
	}
}

int vgic_vdev_mem_read(struct z_virt_dev *vdev, uint64_t addr, uint64_t *value, uint16_t size)
{
	uint32_t offset, type = TYPE_GIC_INVALID;
	struct z_vcpu *vcpu = _current_vcpu;
	struct vgicv3_dev *vgic = (struct vgicv3_dev *)vdev->priv_vdev;
	struct virt_gic_gicd *gicd = &vgic->gicd;
	struct virt_gic_gicr *gicr;

	/*Avoid some case that we only just use '|' to get the value */
	*value = 0;

	if ((addr >= gicd->gicd_base) && (addr < gicd->gicd_base + gicd->gicd_size)) {
		type = TYPE_GIC_GICD;
		offset = addr - gicd->gicd_base;
	} else {
		gicr = get_vcpu_gicr_type(vgic, addr, &type, &offset);
	}

	switch (type) {
	case TYPE_GIC_GICD:
			return vgic_gicd_mem_read(vcpu, gicd, offset, value);
	case TYPE_GIC_GICR_RD:
			return vgic_gicrrd_mem_read(vcpu, gicr, offset, value);
	case TYPE_GIC_GICR_SGI:
			return vgic_gicrsgi_mem_read(vcpu, gicr, offset, value);
	case TYPE_GIC_GICR_VLPI:
			/* ignore vlpi register */
			return 0;
	default:
		return 0;
	}

	return 0;
}

int vgic_vdev_mem_write(struct z_virt_dev *vdev, uint64_t addr, uint64_t *value, uint16_t size)
{
	uint32_t offset;
	int type = TYPE_GIC_INVALID;
	struct z_vcpu *vcpu = _current_vcpu;
	struct vgicv3_dev *vgic = (struct vgicv3_dev *)vdev->priv_vdev;
	struct virt_gic_gicd *gicd = &vgic->gicd;
	struct virt_gic_gicr *gicr;

	if ((addr >= gicd->gicd_base) && (addr < gicd->gicd_base + gicd->gicd_size)) {
		type = TYPE_GIC_GICD;
		offset = addr - gicd->gicd_base;
	} else {
		gicr = get_vcpu_gicr_type(vgic, addr, &type, &offset);
	}

	switch (type) {
	case TYPE_GIC_GICD:
			return vgic_gicd_mem_write(vcpu, gicd, offset, value);
	case TYPE_GIC_GICR_RD:
			return vgic_gicrrd_mem_write(vcpu, gicr, offset, value);
	case TYPE_GIC_GICR_SGI:
			return vgic_gicrsgi_mem_write(vcpu, gicr, offset, value);
	case TYPE_GIC_GICR_VLPI:
			return 0;
	default:
		return 0;
	}

	return 0;
}

int set_virq_to_vcpu(struct z_vcpu *vcpu, uint32_t virq_num)
{
	struct virt_irq_desc *desc;

	desc = vgic_get_virt_irq_desc(vcpu, virq_num);
	if (!desc) {
		ZVM_LOG_WARN("Get virt irq desc error here!");
		return -ESRCH;
	}

	return vgic_set_virq(vcpu, desc);
}

int set_virq_to_vm(struct z_vm *vm, uint32_t virq_num)
{
	uint32_t ret = 0;
	struct virt_irq_desc *desc;
	struct z_vcpu *vcpu, *target_vcpu;

	vcpu = vm->vcpus[DEFAULT_VCPU];

	if (virq_num < VM_LOCAL_VIRQ_NR) {
		desc = &vcpu->virq_block.vcpu_virt_irq_desc[virq_num];
	} else if (virq_num <= VM_GLOBAL_VIRQ_NR) {
		desc = &vm->vm_irq_block.vm_virt_irq_desc[virq_num - VM_LOCAL_VIRQ_NR];
	} else {
		ZVM_LOG_WARN("The spi num that ready to allocate is too big.");
		return -ENODEV;
	}

	target_vcpu = vm->vcpus[desc->vcpu_id];
	ret = vgic_set_virq(target_vcpu, desc);
	if (ret >= 0) {
		return SET_IRQ_TO_VM_SUCCESS;
	}

	return ret;
}

int unset_virq_to_vm(struct z_vm *vm, uint32_t virq_num)
{
	uint32_t ret = 0;
	struct virt_irq_desc *desc;
	struct z_vcpu *vcpu, *target_vcpu;

	vcpu = vm->vcpus[DEFAULT_VCPU];

	if (virq_num < VM_LOCAL_VIRQ_NR) {
		desc = &vcpu->virq_block.vcpu_virt_irq_desc[virq_num];
	} else if (virq_num <= VM_GLOBAL_VIRQ_NR) {
		desc = &vm->vm_irq_block.vm_virt_irq_desc[virq_num - VM_LOCAL_VIRQ_NR];
	} else {
		ZVM_LOG_WARN("The spi num that ready to allocate is too big.");
		return -ENODEV;
	}

	target_vcpu = vm->vcpus[desc->vcpu_id];
	ret = vgic_unset_virq(target_vcpu, desc);
	if (ret >= 0) {
		return UNSET_IRQ_TO_VM_SUCCESS;
	}

	return ret;
}

int virt_irq_sync_vgic(struct z_vcpu *vcpu)
{
	uint8_t lr_state;
	uint64_t elrsr, eisr;
	k_spinlock_key_t key;
	struct virt_irq_desc *desc;
	struct _dnode *d_node, *ds_node;
	struct vcpu_virt_irq_block *vb = &vcpu->virq_block;

	key = k_spin_lock(&vb->spinlock);
	if (vb->virq_pending_counts == 0) {
		k_spin_unlock(&vb->spinlock, key);
		return 0;
	}

	/* Get the maintain or valid irq */
	elrsr = read_elrsr_el2();
	eisr = read_eisr_el2();
	elrsr |= eisr;
	elrsr &= vcpu->arch->list_regs_map;

	SYS_DLIST_FOR_EACH_NODE_SAFE(&vb->active_irqs, d_node, ds_node) {
		desc = CONTAINER_OF(d_node, struct virt_irq_desc, desc_node);
		/* A valid interrupt? store it again! */
		if (!VGIC_ELRSR_REG_TEST(desc->id, elrsr)) {
			continue;
		}

		lr_state = gicv3_get_lr_state(vcpu, desc);
		switch (lr_state) {
		/* vm interrupt is done or need pending again when it is active  */
		case VIRQ_STATE_ACTIVE:
			/* if this sync is not irq trap */
			if (vcpu->exit_type != ARM_VM_EXCEPTION_IRQ) {
				desc->virq_states = lr_state;
				break;
			}
		case VIRQ_STATE_INVALID:
			gicv3_update_lr(vcpu, desc, ACTION_CLEAR_VIRQ, 0);
			vcpu->arch->hcr_el2 &= ~(uint64_t)HCR_VI_BIT;
			sys_dlist_remove(&desc->desc_node);
			/* if software irq is still triggered */
			if (desc->vdev_trigger) {
				/* This means vm interrupt is done, but host interrupt is pending */
				sys_dlist_append(&vb->pending_irqs, &desc->desc_node);
			}
			vb->virq_pending_counts--;
		/* vm interrupt is still pending, no need to inject again. */
		case VIRQ_STATE_PENDING:
		case VIRQ_STATE_ACTIVE_AND_PENDING:
			desc->virq_states = lr_state;
			break;
		}
	}
	k_spin_unlock(&vb->spinlock, key);

	return 0;
}

int virt_irq_flush_vgic(struct z_vcpu *vcpu)
{
	int ret;
	k_spinlock_key_t key;
	struct virt_irq_desc *desc;
	struct _dnode *d_node, *ds_node;
	struct vcpu_virt_irq_block *vb = &vcpu->virq_block;

	key = k_spin_lock(&vb->spinlock);
	if (vb->virq_pending_counts == 0) {
		/* No pending irq, just return! */
		k_spin_unlock(&vb->spinlock, key);
		return 0;
	}

	/* no idle list register */
	if (vcpu->arch->list_regs_map == ((1 << VGIC_TYPER_LR_NUM) - 1)) {
		k_spin_unlock(&vb->spinlock, key);
		ZVM_LOG_WARN("There is no idle list register! ");
		return 0;
	}

	SYS_DLIST_FOR_EACH_NODE_SAFE(&vb->pending_irqs, d_node, ds_node) {
		desc = CONTAINER_OF(d_node, struct virt_irq_desc, desc_node);

		/* if there the vm interrupt is not deactived, avoid inject it again */
		if (!(desc->virq_states == VIRQ_STATE_INVALID ||
		desc->virq_states == VIRQ_STATE_ACTIVE)) {
			continue;
		}

		if (desc->virq_flags & VIRQ_PENDING_FLAG ||
		desc->virq_flags & VIRQ_ACTIVATED_FLAG) {
			switch (VGIC_VIRQ_LEVEL_SORT(desc->virq_num)) {
			case VGIC_VIRQ_IN_SGI:
				vgic_set_sgi2vcpu(vcpu, desc);
			case VGIC_VIRQ_IN_PPI:
			default:
				break;
			}
			desc->id = gicv3_get_idle_lr(vcpu);
			if (desc->id < 0) {
				ZVM_LOG_WARN("No idle list register for virq: %d.\n", desc->id);
				break;
			}
			ret = gicv3_inject_virq(vcpu, desc);
			if (ret) {
				k_spin_unlock(&vb->spinlock, key);
				return ret;
			}
			desc->virq_states = VIRQ_STATE_PENDING;
			desc->virq_flags &= (uint32_t)~VIRQ_PENDING_FLAG;
			sys_dlist_remove(&desc->desc_node);
			sys_dlist_append(&vb->active_irqs, &desc->desc_node);
		} else {
			ZVM_LOG_WARN("Something wrong:\n");
			ZVM_LOG_WARN(
				"virq-id %d is not pending but in the list.\n",
				desc->id);
			gicv3_update_lr(vcpu, desc, ACTION_CLEAR_VIRQ, 0);
			desc->id = VM_INVALID_DESC_ID;
			sys_dlist_remove(&desc->desc_node);
		}
	}
	k_spin_unlock(&vb->spinlock, key);

	return 0;
}

struct virt_irq_desc *get_virt_irq_desc(struct z_vcpu *vcpu, uint32_t virq)
{
	return vgic_get_virt_irq_desc(vcpu, virq);
}

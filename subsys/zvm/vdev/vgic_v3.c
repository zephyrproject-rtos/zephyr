/*
 * Copyright 2024-2025 HNU-ESNL: Guoqi Xie, Chenglai Xiong, Xingyu Hu and etc.
 * Copyright 2024-2025 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/init.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/arch/arm64/lib_helpers.h>
#include <zephyr/arch/common/sys_bitops.h>
#include <zephyr/dt-bindings/interrupt-controller/arm-gic.h>
#include <zephyr/drivers/interrupt_controller/gic.h>
#include <zephyr/logging/log.h>
#include <zephyr/zvm/arm/cpu.h>
#include <zephyr/zvm/vdev/vgic_common.h>
#include <zephyr/zvm/vdev/vgic_v3.h>
#include <zephyr/zvm/zvm.h>
#include <zephyr/zvm/vm_irq.h>
#include <zephyr/zvm/vm_device.h>


LOG_MODULE_DECLARE(ZVM_MODULE_NAME);

#define VM_GIC_NAME			vm_gic_v3

#define DEV_DATA(dev) \
	((struct virt_device_data *)(dev)->data)

#define DEV_CFG(dev) \
	((const struct virt_device_config * const)(dev)->config)

static const struct virtual_device_instance *gic_virtual_device_instance;

static void vgicv3_lrs_load(struct gicv3_vcpuif_ctxt *ctxt)
{
	uint32_t rg_cout = VGIC_TYPER_LR_NUM;

	if (rg_cout > VGIC_TYPER_LR_NUM) {
		ZVM_LOG_WARN("System list registers do not support!\n");
		return;
	}

	switch (rg_cout) {
	case 8:
		write_sysreg(ctxt->ich_lr7_el2, ICH_LR7_EL2);
	case 7:
		write_sysreg(ctxt->ich_lr6_el2, ICH_LR6_EL2);
	case 6:
		write_sysreg(ctxt->ich_lr5_el2, ICH_LR5_EL2);
	case 5:
		write_sysreg(ctxt->ich_lr4_el2, ICH_LR4_EL2);
	case 4:
		write_sysreg(ctxt->ich_lr3_el2, ICH_LR3_EL2);
	case 3:
		write_sysreg(ctxt->ich_lr2_el2, ICH_LR2_EL2);
	case 2:
		write_sysreg(ctxt->ich_lr1_el2, ICH_LR1_EL2);
	case 1:
		write_sysreg(ctxt->ich_lr0_el2, ICH_LR0_EL2);
		break;
	default:
		break;
	}
}

static void vgicv3_prios_load(struct gicv3_vcpuif_ctxt *ctxt)
{
	uint32_t rg_cout = VGIC_TYPER_PRIO_NUM;

	switch (rg_cout) {
	case 7:
		write_sysreg(ctxt->ich_ap0r2_el2, ICH_AP0R2_EL2);
		write_sysreg(ctxt->ich_ap1r2_el2, ICH_AP1R2_EL2);
	case 6:
		write_sysreg(ctxt->ich_ap0r1_el2, ICH_AP0R1_EL2);
		write_sysreg(ctxt->ich_ap1r1_el2, ICH_AP1R1_EL2);
	case 5:
		write_sysreg(ctxt->ich_ap0r0_el2, ICH_AP0R0_EL2);
		write_sysreg(ctxt->ich_ap1r0_el2, ICH_AP1R0_EL2);
		break;
	default:
		ZVM_LOG_ERR("Load prs error");
	}
}

static void vgicv3_ctrls_load(struct gicv3_vcpuif_ctxt *ctxt)
{
	write_sysreg(ctxt->icc_sre_el1, ICC_SRE_EL1);
	write_sysreg(ctxt->ich_vmcr_el2, ICH_VMCR_EL2);
	write_sysreg(ctxt->ich_hcr_el2, ICH_HCR_EL2);
}

static void vgicv3_lrs_save(struct gicv3_vcpuif_ctxt *ctxt)
{
	uint32_t rg_cout = VGIC_TYPER_LR_NUM;

	if (rg_cout > VGIC_TYPER_LR_NUM) {
		ZVM_LOG_WARN("System list registers do not support!\n");
		return;
	}

	switch (rg_cout) {
	case 8:
		ctxt->ich_lr7_el2 = read_sysreg(ICH_LR7_EL2);
	case 7:
		ctxt->ich_lr6_el2 = read_sysreg(ICH_LR6_EL2);
	case 6:
		ctxt->ich_lr5_el2 = read_sysreg(ICH_LR5_EL2);
	case 5:
		ctxt->ich_lr4_el2 = read_sysreg(ICH_LR4_EL2);
	case 4:
		ctxt->ich_lr3_el2 = read_sysreg(ICH_LR3_EL2);
	case 3:
		ctxt->ich_lr2_el2 = read_sysreg(ICH_LR2_EL2);
	case 2:
		ctxt->ich_lr1_el2 = read_sysreg(ICH_LR1_EL2);
	case 1:
		ctxt->ich_lr0_el2 = read_sysreg(ICH_LR0_EL2);
		break;
	default:
		break;
	}
}

static void vgicv3_lrs_init(void)
{
	uint32_t rg_cout = VGIC_TYPER_LR_NUM;

	if (rg_cout > VGIC_TYPER_LR_NUM) {
		ZVM_LOG_WARN("System list registers do not support!\n");
		return;
	}

	rg_cout = rg_cout > 8 ? 8 : rg_cout;

	switch (rg_cout) {
	case 8:
		write_sysreg(0, ICH_LR7_EL2);
	case 7:
		write_sysreg(0, ICH_LR6_EL2);
	case 6:
		write_sysreg(0, ICH_LR5_EL2);
	case 5:
		write_sysreg(0, ICH_LR4_EL2);
	case 4:
		write_sysreg(0, ICH_LR3_EL2);
	case 3:
		write_sysreg(0, ICH_LR2_EL2);
	case 2:
		write_sysreg(0, ICH_LR1_EL2);
	case 1:
		write_sysreg(0, ICH_LR0_EL2);
		break;
	default:
		break;
	}
}

static void vgicv3_prios_save(struct gicv3_vcpuif_ctxt *ctxt)
{
	uint32_t rg_cout = VGIC_TYPER_PRIO_NUM;

	switch (rg_cout) {
	case 7:
		ctxt->ich_ap0r2_el2 = read_sysreg(ICH_AP0R2_EL2);
		ctxt->ich_ap1r2_el2 = read_sysreg(ICH_AP1R2_EL2);
	case 6:
		ctxt->ich_ap0r1_el2 = read_sysreg(ICH_AP0R1_EL2);
		ctxt->ich_ap1r1_el2 = read_sysreg(ICH_AP1R1_EL2);
	case 5:
		ctxt->ich_ap0r0_el2 = read_sysreg(ICH_AP0R0_EL2);
		ctxt->ich_ap1r0_el2 = read_sysreg(ICH_AP1R0_EL2);
		break;
	default:
		ZVM_LOG_ERR(" Set ich_ap priority failed.\n");
	}
}

static void vgicv3_ctrls_save(struct gicv3_vcpuif_ctxt *ctxt)
{
	ctxt->icc_sre_el1 = read_sysreg(ICC_SRE_EL1);
	ctxt->ich_vmcr_el2 = read_sysreg(ICH_VMCR_EL2);
	ctxt->ich_hcr_el2 = read_sysreg(ICH_HCR_EL2);
}

static int vdev_gicv3_init(
	struct z_vm *vm, struct vgicv3_dev *gicv3_vdev,
	uint32_t gicd_base, uint32_t gicd_size,
	uint32_t gicr_base, uint32_t gicr_size)
{
	int i = 0;
	uint32_t spi_num;
	uint64_t tmp_typer = 0;
	struct virt_gic_gicd *gicd = &gicv3_vdev->gicd;
	struct virt_gic_gicr *gicr;

	gicd->gicd_base = gicd_base;
	gicd->gicd_size = gicd_size;
	gicd->gicd_regs_base = (uint32_t *)k_malloc(gicd->gicd_size);
	if (!gicd->gicd_regs_base) {
		return -ENXIO;
	}
	memset(gicd->gicd_regs_base, 0, gicd_size);
	/* GICD PIDR2 */
	vgic_sysreg_write32(0x3<<4, gicd->gicd_regs_base, VGICD_PIDR2);
	spi_num = ((VM_GLOBAL_VIRQ_NR + 32) >> 5) - 1;
	/* GICD TYPER */
	tmp_typer = (vm->vcpu_num << 5) | (9 << 19) | spi_num;
	vgic_sysreg_write32(tmp_typer, gicd->gicd_regs_base, VGICD_TYPER);
	/* Init spinlock */
	ZVM_SPINLOCK_INIT(&gicd->gicd_lock);

	for (i = 0; i < MIN(VGIC_RDIST_SIZE/VGIC_RD_SGI_SIZE, vm->vcpu_num); i++) {
		gicr = (struct virt_gic_gicr *)k_malloc(sizeof(struct virt_gic_gicr));
		if (!gicr) {
			return -ENXIO;
		}
		/* store the vcpu id for gicr */
		gicr->vcpu_id = i;

		/* init redistribute size */
		gicr->gicr_rd_size = VGIC_RD_BASE_SIZE;
		gicr->gicr_rd_reg_base = (uint32_t *)k_malloc(gicr->gicr_rd_size);
		if (!gicr->gicr_rd_reg_base) {
			ZVM_LOG_ERR("Allocat memory for gicr_rd error!\n");
			return -ENXIO;
		}
		memset(gicr->gicr_rd_reg_base, 0, gicr->gicr_rd_size);

		/* init sgi redistribute size */
		gicr->gicr_sgi_size = VGIC_SGI_BASE_SIZE;
		gicr->gicr_sgi_reg_base = (uint32_t *)k_malloc(gicr->gicr_sgi_size);
		if (!gicr->gicr_sgi_reg_base) {
			ZVM_LOG_ERR("Allocat memory for gicr_sgi error!\n");
			return -ENXIO;
		}
		memset(gicr->gicr_sgi_reg_base, 0, gicr->gicr_sgi_size);

		gicr->gicr_rd_base = gicr_base + VGIC_RD_SGI_SIZE * i;
		gicr->gicr_sgi_base = gicr->gicr_rd_base + VGIC_RD_BASE_SIZE;
		vgic_sysreg_write32(0x3<<4, gicr->gicr_rd_reg_base, VGICR_PIDR2);
		ZVM_SPINLOCK_INIT(&gicr->gicr_lock);

		/* GICR TYPER */
		tmp_typer = 1 << GICR_TYPER_LPI_AFFINITY_SHIFT |
		i << GICR_TYPER_PROCESSOR_NUMBER_SHIFT |
		((uint64_t)i << GICR_TYPER_AFFINITY_VALUE_SHIFT);
		if (i >= vm->vcpu_num - 1) {
			/* set last gicr region flag here, means it is the last gicr region */
			tmp_typer |= 1 << GICR_TYPER_LAST_SHIFT;
		}
		vgic_sysreg_write64(tmp_typer, gicr->gicr_rd_reg_base, VGICR_TYPER);
		vgic_sysreg_write64(tmp_typer, gicr->gicr_sgi_reg_base, VGICR_TYPER);

		gicv3_vdev->gicr[i] = gicr;
	}

	ZVM_LOG_INFO("** List register num: %lld\n", VGIC_TYPER_LR_NUM);
	vgicv3_lrs_init();

	return 0;
}

static int vdev_gicv3_deinit(struct z_vm *vm, struct vgicv3_dev *gicv3_vdev)
{
	ARG_UNUSED(vm);
	int i = 0;
	struct virt_gic_gicd *gicd = &gicv3_vdev->gicd;
	struct virt_gic_gicr *gicr;

	for (i = 0; i < MIN(VGIC_RDIST_SIZE/VGIC_RD_SGI_SIZE, vm->vcpu_num); i++) {
		gicr = gicv3_vdev->gicr[i];
		k_free(gicr->gicr_rd_reg_base);
		k_free(gicr->gicr_sgi_reg_base);
		k_free(gicr);
	}
	k_free(gicd->gicd_regs_base);

	return 0;
}

/**
 * @brief init vm gic device for each vm. Including:
 * 1. creating virt device for vm.
 * 2. building memory map for this device.
 */
static int vm_vgicv3_init(
	const struct device *dev, struct z_vm *vm,
	struct z_virt_dev *vdev_desc)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(vdev_desc);
	int ret;
	uint32_t gicd_base, gicd_size, gicr_base, gicr_size;
	struct z_virt_dev *virt_dev;
	struct vgicv3_dev *vgicv3;

	gicd_base = VGIC_DIST_BASE;
	gicd_size = VGIC_DIST_SIZE;
	gicr_base = VGIC_RDIST_BASE;
	gicr_size = VGIC_RDIST_SIZE;
	/* check gic device */
	if (!gicd_base || !gicd_size  || !gicr_base  || !gicr_size) {
		ZVM_LOG_ERR("GIC device has init error!");
		return -ENODEV;
	}

	/* Init virtual device for vm. */
	virt_dev = vm_virt_dev_add(
		vm, gic_virtual_device_instance->name,
		false, false, gicd_base,
		gicd_base, gicr_base+gicr_size-gicd_base, 0, 0);
	if (!virt_dev) {
		return -ENODEV;
	}

	/* Init virtual gic device for virtual device. */
	vgicv3 = (struct vgicv3_dev *)k_malloc(sizeof(struct vgicv3_dev));
	if (!vgicv3) {
		ZVM_LOG_ERR("Allocat memory for vgicv3 error.\n");
		return -ENODEV;
	}
	ret = vdev_gicv3_init(vm, vgicv3, gicd_base, gicd_size, gicr_base, gicr_size);
	if (ret) {
		ZVM_LOG_ERR("Init virt gicv3 error.\n");
		return -ENODEV;
	}

	/* get the private data for vgicv3 */
	virt_dev->priv_data = gic_virtual_device_instance;
	virt_dev->priv_vdev = vgicv3;

	return 0;
}

static int vm_vgicv3_deinit(
	const struct device *dev, struct z_vm *vm,
	struct z_virt_dev *vdev_desc)
{
	ARG_UNUSED(dev);
	int ret;
	struct vgicv3_dev *vgicv3;

	vgicv3 = (struct vgicv3_dev *)vdev_desc->priv_vdev;
	if (!vgicv3) {
		ZVM_LOG_WARN("Can not find virt gicv3 device!\n");
		return 0;
	}
	ret = vdev_gicv3_deinit(vm, vgicv3);
	if (ret) {
		ZVM_LOG_WARN("Deinit virt gicv3 error.\n");
		return 0;
	}
	k_free(vgicv3);

	vdev_desc->priv_vdev = NULL;
	vdev_desc->priv_data = NULL;
	ret = vm_virt_dev_remove(vm, vdev_desc);
	return ret;
}

/**
 * @brief The init function of vgic, it provides the
 * gic hardware device information to ZVM.
 */
static int virt_gic_v3_init(void)
{
	int i;

	for (i = 0; i < zvm_virtual_devices_count_get(); i++) {
		const struct virtual_device_instance *virtual_device =
		zvm_virtual_device_get(i);

		if (strcmp(virtual_device->name, TOSTRING(VM_GIC_NAME))) {
			continue;
		}
		DEV_DATA(virtual_device)->vdevice_type |= VM_DEVICE_PRE_KERNEL_1;
		gic_virtual_device_instance = virtual_device;
		break;
	}
	return 0;
}

static struct virt_device_config virt_gicv3_cfg = {
	.hirq_num = VM_DEVICE_INVALID_VIRQ,
	.device_config = NULL,
};

static struct virt_device_data virt_gicv3_data_port = {
	.device_data = NULL,
};

/**
 * @brief vgic device operations api.
 */
static const struct virt_device_api virt_gicv3_api = {
	.init_fn = vm_vgicv3_init,
	.deinit_fn = vm_vgicv3_deinit,
	.virt_device_read = vgic_vdev_mem_read,
	.virt_device_write = vgic_vdev_mem_write,
};

ZVM_VIRTUAL_DEVICE_DEFINE(virt_gic_v3_init,
			POST_KERNEL, CONFIG_VM_VGICV3_INIT_PRIORITY,
			VM_GIC_NAME,
			virt_gicv3_data_port,
			virt_gicv3_cfg,
			virt_gicv3_api);

/*******************vgicv3 function****************************/

bool virt_irq_ispending(struct z_vcpu *vcpu)
{
	uint32_t *mem_addr_base = NULL;
	uint32_t pend_addrend;
	struct z_vm *vm;
	struct z_virt_dev *vdev;
	struct  _dnode *d_node, *ds_node;

	vm = vcpu->vm;
	SYS_DLIST_FOR_EACH_NODE_SAFE(&vm->vdev_list, d_node, ds_node) {
		vdev = CONTAINER_OF(d_node, struct z_virt_dev, vdev_node);
		if (!strcmp(vdev->name, TOSTRING(VM_GIC_NAME))) {
			mem_addr_base = arm_gic_get_distbase(vdev);
			break;
		}
	}

	if (mem_addr_base == NULL) {
		ZVM_LOG_ERR("Can not find gic controller!\n");
		return false;
	}
	mem_addr_base += VGICD_ISPENDRn;
	pend_addrend = (uint64_t)mem_addr_base+(VGICD_ICPENDRn-VGICD_ISPENDRn);
	for (; (uint64_t)mem_addr_base < pend_addrend; mem_addr_base++) {
		if (vgic_irq_test_bit(vcpu, 0, mem_addr_base, 32, 0)) {
			return true;
		}
	}
	return false;
}

uint32_t *arm_gic_get_distbase(struct z_virt_dev *vdev)
{
	struct vgicv3_dev *vgic = (struct vgicv3_dev *)vdev->priv_vdev;
	struct virt_gic_gicd *gicd = &vgic->gicd;

	return gicd->gicd_regs_base;
}

int gicv3_inject_virq(struct z_vcpu *vcpu, struct virt_irq_desc *desc)
{
	uint64_t value = 0;
	struct gicv3_list_reg *lr = (struct gicv3_list_reg *)&value;

	if (desc->id >= VGIC_TYPER_LR_NUM) {
		ZVM_LOG_WARN("invalid virq id %d, It is used by other device!\n", desc->id);
		return -EINVAL;
	}

	/* List register is not activated. */
	if (VGIC_LIST_REGS_TEST(desc->id, vcpu)) {
		value = gicv3_read_lr(desc->id);
		lr = (struct gicv3_list_reg *)&value;
		if (lr->vINTID == desc->virq_num) {
			desc->virq_flags |= VIRQ_PENDING_FLAG;
		}
	}

	lr->vINTID = desc->virq_num;
	lr->pINTID = desc->pirq_num;
	lr->priority = desc->prio;
	lr->group = LIST_REG_GROUP1;
	lr->hw = LIST_REG_HW_VIRQ;
	lr->state = VIRQ_STATE_PENDING;
	gicv3_update_lr(vcpu, desc, ACTION_SET_VIRQ, value);
	return 0;
}

int vgicv3_raise_sgi(struct z_vcpu *vcpu, unsigned long sgi_value)
{
	int i, bit, sgi_num = 0;
	uint32_t sgi_id, sgi_mode;
	uint32_t target_list, aff1, aff2, aff3, tmp_id;
	uint32_t target_vcpu_list = 0;
	struct z_vcpu *target;
	struct z_vm *vm = vcpu->vm;
	k_spinlock_key_t key;

	sgi_id = (sgi_value & (0xf << 24)) >> 24;
	__ASSERT_NO_MSG(GIC_IS_SGI(sgi_id));

	sgi_mode = sgi_value & (1UL << 40) ? SGI_SIG_TO_OTHERS : SGI_SIG_TO_LIST;
	if (sgi_mode == SGI_SIG_TO_OTHERS) {
		for (i = 0; i < vm->vcpu_num; i++) {
			target = vm->vcpus[i];
			if (target == vcpu) {
				continue;
			}
			target->virq_block.pending_sgi_num = sgi_id;
			key = k_spin_lock(&target->vcpu_lock);
			target->vcpuipi_count++;
			k_spin_unlock(&target->vcpu_lock, key);
		}
		arch_sched_broadcast_ipi();
	} else if (sgi_mode == SGI_SIG_TO_LIST) {
		target_list = sgi_value & 0xffff;
		aff1 = (sgi_value & (uint64_t)(0xffUL << 16)) >> 16;
		aff2 = (sgi_value & (uint64_t)(0xffUL << 32)) >> 32;
		aff3 = (sgi_value & (uint64_t)(0xffUL << 48)) >> 48;
		for (bit = 0; bit < 16; bit++) {
			if (sys_test_bit((uintptr_t)&target_list, bit)) {
				/*Each cluster has CONFIG_MP_MAX_NUM_CPUS*/
				tmp_id = aff1 * CONFIG_MP_MAX_NUM_CPUS + bit;
				sys_set_bits((uintptr_t)&target_vcpu_list, BIT(tmp_id));
				/*TODO: May need modified to vm->vcpu_num. */
				if (++sgi_num > CONFIG_MAX_VCPU_PER_VM ||
				tmp_id >= CONFIG_MAX_VCPU_PER_VM) {
					ZVM_LOG_WARN("The target cpu list is too long.");
					return -ESRCH;
				}
				target = vm->vcpus[tmp_id];
				target->virq_block.pending_sgi_num = sgi_id;
				key = k_spin_lock(&target->vcpu_lock);
				target->vcpuipi_count++;
				k_spin_unlock(&target->vcpu_lock, key);
			}
		}
		if (target_vcpu_list & BIT(tmp_id)) {
			set_virq_to_vm(vcpu->vm, sgi_id);
			/* Set vcpu flag include itself */
			if (target_vcpu_list & ~BIT(tmp_id)) {
				arch_sched_broadcast_ipi();
			}
		} else {
			arch_sched_broadcast_ipi();
		}
	} else {
		ZVM_LOG_WARN("Unsupported sgi signal.");
		return -ESRCH;
	}
	return 0;
}

int vgic_gicrsgi_mem_read(
	struct z_vcpu *vcpu, struct virt_gic_gicr *gicr,
	uint32_t offset, uint64_t *v)
{
	uint32_t *value = (uint32_t *)v;

	switch (offset) {
	case GICR_SGI_CTLR:
		*value = vgic_sysreg_read32(gicr->gicr_sgi_reg_base, VGICR_CTLR) & ~(1 << 31);
		break;
	case GICR_SGI_ISENABLER:
		*value = vgic_sysreg_read32(gicr->gicr_sgi_reg_base, VGICR_ISENABLER0);
		break;
	case GICR_SGI_ICENABLER:
		*value = vgic_sysreg_read32(gicr->gicr_sgi_reg_base, VGICR_ICENABLER0);
		break;
	case GICR_SGI_PENDING:
		vgic_sysreg_write32(*value, gicr->gicr_sgi_reg_base, VGICR_SGI_PENDING);
		break;
	case GICR_SGI_PIDR2:
		*value = (0x03 << 4);
		break;
	default:
		*value = 0;
		break;
	}

	return 0;
}

int vgic_gicrsgi_mem_write(
	struct z_vcpu *vcpu, struct virt_gic_gicr *gicr,
	uint32_t offset, uint64_t *v)
{
	uint32_t *value = (uint32_t *)v;
	uint32_t mem_addr = (uint64_t)v;
	int bit;

	switch (offset) {
	case GICR_SGI_ISENABLER:
		vgic_test_and_set_enable_bit(vcpu, 0, value, 32, 1, gicr);
		break;
	case GICR_SGI_ICENABLER:
		vgic_test_and_set_enable_bit(vcpu, 0, value, 32, 0, gicr);
		break;
	case GICR_SGI_PENDING:
		/* clear pending state */
		for (bit = 0; bit < 32; bit++) {
			if (sys_test_bit(mem_addr, bit)) {
				sys_write32(
					BIT(bit),
					GIC_RDIST_BASE + GICR_SGI_BASE_OFF + GICR_SGI_PENDING);

				vgic_sysreg_write32(
					~BIT(bit),
					gicr->gicr_sgi_reg_base,
					VGICR_SGI_PENDING);
			}
		}
		break;
	default:
		*value = 0;
		break;
	}

	return 0;
}

int vgic_gicrrd_mem_read(
	struct z_vcpu *vcpu, struct virt_gic_gicr *gicr,
	uint32_t offset, uint64_t *v)
{
	uint64_t *value = v;

	/* consider multiple cpu later, Now just return 0 */
	switch (offset) {
	case 0xffe8:
		*value = vgic_sysreg_read32(gicr->gicr_rd_reg_base, VGICR_PIDR2);
		break;
	case GICR_CTLR:
		vgic_sysreg_write32(*value, gicr->gicr_rd_reg_base, VGICR_CTLR);
		break;
	case GICR_TYPER:
		*value = vgic_sysreg_read64(gicr->gicr_rd_reg_base, VGICR_TYPER);
		break;
	default:
		*value = 0;
		break;
	}

	return 0;
}

int vgic_gicrrd_mem_write(
	struct z_vcpu *vcpu, struct virt_gic_gicr *gicr,
	uint32_t offset, uint64_t *v)
{
	return 0;
}

struct virt_gic_gicr *get_vcpu_gicr_type(
	struct vgicv3_dev *vgic, uint32_t addr,
	uint32_t *type,  uint32_t *offset)
{
	int i;
	struct virt_gic_gicr *gicr;
	struct z_vm *vm = get_current_vm();

	for (i = 0; i < MIN(VGIC_RDIST_SIZE / VGIC_RD_SGI_SIZE, vm->vcpu_num); i++) {
		gicr = vgic->gicr[i];
		if ((addr >= gicr->gicr_sgi_base) &&
		addr < gicr->gicr_sgi_base + gicr->gicr_sgi_size) {
			*offset = addr - gicr->gicr_sgi_base;
			*type = TYPE_GIC_GICR_SGI;
			return vgic->gicr[i];
		}

		if (addr >= gicr->gicr_rd_base &&
		addr < (gicr->gicr_rd_base + gicr->gicr_rd_size)) {
			*offset = addr - gicr->gicr_rd_base;
			*type = TYPE_GIC_GICR_RD;
			return vgic->gicr[i];
		}
	}

	*type = TYPE_GIC_INVALID;
	return NULL;
}

int vgicv3_state_load(struct z_vcpu *vcpu, struct gicv3_vcpuif_ctxt *ctxt)
{
	vgicv3_lrs_load(ctxt);
	vgicv3_prios_load(ctxt);
	vgicv3_ctrls_load(ctxt);

	arch_vdev_irq_enable(vcpu);
	return 0;
}

int vgicv3_state_save(struct z_vcpu *vcpu, struct gicv3_vcpuif_ctxt *ctxt)
{
	vgicv3_lrs_save(ctxt);
	vgicv3_prios_save(ctxt);
	vgicv3_ctrls_save(ctxt);

	arch_vdev_irq_disable(vcpu);
	return 0;
}

int vcpu_gicv3_init(struct gicv3_vcpuif_ctxt *ctxt)
{

	ctxt->icc_sre_el1 = 0x07;
	ctxt->icc_ctlr_el1 = read_sysreg(ICC_CTLR_EL1);

	ctxt->ich_vmcr_el2 = GICH_VMCR_VENG1 | GICH_VMCR_DEFAULT_MASK;
	ctxt->ich_hcr_el2 = GICH_HCR_EN;

	return 0;
}

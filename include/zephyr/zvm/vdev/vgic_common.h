/*
 * Copyright 2024-2025 HNU-ESNL: Guoqi Xie, Chenglai Xiong, Xingyu Hu and etc.
 * Copyright 2024-2025 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZVM_ARM_VGIC_COMMON_H_
#define ZEPHYR_INCLUDE_ZVM_ARM_VGIC_COMMON_H_

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/arch/common/sys_bitops.h>
#include <zephyr/drivers/interrupt_controller/gic.h>
#include <zephyr/zvm/vm.h>
#include <zephyr/zvm/arm/switch.h>
#include <zephyr/zvm/zlog.h>

struct z_virt_dev;

#define VGIC_CONTROL_BLOCK_ID		vgic_control_block
#define VGIC_CONTROL_BLOCK_NAME		vm_irq_control_block

/* GIC version */
#define VGIC_V2			BIT(0)
#define VGIC_V3			BIT(8)

/* GIC dev type */
#define TYPE_GIC_GICD		BIT(0)
#define TYPE_GIC_GICR_RD	BIT(1)
#define TYPE_GIC_GICR_SGI	BIT(2)
#define TYPE_GIC_GICR_VLPI	BIT(3)
#define TYPE_GIC_INVALID	(0xFF)

/* GIC device macro here */
#define VGIC_DIST_BASE	DT_REG_ADDR_BY_IDX(DT_INST(0, arm_gic), 0)
#define VGIC_DIST_SIZE	DT_REG_SIZE_BY_IDX(DT_INST(0, arm_gic), 0)
#define VGIC_RDIST_BASE	DT_REG_ADDR_BY_IDX(DT_INST(0, arm_gic), 1)
#define VGIC_RDIST_SIZE	DT_REG_SIZE_BY_IDX(DT_INST(0, arm_gic), 1)

/* GICD registers offset from DIST_base(n) */
#define VGICD_CTLR			(GICD_CTLR-GIC_DIST_BASE)
#define VGICD_TYPER			(GICD_TYPER-GIC_DIST_BASE)
#define VGICD_IIDR			(GICD_IIDR-GIC_DIST_BASE)
#define VGICD_STATUSR		(GICD_STATUSR-GIC_DIST_BASE)
#define VGICD_ISENABLERn	(GICD_ISENABLERn-GIC_DIST_BASE)
#define VGICD_ICENABLERn	(GICD_ICENABLERn-GIC_DIST_BASE)
#define VGICD_ISPENDRn		(GICD_ISPENDRn-GIC_DIST_BASE)
#define VGICD_ICPENDRn		(GICD_ICPENDRn-GIC_DIST_BASE)

#define VGIC_RESERVED		0x0F30
#define VGIC_INMIRn			0x0f80
#define VGICD_PIDR2			0xFFE8

/* Vgic control block flag */
#define VIRQ_HW_SUPPORT				BIT(1)

#define VGIC_VIRQ_IN_SGI			(0x0)
#define VGIC_VIRQ_IN_PPI			(0x1)
/* Sorting virt irq to SGI/PPI/SPI */
#define VGIC_VIRQ_LEVEL_SORT(irq)	((irq)/VM_SGI_VIRQ_NR)

/* VGIC Type for virtual interrupt control */
#define VGIC_TYPER_REGISTER		(read_sysreg(ICH_VTR_EL2))
#define VGIC_TYPER_LR_NUM		((VGIC_TYPER_REGISTER & 0x1F) + 1)
#define VGIC_TYPER_PRIO_NUM		(((VGIC_TYPER_REGISTER >> 29) & 0x07) + 1)

/* 64k frame */
#define VGIC_RD_BASE_SIZE		(64 * 1024)
#define VGIC_SGI_BASE_SIZE		(64 * 1024)
#define VGIC_RD_SGI_SIZE		(VGIC_RD_BASE_SIZE+VGIC_SGI_BASE_SIZE)

/* virtual gic device register operation */
#define vgic_sysreg_read32(base, offset)	\
	sys_read32((unsigned long)(base+((offset)/4)))

#define vgic_sysreg_write32(data, base, offset)	\
	sys_write32(data, (unsigned long)(base+((offset)/4)))

#define vgic_sysreg_read64(base, offset)	\
	sys_read64((unsigned long)(base+((offset)/4)))

#define vgic_sysreg_write64(data, base, offset)	\
	sys_write64(data, (unsigned long)(base+((offset)/4)))

#define DEFAULT_DISABLE_IRQVAL	(0xFFFFFFFF)

/**
 * @brief Virtual generatic interrupt controller distributor
 * struct for each vm.
 */
struct virt_gic_gicd {
	/**
	 * gicd address base and size which
	 * are used to locate vdev access from
	 * vm.
	 */
	uint32_t gicd_base;
	uint32_t gicd_size;
	/* virtual gicr for emulating device for vm. */
	uint32_t *gicd_regs_base;

	/* gicd spin lock */
	struct k_spinlock gicd_lock;
};


typedef int (*vgic_gicd_read_32_t)(const struct device *dev, struct z_vcpu *vcpu,
					uint32_t offset, uint32_t *value);

typedef int (*vgic_gicrrd_read_32_t)(const struct device *dev, struct z_vcpu *vcpu,
					uint32_t offset, uint32_t *value);

typedef int (*vgic_gicd_write_32_t)(const struct device *dev, struct z_vcpu *vcpu,
					uint32_t offset, uint32_t *value);

typedef int (*vgic_gicrrd_write_32_t)(const struct device *dev, struct z_vcpu *vcpu,
					uint32_t offset, uint32_t *value);

__subsystem struct vgic_common_api {

	vgic_gicd_read_32_t		vgicd_read_32;
	vgic_gicd_write_32_t	vgicd_write_32;

	vgic_gicrrd_read_32_t	vgicr_rd_read_32;
	vgic_gicrrd_write_32_t	vgicr_write_32;

};

typedef int (*vm_irq_exit_t)(struct device *dev, struct z_vcpu *vcpu, void *data);

typedef int (*vm_irq_enter_t)(struct device *dev, struct z_vcpu *vcpu, void *data);

__subsystem struct vm_irq_handler_api {
	vm_irq_exit_t irq_exit_from_vm;
	vm_irq_enter_t irq_enter_to_vm;
};

uint32_t *arm_gic_get_distbase(struct z_virt_dev *vdev);

void z_ready_thread(struct k_thread *thread);

/**
 * @brief Just for enable menopoly irq for vcpu.
 */
void arch_vdev_irq_enable(struct z_vcpu *vcpu);

/**
 * @brief Just for disable menopoly irq for vcpu.
 */
void arch_vdev_irq_disable(struct z_vcpu *vcpu);


int vgic_vdev_mem_read(struct z_virt_dev *vdev, uint64_t addr, uint64_t *value, uint16_t size);
int vgic_vdev_mem_write(struct z_virt_dev *vdev, uint64_t addr, uint64_t *value, uint16_t size);

/**
 * @brief set/unset a virt irq signal to a vcpu.
 */
int set_virq_to_vcpu(struct z_vcpu *vcpu, uint32_t virq_num);

/**
 * @brief set/unset a virt irq to vm.
 */
int set_virq_to_vm(struct z_vm *vm, uint32_t virq_num);
int unset_virq_to_vm(struct z_vm *vm, uint32_t virq_num);

int virt_irq_sync_vgic(struct z_vcpu *vcpu);
int virt_irq_flush_vgic(struct z_vcpu *vcpu);

/**
 * @brief Get the virq desc object.
 */
struct virt_irq_desc *get_virt_irq_desc(struct z_vcpu *vcpu, uint32_t virq);

/**
 * @brief When vcpu is loop on idle mode, we must send virq
 * to activate it.
 */
static ALWAYS_INLINE void wakeup_target_vcpu(struct z_vcpu *vcpu, struct virt_irq_desc *desc)
{
	ARG_UNUSED(desc);
	/* Set thread into runnig queue */
	z_ready_thread(vcpu->work->vcpu_thread);
}

/**
 * @brief Check that whether this vm can receive virq?
 */
static ALWAYS_INLINE bool is_vm_irq_valid(struct z_vm *vm, uint32_t flag)
{
	if (vm->vm_status == VM_STATE_NEVER_RUN) {
		return false;
	}

	if (vm->vm_status == VM_STATE_PAUSE) {
		if (flag & VIRQ_WAKEUP_FLAG) {
			return true;
		} else {
			return false;
		}
	}
	return true;
}

static ALWAYS_INLINE struct virt_irq_desc *vgic_get_virt_irq_desc(
	struct z_vcpu *vcpu, uint32_t virq)
{
	struct z_vm *vm = vcpu->vm;

	/* sgi virq num */
	if (virq < VM_LOCAL_VIRQ_NR) {
		return &vcpu->virq_block.vcpu_virt_irq_desc[virq];
	}

	/* spi virq num */
	if ((virq >= VM_LOCAL_VIRQ_NR) && (virq < VM_GLOBAL_VIRQ_NR)) {
		return &vm->vm_irq_block.vm_virt_irq_desc[virq - VM_LOCAL_VIRQ_NR];
	}

	return NULL;
}

static ALWAYS_INLINE int vgic_irq_enable(struct z_vcpu *vcpu, uint32_t virt_irq)
{
	struct virt_irq_desc *desc;

	desc = vgic_get_virt_irq_desc(vcpu, virt_irq);
	if (!desc) {
		return -ENOENT;
	}
	desc->virq_flags |= VIRQ_ENABLED_FLAG;
	if (virt_irq > VM_LOCAL_VIRQ_NR) {
		/*TODO: How to route virtual device's irq to vcpu. */
		if (desc->virq_flags & VIRQ_HW_FLAG && vcpu->vcpu_id == 0) {
			if (desc->pirq_num > VM_LOCAL_VIRQ_NR) {
				irq_enable(desc->pirq_num);
			} else {
				return -ENODEV;
			}
		}
	} else {
		if (desc->virq_flags & VIRQ_HW_FLAG) {
			irq_enable(virt_irq);
		}
	}
	return 0;
}

static ALWAYS_INLINE int vgic_irq_disable(struct z_vcpu *vcpu, uint32_t virt_irq)
{
	struct virt_irq_desc *desc;

	desc = vgic_get_virt_irq_desc(vcpu, virt_irq);
	if (!desc) {
		return -ENOENT;
	}
	desc->virq_flags &= ~VIRQ_ENABLED_FLAG;
	if (virt_irq > VM_LOCAL_VIRQ_NR) {
		if (desc->virq_flags & VIRQ_HW_FLAG  && vcpu->vcpu_id == 0) {
			if (desc->pirq_num > VM_LOCAL_VIRQ_NR) {
				irq_disable(desc->pirq_num);
			} else {
				return -ENODEV;
			}
		}
	} else {
		if (desc->virq_flags & VIRQ_HW_FLAG) {
			irq_disable(virt_irq);
		}
	}
	return 0;
}

static ALWAYS_INLINE bool vgic_irq_test_bit(struct z_vcpu *vcpu, uint32_t spi_nr_count,
						uint32_t *value, uint32_t bit_size, bool enable)
{
	ARG_UNUSED(enable);
	ARG_UNUSED(spi_nr_count);
	int bit;
	uint32_t reg_mem_addr = (uint64_t)value;

	for (bit = 0; bit < bit_size; bit++) {
		if (sys_test_bit(reg_mem_addr, bit)) {
			return true;
		}
	}
	return false;
}

#endif /* ZEPHYR_INCLUDE_ZVM_ARM_VGIC_COMMON_H_ */

/*
 * Copyright 2024-2025 HNU-ESNL: Guoqi Xie, Chenglai Xiong, Xingyu Hu and etc.
 * Copyright 2024-2025 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZVM_ARM_VGIC_V3_H_
#define ZEPHYR_INCLUDE_ZVM_ARM_VGIC_V3_H_

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/spinlock.h>
#include <zephyr/drivers/interrupt_controller/gic.h>
#include <zephyr/arch/arm64/sys_io.h>
#include <zephyr/zvm/vm_device.h>
#include <zephyr/zvm/vm_irq.h>
#include <zephyr/zvm/zvm.h>
#include <zephyr/zvm/vdev/vgic_common.h>
#include "../../../drivers/interrupt_controller/intc_gicv3_priv.h"

/* SGI mode */
#define SGI_SIG_TO_LIST			(0)
#define SGI_SIG_TO_OTHERS		(1)

/* vgic macro here */
#define VGIC_MAX_VCPU	64
#define VGIC_UNDEFINE_ADDR	0xFFFFFFFF

/* vgic action */
#define ACTION_CLEAR_VIRQ		BIT(0)
#define ACTION_SET_VIRQ			BIT(1)

/* GIC control value */
#define GICH_VMCR_VENG0			(1 << 0)
#define GICH_VMCR_VENG1			(1 << 1)
#define GICH_VMCR_VACKCTL		(1 << 2)
#define GICH_VMCR_VFIQEN		(1 << 3)
#define GICH_VMCR_VCBPR			(1 << 4)
#define GICH_VMCR_VEOIM			(1 << 9)
#define GICH_VMCR_DEFAULT_MASK	(0xf8 << 24)

#define GICH_HCR_EN			(1 << 0)
#define GICH_HCR_UIE		(1 << 1)
#define GICH_HCR_LRENPIE	(1 << 2)
#define GICH_HCR_NPIE		(1 << 3)
#define GICH_HCR_TALL1		(1 << 12)

/* list register */
#define LIST_REG_GTOUP0			(0)
#define LIST_REG_GROUP1			(1)
#define LIST_REG_NHW_VIRQ		(0)
#define LIST_REG_HW_VIRQ		(1)

/* GICR registers offset from RDIST_base(n) */
#define VGICR_CTLR				GICR_CTLR
#define VGICR_IIDR				GICR_IIDR
#define VGICR_TYPER				GICR_TYPER
#define VGICR_STATUSR			GICR_STATUSR
#define VGICR_WAKER				GICR_WAKER
#define VGICR_PROPBASER			GICR_PROPBASER
#define VGICR_PENDBASER			GICR_PENDBASER
#define VGICR_ISENABLER0		0x0100
#define VGICR_ICENABLER0		0x0180
#define VGICR_SGI_PENDING		0x0200
#define VGICR_SGI_ICPENDING		0x0280
#define VGICR_PIDR2				0xFFE8

/* list register test and set */
#define VGIC_LIST_REGS_TEST(id, vcpu)	\
			((((struct z_vcpu *)vcpu)->arch->list_regs_map)\
			& (1 << id))
#define VGIC_LIST_REGS_UNSET(id, vcpu) ((((struct z_vcpu *)vcpu)->arch->list_regs_map)\
			= ((((struct z_vcpu *)vcpu)->arch->list_regs_map)\
			& (~(1 << id))))
#define VGIC_LIST_REGS_SET(id, vcpu) ((((struct z_vcpu *)vcpu)->arch->list_regs_map)\
			= ((((struct z_vcpu *)vcpu)->arch->list_regs_map)\
			| (1 << id)))
#define VGIC_ELRSR_REG_TEST(id, elrsr) ((1 << ((id)&0x1F)) & elrsr)

/**
 * @brief vcpu vgicv3 register interface.
 */
struct gicv3_vcpuif_ctxt {
	uint64_t ich_lr0_el2;
	uint64_t ich_lr1_el2;
	uint64_t ich_lr2_el2;
	uint64_t ich_lr3_el2;
	uint64_t ich_lr4_el2;
	uint64_t ich_lr5_el2;
	uint64_t ich_lr6_el2;
	uint64_t ich_lr7_el2;

	uint32_t ich_ap0r2_el2;
	uint32_t ich_ap1r2_el2;
	uint32_t ich_ap0r1_el2;
	uint32_t ich_ap1r1_el2;
	uint32_t ich_ap0r0_el2;
	uint32_t ich_ap1r0_el2;
	uint32_t ich_vmcr_el2;
	uint32_t ich_hcr_el2;

	uint32_t icc_ctlr_el1;
	uint32_t icc_sre_el1;
	uint32_t icc_pmr_el1;
};

/**
 * @brief gicv3_list_reg register bit field, which
 * provides interrupt context information for the virtual
 * CPU interface.
 */
struct gicv3_list_reg {
	uint64_t vINTID		: 32;
	uint64_t pINTID		: 13;
	uint64_t res0		: 3;
	uint64_t priority	: 8;
	uint64_t res1		: 3;
	uint64_t nmi		: 1;
	uint64_t group		: 1;
	uint64_t hw			: 1;
	uint64_t state		: 2;
};

/**
 * @brief Virtual generatic interrupt controller redistributor
 * struct for each vm's vcpu.
 * Each Redistributor defines four 64KB frames as follows:
 * 1. RD_base
 * 2. SGI_base
 * 3. VLPI_base
 * 4. Reserved
 * TODO: support vlpi later.
 */
struct virt_gic_gicr {
	uint32_t vcpu_id;

	/* virtual gicr for emulating device for vm. */
	uint32_t *gicr_rd_reg_base;
	uint32_t *gicr_sgi_reg_base;

	/**
	 * gicr address base and size which
	 * are used to locate vdev access from
	 * vm.
	 */
	uint32_t gicr_rd_base;
	uint32_t gicr_sgi_base;
	uint32_t gicr_rd_size;
	uint32_t gicr_sgi_size;

	struct k_spinlock gicr_lock;
};

/**
 * @brief vgicv3 virtual device struct, for emulate device.
 */
struct vgicv3_dev {
	struct virt_gic_gicd gicd;
	struct virt_gic_gicr *gicr[VGIC_RDIST_SIZE/VGIC_RD_SGI_SIZE];
};

/**
 * @brief virtual gicv3 device information.
 */
struct gicv3_vdevice {
	uint64_t gicd_base;
	uint64_t gicd_size;
	uint64_t gicr_base;
	uint64_t gicr_size;
};

/**
 * @brief gic vcpu interface init.
 */
int vcpu_gicv3_init(struct gicv3_vcpuif_ctxt *ctxt);

/**
 * @brief Check if the virtual interrupt is pending.
 */
bool virt_irq_ispending(struct z_vcpu *vcpu);

/**
 * @brief before enter vm, we need to load the vcpu interrupt state.
 */
int vgicv3_state_load(struct z_vcpu *vcpu, struct gicv3_vcpuif_ctxt *ctxt);

/**
 * @brief before exit from vm, we need to store the vcpu interrupt state.
 */
int vgicv3_state_save(struct z_vcpu *vcpu, struct gicv3_vcpuif_ctxt *ctxt);

/**
 * @brief send a virq to vm for el1 trap.
 */
int gicv3_inject_virq(struct z_vcpu *vcpu, struct virt_irq_desc *desc);

/**
 * @brief gic redistribute vdev mem read.
 */
int vgic_gicrsgi_mem_read(struct z_vcpu *vcpu, struct virt_gic_gicr *gicr,
			uint32_t offset, uint64_t *v);

/**
 * @brief gic redistribute sgi vdev mem write
 */
int vgic_gicrsgi_mem_write(
	struct z_vcpu *vcpu, struct virt_gic_gicr *gicr,
	uint32_t offset, uint64_t *v);

/**
 * @brief gic redistribute rd vdev mem read
 */
int vgic_gicrrd_mem_read(
	struct z_vcpu *vcpu, struct virt_gic_gicr *gicr,
	uint32_t offset, uint64_t *v);

/**
 * @brief gic redistribute rd vdev mem write.
 */
int vgic_gicrrd_mem_write(
	struct z_vcpu *vcpu, struct virt_gic_gicr *gicr,
	uint32_t offset, uint64_t *v);

/**
 * @brief get gicr address type.
 */
struct virt_gic_gicr *get_vcpu_gicr_type(
	struct vgicv3_dev *vgic, uint32_t addr,
	uint32_t *type, uint32_t *offset);
/**
 * @brief raise a sgi signal to a vcpu.
 */
int vgicv3_raise_sgi(struct z_vcpu *vcpu, unsigned long sgi_value);

/**
 * @brief init vgicv3 device for the vm.
 */
struct vgicv3_dev *vgicv3_dev_init(struct z_vm *vm);

/**
 * @brief When VMs enable or disable register, zvm will test
 * related bit and set it to correct value. This function is used
 * for irq enable or disable flag;
 */
static ALWAYS_INLINE void vgic_test_and_set_enable_bit(
	struct z_vcpu *vcpu, uint32_t spi_nr_count,
	uint32_t *value, uint32_t bit_size,
	bool enable, void *vgic_priv)
{
	int bit;
	uint32_t reg_mem_addr = (uint64_t)value;
	struct virt_gic_gicd *gicd = NULL;
	struct virt_gic_gicr *gicr = NULL;

	for (bit = 0; bit < bit_size; bit++) {
		if (sys_test_bit(reg_mem_addr, bit)) {
			if (enable) {
				vgic_irq_enable(vcpu, spi_nr_count + bit);
				if (spi_nr_count < VM_LOCAL_VIRQ_NR) {
					gicr = (struct virt_gic_gicr *)vgic_priv;
					vgic_sysreg_write32(
							vgic_sysreg_read32(
								gicr->gicr_sgi_reg_base,
								VGICR_ISENABLER0) | BIT(bit),
							gicr->gicr_sgi_reg_base, VGICR_ISENABLER0);
				} else {
					gicd = (struct virt_gic_gicd *)vgic_priv;
					vgic_sysreg_write32(
						vgic_sysreg_read32(
							gicd->gicd_regs_base,
							VGICD_ISENABLERn) | BIT(bit),
						gicd->gicd_regs_base, VGICD_ISENABLERn);
				}
			} else {
				/* TODO: add a situation for disable irq interrupt later */
				if (*value != DEFAULT_DISABLE_IRQVAL) {
					vgic_irq_disable(vcpu, spi_nr_count + bit);
				}
				if (spi_nr_count < VM_LOCAL_VIRQ_NR) {
					gicr = (struct virt_gic_gicr *)vgic_priv;
					vgic_sysreg_write32(
						vgic_sysreg_read32(
							gicr->gicr_sgi_reg_base,
							VGICR_ICENABLER0) & ~BIT(bit),
						gicr->gicr_sgi_reg_base, VGICR_ICENABLER0);
				} else {
					gicd = (struct virt_gic_gicd *)vgic_priv;
					vgic_sysreg_write32(
						vgic_sysreg_read32(
							gicd->gicd_regs_base,
							VGICD_ICENABLERn) | ~BIT(bit),
						gicd->gicd_regs_base, VGICD_ICENABLERn);
				}
			}
		}
	}
}

/**
 * @brief When VM write ispending or icpending flag, we
 * should set/unset irq signal to VM.
 */
static ALWAYS_INLINE void vgic_test_and_set_pending_bit(
	struct z_vcpu *vcpu, uint32_t spi_nr_count,
	uint32_t *value, uint32_t bit_size, bool enable, void *vgic_priv)
{
	int bit;
	uint32_t reg_mem_addr = (uint64_t)value;
	struct virt_gic_gicd *gicd = NULL;
	struct virt_gic_gicr *gicr = NULL;

	for (bit = 0; bit < bit_size; bit++) {
		if (sys_test_bit(reg_mem_addr, bit)) {
			if (enable) {
				if (spi_nr_count + bit >= VM_GLOBAL_VIRQ_NR) {
					/* spi num is too big. */
					return;
				}
				set_virq_to_vm(vcpu->vm, spi_nr_count + bit);
				if (spi_nr_count < VM_LOCAL_VIRQ_NR) {
					gicr = (struct virt_gic_gicr *)vgic_priv;
					vgic_sysreg_write32(
						vgic_sysreg_read32(
							gicr->gicr_sgi_reg_base,
							VGICR_SGI_PENDING) | BIT(bit),
						gicr->gicr_sgi_reg_base, VGICR_SGI_PENDING);
				} else {
					gicd = (struct virt_gic_gicd *)vgic_priv;
					vgic_sysreg_write32(
						vgic_sysreg_read32(
							gicd->gicd_regs_base,
							VGICD_ISPENDRn) | BIT(bit),
						gicd->gicd_regs_base, VGICD_ISPENDRn);
				}
			} else {
				if (spi_nr_count + bit >= VM_GLOBAL_VIRQ_NR) {
					return;
				}
				unset_virq_to_vm(vcpu->vm, spi_nr_count + bit);
				if (spi_nr_count < VM_LOCAL_VIRQ_NR) {
					gicr = (struct virt_gic_gicr *)vgic_priv;
					vgic_sysreg_write32(
						vgic_sysreg_read32(
							gicr->gicr_sgi_reg_base,
							VGICR_SGI_ICPENDING) & ~BIT(bit),
						gicr->gicr_sgi_reg_base, VGICR_SGI_ICPENDING);
				} else {
					gicd = (struct virt_gic_gicd *)vgic_priv;
					vgic_sysreg_write32(
						vgic_sysreg_read32(
							gicd->gicd_regs_base,
							VGICD_ICPENDRn) | ~BIT(bit),
						gicd->gicd_regs_base, VGICD_ICPENDRn);
				}
			}
		}
	}
}

static ALWAYS_INLINE uint64_t gicv3_read_lr(uint8_t register_id)
{
	switch (register_id) {
	case 0:
		return read_sysreg(ICH_LR0_EL2);
	case 1:
		return read_sysreg(ICH_LR1_EL2);
	case 2:
		return read_sysreg(ICH_LR2_EL2);
	case 3:
		return read_sysreg(ICH_LR3_EL2);
	case 4:
		return read_sysreg(ICH_LR4_EL2);
	case 5:
		return read_sysreg(ICH_LR5_EL2);
	case 6:
		return read_sysreg(ICH_LR6_EL2);
	case 7:
		return read_sysreg(ICH_LR7_EL2);
	default:
		return 0;
	}
}

static ALWAYS_INLINE void gicv3_write_lr(uint8_t register_id, uint64_t value)
{
	switch (register_id) {
	case 0:
		write_sysreg(value, ICH_LR0_EL2);
		break;
	case 1:
		write_sysreg(value, ICH_LR1_EL2);
		break;
	case 2:
		write_sysreg(value, ICH_LR2_EL2);
		break;
	case 3:
		write_sysreg(value, ICH_LR3_EL2);
		break;
	case 4:
		write_sysreg(value, ICH_LR4_EL2);
		break;
	case 5:
		write_sysreg(value, ICH_LR5_EL2);
		break;
	case 6:
		write_sysreg(value, ICH_LR6_EL2);
		break;
	case 7:
		write_sysreg(value, ICH_LR7_EL2);
		break;
	default:
		return;
	}
}

/**
 * @brief Get virq state from register.
 */
static ALWAYS_INLINE uint8_t gicv3_get_lr_state(
	struct z_vcpu *vcpu, struct virt_irq_desc *desc)
{
	uint64_t value;

	if (desc->id >=  VGIC_TYPER_LR_NUM) {
		return 0;
	}
	value = gicv3_read_lr(desc->id);
	value = (value >> 62) & 0x03;

	return (uint8_t)value;
}

/**
 * @brief Find the idle list register.
 */
static ALWAYS_INLINE uint8_t gicv3_get_idle_lr(struct z_vcpu *vcpu)
{
	uint8_t i;

	for (i = 0; i < VGIC_TYPER_LR_NUM; i++) {
		if (!VGIC_LIST_REGS_TEST(i, vcpu)) {
			return i;
		}
	}
	return -1;
}

/**
 * @brief update virt irq flags aim to reset virq here.
 */
static ALWAYS_INLINE void gicv3_update_lr(
	struct z_vcpu *vcpu, struct virt_irq_desc *desc,
	int action, uint64_t value)
{
	switch (action) {
	case ACTION_CLEAR_VIRQ:
		gicv3_write_lr(desc->id, 0);
		VGIC_LIST_REGS_UNSET(desc->id, vcpu);
		break;
	case ACTION_SET_VIRQ:
		gicv3_write_lr(desc->id, value);
		VGIC_LIST_REGS_SET(desc->id, vcpu);
		break;
	}
}

#endif /* ZEPHYR_INCLUDE_ZVM_ARM_VGIC_V3_H_ */

/*
 * Copyright 2024-2025 HNU-ESNL: Guoqi Xie, Chenglai Xiong, Xingyu Hu and etc.
 * Copyright 2024-2025 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZVM_ARM_CPU_H_
#define ZEPHYR_INCLUDE_ZVM_ARM_CPU_H_

#include <zephyr/kernel.h>
#include <zephyr/arch/arm64/cpu.h>
#include <zephyr/arch/arm64/exception.h>
#include <zephyr/arch/arm64/thread.h>
#include <zephyr/zvm/arm/asm.h>

#define HCR_VHE_FLAGS (HCR_RW_BIT | HCR_TGE_BIT | HCR_E2H_BIT)
 /* Host os NVHE flag */
#define HCR_NVHE_FLAGS (HCR_RW_BIT | HCR_API_BIT | HCR_APK_BIT | HCR_ATA_BIT)
/* Ignored bit: HCR_TVM, and ignore HCR_TSW to avoid cache DC trap */
#define HCR_VM_FLAGS (0UL | HCR_VM_BIT | HCR_FB_BIT |   HCR_AMO_BIT | \
		HCR_FMO_BIT |  HCR_IMO_BIT | HCR_BSU_IS_BIT | HCR_TAC_BIT | HCR_E2H_BIT | \
		HCR_TIDCP_BIT | HCR_RW_BIT | HCR_PTW_BIT)

/* Hypervisor cpu interface related register */
#define ICH_AP0R0_EL2	S3_4_C12_C8_0
#define ICH_AP0R1_EL2	S3_4_C12_C8_1
#define ICH_AP0R2_EL2	S3_4_C12_C8_2
#define ICH_AP1R0_EL2	S3_4_C12_C9_0
#define ICH_AP1R1_EL2	S3_4_C12_C9_1
#define ICH_AP1R2_EL2	S3_4_C12_C9_2
#define ICH_VSEIR_EL2	S3_4_C12_C9_4
#define ICH_SRE_EL2		S3_4_C12_C9_5
#define ICH_HCR_EL2		S3_4_C12_C11_0
#define ICH_EISR_EL2	S3_4_C12_C11_3
#define ICH_VTR_EL2		S3_4_C12_C11_1
#define ICH_VMCR_EL2	S3_4_C12_C11_7
#define ICH_LR0_EL2		S3_4_C12_C12_0
#define ICH_LR1_EL2		S3_4_C12_C12_1
#define ICH_LR2_EL2		S3_4_C12_C12_2
#define ICH_LR3_EL2		S3_4_C12_C12_3
#define ICH_LR4_EL2		S3_4_C12_C12_4
#define ICH_LR5_EL2		S3_4_C12_C12_5
#define ICH_LR6_EL2		S3_4_C12_C12_6
#define ICH_LR7_EL2		S3_4_C12_C12_7

/* commom reg macro */
#define __SYSREG_c0  0
#define __SYSREG_c1  1
#define __SYSREG_c2  2
#define __SYSREG_c3  3
#define __SYSREG_c4  4
#define __SYSREG_c5  5
#define __SYSREG_c6  6
#define __SYSREG_c7  7
#define __SYSREG_c8  8
#define __SYSREG_c9  9
#define __SYSREG_c10 10
#define __SYSREG_c11 11
#define __SYSREG_c12 12
#define __SYSREG_c13 13
#define __SYSREG_c14 14
#define __SYSREG_c15 15

#define __SYSREG_0   0
#define __SYSREG_1   1
#define __SYSREG_2   2
#define __SYSREG_3   3
#define __SYSREG_4   4
#define __SYSREG_5   5
#define __SYSREG_6   6
#define __SYSREG_7   7

/* ESR_ELX related register, May be mov to ../arm64/cpu.h */
#define ESR_SYSINS_OP0_MASK (0x00300000)
#define ESR_SYSINS_OP0_SHIFT (20)
#define ESR_SYSINS_OP2_MASK (0x000e0000)
#define ESR_SYSINS_OP2_SHIFT (17)
#define ESR_SYSINS_OP1_MASK (0x0001c000)
#define ESR_SYSINS_OP1_SHIFT (14)
#define ESR_SYSINS_CRN_MASK (0x00003c00)
#define ESR_SYSINS_CRN_SHIFT (10)
#define ESR_SYSINS_RT_MASK (0x000003e0)
#define ESR_SYSINS_RT_SHIFT (5)
#define ESR_SYSINS_CRM_MASK (0x0000001e)
#define ESR_SYSINS_CRM_SHIFT (1)
#define ESR_SYSINS_REGS_MASK (ESR_SYSINS_OP0_MASK|ESR_SYSINS_OP2_MASK|\
							  ESR_SYSINS_OP1_MASK|ESR_SYSINS_CRN_MASK|\
							  ESR_SYSINS_CRM_MASK)

#define ESR_SYSINS(op0, op1, crn, crm, op2) \
	(((__SYSREG_##op0) << ESR_SYSINS_OP0_SHIFT) | \
	 ((__SYSREG_##op1) << ESR_SYSINS_OP1_SHIFT) | \
	 ((__SYSREG_##crn) << ESR_SYSINS_CRN_SHIFT) | \
	 ((__SYSREG_##crm) << ESR_SYSINS_CRM_SHIFT) | \
	 ((__SYSREG_##op2) << ESR_SYSINS_OP2_SHIFT))

#define ESR_SYSINSREG_SGI1R_EL1		ESR_SYSINS(3, 0, c12, c11, 5)
#define ESR_SYSINSREG_ASGI1R_EL1	ESR_SYSINS(3, 1, c12, c11, 6)
#define ESR_SYSINSREG_SGI0R_EL1		ESR_SYSINS(3, 2, c12, c11, 7)
#define ESR_SYSINSERG_CTLR_EL1		ESR_SYSINS(3, 0, c12, c12, 4)
#define ESR_SYSINSREG_CNTPCT_EL0	ESR_SYSINS(3, 3, c14, c0, 0)
#define ESR_SYSINSREG_CNTP_TVAL_EL0	ESR_SYSINS(3, 3, c14, c2, 0)
#define ESR_SYSINSREG_CNTP_CTL_EL0	ESR_SYSINS(3, 3, c14, c2, 1)
#define ESR_SYSINSREG_CNTP_CVAL_EL0	ESR_SYSINS(3, 3, c14, c2, 2)

enum {
	VCPU_MPIDR_EL1,
	VCPU_CSSELR_EL1,
	VCPU_SCTLR_EL1,
	VCPU_ACTLR_EL1,
	VCPU_CPACR_EL1,
	VCPU_TTBR0_EL1,
	VCPU_TTBR1_EL1,
	VCPU_TCR_EL1,
	VCPU_ESR_EL1,
	VCPU_AFSR0_EL1,
	VCPU_AFSR1_EL1,
	VCPU_FAR_EL1,
	VCPU_MAIR_EL1,
	VCPU_VBAR_EL1,
	VCPU_CONTEXTIDR_EL1,
	VCPU_TPIDR_EL0,
	VCPU_TPIDRRO_EL0,
	VCPU_TPIDR_EL1,
	VCPU_AMAIR_EL1,
	VCPU_CNTKCTL_EL1,
	VCPU_PAR_EL1,
	VCPU_MDSCR_EL1,
	VCPU_DISR_EL1,
	VCPU_ELR_EL1,
	VCPU_SP_EL1,
	VCPU_SPSR_EL1,
	VCPU_VPIDR,
	VCPU_SYS_REG_NUM
};

struct arch_commom_regs {

	struct _callee_saved callee_saved_regs;
	struct arch_esf esf_handle_regs;

	uint64_t pc;
	uint64_t pstate;
	uint64_t lr;

};
typedef struct arch_commom_regs arch_commom_regs_t;

struct zvm_vcpu_context {
	struct arch_commom_regs regs;
	struct z_vcpu *running_vcpu;
	uint64_t sys_regs[VCPU_SYS_REG_NUM];
};
typedef struct zvm_vcpu_context zvm_vcpu_context_t;

struct vcpu_fault_info {
	uint64_t esr_el2;
	uint64_t disr_el1;
	uint64_t far_el2;
	uint64_t hpfar_el2;
};

struct vcpu_arch {
	struct zvm_vcpu_context ctxt;
	struct zvm_vcpu_context host_ctxt;

	/* Don't run the guest on this vcpu */
	bool pause;
	bool first_run_vcpu;
	bool vcpu_sys_register_loaded;

	/* HYP configuration. */
	uint64_t hcr_el2;

	uint64_t host_mdcr_el2;
	uint64_t guest_mdcr_el2;

	/* arm gic list register bitmap for recording used lr */
	uint64_t list_regs_map;

	/* Exception information. */
	struct vcpu_fault_info fault;

	struct virt_timer_context *vtimer_context;
	void *virq_data;
};
typedef struct vcpu_arch vcpu_arch_t;

/* vector and hyp_vector function */
extern void *_vector_table[];
extern void _hyp_vector_table(void);

uint64_t *find_index_reg(uint16_t index, arch_commom_regs_t *regs);

void vcpu_sysreg_load(struct z_vcpu *vcpu);
void vcpu_sysreg_save(struct z_vcpu *vcpu);

void arch_vcpu_context_load(struct z_vcpu *vcpu);
void arch_vcpu_context_save(struct z_vcpu *vcpu);

void switch_to_guest_sysreg(struct z_vcpu *vcpu);
void switch_to_host_sysreg(struct z_vcpu *vcpu);

int arch_vcpu_init(struct z_vcpu *vcpu);
int arch_vcpu_deinit(struct z_vcpu *vcpu);

int zvm_arch_init(void *op);

#endif  /*ZEPHYR_INCLUDE_ZVM_ARM_CPU_H_*/

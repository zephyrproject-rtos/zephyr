/*
 * Copyright 2024-2025 HNU-ESNL: Guoqi Xie, Chenglai Xiong, Xingyu Hu and etc.
 * Copyright 2024-2025 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZVM_ARM_SWITCH_H__
#define ZEPHYR_INCLUDE_ZVM_ARM_SWITCH_H__

#include <zephyr/kernel.h>
#include <zephyr/arch/arm64/exception.h>

/* VM interrupt description MACRO */
#define VM_BASE_VIRQ_NR		(256)
#define VM_SGI_VIRQ_NR		(16)
#define VM_PPI_VIRQ_NR		(16)
#define VM_SPI_VIRQ_NR		(VM_BASE_VIRQ_NR - VM_SGI_VIRQ_NR - VM_PPI_VIRQ_NR)

#define ISS_SAS_8BIT		(0x0)
#define ISS_SAS_16BIT		(0x1)
#define ISS_SAS_32BIT		(0x2)
#define ISS_SAS_64BIT		(0x3)

/* HPFAR_EL2 addr mask */
#define HPFAR_EL2_MASK			GENMASK(39, 4)
#define HPFAR_EL2_SHIFT			(4)
#define HPFAR_EL2_PAGE_MASK		GENMASK(11, 0)
#define HPFAR_EL2_PAGE_SHIFT	(12)

struct z_vcpu;

struct esr_dabt_area {
	uint64_t dfsc   :6;     /* Data Fault Status Code */
	uint64_t wnr    :1;     /* Write / not Read */
	uint64_t s1ptw  :1;     /* Stage 2 fault during stage 1 translation */
	uint64_t cm     :1;     /* Cache Maintenance */
	uint64_t ea     :1;     /* External Abort Type */
	uint64_t fnv    :1;     /* FAR not Valid */
	uint64_t set    :2;     /* Synchronous Error Type */
	uint64_t vncr   :1;     /* Indicates that the fault came from use of VNCR_EL2.*/
	uint64_t ar     :1;     /* Acquire Release */
	uint64_t sf     :1;     /* Sixty Four bit register */
	uint64_t srt    :5;     /* The Register which store the value */
	uint64_t sse    :1;     /* Sign extend */
	uint64_t sas    :2;     /* Syndrome Access Size */
	uint64_t isv    :1;     /* Syndrome Valid */
	uint64_t il     :1;     /* Instruction length */
	uint64_t ec     :6;		/* Exception Class */
	uint64_t ISS2	:5;		/* FEAT_LS64 is Implemented */
	uint64_t res	:27;	/* RES0 */
};

struct esr_sysreg_area {
	uint64_t dire	:1;   /* Direction */
	uint64_t crm	:4;    /* CRm */
	uint64_t rt		:5;    /* Rt */
	uint64_t crn	:4;    /* CRn */
	uint64_t op1	:3;    /* Op1 */
	uint64_t op2	:3;    /* Op2 */
	uint64_t op0	:2;    /* Op0 */
	uint64_t res0	:3;	   /* reserved file */
	uint64_t il		:1;    /* Instruction length */
	uint64_t ec		:6;	   /* Exception Class */
};

/**
 * @brief sync handler for this vm.
 */
void *z_vm_lower_sync_handler(uint64_t esr_elx);

/**
 * @brief irq handler for this vm.
 */
void *z_vm_lower_irq_handler(struct arch_esf *esf_ctxt);

/**
 * @brief ready to run vcpu here, for prepare running guest code.
 * This function aim to make preparetion before running guest os and restore
 * the origin hardware state after guest exit.
 */
int arch_vcpu_run(struct z_vcpu *vcpu);

/**
 * @brief Avoid switch handle when current thread is a vcpu thread,
 * and curretn irq is send to vcpu.
 * @retval
 * true: this irq is sent to vcpu.
 * false: this irq is a normal irq.
 */
bool zvm_switch_handle_pre(uint32_t irq);

/**
 * @brief Get the zvm host context object for context switch
 */
uint64_t get_zvm_host_context(void);

#endif /* ZEPHYR_INCLUDE_ZVM_ARM_SWITCH_H__ */

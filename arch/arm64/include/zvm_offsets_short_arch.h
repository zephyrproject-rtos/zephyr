/*
 * Copyright 2024-2025 HNU-ESNL: Guoqi Xie, Chenglai Xiong, Xingyu Hu and etc.
 * Copyright 2024-2025 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZVM_ARCH_ARM64_INCLUDE_OFFSETS_SHORT_ARCH_H_
#define ZVM_ARCH_ARM64_INCLUDE_OFFSETS_SHORT_ARCH_H_

#include <offsets.h>

/* below macro is for hyp code offset */
#define _zvm_vcpu_ctxt_arch_regs_to_callee_saved_x19_20 \
	(__zvm_vcpu_context_t_regs_OFFSET + __arch_commom_regs_t_callee_saved_regs_OFFSET + \
	___callee_saved_t_x19_x20_OFFSET)
#define _zvm_vcpu_ctxt_arch_regs_to_callee_saved_x21_x22 \
	(__zvm_vcpu_context_t_regs_OFFSET + __arch_commom_regs_t_callee_saved_regs_OFFSET + \
	___callee_saved_t_x21_x22_OFFSET)
#define _zvm_vcpu_ctxt_arch_regs_to_callee_saved_x23_x24 \
	(__zvm_vcpu_context_t_regs_OFFSET + __arch_commom_regs_t_callee_saved_regs_OFFSET + \
	___callee_saved_t_x23_x24_OFFSET)
#define _zvm_vcpu_ctxt_arch_regs_to_callee_saved_x25_x26 \
	(__zvm_vcpu_context_t_regs_OFFSET + __arch_commom_regs_t_callee_saved_regs_OFFSET + \
	___callee_saved_t_x25_x26_OFFSET)
#define _zvm_vcpu_ctxt_arch_regs_to_callee_saved_x27_x28 \
	(__zvm_vcpu_context_t_regs_OFFSET + __arch_commom_regs_t_callee_saved_regs_OFFSET + \
	___callee_saved_t_x27_x28_OFFSET)
#define _zvm_vcpu_ctxt_arch_regs_to_callee_saved_x29_sp_el0 \
	(__zvm_vcpu_context_t_regs_OFFSET + __arch_commom_regs_t_callee_saved_regs_OFFSET + \
	___callee_saved_t_x29_sp_el0_OFFSET)
#define _zvm_vcpu_ctxt_arch_regs_to_callee_saved_sp_elx \
	(__zvm_vcpu_context_t_regs_OFFSET + __arch_commom_regs_t_callee_saved_regs_OFFSET + \
	___callee_saved_t_sp_elx_lr_OFFSET)

#define _zvm_vcpu_ctxt_arch_regs_to_esf_t_x0_x1 \
	(__zvm_vcpu_context_t_regs_OFFSET + __arch_commom_regs_t_esf_handle_regs_OFFSET + \
	___esf_t_x0_x1_OFFSET)
#define _zvm_vcpu_ctxt_arch_regs_to_esf_t_x2_x3 \
	(__zvm_vcpu_context_t_regs_OFFSET + __arch_commom_regs_t_esf_handle_regs_OFFSET + \
	___esf_t_x2_x3_OFFSET)
#define _zvm_vcpu_ctxt_arch_regs_to_esf_t_x4_x5 \
	(__zvm_vcpu_context_t_regs_OFFSET + __arch_commom_regs_t_esf_handle_regs_OFFSET + \
	___esf_t_x4_x5_OFFSET)
#define _zvm_vcpu_ctxt_arch_regs_to_esf_t_x6_x7 \
	(__zvm_vcpu_context_t_regs_OFFSET + __arch_commom_regs_t_esf_handle_regs_OFFSET + \
	___esf_t_x6_x7_OFFSET)
#define _zvm_vcpu_ctxt_arch_regs_to_esf_t_x8_x9 \
	(__zvm_vcpu_context_t_regs_OFFSET + __arch_commom_regs_t_esf_handle_regs_OFFSET + \
	___esf_t_x8_x9_OFFSET)
#define _zvm_vcpu_ctxt_arch_regs_to_esf_t_x10_x11 \
	(__zvm_vcpu_context_t_regs_OFFSET + __arch_commom_regs_t_esf_handle_regs_OFFSET + \
	___esf_t_x10_x11_OFFSET)
#define _zvm_vcpu_ctxt_arch_regs_to_esf_t_x12_x13 \
	(__zvm_vcpu_context_t_regs_OFFSET + __arch_commom_regs_t_esf_handle_regs_OFFSET + \
	___esf_t_x12_x13_OFFSET)
#define _zvm_vcpu_ctxt_arch_regs_to_esf_t_x14_x15 \
	(__zvm_vcpu_context_t_regs_OFFSET + __arch_commom_regs_t_esf_handle_regs_OFFSET + \
	___esf_t_x14_x15_OFFSET)
#define _zvm_vcpu_ctxt_arch_regs_to_esf_t_x16_x17 \
	(__zvm_vcpu_context_t_regs_OFFSET + __arch_commom_regs_t_esf_handle_regs_OFFSET + \
	___esf_t_x16_x17_OFFSET)
#define _zvm_vcpu_ctxt_arch_regs_to_esf_t_x18_lr \
	(__zvm_vcpu_context_t_regs_OFFSET + __arch_commom_regs_t_esf_handle_regs_OFFSET + \
	___esf_t_x18_lr_OFFSET)

#define _vcpu_arch_to_ctxt \
	(__vcpu_t_arch_OFFSET + __vcpu_arch_t_ctxt_OFFSET)

#define _vcpu_context_t_regs_to_lr \
	(__zvm_vcpu_context_t_regs_OFFSET + __arch_commom_regs_t_lr_OFFSET)

#endif /* ZVM_ARCH_ARM64_INCLUDE_OFFSETS_SHORT_ARCH_H_ */

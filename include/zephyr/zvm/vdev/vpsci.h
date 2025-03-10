/*
 * Copyright 2024-2025 HNU-ESNL: Guoqi Xie, Chenglai Xiong, Xingyu Hu and etc.
 * Copyright 2024-2025 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZVM_VDEV_VIRT_PSCI_H_
#define ZEPHYR_INCLUDE_ZVM_VDEV_VIRT_PSCI_H_

#include <zephyr/zvm/arm/cpu.h>
#include <zephyr/zvm/zlog.h>

/* psci func for vcpu */
uint64_t psci_vcpu_suspend(struct z_vcpu *vcpu, arch_commom_regs_t *arch_ctxt);
uint64_t psci_vcpu_off(struct z_vcpu *vcpu, arch_commom_regs_t *arch_ctxt);
uint64_t psci_vcpu_affinity_info(struct z_vcpu *vcpu, arch_commom_regs_t *arch_ctxt);
uint64_t psci_vcpu_migration(struct z_vcpu *vcpu, arch_commom_regs_t *arch_ctxt);
uint64_t psci_vcpu_migration_info_type(struct z_vcpu *vcpu, arch_commom_regs_t *arch_ctxt);
uint64_t psci_vcpu_other(unsigned long psci_func);
uint64_t psci_vcpu_on(struct z_vcpu *vcpu, arch_commom_regs_t *arch_ctxt);

int do_psci_call(struct z_vcpu *vcpu, arch_commom_regs_t *arch_ctxt);

#endif /* ZEPHYR_INCLUDE_ZVM_VDEV_VIRT_PSCI_H_ */

/*
 * Copyright 2024-2025 HNU-ESNL: Guoqi Xie, Chenglai Xiong, Xingyu Hu and etc.
 * Copyright 2024-2025 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZVM_VM_MANAGER_H_
#define ZEPHYR_INCLUDE_ZVM_VM_MANAGER_H_

#include <errno.h>
#include <zephyr/arch/arm64/cpu.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/printk.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/zvm/vm.h>

/**
 * @brief init vm struct.
 * When creating a vm, we must load the vm image and parse it
 * if it is a complicated system.
 */
typedef void (*vm_init_t)(struct z_vm *vm);
typedef void (*vcpu_init_t)(struct z_vcpu *vcpu);
typedef void (*vcpu_run_t)(struct z_vcpu *vcpu);
typedef void (*vcpu_halt_t)(struct z_vcpu *vcpu);

/**
 * @brief VM mapping vir_ad to phy_ad.
 */
typedef int (*vm_mmap_t)(struct vm_mem_domain *vmem_domain);
typedef void (*vmm_init_t)(struct z_vm *vm);
typedef int (*vint_init_t)(struct z_vcpu *vcpu);
typedef int (*vtimer_init_t)(struct z_vcpu *vcpu);

/**
 * @brief VM's vcpu ops function
 */
struct vm_ops {
	vm_init_t	   vm_init;

	vcpu_init_t	 vcpu_init;
	vcpu_run_t	  vcpu_run;
	vcpu_halt_t	 vcpu_halt;

	vmm_init_t	  vmm_init;
	vm_mmap_t	   vm_mmap;

	/* @TODO maybe add load/restor func later */
	vint_init_t	 vint_init;
	vtimer_init_t   vtimer_init;
};


int zvm_new_guest(size_t argc, char **argv);
int zvm_run_guest(size_t argc, char **argv);
int zvm_pause_guest(size_t argc, char **argv);
int zvm_delete_guest(size_t argc, char **argv);
int zvm_info_guest(size_t argc, char **argv);


/**
 * @brief shutdown guest
 */
void zvm_shutdown_guest(struct z_vm *vm);

/**
 * @brief reset guset
 */
void zvm_reboot_guest(struct z_vm *vm);


#endif /* ZEPHYR_INCLUDE_ZVM_VM_MANAGER_H_ */

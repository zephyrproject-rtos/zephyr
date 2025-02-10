/*
 * Copyright 2024-2025 HNU-ESNL: Guoqi Xie, Chenglai Xiong, Xingyu Hu and etc.
 * Copyright 2024-2025 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>
#include <ksched.h>
#include <zephyr/arch/arch_interface.h>
#include <zephyr/zvm/os.h>
#include <zephyr/zvm/zvm.h>
#include <zephyr/zvm/vdev/vgic_v3.h>
#include <zephyr/zvm/vm_cpu.h>
#include <zephyr/zvm/vm.h>
#include "../../lib/posix/options/getopt/getopt.h"


LOG_MODULE_DECLARE(ZVM_MODULE_NAME);

int zvm_new_guest(size_t argc, char **argv)
{
	int ret;
	struct z_vm *new_vm = NULL;
	struct z_os_info *vm_info = NULL;
	struct getopt_state *state;

	state = getopt_state_get();

	if (is_vmid_full()) {
		ZVM_LOG_WARN("System vm's num has reached the limit.\n");
		return -ENXIO;
	}

	new_vm = (struct z_vm *)k_malloc(sizeof(struct z_vm));
	if (!new_vm) {
		ZVM_LOG_WARN("Allocation memory for VM Error!\n");
		return -ENOMEM;
	}

	vm_info = (struct z_os_info *)k_malloc(sizeof(struct z_os_info));
	if (!vm_info) {
		k_free(new_vm);
		ZVM_LOG_WARN("Allocation memory for VM info Error!\n");
		return -ENOMEM;
	}

	ret = vm_sysinfo_init(argc, argv, new_vm, vm_info);
	if (ret) {
		return ret;
	}

	ret = vm_create(vm_info, new_vm);
	if (ret) {
		k_free(new_vm);
		k_free(vm_info);
		ZVM_LOG_WARN("Can not create vm struct, VM struct init failed!\n");
		return ret;
	}
	ZVM_LOG_INFO("\n**Create VM instance successful!\n");

	ret = vm_ops_init(new_vm);
	if (ret) {
		ZVM_LOG_WARN("VM ops init failed!\n");
		return ret;
	}
	ZVM_LOG_INFO("** Init VM ops successful!\n");

	ret = vm_irq_block_init(new_vm);
	if (ret < 0) {
		ZVM_LOG_WARN(" Init vm's irq block error!\n");
		return ret;
	}
	ZVM_LOG_INFO("** Init VM irq block successful!\n");

	ret = vm_vcpus_init(new_vm);
	if (ret < 0) {
		ZVM_LOG_WARN("create vcpu error!\n");
		return -ENXIO;
	}
	ZVM_LOG_INFO("** Init VM vcpus instances successful!\n");

	ret = vm_device_init(new_vm);
	if (ret) {
		ZVM_LOG_WARN(" Init vm's virtual device error!\n");
		return ret;
	}
	ZVM_LOG_INFO("** Init VM devices successful!\n");

	ret = vm_mem_init(new_vm);
	if (ret < 0) {
		return ret;
	}
	ZVM_LOG_INFO("** Init VM memory successful!\n");
	k_free(vm_info);

	ZVM_LOG_INFO("\n|*********************************************|\n");
	ZVM_LOG_INFO("|******  Create vm successful!  **************|\n");
	ZVM_LOG_INFO("|****** \t VM INFO\t \t******|\n");
	ZVM_LOG_INFO("|******  VM-NAME:	 %-12s\t******|\n", new_vm->vm_name);
	ZVM_LOG_INFO("|******  VM-ID:\t\t%-12d\t******|\n", new_vm->vmid);
	ZVM_LOG_INFO("|******  VCPU NUM:\t%-12d\t******|\n", new_vm->vcpu_num);
	switch (new_vm->os->info.os_type) {
	case OS_TYPE_LINUX:
		ZVM_LOG_INFO("|******  VMEM SIZE:   %-12d(M)   ******|\n",
		LINUX_VM_MEMORY_SIZE/(1024*1024));
		break;
	case OS_TYPE_ZEPHYR:
		ZVM_LOG_INFO("|******  VMEM SIZE:   %-12d(M)   ******|\n",
		ZEPHYR_VM_MEMORY_SIZE/(1024*1024));
		break;
	default:
		ZVM_LOG_INFO("|******  OTHER VM, NO MEMORY MSG ******|\n");
	}
	ZVM_LOG_INFO("|*********************************************|\n");

	return 0;
}

int zvm_run_guest(size_t argc, char **argv)
{
	uint16_t vm_id;
	int ret = 0;
	struct z_vm *vm;

	ZVM_LOG_INFO("** Ready to run VM.\n");
	vm_id = z_parse_run_vm_args(argc, argv);
	if (!(BIT(vm_id) & zvm_overall_info->alloced_vmid)) {
		ZVM_LOG_WARN("This vmid is not exist!\n Please input zvm info to show info!\n");
		return -EINVAL;
	}

	vm = zvm_overall_info->vms[vm_id];
	if (vm->vm_status & VM_STATE_RUNNING) {
		ZVM_LOG_WARN("This vm is already running!\n Please input zvm info to check vms!\n");
		return -EINVAL;
	}

	if (vm->vm_status & (VM_STATE_NEVER_RUN | VM_STATE_PAUSE)) {
		if (vm->vm_status & VM_STATE_NEVER_RUN) {
			load_os_image(vm);
		}
		vm_vcpus_ready(vm);
	} else {
		ZVM_LOG_WARN("The VM has a invalid status, abort!\n");
		return -ENODEV;
	}

	ZVM_LOG_INFO("\n|*********************************************|\n");
	ZVM_LOG_INFO("|******\t Start vm successful!  ***************|\n");
	ZVM_LOG_INFO("|******\t\t VM INFO \t \t******|\n");
	ZVM_LOG_INFO("|******\t VM-NAME:	 %s \t******|\n", vm->vm_name);
	ZVM_LOG_INFO("|******\t VM-ID: \t %d \t\t******|\n", vm->vmid);
	ZVM_LOG_INFO("|******\t VCPU NUM: \t %d \t\t******|\n", vm->vcpu_num);
	ZVM_LOG_INFO("|*********************************************|\n");

	return ret;
}

int zvm_pause_guest(size_t argc, char **argv)
{
	uint16_t vm_id;
	int ret = 0;
	struct z_vm *vm;
	k_spinlock_key_t key;

	key = k_spin_lock(&zvm_overall_info->spin_zmi);

	vm_id = z_parse_pause_vm_args(argc, argv);
	if (!(BIT(vm_id) & zvm_overall_info->alloced_vmid)) {
		ZVM_LOG_WARN("This vmid is not exist!\n Please input zvm info to show info!\n");
		k_spin_unlock(&zvm_overall_info->spin_zmi, key);
		return -EINVAL;
	}

	vm = zvm_overall_info->vms[vm_id];
	k_spin_unlock(&zvm_overall_info->spin_zmi, key);
	if (vm->vm_status != VM_STATE_RUNNING) {
		ZVM_LOG_WARN("This vm is not running!\n No need to pause it!\n");
		return -EPERM;
	}
	ret = vm_vcpus_pause(vm);

	return ret;
}

int zvm_delete_guest(size_t argc, char **argv)
{
	uint16_t vm_id;
	int i;
	struct z_vm *vm;

	vm_id = z_parse_delete_vm_args(argc, argv);
	if (!(BIT(vm_id) & zvm_overall_info->alloced_vmid)) {
		ZVM_LOG_WARN("This vm is not exist!\n Please input zvm info to list vms!");
		return 0;
	}

	vm = zvm_overall_info->vms[vm_id];
	switch (vm->vm_status) {
	case VM_STATE_RUNNING:
		ZVM_LOG_INFO("This vm is running!\n Try to stop and delete it!\n");
		vm_vcpus_halt(vm);

		for (i = 0; i < vm->vcpu_num; i++) {
			k_sem_take(&vm->vcpu_exit_sem[i], K_FOREVER);
		}
		barrier_isync_fence_full();
		vm_delete(vm);
		break;
	case VM_STATE_PAUSE:
		ZVM_LOG_INFO("This vm is paused!\n Just delete it!\n");
		vm_delete(vm);
		break;
	case VM_STATE_NEVER_RUN:
		ZVM_LOG_INFO("This vm is created but not run!\n Just delete it!\n");
		vm_delete(vm);
		break;
	default:
		ZVM_LOG_WARN("This vm status is invalid!\n");
		return -ENODEV;
	}

	return 0;
}

int zvm_info_guest(size_t argc, char **argv)
{
	int ret = 0;

	if (zvm_overall_info->vm_total_num > 0) {
		ret = z_list_vms_info(0);
	} else {
		ret = -ENODEV;
	}

	return ret;
}

/*TODO: add shell*/
void zvm_shutdown_guest(struct z_vm *vm)
{
	ARG_UNUSED(vm);
}

void zvm_reboot_guest(struct z_vm *vm)
{
	int ret;

	ZVM_LOG_INFO("vm reboot....\n");
	ret = vm_vcpus_pause(vm);
	if (ret < 0) {
		ZVM_LOG_WARN("VM reboot failed: pausing vm failed!\n");
	}
	/*
	 * TODO: smp
	 */
	vm_vcpus_reset(vm);
	vm->reboot = true;
	vm_vcpus_ready(vm);
}

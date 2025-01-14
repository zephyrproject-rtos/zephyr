/*
 * Copyright 2024-2025 HNU-ESNL: Guoqi Xie, Charlie, Xingyu Hu and etc.;
 * Copyright 2024-2025 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/toolchain.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/thread.h>
#include <zephyr/kernel/thread_stack.h>
#include <kernel_internal.h>
#include <zephyr/logging/log.h>
#include <zephyr/zvm/vm.h>
#include <zephyr/zvm/zvm.h>
#include <zephyr/zvm/vdev/vgic_common.h>
#include "../../lib/posix/options/getopt/getopt.h"

LOG_MODULE_DECLARE(ZVM_MODULE_NAME);

extern struct zvm_manage_info *zvm_overall_info;

static int intra_vm_msg_handler(struct z_vm *vm)
{
    ARG_UNUSED(vm);
    struct z_vcpu *vcpu = _current_vcpu;
    if (!vcpu) {
        ZVM_LOG_WARN("Get current vcpu failed! \n");
        return -ENODEV;
    }
    set_virq_to_vcpu(vcpu, vcpu->virq_block.pending_sgi_num);

    return 0;
}

static int pause_vm_handler(struct z_vm *vm)
{
    ARG_UNUSED(vm);
    return 0;
}

static int stop_vm_handler(struct z_vm *vm)
{
    ARG_UNUSED(vm);
    return 0;
}

static void list_vm_info(uint16_t vmid)
{
    char *vm_ss;
    int mem_size = 0;
    struct z_vm *vm = zvm_overall_info->vms[vmid];

    if (!vm) {
        ZVM_LOG_WARN("Invalid vmid!\n");
        return;
    }

    /* Get the vm's status */
    switch (vm->vm_status) {
    case VM_STATE_RUNNING:
        vm_ss = "running";
        break;
    case VM_STATE_PAUSE:
        vm_ss = "pausing";
        break;
    case VM_STATE_NEVER_RUN:
        vm_ss = "Ready";
        break;
    case VM_STATE_HALT:
        vm_ss = "stopping";
        break;
    case VM_STATE_RESET:
        vm_ss = "reset";
        break;
    default:
        ZVM_LOG_WARN("This vm status is invalid!\n");
        return;
	}

    mem_size = vm->os->info.vm_mem_size / (1024*1024);
    printk("|***%d  %s\t%d\t%d \t%s ***| \n", vm->vmid,
            vm->vm_name, vm->vcpu_num, mem_size, vm_ss);

}

static void list_all_vms_info(void)
{
    uint16_t i;

    printk("\n|******************** All VMS INFO *******************|\n");
    printk("|***vmid name \t    vcpus    vmem(M)\tstatus ***|\n");
    for(i = 0; i < CONFIG_MAX_VM_NUM; i++){
        if(BIT(i) & zvm_overall_info->alloced_vmid)
            list_vm_info(i);
    }
}

int vm_ipi_handler(struct z_vm *vm)
{
    int ret;
    uint32_t vm_status;

    vm_status = vm->vm_status;
    switch (vm_status) {
    case VM_STATE_RUNNING:
        ret = intra_vm_msg_handler(vm);
        break;
    case VM_STATE_PAUSE:
        ret = pause_vm_handler(vm);
        break;
    case VM_STATE_HALT:
        ret = stop_vm_handler(vm);
        break;
    default:
        ret = -1;
        break;
    }

    return ret;
}

int vm_mem_init(struct z_vm *vm)
{
    int ret = 0;
    struct vm_mem_domain *vmem_dm = vm->vmem_domain;

    if (vmem_dm->is_init) {
        ZVM_LOG_WARN("VM's mem has been init before! \n");
        return -ENXIO;
    }

    ret = vm_mem_domain_partitions_add(vmem_dm);
    if (ret) {
        ZVM_LOG_WARN("Add partition to domain failed!, Code: %d \n", ret);
        return ret;
    }

    return 0;
}

int vm_create(struct z_os_info *vm_info, struct z_vm *new_vm)
{
    int ret = 0;
    struct z_vm *vm = new_vm;

    /* init vmid here, this vmid is for vm level*/
    vm->vmid = allocate_vmid(vm_info);
    if (vm->vmid >= CONFIG_MAX_VM_NUM) {
        return -EOVERFLOW;
    }
    /* init vm*/
    vm->reboot = false;

    vm->os = (struct z_os *)k_malloc(sizeof(struct z_os));
	if (!vm->os) {
		ZVM_LOG_WARN("Allocate memory for os error! \n");
		return -ENOMEM;
	}

	ret = vm_os_create(vm->os, vm_info);
    if (ret) {
        ZVM_LOG_WARN("Unknow os type! \n");
        return ret;
    }

    ret = vm_mem_domain_create(vm);
    if (ret) {
        ZVM_LOG_WARN("vm_mem_domain_create failed! \n");
        return ret;
    }

    ret = vm_vcpus_create(vm_info->vcpu_num, vm);
    if (ret) {
        ZVM_LOG_WARN("vm_vcpus_create failed!");
        return ret;
    }

	vm->arch = (struct vm_arch *)k_malloc(sizeof(struct vm_arch));
	if (!vm->arch) {
		ZVM_LOG_WARN("Allocate mm memory for vm arch struct failed!");
		return -ENXIO;
	}

    vm->ops = (struct zvm_ops *)k_malloc(sizeof(struct zvm_ops));
    if (!vm->ops) {
        ZVM_LOG_WARN("Allocate mm memory for vm ops struct failed!");
        return -ENXIO;
    }

    vm->vm_vcpu_id.totle_vcpu_id = 0;
    ZVM_SPINLOCK_INIT(&vm->vm_vcpu_id.vcpu_id_lock);
    ZVM_SPINLOCK_INIT(&vm->spinlock);

    char vmid_str[4];
    uint16_t vmid_str_len = sprintf(vmid_str, "-%d", vm->vmid);
    if (vmid_str_len > 4) {
        ZVM_LOG_WARN("Sprintf put error, may cause str overflow!\n");
        vmid_str[3] = '\0';
    } else {
        vmid_str[vmid_str_len] = '\0';
    }

    if (strcpy(vm->vm_name, vm->os->name) == NULL || strcat(vm->vm_name, vmid_str) == NULL) {
        ZVM_LOG_WARN("VM name init error! \n");
        return -EIO;
    }

    /* set vm status here */
    vm->vm_status = VM_STATE_NEVER_RUN;

    /* store zvm overall info */
    zvm_overall_info->vms[vm->vmid] = vm;
    vm->arch->vm_pgd_base = (uint64_t)
            vm->vmem_domain->vm_mm_domain->arch.ptables.base_xlat_table;

    return 0;
}

int vm_ops_init(struct z_vm *vm)
{
    /* According to OS type to bind vm_ops. We need to add operation func@TODO*/
    return 0;
}

static uint16_t get_vmid_by_id(size_t argc, char **argv)
{
    uint16_t vm_id =  CONFIG_MAX_VM_NUM;
    int opt;
    char *optstring = "t:n:";
    struct getopt_state *state;

    /* Initialize the global state */
    getopt_init();
    /* Get Current getopt_state */
    state = getopt_state_get();

    while ((opt = getopt(argc, argv, optstring)) != -1) {
		switch (opt) {
		case 'n':
            vm_id = (uint16_t)(state->optarg[0] - '0');
			break;
		default:
			ZVM_LOG_WARN("Input number invalid, Please input a valid vmid after \"-n\" command! \n");
			return -EINVAL;
		}
	}
    return vm_id;
}

int vm_vcpus_create(uint16_t vcpu_num, struct z_vm *vm)
{
    /* init vcpu num */
    if (vcpu_num > CONFIG_MAX_VCPU_PER_VM) {
        vcpu_num = CONFIG_MAX_VCPU_PER_VM;
        ZVM_LOG_WARN("Vcpu num is too big, set it to max vcpu num: %d\n", vcpu_num);
    }
    vm->vcpu_num = vcpu_num;

    /* allocate vcpu list here */
    vm->vcpus = (struct z_vcpu **)k_malloc(vcpu_num * sizeof(struct z_vcpu *));
    if (!(vm->vcpus)) {
        ZVM_LOG_WARN("Vcpus struct init error !\n");
        return -ENXIO;
    }
    return 0;
}

int vm_vcpus_init(struct z_vm *vm)
{
    char vcpu_name[VCPU_NAME_LEN];
    int i;
    struct z_vcpu *vcpu;

    if (vm->vcpu_num > CONFIG_MAX_VCPU_PER_VM) {
        ZVM_LOG_WARN("Vcpu counts is too big!");
        return -ESRCH;
    }

    for(i = 0; i < vm->vcpu_num; i++){
        memset(vcpu_name, 0, VCPU_NAME_LEN);
        snprintk(vcpu_name, VCPU_NAME_LEN - 1, "%s-vcpu%d", vm->vm_name, i);

        vcpu = vm_vcpu_init(vm, i, vcpu_name);

        sys_dlist_init(&vcpu->vcpu_lists);
        vm->vcpus[i] = vcpu;
        vcpu->next_vcpu = NULL;

        if (i) {
            vm->vcpus[i-1]->next_vcpu = vcpu;
        }

        vcpu->is_poweroff = true;
        if (i == 0) {
            vcpu->is_poweroff = false;
        }
    }

    return 0;
}

int vm_vcpus_ready(struct z_vm *vm)
{
    uint16_t i=0;
    struct z_vcpu *vcpu;
    struct k_thread *thread;
    k_spinlock_key_t key;
    ARG_UNUSED(thread);

    key = k_spin_lock(&vm->spinlock);
    for(i = 0; i < vm->vcpu_num; i++){
        /* find the vcpu struct */
        vcpu = vm->vcpus[i];
        if (vcpu == NULL) {
            ZVM_LOG_WARN("vm error here, can't find vcpu: vcpu-%d", i);
            k_spin_unlock(&vm->spinlock, key);
            return -ENODEV;
        }
        if (!vcpu->is_poweroff) {
            vm_vcpu_ready(vcpu);
        }
    }
    vm->vm_status = VM_STATE_RUNNING;
    k_spin_unlock(&vm->spinlock, key);

    return 0;
}

int vm_vcpus_pause(struct z_vm *vm)
{
    uint16_t i=0;
    struct z_vcpu *vcpu;
    struct k_thread *thread, *cur_thread;
    k_spinlock_key_t key;
    ARG_UNUSED(thread);
    ARG_UNUSED(cur_thread);

    key = k_spin_lock(&vm->spinlock);
    for(i = 0; i < vm->vcpu_num; i++){
        vcpu = vm->vcpus[i];
        if (vcpu == NULL) {
            ZVM_LOG_WARN("Pause vm error here, can't find vcpu: vcpu-%d \n", i);
            k_spin_unlock(&vm->spinlock, key);
            return -ENODEV;
        }
        vm_vcpu_pause(vcpu);
    }

    vm->vm_status = VM_STATE_PAUSE;
    k_spin_unlock(&vm->spinlock, key);
    return 0;
}

int vm_vcpus_halt(struct z_vm *vm)
{
    uint16_t i=0;
    struct z_vcpu *vcpu;
    struct k_thread *thread;
    k_spinlock_key_t key;
    ARG_UNUSED(thread);

    key = k_spin_lock(&vm->spinlock);
    for(i = 0; i < vm->vcpu_num; i++){
        /* find the vcpu struct */
        vcpu = vm->vcpus[i];
        if (vcpu == NULL) {
            ZVM_LOG_WARN("vm error here, can't find vcpu: vcpu-%d", i);
            k_spin_unlock(&vm->spinlock, key);
            return -ENODEV;
        }
        vm_vcpu_halt(vcpu);
    }
    vm->vm_status = VM_STATE_HALT;
    k_spin_unlock(&vm->spinlock, key);

    return 0;
}

int vm_vcpus_reset(struct z_vm *vm)
{
    uint16_t i=0;
    struct z_vcpu *vcpu;
    k_spinlock_key_t key;

    key = k_spin_lock(&vm->spinlock);
    for(i = 0; i < vm->vcpu_num; i++) {
        vcpu = vm->vcpus[i];
        if (vcpu == NULL) {
            ZVM_LOG_WARN("Pause vm error here, can't find vcpu: vcpu-%d \n", i);
            k_spin_unlock(&vm->spinlock, key);
            return -ENODEV;
        }
        vm_vcpu_reset(vcpu);
    }

    vm->vm_status = VM_STATE_RESET;
    load_os_image(vm);
    k_spin_unlock(&vm->spinlock, key);
    return 0;
}

int vm_delete(struct z_vm *vm)
{
    int ret = 0;
    k_spinlock_key_t key;
    struct vm_mem_domain *vmem_dm = vm->vmem_domain;
    struct z_vcpu *vcpu;
    struct vcpu_work *vwork;

    key = k_spin_lock(&vm->spinlock);
    /* delete vdev struct */
    ret = vm_device_deinit(vm);
    if (ret) {
        ZVM_LOG_WARN("Delete vm devices failed! \n");
    }

    /* remove all the partition in the vmem_domain */
    ret = vm_mem_apart_remove(vmem_dm);
    if(ret) {
        ZVM_LOG_WARN("Delete vm mem domain failed! \n");
    }

    /* delete vcpu struct */
    for(int i = 0; i < vm->vcpu_num; i++){
        vcpu = vm->vcpus[i];
        if(!vcpu) {
            continue;
        }
        /* release the used physical cpu*/
        vm_cpu_reset(vcpu->cpu);
        vwork = vcpu->work;
        if(vwork){
            k_free(vwork->vcpu_thread);
        }

        k_free(vcpu->arch);
        k_free(vcpu->work);
        k_free(vcpu);
    }

    k_free(vm->ops);
    k_free(vm->arch);
    k_free(vm->vcpus);
    k_free(vm->vmem_domain);
    if(vm->os->name) {
        k_free(vm->os->name);
    }
    k_free(vm->os);
    zvm_overall_info->vms[vm->vmid] = NULL;
    k_free(vm);
    k_spin_unlock(&vm->spinlock, key);

    zvm_overall_info->vm_total_num--;
    zvm_overall_info->alloced_vmid &= ~BIT(vm->vmid);
    return 0;
}

int z_parse_new_vm_args(size_t argc, char **argv, struct z_os_info *vm_info,
                        struct z_vm *vm)
{
    int ret = 0;
    int opt;
    char *optstring = "t:n:";
    struct getopt_state *state;

    getopt_init();
    state = getopt_state_get();

    while ((opt = getopt(argc, argv, optstring)) != -1) {
		switch (opt) {
		case 't':
			ret = get_os_info_by_type(vm_info);
			continue;
		case 'n':
            /* @TODO: support allocate vmid chosen by user later */
		default:
            ZVM_LOG_WARN("Input error! \n");
			ZVM_LOG_WARN("Please input \" zvm new -t + os_name \" command to new a vm! \n");
			return -EINVAL;
		}
	}

    return ret;
}

int z_parse_run_vm_args(size_t argc, char **argv)
{
    return get_vmid_by_id(argc, argv);
}

int z_parse_pause_vm_args(size_t argc, char **argv)
{
    return get_vmid_by_id(argc, argv);
}

int z_parse_delete_vm_args(size_t argc, char **argv)
{
    return get_vmid_by_id(argc, argv);
}

int z_parse_info_vm_args(size_t argc, char **argv)
{
    return get_vmid_by_id(argc, argv);
}

int z_list_vms_info(uint16_t vmid)
{
    /* list all vm */
    list_all_vms_info();
    printk("|*****************************************************|\n");
    return 0;
}

int vm_sysinfo_init(size_t argc, char **argv, struct z_vm *vm_ptr, struct z_os_info *vm_info)
{
    return z_parse_new_vm_args(argc, argv, vm_info, vm_ptr);
}

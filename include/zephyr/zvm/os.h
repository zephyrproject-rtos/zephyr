/*
 * Copyright 2024-2025 HNU-ESNL: Guoqi Xie, Chenglai Xiong, Xingyu Hu and etc.
 * Copyright 2024-2025 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZVM_OS_H_
#define ZEPHYR_INCLUDE_ZVM_OS_H_

#include <zephyr/kernel.h>
#include <stdint.h>
#include <zephyr/zvm/vm_mm.h>
#include <zephyr/zvm/zlog.h>

struct getopt_state;

#define OS_NAME_LENGTH      (32)
#define OS_TYPE_ZEPHYR      (0)
#define OS_TYPE_LINUX       (1)
#define OS_TYPE_OTHERS      (2)
#define OS_TYPE_MAX         (3)

#define ZEPHYR_VM_LOAD_BASE     DT_REG_ADDR(DT_NODELABEL(zephyr_ddr))
#define ZEPHYR_VM_LOAD_SIZE     DT_REG_SIZE(DT_NODELABEL(zephyr_ddr))
#define ZEPHYR_VM_MEMORY_BASE   DT_PROP(DT_NODELABEL(zephyr_ddr), vm_reg_base)
#define ZEPHYR_VM_MEMORY_SIZE   DT_PROP(DT_NODELABEL(zephyr_ddr), vm_reg_size)
#define ZEPHYR_IMAGE_BASE       DT_REG_ADDR(DT_ALIAS(zephyrcpy))
#define ZEPHYR_IMAGE_SIZE       DT_REG_SIZE(DT_ALIAS(zephyrcpy))
#define ZEPHYR_VM_VCPU_NUM      DT_PROP(DT_INST(0, zephyr_vm), vcpu_num)

#define LINUX_VM_LOAD_BASE      DT_REG_ADDR(DT_NODELABEL(linux_ddr))
#define LINUX_VM_LOAD_SIZE      DT_REG_SIZE(DT_NODELABEL(linux_ddr))
#define LINUX_VM_MEMORY_BASE    DT_PROP(DT_NODELABEL(linux_ddr), vm_reg_base)
#define LINUX_VM_MEMORY_SIZE    DT_PROP(DT_NODELABEL(linux_ddr), vm_reg_size)
#define LINUX_IMAGE_BASE        DT_REG_ADDR(DT_ALIAS(linuxcpy))
#define LINUX_IMAGE_SIZE        DT_REG_SIZE(DT_ALIAS(linuxcpy))
#define LINUX_VMDTB_BASE        DT_REG_ADDR(DT_ALIAS(linuxdtb))
#define LINUX_VMDTB_SIZE        DT_REG_SIZE(DT_ALIAS(linuxdtb))
#define LINUX_VMRFS_BASE        DT_REG_ADDR(DT_ALIAS(linuxrfs))
#define LINUX_VMRFS_SIZE        DT_REG_SIZE(DT_ALIAS(linuxrfs))
#define LINUX_VMRFS_PHY_BASE    DT_PROP(DT_INST(0, linux_vm), rootfs_address)
#define LINUX_VM_VCPU_NUM       DT_PROP(DT_INST(0, linux_vm), vcpu_num)
#ifdef CONFIG_VM_DTB_FILE_INPUT
#define LINUX_DTB_MEM_BASE        DT_PROP(DT_INST(0, linux_vm), dtb_address)
#define LINUX_DTB_MEM_SIZE        DT_PROP(DT_INST(0, linux_vm), dtb_size)
#endif /* CONFIG_VM_DTB_FILE_INPUT */

/**
 * @brief VM information structure in ZVM.
 *
 * @param os_type: the type of the operating system.
 * @param entry_point: the entry point of the vm, when
 * boot from elf file, this is not equal to vm_mem_base.
 * @param vcpu_num: the number of virtual CPUs.
 * @param vm_mem_base: the base address of the vm memory.
 * @param vm_mem_size: the size of the vm memory.
 * @param vm_image_base: the base address of the vm image in disk.
 * @param vm_image_size: the size of the vm image in disk.
 */
struct z_os_info {
	uint16_t    os_type;
	uint16_t    vcpu_num;
	uint32_t    vm_mem_base;
	uint32_t    vm_mem_size;
	uint32_t    vm_load_base;
	uint64_t    vm_image_base;
	uint64_t    vm_image_size;
	uint64_t    entry_point;
};

struct z_os {
	char *name;
	bool is_rtos;
	struct z_os_info info;
};


int get_os_info_by_type(struct z_os_info *vm_info);

int load_vm_image(struct vm_mem_domain *vmem_domain, struct z_os *os);

int vm_os_create(struct z_os *os, struct z_os_info *vm_info);

#endif  /* ZEPHYR_INCLUDE_ZVM_OS_H_ */

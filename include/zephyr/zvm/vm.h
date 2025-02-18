/*
 * Copyright 2024-2025 HNU-ESNL: Guoqi Xie, Chenglai Xiong, Xingyu Hu and etc.
 * Copyright 2024-2025 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZVM_VM_H_
#define ZEPHYR_INCLUDE_ZVM_VM_H_

#include <zephyr/toolchain/gcc.h>
#include <stddef.h>
#include <stdint.h>
#include <zephyr/kernel/thread.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/zvm/os.h>
#include <zephyr/zvm/vm_cpu.h>
#include <zephyr/zvm/vm_irq.h>
#include <zephyr/zvm/zlog.h>

#define DEFAULT_VM			(0)
#define VM_NAME_LEN			(32)
#define VCPU_NAME_LEN		(32)
#define RAMDISK_NAME_LEN	(32)

#define VCPU_THREAD_STACKSIZE	(40960)
#define VCPU_THREAD_PRIO		(1)

/**
 * @brief VM Status.
 */
#define VM_STATE_NEVER_RUN	(BIT(0))
#define VM_STATE_RUNNING	(BIT(1))
#define VM_STATE_PAUSE		(BIT(2))
#define VM_STATE_HALT		(BIT(3))
#define VM_STATE_RESET		(BIT(4))

/**
 * @brief VM return values.
 */
#define SET_IRQ_TO_VM_SUCCESS	(1)
#define UNSET_IRQ_TO_VM_SUCCESS	(2)
#define VM_IRQ_TO_VCPU_SUCCESS	(3)
#define VM_IRQ_NUM_OUT			(99)

#define _VCPU_STATE_READY		(BIT(0))
#define _VCPU_STATE_RUNNING		(BIT(1))
#define _VCPU_STATE_PAUSED		(BIT(2))
#define _VCPU_STATE_HALTED		(BIT(3))
#define _VCPU_STATE_UNKNOWN		(BIT(4))
#define _VCPU_STATE_RESET		(BIT(5))

#define VCPU_THREAD(thread)		((struct k_thread *)thread->vcpu_struct ? true : false)

#ifdef CONFIG_ZVM
#define _current_vcpu _current->vcpu_struct
#else
#define _current_vcpu NULL
#endif

#define get_current_vcpu_id()	\
({								\
	struct z_vcpu *vcpu = (struct z_vcpu *)_current_vcpu;\
	vcpu->vcpu_id;					\
})

#define get_current_vm()	\
({	\
	struct z_vcpu *vcpu =	\
	(struct z_vcpu *)_current_vcpu;	\
	vcpu->vm;	\
})

#define vcpu_need_switch(tid1, tid2) ((VCPU_THREAD(tid1)) || (VCPU_THREAD(tid2)))

/* VM debug console uart hardware info. */
#define VM_DEBUG_CONSOLE_BASE   DT_REG_ADDR(DT_CHOSEN(vm_console))
#define VM_DEBUG_CONSOLE_SIZE   DT_REG_SIZE(DT_CHOSEN(vm_console))
#define VM_DEBUG_CONSOLE_IRQ	DT_IRQN(DT_CHOSEN(vm_console))

#define VM_DEFAULT_CONSOLE_NAME	 "UART"
#define VM_DEFAULT_CONSOLE_NAME_LEN (4)


struct z_vcpu {
	struct vcpu_arch *arch;
	bool resume_signal;
	bool waitq_flag;

	uint16_t vcpu_id;
	uint16_t cpu;
	uint16_t vcpu_state;
	uint16_t exit_type;

	/**
	 * vcpu may be influeced by host cpu, so we need to record
	 * the vcpu ipi staus.
	 * Just when vcpu call xxx_raise_sgi, the vcpuipi_count will
	 * be plused. The default value is 0.
	 */
	uint64_t vcpuipi_count;

	/* vcpu timers record*/
	uint32_t hcpu_cycles;
	uint32_t runnig_cycles;
	uint32_t paused_cycles;

	struct k_spinlock vcpu_lock;

	/* virt irq block for this vcpu */
	struct vcpu_virt_irq_block virq_block;

	struct z_vcpu *next_vcpu;
	struct vcpu_work *work;
	struct z_vm *vm;

	/* vcpu's thread wait queue */
	_wait_q_t *t_wq;

	sys_dlist_t vcpu_lists;

	bool is_poweroff;
};
typedef struct z_vcpu vcpu_t;

/**
 * @brief  Describes the thread that vcpu binds to.
 */
struct __aligned(4) vcpu_work {
	/* statically allocate stack space */
	K_KERNEL_STACK_MEMBER(vt_stack, VCPU_THREAD_STACKSIZE);

	/* vCPU thread */
	struct k_thread *vcpu_thread;

	/* point to vcpu struct */
	void *v_date;
};

/**
 * @brief The initial information used to create the virtual machine.
 */
struct vm_desc {
	uint16_t vmid;
	char name[VM_NAME_LEN];

	char vm_dtb_image_name[RAMDISK_NAME_LEN];
	char vm_kernel_image_name[RAMDISK_NAME_LEN];

	int32_t vcpu_num;
	uint64_t mem_base;
	uint64_t mem_size;

	/* vm's code entry */
	uint64_t entry;

	/* vm's states*/
	uint64_t flags;
	uint64_t image_load_address;
};

/**
 * @brief Record vm's vcpu num.
 * Recommend:Consider deleting
 */
struct vm_vcpu_num {
	uint16_t count;
	struct k_spinlock vcpu_id_lock;
};

struct vm_arch {
	uint64_t vm_pgd_base;
	uint64_t vttbr;
	uint64_t vtcr_el2;
};

struct z_vm {
	bool is_rtos;
	uint16_t vmid;
	char vm_name[VM_NAME_LEN];
	bool reboot;

	uint32_t vm_status;

	uint32_t vcpu_num;
	struct vm_vcpu_num vm_vcpu_id_count;

	uint32_t vtimer_offset;

	struct vm_virt_irq_block vm_irq_block;

	struct k_spinlock spinlock;

	struct z_vcpu **vcpus;
	struct k_sem *vcpu_exit_sem;

	struct vm_arch *arch;
	struct vm_mem_domain *vmem_domain;
	struct z_os *os;

	/* bind the vm and the os type ops */
	struct zvm_ops *ops;

	/* store the vm's dev list */
	sys_dlist_t vdev_list;
};

int vm_ops_init(struct z_vm *vm);

/**
 * @brief Init guest vm memory manager:
 * this function aim to init vm's memory manager,for below step:
 * 1. allocate virt space to vm(base/size), and distribute vpart_list to it.
 * 2. add this vpart to mapped_vpart_list.
 * 3. divide vpart area to block and init block list,
 * then allocate physical space to these block.
 * 4. build page table from vpart virt address to block physical address.
 *
 * @param vm: The vm which memory need to be init.
 * @return int 0 for success
 */
int vm_mem_init(struct z_vm *vm);

int vm_vcpus_create(uint16_t vcpu_num, struct z_vm *vm);
int vm_vcpus_init(struct z_vm *vm);
int vm_vcpus_ready(struct z_vm *vm);
int vm_vcpus_pause(struct z_vm *vm);
int vm_vcpus_halt(struct z_vm *vm);
int vm_vcpus_reset(struct z_vm *vm);
int vm_delete(struct z_vm *vm);

int z_parse_run_vm_args(size_t argc, char **argv);
int z_parse_pause_vm_args(size_t argc, char **argv);
int z_parse_delete_vm_args(size_t argc, char **argv);
int z_parse_info_vm_args(size_t argc, char **argv);

int z_parse_new_vm_args(size_t argc, char **argv, struct z_os_info *vm_info, struct z_vm *vm);

int z_list_vms_info(uint16_t vmid);
int vm_sysinfo_init(size_t argc, char **argv, struct z_vm *vm_ptr, struct z_os_info *vm_info);

int vm_ipi_handler(struct z_vm *vm);

int vm_create(struct z_os_info *vm_info, struct z_vm *new_vm);

#endif /* ZEPHYR_INCLUDE_ZVM_VM_H_ */

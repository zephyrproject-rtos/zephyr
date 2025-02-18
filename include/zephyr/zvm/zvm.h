/*
 * Copyright 2024-2025 HNU-ESNL: Guoqi Xie, Chenglai Xiong, Xingyu Hu and etc.
 * Copyright 2024-2025 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZVM_H_
#define ZEPHYR_INCLUDE_ZVM_H_

#include <stdint.h>
#include <zephyr/devicetree.h>
#include <errno.h>
#include <zephyr/spinlock.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/dlist.h>
#include <zephyr/sys/printk.h>
#include <zephyr/toolchain/common.h>
#include <zephyr/sys/util.h>
#include <zephyr/zvm/os.h>
#include <zephyr/zvm/vm_cpu.h>

#define ZVM_MODULE_NAME zvm_host
#define SINGLE_CORE	 1U
#define DT_MB		   (1024 * 1024)

/**
 * @brief Spinlock initialization for smp
 */
#if defined(CONFIG_SMP) && defined(CONFIG_SPIN_VALIDATE)
#define ZVM_SPINLOCK_INIT(dev)  \
({									  \
	struct k_spinlock *lock = (struct k_spinlock *)dev; \
	lock->locked = 0x0;				 \
	lock->thread_cpu = 0x0;	  \
})
#elif defined(CONFIG_SMP) && !defined(CONFIG_SPIN_VALIDATE)
#define ZVM_SPINLOCK_INIT(dev)  \
({									  \
	struct k_spinlock *lock = (struct k_spinlock *)dev; \
	lock->locked = 0x0;				 \
})
#else
#define ZVM_SPINLOCK_INIT(dev)
#endif

struct z_os_info;

extern struct z_kernel _kernel;
extern struct zvm_manage_info *zvm_overall_info;

/* ZVM's functions */
typedef	 void (*zvm_new_vm_t)(size_t argc, char **argv);
typedef	 void (*zvm_run_vm_t)(uint32_t vmid);
typedef	 void (*zvm_update_vm_t)(size_t argc, char **argv);
typedef	 void (*zvm_info_vm_t)(size_t argc, char **argv);
typedef	 void (*zvm_pause_vm_t)(uint32_t vmid);
typedef	 void (*zvm_halt_vm_t)(size_t argc, char **argv);
typedef	 void (*zvm_delete_vm_t)(uint32_t vmid);


/**
 * @brief zvm_hwsys_info stores basic information about the ZVM.
 *
 * What hardware resource we concern about now includes CPU and memory(named
 * sram in dts file), such as CPU's compatible property and memory size. Other
 * devices we do not care currently. Then we need a structure to store basic
 * information of hardware.
 */
struct zvm_hwsys_info {
	char *cpu_type;
	uint16_t phy_cpu_num;
	uint64_t phy_mem;
	uint64_t phy_mem_used;
};

/**
 * @brief General operations for virtual machines.
 */
struct zvm_ops {
	zvm_new_vm_t			new_vm;
	zvm_run_vm_t			run_vm;
	zvm_update_vm_t		 update_vm;
	zvm_info_vm_t		   info_vm;
	zvm_pause_vm_t		  pause_vm;
	zvm_halt_vm_t		   halt_vm;
	zvm_delete_vm_t		 delete_vm;
};

/*
 * @brief ZVM manage structure.
 *
 * As a hypervisor, zvm should know how much resource it can use and how many vm
 * it carries.
 * At first aspect, File subsys/_zvm_zvm_host/zvm_host.c can get hardware info
 * from devicetree file. We construct corresponding data structure type
 * "struct zvm_hwsys_info" to store it. "struct zvm_hwsys_info" includes:
 *  -> the number of total vm
 *  -> the number of physical CPU
 *  -> system's CPU typename
 *  -> the number of physical memory
 *  -> how much physical memory has been used
 * and so on.
 * At second aspect, we should konw what kind of resource vm possess is proper.
 * Then we construct a proper data structure, just like "vm_info_t", to describe
 * it. It includes information as below:
 *  -> ...
 */
struct zvm_manage_info {

	/* The hardware information of this device */
	struct zvm_hwsys_info *hw_info;

	struct z_vm *vms[CONFIG_MAX_VM_NUM];

	/* TODO: try to add a flag to describe the running vm and pending vm list */

	/** Each bit of this value represents a virtual machine id.
	 * When the value of a bit is 1,
	 * the ID of that virtual machine has been allocated, and vice versa.
	 */
	uint32_t alloced_vmid;

	/* total num of vm in system */
	uint32_t vm_total_num;
	struct k_spinlock spin_zmi;
};

void zvm_ipi_handler(void);
struct zvm_dev_lists *get_zvm_dev_lists(void);

int load_os_image(struct z_vm *vm);

static ALWAYS_INLINE bool is_vmid_full(void)
{
	return zvm_overall_info->alloced_vmid == BIT_MASK(CONFIG_MAX_VM_NUM);
}

static ALWAYS_INLINE uint32_t find_next_vmid(struct z_os_info *vm_info, uint32_t *vmid)
{
	uint32_t id, maxid = BIT(CONFIG_MAX_VM_NUM);

	for (id = BIT(0), *vmid = 0; id < maxid; id <<= 1, (*vmid)++) {
		if (!(id & zvm_overall_info->alloced_vmid)) {
			zvm_overall_info->alloced_vmid |= id;
			return 0;
		}
	}
	return -EOVERFLOW;
}

/**
 * @brief Allocate a unique vmid for this VM.
 * TODO: Need atomic op to vmid.
 */
static ALWAYS_INLINE uint32_t allocate_vmid(struct z_os_info *vm_info)
{
	int err;
	uint32_t res;
	k_spinlock_key_t key;

	if (unlikely(is_vmid_full())) {
		return CONFIG_MAX_VM_NUM;	  /* Value overflow. */
	}

	key = k_spin_lock(&zvm_overall_info->spin_zmi);
	err = find_next_vmid(vm_info, &res);
	if (err) {
		k_spin_unlock(&zvm_overall_info->spin_zmi, key);
		return CONFIG_MAX_VM_NUM;
	}

	zvm_overall_info->vm_total_num++;

	k_spin_unlock(&zvm_overall_info->spin_zmi, key);

	return res;
}

static ALWAYS_INLINE struct z_vm *get_vm_by_id(uint32_t vmid)
{
	if (unlikely(vmid >= CONFIG_MAX_VM_NUM)) {
		return NULL;
	}
	return zvm_overall_info->vms[vmid];
}

static uint32_t pcpu_list[CONFIG_MP_MAX_NUM_CPUS] = {0};

static ALWAYS_INLINE void set_all_cache_clean(void)
{
	for (int i = 0; i < CONFIG_MP_MAX_NUM_CPUS; i++) {
		pcpu_list[i] = 1;
	}
}

static ALWAYS_INLINE void set_cpu_cache_clean(int pcpu_id)
{
	pcpu_list[pcpu_id] = 1;
}

static ALWAYS_INLINE void reset_cache_clean(int pcpu_id)
{
	pcpu_list[pcpu_id] = 0;
}

static ALWAYS_INLINE uint32_t get_cache_clean(int pcpu_id)
{
	return pcpu_list[pcpu_id];
}

void set_all_pcpu_cache_clean(void);

int get_pcpu_cache_clean(uint64_t cpu_mpidr);

void reset_pcpu_cache_clean(uint64_t cpu_mpidr);

#endif /* ZEPHYR_INCLUDE_ZVM_H_ */

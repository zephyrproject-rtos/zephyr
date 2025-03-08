/*
 * Copyright 2024-2025 HNU-ESNL: Guoqi Xie, Chenglai Xiong, Xingyu Hu and etc.
 * Copyright 2024-2025 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZVM_VM_CPU_H_
#define ZEPHYR_INCLUDE_ZVM_VM_CPU_H_

#include <zephyr/kernel.h>
#include <zephyr/kernel/thread.h>
#include <zephyr/kernel_structs.h>
#ifdef CONFIG_ARM64
#include <zephyr/zvm/arm/cpu.h>
#include <zephyr/zvm/arm/mmu.h>
#include <zephyr/zvm/arm/switch.h>
#include <zephyr/zvm/arm/timer.h>
#endif

/**
 * @brief We should define overall priority for zvm system.
 * A total of 15 priorities are defined by setting
 * CONFIG_NUM_PREEMPT_PRIORITIES = 15 and can be divided into three categories:
 * 1	->   5: high real-time requirement and very critical to the system.
 * 6	->  10: no real-time requirement and very critical to the system.
 * 10   ->  15: normal.
 */
#define RT_VM_WORK_PRIORITY		 (5)
#define NORT_VM_WORK_PRIORITY	   (10)

#ifdef CONFIG_PREEMPT_ENABLED
/* positive num */
#define VCPU_RT_PRIO		RT_VM_WORK_PRIORITY
#define VCPU_NORT_PRIO	  NORT_VM_WORK_PRIORITY
#else
/* negetive num */
#define VCPU_RT_PRIO		K_HIGHEST_THREAD_PRIO + RT_VM_WORK_PRIORITY
#define VCPU_NORT_PRIO	  K_HIGHEST_THREAD_PRIO + NORT_VM_WORK_PRIORITY
#endif

#define VCPU_IPI_MASK_ALL   (0xffffffff)

#define DEFAULT_VCPU		 (0)

/* For clear warning for unknown reason */
struct vcpu;

volatile static uint32_t used_cpus;
static struct k_spinlock cpu_mask_lock;

/**
 * @brief allocate a vcpu struct and init it.
 */
struct z_vcpu *vm_vcpu_init(struct z_vm *vm, uint16_t vcpu_id, char *vcpu_name);

/**
 * @brief release vcpu struct.
 */
int vm_vcpu_deinit(struct z_vcpu *vcpu);

/**
 * @brief the vcpu has below state:
 * running: vcpu is running, and is allocated to physical cpu.
 * ready: prepare to running.
 */
int vm_vcpu_ready(struct z_vcpu *vcpu);
int vm_vcpu_pause(struct z_vcpu *vcpu);
int vm_vcpu_halt(struct z_vcpu *vcpu);
int vm_vcpu_reset(struct z_vcpu *vcpu);

/**
 * @brief vcpu run func entry.
 */
int vcpu_thread_entry(struct z_vcpu *vcpu);

int vcpu_state_switch(struct k_thread *thread, uint16_t new_state);

void do_vcpu_swap(struct k_thread *new_thread, struct k_thread *old_thread);
void do_asm_vcpu_swap(struct k_thread *new_thread, struct k_thread *old_thread);

/**
 * @brief vcpu ipi scheduler to inform system scheduler to schedule vcpu.
 */
int vcpu_ipi_scheduler(uint32_t cpu_mask, uint32_t timeout);

static ALWAYS_INLINE int rt_get_idle_cpu(void)
{

	for (int i = 0; i < CONFIG_MP_MAX_NUM_CPUS; i++) {
#ifdef CONFIG_SMP
		/* In SMP, _current is a field read from _current_cpu, which
		 * can race with preemption before it is read.  We must lock
		 * local interrupts when reading it.
		 */
		unsigned int k = arch_irq_lock();
#endif
		k_tid_t tid = _kernel.cpus[i].current;
#ifdef CONFIG_SMP
		arch_irq_unlock(k);
#endif
		int prio;

		prio = k_thread_priority_get(tid);
		if (prio == K_IDLE_PRIO || (prio < K_IDLE_PRIO && prio > VCPU_RT_PRIO)) {
			return i;
		}
	}
	return -ESRCH;
}

static ALWAYS_INLINE int nrt_get_idle_cpu(void)
{

	for (int i = 0; i < CONFIG_MP_MAX_NUM_CPUS; i++) {
#ifdef CONFIG_SMP
		/* In SMP, _current is a field read from _current_cpu, which
		 * can race with preemption before it is read.  We must lock
		 * local interrupts when reading it.
		 */
		unsigned int k = arch_irq_lock();
#endif
		k_tid_t tid = _kernel.cpus[i].current;
#ifdef CONFIG_SMP
		arch_irq_unlock(k);
#endif
		int prio;

		prio = k_thread_priority_get(tid);
		if (prio == K_IDLE_PRIO) {
			return i;
		}
	}
	return -ESRCH;
}

static ALWAYS_INLINE int get_static_idle_cpu(void)
{
	k_spinlock_key_t key;

	for (int i = 1; i < CONFIG_MP_MAX_NUM_CPUS; i++) {
#ifdef CONFIG_SMP
		/* In SMP, _current is a field read from _current_cpu, which
		 * can race with preemption before it is read.  We must lock
		 * local interrupts when reading it.
		 */
		unsigned int k = arch_irq_lock();
#endif
		k_tid_t tid = _kernel.cpus[i].current;
#ifdef CONFIG_SMP
		arch_irq_unlock(k);
#endif
		int prio;

		prio = k_thread_priority_get(tid);
		if (prio == K_IDLE_PRIO && !(used_cpus & (1 << i))) {
			key = k_spin_lock(&cpu_mask_lock);
			used_cpus |= (1 << i);
			k_spin_unlock(&cpu_mask_lock, key);
			return i;
		}
	}
	return -ESRCH;
}

static ALWAYS_INLINE void reset_idle_cpu(uint16_t cpu_id)
{
	k_spinlock_key_t key;

	if (used_cpus & (1 << cpu_id)) {
		key = k_spin_lock(&cpu_mask_lock);
		used_cpus &= ~(1 << cpu_id);
		k_spin_unlock(&cpu_mask_lock, key);
	}
	barrier_isync_fence_full();
}

#endif /* ZEPHYR_INCLUDE_ZVM_VM_CPU_H_ */

/*
 * Copyright 2024-2025 HNU-ESNL: Guoqi Xie, Chenglai Xiong, Xingyu Hu and etc.
 * Copyright 2024-2025 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <ksched.h>
#include <zephyr/dt-bindings/interrupt-controller/arm-gic.h>
#include <zephyr/zvm/zvm.h>
#include <zephyr/zvm/arm/cpu.h>
#include <zephyr/zvm/arm/switch.h>
#include <zephyr/zvm/arm/timer.h>
#include <zephyr/zvm/vm_device.h>
#include <zephyr/zvm/vm_cpu.h>
#include <../../kernel/include/timeout_q.h>

LOG_MODULE_DECLARE(ZVM_MODULE_NAME);

/**
 * @brief Construct a new vcpu virt irq block. Setting
 * a default description.
 * TODO: all the local irq is inited here, may should be
 * init when vtimer init.
 */
static void init_vcpu_virt_irq_desc(struct vcpu_virt_irq_block *virq_block)
{
	int i;
	struct virt_irq_desc *desc;

	for (i = 0; i < VM_LOCAL_VIRQ_NR; i++) {
		desc = &virq_block->vcpu_virt_irq_desc[i];
		desc->id = VM_INVALID_DESC_ID;
		desc->pirq_num = i;
		desc->virq_num = i;
		desc->prio = 0;
		desc->vdev_trigger = 0;
		desc->vcpu_id = DEFAULT_VCPU;
		desc->virq_flags = 0;
		desc->virq_states = 0;
		desc->vm_id = DEFAULT_VM;

		sys_dnode_init(&(desc->desc_node));
	}
}

static void save_vcpu_context(struct k_thread *thread)
{
	arch_vcpu_context_save(thread->vcpu_struct);
}

static void load_vcpu_context(struct k_thread *thread)
{
	struct z_vcpu *vcpu = thread->vcpu_struct;

	arch_vcpu_context_load(thread->vcpu_struct);

	vcpu->resume_signal = false;
}

static void vcpu_timer_event_pause(struct z_vcpu *vcpu)
{
	struct virt_timer_context *timer_ctxt = vcpu->arch->vtimer_context;

	z_abort_timeout(&timer_ctxt->vtimer_timeout);
	z_abort_timeout(&timer_ctxt->ptimer_timeout);
}

static void vcpu_context_switch(struct k_thread *new_thread,
			struct k_thread *old_thread)
{
	struct z_vcpu *old_vcpu;

	if (VCPU_THREAD(old_thread)) {
		old_vcpu = old_thread->vcpu_struct;

		save_vcpu_context(old_thread);
		switch (old_vcpu->vcpu_state) {
		case _VCPU_STATE_RUNNING:
			old_vcpu->vcpu_state = _VCPU_STATE_READY;
			break;
		case _VCPU_STATE_RESET:
			ZVM_LOG_WARN("Do not support vm reset!\n");
			break;
		case _VCPU_STATE_PAUSED:
			vcpu_timer_event_pause(old_vcpu);
			vm_vdev_pause(old_vcpu);
			break;
		case _VCPU_STATE_HALTED:
			vcpu_timer_event_pause(old_vcpu);
			vm_vdev_pause(old_vcpu);
			break;
		default:
			break;
		}
	}

	if (VCPU_THREAD(new_thread)) {
		struct z_vcpu *new_vcpu = new_thread->vcpu_struct;

		if (new_vcpu->vcpu_state != _VCPU_STATE_READY) {
			ZVM_LOG_ERR("vCPU is not ready, something may be wrong.\n");
		}

		load_vcpu_context(new_thread);
		new_vcpu->vcpu_state = _VCPU_STATE_RUNNING;
	}
}

static void vcpu_state_to_ready(struct z_vcpu *vcpu)
{
	uint16_t cur_state = vcpu->vcpu_state;
	struct k_thread *thread = vcpu->work->vcpu_thread;

	vcpu->hcpu_cycles = sys_clock_cycle_get_32();

	switch (cur_state) {
	case _VCPU_STATE_UNKNOWN:
	case _VCPU_STATE_READY:
		k_thread_start(thread);
		vcpu->vcpu_state = _VCPU_STATE_READY;
		break;
	case _VCPU_STATE_RUNNING:
		vcpu->resume_signal = true;
		break;
	case _VCPU_STATE_RESET:
	case _VCPU_STATE_PAUSED:
		k_wakeup(thread);
		break;
	default:
		ZVM_LOG_WARN("Invalid cpu state!\n");
		break;
	}

}

static void vcpu_state_to_running(struct z_vcpu *vcpu)
{
	ARG_UNUSED(vcpu);
	ZVM_LOG_WARN("No thing to do, running state may be auto switched.\n");
}

static void vcpu_state_to_reset(struct z_vcpu *vcpu)
{
	uint16_t cur_state = vcpu->vcpu_state;
	struct k_thread *thread = vcpu->work->vcpu_thread;

	switch (cur_state) {
	case _VCPU_STATE_READY:
		move_thread_to_end_of_prio_q(thread);
#if defined(CONFIG_SMP) &&  defined(CONFIG_SCHED_IPI_SUPPORTED)
		arch_sched_broadcast_ipi();
#endif
		break;
	case _VCPU_STATE_RESET:
		break;
	case _VCPU_STATE_RUNNING:
	case _VCPU_STATE_PAUSED:
		arch_vcpu_init(vcpu);
		break;
	default:
		ZVM_LOG_WARN("Invalid cpu state here.\n");
		break;
	}
	vcpu->resume_signal = false;

}

static void vcpu_state_to_paused(struct z_vcpu *vcpu)
{
	bool resumed = false;
	uint16_t cur_state = vcpu->vcpu_state;
	struct k_thread *thread = vcpu->work->vcpu_thread;

	switch (cur_state) {
	case _VCPU_STATE_READY:
	case _VCPU_STATE_RUNNING:
		resumed = vcpu->resume_signal;
		vcpu->resume_signal = false;
		if (resumed && vcpu->waitq_flag) {
			vcpu_timer_event_pause(vcpu);
		}
		thread->base.thread_state |= _THREAD_SUSPENDED;
		dequeue_ready_thread(thread);
		break;
	case _VCPU_STATE_RESET:
	case _VCPU_STATE_PAUSED:
	default:
		ZVM_LOG_WARN("Invalid cpu state.\n");
		break;
	}

}

static void vcpu_state_to_halted(struct z_vcpu *vcpu)
{
	uint16_t cur_state = vcpu->vcpu_state;
	struct k_thread *thread = vcpu->work->vcpu_thread;

	switch (cur_state) {
	case _VCPU_STATE_READY:
	case _VCPU_STATE_RUNNING:
	case _VCPU_STATE_PAUSED:
		thread->base.thread_state |= _THREAD_VCPU_NO_SWITCH;
		break;
	case _VCPU_STATE_RESET:
	case _VCPU_STATE_UNKNOWN:
		vm_delete(vcpu->vm);
		break;
	default:
		ZVM_LOG_WARN("Invalid cpu state here.\n");
		break;
	}
	vcpu_ipi_scheduler(VCPU_IPI_MASK_ALL, 0);

}

static void vcpu_state_to_unknown(struct z_vcpu *vcpu)
{
	ARG_UNUSED(vcpu);
}

/**
 * @brief Vcpu scheduler for switch vcpu to different states.
 */
int vcpu_state_switch(struct k_thread *thread, uint16_t new_state)
{
	int ret = 0;
	struct z_vcpu *vcpu = thread->vcpu_struct;
	uint16_t cur_state = vcpu->vcpu_state;

	if (cur_state == new_state) {
		return ret;
	}
	switch (new_state) {
	case _VCPU_STATE_READY:
		vcpu_state_to_ready(vcpu);
		break;
	case _VCPU_STATE_RUNNING:
		vcpu_state_to_running(vcpu);
		break;
	case _VCPU_STATE_RESET:
		vcpu_state_to_reset(vcpu);
		break;
	case _VCPU_STATE_PAUSED:
		vcpu_state_to_paused(vcpu);
		break;
	case _VCPU_STATE_HALTED:
		vcpu_state_to_halted(vcpu);
		break;
	case _VCPU_STATE_UNKNOWN:
		vcpu_state_to_unknown(vcpu);
		break;
	default:
		ZVM_LOG_ERR("Invalid state here.\n");
		ret = EINVAL;
		break;
	}
	vcpu->vcpu_state = new_state;

	return ret;

}

void do_vcpu_swap(struct k_thread *new_thread, struct k_thread *old_thread)
{
	struct z_vcpu *vcpu;

	ARG_UNUSED(vcpu);

	if (new_thread == old_thread) {
		return;
	}

#ifdef CONFIG_SMP
	vcpu_context_switch(new_thread, old_thread);
#else
	if (old_thread && VCPU_THREAD(old_thread)) {
		save_vcpu_context(old_thread);
	}
	if (new_thread && VCPU_THREAD(new_thread)) {
		load_vcpu_context(new_thread);
	}
#endif /* CONFIG_SMP */
}

void do_asm_vcpu_swap(struct k_thread *new_thread, struct k_thread *old_thread)
{
	if (!vcpu_need_switch(new_thread, old_thread)) {
		return;
	}
	do_vcpu_swap(new_thread, old_thread);
}

int vcpu_ipi_scheduler(uint32_t cpu_mask, uint32_t timeout)
{
	ARG_UNUSED(timeout);
	uint32_t mask = cpu_mask;

	switch (mask) {
	case VCPU_IPI_MASK_ALL:
#if defined(CONFIG_SMP) && defined(CONFIG_SCHED_IPI_SUPPORTED)
		arch_sched_broadcast_ipi();
#else
		ZVM_LOG_WARN("Not smp ipi support.");
#endif /* CONFIG_SMP && CONFIG_SCHED_IPI_SUPPORTED */
		break;
	default:
		break;
	}

	return 0;
}

int vcpu_thread_entry(struct z_vcpu *vcpu)
{
	int ret = 0;

	do {
		ret = arch_vcpu_run(vcpu);

		if (vcpu->vm->vm_status == VM_STATE_HALT) {
			/*TODO: Disable all the allocated irq. */
			arch_vcpu_timer_deinit(vcpu);
			break;
		}

	} while (ret >= 0);

	k_sem_give(&vcpu->vm->vcpu_exit_sem[vcpu->vcpu_id]);

	return ret;
}

struct z_vcpu *vm_vcpu_init(struct z_vm *vm, uint16_t vcpu_id, char *vcpu_name)
{
	uint16_t vm_prio;
	int pcpu_num = 0;
	struct z_vcpu *vcpu;
	struct vcpu_work *vwork;
	k_spinlock_key_t key;

	vcpu = (struct z_vcpu *)k_malloc(sizeof(struct z_vcpu));
	if (!vcpu) {
		ZVM_LOG_ERR("Allocate vcpu space failed");
		return NULL;
	}

	vcpu->arch = (struct vcpu_arch *)k_malloc(sizeof(struct vcpu_arch));
	if (!vcpu->arch) {
		ZVM_LOG_ERR("Init vcpu->arch failed");
		k_free(vcpu);
		return  NULL;
	}

	/* init vcpu virt irq block. */
	vcpu->virq_block.virq_pending_counts = 0;
	vcpu->virq_block.vwfi.priv = NULL;
	vcpu->virq_block.vwfi.state = false;
	vcpu->virq_block.vwfi.yeild_count = 0;
	ZVM_SPINLOCK_INIT(&vcpu->virq_block.vwfi.wfi_lock);
	sys_dlist_init(&vcpu->virq_block.pending_irqs);
	sys_dlist_init(&vcpu->virq_block.active_irqs);
	ZVM_SPINLOCK_INIT(&vcpu->virq_block.spinlock);
	init_vcpu_virt_irq_desc(&vcpu->virq_block);
	ZVM_SPINLOCK_INIT(&vcpu->vcpu_lock);

	if (vm->os->is_rtos) {
		vm_prio = VCPU_RT_PRIO;
	} else {
		vm_prio = VCPU_NORT_PRIO;
	}
	vcpu->vm = vm;

	/* vt_stack must be aligned, So we allocate memory with aligned block */
	vwork = (struct vcpu_work *)k_aligned_alloc(0x10, sizeof(struct vcpu_work));
	if (!vwork) {
		ZVM_LOG_ERR("Create vwork error!");
		return NULL;
	}

	/* init tast_vcpu_thread struct here */
	vwork->vcpu_thread = (struct k_thread *)k_malloc(sizeof(struct k_thread));
	if (!vwork->vcpu_thread) {
		ZVM_LOG_ERR("Init thread struct error here!");
		return  NULL;
	}
	/*
	 * TODO: In this stage, the thread is marked as a kernel thread,
	 * For system safe, we will modified it later.
	 */
	k_tid_t tid = k_thread_create(vwork->vcpu_thread, vwork->vt_stack,
			VCPU_THREAD_STACKSIZE, (void *)vcpu_thread_entry, vcpu, NULL, NULL,
			vm_prio, 0, K_FOREVER);
	strcpy(tid->name, vcpu_name);

	/* SMP support */
#ifdef CONFIG_SCHED_CPU_MASK
	/**
	 * Due to the default 'new_thread->base.cpu_mask=1',
	 * BIT(0) must be cleared when enable other mask bit
	 * when CONFIG_SCHED_CPU_MASK_PIN_ONLY=y.
	 */
	k_thread_cpu_mask_disable(tid, 0);

	pcpu_num = get_static_idle_cpu();
	if (pcpu_num < 0 || pcpu_num >= CONFIG_MP_MAX_NUM_CPUS) {
		ZVM_LOG_WARN("No suitable idle cpu for VM!\n");
		return NULL;
	}

	k_thread_cpu_mask_enable(tid, pcpu_num);
	vcpu->cpu = pcpu_num;
#else
	vcpu->cpu = pcpu_num;
#endif /* CONFIG_SCHED_CPU_MASK */

	/* create a new thread and store it in work struct */
	vwork->v_date = vcpu;
	vwork->vcpu_thread->vcpu_struct = vcpu;

	vcpu->work = vwork;
	/* init vcpu timer*/
	vcpu->hcpu_cycles = 0;
	vcpu->runnig_cycles = 0;
	vcpu->paused_cycles = 0;
	vcpu->vcpu_id = vcpu_id;
	vcpu->vcpu_state = _VCPU_STATE_UNKNOWN;
	vcpu->exit_type = 0;
	vcpu->vcpuipi_count = 0;
	vcpu->resume_signal = false;
	vcpu->waitq_flag = false;

	key = k_spin_lock(&vcpu->vm->vm_vcpu_id_count.vcpu_id_lock);
	vcpu->vcpu_id = vcpu->vm->vm_vcpu_id_count.count++;
	k_spin_unlock(&vcpu->vm->vm_vcpu_id_count.vcpu_id_lock, key);

	if (arch_vcpu_init(vcpu)) {
		k_free(vcpu);
		return  NULL;
	}

	return vcpu;
}

int vm_vcpu_deinit(struct z_vcpu *vcpu)
{
	int ret = 0;

	ret = arch_vcpu_deinit(vcpu);
	if (ret) {
		ZVM_LOG_WARN("Deinit arch vcpu error!");
		return ret;
	}

	reset_idle_cpu(vcpu->cpu);
	k_free(vcpu->work);
	k_free(vcpu->arch);
	k_free(vcpu);

	return ret;
}

int vm_vcpu_ready(struct z_vcpu *vcpu)
{
	return vcpu_state_switch(vcpu->work->vcpu_thread, _VCPU_STATE_READY);
}

int vm_vcpu_pause(struct z_vcpu *vcpu)
{
	return vcpu_state_switch(vcpu->work->vcpu_thread, _VCPU_STATE_PAUSED);
}

int vm_vcpu_halt(struct z_vcpu *vcpu)
{
	return vcpu_state_switch(vcpu->work->vcpu_thread, _VCPU_STATE_HALTED);
}

int vm_vcpu_reset(struct z_vcpu *vcpu)
{
	return vcpu_state_switch(vcpu->work->vcpu_thread, _VCPU_STATE_RESET);
}

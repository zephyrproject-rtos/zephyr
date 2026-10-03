/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Hexagon userspace support
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/hexagon/arch.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/linker/linker-defs.h>
#include <kernel_internal.h>
#include <hexagon_vm.h>
#include <offsets_short.h>

#ifdef CONFIG_USERSPACE

int arch_buffer_validate(const void *addr, size_t size, int write)
{
	struct k_thread *thread = k_current_get();
	uintptr_t start = (uintptr_t)addr;
	uintptr_t end;

	/*
	 * No arch_is_user_context() shortcut: the trap0 path always clears
	 * _hexagon_user_mode_active on entry, so always validate against the
	 * calling thread's own bounds instead, like RISC-V and ARM.
	 */
	if (size > (UINTPTR_MAX - start)) {
		return -EPERM;
	}
	end = start + size;

	/* Check thread stack */
	if (start >= thread->stack_info.start &&
	    end <= thread->stack_info.start + thread->stack_info.size) {
		return 0; /* Within thread stack */
	}

	/*
	 * Global rodata/text is not part of any thread's stack or domain
	 * partition, but string literals passed to syscalls come from there
	 * and must be readable without an explicit grant.
	 */
	if (!write) {
		uintptr_t ro_start = (uintptr_t)__rom_region_start;
		uintptr_t ro_end = (uintptr_t)__rom_region_end;

		if (ro_end > ro_start && start >= ro_start && end <= ro_end) {
			return 0;
		}

		uintptr_t rodata_start = (uintptr_t)__rodata_region_start;
		uintptr_t rodata_end = (uintptr_t)__rodata_region_end;

		if (rodata_end > rodata_start && start >= rodata_start && end <= rodata_end) {
			return 0;
		}
	}

	/* Check memory domain partitions */
	if (thread->mem_domain_info.mem_domain != NULL) {
		struct k_mem_domain *domain = thread->mem_domain_info.mem_domain;
		int remaining_partitions;
		k_spinlock_key_t key;

		/*
		 * z_mem_domain_lock also guards partitions[]/num_partitions
		 * against concurrent k_mem_domain_add/remove_partition(): a
		 * timer tick can preempt this scan since trap0 re-enables
		 * guest interrupts.
		 */
		key = k_spin_lock(&z_mem_domain_lock);
		remaining_partitions = domain->num_partitions;

		/*
		 * partitions[] can have unused holes (size == 0) left by a
		 * removed partition, so scan the whole array rather than
		 * assuming the first num_partitions entries are valid.
		 */
		for (int i = 0; remaining_partitions > 0 && i < CONFIG_MAX_DOMAIN_PARTITIONS;
		     i++) {
			const struct k_mem_partition *part = &domain->partitions[i];

			if (part->size == 0) {
				continue;
			}
			remaining_partitions--;

			uintptr_t part_start = part->start;
			uintptr_t part_end = part_start + part->size;

			if (start < part_start || end > part_end) {
				continue;
			}

			/* For a write access, the partition must be writable */
			if (write && !K_MEM_PARTITION_IS_WRITABLE(part->attr)) {
				continue;
			}

			k_spin_unlock(&z_mem_domain_lock, key);
			return 0; /* Within a valid partition */
		}

		k_spin_unlock(&z_mem_domain_lock, key);
	}

	return -EPERM;
}

size_t arch_user_string_nlen(const char *s, size_t maxsize, int *err_arg)
{
	/*
	 * A zero-length request must never touch *s, matching
	 * strnlen(s, 0)'s contract of accepting an arbitrary pointer.
	 */
	if (maxsize == 0) {
		*err_arg = 0;
		return 0;
	}

	if (arch_buffer_validate(s, maxsize, 0)) {
		*err_arg = -1;
		return 0;
	}

	*err_arg = 0;
	return strnlen(s, maxsize);
}

/*
 * __naked comes from picolibc's sys/cdefs.h, so it silently disappears on
 * any other libc (e.g. CONFIG_MINIMAL_LIBC). Define it locally instead.
 */
#ifndef __naked
#define __naked __attribute__((naked))
#endif

/*
 * Guest register usage for vmrte:
 *   G0 = GELR (entry point)
 *   G1 = GSR (bit 31 = user mode, bit 30 = IE)
 *   G2 = GOSP (user stack pointer)
 *   G3 = GBADVA (0 for normal entry)
 */
static void __used __naked hexagon_user_thread_exit(void)
{
	/*
	 * User function returned -- call k_thread_abort(self) via explicit
	 * trap0 syscall; a C wrapper's user-mode check could be optimized
	 * away. Syscall convention: r0 = arg (thread), r6 = syscall number.
	 */
	__asm__ volatile(
		/* r0 = _kernel.cpus[0].current (k_current_get) */
		"r0 = ##_kernel\n\t"
		"r0 = add(r0, #%[cpus_off])\n\t"
		"r0 = memw(r0+#%[cur_off])\n\t"
		/* syscall: k_thread_abort(r0) */
		"r6 = #%[sc_id]\n\t"
		"trap0(#0x1)\n\t"
		/* should not return -- loop as backstop */
		"1: jump 1b\n\t"
		:
		: [cpus_off] "i"(___kernel_t_cpus_OFFSET),
		  [cur_off] "i"(___cpu_t_current_OFFSET),
		  [sc_id] "i"(K_SYSCALL_K_THREAD_ABORT)
		:
	);
}

void arch_user_mode_enter(k_thread_entry_t user_entry, void *p1, void *p2, void *p3)
{
	/*
	 * Not k_current_get(): k_thread_user_mode_enter() just called
	 * arch_tls_stack_setup(), which zeroes z_tls_current along with the
	 * rest of .tbss, and ugp is not reloaded until the vmrte below.
	 */
	struct k_thread *thread = k_sched_current_thread_query();

	/*
	 * stack_info.delta reserves the TLS block at the top of the stack
	 * buffer; without subtracting it, user_sp would land inside the TLS
	 * block arch_tls_stack_setup() just populated.
	 */
	uintptr_t user_sp = thread->stack_info.start + thread->stack_info.size -
			    thread->stack_info.delta;

	user_sp = ROUND_DOWN(user_sp, ARCH_STACK_PTR_ALIGN);

	__ASSERT(user_sp >= thread->stack_info.start,
		 "declared stack (%zu bytes) too small to hold TLS/headroom (%zu bytes)",
		 thread->stack_info.size, thread->stack_info.delta);

	/*
	 * GOSP needs its own per-thread stack, separate from both the
	 * current call frame (overwritten almost immediately) and a single
	 * shared address (a preempted thread can resume onto it mid-syscall,
	 * reproduced with test_syscall_switch_stress). priv_stack is a fixed
	 * CONFIG_PRIVILEGED_STACK_SIZE-byte array in the TCB, the same
	 * approach RISC-V/ARM/ARC/x86/xtensa use for their privileged
	 * stacks.
	 */
	uintptr_t kernel_sp = ROUND_DOWN((uintptr_t)thread->arch.priv_stack +
					 sizeof(thread->arch.priv_stack),
					 ARCH_STACK_PTR_ALIGN);

	/*
	 * Zero only the unused portion below the live C call chain, on this
	 * thread's own stack. Stop 8 bytes short of SP: memset() opens its
	 * own leaf frame there before writing anything.
	 */
	uintptr_t stack_ptr;

	__asm__ volatile("%0 = r29" : "=r"(stack_ptr));

	/*
	 * Guard against underflow for an unusually small stack combined
	 * with a deep call chain before this point; skip the clear instead
	 * of memset() past the stack buffer.
	 */
	if (stack_ptr > thread->stack_info.start + 8) {
		memset((void *)thread->stack_info.start, 0,
		       stack_ptr - 8 - thread->stack_info.start);
	}

	/*
	 * arch_tls_stack_setup() zeroed z_tls_current along with the rest
	 * of .tbss; ugp keeps pointing at this block, so restore it now for
	 * this thread's first syscall onward.
	 */
#ifdef CONFIG_CURRENT_THREAD_USE_TLS
	extern Z_THREAD_LOCAL k_tid_t z_tls_current;

	z_tls_current = thread;
#endif

	/*
	 * Record the drop to user mode so z_hexagon_user_mode_sync() keeps
	 * re-deriving _hexagon_user_mode_active as 1 on every later event
	 * exit, even one unrelated to this thread.
	 */
	thread->arch.priv_level = 1;

	/*
	 * Set the global flag now so arch_is_user_context() is already
	 * correct right after vmrte, before the first trap0 fires.
	 */
	_hexagon_user_mode_active = 1;

	/*
	 * H2 vmrte with GSSR.UM swaps r29 <-> GOSP.  To end up with
	 * r29=user_sp in user mode, set:
	 *   r29 = kernel_sp (will become gosp after swap)
	 *   GOSP (g2) = user_sp (will become r29 after swap)
	 */
	__asm__ volatile(
		"r4 = %[entry]\n\t"
		"r5 = %[vmest]\n\t"
		"r6 = %[stack]\n\t"      /* GOSP = user SP (becomes r29) */
		"r7 = #0\n\t"
		"g0 = r4\n\t"            /* GELR = user entry */
		"g1 = r5\n\t"            /* GSR = UM + IE */
		"g2 = r6\n\t"            /* GOSP = user SP */
		"g3 = r7\n\t"            /* GBADVA = 0 */
		"r0 = %[p1]\n\t"
		"r1 = %[p2]\n\t"
		"r2 = %[p3]\n\t"
		"r29 = %[ksp]\n\t"       /* kernel SP (becomes gosp) */
		"r31 = %[exit_fn]\n\t"   /* LR = user thread exit stub */
		"r30 = #0\n\t"           /* FP = 0 (no parent frame) */
		"trap1(#1)\n\t"          /* vmrte */
		:
		: [entry] "r"((uintptr_t)user_entry),
		  [vmest] "r"((uint32_t)0xC0000000), /* User mode + IE */
		  [ksp] "r"(kernel_sp),
		  [stack] "r"(user_sp),
		  [p1] "r"(p1),
		  [p2] "r"(p2),
		  [p3] "r"(p3),
		  [exit_fn] "r"((uintptr_t)hexagon_user_thread_exit)
		: "r0", "r1", "r2", "r4", "r5", "r6", "r7",
		  "r29", "r30", "r31", "memory"
	);

	CODE_UNREACHABLE;
}

void arch_syscall_invoke(uint32_t syscall_id, uint32_t arg1, uint32_t arg2, uint32_t arg3,
			 uint32_t arg4, uint32_t arg5, uint32_t arg6, struct arch_esf *esf)
{
	if (syscall_id >= K_SYSCALL_LIMIT) {
		esf->r0 = -ENOSYS;
		return;
	}

	esf->r0 = (uint32_t)_k_syscall_table[syscall_id](
		arg1, arg2, arg3, arg4, arg5, arg6, esf);
}

void z_hexagon_syscall_handler(struct arch_esf *esf)
{
	uint32_t syscall_id = esf->r6;

	arch_syscall_invoke(syscall_id, esf->r0, esf->r1, esf->r2, esf->r3, esf->r4, esf->r5, esf);
}

FUNC_NORETURN void arch_syscall_oops(void *ssf)
{
	struct arch_esf *esf = (struct arch_esf *)ssf;

	z_fatal_error(K_ERR_KERNEL_OOPS, esf);
	CODE_UNREACHABLE;
}

int arch_mem_domain_max_partitions_get(void)
{
	return CONFIG_MAX_DOMAIN_PARTITIONS;
}

int arch_mem_domain_init(struct k_mem_domain *domain)
{
	ARG_UNUSED(domain);
	return 0;
}

int arch_mem_domain_partition_add(struct k_mem_domain *domain, uint32_t partition_id)
{
	ARG_UNUSED(domain);
	ARG_UNUSED(partition_id);
	return 0;
}

int arch_mem_domain_partition_remove(struct k_mem_domain *domain, uint32_t partition_id)
{
	ARG_UNUSED(domain);
	ARG_UNUSED(partition_id);
	return 0;
}

void arch_mem_domain_thread_add(struct k_thread *thread)
{
	ARG_UNUSED(thread);
}

void arch_mem_domain_thread_remove(struct k_thread *thread)
{
	ARG_UNUSED(thread);
}

#endif /* CONFIG_USERSPACE */

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
#include <zephyr/logging/log.h>
#include <kernel_internal.h>
#include <hexagon_vm.h>
#include <hexagon_mmu.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

#ifdef CONFIG_USERSPACE

/*
 * Defined near the arch_mem_domain_* hooks below. Called from here,
 * switch.S, and event_handlers.S.
 */
void z_hexagon_mem_domain_switch(void);
static void hexagon_mem_domain_enter(struct k_thread *thread);

/* [start, start + size) inside [base, base + len), overflow-safe. */
static inline bool hexagon_range_within(uintptr_t start, size_t size, uintptr_t base, size_t len)
{
	return start >= base && size <= len && (start - base) <= (len - size);
}

/*
 * A supervisor thread has no installed user map. Allow access to its
 * own stack, plus reads from global text/rodata.
 */
static bool hexagon_supervisor_buffer_ok(const struct k_thread *thread, uintptr_t start,
					 size_t size, int write)
{
	if (hexagon_range_within(start, size, thread->stack_info.start, thread->stack_info.size)) {
		return true;
	}

	if (write != 0) {
		return false;
	}

	if (hexagon_range_within(start, size, (uintptr_t)__rom_region_start,
				 (uintptr_t)__rom_region_end - (uintptr_t)__rom_region_start)) {
		return true;
	}

	return hexagon_range_within(
		start, size, (uintptr_t)__rodata_region_start,
		(uintptr_t)__rodata_region_end - (uintptr_t)__rodata_region_start);
}

/*
 * Walk the calling thread's own installed map -- the same one H2
 * enforces on its user-mode accesses, so this check agrees with the
 * hardware by construction. That covers the thread's stack, its
 * domain's mappable partitions, and the fixed tail's U=1
 * text/rodata/console entries.
 */
int arch_buffer_validate(const void *addr, size_t size, int write)
{
	struct k_thread *thread = k_current_get();
	uint32_t need = __HVM_LINEAR_U | ((write != 0) ? __HVM_LINEAR_W : __HVM_LINEAR_R);
	uintptr_t start = (uintptr_t)addr;
	uintptr_t va;
	uintptr_t last;
	k_spinlock_key_t key;
	int ret = 0;

	if (size > (UINTPTR_MAX - start)) {
		return -EPERM;
	}
	/* Treat a zero-length request like a one-byte one: still confirm addr's page is mapped. */
	last = start + ((size != 0U) ? (size - 1U) : 0U);

	/*
	 * Only a thread that has dropped to user mode has a map. Also
	 * hold z_mem_domain_lock across the walk: trap0 re-enables guest
	 * interrupts, so a partition add/remove could otherwise rewrite
	 * the domain list under it.
	 */
	key = k_spin_lock(&z_mem_domain_lock);

	if (thread->arch.priv_level == 0 || thread->mem_domain_info.mem_domain == NULL) {
		ret = hexagon_supervisor_buffer_ok(thread, start, size, write) ? 0 : -EPERM;
		goto out;
	}

	va = start;
	while (true) {
		const struct hexagon_linear_entry *entry =
			hexagon_linear_lookup(thread->arch.map, va);
		uintptr_t entry_end;

		if (entry == NULL || (hexagon_linear_entry_xwru(entry) & need) != need) {
			ret = -EPERM;
			break;
		}

		entry_end = hexagon_linear_entry_end(entry);
		if (entry_end == 0U || last < entry_end) {
			break;
		}
		va = entry_end;
	}

out:
	k_spin_unlock(&z_mem_domain_lock, key);
	return ret;
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
	 * trap0 syscalls; a C wrapper's user-mode check could be optimized
	 * away. Syscall convention: r0 = arg/return value, r6 = syscall number.
	 *
	 * This runs in user mode, so ask the kernel for the current thread
	 * rather than reading _kernel directly.
	 */
	__asm__ volatile(
		/* syscall: r0 = k_sched_current_thread_query() */
		"r6 = #%[query_id]\n\t"
		"trap0(#0x1)\n\t"
		/* syscall: k_thread_abort(r0) */
		"r6 = #%[abort_id]\n\t"
		"trap0(#0x1)\n\t"
		/* should not return -- loop as backstop */
		"1: jump 1b\n\t"
		:
		: [query_id] "i"(K_SYSCALL_K_SCHED_CURRENT_THREAD_QUERY),
		  [abort_id] "i"(K_SYSCALL_K_THREAD_ABORT)
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
	hexagon_mem_domain_enter(thread);

	/*
	 * Install this thread's hardware memory-domain map before the
	 * privilege drop below -- the first user-mode access must already
	 * be bound by the domain's linear map, not the permissive boot
	 * table.
	 */
	z_hexagon_mem_domain_switch();

	/*
	 * Set the thread-local flag now so arch_is_user_context() is already
	 * correct right after vmrte, before the first trap0 fires.
	 */
	_hexagon_user_mode_active = 1;

#ifdef CONFIG_HW_STACK_PROTECTION
	/*
	 * Once this thread's FRAMELIMIT is priv_stack's, a switch away and
	 * back would load it while SP is still on this stack; keep
	 * interrupts off until vmrte, which re-enables them from GSR.IE.
	 */
	(void)arch_irq_lock();
	z_hexagon_stack_protect_user_enter(thread);
#endif

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

void z_impl_hexagon_user_fault(unsigned int reason)
{
	if (reason != K_ERR_STACK_CHK_FAIL) {
		reason = K_ERR_KERNEL_OOPS;
	}
	z_fatal_error(reason, NULL);
	CODE_UNREACHABLE;
}

static void z_vrfy_hexagon_user_fault(unsigned int reason)
{
	z_impl_hexagon_user_fault(reason);
}
#include <zephyr/syscalls/hexagon_user_fault_mrsh.c>

int arch_mem_domain_max_partitions_get(void)
{
	return CONFIG_MAX_DOMAIN_PARTITIONS;
}

/*
 * A partition, unlike a stack, is never rounded out: that would grant
 * user access to whatever shares its first and last pages. One that
 * isn't page-aligned is rejected and never mapped.
 */
static bool hexagon_partition_is_aligned(const struct k_mem_partition *part)
{
	return ((part->start | part->size) & (CONFIG_MMU_PAGE_SIZE - 1U)) == 0U;
}

#define HEXAGON_PARTITION_IS_PRIV_WRITABLE(attr) ((attr) & 0x08U) /* bit 3 */
#define HEXAGON_PARTITION_IS_PRIV_READABLE(attr) ((attr) & 0x18U) /* bits 3-4 */

/*
 * A user-accessible partition's single HVM entry binds the kernel too,
 * see hexagon_partition_attr_to_xwru(), so the kernel cannot be given
 * write access the user lacks. Such a partition is rejected and never
 * mapped.
 */
static bool hexagon_partition_attr_is_supported(k_mem_partition_attr_t attr)
{
	if (!K_MEM_PARTITION_IS_READABLE(attr) && !K_MEM_PARTITION_IS_WRITABLE(attr)) {
		return true;
	}

	return !HEXAGON_PARTITION_IS_PRIV_WRITABLE(attr) || K_MEM_PARTITION_IS_WRITABLE(attr);
}

static bool hexagon_partition_is_mappable(const struct k_mem_partition *part)
{
	return hexagon_partition_is_aligned(part) &&
	       hexagon_partition_attr_is_supported(part->attr);
}

/*
 * HVM has one R/W/X set per page plus U. Guest mode ignores U but enforces
 * R/W/X. H2 uses the installed map on kernel-mode TLB misses too, so ISR
 * and syscall accesses obey the same permissions.
 *
 * Partition permissions map as follows (X always follows the partition):
 *
 *   Partition type     U   R/W bits come from
 *   User-accessible    1   User permissions
 *   No user access     0   Privileged permissions
 *
 * A user-accessible partition has one permission set for both modes, so
 * hexagon_partition_attr_is_supported() rejects it if privileged write
 * access exceeds user write access. With U clear, a no-user-access
 * partition remains available to the kernel but not user mode.
 */
static uint32_t hexagon_partition_attr_to_xwru(k_mem_partition_attr_t attr)
{
	uint32_t xwru = 0U;

	if (K_MEM_PARTITION_IS_READABLE(attr) || K_MEM_PARTITION_IS_WRITABLE(attr)) {
		xwru |= __HVM_LINEAR_U;
		if (K_MEM_PARTITION_IS_READABLE(attr)) {
			xwru |= __HVM_LINEAR_R;
		}
		if (K_MEM_PARTITION_IS_WRITABLE(attr)) {
			xwru |= __HVM_LINEAR_W;
		}
	} else {
		if (HEXAGON_PARTITION_IS_PRIV_READABLE(attr)) {
			xwru |= __HVM_LINEAR_R;
		}
		if (HEXAGON_PARTITION_IS_PRIV_WRITABLE(attr)) {
			xwru |= __HVM_LINEAR_W;
		}
	}
	if (K_MEM_PARTITION_IS_EXECUTABLE(attr)) {
		xwru |= __HVM_LINEAR_X;
	}

	return xwru;
}

BUILD_ASSERT(!IS_ENABLED(CONFIG_SMP),
	     "Hexagon tracks one installed memory map: SMP is not supported");

/* User thread whose map is installed. NULL means hexagon_kernel_map is active. */
static struct k_thread *hexagon_map_thread;

/*
 * Fall back to the kernel map so the active list may be rewritten. It
 * never changes after boot, so its TLB entries stay.
 */
static void hexagon_mem_domain_uninstall(void)
{
	if (hexagon_map_thread == NULL) {
		return;
	}

	if (hexagon_vm_newmap(hexagon_kernel_map, VM_TRANS_TYPE_LINEAR,
			      VM_TLB_INVALIDATE_FALSE) != 0) {
		k_panic();
	}
	hexagon_map_thread = NULL;
}

/*
 * Rebuild domain->arch.list: one dense run of entries per non-empty,
 * page-aligned partition, then a chain link to the shared fixed tail.
 * Each member user thread reaches it through its own map
 * (hexagon_thread_map_rebuild()).
 *
 * Entries are packed densely: a zero-valued gap entry would look like
 * the all-zero terminator to the HVM walker and truncate the list.
 *
 * H2 walks the installed list on every TLB miss, kernel-mode ones
 * included, so never rewrite it in place while installed. Every member
 * map is marked stale so its next install invalidates its ASID.
 *
 * Called with z_mem_domain_lock held.
 */
static void hexagon_mem_domain_rebuild(struct k_mem_domain *domain, int skip_id)
{
	struct hexagon_linear_entry *list = domain->arch.list;
	uint32_t idx = 0;
	int remaining_partitions = domain->num_partitions;
	struct k_thread *thread;

	if (hexagon_map_thread != NULL &&
	    hexagon_map_thread->mem_domain_info.mem_domain == domain) {
		hexagon_mem_domain_uninstall();
	}

	for (int i = 0; remaining_partitions > 0 && i < CONFIG_MAX_DOMAIN_PARTITIONS; i++) {
		const struct k_mem_partition *part = &domain->partitions[i];

		if (part->size == 0) {
			continue;
		}
		remaining_partitions--;
		if (i == skip_id || !hexagon_partition_is_mappable(part)) {
			continue;
		}

		idx += hexagon_decompose_region(
			&list[idx], HEXAGON_MEM_DOMAIN_PARTITION_ENTRIES, part->start, part->size,
			hexagon_partition_attr_to_xwru(part->attr), __HEXAGON_C_WB_L2);
	}

	hexagon_linear_entry_set_chain(&list[idx], hexagon_fixed_tail);

	SYS_DLIST_FOR_EACH_CONTAINER(&domain->thread_mem_domain_list, thread,
				     mem_domain_info.thread_mem_domain_node) {
		thread->arch.map_stale = 1;
	}
}

/*
 * Rebuild thread->arch.map: put the thread's stack first, then chain to
 * its domain list. Threads in one domain share its partitions and fixed
 * tail, but each has a private stack map and ASID:
 *
 *   Thread A: [A stack] --chain--+
 *                                 +--> [domain partitions] --> [fixed tail]
 *   Thread B: [B stack] --chain--+
 *
 * The stack comes first so it wins over an overlapping partition in the
 * first-match-wins walk. H2 keys ASIDs on the list address
 * (H2K_asid_table_inc()), so no thread maps another's stack.
 *
 * Called with z_mem_domain_lock held.
 */
static void hexagon_thread_map_rebuild(struct k_thread *thread)
{
	struct hexagon_linear_entry *map = thread->arch.map;
	uintptr_t stack_addr = thread->stack_info.start;
	size_t stack_size = thread->stack_info.size;
	uint32_t idx;

	if (hexagon_map_thread == thread) {
		hexagon_mem_domain_uninstall();
	}

	hexagon_align_region_to_page(&stack_addr, &stack_size);

	idx = hexagon_decompose_region(map, CONFIG_HEXAGON_USER_STACK_ENTRIES, stack_addr,
				       stack_size, __HVM_LINEAR_R | __HVM_LINEAR_W | __HVM_LINEAR_U,
				       __HEXAGON_C_WB_L2);

	hexagon_linear_entry_set_chain(&map[idx], thread->mem_domain_info.mem_domain->arch.list);
	thread->arch.map_stale = 1;
}

int arch_mem_domain_init(struct k_mem_domain *domain)
{
	hexagon_mem_domain_rebuild(domain, -1);
	return 0;
}

/*
 * The core only deinits a domain once every member thread is gone,
 * and each one already uninstalled its map on removal, so nothing
 * should still point at domain->arch.list. Uninstall anyway as a
 * belt-and-suspenders check, then zero the list to a bare terminator
 * so a stale chain into it maps nothing.
 */
int arch_mem_domain_deinit(struct k_mem_domain *domain)
{
	if (hexagon_map_thread != NULL &&
	    hexagon_map_thread->mem_domain_info.mem_domain == domain) {
		hexagon_mem_domain_uninstall();
	}

	(void)memset(domain->arch.list, 0, sizeof(domain->arch.list));
	return 0;
}

int arch_mem_domain_partition_add(struct k_mem_domain *domain, uint32_t partition_id)
{
	const struct k_mem_partition *part = &domain->partitions[partition_id];

	/* The core has already recorded it. Rebuild skips it from now on. */
	if (!hexagon_partition_is_aligned(part)) {
		LOG_ERR("mem domain: partition %#lx/%#zx is not page-aligned",
			(unsigned long)part->start, part->size);
		__ASSERT(false, "partition is not page-aligned");
		return -EINVAL;
	}

	if (!hexagon_partition_attr_is_supported(part->attr)) {
		LOG_ERR("mem domain: partition %#lx/%#zx attr %#x gives priv more than user",
			(unsigned long)part->start, part->size, part->attr);
		__ASSERT(false, "partition attr is not supported");
		return -EINVAL;
	}

	hexagon_mem_domain_rebuild(domain, -1);
	return 0;
}

int arch_mem_domain_partition_remove(struct k_mem_domain *domain, uint32_t partition_id)
{
	/* Called before the core clears partitions[partition_id]. */
	hexagon_mem_domain_rebuild(domain, partition_id);
	return 0;
}

/*
 * A thread in supervisor mode has no map yet.
 * hexagon_mem_domain_enter() builds it when the thread drops to user mode.
 */
int arch_mem_domain_thread_add(struct k_thread *thread)
{
	if (thread->arch.priv_level != 0) {
		hexagon_thread_map_rebuild(thread);
	}
	return 0;
}

/*
 * The core has already unlinked thread, so later rebuilds of its old
 * domain no longer mark its map stale: stop using the map until
 * arch_mem_domain_thread_add() rebuilds it.
 */
int arch_mem_domain_thread_remove(struct k_thread *thread)
{
	if (hexagon_map_thread == thread) {
		hexagon_mem_domain_uninstall();
	}
	return 0;
}

/* Build thread's map, called once priv_level is set. */
static void hexagon_mem_domain_enter(struct k_thread *thread)
{
	k_spinlock_key_t key = k_spin_lock(&z_mem_domain_lock);

	hexagon_thread_map_rebuild(thread);
	k_spin_unlock(&z_mem_domain_lock, key);
}

/*
 * Reinstall whichever hardware memory map matches _current: called at
 * every point Hexagon already reloads other per-thread hardware state
 * on switch-in (z_hexagon_thread_start, EVENT_EXIT) and once more from
 * arch_user_mode_enter() for the very first drop to user mode.
 *
 * Resuming the thread whose map is installed is a no-op. Every rebuild
 * of that map, or of the domain list it chains to, uninstalls it first,
 * so the installed map is never stale.
 */
void z_hexagon_mem_domain_switch(void)
{
	struct k_thread *thread = k_sched_current_thread_query();

	if (thread->arch.priv_level == 0 || thread->mem_domain_info.mem_domain == NULL) {
		hexagon_mem_domain_uninstall();
		return;
	}

	if (thread == hexagon_map_thread) {
		return;
	}

	if (hexagon_vm_newmap(thread->arch.map, VM_TRANS_TYPE_LINEAR,
			      thread->arch.map_stale ? VM_TLB_INVALIDATE_TRUE
						     : VM_TLB_INVALIDATE_FALSE) != 0) {
		k_panic();
	}

	thread->arch.map_stale = 0;
	hexagon_map_thread = thread;
}

#endif /* CONFIG_USERSPACE */

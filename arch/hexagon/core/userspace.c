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
#include <zephyr/init.h>
#include <kernel_internal.h>
#include <hexagon_vm.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

#ifdef CONFIG_USERSPACE

/* Defined near the arch_mem_domain_* hooks below; called from here, from
 * switch.S, and from event_handlers.S.
 */
void z_hexagon_mem_domain_switch(void);

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

	/*
	 * Install this thread's hardware memory-domain map before the
	 * privilege drop below -- the first user-mode access must already
	 * be bound by the domain's linear map, not the permissive boot
	 * table.
	 */
	z_hexagon_mem_domain_switch();

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

struct hexagon_size_class {
	uint32_t size_class; /* one of __HVM_LINEAR_SIZE_* */
	uint32_t bytes;
};

/* zephyr-keep-sorted start */
static const struct hexagon_size_class hexagon_size_classes[] = {
	{ __HVM_LINEAR_SIZE_16MB, 0x1000000U },
	{ __HVM_LINEAR_SIZE_4MB, 0x400000U },
	{ __HVM_LINEAR_SIZE_1MB, 0x100000U },
	{ __HVM_LINEAR_SIZE_256KB, 0x40000U },
	{ __HVM_LINEAR_SIZE_64KB, 0x10000U },
	{ __HVM_LINEAR_SIZE_16KB, 0x4000U },
	{ __HVM_LINEAR_SIZE_4KB, 0x1000U },
};
/* zephyr-keep-sorted stop */

/*
 * hexagon_size_classes[] bottoms out at 4KB: HVM's VM_TRANS_TYPE_LINEAR
 * format has no finer granularity. Zephyr's own ARCH_STACK_PTR_ALIGN on
 * Hexagon is only 8 bytes, so a thread's stack_info.start is routinely
 * not 4KB-aligned -- round outward to the enclosing page here rather
 * than in hexagon_decompose_region() itself, so that helper's "exact
 * multiple of a size class" invariant stays simple.
 */
static void hexagon_align_region_to_page(uintptr_t *addr, size_t *size)
{
	uintptr_t start = ROUND_DOWN(*addr, 0x1000U);
	uintptr_t end = ROUND_UP(*addr + *size, 0x1000U);

	*addr = start;
	*size = end - start;
}

/*
 * Greedily decompose [addr, addr+size) into the fewest
 * largest-aligned-size-class-first VM_TRANS_TYPE_LINEAR entries that
 * fit within max_entries, written starting at entries[0]. Returns the
 * number of entries used.
 *
 * Never asserts or panics: like RISC-V PMP's own "stop programming
 * rather than assert" precedent, a region too irregularly shaped or
 * too large for its budget just loses hardware coverage for the
 * uncovered tail (logged) instead of crashing the kernel. A
 * well-aligned region -- the common case -- needs exactly one entry.
 * Callers must page-align addr/size first (see
 * hexagon_align_region_to_page()): the smallest size class is 4KB, so
 * an unaligned addr can never match any class.
 */
static uint32_t hexagon_decompose_region(struct hexagon_linear_entry *entries,
					  uint32_t max_entries, uintptr_t addr, size_t size,
					  uint32_t xwru, uint32_t cache_attr)
{
	uint32_t n = 0;

	while (size > 0) {
		if (n >= max_entries) {
			LOG_ERR("mem domain: region %#lx/%#zx exhausted its %u-entry budget",
				(unsigned long)addr, size, max_entries);
			break;
		}

		bool placed = false;

		for (size_t i = 0; i < ARRAY_SIZE(hexagon_size_classes); i++) {
			uint32_t chunk = hexagon_size_classes[i].bytes;

			if (chunk > size || (addr % chunk) != 0) {
				continue;
			}

			hexagon_linear_entry_set(&entries[n], addr, addr,
						  hexagon_size_classes[i].size_class, xwru,
						  cache_attr, 0);
			n++;
			addr += chunk;
			size -= chunk;
			placed = true;
			break;
		}

		if (!placed) {
			LOG_ERR("mem domain: region %#lx/%#zx not coverable by any size class",
				(unsigned long)addr, size);
			break;
		}
	}

	return n;
}

extern char __start[];
extern char _hexagon_page_table[];

/* Matches _setup_page_table's own RAM granule (hvm_event_vectors.S). */
#define HEXAGON_RAM_CHUNK_SIZE 0x400000U

#define HEXAGON_FIXED_TAIL_MAX_RAM_ENTRIES 8

/*
 * Budgets for the page-granular carve-outs decomposed via
 * hexagon_decompose_region(), same rationale as
 * HEXAGON_MEM_DOMAIN_STACK_ENTRIES/HEXAGON_MEM_DOMAIN_PARTITION_ENTRIES
 * (thread.h): a well-aligned region needs exactly one entry, this
 * budget just covers oddly-aligned/sized ones too.
 */
#define HEXAGON_FIXED_TAIL_TEXT_ENTRIES   6
#define HEXAGON_FIXED_TAIL_RODATA_ENTRIES 5

/*
 * Text/rodata carve-out entries + RAM entries (up to
 * HEXAGON_FIXED_TAIL_MAX_RAM_ENTRIES) + UART + two H2-kernel entries +
 * one all-zero terminator.
 */
#define HEXAGON_FIXED_TAIL_ENTRIES                                                               \
	(HEXAGON_FIXED_TAIL_TEXT_ENTRIES + HEXAGON_FIXED_TAIL_RODATA_ENTRIES +                    \
	 HEXAGON_FIXED_TAIL_MAX_RAM_ENTRIES + 4)

/*
 * Shared tail chained onto every per-thread linear list (own-stack and
 * partition entries first, see hexagon_mem_domain_rebuild()): kernel
 * text, kernel rodata, RAM, UART, and the H2 kernel image, at the same
 * addresses/cache attributes the boot Table map already grants
 * everywhere -- except U is cleared for the RAM/H2-kernel entries,
 * unlike the boot table's blanket U=1. That is what actually enforces
 * a domain: a domain-restricted thread's own user-mode accesses
 * (GSR.UM=1) are gated by U, so RAM/H2-kernel access outside this
 * thread's own domain entries has to come from the domain's own list,
 * not this fallback.
 *
 * Text and rodata are the exception, kept U=1 here and listed first so
 * the first-match-wins linear walk (H2K_linear_translate()) prefers
 * them over the U=0 RAM entries that would otherwise also cover the
 * same addresses: Zephyr's memory-domain model isolates *data*
 * partitions, but ordinary kernel/shared code (this includes the
 * ztest framework and any other kernel-image .text/.rodata a user
 * thread calls into or reads string literals from) stays globally
 * executable/readable to every thread regardless of domain, the same
 * way RISC-V PMP carves out __rom_region/__rodata_region as locked,
 * always-present entries ahead of the per-thread dynamic ones
 * (arch/riscv/core/pmp.c). This uses __text_region_start/end rather
 * than __rom_region_start/end: on this board __rom_region_start
 * resolves to 0 rather than the image base, making __rom_region span
 * the entire low address space instead of just text+rodata (also
 * affects arch_buffer_validate()'s software check above, a
 * pre-existing and separate issue left untouched here).
 * __text_region_start/end is tight and correct. UART keeps U=1
 * unchanged (uart_poll_out()
 * runs at USER tier during trap0 dispatch, same as the boot table's
 * own comment on that bit -- unrelated to domain isolation).
 * Guest-mode/kernel-mode accesses (GSR.UM=0, i.e. while handling a
 * trap0/exception) ignore U entirely, so this tail is what keeps
 * kernel code, ISR stubs, and this thread's own priv_stack reachable
 * no matter which domain is installed.
 */
static struct hexagon_linear_entry hexagon_fixed_tail[HEXAGON_FIXED_TAIL_ENTRIES];

static int hexagon_fixed_tail_init(void)
{
	uintptr_t aligned_start = ROUND_DOWN((uintptr_t)__start, HEXAGON_RAM_CHUNK_SIZE);
	size_t len = (uintptr_t)_image_ram_end - aligned_start;
	uint32_t ram_chunks = (uint32_t)DIV_ROUND_UP(len, HEXAGON_RAM_CHUNK_SIZE);
	uint32_t idx = 0;
	uintptr_t region_addr;
	size_t region_size;
	uintptr_t text_start = (uintptr_t)__text_region_start;
	uintptr_t text_end = (uintptr_t)__text_region_end;
	uintptr_t rodata_start = (uintptr_t)__rodata_region_start;
	uintptr_t rodata_end = (uintptr_t)__rodata_region_end;

	if (ram_chunks > HEXAGON_FIXED_TAIL_MAX_RAM_ENTRIES) {
		k_panic();
	}

	if (text_end > text_start) {
		region_addr = text_start;
		region_size = text_end - text_start;
		hexagon_align_region_to_page(&region_addr, &region_size);
		idx += hexagon_decompose_region(&hexagon_fixed_tail[idx],
						 HEXAGON_FIXED_TAIL_TEXT_ENTRIES, region_addr,
						 region_size,
						 __HVM_LINEAR_R | __HVM_LINEAR_X | __HVM_LINEAR_U,
						 __HEXAGON_C_WB_L2);
	}

	if (rodata_end > rodata_start) {
		region_addr = rodata_start;
		region_size = rodata_end - rodata_start;
		hexagon_align_region_to_page(&region_addr, &region_size);
		idx += hexagon_decompose_region(&hexagon_fixed_tail[idx],
						 HEXAGON_FIXED_TAIL_RODATA_ENTRIES, region_addr,
						 region_size, __HVM_LINEAR_R | __HVM_LINEAR_U,
						 __HEXAGON_C_WB_L2);
	}

	for (uint32_t i = 0; i < ram_chunks; i++) {
		uintptr_t pa = aligned_start + (uintptr_t)i * HEXAGON_RAM_CHUNK_SIZE;

		hexagon_linear_entry_set(&hexagon_fixed_tail[idx], pa, pa,
					  __HVM_LINEAR_SIZE_4MB,
					  __HVM_LINEAR_R | __HVM_LINEAR_W | __HVM_LINEAR_X,
					  __HEXAGON_C_WB_L2, 0);
		idx++;
	}

	hexagon_linear_entry_set(&hexagon_fixed_tail[idx], 0x10000000U, 0x10000000U,
				  __HVM_LINEAR_SIZE_4MB,
				  __HVM_LINEAR_R | __HVM_LINEAR_W | __HVM_LINEAR_U,
				  __HEXAGON_C_DEV, __HVM_LINEAR_SHARED);
	idx++;

	hexagon_linear_entry_set(&hexagon_fixed_tail[idx], 0x9b800000U, 0x9b800000U,
				  __HVM_LINEAR_SIZE_4MB,
				  __HVM_LINEAR_R | __HVM_LINEAR_W | __HVM_LINEAR_X,
				  __HEXAGON_C_WB_L2, 0);
	idx++;

	hexagon_linear_entry_set(&hexagon_fixed_tail[idx], 0x9bc00000U, 0x9bc00000U,
				  __HVM_LINEAR_SIZE_4MB,
				  __HVM_LINEAR_R | __HVM_LINEAR_W | __HVM_LINEAR_X,
				  __HEXAGON_C_WB_L2, 0);
	idx++;

	/* hexagon_fixed_tail[idx] stays all-zero: list terminator. */

	return 0;
}

SYS_INIT(hexagon_fixed_tail_init, POST_KERNEL, 0);

/*
 * Only the *user*-visible bits of K_MEM_PARTITION_P_* map onto the
 * single, shared HVM R/W/X(+U) per page -- there is no hardware split
 * between what privileged and user code may do to the same page.
 * Kernel/supervisor code never runs under a domain's restricted map
 * (only a user thread ever gets one installed, see
 * z_hexagon_mem_domain_switch()), so the priv-side bits of the attr
 * encoding are irrelevant here and intentionally ignored.
 *
 * U is set unconditionally: this entry only ever appears in a list
 * that is installed specifically for this domain's own user thread, so
 * an R/W/X-less, no-user-access partition (K_MEM_PARTITION_P_*_U_NA)
 * correctly ends up denying all access (no R/W/X bits set) rather than
 * also blocking supervisor-side use of the same address elsewhere,
 * which this bit has no bearing on.
 */
static uint32_t hexagon_partition_attr_to_xwru(k_mem_partition_attr_t attr)
{
	uint32_t xwru = __HVM_LINEAR_U;

	if (attr & 0x01) { /* bit 0: user-readable, see arch.h */
		xwru |= __HVM_LINEAR_R;
	}
	if (K_MEM_PARTITION_IS_WRITABLE(attr)) {
		xwru |= __HVM_LINEAR_W;
	}
	if (K_MEM_PARTITION_IS_EXECUTABLE(attr)) {
		xwru |= __HVM_LINEAR_X;
	}

	return xwru;
}

/*
 * Rebuild thread->arch.mem_domain_list from scratch: own-stack entries
 * first (narrowest, so they win the first-match-wins walk over a
 * partition that happens to overlap), then one dense run of entries
 * per non-empty partition in domain, then a chain link to the shared
 * fixed tail (RAM/UART/H2-kernel fallback + terminator).
 *
 * Entries are packed densely (idx += actual entries used, never a
 * fixed per-partition stride): a zero-valued gap entry in the middle
 * of the list would look like the all-zero terminator to the HVM
 * walker and silently truncate everything after it. Stale content
 * *after* the chain entry, left over from a previous, longer rebuild,
 * is harmless -- the walker jumps via the chain entry's target rather
 * than continuing to scan past it.
 */
static void hexagon_mem_domain_rebuild(struct k_thread *thread, struct k_mem_domain *domain)
{
	struct hexagon_linear_entry *list = thread->arch.mem_domain_list;
	uint32_t idx;
	k_spinlock_key_t key;
	uintptr_t stack_addr = thread->stack_info.start;
	size_t stack_size = thread->stack_info.size;

	hexagon_align_region_to_page(&stack_addr, &stack_size);

	idx = hexagon_decompose_region(&list[0], HEXAGON_MEM_DOMAIN_STACK_ENTRIES,
					stack_addr, stack_size,
					__HVM_LINEAR_R | __HVM_LINEAR_W | __HVM_LINEAR_U,
					__HEXAGON_C_WB_L2);

	/*
	 * Guards partitions[]/num_partitions against a concurrent
	 * k_mem_domain_add/remove_partition() on another CPU/thread, same
	 * as arch_buffer_validate()'s own scan above.
	 */
	key = k_spin_lock(&z_mem_domain_lock);

	int remaining_partitions = domain->num_partitions;

	for (int i = 0; remaining_partitions > 0 && i < CONFIG_MAX_DOMAIN_PARTITIONS; i++) {
		const struct k_mem_partition *part = &domain->partitions[i];

		if (part->size == 0) {
			continue;
		}
		remaining_partitions--;

		uintptr_t part_addr = part->start;
		size_t part_size = part->size;

		hexagon_align_region_to_page(&part_addr, &part_size);

		idx += hexagon_decompose_region(&list[idx], HEXAGON_MEM_DOMAIN_PARTITION_ENTRIES,
						 part_addr, part_size,
						 hexagon_partition_attr_to_xwru(part->attr),
						 __HEXAGON_C_WB_L2);
	}

	k_spin_unlock(&z_mem_domain_lock, key);

	hexagon_linear_entry_set_chain(&list[idx], hexagon_fixed_tail);
}

int arch_mem_domain_init(struct k_mem_domain *domain)
{
	/*
	 * Start above the sentinel value _thread_arch.mem_domain_generation
	 * is zero-initialized to, so a thread's very first switch-in under
	 * this domain always forces a resync instead of coincidentally
	 * matching before arch_mem_domain_thread_add() has ever run for it.
	 */
	domain->arch.generation = 1;
	return 0;
}

int arch_mem_domain_partition_add(struct k_mem_domain *domain, uint32_t partition_id)
{
	ARG_UNUSED(partition_id);

	domain->arch.generation++;
	return 0;
}

int arch_mem_domain_partition_remove(struct k_mem_domain *domain, uint32_t partition_id)
{
	ARG_UNUSED(partition_id);

	domain->arch.generation++;
	return 0;
}

void arch_mem_domain_thread_add(struct k_thread *thread)
{
	/*
	 * Force a resync before this thread's next switch-in. Without
	 * this, a thread migrating from domain A to domain B could keep a
	 * generation value that coincidentally matches B's *current*
	 * counter, skipping the resync and running under A's stale map
	 * while believing it is under B. Domain generations only ever
	 * count up from 1 (arch_mem_domain_init()), so 0 can never
	 * coincidentally match a real domain's generation.
	 */
	thread->arch.mem_domain_generation = 0;
}

void arch_mem_domain_thread_remove(struct k_thread *thread)
{
	/*
	 * No action needed: thread->mem_domain_info.mem_domain already
	 * becomes NULL (by kernel core, outside arch code) before this
	 * thread can switch in again, and z_hexagon_mem_domain_switch()'s
	 * domain == NULL branch already falls back to the boot table map
	 * in that case.
	 */
	ARG_UNUSED(thread);
}

/* NULL means "the boot Table map is currently installed". */
static struct k_thread *hexagon_mem_domain_active_thread;

/*
 * Reinstall the flat, permissive boot Table map -- current behavior
 * for any thread with no mem domain, or that has never dropped to user
 * mode. Matches _setup_page_table's own install call at boot.
 */
static void hexagon_mem_domain_install_boot_table(void)
{
	int ret = hexagon_vm_newmap(_hexagon_page_table, VM_TRANS_TYPE_TABLE,
				     VM_TLB_INVALIDATE_TRUE);

	if (ret != 0) {
		k_panic();
	}
}

/*
 * Reinstall whichever hardware memory map matches _current: called at
 * every point Hexagon already reloads other per-thread hardware state
 * on switch-in (z_hexagon_thread_start, EVENT_EXIT) and once more from
 * arch_user_mode_enter() for the very first drop to user mode.
 *
 * hexagon_mem_domain_active_thread avoids a redundant vmnewmap() -- a
 * real hypercall, same cost class as an ARM64 TTBR or x86 CR3 reload
 * -- when the same thread/table is already installed and nothing about
 * its domain has changed since.
 */
void z_hexagon_mem_domain_switch(void)
{
	struct k_thread *thread = k_sched_current_thread_query();
	struct k_mem_domain *domain =
		thread->arch.priv_level != 0 ? thread->mem_domain_info.mem_domain : NULL;

	if (domain == NULL) {
		if (hexagon_mem_domain_active_thread != NULL) {
			hexagon_mem_domain_install_boot_table();
			hexagon_mem_domain_active_thread = NULL;
		}
		return;
	}

	if (hexagon_mem_domain_active_thread == thread &&
	    thread->arch.mem_domain_generation == domain->arch.generation) {
		return;
	}

	hexagon_mem_domain_rebuild(thread, domain);

	int ret = hexagon_vm_newmap(thread->arch.mem_domain_list, VM_TRANS_TYPE_LINEAR,
				     VM_TLB_INVALIDATE_TRUE);

	if (ret != 0) {
		k_panic();
	}

	thread->arch.mem_domain_generation = domain->arch.generation;
	hexagon_mem_domain_active_thread = thread;
}

#endif /* CONFIG_USERSPACE */

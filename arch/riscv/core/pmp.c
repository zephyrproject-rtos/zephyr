/*
 * Copyright (c) 2022 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Physical Memory Protection (PMP) is RISC-V parlance for an MPU.
 *
 * The PMP is comprized of a number of entries or slots. This number depends
 * on the hardware design. For each slot there is an address register and
 * a configuration register. While each address register is matched to an
 * actual CSR register, configuration registers are small and therefore
 * several of them are bundled in a few additional CSR registers.
 *
 * PMP slot configurations are updated in memory to avoid read-modify-write
 * cycles on corresponding CSR registers. Relevant CSR registers are always
 * written in batch from their shadow copy in RAM for better efficiency.
 *
 * In the stackguard case we keep an m-mode copy for each thread. Each user
 * mode threads also has a u-mode copy. This makes faster context switching
 * as precomputed content just have to be written to actual registers with
 * no additional processing.
 *
 * Thread-specific m-mode and u-mode PMP entries start from the PMP slot
 * indicated by global_pmp_end_index. Lower slots are used by global entries
 * which are never modified.
 */

#include <zephyr/kernel.h>
#include <kernel_internal.h>
#include <zephyr/linker/linker-defs.h>
#include <pmp.h>
#include <zephyr/sys/arch_interface.h>
#include <zephyr/arch/riscv/csr.h>

#define LOG_LEVEL CONFIG_MPU_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mpu);

#define PMP_DEBUG_DUMP 0

#ifdef CONFIG_64BIT
# define PR_ADDR "0x%016lx"
#else
# define PR_ADDR "0x%08lx"
#endif

#define PMPCFG_STRIDE sizeof(unsigned long)

#define PMP_ADDR(addr)			((addr) >> 2)
#define NAPOT_RANGE(size)		(((size) - 1) >> 1)
#define PMP_ADDR_NAPOT(addr, size)	PMP_ADDR(addr | NAPOT_RANGE(size))

#define PMP_NONE 0

static void print_pmp_entries(unsigned int start, unsigned int end,
			      unsigned long *pmp_addr, unsigned long *pmp_cfg,
			      const char *banner)
{
	uint8_t *pmp_n_cfg = (uint8_t *)pmp_cfg;
	unsigned int index;

	LOG_DBG("PMP %s:", banner);
	for (index = start; index < end; index++) {
		unsigned long start, end, tmp;

		switch (pmp_n_cfg[index] & PMP_A) {
		case PMP_TOR:
			start = (index == 0) ? 0 : (pmp_addr[index - 1] << 2);
			end = (pmp_addr[index] << 2) - 1;
			break;
		case PMP_NA4:
			start = pmp_addr[index] << 2;
			end = start + 3;
			break;
		case PMP_NAPOT:
			tmp = (pmp_addr[index] << 2) | 0x3;
			start = tmp & (tmp + 1);
			end   = tmp | (tmp + 1);
			break;
		default:
			start = 0;
			end = 0;
			break;
		}

		if (end == 0) {
			LOG_DBG("%3d: "PR_ADDR" 0x%02x", index,
				pmp_addr[index],
				pmp_n_cfg[index]);
		} else {
			LOG_DBG("%3d: "PR_ADDR" 0x%02x --> "
				PR_ADDR"-"PR_ADDR" %c%c%c%s",
				index, pmp_addr[index], pmp_n_cfg[index],
				start, end,
				(pmp_n_cfg[index] & PMP_R) ? 'R' : '-',
				(pmp_n_cfg[index] & PMP_W) ? 'W' : '-',
				(pmp_n_cfg[index] & PMP_X) ? 'X' : '-',
				(pmp_n_cfg[index] & PMP_L) ? " LOCKED" : "");
		}
	}
}

static void dump_pmp_regs(const char *banner)
{
	unsigned long pmp_addr[CONFIG_PMP_SLOTS];
	unsigned long pmp_cfg[CONFIG_PMP_SLOTS / PMPCFG_STRIDE];

#define PMPADDR_READ(x) pmp_addr[x] = csr_read(pmpaddr##x)

	FOR_EACH(PMPADDR_READ, (;), 0, 1, 2, 3, 4, 5, 6, 7);
#if CONFIG_PMP_SLOTS > 8
	FOR_EACH(PMPADDR_READ, (;), 8, 9, 10, 11, 12, 13, 14, 15);
#endif

#undef PMPADDR_READ

#ifdef CONFIG_64BIT
	pmp_cfg[0] = csr_read(pmpcfg0);
#if CONFIG_PMP_SLOTS > 8
	pmp_cfg[1] = csr_read(pmpcfg2);
#endif
#else
	pmp_cfg[0] = csr_read(pmpcfg0);
	pmp_cfg[1] = csr_read(pmpcfg1);
#if CONFIG_PMP_SLOTS > 8
	pmp_cfg[2] = csr_read(pmpcfg2);
	pmp_cfg[3] = csr_read(pmpcfg3);
#endif
#endif

	print_pmp_entries(0, CONFIG_PMP_SLOTS, pmp_addr, pmp_cfg, banner);
}

/**
 * @brief Set PMP shadow register values in memory
 *
 * Register content is built using this function which selects the most
 * appropriate address matching mode automatically. Note that the special
 * case start=0 size=0 is valid and means the whole address range.
 *
 * @param index_p Location of the current PMP slot index to use. This index
 *                will be updated according to the number of slots used.
 * @param perm PMP permission flags
 * @param start Start address of the memory area to cover
 * @param size Size of the memory area to cover
 * @param pmp_addr Array of pmpaddr values (starting at entry 0).
 * @param pmp_cfg Array of pmpcfg values (starting at entry 0).
 * @param index_limit Index value representing the size of the provided arrays.
 * @return true on success, false when out of free PMP slots.
 */
static bool set_pmp_entry(unsigned int *index_p, uint8_t perm,
			  uintptr_t start, size_t size,
			  unsigned long *pmp_addr, unsigned long *pmp_cfg,
			  unsigned int index_limit)
{
	uint8_t *pmp_n_cfg = (uint8_t *)pmp_cfg;
	unsigned int index = *index_p;
	bool ok = true;

	__ASSERT((start & 0x3) == 0, "misaligned start address");
	__ASSERT((size & 0x3) == 0, "misaligned size");

	if (index >= index_limit) {
		LOG_ERR("out of PMP slots");
		ok = false;
	} else if ((index == 0 && start == 0) ||
		   (index != 0 && pmp_addr[index - 1] == PMP_ADDR(start))) {
		/* We can use TOR using only one additional slot */
		pmp_addr[index] = PMP_ADDR(start + size);
		pmp_n_cfg[index] = perm | PMP_TOR;
		index += 1;
	} else if (((size  & (size - 1)) == 0) /* power of 2 */ &&
		   ((start & (size - 1)) == 0) /* naturally aligned */) {
		pmp_addr[index] = PMP_ADDR_NAPOT(start, size);
		pmp_n_cfg[index] = perm | (size == 4 ? PMP_NA4 : PMP_NAPOT);
		index += 1;
	} else if (index + 1 >= index_limit) {
		LOG_ERR("out of PMP slots");
		ok = false;
	} else {
		pmp_addr[index] = PMP_ADDR(start);
		pmp_n_cfg[index] = 0;
		index += 1;
		pmp_addr[index] = PMP_ADDR(start + size);
		pmp_n_cfg[index] = perm | PMP_TOR;
		index += 1;
	}

	*index_p = index;
	return ok;
}

/**
 * @brief Write a range of PMP entries to corresponding PMP registers
 *
 * PMP registers are accessed with the csr instruction which only takes an
 * immediate value as the actual register. This is performed more efficiently
 * in assembly code (pmp.S) than what is possible with C code.
 *
 * Requirement: start < end && end <= CONFIG_PMP_SLOTS
 *
 * @param start Start of the PMP range to be written
 * @param end End (exclusive) of the PMP range to be written
 * @param clear_trailing_entries True if trailing entries must be turned off
 * @param pmp_addr Array of pmpaddr values (starting at entry 0).
 * @param pmp_cfg Array of pmpcfg values (starting at entry 0).
 */
extern void z_riscv_write_pmp_entries(unsigned int start, unsigned int end,
				      bool clear_trailing_entries,
				      const unsigned long *pmp_addr,
				      const unsigned long *pmp_cfg);

/**
 * @brief Write a range of PMP entries to corresponding PMP registers
 *
 * This performs some sanity checks before calling z_riscv_write_pmp_entries().
 *
 * @param start Start of the PMP range to be written
 * @param end End (exclusive) of the PMP range to be written
 * @param clear_trailing_entries True if trailing entries must be turned off
 * @param pmp_addr Array of pmpaddr values (starting at entry 0).
 * @param pmp_cfg Array of pmpcfg values (starting at entry 0).
 * @param index_limit Index value representing the size of the provided arrays.
 */
static void write_pmp_entries(unsigned int start, unsigned int end,
			      bool clear_trailing_entries,
			      unsigned long *pmp_addr, unsigned long *pmp_cfg,
			      unsigned int index_limit)
{
	__ASSERT(start < end && end <= index_limit &&
		 index_limit <= CONFIG_PMP_SLOTS,
		 "bad PMP range (start=%u end=%u)", start, end);

	/* Be extra paranoid in case assertions are disabled */
	if (start >= end || end > index_limit) {
		k_panic();
	}

	if (clear_trailing_entries) {
		/*
		 * There are many config entries per pmpcfg register.
		 * Make sure to clear trailing garbage in the last
		 * register to be written if any. Remaining registers
		 * will be cleared in z_riscv_write_pmp_entries().
		 */
		uint8_t *pmp_n_cfg = (uint8_t *)pmp_cfg;
		unsigned int index;

		for (index = end; index % PMPCFG_STRIDE != 0; index++) {
			pmp_n_cfg[index] = 0;
		}
	}

	print_pmp_entries(start, end, pmp_addr, pmp_cfg, "register write");

#ifdef CONFIG_QEMU_TARGET
	/*
	 * A QEMU bug may create bad transient PMP representations causing
	 * false access faults to be reported. Work around it by setting
	 * pmp registers to zero from the update start point to the end
	 * before updating them with new values.
	 * The QEMU fix is here with more details about this bug:
	 * https://lists.gnu.org/archive/html/qemu-devel/2022-06/msg02800.html
	 */
	static const unsigned long pmp_zero[CONFIG_PMP_SLOTS] = { 0, };

	z_riscv_write_pmp_entries(start, CONFIG_PMP_SLOTS, false,
				  pmp_zero, pmp_zero);
#endif

	z_riscv_write_pmp_entries(start, end, clear_trailing_entries,
				  pmp_addr, pmp_cfg);
}

/**
 * @brief Abstract the last 3 arguments to set_pmp_entry() and
 *        write_pmp_entries( for m-mode.
 */
#define PMP_M_MODE(thread) \
	thread->arch.m_mode_pmpaddr_regs, \
	thread->arch.m_mode_pmpcfg_regs, \
	ARRAY_SIZE(thread->arch.m_mode_pmpaddr_regs)

/**
 * @brief Abstract the last 3 arguments to set_pmp_entry() and
 *        write_pmp_entries( for u-mode.
 */
#define PMP_U_MODE(thread) \
	thread->arch.u_mode_pmpaddr_regs, \
	thread->arch.u_mode_pmpcfg_regs, \
	ARRAY_SIZE(thread->arch.u_mode_pmpaddr_regs)

/*
 * This is used to seed thread PMP copies with global m-mode cfg entries
 * sharing the same cfg register. Locked entries aren't modifiable but
 * we could have non-locked entries here too.
 */
static unsigned long global_pmp_cfg[1];
static unsigned long global_pmp_last_addr;

/* End of global PMP entry range */
static unsigned int global_pmp_end_index;

/**
 * @Brief Initialize the PMP with global entries on each CPU
 */
void z_riscv_pmp_init(void)
{
	unsigned long pmp_addr[4];
	unsigned long pmp_cfg[1];
	unsigned int index = 0;

	/* The read-only area is always there for every mode */
	set_pmp_entry(&index, PMP_R | PMP_X | PMP_L,
		      (uintptr_t)__rom_region_start,
		      (size_t)__rom_region_size,
		      pmp_addr, pmp_cfg, ARRAY_SIZE(pmp_addr));

#ifdef CONFIG_PMP_STACK_GUARD
	/*
	 * Set the stack guard for this CPU's IRQ stack by making the bottom
	 * addresses inaccessible. This will never change so we do it here
	 * and lock it too.
	 */
	set_pmp_entry(&index, PMP_NONE | PMP_L,
		      (uintptr_t)z_interrupt_stacks[_current_cpu->id],
		      Z_RISCV_STACK_GUARD_SIZE,
		      pmp_addr, pmp_cfg, ARRAY_SIZE(pmp_addr));
#endif

	write_pmp_entries(0, index, true, pmp_addr, pmp_cfg, ARRAY_SIZE(pmp_addr));

#ifdef CONFIG_SMP
#ifdef CONFIG_PMP_STACK_GUARD
	/*
	 * The IRQ stack guard area is different for each CPU.
	 * Make sure TOR entry sharing won't be attempted with it by
	 * remembering a bogus address for those entries.
	 */
	pmp_addr[index - 1] = -1L;
#endif

	/* Make sure secondary CPUs produced the same values */
	if (global_pmp_end_index != 0) {
		__ASSERT(global_pmp_end_index == index, "");
		__ASSERT(global_pmp_cfg[0] == pmp_cfg[0], "");
		__ASSERT(global_pmp_last_addr == pmp_addr[index - 1], "");
	}
#endif

	global_pmp_cfg[0] = pmp_cfg[0];
	global_pmp_last_addr = pmp_addr[index - 1];
	global_pmp_end_index = index;

	if (PMP_DEBUG_DUMP) {
		dump_pmp_regs("initial register dump");
	}
}

/**
 * @Brief Initialize the per-thread PMP register copy with global values.
 */
static inline unsigned int z_riscv_pmp_thread_init(unsigned long *pmp_addr,
						   unsigned long *pmp_cfg,
						   unsigned int index_limit)
{
	ARG_UNUSED(index_limit);

	/*
	 * Retrieve pmpcfg0 partial content from global entries.
	 */
	pmp_cfg[0] = global_pmp_cfg[0];

	/*
	 * Retrieve the pmpaddr value matching the last global PMP slot.
	 * This is so that set_pmp_entry() can safely attempt TOR with it.
	 */
	pmp_addr[global_pmp_end_index - 1] = global_pmp_last_addr;

	return global_pmp_end_index;
}

#ifdef CONFIG_PMP_STACK_GUARD

/**
 * @brief Prepare the PMP stackguard content for given thread.
 *
 * This is called once during new thread creation.
 */
void z_riscv_pmp_stackguard_prepare(struct k_thread *thread)
{
	unsigned int index = z_riscv_pmp_thread_init(PMP_M_MODE(thread));
	uintptr_t stack_bottom;

	/* make the bottom addresses of our stack inaccessible */
	stack_bottom = thread->stack_info.start - K_KERNEL_STACK_RESERVED;
#ifdef CONFIG_USERSPACE
	if (thread->arch.priv_stack_start != 0) {
		stack_bottom = thread->arch.priv_stack_start;
	} else if (z_stack_is_user_capable(thread->stack_obj)) {
		stack_bottom = thread->stack_info.start - K_THREAD_STACK_RESERVED;
	}
#endif
	set_pmp_entry(&index, PMP_NONE,
		      stack_bottom, Z_RISCV_STACK_GUARD_SIZE,
		      PMP_M_MODE(thread));

	/*
	 * We'll be using MPRV. Make a fallback entry with everything
	 * accessible as if no PMP entries were matched which is otherwise
	 * the default behavior for m-mode without MPRV.
	 */
	set_pmp_entry(&index, PMP_R | PMP_W | PMP_X,
		      0, 0, PMP_M_MODE(thread));
#ifdef CONFIG_QEMU_TARGET
	/*
	 * Workaround: The above produced 0x1fffffff which is correct.
	 * But there is a QEMU bug that prevents it from interpreting this
	 * value correctly. Hardcode the special case used by QEMU to
	 * bypass this bug for now. The QEMU fix is here:
	 * https://lists.gnu.org/archive/html/qemu-devel/2022-04/msg00961.html
	 */
	thread->arch.m_mode_pmpaddr_regs[index-1] = -1L;
#endif

	/* remember how many entries we use */
	thread->arch.m_mode_pmp_end_index = index;
}

/**
 * @brief Write PMP stackguard content to actual PMP registers
 *
 * This is called on every context switch.
 */
void z_riscv_pmp_stackguard_enable(struct k_thread *thread)
{
	LOG_DBG("pmp_stackguard_enable for thread %p", thread);

	/*
	 * Disable (non-locked) PMP entries for m-mode while we update them.
	 * While at it, also clear MSTATUS_MPP as it must be cleared for
	 * MSTATUS_MPRV to be effective later.
	 */
	csr_clear(mstatus, MSTATUS_MPRV | MSTATUS_MPP);

	/* Write our m-mode MPP entries */
	write_pmp_entries(global_pmp_end_index, thread->arch.m_mode_pmp_end_index,
			  false /* no need to clear to the end */,
			  PMP_M_MODE(thread));

	if (PMP_DEBUG_DUMP) {
		dump_pmp_regs("m-mode register dump");
	}

	/* Activate our non-locked PMP entries in m-mode */
	csr_set(mstatus, MSTATUS_MPRV);
}

#endif /* CONFIG_PMP_STACK_GUARD */

#ifdef CONFIG_USERSPACE

/**
 * @brief Initialize the usermode portion of the PMP configuration.
 *
 * This is called once during new thread creation.
 */
void z_riscv_pmp_usermode_init(struct k_thread *thread)
{
	/* Only indicate that the u-mode PMP is not prepared yet */
	thread->arch.u_mode_pmp_end_index = 0;
}

/**
 * @brief Prepare the u-mode PMP content for given thread.
 *
 * This is called once before making the transition to usermode.
 */
void z_riscv_pmp_usermode_prepare(struct k_thread *thread)
{
	unsigned int index = z_riscv_pmp_thread_init(PMP_U_MODE(thread));

	/* Map the usermode stack */
	set_pmp_entry(&index, PMP_R | PMP_W,
		      thread->stack_info.start, thread->stack_info.size,
		      PMP_U_MODE(thread));

	thread->arch.u_mode_pmp_domain_offset = index;
	thread->arch.u_mode_pmp_end_index = index;
	thread->arch.u_mode_pmp_update_nr = 0;
}

/**
 * @brief Convert partition information into PMP entries
 */
static void resync_pmp_domain(struct k_thread *thread,
			      struct k_mem_domain *domain)
{
	unsigned int index = thread->arch.u_mode_pmp_domain_offset;
	int p_idx, remaining_partitions;
	bool ok;

	k_spinlock_key_t key = k_spin_lock(&z_mem_domain_lock);

	remaining_partitions = domain->num_partitions;
	for (p_idx = 0; remaining_partitions > 0; p_idx++) {
		struct k_mem_partition *part = &domain->partitions[p_idx];

		if (part->size == 0) {
			/* skip empty partition */
			continue;
		}

		remaining_partitions--;

		if (part->size < 4) {
			/* * 4 bytes is the minimum we can map */
			LOG_ERR("non-empty partition too small");
			__ASSERT(false, "");
			continue;
		}

		ok = set_pmp_entry(&index, part->attr.pmp_attr,
				   part->start, part->size, PMP_U_MODE(thread));
		__ASSERT(ok,
			 "no PMP slot left for %d remaining partitions in domain %p",
			 remaining_partitions + 1, domain);
	}

	thread->arch.u_mode_pmp_end_index = index;
	thread->arch.u_mode_pmp_update_nr = domain->arch.pmp_update_nr;

	k_spin_unlock(&z_mem_domain_lock, key);
}

/**
 * @brief Write PMP usermode content to actual PMP registers
 *
 * This is called on every context switch.
 */
void z_riscv_pmp_usermode_enable(struct k_thread *thread)
{
	struct k_mem_domain *domain = thread->mem_domain_info.mem_domain;

	LOG_DBG("pmp_usermode_enable for thread %p with domain %p", thread, domain);

	if (thread->arch.u_mode_pmp_end_index == 0) {
		/* z_riscv_pmp_usermode_prepare() has not been called yet */
		return;
	}

	if (thread->arch.u_mode_pmp_update_nr != domain->arch.pmp_update_nr) {
		/*
		 * Resynchronize our PMP entries with
		 * the latest domain partition information.
		 */
		resync_pmp_domain(thread, domain);
	}

#ifdef CONFIG_PMP_STACK_GUARD
	/* Make sure m-mode PMP usage is disabled before we reprogram it */
	csr_clear(mstatus, MSTATUS_MPRV);
#endif

	/* Write our u-mode MPP entries */
	write_pmp_entries(global_pmp_end_index, thread->arch.u_mode_pmp_end_index,
			  true /* must clear to the end */,
			  PMP_U_MODE(thread));

	if (PMP_DEBUG_DUMP) {
		dump_pmp_regs("u-mode register dump");
	}
}

int arch_mem_domain_max_partitions_get(void)
{
	int available_pmp_slots = CONFIG_PMP_SLOTS;

	/* remove those slots dedicated to global entries */
	available_pmp_slots -= global_pmp_end_index;

	/* At least one slot to map the user thread's stack */
	available_pmp_slots -= 1;

	/*
	 * Each partition may require either 1 or 2 PMP slots depending
	 * on a couple factors that are not known in advance. Even when
	 * arch_mem_domain_partition_add() is called, we can't tell if a
	 * given partition will fit in the remaining PMP slots of an
	 * affected thread if it hasn't executed in usermode yet.
	 *
	 * Give the most optimistic answer here (which should be pretty
	 * accurate if CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT=y) and be
	 * prepared to deny availability in resync_pmp_domain() if this
	 * estimate was too high.
	 */
	return available_pmp_slots;
}

int arch_mem_domain_init(struct k_mem_domain *domain)
{
	domain->arch.pmp_update_nr = 0;
	return 0;
}

int arch_mem_domain_partition_add(struct k_mem_domain *domain,
				  uint32_t partition_id)
{
	/* Force resynchronization for every thread using this domain */
	domain->arch.pmp_update_nr += 1;
	return 0;
}

int arch_mem_domain_partition_remove(struct k_mem_domain *domain,
				     uint32_t partition_id)
{
	/* Force resynchronization for every thread using this domain */
	domain->arch.pmp_update_nr += 1;
	return 0;
}

int arch_mem_domain_thread_add(struct k_thread *thread)
{
	/* Force resynchronization for this thread */
	thread->arch.u_mode_pmp_update_nr = 0;
	return 0;
}

int arch_mem_domain_thread_remove(struct k_thread *thread)
{
	return 0;
}

#define IS_WITHIN(inner_start, inner_size, outer_start, outer_size) \
	((inner_start) >= (outer_start) && (inner_size) <= (outer_size) && \
	 ((inner_start) - (outer_start)) <= ((outer_size) - (inner_size)))

int arch_buffer_validate(void *addr, size_t size, int write)
{
	uintptr_t start = (uintptr_t)addr;
	int ret = -1;

	/* Check if this is on the stack */
	if (IS_WITHIN(start, size,
		      _current->stack_info.start, _current->stack_info.size)) {
		return 0;
	}

	/* Check if this is within the global read-only area */
	if (!write) {
		uintptr_t ro_start = (uintptr_t)__rom_region_start;
		size_t ro_size = (size_t)__rom_region_size;

		if (IS_WITHIN(start, size, ro_start, ro_size)) {
			return 0;
		}
	}

	/* Look for a matching partition in our memory domain */
	struct k_mem_domain *domain = _current->mem_domain_info.mem_domain;
	int p_idx, remaining_partitions;
	k_spinlock_key_t key = k_spin_lock(&z_mem_domain_lock);

	remaining_partitions = domain->num_partitions;
	for (p_idx = 0; remaining_partitions > 0; p_idx++) {
		struct k_mem_partition *part = &domain->partitions[p_idx];

		if (part->size == 0) {
			/* unused partition */
			continue;
		}

		remaining_partitions--;

		if (!IS_WITHIN(start, size, part->start, part->size)) {
			/* unmatched partition */
			continue;
		}

		/* partition matched: determine access result */
		if ((part->attr.pmp_attr & (write ? PMP_W : PMP_R)) != 0) {
			ret = 0;
		}
		break;
	}

	k_spin_unlock(&z_mem_domain_lock, key);
	return ret;
}

#endif /* CONFIG_USERSPACE */

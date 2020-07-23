/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 * Contributors: 2020 RISE Research Institutes of Sweden <www.ri.se>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <ksched.h>

void z_thread_entry_wrapper(k_thread_entry_t thread,
				void *arg1,
				void *arg2,
				void *arg3);

void arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack,
			 size_t stack_size, k_thread_entry_t thread_func,
			 void *arg1, void *arg2, void *arg3,
			 int priority, unsigned int options)
{
	char *stack_memory = Z_THREAD_STACK_BUFFER(stack);

	struct __esf *stack_init;

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
	const struct soc_esf soc_esf_init = {SOC_ESF_INIT};
#endif

	z_new_thread_init(thread, stack_memory, stack_size);

	/* Initial stack frame for thread */
	stack_init = (struct __esf *)
			 Z_STACK_PTR_ALIGN(stack_memory +
					   stack_size - sizeof(struct __esf));

	/* Setup the initial stack frame */
	stack_init->a0 = (ulong_t)thread_func;
	stack_init->a1 = (ulong_t)arg1;
	stack_init->a2 = (ulong_t)arg2;
	stack_init->a3 = (ulong_t)arg3;

	/*
	 * Following the RISC-V architecture,
	 * the MSTATUS register (used to globally enable/disable interrupt),
	 * as well as the MEPC register (used to by the core to save the
	 * value of the program counter at which an interrupt/exception occcurs)
	 * need to be saved on the stack, upon an interrupt/exception
	 * and restored prior to returning from the interrupt/exception.
	 * This shall allow to handle nested interrupts.
	 *
	 * Given that context switching is performed via a system call exception
	 * within the RISCV architecture implementation, initially set:
	 * 1) MSTATUS to MSTATUS_DEF_RESTORE in the thread stack to enable
	 *    interrupts when the newly created thread will be scheduled;
	 * 2) MEPC to the address of the z_thread_entry_wrapper in the thread
	 *	stack.
	 * Hence, when going out of an interrupt/exception/context-switch,
	 * after scheduling the newly created thread:
	 * 1) interrupts will be enabled, as the MSTATUS register will be
	 *    restored following the MSTATUS value set within the thread stack;
	 * 2) the core will jump to z_thread_entry_wrapper, as the program
	 *    counter will be restored following the MEPC value set within the
	 *    thread stack.
	 */
	stack_init->mstatus = MSTATUS_DEF_RESTORE;

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
	if ((thread->base.user_options & K_FP_REGS) != 0) {
		stack_init->mstatus |= MSTATUS_FS_INIT;
	}
	stack_init->fp_state = 0;
#endif

#if defined(CONFIG_USERSPACE)
	if (options & K_USER) {
		stack_init->mepc = (ulong_t)arch_user_mode_enter;
	} else {
		stack_init->mepc = (ulong_t)z_thread_entry_wrapper;
	}
#else
	stack_init->mepc = (ulong_t)z_thread_entry_wrapper;
#endif

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
	stack_init->soc_context = soc_esf_init;
#endif

	thread->callee_saved.sp = (ulong_t)stack_init;
}

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
int arch_float_disable(struct k_thread *thread)
{
	unsigned int key;

	if (thread != _current) {
		return -EINVAL;
	}

	if (arch_is_in_isr()) {
		return -EINVAL;
	}

	/* Ensure a preemptive context switch does not occur */
	key = irq_lock();

	/* Disable all floating point capabilities for the thread */
	thread->base.user_options &= ~K_FP_REGS;

	/* Clear the FS bits to disable the FPU. */
	__asm__ volatile (
		"mv t0, %0\n"
		"csrrc x0, mstatus, t0\n"
		:
		: "r" (MSTATUS_FS_MASK)
		);

	irq_unlock(key);

	return 0;
}

int arch_float_enable(struct k_thread *thread)
{
	unsigned int key;

	if (thread != _current) {
		return -EINVAL;
	}

	if (arch_is_in_isr()) {
		return -EINVAL;
	}

	/* Ensure a preemptive context switch does not occur */
	key = irq_lock();

	/* Enable all floating point capabilities for the thread. */
	thread->base.user_options |= K_FP_REGS;

	/* Set the FS bits to Initial to enable the FPU. */
	__asm__ volatile (
		"mv t0, %0\n"
		"csrrs x0, mstatus, t0\n"
		:
		: "r" (MSTATUS_FS_INIT)
		);

	irq_unlock(key);

	return 0;
}

#endif /* CONFIG_FPU && CONFIG_FPU_SHARING */

#ifdef CONFIG_USERSPACE

#include <offsets_short.h>
#include <linker/linker-defs.h>

/* Each 32-bit pmpcfg# register contains four 8-bit configuration sections.
 * These section numbers contain flags which apply to region defined by the
 * corresponding pmpaddr# register.
 *
 *    3                   2                   1
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |    pmp3cfg    |    pmp2cfg    |    pmp1cfg    |    pmp0cfg    | pmpcfg0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |    pmp7cfg    |    pmp6cfg    |    pmp5cfg    |    pmp4cfg    | pmpcfg2
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *     7       6       5       4       3       2       1       0
 * +-------+-------+-------+-------+-------+-------+-------+-------+
 * |   L   |       0       |       A       |   X   |   W   |   R   | pmp#cfg
 * +-------+-------+-------+-------+-------+-------+-------+-------+
 *
 *	  L: locks configuration until system reset (including M-mode)
 *	  0: hardwired to zero
 *	  A: 0 = OFF (null region / disabled)
 *	     1 = TOR (top of range)
 * 	     2 = NA4 (naturally aligned four-byte region)
 *	     3 = NAPOT (naturally aligned power-of-two region, > 7 bytes)
 *	  X: execute
 *	  W: write
 *	  R: read
 *
 * TOR: Each 32-bit pmpaddr# register defines the upper bound of the PMP region
 * right-shifted by two bits. The lower bound of the region is the previous
 * pmpaddr# register. In the case of pmpaddr0, the lower bound is address 0x0.
 *
 *    3                   2                   1
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                        address[33:2]                          | pmpaddr#
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * NAPOT: Each 32-bit pmpaddr# register defines the start address and the size
 * of the PMP region. The number of concurrent 1s begging at the LSB indicates
 * the size of the region as a power of two (e.g. 0x...0 = 8-byte, 0x...1 =
 * 16-byte, 0x...11 = 32-byte, etc.).
 *
 *    3                   2                   1
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                        address[33:2]                |0|1|1|1|1| pmpaddr#
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * NA4: This is essentially an edge case of NAPOT where the entire pmpaddr#
 * register defines a 4-byte wide region.
 */

/* Encode a region address and size into the RISC-V PMP NAPOT format. This macro
 * will round up the region size to the nearest power of two.
 */
#define NAPOT_CALC(addr, size) ((((uint32_t)addr) >> 2)) | \
	(0xffffffff >> (32 - (__builtin_ctz(POW2_CEIL(size) >> 3))))

/* Decode a region address and size from the RISC-V PMP NAPOT format. */
#define NAPOT_SIZE(pmpaddr) (1 << (__builtin_ctz(~pmpaddr) + 3))
#define NAPOT_ADDR(pmpaddr) ((pmpaddr >> __builtin_ctz(~pmpaddr)) \
		<< (__builtin_ctz(~pmpaddr) + 2))

extern void z_riscv_userspace_enter(void);

void z_riscv_pmp_configure(struct k_thread *thread)
{
	/* Userspace threads are configured by default to isolate .text,
	 * .rodata and their own stack. The remaining PMP registers are
	 * reserved for user-defined memory partitions.
	 *
	 *  +=========+ <--  0x0
	 *  |   ...   |
	 *  +---------+ <--  pmpaddr0
	 *  |  .text  |        [RX]
	 *  +---------+
	 *  |   ...   |
	 *  +---------+ <--  pmpaddr1
	 *  | .rodata |        [RO]
	 *  +---------+
	 *  |   ...   |
	 *  +---------+ <--  pmpaddr2
	 *  |  stack  |        [RW]
	 *  +---------+
	 *  |   ...   |
	 *  +=========+
	 */

	size_t pmpcfg[] = {0x0, 0x0};
	size_t pmpaddr[8];

	pmpcfg[0] = ((RV_PMP_NAPOT | RV_PMP_RX) << (RV_CFG_OFFSET * 0)) |
		    ((RV_PMP_NAPOT | RV_PMP_RO) << (RV_CFG_OFFSET * 1)) |
		    ((RV_PMP_NAPOT | RV_PMP_RW) << (RV_CFG_OFFSET * 2));

	pmpaddr[0] = NAPOT_CALC(_image_text_start,
			_image_text_end - _image_text_start);
	pmpaddr[1] = NAPOT_CALC(_image_rodata_start,
			_image_rodata_end - _image_rodata_start);
	pmpaddr[2] = NAPOT_CALC(_current->stack_info.start,
			_current->stack_info.size);

	struct k_mem_domain *mem_domain = thread->mem_domain_info.mem_domain;
	struct k_mem_partition partition;

	if (mem_domain) {
		int rv_pmp_idx;

		for (int i = 0; i < mem_domain->num_partitions; i++) {
			partition = mem_domain->partitions[i];
			if (partition.size == 0) {
				/* The partition does not exist. */
				continue;
			}

			/* pmpcfg0 has room for a single memory partition.
			 * pmpcfg1 can accommodate four.
			 */
			rv_pmp_idx = i + 3;
			pmpaddr[rv_pmp_idx] = NAPOT_CALC(partition.start,
					partition.size);
			pmpcfg[rv_pmp_idx/4] |= ((RV_PMP_NAPOT | partition.attr)
					<< ((rv_pmp_idx % 4) * RV_CFG_OFFSET));
		}

		__asm__ volatile ("csrw pmpcfg1, %0" :: "r" (pmpcfg[1]));
		__asm__ volatile ("csrw pmpaddr3, %0" :: "r" (pmpaddr[3]));
		__asm__ volatile ("csrw pmpaddr4, %0" :: "r" (pmpaddr[4]));
		__asm__ volatile ("csrw pmpaddr5, %0" :: "r" (pmpaddr[5]));
		__asm__ volatile ("csrw pmpaddr6, %0" :: "r" (pmpaddr[6]));
		__asm__ volatile ("csrw pmpaddr7, %0" :: "r" (pmpaddr[7]));
	}

	__asm__ volatile ("csrw pmpcfg0, %0" :: "r" (pmpcfg[0]));
	__asm__ volatile ("csrw pmpaddr0, %0" :: "r" (pmpaddr[0]));
	__asm__ volatile ("csrw pmpaddr1, %0" :: "r" (pmpaddr[1]));
	__asm__ volatile ("csrw pmpaddr2, %0" :: "r" (pmpaddr[2]));
}

int arch_buffer_validate(void *addr, size_t size, int write)
{
	size_t pmpcfg[2];
	size_t pmpaddr[8];

	uint8_t region_cfg[8];
	size_t region_size, region_addr, region_end;

	__asm__ volatile ("csrr %0, pmpcfg0" : "=r" (pmpcfg[0]));
	__asm__ volatile ("csrr %0, pmpcfg1" : "=r" (pmpcfg[1]));

	region_cfg[0] = (uint8_t)(pmpcfg[0] >> (RV_CFG_OFFSET * 0));
	region_cfg[1] = (uint8_t)(pmpcfg[0] >> (RV_CFG_OFFSET * 1));
	region_cfg[2] = (uint8_t)(pmpcfg[0] >> (RV_CFG_OFFSET * 2));
	region_cfg[3] = (uint8_t)(pmpcfg[0] >> (RV_CFG_OFFSET * 3));
	region_cfg[4] = (uint8_t)(pmpcfg[1] >> (RV_CFG_OFFSET * 0));
	region_cfg[5] = (uint8_t)(pmpcfg[1] >> (RV_CFG_OFFSET * 1));
	region_cfg[6] = (uint8_t)(pmpcfg[1] >> (RV_CFG_OFFSET * 2));
	region_cfg[7] = (uint8_t)(pmpcfg[1] >> (RV_CFG_OFFSET * 3));

	__asm__ volatile ("csrr %0, pmpaddr0" : "=r" (pmpaddr[0]));
	__asm__ volatile ("csrr %0, pmpaddr1" : "=r" (pmpaddr[1]));
	__asm__ volatile ("csrr %0, pmpaddr2" : "=r" (pmpaddr[2]));
	__asm__ volatile ("csrr %0, pmpaddr3" : "=r" (pmpaddr[3]));
	__asm__ volatile ("csrr %0, pmpaddr4" : "=r" (pmpaddr[4]));
	__asm__ volatile ("csrr %0, pmpaddr5" : "=r" (pmpaddr[5]));
	__asm__ volatile ("csrr %0, pmpaddr6" : "=r" (pmpaddr[6]));
	__asm__ volatile ("csrr %0, pmpaddr7" : "=r" (pmpaddr[7]));

	for (int i = 0; i < 8; i++) {
		if (region_cfg[i] & RV_PMP_NAPOT) {
			region_size = NAPOT_SIZE(pmpaddr[i]);
			region_addr = NAPOT_ADDR(pmpaddr[i]);
			region_end = region_addr + region_size;
			if ((((size_t)addr + size) < region_end) &&
					((size_t) addr >= region_addr) &&
					(region_cfg[i] & RV_PMP_RO)) {
				if (write) {
					if (region_cfg[i] & RV_PMP_W) {
						return 0;
					} else {
						return 1;
					}
				} else {
					return 0;
				}
			}
		}
	}

	return 1;
}

int arch_mem_domain_max_partitions_get(void)
{
	return 8;
}

/* These arch_mem_domain_## functions are all no-ops on RISC-V, because the PMP
 * registers are reinitialized with z_riscv_pmp_configure on every reschedule.
 * If the kernel modifies a user-mode thread's k_mem_domain struct, those 
 * changes are applied automatically when that thread is rescheduled. 
 */
void arch_mem_domain_partition_add(struct k_mem_domain *domain,
		uint32_t partition_id)
{
	/* No-op on this architecture. */
}

void arch_mem_domain_partition_remove(struct k_mem_domain *domain,
		uint32_t partition_id)
{
	/* No-op on this architecture. */
}

void arch_mem_domain_thread_add(struct k_thread *thread)
{
	/* No-op on this architecture. */
}

void arch_mem_domain_thread_remove(struct k_thread *thread)
{
	/* No-op on this architecture. */
}

void arch_mem_domain_destroy(struct k_mem_domain *domain)
{
	/* No-op on this architecture. */
}

FUNC_NORETURN void arch_user_mode_enter(k_thread_entry_t user_entry,
		void *p1, void *p2, void *p3)
{
	z_riscv_pmp_configure(k_current_get());
	z_riscv_userspace_enter();
	z_thread_entry_wrapper(user_entry, p1, p2, p3);

	CODE_UNREACHABLE;
}

#endif /* CONFIG_USERSPACE */

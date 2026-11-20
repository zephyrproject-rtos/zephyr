/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Faults in a thread, three calls deep, for each fault type and from kernel
 * and user mode. The pytest script checks that GDB unwinds from the fault
 * handlers back to the thread entry, and that the fault dump shows the SP
 * of the faulting instruction.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/app_memory/app_memdomain.h>

enum fault_kind {
	FAULT_UNDEF,
	FAULT_PREFETCH,
	FAULT_DATA_WRITE,
	FAULT_DATA_PERM,
	FAULT_FPU_TRAP,
};

static const char *const fault_names[] = {
	[FAULT_UNDEF] = "undef",
	[FAULT_PREFETCH] = "prefetch",
	[FAULT_DATA_WRITE] = "data_write",
	[FAULT_DATA_PERM] = "data_perm",
	[FAULT_FPU_TRAP] = "fpu_trap",
};

#if defined(CONFIG_USERSPACE)
K_APPMEM_PARTITION_DEFINE(part);
#define APP_DATA K_APP_DMEM(part)
#else
#define APP_DATA
#endif

/*
 * The SP of the faulting instruction, a word user mode cannot read, and one
 * that no thread can write
 */
APP_DATA volatile uint32_t fault_sp;
APP_DATA volatile uint32_t depth;
static uint32_t kernel_only;
static const uint32_t read_only;

/* Record SP in fault_sp, then fault, both in one asm statement */
static void __noinline fault_here(enum fault_kind kind)
{
	uint32_t tmp;

	switch (kind) {
	case FAULT_UNDEF:
		__asm__ volatile("mov %0, sp\n\t"
				 "str %0, [%1]\n\t"
				 "udf #0"
				 : "=&r"(tmp) : "r"(&fault_sp) : "memory");
		break;
	case FAULT_PREFETCH:
		/* BKPT raises a prefetch abort when no debugger halts on it */
		__asm__ volatile("mov %0, sp\n\t"
				 "str %0, [%1]\n\t"
				 "bkpt #0"
				 : "=&r"(tmp) : "r"(&fault_sp) : "memory");
		break;
	case FAULT_DATA_WRITE:
		/* A data abort for writing read-only memory */
		__asm__ volatile("mov %0, sp\n\t"
				 "str %0, [%1]\n\t"
				 "str %0, [%2]"
				 : "=&r"(tmp) : "r"(&fault_sp), "r"(&read_only)
				 : "memory");
		break;
	case FAULT_DATA_PERM:
		/* A data abort for a user thread reading kernel memory */
		__asm__ volatile("mov %0, sp\n\t"
				 "str %0, [%1]\n\t"
				 "ldr %0, [%2]"
				 : "=&r"(tmp) : "r"(&fault_sp), "r"(&kernel_only)
				 : "memory");
		break;
#if defined(CONFIG_FPU_SHARING)
	case FAULT_FPU_TRAP:
		/*
		 * Not a fault: the first VFP instruction of the thread traps
		 * into z_arm_fpu_trap(), which enables the FPU and returns.
		 */
		__asm__ volatile("mov %0, sp\n\t"
				 "str %0, [%1]\n\t"
				 "vmrs %0, fpscr"
				 : "=&r"(tmp) : "r"(&fault_sp) : "memory");
		break;
#endif
	default:
		break;
	}
}

/* depth keeps the calls from being turned into tail calls */
static void __noinline fault_level2(enum fault_kind kind)
{
	fault_here(kind);
	depth++;
}

static void __noinline fault_level1(enum fault_kind kind)
{
	fault_level2(kind);
	depth++;
}

static void fault_entry(void *p1, void *p2, void *p3)
{
	fault_level1((enum fault_kind)(uintptr_t)p1);
	depth++;
}

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *esf)
{
	ARG_UNUSED(esf);

	/* Returning aborts the faulting thread */
	printk("fault reason %u, fault_sp 0x%08x\n", reason, fault_sp);
}

#define STACK_SIZE 2048
K_THREAD_STACK_DEFINE(fault_stack, STACK_SIZE);
static struct k_thread fault_thread;

/* GDB stops here once all the faults have been taken */
void __noinline test_done(void)
{
	printk("DONE\n");
}

static void run(enum fault_kind kind, uint32_t options)
{
	printk("case %s %s\n", fault_names[kind], ((options & K_USER) != 0U) ? "user" : "kernel");
	k_thread_create(&fault_thread, fault_stack, STACK_SIZE, fault_entry,
			(void *)(uintptr_t)kind, NULL, NULL, K_PRIO_PREEMPT(1), options,
			K_FOREVER);
#if defined(CONFIG_USERSPACE)
	if ((options & K_USER) != 0U) {
		k_mem_domain_add_thread(&k_mem_domain_default, &fault_thread);
	}
#endif
	k_thread_start(&fault_thread);
	k_thread_join(&fault_thread, K_FOREVER);
}

int main(void)
{
#if defined(CONFIG_USERSPACE)
	k_mem_domain_add_partition(&k_mem_domain_default, &part);
#endif
	run(FAULT_UNDEF, 0);
	run(FAULT_PREFETCH, 0);
	run(FAULT_DATA_WRITE, 0);
#if defined(CONFIG_FPU_SHARING)
	run(FAULT_FPU_TRAP, 0);
#endif
#if defined(CONFIG_USERSPACE)
	run(FAULT_UNDEF, K_USER);
	run(FAULT_PREFETCH, K_USER);
	run(FAULT_DATA_WRITE, K_USER);
	run(FAULT_DATA_PERM, K_USER);
#endif
	test_done();
	return 0;
}

/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <init.h>
#include <kernel.h>
#include <kernel_structs.h>
#include <toolchain.h>
#include <sys/__assert.h>
#include <sys/sys_io.h>

#include <xtensa/config/core-isa.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(soc_mp, CONFIG_SOC_LOG_LEVEL);

#include <soc.h>
#include <adsp/io.h>

#include <soc/shim.h>

#include <drivers/ipm.h>
#include <ipm/ipm_cavs_idc.h>

#if CONFIG_MP_NUM_CPUS > 1 && !defined(CONFIG_IPM_CAVS_IDC) && defined(CONFIG_SMP)
#error Need to enable the IPM driver for multiprocessing
#endif

/* ROM wake version parsed by ROM during core wake up. */
#define IDC_ROM_WAKE_VERSION	0x2

/* IDC message type. */
#define IDC_TYPE_SHIFT		24
#define IDC_TYPE_MASK		0x7f
#define IDC_TYPE(x)		(((x) & IDC_TYPE_MASK) << IDC_TYPE_SHIFT)

/* IDC message header. */
#define IDC_HEADER_MASK		0xffffff
#define IDC_HEADER(x)		((x) & IDC_HEADER_MASK)

/* IDC message extension. */
#define IDC_EXTENSION_MASK	0x3fffffff
#define IDC_EXTENSION(x)	((x) & IDC_EXTENSION_MASK)

/* IDC power up message. */
#define IDC_MSG_POWER_UP	\
	(IDC_TYPE(0x1) | IDC_HEADER(IDC_ROM_WAKE_VERSION))

#define IDC_MSG_POWER_UP_EXT(x)	IDC_EXTENSION((x) >> 2)

#ifdef CONFIG_IPM_CAVS_IDC
static const struct device *idc;
#endif

extern void __start(void);

struct cpustart_rec {
	uint32_t		cpu;

	arch_cpustart_t	fn;
	char		*stack_top;
	void		*arg;
	uint32_t		vecbase;

	uint32_t		alive;

	/* padding to cache line */
	uint8_t		padding[XCHAL_DCACHE_LINESIZE - 6 * 4];
};

static __aligned(XCHAL_DCACHE_LINESIZE)
struct cpustart_rec start_rec;

static void *mp_top;

static void mp_entry2(void)
{
	volatile int ie;
	uint32_t idc_reg;

	/* We don't know what the boot ROM might have touched and we
	 * don't care.  Make sure it's not in our local cache.
	 */
	xthal_dcache_all_writeback_inv();

	/* Copy over VECBASE from the main CPU for an initial value
	 * (will need to revisit this if we ever allow a user API to
	 * change interrupt vectors at runtime).
	 */
	ie = 0;
	__asm__ volatile("wsr.INTENABLE %0" : : "r"(ie));
	__asm__ volatile("wsr.VECBASE %0" : : "r"(start_rec.vecbase));
	__asm__ volatile("rsync");

	/* Set up the CPU pointer. */
	_cpu_t *cpu = &_kernel.cpus[start_rec.cpu];

	__asm__ volatile(
		"wsr." CONFIG_XTENSA_KERNEL_CPU_PTR_SR " %0" : : "r"(cpu));

	/* Clear busy bit set by power up message */
	idc_reg = idc_read(IPC_IDCTFC(0), start_rec.cpu) | IPC_IDCTFC_BUSY;
	idc_write(IPC_IDCTFC(0), start_rec.cpu, idc_reg);

#ifdef CONFIG_IPM_CAVS_IDC
	/* Interrupt must be enabled while running on current core */
	irq_enable(XTENSA_IRQ_NUMBER(DT_IRQN(DT_INST(0, intel_cavs_idc))));
#endif /* CONFIG_IPM_CAVS_IDC */

	start_rec.alive = 1;
	SOC_DCACHE_FLUSH(&start_rec, sizeof(start_rec));

	start_rec.fn(start_rec.arg);

#if CONFIG_MP_NUM_CPUS == 1
	/* CPU#1 can be under manual control running custom functions
	 * instead of participating in general thread execution.
	 * Put the CPU into idle after those functions return
	 * so this won't return.
	 */
	for (;;) {
		k_cpu_idle();
	}
#endif
}

/* Defines a locally callable "function" named mp_stack_switch().  The
 * first argument (in register a2 post-ENTRY) is the new stack pointer
 * to go into register a1.  The second (a3) is the entry point.
 * Because this never returns, a0 is used as a scratch register then
 * set to zero for the called function (a null return value is the
 * signal for "top of stack" to the debugger).
 */
void mp_stack_switch(void *stack, void *entry);
__asm__("\n"
	".align 4		\n"
	"mp_stack_switch:	\n\t"

	"entry a1, 16		\n\t"

	"movi a0, 0		\n\t"

	"jx a3			\n\t");

/* Carefully constructed to use no stack beyond compiler-generated ABI
 * instructions. Stack pointer is pointing to __stack at this point.
 */
void z_mp_entry(void)
{
	*(uint32_t *)CONFIG_SRAM_BASE_ADDRESS = 0xDEADBEEF;
	SOC_DCACHE_FLUSH((uint32_t *)CONFIG_SRAM_BASE_ADDRESS, 64);

	mp_stack_switch(mp_top, mp_entry2);
}

void arch_start_cpu(int cpu_num, k_thread_stack_t *stack, int sz,
		    arch_cpustart_t fn, void *arg)
{
	uint32_t vecbase;
	uint32_t idc_reg;

	__ASSERT(cpu_num == 1, "Only supports only two CPUs!");

	/* Setup data to boot core #1 */
	__asm__ volatile("rsr.VECBASE %0\n\t" : "=r"(vecbase));

	start_rec.cpu = cpu_num;
	start_rec.fn = fn;
	start_rec.stack_top = Z_THREAD_STACK_BUFFER(stack) + sz;
	start_rec.arg = arg;
	start_rec.vecbase = vecbase;
	start_rec.alive = 0;

	mp_top = Z_THREAD_STACK_BUFFER(stack) + sz;

	SOC_DCACHE_FLUSH(&start_rec, sizeof(start_rec));

#ifdef CONFIG_IPM_CAVS_IDC
	idc = device_get_binding(DT_LABEL(DT_INST(0, intel_cavs_idc)));
#endif

	/* Enable IDC interrupt on the other core */
	idc_reg = idc_read(IPC_IDCCTL, cpu_num);
	idc_reg |= IPC_IDCCTL_IDCTBIE(0);
	idc_write(IPC_IDCCTL, cpu_num, idc_reg);
	sys_set_bit(DT_REG_ADDR(DT_NODELABEL(cavs0)) + 0x04 +
		    CAVS_ICTL_INT_CPU_OFFSET(cpu_num), 8);

	/* Send power up message to the other core */
	idc_write(IPC_IDCIETC(cpu_num), 0, IDC_MSG_POWER_UP_EXT(RAM_BASE));
	idc_write(IPC_IDCITC(cpu_num), 0, IDC_MSG_POWER_UP | IPC_IDCITC_BUSY);

	/* Disable IDC interrupt on other core so IPI won't cause
	 * them to jump to ISR until the core is fully initialized.
	 */
	idc_reg = idc_read(IPC_IDCCTL, cpu_num);
	idc_reg &= ~IPC_IDCCTL_IDCTBIE(0);
	idc_write(IPC_IDCCTL, cpu_num, idc_reg);
	sys_clear_bit(DT_REG_ADDR(DT_NODELABEL(cavs0)) + 0x04 +
		      CAVS_ICTL_INT_CPU_OFFSET(cpu_num), 8);

	do {
		SOC_DCACHE_INVALIDATE(&start_rec, sizeof(start_rec));
	} while (start_rec.alive == 0);

	/* Clear done bit from responding the power up message */
	idc_reg = idc_read(IPC_IDCIETC(cpu_num), 0) | IPC_IDCIETC_DONE;
	idc_write(IPC_IDCIETC(cpu_num), 0, idc_reg);
}

#ifdef CONFIG_SCHED_IPI_SUPPORTED
FUNC_ALIAS(soc_sched_ipi, arch_sched_ipi, void);
void soc_sched_ipi(void)
{
	if (idc != NULL) {
		ipm_send(idc, 0, IPM_CAVS_IDC_MSG_SCHED_IPI_ID,
			 IPM_CAVS_IDC_MSG_SCHED_IPI_DATA, 0);
	}
}
#endif

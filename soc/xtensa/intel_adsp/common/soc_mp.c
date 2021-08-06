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
#include <arch/xtensa/cache.h>
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

struct cpustart_rec {
	uint32_t		cpu;

	arch_cpustart_t	fn;
	void		*arg;
	uint32_t		vecbase;

	uint32_t		alive;
};

char *z_mp_stack_top;

#ifdef CONFIG_KERNEL_COHERENCE
/* Coherence guarantees that normal .data will be coherent and that it
 * won't overlap any cached memory.
 */
static struct {
	struct cpustart_rec cpustart;
} cpustart_mem;
#else
/* If .data RAM is by default incoherent, then the start record goes
 * into its own dedicated cache line(s)
 */
static __aligned(XCHAL_DCACHE_LINESIZE) union {
	struct cpustart_rec cpustart;
	char pad[XCHAL_DCACHE_LINESIZE];
} cpustart_mem;
#endif

#define start_rec \
	(*((volatile struct cpustart_rec *) \
	   z_soc_uncached_ptr(&cpustart_mem.cpustart)))

static uint32_t cpu_mask;

/* Tiny assembly stub for calling z_mp_entry() on the auxiliary CPUs.
 * Mask interrupts, clear the register window state and set the stack
 * pointer.  This represents the minimum work required to run C code
 * safely.
 *
 * Note that alignment is absolutely required: the IDC protocol passes
 * only the upper 30 bits of the address to the second CPU.
 */
void z_soc_mp_asm_entry(void);
__asm__(".align 4                   \n\t"
	".global z_soc_mp_asm_entry \n\t"
	"z_soc_mp_asm_entry:        \n\t"
	"  movi  a0, 0x40025        \n\t" /* WOE | UM | INTLEVEL(5) */
	"  wsr   a0, PS             \n\t"
	"  movi  a0, 0              \n\t"
	"  wsr   a0, WINDOWBASE     \n\t"
	"  movi  a0, 1              \n\t"
	"  wsr   a0, WINDOWSTART    \n\t"
	"  rsync                    \n\t"
	"  movi  a1, z_mp_stack_top \n\t"
	"  l32i  a1, a1, 0          \n\t"
	"  call4 z_mp_entry         \n\t");
BUILD_ASSERT(XCHAL_EXCM_LEVEL == 5);

int cavs_idc_smp_init(const struct device *dev);

#define CxL1CCAP (*(volatile uint32_t *)0x9F080080)
#define CxL1CCFG (*(volatile uint32_t *)0x9F080084)
#define CxL1PCFG (*(volatile uint32_t *)0x9F080088)

/* "Data/Instruction Cache Memory Way Count" fields */
#define CxL1CCAP_DCMWC ((CxL1CCAP >> 16) & 7)
#define CxL1CCAP_ICMWC ((CxL1CCAP >> 20) & 7)

static ALWAYS_INLINE void enable_l1_cache(void)
{
	uint32_t reg;

#ifdef CONFIG_SOC_SERIES_INTEL_CAVS_V25
	/* First, on cAVS 2.5 we need to power the cache SRAM banks
	 * on!  Write a bit for each cache way in the bottom half of
	 * the L1CCFG register and poll the top half for them to turn
	 * on.
	 */
	uint32_t dmask = BIT(CxL1CCAP_DCMWC) - 1;
	uint32_t imask = BIT(CxL1CCAP_ICMWC) - 1;
	uint32_t waymask = (imask << 8) | dmask;

	CxL1CCFG = waymask;
	while (((CxL1CCFG >> 16) & waymask) != waymask) {
	}

	/* Prefetcher also power gates, same interface */
	CxL1PCFG = 1;
	while ((CxL1PCFG & 0x10000) == 0) {
	}
#endif

	/* Now set up the Xtensa CPU to enable the cache logic.  The
	 * details of the fields are somewhat complicated, but per the
	 * ISA ref: "Turning on caches at power-up usually consists of
	 * writing a constant with bits[31:8] all 1’s to MEMCTL.".
	 * Also set bit 0 to enable the LOOP extension instruction
	 * fetch buffer.
	 */
	reg = 0xffffff01;
	__asm__ volatile("wsr %0, MEMCTL; rsync" :: "r"(reg));

	/* Likewise enable prefetching.  Sadly these values are not
	 * architecturally defined by Xtensa (they're just documented
	 * as priority hints), so this constant is just copied from
	 * SOF for now.  If we care about prefetch priority tuning
	 * we're supposed to ask Cadence I guess.
	 */
	reg = IS_ENABLED(CONFIG_SOC_SERIES_INTEL_CAVS_V25) ? 0x1038 : 0;
	__asm__ volatile("wsr %0, PREFCTL; rsync" :: "r"(reg));

	/* Finally we need to enable the cache in the Region
	 * Protection Option "TLB" entries.  The hardware defaults
	 * have this set to RW/uncached (2) everywhere.  We want
	 * writeback caching (4) in the sixth mapping (the second of
	 * two RAM mappings) and to mark all unused regions
	 * inaccessible (15) for safety.  Note that there is a HAL
	 * routine that does this (by emulating the older "cacheattr"
	 * hardware register), but it generates significantly larger
	 * code.
	 */
	const uint8_t attribs[] = { 2, 15, 15, 15, 2, 4, 15, 15 };

	for (int region = 0; region < 8; region++) {
		reg = 0x20000000 * region;
		__asm__ volatile("wdtlb %0, %1" :: "r"(attribs[region]), "r"(reg));
	}
}

void z_mp_entry(void)
{
	volatile int ie;
	uint32_t idc_reg, reg;

	enable_l1_cache();

	/* Fix ATOMCTL to match CPU0.  Hardware defaults for S32C1I
	 * use internal operations (and are thus presumably atomic
	 * only WRT the local CPU!).  We need external transactions on
	 * the shared bus.
	 */
	reg = 0x15;
	__asm__ volatile("wsr %0, ATOMCTL" :: "r"(reg));

	/* We don't know what the boot ROM (on pre-2.5 DSPs) might
	 * have touched and we don't care.  Make sure it's not in our
	 * local cache to be flushed accidentally later.
	 *
	 * Note that technically this is dropping our own (cached)
	 * stack memory, which we don't have a guarantee the compiler
	 * isn't using yet.  Manual inspection of generated code says
	 * we're safe, but really we need a better solution here.
	 */
#ifndef CONFIG_SOC_SERIES_INTEL_CAVS_V25
	z_xtensa_cache_flush_inv_all();
#endif

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
	irq_enable(DT_IRQN(DT_INST(0, intel_cavs_idc)));
#endif /* CONFIG_IPM_CAVS_IDC */

#ifdef CONFIG_SMP_BOOT_DELAY
	cavs_idc_smp_init(NULL);
#endif

	start_rec.alive = 1;

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

bool arch_cpu_active(int cpu_num)
{
	return !!(cpu_mask & BIT(cpu_num));
}

void arch_start_cpu(int cpu_num, k_thread_stack_t *stack, int sz,
		    arch_cpustart_t fn, void *arg)
{
	uint32_t vecbase;
	uint32_t idc_reg;

#ifdef CONFIG_SOC_SERIES_INTEL_CAVS_V25
	/* On cAVS v2.5, MP startup works differently.  The core has
	 * no ROM, and starts running immediately upon receipt of an
	 * IDC interrupt at the start of LPSRAM at 0xbe800000.  Note
	 * that means we don't need to bother constructing a "message"
	 * below, it will be ignored.  But it's left in place for
	 * simplicity and compatibility.
	 *
	 * All we need to do is place a single jump at that address to
	 * our existing MP entry point.  Unfortunately Xtensa makes
	 * this difficult, as the region is beyond the range of a
	 * relative jump instruction, so we need an immediate, which
	 * can only be backwards-referenced.  So we hand-assemble a
	 * tiny trampoline here ("jump over the immediate address,
	 * load it, jump to it").
	 *
	 * Long term we want to have this in linkable LP-SRAM memory
	 * such that the standard system bootstrap out of IMR can
	 * place it there.  But this is fine for now.
	 */
	void **lpsram = z_soc_uncached_ptr((void *)LP_SRAM_BASE);
	uint8_t tramp[] = {
		0x06, 0x01, 0x00, /* J <PC+8>  (jump to L32R) */
		0,                /* (padding to align entry_addr) */
		0, 0, 0, 0,       /* (entry_addr goes here) */
		0x01, 0xff, 0xff, /* L32R a0, <entry_addr> */
		0xa0, 0x00, 0x00, /* JX a0 */
	};

	memcpy(lpsram, tramp, ARRAY_SIZE(tramp));
	lpsram[1] = z_soc_mp_asm_entry;
#endif

	__asm__ volatile("rsr.VECBASE %0\n\t" : "=r"(vecbase));

	start_rec.cpu = cpu_num;
	start_rec.fn = fn;
	start_rec.arg = arg;
	start_rec.vecbase = vecbase;
	start_rec.alive = 0;

	z_mp_stack_top = Z_THREAD_STACK_BUFFER(stack) + sz;

#ifdef CONFIG_IPM_CAVS_IDC
	idc = device_get_binding(DT_LABEL(DT_INST(0, intel_cavs_idc)));
#endif

	/* Enable IDC interrupt on the other core */
	idc_reg = idc_read(IPC_IDCCTL, cpu_num);
	idc_reg |= IPC_IDCCTL_IDCTBIE(0);
	idc_write(IPC_IDCCTL, cpu_num, idc_reg);
	/* FIXME: 8 is IRQ_BIT_LVL2_IDC / PLATFORM_IDC_INTERRUPT */
	sys_set_bit(DT_REG_ADDR(DT_NODELABEL(cavs0)) + 0x04 +
		    CAVS_ICTL_INT_CPU_OFFSET(cpu_num), 8);

	/* Send power up message to the other core */
	uint32_t ietc = IDC_MSG_POWER_UP_EXT((long) z_soc_mp_asm_entry);

	idc_write(IPC_IDCIETC(cpu_num), 0, ietc);
	idc_write(IPC_IDCITC(cpu_num), 0, IDC_MSG_POWER_UP | IPC_IDCITC_BUSY);

	/* Disable IDC interrupt on other core so IPI won't cause
	 * them to jump to ISR until the core is fully initialized.
	 */
	idc_reg = idc_read(IPC_IDCCTL, cpu_num);
	idc_reg &= ~IPC_IDCCTL_IDCTBIE(0);
	idc_write(IPC_IDCCTL, cpu_num, idc_reg);
	sys_set_bit(DT_REG_ADDR(DT_NODELABEL(cavs0)) + 0x00 +
		      CAVS_ICTL_INT_CPU_OFFSET(cpu_num), 8);

	k_busy_wait(100);

#ifdef CONFIG_SMP_BOOT_DELAY
	cavs_idc_smp_init(NULL);
#endif

	/* Clear done bit from responding the power up message */
	idc_reg = idc_read(IPC_IDCIETC(cpu_num), 0) | IPC_IDCIETC_DONE;
	idc_write(IPC_IDCIETC(cpu_num), 0, idc_reg);

	while (!start_rec.alive)
		;

	/*
	 * No locking needed as long as CPUs can only be powered on by the main
	 * CPU and cannot be powered off
	 */
	cpu_mask |= BIT(cpu_num);
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

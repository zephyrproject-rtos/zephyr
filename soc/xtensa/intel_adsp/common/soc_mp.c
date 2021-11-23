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

#include <cavs-idc.h>
#include <soc.h>
#include <arch/xtensa/cache.h>
#include <adsp/io.h>
#include <cavs-shim.h>
#include <cavs-mem.h>

#ifdef CONFIG_IPM_CAVS_IDC
#include <drivers/ipm.h>
#include <ipm/ipm_cavs_idc.h>
#endif

extern void z_sched_ipi(void);
extern void z_smp_start_cpu(int id);
extern void z_reinit_idle_thread(int i);

/* IDC power up message to the ROM firmware.  This isn't documented
 * anywhere, it's basically just a magic number (except the high bit,
 * which signals the hardware)
 */
#define IDC_MSG_POWER_UP				  \
	(BIT(31) |     /* Latch interrupt in ITC write */ \
	 (0x1 << 24) | /* "ROM control version" = 1 */	  \
	 (0x2 << 0))   /* "Core wake version" = 2 */

#define IDC_ALL_CORES (BIT(CONFIG_MP_NUM_CPUS) - 1)

struct cpustart_rec {
	uint32_t        cpu;
	arch_cpustart_t	fn;
	void            *arg;
	uint32_t        vecbase;
};

static struct cpustart_rec start_rec;

static struct k_spinlock mplock;

char *z_mp_stack_top;

/* Vestigial silliness: An old mechanism for core startup would embed
 * a "manifest" of code to copy to LP-SRAM at startup (vs. the tiny
 * trampoline we use here).  This was constructed in the linker
 * script, and the first word would encode the number of entries.  As
 * it happens, SOF still emits the code to copy this data, so it needs
 * to see this symbol point to a zero.
 */
uint32_t _loader_storage_manifest_start;

/* Simple array of CPUs that are active and available for an IPI.  The
 * IDC interrupt is ALSO used to bring a CPU out of reset, so we need
 * to be absolutely sure we don't try to IPI a CPU that isn't ready to
 * start, or else we'll launch it into garbage and crash the DSP.
 */
static bool cpus_active[CONFIG_MP_NUM_CPUS];

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
#ifdef XCHAL_HAVE_ICACHE_DYN_ENABLE
	reg = 0xffffff01;
	__asm__ volatile("wsr %0, MEMCTL; rsync" :: "r"(reg));
#endif

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
#ifdef CONFIG_SOC_SERIES_INTEL_CAVS_V25
	/* Already set up by the ROM on older hardware. */
	const uint8_t attribs[] = { 2, 15, 15, 15, 2, 4, 15, 15 };

	for (int region = 0; region < 8; region++) {
		reg = 0x20000000 * region;
		__asm__ volatile("wdtlb %0, %1" :: "r"(attribs[region]), "r"(reg));
	}
#endif
}

void z_mp_entry(void)
{
	uint32_t reg;

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
	if (!IS_ENABLED(CONFIG_SOC_SERIES_INTEL_CAVS_V25)) {
		z_xtensa_cache_flush_inv_all();
	}

	/* Copy over VECBASE from the main CPU for an initial value
	 * (will need to revisit this if we ever allow a user API to
	 * change interrupt vectors at runtime).
	 */
	reg = 0;
	__asm__ volatile("wsr %0, INTENABLE" : : "r"(reg));
	__asm__ volatile("wsr %0, VECBASE" : : "r"(start_rec.vecbase));
	__asm__ volatile("rsync");

	/* Set up the CPU pointer. */
	_cpu_t *cpu = &_kernel.cpus[start_rec.cpu];

	__asm__ volatile(
		"wsr." CONFIG_XTENSA_KERNEL_CPU_PTR_SR " %0" : : "r"(cpu));

	/* We got here via an IDC interrupt.  Clear the TFC high bit
	 * (by writing a one!) to acknowledge and clear the latched
	 * hardware interrupt (so we don't have to service it as a
	 * spurious IPI when we enter user code).  Remember: this
	 * could have come from any core, clear all of them.
	 */
	for (int i = 0; i < CONFIG_MP_NUM_CPUS; i++) {
		IDC[start_rec.cpu].core[i].tfc = BIT(31);
	}

	/* Interrupt must be enabled while running on current core */
	irq_enable(DT_IRQN(DT_INST(0, intel_cavs_idc)));

#ifdef CONFIG_IPM_CAVS_IDC
	if (IS_ENABLED(CONFIG_SMP_BOOT_DELAY)) {
		cavs_idc_smp_init(NULL);
	}
#else
	/* Unfortunately the interrupt controller doesn't understand
	 * that each CPU has its own mask register (the timer has a
	 * similar hook).  Needed only on hardware with ROMs that
	 * disable this; cAVS 2.5 starts with an unmasked hardware
	 * default.
	 */
	if (!IS_ENABLED(CONFIG_SOC_SERIES_INTEL_CAVS_V25)) {
		CAVS_INTCTRL[start_rec.cpu].l2.clear = CAVS_L2_IDC;
	}

	/* Unmask IDC interrupts from this core to all others.  A
	 * delay is needed following the write on older hardware, or
	 * else the modification gets lost.  Voodoo.
	 */
	if (IS_ENABLED(CONFIG_SOC_SERIES_INTEL_CAVS_V15)) {
		k_busy_wait(10);
	}
	IDC[start_rec.cpu].busy_int = IDC_ALL_CORES;
#endif

	cpus_active[start_rec.cpu] = true;

	start_rec.fn(start_rec.arg);
	__ASSERT(false, "arch_start_cpu() handler should never return");
}

bool arch_cpu_active(int cpu_num)
{
	return cpus_active[cpu_num];
}

static ALWAYS_INLINE uint32_t prid(void)
{
	uint32_t prid;

	__asm__ volatile("rsr %0, PRID" : "=r"(prid));
	return prid;
}

void arch_start_cpu(int cpu_num, k_thread_stack_t *stack, int sz,
		    arch_cpustart_t fn, void *arg)
{
	uint32_t vecbase, curr_cpu = prid();

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

	__asm__ volatile("rsr %0, VECBASE\n\t" : "=r"(vecbase));

	start_rec.cpu = cpu_num;
	start_rec.fn = fn;
	start_rec.arg = arg;
	start_rec.vecbase = vecbase;

	z_mp_stack_top = Z_THREAD_STACK_BUFFER(stack) + sz;

	/* Pre-2.x cAVS delivers the IDC to ROM code, so unmask it */
	CAVS_INTCTRL[cpu_num].l2.clear = CAVS_L2_IDC;

	/* Disable automatic power and clock gating for that CPU, so
	 * it won't just go back to sleep.  Note that after startup,
	 * the cores are NOT power gated even if they're configured to
	 * be, so by default a core will launch successfully but then
	 * turn itself off when it gets to the WAITI instruction in
	 * the idle thread.
	 */
	CAVS_SHIM.pwrctl |= BIT(cpu_num);
	if (!IS_ENABLED(CONFIG_SOC_SERIES_INTEL_CAVS_V15)) {
		CAVS_SHIM.clkctl |= BIT(16 + cpu_num);
	}

	/* Send power-up message to the other core.  Start address
	 * gets passed via the IETC scratch register (only 30 bits
	 * available, so it's sent shifted).  The write to ITC
	 * triggers the interrupt, so that comes last.
	 */
	uint32_t ietc = ((long) z_soc_mp_asm_entry) >> 2;

	IDC[curr_cpu].core[cpu_num].ietc = ietc;
	IDC[curr_cpu].core[cpu_num].itc = IDC_MSG_POWER_UP;

#ifndef CONFIG_SOC_SERIES_INTEL_CAVS_V25
	/* Early DSPs have a ROM that actually receives the startup
	 * IDC as an interrupt, and we don't want that to be confused
	 * by IPIs sent by the OS elsewhere.  Mask the IDC interrupt
	 * on the new core so Zephyr IPIs from existing cores won't
	 * cause it to jump to ISR until the core is fully
	 * initialized.  Wait for the startup IDC to arrive though.
	 */
# ifdef CONFIG_IPM_CAVS_IDC
	uint32_t idc_reg = idc_read(IPC_IDCCTL, cpu_num);

	idc_reg &= ~IPC_IDCCTL_IDCTBIE(0);
	idc_write(IPC_IDCCTL, cpu_num, idc_reg);
	sys_set_bit(DT_REG_ADDR(DT_NODELABEL(cavs0)) + 0x00 +
		      CAVS_ICTL_INT_CPU_OFFSET(cpu_num), 8);

	k_busy_wait(100);

	if (IS_ENABLED(CONFIG_SMP_BOOT_DELAY)) {
		cavs_idc_smp_init(NULL);
	}
# else
	IDC[cpu_num].busy_int &= ~IDC_ALL_CORES;
	k_busy_wait(100);
# endif
#endif
}

void arch_sched_ipi(void)
{
#ifndef CONFIG_IPM_CAVS_IDC
	uint32_t curr = prid();

	for (int c = 0; c < CONFIG_MP_NUM_CPUS; c++) {
		if (c != curr && cpus_active[c]) {
			IDC[curr].core[c].itc = BIT(31);
		}
	}
#else
	/* Legacy implementation for cavs15 based on the 2-core-only
	 * IPM driver.  To be replaced with the general one when
	 * validated.
	 */
	const struct device *idcdev =
		device_get_binding(DT_LABEL(DT_INST(0, intel_cavs_idc)));

	ipm_send(idcdev, 0, IPM_CAVS_IDC_MSG_SCHED_IPI_ID,
		 IPM_CAVS_IDC_MSG_SCHED_IPI_DATA, 0);
#endif
}

void idc_isr(void *param)
{
	ARG_UNUSED(param);

#ifdef CONFIG_SMP
	/* Right now this interrupt is only used for IPIs */
	z_sched_ipi();
#endif

	/* ACK the interrupt to all the possible sources.  This is a
	 * level-sensitive interrupt triggered by a logical OR of each
	 * of the ITC/TFC high bits, INCLUDING the one "from this
	 * CPU".
	 */
	for (int i = 0; i < CONFIG_MP_NUM_CPUS; i++) {
		IDC[prid()].core[i].tfc = BIT(31);
	}
}

#ifndef CONFIG_IPM_CAVS_IDC
/* Fallback stub for external SOF code */
int cavs_idc_smp_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}
#endif

void soc_idc_init(void)
{
#ifndef CONFIG_IPM_CAVS_IDC
	IRQ_CONNECT(DT_IRQN(DT_NODELABEL(idc)), 0, idc_isr, NULL, 0);
#endif

	/* Every CPU should be able to receive an IDC interrupt from
	 * every other CPU, but not to be back-interrupted when the
	 * target core clears the busy bit.
	 */
	for (int core = 0; core < CONFIG_MP_NUM_CPUS; core++) {
		IDC[core].busy_int |= IDC_ALL_CORES;
		IDC[core].done_int &= ~IDC_ALL_CORES;

		/* Also unmask the IDC interrupt for every core in the
		 * L2 mask register.
		 */
		CAVS_INTCTRL[core].l2.clear = CAVS_L2_IDC;
	}

	/* Clear out any existing pending interrupts that might be present */
	for (int i = 0; i < CONFIG_MP_NUM_CPUS; i++) {
		for (int j = 0; j < CONFIG_MP_NUM_CPUS; j++) {
			IDC[i].core[j].tfc = BIT(31);
		}
	}

	cpus_active[0] = true;
}

/**
 * @brief Restart halted SMP CPU
 *
 * Relaunches a CPU that has entered an idle power state via
 * soc_halt_cpu().  Returns -EINVAL if the CPU is not in a power-gated
 * idle state.  Upon successful return, the CPU is online and
 * available to run any Zephyr thread.
 *
 * @param id CPU to start, in the range [1:CONFIG_MP_NUM_CPUS)
 */
int soc_relaunch_cpu(int id)
{
	int ret = 0;
	k_spinlock_key_t k = k_spin_lock(&mplock);

	if (id < 1 || id >= CONFIG_MP_NUM_CPUS) {
		ret = -EINVAL;
		goto out;
	}

	if (CAVS_SHIM.pwrsts & BIT(id)) {
		ret = -EINVAL;
		goto out;
	}

	CAVS_INTCTRL[id].l2.clear = CAVS_L2_IDC;
	z_reinit_idle_thread(id);
	z_smp_start_cpu(id);

 out:
	k_spin_unlock(&mplock, k);
	return ret;
}

/**
 * @brief Halts and offlines a running CPU
 *
 * Enables power gating on the specified CPU, which cannot be the
 * current CPU or CPU 0.  The CPU must be idle; no application threads
 * may be runnable on it when this function is called (or at least the
 * CPU must be guaranteed to reach idle in finite time without
 * deadlock).  Actual CPU shutdown can only happen in the context of
 * the idle thread, and synchronization is an application
 * responsibility.  This function will hang if the other CPU fails to
 * reach idle.
 *
 * @param id CPU to halt, not current cpu or cpu 0
 * @return 0 on success, -EINVAL on error
 */
int soc_halt_cpu(int id)
{
	int ret = 0;
	k_spinlock_key_t k = k_spin_lock(&mplock);

	if (id == 0 || id == _current_cpu->id) {
		ret = -EINVAL;
		goto out;
	}

	/* Turn off the "prevent power/clock gating" bits, enabling
	 * low power idle, and mask off IDC interrupts so it will not
	 * be woken up by scheduler IPIs
	 */
	CAVS_INTCTRL[id].l2.set = CAVS_L2_IDC;
	CAVS_SHIM.pwrctl &= ~BIT(id);
	CAVS_SHIM.clkctl &= ~BIT(16 + id);

	/* Wait for the CPU to reach an idle state before returing */
	while (CAVS_SHIM.pwrsts & BIT(id)) {
	}

 out:
	k_spin_unlock(&mplock, k);
	return ret;
}

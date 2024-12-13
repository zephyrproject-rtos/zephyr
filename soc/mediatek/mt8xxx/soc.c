/* Copyright 2023 The ChromiumOS Authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/sys/libc-hooks.h>
#include <string.h>
#include <kernel_internal.h>

extern char _mtk_adsp_sram_end[];

#define SRAM_START DT_REG_ADDR(DT_NODELABEL(sram0))
#define SRAM_SIZE  DT_REG_SIZE(DT_NODELABEL(sram0))
#define SRAM_END   (SRAM_START + SRAM_SIZE)

extern char _mtk_adsp_dram_end[];

#define DRAM_START DT_REG_ADDR(DT_NODELABEL(dram0))
#define DRAM_SIZE  DT_REG_SIZE(DT_NODELABEL(dram0))
#define DRAM_END   (DRAM_START + DRAM_SIZE)

#define DMA_START DT_REG_ADDR(DT_NODELABEL(dram1))
#define DMA_SIZE  DT_REG_SIZE(DT_NODELABEL(dram1))
#define DMA_END   (DMA_START + DMA_SIZE)


#ifdef CONFIG_SOC_MT8196
#define INIT_STACK "0x90400000"
#define LOG_BASE   0x90580000
#define LOG_LEN    0x80000
#else
#define INIT_STACK "0x60e00000"
#define LOG_BASE   0x60700000
#define LOG_LEN    0x100000
#endif

/* The MT8196 interrupt controller is very simple at runtime, with
 * just an enable and status register needed, like its
 * predecessors. But it has routing control which resets to "nothing
 * enabled", so needs a driver.
 *
 * There are 64 interrupt inputs to the controller, controlled by
 * pairs of words (the "intc64" type below).  Each interrupt is
 * associated with one[1] of 16 "groups", each of which directs to a
 * different Xtensa architectural interrupt.  So each Xtensa interrupt
 * can be configured to handle any subset of interrupt inputs.
 *
 * The mapping of groups to Xtensa interrupts is given below.  Note
 * particularly that the final two groups are NMIs directed to an
 * interrupt level higher than EXCM_LEVEL, so cannot be safely used
 * for OS code (they'll interrupt spinlocks), but an app might exploit
 * them for e.g. debug or watchdog hooks.
 *
 * GroupNum  XtensaIRQ  XtensaLevel
 *     0-5        0-5       1  (L1 is shared w/exceptions, poor choice)
 *     6-7        7-8       1
 *    8-10       9-11       2
 *   11-13      16-18       3
 *   14,15      20,21       4  (Unmaskable! Do not use w/Zephyr code!)
 *
 * Naming of the inputs looks like this, though obviously only a small
 * fraction have been validated (or are even useful for an audio DSP):
 *
 *  0: CCU              20: USB1             40: WDT
 *  1: SCP              21: SCPVOW           41: CONNSYS1
 *  2: SPM              22: CCIF3_C0         42: CONNSYS3
 *  3: PCIE             23: CCIF3_C1         43: CONNSYS4
 *  4: INFRA_HANG       24: PWR_CTRL         44: CONNSYS2
 *  5: PERI_TIMEOUT     25: DMA_C0           45: IPIC
 *  6: MBOX_C0          26: DMA_C1           46: AXI_DMA2
 *  7: MBOX_C1          27: AXI_DMA0         47: AXI_DMA3
 *  8: TIMER0           28: AXI_DMA1         48: APSRC_DDREN
 *  9: TIMER1           29: AUDIO_C0         49: LAT_MON_EMI
 * 10: IPC_C0           30: AUDIO_C1         50: LAT_MON_INFRA
 * 11: IPC_C1           31: HIFI5_WDT_C0     51: DEVAPC_VIO
 * 12: IPC1_RSV         32: HIFI5_WDT_C1     52: AO_INFRA_HANG
 * 13: C2C_SW_C0        33: APU_MBOX_C0      53: BUS_TRA_EMI
 * 14: C2C_SW_C1        34: APU_MBOX_C1      54: BUS_TRA_INFRA
 * 15: UART             35: TIMER2           55: L2SRAM_VIO
 * 16: UART_BT          36: PWR_ON_C0_IRQ    56: L2SRAM_SETERR
 * 17: LATENCY_MON      37: PWR_ON_C1_IRQ    57: PCIERC_GRP2
 * 18: BUS_TRACKER      38: WAKEUP_SRC_C0    58: PCIERC_GRP3
 * 19: USB0             39: WAKEUP_SRC_C1    59: IRQ_MAX_CHANNEL
 *
 * [1] It is legal and works as expected for an interrupt to be part
 *     of more than one group (more than one interrupt fires to handle
 *     it), though I don't understand why an application would want to
 *     do that.
 */

struct intc64 { uint32_t lo, hi; };

struct intc_8196 {
	struct intc64 input;        /* Raw (?) input signal, normally high */
	struct intc64 status;       /* Latched input, inverted (active == 1) */
	struct intc64 enable;       /* Interrupt enable */
	struct intc64 polarity;     /* 1 == active low */
	struct intc64 wake_enable;
	struct intc64 _unused;
	struct intc64 stage1_enable;
	struct intc64 sw_trigger;
	struct intc64 groups[16];       /* set bit == "member of group" */
	struct intc64 group_status[16]; /* status, but masked by group */
};

#define INTC (*(volatile struct intc_8196 *)0x1a014000)

static void set_group_bit(volatile struct intc64 *g, uint32_t bit, bool val)
{
	volatile uint32_t *p  = bit < 32 ? &g->lo : &g->hi;
	volatile uint32_t mask = BIT(bit & 0x1f);

	*p = val ? (*p | mask) : (*p & ~mask);
}

static void mt8196_intc_set_irq_group(uint32_t irq, uint32_t group)
{
	for (int i = 0; i < 16; i++) {
		set_group_bit(&INTC.groups[i], irq, i == group);
	}
}

void mt8196_intc_init(void)
{
	struct intc64 zero = { 0, 0 };

	INTC.enable = zero;
	INTC.polarity.lo = 0xffffffff;
	INTC.polarity.hi = 0xffffffff;
	INTC.wake_enable = zero;
	INTC.stage1_enable = zero;
	for (int i = 0; i < ARRAY_SIZE(INTC.groups); i++) {
		INTC.groups[i] = zero;
	}

	/* Now wire up known interrupts for existing drivers to their
	 * legacy settings
	 */
	mt8196_intc_set_irq_group(6, 2); /* mbox0 in group 2 */
	mt8196_intc_set_irq_group(7, 2); /* mbox1 in group 2 */
	mt8196_intc_set_irq_group(8, 1); /* ostimer in group 1 */
}

/* This is the true boot vector.  This device allows for direct
 * setting of the alternate reset vector, so we let it link wherever
 * it lands and extract its address in the loader.  This represents
 * the minimum amount of effort required to successfully call a C
 * function (and duplicates a few versions elsewhere in the tree:
 * really this should move to the arch layer).  The initial stack
 * really should be the end of _interrupt_stacks[0]
 */
__asm__(".align 4\n\t"
	".global mtk_adsp_boot_entry\n\t"
	"mtk_adsp_boot_entry:\n\t"
	"  movi  a0, 0x4002f\n\t" /* WOE|EXCM|INTLVL=15 */
	"  wsr   a0, PS\n\t"
	"  movi  a0, 0\n\t"
	"  wsr   a0, WINDOWBASE\n\t"
	"  movi  a0, 1\n\t"
	"  wsr   a0, WINDOWSTART\n\t"
	"  rsync\n\t"
	"  movi  a1, " INIT_STACK "\n\t"
	"  call4 c_boot\n\t");

/* Unfortunately the SOF kernel loader doesn't understand the boot
 * vector in the ELF/rimage file yet, so we still need a stub to get
 * actual audio firmware to load.  Leave a stub in place that jumps to
 * our "real" vector.  Note that this is frustratingly pessimal: the
 * kernel wants the entry point to be at the start of the SRAM region,
 * but (1) Xtensa can only load an immediate from addresses LOWER than
 * a L32R instruction, which we can't do and so need to jump across a
 * region to put one, and (2) the vector table that gets displaced has
 * a 1024 byte alignment requirement, forcing us to waste ~1011 bytes
 * needlessly.
 */
__asm__(".pushsection .sof_entry.text\n\t"
	"  j 2f\n"
	".align 4\n\t"
	"1:\n\t"
	"  .word mtk_adsp_boot_entry\n"
	"2:\n\t"
	"  l32r a0, 1b\n\t"
	"  jx a0\n\t"
	".popsection");

/* Initial MPU configuration, needed to enable caching */
static void enable_mpu(void)
{
	/* Note: we set the linked/in-use-by-zephyr regions of both
	 * SRAM and DRAM cached for performance.  The remainder is
	 * left uncached, as it's likely to be shared with the host
	 * and/or DMA.  This seems like a good default choice pending
	 * proper MPU integration
	 */
	static const uint32_t mpu[][2] = {
		{ 0x00000000, 0x06000 },          /* inaccessible null region */
		{ 0x10000000, 0x06f00 },          /* MMIO registers */
		{ 0x1d000000, 0x06000 },          /* inaccessible */
		{ SRAM_START, 0xf7f00 },          /* cached SRAM */
		{ (uint32_t)&_mtk_adsp_sram_end, 0x06f00 }, /* uncached SRAM */
		{ SRAM_END,   0x06000 },          /* inaccessible */
		{ DRAM_START, 0xf7f00 },          /* cached DRAM */
		{ (uint32_t)&_mtk_adsp_dram_end, 0x06f00 }, /* uncached DRAM */
		{ DRAM_END,   0x06000 },          /* inaccessible top of mem */
		{ DMA_START,  0x06f00 },          /* uncached host "DMA" area */
		{ DMA_END,    0x06000 },          /* inaccessible top of mem */
	};

	/* Must write BACKWARDS FROM THE END to avoid introducing a
	 * non-monotonic segment at the current instruction fetch.  The
	 * exception triggers even if all the segments involved are
	 * disabled!
	 */
	int32_t nseg = ARRAY_SIZE(mpu);

	for (int32_t i = 31; i >= 32 - nseg; i--) {
		int32_t mpuidx = i - (32 - nseg);
		uint32_t addren = mpu[mpuidx][0] | 1;
		uint32_t segprot = (mpu[mpuidx][1]) | i;

		/* If an active pipelined instruction fetch is in the
		 * same segment, wptlb must be preceded by a memw in
		 * the same cache line.  Jumping to an aligned-by-8
		 * address ensures that the following two (3-byte)
		 * instructions are in the same 8 byte-aligned region.
		 */
		__asm__ volatile("  j 1f\n"
				 ".align 8\n"
				 "1:\n"
				 "  memw\n"
				 "  wptlb %1, %0"
				 :: "r"(addren), "r"(segprot));
	}
}

/* Temporary console output, pending integration of a winstream
 * backend.  This simply appends a null-terminated string to an
 * otherwise unused 1M region of shared DRAM (it's a hole in the SOF
 * memory map before the DMA memory, so untouched by existing audio
 * firmware), making early debugging much easier: it can be read
 * directly out of /dev/mem (with e.g. dd | hexdump) and survives
 * device resets/panics/etc.  But it doesn't handle more than 1M of
 * output, there's no way to detect a reset of the stream, and in fact
 * it's actually racy with device startup as if you read too early
 * you'll see the old run and not the new one.  And it's wasteful,
 * even if this device has a ton of usably-mapped DRAM
 *
 * Also note that the storage for the buffer and length value get
 * reset by the DRAM clear near the end of c_boot().  If you want to
 * use this for extremely early logging you'll need to stub out the
 * dram clear and also set buf[0] to 0 manually (as it isn't affected
 * by device reset).
 */
#ifndef CONFIG_WINSTREAM_CONSOLE
int arch_printk_char_out(int c)
{
	char volatile * const buf = (void *)LOG_BASE;
	const size_t max = LOG_LEN - 4;
	int volatile * const len = (int *)&buf[max];

	if (*len < max) {
		buf[*len + 1] = 0;
		buf[(*len)++] = c;
	}
	return 0;
}
#endif

/* Define this here as a simple uncached array, no special linkage requirements */
__nocache char _winstream_console_buf[CONFIG_WINSTREAM_CONSOLE_STATIC_SIZE];

void c_boot(void)
{
	extern char _bss_start, _bss_end, z_xtensa_vecbase; /* Linker-emitted */
	uint32_t memctl = 0xffffff00; /* enable all caches */

	/* Clear bss before doing anything else, device memory is
	 * persistent across resets (!) and we'd like our static
	 * variables to be actually zero.  Do this without using
	 * memset() out of pedantry (because we don't know which libc is
	 * in use or whether it requires statics).
	 */
	for (char *p = &_bss_start; p < &_bss_end; p++) {
		*p = 0;
	}

	/* Set up MPU memory regions, both for protection and to
	 * enable caching (the hardware defaults is "uncached rwx
	 * memory everywhere").
	 */
	enable_mpu();

	/* But the CPU core won't actually use the cache without MEMCTL... */
	__asm__ volatile("wsr %0, MEMCTL; rsync" :: "r"(memctl));

	/* Need the vector base set to receive exceptions and
	 * interrupts (including register window exceptions, meaning
	 * we can't make C function calls until this is done!)
	 */
	__asm__ volatile("wsr %0, VECBASE; rsync" :: "r"(&z_xtensa_vecbase));

#ifdef CONFIG_SOC_SERIES_MT8195
	mtk_adsp_cpu_freq_init();
#endif

	/* Likewise, memory power is external to the device, and the
	 * kernel SOF loader doesn't zero it, so zero our unlinked
	 * memory to prevent possible pollution from previous runs.
	 * This region is uncached, no need to flush.
	 */
	memset(_mtk_adsp_sram_end, 0, SRAM_END - (uint32_t)&_mtk_adsp_sram_end);
	memset(_mtk_adsp_dram_end, 0, DRAM_END - (uint32_t)&_mtk_adsp_dram_end);

	/* Clear pending interrupts.  Note that this hardware has a
	 * habit of starting with all its timer interrupts flagged.
	 * These have to be cleared by writing to the equivalent
	 * CCOMPAREn register.  Assumes XCHAL_NUM_TIMERS == 3...
	 */
	uint32_t val = 0;

	__asm__ volatile("wsr %0, CCOMPARE0" :: "r"(val));
	__asm__ volatile("wsr %0, CCOMPARE1" :: "r"(val));
	__asm__ volatile("wsr %0, CCOMPARE2" :: "r"(val));
	__ASSERT_NO_MSG(XCHAL_NUM_TIMERS == 3);
	val = 0xffffffff;
	__asm__ volatile("wsr %0, INTCLEAR" :: "r"(val));

	/* Default console, a driver can override this later */
	__stdout_hook_install(arch_printk_char_out);

#ifdef CONFIG_SOC_MT8196
	mt8196_intc_init();
#endif

	void z_prep_c(void);
	z_prep_c();
}

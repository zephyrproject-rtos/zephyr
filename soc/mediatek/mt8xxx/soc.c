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

#ifdef CONFIG_SOC_MT8196
#define INIT_STACK "0x90400000"
#define LOG_BASE   0x90580000
#define LOG_LEN    0x80000
#else
#define INIT_STACK "0x60e00000"
#define LOG_BASE   0x60700000
#define LOG_LEN    0x100000
#endif

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

	void z_prep_c(void);
	z_prep_c();
}

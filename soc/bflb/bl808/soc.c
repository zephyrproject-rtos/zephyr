/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Bouffalo Lab RISC-V MCU series initialization code
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/cache.h>

#include <bflb_soc.h>
#include <glb_reg.h>
#include <hbn_reg.h>
#include <mm_glb_reg.h>
#include <mm_misc_reg.h>
#include <mcu_misc_reg.h>
#include <pds_reg.h>
#include <tzc_sec_reg.h>

/* T-Head E907 sysmap configuration */
#define SYSMAP_BASE              0xEFFFF000UL
#define SYSMAP_BASE_SHIFT        (12)
#define SYSMAP_ATTR_STRONG_ORDER BIT(4)
#define SYSMAP_ATTR_CACHE_ABLE   BIT(3)
#define SYSMAP_ATTR_BUFFER_ABLE  BIT(2)
#define SYSMAP_ADDR_OFFSET       0x0
#define SYSMAP_FLAGS_OFFSET      0x4
#define SYSMAP_ENTRY_OFFSET      0x8

/* T-Head E907 CSR bit definitions */
#define THEAD_MHCR_DE    BIT(0)  /* D-cache enable */
#define THEAD_MHCR_WB    BIT(1)  /* Write-back enable */
#define THEAD_MHCR_BPE   BIT(5)  /* Branch prediction enable */
#define THEAD_MHCR_L0BTB BIT(12) /* L0 BTB enable */
#define THEAD_MHINT_DPLD BIT(2)  /* D-cache prefetch */
#define THEAD_MHINT_AMR  BIT(3)  /* Auto memory request */

/* MXSTATUS (CSR 0x7C0): enable T-Head ISA extensions (bit 22) + unaligned access (bit 15).
 * SDK does both in one write during SystemInit.
 */
static void init_mxstatus(void)
{
	uint32_t tmp;

	__asm__ volatile("csrr %0, 0x7C0" : "=r"(tmp));
	tmp |= (1 << 22) | (1 << 15);
	__asm__ volatile("csrw 0x7C0, %0" : : "r"(tmp));
}

static void disable_interrupt_autostacking(void)
{
	uint32_t tmp;

	__asm__ volatile("csrr %0, 0x7E1" : "=r"(tmp));
	tmp &= ~(0x3 << 16);
	__asm__ volatile("csrw 0x7E1, %0" : : "r"(tmp));
}

static void set_thead_mhint(void)
{
	uint32_t tmp;

	__asm__ volatile("csrr %0, 0x7C5" : "=r"(tmp));
	tmp |= THEAD_MHINT_DPLD | THEAD_MHINT_AMR;
	__asm__ volatile("csrw 0x7C5, %0" : : "r"(tmp));
}

static void enable_branchpred(bool yes)
{
	uint32_t tmp;

	__asm__ volatile("fence\n"
			 "fence.i\n");
	__asm__ volatile("csrr %0, 0x7C1" : "=r"(tmp));
	if (yes) {
		tmp |= (1 << 5) | (1 << 12);
	} else {
		tmp &= ~((1 << 5) | (1 << 12));
	}
	__asm__ volatile("csrw 0x7C1, %0" : : "r"(tmp));
	__asm__ volatile("fence\n"
			 "fence.i\n");
}

/*
 * Halt secondary cores (D0 + LP) using the same sequence as BL808 SDK
 * GLB_Halt_CPU() for those cores, so M0 fully owns early bring-up.
 */
static void soc_bl808_halt_secondary_cores(void)
{
	uint32_t tmp;

	/* Halt D0 core: disable clock, then assert reset.
	 * Matches SDK GLB_Halt_CPU(GLB_CORE_ID_D0) → GLB_DSP0_Clock_Disable()
	 * followed by setting MM_GLB_REG_CTRL_MMCPU0_RESET.
	 */
	tmp = sys_read32(MM_GLB_BASE + MM_GLB_MM_CLK_CTRL_CPU_OFFSET);
	tmp &= ~MM_GLB_REG_MMCPU0_CLK_EN_MSK;
	sys_write32(tmp, MM_GLB_BASE + MM_GLB_MM_CLK_CTRL_CPU_OFFSET);
	k_busy_wait(1);
	tmp = sys_read32(MM_GLB_BASE + MM_GLB_MM_SW_SYS_RESET_OFFSET);
	tmp |= MM_GLB_REG_CTRL_MMCPU0_RESET_MSK;
	sys_write32(tmp, MM_GLB_BASE + MM_GLB_MM_SW_SYS_RESET_OFFSET);

	/* Halt LP (pico) core: disable clock, then assert reset.
	 * Matches SDK GLB_Halt_CPU(GLB_CORE_ID_LP) → PDS_Set_LP_Clock_Disable()
	 * followed by setting GLB_REG_CTRL_PICO_RESET.
	 */
	tmp = sys_read32(PDS_BASE + PDS_CPU_CORE_CFG0_OFFSET);
	tmp &= ~PDS_REG_PICO_CLK_EN_MSK;
	sys_write32(tmp, PDS_BASE + PDS_CPU_CORE_CFG0_OFFSET);
	k_busy_wait(1);
	tmp = sys_read32(GLB_BASE + GLB_SWRST_CFG2_OFFSET);
	tmp |= GLB_REG_CTRL_PICO_RESET_MSK;
	sys_write32(tmp, GLB_BASE + GLB_SWRST_CFG2_OFFSET);
}

/*
 * Configure T-Head E907 sysmap regions for BL808 memory map.
 * The bootloader sets up sysmap but lumps flash XIP and RAM into one CB region.
 * We reconfigure for correctness: flash XIP should be cacheable but not bufferable,
 * and PSRAM data bus at 0x50000000 needs cacheable+bufferable for performance.
 *
 * BL808 M0 core address map:
 *   0x00000000 - 0x50000000  Peripherals, uncached OCRAM/WRAM      → SO
 *   0x50000000 - 0x54000000  PSRAM (64MB window, data bus)         → CB
 *   0x54000000 - 0x58000000  Gap                                   → SO
 *   0x58000000 - 0x5C000000  Flash XIP (primary)                   → C
 *   0x5C000000 - 0x62020000  Flash XIP (secondary) + gap           → SO
 *   0x62020000 - 0x62058000  OCRAM + WRAM (cacheable aliases)      → CB
 *   0x62058000 - 0x7EF80000  Gap                                   → SO
 *   0x7EF80000 - 0xFFFFF000  DRAM/VRAM/CLIC/sysmap                → SO
 */
static void system_sysmap_init(void)
{
	uintptr_t base = SYSMAP_BASE;

	/* 0: 0x00000000 ~ 0x50000000: SO (peripherals, uncached memory) */
	sys_write32(0x50000000U >> SYSMAP_BASE_SHIFT, base + SYSMAP_ADDR_OFFSET);
	sys_write32(SYSMAP_ATTR_STRONG_ORDER, base + SYSMAP_FLAGS_OFFSET);
	base += SYSMAP_ENTRY_OFFSET;

	/* 1: 0x50000000 ~ 0x54000000: CB (PSRAM data bus, 64MB window) */
	sys_write32(0x54000000U >> SYSMAP_BASE_SHIFT, base + SYSMAP_ADDR_OFFSET);
	sys_write32(SYSMAP_ATTR_CACHE_ABLE | SYSMAP_ATTR_BUFFER_ABLE, base + SYSMAP_FLAGS_OFFSET);
	base += SYSMAP_ENTRY_OFFSET;

	/* 2: 0x54000000 ~ 0x58000000: SO (gap between PSRAM and flash XIP) */
	sys_write32(BL808_FLASH_XIP_BASE >> SYSMAP_BASE_SHIFT, base + SYSMAP_ADDR_OFFSET);
	sys_write32(SYSMAP_ATTR_STRONG_ORDER, base + SYSMAP_FLAGS_OFFSET);
	base += SYSMAP_ENTRY_OFFSET;

	/* 3: 0x58000000 ~ 0x5C000000: C (flash XIP, cacheable, not bufferable) */
	sys_write32(BL808_FLASH2_XIP_BASE >> SYSMAP_BASE_SHIFT, base + SYSMAP_ADDR_OFFSET);
	sys_write32(SYSMAP_ATTR_CACHE_ABLE, base + SYSMAP_FLAGS_OFFSET);
	base += SYSMAP_ENTRY_OFFSET;

	/* 4: 0x5C000000 ~ 0x62020000: SO (flash2 XIP + gap) */
	sys_write32(BL808_OCRAM_CACHEABLE_BASE >> SYSMAP_BASE_SHIFT,
		    base + SYSMAP_ADDR_OFFSET);
	sys_write32(SYSMAP_ATTR_STRONG_ORDER, base + SYSMAP_FLAGS_OFFSET);
	base += SYSMAP_ENTRY_OFFSET;

	/* 5: 0x62020000 ~ 0x62058000: CB (OCRAM + WRAM cacheable) */
	sys_write32(BL808_WRAM_CACHEABLE_END >> SYSMAP_BASE_SHIFT,
		    base + SYSMAP_ADDR_OFFSET);
	sys_write32(SYSMAP_ATTR_CACHE_ABLE | SYSMAP_ATTR_BUFFER_ABLE, base + SYSMAP_FLAGS_OFFSET);
	base += SYSMAP_ENTRY_OFFSET;

	/* 6: 0x62058000 ~ 0x7EF80000: SO (gap) */
	sys_write32(BL808_DRAM_CACHEABLE_BASE >> SYSMAP_BASE_SHIFT, base + SYSMAP_ADDR_OFFSET);
	sys_write32(SYSMAP_ATTR_STRONG_ORDER, base + SYSMAP_FLAGS_OFFSET);
	base += SYSMAP_ENTRY_OFFSET;

	/* 7: 0x7EF80000 ~ 0xFFFFF000: SO (DRAM/VRAM, CLIC, sysmap regs) */
	sys_write32(0xFFFFF000U >> SYSMAP_BASE_SHIFT, base + SYSMAP_ADDR_OFFSET);
	sys_write32(SYSMAP_ATTR_STRONG_ORDER, base + SYSMAP_FLAGS_OFFSET);
}

/*
 * TrustZone Controller: configure PSRAM-A and PSRAM-B security regions.
 * SDK SystemInit() sets region 0 covering 0-64MB, group 0, enabled but not locked.
 */
static void tzc_sec_psram_init(void)
{
	uint32_t tmp;
	/* endAddr=64MB: (0x4000000 >> 10) & 0xffff = 0; (0 - 1) & 0xffff = 0xFFFF
	 * startAddr=0: (0 >> 10) & 0xffff = 0
	 * Combined: END[15:0] | START[31:16] = 0x0000FFFF
	 */
	const uint32_t addr_val = 0x0000FFFFU;

	/* PSRAM-A region 0: set group=0, write address range, enable */
	tmp = sys_read32(TZC_SEC_BASE + TZC_SEC_TZC_PSRAMA_TZSRG_CTRL_OFFSET);
	tmp &= ~(3U << 0);
	sys_write32(tmp, TZC_SEC_BASE + TZC_SEC_TZC_PSRAMA_TZSRG_CTRL_OFFSET);
	sys_write32(addr_val, TZC_SEC_BASE + TZC_SEC_TZC_PSRAMA_TZSRG_R0_OFFSET);
	tmp = sys_read32(TZC_SEC_BASE + TZC_SEC_TZC_PSRAMA_TZSRG_CTRL_OFFSET);
	tmp |= (1U << 16);
	sys_write32(tmp, TZC_SEC_BASE + TZC_SEC_TZC_PSRAMA_TZSRG_CTRL_OFFSET);

	/* PSRAM-B region 0: same configuration */
	tmp = sys_read32(TZC_SEC_BASE + TZC_SEC_TZC_PSRAMB_TZSRG_CTRL_OFFSET);
	tmp &= ~(3U << 0);
	sys_write32(tmp, TZC_SEC_BASE + TZC_SEC_TZC_PSRAMB_TZSRG_CTRL_OFFSET);
	sys_write32(addr_val, TZC_SEC_BASE + TZC_SEC_TZC_PSRAMB_TZSRG_R0_OFFSET);
	tmp = sys_read32(TZC_SEC_BASE + TZC_SEC_TZC_PSRAMB_TZSRG_CTRL_OFFSET);
	tmp |= (1U << 16);
	sys_write32(tmp, TZC_SEC_BASE + TZC_SEC_TZC_PSRAMB_TZSRG_CTRL_OFFSET);
}

/*
 * PMP: block 256MB at 0x80000000 with no access, M-mode locked.
 * SDK pmp_init() uses entry 0 with NAPOT addressing.
 *
 * NAPOT encoding for base=0x80000000, size=256MB (0x10000000):
 *   pmpaddr0 = (base >> 2) | ((size >> 3) - 1) = 0x20000000 | 0x01FFFFFF = 0x21FFFFFF
 *   pmpcfg0[7:0] = L(0x80) | A=NAPOT(0x18) | no RWX = 0x98
 */
static void pmp_init(void)
{
	__asm__ volatile("csrw 0x3B0, %0" : : "r"(0x21FFFFFFUL)); /* pmpaddr0 */
	__asm__ volatile("csrw 0x3A0, %0" : : "r"(0x98UL));       /* pmpcfg0 */
}

/*
 * Power on MM (Multimedia) subsystem.
 * SDK System_Post_Init() calls PDS_Power_On_MM_System() which clears
 * force-off bits in PDS_CTL2 in a specific sequence with delay.
 */
static void mm_system_power_on(void)
{
	uint32_t tmp;

	/* mm_pwr_off = 0 */
	tmp = sys_read32(PDS_BASE + PDS_CTL2_OFFSET);
	tmp &= ~PDS_CR_PDS_FORCE_MM_PWR_OFF_MSK;
	sys_write32(tmp, PDS_BASE + PDS_CTL2_OFFSET);

	/* SDK waits >30us, uses 45us */
	k_busy_wait(45);

	/* mm_iso_en = 0 */
	tmp = sys_read32(PDS_BASE + PDS_CTL2_OFFSET);
	tmp &= ~PDS_CR_PDS_FORCE_MM_ISO_EN_MSK;
	sys_write32(tmp, PDS_BASE + PDS_CTL2_OFFSET);

	/* mm_gate_clk = 0 */
	tmp = sys_read32(PDS_BASE + PDS_CTL2_OFFSET);
	tmp &= ~PDS_CR_PDS_FORCE_MM_GATE_CLK_MSK;
	sys_write32(tmp, PDS_BASE + PDS_CTL2_OFFSET);

	/* mm_mem_stby = 0 */
	tmp = sys_read32(PDS_BASE + PDS_CTL2_OFFSET);
	tmp &= ~PDS_CR_PDS_FORCE_MM_MEM_STBY_MSK;
	sys_write32(tmp, PDS_BASE + PDS_CTL2_OFFSET);

	/* mm_pds_rst = 0 */
	tmp = sys_read32(PDS_BASE + PDS_CTL2_OFFSET);
	tmp &= ~PDS_CR_PDS_FORCE_MM_PDS_RST_MSK;
	sys_write32(tmp, PDS_BASE + PDS_CTL2_OFFSET);
}

/*
 * Make D0 L2SRAM available for MCU usage.
 * SDK System_Post_Init() calls GLB_Set_DSP_L2SRAM_Available_Size(3, 1, 1, 1).
 */
static void dsp_l2sram_init(void)
{
	uint32_t tmp;

	tmp = sys_read32(MM_MISC_BASE + MM_MISC_VRAM_CTRL_OFFSET);
	tmp = (tmp & MM_MISC_REG_H2PF_SRAM_REL_UMSK) | (3U << MM_MISC_REG_H2PF_SRAM_REL_POS);
	tmp = (tmp & MM_MISC_REG_VRAM_SRAM_REL_UMSK) | (1U << MM_MISC_REG_VRAM_SRAM_REL_POS);
	tmp = (tmp & MM_MISC_REG_SUB_SRAM_REL_UMSK) | (1U << MM_MISC_REG_SUB_SRAM_REL_POS);
	tmp = (tmp & MM_MISC_REG_BLAI_SRAM_REL_UMSK) | (1U << MM_MISC_REG_BLAI_SRAM_REL_POS);
	sys_write32(tmp, MM_MISC_BASE + MM_MISC_VRAM_CTRL_OFFSET);

	/* Commit the sysram setting */
	tmp = sys_read32(MM_MISC_BASE + MM_MISC_VRAM_CTRL_OFFSET);
	tmp |= MM_MISC_REG_SYSRAM_SET_MSK;
	sys_write32(tmp, MM_MISC_BASE + MM_MISC_VRAM_CTRL_OFFSET);
}

/* Disable machine interrupts (MIE in mstatus) to prevent unhandled traps
 * before mtvec is configured.
 */
static inline __attribute__((always_inline)) void disable_machine_interrupts(void)
{
	__asm__ volatile("csrc mstatus, 8");
}

/* Disable D-cache, write-back, and branch prediction (MHCR CSR 0x7C1).
 * The bootloader leaves these active; D-cache speculation through SF_CTRL
 * can conflict with I-cache line fills at certain XIP addresses.
 */
static inline __attribute__((always_inline)) void disable_dcache_and_branchpred(void)
{
	const uint32_t bits = THEAD_MHCR_DE | THEAD_MHCR_WB |
			      THEAD_MHCR_BPE | THEAD_MHCR_L0BTB;

	__asm__ volatile("csrc 0x7C1, %0" : : "r"(bits));
}

/* Clear speculative prefetch hints (MHINT CSR 0x7C5).
 * arch_cache_init() re-enables with DPLD|AMR after proper flush/invalidate.
 */
static inline __attribute__((always_inline)) void clear_speculation_hints(void)
{
	__asm__ volatile("csrw 0x7C5, zero");
}

/*
 * soc_reset_hook: called from __start (vectors section) BEFORE stack/BSS setup.
 * Must be naked — no stack available at this point. RISC-V 'call' only uses ra,
 * so leaf naked functions work without a stack.
 */
void __attribute__((naked)) soc_reset_hook(void)
{
	disable_machine_interrupts();
	disable_dcache_and_branchpred();
	clear_speculation_hints();
	__asm__ volatile("ret");
}

void soc_prep_hook(void)
{
	uint32_t tmp;

	tzc_sec_psram_init();
	pmp_init();

	/* Set EM to 160KB WRAM / 0KB EM (GLB_WRAM160KB_EM0KB = 0) */
	tmp = sys_read32(GLB_BASE + GLB_SRAM_CFG3_OFFSET);
	tmp &= GLB_EM_SEL_UMSK;
	sys_write32(tmp, GLB_BASE + GLB_SRAM_CFG3_OFFSET);
}

void soc_early_init_hook(void)
{
	uint32_t key = irq_lock();

	soc_bl808_halt_secondary_cores();
	system_sysmap_init();

	sys_cache_data_flush_all();
	sys_cache_instr_invd_all();
	mm_system_power_on();
	dsp_l2sram_init();

	irq_unlock(key);
}

/* Match SDK SystemInit() order exactly:
 * 1. MXSTATUS: THEADISAEE + MM
 * 2. csi_dcache_enable()  (DE|WB|WA|RS|BPE|L0BTB)
 * 3. csi_icache_enable()  (IE)
 * 4. MHINT 0x000c         (DPLD|AMR)
 * 5. MEXSTATUS             (disable SPUSHEN/SPSWAPEN)
 */
void arch_cache_init(void)
{
	init_mxstatus();
	sys_cache_data_enable();
	enable_branchpred(true);
	sys_cache_instr_enable();
	set_thead_mhint();
	disable_interrupt_autostacking();
	sys_cache_data_flush_and_invd_all();
	sys_cache_instr_invd_all();
}

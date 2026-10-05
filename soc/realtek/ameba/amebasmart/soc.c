/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <ameba_soc.h>

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/interrupt_controller/gic.h>
#include <zephyr/linker/linker-defs.h>
#include <cmsis_core.h>
#include <zephyr/sys/barrier.h>

#include "mmu_regions.h"

#define VECTOR_ADDRESS ((uintptr_t)_vector_start)

/*
 * Runs on every core before C runtime init, so no .data/.bss access. Enables
 * the FPU here because z_prep_c does it on the primary core only, and the
 * lazy-FPU UNDEF hook itself uses vmrs/vmsr, so a secondary would recurse.
 */
void soc_reset_hook(void)
{
	uint32_t reg_val = __get_CPACR();

	reg_val |= (CPACR_CP10(CPACR_FA) | CPACR_CP11(CPACR_FA));
	__set_CPACR(reg_val);
	barrier_isync_fence_full();

	__set_FPEXC(FPEXC_EN);
}

/* Linker-symbol MMU region sizes cannot be static initialisers. */
extern char _image_ram_end[];
extern char __image2_backtrace_start__[];
extern char __image2_backtrace_end__[];

void soc_prep_hook(void)
{
	uintptr_t start = (uintptr_t)__image2_backtrace_start__;
	uintptr_t end = (uintptr_t)__image2_backtrace_end__;

	__ASSERT_NO_MSG((start & 0xfff) == 0);
	__ASSERT_NO_MSG(end > start);

	if ((start & 0xfff) != 0 || end <= start) {
		return;
	}

	amebasmart_mmu_set_psram_image2_size((size_t)(end - start));

	uintptr_t img_end = (uintptr_t)_image_ram_end;

	__ASSERT_NO_MSG((img_end & 0xfff) == 0);
	__ASSERT_NO_MSG(img_end < AMEBASMART_DRAM_END);

	if ((img_end & 0xfff) != 0 || img_end >= AMEBASMART_DRAM_END) {
		return;
	}

	amebasmart_mmu_set_dram_beyond_size(img_end);
}

uint64_t vGetGenericTimerFreq(void)
{
	extern unsigned int z_clock_hw_cycles_per_sec;

	return (uint64_t)z_clock_hw_cycles_per_sec;
}

/* Flash-programming IPI: quiesce Core1 during XIP flash erase/write. */

#ifdef CONFIG_SMP

/* SGI#0 = sched, SGI#1 = mmcfg. */
#define FLASH_PG_SGI 2U

static volatile uint32_t flash_pg_flag;
static struct k_spinlock flash_pg_lock;
static k_spinlock_key_t flash_pg_lock_key;

/* SGI#2 handler: spin in WFE until Core0 clears flash_pg_flag. */
static void flash_pg_ipi_handler(const void *arg)
{
	ARG_UNUSED(arg);

#if defined(CONFIG_XIP)
	/* Flush TLB/BTAC entries stale after the XIP flash remap. */
	__set_TLBIALL(0);
	__set_BPIALL(0);
	barrier_dsync_fence_full();
#endif

	while (flash_pg_flag) {
		__WFE();
	}
}

void vPortGateOtherCore(void)
{
	/* Released in vPortWakeOtherCore. */
	flash_pg_lock_key = k_spin_lock(&flash_pg_lock);
	CA32_TypeDef *ca32 = CA32_BASE;

#if defined(CONFIG_XIP)
	/* Flush local TLB/BTAC for the XIP remap. */
	__set_TLBIALL(0);
	__set_BPIALL(0);
#endif

	flash_pg_flag = 1U;
	barrier_dsync_fence_full();

	unsigned int this_cpu = arch_curr_cpu()->id;
	unsigned int other_cpu = (this_cpu + 1U) % (unsigned int)CONFIG_MP_MAX_NUM_CPUS;

	gic_raise_sgi(FLASH_PG_SGI, 0ULL, (uint16_t)BIT(other_cpu));

	while (!(CA32_GET_STANDBYWFE(ca32->CA32_C0_CPU_STATUS) & BIT(other_cpu))) {
	}
}

void vPortWakeOtherCore(void)
{
	flash_pg_flag = 0U;
	barrier_dsync_fence_full();
	__SEV(); /* wake Core1 from WFE */
	k_spin_unlock(&flash_pg_lock, flash_pg_lock_key);
}

#else /* !CONFIG_SMP: stubs so the HAL links */

void vPortGateOtherCore(void)
{
}

void vPortWakeOtherCore(void)
{
}

#endif /* CONFIG_SMP */

void xlat_flash_region_device(void)
{
#if defined(CONFIG_ARM_MMU) && defined(CONFIG_XIP)
	arch_mem_map((void *)0x08000000U, 0x08000000U, 0x10000000U - 0x08000000U,
		     K_MEM_CACHE_NONE | K_MEM_DIRECT_MAP);
#endif
}

void xlat_flash_region_xip(void)
{
#if defined(CONFIG_ARM_MMU) && defined(CONFIG_XIP)
	arch_mem_map((void *)0x08000000U, 0x08000000U, 0x10000000U - 0x08000000U,
		     K_MEM_CACHE_WB | K_MEM_PERM_EXEC | K_MEM_DIRECT_MAP);
#endif
}

#if !defined(CONFIG_XIP)
/*
 * Overrides the __weak HAL cross-core flash gate: with no core fetching from
 * flash it is unneeded, and it deadlocks if Core1 cannot take SGI#2. The flash
 * API serialises callers, so a local IRQ lock keeps one SPIC transaction atomic.
 */
static unsigned int flash_write_lock_key;

void FLASH_Write_Lock(void)
{
	flash_write_lock_key = irq_lock();
}

void FLASH_Write_Unlock(void)
{
	irq_unlock(flash_write_lock_key);
}
#endif /* !CONFIG_XIP */

void relocate_vector_table(void)
{
	__set_VBAR(VECTOR_ADDRESS & ~0x1f);
	__ISB();
}

/* The Cortex-A/R default sys_arch_reboot() is a no-op __weak stub. */
void sys_arch_reboot(int type)
{
	ARG_UNUSED(type);

	System_Reset();
}

/* Must run at PRE_KERNEL_2: gic_dist_init() at PRE_KERNEL_1 disables every
 * SPI, wiping any earlier enable.
 */
static int soc_ipc_irq_init(void)
{
	/* The rpmsg MBOX driver, when enabled, owns IPCAP and its IRQ instead. */
#ifndef CONFIG_MBOX_REALTEK_AMEBA_IPC
	ipc_table_init(IPCAP_DEV);
	IRQ_CONNECT(IPC_AP_IRQ, INT_PRI_MIDDLE, IPC_INTHandler, (uint32_t)IPCAP_DEV, 0);
	irq_enable(IPC_AP_IRQ);
#else
	BUILD_ASSERT(!IS_ENABLED(CONFIG_WIFI_AMEBA),
		     "CONFIG_WIFI_AMEBA needs the vendor IPC dispatcher, which the "
		     "rpmsg MBOX driver replaces on the shared AP IPC interrupt");
#endif

#ifdef CONFIG_SMP
	IRQ_CONNECT(FLASH_PG_SGI, INT_PRI_MIDDLE, flash_pg_ipi_handler, NULL, 0);
	irq_enable(FLASH_PG_SGI);
#endif

	return 0;
}
SYS_INIT(soc_ipc_irq_init, PRE_KERNEL_2, 0);

#if CONFIG_MP_MAX_NUM_CPUS > 1
/* Pin all SPI IRQs to Core0 (SGI/PPI target bytes are read-only in GICv2). */
static int soc_pin_spi_irqs_to_core0(void)
{
	const uint32_t gic_irqs_field = sys_read32(GICD_TYPER) & 0x1f;
	const unsigned int gic_irqs = MIN((gic_irqs_field + 1) * 32U, 1020U);
	/* ITARGETSRn: one byte per IRQ, four IRQs per word. */
	const uint32_t core0_mask = BIT(0) | (BIT(0) << 8) | (BIT(0) << 16) | (BIT(0) << 24);

	for (unsigned int i = GIC_SPI_INT_BASE; i < gic_irqs; i += 4) {
		sys_write32(core0_mask, GICD_ITARGETSRn + i);
	}
	barrier_dmem_fence_full();

	return 0;
}
/* prio 1: after soc_ipc_irq_init (prio 0), before any POST_KERNEL IRQ enable. */
SYS_INIT(soc_pin_spi_irqs_to_core0, PRE_KERNEL_2, 1);
#endif /* CONFIG_MP_MAX_NUM_CPUS > 1 */

void soc_early_init_hook(void)
{
	extern unsigned int z_clock_hw_cycles_per_sec;

	z_clock_hw_cycles_per_sec = PLL_GetHBUSClk() / 2;

	uint32_t flash_para_addr = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_LSYS_FLASH_PARA_ADDR);

	if (flash_para_addr == 0U) {
		return;
	}

	DCache_Invalidate(flash_para_addr, sizeof(FLASH_InitTypeDef));
	_memcpy(&flash_init_para, (const void *)flash_para_addr, sizeof(FLASH_InitTypeDef));
}

#ifdef CONFIG_SMP
/* Power on Core1; must complete before PSCI CPU_ON is invoked. */
void soc_cpu_power_on(uint32_t cpu_mpid)
{
	uint32_t val;
	CA32_TypeDef *ca32 = CA32_BASE;

	/* Isolation released means Core1 is already powered. */
	if ((HSYS_GET_ISO_HP_AP_CORE(HAL_READ32(SYSTEM_CTRL_BASE_HP, REG_HSYS_HP_ISO)) == 0)) {
		return;
	}

	/* 1. Assert Core1 reset */
	ca32->CA32_C0_RST_CTRL &= ~(CA32_NCOREPORESET(0x2) | CA32_NCORERESET(0x2));

	/* 2. Set isolation for Core1 power domains */
	val = HAL_READ32(SYSTEM_CTRL_BASE_HP, REG_HSYS_HP_ISO);
	val |= HSYS_ISO_HP_AP_CORE(0x2);
	HAL_WRITE32(SYSTEM_CTRL_BASE_HP, REG_HSYS_HP_ISO, val);
	DelayUs(50);

	/* 3a. Power on Core1 primary power switch */
	val = HAL_READ32(SYSTEM_CTRL_BASE_HP, REG_HSYS_HP_PWC);
	val |= HSYS_PSW_HP_AP_CORE(0x3);
	HAL_WRITE32(SYSTEM_CTRL_BASE_HP, REG_HSYS_HP_PWC, val);
	DelayUs(50);

	/* 3b. Power on Core1 secondary power switch */
	val = HAL_READ32(SYSTEM_CTRL_BASE_HP, REG_HSYS_HP_PWC);
	val |= HSYS_PSW_HP_AP_CORE_2ND(0x3);
	HAL_WRITE32(SYSTEM_CTRL_BASE_HP, REG_HSYS_HP_PWC, val);
	DelayUs(500);

	/* 4. Release isolation */
	val = HAL_READ32(SYSTEM_CTRL_BASE_HP, REG_HSYS_HP_ISO);
	val &= ~HSYS_ISO_HP_AP_CORE(0x3);
	HAL_WRITE32(SYSTEM_CTRL_BASE_HP, REG_HSYS_HP_ISO, val);
	DelayUs(50);

	/* 5. De-assert Core1 resets */
	ca32->CA32_C0_RST_CTRL |= (CA32_NCOREPORESET(0x2) | CA32_NCORERESET(0x2));

	/* Core1 reaches the ATF WFE hold loop in ~7 us; 40 us is margin. */
	DelayUs(40);
}

/* SGIs are banked per CPU. */
void soc_per_core_init_hook(void)
{
	irq_enable(FLASH_PG_SGI);
}
#endif /* CONFIG_SMP */

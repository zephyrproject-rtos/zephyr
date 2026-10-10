/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * RTL8730E KM0 (Real-M200, Cortex-M23) system support.
 */

#include <stdint.h>
#include <ameba_soc.h>

/* XTAL 40 MHz; CPU_ClkGet() reads the live rate. */
uint32_t SystemCoreClock = 40000000U;

extern void z_arm_reset(void);
void km0_boot_trampoline(void);

/*
 * Boot ROM entry table at ORIGIN(RAM): cold boot calls FlashStartFun, wake from
 * powergate calls RamWakeupFun. An empty field makes the ROM branch to 0.
 */
__attribute__((used, section(".image2.entry.data")))
const RAM_FUNCTION_START_TABLE km0_image2_entry_tbl = {
	.RamStartFun   = km0_boot_trampoline,
	.RamWakeupFun  = km0_boot_trampoline,
	.FlashStartFun = km0_boot_trampoline,
};

/*
 * Replaces the vendor BOOT_Image1 prologue (LP clock = XTAL, caches on) before
 * Zephyr startup. Runs on the ROM-provided MSP.
 */
__attribute__((used, section(".image2.entry.text")))
void km0_boot_trampoline(void)
{
	u32 reg;

	reg = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_LSYS_CKSL_GRP0);
	reg |= LSYS_CKSL_LSOC(BIT_LSYS_CKSL_LP_XTAL);
	HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_LSYS_CKSL_GRP0, reg);

	/*
	 * Drop stale I-cache lines before enabling: SCB_EnableICache() skips its
	 * invalidate if the ROM left the cache on.
	 */
	ICache_Invalidate();
	Cache_Enable(ENABLE);

	z_arm_reset(); /* sets MSP/VTOR; never returns */
}

/* Required by CONFIG_SOC_EARLY_INIT_HOOK; the trampoline set up the clocks. */
void soc_early_init_hook(void)
{
}

/* ram_lp routines the prebuilt LP libraries import; the HAL lacks ram_lp. */
u32 CPU_InInterrupt(void)
{
	return __get_IPSR() != 0;
}

u32 np_status_on(void)
{
	return (HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_LSYS_CKE_GRP0) & APBPeriph_NP_CLOCK) ? 1 : 0;
}

/*
 * Vendor IPC dispatcher, which only the WiFi firmware role uses. The HAL
 * builds it for that role only, and the rpmsg MBOX driver owns IPCLP otherwise.
 */
#ifdef CONFIG_WIFI_FW_EN
#include <zephyr/init.h>
#include <zephyr/irq.h>

BUILD_ASSERT(!IS_ENABLED(CONFIG_MBOX_REALTEK_AMEBA_IPC),
	     "CONFIG_WIFI_FW_EN needs the vendor IPC dispatcher, which the "
	     "rpmsg MBOX driver replaces on the shared LP IPC interrupt");

static int soc_ipc_irq_init(void)
{
	ipc_table_init(IPCLP_DEV);
	IRQ_CONNECT(IPC_IRQ, INT_PRI_MIDDLE, IPC_INTHandler, (uint32_t)IPCLP_DEV, 0);
	irq_enable(IPC_IRQ);

	return 0;
}
SYS_INIT(soc_ipc_irq_init, PRE_KERNEL_2, 0);
#endif

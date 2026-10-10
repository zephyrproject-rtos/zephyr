/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * RTL8730E KM4 SoC hooks. The bootloader has already brought up PLL, PSRAM and
 * pin-mux, so unlike the SDK ameba_app_start.c no clock/PSRAM init runs here.
 */

#include <soc.h>
#include <ameba_soc.h>

#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/cache.h>

/*
 * Image2 entry header that IMG1 reads at ORIGIN(RAM). The HAL has no
 * SOCPS_WakeFromPG_AP for this core, so wakeup also enters z_arm_reset.
 */
extern void z_arm_reset(void);
extern char _vector_start[];

IMAGE2_ENTRY_SECTION
RAM_START_FUNCTION Img2EntryFun0 = {
	z_arm_reset,
	z_arm_reset,
	(uint32_t)_vector_start,
};

/*
 * Referenced by the HAL and lib_chipinfo.a. 200 MHz at boot; refreshed from
 * the live clock tree in soc_early_init_hook.
 */
uint32_t SystemCoreClock = 200000000U;

void SystemCoreClockUpdate(void)
{
	SystemCoreClock = CPU_ClkGet();
}

/* ram_hp routine the prebuilt WiFi libraries import; the HAL lacks ram_hp. */
u32 CPU_InInterrupt(void)
{
	return __get_IPSR() != 0;
}

void soc_early_init_hook(void)
{
	SystemCoreClockUpdate();

	sys_cache_instr_enable();
	sys_cache_data_enable();
}

/*
 * Vendor IPC dispatcher, which only the WiFi network-processor role uses. The
 * HAL builds it for that role only, and the rpmsg MBOX driver owns IPCNP otherwise.
 */
#ifdef CONFIG_AS_INIC_NP
BUILD_ASSERT(!IS_ENABLED(CONFIG_MBOX_REALTEK_AMEBA_IPC),
	     "CONFIG_AS_INIC_NP needs the vendor IPC dispatcher, which the "
	     "rpmsg MBOX driver replaces on the shared NP IPC interrupt");

static int soc_ipc_irq_init(void)
{
	ipc_table_init(IPCNP_DEV);
	IRQ_CONNECT(IPC_NP_IRQ, INT_PRI_MIDDLE, IPC_INTHandler, (uint32_t)IPCNP_DEV, 0);
	irq_enable(IPC_NP_IRQ);

	return 0;
}
SYS_INIT(soc_ipc_irq_init, PRE_KERNEL_2, 0);
#endif

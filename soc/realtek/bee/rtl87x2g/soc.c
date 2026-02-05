/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/arch/common/init.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/logging/log.h>
#include <soc.h>

#include "system_init_ns.h"
#include "utils.h"
#include "sys_reset.h"
#include "osif_zephyr.h"
#include "clock_manager.h"
#include "vector_table.h"
#include "image_info.h"
#include "image_check.h"
#include "rom_uuid.h"

LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

extern char __extram_data_start[];
extern char __extram_data_end[];
extern char __extram_data_load_start[];
extern char __extram_bss_start[];
extern char __extram_bss_end[];

extern void _isr_wrapper(void);

#define S_RAM_VECTOR_ADDR (0x14ec00)
#define STACK_ROM_ADDRESS DT_REG_ADDR(DT_NODELABEL(bee_bt_controller))

typedef bool (*BOOL_PATCH_FUNC)();

static void rtl87x2g_extra_ram_init(void)
{
	arch_early_memcpy(__extram_data_start, __extram_data_load_start,
			  (uintptr_t)__extram_data_end - (uintptr_t)__extram_data_start);
	arch_early_memcpy(__extram_bss_start, 0,
			  (uintptr_t)__extram_bss_end - (uintptr_t)__extram_bss_start);
}

/*
 * ----------------------------------------------------------------------------
 * RTL87x2G Interrupt Management Adaptation for Zephyr
 * ----------------------------------------------------------------------------
 * Context & Problem:
 * The RTL87x2G ROM code assumes a writable Vector Table exists in RAM.
 * During initialization, internal routines (e.g., phy_init) utilize
 * `RamVectorTableUpdate()` to directly overwrite ISR handlers in the vector table.
 *
 * This conflicts with Zephyr's interrupt management, which requires all interrupts to be
 * routed through a centralized `_isr_wrapper` to properly manage context
 * switching and kernel scheduling.
 *
 * Solution Strategy:
 * To reconcile the ROM's direct access behavior with Zephyr's interrupt management,
 * we implement a two-phase adaptation process:
 *
 * Phase 1: Vector Table Relocation (Enable Write Access)
 *    Executed in `soc_early_init_hook`:
 *    We relocate the VTOR to the reserved RAM buffer (`S_RAM_VECTOR_ADDR`) and
 *    populate it with Zephyr's initial vectors. This provides a valid, writable
 *    memory region. When SoC initialization attempts to update an ISR, it
 *    writes to this RAM buffer instead of read-only Flash, preventing HardFaults.
 *
 * Phase 2: ISR Restoration
 *    Executed in `soc_late_init_hook`:
 *    After SoC initialization concludes (where ROM codes may have modified the vectors),
 *    we synchronize the RAM vector table with Zephyr. Any ISR handler installed by ROM
 *    is captured and registered into Zephyr's `sw_isr_table`. Finally,
 *    the hardware vector entry is restored to `_isr_wrapper`, ensuring full
 *    integration with Zephyr's interrupt system.
 * ----------------------------------------------------------------------------
 */

static void rtl87x2g_bt_controller_init(void)
{
	BOOL_PATCH_FUNC bt_controller_entry;
	T_ROM_HEADER_FORMAT *stack_header = (T_ROM_HEADER_FORMAT *)STACK_ROM_ADDRESS;

	uint8_t target_uuid[] = DEFINE_symboltable_uuid;

	if (memcmp(stack_header->uuid, target_uuid, UUID_SIZE) == 0) {
		bt_controller_entry = (BOOL_PATCH_FUNC)((uint32_t)stack_header->entry_ptr);
		bt_controller_entry();
		LOG_INF("Loaded Realtek Bee BT Controller ROM.");
	} else {
		LOG_ERR("Failed to load Realtek Bee BT Controller ROM.");
	}
}

/*
 * Sync ROM-initialized ISRs with Zephyr by wrapping them via z_isr_install.
 */
static void rtl87x2g_isr_register(void)
{
	uint32_t *RamVectorTable_INT = (uint32_t *)(SCB->VTOR + 16 * 4);

	for (int irq = 0; irq < CONFIG_NUM_IRQS; irq++) {
		if (RamVectorTable_INT[irq] != (uint32_t)_isr_wrapper) {
			if (NVIC_GetEnableIRQ(irq) == 1) {
				NVIC_DisableIRQ(irq);
				z_isr_install(irq, (void *)RamVectorTable_INT[irq], NULL);
				NVIC_EnableIRQ(irq);
			} else {
				z_isr_install(irq, (void *)RamVectorTable_INT[irq], NULL);
			}
			RamVectorTableUpdate(irq + 16, (IRQ_Fun)_isr_wrapper);
		}
	}
}

void soc_early_init_hook(void)
{
	/* [Phase 1] Vector Table Relocation:
	 * Point VTOR to RAM to allow ROM code to update vectors safely.
	 */
	size_t vector_size = (size_t)_vector_end - (size_t)_vector_start;

	SCB->VTOR = (uint32_t)S_RAM_VECTOR_ADDR;
	(void)memcpy((void *)S_RAM_VECTOR_ADDR, _vector_start, vector_size);

	rtl87x2g_extra_ram_init();

	/* Initialize OS interface patches and queue primitives. */
	os_zephyr_patch_init();
	os_queue_func_init();

	/* Configure MPU regions. */
	mpu_setup();

	/* Set active mode clock source. */
	set_active_mode_clk_src();

	/* Initialize silicon flow data and apply FT parameters. */
	si_flow_data_init();
	ft_paras_apply();

	/* Apply PMU voltage tuning (LDO/PWM/PFM). */
	pmu_apply_voltage_tune();

	/* Restart power sequence to latch new settings. */
	pmu_power_on_sequence_restart();

	/* Initialize HAL hardware (RXI300) and CPU features (DWT, FPU). */
	hal_setup_hardware();
	hal_setup_cpu();

	/* Initialize secure OS function pointers (handles TrustZone NS-callable entries). */
	secure_os_func_ptr_init_rom();

	/* Configure 32K and LP module clocks. */
	set_up_32k_clk_src();
	set_lp_module_clk_info();

	/* PM Initialization */
	os_register_pm_excluded_handle = os_register_pm_excluded_handle_imp;
	os_unregister_pm_excluded_handle = os_unregister_pm_excluded_handle_imp;
	platform_rtc_aon_init();
	power_manager_master_init();
	power_manager_slave_init();
	platform_pm_init();

	/* Initialize OSC32 SDM software timer. */
	init_osc_sdm_timer();

	/* Initialize DVFS (Dynamic Voltage Frequency Scaling). */
	dvfs_init();

	/* Initialize PHY hardware control. */
	phy_hw_control_init(false);
	phy_init(false);

	/* Initialize thermal compensation tracking. */
	thermal_tracking_timer_init();

#ifdef CONFIG_TRUSTED_EXECUTION_NONSECURE
	/* Route interrupts to Non-Secure state. */
	setup_non_secure_nvic();
#endif
}

void soc_late_init_hook(void)
{
	/* Initialize HW AES mutex. */
	hw_aes_mutex_init();

	rtl87x2g_bt_controller_init();

	/* [Phase 2] ISR Restoration:
	 * Register ROM-installed ISRs to Zephyr and restore _isr_wrapper.
	 */
	rtl87x2g_isr_register();
}

#ifdef CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT
void arch_busy_wait(uint32_t usec_to_wait)
{
	platform_delay_us(usec_to_wait);
}
#endif

/* Overrides the weak ARM implementation */
void sys_arch_reboot(int type)
{
	/* Convert SYS_REBOOT_WARM (0) to RESET_ALL_EXCEPT_AON (1).
	 * Convert SYS_REBOOT_COLD (1) to RESET_ALL (0).
	 */
	int wdt_mode = (type == SYS_REBOOT_WARM) ? RESET_ALL_EXCEPT_AON : RESET_ALL;

	/* Call the watchdog system reset with the converted mode and reset reason. */
	WDG_SystemReset(wdt_mode, RESET_REASON_ZEPHYR);
}

/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/logging/log.h>
#include <soc.h>

#include "clock_manager.h"
#include "image_header.h"
#include "osif_zephyr.h"
#include "rom_uuid.h"
#include "rtl_boot_record.h"
#include "system_rtl876x.h"
#include "utils.h"
#include "vector_table.h"

extern void _isr_wrapper(void);

#define RAM_VECTOR_ADDR   (0x200000)
#define STACK_ROM_ADDRESS DT_REG_ADDR(DT_NODELABEL(bee_bt_controller))

typedef bool (*BOOL_PATCH_FUNC)(void);

LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

static void rtl8752h_bt_controller_init(void)
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
static void rtl8752h_isr_register(void)
{
	/* Skip the first 16 System Exceptions for both tables */
	uint32_t *RamVectorTable_INT = (uint32_t *)(SCB->VTOR + 16 * 4);
	uint32_t *FlashVectorTable_INT = (uint32_t *)(_vector_start + 16 * 4);
	unsigned int key = irq_lock();

	for (int irq = 0; irq < CONFIG_NUM_IRQS; irq++) {
		uint32_t current_isr = RamVectorTable_INT[irq];
		uint32_t expected_zephyr_isr = FlashVectorTable_INT[irq];

		if (current_isr != expected_zephyr_isr) {
			if (NVIC_GetEnableIRQ(irq) == 1) {
				NVIC_DisableIRQ(irq);
				z_isr_install(irq, (void *)current_isr, NULL);
				RamVectorTableUpdate(irq + 16, (IRQ_Fun)_isr_wrapper);
				NVIC_EnableIRQ(irq);
			} else {
				z_isr_install(irq, (void *)current_isr, NULL);
				RamVectorTableUpdate(irq + 16, (IRQ_Fun)_isr_wrapper);
			}
		}
	}

	irq_unlock(key);
}

void soc_early_reset_hook(void)
{
	rtl_boot_stage_record(START_PLATFORM_INIT);

	/* Initialize silicon flow data and apply FT parameters. */
	si_flow_data_init();
	ft_paras_apply();

	/*
	 * Connect the Level 1 interrupt ISR (Peripheral_Handler,
	 * which is implemented in ROM) to the Zephyr interrupt subsystem.
	 */
	IRQ_CONNECT(Peripheral_IRQn, 0, Peripheral_Handler, NULL, 0);
	irq_enable(Peripheral_IRQn);

	/* Enable cache */
	share_cache_ram();
}

void soc_early_init_hook(void)
{
	/* [Phase 1] Vector Table Relocation:
	 * Point VTOR to RAM to allow ROM code to update vectors safely.
	 */
	size_t vector_size = (size_t)_vector_end - (size_t)_vector_start;

	SCB->VTOR = (uint32_t)RAM_VECTOR_ADDR;
	(void)memcpy((void *)RAM_VECTOR_ADDR, _vector_start, vector_size);

	/* Initialize OS interface patches. */
	os_zephyr_patch_init();

	/* Initialize cpu clock source and apply calibrations. */
	set_active_mode_clk_src();

	/* Apply PMU voltage tuning (LDO/PWM/PFM). */
	pmu_apply_voltage_tune();

	/* Initialize 32 kHz clock source. */
	set_up_32k_clk_src();

	work_around_32k_power_glitch();

	/* Restart power sequence to latch new settings. */
	pmu_power_on_sequence_restart();

	si_flow_after_power_on_sequence_restart();

	work_around_32k_power_glitch_after_restart();

	rtl_boot_stage_record(AON_BOOT_DONE);

	/* Enable clocks, vendor regs, RAM power control & TRNG. */
	hal_setup_hardware();

	/* Enable cpu related features. */
	hal_setup_cpu();

	/* PM Initialization */
	platform_rtc_aon_init();
	power_manager_master_init();
	power_manager_slave_init();
	platform_pm_init();

	/* Initialize OSC32 SDM software timer. */
	init_osc_sdm_timer();

	/* Initialize PHY hardware control. */
	phy_hw_control_init(false);
	phy_init(false);
}

void soc_late_init_hook(void)
{
	/* Initialize HW AES mutex. */
	hw_aes_create_mutex();

	rtl8752h_bt_controller_init();

	/* [Phase 2] ISR Restoration:
	 * Register ROM-installed ISRs to Zephyr and restore _isr_wrapper.
	 */
	rtl8752h_isr_register();

	rtl_boot_stage_record(PON_BOOT_DONE);
}

#ifdef CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT
void arch_busy_wait(uint32_t usec_to_wait)
{
	platform_delay_us(usec_to_wait);
}
#endif

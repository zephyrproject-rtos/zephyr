/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <soc_init.h>
#include <flash_init.h>
#include <esp_private/system_internal.h>
#include <esp_timer.h>
#include <efuse_virtual.h>
#include <esp_rom_serial_output.h>
#include <zephyr/arch/riscv/arch.h>
#include <hal/interrupt_clic_ll.h>
#include <riscv/interrupt.h>
#include <soc/interrupts.h>
#include <soc/hp_sys_clkrst_reg.h>

extern void esp_reset_reason_init(void);
extern FUNC_NORETURN void z_cstart(void);

static void core_intr_matrix_clear(void)
{
	for (int i = 0; i < ETS_MAX_INTR_SOURCE; i++) {
		interrupt_clic_ll_route(0, i, ETS_INVALID_INUM);
	}

	for (int i = 0; i < 32; i++) {
		esprv_int_set_vectored(i, true);
	}
}

void IRAM_ATTR __esp_platform_app_start(void)
{
#ifdef CONFIG_FPU
	/* Enable the FPU before any float instruction runs, since the
	 * boot path does not go through the generic RISC-V reset code.
	 */
	__asm__ volatile("li t0, %0\n"
			 "csrs mstatus, t0\n"
			 "fscsr zero\n" ::"i"(MSTATUS_FS_INIT)
			 : "t0");
#endif

	if (!IS_ENABLED(CONFIG_SMP)) {
		/* Keep CPU1 clock gated and in reset */
		REG_CLR_BIT(HP_SYS_CLKRST_HPCORE1_CTRL0_REG, HP_SYS_CLKRST_REG_CORE1_CPU_CLK_EN);
		REG_CLR_BIT(HP_SYS_CLKRST_HPCORE1_CTRL0_REG, HP_SYS_CLKRST_REG_CORE1_CLIC_CLK_EN);
		REG_SET_BIT(HP_SYS_CLKRST_HPCORE1_CTRL0_REG, HP_SYS_CLKRST_REG_CORE1_GLOBAL_RST_EN);
	}

	core_intr_matrix_clear();

	esp_reset_reason_init();

	esp_timer_early_init();

	esp_flash_config();

	esp_efuse_init_virtual();

	z_cstart();

	CODE_UNREACHABLE;
}

void IRAM_ATTR __esp_platform_mcuboot_start(void)
{
	z_cstart();

	CODE_UNREACHABLE;
}

int IRAM_ATTR arch_printk_char_out(int c)
{
	if (c == '\n') {
		esp_rom_output_tx_one_char('\r');
	}
	esp_rom_output_tx_one_char(c);
	return 0;
}

void sys_arch_reboot(int type)
{
	esp_restart();
}

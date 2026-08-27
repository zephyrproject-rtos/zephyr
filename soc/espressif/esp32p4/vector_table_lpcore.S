/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

	.section	.init.vector,"ax"

	.global _vector_table
	.type _vector_table, @function
_vector_table:
	.option push
	.option norvc

	/*
	 * Entry 0: LP core boots from the first entry of the vector table.
	 * Jump to reset_vector to begin execution.
	 */
	j reset_vector

	.rept 2
	j _panic_handler
	.endr
	j ulp_lp_core_sw_intr_handler
	.rept 3
	j _panic_handler
	.endr
	j ulp_lp_core_lp_uart_intr_handler
	.rept 3
	j _panic_handler
	.endr
	j ulp_lp_core_lp_spi_intr_handler
	.rept 4
	j _panic_handler
	.endr
	j ulp_lp_core_trng_intr_handler
	j ulp_lp_core_lp_i2c_intr_handler
	j ulp_lp_core_lp_io_intr_handler
	j ulp_lp_core_lp_adc_intr_handler
	j ulp_lp_core_lp_touch_intr_handler
	j ulp_lp_core_tsens_intr_handler
	j ulp_lp_core_efuse_intr_handler
	j ulp_lp_core_lp_sysreg_intr_handler
	j ulp_lp_core_lp_ana_peri_intr_handler
	j ulp_lp_core_lp_pmu_intr_handler
	j ulp_lp_core_mailbox_intr_handler
	j ulp_lp_core_lp_timer_intr_handler
	j ulp_lp_core_lp_wdt_intr_handler
	j ulp_lp_core_lp_rtc_intr_handler
	j _panic_handler
	j _panic_handler

	.option pop
	.size _vector_table, .-_vector_table

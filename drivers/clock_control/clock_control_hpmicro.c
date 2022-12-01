/*
 * Copyright (c) 2022 HPMicro
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#define DT_DRV_COMPAT hpmicro_hpm_clock

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/hpmicro_clock_control.h>
#include <zephyr/sys/util.h>
#include <zephyr/device.h>
#include <soc.h>
#include <hpm_common.h>
#include <hpm_clock_drv.h>
#include <hpm_pllctl_drv.h>

struct clock_control_hpmicro_config {
	PLLCTL_Type *base;
	uint32_t freq;
	uint32_t sys_core;
	uint32_t ram_up_time;
	uint32_t sysctl_present;
};

const static uint32_t hpm_clock_src_map[] = {
IF_ENABLED(HPM_CLK_HAS_SRC_OSC24M, ([HPM_CLK_SRC_OSC24M] = clk_src_osc24m,))
IF_ENABLED(HPM_CLK_HAS_SRC_PLL0_CLK0, ([HPM_CLK_SRC_PLL0_CLK0] = clk_src_pll0_clk0,))
IF_ENABLED(HPM_CLK_HAS_SRC_PLL1_CLK0, ([HPM_CLK_SRC_PLL1_CLK0] = clk_src_pll1_clk0,))
IF_ENABLED(HPM_CLK_HAS_SRC_PLL1_CLK1, ([HPM_CLK_SRC_PLL1_CLK1] = clk_src_pll1_clk1,))
IF_ENABLED(HPM_CLK_HAS_SRC_PLL2_CLK0, ([HPM_CLK_SRC_PLL2_CLK0] = clk_src_pll2_clk0,))
IF_ENABLED(HPM_CLK_HAS_SRC_PLL2_CLK1, ([HPM_CLK_SRC_PLL2_CLK1] = clk_src_pll2_clk1,))
IF_ENABLED(HPM_CLK_HAS_SRC_PLL3_CLK0, ([HPM_CLK_SRC_PLL3_CLK0] = clk_src_pll3_clk0,))
IF_ENABLED(HPM_CLK_HAS_SRC_PLL4_CLK0, ([HPM_CLK_SRC_PLL4_CLK0] = clk_src_pll4_clk0,))
IF_ENABLED(HPM_CLK_HAS_SRC_OSC32K, ([HPM_CLK_SRC_OSC32K] = clk_src_osc32k,))
IF_ENABLED(HPM_CLK_HAS_ADC_SRC_AHB0, ([HPM_CLK_ADC_SRC_AHB0] = clk_adc_src_ahb0,))
IF_ENABLED(HPM_CLK_HAS_ADC_SRC_ANA0, ([HPM_CLK_ADC_SRC_ANA0] = clk_adc_src_ana0,))
IF_ENABLED(HPM_CLK_HAS_ADC_SRC_ANA1, ([HPM_CLK_ADC_SRC_ANA1] = clk_adc_src_ana1,))
IF_ENABLED(HPM_CLK_HAS_ADC_SRC_ANA2, ([HPM_CLK_ADC_SRC_ANA2] = clk_adc_src_ana2,))
IF_ENABLED(HPM_CLK_HAS_I2S_SRC_AHB0, ([HPM_CLK_I2S_SRC_AHB0] = clk_i2s_src_ahb0,))
IF_ENABLED(HPM_CLK_HAS_I2S_SRC_AUD0, ([HPM_CLK_I2S_SRC_AUD0] = clk_i2s_src_aud0,))
IF_ENABLED(HPM_CLK_HAS_I2S_SRC_AUD1, ([HPM_CLK_I2S_SRC_AUD1] = clk_i2s_src_aud1,))
IF_ENABLED(HPM_CLK_HAS_I2S_SRC_AUD2, ([HPM_CLK_I2S_SRC_AUD2] = clk_i2s_src_aud2))
};

const static uint32_t hpm_clock_name_map[] = {
IF_ENABLED(HPM_CLOCK_HAS_CPU0, ([HPM_CLOCK_CPU0] = clock_cpu0,))
IF_ENABLED(HPM_CLOCK_HAS_CPU1, ([HPM_CLOCK_CPU1] = clock_cpu1,))
IF_ENABLED(HPM_CLOCK_HAS_MCHTMR0, ([HPM_CLOCK_MCHTMR0] = clock_mchtmr0,))
IF_ENABLED(HPM_CLOCK_HAS_MCHTMR1, ([HPM_CLOCK_MCHTMR1] = clock_mchtmr1,))
IF_ENABLED(HPM_CLOCK_HAS_AXI0, ([HPM_CLOCK_AXI0] = clock_axi0,))
IF_ENABLED(HPM_CLOCK_HAS_AXI1, ([HPM_CLOCK_AXI1] = clock_axi1,))
IF_ENABLED(HPM_CLOCK_HAS_AXI2, ([HPM_CLOCK_AXI2] = clock_axi2,))
IF_ENABLED(HPM_CLOCK_HAS_AHB, ([HPM_CLOCK_AHB] = clock_ahb,))
IF_ENABLED(HPM_CLOCK_HAS_DRAM, ([HPM_CLOCK_DRAM] = clock_dram,))
IF_ENABLED(HPM_CLOCK_HAS_XPI0, ([HPM_CLOCK_XPI0] = clock_xpi0,))
IF_ENABLED(HPM_CLOCK_HAS_XPI1, ([HPM_CLOCK_XPI1] = clock_xpi1,))
IF_ENABLED(HPM_CLOCK_HAS_GPTMR0, ([HPM_CLOCK_GPTMR0] = clock_gptmr0,))
IF_ENABLED(HPM_CLOCK_HAS_GPTMR1, ([HPM_CLOCK_GPTMR1] = clock_gptmr1,))
IF_ENABLED(HPM_CLOCK_HAS_GPTMR2, ([HPM_CLOCK_GPTMR2] = clock_gptmr2,))
IF_ENABLED(HPM_CLOCK_HAS_GPTMR3, ([HPM_CLOCK_GPTMR3] = clock_gptmr3,))
IF_ENABLED(HPM_CLOCK_HAS_GPTMR4, ([HPM_CLOCK_GPTMR4] = clock_gptmr4,))
IF_ENABLED(HPM_CLOCK_HAS_GPTMR5, ([HPM_CLOCK_GPTMR5] = clock_gptmr5,))
IF_ENABLED(HPM_CLOCK_HAS_GPTMR6, ([HPM_CLOCK_GPTMR6] = clock_gptmr6,))
IF_ENABLED(HPM_CLOCK_HAS_GPTMR7, ([HPM_CLOCK_GPTMR7] = clock_gptmr7,))
IF_ENABLED(HPM_CLOCK_HAS_UART0, ([HPM_CLOCK_UART0] = clock_uart0,))
IF_ENABLED(HPM_CLOCK_HAS_UART1, ([HPM_CLOCK_UART1] = clock_uart1,))
IF_ENABLED(HPM_CLOCK_HAS_UART2, ([HPM_CLOCK_UART2] = clock_uart2,))
IF_ENABLED(HPM_CLOCK_HAS_UART3, ([HPM_CLOCK_UART3] = clock_uart3,))
IF_ENABLED(HPM_CLOCK_HAS_UART4, ([HPM_CLOCK_UART4] = clock_uart4,))
IF_ENABLED(HPM_CLOCK_HAS_UART5, ([HPM_CLOCK_UART5] = clock_uart5,))
IF_ENABLED(HPM_CLOCK_HAS_UART6, ([HPM_CLOCK_UART6] = clock_uart6,))
IF_ENABLED(HPM_CLOCK_HAS_UART7, ([HPM_CLOCK_UART7] = clock_uart7,))
IF_ENABLED(HPM_CLOCK_HAS_UART8, ([HPM_CLOCK_UART8] = clock_uart8,))
IF_ENABLED(HPM_CLOCK_HAS_UART9, ([HPM_CLOCK_UART9] = clock_uart9,))
IF_ENABLED(HPM_CLOCK_HAS_UART10, ([HPM_CLOCK_UART10] = clock_uart10,))
IF_ENABLED(HPM_CLOCK_HAS_UART11, ([HPM_CLOCK_UART11] = clock_uart11,))
IF_ENABLED(HPM_CLOCK_HAS_UART12, ([HPM_CLOCK_UART12] = clock_uart12,))
IF_ENABLED(HPM_CLOCK_HAS_UART13, ([HPM_CLOCK_UART13] = clock_uart13,))
IF_ENABLED(HPM_CLOCK_HAS_UART14, ([HPM_CLOCK_UART14] = clock_uart14,))
IF_ENABLED(HPM_CLOCK_HAS_UART15, ([HPM_CLOCK_UART15] = clock_uart15,))
IF_ENABLED(HPM_CLOCK_HAS_I2C0, ([HPM_CLOCK_I2C0] = clock_i2c0,))
IF_ENABLED(HPM_CLOCK_HAS_I2C1, ([HPM_CLOCK_I2C1] = clock_i2c1,))
IF_ENABLED(HPM_CLOCK_HAS_I2C2, ([HPM_CLOCK_I2C2] = clock_i2c2,))
IF_ENABLED(HPM_CLOCK_HAS_I2C3, ([HPM_CLOCK_I2C3] = clock_i2c3,))
IF_ENABLED(HPM_CLOCK_HAS_SPI0, ([HPM_CLOCK_SPI0] = clock_spi0,))
IF_ENABLED(HPM_CLOCK_HAS_SPI1, ([HPM_CLOCK_SPI1] = clock_spi1,))
IF_ENABLED(HPM_CLOCK_HAS_SPI2, ([HPM_CLOCK_SPI2] = clock_spi2,))
IF_ENABLED(HPM_CLOCK_HAS_SPI3, ([HPM_CLOCK_SPI3] = clock_spi3,))
IF_ENABLED(HPM_CLOCK_HAS_CAN0, ([HPM_CLOCK_CAN0] = clock_can0,))
IF_ENABLED(HPM_CLOCK_HAS_CAN1, ([HPM_CLOCK_CAN1] = clock_can1,))
IF_ENABLED(HPM_CLOCK_HAS_CAN2, ([HPM_CLOCK_CAN2] = clock_can2,))
IF_ENABLED(HPM_CLOCK_HAS_CAN3, ([HPM_CLOCK_CAN3] = clock_can3,))
IF_ENABLED(HPM_CLOCK_HAS_DISPLAY, ([HPM_CLOCK_DISPLAY] = clock_display,))
IF_ENABLED(HPM_CLOCK_HAS_SDXC0, ([HPM_CLOCK_SDXC0] = clock_sdxc0,))
IF_ENABLED(HPM_CLOCK_HAS_SDXC1, ([HPM_CLOCK_SDXC1] = clock_sdxc1,))
IF_ENABLED(HPM_CLOCK_HAS_CAMERA0, ([HPM_CLOCK_CAMERA0] = clock_camera0,))
IF_ENABLED(HPM_CLOCK_HAS_CAMERA1, ([HPM_CLOCK_CAMERA1] = clock_camera1,))
IF_ENABLED(HPM_CLOCK_HAS_NTMR0, ([HPM_CLOCK_NTMR0] = clock_ntmr0,))
IF_ENABLED(HPM_CLOCK_HAS_NTMR1, ([HPM_CLOCK_NTMR1] = clock_ntmr1,))
IF_ENABLED(HPM_CLOCK_HAS_PTPC, ([HPM_CLOCK_PTPC] = clock_ptpc,))
IF_ENABLED(HPM_CLOCK_HAS_REF0, ([HPM_CLOCK_REF0] = clock_ref0,))
IF_ENABLED(HPM_CLOCK_HAS_REF1, ([HPM_CLOCK_REF1] = clock_ref1,))
IF_ENABLED(HPM_CLOCK_HAS_WATCHDOG0, ([HPM_CLOCK_WATCHDOG0] = clock_watchdog0,))
IF_ENABLED(HPM_CLOCK_HAS_WATCHDOG1, ([HPM_CLOCK_WATCHDOG1] = clock_watchdog1,))
IF_ENABLED(HPM_CLOCK_HAS_WATCHDOG2, ([HPM_CLOCK_WATCHDOG2] = clock_watchdog2,))
IF_ENABLED(HPM_CLOCK_HAS_WATCHDOG3, ([HPM_CLOCK_WATCHDOG3] = clock_watchdog3,))
IF_ENABLED(HPM_CLOCK_HAS_PUART, ([HPM_CLOCK_PUART] = clock_puart,))
IF_ENABLED(HPM_CLOCK_HAS_PWDG, ([HPM_CLOCK_PWDG] = clock_pwdg,))
IF_ENABLED(HPM_CLOCK_HAS_ETH0, ([HPM_CLOCK_ETH0] = clock_eth0,))
IF_ENABLED(HPM_CLOCK_HAS_ETH1, ([HPM_CLOCK_ETH1] = clock_eth1,))
IF_ENABLED(HPM_CLOCK_HAS_PTP0, ([HPM_CLOCK_PTP0] = clock_ptp0,))
IF_ENABLED(HPM_CLOCK_HAS_PTP1, ([HPM_CLOCK_PTP1] = clock_ptp1,))
IF_ENABLED(HPM_CLOCK_HAS_SDP, ([HPM_CLOCK_SDP] = clock_sdp,))
IF_ENABLED(HPM_CLOCK_HAS_XDMA, ([HPM_CLOCK_XDMA] = clock_xdma,))
IF_ENABLED(HPM_CLOCK_HAS_ROM, ([HPM_CLOCK_ROM] = clock_rom,))
IF_ENABLED(HPM_CLOCK_HAS_RAM0, ([HPM_CLOCK_RAM0] = clock_ram0,))
IF_ENABLED(HPM_CLOCK_HAS_RAM1, ([HPM_CLOCK_RAM1] = clock_ram1,))
IF_ENABLED(HPM_CLOCK_HAS_USB0, ([HPM_CLOCK_USB0] = clock_usb0,))
IF_ENABLED(HPM_CLOCK_HAS_USB1, ([HPM_CLOCK_USB1] = clock_usb1,))
IF_ENABLED(HPM_CLOCK_HAS_JPEG, ([HPM_CLOCK_JPEG] = clock_jpeg,))
IF_ENABLED(HPM_CLOCK_HAS_PDMA, ([HPM_CLOCK_PDMA] = clock_pdma,))
IF_ENABLED(HPM_CLOCK_HAS_KMAN, ([HPM_CLOCK_KMAN] = clock_kman,))
IF_ENABLED(HPM_CLOCK_HAS_GPIO, ([HPM_CLOCK_GPIO] = clock_gpio,))
IF_ENABLED(HPM_CLOCK_HAS_MBX0, ([HPM_CLOCK_MBX0] = clock_mbx0,))
IF_ENABLED(HPM_CLOCK_HAS_MBX1, ([HPM_CLOCK_MBX1] = clock_mbx1,))
IF_ENABLED(HPM_CLOCK_HAS_HDMA, ([HPM_CLOCK_HDMA] = clock_hdma,))
IF_ENABLED(HPM_CLOCK_HAS_RNG, ([HPM_CLOCK_RNG] = clock_rng,))
IF_ENABLED(HPM_CLOCK_HAS_MOT0, ([HPM_CLOCK_MOT0] = clock_mot0,))
IF_ENABLED(HPM_CLOCK_HAS_MOT1, ([HPM_CLOCK_MOT1] = clock_mot1,))
IF_ENABLED(HPM_CLOCK_HAS_MOT2, ([HPM_CLOCK_MOT2] = clock_mot2,))
IF_ENABLED(HPM_CLOCK_HAS_MOT3, ([HPM_CLOCK_MOT3] = clock_mot3,))
IF_ENABLED(HPM_CLOCK_HAS_ACMP, ([HPM_CLOCK_ACMP] = clock_acmp,))
IF_ENABLED(HPM_CLOCK_HAS_PDM, ([HPM_CLOCK_PDM] = clock_pdm,))
IF_ENABLED(HPM_CLOCK_HAS_DAO, ([HPM_CLOCK_DAO] = clock_dao,))
IF_ENABLED(HPM_CLOCK_HAS_MSYN, ([HPM_CLOCK_MSYN] = clock_msyn,))
IF_ENABLED(HPM_CLOCK_HAS_LMM0, ([HPM_CLOCK_LMM0] = clock_lmm0,))
IF_ENABLED(HPM_CLOCK_HAS_LMM1, ([HPM_CLOCK_LMM1] = clock_lmm1,))
IF_ENABLED(HPM_CLOCK_HAS_ANA0, ([HPM_CLOCK_ANA0] = clock_ana0,))
IF_ENABLED(HPM_CLOCK_HAS_ANA1, ([HPM_CLOCK_ANA1] = clock_ana1,))
IF_ENABLED(HPM_CLOCK_HAS_ANA2, ([HPM_CLOCK_ANA2] = clock_ana2,))
IF_ENABLED(HPM_CLOCK_HAS_ADC0, ([HPM_CLOCK_ADC0] = clock_adc0,))
IF_ENABLED(HPM_CLOCK_HAS_ADC1, ([HPM_CLOCK_ADC1] = clock_adc1,))
IF_ENABLED(HPM_CLOCK_HAS_ADC2, ([HPM_CLOCK_ADC2] = clock_adc2,))
IF_ENABLED(HPM_CLOCK_HAS_ADC3, ([HPM_CLOCK_ADC3] = clock_adc3,))
IF_ENABLED(HPM_CLOCK_HAS_AUD0, ([HPM_CLOCK_AUD0] = clock_aud0,))
IF_ENABLED(HPM_CLOCK_HAS_AUD1, ([HPM_CLOCK_AUD1] = clock_aud1,))
IF_ENABLED(HPM_CLOCK_HAS_AUD2, ([HPM_CLOCK_AUD2] = clock_aud2,))
IF_ENABLED(HPM_CLOCK_HAS_I2S0, ([HPM_CLOCK_I2S0] = clock_i2s0,))
IF_ENABLED(HPM_CLOCK_HAS_I2S1, ([HPM_CLOCK_I2S1] = clock_i2s1,))
IF_ENABLED(HPM_CLOCK_HAS_I2S2, ([HPM_CLOCK_I2S2] = clock_i2s2,))
IF_ENABLED(HPM_CLOCK_HAS_I2S3, ([HPM_CLOCK_I2S3] = clock_i2s3,))
IF_ENABLED(HPM_CLOCK_HAS_OSC0CLK0, ([HPM_CLOCK_OSC0CLK0] = clk_osc0clk0,))
IF_ENABLED(HPM_CLOCK_HAS_PLL0CLK0, ([HPM_CLOCK_PLL0CLK0] = clk_pll0clk0,))
IF_ENABLED(HPM_CLOCK_HAS_PLL1CLK0, ([HPM_CLOCK_PLL1CLK0] = clk_pll1clk0,))
IF_ENABLED(HPM_CLOCK_HAS_PLL1CLK1, ([HPM_CLOCK_PLL1CLK1] = clk_pll1clk1,))
IF_ENABLED(HPM_CLOCK_HAS_PLL2CLK0, ([HPM_CLOCK_PLL2CLK0] = clk_pll2clk0,))
IF_ENABLED(HPM_CLOCK_HAS_PLL2CLK1, ([HPM_CLOCK_PLL2CLK1] = clk_pll2clk1,))
IF_ENABLED(HPM_CLOCK_HAS_PLL3CLK0, ([HPM_CLOCK_PLL3CLK0] = clk_pll3clk0,))
IF_ENABLED(HPM_CLOCK_HAS_PLL4CLK0, ([HPM_CLOCK_PLL4CLK0] = clk_pll4clk0,))
};

static int clock_control_hpmicro_init(const struct device *dev)
{
	const struct clock_control_hpmicro_config *config = dev->config;
	uint32_t cpu0_freq = clock_get_frequency(config->sys_core);
	uint32_t clock_group = 0;

	if (cpu0_freq == PLLCTL_SOC_PLL_REFCLK_FREQ) {
		/* Configure the External OSC ramp-up time*/
		pllctl_xtal_set_rampup_time(config->base, config->ram_up_time);

		/* Select clock setting preset1 */
		sysctl_clock_set_preset(HPM_SYSCTL, config->sysctl_present);
	}
	/* Add most Clocks to group  */
	IF_ENABLED(HPM_CLOCK_HAS_CPU0, (clock_add_to_group(clock_cpu0, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_MCHTMR0, (clock_add_to_group(clock_mchtmr0, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_AXI0, (clock_add_to_group(clock_axi0, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_AXI1, (clock_add_to_group(clock_axi1, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_AXI2, (clock_add_to_group(clock_axi2, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_AHB, (clock_add_to_group(clock_ahb, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_DRAM, (clock_add_to_group(clock_dram, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_XPI0, (clock_add_to_group(clock_xpi0, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_XPI1, (clock_add_to_group(clock_xpi1, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_GPTMR0, (clock_add_to_group(clock_gptmr0, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_GPTMR1, (clock_add_to_group(clock_gptmr1, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_GPTMR2, (clock_add_to_group(clock_gptmr2, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_GPTMR3, (clock_add_to_group(clock_gptmr3, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_GPTMR4, (clock_add_to_group(clock_gptmr4, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_GPTMR5, (clock_add_to_group(clock_gptmr5, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_GPTMR6, (clock_add_to_group(clock_gptmr6, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_GPTMR7, (clock_add_to_group(clock_gptmr7, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_UART0, (clock_add_to_group(clock_uart0, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_UART1, (clock_add_to_group(clock_uart1, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_UART2, (clock_add_to_group(clock_uart2, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_UART3, (clock_add_to_group(clock_uart3, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_UART13, (clock_add_to_group(clock_uart13, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_I2C0, (clock_add_to_group(clock_i2c0, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_I2C1, (clock_add_to_group(clock_i2c1, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_I2C2, (clock_add_to_group(clock_i2c2, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_I2C3, (clock_add_to_group(clock_i2c3, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_SPI0, (clock_add_to_group(clock_spi0, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_SPI1, (clock_add_to_group(clock_spi1, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_SPI2, (clock_add_to_group(clock_spi2, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_SPI3, (clock_add_to_group(clock_spi3, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_CAN0, (clock_add_to_group(clock_can0, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_CAN1, (clock_add_to_group(clock_can1, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_CAN2, (clock_add_to_group(clock_can2, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_CAN3, (clock_add_to_group(clock_can3, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_DISPLAY, (clock_add_to_group(clock_display, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_SDXC0, (clock_add_to_group(clock_sdxc0, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_SDXC1, (clock_add_to_group(clock_sdxc1, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_CAMERA0, (clock_add_to_group(clock_camera0, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_CAMERA1, (clock_add_to_group(clock_camera1, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_PTPC, (clock_add_to_group(clock_ptpc, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_REF0, (clock_add_to_group(clock_ref0, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_REF1, (clock_add_to_group(clock_ref1, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_WATCHDOG0, (clock_add_to_group(clock_watchdog0, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_ETH0, (clock_add_to_group(clock_eth0, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_ETH1, (clock_add_to_group(clock_eth1, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_SDP, (clock_add_to_group(clock_sdp, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_XDMA, (clock_add_to_group(clock_xdma, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_RAM0, (clock_add_to_group(clock_ram0, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_RAM1, (clock_add_to_group(clock_ram1, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_USB0, (clock_add_to_group(clock_usb0, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_USB1, (clock_add_to_group(clock_usb1, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_JPEG, (clock_add_to_group(clock_jpeg, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_PDMA, (clock_add_to_group(clock_pdma, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_KMAN, (clock_add_to_group(clock_kman, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_GPIO, (clock_add_to_group(clock_gpio, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_MBX0, (clock_add_to_group(clock_mbx0, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_HDMA, (clock_add_to_group(clock_hdma, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_RNG, (clock_add_to_group(clock_rng, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_MOT0, (clock_add_to_group(clock_mot0, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_MOT1, (clock_add_to_group(clock_mot1, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_MOT2, (clock_add_to_group(clock_mot2, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_MOT3, (clock_add_to_group(clock_mot3, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_ACMP, (clock_add_to_group(clock_acmp, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_DAO, (clock_add_to_group(clock_dao, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_MSYN, (clock_add_to_group(clock_msyn, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_LMM0, (clock_add_to_group(clock_lmm0, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_LMM1, (clock_add_to_group(clock_lmm1, clock_group);))

	IF_ENABLED(HPM_CLOCK_HAS_ADC0, (clock_add_to_group(clock_adc0, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_ADC1, (clock_add_to_group(clock_adc1, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_ADC2, (clock_add_to_group(clock_adc2, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_ADC3, (clock_add_to_group(clock_adc3, clock_group);))

	IF_ENABLED(HPM_CLOCK_HAS_I2S0, (clock_add_to_group(clock_i2s0, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_I2S1, (clock_add_to_group(clock_i2s1, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_I2S2, (clock_add_to_group(clock_i2s2, clock_group);))
	IF_ENABLED(HPM_CLOCK_HAS_I2S3, (clock_add_to_group(clock_i2s3, clock_group);))

	/* Connect Group0 to CPU0 */
	clock_connect_group_to_cpu(0, 0);

	if (status_success != pllctl_init_int_pll_with_freq(config->base, 0, config->freq)) {
		return -1;
	}

	clock_set_source_divider(clock_cpu0, clk_src_pll0_clk0, 1);
	/* Connect Group1 to CPU1 */
	clock_connect_group_to_cpu(1, 1);
	clock_update_core_clock();
	clock_set_source_divider(clock_ahb, clk_src_pll1_clk1, 2);
	clock_set_source_divider(clock_mchtmr0, clk_src_osc24m, 1);
	/* Keep cpu clock on wfi, so that mchtmr irq can still work after wfi */
	sysctl_set_cpu_lp_mode(HPM_SYSCTL, HPM_CORE0, cpu_lp_mode_ungate_cpu_clock);

	return 0;
}

static int hpmicro_check_sys_data(clock_control_subsys_t sys,
			struct hpm_clock_configure_data *data)
{
	struct hpm_clock_configure_data cfg_data = *(struct hpm_clock_configure_data *)sys;

	if (sizeof(hpm_clock_name_map)/sizeof(uint32_t) < cfg_data.clock_name) {
		return -EPERM;
	}
	if (sizeof(hpm_clock_src_map)/sizeof(uint32_t) < cfg_data.clock_src) {
		return -EPERM;
	}
	*data = cfg_data;
	return 0;
}

static int clock_control_hpmicro_on(const struct device *dev,
				 clock_control_subsys_t sys)
{
	struct hpm_clock_configure_data cfg_data;

	if (hpmicro_check_sys_data(sys, &cfg_data) == 0) {
		clock_enable(hpm_clock_name_map[cfg_data.clock_name]);
	} else {
		return -EPERM;
	}

	return 0;
}

static int clock_control_hpmicro_off(const struct device *dev,
				  clock_control_subsys_t sys)
{
	struct hpm_clock_configure_data cfg_data;

	if (hpmicro_check_sys_data(sys, &cfg_data) == 0) {
		clock_disable(hpm_clock_name_map[cfg_data.clock_name]);
	} else {
		return -EPERM;
	}

	return 0;
}

static int clock_control_hpmicro_get_rate(const struct device *dev,
					   clock_control_subsys_t sys,
					   uint32_t *rate)
{
	struct hpm_clock_configure_data cfg_data;

	if (hpmicro_check_sys_data(sys, &cfg_data) == 0) {
		*rate = clock_get_frequency(hpm_clock_name_map[cfg_data.clock_name]);
	} else {
		return -EPERM;
	}
	return 0;
}

static int clock_control_hpmicro_configure(const struct device *dev,
					  clock_control_subsys_t sys,
					  void *data)
{
	struct hpm_clock_configure_data cfg_data;

	if (hpmicro_check_sys_data(sys, &cfg_data) != 0) {
		return -EPERM;
	}
	clock_set_source_divider(hpm_clock_name_map[cfg_data.clock_name],
	 hpm_clock_src_map[cfg_data.clock_src], cfg_data.clock_div);
	return 0;
}

static const struct clock_control_hpmicro_config config = {
	.base = (PLLCTL_Type *)DT_INST_REG_ADDR(0),
	.freq = DT_INST_PROP(0, clock_frequency),
	.sys_core = DT_INST_PROP(0, clock_sys_core),
	.ram_up_time = DT_INST_PROP(0, ram_up_time),
	.sysctl_present = HPM_MAKE_SYSCTL_PRESENT(DT_INST_ENUM_IDX(0, sysctl_present)),
};

static const struct clock_control_driver_api clock_control_hpmicro_api = {
	.on = clock_control_hpmicro_on,
	.off = clock_control_hpmicro_off,
	.get_rate = clock_control_hpmicro_get_rate,
	.configure = clock_control_hpmicro_configure,
};

DEVICE_DT_INST_DEFINE(0, clock_control_hpmicro_init, NULL, NULL, &config,
		 PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		 &clock_control_hpmicro_api);

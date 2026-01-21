/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_bee_cctl

#include <stdint.h>

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>

#include <rtl_rcc.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(clock_control_bee, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

struct clock_control_bee_config {
	uint32_t reg;
};

struct apb_cfg {
	uint32_t apbperiph;
	uint32_t apbperiph_clk;
};

static const struct apb_cfg bee_apb_table[] = {
	{APBPeriph_SPIC0, APBPeriph_SPIC0_CLOCK},
	{APBPeriph_SPIC1, APBPeriph_SPIC1_CLOCK},
	{APBPeriph_SPIC2, APBPeriph_SPIC2_CLOCK},
	{APBPeriph_GDMA, APBPeriph_GDMA_CLOCK},
	{APBPeriph_SPI0_SLAVE, APBPeriph_SPI0_SLAVE_CLOCK},
	{APBPeriph_SPI1, APBPeriph_SPI1_CLOCK},
	{APBPeriph_SPI0, APBPeriph_SPI0_CLOCK},
	{APBPeriph_I2C3, APBPeriph_I2C3_CLOCK},
	{APBPeriph_I2C2, APBPeriph_I2C2_CLOCK},
	{APBPeriph_I2C1, APBPeriph_I2C1_CLOCK},
	{APBPeriph_I2C0, APBPeriph_I2C0_CLOCK},
	{APBPeriph_UART3, APBPeriph_UART3_CLOCK},
	{APBPeriph_UART2, APBPeriph_UART2_CLOCK},
	{APBPeriph_UART1, APBPeriph_UART1_CLOCK},
	{APBPeriph_UART0, APBPeriph_UART0_CLOCK},
	{APBPeriph_ACCXTAL, APBPeriph_ACCXTAL_CLOCK},
	{APBPeriph_PDCK, APBPeriph_PDCK_CLOCK},
	{APBPeriph_ZBMAC, APBPeriph_ZBMAC_CLOCK},
	{APBPeriph_BTPHY, APBPeriph_BTPHY_CLOCK},
	{APBPeriph_BTMAC, APBPeriph_BTMAC_CLOCK},
	{APBPeriph_SEGCOM, APBPeriph_SEGCOM_CLOCK},
	{APBPeriph_SPI3W, APBPeriph_SPI3W_CLOCK},
	{APBPeriph_ETH, APBPeriph_ETH_CLOCK},
	{APBPeriph_PPE, APBPeriph_PPE_CLOCK},
	{APBPeriph_KEYSCAN, APBPeriph_KEYSCAN_CLOCK},
	{APBPeriph_HRADC, APBPeriph_HRADC_CLOCK},
	{APBPeriph_ADC, APBPeriph_ADC_CLOCK},
	{APBPeriph_CAN, APBPeriph_CAN_CLOCK},
	{APBPeriph_IR, APBPeriph_IR_CLOCK},
	{APBPeriph_ISO7816, APBPeriph_ISO7816_CLOCK},
	{APBPeriph_GPIOB, APBPeriph_GPIOB_CLOCK},
	{APBPeriph_GPIOA, APBPeriph_GPIOA_CLOCK},
	{APBPeriph_DISP, APBPeriph_DISP_CLOCK},
	{APBPeriph_IDU, APBPeriph_IDU_CLOCK},
	{APBPeriph_TIMER, APBPeriph_TIMER_CLOCK},
	{APBPeriph_ENHTIMER, APBPeriph_ENHTIMER_CLOCK},
	{APBPeriph_ENHTIMER_PWM1, APBPeriph_ENHTIMER_PWM1_CLOCK},
	{APBPeriph_ENHTIMER_PWM0, APBPeriph_ENHTIMER_PWM0_CLOCK},
	{APBPeriph_ENHTIMER_PWM3, APBPeriph_ENHTIMER_PWM3_CLOCK},
	{APBPeriph_ENHTIMER_PWM2, APBPeriph_ENHTIMER_PWM2_CLOCK},
	{APBPeriph_SDHC, APBPeriph_SDHC_CLOCK},
	{APBPeriph_UART5, APBPeriph_UART5_CLOCK},
	{APBPeriph_UART4, APBPeriph_UART4_CLOCK},
	{APBPeriph_CODEC, APBPeriph_CODEC_CLOCK},
	{APBPeriph_I2S1, APBPeriph_I2S1_CLOCK},
	{APBPeriph_I2S0, APBPeriph_I2S0_CLOCK},
};

static int clock_control_bee_on(const struct device *dev, clock_control_subsys_t sys)
{
	uint16_t id = *(uint16_t *)sys;

	RCC_PeriphClockCmd(bee_apb_table[id].apbperiph, bee_apb_table[id].apbperiph_clk, ENABLE);
	LOG_DBG("Sys: %d, APB: %d, Clk: %d", id,
			bee_apb_table[id].apbperiph,
			bee_apb_table[id].apbperiph_clk);
	return 0;
}

static int clock_control_bee_off(const struct device *dev, clock_control_subsys_t sys)
{
	uint16_t id = *(uint16_t *)sys;

	RCC_PeriphClockCmd(bee_apb_table[id].apbperiph, bee_apb_table[id].apbperiph_clk, DISABLE);

	LOG_DBG("Sys: %d, APB: %d, Clk: %d", id,
			bee_apb_table[id].apbperiph,
			bee_apb_table[id].apbperiph_clk);
	return 0;
}

static enum clock_control_status clock_control_bee_get_status(const struct device *dev,
							      clock_control_subsys_t sys)
{
	const struct clock_control_bee_config *config = dev->config;
	uint16_t id = *(uint16_t *)sys;

	uint32_t apb_reg_off = (bee_apb_table[id].apbperiph & (0xff));
	uint32_t clk_func = bee_apb_table[id].apbperiph_clk;

	if (bee_apb_table[id].apbperiph == APBPeriph_CODEC) {
		if (sys_test_bit(PERIBLKCTRL_AUDIO_REG_BASE + apb_reg_off, clk_func) != 0) {
			return CLOCK_CONTROL_STATUS_ON;
		}
	} else {
		if (sys_test_bit(config->reg + apb_reg_off, clk_func) != 0) {
			LOG_DBG("Sys: %d, Status: ON", id);
			return CLOCK_CONTROL_STATUS_ON;
		}
	}

	LOG_DBG("Sys: %d, Status: OFF", id);
	return CLOCK_CONTROL_STATUS_OFF;
}

static DEVICE_API(clock_control, clock_control_bee_api) = {
	.on = clock_control_bee_on,
	.off = clock_control_bee_off,
	.get_status = clock_control_bee_get_status,
};

static const struct clock_control_bee_config config = {
	.reg = DT_INST_REG_ADDR(0),
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, &config, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &clock_control_bee_api);

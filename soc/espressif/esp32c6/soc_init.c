/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdbool.h>
#include <assert.h>
#include "soc_init.h"
#include <soc/soc.h>
#include <soc/lp_analog_peri_reg.h>
#include <hal/clk_tree_ll.h>
#include <esp32c6/rom/spi_flash.h>
#include <modem/modem_lpcon_reg.h>

void soc_hw_init(void)
{
	/* In 80MHz flash mode, ROM sets the mspi module clk divider to 2, fix it here */
#if CONFIG_ESPTOOLPY_FLASHFREQ_80M
	clk_ll_mspi_fast_set_hs_divider(6);
	esp_rom_spiflash_config_clk(1, 0);
	esp_rom_spiflash_config_clk(1, 1);
	esp_rom_spiflash_fix_dummylen(0, 1);
	esp_rom_spiflash_fix_dummylen(1, 1);
#endif

	/* Enable analog i2c master clock */
	SET_PERI_REG_MASK(MODEM_LPCON_CLK_CONF_REG, MODEM_LPCON_CLK_I2C_MST_EN);
	SET_PERI_REG_MASK(MODEM_LPCON_I2C_MST_CLK_CONF_REG, MODEM_LPCON_CLK_I2C_MST_SEL_160M);
}

void ana_super_wdt_reset_config(bool enable)
{
	/* C6 doesn't support bypass super WDT reset */
	assert(enable);
	REG_CLR_BIT(LP_ANALOG_PERI_LP_ANA_FIB_ENABLE_REG, LP_ANALOG_PERI_LP_ANA_FIB_SUPER_WDT_RST);
}

void ana_bod_reset_config(bool enable)
{
	REG_CLR_BIT(LP_ANALOG_PERI_LP_ANA_FIB_ENABLE_REG, LP_ANALOG_PERI_LP_ANA_FIB_BOD_RST);

	if (enable) {
		REG_SET_BIT(LP_ANALOG_PERI_LP_ANA_BOD_MODE1_CNTL_REG,
			    LP_ANALOG_PERI_LP_ANA_BOD_MODE1_RESET_ENA);
	} else {
		REG_CLR_BIT(LP_ANALOG_PERI_LP_ANA_BOD_MODE1_CNTL_REG,
			    LP_ANALOG_PERI_LP_ANA_BOD_MODE1_RESET_ENA);
	}
}

void ana_reset_config(void)
{
	ana_super_wdt_reset_config(true);
	ana_bod_reset_config(true);
}

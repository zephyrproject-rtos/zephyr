/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdbool.h>
#include <soc/rtc_cntl_reg.h>
#include <esp_private/regi2c_ctrl.h>
#include <soc/regi2c_lp_bias.h>
#include <soc/regi2c_bias.h>
#include <hal/efuse_hal.h>
#include <soc/chip_revision.h>

void soc_hw_init(void)
{
	if (!ESP_CHIP_REV_ABOVE(efuse_hal_chip_revision(), 3)) {
		REGI2C_WRITE_MASK(I2C_ULP, I2C_ULP_IR_FORCE_XPD_IPH, 1);
		REGI2C_WRITE_MASK(I2C_BIAS, I2C_BIAS_DREG_1P1_PVT, 12);
	}
}

void ana_super_wdt_reset_config(bool enable)
{
	REG_CLR_BIT(RTC_CNTL_FIB_SEL_REG, RTC_CNTL_FIB_SUPER_WDT_RST);

	if (enable) {
		REG_CLR_BIT(RTC_CNTL_SWD_CONF_REG, RTC_CNTL_SWD_BYPASS_RST);
	} else {
		REG_SET_BIT(RTC_CNTL_SWD_CONF_REG, RTC_CNTL_SWD_BYPASS_RST);
	}
}

void ana_bod_reset_config(bool enable)
{
	REG_CLR_BIT(RTC_CNTL_FIB_SEL_REG, RTC_CNTL_FIB_BOD_RST);

	if (enable) {
		REG_SET_BIT(RTC_CNTL_BROWN_OUT_REG, RTC_CNTL_BROWN_OUT_ANA_RST_EN);
	} else {
		REG_CLR_BIT(RTC_CNTL_BROWN_OUT_REG, RTC_CNTL_BROWN_OUT_ANA_RST_EN);
	}
}

void ana_clock_glitch_reset_config(bool enable)
{
	REG_CLR_BIT(RTC_CNTL_FIB_SEL_REG, RTC_CNTL_FIB_GLITCH_RST);

	if (enable) {
		REG_SET_BIT(RTC_CNTL_ANA_CONF_REG, RTC_CNTL_GLITCH_RST_EN);
	} else {
		REG_CLR_BIT(RTC_CNTL_ANA_CONF_REG, RTC_CNTL_GLITCH_RST_EN);
	}
}

void ana_reset_config(void)
{
	ana_super_wdt_reset_config(true);

	/* For origin chip & ECO1: brownout & clock glitch reset not available
	 * For ECO2: fix brownout reset bug
	 * For ECO3: fix clock glitch reset bug
	 */
	switch (efuse_hal_chip_revision()) {
	case 0:
	case 1:
		/* Disable BOD and GLITCH reset */
		ana_bod_reset_config(false);
		ana_clock_glitch_reset_config(false);
		break;
	case 2:
		/* Enable BOD reset. Disable GLITCH reset */
		ana_bod_reset_config(true);
		ana_clock_glitch_reset_config(false);
		break;
	case 3:
	default:
		/* Enable BOD, and GLITCH reset */
		ana_bod_reset_config(true);
		ana_clock_glitch_reset_config(true);
		break;
	}
}

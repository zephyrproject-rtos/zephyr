/*********************************************************************
 *   @file   LTC268X.h
 *   @brief  Header file of LTC2686/8 Driver
 *   @author Mircea Caprioru (mircea.caprioru@analog.com)
 *   @author Meta Platforms, Inc. (adaptations/modifications for Zephyr OS)
 **********************************************************************
 * Copyright 2021(c) Analog Devices, Inc.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  - Neither the name of Analog Devices, Inc. nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *  - The use of this software may or may not infringe the patent rights
 *    of one or more patent holders.  This license does not release you
 *    from the requirement that you obtain separate licenses from these
 *    patent holders to use this software.
 *  - Use of the software either in source or binary form, must be run
 *    on or directly connected to an Analog Devices Inc. component.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ANALOG DEVICES BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, INTELLECTUAL PROPERTY RIGHTS, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ********************************************************************/
#ifndef __LTC268X_H__
#define __LTC268X_H__

#include <errno.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/dac.h>

#if DT_HAS_COMPAT_STATUS_OKAY(lltc_ltc2688) || DT_HAS_COMPAT_STATUS_OKAY(lltc_ltc2686)

/*********************************************************************/
/******************* Macros and Constants Definitions ****************/
/*********************************************************************/
#define LTC268X_CHANNEL_SEL(x, id) (id ? x : (x << 1))

#define LTC268X_CMD_CH_CODE(x, id)	      (0x00 + LTC268X_CHANNEL_SEL(x, id))
#define LTC268X_CMD_CH_SETTING(x, id)	      (0x10 + LTC268X_CHANNEL_SEL(x, id))
#define LTC268X_CMD_CH_OFFSET(x, id)	      (0X20 + LTC268X_CHANNEL_SEL(x, id))
#define LTC268X_CMD_CH_GAIN(x, id)	      (0x30 + LTC268X_CHANNEL_SEL(x, id))
#define LTC268X_CMD_CH_CODE_UPDATE(x, id)     (0x40 + LTC268X_CHANNEL_SEL(x, id))
#define LTC268X_CMD_CH_CODE_UPDATE_ALL(x, id) (0x50 + LTC268X_CHANNEL_SEL(x, id))
#define LTC268X_CMD_CH_UPDATE(x, id)	      (0x60 + LTC268X_CHANNEL_SEL(x, id))

#define LTC268X_CMD_CONFIG_REG		 0x70
#define LTC268X_CMD_POWERDOWN_REG	 0x71
#define LTC268X_CMD_A_B_SELECT_REG	 0x72
#define LTC268X_CMD_SW_TOGGLE_REG	 0x73
#define LTC268X_CMD_TOGGLE_DITHER_EN_REG 0x74
#define LTC268X_CMD_MUX_CTRL_REG	 0x75
#define LTC268X_CMD_FAULT_REG		 0x76
#define LTC268X_CMD_CODE_ALL		 0x78
#define LTC268X_CMD_CODE_UPDATE_ALL	 0x79
#define LTC268X_CMD_SETTING_ALL		 0x7A
#define LTC268X_CMD_SETTING_UPDATE_ALL	 0x7B
#define LTC268X_CMD_UPDATE_ALL		 0x7C
#define LTC268X_CMD_NOOP		 0xFF

#define LTC268X_READ_OPERATION 0x80

/* Channel Settings */
#define LTC268X_CH_SPAN_MSK    GENMASK(3, 0)
#define LTC268X_CH_SPAN(x)     FIELD_PREP(LTC268X_CH_SPAN_MSK, x)
#define LTC268X_CH_TD_SEL_MSK  GENMASK(5, 4)
#define LTC268X_CH_TD_SEL(x)   FIELD_PREP(LTC268X_CH_TD_SEL_MSK, x)
#define LTC268X_CH_DIT_PER_MSK GENMASK(8, 6)
#define LTC268X_CH_DIT_PER(x)  FIELD_PREP(LTC268X_CH_DIT_PER_MSK, x)
#define LTC268X_CH_DIT_PH_MSK  GENMASK(10, 9)
#define LTC268X_CH_DIT_PH(x)   FIELD_PREP(LTC268X_CH_DIT_PH_MSK, x)
#define LTC268X_CH_MODE	       BIT(11)

/* Configuration register */
#define LTC268X_CONFIG_RST BIT(15)

#define LTC268X_PWDN(x)	   (1 << ((x) & 0xF))
#define LTC268X_DITH_EN(x) (1 << ((x) & 0xF))

/*********************************************************************/
/*************************** Types Declarations **********************/
/*********************************************************************/
enum ltc268x_voltage_range {
	LTC268X_VOLTAGE_RANGE_0V_5V,
	LTC268X_VOLTAGE_RANGE_0V_10V,
	LTC268X_VOLTAGE_RANGE_M5V_5V,
	LTC268X_VOLTAGE_RANGE_M10V_10V,
	LTC268X_VOLTAGE_RANGE_M15V_15V,
};

struct ltc268x_span_tbl {
	int min;
	int max;
};

enum ltc268x_dither_period {
	LTC268X_DITH_PERIOD_4,
	LTC268X_DITH_PERIOD_8,
	LTC268X_DITH_PERIOD_16,
	LTC268X_DITH_PERIOD_32,
	LTC268X_DITH_PERIOD_64
};

enum ltc268x_dither_phase {
	LTC268X_DITH_PHASE_0,
	LTC268X_DITH_PHASE_90,
	LTC268X_DITH_PHASE_180,
	LTC268X_DITH_PHASE_270
};

enum ltc268x_a_b_register {
	LTC268X_SELECT_A_REG,
	LTC268X_SELECT_B_REG
};

enum ltc268x_clk_input {
	LTC268X_SOFT_TGL,
	LTC268X_TGP0,
	LTC268X_TGP1,
	LTC268X_TGP2
};

enum ltc268x_configice_id {
	LTC2686 = 0,
	LTC2688 = 1
};

// build-time configuration
struct ltc268x_config {
	struct spi_dt_spec bus;
	enum ltc268x_configice_id dev_id;
	uint8_t nchannels;
	uint8_t resolution;
};

// runtime configuration data
struct ltc268x_data {
	uint16_t pwd_dac_setting;
	uint16_t dither_toggle_en;
	bool dither_mode[16];
	uint16_t dac_code[16];
	enum ltc268x_voltage_range crt_range[16];
	enum ltc268x_dither_phase dither_phase[16];
	enum ltc268x_dither_period dither_period[16];
	enum ltc268x_clk_input clk_input[16];
	enum ltc268x_a_b_register reg_select[16];
};

/*******************************************************************/
/******************************** LTC268X **************************/
/*******************************************************************/
int32_t ltc268x_set_pwr_dac(const struct device *dev, uint16_t setting);
int32_t ltc268x_set_dither_toggle(const struct device *dev, uint16_t setting);
int32_t ltc268x_set_dither_mode(const struct device *dev, uint8_t channel, bool en);
int32_t ltc268x_set_span(const struct device *dev, uint8_t channel,
			 enum ltc268x_voltage_range range);
int32_t ltc268x_set_dither_phase(const struct device *dev, uint8_t channel,
				 enum ltc268x_dither_phase phase);
int32_t ltc268x_set_dither_period(const struct device *dev, uint8_t channel,
				  enum ltc268x_dither_period period);
int32_t ltc268x_select_tg_dith_clk(const struct device *dev, uint8_t channel,
				   enum ltc268x_clk_input clk_input);
int32_t ltc268x_select_reg(const struct device *dev, uint8_t channel,
			   enum ltc268x_a_b_register sel_reg);
int32_t ltc268x_software_reset(const struct device *dev);
int32_t ltc268x_set_voltage(const struct device *dev, uint8_t channel, float voltage);
int32_t ltc268x_software_toggle(const struct device *dev, uint8_t channel);
int32_t ltc268x_init(const struct device *dev);

#endif // DT_HAS_COMPAT_STATUS_OKAY(lltc_ltc2688) || DT_HAS_COMPAT_STATUS_OKAY(lltc_ltc2686)

#endif // __LTC268X_H__

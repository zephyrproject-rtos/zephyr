/*
 * Copyright (c) 2026 Egis Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <et171_aosmu.h>
#include <et171_otp.h>

typedef union {
	uint32_t __word;
	struct {
		/* 0x18: analog settings with lock / 29 bits + 3 reserved */
		uint32_t otp_atop_ldo30_trim: 4;
		uint32_t otp_atop_ldo_vref_trim: 3;
		uint32_t otp_atop_ldo18_trim: 4;
		uint32_t otp_atop_dldo_trim: 4;
		uint32_t otp_atop_dldo_vref_trim: 3;
		uint32_t otp_atop_osc100k_freq: 4;
		uint32_t otp_atop_osc360m_freq: 6;
		uint32_t spis_pad_mux: 1;
		uint32_t __reserved: 3;
	};
} otp_analog_option_t;

#define OTP_DATA_BASE       DT_REG_ADDR_BY_NAME(DT_INST(0, egis_et171_otp), mem)
#define OTP_DATA_ROOT_CLOCK 0x000C
#define OTP_DATA_ANALOG     0x0018

#define LL_READ_OTP_DATA(data_offset) \
	((uint32_t)sys_read32(OTP_DATA_BASE + (data_offset)))


static otp_analog_option_t et171_otp_ll_get_analog_config(void)
{
	otp_analog_option_t ret = { .__word = LL_READ_OTP_DATA(OTP_DATA_ANALOG) };

	return ret;
}

static void et171_otp_set_analog_config(otp_analog_option_t otp_analog)
{
	uint32_t reg;

	/*/ Trim OSC360M */
	reg = READ_AOSMU_REG(AOSMU_REG_ANALOG_OSC360M)
	    & (~SMU_ATOP_OSC360M_FREQ_MASK);
	reg |= (otp_analog.otp_atop_osc360m_freq << SMU_ATOP_OSC360M_FREQ_POS);
	WRITE_AOSMU_REG(AOSMU_REG_ANALOG_OSC360M, reg);

	/* Trim DLDO11 */
	reg = READ_AOSMU_REG(AOSMU_REG_ANALOG_LDO11)
	    & (~(SMU_ATOP_DLDO_TRIM_MASK | SMU_ATOP_DLDO_VERF_TRIM_MASK));
	reg |= (otp_analog.otp_atop_dldo_trim << SMU_ATOP_DLDO_TRIM_POS)
	    | (otp_analog.otp_atop_dldo_vref_trim
	       << SMU_ATOP_DLDO_VERF_TRIM_POS);
	WRITE_AOSMU_REG(AOSMU_REG_ANALOG_LDO11, reg);

	/* Trim OSC100K */
	reg = READ_AOSMU_REG(AOSMU_REG_ANALOG_OSC100K)
	    & (~SMU_ATOP_OSC100K_FREQ_MASK);
	reg |= (otp_analog.otp_atop_osc100k_freq
		<< SMU_ATOP_OSC100K_FREQ_POS);
	WRITE_AOSMU_REG(AOSMU_REG_ANALOG_OSC100K, reg);

	/* Trim LDO18 */
	reg = READ_AOSMU_REG(AOSMU_REG_ANALOG_LDO18)
	    & (~SMU_ATOP_LDO18_TRIM_MASK);
	reg |= (otp_analog.otp_atop_ldo18_trim
		<< SMU_ATOP_LDO18_TRIM_POS);
	WRITE_AOSMU_REG(AOSMU_REG_ANALOG_LDO18, reg);

	/* Trim LDO30 */
	reg = READ_AOSMU_REG(AOSMU_REG_ANALOG_LDO30)
	    & (~(SMU_ATOP_LDO30_TRIM_MASK | SMU_ATOP_LDO_VERF_TRIM_MASK));
	reg |= (otp_analog.otp_atop_ldo30_trim << SMU_ATOP_LDO30_TRIM_POS)
	    | (otp_analog.otp_atop_ldo_vref_trim
	       << SMU_ATOP_LDO_VERF_TRIM_POS);
	WRITE_AOSMU_REG(AOSMU_REG_ANALOG_LDO30, reg);
}

void et171_otp_ll_load_analog_config(void)
{
	et171_otp_set_analog_config(et171_otp_ll_get_analog_config());
}

uint32_t et171_otp_ll_get_root_clock(void)
{
	return LL_READ_OTP_DATA(OTP_DATA_ROOT_CLOCK);
}

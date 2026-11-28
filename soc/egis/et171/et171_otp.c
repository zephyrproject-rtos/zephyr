/*
 * Copyright (c) 2026 Egis Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include "et171_aosmu.h"
#include "et171_otp.h"

#define OTP_ATOP_LDO30_TRIM     GENMASK(3, 0)
#define OTP_ATOP_LDO_VREF_TRIM  GENMASK(6, 4)
#define OTP_ATOP_LDO18_TRIM     GENMASK(10, 7)
#define OTP_ATOP_DLDO_TRIM      GENMASK(14, 11)
#define OTP_ATOP_DLDO_VREF_TRIM GENMASK(17, 15)
#define OTP_ATOP_OSC100K_FREQ   GENMASK(21, 18)
#define OTP_ATOP_OSC360M_FREQ   GENMASK(27, 22)
#define OTP_SPIS_PAD_MUX        GENMASK(28, 28)

#define OTP_DATA_BASE              DT_REG_ADDR_BY_NAME(DT_INST(0, egis_et171_otp), mem)
#define OTP_DATA_OFFSET_ROOT_CLOCK DT_REG_ADDR(DT_NODELABEL(root_clk))
#define OTP_DATA_OFFSET_ANALOG     DT_REG_ADDR(DT_NODELABEL(analog_cfg))

static inline uint32_t otp_ll_read_data(uint32_t data_offset)
{
	return sys_read32(OTP_DATA_BASE + data_offset);
}

static uint32_t et171_otp_ll_get_analog_config(void)
{
	return otp_ll_read_data(OTP_DATA_OFFSET_ANALOG);
}

static void et171_otp_ll_set_analog_config(uint32_t analog_cfg)
{
	uint32_t reg;

	/*/ Trim OSC360M */
	reg = READ_AOSMU_REG(AOSMU_REG_ANALOG_OSC360M)
	    & (~SMU_ATOP_OSC360M_FREQ_MASK);
	reg |= FIELD_PREP(SMU_ATOP_OSC360M_FREQ_MASK,
			  FIELD_GET(OTP_ATOP_OSC360M_FREQ, analog_cfg));
	WRITE_AOSMU_REG(AOSMU_REG_ANALOG_OSC360M, reg);

	/* Trim DLDO11 */
	reg = READ_AOSMU_REG(AOSMU_REG_ANALOG_LDO11)
	    & (~(SMU_ATOP_DLDO_TRIM_MASK | SMU_ATOP_DLDO_VERF_TRIM_MASK));
	reg |= FIELD_PREP(SMU_ATOP_DLDO_TRIM_MASK,
			  FIELD_GET(OTP_ATOP_DLDO_TRIM, analog_cfg))
	    | FIELD_PREP(SMU_ATOP_DLDO_VERF_TRIM_MASK,
			 FIELD_GET(OTP_ATOP_DLDO_VREF_TRIM, analog_cfg));
	WRITE_AOSMU_REG(AOSMU_REG_ANALOG_LDO11, reg);

	/* Trim OSC100K */
	reg = READ_AOSMU_REG(AOSMU_REG_ANALOG_OSC100K)
	    & (~SMU_ATOP_OSC100K_FREQ_MASK);
	reg |= FIELD_PREP(SMU_ATOP_OSC100K_FREQ_MASK,
			  FIELD_GET(OTP_ATOP_OSC100K_FREQ, analog_cfg));
	WRITE_AOSMU_REG(AOSMU_REG_ANALOG_OSC100K, reg);

	/* Trim LDO18 */
	reg = READ_AOSMU_REG(AOSMU_REG_ANALOG_LDO18)
	    & (~SMU_ATOP_LDO18_TRIM_MASK);
	reg |= FIELD_PREP(SMU_ATOP_LDO18_TRIM_MASK,
			  FIELD_GET(OTP_ATOP_LDO18_TRIM, analog_cfg));
	WRITE_AOSMU_REG(AOSMU_REG_ANALOG_LDO18, reg);

	/* Trim LDO30 */
	reg = READ_AOSMU_REG(AOSMU_REG_ANALOG_LDO30)
	    & (~(SMU_ATOP_LDO30_TRIM_MASK | SMU_ATOP_LDO_VERF_TRIM_MASK));
	reg |= FIELD_PREP(SMU_ATOP_LDO30_TRIM_MASK,
			  FIELD_GET(OTP_ATOP_LDO30_TRIM, analog_cfg))
	    | FIELD_PREP(SMU_ATOP_LDO_VERF_TRIM_MASK,
			 FIELD_GET(OTP_ATOP_LDO_VREF_TRIM, analog_cfg));
	WRITE_AOSMU_REG(AOSMU_REG_ANALOG_LDO30, reg);
}

void et171_otp_ll_apply_analog_config(void)
{
	et171_otp_ll_set_analog_config(et171_otp_ll_get_analog_config());
}

uint32_t et171_otp_ll_get_root_clock(void)
{
	return otp_ll_read_data(OTP_DATA_OFFSET_ROOT_CLOCK);
}

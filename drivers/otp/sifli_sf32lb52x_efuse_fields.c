/*
 * Copyright (c) 2026 SiFli Technologies(Nanjing) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/sys/util.h>

#include <zephyr/drivers/otp/sifli_sf32lb52x_efuse.h>

struct sf32lb52x_efuse_bitfield {
	uint16_t pos;
	uint8_t bits;
};

static const struct sf32lb52x_efuse_bitfield sf32lb52x_efuse_bank1_fields[] = {
	/* bank1描述 (52x_Efuse map_20241029.xlsx) */
	[SF32LB52X_EFUSE_BANK1_FIELD_BUCK_VOS_TRIM] = {.pos = 0, .bits = 3},
	[SF32LB52X_EFUSE_BANK1_FIELD_BUCK_VOS_POLAR] = {.pos = 3, .bits = 1},
	[SF32LB52X_EFUSE_BANK1_FIELD_HPSYS_LDO_VOUT] = {.pos = 4, .bits = 4},
	[SF32LB52X_EFUSE_BANK1_FIELD_LPSYS_LDO_VOUT] = {.pos = 8, .bits = 4},
	[SF32LB52X_EFUSE_BANK1_FIELD_VRET_TRIM] = {.pos = 12, .bits = 4},
	[SF32LB52X_EFUSE_BANK1_FIELD_LDO18_VREF_SEL] = {.pos = 16, .bits = 4},
	[SF32LB52X_EFUSE_BANK1_FIELD_VDD33_LDO2_VOUT] = {.pos = 20, .bits = 4},
	[SF32LB52X_EFUSE_BANK1_FIELD_VDD33_LDO3_VOUT] = {.pos = 24, .bits = 4},
	[SF32LB52X_EFUSE_BANK1_FIELD_AON_VOS_TRIM] = {.pos = 28, .bits = 4},
	[SF32LB52X_EFUSE_BANK1_FIELD_AON_VOS_POLAR] = {.pos = 31, .bits = 1},
	[SF32LB52X_EFUSE_BANK1_FIELD_ADC_VOL1_REG] = {.pos = 32, .bits = 12},
	[SF32LB52X_EFUSE_BANK1_FIELD_VOLT1_100MV] = {.pos = 44, .bits = 5},
	[SF32LB52X_EFUSE_BANK1_FIELD_ADC_VOL2_REG] = {.pos = 49, .bits = 12},
	[SF32LB52X_EFUSE_BANK1_FIELD_VOLT2_100MV] = {.pos = 61, .bits = 5},
	[SF32LB52X_EFUSE_BANK1_FIELD_VBAT_REG] = {.pos = 66, .bits = 12},
	[SF32LB52X_EFUSE_BANK1_FIELD_VBAT_VOLT_100MV] = {.pos = 78, .bits = 6},
	[SF32LB52X_EFUSE_BANK1_FIELD_PROG_V1P2] = {.pos = 84, .bits = 4},
	[SF32LB52X_EFUSE_BANK1_FIELD_CV_VCTRL] = {.pos = 88, .bits = 6},
	[SF32LB52X_EFUSE_BANK1_FIELD_CC_MN] = {.pos = 94, .bits = 5},
	[SF32LB52X_EFUSE_BANK1_FIELD_CC_MP] = {.pos = 99, .bits = 5},
	[SF32LB52X_EFUSE_BANK1_FIELD_BUCK_VOS_TRIM2] = {.pos = 104, .bits = 3},
	[SF32LB52X_EFUSE_BANK1_FIELD_BUCK_VOS_POLAR2] = {.pos = 107, .bits = 1},
	[SF32LB52X_EFUSE_BANK1_FIELD_HPSYS_LDO_VOUT2] = {.pos = 108, .bits = 4},
	[SF32LB52X_EFUSE_BANK1_FIELD_LPSYS_LDO_VOUT2] = {.pos = 112, .bits = 4},
	[SF32LB52X_EFUSE_BANK1_FIELD_VBAT_STEP] = {.pos = 116, .bits = 8},
	[SF32LB52X_EFUSE_BANK1_FIELD_ERD_CAL_DONE] = {.pos = 125, .bits = 1},
	[SF32LB52X_EFUSE_BANK1_FIELD_PA_BM] = {.pos = 126, .bits = 2},
	[SF32LB52X_EFUSE_BANK1_FIELD_DAC_LSB_CNT] = {.pos = 128, .bits = 2},
	[SF32LB52X_EFUSE_BANK1_FIELD_TMXCAP_FLAG] = {.pos = 130, .bits = 1},
	[SF32LB52X_EFUSE_BANK1_FIELD_TMXCAP_CH78] = {.pos = 131, .bits = 4},
	[SF32LB52X_EFUSE_BANK1_FIELD_TMXCAP_CH00] = {.pos = 135, .bits = 4},
	[SF32LB52X_EFUSE_BANK1_FIELD_VREF_FLAG] = {.pos = 139, .bits = 1},
	[SF32LB52X_EFUSE_BANK1_FIELD_VREF_REG] = {.pos = 140, .bits = 4},

	/* Vol2 fields */
	[SF32LB52X_EFUSE_BANK1_FIELD_BUCK_VOS_TRIM_VOL2] = {.pos = 160, .bits = 3},
	[SF32LB52X_EFUSE_BANK1_FIELD_BUCK_VOS_POLAR_VOL2] = {.pos = 163, .bits = 1},
	[SF32LB52X_EFUSE_BANK1_FIELD_HPSYS_LDO_VOUT_VOL2] = {.pos = 164, .bits = 4},
	[SF32LB52X_EFUSE_BANK1_FIELD_LPSYS_LDO_VOUT_VOL2] = {.pos = 168, .bits = 4},
	[SF32LB52X_EFUSE_BANK1_FIELD_VRET_TRIM_VOL2] = {.pos = 172, .bits = 4},
	[SF32LB52X_EFUSE_BANK1_FIELD_ADC_VOL1_REG_VOL2] = {.pos = 176, .bits = 12},
	[SF32LB52X_EFUSE_BANK1_FIELD_VOLT1_100MV_VOL2] = {.pos = 188, .bits = 5},
	[SF32LB52X_EFUSE_BANK1_FIELD_ADC_VOL2_REG_VOL2] = {.pos = 193, .bits = 12},
	[SF32LB52X_EFUSE_BANK1_FIELD_VOLT2_100MV_VOL2] = {.pos = 205, .bits = 5},
	[SF32LB52X_EFUSE_BANK1_FIELD_VBAT_REG_VOL2] = {.pos = 210, .bits = 12},
	[SF32LB52X_EFUSE_BANK1_FIELD_VBAT_VOLT_100MV_VOL2] = {.pos = 222, .bits = 6},
	[SF32LB52X_EFUSE_BANK1_FIELD_HPSYS_LDO_VOUT2_VOL2] = {.pos = 228, .bits = 4},
	[SF32LB52X_EFUSE_BANK1_FIELD_ERD_CAL_VOL2_FLAG] = {.pos = 232, .bits = 1},
	[SF32LB52X_EFUSE_BANK1_FIELD_PA_BM_VOL2] = {.pos = 233, .bits = 2},
	[SF32LB52X_EFUSE_BANK1_FIELD_DAC_LSB_CNT_VOL2] = {.pos = 235, .bits = 2},
	[SF32LB52X_EFUSE_BANK1_FIELD_TMXCAP_VOL2_FLAG] = {.pos = 237, .bits = 1},
	[SF32LB52X_EFUSE_BANK1_FIELD_TMXCAP_CH78_VOL2] = {.pos = 238, .bits = 4},
	[SF32LB52X_EFUSE_BANK1_FIELD_TMXCAP_CH00_VOL2] = {.pos = 242, .bits = 4},
};

int sf32lb52x_efuse_bank1_extract(const uint8_t bank1[SF32LB52X_EFUSE_BANK1_SIZE],
				  enum sf32lb52x_efuse_bank1_field field, uint32_t *out)
{
	const struct sf32lb52x_efuse_bitfield *desc;
	const size_t bank_bits = SF32LB52X_EFUSE_BANK1_SIZE * 8U;
	uint32_t value = 0;

	if (bank1 == NULL || out == NULL) {
		return -EINVAL;
	}

	if ((unsigned int)field >= SF32LB52X_EFUSE_BANK1_FIELD_COUNT) {
		return -EINVAL;
	}

	if (ARRAY_SIZE(sf32lb52x_efuse_bank1_fields) != SF32LB52X_EFUSE_BANK1_FIELD_COUNT) {
		return -EINVAL;
	}

	desc = &sf32lb52x_efuse_bank1_fields[field];

	if (desc->bits == 0U || desc->bits > 32U) {
		return -EINVAL;
	}

	if ((size_t)desc->pos + (size_t)desc->bits > bank_bits) {
		return -EINVAL;
	}

	for (uint8_t i = 0U; i < desc->bits; i++) {
		const uint16_t bit_index = desc->pos + i;
		const uint8_t byte = bit_index / 8U;
		const uint8_t bit = bit_index % 8U;

		if ((bank1[byte] & BIT(bit)) != 0U) {
			value |= BIT(i);
		}
	}

	*out = value;

	return 0;
}


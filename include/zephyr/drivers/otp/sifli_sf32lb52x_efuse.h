/*
 * Copyright (c) 2026 SiFli Technologies(Nanjing) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_OTP_SIFLI_SF32LB52X_EFUSE_H_
#define ZEPHYR_INCLUDE_DRIVERS_OTP_SIFLI_SF32LB52X_EFUSE_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SF32LB52X_EFUSE_UID_SIZE   16U
#define SF32LB52X_EFUSE_BANK1_SIZE 32U

enum sf32lb52x_efuse_bank1_field {
	SF32LB52X_EFUSE_BANK1_FIELD_BUCK_VOS_TRIM = 0,
	SF32LB52X_EFUSE_BANK1_FIELD_BUCK_VOS_POLAR,
	SF32LB52X_EFUSE_BANK1_FIELD_HPSYS_LDO_VOUT,
	SF32LB52X_EFUSE_BANK1_FIELD_LPSYS_LDO_VOUT,
	SF32LB52X_EFUSE_BANK1_FIELD_VRET_TRIM,
	SF32LB52X_EFUSE_BANK1_FIELD_LDO18_VREF_SEL,
	SF32LB52X_EFUSE_BANK1_FIELD_VDD33_LDO2_VOUT,
	SF32LB52X_EFUSE_BANK1_FIELD_VDD33_LDO3_VOUT,
	SF32LB52X_EFUSE_BANK1_FIELD_AON_VOS_TRIM,
	SF32LB52X_EFUSE_BANK1_FIELD_AON_VOS_POLAR,
	SF32LB52X_EFUSE_BANK1_FIELD_ADC_VOL1_REG,
	SF32LB52X_EFUSE_BANK1_FIELD_VOLT1_100MV,
	SF32LB52X_EFUSE_BANK1_FIELD_ADC_VOL2_REG,
	SF32LB52X_EFUSE_BANK1_FIELD_VOLT2_100MV,
	SF32LB52X_EFUSE_BANK1_FIELD_VBAT_REG,
	SF32LB52X_EFUSE_BANK1_FIELD_VBAT_VOLT_100MV,
	SF32LB52X_EFUSE_BANK1_FIELD_PROG_V1P2,
	SF32LB52X_EFUSE_BANK1_FIELD_CV_VCTRL,
	SF32LB52X_EFUSE_BANK1_FIELD_CC_MN,
	SF32LB52X_EFUSE_BANK1_FIELD_CC_MP,
	SF32LB52X_EFUSE_BANK1_FIELD_BUCK_VOS_TRIM2,
	SF32LB52X_EFUSE_BANK1_FIELD_BUCK_VOS_POLAR2,
	SF32LB52X_EFUSE_BANK1_FIELD_HPSYS_LDO_VOUT2,
	SF32LB52X_EFUSE_BANK1_FIELD_LPSYS_LDO_VOUT2,
	SF32LB52X_EFUSE_BANK1_FIELD_VBAT_STEP,
	SF32LB52X_EFUSE_BANK1_FIELD_ERD_CAL_DONE,
	SF32LB52X_EFUSE_BANK1_FIELD_PA_BM,
	SF32LB52X_EFUSE_BANK1_FIELD_DAC_LSB_CNT,
	SF32LB52X_EFUSE_BANK1_FIELD_TMXCAP_FLAG,
	SF32LB52X_EFUSE_BANK1_FIELD_TMXCAP_CH78,
	SF32LB52X_EFUSE_BANK1_FIELD_TMXCAP_CH00,
	SF32LB52X_EFUSE_BANK1_FIELD_VREF_FLAG,
	SF32LB52X_EFUSE_BANK1_FIELD_VREF_REG,

	SF32LB52X_EFUSE_BANK1_FIELD_BUCK_VOS_TRIM_VOL2,
	SF32LB52X_EFUSE_BANK1_FIELD_BUCK_VOS_POLAR_VOL2,
	SF32LB52X_EFUSE_BANK1_FIELD_HPSYS_LDO_VOUT_VOL2,
	SF32LB52X_EFUSE_BANK1_FIELD_LPSYS_LDO_VOUT_VOL2,
	SF32LB52X_EFUSE_BANK1_FIELD_VRET_TRIM_VOL2,
	SF32LB52X_EFUSE_BANK1_FIELD_ADC_VOL1_REG_VOL2,
	SF32LB52X_EFUSE_BANK1_FIELD_VOLT1_100MV_VOL2,
	SF32LB52X_EFUSE_BANK1_FIELD_ADC_VOL2_REG_VOL2,
	SF32LB52X_EFUSE_BANK1_FIELD_VOLT2_100MV_VOL2,
	SF32LB52X_EFUSE_BANK1_FIELD_VBAT_REG_VOL2,
	SF32LB52X_EFUSE_BANK1_FIELD_VBAT_VOLT_100MV_VOL2,
	SF32LB52X_EFUSE_BANK1_FIELD_HPSYS_LDO_VOUT2_VOL2,
	SF32LB52X_EFUSE_BANK1_FIELD_ERD_CAL_VOL2_FLAG,
	SF32LB52X_EFUSE_BANK1_FIELD_PA_BM_VOL2,
	SF32LB52X_EFUSE_BANK1_FIELD_DAC_LSB_CNT_VOL2,
	SF32LB52X_EFUSE_BANK1_FIELD_TMXCAP_VOL2_FLAG,
	SF32LB52X_EFUSE_BANK1_FIELD_TMXCAP_CH78_VOL2,
	SF32LB52X_EFUSE_BANK1_FIELD_TMXCAP_CH00_VOL2,

	SF32LB52X_EFUSE_BANK1_FIELD_COUNT,
};

/**
 * @brief Extract a Bank1 bitfield from the raw 32-byte Bank1 data.
 *
 * Field definitions are based on "52x_Efuse map_20241029.xlsx" (sheet "bank1描述").
 *
 * Bit numbering: POS=0 corresponds to bit0 (LSB) of bank1[0]. The extracted
 * value is returned LSB-first (i.e. the lowest bit of the returned value is
 * the bit at POS).
 *
 * @param bank1 Raw Bank1 data (32 bytes).
 * @param field Field to extract.
 * @param[out] out Extracted value.
 *
 * @retval 0 on success.
 * @retval -EINVAL on invalid parameters.
 */
int sf32lb52x_efuse_bank1_extract(const uint8_t bank1[SF32LB52X_EFUSE_BANK1_SIZE],
				  enum sf32lb52x_efuse_bank1_field field, uint32_t *out);

#if defined(CONFIG_SF32LB52X_EFUSE_NVMEM) || defined(__DOXYGEN__)
/**
 * @brief Read the 16-byte UID from eFuse using the NVMEM subsystem.
 *
 * Requires a devicetree NVMEM cell labeled @c sf32lb52_uid under the eFuse
 * node.
 *
 * @param[out] uid UID output buffer (16 bytes).
 *
 * @retval 0 on success.
 * @retval <0 on errors from the underlying NVMEM/OTP provider.
 */
int sf32lb52x_efuse_read_uid(uint8_t uid[SF32LB52X_EFUSE_UID_SIZE]);

/**
 * @brief Read the raw 32-byte Bank1 calibration region using the NVMEM subsystem.
 *
 * Requires a devicetree NVMEM cell labeled @c sf32lb52_calib_bank1 under the
 * eFuse node.
 *
 * @param[out] bank1 Raw Bank1 output buffer (32 bytes).
 *
 * @retval 0 on success.
 * @retval <0 on errors from the underlying NVMEM/OTP provider.
 */
int sf32lb52x_efuse_read_bank1(uint8_t bank1[SF32LB52X_EFUSE_BANK1_SIZE]);

/**
 * @brief Read and extract a Bank1 field using the NVMEM subsystem.
 *
 * @param field Field to extract.
 * @param[out] out Extracted value.
 *
 * @retval 0 on success.
 * @retval <0 on errors.
 */
int sf32lb52x_efuse_read_bank1_field(enum sf32lb52x_efuse_bank1_field field, uint32_t *out);
#endif /* CONFIG_SF32LB52X_EFUSE_NVMEM */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_OTP_SIFLI_SF32LB52X_EFUSE_H_ */


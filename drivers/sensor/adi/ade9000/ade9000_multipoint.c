/*
 * Copyright (c) 2026 A-Eberle <Daniel.Kampert@a-eberle.de>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Multipoint gain and phase calibration of the ADE9000 at run time.
 */

#include "ade9000.h"
#include "ade9000_spi.h"

/**
 * @brief  Register codes of a multipoint table.
 */
struct ade9000_mt_codes {
	/** MTTHR_L0 to MTTHR_L4 */
	uint32_t thr_low[ADE9000_MT_REGION_COUNT];
	/** MTTHR_H0 to MTTHR_H4 */
	uint32_t thr_high[ADE9000_MT_REGION_COUNT];
	/** xIGAIN0 to xIGAIN4 per phase */
	int32_t gain[ADE9000_PHASE_COUNT][ADE9000_MT_REGION_COUNT];
	/** xPHCAL0 to xPHCAL4 per phase */
	int32_t phcal[ADE9000_PHASE_COUNT][ADE9000_MT_REGION_COUNT];
};

/**
 * @brief         Check the thresholds against the regions of data sheet
 *                figure 59: region n spans MTTHR_Ln to MTTHR_Hn, and
 *                MTTHR_L(n+1) to MTTHR_Hn is the hysteresis between region n
 *                and n + 1.
 * @param codes   Register codes of the table
 * @return        0 if the regions are in order, -EINVAL otherwise
 */
static int ade9000_mt_check(const struct ade9000_mt_codes *codes)
{
	for (size_t i = 0; i < ADE9000_MT_REGION_COUNT; i++) {
		if (codes->thr_low[i] >= codes->thr_high[i]) {
			return -EINVAL;
		}

		if (i == 0) {
			continue;
		}

		if ((codes->thr_low[i] <= codes->thr_low[i - 1]) ||
		    (codes->thr_high[i] <= codes->thr_high[i - 1]) ||
		    (codes->thr_low[i] > codes->thr_high[i - 1])) {
			return -EINVAL;
		}
	}

	return 0;
}

/**
 * @brief         Set or clear CONFIG0.MTEN.
 * @param dev     ADE9000 device
 * @param enable  true to turn the multipoint compensation on
 * @return        0 on success, negative errno of the SPI transfer otherwise
 */
static int ade9000_mt_enable(const struct device *dev, bool enable)
{
	uint32_t config0;
	int ret;

	ret = ade9000_reg_read32(dev, ADE9000_REG_CONFIG0, &config0);
	if (ret) {
		return ret;
	}

	if (enable) {
		config0 |= ADE9000_CONFIG0_MTEN;
	} else {
		config0 &= ~ADE9000_CONFIG0_MTEN;
	}

	return ade9000_reg_write32(dev, ADE9000_REG_CONFIG0, config0);
}

int ade9000_multipoint_set(const struct device *dev, const struct ade9000_multipoint *mp)
{
	struct ade9000_mt_codes codes;
	int ret;

	if (mp == NULL) {
		return -EINVAL;
	}

	/* Convert and check everything first, so a bad table leaves every register unchanged */
	for (size_t i = 0; i < ADE9000_MT_REGION_COUNT; i++) {
		ret = ade9000_mtthr_to_code(dev, &mp->thr_low[i], &codes.thr_low[i]);
		if (ret) {
			return ret;
		}

		ret = ade9000_mtthr_to_code(dev, &mp->thr_high[i], &codes.thr_high[i]);
		if (ret) {
			return ret;
		}

		for (size_t phase = 0; phase < ADE9000_PHASE_COUNT; phase++) {
			ret = ade9000_gain_to_code(&mp->phase[phase].gain[i],
						   &codes.gain[phase][i]);
			if (ret) {
				return ret;
			}

			ret = ade9000_phcal_to_code(dev, &mp->phase[phase].phcal[i],
						    &codes.phcal[phase][i]);
			if (ret) {
				return ret;
			}
		}
	}

	/* A disabled table may be incomplete, e.g. all zero after a reset */
	if (mp->enable) {
		ret = ade9000_mt_check(&codes);
		if (ret) {
			return ret;
		}
	}

	/* No compensation with a half written table */
	ret = ade9000_mt_enable(dev, false);
	if (ret) {
		return ret;
	}

	for (size_t i = 0; i < ADE9000_MT_REGION_COUNT; i++) {
		ret = ade9000_reg_write32(dev, ADE9000_REG_MTTHR_L0 + i, codes.thr_low[i]);
		if (ret) {
			return ret;
		}

		ret = ade9000_reg_write32(dev, ADE9000_REG_MTTHR_H0 + i, codes.thr_high[i]);
		if (ret) {
			return ret;
		}

		for (size_t phase = 0; phase < ADE9000_PHASE_COUNT; phase++) {
			uint16_t offset = (phase * ADE9000_STRIDE_PHASE) + i;

			ret = ade9000_reg_write32(dev, ADE9000_REG_AIGAIN0 + offset,
						  (uint32_t)codes.gain[phase][i]);
			if (ret) {
				return ret;
			}

			ret = ade9000_reg_write32(dev, ADE9000_REG_APHCAL0 + offset,
						  (uint32_t)codes.phcal[phase][i]);
			if (ret) {
				return ret;
			}
		}
	}

	if (mp->enable) {
		ret = ade9000_mt_enable(dev, true);
	}

	return ret;
}

int ade9000_multipoint_get(const struct device *dev, struct ade9000_multipoint *mp)
{
	uint32_t value32;
	int ret;

	if (mp == NULL) {
		return -EINVAL;
	}

	ret = ade9000_reg_read32(dev, ADE9000_REG_CONFIG0, &value32);
	if (ret) {
		return ret;
	}

	mp->enable = (value32 & ADE9000_CONFIG0_MTEN) != 0;

	for (size_t i = 0; i < ADE9000_MT_REGION_COUNT; i++) {
		ret = ade9000_reg_read32(dev, ADE9000_REG_MTTHR_L0 + i, &value32);
		if (ret) {
			return ret;
		}

		ret = ade9000_mtthr_from_code(dev, value32, &mp->thr_low[i]);
		if (ret) {
			return ret;
		}

		ret = ade9000_reg_read32(dev, ADE9000_REG_MTTHR_H0 + i, &value32);
		if (ret) {
			return ret;
		}

		ret = ade9000_mtthr_from_code(dev, value32, &mp->thr_high[i]);
		if (ret) {
			return ret;
		}

		for (size_t phase = 0; phase < ADE9000_PHASE_COUNT; phase++) {
			uint16_t offset = (phase * ADE9000_STRIDE_PHASE) + i;

			ret = ade9000_reg_read32(dev, ADE9000_REG_AIGAIN0 + offset, &value32);
			if (ret) {
				return ret;
			}

			ret = ade9000_gain_from_code((int32_t)value32, &mp->phase[phase].gain[i]);
			if (ret) {
				return ret;
			}

			ret = ade9000_reg_read32(dev, ADE9000_REG_APHCAL0 + offset, &value32);
			if (ret) {
				return ret;
			}

			ret = ade9000_phcal_from_code(dev, (int32_t)value32,
						      &mp->phase[phase].phcal[i]);
			if (ret) {
				return ret;
			}
		}
	}

	return 0;
}

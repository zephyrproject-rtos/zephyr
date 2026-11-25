/*
 * Copyright (c) 2026 A-Eberle <Daniel.Kampert@a-eberle.de>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Configuration of the ADE9000: calibration and operating mode from the
 *        devicetree, and the attributes of the driver.
 */

#define DT_DRV_COMPAT adi_ade9000

#include <zephyr/logging/log.h>

#include "ade9000.h"
#include "ade9000_spi.h"

#define ADE9000_REG_NONE 0xFFFF

/**
 * @brief          Entry of the raw register attribute table.
 * @param _attr    Attribute name without the SENSOR_ATTR_ADE9000_ prefix
 * @param _reg     First register without the ADE9000_REG_ prefix
 * @param _count   Number of consecutive registers
 * @param _wide16  true for 16 bit registers, false for 32 bit
 * @param _ro      true if the attribute can only be read
 */
#define ADE9000_ATTR(_attr, _reg, _count, _wide16, _ro)                                            \
	{                                                                                          \
		.attr = SENSOR_ATTR_ADE9000_##_attr,                                               \
		.reg = ADE9000_REG_##_reg,                                                         \
		.count = _count,                                                                   \
		.wide16 = _wide16,                                                                 \
		.read_only = _ro,                                                                  \
	}

/**
 * @brief  Offset registers in the order of enum ade9000_os. Three phase
 *         registers are those of phase A, phase B and C follow at
 *         ADE9000_STRIDE_PHASE.
 */
struct ade9000_os_desc {
	uint16_t reg;
	uint8_t count;
};

/**
 * @brief  Calibration registers behind the channels that accept
 *         SENSOR_ATTR_GAIN (get only), SENSOR_ATTR_OFFSET and the one cycle
 *         and 10/12 cycle offsets. The gains are those of phase A, phase B
 *         and C follow at ADE9000_STRIDE_PHASE.
 */
struct ade9000_cal_desc {
	enum sensor_channel_ade9000 chan;
	uint16_t gain;
	uint8_t os;
	uint8_t os_one;
	uint8_t os_1012;
};

/**
 * @brief  Raw register behind an attribute on SENSOR_CHAN_ALL. Attributes with
 *         a count cover that many consecutive registers.
 */
struct ade9000_reg_attr {
	enum sensor_attribute_ade9000 attr;
	uint16_t reg;
	uint8_t count;
	bool wide16;
	bool read_only;
};

/**
 * @brief  Operating mode register and the value from the devicetree.
 */
struct ade9000_mode_reg {
	uint16_t reg;
	bool wide16;
	uint32_t value;
};

static const struct ade9000_os_desc ade9000_os_regs[ADE9000_OS_COUNT] = {
	[ADE9000_OS_IRMS] = {ADE9000_REG_AIRMSOS, ADE9000_PHASE_COUNT},
	[ADE9000_OS_VRMS] = {ADE9000_REG_AVRMSOS, ADE9000_PHASE_COUNT},
	[ADE9000_OS_IFRMS] = {ADE9000_REG_AIFRMSOS, ADE9000_PHASE_COUNT},
	[ADE9000_OS_VFRMS] = {ADE9000_REG_AVFRMSOS, ADE9000_PHASE_COUNT},
	[ADE9000_OS_IRMSONE] = {ADE9000_REG_AIRMSONEOS, ADE9000_PHASE_COUNT},
	[ADE9000_OS_VRMSONE] = {ADE9000_REG_AVRMSONEOS, ADE9000_PHASE_COUNT},
	[ADE9000_OS_IRMS1012] = {ADE9000_REG_AIRMS1012OS, ADE9000_PHASE_COUNT},
	[ADE9000_OS_VRMS1012] = {ADE9000_REG_AVRMS1012OS, ADE9000_PHASE_COUNT},
	[ADE9000_OS_WATT] = {ADE9000_REG_AWATTOS, ADE9000_PHASE_COUNT},
	[ADE9000_OS_VAR] = {ADE9000_REG_AVAROS, ADE9000_PHASE_COUNT},
	[ADE9000_OS_FWATT] = {ADE9000_REG_AFWATTOS, ADE9000_PHASE_COUNT},
	[ADE9000_OS_FVAR] = {ADE9000_REG_AFVAROS, ADE9000_PHASE_COUNT},
	[ADE9000_OS_NIRMS] = {ADE9000_REG_NIRMSOS, 1},
	[ADE9000_OS_NIRMSONE] = {ADE9000_REG_NIRMSONEOS, 1},
	[ADE9000_OS_NIRMS1012] = {ADE9000_REG_NIRMS1012OS, 1},
	[ADE9000_OS_ISUMRMS] = {ADE9000_REG_ISUMRMSOS, 1},
};

static const struct ade9000_cal_desc ade9000_cal[] = {
	{SENSOR_CHAN_ADE9000_IRMS, ADE9000_REG_AIGAIN, ADE9000_OS_IRMS, ADE9000_OS_IRMSONE,
	 ADE9000_OS_IRMS1012},
	{SENSOR_CHAN_ADE9000_VRMS, ADE9000_REG_AVGAIN, ADE9000_OS_VRMS, ADE9000_OS_VRMSONE,
	 ADE9000_OS_VRMS1012},
	{SENSOR_CHAN_ADE9000_NIRMS, ADE9000_REG_NIGAIN, ADE9000_OS_NIRMS, ADE9000_OS_NIRMSONE,
	 ADE9000_OS_NIRMS1012},
	{SENSOR_CHAN_ADE9000_ISUMRMS, ADE9000_REG_NONE, ADE9000_OS_ISUMRMS, ADE9000_OS_NONE,
	 ADE9000_OS_NONE},
	{SENSOR_CHAN_ADE9000_IFRMS, ADE9000_REG_AIGAIN, ADE9000_OS_IFRMS, ADE9000_OS_NONE,
	 ADE9000_OS_NONE},
	{SENSOR_CHAN_ADE9000_VFRMS, ADE9000_REG_AVGAIN, ADE9000_OS_VFRMS, ADE9000_OS_NONE,
	 ADE9000_OS_NONE},
	{SENSOR_CHAN_ADE9000_WATT, ADE9000_REG_APGAIN, ADE9000_OS_WATT, ADE9000_OS_NONE,
	 ADE9000_OS_NONE},
	{SENSOR_CHAN_ADE9000_VAR, ADE9000_REG_APGAIN, ADE9000_OS_VAR, ADE9000_OS_NONE,
	 ADE9000_OS_NONE},
	{SENSOR_CHAN_ADE9000_FWATT, ADE9000_REG_APGAIN, ADE9000_OS_FWATT, ADE9000_OS_NONE,
	 ADE9000_OS_NONE},
	{SENSOR_CHAN_ADE9000_FVAR, ADE9000_REG_APGAIN, ADE9000_OS_FVAR, ADE9000_OS_NONE,
	 ADE9000_OS_NONE},
};

static const struct ade9000_reg_attr ade9000_reg_attrs[] = {
	ADE9000_ATTR(MASK0, MASK0, 1, false, false),
	ADE9000_ATTR(MASK1, MASK1, 1, false, false),
	ADE9000_ATTR(WFB_CFG, WFB_CFG, 1, true, false),
	ADE9000_ATTR(WFB_PG_IRQEN, WFB_PG_IRQEN, 1, true, false),
	ADE9000_ATTR(EVENT_MASK, EVENT_MASK, 1, false, false),
	ADE9000_ATTR(EVENT_STATUS, EVENT_STATUS, 1, false, true),
	ADE9000_ATTR(OILVL, OILVL, 1, false, false),
	ADE9000_ATTR(OI, OIA, 4, false, true),
	ADE9000_ATTR(OISTATUS, OISTATUS, 1, true, true),
	ADE9000_ATTR(DIP_LVL, DIP_LVL, 1, false, false),
	ADE9000_ATTR(DIP_CYC, DIP_CYC, 1, true, false),
	ADE9000_ATTR(DIP, DIPA, 3, false, true),
	ADE9000_ATTR(SWELL_LVL, SWELL_LVL, 1, false, false),
	ADE9000_ATTR(SWELL_CYC, SWELL_CYC, 1, true, false),
	ADE9000_ATTR(SWELL, SWELLA, 3, false, true),
	ADE9000_ATTR(IPEAK, IPEAK, 1, false, true),
	ADE9000_ATTR(VPEAK, VPEAK, 1, false, true),
	ADE9000_ATTR(CONFIG3, CONFIG3, 1, true, false),
	ADE9000_ATTR(ZXTOUT, ZXTOUT, 1, true, false),
	ADE9000_ATTR(ZXTHRSH, ZXTHRSH, 1, true, false),
	ADE9000_ATTR(ZX_LP_SEL, ZX_LP_SEL, 1, true, false),
	ADE9000_ATTR(SEQ_CYC, SEQ_CYC, 1, true, false),
	ADE9000_ATTR(USER_PERIOD, USER_PERIOD, 1, false, false),
	ADE9000_ATTR(COM_PERIOD, COM_PERIOD, 1, false, true),
	ADE9000_ATTR(ISUMLVL, ISUMLVL, 1, false, false),
	ADE9000_ATTR(NL_LVL, ACT_NL_LVL, 3, false, false),
	ADE9000_ATTR(PHNOLOAD, PHNOLOAD, 1, false, true),
	ADE9000_ATTR(PHSIGN, PHSIGN, 1, true, true),
	ADE9000_ATTR(PWR_TIME, PWR_TIME, 1, true, false),
	ADE9000_ATTR(WFB_TRG_CFG, WFB_TRG_CFG, 1, true, false),
	ADE9000_ATTR(WFB_TRG_STAT, WFB_TRG_STAT, 1, true, true),
	ADE9000_ATTR(CFMODE, CFMODE, 1, true, false),
	ADE9000_ATTR(COMPMODE, COMPMODE, 1, true, false),
	ADE9000_ATTR(CFDEN, CF1DEN, 4, true, false),
	ADE9000_ATTR(CF_LCFG, CF_LCFG, 1, false, false),
	ADE9000_ATTR(CF_THR, WTHR, 3, false, false),
	ADE9000_ATTR(CRC_OPTEN, CRC_OPTEN, 1, true, false),
	ADE9000_ATTR(CRC_FORCE, CRC_FORCE, 1, true, false),
	ADE9000_ATTR(CRC_RSLT, CRC_RSLT, 1, true, true),
	ADE9000_ATTR(CRC_SPI, CRC_SPI, 1, true, true),
	ADE9000_ATTR(LAST_CMD, LAST_CMD, 1, true, true),
	ADE9000_ATTR(LAST_DATA_16, LAST_DATA_16, 1, true, true),
	ADE9000_ATTR(LAST_DATA_32, LAST_DATA_32, 1, false, true),
	ADE9000_ATTR(ADC_REDIRECT, ADC_REDIRECT, 1, false, false),
	ADE9000_ATTR(WR_LOCK, WR_LOCK, 1, true, false),
};

LOG_MODULE_DECLARE(ADE9000, CONFIG_SENSOR_LOG_LEVEL);

/**
 * @brief        Look up the calibration registers of a channel.
 * @param chan   Channel
 * @return       Calibration registers, NULL if the channel has none
 */
static const struct ade9000_cal_desc *ade9000_cal_find(enum sensor_channel chan)
{
	for (size_t i = 0; i < ARRAY_SIZE(ade9000_cal); i++) {
		if ((int)ade9000_cal[i].chan == (int)chan) {
			return &ade9000_cal[i];
		}
	}

	return NULL;
}

/**
 * @brief        Look up the offset register an attribute selects on a channel.
 * @param cal    Calibration registers of the channel
 * @param attr   SENSOR_ATTR_OFFSET, SENSOR_ATTR_ADE9000_OFFSET_ONE or
 *               SENSOR_ATTR_ADE9000_OFFSET_1012
 * @return       Offset register, ADE9000_OS_NONE if there is none
 */
static uint8_t ade9000_cal_os(const struct ade9000_cal_desc *cal, enum sensor_attribute attr)
{
	switch ((int)attr) {
	case SENSOR_ATTR_OFFSET: {
		return cal->os;
	}
	case SENSOR_ATTR_ADE9000_OFFSET_ONE: {
		return cal->os_one;
	}
	case SENSOR_ATTR_ADE9000_OFFSET_1012: {
		return cal->os_1012;
	}
	default: {
		return ADE9000_OS_NONE;
	}
	}
}

/**
 * @brief        Look up the register behind an attribute on SENSOR_CHAN_ALL.
 * @param attr   Attribute
 * @return       Register, NULL if the attribute has none
 */
static const struct ade9000_reg_attr *ade9000_reg_attr_find(enum sensor_attribute attr)
{
	for (size_t i = 0; i < ARRAY_SIZE(ade9000_reg_attrs); i++) {
		if ((int)ade9000_reg_attrs[i].attr == (int)attr) {
			return &ade9000_reg_attrs[i];
		}
	}

	return NULL;
}

/**
 * @brief        Write the registers behind an attribute on SENSOR_CHAN_ALL.
 * @param dev    ADE9000 device
 * @param attr   Attribute
 * @param val    One value per register in val1
 * @return       0 on success, -ENOTSUP for unknown or read only attributes,
 *               negative errno of the SPI transfer otherwise
 */
static int ade9000_reg_attr_set(const struct device *dev, enum sensor_attribute attr,
				const struct sensor_value *val)
{
	const struct ade9000_reg_attr *ra = ade9000_reg_attr_find(attr);
	int ret;

	if ((ra == NULL) || ra->read_only) {
		return -ENOTSUP;
	}

	for (size_t i = 0; i < ra->count; i++) {
		if (ra->wide16) {
			ret = ade9000_reg_write16(dev, ra->reg + i, (uint16_t)val[i].val1);
		} else {
			ret = ade9000_reg_write32(dev, ra->reg + i, (uint32_t)val[i].val1);
		}

		if (ret) {
			return ret;
		}
	}

	return 0;
}

/**
 * @brief        Read the registers behind an attribute on SENSOR_CHAN_ALL.
 * @param dev    ADE9000 device
 * @param attr   Attribute
 * @param val    Takes one value per register in val1
 * @return       0 on success, -ENOTSUP for unknown attributes, negative errno
 *               of the SPI transfer otherwise
 */
static int ade9000_reg_attr_get(const struct device *dev, enum sensor_attribute attr,
				struct sensor_value *val)
{
	const struct ade9000_reg_attr *ra = ade9000_reg_attr_find(attr);
	uint32_t value32;
	uint16_t value16;
	int ret;

	if (ra == NULL) {
		return -ENOTSUP;
	}

	for (size_t i = 0; i < ra->count; i++) {
		if (ra->wide16) {
			ret = ade9000_reg_read16(dev, ra->reg + i, &value16);
			value32 = value16;
		} else {
			ret = ade9000_reg_read32(dev, ra->reg + i, &value32);
		}

		if (ret) {
			return ret;
		}

		val[i].val1 = (int32_t)value32;
		val[i].val2 = 0;
	}

	return 0;
}

/**
 * @brief        Change the bits of ACCMODE behind an operating mode attribute.
 *               Keeps the other bits and the cached line frequency in sync.
 * @param dev    ADE9000 device
 * @param attr   SENSOR_ATTR_ADE9000_LINE_FREQUENCY, _VCONSEL or _ICONSEL
 * @param val    New value in val1
 * @return       0 on success, -EINVAL for a value out of range, -ENOTSUP for
 *               other attributes, negative errno of the SPI transfer otherwise
 */
static int ade9000_mode_set(const struct device *dev, enum sensor_attribute attr,
			    const struct sensor_value *val)
{
	struct ade9000_data *data = dev->data;
	uint16_t mask;
	uint16_t bits;
	uint16_t accmode;
	int ret;

	switch ((int)attr) {
	case SENSOR_ATTR_ADE9000_LINE_FREQUENCY: {
		if ((val->val1 != 50) && (val->val1 != 60)) {
			return -EINVAL;
		}

		mask = ADE9000_ACCMODE_SELFREQ;
		bits = (val->val1 == 60) ? ADE9000_ACCMODE_SELFREQ : 0;
		break;
	}
	case SENSOR_ATTR_ADE9000_VCONSEL: {
		if ((val->val1 < ADE9000_DT_VCONSEL_4W_WYE) ||
		    (val->val1 > ADE9000_DT_VCONSEL_3W_DELTA_LL)) {
			return -EINVAL;
		}

		mask = ADE9000_ACCMODE_VCONSEL;
		bits = FIELD_PREP(ADE9000_ACCMODE_VCONSEL, val->val1);
		break;
	}
	case SENSOR_ATTR_ADE9000_ICONSEL: {
		if ((val->val1 != 0) && (val->val1 != 1)) {
			return -EINVAL;
		}

		mask = ADE9000_ACCMODE_ICONSEL;
		bits = (val->val1 == 1) ? ADE9000_ACCMODE_ICONSEL : 0;
		break;
	}
	default: {
		return -ENOTSUP;
	}
	}

	/* The lock keeps the read-modify-write and the cached frequency together */
	k_mutex_lock(&data->lock, K_FOREVER);

	ret = ade9000_reg_read16(dev, ADE9000_REG_ACCMODE, &accmode);
	if (ret == 0) {
		ret = ade9000_reg_write16(dev, ADE9000_REG_ACCMODE, (accmode & ~mask) | bits);
	}

	if ((ret == 0) && ((int)attr == SENSOR_ATTR_ADE9000_LINE_FREQUENCY)) {
		data->line_frequency = (uint8_t)val->val1;
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

/**
 * @brief        Read an operating mode attribute back from ACCMODE.
 * @param dev    ADE9000 device
 * @param attr   SENSOR_ATTR_ADE9000_LINE_FREQUENCY, _VCONSEL or _ICONSEL
 * @param val    Takes the value in val1
 * @return       0 on success, -ENOTSUP for other attributes, negative errno of
 *               the SPI transfer otherwise
 */
static int ade9000_mode_get(const struct device *dev, enum sensor_attribute attr,
			    struct sensor_value *val)
{
	uint16_t accmode;
	int ret;

	if (((int)attr != SENSOR_ATTR_ADE9000_LINE_FREQUENCY) &&
	    ((int)attr != SENSOR_ATTR_ADE9000_VCONSEL) &&
	    ((int)attr != SENSOR_ATTR_ADE9000_ICONSEL)) {
		return -ENOTSUP;
	}

	ret = ade9000_reg_read16(dev, ADE9000_REG_ACCMODE, &accmode);
	if (ret) {
		return ret;
	}

	switch ((int)attr) {
	case SENSOR_ATTR_ADE9000_LINE_FREQUENCY: {
		val->val1 = (accmode & ADE9000_ACCMODE_SELFREQ) ? 60 : 50;
		break;
	}
	case SENSOR_ATTR_ADE9000_VCONSEL: {
		val->val1 = FIELD_GET(ADE9000_ACCMODE_VCONSEL, accmode);
		break;
	}
	default: {
		val->val1 = (accmode & ADE9000_ACCMODE_ICONSEL) ? 1 : 0;
		break;
	}
	}

	val->val2 = 0;

	return 0;
}

/**
 * @brief        Write the gain of a calibrated channel.
 * @param dev    ADE9000 device
 * @param cal    Calibration registers of the channel
 * @param val    Gain as a factor, one value per phase of the channel
 * @return       0 on success, -ENOTSUP if the channel has no gain, -EINVAL for
 *               a factor out of range, negative errno of the SPI transfer
 *               otherwise
 */
static int ade9000_gain_set(const struct device *dev, const struct ade9000_cal_desc *cal,
			    const struct sensor_value *val)
{
	size_t count = ade9000_os_regs[cal->os].count;
	int32_t codes[ADE9000_PHASE_COUNT];
	int ret;

	if (cal->gain == ADE9000_REG_NONE) {
		return -ENOTSUP;
	}

	/* Check all phases first, so a bad value leaves every register unchanged */
	for (size_t i = 0; i < count; i++) {
		ret = ade9000_gain_to_code(&val[i], &codes[i]);
		if (ret) {
			return ret;
		}
	}

	/* A gain covers the same phases as the offset of the channel */
	for (size_t i = 0; i < count; i++) {
		ret = ade9000_reg_write32(dev, cal->gain + (i * ADE9000_STRIDE_PHASE),
					  (uint32_t)codes[i]);
		if (ret) {
			return ret;
		}
	}

	return 0;
}

/**
 * @brief      Write the gains and the phase compensation from the devicetree.
 * @param dev  ADE9000 device
 * @return     0 on success, negative errno of the SPI transfer otherwise
 */
static int ade9000_gains_write(const struct device *dev)
{
	const struct ade9000_config *cfg = dev->config;
	int ret;

	for (size_t phase = 0; phase < ADE9000_PHASE_COUNT; phase++) {
		uint16_t offset = phase * ADE9000_STRIDE_PHASE;

		for (size_t i = 0; i < ADE9000_IGAIN_COUNT; i++) {
			ret = ade9000_reg_write32(dev, ADE9000_REG_AIGAIN + offset + i,
						  (uint32_t)cfg->igain[phase][i]);
			if (ret) {
				LOG_ERR("Failed to write the current gains (err %d)", ret);

				return ret;
			}
		}

		for (size_t i = 0; i < ADE9000_MT_REGION_COUNT; i++) {
			ret = ade9000_reg_write32(dev, ADE9000_REG_APHCAL0 + offset + i,
						  (uint32_t)cfg->phcal[phase][i]);
			if (ret) {
				LOG_ERR("Failed to write the phase compensation (err %d)", ret);

				return ret;
			}
		}

		ret = ade9000_reg_write32(dev, ADE9000_REG_AVGAIN + offset,
					  (uint32_t)cfg->vgain[phase]);
		if (ret) {
			LOG_ERR("Failed to write the voltage gain (err %d)", ret);

			return ret;
		}

		ret = ade9000_reg_write32(dev, ADE9000_REG_APGAIN + offset,
					  (uint32_t)cfg->pgain[phase]);
		if (ret) {
			LOG_ERR("Failed to write the power gain (err %d)", ret);

			return ret;
		}
	}

	ret = ade9000_reg_write32(dev, ADE9000_REG_NIGAIN, (uint32_t)cfg->nigain);
	if (ret) {
		LOG_ERR("Failed to write NIGAIN (err %d)", ret);

		return ret;
	}

	ret = ade9000_reg_write32(dev, ADE9000_REG_NPHCAL, (uint32_t)cfg->nphcal);
	if (ret) {
		LOG_ERR("Failed to write NPHCAL (err %d)", ret);
	}

	return ret;
}

/**
 * @brief      Write the offsets from the devicetree.
 * @param dev  ADE9000 device
 * @return     0 on success, negative errno of the SPI transfer otherwise
 */
static int ade9000_offsets_write(const struct device *dev)
{
	const struct ade9000_config *cfg = dev->config;
	int ret;

	for (size_t os = 0; os < ADE9000_OS_COUNT; os++) {
		for (size_t i = 0; i < ade9000_os_regs[os].count; i++) {
			ret = ade9000_reg_write32(
				dev, ade9000_os_regs[os].reg + (i * ADE9000_STRIDE_PHASE),
				(uint32_t)cfg->os[os][i]);
			if (ret) {
				LOG_ERR("Failed to write the offsets (err %d)", ret);

				return ret;
			}
		}
	}

	return 0;
}

/**
 * @brief      Write the multipoint thresholds, if the devicetree enables
 *             multipoint compensation. CONFIG0.MTEN is set by
 *             ade9000_configure().
 * @param dev  ADE9000 device
 * @return     0 on success, negative errno of the SPI transfer otherwise
 */
static int ade9000_multipoint_write(const struct device *dev)
{
	const struct ade9000_config *cfg = dev->config;
	int ret;

	if (cfg->multi_point_compensation == false) {
		return 0;
	}

	for (size_t i = 0; i < ADE9000_MT_REGION_COUNT; i++) {
		ret = ade9000_reg_write32(dev, ADE9000_REG_MTTHR_L0 + i, cfg->mtthr_low[i]);
		if (ret) {
			LOG_ERR("Failed to write MTTHR_L%zu (err %d)", i, ret);

			return ret;
		}

		ret = ade9000_reg_write32(dev, ADE9000_REG_MTTHR_H0 + i, cfg->mtthr_high[i]);
		if (ret) {
			LOG_ERR("Failed to write MTTHR_H%zu (err %d)", i, ret);

			return ret;
		}
	}

	return 0;
}

/**
 * @brief        Look up the phase compensation register of a channel.
 * @param chan   SENSOR_CHAN_ADE9000_IRMS or SENSOR_CHAN_ADE9000_NIRMS
 * @param reg    Takes the register of phase A, or NPHCAL
 * @param count  Takes the number of phases
 * @return       0 on success, -ENOTSUP for other channels
 */
static int ade9000_phcal_reg(enum sensor_channel chan, uint16_t *reg, size_t *count)
{
	switch ((int)chan) {
	case SENSOR_CHAN_ADE9000_IRMS: {
		*reg = ADE9000_REG_APHCAL0;
		*count = ADE9000_PHASE_COUNT;

		return 0;
	}
	case SENSOR_CHAN_ADE9000_NIRMS: {
		*reg = ADE9000_REG_NPHCAL;
		*count = 1;

		return 0;
	}
	default: {
		return -ENOTSUP;
	}
	}
}

/**
 * @brief        Write the phase compensation xPHCAL0 or NPHCAL from angles in
 *               degrees.
 * @param dev    ADE9000 device
 * @param chan   SENSOR_CHAN_ADE9000_IRMS or SENSOR_CHAN_ADE9000_NIRMS
 * @param val    One angle per phase
 * @return       0 on success, -ENOTSUP for other channels, -EINVAL for an angle
 *               out of range, negative errno of the SPI transfer otherwise
 */
static int ade9000_phcal_set(const struct device *dev, enum sensor_channel chan,
			     const struct sensor_value *val)
{
	int32_t codes[ADE9000_PHASE_COUNT];
	uint16_t reg;
	size_t count;
	int ret;

	ret = ade9000_phcal_reg(chan, &reg, &count);
	if (ret) {
		return ret;
	}

	/* Check all phases first, so a bad value leaves every register unchanged */
	for (size_t i = 0; i < count; i++) {
		ret = ade9000_phcal_to_code(dev, &val[i], &codes[i]);
		if (ret) {
			return ret;
		}
	}

	for (size_t i = 0; i < count; i++) {
		ret = ade9000_reg_write32(dev, reg + (i * ADE9000_STRIDE_PHASE),
					  (uint32_t)codes[i]);
		if (ret) {
			return ret;
		}
	}

	return 0;
}

/**
 * @brief        Read the phase compensation xPHCAL0 or NPHCAL as angles in
 *               degrees.
 * @param dev    ADE9000 device
 * @param chan   SENSOR_CHAN_ADE9000_IRMS or SENSOR_CHAN_ADE9000_NIRMS
 * @param val    Takes one angle per phase
 * @return       0 on success, -ENOTSUP for other channels, negative errno of
 *               the SPI transfer otherwise
 */
static int ade9000_phcal_get(const struct device *dev, enum sensor_channel chan,
			     struct sensor_value *val)
{
	uint32_t value32;
	uint16_t reg;
	size_t count;
	int ret;

	ret = ade9000_phcal_reg(chan, &reg, &count);
	if (ret) {
		return ret;
	}

	for (size_t i = 0; i < count; i++) {
		ret = ade9000_reg_read32(dev, reg + (i * ADE9000_STRIDE_PHASE), &value32);
		if (ret) {
			return ret;
		}

		ret = ade9000_phcal_from_code(dev, (int32_t)value32, &val[i]);
		if (ret) {
			return ret;
		}
	}

	return 0;
}

int ade9000_attr_set(const struct device *dev, enum sensor_channel chan, enum sensor_attribute attr,
		     const struct sensor_value *val)
{
	const struct ade9000_cal_desc *cal;
	uint8_t os;
	int ret;

	if (chan == SENSOR_CHAN_ALL) {
		ret = ade9000_mode_set(dev, attr, val);
		if (ret != -ENOTSUP) {
			return ret;
		}

		return ade9000_reg_attr_set(dev, attr, val);
	}

	if (((int)chan >= (int)SENSOR_CHAN_ADE9000_WATTHR) &&
	    ((int)chan <= (int)SENSOR_CHAN_ADE9000_FVAHR)) {
		if (attr != SENSOR_ATTR_OFFSET) {
			return -ENOTSUP;
		}

		return ade9000_energy_base(dev, chan, (struct sensor_value *)val, true);
	}

	if ((int)attr == SENSOR_ATTR_ADE9000_PHCAL) {
		return ade9000_phcal_set(dev, chan, val);
	}

	cal = ade9000_cal_find(chan);
	if (cal == NULL) {
		return -ENOTSUP;
	}

	if (attr == SENSOR_ATTR_GAIN) {
		return ade9000_gain_set(dev, cal, val);
	}

	os = ade9000_cal_os(cal, attr);
	if (os == ADE9000_OS_NONE) {
		return -ENOTSUP;
	}

	for (size_t i = 0; i < ade9000_os_regs[os].count; i++) {
		ret = ade9000_reg_write32(dev, ade9000_os_regs[os].reg + (i * ADE9000_STRIDE_PHASE),
					  (uint32_t)val[i].val1);
		if (ret) {
			return ret;
		}
	}

	return 0;
}

int ade9000_attr_get(const struct device *dev, enum sensor_channel chan, enum sensor_attribute attr,
		     struct sensor_value *val)
{
	const struct ade9000_cal_desc *cal;
	uint32_t value32 = 0;
	uint8_t os;
	int ret;

	if (chan == SENSOR_CHAN_ALL) {
		if (((int)attr == SENSOR_ATTR_ADE9000_STATUS0) ||
		    ((int)attr == SENSOR_ATTR_ADE9000_STATUS1)) {
			struct ade9000_data *data = dev->data;

			k_mutex_lock(&data->lock, K_FOREVER);
			value32 = ((int)attr == SENSOR_ATTR_ADE9000_STATUS0) ? data->status0
									     : data->status1;
			k_mutex_unlock(&data->lock);

			val->val1 = (int32_t)value32;
			val->val2 = 0;

			return 0;
		}

		ret = ade9000_mode_get(dev, attr, val);
		if (ret != -ENOTSUP) {
			return ret;
		}

		return ade9000_reg_attr_get(dev, attr, val);
	}

	if (((int)chan >= (int)SENSOR_CHAN_ADE9000_WATTHR) &&
	    ((int)chan <= (int)SENSOR_CHAN_ADE9000_FVAHR)) {
		if (attr != SENSOR_ATTR_OFFSET) {
			return -ENOTSUP;
		}

		return ade9000_energy_base(dev, chan, val, false);
	}

	if ((int)attr == SENSOR_ATTR_ADE9000_PHCAL) {
		return ade9000_phcal_get(dev, chan, val);
	}

	cal = ade9000_cal_find(chan);
	if (cal == NULL) {
		return -ENOTSUP;
	}

	if (attr == SENSOR_ATTR_GAIN) {
		if (cal->gain == ADE9000_REG_NONE) {
			return -ENOTSUP;
		}

		/* A gain covers the same phases as the offset of the channel */
		for (size_t i = 0; i < ade9000_os_regs[cal->os].count; i++) {
			ret = ade9000_reg_read32(dev, cal->gain + (i * ADE9000_STRIDE_PHASE),
						 &value32);
			if (ret) {
				return ret;
			}

			ret = ade9000_gain_from_code((int32_t)value32, &val[i]);
			if (ret) {
				return ret;
			}
		}

		return 0;
	}

	os = ade9000_cal_os(cal, attr);
	if (os == ADE9000_OS_NONE) {
		return -ENOTSUP;
	}

	for (size_t i = 0; i < ade9000_os_regs[os].count; i++) {
		ret = ade9000_reg_read32(dev, ade9000_os_regs[os].reg + (i * ADE9000_STRIDE_PHASE),
					 &value32);
		if (ret) {
			return ret;
		}

		val[i].val1 = (int32_t)value32;
		val[i].val2 = 0;
	}

	return 0;
}

int ade9000_configure(const struct device *dev)
{
	const struct ade9000_config *cfg = dev->config;
	bool integrator = (cfg->config0 & (ADE9000_CONFIG0_INTEN | ADE9000_CONFIG0_ININTEN)) != 0;
	const struct ade9000_mode_reg regs[] = {
		{ADE9000_REG_PGA_GAIN, true, cfg->pga_gain},
		{ADE9000_REG_CHNL_DIS, true, cfg->chnl_dis},
		{ADE9000_REG_DICOEFF, false, integrator ? ADE9000_DICOEFF_INTEGRATOR : 0},
		{ADE9000_REG_VNOM, false, cfg->vnom},
		{ADE9000_REG_CONFIG0, false, cfg->config0},
		{ADE9000_REG_CONFIG1, true, cfg->config1 | ADE9000_CONFIG1_BURST_EN},
		{ADE9000_REG_CONFIG2, true, cfg->config2},
		{ADE9000_REG_ACCMODE, true, cfg->accmode},
		{ADE9000_REG_VLEVEL, false, ADE9000_VLEVEL_DEFAULT},
		{ADE9000_REG_VAR_DIS, true, cfg->var_dis ? ADE9000_VAR_DIS_VARDIS : 0},
		{ADE9000_REG_EGY_TIME, true, cfg->egy_time},
		{ADE9000_REG_EP_CFG, true, cfg->ep_cfg},
	};
	int ret;

	ret = ade9000_gains_write(dev);
	if (ret) {
		return ret;
	}

	ret = ade9000_offsets_write(dev);
	if (ret) {
		return ret;
	}

	ret = ade9000_multipoint_write(dev);
	if (ret) {
		return ret;
	}

	for (size_t i = 0; i < ARRAY_SIZE(regs); i++) {
		if (regs[i].wide16) {
			ret = ade9000_reg_write16(dev, regs[i].reg, (uint16_t)regs[i].value);
		} else {
			ret = ade9000_reg_write32(dev, regs[i].reg, regs[i].value);
		}

		if (ret) {
			LOG_ERR("Failed to write register 0x%03x (err %d)", regs[i].reg, ret);

			return ret;
		}
	}

	return 0;
}

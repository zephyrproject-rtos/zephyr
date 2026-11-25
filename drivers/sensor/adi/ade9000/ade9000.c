/*
 * Copyright (c) 2026 A-Eberle <Daniel.Kampert@a-eberle.de>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Driver for the Analog Devices ADE9000 polyphase energy metering IC:
 *        register access, measurements, reset and init. The configuration
 *        is in ade9000_config.c, the interrupts in ade9000_trigger.c.
 */

#define DT_DRV_COMPAT adi_ade9000

#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>

#include <string.h>

#include "ade9000.h"
#include "ade9000_spi.h"

/**
 * @brief  xVRMS and xIRMS code with full-scale inputs, data sheet page 33.
 */
#define ADE9000_FULL_SCALE_RMS 52702092

/**
 * @brief  xWATT, xVAR and xVA code with full-scale inputs, data sheet page 33 and 34.
 */
#define ADE9000_FULL_SCALE_POWER 20694066

/**
 * @brief  Ratio of peak to rms of a sine.
 */
#define ADE9000_SQRT2 1.4142135623730951

/**
 * @brief  ANGL_x_y counts at 1.024 MHz, so the angle in degrees is
 *         ANGL_x_y * 360 * f / 1,024,000. With f = 8000 * 2^16 / (xPERIOD + 1)
 *         this is ANGL_x_y * 184,320 / (xPERIOD + 1), data sheet page 37.
 */
#define ADE9000_ANGLE_PERIOD_FACTOR 184320LL

/**
 * @brief  Seconds one xWATT code takes to add one xWATTHR_HI code: 2^13
 *         samples at 8 kSPS.
 */
#define ADE9000_ENERGY_SECONDS_PER_CODE (8192.0 / 8000.0)

/**
 * @brief  Upper bound for RSTDONE after a reset, the data sheet gives up to 50 ms.
 */
#define ADE9000_RESET_TIMEOUT_MS 100

/**
 * @brief       CONFIG1.CF4_CFG value from the cf4-function property, whose enum
 *              order is CF4, EVENT, DREADY.
 * @param inst  Devicetree instance number
 */
#define ADE9000_INST_CF4_CFG(inst)                                                                 \
	((DT_INST_ENUM_IDX(inst, cf4_function) == 0)   ? ADE9000_CONFIG1_CF4_CFG_CF                \
	 : (DT_INST_ENUM_IDX(inst, cf4_function) == 1) ? ADE9000_CONFIG1_CF4_CFG_EVENT             \
						       : ADE9000_CONFIG1_CF4_CFG_DREADY)

/**
 * @brief       PGA_GAIN register value from the devicetree. The enum index of
 *              the PGA gain properties is the field value.
 * @param inst  Devicetree instance number
 */
#define ADE9000_INST_PGA_GAIN(inst)                                                                \
	(FIELD_PREP(ADE9000_PGA_GAIN_IA, DT_INST_ENUM_IDX(inst, current_pga_gain)) |               \
	 FIELD_PREP(ADE9000_PGA_GAIN_IB, DT_INST_ENUM_IDX(inst, current_pga_gain)) |               \
	 FIELD_PREP(ADE9000_PGA_GAIN_IC, DT_INST_ENUM_IDX(inst, current_pga_gain)) |               \
	 FIELD_PREP(ADE9000_PGA_GAIN_IN, DT_INST_ENUM_IDX(inst, neutral_pga_gain)) |               \
	 FIELD_PREP(ADE9000_PGA_GAIN_VA, DT_INST_ENUM_IDX(inst, voltage_pga_gain)) |               \
	 FIELD_PREP(ADE9000_PGA_GAIN_VB, DT_INST_ENUM_IDX(inst, voltage_pga_gain)) |               \
	 FIELD_PREP(ADE9000_PGA_GAIN_VC, DT_INST_ENUM_IDX(inst, voltage_pga_gain)))

/**
 * @brief       CONFIG0 register value from the devicetree.
 * @param inst  Devicetree instance number
 */
#define ADE9000_INST_CONFIG0(inst)                                                                 \
	(FIELD_PREP(ADE9000_CONFIG0_ISUM_CFG, DT_INST_PROP(inst, isum_cfg)) |                      \
	 (DT_INST_PROP(inst, hpf_disable) ? ADE9000_CONFIG0_HPFDIS : 0) |                          \
	 (DT_INST_PROP(inst, multi_point_compensation) ? ADE9000_CONFIG0_MTEN : 0) |               \
	 (DT_INST_PROP(inst, integrator) ? ADE9000_CONFIG0_INTEN : 0) |                            \
	 (DT_INST_PROP(inst, zx_src_sel) ? ADE9000_CONFIG0_ZX_SRC_SEL : 0) |                       \
	 (DT_INST_PROP(inst, rms_src_sel) ? ADE9000_CONFIG0_RMS_SRC_SEL : 0) |                     \
	 FIELD_PREP(ADE9000_CONFIG0_VNOM_EN, DT_INST_PROP(inst, vnom_phases)) |                    \
	 (DT_INST_PROP(inst, neutral_integrator) ? ADE9000_CONFIG0_ININTEN : 0) |                  \
	 (DT_INST_PROP(inst, disable_active_power_lpf) ? ADE9000_CONFIG0_DISAPLPF : 0) |           \
	 (DT_INST_PROP(inst, disable_reactive_power_lpf) ? ADE9000_CONFIG0_DISRPLPF : 0))

/**
 * @brief       CONFIG1 register value from the devicetree, without SWRST.
 * @param inst  Devicetree instance number
 */
#define ADE9000_INST_CONFIG1(inst)                                                                 \
	((DT_INST_PROP(inst, cf3_zero_crossing) ? ADE9000_CONFIG1_CF3_CFG : 0) |                   \
	 FIELD_PREP(ADE9000_CONFIG1_CF4_CFG, ADE9000_INST_CF4_CFG(inst)) |                         \
	 (DT_INST_PROP(inst, dip_swell_irq_mode) ? ADE9000_CONFIG1_DIP_SWELL_IRQ_MODE : 0) |       \
	 (DT_INST_PROP(inst, irq0_on_irq1) ? ADE9000_CONFIG1_IRQ0_ON_IRQ1 : 0) |                   \
	 (DT_INST_PROP(inst, external_reference) ? ADE9000_CONFIG1_EXT_REF : 0))

/**
 * @brief       CONFIG2 register value from the devicetree. The enum index of
 *              hpf-corner-millihertz is the HPF_CRN field value.
 * @param inst  Devicetree instance number
 */
#define ADE9000_INST_CONFIG2(inst)                                                                 \
	(FIELD_PREP(ADE9000_CONFIG2_HPF_CRN, DT_INST_ENUM_IDX(inst, hpf_corner_millihertz)) |      \
	 (DT_INST_PROP(inst, use_user_period) ? ADE9000_CONFIG2_UPERIOD_SEL : 0))

/**
 * @brief       ACCMODE register value from the devicetree. The enum index of
 *              the accumulation properties is the field value.
 * @param inst  Devicetree instance number
 */
#define ADE9000_INST_ACCMODE(inst)                                                                 \
	(FIELD_PREP(ADE9000_ACCMODE_WATTACC, DT_INST_ENUM_IDX(inst, active_accumulation)) |        \
	 FIELD_PREP(ADE9000_ACCMODE_VARACC, DT_INST_ENUM_IDX(inst, reactive_accumulation)) |       \
	 FIELD_PREP(ADE9000_ACCMODE_VCONSEL, DT_INST_PROP(inst, vconsel)) |                        \
	 (DT_INST_PROP(inst, iconsel) ? ADE9000_ACCMODE_ICONSEL : 0) |                             \
	 ((DT_INST_PROP(inst, line_frequency) == 60) ? ADE9000_ACCMODE_SELFREQ : 0))

/**
 * @brief       EP_CFG register value from the devicetree. The enum index of
 *              no-load-samples is the NOLOAD_TMR field value. THD and power
 *              factor stay zero without EGY_PWR_EN. EGY_LD_ACCUM stays clear,
 *              so the energy registers keep adding up and
 *              ade9000_energy_update() loses nothing.
 * @param inst  Devicetree instance number
 */
#define ADE9000_INST_EP_CFG(inst)                                                                  \
	(FIELD_PREP(ADE9000_EP_CFG_NOLOAD_TMR, DT_INST_ENUM_IDX(inst, no_load_samples)) |          \
	 ADE9000_EP_CFG_EGY_PWR_EN)

/**
 * @brief         Entry of the channel table, placed at the index of its channel.
 * @param _chan   Channel name without the SENSOR_CHAN_ADE9000_ prefix
 * @param _slot   First slot without the ADE9000_SLOT_ prefix
 * @param _count  Number of consecutive slots, 1 or ADE9000_PHASE_COUNT
 * @param _scale  Conversion without the ADE9000_SCALE_ prefix, see enum ade9000_scale
 */
#define ADE9000_CHAN(_chan, _slot, _count, _scale)                                                 \
	[ADE9000_CHAN_IDX(SENSOR_CHAN_ADE9000_##_chan)] = {                                        \
		.slot = ADE9000_SLOT_##_slot,                                                      \
		.count = _count,                                                                   \
		.scale = ADE9000_SCALE_##_scale,                                                   \
	}

/**
 * @brief  How channel_get turns the raw register code into a sensor_value.
 */
enum ade9000_scale {
	/** Total and fundamental rms, xVRMS and xVFRMS */
	ADE9000_SCALE_VOLTAGE_RMS,
	/** Total and fundamental rms, xIRMS and xIFRMS */
	ADE9000_SCALE_CURRENT_RMS,
	/** Total and fundamental active power, xWATT and xFWATT */
	ADE9000_SCALE_ACTIVE_POWER,
	/** Total and fundamental reactive power, xVAR and xFVAR */
	ADE9000_SCALE_REACTIVE_POWER,
	/** Total and fundamental apparent power, xVA and xFVA */
	ADE9000_SCALE_APPARENT_POWER,
	/** Signed 5.27 fixed point */
	ADE9000_SCALE_5_27,
	/** xTHD register, signed 5.27 fixed point, converted to percent */
	ADE9000_SCALE_THD,
	/** xPERIOD register (line period), converted to a frequency in Hz */
	ADE9000_SCALE_LINE_PERIOD,
	/** 16 bit ANGL_x_y register, converted to degrees */
	ADE9000_SCALE_ANGLE,
	/** Instantaneous voltage, xV_PCF */
	ADE9000_SCALE_VOLTAGE_WAVE,
	/** Instantaneous current, xI_PCF and NI_PCF */
	ADE9000_SCALE_CURRENT_WAVE,
	/** xMTREGION register, region number */
	ADE9000_SCALE_MTREGION,
};

/**
 * @brief  Where channel_get finds the samples of a custom channel and how it
 *         converts them.
 */
struct ade9000_chan_desc {
	/** First slot in ade9000_data.samples */
	uint8_t slot;
	/** Number of consecutive slots, 1 or ADE9000_PHASE_COUNT */
	uint8_t count;
	/** Conversion, see enum ade9000_scale */
	uint8_t scale;
};

/**
 * @brief  Slots and scaling behind each custom channel.
 */
static const struct ade9000_chan_desc ade9000_chans[ADE9000_CHAN_COUNT] = {
	ADE9000_CHAN(IRMS, AIRMS, ADE9000_PHASE_COUNT, CURRENT_RMS),
	ADE9000_CHAN(VRMS, AVRMS, ADE9000_PHASE_COUNT, VOLTAGE_RMS),
	ADE9000_CHAN(NIRMS, NIRMS, 1, CURRENT_RMS),
	ADE9000_CHAN(ISUMRMS, ISUMRMS, 1, CURRENT_RMS),
	ADE9000_CHAN(IFRMS, AIFRMS, ADE9000_PHASE_COUNT, CURRENT_RMS),
	ADE9000_CHAN(VFRMS, AVFRMS, ADE9000_PHASE_COUNT, VOLTAGE_RMS),
	ADE9000_CHAN(WATT, AWATT, ADE9000_PHASE_COUNT, ACTIVE_POWER),
	ADE9000_CHAN(VAR, AVAR, ADE9000_PHASE_COUNT, REACTIVE_POWER),
	ADE9000_CHAN(VA, AVA, ADE9000_PHASE_COUNT, APPARENT_POWER),
	ADE9000_CHAN(FWATT, AFWATT, ADE9000_PHASE_COUNT, ACTIVE_POWER),
	ADE9000_CHAN(FVAR, AFVAR, ADE9000_PHASE_COUNT, REACTIVE_POWER),
	ADE9000_CHAN(FVA, AFVA, ADE9000_PHASE_COUNT, APPARENT_POWER),
	ADE9000_CHAN(PF, APF, ADE9000_PHASE_COUNT, 5_27),
	ADE9000_CHAN(VTHD, AVTHD, ADE9000_PHASE_COUNT, THD),
	ADE9000_CHAN(ITHD, AITHD, ADE9000_PHASE_COUNT, THD),
	ADE9000_CHAN(FREQ, APERIOD, ADE9000_PHASE_COUNT, LINE_PERIOD),
	ADE9000_CHAN(ANGL_VV, ANGL_VA_VB, ADE9000_PHASE_COUNT, ANGLE),
	ADE9000_CHAN(ANGL_VI, ANGL_VA_IA, ADE9000_PHASE_COUNT, ANGLE),
	ADE9000_CHAN(ANGL_II, ANGL_IA_IB, ADE9000_PHASE_COUNT, ANGLE),
	ADE9000_CHAN(I_PCF, AI_PCF, ADE9000_PHASE_COUNT, CURRENT_WAVE),
	ADE9000_CHAN(V_PCF, AV_PCF, ADE9000_PHASE_COUNT, VOLTAGE_WAVE),
	ADE9000_CHAN(NI_PCF, NI_PCF, 1, CURRENT_WAVE),
	ADE9000_CHAN(MTREGION, AMTREGION, ADE9000_PHASE_COUNT, MTREGION),
	ADE9000_CHAN(IRMSONE, AIRMSONE, ADE9000_PHASE_COUNT, CURRENT_RMS),
	ADE9000_CHAN(VRMSONE, AVRMSONE, ADE9000_PHASE_COUNT, VOLTAGE_RMS),
	ADE9000_CHAN(NIRMSONE, NIRMSONE, 1, CURRENT_RMS),
	ADE9000_CHAN(IRMS1012, AIRMS1012, ADE9000_PHASE_COUNT, CURRENT_RMS),
	ADE9000_CHAN(VRMS1012, AVRMS1012, ADE9000_PHASE_COUNT, VOLTAGE_RMS),
	ADE9000_CHAN(NIRMS1012, NIRMS1012, 1, CURRENT_RMS),
};

/**
 * @brief  Registers of the slots behind the burst block, in slot order. The
 *         burst slots are read from ADE9000_REG_AV_PCF_1 + slot.
 */
static const uint16_t ade9000_single_regs[] = {
	ADE9000_REG_ISUMRMS,    ADE9000_REG_APERIOD,    ADE9000_REG_BPERIOD,
	ADE9000_REG_CPERIOD,    ADE9000_REG_AMTREGION,  ADE9000_REG_BMTREGION,
	ADE9000_REG_CMTREGION,  ADE9000_REG_ANGL_VA_VB, ADE9000_REG_ANGL_VB_VC,
	ADE9000_REG_ANGL_VA_VC, ADE9000_REG_ANGL_VA_IA, ADE9000_REG_ANGL_VB_IB,
	ADE9000_REG_ANGL_VC_IC, ADE9000_REG_ANGL_IA_IB, ADE9000_REG_ANGL_IB_IC,
	ADE9000_REG_ANGL_IA_IC,
};

LOG_MODULE_REGISTER(ADE9000, CONFIG_SENSOR_LOG_LEVEL);

/**
 * @brief        Check whether a channel is one of the energy channels.
 * @param chan   Channel to check
 * @return       true for SENSOR_CHAN_ADE9000_WATTHR to SENSOR_CHAN_ADE9000_FVAHR
 */
static bool ade9000_chan_is_energy(enum sensor_channel chan)
{
	return ((int)chan >= (int)SENSOR_CHAN_ADE9000_WATTHR) &&
	       ((int)chan <= (int)SENSOR_CHAN_ADE9000_FVAHR);
}

/**
 * @brief        Check whether a channel is one of the custom ADE9000 channels.
 * @param chan   Channel to check
 * @return       true for the channels of enum sensor_channel_ade9000
 */
static bool ade9000_chan_is_custom(enum sensor_channel chan)
{
	return (ADE9000_CHAN_IDX(chan) >= 0) && (ADE9000_CHAN_IDX(chan) < ADE9000_CHAN_COUNT);
}

int ade9000_read(const struct device *dev, uint16_t addr, uint8_t *buf, size_t len)
{
	uint8_t cmd[2];
	const struct ade9000_config *cfg = dev->config;
	const struct spi_buf tx_buf = {.buf = cmd, .len = sizeof(cmd)};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};
	struct spi_buf rx_bufs[] = {
		{.buf = NULL, .len = sizeof(cmd)},
		{.buf = buf, .len = len},
	};
	const struct spi_buf_set rx = {.buffers = rx_bufs, .count = ARRAY_SIZE(rx_bufs)};

	sys_put_be16(ADE9000_CMD(addr, true), cmd);

	return spi_transceive_dt(&cfg->spi, &tx, &rx);
}

/**
 * @brief        Write raw bytes behind a write command.
 * @param dev    ADE9000 device
 * @param addr   Register address
 * @param buf    Big endian register value
 * @param len    Width of the register in bytes
 * @return       0 on success, negative errno of the SPI transfer otherwise
 */
static int ade9000_write(const struct device *dev, uint16_t addr, const uint8_t *buf, size_t len)
{
	uint8_t cmd[2];
	const struct ade9000_config *cfg = dev->config;
	const struct spi_buf tx_bufs[] = {
		{.buf = cmd, .len = sizeof(cmd)},
		{.buf = (void *)buf, .len = len},
	};
	const struct spi_buf_set tx = {.buffers = tx_bufs, .count = ARRAY_SIZE(tx_bufs)};

	sys_put_be16(ADE9000_CMD(addr, false), cmd);

	return spi_write_dt(&cfg->spi, &tx);
}

/**
 * @brief        Read consecutive slots of the sample buffer. Burst slots are
 *               read in one transfer, the others one by one.
 * @param dev    ADE9000 device
 * @param first  First slot
 * @param count  Number of slots
 * @param raw    Takes the register values in CPU byte order
 * @return       0 on success, negative errno of the SPI transfer otherwise
 */
static int ade9000_slots_read(const struct device *dev, size_t first, size_t count, uint32_t *raw)
{
	size_t burst = 0;
	int ret;

	if (first < ADE9000_SLOT_BURST_COUNT) {
		burst = MIN(count, ADE9000_SLOT_BURST_COUNT - first);

		ret = ade9000_read(dev, ADE9000_REG_AV_PCF_1 + first, (uint8_t *)raw,
				   burst * sizeof(raw[0]));
		if (ret) {
			return ret;
		}

		/* The burst leaves the raw big endian bytes in the buffer */
		for (size_t i = 0; i < burst; i++) {
			raw[i] = sys_be32_to_cpu(raw[i]);
		}
	}

	for (size_t i = burst; i < count; i++) {
		size_t slot = first + i;
		uint16_t reg = ade9000_single_regs[slot - ADE9000_SLOT_BURST_COUNT];

		if (slot >= ADE9000_SLOT_ANGL_VA_VB) {
			uint16_t value16;

			ret = ade9000_reg_read16(dev, reg, &value16);
			raw[i] = value16;
		} else {
			ret = ade9000_reg_read32(dev, reg, &raw[i]);
		}

		if (ret) {
			return ret;
		}
	}

	return 0;
}

/**
 * @brief      Read all energy registers and add their change since the last
 *             read to the software counters. The registers are not reset, so
 *             no energy is lost between two reads.
 * @param dev  ADE9000 device
 * @return     0 on success, negative errno of the SPI transfer otherwise
 */
static int ade9000_energy_update(const struct device *dev)
{
	struct ade9000_data *data = dev->data;
	uint32_t raw[ADE9000_ENERGY_COUNT][ADE9000_PHASE_COUNT];
	int ret;

	for (size_t type = 0; type < ADE9000_ENERGY_COUNT; type++) {
		for (size_t phase = 0; phase < ADE9000_PHASE_COUNT; phase++) {
			ret = ade9000_reg_read32(dev,
						 ADE9000_REG_AWATTHR_HI +
							 (type * ADE9000_STRIDE_ENERGY_TYPE) +
							 (phase * ADE9000_STRIDE_ENERGY_PHASE),
						 &raw[type][phase]);
			if (ret) {
				return ret;
			}
		}
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	for (size_t type = 0; type < ADE9000_ENERGY_COUNT; type++) {
		for (size_t phase = 0; phase < ADE9000_PHASE_COUNT; phase++) {
			/* The signed difference stays right across one wrap of the register */
			data->energy[type][phase] +=
				(int32_t)(raw[type][phase] - data->energy_last[type][phase]);
			data->energy_last[type][phase] = raw[type][phase];
		}
	}

	k_mutex_unlock(&data->lock);

	return 0;
}

/**
 * @brief        Convert xWATTHR_HI codes into Wh, varh or VAh in micro units.
 * @param cfg    Device configuration
 * @param codes  Energy in xWATTHR_HI codes
 * @return       Energy in micro Wh, varh or VAh
 */
static int64_t ade9000_energy_to_micro(const struct ade9000_config *cfg, int64_t codes)
{
	double v_fs = (double)cfg->voltage_fs_uv / 1000000.0;
	double i_fs = (double)cfg->current_fs_ua / 1000000.0;

	/* E (Wh) = codes * P_lsb * 2^13 / 8000 s / 3600, P_lsb = V_fs * I_fs / 20,694,066 */
	return (int64_t)(((double)codes * v_fs * i_fs * ADE9000_ENERGY_SECONDS_PER_CODE *
			  1000000.0) /
			 (ADE9000_FULL_SCALE_POWER * 3600.0));
}

/**
 * @brief        Read the registers behind one custom channel, the die
 *               temperature, or all channels plus the temperature. Any energy
 *               channel updates all energy counters. An angle channel also
 *               reads xPERIOD for the conversion.
 * @param dev    ADE9000 device
 * @param chan   Custom channel, SENSOR_CHAN_DIE_TEMP or SENSOR_CHAN_ALL
 * @return       0 on success, -ENOTSUP for other channels, negative errno of
 *               the SPI transfer otherwise
 */
static int ade9000_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct ade9000_data *data = dev->data;
	uint16_t temp_rslt;
	int ret;

	if ((chan != SENSOR_CHAN_ALL) && (chan != SENSOR_CHAN_DIE_TEMP) &&
	    (ade9000_chan_is_custom(chan) == false)) {

		return -ENOTSUP;
	}

#ifndef CONFIG_ADE9000_TRIGGER
	/* Without interrupts the status is only read here */
	ret = ade9000_status_update(dev, ADE9000_REG_STATUS0, &data->status0);
	if (ret) {
		return ret;
	}

	ret = ade9000_status_update(dev, ADE9000_REG_STATUS1, &data->status1);
	if (ret) {
		return ret;
	}
#endif

	if (ade9000_chan_is_energy(chan)) {
		return ade9000_energy_update(dev);
	}

	switch (chan) {
	case SENSOR_CHAN_ALL: {
		uint32_t samples[ADE9000_SLOT_COUNT];

		ret = ade9000_slots_read(dev, 0, ADE9000_SLOT_COUNT, samples);
		if (ret) {
			return ret;
		}

		k_mutex_lock(&data->lock, K_FOREVER);
		memcpy(data->samples, samples, sizeof(samples));
		k_mutex_unlock(&data->lock);

		ret = ade9000_energy_update(dev);
		if (ret) {
			return ret;
		}

		__fallthrough;
	}
	case SENSOR_CHAN_DIE_TEMP: {
		ret = ade9000_reg_read16(dev, ADE9000_REG_TEMP_RSLT, &temp_rslt);
		if (ret) {
			return ret;
		}

		k_mutex_lock(&data->lock, K_FOREVER);
		data->temp_rslt = temp_rslt;
		k_mutex_unlock(&data->lock);

		/* Restart the conversion, the result is ready 320 ms later */
		ret = ade9000_reg_write16(dev, ADE9000_REG_TEMP_CFG, ADE9000_TEMP_CFG_START);

		break;
	}
	default: {
		const struct ade9000_chan_desc *desc = &ade9000_chans[ADE9000_CHAN_IDX(chan)];
		uint32_t raw[ADE9000_PHASE_COUNT];
		uint32_t period[ADE9000_PHASE_COUNT];

		ret = ade9000_slots_read(dev, desc->slot, desc->count, raw);
		if (ret) {
			return ret;
		}

		/* The angle conversion needs the line period of the same fetch */
		if (desc->scale == ADE9000_SCALE_ANGLE) {
			ret = ade9000_slots_read(dev, ADE9000_SLOT_APERIOD, ADE9000_PHASE_COUNT,
						 period);
			if (ret) {
				return ret;
			}
		}

		k_mutex_lock(&data->lock, K_FOREVER);
		memcpy(&data->samples[desc->slot], raw, desc->count * sizeof(raw[0]));
		if (desc->scale == ADE9000_SCALE_ANGLE) {
			memcpy(&data->samples[ADE9000_SLOT_APERIOD], period, sizeof(period));
		}
		k_mutex_unlock(&data->lock);

		break;
	}
	}

	return ret;
}

/**
 * @brief        Convert one raw register value into its unit.
 * @param dev    ADE9000 device
 * @param period xPERIOD of the phase for the angle conversion, 0 to use the
 *               nominal line frequency instead
 * @param scale  Conversion, see enum ade9000_scale
 * @param raw    Register value
 * @param val    Takes the value
 * @return       0 on success, -ERANGE if the value does not fit a sensor_value
 */
static int ade9000_raw_to_value(const struct device *dev, uint32_t period, uint8_t scale,
				uint32_t raw, struct sensor_value *val)
{
	const struct ade9000_config *cfg = dev->config;
	const struct ade9000_data *data = dev->data;
	double v_fs = (double)cfg->voltage_fs_uv / 1000000.0;
	double i_fs = (double)cfg->current_fs_ua / 1000000.0;

	switch (scale) {
	case ADE9000_SCALE_VOLTAGE_RMS: {
		/* Total and Fundamental RMS according to the datasheet page 33: */
		/* V (V) = xVRMS / 52,702,092 * V_fs */
		return sensor_value_from_double(val, ((double)((int32_t)raw) * v_fs) /
							     ADE9000_FULL_SCALE_RMS);
	}
	case ADE9000_SCALE_CURRENT_RMS: {
		/* Total and Fundamental RMS according to the datasheet page 33: */
		/* I (A) = xIRMS / 52,702,092 * I_fs */
		return sensor_value_from_double(val, ((double)((int32_t)raw) * i_fs) /
							     ADE9000_FULL_SCALE_RMS);
	}
	case ADE9000_SCALE_ACTIVE_POWER:
	case ADE9000_SCALE_REACTIVE_POWER:
	case ADE9000_SCALE_APPARENT_POWER: {
		/* Total and Fundamental Active Power according to the datasheet page 33: */
		/* P (W) = xWATT / 20,694,066 * V_fs * I_fs */
		/* Total and Fundamental Reactive Power according to the datasheet page 33: */
		/* Q (var) = xVAR / 20,694,066 * V_fs * I_fs */
		/* Total and Fundamental Apparent Power according to the datasheet page 34: */
		/* S (VA) = xVA / 20,694,066 * V_fs * I_fs */
		return sensor_value_from_double(val, ((double)((int32_t)raw) * v_fs * i_fs) /
							     ADE9000_FULL_SCALE_POWER);
	}
	case ADE9000_SCALE_5_27: {
		/* Power factor according to the datasheet page 39: */
		/* PF [-1, 1] = xPF * 2^-27 */
		return sensor_value_from_micro(val, ((int64_t)((int32_t)raw) * 1000000) /
							    (INT64_C(1) << 27));
	}
	case ADE9000_SCALE_THD: {
		/* THD according to the datasheet page 39: */
		/* THD (%) = xTHD * 2^-27 * 100 */
		return sensor_value_from_micro(val, ((int64_t)((int32_t)raw) * 100000000) /
							    (INT64_C(1) << 27));
	}
	case ADE9000_SCALE_LINE_PERIOD: {
		/* Line Period according to the datasheet page 37: */
		/* line period (s) = (xPERIOD + 1) / (8000 * 2^16) */
		/* Without zero crossings xPERIOD keeps its value, after a reset */
		/* 0x00A00000, which gives 49.999995 Hz instead of 0 Hz. */
		return sensor_value_from_micro(
			val, (int64_t)((8000ULL * BIT(16) * 1000000ULL) / ((uint64_t)raw + 1)));
	}
	case ADE9000_SCALE_ANGLE: {
		/* Angle Measurement according to the datasheet page 37: */
		/* angle (deg) = ANGL_x_y * 184,320 / (xPERIOD + 1) with the measured line period */
		if (period != 0) {
			return sensor_value_from_micro(
				val, ((int64_t)raw * ADE9000_ANGLE_PERIOD_FACTOR * 1000000LL) /
					     ((int64_t)period + 1));
		}

		/* Nominal: ANGL_x_y * 0.017578125 at 50 Hz, ANGL_x_y * 0.02109375 at 60 Hz */
		if (data->line_frequency == 60) {
			return sensor_value_from_micro(val, ((int64_t)raw * 84375) / 4);
		}

		return sensor_value_from_micro(val, ((int64_t)raw * 140625) / 8);
	}
	case ADE9000_SCALE_VOLTAGE_WAVE: {
		/* The full-scale code is the peak of a sine with the full-scale rms value: */
		/* v (V) = xV_PCF / ADE9000_FULL_SCALE_WAVE * V_fs * sqrt(2) */
		return sensor_value_from_double(val,
						((double)((int32_t)raw) * v_fs * ADE9000_SQRT2) /
							ADE9000_FULL_SCALE_WAVE);
	}
	case ADE9000_SCALE_CURRENT_WAVE: {
		/* i (A) = xI_PCF / ADE9000_FULL_SCALE_WAVE * I_fs * sqrt(2) */
		return sensor_value_from_double(val,
						((double)((int32_t)raw) * i_fs * ADE9000_SQRT2) /
							ADE9000_FULL_SCALE_WAVE);
	}
	case ADE9000_SCALE_MTREGION: {
		val->val1 = (int32_t)(raw & ADE9000_MTREGION_REGION);
		val->val2 = 0;

		return 0;
	}
	default: {
		return -EINVAL;
	}
	}
}

/**
 * @brief        Convert the last fetched samples of a channel into their unit.
 * @param dev    ADE9000 device
 * @param chan   Custom channel or SENSOR_CHAN_DIE_TEMP
 * @param val    Takes the value, or ADE9000_PHASE_COUNT values for a three
 *               phase channel
 * @return       0 on success, -ENOTSUP for other channels, -ERANGE if a
 *               value does not fit a sensor_value
 */
static int ade9000_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	const struct ade9000_config *cfg = dev->config;
	struct ade9000_data *data = dev->data;
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (chan == SENSOR_CHAN_DIE_TEMP) {
		/* Temperature according to the datasheet page 39: */
		/* T = TEMP_RSLT * (-TEMP_GAIN / 65536) + (TEMP_OFFSET / 32) */
		float temp = ((float)data->temp_rslt * (-(float)data->temp_gain / 65536.0f)) +
			     ((float)data->temp_offset / 32.0f);

		ret = sensor_value_from_float(val, temp);
	} else if (ade9000_chan_is_energy(chan)) {
		size_t type = (int)chan - (int)SENSOR_CHAN_ADE9000_WATTHR;

		for (size_t i = 0; i < ADE9000_PHASE_COUNT; i++) {
			ret = sensor_value_from_micro(
				&val[i],
				data->energy_base_u[type][i] +
					ade9000_energy_to_micro(cfg, data->energy[type][i]));
			if (ret) {
				break;
			}
		}
	} else if (ade9000_chan_is_custom(chan)) {
		const struct ade9000_chan_desc *desc = &ade9000_chans[ADE9000_CHAN_IDX(chan)];

		for (size_t i = 0; i < desc->count; i++) {
			/* xPERIOD is stale without zero crossings */
			bool period_valid = (data->status1 & ADE9000_STATUS1_ZXTOV(i)) == 0;
			uint32_t period = 0;

			if ((desc->scale == ADE9000_SCALE_LINE_PERIOD) && !period_valid) {
				val[i].val1 = 0;
				val[i].val2 = 0;

				continue;
			}

			/* Angle i uses the line period of phase i */
			if ((desc->scale == ADE9000_SCALE_ANGLE) && period_valid) {
				period = data->samples[ADE9000_SLOT_APERIOD + i];
			}

			ret = ade9000_raw_to_value(dev, period, desc->scale,
						   data->samples[desc->slot + i], &val[i]);
			if (ret) {
				break;
			}
		}
	} else {
		ret = -ENOTSUP;
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

/**
 * @brief      Start a hardware reset through reset-gpios, or a software reset
 *             through CONFIG1.SWRST if the line is not in the devicetree.
 * @param dev  ADE9000 device
 * @return     0 on success, negative errno otherwise
 */
static int ade9000_reset_start(const struct device *dev)
{
#if ADE9000_RESET_GPIO
	const struct ade9000_config *cfg = dev->config;
	int ret;

	if (cfg->reset.port != NULL) {
		if (gpio_is_ready_dt(&cfg->reset) == false) {
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&cfg->reset, GPIO_OUTPUT_ACTIVE);
		if (ret) {
			return ret;
		}

		k_msleep(10);

		return gpio_pin_set_dt(&cfg->reset, 0);
	}
#endif

	return ade9000_reg_write16(dev, ADE9000_REG_CONFIG1, ADE9000_CONFIG1_SWRST);
}

/**
 * @brief      Reset the IC with ade9000_reset_start() and wait for RSTDONE.
 * @param dev  ADE9000 device
 * @return     0 on success, -ETIMEDOUT without RSTDONE, negative errno
 *             otherwise
 */
static int ade9000_reset(const struct device *dev)
{
	int ret;

	ret = ade9000_reset_start(dev);
	if (ret) {
		return ret;
	}

	/* Power-up takes up to 50 ms according to the data sheet. Reads return all ones
	 * while the SPI port is not up yet, so these are not taken as RSTDONE.
	 */
	for (int i = 0; i < ADE9000_RESET_TIMEOUT_MS; i++) {
		uint32_t status1;

		k_msleep(1);

		ret = ade9000_reg_read32(dev, ADE9000_REG_STATUS1, &status1);
		if ((ret == 0) && (status1 != UINT32_MAX) && (status1 & ADE9000_STATUS1_RSTDONE)) {
			/* Write one to clear, this releases IRQ1 */
			return ade9000_reg_write32(dev, ADE9000_REG_STATUS1,
						   ADE9000_STATUS1_RSTDONE);
		}
	}

	LOG_ERR("No RSTDONE after %d ms", ADE9000_RESET_TIMEOUT_MS);

	return -ETIMEDOUT;
}

/**
 * @brief      Reset the IC, check PART_ID, write the devicetree calibration
 *             and start the DSP.
 * @param dev  ADE9000 device
 * @return     0 on success, -ENODEV if the bus is not ready or no ADE9000
 *             answers, negative errno otherwise
 */
static int ade9000_init(const struct device *dev)
{
	const struct ade9000_config *cfg = dev->config;
	struct ade9000_data *data = dev->data;
	uint32_t part_id;
	uint32_t temp_trim;
	uint16_t version;
	int ret;

	k_mutex_init(&data->lock);
	data->line_frequency = cfg->line_frequency;

	if (spi_is_ready_dt(&cfg->spi) == false) {
		LOG_ERR("SPI bus not ready");

		return -ENODEV;
	}

	ret = ade9000_reset(dev);
	if (ret) {
		LOG_ERR("Reset failed (err %d)", ret);

		return ret;
	}

	ret = ade9000_reg_read32(dev, ADE9000_REG_PART_ID, &part_id);
	if (ret) {
		return ret;
	}

	if ((part_id & ADE9000_PART_ID_ADE9000) == 0) {
		LOG_ERR("No ADE9000 found (PART_ID 0x%08x)", part_id);

		return -ENODEV;
	}

	ret = ade9000_reg_read16(dev, ADE9000_REG_VERSION, &version);
	if (ret) {
		return ret;
	}

	LOG_DBG("ADE9000 part ID 0x%08x", part_id);
	LOG_DBG("ADE9000 version 0x%04x", version);

	ret = ade9000_reg_read32(dev, ADE9000_REG_TEMP_TRIM, &temp_trim);
	if (ret) {
		return ret;
	}

	data->temp_offset = temp_trim >> 16;
	data->temp_gain = temp_trim & 0xFFFF;

	ret = ade9000_configure(dev);
	if (ret) {
		return ret;
	}

	ret = ade9000_reg_write16(dev, ADE9000_REG_TEMP_CFG, ADE9000_TEMP_CFG_START);
	if (ret) {
		LOG_ERR("Failed to write TEMP_CFG (err %d)", ret);

		return ret;
	}

#ifdef CONFIG_ADE9000_TRIGGER
	ret = ade9000_init_interrupt(dev);
	if (ret) {
		LOG_ERR("Interrupt setup failed (err %d)", ret);

		return ret;
	}
#endif

	return ade9000_reg_write16(dev, ADE9000_REG_RUN, ADE9000_RUN_ON);
}

static DEVICE_API(sensor, ade9000_api) = {
	.attr_set = ade9000_attr_set,
	.attr_get = ade9000_attr_get,
#ifdef CONFIG_ADE9000_TRIGGER
	.trigger_set = ade9000_trigger_set,
#endif
	.sample_fetch = ade9000_sample_fetch,
	.channel_get = ade9000_channel_get,
};

#ifdef CONFIG_PM_DEVICE
/**
 * @brief         Stop the DSP on suspend and restart it on resume. The
 *                configuration is kept.
 * @param dev     ADE9000 device
 * @param action  Power management action
 * @return        0 on success, -ENOTSUP for other actions, negative errno of
 *                the SPI transfer otherwise
 */
static int ade9000_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND: {
		return ade9000_reg_write16(dev, ADE9000_REG_RUN, 0);
	}
	case PM_DEVICE_ACTION_RESUME: {
		return ade9000_reg_write16(dev, ADE9000_REG_RUN, ADE9000_RUN_ON);
	}
	default: {
		return -ENOTSUP;
	}
	}
}
#endif

int ade9000_reg_read16(const struct device *dev, uint16_t addr, uint16_t *value)
{
	uint8_t buf[2];
	int ret;

	ret = ade9000_read(dev, addr, buf, sizeof(buf));
	if (ret == 0) {
		*value = sys_get_be16(buf);
	}

	return ret;
}

int ade9000_reg_read32(const struct device *dev, uint16_t addr, uint32_t *value)
{
	uint8_t buf[4];
	int ret;

	ret = ade9000_read(dev, addr, buf, sizeof(buf));
	if (ret == 0) {
		*value = sys_get_be32(buf);
	}

	return ret;
}

int ade9000_reg_write16(const struct device *dev, uint16_t addr, uint16_t value)
{
	uint8_t buf[2];

	sys_put_be16(value, buf);

	return ade9000_write(dev, addr, buf, sizeof(buf));
}

int ade9000_reg_write32(const struct device *dev, uint16_t addr, uint32_t value)
{
	uint8_t buf[4];

	sys_put_be32(value, buf);

	return ade9000_write(dev, addr, buf, sizeof(buf));
}

int ade9000_status_update(const struct device *dev, uint16_t reg, uint32_t *status)
{
	struct ade9000_data *data = dev->data;
	uint32_t value;
	int ret;

	ret = ade9000_reg_read32(dev, reg, &value);
	if (ret) {
		LOG_ERR("Failed to read status register 0x%03x", reg);

		return ret;
	}

	/* Write one to clear, this also releases the IRQ line */
	ret = ade9000_reg_write32(dev, reg, value);

	k_mutex_lock(&data->lock, K_FOREVER);
	*status = value;
	k_mutex_unlock(&data->lock);

	return ret;
}

int ade9000_waveform_read(const struct device *dev, uint16_t index, int32_t *buf, size_t count)
{
	int ret;

	if ((buf == NULL) || (count == 0) || (((size_t)index + count) > ADE9000_WFB_SIZE)) {
		return -EINVAL;
	}

	ret = ade9000_read(dev, ADE9000_WFB_BASE + index, (uint8_t *)buf, count * sizeof(*buf));
	if (ret) {
		return ret;
	}

	for (size_t i = 0; i < count; i++) {
		buf[i] = (int32_t)sys_be32_to_cpu((uint32_t)buf[i]);
	}

	return 0;
}

int ade9000_wave_scale(const struct device *dev, enum sensor_channel chan, float *scale)
{
	const struct ade9000_config *cfg = dev->config;
	uint32_t fs;

	if (scale == NULL) {
		return -EINVAL;
	}

	switch ((int)chan) {
	case SENSOR_CHAN_ADE9000_V_PCF: {
		fs = cfg->voltage_fs_uv;

		break;
	}
	case SENSOR_CHAN_ADE9000_I_PCF:
	case SENSOR_CHAN_ADE9000_NI_PCF: {
		fs = cfg->current_fs_ua;

		break;
	}
	default: {
		return -EINVAL;
	}
	}

	*scale = (float)(((double)fs / 1000000.0) * ADE9000_SQRT2 / ADE9000_FULL_SCALE_WAVE);

	return 0;
}

int ade9000_energy_base(const struct device *dev, enum sensor_channel chan,
			struct sensor_value *val, bool set)
{
	struct ade9000_data *data = dev->data;
	size_t type = (int)chan - (int)SENSOR_CHAN_ADE9000_WATTHR;
	int ret = 0;

	if (ade9000_chan_is_energy(chan) == false) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	for (size_t phase = 0; phase < ADE9000_PHASE_COUNT; phase++) {
		if (set) {
			data->energy_base_u[type][phase] = sensor_value_to_micro(&val[phase]);
			data->energy[type][phase] = 0;
		} else {
			ret = sensor_value_from_micro(&val[phase],
						      data->energy_base_u[type][phase]);
			if (ret) {
				break;
			}
		}
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

/**
 * @brief       Configuration, data and device of one devicetree instance.
 * @param inst  Devicetree instance number
 */
/* clang-format breaks the IF_ENABLED() lines of this macro */
/* clang-format off */
#define ADE9000_DEFINE(inst)                                                                       \
	static struct ade9000_data ade9000_data_##inst;                                            \
                                                                                                   \
	static const struct ade9000_config ade9000_config_##inst = {                               \
		.spi = SPI_DT_SPEC_INST_GET(inst, SPI_OP_MODE_CONTROLLER |                         \
					    SPI_WORD_SET(8) | SPI_TRANSFER_MSB),                   \
		IF_ENABLED(ADE9000_RESET_GPIO,                                                     \
			   (.reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),))           \
		IF_ENABLED(CONFIG_ADE9000_TRIGGER,                                                 \
			   (.irq0 = GPIO_DT_SPEC_INST_GET_OR(inst, irq0_gpios, {0}),               \
			    .irq1 = GPIO_DT_SPEC_INST_GET_OR(inst, irq1_gpios, {0}),))             \
		.igain = {DT_INST_PROP(inst, a_igains), DT_INST_PROP(inst, b_igains),              \
			  DT_INST_PROP(inst, c_igains)},                                           \
		.phcal = {DT_INST_PROP(inst, a_phcals), DT_INST_PROP(inst, b_phcals),              \
			  DT_INST_PROP(inst, c_phcals)},                                           \
		.vgain = DT_INST_PROP(inst, vgains),                                               \
		.pgain = DT_INST_PROP(inst, pgains),                                               \
		.nigain = DT_INST_PROP(inst, nigain),                                              \
		.nphcal = DT_INST_PROP(inst, nphcal),                                              \
		.os = {                                                                            \
			[ADE9000_OS_IRMS] = DT_INST_PROP(inst, irmsos),                            \
			[ADE9000_OS_VRMS] = DT_INST_PROP(inst, vrmsos),                            \
			[ADE9000_OS_IFRMS] = DT_INST_PROP(inst, ifrmsos),                          \
			[ADE9000_OS_VFRMS] = DT_INST_PROP(inst, vfrmsos),                          \
			[ADE9000_OS_IRMSONE] = DT_INST_PROP(inst, irmsoneos),                      \
			[ADE9000_OS_VRMSONE] = DT_INST_PROP(inst, vrmsoneos),                      \
			[ADE9000_OS_IRMS1012] = DT_INST_PROP(inst, irms1012os),                    \
			[ADE9000_OS_VRMS1012] = DT_INST_PROP(inst, vrms1012os),                    \
			[ADE9000_OS_WATT] = DT_INST_PROP(inst, wattos),                            \
			[ADE9000_OS_VAR] = DT_INST_PROP(inst, varos),                              \
			[ADE9000_OS_FWATT] = DT_INST_PROP(inst, fwattos),                          \
			[ADE9000_OS_FVAR] = DT_INST_PROP(inst, fvaros),                            \
			[ADE9000_OS_NIRMS] = {DT_INST_PROP(inst, nirmsos)},                        \
			[ADE9000_OS_NIRMSONE] = {DT_INST_PROP(inst, nirmsoneos)},                  \
			[ADE9000_OS_NIRMS1012] = {DT_INST_PROP(inst, nirms1012os)},                \
			[ADE9000_OS_ISUMRMS] = {DT_INST_PROP(inst, isumrmsos)},                    \
		},                                                                                 \
		.voltage_fs_uv = DT_INST_PROP(inst, voltage_full_scale_microvolt),                 \
		.current_fs_ua = DT_INST_PROP(inst, current_full_scale_microampere),               \
		.line_frequency = DT_INST_PROP(inst, line_frequency),                              \
		.multi_point_compensation = DT_INST_PROP(inst, multi_point_compensation),          \
		.config0 = ADE9000_INST_CONFIG0(inst),                                             \
		.config1 = ADE9000_INST_CONFIG1(inst),                                             \
		.config2 = ADE9000_INST_CONFIG2(inst),                                             \
		.accmode = ADE9000_INST_ACCMODE(inst),                                             \
		.ep_cfg = ADE9000_INST_EP_CFG(inst),                                               \
		.pga_gain = ADE9000_INST_PGA_GAIN(inst),                                           \
		.chnl_dis = DT_INST_PROP(inst, disabled_adcs),                                     \
		.egy_time = DT_INST_PROP(inst, energy_time),                                       \
		.vnom = DT_INST_PROP(inst, vnom),                                                  \
		.var_dis = DT_INST_PROP(inst, var_disable),                                        \
		.irq0_on_irq1 = DT_INST_PROP(inst, irq0_on_irq1),                                  \
		.mtthr_low = DT_INST_PROP(inst, mtthr_low),                                        \
		.mtthr_high = DT_INST_PROP(inst, mtthr_high),                                      \
	};                                                                                         \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, ade9000_pm_action);                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, ade9000_init, PM_DEVICE_DT_INST_GET(inst),              \
				     &ade9000_data_##inst, &ade9000_config_##inst, POST_KERNEL,    \
				     CONFIG_SENSOR_INIT_PRIORITY, &ade9000_api);
/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(ADE9000_DEFINE)

/*
 * Copyright (c) 2025 Pedro Ramos
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_atm90e26

#include <string.h>

#include <zephyr/drivers/energy_meter.h>
#include <zephyr/drivers/energy_meter/atm90e26.h>
#include <zephyr/logging/log.h>

#include "atm90e26.h"

LOG_MODULE_REGISTER(atm90e26, CONFIG_ENERGY_METER_LOG_LEVEL);

#define WRITE_RETRIES 3

#define QUANTITY_COUNT ENERGY_METER_ATM90E26_QUANTITY_COUNT

enum atm90e26_fmt {
	FMT_NONE = 0,
	FMT_URMS,
	FMT_IRMS,
	FMT_POWER,
	FMT_PF,
	FMT_ANGLE,
	FMT_FREQ,
	FMT_ENERGY,
	FMT_RAW,
};

struct atm90e26_chan {
	uint8_t fmt;
	uint8_t reg;
};

#define AT(line, f, r) [line] = {.fmt = (f), .reg = (r)}

/* The L line current channel on line A and the N line current channel on line N */
#define LN(f, l, n) AT(ENERGY_METER_LINE_A, f, l), AT(ENERGY_METER_LINE_N, f, n)

#define TOTAL(f, r) AT(ENERGY_METER_LINE_TOTAL, f, r)

static const struct atm90e26_chan atm90e26_chans[QUANTITY_COUNT][ENERGY_METER_LINE_COUNT] = {
	[ENERGY_METER_VOLTAGE] = {AT(ENERGY_METER_LINE_A, FMT_URMS, ATM90E26_REG_URMS)},
	[ENERGY_METER_CURRENT] = {LN(FMT_IRMS, ATM90E26_REG_IRMS, ATM90E26_REG_IRMS2)},
	[ENERGY_METER_ACTIVE_POWER] = {LN(FMT_POWER, ATM90E26_REG_PMEAN, ATM90E26_REG_PMEAN2)},
	[ENERGY_METER_REACTIVE_POWER] = {LN(FMT_POWER, ATM90E26_REG_QMEAN, ATM90E26_REG_QMEAN2)},
	[ENERGY_METER_APPARENT_POWER] = {LN(FMT_POWER, ATM90E26_REG_SMEAN, ATM90E26_REG_SMEAN2)},
	[ENERGY_METER_POWER_FACTOR] = {LN(FMT_PF, ATM90E26_REG_POWERF, ATM90E26_REG_POWERF2)},
	[ENERGY_METER_PHASE_ANGLE] = {LN(FMT_ANGLE, ATM90E26_REG_PANGLE, ATM90E26_REG_PANGLE2)},
	[ENERGY_METER_FREQUENCY] = {TOTAL(FMT_FREQ, ATM90E26_REG_FREQ)},
	[ENERGY_METER_ACTIVE_ENERGY_IMPORT] = {TOTAL(FMT_ENERGY, ATM90E26_REG_AP_ENERGY)},
	[ENERGY_METER_ACTIVE_ENERGY_EXPORT] = {TOTAL(FMT_ENERGY, ATM90E26_REG_AN_ENERGY)},
	[ENERGY_METER_REACTIVE_ENERGY_IMPORT] = {TOTAL(FMT_ENERGY, ATM90E26_REG_RP_ENERGY)},
	[ENERGY_METER_REACTIVE_ENERGY_EXPORT] = {TOTAL(FMT_ENERGY, ATM90E26_REG_RN_ENERGY)},
	[ENERGY_METER_ATM90E26_ACTIVE_ENERGY_ABS] = {TOTAL(FMT_ENERGY, ATM90E26_REG_AT_ENERGY)},
	[ENERGY_METER_ATM90E26_REACTIVE_ENERGY_ABS] = {TOTAL(FMT_ENERGY, ATM90E26_REG_RT_ENERGY)},
	[ENERGY_METER_ATM90E26_SYS_STATUS] = {TOTAL(FMT_RAW, ATM90E26_REG_SYS_STATUS)},
	[ENERGY_METER_ATM90E26_EN_STATUS] = {TOTAL(FMT_RAW, ATM90E26_REG_EN_STATUS)},
};

uint16_t atm90e26_checksum(const uint16_t *regs, size_t len)
{
	uint8_t sum = 0U;
	uint8_t xor = 0U;

	/* Low byte: sum of all register bytes; high byte: XOR of all register bytes */
	for (size_t i = 0; i < len; i++) {
		uint8_t hi = (uint8_t)(regs[i] >> 8);
		uint8_t lo = (uint8_t)regs[i];

		sum += hi + lo;
		xor ^= hi ^ lo;
	}

	return ((uint16_t) xor << 8) | sum;
}

/* Recover the last value the chip sent from two matching copies of LastData */
static int atm90e26_recover(const struct device *dev, uint8_t addr, uint16_t *val)
{
	const struct atm90e26_config *cfg = dev->config;
	uint16_t first;
	uint16_t second;
	int ret;

	ret = cfg->bus_io->read(dev, ATM90E26_REG_LAST_DATA, &first);
	if (ret == 0) {
		ret = cfg->bus_io->read(dev, ATM90E26_REG_LAST_DATA, &second);
	}

	if ((ret < 0) || (first != second)) {
		LOG_ERR("Read of 0x%02x could not be verified", addr);
		return -EIO;
	}

	*val = first;

	return 0;
}

static int atm90e26_reg_read(const struct device *dev, uint8_t addr, uint16_t *val)
{
	const struct atm90e26_config *cfg = dev->config;
	struct atm90e26_data *data = dev->data;
	uint16_t last;
	int ret;

	(void)k_mutex_lock(&data->lock, K_FOREVER);

	ret = cfg->bus_io->read(dev, addr, val);
	if ((ret == 0) && IS_ENABLED(CONFIG_ATM90E26_VERIFY_READ)) {
		ret = cfg->bus_io->read(dev, ATM90E26_REG_LAST_DATA, &last);
		if ((ret == 0) && (last != *val)) {
			ret = -EBADMSG;
		}
	}

	/*
	 * The register itself is not read again: energy registers and SysStatus are
	 * cleared by the first read.
	 */
	if (ret == -EBADMSG) {
		ret = atm90e26_recover(dev, addr, val);
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static int atm90e26_reg_write(const struct device *dev, uint8_t addr, uint16_t val)
{
	const struct atm90e26_config *cfg = dev->config;
	struct atm90e26_data *data = dev->data;
	uint16_t last = 0U;
	int ret = -EIO;

	(void)k_mutex_lock(&data->lock, K_FOREVER);

	for (int i = 0; i < WRITE_RETRIES; i++) {
		ret = cfg->bus_io->write(dev, addr, val);
		if (ret < 0) {
			continue;
		}

		ret = cfg->bus_io->read(dev, ATM90E26_REG_LAST_DATA, &last);
		if ((ret == 0) && (last == val)) {
			break;
		}

		ret = -EIO;
	}

	k_mutex_unlock(&data->lock);

	if (ret < 0) {
		LOG_ERR("Write of 0x%04x to 0x%02x failed (%d)", val, addr, ret);
	}

	return ret;
}

static int atm90e26_write_block(const struct device *dev, uint8_t start_reg, uint8_t first,
				const uint16_t *regs, size_t len, uint8_t cs_reg)
{
	int ret;

	/* The start command also restores the power-on values of the block */
	ret = atm90e26_reg_write(dev, start_reg, ATM90E26_CAL_START_CMD);
	if (ret < 0) {
		return ret;
	}

	for (size_t i = 0; i < len; i++) {
		ret = atm90e26_reg_write(dev, first + i, regs[i]);
		if (ret < 0) {
			return ret;
		}
	}

	ret = atm90e26_reg_write(dev, cs_reg, atm90e26_checksum(regs, len));
	if (ret < 0) {
		return ret;
	}

	return atm90e26_reg_write(dev, start_reg, ATM90E26_CAL_CHECK_CMD);
}

static int atm90e26_check_calibration(const struct device *dev)
{
	uint16_t status;
	int ret;

	ret = atm90e26_reg_read(dev, ATM90E26_REG_SYS_STATUS, &status);
	if (ret < 0) {
		return ret;
	}

	if ((status & (ATM90E26_SYS_STATUS_CAL_ERR | ATM90E26_SYS_STATUS_ADJ_ERR)) != 0U) {
		LOG_ERR("Calibration checksum error, SysStatus 0x%04x", status);
		return -EIO;
	}

	return 0;
}

static int atm90e26_configure(const struct device *dev)
{
	const struct atm90e26_config *cfg = dev->config;
	uint16_t val;
	int ret;

	ret = cfg->bus_io->write(dev, ATM90E26_REG_SOFT_RESET, ATM90E26_SOFT_RESET_CMD);
	if (ret < 0) {
		return ret;
	}

	k_msleep(ATM90E26_RESET_DELAY_MS);

	ret = atm90e26_reg_read(dev, ATM90E26_REG_PL_CONST_H, &val);
	if (ret < 0) {
		return ret;
	}

	if (val != ATM90E26_PL_CONST_H_DEFAULT) {
		LOG_ERR("Unexpected PLconstH after reset: 0x%04x", val);
		return -ENODEV;
	}

	ret = atm90e26_reg_write(dev, ATM90E26_REG_FUNC_EN, cfg->func_en);
	if (ret < 0) {
		return ret;
	}

	ret = atm90e26_reg_write(dev, ATM90E26_REG_SAG_TH, cfg->sag_th);
	if (ret < 0) {
		return ret;
	}

	ret = atm90e26_write_block(dev, ATM90E26_REG_CAL_START, ATM90E26_REG_PL_CONST_H,
				   cfg->meter_cal, ATM90E26_METER_CAL_LEN, ATM90E26_REG_CS1);
	if (ret < 0) {
		return ret;
	}

	ret = atm90e26_write_block(dev, ATM90E26_REG_ADJ_START, ATM90E26_REG_U_GAIN, cfg->meas_cal,
				   ATM90E26_MEAS_CAL_LEN, ATM90E26_REG_CS2);
	if (ret < 0) {
		return ret;
	}

	return atm90e26_check_calibration(dev);
}

static bool atm90e26_is_energy_reg(uint8_t reg)
{
	return (reg >= ATM90E26_REG_AP_ENERGY) && (reg <= ATM90E26_REG_RT_ENERGY);
}

/* Read an energy register into its accumulator */
static int atm90e26_accumulate(const struct device *dev, uint8_t reg, uint16_t *raw)
{
	struct atm90e26_data *data = dev->data;
	uint16_t val;
	int ret;

	ret = atm90e26_reg_read(dev, reg, &val);
	if (ret < 0) {
		return ret;
	}

	data->energy[reg - ATM90E26_REG_AP_ENERGY] += val;

	if (raw != NULL) {
		*raw = val;
	}

	return 0;
}

/* PowerF and Pangle use a sign bit and a 15-bit magnitude */
static int64_t atm90e26_sign_magnitude(uint16_t raw)
{
	int64_t magnitude = raw & 0x7FFFU;

	return ((raw & 0x8000U) != 0U) ? -magnitude : magnitude;
}

static int64_t atm90e26_energy_to_micro(const struct atm90e26_config *cfg, uint64_t count)
{
	/* One count is 0.1 CF and one CF is 1 kWh / meter constant: uWh = count * 1e8 / MC */
	uint64_t whole = count / cfg->meter_constant;
	uint64_t rem = count % cfg->meter_constant;

	return (int64_t)(whole * 100000000U + (rem * 100000000U) / cfg->meter_constant);
}

/*
 * Current, power and energy are multiplied by the current-scale ratio when the chip is
 * calibrated to read a fraction of the actual current. The split keeps the product in range.
 */
static int64_t atm90e26_current_scaled(const struct atm90e26_config *cfg, int64_t micro)
{
	int64_t num = cfg->current_scale[0];
	int64_t den = cfg->current_scale[1];

	return (micro / den) * num + ((micro % den) * num) / den;
}

static int atm90e26_read_value(const struct device *dev, const struct atm90e26_chan *chan,
			       int64_t *value)
{
	const struct atm90e26_config *cfg = dev->config;
	struct atm90e26_data *data = dev->data;
	uint16_t raw;
	int ret;

	if (chan->fmt == FMT_ENERGY) {
		ret = atm90e26_accumulate(dev, chan->reg, NULL);
		if (ret == 0) {
			*value = atm90e26_current_scaled(
				cfg,
				atm90e26_energy_to_micro(
					cfg, data->energy[chan->reg - ATM90E26_REG_AP_ENERGY]));
		}
		return ret;
	}

	ret = atm90e26_reg_read(dev, chan->reg, &raw);
	if (ret < 0) {
		return ret;
	}

	switch (chan->fmt) {
	case FMT_URMS:
		/* 0.01 V per LSB */
		*value = (int64_t)raw * 10000;
		break;
	case FMT_IRMS:
		/* 0.001 A per LSB */
		*value = atm90e26_current_scaled(cfg, (int64_t)raw * 1000);
		break;
	case FMT_POWER:
		/* Two's complement, 1 W/var/VA per LSB */
		*value = atm90e26_current_scaled(cfg, (int64_t)(int16_t)raw * 1000000);
		break;
	case FMT_PF:
		/* 0.001 per LSB */
		*value = atm90e26_sign_magnitude(raw) * 1000;
		break;
	case FMT_ANGLE:
		/* 0.1 degree per LSB; -180.0 is reported as 180.0 */
		*value = atm90e26_sign_magnitude(raw) * 100000;
		if (*value == -180000000) {
			*value = 180000000;
		}
		break;
	case FMT_FREQ:
		/* 0.01 Hz per LSB */
		*value = (int64_t)raw * 10000;
		break;
	default:
		*value = raw;
		break;
	}

	return 0;
}

static const struct atm90e26_chan *atm90e26_lookup(const struct energy_meter_channel *channel)
{
	const struct atm90e26_chan *chan;

	if ((channel->quantity >= QUANTITY_COUNT) || (channel->line >= ENERGY_METER_LINE_COUNT)) {
		return NULL;
	}

	chan = &atm90e26_chans[channel->quantity][channel->line];

	return (chan->fmt == FMT_NONE) ? NULL : chan;
}

static int atm90e26_get_info(const struct device *dev, struct energy_meter_info *info)
{
	ARG_UNUSED(dev);

	info->phases = 1;

	for (uint8_t line = 0; line < ENERGY_METER_LINE_COUNT; line++) {
		info->quantities[line] = 0U;

		for (uint8_t q = 0; q < ENERGY_METER_QUANTITY_COMMON_COUNT; q++) {
			if (atm90e26_chans[q][line].fmt != FMT_NONE) {
				info->quantities[line] |= BIT(q);
			}
		}
	}

	return 0;
}

static int atm90e26_get(const struct device *dev, const struct energy_meter_channel *channels,
			int64_t *values, size_t count)
{
	struct atm90e26_data *data = dev->data;
	int ret = 0;

	for (size_t i = 0; i < count; i++) {
		if (atm90e26_lookup(&channels[i]) == NULL) {
			return -ENOTSUP;
		}
	}

	(void)k_mutex_lock(&data->lock, K_FOREVER);
	for (size_t i = 0; (ret == 0) && (i < count); i++) {
		ret = atm90e26_read_value(dev, atm90e26_lookup(&channels[i]), &values[i]);
	}
	k_mutex_unlock(&data->lock);

	return ret;
}

static int atm90e26_reset_energy(const struct device *dev)
{
	struct atm90e26_data *data = dev->data;
	int ret = 0;

	(void)k_mutex_lock(&data->lock, K_FOREVER);

	/* Empty the chip registers, then the accumulators */
	for (uint8_t reg = ATM90E26_REG_AP_ENERGY; (ret == 0) && (reg <= ATM90E26_REG_RT_ENERGY);
	     reg++) {
		ret = atm90e26_accumulate(dev, reg, NULL);
	}

	if (ret == 0) {
		memset(data->energy, 0, sizeof(data->energy));
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

int atm90e26_debug_read(const struct device *dev, uint8_t addr, uint16_t *val)
{
	if (atm90e26_is_energy_reg(addr)) {
		return atm90e26_accumulate(dev, addr, val);
	}

	return atm90e26_reg_read(dev, addr, val);
}

/* Read a calibration block, replace one register and write it back with its checksum */
static int atm90e26_debug_write_block(const struct device *dev, uint8_t start_reg, uint8_t first,
				      size_t len, uint8_t cs_reg, uint8_t addr, uint16_t val)
{
	uint16_t regs[MAX(ATM90E26_METER_CAL_LEN, ATM90E26_MEAS_CAL_LEN)];
	int ret;

	for (size_t i = 0; i < len; i++) {
		ret = atm90e26_reg_read(dev, first + i, &regs[i]);
		if (ret < 0) {
			return ret;
		}
	}

	regs[addr - first] = val;

	ret = atm90e26_write_block(dev, start_reg, first, regs, len, cs_reg);
	if (ret < 0) {
		return ret;
	}

	return atm90e26_check_calibration(dev);
}

int atm90e26_debug_write(const struct device *dev, uint8_t addr, uint16_t val)
{
	struct atm90e26_data *data = dev->data;
	int ret;

	(void)k_mutex_lock(&data->lock, K_FOREVER);

	if ((addr >= ATM90E26_REG_PL_CONST_H) && (addr <= ATM90E26_REG_MMODE)) {
		ret = atm90e26_debug_write_block(dev, ATM90E26_REG_CAL_START,
						 ATM90E26_REG_PL_CONST_H, ATM90E26_METER_CAL_LEN,
						 ATM90E26_REG_CS1, addr, val);
	} else if ((addr >= ATM90E26_REG_U_GAIN) && (addr <= ATM90E26_REG_Q_OFFSET_N)) {
		ret = atm90e26_debug_write_block(dev, ATM90E26_REG_ADJ_START, ATM90E26_REG_U_GAIN,
						 ATM90E26_MEAS_CAL_LEN, ATM90E26_REG_CS2, addr,
						 val);
	} else {
		ret = atm90e26_reg_write(dev, addr, val);
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static int atm90e26_init(const struct device *dev)
{
	const struct atm90e26_config *cfg = dev->config;
	struct atm90e26_data *data = dev->data;
	int ret;

	k_mutex_init(&data->lock);

	ret = cfg->bus_io->check(dev);
	if (ret < 0) {
		return ret;
	}

	ret = atm90e26_configure(dev);
	if (ret < 0) {
		LOG_ERR("Configuration failed (%d)", ret);
		return ret;
	}

#ifdef CONFIG_ATM90E26_EVENTS
	ret = atm90e26_events_init(dev);
	if (ret < 0) {
		return ret;
	}
#endif

	return 0;
}

static DEVICE_API(energy_meter, atm90e26_api) = {
	.get_info = atm90e26_get_info,
	.get = atm90e26_get,
	.reset_energy = atm90e26_reset_energy,
#ifdef CONFIG_ATM90E26_EVENTS
	.set_event_callback = atm90e26_set_event_callback,
#endif
};

bool atm90e26_is_device(const struct device *dev)
{
	return dev->api == &atm90e26_api;
}

#define ATM90E26_BUS(inst)                                                                         \
	COND_CODE_1(DT_INST_ON_BUS(inst, spi),                                                     \
		    ({.spi = SPI_DT_SPEC_INST_GET(inst, ATM90E26_SPI_OPERATION)}),                 \
		    ({.uart = DEVICE_DT_GET(DT_INST_BUS(inst))}))

#define ATM90E26_BUS_IO(inst)                                                                      \
	COND_CODE_1(DT_INST_ON_BUS(inst, spi), (&atm90e26_bus_io_spi), (&atm90e26_bus_io_uart))

#define ATM90E26_GPIO(inst, name) GPIO_DT_SPEC_INST_GET_OR(inst, name##_gpios, {0})

#define ATM90E26_EVENT_GPIOS(inst)                                                                 \
	IF_ENABLED(CONFIG_ATM90E26_EVENTS,                                                         \
		   (.irq_gpios = {                                                                 \
			    [ATM90E26_IRQ] = ATM90E26_GPIO(inst, irq),                             \
			    [ATM90E26_WARN] = ATM90E26_GPIO(inst, wrn_out),                        \
			    [ATM90E26_CF1] = ATM90E26_GPIO(inst, cf1),                             \
			    [ATM90E26_CF2] = ATM90E26_GPIO(inst, cf2),                             \
		    },))

#define ATM90E26_DEFINE(inst)                                                                      \
	BUILD_ASSERT(DT_INST_PROP(inst, meter_constant) > 0, "meter-constant must not be 0");      \
	BUILD_ASSERT(IN_RANGE(DT_INST_PROP_BY_IDX(inst, current_scale, 0), 1, UINT16_MAX) &&       \
			     IN_RANGE(DT_INST_PROP_BY_IDX(inst, current_scale, 1), 1, UINT16_MAX), \
		     "current-scale values must be from 1 to 65535");                              \
                                                                                                   \
	static const struct atm90e26_config atm90e26_config_##inst = {                             \
		.bus = ATM90E26_BUS(inst),                                                         \
		.bus_io = ATM90E26_BUS_IO(inst),                                                   \
		.meter_constant = DT_INST_PROP(inst, meter_constant),                              \
		.current_scale = DT_INST_PROP(inst, current_scale),                                \
		.func_en = DT_INST_PROP(inst, func_en),                                            \
		.sag_th = DT_INST_PROP(inst, sag_th),                                              \
		.meter_cal =                                                                       \
			{                                                                          \
				DT_INST_PROP(inst, pl_constant) >> 16,                             \
				DT_INST_PROP(inst, pl_constant) & 0xFFFF,                          \
				DT_INST_PROP(inst, l_gain),                                        \
				DT_INST_PROP(inst, l_phi),                                         \
				DT_INST_PROP(inst, n_gain),                                        \
				DT_INST_PROP(inst, n_phi),                                         \
				DT_INST_PROP(inst, p_start_th),                                    \
				DT_INST_PROP(inst, p_nol_th),                                      \
				DT_INST_PROP(inst, q_start_th),                                    \
				DT_INST_PROP(inst, q_nol_th),                                      \
				DT_INST_PROP(inst, mmode),                                         \
			},                                                                         \
		.meas_cal =                                                                        \
			{                                                                          \
				DT_INST_PROP(inst, u_gain),                                        \
				DT_INST_PROP(inst, i_gain_l),                                      \
				DT_INST_PROP(inst, i_gain_n),                                      \
				DT_INST_PROP(inst, u_offset),                                      \
				DT_INST_PROP(inst, i_offset_l),                                    \
				DT_INST_PROP(inst, i_offset_n),                                    \
				DT_INST_PROP(inst, p_offset_l),                                    \
				DT_INST_PROP(inst, q_offset_l),                                    \
				DT_INST_PROP(inst, p_offset_n),                                    \
				DT_INST_PROP(inst, q_offset_n),                                    \
			},                                                                         \
		ATM90E26_EVENT_GPIOS(inst)};                                                       \
                                                                                                   \
	static struct atm90e26_data atm90e26_data_##inst;                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, atm90e26_init, NULL, &atm90e26_data_##inst,                    \
			      &atm90e26_config_##inst, POST_KERNEL,                                \
			      CONFIG_ENERGY_METER_INIT_PRIORITY, &atm90e26_api);

DT_INST_FOREACH_STATUS_OKAY(ATM90E26_DEFINE)

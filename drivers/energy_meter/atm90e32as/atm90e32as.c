/*
 * Copyright (c) 2025 Pedro Ramos
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_atm90e32as

#include <string.h>

#include <zephyr/drivers/energy_meter.h>
#include <zephyr/drivers/energy_meter/atm90e32as.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/byteorder.h>

#include "atm90e32as.h"

LOG_MODULE_REGISTER(atm90e32as, CONFIG_ENERGY_METER_LOG_LEVEL);

#define WRITE_RETRIES 3

#define QUANTITY_COUNT ENERGY_METER_ATM90E32AS_QUANTITY_COUNT

enum atm90e32as_fmt {
	FMT_NONE = 0,
	FMT_URMS,
	FMT_IRMS,
	FMT_POWER,
	FMT_PF,
	FMT_PANGLE,
	FMT_UANGLE,
	FMT_UPEAK,
	FMT_IPEAK,
	FMT_ENERGY,
	FMT_FREQ,
	FMT_TEMP,
	FMT_STATE,
	FMT_INT_STATE,
};

/* How a value is read and converted. lsb is 0 when the value has no lower word. */
struct atm90e32as_chan {
	uint8_t fmt;
	uint8_t reg;
	uint8_t lsb;
	uint8_t phase;
};

#define LINE_A     ENERGY_METER_LINE_A
#define LINE_B     ENERGY_METER_LINE_B
#define LINE_C     ENERGY_METER_LINE_C
#define LINE_N     ENERGY_METER_LINE_N
#define LINE_TOTAL ENERGY_METER_LINE_TOTAL

/* Lower word register n positions after l, or 0 when the value has no lower word */
#define LSB_AT(l, n) (((l) == 0) ? 0 : ((l) + (n)))

#define AT(line, f, r, l, p) [line] = {.fmt = (f), .reg = (r), .lsb = (l), .phase = (p)}

/* Phases A, B and C backed by consecutive registers */
#define ABC(f, r, l)                                                                               \
	AT(LINE_A, f, r, l, 0), AT(LINE_B, f, (r) + 1, LSB_AT(l, 1), 1),                           \
		AT(LINE_C, f, (r) + 2, LSB_AT(l, 2), 2)

/* Total followed by phases A, B and C, backed by consecutive registers */
#define TABC(f, r, l) AT(LINE_TOTAL, f, r, l, 0), ABC(f, (r) + 1, LSB_AT(l, 1))

static const struct atm90e32as_chan atm90e32as_chans[QUANTITY_COUNT][ENERGY_METER_LINE_COUNT] = {
	[ENERGY_METER_VOLTAGE] = {ABC(FMT_URMS, ATM90E32AS_REG_URMS_A, ATM90E32AS_REG_URMS_A_LSB)},
	[ENERGY_METER_CURRENT] = {ABC(FMT_IRMS, ATM90E32AS_REG_IRMS_A, ATM90E32AS_REG_IRMS_A_LSB),
				  AT(LINE_N, FMT_IRMS, ATM90E32AS_REG_IRMS_N, 0, 0)},
	[ENERGY_METER_ACTIVE_POWER] = {TABC(FMT_POWER, ATM90E32AS_REG_PMEAN_T,
					    ATM90E32AS_REG_PMEAN_T_LSB)},
	[ENERGY_METER_REACTIVE_POWER] = {TABC(FMT_POWER, ATM90E32AS_REG_QMEAN_T,
					      ATM90E32AS_REG_QMEAN_T_LSB)},
	[ENERGY_METER_APPARENT_POWER] = {TABC(FMT_POWER, ATM90E32AS_REG_SMEAN_T,
					      ATM90E32AS_REG_SMEAN_T_LSB)},
	[ENERGY_METER_POWER_FACTOR] = {TABC(FMT_PF, ATM90E32AS_REG_PFMEAN_T, 0)},
	[ENERGY_METER_PHASE_ANGLE] = {ABC(FMT_PANGLE, ATM90E32AS_REG_PANGLE_A, 0)},
	[ENERGY_METER_VOLTAGE_ANGLE] = {ABC(FMT_UANGLE, ATM90E32AS_REG_UANGLE_A, 0)},
	[ENERGY_METER_FREQUENCY] = {AT(LINE_TOTAL, FMT_FREQ, ATM90E32AS_REG_FREQ, 0, 0)},
	[ENERGY_METER_ACTIVE_ENERGY_IMPORT] = {TABC(FMT_ENERGY, ATM90E32AS_REG_AP_ENERGY_T, 0)},
	[ENERGY_METER_ACTIVE_ENERGY_EXPORT] = {TABC(FMT_ENERGY, ATM90E32AS_REG_AN_ENERGY_T, 0)},
	[ENERGY_METER_REACTIVE_ENERGY_IMPORT] = {TABC(FMT_ENERGY, ATM90E32AS_REG_RP_ENERGY_T, 0)},
	[ENERGY_METER_REACTIVE_ENERGY_EXPORT] = {TABC(FMT_ENERGY, ATM90E32AS_REG_RN_ENERGY_T, 0)},
	[ENERGY_METER_APPARENT_ENERGY] = {TABC(FMT_ENERGY, ATM90E32AS_REG_SA_ENERGY_T, 0)},
	[ENERGY_METER_TEMPERATURE] = {AT(LINE_TOTAL, FMT_TEMP, ATM90E32AS_REG_TEMP, 0, 0)},
	[ENERGY_METER_ATM90E32AS_FUND_POWER] = {TABC(FMT_POWER, ATM90E32AS_REG_PMEAN_TF,
						     ATM90E32AS_REG_PMEAN_TF_LSB)},
	[ENERGY_METER_ATM90E32AS_HARM_POWER] = {TABC(FMT_POWER, ATM90E32AS_REG_PMEAN_TH,
						     ATM90E32AS_REG_PMEAN_TH_LSB)},
	[ENERGY_METER_ATM90E32AS_FUND_ENERGY_IMPORT] = {TABC(FMT_ENERGY,
							     ATM90E32AS_REG_AP_ENERGY_TF, 0)},
	[ENERGY_METER_ATM90E32AS_FUND_ENERGY_EXPORT] = {TABC(FMT_ENERGY,
							     ATM90E32AS_REG_AN_ENERGY_TF, 0)},
	[ENERGY_METER_ATM90E32AS_HARM_ENERGY_IMPORT] = {TABC(FMT_ENERGY,
							     ATM90E32AS_REG_AP_ENERGY_TH, 0)},
	[ENERGY_METER_ATM90E32AS_HARM_ENERGY_EXPORT] = {TABC(FMT_ENERGY,
							     ATM90E32AS_REG_AN_ENERGY_TH, 0)},
	[ENERGY_METER_ATM90E32AS_VOLTAGE_PEAK] = {ABC(FMT_UPEAK, ATM90E32AS_REG_UPEAK_A, 0)},
	[ENERGY_METER_ATM90E32AS_CURRENT_PEAK] = {ABC(FMT_IPEAK, ATM90E32AS_REG_IPEAK_A, 0)},
	[ENERGY_METER_ATM90E32AS_EMM_STATE] = {AT(LINE_TOTAL, FMT_STATE, ATM90E32AS_REG_EMM_STATE0,
						  0, 0)},
	[ENERGY_METER_ATM90E32AS_EMM_INT_STATE] = {AT(LINE_TOTAL, FMT_INT_STATE,
						      ATM90E32AS_REG_EMM_INT_STATE0, 0, 0)},
};

static int atm90e32as_transfer(const struct device *dev, uint16_t cmd, uint16_t wval,
			       uint16_t *rval)
{
	const struct atm90e32as_config *cfg = dev->config;
	uint8_t tx[4];
	uint8_t rx[4];
	const struct spi_buf tx_buf = {.buf = tx, .len = sizeof(tx)};
	const struct spi_buf rx_buf = {.buf = rx, .len = sizeof(rx)};
	const struct spi_buf_set tx_set = {.buffers = &tx_buf, .count = 1};
	const struct spi_buf_set rx_set = {.buffers = &rx_buf, .count = 1};
	int ret;

	sys_put_be16(cmd, &tx[0]);
	sys_put_be16(wval, &tx[2]);

	ret = spi_transceive_dt(&cfg->bus, &tx_set, (rval != NULL) ? &rx_set : NULL);
	if (ret < 0) {
		LOG_ERR("SPI transfer to 0x%03x failed (%d)",
			(unsigned int)(cmd & ATM90E32AS_SPI_ADDR_MASK), ret);
		return ret;
	}

	if (rval != NULL) {
		*rval = sys_get_be16(&rx[2]);
	}

	return 0;
}

static inline int atm90e32as_raw_read(const struct device *dev, uint16_t addr, uint16_t *val)
{
	return atm90e32as_transfer(dev, ATM90E32AS_SPI_READ | (addr & ATM90E32AS_SPI_ADDR_MASK), 0,
				   val);
}

static inline int atm90e32as_raw_write(const struct device *dev, uint16_t addr, uint16_t val)
{
	return atm90e32as_transfer(dev, addr & ATM90E32AS_SPI_ADDR_MASK, val, NULL);
}

int atm90e32as_reg_read(const struct device *dev, uint16_t addr, uint16_t *val)
{
	struct atm90e32as_data *data = dev->data;
	uint16_t last;
	uint16_t again;
	int ret;

	if (!IS_ENABLED(CONFIG_ATM90E32AS_VERIFY_READ)) {
		return atm90e32as_raw_read(dev, addr, val);
	}

	(void)k_mutex_lock(&data->lock, K_FOREVER);

	ret = atm90e32as_raw_read(dev, addr, val);
	if (ret < 0) {
		goto out;
	}

	ret = atm90e32as_raw_read(dev, ATM90E32AS_REG_LAST_SPI_DATA, &last);
	if ((ret < 0) || (last == *val)) {
		goto out;
	}

	/*
	 * One of the two transfers was corrupted. Reading LastSPIData does not change it, so
	 * a second copy that matches the first one is the value the chip sent. Reading addr
	 * again is not an option: energy registers are cleared by the first read.
	 */
	ret = atm90e32as_raw_read(dev, ATM90E32AS_REG_LAST_SPI_DATA, &again);
	if (ret < 0) {
		goto out;
	}

	if (again != last) {
		LOG_ERR("Read of 0x%02x could not be verified", addr);
		ret = -EIO;
		goto out;
	}

	*val = last;

out:
	k_mutex_unlock(&data->lock);
	return ret;
}

int atm90e32as_reg_write(const struct device *dev, uint16_t addr, uint16_t val)
{
	struct atm90e32as_data *data = dev->data;
	uint16_t last = 0U;
	int ret = -EIO;

	(void)k_mutex_lock(&data->lock, K_FOREVER);

	for (int i = 0; i < WRITE_RETRIES; i++) {
		ret = atm90e32as_raw_write(dev, addr, val);
		if (ret < 0) {
			continue;
		}

		ret = atm90e32as_raw_read(dev, ATM90E32AS_REG_LAST_SPI_DATA, &last);
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

static bool atm90e32as_is_energy_reg(uint8_t reg)
{
	return ((reg >= ATM90E32AS_REG_AP_ENERGY_T) && (reg < ATM90E32AS_REG_SA_ENERGY_T + 4U)) ||
	       ((reg >= ATM90E32AS_REG_AP_ENERGY_TF) && (reg < ATM90E32AS_REG_AN_ENERGY_TH + 4U));
}

static size_t atm90e32as_energy_index(uint8_t reg)
{
	if (reg < ATM90E32AS_REG_AP_ENERGY_TF) {
		return reg - ATM90E32AS_REG_AP_ENERGY_T;
	}

	return (reg - ATM90E32AS_REG_AP_ENERGY_TF) + 20U;
}

/* Read an energy register into its accumulator */
static int atm90e32as_accumulate(const struct device *dev, uint8_t reg, uint16_t *raw)
{
	struct atm90e32as_data *data = dev->data;
	uint16_t val;
	int ret;

	ret = atm90e32as_reg_read(dev, reg, &val);
	if (ret < 0) {
		return ret;
	}

	data->energy[atm90e32as_energy_index(reg)] += val;

	if (raw != NULL) {
		*raw = val;
	}

	return 0;
}

/* Read every energy register; with discard, the accumulators are zeroed afterwards */
static int atm90e32as_collect_energy(const struct device *dev, bool discard)
{
	struct atm90e32as_data *data = dev->data;
	int ret = 0;

	(void)k_mutex_lock(&data->lock, K_FOREVER);

	for (uint16_t reg = ATM90E32AS_REG_AP_ENERGY_T; reg <= ATM90E32AS_REG_AN_ENERGY_TH + 3U;
	     reg++) {
		if (!atm90e32as_is_energy_reg(reg)) {
			continue;
		}

		ret = atm90e32as_accumulate(dev, reg, NULL);
		if (ret < 0) {
			break;
		}
	}

	if ((ret == 0) && discard) {
		memset(data->energy, 0, sizeof(data->energy));
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static int64_t atm90e32as_energy_to_micro(const struct atm90e32as_config *cfg, uint64_t count)
{
	/* One count is 0.01 CF and one CF is 1 kWh / meter constant: uWh = count * 1e7 / MC */
	uint64_t whole = count / cfg->meter_constant;
	uint64_t rem = count % cfg->meter_constant;

	return (int64_t)(whole * 10000000U + (rem * 10000000U) / cfg->meter_constant);
}

/*
 * Current, power and energy are multiplied by the current-scale ratio when the chip is
 * calibrated to read a fraction of the actual current. The split keeps the product in range.
 */
static int64_t atm90e32as_current_scaled(const struct atm90e32as_config *cfg, int64_t micro)
{
	int64_t num = cfg->current_scale[0];
	int64_t den = cfg->current_scale[1];

	return (micro / den) * num + ((micro % den) * num) / den;
}

static int64_t atm90e32as_convert(const struct atm90e32as_config *cfg,
				  const struct atm90e32as_chan *chan, uint16_t raw, uint16_t lsb)
{
	switch (chan->fmt) {
	case FMT_URMS:
		/* 0.01 V per LSB; bits 15:8 of the lower word add 0.01/256 V per LSB */
		return (int64_t)raw * 10000 + (((int64_t)(lsb >> 8) * 10000) / 256);
	case FMT_IRMS:
		/* 0.001 A per LSB; bits 15:8 of the lower word add 0.001/256 A per LSB */
		return atm90e32as_current_scaled(cfg, (int64_t)raw * 1000 +
							      (((int64_t)(lsb >> 8) * 1000) / 256));
	case FMT_POWER:
		/* 32-bit two's complement, 0.00032 W/var/VA per LSB */
		return atm90e32as_current_scaled(
			cfg, (int64_t)(int32_t)(((uint32_t)raw << 16) | lsb) * 320);
	case FMT_PF:
		/* Two's complement, 0.001 per LSB */
		return (int64_t)(int16_t)raw * 1000;
	case FMT_PANGLE:
		/* 0.1 degree per LSB, from 0 to 360.0 degrees; reported from -179.9 to 180.0 */
		return ((raw > 1800U) ? ((int64_t)raw - 3600) : (int64_t)raw) * 100000;
	case FMT_UANGLE:
		/* 0.1 degree per LSB, from 0 to 360.0 degrees; 360.0 is reported as 0 */
		return (int64_t)(raw % 3600U) * 100000;
	case FMT_UPEAK:
		/* UPeak = raw * Ugain / (100 * 2^13) V */
		return ((int64_t)raw * cfg->u_gain[chan->phase] * 10000) / 8192;
	case FMT_IPEAK:
		/* IPeak = raw * Igain / (1000 * 2^13) A */
		return atm90e32as_current_scaled(
			cfg, ((int64_t)raw * cfg->i_gain[chan->phase] * 1000) / 8192);
	case FMT_FREQ:
		/* 0.01 Hz per LSB */
		return (int64_t)raw * 10000;
	case FMT_TEMP:
		/* 1 degree Celsius per LSB */
		return (int64_t)(int16_t)raw * 1000000;
	default:
		return 0;
	}
}

static int atm90e32as_read_pair(const struct device *dev, uint8_t first, uint16_t vals[2])
{
	int ret;

	ret = atm90e32as_reg_read(dev, first, &vals[0]);
	if (ret < 0) {
		return ret;
	}

	return atm90e32as_reg_read(dev, first + 1U, &vals[1]);
}

static int atm90e32as_read_value(const struct device *dev, const struct atm90e32as_chan *chan,
				 int64_t *value)
{
	const struct atm90e32as_config *cfg = dev->config;
	struct atm90e32as_data *data = dev->data;
	uint16_t vals[2];
	uint16_t raw;
	uint16_t lsb = 0U;
	int ret;

	switch (chan->fmt) {
	case FMT_ENERGY:
		ret = atm90e32as_accumulate(dev, chan->reg, NULL);
		if (ret == 0) {
			*value = atm90e32as_current_scaled(
				cfg,
				atm90e32as_energy_to_micro(
					cfg, data->energy[atm90e32as_energy_index(chan->reg)]));
		}
		return ret;
	case FMT_STATE:
	case FMT_INT_STATE:
		ret = atm90e32as_read_pair(dev, chan->reg, vals);
		if (ret < 0) {
			return ret;
		}

		/* The interrupt flags are write-one-to-clear */
		for (int i = 0; (chan->fmt == FMT_INT_STATE) && (i < 2); i++) {
			if (vals[i] != 0U) {
				ret = atm90e32as_reg_write(dev, chan->reg + i, vals[i]);
				if (ret < 0) {
					return ret;
				}
			}
		}

		*value = ((int64_t)vals[0] << 16) | vals[1];
		return 0;
	default:
		break;
	}

	ret = atm90e32as_reg_read(dev, chan->reg, &raw);
	if ((ret == 0) && (chan->lsb != 0U)) {
		ret = atm90e32as_reg_read(dev, chan->lsb, &lsb);
	}

	if ((ret == 0) && (chan->fmt == FMT_TEMP) && (raw == ATM90E32AS_TEMP_NOT_READY)) {
		/* No measurement since the last reset */
		ret = -EAGAIN;
	}

	if (ret == 0) {
		*value = atm90e32as_convert(cfg, chan, raw, lsb);
	}

	return ret;
}

static const struct atm90e32as_chan *atm90e32as_lookup(const struct energy_meter_channel *channel)
{
	const struct atm90e32as_chan *chan;

	if ((channel->quantity >= QUANTITY_COUNT) || (channel->line >= ENERGY_METER_LINE_COUNT)) {
		return NULL;
	}

	chan = &atm90e32as_chans[channel->quantity][channel->line];

	return (chan->fmt == FMT_NONE) ? NULL : chan;
}

static int atm90e32as_get_info(const struct device *dev, struct energy_meter_info *info)
{
	ARG_UNUSED(dev);

	info->phases = ATM90E32AS_PHASES;

	for (uint8_t line = 0; line < ENERGY_METER_LINE_COUNT; line++) {
		info->quantities[line] = 0U;

		for (uint8_t q = 0; q < ENERGY_METER_QUANTITY_COMMON_COUNT; q++) {
			if (atm90e32as_chans[q][line].fmt != FMT_NONE) {
				info->quantities[line] |= BIT(q);
			}
		}
	}

	return 0;
}

static int atm90e32as_check_active(const struct device *dev)
{
#ifdef CONFIG_PM_DEVICE
	enum pm_device_state state;

	if ((pm_device_state_get(dev, &state) == 0) && (state != PM_DEVICE_STATE_ACTIVE)) {
		return -EBUSY;
	}
#else
	ARG_UNUSED(dev);
#endif

	return 0;
}

static int atm90e32as_get(const struct device *dev, const struct energy_meter_channel *channels,
			  int64_t *values, size_t count)
{
	struct atm90e32as_data *data = dev->data;
	int ret;

	for (size_t i = 0; i < count; i++) {
		if (atm90e32as_lookup(&channels[i]) == NULL) {
			return -ENOTSUP;
		}
	}

	ret = pm_device_runtime_get(dev);
	if (ret < 0) {
		return ret;
	}

	ret = atm90e32as_check_active(dev);

	(void)k_mutex_lock(&data->lock, K_FOREVER);
	for (size_t i = 0; (ret == 0) && (i < count); i++) {
		ret = atm90e32as_read_value(dev, atm90e32as_lookup(&channels[i]), &values[i]);
	}
	k_mutex_unlock(&data->lock);

	(void)pm_device_runtime_put(dev);

	return ret;
}

static int atm90e32as_reset_energy(const struct device *dev)
{
	int ret;

	ret = atm90e32as_check_active(dev);
	if (ret < 0) {
		return ret;
	}

	/* Empty the chip registers, then the accumulators */
	return atm90e32as_collect_energy(dev, true);
}

int atm90e32as_debug_read(const struct device *dev, uint8_t addr, uint16_t *val)
{
	struct atm90e32as_data *data = dev->data;
	int ret;

	(void)k_mutex_lock(&data->lock, K_FOREVER);

	if (atm90e32as_is_energy_reg(addr)) {
		ret = atm90e32as_accumulate(dev, addr, val);
	} else {
		ret = atm90e32as_reg_read(dev, addr, val);
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

int atm90e32as_debug_write(const struct device *dev, uint8_t addr, uint16_t val)
{
	struct atm90e32as_data *data = dev->data;
	int ret;

	if (addr >= ATM90E32AS_REG_SOFT_RESET) {
		return atm90e32as_reg_write(dev, addr, val);
	}

	(void)k_mutex_lock(&data->lock, K_FOREVER);

	ret = atm90e32as_reg_write(dev, ATM90E32AS_REG_CFG_REG_ACC_EN,
				   ATM90E32AS_CFG_ACCESS_ENABLE);
	if (ret == 0) {
		ret = atm90e32as_reg_write(dev, addr, val);
		ret |= atm90e32as_reg_write(dev, ATM90E32AS_REG_CFG_REG_ACC_EN,
					    ATM90E32AS_CFG_ACCESS_DISABLE);
	}

	k_mutex_unlock(&data->lock);

	return (ret < 0) ? -EIO : 0;
}

static int atm90e32as_configure(const struct device *dev)
{
	const struct atm90e32as_config *cfg = dev->config;
	uint16_t id;
	int ret;

	/* Leaving Idle mode resets the chip; a soft reset covers boards without PM pins */
	ret = atm90e32as_raw_write(dev, ATM90E32AS_REG_SOFT_RESET, ATM90E32AS_SOFT_RESET_CMD);
	if (ret < 0) {
		return ret;
	}

	k_msleep(ATM90E32AS_RESET_DELAY_MS);

	ret = atm90e32as_raw_read(dev, ATM90E32AS_REG_PL_CONST_H, &id);
	if (ret < 0) {
		return ret;
	}

	if (id != ATM90E32AS_PL_CONST_H_DEFAULT) {
		LOG_ERR("Unexpected PLconstH after reset: 0x%04x", id);
		return -ENODEV;
	}

	ret = atm90e32as_reg_write(dev, ATM90E32AS_REG_CFG_REG_ACC_EN,
				   ATM90E32AS_CFG_ACCESS_ENABLE);
	if (ret < 0) {
		return ret;
	}

	for (size_t i = 0; i < cfg->init_regs_len; i++) {
		ret = atm90e32as_reg_write(dev, cfg->init_regs[i].addr, cfg->init_regs[i].val);
		if (ret < 0) {
			return ret;
		}
	}

	ret = atm90e32as_reg_write(dev, ATM90E32AS_REG_METER_EN, ATM90E32AS_METER_EN_ON);
	if (ret < 0) {
		return ret;
	}

	return atm90e32as_reg_write(dev, ATM90E32AS_REG_CFG_REG_ACC_EN,
				    ATM90E32AS_CFG_ACCESS_DISABLE);
}

static inline bool atm90e32as_has_pm_pins(const struct atm90e32as_config *cfg)
{
	return (cfg->pm0.port != NULL) && (cfg->pm1.port != NULL);
}

static int atm90e32as_set_pm_pins(const struct atm90e32as_config *cfg, int value)
{
	int ret;

	ret = gpio_pin_set_dt(&cfg->pm0, value);
	if (ret < 0) {
		return ret;
	}

	return gpio_pin_set_dt(&cfg->pm1, value);
}

static int atm90e32as_resume(const struct device *dev)
{
	const struct atm90e32as_config *cfg = dev->config;
	int ret;

	if (atm90e32as_has_pm_pins(cfg)) {
		ret = atm90e32as_set_pm_pins(cfg, 1);
		if (ret < 0) {
			return ret;
		}

		k_msleep(ATM90E32AS_RESET_DELAY_MS);
	}

	ret = atm90e32as_configure(dev);
	if (ret < 0) {
		LOG_ERR("Configuration failed (%d)", ret);
		return ret;
	}

#ifdef CONFIG_ATM90E32AS_EVENTS
	atm90e32as_events_enable(dev, true);
#endif

	return 0;
}

static int atm90e32as_suspend(const struct device *dev)
{
	const struct atm90e32as_config *cfg = dev->config;
	int ret;

	if (!atm90e32as_has_pm_pins(cfg)) {
		return -ENOTSUP;
	}

	/* Idle mode resets the chip; collect the energy it has accumulated so far */
	ret = atm90e32as_collect_energy(dev, false);
	if (ret < 0) {
		return ret;
	}

#ifdef CONFIG_ATM90E32AS_EVENTS
	atm90e32as_events_enable(dev, false);
#endif

	return atm90e32as_set_pm_pins(cfg, 0);
}

static int atm90e32as_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		return atm90e32as_resume(dev);
	case PM_DEVICE_ACTION_SUSPEND:
		return atm90e32as_suspend(dev);
	default:
		return -ENOTSUP;
	}
}

static int atm90e32as_init(const struct device *dev)
{
	const struct atm90e32as_config *cfg = dev->config;
	struct atm90e32as_data *data = dev->data;
	int ret;

	k_mutex_init(&data->lock);

	if (!spi_is_ready_dt(&cfg->bus)) {
		LOG_ERR("SPI bus not ready");
		return -ENODEV;
	}

	if ((cfg->pm0.port != NULL) != (cfg->pm1.port != NULL)) {
		LOG_ERR("pm0-gpios and pm1-gpios must be set together");
		return -EINVAL;
	}

	if (atm90e32as_has_pm_pins(cfg)) {
		if (!gpio_is_ready_dt(&cfg->pm0) || !gpio_is_ready_dt(&cfg->pm1)) {
			LOG_ERR("PM GPIOs not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&cfg->pm0, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			return ret;
		}

		ret = gpio_pin_configure_dt(&cfg->pm1, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			return ret;
		}
	}

#ifdef CONFIG_ATM90E32AS_EVENTS
	ret = atm90e32as_events_init(dev);
	if (ret < 0) {
		return ret;
	}
#endif

	return pm_device_driver_init(dev, atm90e32as_pm_action);
}

static DEVICE_API(energy_meter, atm90e32as_api) = {
	.get_info = atm90e32as_get_info,
	.get = atm90e32as_get,
	.reset_energy = atm90e32as_reset_energy,
#ifdef CONFIG_ATM90E32AS_EVENTS
	.set_event_callback = atm90e32as_set_event_callback,
#endif
};

bool atm90e32as_is_device(const struct device *dev)
{
	return dev->api == &atm90e32as_api;
}

#define REG(a, v) {.addr = (a), .val = (v)}

#define REG_ABC(inst, prop, first, stride)                                                         \
	REG((first), DT_INST_PROP_BY_IDX(inst, prop, 0)),                                          \
		REG((first) + (stride), DT_INST_PROP_BY_IDX(inst, prop, 1)),                       \
		REG((first) + 2 * (stride), DT_INST_PROP_BY_IDX(inst, prop, 2))

#define ATM90E32AS_INIT_REGS(inst)                                                                 \
	REG(ATM90E32AS_REG_CHANNEL_MAP_I, DT_INST_PROP(inst, channel_map_i)),                      \
		REG(ATM90E32AS_REG_CHANNEL_MAP_U, DT_INST_PROP(inst, channel_map_u)),              \
		REG(ATM90E32AS_REG_SAG_PEAK_DET, DT_INST_PROP(inst, sag_peak_det_cfg)),            \
		REG(ATM90E32AS_REG_OV_TH, DT_INST_PROP(inst, ov_th_cfg)),                          \
		REG(ATM90E32AS_REG_ZX_CONFIG, DT_INST_PROP(inst, zx_config)),                      \
		REG(ATM90E32AS_REG_SAG_TH, DT_INST_PROP(inst, sag_th)),                            \
		REG(ATM90E32AS_REG_PHASE_LOSS_TH, DT_INST_PROP(inst, phase_loss_th)),              \
		REG(ATM90E32AS_REG_IN_WARN_TH, DT_INST_PROP(inst, in_warn_th)),                    \
		REG(ATM90E32AS_REG_OI_TH, DT_INST_PROP(inst, oi_th)),                              \
		REG(ATM90E32AS_REG_FREQ_LO_TH, DT_INST_PROP(inst, freq_lo_th)),                    \
		REG(ATM90E32AS_REG_FREQ_HI_TH, DT_INST_PROP(inst, freq_hi_th)),                    \
		REG(ATM90E32AS_REG_PM_PWR_CTRL, DT_INST_PROP(inst, pm_pwr_ctrl)),                  \
		REG(ATM90E32AS_REG_IRQ0_MERGE_CFG, DT_INST_PROP(inst, irq0_merge_cfg)),            \
		REG(ATM90E32AS_REG_DETECT_CTRL, DT_INST_PROP(inst, detect_ctrl)),                  \
		REG_ABC(inst, detect_th, ATM90E32AS_REG_DETECT_TH1, 1),                            \
		REG_ABC(inst, idc_offset, ATM90E32AS_REG_IDC_OFFSET_A, 1),                         \
		REG_ABC(inst, udc_offset, ATM90E32AS_REG_UDC_OFFSET_A, 1),                         \
		REG(ATM90E32AS_REG_U_GAIN_TAB, DT_INST_PROP(inst, u_gain_tab)),                    \
		REG(ATM90E32AS_REG_U_GAIN_TC, DT_INST_PROP(inst, u_gain_tc)),                      \
		REG(ATM90E32AS_REG_PHI_FREQ_COMP, DT_INST_PROP(inst, phi_freq_comp)),              \
		REG(ATM90E32AS_REG_LOG_IRMS0, DT_INST_PROP_BY_IDX(inst, log_irms, 0)),             \
		REG(ATM90E32AS_REG_LOG_IRMS1, DT_INST_PROP_BY_IDX(inst, log_irms, 1)),             \
		REG(ATM90E32AS_REG_F0, DT_INST_PROP(inst, f0)),                                    \
		REG(ATM90E32AS_REG_T0, DT_INST_PROP(inst, t0)),                                    \
		REG_ABC(inst, phi_irms01, ATM90E32AS_REG_PHI_A_IRMS01, ATM90E32AS_SEGMENT_STRIDE), \
		REG_ABC(inst, phi_irms2, ATM90E32AS_REG_PHI_A_IRMS2, ATM90E32AS_SEGMENT_STRIDE),   \
		REG_ABC(inst, gain_irms01, ATM90E32AS_REG_GAIN_A_IRMS01,                           \
			ATM90E32AS_SEGMENT_STRIDE),                                                \
		REG_ABC(inst, gain_irms2, ATM90E32AS_REG_GAIN_A_IRMS2, ATM90E32AS_SEGMENT_STRIDE), \
		REG(ATM90E32AS_REG_PL_CONST_H, DT_INST_PROP(inst, pl_constant) >> 16),             \
		REG(ATM90E32AS_REG_PL_CONST_L, DT_INST_PROP(inst, pl_constant) & 0xFFFF),          \
		REG(ATM90E32AS_REG_MMODE0, DT_INST_PROP(inst, mmode0)),                            \
		REG(ATM90E32AS_REG_MMODE1, DT_INST_PROP(inst, mmode1)),                            \
		REG(ATM90E32AS_REG_P_START_TH, DT_INST_PROP(inst, p_start_th)),                    \
		REG(ATM90E32AS_REG_Q_START_TH, DT_INST_PROP(inst, q_start_th)),                    \
		REG(ATM90E32AS_REG_S_START_TH, DT_INST_PROP(inst, s_start_th)),                    \
		REG(ATM90E32AS_REG_P_PHASE_TH, DT_INST_PROP(inst, p_phase_th)),                    \
		REG(ATM90E32AS_REG_Q_PHASE_TH, DT_INST_PROP(inst, q_phase_th)),                    \
		REG(ATM90E32AS_REG_S_PHASE_TH, DT_INST_PROP(inst, s_phase_th)),                    \
		REG_ABC(inst, p_offset, ATM90E32AS_REG_P_OFFSET_A, ATM90E32AS_CAL_STRIDE),         \
		REG_ABC(inst, q_offset, ATM90E32AS_REG_Q_OFFSET_A, ATM90E32AS_CAL_STRIDE),         \
		REG_ABC(inst, pq_gain, ATM90E32AS_REG_PQ_GAIN_A, ATM90E32AS_CAL_STRIDE),           \
		REG_ABC(inst, phi, ATM90E32AS_REG_PHI_A, ATM90E32AS_CAL_STRIDE),                   \
		REG_ABC(inst, p_offset_fund, ATM90E32AS_REG_P_OFFSET_AF, 1),                       \
		REG_ABC(inst, p_gain_fund, ATM90E32AS_REG_P_GAIN_AF, 1),                           \
		REG_ABC(inst, u_gain, ATM90E32AS_REG_U_GAIN_A, ATM90E32AS_MEAS_CAL_STRIDE),        \
		REG_ABC(inst, i_gain, ATM90E32AS_REG_I_GAIN_A, ATM90E32AS_MEAS_CAL_STRIDE),        \
		REG_ABC(inst, u_offset, ATM90E32AS_REG_U_OFFSET_A, ATM90E32AS_MEAS_CAL_STRIDE),    \
		REG_ABC(inst, i_offset, ATM90E32AS_REG_I_OFFSET_A, ATM90E32AS_MEAS_CAL_STRIDE),    \
		REG(ATM90E32AS_REG_EMM_INT_EN0, DT_INST_PROP_BY_IDX(inst, emm_int_en, 0)),         \
		REG(ATM90E32AS_REG_EMM_INT_EN1, DT_INST_PROP_BY_IDX(inst, emm_int_en, 1))

#define ATM90E32AS_GPIO(inst, name) GPIO_DT_SPEC_INST_GET_OR(inst, name##_gpios, {0})

#define ATM90E32AS_EVENT_GPIOS(inst)                                                               \
	IF_ENABLED(CONFIG_ATM90E32AS_EVENTS,                                                       \
		   (.irq_gpios = {                                                                 \
			    [ATM90E32AS_IRQ0] = ATM90E32AS_GPIO(inst, irq0),                       \
			    [ATM90E32AS_IRQ1] = ATM90E32AS_GPIO(inst, irq1),                       \
			    [ATM90E32AS_WARN] = ATM90E32AS_GPIO(inst, wrn_out),                    \
			    [ATM90E32AS_CF1] = ATM90E32AS_GPIO(inst, cf1),                         \
			    [ATM90E32AS_CF2] = ATM90E32AS_GPIO(inst, cf2),                         \
			    [ATM90E32AS_CF3] = ATM90E32AS_GPIO(inst, cf3),                         \
			    [ATM90E32AS_CF4] = ATM90E32AS_GPIO(inst, cf4),                         \
		    },))

#define ATM90E32AS_DEFINE(inst)                                                                    \
	BUILD_ASSERT(DT_INST_PROP(inst, meter_constant) > 0, "meter-constant must not be 0");      \
	BUILD_ASSERT(IN_RANGE(DT_INST_PROP_BY_IDX(inst, current_scale, 0), 1, UINT16_MAX) &&       \
			     IN_RANGE(DT_INST_PROP_BY_IDX(inst, current_scale, 1), 1, UINT16_MAX), \
		     "current-scale values must be from 1 to 65535");                              \
                                                                                                   \
	static const struct atm90e32as_reg_val atm90e32as_init_regs_##inst[] = {                   \
		ATM90E32AS_INIT_REGS(inst)};                                                       \
                                                                                                   \
	static const struct atm90e32as_config atm90e32as_config_##inst = {                         \
		.bus = SPI_DT_SPEC_INST_GET(inst, ATM90E32AS_SPI_OPERATION),                       \
		.init_regs = atm90e32as_init_regs_##inst,                                          \
		.init_regs_len = ARRAY_SIZE(atm90e32as_init_regs_##inst),                          \
		.meter_constant = DT_INST_PROP(inst, meter_constant),                              \
		.current_scale = DT_INST_PROP(inst, current_scale),                                \
		.u_gain = DT_INST_PROP(inst, u_gain),                                              \
		.i_gain = DT_INST_PROP(inst, i_gain),                                              \
		.pm0 = ATM90E32AS_GPIO(inst, pm0),                                                 \
		.pm1 = ATM90E32AS_GPIO(inst, pm1),                                                 \
		ATM90E32AS_EVENT_GPIOS(inst)};                                                     \
                                                                                                   \
	static struct atm90e32as_data atm90e32as_data_##inst;                                      \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, atm90e32as_pm_action);                                      \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, atm90e32as_init, PM_DEVICE_DT_INST_GET(inst),                  \
			      &atm90e32as_data_##inst, &atm90e32as_config_##inst, POST_KERNEL,     \
			      CONFIG_ENERGY_METER_INIT_PRIORITY, &atm90e32as_api);

DT_INST_FOREACH_STATUS_OKAY(ATM90E32AS_DEFINE)

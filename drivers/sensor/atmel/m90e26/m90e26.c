/*
 * Copyright (c) 2025, Atmel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * More info, follow link:
 * https://www.microchip.com/en-us/product/atm90e26
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(m90e26, CONFIG_SENSOR_LOG_LEVEL);

#define DT_DRV_COMPAT atmel_m90e26

#include "m90e26.h"
#include "m90e26_regs.h"

#include <zephyr/drivers/sensor/m90e26.h>

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "M90E26 driver enabled without any devices"
#endif

#define M90E26_RETRY_COUNT 5

static inline int m90e26_bus_check(const struct device *dev)
{
	const struct m90e26_config *cfg = (const struct m90e26_config *)dev->config;

	return cfg->bus_io->bus_check(dev);
}

static inline int m90e26_read_register(const struct device *dev, const m90e26_register_t reg,
				       m90e26_data_value_t *value)
{
	const struct m90e26_config *cfg = (const struct m90e26_config *)dev->config;
	struct m90e26_data *data = (struct m90e26_data *)dev->data;
	int ret = 0;

	ret = k_mutex_lock(&data->bus_lock, K_FOREVER);
	if (ret < 0) {
		return ret;
	}

#if IS_ENABLED(CONFIG_M90EXX_ROBUSTNESS_WORKAROUND)
	uint8_t retry = 0;

retry_read:
#endif

	ret = cfg->bus_io->read(dev, reg, value);
	if (ret < 0) {
		goto end;
	}

#if IS_ENABLED(CONFIG_M90EXX_ROBUSTNESS_WORKAROUND)

	m90e26_data_value_t last_data;

	ret = cfg->bus_io->read(dev, LASTDATA, &last_data);

	if (ret == 0 && last_data.uint16 == value->uint16) {
		goto end;
	} else {
		if (retry < M90E26_RETRY_COUNT) {
			retry++;
			goto retry_read;
		} else {
			LOG_ERR("SPI read verification failed for Reg 0x%04X", reg);
			ret = -EIO;
			goto end;
		}
	}

#endif
end:
	k_mutex_unlock(&data->bus_lock);
	return ret;
}

static inline int m90e26_write_register(const struct device *dev, const m90e26_register_t addr,
					const m90e26_data_value_t *value)
{
	const struct m90e26_config *cfg = (const struct m90e26_config *)dev->config;
	struct m90e26_data *data = (struct m90e26_data *)dev->data;
	int ret = 0;

	if (addr == SYSSTATUS || addr == LASTDATA || addr >= APENERGY) {
		return -EINVAL;
	}

	ret = k_mutex_lock(&data->bus_lock, K_FOREVER);
	if (ret < 0) {
		return ret;
	}

#if IS_ENABLED(CONFIG_M90EXX_ROBUSTNESS_WORKAROUND)
	uint8_t retry = 0;

retry_write:
#endif

	ret = cfg->bus_io->write(dev, addr, value);
	if (ret < 0) {
		goto end;
	}

#if IS_ENABLED(CONFIG_M90EXX_ROBUSTNESS_WORKAROUND)
	if (addr == LASTDATA || addr == SOFTRESET) {
		goto end;
	}
	m90e26_data_value_t verify_data;

	ret = cfg->bus_io->read(dev, LASTDATA, &verify_data);

	if (ret == 0 && verify_data.uint16 == value->uint16) {
		goto end;
	} else {
		if (retry < M90E26_RETRY_COUNT) {
			retry++;
			goto retry_write;
		} else {
			LOG_ERR("SPI write verification failed for Reg 0x%04X", addr);
			ret = -EIO;
			goto end;
		}
	}
#endif
end:
	k_mutex_unlock(&data->bus_lock);
	return ret;
}

static int m90e26_checksum1(const struct device *dev)
{
	const struct m90e26_data *data = (const struct m90e26_data *)dev->data;
	const struct m90e26_config_registers *reg = &data->config_registers;
	uint8_t lsb = 0;
	uint8_t msb = 0;

	/* PLconstH */
	lsb += (reg->PLconstH.uint16 & 0xFF) + (reg->PLconstH.uint16 >> 8);
	msb ^= (reg->PLconstH.uint16 & 0xFF) ^ (reg->PLconstH.uint16 >> 8);
	/* PLconstL */
	lsb += (reg->PLconstL.uint16 & 0xFF) + (reg->PLconstL.uint16 >> 8);
	msb ^= (reg->PLconstL.uint16 & 0xFF) ^ (reg->PLconstL.uint16 >> 8);
	/* Lgain */
	lsb += (reg->Lgain.uint16 & 0xFF) + (reg->Lgain.uint16 >> 8);
	msb ^= (reg->Lgain.uint16 & 0xFF) ^ (reg->Lgain.uint16 >> 8);
	/* Lphi */
	lsb += (reg->Lphi.uint16 & 0xFF) + (reg->Lphi.uint16 >> 8);
	msb ^= (reg->Lphi.uint16 & 0xFF) ^ (reg->Lphi.uint16 >> 8);
	/* Ngain */
	lsb += (reg->Ngain.uint16 & 0xFF) + (reg->Ngain.uint16 >> 8);
	msb ^= (reg->Ngain.uint16 & 0xFF) ^ (reg->Ngain.uint16 >> 8);
	/* Nphi */
	lsb += (reg->Nphi.uint16 & 0xFF) + (reg->Nphi.uint16 >> 8);
	msb ^= (reg->Nphi.uint16 & 0xFF) ^ (reg->Nphi.uint16 >> 8);
	/* PStartTh */
	lsb += (reg->PStartTh.uint16 & 0xFF) + (reg->PStartTh.uint16 >> 8);
	msb ^= (reg->PStartTh.uint16 & 0xFF) ^ (reg->PStartTh.uint16 >> 8);
	/* PNolTh */
	lsb += (reg->PNolTh.uint16 & 0xFF) + (reg->PNolTh.uint16 >> 8);
	msb ^= (reg->PNolTh.uint16 & 0xFF) ^ (reg->PNolTh.uint16 >> 8);
	/* QStartTh */
	lsb += (reg->QStartTh.uint16 & 0xFF) + (reg->QStartTh.uint16 >> 8);
	msb ^= (reg->QStartTh.uint16 & 0xFF) ^ (reg->QStartTh.uint16 >> 8);
	/* QNolTh */
	lsb += (reg->QNolTh.uint16 & 0xFF) + (reg->QNolTh.uint16 >> 8);
	msb ^= (reg->QNolTh.uint16 & 0xFF) ^ (reg->QNolTh.uint16 >> 8);
	/* MMode */
	lsb += (reg->MMode.uint16 & 0xFF) + (reg->MMode.uint16 >> 8);
	msb ^= (reg->MMode.uint16 & 0xFF) ^ (reg->MMode.uint16 >> 8);

	m90e26_data_value_t checksum = {.uint16 = (uint16_t)((msb << 8) | lsb)};

	return m90e26_write_register(dev, CS1, &checksum);
}

static int m90e26_checksum2(const struct device *dev)
{
	const struct m90e26_data *data = (const struct m90e26_data *)dev->data;
	const struct m90e26_config_registers *reg = &data->config_registers;
	uint8_t lsb = 0;
	uint8_t msb = 0;

	/* Ugain */
	lsb += (reg->Ugain.uint16 & 0xFF) + (reg->Ugain.uint16 >> 8);
	msb ^= (reg->Ugain.uint16 & 0xFF) ^ (reg->Ugain.uint16 >> 8);
	/* IgainL */
	lsb += (reg->IgainL.uint16 & 0xFF) + (reg->IgainL.uint16 >> 8);
	msb ^= (reg->IgainL.uint16 & 0xFF) ^ (reg->IgainL.uint16 >> 8);
	/* IgainN */
	lsb += (reg->IgainN.uint16 & 0xFF) + (reg->IgainN.uint16 >> 8);
	msb ^= (reg->IgainN.uint16 & 0xFF) ^ (reg->IgainN.uint16 >> 8);
	/* Uoffset */
	lsb += (reg->Uoffset.uint16 & 0xFF) + (reg->Uoffset.uint16 >> 8);
	msb ^= (reg->Uoffset.uint16 & 0xFF) ^ (reg->Uoffset.uint16 >> 8);
	/* IoffsetL */
	lsb += (reg->IoffsetL.uint16 & 0xFF) + (reg->IoffsetL.uint16 >> 8);
	msb ^= (reg->IoffsetL.uint16 & 0xFF) ^ (reg->IoffsetL.uint16 >> 8);
	/* IoffsetN */
	lsb += (reg->IoffsetN.uint16 & 0xFF) + (reg->IoffsetN.uint16 >> 8);
	msb ^= (reg->IoffsetN.uint16 & 0xFF) ^ (reg->IoffsetN.uint16 >> 8);
	/* PoffsetL */
	lsb += (reg->PoffsetL.uint16 & 0xFF) + (reg->PoffsetL.uint16 >> 8);
	msb ^= (reg->PoffsetL.uint16 & 0xFF) ^ (reg->PoffsetL.uint16 >> 8);
	/* QoffsetL */
	lsb += (reg->QoffsetL.uint16 & 0xFF) + (reg->QoffsetL.uint16 >> 8);
	msb ^= (reg->QoffsetL.uint16 & 0xFF) ^ (reg->QoffsetL.uint16 >> 8);
	/* PoffsetN */
	lsb += (reg->PoffsetN.uint16 & 0xFF) + (reg->PoffsetN.uint16 >> 8);
	msb ^= (reg->PoffsetN.uint16 & 0xFF) ^ (reg->PoffsetN.uint16 >> 8);
	/* QoffsetN */
	lsb += (reg->QoffsetN.uint16 & 0xFF) + (reg->QoffsetN.uint16 >> 8);
	msb ^= (reg->QoffsetN.uint16 & 0xFF) ^ (reg->QoffsetN.uint16 >> 8);

	m90e26_data_value_t checksum = {.uint16 = (uint16_t)((msb << 8) | lsb)};

	return m90e26_write_register(dev, CS2, &checksum);
}

static inline int m90e26_metering_calibration_start(const struct device *dev)
{
	const m90e26_data_value_t cal_start_value = {.uint16 = 0x5678};

	return m90e26_write_register(dev, CALSTART, &cal_start_value);
}

static int m90e26_metering_calibration_finish(const struct device *dev)
{
	m90e26_data_value_t buffer = {.uint16 = 0x8765};

	m90e26_checksum1(dev);

	m90e26_write_register(dev, CALSTART, &buffer);
	m90e26_read_register(dev, SYSSTATUS, &buffer);
	if ((buffer.uint16 & M90E26_SYSSTATUS_CALERR_BIT_MASK) != 0) {
		LOG_ERR("Metering calibration error.");
		return -EIO;
	}

	return 0;
}

static inline int m90e26_measurement_calibration_start(const struct device *dev)
{
	const m90e26_data_value_t cal_start_value = {.uint16 = 0x5678};

	return m90e26_write_register(dev, ADJSTART, &cal_start_value);
}

static int m90e26_measurement_calibration_finish(const struct device *dev)
{
	m90e26_data_value_t buffer = {.uint16 = 0x8765};

	m90e26_checksum2(dev);

	m90e26_write_register(dev, ADJSTART, &buffer);
	m90e26_read_register(dev, SYSSTATUS, &buffer);
	if ((buffer.uint16 & M90E26_SYSSTATUS_ADJERR_BIT_MASK) != 0) {
		LOG_ERR("Measurement calibration error.");
		return -EIO;
	}

	return 0;
}

static int m90e26_reload_config(const struct device *dev)
{
	int ret = 0;
	struct m90e26_data *data = (struct m90e26_data *)dev->data;

	k_mutex_lock(&data->config_lock, K_FOREVER);

	m90e26_write_register(dev, FUNCEN, &data->config_registers.FuncEn);
	m90e26_write_register(dev, SAGTH, &data->config_registers.SagTh);
	m90e26_write_register(dev, SMALLPMOD, &data->config_registers.SmallPMod);

	ret = m90e26_metering_calibration_start(dev);
	if (ret < 0) {
		goto end;
	}

	m90e26_write_register(dev, PLCONSTH, &data->config_registers.PLconstH);
	m90e26_write_register(dev, PLCONSTL, &data->config_registers.PLconstL);
	m90e26_write_register(dev, LGAIN, &data->config_registers.Lgain);
	m90e26_write_register(dev, LPHI, &data->config_registers.Lphi);
	m90e26_write_register(dev, NGAIN, &data->config_registers.Ngain);
	m90e26_write_register(dev, NPHI, &data->config_registers.Nphi);
	m90e26_write_register(dev, PSTARTTH, &data->config_registers.PStartTh);
	m90e26_write_register(dev, PNOLTH, &data->config_registers.PNolTh);
	m90e26_write_register(dev, QSTARTTH, &data->config_registers.QStartTh);
	m90e26_write_register(dev, QNOLTH, &data->config_registers.QNolTh);
	m90e26_write_register(dev, MMODE, &data->config_registers.MMode);

	ret = m90e26_metering_calibration_finish(dev);
	if (ret < 0) {
		goto end;
	}

	ret = m90e26_measurement_calibration_start(dev);
	if (ret < 0) {
		goto end;
	}

	m90e26_write_register(dev, UGAIN, &data->config_registers.Ugain);
	m90e26_write_register(dev, IGAINL, &data->config_registers.IgainL);
	m90e26_write_register(dev, IGAINN, &data->config_registers.IgainN);
	m90e26_write_register(dev, UOFFSET, &data->config_registers.Uoffset);
	m90e26_write_register(dev, IOFFSETL, &data->config_registers.IoffsetL);
	m90e26_write_register(dev, IOFFSETN, &data->config_registers.IoffsetN);
	m90e26_write_register(dev, POFFSETL, &data->config_registers.PoffsetL);
	m90e26_write_register(dev, QOFFSETL, &data->config_registers.QoffsetL);
	m90e26_write_register(dev, POFFSETN, &data->config_registers.PoffsetN);
	m90e26_write_register(dev, QOFFSETN, &data->config_registers.QoffsetN);

	ret = m90e26_measurement_calibration_finish(dev);
	if (ret < 0) {
		goto end;
	}

	k_mutex_unlock(&data->config_lock);
end:
	return ret;
}

static int m90e26_reset(const struct device *dev)
{
	int ret = 0;
	const m90e26_data_value_t reset_value = {.uint16 = 0x789A};

	ret = m90e26_write_register(dev, SOFTRESET, &reset_value); /* Reset Software */
	if (ret < 0) {
		LOG_ERR("Could not write reset command to %s.", dev->name);
		return ret;
	}

	k_msleep(5); /* Wait for reset to complete (T1) */

	ret = m90e26_reload_config(dev);
	if (ret < 0) {
		LOG_ERR("Could not reload configuration for %s.", dev->name);
		return ret;
	}

	LOG_DBG("Reset done.");

	return ret;
}

static int m90e26_init(const struct device *dev)
{
	int ret;

	ret = m90e26_bus_check(dev);
	if (ret < 0) {
		LOG_ERR("Bus check failed for device %s.", dev->name);
		return ret;
	}

	k_msleep(5); /* Wait for device to power up */

	ret = m90e26_reset(dev);
	if (ret < 0) {
		LOG_ERR("Could not reset %s device.", dev->name);
		return ret;
	}

	return ret;
}

static int m90e26_sample_fetch(const struct device *dev, enum sensor_channel channel)
{
	int ret = 0;
	struct m90e26_data *data = (struct m90e26_data *)dev->data;

	switch ((uint16_t)channel) {
	case SENSOR_CHAN_ALL:
		LOG_WRN("Fetching for all channels is not available.");
		break;
	case M90E26_SENSOR_CHANNEL_ENERGY:
		m90e26_read_register(dev, APENERGY,
				     (m90e26_data_value_t *)&data->energy_values.APenergy);
		m90e26_read_register(dev, ANENERGY,
				     (m90e26_data_value_t *)&data->energy_values.ANenergy);
		m90e26_read_register(dev, ATENERGY,
				     (m90e26_data_value_t *)&data->energy_values.ATenergy);
		m90e26_read_register(dev, RPENERGY,
				     (m90e26_data_value_t *)&data->energy_values.RPenergy);
		m90e26_read_register(dev, RNENERGY,
				     (m90e26_data_value_t *)&data->energy_values.RNenergy);
		m90e26_read_register(dev, RTENERGY,
				     (m90e26_data_value_t *)&data->energy_values.RTenergy);
		break;
	case M90E26_SENSOR_CHANNEL_POWER:
		m90e26_read_register(dev, PMEAN, (m90e26_data_value_t *)&data->power_values.Pmean);
		m90e26_read_register(dev, PMEAN2,
				     (m90e26_data_value_t *)&data->power_values.Pmean2);
		m90e26_read_register(dev, QMEAN, (m90e26_data_value_t *)&data->power_values.Qmean);
		m90e26_read_register(dev, QMEAN2,
				     (m90e26_data_value_t *)&data->power_values.Qmean2);
		m90e26_read_register(dev, SMEAN, (m90e26_data_value_t *)&data->power_values.Smean);
		m90e26_read_register(dev, SMEAN2,
				     (m90e26_data_value_t *)&data->power_values.Smean2);
		break;
	case M90E26_SENSOR_CHANNEL_VOLTAGE:
		m90e26_read_register(dev, URMS, (m90e26_data_value_t *)&data->Urms);
		break;
	case M90E26_SENSOR_CHANNEL_CURRENT:
		m90e26_read_register(dev, IRMS, (m90e26_data_value_t *)&data->current_values.Irms);
		m90e26_read_register(dev, IRMS2,
				     (m90e26_data_value_t *)&data->current_values.Irms2);
		break;
	case M90E26_SENSOR_CHANNEL_FREQUENCY:
		m90e26_read_register(dev, FREQ, (m90e26_data_value_t *)&data->Freq);
		break;
	case M90E26_SENSOR_CHANNEL_PHASE_ANGLE:
		m90e26_read_register(dev, PANGLE,
				     (m90e26_data_value_t *)&data->pangle_values.Pangle);
		m90e26_read_register(dev, PANGLE2,
				     (m90e26_data_value_t *)&data->pangle_values.Pangle2);
		break;
	case M90E26_SENSOR_CHANNEL_POWER_FACTOR:
		m90e26_read_register(dev, POWERF,
				     (m90e26_data_value_t *)&data->pfactor_values.PowerF);
		m90e26_read_register(dev, POWERF2,
				     (m90e26_data_value_t *)&data->pfactor_values.PowerF2);
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static int m90e26_channel_get(const struct device *dev, enum sensor_channel channel,
			      struct sensor_value *value)
{
	struct m90e26_data *data = (struct m90e26_data *)dev->data;
	int ret = 0;

	switch ((uint16_t)channel) {
	case SENSOR_CHAN_ALL:
		LOG_WRN("Getting all channels not available.");
		break;
	case M90E26_SENSOR_CHANNEL_ENERGY: {
		struct m90e26_energy_sensor_data *energy_value =
			(struct m90e26_energy_sensor_data *)value;
		ret |= m90e26_convert_energy((m90e26_data_value_t *)&data->energy_values.APenergy,
					     &energy_value->APenergy);
		ret |= m90e26_convert_energy((m90e26_data_value_t *)&data->energy_values.ANenergy,
					     &energy_value->ANenergy);
		ret |= m90e26_convert_energy((m90e26_data_value_t *)&data->energy_values.ATenergy,
					     &energy_value->ATenergy);
		ret |= m90e26_convert_energy((m90e26_data_value_t *)&data->energy_values.RPenergy,
					     &energy_value->RPenergy);
		ret |= m90e26_convert_energy((m90e26_data_value_t *)&data->energy_values.RNenergy,
					     &energy_value->RNenergy);
		ret |= m90e26_convert_energy((m90e26_data_value_t *)&data->energy_values.RTenergy,
					     &energy_value->RTenergy);
		break;
	}
	case M90E26_SENSOR_CHANNEL_POWER: {
		struct m90e26_power_sensor_data *power_value =
			(struct m90e26_power_sensor_data *)value;
		ret |= m90e26_convert_power((m90e26_data_value_t *)&data->power_values.Pmean,
					    &power_value->Pmean);
		ret |= m90e26_convert_power((m90e26_data_value_t *)&data->power_values.Pmean2,
					    &power_value->Pmean2);
		ret |= m90e26_convert_power((m90e26_data_value_t *)&data->power_values.Qmean,
					    &power_value->Qmean);
		ret |= m90e26_convert_power((m90e26_data_value_t *)&data->power_values.Qmean2,
					    &power_value->Qmean2);
		ret |= m90e26_convert_power((m90e26_data_value_t *)&data->power_values.Smean,
					    &power_value->Smean);
		ret |= m90e26_convert_power((m90e26_data_value_t *)&data->power_values.Smean2,
					    &power_value->Smean2);
		break;
	}
	case M90E26_SENSOR_CHANNEL_VOLTAGE:
		ret = sensor_value_from_float(value, data->Urms * 0.01f);
		break;
	case M90E26_SENSOR_CHANNEL_CURRENT: {
		struct m90e26_current_sensor_data *current_value =
			(struct m90e26_current_sensor_data *)value;
		ret |= m90e26_convert_current((m90e26_data_value_t *)&data->current_values.Irms,
					      &current_value->Irms);
		ret |= m90e26_convert_current((m90e26_data_value_t *)&data->current_values.Irms2,
					      &current_value->Irms2);
		break;
	}
	case M90E26_SENSOR_CHANNEL_FREQUENCY:
		ret = sensor_value_from_float(value, data->Freq * 0.01f);
		break;
	case M90E26_SENSOR_CHANNEL_PHASE_ANGLE: {
		struct m90e26_phase_angle_sensor_data *phase_angle_value =
			(struct m90e26_phase_angle_sensor_data *)value;
		ret |= m90e26_convert_pangle((m90e26_data_value_t *)&data->pangle_values.Pangle,
					     &phase_angle_value->Pangle);
		ret |= m90e26_convert_pangle((m90e26_data_value_t *)&data->pangle_values.Pangle2,
					     &phase_angle_value->Pangle2);
		break;
	}
	case M90E26_SENSOR_CHANNEL_POWER_FACTOR: {
		struct m90e26_power_factor_sensor_data *power_factor_value =
			(struct m90e26_power_factor_sensor_data *)value;
		ret |= m90e26_convert_pfactor((m90e26_data_value_t *)&data->pfactor_values.PowerF,
					      &power_factor_value->PowerF);
		ret |= m90e26_convert_pfactor((m90e26_data_value_t *)&data->pfactor_values.PowerF2,
					      &power_factor_value->PowerF2);
		break;
	}
	default: {
		LOG_ERR("Channel type not supported.");
		ret = -EINVAL;
		break;
	}
	}

	return ret;
}

static void m90e26_gpio_callback_irq(const struct device *port, struct gpio_callback *cb,
				     uint32_t pins)
{
	ARG_UNUSED(pins);
	const struct m90e26_data *data = CONTAINER_OF(cb, struct m90e26_data, irq_ctx.gpio_cb);

	if (data->irq_ctx.handler) {
		data->irq_ctx.handler(port, &data->irq_ctx.trigger);
	}
}

static void m90e26_gpio_callback_wrn_out(const struct device *port, struct gpio_callback *cb,
					 uint32_t pins)
{
	ARG_UNUSED(pins);
	const struct m90e26_data *data = CONTAINER_OF(cb, struct m90e26_data, wrn_out_ctx.gpio_cb);

	if (data->wrn_out_ctx.handler) {
		data->wrn_out_ctx.handler(port, &data->wrn_out_ctx.trigger);
	}
}

static void m90e26_gpio_callback_cf1(const struct device *port, struct gpio_callback *cb,
				     uint32_t pins)
{
	ARG_UNUSED(pins);
	const struct m90e26_data *data = CONTAINER_OF(cb, struct m90e26_data, cf1.gpio_cb);

	if (data->cf1.handler) {
		data->cf1.handler(port, &data->cf1.trigger);
	}
}

static void m90e26_gpio_callback_cf2(const struct device *port, struct gpio_callback *cb,
				     uint32_t pins)
{
	ARG_UNUSED(pins);
	const struct m90e26_data *data = CONTAINER_OF(cb, struct m90e26_data, cf2.gpio_cb);

	if (data->cf2.handler) {
		data->cf2.handler(port, &data->cf2.trigger);
	}
}

static int m90e26_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			      sensor_trigger_handler_t handler)
{
	struct m90e26_data *data = (struct m90e26_data *)dev->data;
	const struct m90e26_config *cfg = (const struct m90e26_config *)dev->config;
	int ret = -ENOTSUP;

#if CONFIG_PM_DEVICE
	pm_device_busy_set(dev);
#endif /* CONFIG_PM_DEVICE */

	if (trig->type == M90E26_SENSOR_TRIG_TYPE_IRQ) {
		if (gpio_is_ready_dt(&cfg->irq)) {
			data->irq_ctx.trigger = *trig;
			data->irq_ctx.handler = handler;
			gpio_init_callback(&data->irq_ctx.gpio_cb, m90e26_gpio_callback_irq,
					   BIT(cfg->irq.pin));
			ret = gpio_add_callback(cfg->irq.port, &data->irq_ctx.gpio_cb);
			if (ret == 0) {
				ret = gpio_pin_interrupt_configure_dt(&cfg->irq,
								      GPIO_INT_EDGE_TO_ACTIVE);
			}
		}
	} else if (trig->type == M90E26_SENSOR_TRIG_TYPE_WRN_OUT) {
		if (gpio_is_ready_dt(&cfg->wrn_out)) {
			data->wrn_out_ctx.trigger = *trig;
			data->wrn_out_ctx.handler = handler;
			gpio_init_callback(&data->wrn_out_ctx.gpio_cb, m90e26_gpio_callback_wrn_out,
					   BIT(cfg->wrn_out.pin));
			ret = gpio_add_callback(cfg->wrn_out.port, &data->wrn_out_ctx.gpio_cb);
			if (ret == 0) {
				ret = gpio_pin_interrupt_configure_dt(&cfg->wrn_out,
								      GPIO_INT_EDGE_TO_ACTIVE);
			}
		}
	} else if (trig->type == M90E26_SENSOR_TRIG_TYPE_CF1) {
		if (gpio_is_ready_dt(&cfg->cf1)) {
			data->cf1.trigger = *trig;
			data->cf1.handler = handler;
			gpio_init_callback(&data->cf1.gpio_cb, m90e26_gpio_callback_cf1,
					   BIT(cfg->cf1.pin));
			ret = gpio_add_callback(cfg->cf1.port, &data->cf1.gpio_cb);
			if (ret == 0) {
				ret = gpio_pin_interrupt_configure_dt(&cfg->cf1,
								      GPIO_INT_EDGE_TO_ACTIVE);
			}
		}
	} else if (trig->type == M90E26_SENSOR_TRIG_TYPE_CF2) {
		if (gpio_is_ready_dt(&cfg->cf2)) {
			data->cf2.trigger = *trig;
			data->cf2.handler = handler;
			gpio_init_callback(&data->cf2.gpio_cb, m90e26_gpio_callback_cf2,
					   BIT(cfg->cf2.pin));
			ret = gpio_add_callback(cfg->cf2.port, &data->cf2.gpio_cb);
			if (ret == 0) {
				ret = gpio_pin_interrupt_configure_dt(&cfg->cf2,
								      GPIO_INT_EDGE_TO_ACTIVE);
			}
		}
	} else {
		return -ENOTSUP;
	}

#if CONFIG_PM_DEVICE
	pm_device_busy_clear(dev);
#endif /* CONFIG_PM_DEVICE */

	return ret;
}

static DEVICE_API(sensor, m90e26_api) = {
	.sample_fetch = &m90e26_sample_fetch,
	.channel_get = &m90e26_channel_get,
	.trigger_set = &m90e26_trigger_set,
};

#define M90E26_DEFAULT_CONFIG_REGISTER_VALUES                                                      \
	{                                                                                          \
		.FuncEn = {CONFIG_M90E26_FUNCEN},                                                  \
		.SagTh = {CONFIG_M90E26_SAGTH},                                                    \
		.SmallPMod = {CONFIG_M90E26_SMALLPMOD},                                            \
		.PLconstH = {CONFIG_M90E26_PLCONSTH},                                              \
		.PLconstL = {CONFIG_M90E26_PLCONSTL},                                              \
		.Lgain = {CONFIG_M90E26_LGAIN},                                                    \
		.Lphi = {CONFIG_M90E26_LPHI},                                                      \
		.Ngain = {CONFIG_M90E26_NGAIN},                                                    \
		.Nphi = {CONFIG_M90E26_NPHI},                                                      \
		.PStartTh = {CONFIG_M90E26_PSTARTTH},                                              \
		.PNolTh = {CONFIG_M90E26_PNOLTH},                                                  \
		.QStartTh = {CONFIG_M90E26_QSTARTTH},                                              \
		.QNolTh = {CONFIG_M90E26_QNOLTH},                                                  \
		.MMode = {CONFIG_M90E26_MMODE},                                                    \
		.Ugain = {CONFIG_M90E26_UGAIN},                                                    \
		.IgainL = {CONFIG_M90E26_IGAINL},                                                  \
		.IgainN = {CONFIG_M90E26_IGAINN},                                                  \
		.Uoffset = {CONFIG_M90E26_UOFFSET},                                                \
		.IoffsetL = {CONFIG_M90E26_IOFFSETL},                                              \
		.IoffsetN = {CONFIG_M90E26_IOFFSETN},                                              \
		.PoffsetL = {CONFIG_M90E26_POFFSETL},                                              \
		.QoffsetL = {CONFIG_M90E26_QOFFSETL},                                              \
		.PoffsetN = {CONFIG_M90E26_POFFSETN},                                              \
		.QoffsetN = {CONFIG_M90E26_QOFFSETN},                                              \
	}

/* Initializes a struct m90e26_config for an instance on a SPI bus. */
#define M90E26_CONFIG_SPI                                                                          \
	.bus.spi = SPI_DT_SPEC_INST_GET(inst, M90E26_SPI_OPERATION), .bus_io = &m90e26_bus_io_spi,

/* Initializes a struct m90e26_config for an instance on a UART bus. */
#define M90E26_CONFIG_UART                                                                         \
	.bus.uart.bus = DEVICE_DT_GET(DT_INST_BUS(inst)),                                          \
	.bus.uart.config = {.baudrate = DT_PROP(DT_INST_PARENT(inst), current_speed),              \
			    .parity = UART_CFG_PARITY_NONE,                                        \
			    .stop_bits = UART_CFG_STOP_BITS_1,                                     \
			    .data_bits = UART_CFG_DATA_BITS_8,                                     \
			    .flow_ctrl = UART_CFG_FLOW_CTRL_NONE},                                 \
	.bus_io = &m90e26_bus_io_uart,

#define M90E26_ASSIGN_PIN(pin_name)                                                                \
	IF_ENABLED(DT_INST_NODE_HAS_PROP( \
		inst, pin_name##_gpios), \
		(.pin_name = GPIO_DT_SPEC_INST_GET(inst, pin_name##_gpios),))

#define M90E26_DT_BUS COND_CODE_1(DT_INST_ON_BUS(inst, spi), \
			(M90E26_CONFIG_SPI), \
			(M90E26_CONFIG_UART) \
	)

#define M90E26_DEVICE                                                                              \
	static struct m90e26_data m90e26_data_##inst = {                                           \
		.bus_lock = Z_MUTEX_INITIALIZER(m90e26_data_##inst.bus_lock),                      \
		.config_lock = Z_MUTEX_INITIALIZER(m90e26_data_##inst.config_lock),                \
		.config_registers = M90E26_DEFAULT_CONFIG_REGISTER_VALUES,                         \
	};                                                                                         \
                                                                                                   \
	static const struct m90e26_config m90e26_config_##inst = {                                 \
		M90E26_DT_BUS, IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, irq0_gpios), ( \
	.irq = GPIO_DT_SPEC_INST_GET(inst, irq0_gpios),))             \
						M90E26_ASSIGN_PIN(wrn_out) M90E26_ASSIGN_PIN(cf1)  \
							M90E26_ASSIGN_PIN(cf2)};                   \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, m90e26_init, NULL, &m90e26_data_##inst,                 \
				     &m90e26_config_##inst, POST_KERNEL,                           \
				     CONFIG_SENSOR_INIT_PRIORITY, &m90e26_api)

/* Create the struct device for every status "okay" node in the devicetree. */
DT_INST_FOREACH_STATUS_OKAY(M90E26_DEVICE)

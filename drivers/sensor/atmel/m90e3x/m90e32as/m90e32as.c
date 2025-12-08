/*
 * Copyright (c) 2025, Atmel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * More info, follow link:
 * https://www.microchip.com/en-us/product/atm90e32as
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
LOG_MODULE_REGISTER(m90e32as, CONFIG_SENSOR_LOG_LEVEL);

#define DT_DRV_COMPAT atmel_m90e32as

#include "../m90e3x.h"
#include "../m90e3x_regs.h"
#include "m90e32as_regs.h"

#include <zephyr/drivers/sensor/m90e3x.h>

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "M90E32AS driver enabled without any devices"
#endif

#define M90E32AS_RETRY_COUNT 5

static inline int m90e32as_bus_check(const struct device *dev)
{
	const struct m90e3x_config *cfg = (struct m90e3x_config *)dev->config;

	return cfg->bus_io->bus_check(dev);
}

static inline int m90e32as_read_register(const struct device *dev, const m90e3x_register_t reg,
					 m90e3x_data_value_t *value)
{
	const struct m90e3x_config *cfg = (struct m90e3x_config *)dev->config;
	struct m90e3x_data *data = (struct m90e3x_data *)dev->data;
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

	m90e3x_data_value_t last_data;

	ret = cfg->bus_io->read(dev, LASTSPIDATA, &last_data);
	if (ret == 0 && last_data.uint16 == value->uint16) {
		goto end;
	} else {
		if (retry < M90E32AS_RETRY_COUNT) {
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

static inline int m90e32as_write_register(const struct device *dev, const m90e3x_register_t addr,
					  const m90e3x_data_value_t *value)
{
	const struct m90e3x_config *cfg = (struct m90e3x_config *)dev->config;
	struct m90e3x_data *data = (struct m90e3x_data *)dev->data;
	int ret = 0;

	if (addr == EMMSTATE0 || addr == EMMSTATE1 || addr == LASTSPIDATA || addr == CRCERRSTATUS ||
	    addr >= APENERGYT) {
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
	if (addr == LASTSPIDATA || addr == SOFTRESET) {
		goto end;
	}
	m90e3x_data_value_t verify_data;

	ret = cfg->bus_io->read(dev, LASTSPIDATA, &verify_data);
	if (ret == 0 && verify_data.uint16 == value->uint16) {
		goto end;
	} else {
		if (retry < M90E32AS_RETRY_COUNT) {
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

static int m90e32as_config_access_enable(const struct device *dev)
{
	m90e3x_data_value_t value;

	value.uint16 = 0x55AA;

	return m90e32as_write_register(dev, CFGREGACCEN, &value);
}

static int m90e32as_config_access_disable(const struct device *dev)
{
	m90e3x_data_value_t value;

	value.uint16 = 0xAA55;

	return m90e32as_write_register(dev, CFGREGACCEN, &value);
}

static void m90e32as_reload_config(const struct device *dev)
{
	struct m90e3x_data *data = (struct m90e3x_data *)dev->data;

	m90e32as_config_access_enable(dev);

	/* Status and Special Registers */

	m90e32as_write_register(dev, METEREN, &data->m90e32as_config_registers.MeterEn);
	m90e32as_write_register(dev, CHANNELMAPI, &data->m90e32as_config_registers.ChannelMapI);
	m90e32as_write_register(dev, CHANNELMAPU, &data->m90e32as_config_registers.ChannelMapU);
	m90e32as_write_register(dev, SAGPEAKDETCFG, &data->m90e32as_config_registers.SagPeakDetCfg);
	m90e32as_write_register(dev, OVTHCFG, &data->m90e32as_config_registers.OVthCfg);
	m90e32as_write_register(dev, ZXCONFIG, &data->m90e32as_config_registers.ZXConfig);
	m90e32as_write_register(dev, SAGTH, &data->m90e32as_config_registers.SagTh);
	m90e32as_write_register(dev, PHASELOSSTH, &data->m90e32as_config_registers.PhaseLossTh);
	m90e32as_write_register(dev, INWARNTH, &data->m90e32as_config_registers.InWarnTh);
	m90e32as_write_register(dev, OITH, &data->m90e32as_config_registers.OIth);
	m90e32as_write_register(dev, FREQLOTH, &data->m90e32as_config_registers.FreqLoTh);
	m90e32as_write_register(dev, FREQHITH, &data->m90e32as_config_registers.FreqHiTh);
	m90e32as_write_register(dev, PMPWRCTRL, &data->m90e32as_config_registers.PMPwrCtrl);
	m90e32as_write_register(dev, IRQ0MERGECFG, &data->m90e32as_config_registers.IRQ0MergeCfg);

	/* Low Power Mode Registers */

	m90e32as_write_register(dev, DETECTCTRL, &data->m90e32as_config_registers.DetectCtrl);
	m90e32as_write_register(dev, DETECTTH1, &data->m90e32as_config_registers.DetectTh1);
	m90e32as_write_register(dev, DETECTTH2, &data->m90e32as_config_registers.DetectTh2);
	m90e32as_write_register(dev, DETECTTH3, &data->m90e32as_config_registers.DetectTh3);
	m90e32as_write_register(dev, IDCOFFSETA, &data->m90e32as_config_registers.IDCoffsetA);
	m90e32as_write_register(dev, IDCOFFSETB, &data->m90e32as_config_registers.IDCoffsetB);
	m90e32as_write_register(dev, IDCOFFSETC, &data->m90e32as_config_registers.IDCoffsetC);
	m90e32as_write_register(dev, UDCOFFSETA, &data->m90e32as_config_registers.UDCoffsetA);
	m90e32as_write_register(dev, UDCOFFSETB, &data->m90e32as_config_registers.UDCoffsetB);
	m90e32as_write_register(dev, UDCOFFSETC, &data->m90e32as_config_registers.UDCoffsetC);
	m90e32as_write_register(dev, UGAINTAB, &data->m90e32as_config_registers.UGainTAB);
	m90e32as_write_register(dev, UGAINTC, &data->m90e32as_config_registers.UGainTC);
	m90e32as_write_register(dev, PHIFREQCOMP, &data->m90e32as_config_registers.PhiFreqComp);
	m90e32as_write_register(dev, LOGIRMS0, &data->m90e32as_config_registers.LOGIrms0);
	m90e32as_write_register(dev, LOGIRMS1, &data->m90e32as_config_registers.LOGIrms1);
	m90e32as_write_register(dev, F0, &data->m90e32as_config_registers.F0);
	m90e32as_write_register(dev, T0, &data->m90e32as_config_registers.T0);
	m90e32as_write_register(dev, PHIAIRMS01, &data->m90e32as_config_registers.PhiAIrms01);
	m90e32as_write_register(dev, PHIAIRMS2, &data->m90e32as_config_registers.PhiAIrms2);
	m90e32as_write_register(dev, GAINAIRMS01, &data->m90e32as_config_registers.GainAIrms01);
	m90e32as_write_register(dev, GAINAIRMS2, &data->m90e32as_config_registers.GainAIrms2);
	m90e32as_write_register(dev, PHIBIRMS01, &data->m90e32as_config_registers.PhiBIrms01);
	m90e32as_write_register(dev, PHIBIRMS2, &data->m90e32as_config_registers.PhiBIrms2);
	m90e32as_write_register(dev, GAINBIRMS01, &data->m90e32as_config_registers.GainBIrms01);
	m90e32as_write_register(dev, GAINBIRMS2, &data->m90e32as_config_registers.GainBIrms2);
	m90e32as_write_register(dev, PHICIRMS01, &data->m90e32as_config_registers.PhiCIrms01);
	m90e32as_write_register(dev, PHICIRMS2, &data->m90e32as_config_registers.PhiCIrms2);
	m90e32as_write_register(dev, GAINCIRMS01, &data->m90e32as_config_registers.GainCIrms01);
	m90e32as_write_register(dev, GAINCIRMS2, &data->m90e32as_config_registers.GainCIrms2);

	/* Configuration Registers */

	m90e32as_write_register(dev, PLCONSTH, &data->m90e32as_config_registers.PLconstH);
	m90e32as_write_register(dev, PLCONSTL, &data->m90e32as_config_registers.PLconstL);
	m90e32as_write_register(dev, MMODE0, &data->m90e32as_config_registers.MMode0);
	m90e32as_write_register(dev, MMODE1, &data->m90e32as_config_registers.MMode1);
	m90e32as_write_register(dev, PSTARTTH, &data->m90e32as_config_registers.PStartTh);
	m90e32as_write_register(dev, QSTARTTH, &data->m90e32as_config_registers.QStartTh);
	m90e32as_write_register(dev, SSTARTTH, &data->m90e32as_config_registers.SStartTh);
	m90e32as_write_register(dev, PPHASETH, &data->m90e32as_config_registers.PPhaseTh);
	m90e32as_write_register(dev, QPHASETH, &data->m90e32as_config_registers.QPhaseTh);
	m90e32as_write_register(dev, SPHASETH, &data->m90e32as_config_registers.SPhaseTh);

	/* Calibration Registers */

	m90e32as_write_register(dev, POFFSETA, &data->m90e32as_config_registers.PoffsetA);
	m90e32as_write_register(dev, QOFFSETA, &data->m90e32as_config_registers.QoffsetA);
	m90e32as_write_register(dev, POFFSETB, &data->m90e32as_config_registers.PoffsetB);
	m90e32as_write_register(dev, QOFFSETB, &data->m90e32as_config_registers.QoffsetB);
	m90e32as_write_register(dev, POFFSETC, &data->m90e32as_config_registers.PoffsetC);
	m90e32as_write_register(dev, QOFFSETC, &data->m90e32as_config_registers.QoffsetC);
	m90e32as_write_register(dev, PQGAINA, &data->m90e32as_config_registers.PQGainA);
	m90e32as_write_register(dev, PHIA, &data->m90e32as_config_registers.PhiA);
	m90e32as_write_register(dev, PQGAINB, &data->m90e32as_config_registers.PQGainB);
	m90e32as_write_register(dev, PHIB, &data->m90e32as_config_registers.PhiB);
	m90e32as_write_register(dev, PQGAINC, &data->m90e32as_config_registers.PQGainC);
	m90e32as_write_register(dev, PHIC, &data->m90e32as_config_registers.PhiC);

	/* Fundamental/Harmonic Calibration Registers */

	m90e32as_write_register(dev, POFFSETAF, &data->m90e32as_config_registers.PoffsetAF);
	m90e32as_write_register(dev, POFFSETBF, &data->m90e32as_config_registers.PoffsetBF);
	m90e32as_write_register(dev, POFFSETCF, &data->m90e32as_config_registers.PoffsetCF);
	m90e32as_write_register(dev, PGAINAF, &data->m90e32as_config_registers.PGainAF);
	m90e32as_write_register(dev, PGAINBF, &data->m90e32as_config_registers.PGainBF);
	m90e32as_write_register(dev, PGAINCF, &data->m90e32as_config_registers.PGainCF);

	/* Measurement Calibration Registers */

	m90e32as_write_register(dev, UGAINA, &data->m90e32as_config_registers.UgainA);
	m90e32as_write_register(dev, IGAINA, &data->m90e32as_config_registers.IgainA);
	m90e32as_write_register(dev, UOFFSETA, &data->m90e32as_config_registers.UoffsetA);
	m90e32as_write_register(dev, UGAINB, &data->m90e32as_config_registers.UgainB);
	m90e32as_write_register(dev, IGAINB, &data->m90e32as_config_registers.IgainB);
	m90e32as_write_register(dev, UOFFSETB, &data->m90e32as_config_registers.UoffsetB);
	m90e32as_write_register(dev, UGAINC, &data->m90e32as_config_registers.UgainC);
	m90e32as_write_register(dev, IGAINC, &data->m90e32as_config_registers.IgainC);
	m90e32as_write_register(dev, UOFFSETC, &data->m90e32as_config_registers.UoffsetC);

	m90e32as_config_access_disable(dev);
}

static int m90e32as_reset(const struct device *dev)
{
	const m90e3x_data_value_t reset_value = {.uint16 = 0x789A};
	int ret = m90e32as_write_register(dev, SOFTRESET, &reset_value); /* Reset Software */

	if (ret < 0) {
		LOG_ERR("Could not write reset command to %s.", dev->name);
		return ret;
	}

	k_sleep(K_MSEC(40)); /* Wait for reset to complete (T1) */

	m90e32as_reload_config(dev);

	LOG_DBG("Reset done.");

	return ret;
}

#if CONFIG_PM_DEVICE

static int m90e32as_pm_resume(const struct device *dev)
{
	LOG_DBG("Resuming device %s.", dev->name);

	const struct m90e3x_config *cfg = (struct m90e3x_config *)dev->config;
	struct m90e3x_data *data = (struct m90e3x_data *)dev->data;
	int ret = 0;

	/* Request device pins needed */
	ret = pm_device_runtime_get(cfg->pm0.port);
	if (ret < 0) {
		return ret;
	}
	ret = pm_device_runtime_get(cfg->pm1.port);
	if (ret < 0) {
		return ret;
	}
	ret = pm_device_runtime_get(cfg->bus.bus);
	if (ret < 0) {
		return ret;
	}

	/* Disable and remove interrupt */
	if (data->cf1.handler) {
		gpio_pin_interrupt_configure_dt(&cfg->cf1, GPIO_INT_EDGE_TO_ACTIVE);
	}
	if (data->cf2.handler) {
		gpio_pin_interrupt_configure_dt(&cfg->cf2, GPIO_INT_EDGE_TO_ACTIVE);
	}
	if (data->cf3.handler) {
		gpio_pin_interrupt_configure_dt(&cfg->cf3, GPIO_INT_EDGE_TO_ACTIVE);
	}
	if (data->cf4.handler) {
		gpio_pin_interrupt_configure_dt(&cfg->cf4, GPIO_INT_EDGE_TO_ACTIVE);
	}
	if (data->wrn_out_ctx.handler) {
		gpio_pin_interrupt_configure_dt(&cfg->wrn_out, GPIO_INT_EDGE_TO_ACTIVE);
	}
	if (data->irq0_ctx.handler) {
		gpio_pin_interrupt_configure_dt(&cfg->irq0, GPIO_INT_EDGE_TO_ACTIVE);
	}
	if (data->irq1_ctx.handler) {
		gpio_pin_interrupt_configure_dt(&cfg->irq1, GPIO_INT_EDGE_TO_ACTIVE);
	}

	ret = cfg->pm_mode_ops->enter_normal_mode(dev);
	if (ret < 0) {
		return ret;
	}

	/* Release device pins */
	ret = pm_device_runtime_put(cfg->pm0.port);
	if (ret < 0) {
		return ret;
	}
	ret = pm_device_runtime_put(cfg->pm1.port);
	if (ret < 0) {
		return ret;
	}
	ret = pm_device_runtime_put(cfg->bus.bus);
	if (ret < 0) {
		return ret;
	}

	data->current_power_mode = M90E3X_NORMAL;

	return ret;
}

static int m90e32as_pm_suspend(const struct device *dev)
{
	LOG_DBG("Suspending device %s.", dev->name);

	const struct m90e3x_config *cfg = (struct m90e3x_config *)dev->config;
	struct m90e3x_data *data = (struct m90e3x_data *)dev->data;
	int ret = 0;

	/* Request device pins needed */
	ret = pm_device_runtime_get(cfg->pm0.port);
	if (ret < 0) {
		return ret;
	}
	ret = pm_device_runtime_get(cfg->pm1.port);
	if (ret < 0) {
		return ret;
	}
	ret = pm_device_runtime_get(cfg->bus.bus);
	if (ret < 0) {
		return ret;
	}

	/* Disable and remove interrupt */
	if (data->cf1.handler) {
		gpio_pin_interrupt_configure_dt(&cfg->cf1, GPIO_INT_DISABLE);
	}
	if (data->cf2.handler) {
		gpio_pin_interrupt_configure_dt(&cfg->cf2, GPIO_INT_DISABLE);
	}
	if (data->cf3.handler) {
		gpio_pin_interrupt_configure_dt(&cfg->cf3, GPIO_INT_DISABLE);
	}
	if (data->cf4.handler) {
		gpio_pin_interrupt_configure_dt(&cfg->cf4, GPIO_INT_DISABLE);
	}
	if (data->wrn_out_ctx.handler) {
		gpio_pin_interrupt_configure_dt(&cfg->wrn_out, GPIO_INT_DISABLE);
	}
	if (data->irq0_ctx.handler) {
		gpio_pin_interrupt_configure_dt(&cfg->irq0, GPIO_INT_DISABLE);
	}
	if (data->irq1_ctx.handler) {
		gpio_pin_interrupt_configure_dt(&cfg->irq1, GPIO_INT_DISABLE);
	}

	ret = cfg->pm_mode_ops->enter_idle_mode(dev);
	if (ret < 0) {
		return ret;
	}

	/* Release device pins */
	ret = pm_device_runtime_put(cfg->pm0.port);
	if (ret < 0) {
		return ret;
	}
	ret = pm_device_runtime_put(cfg->pm1.port);
	if (ret < 0) {
		return ret;
	}
	ret = pm_device_runtime_put(cfg->bus.bus);
	if (ret < 0) {
		return ret;
	}

	data->current_power_mode = M90E3X_IDLE;

	return ret;
}

static int m90e32as_pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = m90e32as_pm_resume(dev);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		ret = m90e32as_pm_suspend(dev);
		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

static int m90e32as_init(const struct device *dev)
{
	int ret;

	ret = m90e32as_bus_check(dev);
	if (ret < 0) {
		LOG_ERR("Bus check failed for device %s.", dev->name);
		return ret;
	}

#if CONFIG_PM_DEVICE
	const struct m90e3x_config *cfg = (struct m90e3x_config *)dev->config;

	if (!gpio_is_ready_dt(&cfg->pm0)) {
		LOG_WRN("PM0 pin is not assigned from dev %s.", dev->name);
	} else {
		gpio_pin_configure_dt(&cfg->pm0, GPIO_OUTPUT_ACTIVE);
	}
	if (!gpio_is_ready_dt(&cfg->pm1)) {
		LOG_WRN("PM1 pin is not assigned from dev %s.", dev->name);
	} else {
		gpio_pin_configure_dt(&cfg->pm1, GPIO_OUTPUT_ACTIVE);
	}
#endif /* CONFIG_PM_DEVICE */

	k_msleep(100); /* Wait for device to power up */

	ret = m90e32as_reset(dev);
	if (ret < 0) {
		LOG_ERR("Could not reset %s device.", dev->name);
		return ret;
	}

#if CONFIG_PM_DEVICE

	ret = pm_device_runtime_enable(dev);

	if (ret < 0) {
		LOG_ERR("Failed to enabled runtime power management for device %s.", dev->name);
		return -EIO;
	}
#endif /* CONFIG_PM_DEVICE */

	return ret;
}

static int m90e32as_energy_values_to_sensor(const struct device *dev,
					    const struct m90e3x_energy_data *energy_values,
					    struct sensor_value *sensor_values)
{
	/*
	 * Conversion formulas from datasheet:
	 * Energy Registers: 0.01 kWh per CF pulse
	 */

	ARG_UNUSED(dev);

	if (!energy_values || !sensor_values) {
		return -EINVAL;
	}

	int ret = 0;

	struct m90e3x_energy_sensor_data *energy_sensor_values =
		(struct m90e3x_energy_sensor_data *)sensor_values;

	ret |= sensor_value_from_float(
		&energy_sensor_values->APenergyT,
		m90e3x_convert_energy_reg((const m90e3x_data_value_t *)&energy_values->APenergyT));
	ret |= sensor_value_from_float(
		&energy_sensor_values->RPenergyT,
		m90e3x_convert_energy_reg((const m90e3x_data_value_t *)&energy_values->RPenergyT));
	ret |= sensor_value_from_float(
		&energy_sensor_values->APenergyA,
		m90e3x_convert_energy_reg((const m90e3x_data_value_t *)&energy_values->APenergyA));
	ret |= sensor_value_from_float(
		&energy_sensor_values->APenergyB,
		m90e3x_convert_energy_reg((const m90e3x_data_value_t *)&energy_values->APenergyB));
	ret |= sensor_value_from_float(
		&energy_sensor_values->APenergyC,
		m90e3x_convert_energy_reg((const m90e3x_data_value_t *)&energy_values->APenergyC));
	ret |= sensor_value_from_float(
		&energy_sensor_values->RPenergyA,
		m90e3x_convert_energy_reg((const m90e3x_data_value_t *)&energy_values->RPenergyA));
	ret |= sensor_value_from_float(
		&energy_sensor_values->RPenergyB,
		m90e3x_convert_energy_reg((const m90e3x_data_value_t *)&energy_values->RPenergyB));
	ret |= sensor_value_from_float(
		&energy_sensor_values->RPenergyC,
		m90e3x_convert_energy_reg((const m90e3x_data_value_t *)&energy_values->RPenergyC));
	ret |= sensor_value_from_float(
		&energy_sensor_values->RNenergyT,
		m90e3x_convert_energy_reg((const m90e3x_data_value_t *)&energy_values->RNenergyT));
	ret |= sensor_value_from_float(
		&energy_sensor_values->RNenergyA,
		m90e3x_convert_energy_reg((const m90e3x_data_value_t *)&energy_values->RNenergyA));
	ret |= sensor_value_from_float(
		&energy_sensor_values->RNenergyB,
		m90e3x_convert_energy_reg((const m90e3x_data_value_t *)&energy_values->RNenergyB));
	ret |= sensor_value_from_float(
		&energy_sensor_values->RNenergyC,
		m90e3x_convert_energy_reg((const m90e3x_data_value_t *)&energy_values->RNenergyC));
	ret |= sensor_value_from_float(
		&energy_sensor_values->SAenergyT,
		m90e3x_convert_energy_reg((const m90e3x_data_value_t *)&energy_values->SAenergyT));
	ret |= sensor_value_from_float(
		&energy_sensor_values->SenergyA,
		m90e3x_convert_energy_reg((const m90e3x_data_value_t *)&energy_values->SenergyA));
	ret |= sensor_value_from_float(
		&energy_sensor_values->SenergyB,
		m90e3x_convert_energy_reg((const m90e3x_data_value_t *)&energy_values->SenergyB));
	ret |= sensor_value_from_float(
		&energy_sensor_values->SenergyC,
		m90e3x_convert_energy_reg((const m90e3x_data_value_t *)&energy_values->SenergyC));

	return ret;
}

static int m90e32as_fund_energy_values_to_sensor(
	const struct device *dev,
	const struct m90e3x_fundamental_energy_data *fundamental_energy_values,
	struct sensor_value *sensor_values)
{
	/*
	 * Conversion formulas from datasheet:
	 * Energy Registers: 0.01 kWh per CF pulse
	 */

	ARG_UNUSED(dev);

	if (!fundamental_energy_values || !sensor_values) {
		return -EINVAL;
	}

	int ret = 0;

	struct m90e3x_fundamental_energy_sensor_data *fundamental_energy_sensor_values =
		(struct m90e3x_fundamental_energy_sensor_data *)sensor_values;

	ret |= sensor_value_from_float(
		&fundamental_energy_sensor_values->APenergyTF,
		m90e3x_convert_energy_reg(
			(const m90e3x_data_value_t *)&fundamental_energy_values->APenergyTF));
	ret |= sensor_value_from_float(
		&fundamental_energy_sensor_values->APenergyAF,
		m90e3x_convert_energy_reg(
			(const m90e3x_data_value_t *)&fundamental_energy_values->APenergyAF));
	ret |= sensor_value_from_float(
		&fundamental_energy_sensor_values->APenergyBF,
		m90e3x_convert_energy_reg(
			(const m90e3x_data_value_t *)&fundamental_energy_values->APenergyBF));
	ret |= sensor_value_from_float(
		&fundamental_energy_sensor_values->APenergyCF,
		m90e3x_convert_energy_reg(
			(const m90e3x_data_value_t *)&fundamental_energy_values->APenergyCF));
	ret |= sensor_value_from_float(
		&fundamental_energy_sensor_values->ANenergyTF,
		m90e3x_convert_energy_reg(
			(const m90e3x_data_value_t *)&fundamental_energy_values->ANenergyTF));
	ret |= sensor_value_from_float(
		&fundamental_energy_sensor_values->ANenergyAF,
		m90e3x_convert_energy_reg(
			(const m90e3x_data_value_t *)&fundamental_energy_values->ANenergyAF));
	ret |= sensor_value_from_float(
		&fundamental_energy_sensor_values->ANenergyBF,
		m90e3x_convert_energy_reg(
			(const m90e3x_data_value_t *)&fundamental_energy_values->ANenergyBF));
	ret |= sensor_value_from_float(
		&fundamental_energy_sensor_values->ANenergyCF,
		m90e3x_convert_energy_reg(
			(const m90e3x_data_value_t *)&fundamental_energy_values->ANenergyCF));

	return ret;
}

static int m90e32as_harmonic_energy_values_to_sensor(
	const struct device *dev, const struct m90e3x_harmonic_energy_data *harmonic_energy_values,
	struct sensor_value *sensor_values)
{
	/*
	 * Conversion formulas from datasheet:
	 * Energy Registers: 0.01 kWh per CF pulse
	 */

	ARG_UNUSED(dev);

	if (!harmonic_energy_values || !sensor_values) {
		return -EINVAL;
	}

	int ret = 0;

	struct m90e3x_harmonic_energy_sensor_data *harmonic_energy_sensor_values =
		(struct m90e3x_harmonic_energy_sensor_data *)sensor_values;

	ret |= sensor_value_from_float(
		&harmonic_energy_sensor_values->APenergyTH,
		m90e3x_convert_energy_reg(
			(const m90e3x_data_value_t *)&harmonic_energy_values->APenergyTH));
	ret |= sensor_value_from_float(
		&harmonic_energy_sensor_values->APenergyAH,
		m90e3x_convert_energy_reg(
			(const m90e3x_data_value_t *)&harmonic_energy_values->APenergyAH));
	ret |= sensor_value_from_float(
		&harmonic_energy_sensor_values->APenergyBH,
		m90e3x_convert_energy_reg(
			(const m90e3x_data_value_t *)&harmonic_energy_values->APenergyBH));
	ret |= sensor_value_from_float(
		&harmonic_energy_sensor_values->APenergyCH,
		m90e3x_convert_energy_reg(
			(const m90e3x_data_value_t *)&harmonic_energy_values->APenergyCH));
	ret |= sensor_value_from_float(
		&harmonic_energy_sensor_values->ANenergyTH,
		m90e3x_convert_energy_reg(
			(const m90e3x_data_value_t *)&harmonic_energy_values->ANenergyTH));
	ret |= sensor_value_from_float(
		&harmonic_energy_sensor_values->ANenergyAH,
		m90e3x_convert_energy_reg(
			(const m90e3x_data_value_t *)&harmonic_energy_values->ANenergyAH));
	ret |= sensor_value_from_float(
		&harmonic_energy_sensor_values->ANenergyBH,
		m90e3x_convert_energy_reg(
			(const m90e3x_data_value_t *)&harmonic_energy_values->ANenergyBH));
	ret |= sensor_value_from_float(
		&harmonic_energy_sensor_values->ANenergyCH,
		m90e3x_convert_energy_reg(
			(const m90e3x_data_value_t *)&harmonic_energy_values->ANenergyCH));

	return ret;
}

static int m90e32as_power_values_to_sensor(const struct device *dev,
					   const struct m90e3x_power_data *power_values,
					   struct sensor_value *sensor_values)
{
	/*
	 * Conversion formulas from datasheet:
	 * Power Registers (32-bit Signed): 0.00032
	 */

	ARG_UNUSED(dev);

	if (!power_values || !sensor_values) {
		return -EINVAL;
	}

	int ret = 0;

	struct m90e3x_power_sensor_data *power_sensor_values =
		(struct m90e3x_power_sensor_data *)sensor_values;

	ret |= sensor_value_from_float(
		&power_sensor_values->PmeanT,
		m90e3x_convert_power32_regs((const m90e3x_data_value_t *)&power_values->PmeanT,
					    (const m90e3x_data_value_t *)&power_values->PmeanTLSB));
	ret |= sensor_value_from_float(
		&power_sensor_values->PmeanA,
		m90e3x_convert_power32_regs((const m90e3x_data_value_t *)&power_values->PmeanA,
					    (const m90e3x_data_value_t *)&power_values->PmeanALSB));
	ret |= sensor_value_from_float(
		&power_sensor_values->PmeanB,
		m90e3x_convert_power32_regs((const m90e3x_data_value_t *)&power_values->PmeanB,
					    (const m90e3x_data_value_t *)&power_values->PmeanBLSB));
	ret |= sensor_value_from_float(
		&power_sensor_values->PmeanC,
		m90e3x_convert_power32_regs((const m90e3x_data_value_t *)&power_values->PmeanC,
					    (const m90e3x_data_value_t *)&power_values->PmeanCLSB));
	ret |= sensor_value_from_float(
		&power_sensor_values->QmeanT,
		m90e3x_convert_power32_regs((const m90e3x_data_value_t *)&power_values->QmeanT,
					    (const m90e3x_data_value_t *)&power_values->QmeanTLSB));
	ret |= sensor_value_from_float(
		&power_sensor_values->QmeanA,
		m90e3x_convert_power32_regs((const m90e3x_data_value_t *)&power_values->QmeanA,
					    (const m90e3x_data_value_t *)&power_values->QmeanALSB));
	ret |= sensor_value_from_float(
		&power_sensor_values->QmeanB,
		m90e3x_convert_power32_regs((const m90e3x_data_value_t *)&power_values->QmeanB,
					    (const m90e3x_data_value_t *)&power_values->QmeanBLSB));
	ret |= sensor_value_from_float(
		&power_sensor_values->QmeanC,
		m90e3x_convert_power32_regs((const m90e3x_data_value_t *)&power_values->QmeanC,
					    (const m90e3x_data_value_t *)&power_values->QmeanCLSB));
	ret |= sensor_value_from_float(
		&power_sensor_values->SmeanT,
		m90e3x_convert_power32_regs(
			(const m90e3x_data_value_t *)&power_values->SmeanT,
			(const m90e3x_data_value_t *)&power_values->SAmeanTLSB));
	ret |= sensor_value_from_float(
		&power_sensor_values->SmeanA,
		m90e3x_convert_power32_regs((const m90e3x_data_value_t *)&power_values->SmeanA,
					    (const m90e3x_data_value_t *)&power_values->SmeanALSB));
	ret |= sensor_value_from_float(
		&power_sensor_values->SmeanB,
		m90e3x_convert_power32_regs((const m90e3x_data_value_t *)&power_values->SmeanB,
					    (const m90e3x_data_value_t *)&power_values->SmeanBLSB));
	ret |= sensor_value_from_float(
		&power_sensor_values->SmeanC,
		m90e3x_convert_power32_regs((const m90e3x_data_value_t *)&power_values->SmeanC,
					    (const m90e3x_data_value_t *)&power_values->SmeanCLSB));

	return ret;
}

static int
m90e32as_power_factor_values_to_sensor(const struct device *dev,
				       const struct m90e3x_power_factor_data *power_factor_values,
				       struct sensor_value *sensor_values)
{
	/*
	 * Conversion formulas from datasheet:
	 * Power Factor Registers: 0.001 per LSB, Signed
	 */

	ARG_UNUSED(dev);

	if (!power_factor_values || !sensor_values) {
		return -EINVAL;
	}

	int ret = 0;

	struct m90e3x_power_factor_sensor_data *power_factor_sensor_values =
		(struct m90e3x_power_factor_sensor_data *)sensor_values;

	ret |= sensor_value_from_float(
		&power_factor_sensor_values->PFmeanT,
		m90e3x_convert_power_factor_reg(
			(const m90e3x_data_value_t *)&power_factor_values->PFmeanT));
	ret |= sensor_value_from_float(
		&power_factor_sensor_values->PFmeanA,
		m90e3x_convert_power_factor_reg(
			(const m90e3x_data_value_t *)&power_factor_values->PFmeanA));
	ret |= sensor_value_from_float(
		&power_factor_sensor_values->PFmeanB,
		m90e3x_convert_power_factor_reg(
			(const m90e3x_data_value_t *)&power_factor_values->PFmeanB));
	ret |= sensor_value_from_float(
		&power_factor_sensor_values->PFmeanC,
		m90e3x_convert_power_factor_reg(
			(const m90e3x_data_value_t *)&power_factor_values->PFmeanC));

	return ret;
}

static int m90e32as_fundamental_power_values_to_sensor(
	const struct device *dev,
	const struct m90e3x_fundamental_power_data *power_fundamental_values,
	struct sensor_value *sensor_values)
{
	/*
	 * Conversion formulas from datasheet:
	 * Power Registers (32-bit Signed): 0.00032
	 */

	ARG_UNUSED(dev);

	if (!power_fundamental_values || !sensor_values) {
		return -EINVAL;
	}

	int ret = 0;

	struct m90e3x_fundamental_power_sensor_data *power_fundamental_sensor_values =
		(struct m90e3x_fundamental_power_sensor_data *)sensor_values;

	ret |= sensor_value_from_float(
		&power_fundamental_sensor_values->PmeanTF,
		m90e3x_convert_power32_regs(
			(const m90e3x_data_value_t *)&power_fundamental_values->PmeanTF,
			(const m90e3x_data_value_t *)&power_fundamental_values->PmeanTFLSB));
	ret |= sensor_value_from_float(
		&power_fundamental_sensor_values->PmeanAF,
		m90e3x_convert_power32_regs(
			(const m90e3x_data_value_t *)&power_fundamental_values->PmeanAF,
			(const m90e3x_data_value_t *)&power_fundamental_values->PmeanAFLSB));
	ret |= sensor_value_from_float(
		&power_fundamental_sensor_values->PmeanBF,
		m90e3x_convert_power32_regs(
			(const m90e3x_data_value_t *)&power_fundamental_values->PmeanBF,
			(const m90e3x_data_value_t *)&power_fundamental_values->PmeanBFLSB));
	ret |= sensor_value_from_float(
		&power_fundamental_sensor_values->PmeanCF,
		m90e3x_convert_power32_regs(
			(const m90e3x_data_value_t *)&power_fundamental_values->PmeanCF,
			(const m90e3x_data_value_t *)&power_fundamental_values->PmeanCFLSB));

	return ret;
}

static int m90e32as_harmonic_power_values_to_sensor(
	const struct device *dev, const struct m90e3x_harmonic_power_data *power_harmonic_values,
	struct sensor_value *sensor_values)
{
	/*
	 * Conversion formulas from datasheet:
	 * Power Registers (32-bit Signed): 0.00032
	 */

	ARG_UNUSED(dev);

	if (!power_harmonic_values || !sensor_values) {
		return -EINVAL;
	}

	int ret = 0;

	struct m90e3x_harmonic_power_sensor_data *harmonic_power_sensor_values =
		(struct m90e3x_harmonic_power_sensor_data *)sensor_values;

	ret |= sensor_value_from_float(
		&harmonic_power_sensor_values->PmeanTH,
		m90e3x_convert_power32_regs(
			(const m90e3x_data_value_t *)&power_harmonic_values->PmeanTH,
			(const m90e3x_data_value_t *)&power_harmonic_values->PmeanTHLSB));
	ret |= sensor_value_from_float(
		&harmonic_power_sensor_values->PmeanAH,
		m90e3x_convert_power32_regs(
			(const m90e3x_data_value_t *)&power_harmonic_values->PmeanAH,
			(const m90e3x_data_value_t *)&power_harmonic_values->PmeanAHLSB));
	ret |= sensor_value_from_float(
		&harmonic_power_sensor_values->PmeanBH,
		m90e3x_convert_power32_regs(
			(const m90e3x_data_value_t *)&power_harmonic_values->PmeanBH,
			(const m90e3x_data_value_t *)&power_harmonic_values->PmeanBHLSB));
	ret |= sensor_value_from_float(
		&harmonic_power_sensor_values->PmeanCH,
		m90e3x_convert_power32_regs(
			(const m90e3x_data_value_t *)&power_harmonic_values->PmeanCH,
			(const m90e3x_data_value_t *)&power_harmonic_values->PmeanCHLSB));

	return ret;
}

static int m90e32as_voltage_values_to_sensor(const struct device *dev,
					     const struct m90e3x_voltage_rms_data *urms_values,
					     struct sensor_value *sensor_values)
{
	/*
	 * Conversion formulas from datasheet:
	 * URMS Registers: 0.01 V per LSB
	 */

	ARG_UNUSED(dev);

	if (!urms_values || !sensor_values) {
		return -EINVAL;
	}

	int ret = 0;

	struct m90e3x_voltage_rms_sensor_data *urms_sensor_values =
		(struct m90e3x_voltage_rms_sensor_data *)sensor_values;

	ret |= sensor_value_from_float(
		&urms_sensor_values->UrmsA,
		m90e3x_convert_voltage32_regs((const m90e3x_data_value_t *)&urms_values->UrmsA,
					      (const m90e3x_data_value_t *)&urms_values->UrmsALSB));
	ret |= sensor_value_from_float(
		&urms_sensor_values->UrmsB,
		m90e3x_convert_voltage32_regs((const m90e3x_data_value_t *)&urms_values->UrmsB,
					      (const m90e3x_data_value_t *)&urms_values->UrmsBLSB));
	ret |= sensor_value_from_float(
		&urms_sensor_values->UrmsC,
		m90e3x_convert_voltage32_regs((const m90e3x_data_value_t *)&urms_values->UrmsC,
					      (const m90e3x_data_value_t *)&urms_values->UrmsCLSB));

	return ret;
}

static int m90e32as_current_values_to_sensor(const struct device *dev,
					     const struct m90e3x_current_rms_data *irms_values,
					     struct sensor_value *sensor_values)
{
	/*
	 * Conversion formulas from datasheet:
	 * IRMS Registers: 0.001 A per LSB
	 */

	ARG_UNUSED(dev);

	if (!irms_values || !sensor_values) {
		return -EINVAL;
	}

	int ret = 0;

	struct m90e3x_current_rms_sensor_data *irms_sensor_values =
		(struct m90e3x_current_rms_sensor_data *)sensor_values;

	ret |= sensor_value_from_float(&irms_sensor_values->IrmsN, irms_values->IrmsN * 0.001f);
	ret |= sensor_value_from_float(
		&irms_sensor_values->IrmsA,
		m90e3x_convert_current32_regs((const m90e3x_data_value_t *)&irms_values->IrmsA,
					      (const m90e3x_data_value_t *)&irms_values->IrmsALSB));
	ret |= sensor_value_from_float(
		&irms_sensor_values->IrmsB,
		m90e3x_convert_current32_regs((const m90e3x_data_value_t *)&irms_values->IrmsB,
					      (const m90e3x_data_value_t *)&irms_values->IrmsBLSB));
	ret |= sensor_value_from_float(
		&irms_sensor_values->IrmsC,
		m90e3x_convert_current32_regs((const m90e3x_data_value_t *)&irms_values->IrmsC,
					      (const m90e3x_data_value_t *)&irms_values->IrmsCLSB));

	return ret;
}

static int m90e32as_peak_values_to_sensor(const struct device *dev,
					  const struct m90e32as_peak_data *peak_values,
					  struct sensor_value *sensor_values)
{
	/*
	 * Conversion formulas from datasheet:
	 * UPeak: UPeakRegValue * UgainRegValue/(100*2^13) [V]
	 * IPeak: IPeakRegValue * IgainRegValue/(1000*2^13) [A]
	 */

	ARG_UNUSED(dev);

	int ret = 0;

	struct m90e32as_peak_sensor_data *peak_sensor_values =
		(struct m90e32as_peak_sensor_data *)sensor_values;
	const struct m90e32as_config_registers *m90e32as_config_registers =
		&((const struct m90e3x_data *)dev->data)->m90e32as_config_registers;

	ret |= sensor_value_from_float(&peak_sensor_values->UpkA,
				       m90e32as_config_registers->UgainA.uint16 *
					       peak_values->UPeakA / 819200.0f);
	ret |= sensor_value_from_float(&peak_sensor_values->UpkB,
				       m90e32as_config_registers->UgainB.uint16 *
					       peak_values->UPeakB / 819200.0f);
	ret |= sensor_value_from_float(&peak_sensor_values->UpkC,
				       m90e32as_config_registers->UgainC.uint16 *
					       peak_values->UPeakC / 819200.0f);
	ret |= sensor_value_from_float(&peak_sensor_values->IpkA,
				       m90e32as_config_registers->IgainA.uint16 *
					       peak_values->IPeakA / 8192000.0f);
	ret |= sensor_value_from_float(&peak_sensor_values->IpkB,
				       m90e32as_config_registers->IgainB.uint16 *
					       peak_values->IPeakB / 8192000.0f);
	ret |= sensor_value_from_float(&peak_sensor_values->IpkC,
				       m90e32as_config_registers->IgainC.uint16 *
					       peak_values->IPeakC / 8192000.0f);
	return ret;
}

static int
m90e32as_phase_angle_values_to_sensor(const struct device *dev,
				      const struct m90e3x_phase_angle_data *phase_angle_values,
				      struct sensor_value *sensor_values)
{
	/*
	 * Conversion formulas from datasheet:
	 * Phase Angle Registers: Signed 0.1° per LSB
	 */

	ARG_UNUSED(dev);

	if (!phase_angle_values || !sensor_values) {
		return -EINVAL;
	}

	int ret = 0;

	struct m90e3x_phase_angle_sensor_data *phase_angle_sensor_values =
		(struct m90e3x_phase_angle_sensor_data *)sensor_values;

	ret |= sensor_value_from_float(
		&phase_angle_sensor_values->PAngleA,
		m90e32as_convert_phase_angle_reg(
			(const m90e3x_data_value_t *)&phase_angle_values->PAngleA));
	ret |= sensor_value_from_float(
		&phase_angle_sensor_values->PAngleB,
		m90e32as_convert_phase_angle_reg(
			(const m90e3x_data_value_t *)&phase_angle_values->PAngleB));
	ret |= sensor_value_from_float(
		&phase_angle_sensor_values->PAngleC,
		m90e32as_convert_phase_angle_reg(
			(const m90e3x_data_value_t *)&phase_angle_values->PAngleC));
	ret |= sensor_value_from_float(
		&phase_angle_sensor_values->UAngleA,
		m90e32as_convert_phase_angle_reg(
			(const m90e3x_data_value_t *)&phase_angle_values->UangleA));
	ret |= sensor_value_from_float(
		&phase_angle_sensor_values->UAngleB,
		m90e32as_convert_phase_angle_reg(
			(const m90e3x_data_value_t *)&phase_angle_values->UangleB));
	ret |= sensor_value_from_float(
		&phase_angle_sensor_values->UAngleC,
		m90e32as_convert_phase_angle_reg(
			(const m90e3x_data_value_t *)&phase_angle_values->UangleC));

	return ret;
}

static int m90e32as_sample_fetch(const struct device *dev, enum sensor_channel channel)
{
	const struct m90e3x_data *data = (const struct m90e3x_data *)dev->data;

#if CONFIG_PM_DEVICE
	if (data->current_power_mode == M90E3X_IDLE) {
		LOG_ERR("Cannot fetch data while device is in IDLE power mode.");
		return -EIO;
	}

	pm_device_busy_set(dev);

#endif /* CONFIG_PM_DEVICE */

	switch ((uint16_t)channel) {
	case SENSOR_CHAN_ALL:
		LOG_WRN("Fetching for all channels is not available.");
		break;
	case M90E3X_SENSOR_CHANNEL_ENERGY:
		m90e32as_read_register(dev, APENERGYT,
				       (m90e3x_data_value_t *)&data->energy_values.APenergyT);
		m90e32as_read_register(dev, APENERGYA,
				       (m90e3x_data_value_t *)&data->energy_values.APenergyA);
		m90e32as_read_register(dev, APENERGYB,
				       (m90e3x_data_value_t *)&data->energy_values.APenergyB);
		m90e32as_read_register(dev, APENERGYC,
				       (m90e3x_data_value_t *)&data->energy_values.APenergyC);
		m90e32as_read_register(dev, ANENERGYT,
				       (m90e3x_data_value_t *)&data->energy_values.ANenergyT);
		m90e32as_read_register(dev, ANENERGYA,
				       (m90e3x_data_value_t *)&data->energy_values.ANenergyA);
		m90e32as_read_register(dev, ANENERGYB,
				       (m90e3x_data_value_t *)&data->energy_values.ANenergyB);
		m90e32as_read_register(dev, ANENERGYC,
				       (m90e3x_data_value_t *)&data->energy_values.ANenergyC);
		m90e32as_read_register(dev, RPENERGYT,
				       (m90e3x_data_value_t *)&data->energy_values.RPenergyT);
		m90e32as_read_register(dev, RPENERGYA,
				       (m90e3x_data_value_t *)&data->energy_values.RPenergyA);
		m90e32as_read_register(dev, RPENERGYB,
				       (m90e3x_data_value_t *)&data->energy_values.RPenergyB);
		m90e32as_read_register(dev, RPENERGYC,
				       (m90e3x_data_value_t *)&data->energy_values.RPenergyC);
		m90e32as_read_register(dev, RNENERGYT,
				       (m90e3x_data_value_t *)&data->energy_values.RNenergyT);
		m90e32as_read_register(dev, RNENERGYA,
				       (m90e3x_data_value_t *)&data->energy_values.RNenergyA);
		m90e32as_read_register(dev, RNENERGYB,
				       (m90e3x_data_value_t *)&data->energy_values.RNenergyB);
		m90e32as_read_register(dev, RNENERGYC,
				       (m90e3x_data_value_t *)&data->energy_values.RNenergyC);
		m90e32as_read_register(dev, SAENERGYT,
				       (m90e3x_data_value_t *)&data->energy_values.SAenergyT);
		m90e32as_read_register(dev, SENERGYA,
				       (m90e3x_data_value_t *)&data->energy_values.SenergyA);
		m90e32as_read_register(dev, SENERGYB,
				       (m90e3x_data_value_t *)&data->energy_values.SenergyB);
		m90e32as_read_register(dev, SENERGYC,
				       (m90e3x_data_value_t *)&data->energy_values.SenergyC);
		break;
	case M90E3X_SENSOR_CHANNEL_FUNDAMENTAL_ENERGY:
		m90e32as_read_register(
			dev, APENERGYTF,
			(m90e3x_data_value_t *)&data->fundamental_energy_values.APenergyTF);
		m90e32as_read_register(
			dev, APENERGYAF,
			(m90e3x_data_value_t *)&data->fundamental_energy_values.APenergyAF);
		m90e32as_read_register(
			dev, APENERGYBF,
			(m90e3x_data_value_t *)&data->fundamental_energy_values.APenergyBF);
		m90e32as_read_register(
			dev, APENERGYCF,
			(m90e3x_data_value_t *)&data->fundamental_energy_values.APenergyCF);
		m90e32as_read_register(
			dev, ANENERGYTF,
			(m90e3x_data_value_t *)&data->fundamental_energy_values.ANenergyTF);
		m90e32as_read_register(
			dev, ANENERGYAF,
			(m90e3x_data_value_t *)&data->fundamental_energy_values.ANenergyAF);
		m90e32as_read_register(
			dev, ANENERGYBF,
			(m90e3x_data_value_t *)&data->fundamental_energy_values.ANenergyBF);
		m90e32as_read_register(
			dev, ANENERGYCF,
			(m90e3x_data_value_t *)&data->fundamental_energy_values.ANenergyCF);
		break;
	case M90E3X_SENSOR_CHANNEL_HARMONIC_ENERGY:
		m90e32as_read_register(
			dev, APENERGYTH,
			(m90e3x_data_value_t *)&data->harmonic_energy_values.APenergyTH);
		m90e32as_read_register(
			dev, APENERGYAH,
			(m90e3x_data_value_t *)&data->harmonic_energy_values.APenergyAH);
		m90e32as_read_register(
			dev, APENERGYBH,
			(m90e3x_data_value_t *)&data->harmonic_energy_values.APenergyBH);
		m90e32as_read_register(
			dev, APENERGYCH,
			(m90e3x_data_value_t *)&data->harmonic_energy_values.APenergyCH);
		m90e32as_read_register(
			dev, ANENERGYTH,
			(m90e3x_data_value_t *)&data->harmonic_energy_values.ANenergyTH);
		m90e32as_read_register(
			dev, ANENERGYAH,
			(m90e3x_data_value_t *)&data->harmonic_energy_values.ANenergyAH);
		m90e32as_read_register(
			dev, ANENERGYBH,
			(m90e3x_data_value_t *)&data->harmonic_energy_values.ANenergyBH);
		m90e32as_read_register(
			dev, ANENERGYCH,
			(m90e3x_data_value_t *)&data->harmonic_energy_values.ANenergyCH);
		break;
	case M90E3X_SENSOR_CHANNEL_POWER:
		m90e32as_read_register(dev, PMEANT,
				       (m90e3x_data_value_t *)&data->power_values.PmeanT);
		m90e32as_read_register(dev, PMEANTLSB,
				       (m90e3x_data_value_t *)&data->power_values.PmeanTLSB);
		m90e32as_read_register(dev, PMEANA,
				       (m90e3x_data_value_t *)&data->power_values.PmeanA);
		m90e32as_read_register(dev, PMEANALSB,
				       (m90e3x_data_value_t *)&data->power_values.PmeanALSB);
		m90e32as_read_register(dev, PMEANB,
				       (m90e3x_data_value_t *)&data->power_values.PmeanB);
		m90e32as_read_register(dev, PMEANBLSB,
				       (m90e3x_data_value_t *)&data->power_values.PmeanBLSB);
		m90e32as_read_register(dev, PMEANC,
				       (m90e3x_data_value_t *)&data->power_values.PmeanC);
		m90e32as_read_register(dev, PMEANCLSB,
				       (m90e3x_data_value_t *)&data->power_values.PmeanCLSB);
		m90e32as_read_register(dev, QMEANT,
				       (m90e3x_data_value_t *)&data->power_values.QmeanT);
		m90e32as_read_register(dev, QMEANTLSB,
				       (m90e3x_data_value_t *)&data->power_values.QmeanTLSB);
		m90e32as_read_register(dev, QMEANA,
				       (m90e3x_data_value_t *)&data->power_values.QmeanA);
		m90e32as_read_register(dev, QMEANALSB,
				       (m90e3x_data_value_t *)&data->power_values.QmeanALSB);
		m90e32as_read_register(dev, QMEANB,
				       (m90e3x_data_value_t *)&data->power_values.QmeanB);
		m90e32as_read_register(dev, QMEANBLSB,
				       (m90e3x_data_value_t *)&data->power_values.QmeanBLSB);
		m90e32as_read_register(dev, QMEANC,
				       (m90e3x_data_value_t *)&data->power_values.QmeanC);
		m90e32as_read_register(dev, QMEANCLSB,
				       (m90e3x_data_value_t *)&data->power_values.QmeanCLSB);
		m90e32as_read_register(dev, SMEANT,
				       (m90e3x_data_value_t *)&data->power_values.SmeanT);
		m90e32as_read_register(dev, SAMEANTLSB,
				       (m90e3x_data_value_t *)&data->power_values.SAmeanTLSB);
		m90e32as_read_register(dev, SMEANA,
				       (m90e3x_data_value_t *)&data->power_values.SmeanA);
		m90e32as_read_register(dev, SMEANALSB,
				       (m90e3x_data_value_t *)&data->power_values.SmeanALSB);
		m90e32as_read_register(dev, SMEANB,
				       (m90e3x_data_value_t *)&data->power_values.SmeanB);
		m90e32as_read_register(dev, SMEANBLSB,
				       (m90e3x_data_value_t *)&data->power_values.SmeanBLSB);
		m90e32as_read_register(dev, SMEANC,
				       (m90e3x_data_value_t *)&data->power_values.SmeanC);
		m90e32as_read_register(dev, SMEANCLSB,
				       (m90e3x_data_value_t *)&data->power_values.SmeanCLSB);
		break;
	case M90E3X_SENSOR_CHANNEL_POWER_FACTOR:
		m90e32as_read_register(dev, PFMEANT,
				       (m90e3x_data_value_t *)&data->power_factor_values.PFmeanT);
		m90e32as_read_register(dev, PFMEANA,
				       (m90e3x_data_value_t *)&data->power_factor_values.PFmeanA);
		m90e32as_read_register(dev, PFMEANB,
				       (m90e3x_data_value_t *)&data->power_factor_values.PFmeanB);
		m90e32as_read_register(dev, PFMEANC,
				       (m90e3x_data_value_t *)&data->power_factor_values.PFmeanC);
		break;
	case M90E3X_SENSOR_CHANNEL_FUNDAMENTAL_POWER:
		m90e32as_read_register(
			dev, PMEANTF,
			(m90e3x_data_value_t *)&data->fundamental_power_values.PmeanTF);
		m90e32as_read_register(
			dev, PMEANTFLSB,
			(m90e3x_data_value_t *)&data->fundamental_power_values.PmeanTFLSB);
		m90e32as_read_register(
			dev, PMEANAF,
			(m90e3x_data_value_t *)&data->fundamental_power_values.PmeanAF);
		m90e32as_read_register(
			dev, PMEANAFLSB,
			(m90e3x_data_value_t *)&data->fundamental_power_values.PmeanAFLSB);
		m90e32as_read_register(
			dev, PMEANBF,
			(m90e3x_data_value_t *)&data->fundamental_power_values.PmeanBF);
		m90e32as_read_register(
			dev, PMEANBFLSB,
			(m90e3x_data_value_t *)&data->fundamental_power_values.PmeanBFLSB);
		m90e32as_read_register(
			dev, PMEANCF,
			(m90e3x_data_value_t *)&data->fundamental_power_values.PmeanCF);
		m90e32as_read_register(
			dev, PMEANCFLSB,
			(m90e3x_data_value_t *)&data->fundamental_power_values.PmeanCFLSB);
		break;
	case M90E3X_SENSOR_CHANNEL_HARMONIC_POWER:
		m90e32as_read_register(dev, PMEANTH,
				       (m90e3x_data_value_t *)&data->harmonic_power_values.PmeanTH);
		m90e32as_read_register(
			dev, PMEANTHLSB,
			(m90e3x_data_value_t *)&data->harmonic_power_values.PmeanTHLSB);
		m90e32as_read_register(dev, PMEANAH,
				       (m90e3x_data_value_t *)&data->harmonic_power_values.PmeanAH);
		m90e32as_read_register(
			dev, PMEANAHLSB,
			(m90e3x_data_value_t *)&data->harmonic_power_values.PmeanAHLSB);
		m90e32as_read_register(dev, PMEANBH,
				       (m90e3x_data_value_t *)&data->harmonic_power_values.PmeanBH);
		m90e32as_read_register(
			dev, PMEANBHLSB,
			(m90e3x_data_value_t *)&data->harmonic_power_values.PmeanBHLSB);
		m90e32as_read_register(dev, PMEANCH,
				       (m90e3x_data_value_t *)&data->harmonic_power_values.PmeanCH);
		m90e32as_read_register(
			dev, PMEANCHLSB,
			(m90e3x_data_value_t *)&data->harmonic_power_values.PmeanCHLSB);
		break;
	case M90E3X_SENSOR_CHANNEL_VOLTAGE:
		m90e32as_read_register(dev, URMSA,
				       (m90e3x_data_value_t *)&data->voltage_rms_values.UrmsA);
		m90e32as_read_register(dev, URMSALSB,
				       (m90e3x_data_value_t *)&data->voltage_rms_values.UrmsALSB);
		m90e32as_read_register(dev, URMSB,
				       (m90e3x_data_value_t *)&data->voltage_rms_values.UrmsB);
		m90e32as_read_register(dev, URMSBLSB,
				       (m90e3x_data_value_t *)&data->voltage_rms_values.UrmsBLSB);
		m90e32as_read_register(dev, URMSC,
				       (m90e3x_data_value_t *)&data->voltage_rms_values.UrmsC);
		m90e32as_read_register(dev, URMSCLSB,
				       (m90e3x_data_value_t *)&data->voltage_rms_values.UrmsCLSB);
		break;
	case M90E3X_SENSOR_CHANNEL_CURRENT:
		m90e32as_read_register(dev, IRMSN,
				       (m90e3x_data_value_t *)&data->current_rms_values.IrmsN);
		m90e32as_read_register(dev, IRMSA,
				       (m90e3x_data_value_t *)&data->current_rms_values.IrmsA);
		m90e32as_read_register(dev, IRMSALSB,
				       (m90e3x_data_value_t *)&data->current_rms_values.IrmsALSB);
		m90e32as_read_register(dev, IRMSB,
				       (m90e3x_data_value_t *)&data->current_rms_values.IrmsB);
		m90e32as_read_register(dev, IRMSBLSB,
				       (m90e3x_data_value_t *)&data->current_rms_values.IrmsBLSB);
		m90e32as_read_register(dev, IRMSC,
				       (m90e3x_data_value_t *)&data->current_rms_values.IrmsC);
		m90e32as_read_register(dev, IRMSCLSB,
				       (m90e3x_data_value_t *)&data->current_rms_values.IrmsCLSB);
		break;
	case M90E3X_SENSOR_CHANNEL_PEAK:
		m90e32as_read_register(dev, UPEAKA,
				       (m90e3x_data_value_t *)&data->peak_values.UPeakA);
		m90e32as_read_register(dev, UPEAKB,
				       (m90e3x_data_value_t *)&data->peak_values.UPeakB);
		m90e32as_read_register(dev, UPEAKC,
				       (m90e3x_data_value_t *)&data->peak_values.UPeakC);
		m90e32as_read_register(dev, IPEAKA,
				       (m90e3x_data_value_t *)&data->peak_values.IPeakA);
		m90e32as_read_register(dev, IPEAKB,
				       (m90e3x_data_value_t *)&data->peak_values.IPeakB);
		m90e32as_read_register(dev, IPEAKC,
				       (m90e3x_data_value_t *)&data->peak_values.IPeakC);
		break;
	case M90E3X_SENSOR_CHANNEL_FREQUENCY:
		m90e32as_read_register(dev, FREQ, (m90e3x_data_value_t *)&data->Freq);
		break;
	case M90E3X_SENSOR_CHANNEL_PHASE_ANGLE:
		m90e32as_read_register(dev, PANGLEA,
				       (m90e3x_data_value_t *)&data->phase_angle_values.PAngleA);
		m90e32as_read_register(dev, PANGLEB,
				       (m90e3x_data_value_t *)&data->phase_angle_values.PAngleB);
		m90e32as_read_register(dev, PANGLEC,
				       (m90e3x_data_value_t *)&data->phase_angle_values.PAngleC);
		m90e32as_read_register(dev, UANGLEA,
				       (m90e3x_data_value_t *)&data->phase_angle_values.UangleA);
		m90e32as_read_register(dev, UANGLEB,
				       (m90e3x_data_value_t *)&data->phase_angle_values.UangleB);
		m90e32as_read_register(dev, UANGLEC,
				       (m90e3x_data_value_t *)&data->phase_angle_values.UangleC);
		break;
	case M90E3X_SENSOR_CHANNEL_TEMPERATURE:
		m90e32as_read_register(dev, TEMP, (m90e3x_data_value_t *)&data->Temp);
		break;
	default:
		return -ENOTSUP;
	}

#if CONFIG_PM_DEVICE
	pm_device_busy_clear(dev);
#endif /* CONFIG_PM_DEVICE */

	return 0;
}

static int m90e32as_channel_get(const struct device *dev, enum sensor_channel channel,
				struct sensor_value *value)
{
	struct m90e3x_data *data = (struct m90e3x_data *)dev->data;
	int ret = 0;

	switch ((uint16_t)channel) {
	case SENSOR_CHAN_ALL:
		LOG_WRN("Getting all channels not available.");
		break;
	case M90E3X_SENSOR_CHANNEL_ENERGY:
		ret = m90e32as_energy_values_to_sensor(dev, &data->energy_values, value);
		break;
	case M90E3X_SENSOR_CHANNEL_FUNDAMENTAL_ENERGY:
		ret = m90e32as_fund_energy_values_to_sensor(dev, &data->fundamental_energy_values,
							    value);
		break;
	case M90E3X_SENSOR_CHANNEL_HARMONIC_ENERGY:
		ret = m90e32as_harmonic_energy_values_to_sensor(dev, &data->harmonic_energy_values,
								value);
		break;
	case M90E3X_SENSOR_CHANNEL_POWER:
		ret = m90e32as_power_values_to_sensor(dev, &data->power_values, value);
		break;
	case M90E3X_SENSOR_CHANNEL_POWER_FACTOR:
		ret = m90e32as_power_factor_values_to_sensor(dev, &data->power_factor_values,
							     value);
		break;
	case M90E3X_SENSOR_CHANNEL_FUNDAMENTAL_POWER:
		ret = m90e32as_fundamental_power_values_to_sensor(
			dev, &data->fundamental_power_values, value);
		break;
	case M90E3X_SENSOR_CHANNEL_HARMONIC_POWER:
		ret = m90e32as_harmonic_power_values_to_sensor(dev, &data->harmonic_power_values,
							       value);
		break;
	case M90E3X_SENSOR_CHANNEL_VOLTAGE:
		ret = m90e32as_voltage_values_to_sensor(dev, &data->voltage_rms_values, value);
		break;
	case M90E3X_SENSOR_CHANNEL_CURRENT:
		ret = m90e32as_current_values_to_sensor(dev, &data->current_rms_values, value);
		break;
	case M90E3X_SENSOR_CHANNEL_PEAK:
		ret = m90e32as_peak_values_to_sensor(dev, &data->peak_values, value);
		break;
	case M90E3X_SENSOR_CHANNEL_FREQUENCY:
		ret = sensor_value_from_float(value, data->Freq * 0.01f);
		break;
	case M90E3X_SENSOR_CHANNEL_PHASE_ANGLE:
		ret = m90e32as_phase_angle_values_to_sensor(dev, &data->phase_angle_values, value);
		break;
	case M90E3X_SENSOR_CHANNEL_TEMPERATURE:
		ret = sensor_value_from_float(value, (float)data->Temp);
		break;
	default:
		LOG_ERR("Channel type not supported.");
		return -EINVAL;
	}

	return ret;
}

static void m90e32as_gpio_callback_irq0(const struct device *port, struct gpio_callback *cb,
					uint32_t pins)
{
	struct m90e3x_data *data = CONTAINER_OF(cb, struct m90e3x_data, irq0_ctx.gpio_cb);

	if (data->irq0_ctx.handler) {
		data->irq0_ctx.handler(port, &data->irq0_ctx.trigger);
	}
}

static void m90e32as_gpio_callback_irq1(const struct device *port, struct gpio_callback *cb,
					uint32_t pins)
{
	struct m90e3x_data *data = CONTAINER_OF(cb, struct m90e3x_data, irq1_ctx.gpio_cb);

	if (data->irq1_ctx.handler) {
		data->irq1_ctx.handler(port, &data->irq1_ctx.trigger);
	}
}

static void m90e32as_gpio_callback_wrn_out(const struct device *port, struct gpio_callback *cb,
					   uint32_t pins)
{
	struct m90e3x_data *data = CONTAINER_OF(cb, struct m90e3x_data, wrn_out_ctx.gpio_cb);

	if (data->wrn_out_ctx.handler) {
		data->wrn_out_ctx.handler(port, &data->wrn_out_ctx.trigger);
	}
}

static void m90e32as_gpio_callback_cf1(const struct device *port, struct gpio_callback *cb,
				       uint32_t pins)
{
	struct m90e3x_data *data = CONTAINER_OF(cb, struct m90e3x_data, cf1.gpio_cb);

	if (data->cf1.handler) {
		data->cf1.handler(port, &data->cf1.trigger);
	}
}

static void m90e32as_gpio_callback_cf2(const struct device *port, struct gpio_callback *cb,
				       uint32_t pins)
{
	struct m90e3x_data *data = CONTAINER_OF(cb, struct m90e3x_data, cf2.gpio_cb);

	if (data->cf2.handler) {
		data->cf2.handler(port, &data->cf2.trigger);
	}
}

static void m90e32as_gpio_callback_cf3(const struct device *port, struct gpio_callback *cb,
				       uint32_t pins)
{
	struct m90e3x_data *data = CONTAINER_OF(cb, struct m90e3x_data, cf3.gpio_cb);

	if (data->cf3.handler) {
		data->cf3.handler(port, &data->cf3.trigger);
	}
}

static void m90e32as_gpio_callback_cf4(const struct device *port, struct gpio_callback *cb,
				       uint32_t pins)
{
	struct m90e3x_data *data = CONTAINER_OF(cb, struct m90e3x_data, cf4.gpio_cb);

	if (data->cf4.handler) {
		data->cf4.handler(port, &data->cf4.trigger);
	}
}

static int m90e32as_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
				sensor_trigger_handler_t handler)
{
	struct m90e3x_data *data = (struct m90e3x_data *)dev->data;
	const struct m90e3x_config *cfg = (const struct m90e3x_config *)dev->config;
	int ret = -ENOTSUP;

#if CONFIG_PM_DEVICE
	pm_device_busy_set(dev);
#endif /* CONFIG_PM_DEVICE */

	switch ((uint16_t)trig->type) {
	case M90E3X_SENSOR_TRIG_TYPE_IRQ0:
		if (gpio_is_ready_dt(&cfg->irq0)) {
			data->irq0_ctx.trigger = *trig;
			data->irq0_ctx.handler = handler;
			gpio_init_callback(&data->irq0_ctx.gpio_cb, m90e32as_gpio_callback_irq0,
					   BIT(cfg->irq0.pin));
			ret = gpio_add_callback(cfg->irq0.port, &data->irq0_ctx.gpio_cb);
			if (ret == 0) {
				ret = gpio_pin_interrupt_configure_dt(&cfg->irq0,
								      GPIO_INT_EDGE_TO_ACTIVE);
			}
		}
		break;
	case M90E3X_SENSOR_TRIG_TYPE_IRQ1:
		if (gpio_is_ready_dt(&cfg->irq1)) {
			data->irq1_ctx.trigger = *trig;
			data->irq1_ctx.handler = handler;
			gpio_init_callback(&data->irq1_ctx.gpio_cb, m90e32as_gpio_callback_irq1,
					   BIT(cfg->irq1.pin));
			ret = gpio_add_callback(cfg->irq1.port, &data->irq1_ctx.gpio_cb);
			if (ret == 0) {
				ret = gpio_pin_interrupt_configure_dt(&cfg->irq1,
								      GPIO_INT_EDGE_TO_ACTIVE);
			}
		}
		break;
	case M90E3X_SENSOR_TRIG_TYPE_WRN_OUT:
		if (gpio_is_ready_dt(&cfg->wrn_out)) {
			data->wrn_out_ctx.trigger = *trig;
			data->wrn_out_ctx.handler = handler;
			gpio_init_callback(&data->wrn_out_ctx.gpio_cb,
					   m90e32as_gpio_callback_wrn_out, BIT(cfg->wrn_out.pin));
			ret = gpio_add_callback(cfg->wrn_out.port, &data->wrn_out_ctx.gpio_cb);
			if (ret == 0) {
				ret = gpio_pin_interrupt_configure_dt(&cfg->wrn_out,
								      GPIO_INT_EDGE_TO_ACTIVE);
			}
		}
		break;
	case M90E3X_SENSOR_TRIG_TYPE_CF1:
		if (gpio_is_ready_dt(&cfg->cf1)) {
			data->cf1.trigger = *trig;
			data->cf1.handler = handler;
			gpio_init_callback(&data->cf1.gpio_cb, m90e32as_gpio_callback_cf1,
					   BIT(cfg->cf1.pin));
			ret = gpio_add_callback(cfg->cf1.port, &data->cf1.gpio_cb);
			if (ret == 0) {
				ret = gpio_pin_interrupt_configure_dt(&cfg->cf1,
								      GPIO_INT_EDGE_TO_ACTIVE);
			}
		}
		break;

	case M90E3X_SENSOR_TRIG_TYPE_CF2:
		if (gpio_is_ready_dt(&cfg->cf2)) {
			data->cf2.trigger = *trig;
			data->cf2.handler = handler;
			gpio_init_callback(&data->cf2.gpio_cb, m90e32as_gpio_callback_cf2,
					   BIT(cfg->cf2.pin));
			ret = gpio_add_callback(cfg->cf2.port, &data->cf2.gpio_cb);
			if (ret == 0) {
				ret = gpio_pin_interrupt_configure_dt(&cfg->cf2,
								      GPIO_INT_EDGE_TO_ACTIVE);
			}
		}
		break;
	case M90E3X_SENSOR_TRIG_TYPE_CF3:
		if (gpio_is_ready_dt(&cfg->cf3)) {
			data->cf3.trigger = *trig;
			data->cf3.handler = handler;
			gpio_init_callback(&data->cf3.gpio_cb, m90e32as_gpio_callback_cf3,
					   BIT(cfg->cf3.pin));
			ret = gpio_add_callback(cfg->cf3.port, &data->cf3.gpio_cb);
			if (ret == 0) {
				ret = gpio_pin_interrupt_configure_dt(&cfg->cf3,
								      GPIO_INT_EDGE_TO_ACTIVE);
			}
		}
		break;
	case M90E3X_SENSOR_TRIG_TYPE_CF4:
		if (gpio_is_ready_dt(&cfg->cf4)) {
			data->cf4.trigger = *trig;
			data->cf4.handler = handler;
			gpio_init_callback(&data->cf4.gpio_cb, m90e32as_gpio_callback_cf4,
					   BIT(cfg->cf4.pin));
			ret = gpio_add_callback(cfg->cf4.port, &data->cf4.gpio_cb);
			if (ret == 0) {
				ret = gpio_pin_interrupt_configure_dt(&cfg->cf4,
								      GPIO_INT_EDGE_TO_ACTIVE);
			}
		}
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

#if CONFIG_PM_DEVICE
	pm_device_busy_clear(dev);
#endif /* CONFIG_PM_DEVICE */

	return ret;
}

static DEVICE_API(sensor, m90e32as_api) = {
	.sample_fetch = &m90e32as_sample_fetch,
	.channel_get = &m90e32as_channel_get,
	.trigger_set = &m90e32as_trigger_set,
};

#define M90E32AS_ASSIGN_PIN(pin_name)                                                              \
	IF_ENABLED(DT_INST_NODE_HAS_PROP( \
		inst, pin_name##_gpios), \
		(.pin_name = GPIO_DT_SPEC_INST_GET(inst, pin_name##_gpios),))

#define M90E32AS_DEVICE(inst)                                                                      \
	static struct m90e3x_data m90e32as_data_##inst = {                                         \
		.bus_lock = Z_MUTEX_INITIALIZER(m90e32as_data_##inst.bus_lock),                    \
		.current_power_mode = M90E3X_NORMAL,                                               \
		.m90e32as_config_registers =                                                       \
			{                                                                          \
				.MeterEn = {CONFIG_M90E32AS_METEREN},                              \
				.ChannelMapI = {CONFIG_M90E32AS_CHANNELMAPI},                      \
				.ChannelMapU = {CONFIG_M90E32AS_CHANNELMAPU},                      \
				.SagPeakDetCfg = {CONFIG_M90E32AS_SAGPEAKDETCFG},                  \
				.OVthCfg = {CONFIG_M90E32AS_OVTHCFG},                              \
				.ZXConfig = {CONFIG_M90E32AS_ZXCONFIG},                            \
				.SagTh = {CONFIG_M90E32AS_SAGTH},                                  \
				.PhaseLossTh = {CONFIG_M90E32AS_PHASELOSSTH},                      \
				.InWarnTh = {CONFIG_M90E32AS_INWARNTH},                            \
				.OIth = {CONFIG_M90E32AS_OITH},                                    \
				.FreqLoTh = {CONFIG_M90E32AS_FREQLOTH},                            \
				.FreqHiTh = {CONFIG_M90E32AS_FREQHITH},                            \
				.PMPwrCtrl = {CONFIG_M90E32AS_PMPWRCTRL},                          \
				.IRQ0MergeCfg = {CONFIG_M90E32AS_IRQ0MERGECFG},                    \
				.DetectCtrl = {CONFIG_M90E32AS_DETECTCTRL},                        \
				.DetectTh1 = {CONFIG_M90E32AS_DETECTTH1},                          \
				.DetectTh2 = {CONFIG_M90E32AS_DETECTTH2},                          \
				.DetectTh3 = {CONFIG_M90E32AS_DETECTTH3},                          \
				.IDCoffsetA = {CONFIG_M90E32AS_IDCOFFSETA},                        \
				.IDCoffsetB = {CONFIG_M90E32AS_IDCOFFSETB},                        \
				.IDCoffsetC = {CONFIG_M90E32AS_IDCOFFSETC},                        \
				.UDCoffsetA = {CONFIG_M90E32AS_UDCOFFSETA},                        \
				.UDCoffsetB = {CONFIG_M90E32AS_UDCOFFSETB},                        \
				.UDCoffsetC = {CONFIG_M90E32AS_UDCOFFSETC},                        \
				.UGainTAB = {CONFIG_M90E32AS_UGAINTAB},                            \
				.UGainTC = {CONFIG_M90E32AS_UGAINTC},                              \
				.PhiFreqComp = {CONFIG_M90E32AS_PHIFREQCOMP},                      \
				.LOGIrms0 = {CONFIG_M90E32AS_LOGIRMS0},                            \
				.LOGIrms1 = {CONFIG_M90E32AS_LOGIRMS1},                            \
				.F0 = {CONFIG_M90E32AS_F0},                                        \
				.T0 = {CONFIG_M90E32AS_T0},                                        \
				.PhiAIrms01 = {CONFIG_M90E32AS_PHIAIRMS01},                        \
				.PhiAIrms2 = {CONFIG_M90E32AS_PHIAIRMS2},                          \
				.GainAIrms01 = {CONFIG_M90E32AS_GAINAIRMS01},                      \
				.GainAIrms2 = {CONFIG_M90E32AS_GAINAIRMS2},                        \
				.PhiBIrms01 = {CONFIG_M90E32AS_PHIBIRMS01},                        \
				.PhiBIrms2 = {CONFIG_M90E32AS_PHIBIRMS2},                          \
				.GainBIrms01 = {CONFIG_M90E32AS_GAINBIRMS01},                      \
				.GainBIrms2 = {CONFIG_M90E32AS_GAINBIRMS2},                        \
				.PhiCIrms01 = {CONFIG_M90E32AS_PHICIRMS01},                        \
				.PhiCIrms2 = {CONFIG_M90E32AS_PHICIRMS2},                          \
				.GainCIrms01 = {CONFIG_M90E32AS_GAINCIRMS01},                      \
				.GainCIrms2 = {CONFIG_M90E32AS_GAINCIRMS2},                        \
				.PLconstH = {CONFIG_M90E32AS_PLCONSTH},                            \
				.PLconstL = {CONFIG_M90E32AS_PLCONSTL},                            \
				.MMode0 = {CONFIG_M90E32AS_MMODE0},                                \
				.MMode1 = {CONFIG_M90E32AS_MMODE1},                                \
				.PStartTh = {CONFIG_M90E32AS_PSTARTTH},                            \
				.QStartTh = {CONFIG_M90E32AS_QSTARTTH},                            \
				.SStartTh = {CONFIG_M90E32AS_SSTARTTH},                            \
				.PPhaseTh = {CONFIG_M90E32AS_PPHASETH},                            \
				.QPhaseTh = {CONFIG_M90E32AS_QPHASETH},                            \
				.SPhaseTh = {CONFIG_M90E32AS_SPHASETH},                            \
				.PoffsetA = {CONFIG_M90E32AS_POFFSETA},                            \
				.QoffsetA = {CONFIG_M90E32AS_QOFFSETA},                            \
				.PoffsetB = {CONFIG_M90E32AS_POFFSETB},                            \
				.QoffsetB = {CONFIG_M90E32AS_QOFFSETB},                            \
				.PoffsetC = {CONFIG_M90E32AS_POFFSETC},                            \
				.QoffsetC = {CONFIG_M90E32AS_QOFFSETC},                            \
				.PQGainA = {CONFIG_M90E32AS_PQGAINA},                              \
				.PhiA = {CONFIG_M90E32AS_PHIA},                                    \
				.PQGainB = {CONFIG_M90E32AS_PQGAINB},                              \
				.PhiB = {CONFIG_M90E32AS_PHIB},                                    \
				.PQGainC = {CONFIG_M90E32AS_PQGAINC},                              \
				.PhiC = {CONFIG_M90E32AS_PHIC},                                    \
				.PoffsetAF = {CONFIG_M90E32AS_POFFSETAF},                          \
				.PoffsetBF = {CONFIG_M90E32AS_POFFSETBF},                          \
				.PoffsetCF = {CONFIG_M90E32AS_POFFSETCF},                          \
				.PGainAF = {CONFIG_M90E32AS_PGAINAF},                              \
				.PGainBF = {CONFIG_M90E32AS_PGAINBF},                              \
				.PGainCF = {CONFIG_M90E32AS_PGAINCF},                              \
				.UgainA = {CONFIG_M90E32AS_UGAINA},                                \
				.IgainA = {CONFIG_M90E32AS_IGAINA},                                \
				.UoffsetA = {CONFIG_M90E32AS_UOFFSETA},                            \
				.IoffsetA = {CONFIG_M90E32AS_IOFFSETA},                            \
				.UgainB = {CONFIG_M90E32AS_UGAINB},                                \
				.IgainB = {CONFIG_M90E32AS_IGAINB},                                \
				.UoffsetB = {CONFIG_M90E32AS_UOFFSETB},                            \
				.IoffsetB = {CONFIG_M90E32AS_IOFFSETB},                            \
				.UgainC = {CONFIG_M90E32AS_UGAINC},                                \
				.IgainC = {CONFIG_M90E32AS_IGAINC},                                \
				.UoffsetC = {CONFIG_M90E32AS_UOFFSETC},                            \
				.IoffsetC = {CONFIG_M90E32AS_IOFFSETC},                            \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	static const struct m90e3x_config m90e32as_config_##inst = {                               \
		.bus = SPI_DT_SPEC_INST_GET(inst, M90E3X_SPI_OPERATION),                           \
		.bus_io = &m90e3x_bus_io_spi,                                                      \
		IF_ENABLED( \
			CONFIG_PM_DEVICE, (.pm_mode_ops = &m90e3x_pm_mode_ops,)			\
		) M90E32AS_ASSIGN_PIN(irq0)          \
				   M90E32AS_ASSIGN_PIN(irq1) M90E32AS_ASSIGN_PIN(wrn_out)          \
					   M90E32AS_ASSIGN_PIN(cf1) M90E32AS_ASSIGN_PIN(cf2)       \
						   M90E32AS_ASSIGN_PIN(cf3) M90E32AS_ASSIGN_PIN(   \
							   cf4) M90E32AS_ASSIGN_PIN(pm0)           \
							   M90E32AS_ASSIGN_PIN(pm1)};              \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, m90e32as_pm_action);                                        \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, m90e32as_init, PM_DEVICE_DT_INST_GET(inst),             \
				     &m90e32as_data_##inst, &m90e32as_config_##inst, POST_KERNEL,  \
				     CONFIG_SENSOR_INIT_PRIORITY, &m90e32as_api)

/* Create the struct device for every status "okay" node in the devicetree. */
DT_INST_FOREACH_STATUS_OKAY(M90E32AS_DEVICE)

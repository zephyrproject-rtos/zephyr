/*
 * Copyright (c) 2024 Aurelien Jarno
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_digi_temp

#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(stm32_digi_temp, CONFIG_SENSOR_LOG_LEVEL);

/* Constants */
#define ONE_MHZ			1000000 /* Hz */
#define TS1_T0_VAL0		30	/* °C */
#define TS1_T0_VAL1		130	/* °C */
#define SAMPLING_TIME		15	/* best precision */

struct stm32_digi_temp_data {
	struct k_sem sem_isr;
	struct k_mutex mutex;

	/* Peripheral clock frequency */
	uint32_t pclk_freq;
	/* Engineering value of the frequency measured at T0 in Hz */
	uint32_t t0_freq;
	/* Engineering value of the T0 temperature in °C */
	uint16_t t0;
	/* Engineering value of the ramp coefficient in Hz / °C */
	uint16_t ramp_coeff;

	/* Raw sensor value */
	uint16_t raw;
};

struct stm32_digi_temp_config {
	/* DTS instance. */
	DTS_TypeDef *base;
	/* Clock configuration. */
	struct stm32_pclken pclken;
	/* Interrupt configuration. */
	void (*irq_config)(const struct device *dev);
};

static void stm32_digi_temp_isr(const struct device *dev)
{
	struct stm32_digi_temp_data *data = dev->data;
	const struct stm32_digi_temp_config *cfg = dev->config;
	DTS_TypeDef *dts = cfg->base;

	/* Clear interrupt */
	SET_BIT(dts->ICIFR, DTS_ICIFR_TS1_CITEF);

	/* Give semaphore */
	k_sem_give(&data->sem_isr);
}

static int stm32_digi_temp_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct stm32_digi_temp_config *cfg = dev->config;
	struct stm32_digi_temp_data *data = dev->data;
	DTS_TypeDef *dts = cfg->base;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);

	/* Wait for the sensor to be ready (~40µS delay after enabling it) */
	while (READ_BIT(dts->SR, DTS_SR_TS1_RDY) == 0) {
		k_yield();
	}

	/* Trigger a measurement */
	SET_BIT(dts->CFGR1, DTS_CFGR1_TS1_START);
	CLEAR_BIT(dts->CFGR1, DTS_CFGR1_TS1_START);

	/* Wait for interrupt */
	k_sem_take(&data->sem_isr, K_FOREVER);

	/* Read value */
	data->raw = READ_REG(dts->DR);

	k_mutex_unlock(&data->mutex);

	return 0;
}

static int stm32_digi_temp_channel_get(const struct device *dev, enum sensor_channel chan,
				  struct sensor_value *val)
{
	struct stm32_digi_temp_data *data = dev->data;
	float meas_freq, temp;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	meas_freq = ((float)data->pclk_freq * SAMPLING_TIME) / data->raw;
	temp = data->t0 + (meas_freq - data->t0_freq) / data->ramp_coeff;

	return sensor_value_from_float(val, temp);
}

static void stm32_digi_temp_configure(const struct device *dev)
{
	const struct stm32_digi_temp_config *cfg = dev->config;
	struct stm32_digi_temp_data *data = dev->data;
	DTS_TypeDef *dts = cfg->base;
	int clk_div;

	/* Use the prescaler to obtain an internal frequency lower than 1 MHz.
	 * Allowed values are between 0 and 127.
	 */
	clk_div = MIN(DIV_ROUND_UP(data->pclk_freq, ONE_MHZ), 127);
	MODIFY_REG(dts->CFGR1, DTS_CFGR1_HSREF_CLK_DIV_Msk,
		   clk_div << DTS_CFGR1_HSREF_CLK_DIV_Pos);

	/* Select PCLK as reference clock */
	MODIFY_REG(dts->CFGR1, DTS_CFGR1_REFCLK_SEL_Msk,
		   0 << DTS_CFGR1_REFCLK_SEL_Pos);

	/* Select trigger */
	MODIFY_REG(dts->CFGR1, DTS_CFGR1_TS1_INTRIG_SEL_Msk,
		   0 << DTS_CFGR1_TS1_INTRIG_SEL_Pos);

	/* Set sampling time */
	MODIFY_REG(dts->CFGR1, DTS_CFGR1_TS1_SMP_TIME_Msk,
		   SAMPLING_TIME << DTS_CFGR1_TS1_SMP_TIME_Pos);
}

static void stm32_digi_temp_enable(const struct device *dev)
{
	const struct stm32_digi_temp_config *cfg = dev->config;
	DTS_TypeDef *dts = cfg->base;

	/* Enable the sensor */
	SET_BIT(dts->CFGR1, DTS_CFGR1_TS1_EN);

	/* Enable interrupt */
	SET_BIT(dts->ITENR, DTS_ITENR_TS1_ITEEN);
}

#ifdef CONFIG_PM_DEVICE
static void stm32_digi_temp_disable(const struct device *dev)
{
	const struct stm32_digi_temp_config *cfg = dev->config;
	DTS_TypeDef *dts = cfg->base;

	/* Disable interrupt */
	CLEAR_BIT(dts->ITENR, DTS_ITENR_TS1_ITEEN);

	/* Disable the sensor */
	CLEAR_BIT(dts->CFGR1, DTS_CFGR1_TS1_EN);
}
#endif

static int stm32_digi_temp_init(const struct device *dev)
{
	const struct stm32_digi_temp_config *cfg = dev->config;
	struct stm32_digi_temp_data *data = dev->data;
	DTS_TypeDef *dts = cfg->base;

	/* enable clock for subsystem */
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	if (!device_is_ready(clk)) {
		LOG_ERR("Clock control device not ready");
		return -ENODEV;
	}

	if (clock_control_on(clk, (clock_control_subsys_t) &cfg->pclken) != 0) {
		LOG_ERR("Could not enable DTS clock");
		return -EIO;
	}

	/* Save the peripheral clock frequency in the data structure to avoid
	 * querying it for each call to the channel_get method.
	 */
	if (clock_control_get_rate(clk, (clock_control_subsys_t) &cfg->pclken,
				   &data->pclk_freq) < 0) {
		LOG_ERR("Failed call clock_control_get_rate(pclken)");
		return -EIO;
	}

	/* Save the calibration data in the data structure to avoid reading
	 * them for each call to the channel_get method, as this requires
	 * enabling the peripheral clock.
	 */
	data->ramp_coeff = dts->RAMPVALR & DTS_RAMPVALR_TS1_RAMP_COEFF;
	data->t0_freq = (dts->T0VALR1 & DTS_T0VALR1_TS1_FMT0) * 100; /* 0.1 kHz -> Hz */

	/* T0 temperature from the datasheet */
	switch (dts->T0VALR1 >> DTS_T0VALR1_TS1_T0_Pos) {
	case 0:
		data->t0 = TS1_T0_VAL0;
		break;
	case 1:
		data->t0 = TS1_T0_VAL1;
		break;
	default:
		LOG_ERR("Unknown T0 temperature value");
		return -EIO;
	}

	/* Init mutex and semaphore */
	k_mutex_init(&data->mutex);
	k_sem_init(&data->sem_isr, 0, 1);

	/* Configure and enable the sensor */
	cfg->irq_config(dev);
	stm32_digi_temp_configure(dev);
	stm32_digi_temp_enable(dev);

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int stm32_digi_temp_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct stm32_digi_temp_config *cfg = dev->config;
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	int err;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		/* enable clock */
		err = clock_control_on(clk, (clock_control_subsys_t)&cfg->pclken);
		if (err != 0) {
			LOG_ERR("Could not enable DTS clock");
			return err;
		}
		/* Enable sensor */
		stm32_digi_temp_enable(dev);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		/* Disable sensor */
		stm32_digi_temp_disable(dev);
		/* Stop device clock */
		err = clock_control_off(clk, (clock_control_subsys_t)&cfg->pclken);
		if (err != 0) {
			LOG_ERR("Could not disable DTS clock");
			return err;
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

static const struct sensor_driver_api stm32_digi_temp_driver_api = {
	.sample_fetch = stm32_digi_temp_sample_fetch,
	.channel_get = stm32_digi_temp_channel_get,
};

#define STM32_DIGI_TEMP_INIT(index)							\
static void stm32_digi_temp_irq_config_func_##index(const struct device *dev)		\
{											\
	IRQ_CONNECT(DT_INST_IRQN(index),						\
		    DT_INST_IRQ(index, priority),					\
		    stm32_digi_temp_isr, DEVICE_DT_INST_GET(index), 0);			\
	irq_enable(DT_INST_IRQN(index));						\
}											\
											\
static struct stm32_digi_temp_data stm32_digi_temp_dev_data_##index;			\
											\
static const struct stm32_digi_temp_config stm32_digi_temp_dev_config_##index = {	\
	.base = (DTS_TypeDef *)DT_INST_REG_ADDR(index),					\
	.pclken = {									\
		.enr = DT_INST_CLOCKS_CELL(index, bits),				\
		.bus = DT_INST_CLOCKS_CELL(index, bus)					\
	},										\
	.irq_config = stm32_digi_temp_irq_config_func_##index,				\
};											\
											\
PM_DEVICE_DT_INST_DEFINE(index, stm32_digi_temp_pm_action);				\
											\
SENSOR_DEVICE_DT_INST_DEFINE(index, stm32_digi_temp_init,				\
			     PM_DEVICE_DT_INST_GET(index),				\
			     &stm32_digi_temp_dev_data_##index,				\
			     &stm32_digi_temp_dev_config_##index,			\
			     POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,			\
			     &stm32_digi_temp_driver_api);				\

DT_INST_FOREACH_STATUS_OKAY(STM32_DIGI_TEMP_INIT)

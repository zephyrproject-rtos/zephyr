/*
 * Copyright (c) 2018 Makaio GmbH.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/power/max170xx.h>
#include <misc/util.h>
#include <device.h>
#include <i2c.h>
#include <logging/log.h>
#include <logging/sys_log.h>
#include <logging/log.h>
#include <stdio.h>

#define MAX170XX_SLEEPS_AFTER_MS	2500
LOG_MODULE_REGISTER(max170xx, 4);

static int max170xx_reg_read(struct max170xx_data *drv_data, u8_t reg,
							 u16_t *val)
{
	u8_t vals[2] = {0};

	i2c_burst_read(drv_data->i2c, MAX170XX_I2C_ADDRESS, reg, vals, 2);
	*val = ((uint16_t)vals[0] << 8) | vals[1];
	drv_data->last_activity_ms = k_uptime_get_32();

	return 0;
}

static int max170xx_reg_write(struct max170xx_data *drv_data, u8_t reg,
							  u16_t data)
{
	u8_t msb;
	u8_t lsb;

	msb = (u8_t)(data & 0Xff00) >> 8;
	lsb = (u8_t)(data & 0X00ff);

	u8_t tx_buf[] = {reg, msb, lsb};

	drv_data->last_activity_ms = k_uptime_get_32();

	return i2c_write(drv_data->i2c, tx_buf, ARRAY_SIZE(tx_buf),
					 MAX170XX_I2C_ADDRESS);
}

void max170xx_wake(struct max170xx_data *drv_data)
{
	u16_t config;

	max170xx_reg_read(drv_data, 0x0c, &config);
	max170xx_reg_write(drv_data, 0x0c, config &= ~(1<<7));

	drv_data->awake_since_ms = k_uptime_get_32();
}

void max170xx_sleep(struct device *dev)
{
	struct max170xx_data *drv_data = dev->driver_data;
	u16_t config;

	max170xx_reg_read(drv_data, 0x0c, &config);
	max170xx_reg_write(drv_data, 0x0c, config |= (1<<7));

	drv_data->awake_since_ms = 0;
}

static bool max170xx_is_awake(struct max170xx_data *drv_data,
							  u32_t current_millis)
{
	if (current_millis == 0) {
		current_millis = k_uptime_get_32();
	}
	/* MAX17043/MAX17044 goes to sleep after max 2.5s */
	if (drv_data->last_activity_ms == 0
		|| (current_millis - drv_data->last_activity_ms)
		   >= MAX170XX_SLEEPS_AFTER_MS) {
		return false;
	}

	/* Sleep timeout did not expire - make sure that SLEEP bit is NOT set */
	u16_t config;

	max170xx_reg_read(drv_data, 0x0c, &config);

	return ((config & (1<<7)) == 0);
}

static void max170xx_ensure_awake(struct max170xx_data *drv_data,
		u32_t since_millis)
{
	u32_t current_millis = k_uptime_get_32();
	u32_t sleep_millis = since_millis;

	if (drv_data->awake_since_ms > 0
	   && current_millis > drv_data->awake_since_ms
	   && max170xx_is_awake(drv_data, current_millis)) {
		u32_t delta = current_millis - drv_data->awake_since_ms;

		if (delta < since_millis) {
			sleep_millis -= delta;
		} else {
			return;
		}
	} else {
		max170xx_wake(drv_data);
	}
	k_sleep(sleep_millis);
}



float max170xx_soc_get(struct device *dev)
{
	struct max170xx_data *drv_data = dev->driver_data;
	u16_t soc_raw;

	max170xx_reg_read(drv_data, 0x04, &soc_raw);
	float soc_percent = ((soc_raw >> 8) + (soc_raw & 0x00ff) / 256.f);

	return soc_percent;
}

int max170xx_vcell_get(struct device *dev)
{
	struct max170xx_data *drv_data = dev->driver_data;
	u16_t volt_raw;

	/** The VCELL register requires 500ms
	 * to update after exiting Sleep mode
	 */
	max170xx_ensure_awake(drv_data, 500);

	max170xx_reg_read(drv_data, 0x02, &volt_raw);
	return (int)((volt_raw >> 4)*1.25f);

}

#ifndef CONFIG_MAX170XX_MAX17043
int max170xx_chargerate_get(void)
{
	u16_t crate_raw;

	max170xx_reg_read(&max170xx_drv_data, 0x16, &crate_raw);
	return (int)(crate_raw * 0.208f);
}
#endif



void max170xx_info_print(struct device *dev)
{
	int voltage = max170xx_vcell_get(dev);

	LOG_DBG("%d", voltage);
#ifdef CONFIG_MAX170XX_MAX17043
	LOG_WRN("Reading charge rate is not supported");
#else

#endif
	max170xx_sleep(dev);
}

int max170xx_init(struct device *dev)
{
	struct max170xx_data *drv_data = dev->driver_data;
	int ret = 0;

	drv_data->i2c = device_get_binding(drv_data->bus_name);

	if (drv_data->i2c == NULL) {
		LOG_DBG("Failed to get pointer to %s device!",
				drv_data->bus_name);
		ret = -EINVAL;
	} else {
		LOG_INF("MAX17043 initialized on %s",
				log_strdup(drv_data->bus_name));

		max170xx_wake(drv_data);
		k_sleep(500);

		u16_t rev_val;

		max170xx_reg_read(drv_data, 0x0, &rev_val);
		LOG_DBG("MAX17043 on %s returned %u",
				log_strdup(drv_data->bus_name), rev_val);

		k_sleep(1000);
		if (rev_val == 0xc0) {

		}
	}

	return ret;
}

static const struct pmic_gauge_api max170xx_gauge_api = {
		.voltage_get = max170xx_vcell_get,
		.soc_get = max170xx_soc_get
};

static const struct pmic_api max170xx_api = {
		.gauge_api = &max170xx_gauge_api
};


/*
 * when using multiple instances, max170xx_drv_data
 * should be a multi-instance object
 *
 */
#define DEVICE_INSTANCE_INIT(dev_name, drv_name, instance, init_fn, data, \
				cfg_info, level, prio, api) \
			DEVICE_AND_API_INIT( \
				_CONCAT(_CONCAT(dev_name, _), instance), drv_name, \
				init_fn, \
				data, \
				cfg_info, level, \
				prio, api \
			)

#define DEVICE_INSTANCE_AND_DATA_INIT(dev_name, drv_name, instance, \
				init_fn, data_struct, cfg_info, \
			    level, prio, api) \
			static struct data_struct _CONCAT( \
										_CONCAT(dev_name, _data_), \
										instance) = { \
				.idx = instance, \
				.bus_name = _CONCAT(_CONCAT(_CONCAT(drv_name, _), instance), \
								_BUS_NAME), \
			}; \
			DEVICE_AND_API_INIT( \
				_CONCAT(_CONCAT(dev_name, _), instance), \
				_CONCAT(_CONCAT(_CONCAT(drv_name, _), instance), _LABEL), \
				init_fn, \
				&_CONCAT(_CONCAT(dev_name, _data_), instance), \
				cfg_info, level, \
				prio, api \
			)

#define MAX170XX_INSTANCE_INIT(instance)  \
	DEVICE_INSTANCE_AND_DATA_INIT( \
		max170xx, DT_MAXIM_MAX170XX_MAX170XX, \
		instance, &max170xx_init, \
		max170xx_data, \
		NULL, POST_KERNEL, \
		CONFIG_POWER_INIT_PRIORITY, &max170xx_api \
	)

#ifdef DT_MAXIM_MAX170XX_MAX170XX_0_LABEL
MAX170XX_INSTANCE_INIT(0);
#endif

#ifdef DT_MAXIM_MAX170XX_MAX170XX_1_LABEL
MAX170XX_INSTANCE_INIT(1);
#endif

#ifdef DT_MAXIM_MAX170XX_MAX170XX_2_LABEL
MAX170XX_INSTANCE_INIT(2);
#endif

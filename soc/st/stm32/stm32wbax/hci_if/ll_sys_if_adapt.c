/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
LOG_MODULE_REGISTER(ll_sys_if_adapt);

#include "ll_sys.h"
#include "ll_intf_cmn.h"
#if defined(CONFIG_BT_STM32WBA_USE_TEMP_BASED_CALIB)
#include <zephyr/drivers/sensor.h>
#endif /* defined(CONFIG_BT_STM32WBA_USE_TEMP_BASED_CALIB) */
#if defined(CONFIG_BT_STM32WBA)
extern struct k_mutex ble_ctrl_stack_mutex;
#endif /* CONFIG_BT_STM32WBA */
extern struct k_work_q ll_work_q;
struct k_work ll_sys_work;

#if defined(CONFIG_BT_STM32WBA_USE_TEMP_BASED_CALIB)
static const struct device *dev_temp_sensor = DEVICE_DT_GET(DT_ALIAS(die_temp0));
static struct k_work temp_calibration_work;
#endif /* defined(CONFIG_BT_STM32WBA_USE_TEMP_BASED_CALIB) */

static void ll_sys_bg_process_handler(struct k_work *work)
{
#if defined(CONFIG_BT_STM32WBA)
	k_mutex_lock(&ble_ctrl_stack_mutex, K_FOREVER);
	ll_sys_bg_process();
	k_mutex_unlock(&ble_ctrl_stack_mutex);
#else
	ll_sys_bg_process();
#endif /* CONFIG_BT_STM32WBA */
}

void ll_sys_schedule_bg_process(void)
{
	k_work_submit_to_queue(&ll_work_q, &ll_sys_work);
}

void ll_sys_schedule_bg_process_isr(void)
{
	k_work_submit_to_queue(&ll_work_q, &ll_sys_work);
}

void ll_sys_bg_process_init(void)
{
	k_work_init(&ll_sys_work, &ll_sys_bg_process_handler);
}

 #if defined(CONFIG_BT_STM32WBA_USE_TEMP_BASED_CALIB)

/**
 * @brief  Link Layer temperature calibration function
 * @param  None
 * @retval None
 */
static void ll_sys_temperature_calibration_measurement(void)
{
	struct sensor_value val;

	int rc = sensor_sample_fetch(dev_temp_sensor);

	if (rc) {
		LOG_ERR("Failed to fetch sample (%d)", rc);
		return;
	}

	rc = sensor_channel_get(dev_temp_sensor, SENSOR_CHAN_DIE_TEMP, &val);
	if (rc) {
		LOG_ERR("Failed to get data (%d)", rc);
		return;
	}

	LOG_DBG("Radio calibration, temperature : %u °C", val.val1);
	ll_intf_cmn_set_temperature_value(val.val1);
}

/**
 * @brief  Link Layer temperature calibration work handler task
 * @param  work
 * @retval None
 */
void ll_sys_temperature_calibration_measurement_work_handler(struct k_work *work)
{
	ll_sys_temperature_calibration_measurement();
}

/**
 * @brief  Link Layer temperature request background process initialization
 * @param  None
 * @retval None
 */
void ll_sys_bg_temperature_measurement_init(void)
{
	if (!device_is_ready(dev_temp_sensor)) {
		LOG_ERR("dev_temp_sensor: device %s not ready", dev_temp_sensor->name);
		k_panic();
	} else {
		/* Register Temperature Measurement task */
		k_work_init(&temp_calibration_work,
			ll_sys_temperature_calibration_measurement_work_handler);
	}
}

/**
 * @brief  Request background task processing for temperature measurement
 * @param  None
 * @retval None
 */
void ll_sys_bg_temperature_measurement(void)
{
	static uint8_t initial_temperature_acquisition;

	if (initial_temperature_acquisition == 0) {
		ll_sys_temperature_calibration_measurement();
		initial_temperature_acquisition = 1;
	} else {
		k_work_submit_to_queue(&ll_work_q, &temp_calibration_work);
	}
}

#endif /* defined(CONFIG_BT_STM32WBA_USE_TEMP_BASED_CALIB) */

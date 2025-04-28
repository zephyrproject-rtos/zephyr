#include <zephyr/devicetree.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/../../drivers/sensor/st/lsm6dsl/lsm6dsl.h>
#include <hal/nrf_gpio.h>
#include "lsm6ds3.h"

#define LSM6DS3_POWER_PIN	40 	/* Pin 8, Port 1 */

static const struct device *lsm6ds3_dev =
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(lsm6ds3tr_c));

static int lsm6ds3_power_on(const struct device *arg)
{
	/* Workaround to power on LSM6DS3 before LSM6DS3 sensor driver is initialized inside zephyr */
	nrf_gpio_cfg(LSM6DS3_POWER_PIN, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL,
				     NRF_GPIO_PIN_S0H1, NRF_GPIO_PIN_NOSENSE);
	nrf_gpio_pin_set(LSM6DS3_POWER_PIN);

	k_sleep(K_MSEC(10));

	return 0;
}

SYS_INIT(lsm6ds3_power_on, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

int lsm6ds3_init(void)
{
	if (lsm6ds3_dev == NULL) {
		return -ENODEV;
	}

	return 0;
}

int lsm6ds3_read(lsm6ds3_data_t *data)
{
	struct lsm6dsl_data *raw_data;

	if (lsm6ds3_dev == NULL) {
		return -ENODEV;
	}

	if (sensor_sample_fetch_chan(lsm6ds3_dev, SENSOR_CHAN_ACCEL_XYZ)) {
		return -EBUSY;
	}

	if (sensor_sample_fetch_chan(lsm6ds3_dev, SENSOR_CHAN_GYRO_XYZ)) {
		return -EBUSY;
	}

	raw_data = (struct lsm6dsl_data *)lsm6ds3_dev->data;

	data->accel.x = raw_data->accel_sample_x;
	data->accel.y = raw_data->accel_sample_y; 
	data->accel.z = raw_data->accel_sample_z;
	data->gyro.x = raw_data->gyro_sample_x;
	data->gyro.y = raw_data->gyro_sample_y;
	data->gyro.z = raw_data->gyro_sample_z;

	return 0;
}
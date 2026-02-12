/*
 * Copyright (c) 2024 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>

/** @brief Power Mode
 *  @details Power Mode Selection
 */
typedef enum {
	TAD214X_MODE_SBY	= 0,
	TAD214X_MODE_CONT	= 1,
	TAD214X_MODE_LPM	= 2,
	TAD214X_MODE_SINGLE	= 3
} TAD214X_PowerMode_t;

/** @brief ODR
 *  @details ODR Selection
 */
typedef enum {
	TAD214X_ODR_10	= 0,
	TAD214X_ODR_100	= 1,
	TAD214X_ODR_300	= 2
} TAD214X_ODR_t;


static struct sensor_trigger data_trigger;

static const struct device *get_tad2144_device(void)
{
    const struct device *const dev = DEVICE_DT_GET_ANY(invensense_tad2144);
    if (dev == NULL)
    {
        /* No such node, or the node does not have status "okay". */
        printk("\nError: no device found.\n");
        return NULL;
    }

    if (!device_is_ready(dev))
    {
        printk("\nError: Device \"%s\" is not ready; "
            "check the driver initialization logs for errors.\n", dev->name);
        return NULL;
    }

    printk("Found device \"%s\", getting sensor data\n", dev->name);
    return dev;
}

static int set_sampling_freq(const struct device *dev, TAD214X_ODR_t freq)
{
	int ret;
	struct sensor_value config;
    config.val1 = freq;

	ret = sensor_attr_set(dev, SENSOR_CHAN_MAGN_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &config);

	if (ret != 0) {
		printf("%s : failed to set sampling frequency or in encoder mode\n", dev->name);
	}

	return 0;
}

static int set_sampling_mode(const struct device *dev, TAD214X_PowerMode_t mode)
{
	int ret;
	struct sensor_value config;
    config.val1 = mode;

	ret = sensor_attr_set(dev, SENSOR_CHAN_MAGN_XYZ, SENSOR_ATTR_CONFIGURATION, &config);

	if (ret != 0) {
		printf("%s : failed to set sampling mode or in encoder mode\n", dev->name);
	}

	return 0;
}

static void handle_tad2144_drdy(const struct device *dev, const struct sensor_trigger *trig)
{
	if (trig->type == SENSOR_TRIG_DATA_READY) {
        int rc = sensor_sample_fetch(dev);
        struct sensor_value tmr_angle, tmr_temperature;
        float angle = 0.0, temperature = 0.0;

        if (rc == 0) {
            rc = sensor_channel_get(dev, SENSOR_CHAN_MAGN_XYZ,
                        &tmr_angle);
            rc = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP,
                        &tmr_temperature);
        }

        angle = (float)tmr_angle.val1/100.0;
        temperature = (float)tmr_temperature.val1/100.0;

        printf("Angle : %.2f, Temp: %.2f C\n", angle, temperature);

		if (rc < 0) {
			printf("sample fetch failed: %d\n", rc);
			printf("cancelling trigger due to failure: %d\n", rc);
			(void)sensor_trigger_set(dev, trig, NULL);
			return;
		}
	}
}

static int get_angle_encoder(const struct device *dev)
{
	struct sensor_value tmr_angle;
    float angle = 0.0;

	int rc = sensor_sample_fetch(dev);

	if (rc == 0) {
		rc = sensor_channel_get(dev, SENSOR_CHAN_MAGN_XYZ,
					&tmr_angle);
	}

	if (rc == 0) {
        angle = (float)tmr_angle.val1/100.0;
		printf("Angle : %.2f\n", angle);
	} else {
		printf("sample fetch/get failed: %d\n", rc);
	}

	return rc;
}


int main(void)
{
	const struct device *dev = get_tad2144_device();
    struct sensor_value angle;

	if (dev == NULL) {
		return 0;
	}

    set_sampling_freq(dev, TAD214X_ODR_100);
    set_sampling_mode(dev, TAD214X_MODE_LPM);

	data_trigger = (struct sensor_trigger){
		.type = SENSOR_TRIG_DATA_READY,
	};
    
	if (sensor_trigger_set(dev, &data_trigger, handle_tad2144_drdy) < 0) {
		printf("Cannot configure data trigger!!!\n");
		return 0;
	}

	printf("Configured for TAD2144 data collecting.\n");

    /* Only use in ENC mode */
	while (!IS_ENABLED(CONFIG_SPI) && !IS_ENABLED(CONFIG_I2C)) {
		int rc = get_angle_encoder(dev);

		if (rc != 0) {
			break;
		}
		k_sleep(K_MSEC(100));
	}
    
	return 0;
}

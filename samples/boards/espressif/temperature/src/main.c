/*
 * Copyright (c) 2026 NotioNext Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(aht30_sensor, LOG_LEVEL_INF);

/* AHT30 sensor device - using device tree */
#define AHT30_NODE DT_NODELABEL(aht30)

static const struct device *aht30_dev;

static void print_sensor_data(void)
{
    struct sensor_value temp, humidity;
    int ret;

    /* Fetch sensor data */
    ret = sensor_sample_fetch(aht30_dev);
    if (ret < 0) {
        LOG_ERR("Failed to fetch sensor data: %d", ret);
        return;
    }

    /* Get temperature */
    ret = sensor_channel_get(aht30_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
    if (ret < 0) {
        LOG_ERR("Failed to get temperature: %d", ret);
        return;
    }

    /* Get humidity */
    ret = sensor_channel_get(aht30_dev, SENSOR_CHAN_HUMIDITY, &humidity);
    if (ret < 0) {
        LOG_ERR("Failed to get humidity: %d", ret);
        return;
    }

    /* Convert sensor values to readable format */
    double temp_celsius = sensor_value_to_double(&temp);
    double humidity_percent = sensor_value_to_double(&humidity);

    /* Print to console with both float and integer representations */
    printk("=== AHT30 Sensor Reading ===\n");
    printk("Temperature: %.2f °C\n", temp_celsius);
    printk("Humidity: %.2f %%RH\n", humidity_percent);
    
    /* Also show as integer values for compatibility */
    int temp_int = (int)(temp_celsius * 100);  /* Temperature * 100 */
    int humidity_int = (int)(humidity_percent * 100);  /* Humidity * 100 */
    printk("Temperature: %d.%02d °C\n", temp_int / 100, temp_int % 100);
    printk("Humidity: %d.%02d %%RH\n", humidity_int / 100, humidity_int % 100);
    
    printk("Raw values - Temp: %d.%06d, Humidity: %d.%06d\n", 
           temp.val1, temp.val2, humidity.val1, humidity.val2);
    printk("=============================\n\n");

    /* Also log the data */
    LOG_INF("Temperature: %d.%02d°C, Humidity: %d.%02d%%RH", 
            temp_int / 100, temp_int % 100, humidity_int / 100, humidity_int % 100);
}

int main(void)
{
    LOG_INF("AHT30 Temperature & Humidity Sensor Sample");
    LOG_INF("ESP32-S3 Box 3 - I2C0 (SDA=GPIO41, SCL=GPIO40)");

    /* Get AHT30 device */
    aht30_dev = DEVICE_DT_GET(AHT30_NODE);
    if (!device_is_ready(aht30_dev)) {
        LOG_ERR("AHT30 sensor device not ready");
        return -ENODEV;
    }

    LOG_INF("AHT30 sensor device ready");
    LOG_INF("Device name: %s", aht30_dev->name);

    /* Wait a bit for sensor to stabilize */
    k_sleep(K_SECONDS(2));

    LOG_INF("Starting sensor readings every 5 seconds...");

    while (1) {
        print_sensor_data();
        k_sleep(K_SECONDS(5));
    }

    return 0;
}
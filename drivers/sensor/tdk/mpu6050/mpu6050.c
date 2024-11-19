/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT invensense_mpu6050

#include <zephyr/drivers/i2c.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include "mpu6050.h"

LOG_MODULE_REGISTER(MPU6050, CONFIG_SENSOR_LOG_LEVEL);

/* see "Accelerometer Measurements" section from register map description */
static void mpu6050_convert_accel(struct sensor_value *val, int16_t raw_val,
                                  uint16_t sensitivity_shift)
{
        int64_t conv_val;

        conv_val = ((int64_t)raw_val * SENSOR_G) >> sensitivity_shift;
        val->val1 = conv_val / 1000000;
        val->val2 = conv_val % 1000000;
}

/* see "Gyroscope Measurements" section from register map description */
static void mpu6050_convert_gyro(struct sensor_value *val, int16_t raw_val,
                                 uint16_t sensitivity_x10)
{
        int64_t conv_val;

        conv_val = ((int64_t)raw_val * SENSOR_PI * 10) / (sensitivity_x10 * 180U);
        val->val1 = conv_val / 1000000;
        val->val2 = conv_val % 1000000;
}

/* see "Temperature Measurement" section from register map description */
static inline void mpu6050_convert_temp(enum mpu6050_device_type device_type,
                                        struct sensor_value *val, int16_t raw_val)
{
        int64_t tmp_val = (int64_t)raw_val * 1000000;

        switch (device_type) {
        case DEVICE_TYPE_MPU6500:
                tmp_val = (tmp_val * 1000 / 333870) + 21000000;
                break;

        case DEVICE_TYPE_MPU6050:
        default:
                tmp_val = (tmp_val / 340) + 36000000;
        };

        val->val1 = tmp_val / 1000000;
        val->val2 = tmp_val % 1000000;
}

static int mpu6050_channel_get(const struct device *dev, enum sensor_channel chan,
                               struct sensor_value *val)
{
        struct mpu6050_data *drv_data = dev->data;

        switch (chan) {
        case SENSOR_CHAN_ACCEL_XYZ:
                mpu6050_convert_accel(val, drv_data->accel_x, drv_data->accel_sensitivity_shift);
                mpu6050_convert_accel(val + 1, drv_data->accel_y,
                                      drv_data->accel_sensitivity_shift);
                mpu6050_convert_accel(val + 2, drv_data->accel_z,
                                      drv_data->accel_sensitivity_shift);
                break;
        case SENSOR_CHAN_ACCEL_X:
                mpu6050_convert_accel(val, drv_data->accel_x, drv_data->accel_sensitivity_shift);
                break;
        case SENSOR_CHAN_ACCEL_Y:
                mpu6050_convert_accel(val, drv_data->accel_y, drv_data->accel_sensitivity_shift);
                break;
        case SENSOR_CHAN_ACCEL_Z:
                mpu6050_convert_accel(val, drv_data->accel_z, drv_data->accel_sensitivity_shift);
                break;
        case SENSOR_CHAN_GYRO_XYZ:
                mpu6050_convert_gyro(val, drv_data->gyro_x, drv_data->gyro_sensitivity_x10);
                mpu6050_convert_gyro(val + 1, drv_data->gyro_y, drv_data->gyro_sensitivity_x10);
                mpu6050_convert_gyro(val + 2, drv_data->gyro_z, drv_data->gyro_sensitivity_x10);
                break;
        case SENSOR_CHAN_GYRO_X:
                mpu6050_convert_gyro(val, drv_data->gyro_x, drv_data->gyro_sensitivity_x10);
                break;
        case SENSOR_CHAN_GYRO_Y:
                mpu6050_convert_gyro(val, drv_data->gyro_y, drv_data->gyro_sensitivity_x10);
                break;
        case SENSOR_CHAN_GYRO_Z:
                mpu6050_convert_gyro(val, drv_data->gyro_z, drv_data->gyro_sensitivity_x10);
                break;
        case SENSOR_CHAN_DIE_TEMP:
                mpu6050_convert_temp(drv_data->device_type, val, drv_data->temp);
                break;
        default:
                return -ENOTSUP;
        }

        return 0;
}

static int mpu6050_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
        struct mpu6050_data *drv_data = dev->data;
        const struct mpu6050_config *cfg = dev->config;
        int16_t buf[7];

        if (i2c_burst_read_dt(&cfg->i2c, MPU6050_REG_DATA_START, (uint8_t *)buf, 14) < 0) {
                LOG_ERR("Failed to read data sample.");
                return -EIO;
        }

        drv_data->accel_x = sys_be16_to_cpu(buf[0]);
        drv_data->accel_y = sys_be16_to_cpu(buf[1]);
        drv_data->accel_z = sys_be16_to_cpu(buf[2]);
        drv_data->temp = sys_be16_to_cpu(buf[3]);
        drv_data->gyro_x = sys_be16_to_cpu(buf[4]);
        drv_data->gyro_y = sys_be16_to_cpu(buf[5]);
        drv_data->gyro_z = sys_be16_to_cpu(buf[6]);

        return 0;
}

static const struct sensor_driver_api mpu6050_driver_api = {
#if CONFIG_MPU6050_TRIGGER
        .trigger_set = mpu6050_trigger_set,
#endif
        .sample_fetch = mpu6050_sample_fetch,
        .channel_get = mpu6050_channel_get,
};

int mpu6050_init(const struct device *dev)
{
        struct mpu6050_data *drv_data = dev->data;
        const struct mpu6050_config *cfg = dev->config;
        uint8_t id, i;

        if (!device_is_ready(cfg->i2c.bus)) {
                LOG_ERR("Bus device is not ready");
                return -ENODEV;
        }

        /* check chip ID */
        if (i2c_reg_read_byte_dt(&cfg->i2c, MPU6050_REG_CHIP_ID, &id) < 0) {
                LOG_ERR("Failed to read chip ID.");
                return -EIO;
        }

        if (id == MPU6050_CHIP_ID || id == MPU9250_CHIP_ID || id == MPU6880_CHIP_ID) {
                LOG_DBG("MPU6050/MPU9250/MPU6880 detected");
                drv_data->device_type = DEVICE_TYPE_MPU6050;
        } else if (id == MPU6500_CHIP_ID) {
                LOG_DBG("MPU6500 detected");
                drv_data->device_type = DEVICE_TYPE_MPU6500;
        } else {
                LOG_ERR("Invalid chip ID.");
                return -EINVAL;
        }

        /* Reset sequence is added to ensure all registers are reset to their
         * default values, and also to enable easier addition of SPI support in
         * future.
         */
        /* When using SPI interface, user should use DEVICE_RESET as well as
         * SIGNAL_PATH_RESET to ensure th ereest is performed properly.
         * The sequence used should be:
         * 1. Set DEVICE_RESET = 1 (reg PWR_MGMT_1)
         * 2. Wait 100ms
         * 3. Set GYRO_RESET = ACCEL_RESET = TEMP_RESET = 1 (reg SIGNAL_PATH_RESET)
         * 4. Wait 100ms
         * (RM-MPU-6000A-00 rev 4.2 page 41 of 46) */
        if (i2c_reg_write_byte_dt(&cfg->i2c, MPU6050_REG_PWR_MGMT1, MPU6050_DEVICE_RESET) < 0) {
                LOG_ERR("Device reset failed.");
                return -EIO;
        }

        k_sleep(K_MSEC(100));

        /* check content of Power Management 1 register. */
        uint8_t tmp;
        if ((i2c_reg_read_byte_dt(&cfg->i2c, MPU6050_REG_PWR_MGMT1, &tmp) < 0)) {
                LOG_ERR("Device reset request failed.");
                return -EIO;
        }

        if (tmp != MPU6050_PWR_MGMT1_RST_VAL) {
                LOG_ERR("Device reset failed.");
                return -EINVAL;
        }

        /* select clock source.
         * While gyros are active, selecting the gyros as the clock source provides
         * for a more accurate clock source.
         * (Document Number: PS-MPU-6000A-00 Page 30 of 52) */
        if (i2c_reg_write_byte_dt(&cfg->i2c, MPU6050_REG_PWR_MGMT1, 0x01) < 0) {
                LOG_ERR("Clock select failed.");
                return -EIO;
        }

        /* signal paths reset. */
        if (i2c_reg_update_byte_dt(&cfg->i2c, MPU6050_REG_SIGNAL_PATH_RESET,
                        (MPU6050_GYRO_RESET | MPU6050_ACCEL_RESET | MPU6050_TEMP_RESET),
                        0b00000111) < 0) {
                LOG_ERR("Signal path reset failed.");
                return -EIO;
        }

        /* reset signal paths of all sensors and clear sensor registers. */
        if (i2c_reg_update_byte_dt(&cfg->i2c, MPU6050_REG_USER_CTRL,
                                                MPU6050_SIG_COND_RESET, 1) < 0) {
                LOG_ERR("Signal path reset failed.");
                return -EIO;
        }

        k_sleep(K_MSEC(100));

        /* Sample Rate = Gyroscope Output Rate / (1 + smplrt_div)
         * (RM-MPU-6000A-00 rev 4.2 page 12 of 46) */
        if (i2c_reg_write_byte_dt(&cfg->i2c, MPU6050_REG_SAMPLE_RATE_DIVIDER,
                                  CONFIG_MPU6050_SAMPLE_RATE_DIVIDER) < 0) {
                LOG_ERR("Sample rate divider configuration failed.");
                return -EIO;
        }

        /* set accelerometer full-scale range */
        for (i = 0U; i < 4; i++) {
                if (BIT(i + 1) == CONFIG_MPU6050_ACCEL_FS) {
                        break;
                }
        }

        if (i == 4U) {
                LOG_ERR("Invalid value for accel full-scale range.");
                return -EINVAL;
        }

        if (i2c_reg_write_byte_dt(&cfg->i2c, MPU6050_REG_ACCEL_CFG, i << MPU6050_ACCEL_FS_SHIFT) <
            0) {
                LOG_ERR("Failed to write accel full-scale range.");
                return -EIO;
        }

        drv_data->accel_sensitivity_shift = 14 - i;

        /* set gyroscope full-scale range */
        for (i = 0U; i < 4; i++) {
                if (BIT(i) * 250 == CONFIG_MPU6050_GYRO_FS) {
                        break;
                }
        }

        if (i == 4U) {
                LOG_ERR("Invalid value for gyro full-scale range.");
                return -EINVAL;
        }

        if (i2c_reg_write_byte_dt(&cfg->i2c, MPU6050_REG_GYRO_CFG, i << MPU6050_GYRO_FS_SHIFT) <
            0) {
                LOG_ERR("Failed to write gyro full-scale range.");
                return -EIO;
        }

        drv_data->gyro_sensitivity_x10 = mpu6050_gyro_sensitivity_x10[i];

#ifdef CONFIG_MPU6050_TRIGGER
        if (cfg->int_gpio.port) {
                if (mpu6050_init_interrupt(dev) < 0) {
                        LOG_DBG("Failed to initialize interrupts.");
                        return -EIO;
                }
        }
#endif

        /* wake up chip */
        if (i2c_reg_update_byte_dt(&cfg->i2c, MPU6050_REG_PWR_MGMT1, MPU6050_SLEEP_EN, 0) < 0) {
                LOG_ERR("Failed to wake up chip.");
                return -EIO;
        }

        return 0;
}

#define MPU6050_DEFINE(inst)                                                                       \
        static struct mpu6050_data mpu6050_data_##inst;                                            \
                                                                                                   \
        static const struct mpu6050_config mpu6050_config_##inst = {                               \
                .i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
                IF_ENABLED(CONFIG_MPU6050_TRIGGER,                                              \
                           (.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, { 0 }),)) };     \
                                                                                                   \
        SENSOR_DEVICE_DT_INST_DEFINE(inst, mpu6050_init, NULL, &mpu6050_data_##inst,               \
                                     &mpu6050_config_##inst, POST_KERNEL,                          \
                                     CONFIG_SENSOR_INIT_PRIORITY, &mpu6050_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MPU6050_DEFINE)
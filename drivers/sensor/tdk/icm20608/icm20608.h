/*
 * Copyright (c) 2024, Han Wu
 * email: wuhanstudios@gmail.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM20608_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM20608_H_

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

/* Registers */
#define ICM20608_REG_SELF_TEST_X_GYRO 0x00
#define ICM20608_REG_SELF_TEST_Y_GYRO 0x01
#define ICM20608_REG_SELF_TEST_Z_GYRO 0x02

#define ICM20608_REG_SELF_TEST_X_ACCEL 0x0D
#define ICM20608_REG_SELF_TEST_Y_ACCEL 0x0E
#define ICM20608_REG_SELF_TEST_Z_ACCEL 0x0F

#define ICM20608_REG_XG_OFFS_USRH 0x13
#define ICM20608_REG_XG_OFFS_USRL 0x14
#define ICM20608_REG_YG_OFFS_USRH 0x15
#define ICM20608_REG_YG_OFFS_USRL 0x16
#define ICM20608_REG_ZG_OFFS_USRH 0x17
#define ICM20608_REG_ZG_OFFS_USRL 0x18
#define ICM20608_REG_SMPLRT_DIV   0x19

#define ICM20608_REG_CONFIG        0x1A
#define ICM20608_REG_GYRO_CONFIG   0x1B
#define ICM20608_REG_ACCEL_CONFIG  0x1C
#define ICM20608_REG_ACCEL_CONFIG2 0x1D
#define ICM20608_REG_LP_MODE_CFG   0x1E
#define ICM20608_REG_ACCEL_WOM_THR 0x1F

#define ICM20608_REG_FIFO_EN 0x23

#define ICM20608_REG_FSYNC_INT   0x36
#define ICM20608_REG_INT_PIN_CFG 0x37
#define ICM20608_REG_INT_ENABLE  0x38

#define ICM20608_REG_INT_STATUS   0x3A
#define ICM20608_REG_ACCEL_XOUT_H 0x3B
#define ICM20608_REG_ACCEL_XOUT_L 0x3C
#define ICM20608_REG_ACCEL_YOUT_H 0x3D
#define ICM20608_REG_ACCEL_YOUT_L 0x3E
#define ICM20608_REG_ACCEL_ZOUT_H 0x3F
#define ICM20608_REG_ACCEL_ZOUT_L 0x40
#define ICM20608_REG_TEMP_OUT_H   0x41
#define ICM20608_REG_TEMP_OUT_L   0x42
#define ICM20608_REG_GYRO_XOUT_H  0x43
#define ICM20608_REG_GYRO_XOUT_L  0x44
#define ICM20608_REG_GYRO_YOUT_H  0x45
#define ICM20608_REG_GYRO_YOUT_L  0x46
#define ICM20608_REG_GYRO_ZOUT_H  0x47
#define ICM20608_REG_GYRO_ZOUT_L  0x48

#define ICM20608_REG_SIGNAL_PATH_RESET 0x68
#define ICM20608_REG_ACCEL_INTEL_CTRL  0x69
#define ICM20608_REG_USER_CTRL         0x6A
#define ICM20608_REG_PWR_MGMT_1        0x6B
#define ICM20608_REG_PWR_MGMT_2        0x6C

#define ICM20608_REG_FIFO_COUNTH  0x72
#define ICM20608_REG_EFIFO_COUNTL 0x73
#define ICM20608_REG_FIFO_R_W     0x74
#define ICM20608_REG_WHO_AM_I     0x75

#define ICM20608_REG_XA_OFFSET_H 0x77
#define ICM20608_REG_XA_OFFSET_L 0x78

#define ICM20608_REG_YA_OFFSET_H 0x7A
#define ICM20608_REG_YA_OFFSET_L 0x7B

#define ICM20608_REG_ZA_OFFSET_H 0x7D
#define ICM20608_REG_ZA_OFFSET_L 0x7E

/* Configs */
#define ICM20608_I2C_ADDR   0x68
#define ICM20608D_DEVICE_ID 0xAE
#define ICM20608G_DEVICE_ID 0xAF

#define ICM20608_GYRO_FS_MAX   3
#define ICM20608_GYRO_FS_SHIFT 3
#define ICM20608_GYRO_DLPF_MAX 7

#define ICM20608_ACCEL_FS_MAX   3
#define ICM20608_ACCEL_FS_SHIFT 3
#define ICM20608_ACCEL_DLPF_MAX 7

#define ICM20608_SENS_READ_BUFF_LEN    16
#define ICM20608_ROOM_TEMP_OFFSET_DEG  21
#define ICM20608_TEMP_SENSITIVITY_X100 33387
#define ICM20608_DEG_TO_RAD            180LL

struct icm20608_data {
	int16_t accel_x;
	int16_t accel_y;
	int16_t accel_z;
	uint16_t accel_sensitivity_shift;

	int16_t temp;

	int16_t gyro_x;
	int16_t gyro_y;
	int16_t gyro_z;
	uint16_t gyro_sensitivity_x10;

#ifdef CONFIG_ICM20608_TRIGGER
	const struct device *dev;
	struct gpio_callback gpio_cb;

	const struct sensor_trigger *data_ready_trigger;
	sensor_trigger_handler_t data_ready_handler;

#if defined(CONFIG_ICM20608_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ICM20608_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_ICM20608_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif

#endif
};

struct icm20608_config {
	struct i2c_dt_spec i2c;
	uint8_t gyro_sr_div;

	uint8_t gyro_fs;
	uint8_t gyro_dlpf;

	uint8_t accel_fs;
	uint8_t accel_dlpf;

#ifdef CONFIG_ICM20608_TRIGGER
	const struct gpio_dt_spec irq_pin;
#endif /* CONFIG_ICM20608_TRIGGER */
};

enum icm20608_accel_fs_sel {
	ICM20608_ACCEL_FS_SEL_2G = 0,
	ICM20608_ACCEL_FS_SEL_4G = 1,
	ICM20608_ACCEL_FS_SEL_8G = 2,
	ICM20608_ACCEL_FS_SEL_16G = 3
};

enum icm20608_gyro_fs_sel {
	ICM20608_GYRO_FS_250 = 0,
	ICM20608_GYRO_FS_500 = 1,
	ICM20608_GYRO_FS_1000 = 2,
	ICM20608_GYRO_FS_2000 = 3,
};

#ifdef CONFIG_ICM20608_TRIGGER
int icm20608_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler);

int icm20608_init_interrupt(const struct device *dev);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM20608_H_ */

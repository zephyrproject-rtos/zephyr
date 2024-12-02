/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM42688_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM42688_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/dt-bindings/sensor/icm42688.h>
#include <stdlib.h>

static inline uint8_t icm42688_accel_fs_to_reg(uint8_t g)
{
	if (g >= 16) {
		return ICM42688_DT_ACCEL_FS_16;
	} else if (g >= 8) {
		return ICM42688_DT_ACCEL_FS_8;
	} else if (g >= 4) {
		return ICM42688_DT_ACCEL_FS_4;
	} else {
		return ICM42688_DT_ACCEL_FS_2;
	}
}

static inline void icm42688_accel_reg_to_fs(uint8_t fs, struct sensor_value *out)
{
	switch (fs) {
	case ICM42688_DT_ACCEL_FS_16:
		sensor_g_to_ms2(16, out);
		return;
	case ICM42688_DT_ACCEL_FS_8:
		sensor_g_to_ms2(8, out);
		return;
	case ICM42688_DT_ACCEL_FS_4:
		sensor_g_to_ms2(4, out);
		return;
	case ICM42688_DT_ACCEL_FS_2:
		sensor_g_to_ms2(2, out);
		return;
	}
}

static inline uint8_t icm42688_gyro_fs_to_reg(uint16_t dps)
{
	if (dps >= 2000) {
		return ICM42688_DT_GYRO_FS_2000;
	} else if (dps >= 1000) {
		return ICM42688_DT_GYRO_FS_1000;
	} else if (dps >= 500) {
		return ICM42688_DT_GYRO_FS_500;
	} else if (dps >= 250) {
		return ICM42688_DT_GYRO_FS_250;
	} else if (dps >= 125) {
		return ICM42688_DT_GYRO_FS_125;
	} else if (dps >= 62) {
		return ICM42688_DT_GYRO_FS_62_5;
	} else if (dps >= 31) {
		return ICM42688_DT_GYRO_FS_31_25;
	} else {
		return ICM42688_DT_GYRO_FS_15_625;
	}
}

static inline void icm42688_gyro_reg_to_fs(uint8_t fs, struct sensor_value *out)
{
	switch (fs) {
	case ICM42688_DT_GYRO_FS_2000:
		sensor_degrees_to_rad(2000, out);
		return;
	case ICM42688_DT_GYRO_FS_1000:
		sensor_degrees_to_rad(1000, out);
		return;
	case ICM42688_DT_GYRO_FS_500:
		sensor_degrees_to_rad(500, out);
		return;
	case ICM42688_DT_GYRO_FS_250:
		sensor_degrees_to_rad(250, out);
		return;
	case ICM42688_DT_GYRO_FS_125:
		sensor_degrees_to_rad(125, out);
		return;
	case ICM42688_DT_GYRO_FS_62_5:
		sensor_10udegrees_to_rad(6250000, out);
		return;
	case ICM42688_DT_GYRO_FS_31_25:
		sensor_10udegrees_to_rad(3125000, out);
		return;
	case ICM42688_DT_GYRO_FS_15_625:
		sensor_10udegrees_to_rad(1562500, out);
		return;
	}
}

static inline uint8_t icm42688_accel_hz_to_reg(uint16_t hz)
{
	if (hz >= 32000) {
		return ICM42688_DT_ACCEL_ODR_32000;
	} else if (hz >= 16000) {
		return ICM42688_DT_ACCEL_ODR_16000;
	} else if (hz >= 8000) {
		return ICM42688_DT_ACCEL_ODR_8000;
	} else if (hz >= 4000) {
		return ICM42688_DT_ACCEL_ODR_4000;
	} else if (hz >= 2000) {
		return ICM42688_DT_ACCEL_ODR_2000;
	} else if (hz >= 1000) {
		return ICM42688_DT_ACCEL_ODR_1000;
	} else if (hz >= 500) {
		return ICM42688_DT_ACCEL_ODR_500;
	} else if (hz >= 200) {
		return ICM42688_DT_ACCEL_ODR_200;
	} else if (hz >= 100) {
		return ICM42688_DT_ACCEL_ODR_100;
	} else if (hz >= 50) {
		return ICM42688_DT_ACCEL_ODR_50;
	} else if (hz >= 25) {
		return ICM42688_DT_ACCEL_ODR_25;
	} else if (hz >= 12) {
		return ICM42688_DT_ACCEL_ODR_12_5;
	} else if (hz >= 6) {
		return ICM42688_DT_ACCEL_ODR_6_25;
	} else if (hz >= 3) {
		return ICM42688_DT_ACCEL_ODR_3_125;
	} else {
		return ICM42688_DT_ACCEL_ODR_1_5625;
	}
}

static inline void icm42688_accel_reg_to_hz(uint8_t odr, struct sensor_value *out)
{
	switch (odr) {
	case ICM42688_DT_ACCEL_ODR_32000:
		out->val1 = 32000;
		out->val2 = 0;
		return;
	case ICM42688_DT_ACCEL_ODR_16000:
		out->val1 = 1600;
		out->val2 = 0;
		return;
	case ICM42688_DT_ACCEL_ODR_8000:
		out->val1 = 8000;
		out->val2 = 0;
		return;
	case ICM42688_DT_ACCEL_ODR_4000:
		out->val1 = 4000;
		out->val2 = 0;
		return;
	case ICM42688_DT_ACCEL_ODR_2000:
		out->val1 = 2000;
		out->val2 = 0;
		return;
	case ICM42688_DT_ACCEL_ODR_1000:
		out->val1 = 1000;
		out->val2 = 0;
		return;
	case ICM42688_DT_ACCEL_ODR_500:
		out->val1 = 500;
		out->val2 = 0;
		return;
	case ICM42688_DT_ACCEL_ODR_200:
		out->val1 = 200;
		out->val2 = 0;
		return;
	case ICM42688_DT_ACCEL_ODR_100:
		out->val1 = 100;
		out->val2 = 0;
		return;
	case ICM42688_DT_ACCEL_ODR_50:
		out->val1 = 50;
		out->val2 = 0;
		return;
	case ICM42688_DT_ACCEL_ODR_25:
		out->val1 = 25;
		out->val2 = 0;
		return;
	case ICM42688_DT_ACCEL_ODR_12_5:
		out->val1 = 12;
		out->val2 = 500000;
		return;
	case ICM42688_DT_ACCEL_ODR_6_25:
		out->val1 = 6;
		out->val2 = 250000;
		return;
	case ICM42688_DT_ACCEL_ODR_3_125:
		out->val1 = 3;
		out->val2 = 125000;
		return;
	case ICM42688_DT_ACCEL_ODR_1_5625:
		out->val1 = 1;
		out->val2 = 562500;
		return;
	}
}

static inline uint8_t icm42688_gyro_odr_to_reg(uint16_t hz)
{
	if (hz >= 32000) {
		return ICM42688_DT_GYRO_ODR_32000;
	} else if (hz >= 16000) {
		return ICM42688_DT_GYRO_ODR_16000;
	} else if (hz >= 8000) {
		return ICM42688_DT_GYRO_ODR_8000;
	} else if (hz >= 4000) {
		return ICM42688_DT_GYRO_ODR_4000;
	} else if (hz >= 2000) {
		return ICM42688_DT_GYRO_ODR_2000;
	} else if (hz >= 1000) {
		return ICM42688_DT_GYRO_ODR_1000;
	} else if (hz >= 500) {
		return ICM42688_DT_GYRO_ODR_500;
	} else if (hz >= 200) {
		return ICM42688_DT_GYRO_ODR_200;
	} else if (hz >= 100) {
		return ICM42688_DT_GYRO_ODR_100;
	} else if (hz >= 50) {
		return ICM42688_DT_GYRO_ODR_50;
	} else if (hz >= 25) {
		return ICM42688_DT_GYRO_ODR_25;
	} else {
		return ICM42688_DT_GYRO_ODR_12_5;
	}
}

static inline void icm42688_gyro_reg_to_odr(uint8_t odr, struct sensor_value *out)
{
	switch (odr) {
	case ICM42688_DT_GYRO_ODR_32000:
		out->val1 = 32000;
		out->val2 = 0;
		return;
	case ICM42688_DT_GYRO_ODR_16000:
		out->val1 = 16000;
		out->val2 = 0;
		return;
	case ICM42688_DT_GYRO_ODR_8000:
		out->val1 = 8000;
		out->val2 = 0;
		return;
	case ICM42688_DT_GYRO_ODR_4000:
		out->val1 = 4000;
		out->val2 = 0;
		return;
	case ICM42688_DT_GYRO_ODR_2000:
		out->val1 = 2000;
		out->val2 = 0;
		return;
	case ICM42688_DT_GYRO_ODR_1000:
		out->val1 = 1000;
		out->val2 = 0;
		return;
	case ICM42688_DT_GYRO_ODR_500:
		out->val1 = 500;
		out->val2 = 0;
		return;
	case ICM42688_DT_GYRO_ODR_200:
		out->val1 = 200;
		out->val2 = 0;
		return;
	case ICM42688_DT_GYRO_ODR_100:
		out->val1 = 100;
		out->val2 = 0;
		return;
	case ICM42688_DT_GYRO_ODR_50:
		out->val1 = 50;
		out->val2 = 0;
		return;
	case ICM42688_DT_GYRO_ODR_25:
		out->val1 = 25;
		out->val2 = 0;
		return;
	case ICM42688_DT_GYRO_ODR_12_5:
		out->val1 = 12;
		out->val2 = 500000;
		return;
	}
}

/**
 * @brief All sensor configuration options
 */
struct icm42688_cfg {
	uint8_t accel_pwr_mode;
	uint8_t accel_fs;
	uint8_t accel_odr;
	/* TODO accel signal processing options */

	uint8_t gyro_pwr_mode;
	uint8_t gyro_fs;
	uint8_t gyro_odr;
	/* TODO gyro signal processing options */

	bool temp_dis;
	/* TODO temp signal processing options */

	/* TODO timestamp options */

	bool fifo_en;
	int32_t batch_ticks;
	bool fifo_hires;
	/* TODO additional FIFO options */

	/* TODO interrupt options */
	bool interrupt1_drdy;
	bool interrupt1_fifo_ths;
	bool interrupt1_fifo_full;
};

struct icm42688_trigger_entry {
	struct sensor_trigger trigger;
	sensor_trigger_handler_t handler;
};

/**
 * @brief Device data (struct device)
 */
struct icm42688_dev_data {
	struct icm42688_cfg cfg;
#ifdef CONFIG_ICM42688_TRIGGER
#if defined(CONFIG_ICM42688_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ICM42688_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_ICM42688_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif
#ifdef CONFIG_ICM42688_STREAM
	struct rtio_iodev_sqe *streaming_sqe;
	struct rtio *r;
	struct rtio_iodev *spi_iodev;
	uint8_t int_status;
	uint16_t fifo_count;
	uint64_t timestamp;
	atomic_t reading_fifo;
#endif /* CONFIG_ICM42688_STREAM */
	const struct device *dev;
	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t data_ready_handler;
	const struct sensor_trigger *data_ready_trigger;
	struct k_mutex mutex;
#endif /* CONFIG_ICM42688_TRIGGER */

	int16_t readings[7];
};

/**
 * @brief Device config (struct device)
 */
struct icm42688_dev_cfg {
	struct spi_dt_spec spi;
	struct gpio_dt_spec gpio_int1;
	struct gpio_dt_spec gpio_int2;
};

/**
 * @brief Reset the sensor
 *
 * @param dev icm42688 device pointer
 *
 * @retval 0 success
 * @retval -EINVAL Reset status or whoami register returned unexpected value.
 */
int icm42688_reset(const struct device *dev);

/**
 * @brief (Re)Configure the sensor with the given configuration
 *
 * @param dev icm42688 device pointer
 * @param cfg icm42688_cfg pointer
 *
 * @retval 0 success
 * @retval -errno Error
 */
int icm42688_configure(const struct device *dev, struct icm42688_cfg *cfg);


/**
 * @brief Safely (re)Configure the sensor with the given configuration
 *
 * Will rollback to prior configuration if new configuration is invalid
 *
 * @param dev icm42688 device pointer
 * @param cfg icm42688_cfg pointer
 *
 * @retval 0 success
 * @retval -errno Error
 */
int icm42688_safely_configure(const struct device *dev, struct icm42688_cfg *cfg);

/**
 * @brief Reads all channels
 *
 * Regardless of what is enabled/disabled this reads all data registers
 * as the time to read the 14 bytes at 1MHz is going to be 112 us which
 * is less time than a SPI transaction takes to setup typically.
 *
 * @param dev icm42688 device pointer
 * @param buf 14 byte buffer to store data values (7 channels, 2 bytes each)
 *
 * @retval 0 success
 * @retval -errno Error
 */
int icm42688_read_all(const struct device *dev, uint8_t data[14]);

/**
 * @brief Convert icm42688 accelerometer value to useful g values
 *
 * @param cfg icm42688_cfg current device configuration
 * @param in raw data value in int32_t format
 * @param out_g whole G's output in int32_t
 * @param out_ug micro (1/1000000) of a G output as uint32_t
 */
static inline void icm42688_accel_g(struct icm42688_cfg *cfg, int32_t in, int32_t *out_g,
				    uint32_t *out_ug)
{
	int32_t sensitivity;

	switch (cfg->accel_fs) {
	case ICM42688_DT_ACCEL_FS_2:
		sensitivity = 16384;
		break;
	case ICM42688_DT_ACCEL_FS_4:
		sensitivity = 8192;
		break;
	case ICM42688_DT_ACCEL_FS_8:
		sensitivity = 4096;
		break;
	case ICM42688_DT_ACCEL_FS_16:
		sensitivity = 2048;
		break;
	default:
		CODE_UNREACHABLE;
	}

	/* Whole g's */
	*out_g = in / sensitivity;

	/* Micro g's */
	*out_ug = ((abs(in) - (abs((*out_g)) * sensitivity)) * 1000000) / sensitivity;
}

/**
 * @brief Convert icm42688 gyroscope value to useful deg/s values
 *
 * @param cfg icm42688_cfg current device configuration
 * @param in raw data value in int32_t format
 * @param out_dps whole deg/s output in int32_t
 * @param out_udps micro (1/1000000) deg/s as uint32_t
 */
static inline void icm42688_gyro_dps(const struct icm42688_cfg *cfg, int32_t in, int32_t *out_dps,
				     uint32_t *out_udps)
{
	int64_t sensitivity;

	switch (cfg->gyro_fs) {
	case ICM42688_DT_GYRO_FS_2000:
		sensitivity = 164;
		break;
	case ICM42688_DT_GYRO_FS_1000:
		sensitivity = 328;
		break;
	case ICM42688_DT_GYRO_FS_500:
		sensitivity = 655;
		break;
	case ICM42688_DT_GYRO_FS_250:
		sensitivity = 1310;
		break;
	case ICM42688_DT_GYRO_FS_125:
		sensitivity = 2620;
		break;
	case ICM42688_DT_GYRO_FS_62_5:
		sensitivity = 5243;
		break;
	case ICM42688_DT_GYRO_FS_31_25:
		sensitivity = 10486;
		break;
	case ICM42688_DT_GYRO_FS_15_625:
		sensitivity = 20972;
		break;
	default:
		CODE_UNREACHABLE;
	}

	int32_t in10 = in * 10;

	/* Whole deg/s */
	*out_dps = in10 / sensitivity;

	/* Micro deg/s */
	*out_udps = ((int64_t)(llabs(in10) - (llabs((*out_dps)) * sensitivity)) * 1000000LL) /
		    sensitivity;
}

/**
 * @brief Convert icm42688 accelerometer value to useful m/s^2 values
 *
 * @param cfg icm42688_cfg current device configuration
 * @param in raw data value in int32_t format
 * @param out_ms meters/s^2 (whole) output in int32_t
 * @param out_ums micrometers/s^2 output as uint32_t
 */
static inline void icm42688_accel_ms(const struct icm42688_cfg *cfg, int32_t in, int32_t *out_ms,
				     int32_t *out_ums)
{
	int64_t sensitivity;

	switch (cfg->accel_fs) {
	case ICM42688_DT_ACCEL_FS_2:
		sensitivity = 16384;
		break;
	case ICM42688_DT_ACCEL_FS_4:
		sensitivity = 8192;
		break;
	case ICM42688_DT_ACCEL_FS_8:
		sensitivity = 4096;
		break;
	case ICM42688_DT_ACCEL_FS_16:
		sensitivity = 2048;
		break;
	default:
		CODE_UNREACHABLE;
	}

	/* Convert to micrometers/s^2 */
	int64_t in_ms = in * SENSOR_G;

	/* meters/s^2 whole values */
	*out_ms = in_ms / (sensitivity * 1000000LL);

	/* micrometers/s^2 */
	*out_ums = (in_ms - (*out_ms * sensitivity * 1000000LL)) / sensitivity;
}

/**
 * @brief Convert icm42688 gyroscope value to useful rad/s values
 *
 * @param cfg icm42688_cfg current device configuration
 * @param in raw data value in int32_t format
 * @param out_rads whole rad/s output in int32_t
 * @param out_urads microrad/s as uint32_t
 */
static inline void icm42688_gyro_rads(const struct icm42688_cfg *cfg, int32_t in, int32_t *out_rads,
				      int32_t *out_urads)
{
	int64_t sensitivity;

	switch (cfg->gyro_fs) {
	case ICM42688_DT_GYRO_FS_2000:
		sensitivity = 164;
		break;
	case ICM42688_DT_GYRO_FS_1000:
		sensitivity = 328;
		break;
	case ICM42688_DT_GYRO_FS_500:
		sensitivity = 655;
		break;
	case ICM42688_DT_GYRO_FS_250:
		sensitivity = 1310;
		break;
	case ICM42688_DT_GYRO_FS_125:
		sensitivity = 2620;
		break;
	case ICM42688_DT_GYRO_FS_62_5:
		sensitivity = 5243;
		break;
	case ICM42688_DT_GYRO_FS_31_25:
		sensitivity = 10486;
		break;
	case ICM42688_DT_GYRO_FS_15_625:
		sensitivity = 20972;
		break;
	default:
		CODE_UNREACHABLE;
	}

	int64_t in10_rads = (int64_t)in * SENSOR_PI * 10LL;

	/* Whole rad/s */
	*out_rads = in10_rads / (sensitivity * 180LL * 1000000LL);

	/* microrad/s */
	*out_urads =
		(in10_rads - (*out_rads * sensitivity * 180LL * 1000000LL)) / (sensitivity * 180LL);
}

/**
 * @brief Convert icm42688 temp value to useful celsius values
 *
 * @param cfg icm42688_cfg current device configuration
 * @param in raw data value in int32_t format
 * @param out_c whole celsius output in int32_t
 * @param out_uc micro (1/1000000) celsius as uint32_t
 */
static inline void icm42688_temp_c(int32_t in, int32_t *out_c, uint32_t *out_uc)
{
	int64_t sensitivity = 13248; /* value equivalent for x100 1c */

	/* Offset by 25 degrees Celsius */
	int64_t in100 = (in * 100) + (25 * sensitivity);

	/* Whole celsius */
	*out_c = in100 / sensitivity;

	/* Micro celsius */
	*out_uc = ((in100 - (*out_c) * sensitivity) * INT64_C(1000000)) / sensitivity;
}

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM42688_H_ */

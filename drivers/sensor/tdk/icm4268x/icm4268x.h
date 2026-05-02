/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM4268X_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM4268X_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/dt-bindings/sensor/icm42688.h>
#include <zephyr/dt-bindings/sensor/icm42686.h>
#include <stdlib.h>
#include "icm4268x_bus.h"

struct alignment {
	int8_t index;
	int8_t sign;
};

enum icm4268x_variant {
	ICM4268X_VARIANT_ICM42688 = 0,
	ICM4268X_VARIANT_ICM42686 = 1,
};

/** Helper struct used to map between values and DT-options (e.g: DT_ACCEL_FS) */
struct icm4268x_reg_val_pair {
	uint8_t reg;
	int32_t val;
};

static const uint8_t table_accel_fs_to_reg_array_size[] = {
	[ICM4268X_VARIANT_ICM42688] = 4, /* FS16 to FS2 */
	[ICM4268X_VARIANT_ICM42686] = 5, /* FS32 to FS2 */
};

static const struct icm4268x_reg_val_pair table_accel_fs_to_reg[][5] = {
	[ICM4268X_VARIANT_ICM42688] = {
		{.val = 16, .reg = ICM42688_DT_ACCEL_FS_16},
		{.val = 8, .reg = ICM42688_DT_ACCEL_FS_8},
		{.val = 4, .reg = ICM42688_DT_ACCEL_FS_4},
		{.val = 2, .reg = ICM42688_DT_ACCEL_FS_2},
	},
	[ICM4268X_VARIANT_ICM42686] = {
		{.val = 32, .reg = ICM42686_DT_ACCEL_FS_32},
		{.val = 16, .reg = ICM42686_DT_ACCEL_FS_16},
		{.val = 8, .reg = ICM42686_DT_ACCEL_FS_8},
		{.val = 4, .reg = ICM42686_DT_ACCEL_FS_4},
		{.val = 2, .reg = ICM42686_DT_ACCEL_FS_2},
	},
};

static inline uint8_t icm4268x_accel_fs_to_reg(uint8_t g, enum icm4268x_variant variant)
{
	for (uint8_t i = 0 ; i < table_accel_fs_to_reg_array_size[variant] ; i++) {
		if (g >= table_accel_fs_to_reg[variant][i].val) {
			return table_accel_fs_to_reg[variant][i].reg;
		}
	}

	/** Force values less than lower boundary */
	return table_accel_fs_to_reg[variant][table_accel_fs_to_reg_array_size[variant] - 1].reg;
}

static inline void icm4268x_accel_reg_to_fs(uint8_t fs, enum icm4268x_variant variant,
					    struct sensor_value *out)
{
	for (uint8_t i = 0 ; i < table_accel_fs_to_reg_array_size[variant] ; i++) {
		if (fs == table_accel_fs_to_reg[variant][i].reg) {
			sensor_g_to_ms2(table_accel_fs_to_reg[variant][i].val, out);
			return;
		}
	}

	CODE_UNREACHABLE;
}

static const uint8_t table_gyro_fs_to_reg_array_size[] = {
	[ICM4268X_VARIANT_ICM42688] = 8, /* FS2000 to FS15_625 */
	[ICM4268X_VARIANT_ICM42686] = 8, /* FS4000 to FS31_25 */
};

static const struct icm4268x_reg_val_pair table_gyro_fs_to_reg[][8] = {
	[ICM4268X_VARIANT_ICM42688] = {
		{.val = 200000000, .reg = ICM42688_DT_GYRO_FS_2000},
		{.val = 100000000, .reg = ICM42688_DT_GYRO_FS_1000},
		{.val = 50000000,  .reg = ICM42688_DT_GYRO_FS_500},
		{.val = 25000000,  .reg = ICM42688_DT_GYRO_FS_250},
		{.val = 12500000,  .reg = ICM42688_DT_GYRO_FS_125},
		{.val = 6250000,   .reg = ICM42688_DT_GYRO_FS_62_5},
		{.val = 3125000,   .reg = ICM42688_DT_GYRO_FS_31_25},
		{.val = 1562500,   .reg = ICM42688_DT_GYRO_FS_15_625},
	},
	[ICM4268X_VARIANT_ICM42686] = {
		{.val = 400000000, .reg = ICM42686_DT_GYRO_FS_4000},
		{.val = 200000000, .reg = ICM42686_DT_GYRO_FS_2000},
		{.val = 100000000, .reg = ICM42686_DT_GYRO_FS_1000},
		{.val = 50000000,  .reg = ICM42686_DT_GYRO_FS_500},
		{.val = 25000000,  .reg = ICM42686_DT_GYRO_FS_250},
		{.val = 12500000,  .reg = ICM42686_DT_GYRO_FS_125},
		{.val = 6250000,   .reg = ICM42686_DT_GYRO_FS_62_5},
		{.val = 3125000,   .reg = ICM42686_DT_GYRO_FS_31_25},
	},
};

static inline uint8_t icm4268x_gyro_fs_to_reg(uint16_t dps, enum icm4268x_variant variant)
{
	for (uint8_t i = 0 ; i < table_gyro_fs_to_reg_array_size[variant] ; i++) {
		if (dps * 100000 >= table_gyro_fs_to_reg[variant][i].val) {
			return table_gyro_fs_to_reg[variant][i].reg;
		}
	}

	/** Force values less than lower boundary */
	return table_gyro_fs_to_reg[variant][table_gyro_fs_to_reg_array_size[variant] - 1].reg;
}

static inline void icm4268x_gyro_reg_to_fs(uint8_t fs, enum icm4268x_variant variant,
					   struct sensor_value *out)
{
	for (uint8_t i = 0 ; i < table_gyro_fs_to_reg_array_size[variant] ; i++) {
		if (fs == table_gyro_fs_to_reg[variant][i].reg) {
			sensor_10udegrees_to_rad(table_gyro_fs_to_reg[variant][i].val, out);
			return;
		}
	}

	CODE_UNREACHABLE;
}

static inline uint8_t icm4268x_accel_hz_to_reg(uint16_t hz)
{
	if (hz >= 32000) {
		return ICM4268X_DT_ACCEL_ODR_32000;
	} else if (hz >= 16000) {
		return ICM4268X_DT_ACCEL_ODR_16000;
	} else if (hz >= 8000) {
		return ICM4268X_DT_ACCEL_ODR_8000;
	} else if (hz >= 4000) {
		return ICM4268X_DT_ACCEL_ODR_4000;
	} else if (hz >= 2000) {
		return ICM4268X_DT_ACCEL_ODR_2000;
	} else if (hz >= 1000) {
		return ICM4268X_DT_ACCEL_ODR_1000;
	} else if (hz >= 500) {
		return ICM4268X_DT_ACCEL_ODR_500;
	} else if (hz >= 200) {
		return ICM4268X_DT_ACCEL_ODR_200;
	} else if (hz >= 100) {
		return ICM4268X_DT_ACCEL_ODR_100;
	} else if (hz >= 50) {
		return ICM4268X_DT_ACCEL_ODR_50;
	} else if (hz >= 25) {
		return ICM4268X_DT_ACCEL_ODR_25;
	} else if (hz >= 12) {
		return ICM4268X_DT_ACCEL_ODR_12_5;
	} else if (hz >= 6) {
		return ICM4268X_DT_ACCEL_ODR_6_25;
	} else if (hz >= 3) {
		return ICM4268X_DT_ACCEL_ODR_3_125;
	} else {
		return ICM4268X_DT_ACCEL_ODR_1_5625;
	}
}

static inline void icm4268x_accel_reg_to_hz(uint8_t odr, struct sensor_value *out)
{
	switch (odr) {
	case ICM4268X_DT_ACCEL_ODR_32000:
		out->val1 = 32000;
		out->val2 = 0;
		return;
	case ICM4268X_DT_ACCEL_ODR_16000:
		out->val1 = 16000;
		out->val2 = 0;
		return;
	case ICM4268X_DT_ACCEL_ODR_8000:
		out->val1 = 8000;
		out->val2 = 0;
		return;
	case ICM4268X_DT_ACCEL_ODR_4000:
		out->val1 = 4000;
		out->val2 = 0;
		return;
	case ICM4268X_DT_ACCEL_ODR_2000:
		out->val1 = 2000;
		out->val2 = 0;
		return;
	case ICM4268X_DT_ACCEL_ODR_1000:
		out->val1 = 1000;
		out->val2 = 0;
		return;
	case ICM4268X_DT_ACCEL_ODR_500:
		out->val1 = 500;
		out->val2 = 0;
		return;
	case ICM4268X_DT_ACCEL_ODR_200:
		out->val1 = 200;
		out->val2 = 0;
		return;
	case ICM4268X_DT_ACCEL_ODR_100:
		out->val1 = 100;
		out->val2 = 0;
		return;
	case ICM4268X_DT_ACCEL_ODR_50:
		out->val1 = 50;
		out->val2 = 0;
		return;
	case ICM4268X_DT_ACCEL_ODR_25:
		out->val1 = 25;
		out->val2 = 0;
		return;
	case ICM4268X_DT_ACCEL_ODR_12_5:
		out->val1 = 12;
		out->val2 = 500000;
		return;
	case ICM4268X_DT_ACCEL_ODR_6_25:
		out->val1 = 6;
		out->val2 = 250000;
		return;
	case ICM4268X_DT_ACCEL_ODR_3_125:
		out->val1 = 3;
		out->val2 = 125000;
		return;
	case ICM4268X_DT_ACCEL_ODR_1_5625:
		out->val1 = 1;
		out->val2 = 562500;
		return;
	default:
		CODE_UNREACHABLE;
		return;
	}
}

static inline uint8_t icm4268x_gyro_odr_to_reg(uint16_t hz)
{
	if (hz >= 32000) {
		return ICM4268X_DT_GYRO_ODR_32000;
	} else if (hz >= 16000) {
		return ICM4268X_DT_GYRO_ODR_16000;
	} else if (hz >= 8000) {
		return ICM4268X_DT_GYRO_ODR_8000;
	} else if (hz >= 4000) {
		return ICM4268X_DT_GYRO_ODR_4000;
	} else if (hz >= 2000) {
		return ICM4268X_DT_GYRO_ODR_2000;
	} else if (hz >= 1000) {
		return ICM4268X_DT_GYRO_ODR_1000;
	} else if (hz >= 500) {
		return ICM4268X_DT_GYRO_ODR_500;
	} else if (hz >= 200) {
		return ICM4268X_DT_GYRO_ODR_200;
	} else if (hz >= 100) {
		return ICM4268X_DT_GYRO_ODR_100;
	} else if (hz >= 50) {
		return ICM4268X_DT_GYRO_ODR_50;
	} else if (hz >= 25) {
		return ICM4268X_DT_GYRO_ODR_25;
	} else {
		return ICM4268X_DT_GYRO_ODR_12_5;
	}
}

static inline void icm4268x_gyro_reg_to_odr(uint8_t odr, struct sensor_value *out)
{
	switch (odr) {
	case ICM4268X_DT_GYRO_ODR_32000:
		out->val1 = 32000;
		out->val2 = 0;
		return;
	case ICM4268X_DT_GYRO_ODR_16000:
		out->val1 = 16000;
		out->val2 = 0;
		return;
	case ICM4268X_DT_GYRO_ODR_8000:
		out->val1 = 8000;
		out->val2 = 0;
		return;
	case ICM4268X_DT_GYRO_ODR_4000:
		out->val1 = 4000;
		out->val2 = 0;
		return;
	case ICM4268X_DT_GYRO_ODR_2000:
		out->val1 = 2000;
		out->val2 = 0;
		return;
	case ICM4268X_DT_GYRO_ODR_1000:
		out->val1 = 1000;
		out->val2 = 0;
		return;
	case ICM4268X_DT_GYRO_ODR_500:
		out->val1 = 500;
		out->val2 = 0;
		return;
	case ICM4268X_DT_GYRO_ODR_200:
		out->val1 = 200;
		out->val2 = 0;
		return;
	case ICM4268X_DT_GYRO_ODR_100:
		out->val1 = 100;
		out->val2 = 0;
		return;
	case ICM4268X_DT_GYRO_ODR_50:
		out->val1 = 50;
		out->val2 = 0;
		return;
	case ICM4268X_DT_GYRO_ODR_25:
		out->val1 = 25;
		out->val2 = 0;
		return;
	case ICM4268X_DT_GYRO_ODR_12_5:
		out->val1 = 12;
		out->val2 = 500000;
		return;
	default:
		CODE_UNREACHABLE;
	}
}

/**
 * @brief All sensor configuration options
 */
struct icm4268x_cfg {
	enum icm4268x_variant variant;
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
	uint16_t fifo_wm;
	bool fifo_hires;
	/* TODO additional FIFO options */

	/* TODO interrupt options */
	bool interrupt1_drdy;
	bool interrupt1_fifo_ths;
	bool interrupt1_fifo_full;
	struct alignment axis_align[3];
	uint8_t pin9_function;
	uint16_t rtc_freq;
};

struct icm4268x_trigger_entry {
	struct sensor_trigger trigger;
	sensor_trigger_handler_t handler;
};

enum icm4268x_stream_state {
	ICM4268X_STREAM_OFF = 0,
	ICM4268X_STREAM_ON = 1,
	ICM4268X_STREAM_BUSY = 2,
};

/**
 * @brief Device data (struct device)
 */
struct icm4268x_dev_data {
	struct icm4268x_cfg cfg;
#ifdef CONFIG_ICM4268X_TRIGGER
#if defined(CONFIG_ICM4268X_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ICM4268X_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_ICM4268X_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif
#ifdef CONFIG_ICM4268X_STREAM
	struct rtio_iodev_sqe *streaming_sqe;
	struct icm4268x_bus bus;
	uint8_t int_status;
	uint16_t fifo_count;
	uint64_t timestamp;
	atomic_t state;
#endif /* CONFIG_ICM4268X_STREAM */
	const struct device *dev;
	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t data_ready_handler;
	const struct sensor_trigger *data_ready_trigger;
	struct k_mutex mutex;
#endif /* CONFIG_ICM4268X_TRIGGER */

	int16_t readings[7];
};

/**
 * @brief Device config (struct device)
 */
struct icm4268x_dev_cfg {
	struct spi_dt_spec spi;
	struct gpio_dt_spec gpio_int1;
	struct gpio_dt_spec gpio_int2;
};

/**
 * @brief Reset the sensor
 *
 * @param dev icm4268x device pointer
 *
 * @retval 0 success
 * @retval -EINVAL Reset status or whoami register returned unexpected value.
 */
int icm4268x_reset(const struct device *dev);

/**
 * @brief (Re)Configure the sensor with the given configuration
 *
 * @param dev icm4268x device pointer
 * @param cfg icm4268x_cfg pointer
 *
 * @retval 0 success
 * @retval -errno Error
 */
int icm4268x_configure(const struct device *dev, struct icm4268x_cfg *cfg);


/**
 * @brief Safely (re)Configure the sensor with the given configuration
 *
 * Will rollback to prior configuration if new configuration is invalid
 *
 * @param dev icm4268x device pointer
 * @param cfg icm4268x_cfg pointer
 *
 * @retval 0 success
 * @retval -errno Error
 */
int icm4268x_safely_configure(const struct device *dev, struct icm4268x_cfg *cfg);

/**
 * @brief Reads all channels
 *
 * Regardless of what is enabled/disabled this reads all data registers
 * as the time to read the 14 bytes at 1MHz is going to be 112 us which
 * is less time than a SPI transaction takes to setup typically.
 *
 * @param dev icm4268x device pointer
 * @param buf 14 byte buffer to store data values (7 channels, 2 bytes each)
 *
 * @retval 0 success
 * @retval -errno Error
 */
int icm4268x_read_all(const struct device *dev, uint8_t data[14]);

static const struct icm4268x_reg_val_pair table_accel_sensitivity_to_reg[][5] = {
	[ICM4268X_VARIANT_ICM42688] = {
		{.val = 2048, .reg = ICM42688_DT_ACCEL_FS_16},
		{.val = 4096, .reg = ICM42688_DT_ACCEL_FS_8},
		{.val = 8192, .reg = ICM42688_DT_ACCEL_FS_4},
		{.val = 16384, .reg = ICM42688_DT_ACCEL_FS_2},
	},
	[ICM4268X_VARIANT_ICM42686] = {
		{.val = 1024, .reg = ICM42686_DT_ACCEL_FS_32},
		{.val = 2048, .reg = ICM42686_DT_ACCEL_FS_16},
		{.val = 4096, .reg = ICM42686_DT_ACCEL_FS_8},
		{.val = 8192, .reg = ICM42686_DT_ACCEL_FS_4},
		{.val = 16384, .reg = ICM42686_DT_ACCEL_FS_2},
	},
};

/**
 * @brief Convert icm4268x accelerometer value to useful m/s^2 values
 *
 * @param cfg icm4268x_cfg current device configuration
 * @param in raw data value in int32_t format
 * @param out_ms meters/s^2 (whole) output in int32_t
 * @param out_ums micrometers/s^2 output as int32_t
 */
static inline void icm4268x_accel_ms(const struct icm4268x_cfg *cfg, int32_t in, int32_t *out_ms,
				     int32_t *out_ums)
{
	int64_t sensitivity = 0;

	for (uint8_t i = 0 ; i < table_accel_fs_to_reg_array_size[cfg->variant] ; i++) {
		if (cfg->accel_fs == table_accel_sensitivity_to_reg[cfg->variant][i].reg) {
			sensitivity = table_accel_sensitivity_to_reg[cfg->variant][i].val;
			break;
		}
	}

	if (sensitivity != 0) {
		/* Convert to micrometers/s^2 */
		int64_t in_ms = in * SENSOR_G;

		/* meters/s^2 whole values */
		*out_ms = in_ms / (sensitivity * 1000000LL);

		/* micrometers/s^2 */
		*out_ums = (in_ms - (*out_ms * sensitivity * 1000000LL)) / sensitivity;
	} else {
		CODE_UNREACHABLE;
	}
}

static const struct icm4268x_reg_val_pair table_gyro_sensitivity_to_reg[][8] = {
	[ICM4268X_VARIANT_ICM42688] = {
		{.val = 164, .reg = ICM42688_DT_GYRO_FS_2000},
		{.val = 328, .reg = ICM42688_DT_GYRO_FS_1000},
		{.val = 655,  .reg = ICM42688_DT_GYRO_FS_500},
		{.val = 1310,  .reg = ICM42688_DT_GYRO_FS_250},
		{.val = 2620,  .reg = ICM42688_DT_GYRO_FS_125},
		{.val = 5243,   .reg = ICM42688_DT_GYRO_FS_62_5},
		{.val = 10486,   .reg = ICM42688_DT_GYRO_FS_31_25},
		{.val = 20972,   .reg = ICM42688_DT_GYRO_FS_15_625},
	},
	[ICM4268X_VARIANT_ICM42686] = {
		{.val = 82, .reg = ICM42686_DT_GYRO_FS_4000},
		{.val = 164, .reg = ICM42686_DT_GYRO_FS_2000},
		{.val = 328, .reg = ICM42686_DT_GYRO_FS_1000},
		{.val = 655,  .reg = ICM42686_DT_GYRO_FS_500},
		{.val = 1310,  .reg = ICM42686_DT_GYRO_FS_250},
		{.val = 2620,  .reg = ICM42686_DT_GYRO_FS_125},
		{.val = 5243,   .reg = ICM42686_DT_GYRO_FS_62_5},
		{.val = 10486,   .reg = ICM42686_DT_GYRO_FS_31_25},
	},
};

/**
 * @brief Convert icm4268x gyroscope value to useful rad/s values
 *
 * @param cfg icm4268x_cfg current device configuration
 * @param in raw data value in int32_t format
 * @param out_rads whole rad/s output in int32_t
 * @param out_urads microrad/s as int32_t
 */
static inline void icm4268x_gyro_rads(const struct icm4268x_cfg *cfg, int32_t in, int32_t *out_rads,
				      int32_t *out_urads)
{
	int64_t sensitivity = 0;

	for (uint8_t i = 0 ; i < table_gyro_fs_to_reg_array_size[cfg->variant] ; i++) {
		if (cfg->gyro_fs == table_gyro_sensitivity_to_reg[cfg->variant][i].reg) {
			sensitivity = table_gyro_sensitivity_to_reg[cfg->variant][i].val;
			break;
		}
	}

	if (sensitivity != 0) {
		int64_t in10_rads = (int64_t)in * SENSOR_PI * 10LL;

		/* Whole rad/s */
		*out_rads = in10_rads / (sensitivity * 180LL * 1000000LL);

		/* microrad/s */
		*out_urads =
			(in10_rads - (*out_rads * sensitivity * 180LL * 1000000LL)) /
			(sensitivity * 180LL);
	} else {
		CODE_UNREACHABLE;
	}
}

/**
 * @brief Convert icm4268x temp value to useful celsius values
 *
 * @param cfg icm4268x_cfg current device configuration
 * @param in raw data value in int32_t format
 * @param out_c whole celsius output in int32_t
 * @param out_uc micro (1/1000000) celsius as int32_t
 */
static inline void icm4268x_temp_c(int32_t in, int32_t *out_c, int32_t *out_uc)
{
	int64_t sensitivity = 13248; /* value equivalent for x100 1c */

	/* Offset by 25 degrees Celsius */
	int64_t in100 = (in * 100) + (25 * sensitivity);

	/* Whole celsius */
	*out_c = in100 / sensitivity;

	/* Micro celsius */
	*out_uc = ((in100 - (*out_c) * sensitivity) * INT64_C(1000000)) / sensitivity;
}

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM4268X_H_ */

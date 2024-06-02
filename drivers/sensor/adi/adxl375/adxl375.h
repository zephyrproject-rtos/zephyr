/*
 * Copyright (c) 2023 Aaron Chan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ADXL375_ADXL375_H_
#define ZEPHYR_DRIVERS_SENSOR_ADXL375_ADXL375_H_

#include <zephyr/drivers/sensor.h>
#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */

/*
 * ADXL375 registers definition
 */
#define ADXL375_DEVID            0x00u /* Analog Devices accelerometer ID */
#define ADXL375_THRESH_SHOCK     0x1Du /* Shock Threshold */
#define ADXL375_OFFSET_X         0x1E  /* X-axis Offset */
#define ADXL375_OFFSET_Y         0x1F  /* Y-axis Offset */
#define ADXL375_OFFSET_Z         0x20  /* Z-axis Offset */
#define ADXL375_DUR_SHOCK        0x21  /* Shock Duration */
#define ADXL375_LATENT_SHOCK     0x22  /* Shock Latency */
#define ADXL375_WINDOW_SHOCK     0x23  /* Shock Window */
#define ADXL375_THRESH_ACT       0x24  /* Activity Threshold */
#define ADXL375_THRESH_INACT     0x25  /* Inctivity Threshold */
#define ADXL375_TIME_INACT       0x26  /* Inactivity Time */
#define ADXL375_ACT_INACT_CTL    0x27  /* Axis enable control for activity/inactivity detection */
#define ADXL375_SHOCK_AXES       0x2A  /* Axis control for single shock/double shock */
#define ADXL375_ACT_SHOCK_STATUS 0x2B  /* Source of single shock/double shock */
#define ADXL375_BW_RATE          0x2C  /* Data rate and power mode control */
#define ADXL375_POWER_CTL        0x2D  /* Power saving features control */
#define ADXL375_INT_ENABLE       0x2E  /* Interrupt enable control */
#define ADXL375_INT_MAP          0x2F  /* Interrupt mapping control */
#define ADXL375_INT_SOURCE       0x30  /* Interrupt source */
#define ADXL375_DATA_FORMAT      0x31  /* Data format control */
#define ADXL375_DATAX0           0x32  /* X-Axis Data 0 (LSB) */
#define ADXL375_DATAX1           0x33  /* X-Axis Data 1 (MSB) */
#define ADXL375_DATAY0           0x34  /* Y-Axis Data 0 (LSB) */
#define ADXL375_DATAY1           0x35  /* Y-Axis Data 1 (MSB) */
#define ADXL375_DATAZ0           0x36  /* Z-Axis Data 0 (LSB) */
#define ADXL375_DATAZ1           0x37  /* Z-Axis Data 1 (MSB) */
#define ADXL375_FIFO_CTL         0x38  /* FIFO control */
#define ADXL375_FIFO_STATUS      0x39  /* FIFO status */

#define ADXL375_DEVID_VAL 0xE5u /* Analog Devices accelerometer ID */

#define ADXL375_READ          0x01u
#define ADXL375_REG_READ(x)   (((x & 0xFF) << 1) | ADXL375_READ)
#define ADXL375_REG_WRITE(x)  ((x & 0xFF) << 1)
#define ADXL375_TO_I2C_REG(x) ((x) >> 1)

/* ADXL375_ACT_INACT_CTL */
#define ADXL375_POWER_CTL_ACT_ACDC_MSK   BIT(7)
#define ADXL375_POWER_CTL_ACT_X_EN_MSK   BIT(6)
#define ADXL375_POWER_CTL_ACT_Y_EN_MSK   BIT(5)
#define ADXL375_POWER_CTL_ACT_Z_EN_MSK   BIT(4)
#define ADXL375_POWER_CTL_INACT_ACDC_MSK BIT(3)
#define ADXL375_POWER_CTL_INACT_X_EN_MSK BIT(2)
#define ADXL375_POWER_CTL_INACT_Y_EN_MSK BIT(1)
#define ADXL375_POWER_CTL_INACT_Z_EN_MSK BIT(0)

#define ADXL375_POWER_CTL_ACT_ACDC_MODE(x)   (((x) & 0x1) << 7)
#define ADXL375_POWER_CTL_ACT_X_EN_MODE(x)   (((x) & 0x1) << 6)
#define ADXL375_POWER_CTL_ACT_Y_EN_MODE(x)   (((x) & 0x1) << 5)
#define ADXL375_POWER_CTL_ACT_Z_EN_MODE(x)   (((x) & 0x1) << 4)
#define ADXL375_POWER_CTL_INACT_ACDC_MODE(x) (((x) & 0x1) << 3)
#define ADXL375_POWER_CTL_INACT_X_EN_MODE(x) (((x) & 0x1) << 2)
#define ADXL375_POWER_CTL_INACT_Y_EN_MODE(x) (((x) & 0x1) << 1)
#define ADXL375_POWER_CTL_INACT_Z_EN_MODE(x) (((x) & 0x1) << 0)

/* ADXL375_SHOCK_AXES */
#define ADXL375_SHOCK_CTL_SUPPRESS_MSK   BIT(3)
#define ADXL375_SHOCK_CTL_SHOCK_X_EN_MSK BIT(2)
#define ADXL375_SHOCK_CTL_SHOCK_Y_EN_MSK BIT(1)
#define ADXL375_SHOCK_CTL_SHOCK_Z_EN_MSK BIT(0)

#define ADXL375_SHOCK_CTL_SUPPRESS_MODE(x)   (((x) & 0x1) << 3)
#define ADXL375_SHOCK_CTL_SHOCK_X_EN_MODE(x) (((x) & 0x1) << 2)
#define ADXL375_SHOCK_CTL_SHOCK_Y_EN_MODE(x) (((x) & 0x1) << 1)
#define ADXL375_SHOCK_CTL_SHOCK_Z_EN_MODE(x) (((x) & 0x1) << 0)

/* ADXL375_ACT_SHOCK_STATUS */
#define ADXL375_ACT_SHOCK_STATUS_ACT_X_SRC(x)   (((x) >> 6) & 0x1)
#define ADXL375_ACT_SHOCK_STATUS_ACT_Y_SRC(x)   (((x) >> 5) & 0x1)
#define ADXL375_ACT_SHOCK_STATUS_ACT_Z_SRC(x)   (((x) >> 4) & 0x1)
#define ADXL375_ACT_SHOCK_STATUS_ASLEEP(x)      (((x) >> 3) & 0x1)
#define ADXL375_ACT_SHOCK_STATUS_SHOCK_X_SRC(x) (((x) >> 2) & 0x1)
#define ADXL375_ACT_SHOCK_STATUS_SHOCK_Y_SRC(x) (((x) >> 1) & 0x1)
#define ADXL375_ACT_SHOCK_STATUS_SHOCK_Z_SRC(x) (((x) >> 0) & 0x1)

/* ADXL375_BW_RATE */
#define ADXL375_BW_RATE_LOW_POWER_MSK BIT(4)
#define ADXL375_BW_RATE_RATE_MSK      GENMASK(3, 0)

#define ADXL375_BW_RATE_LOW_POWER_MODE (((x) & 0x1) << 4)
#define ADXL375_BW_RATE_RATE_MODE      (((x) & 0x15) << 4)

/* ADXL375_POWER_CTL */
#define ADXL375_POWER_CTL_LINK_MSK       BIT(5)
#define ADXL375_POWER_CTL_AUTO_SLEEP_MSK BIT(4)
#define ADXL375_POWER_CTL_MEASURE_MSK    BIT(3)
#define ADXL375_POWER_CTL_SLEEP_MSK      BIT(2)
#define ADXL375_POWER_CTL_WAKEUP_MSK     GENMASK(1, 0)

#define ADXL375_POWER_CTL_LINK_MODE(x)       (((x) & 0x1) << 5)
#define ADXL375_POWER_CTL_AUTO_SLEEP_MODE(x) (((x) & 0x1) << 4)
#define ADXL375_POWER_CTL_MEASURE_MODE(x)    (((x) & 0x1) << 3)
#define ADXL375_POWER_CTL_SLEEP_MODE(x)      (((x) & 0x1) << 2)
#define ADXL375_POWER_CTL_WAKEUP_MODE(x)     (((x) & 0x3) << 0)

/* ADXL375_MEASURE */
#define ADXL375_MEASURE_AUTOSLEEP_MSK     BIT(6)
#define ADXL375_MEASURE_AUTOSLEEP_MODE(x) (((x) & 0x1) << 6)
#define ADXL375_MEASURE_LINKLOOP_MSK      GENMASK(5, 4)
#define ADXL375_MEASURE_LINKLOOP_MODE(x)  (((x) & 0x3) << 4)
#define ADXL375_MEASURE_LOW_NOISE_MSK     BIT(3)
#define ADXL375_MEASURE_LOW_NOISE_MODE(x) (((x) & 0x1) << 3)
#define ADXL375_MEASURE_BANDWIDTH_MSK     GENMASK(2, 0)
#define ADXL375_MEASURE_BANDWIDTH_MODE(x) (((x) & 0x7) << 0)

/* ADXL375_INT_ENABLE */
#define ADXL375_INT_ENABLE_DATA_READY_MSK   BIT(7)
#define ADXL375_INT_ENABLE_SINGLE_SHOCK_MSK BIT(6)
#define ADXL375_INT_ENABLE_DOUBLE_SHOCK_MSK BIT(5)
#define ADXL375_INT_ENABLE_ACTIVITY_MSK     BIT(4)
#define ADXL375_INT_ENABLE_INACTIVITY_MSK   BIT(3)
#define ADXL375_INT_ENABLE_WATERMARK_MSK    BIT(1)
#define ADXL375_INT_ENABLE_OVERRUN_MSK      BIT(0)

#define ADXL375_INT_ENABLE_DATA_READY_MODE(x)   (((x) & 0x1) << 7)
#define ADXL375_INT_ENABLE_SINGLE_SHOCK_MODE(x) (((x) & 0x1) << 6)
#define ADXL375_INT_ENABLE_DOUBLE_SHOCK_MODE(x) (((x) & 0x1) << 5)
#define ADXL375_INT_ENABLE_ACTIVITY_MODE(x)     (((x) & 0x1) << 4)
#define ADXL375_INT_ENABLE_INACTIVITY_MODE(x)   (((x) & 0x1) << 3)
#define ADXL375_INT_ENABLE_WATERMARK_MODE(x)    (((x) & 0x1) << 1)
#define ADXL375_INT_ENABLE_OVERRUN_MODE(x)      (((x) & 0x1) << 0)

/* ADXL375_INT_MAP */
#define ADXL375_INT_MAP_DATA_READY_MSK   BIT(7)
#define ADXL375_INT_MAP_SINGLE_SHOCK_MSK BIT(6)
#define ADXL375_INT_MAP_DOUBLE_SHOCK_MSK BIT(5)
#define ADXL375_INT_MAP_ACTIVITY_MSK     BIT(4)
#define ADXL375_INT_MAP_INACTIVITY_MSK   BIT(3)
#define ADXL375_INT_MAP_WATERMARK_MSK    BIT(1)
#define ADXL375_INT_MAP_OVERRUN_MSK      BIT(0)

#define ADXL375_INT_MAP_DATA_READY_MODE(x)   (((x) & 0x1) << 7)
#define ADXL375_INT_MAP_SINGLE_SHOCK_MODE(x) (((x) & 0x1) << 6)
#define ADXL375_INT_MAP_DOUBLE_SHOCK_MODE(x) (((x) & 0x1) << 5)
#define ADXL375_INT_MAP_ACTIVITY_MODE(x)     (((x) & 0x1) << 4)
#define ADXL375_INT_MAP_INACTIVITY_MODE(x)   (((x) & 0x1) << 3)
#define ADXL375_INT_MAP_WATERMARK_MODE(x)    (((x) & 0x1) << 1)
#define ADXL375_INT_MAP_OVERRUN_MODE(x)      (((x) & 0x1) << 0)

/* ADXL375_INT_SOURCE */
#define ADXL375_INT_DATA_READY_SRC(x)   (((x) >> 7) & 0x1)
#define ADXL375_INT_SINGLE_SHOCK_SRC(x) (((x) >> 6) & 0x1)
#define ADXL375_INT_DOUBLE_SHOCK_SRC(x) (((x) >> 5) & 0x1)
#define ADXL375_INT_ACTIVITY_SRC(x)     (((x) >> 4) & 0x1)
#define ADXL375_INT_INACTIVITY_SRC(x)   (((x) >> 3) & 0x1)
#define ADXL375_INT_WATERMARK_SRC(x)    (((x) >> 1) & 0x1)
#define ADXL375_INT_OVERRUN_SRC(x)      (((x) >> 0) & 0x1)

/* ADX375_DATA_FORMAT */
#define ADXL375_FORMAT_SELF_TEST_MSK  BIT(7)
#define ADXL375_FORMAT_SPI_MSK        BIT(6)
#define ADXL375_FORMAT_INT_INVERT_MSK BIT(5)
#define ADXL375_FORMAT_JUSTIFY_MSK    BIT(2)

#define ADXL375_FORMAT_SELF_TEST_MODE(x)  (((x) & 0x1) << 7)
#define ADXL375_FORMAT_SPI_MODE(x)        (((x) & 0x1) << 6)
#define ADXL375_FORMAT_INT_INVERT_MODE(x) (((x) & 0x1) << 5)
#define ADXL375_FORMAT_JUSTIFY_MODE(x)    (((x) & 0x1) << 2)

/* ADXL375_FIFO_CTL */
#define ADXL375_FIFO_CTL_FIFO_MODE_MSK GENMASK(7, 5)
#define ADXL375_FIFO_CTL_TRIGGER_MSK   BIT(5)
#define ADXL375_FIFO_CTL_SAMPLES_MSK   GENMASK(4, 0)

#define ADXL375_FIFO_CTL_FIFO_MODE_MODE(x) (((x) & 0x7) << 5)
#define ADXL375_FIFO_CTL_TRIGGER_MODE(x)   (((x) & 0x1) << 5)
#define ADXL375_FIFO_CTL_SAMPLES_MODE(x)   ((x) & 0x1F)

/* ADXL375_FIFO_STATUS */
#define ADXL375_FIFO_STATUS_FIFO_TRIG(x) (((x) >> 7) & 0x1)
#define ADXL375_FIFO_STATUS_ENTRIES(x)   ((x) & 0x7F)

/* ADXL375 scale factors specified in page 3, table 1 of datasheet */
#define ADXL375_MG2G_MULTIPLIER 0.049

/* Set bits 3, 1 and 0 since register defaults to 0. Datasheet pages 20-24 */
#define ADXL375_DATA_FORMAT_DEFAULT_BITS 0x0B

enum adxl375_axis {
	ADXL375_X_AXIS,
	ADXL375_Y_AXIS,
	ADXL375_Z_AXIS
};

enum adxl375_op_mode {
	ADXL375_STANDBY = 0x04,
	ADXL375_MEASUREMENT = 0x08,
	ADXL375_AUTOSLEEP = 0x24
};

enum adxl375_bandwidth {
	ADXL375_BW_0_05HZ,
	ADXL375_BW_0_10HZ,
	ADXL375_BW_0_20HZ,
	ADXL375_BW_0_39HZ,
	ADXL375_BW_0_78HZ,
	ADXL375_BW_1_56HZ,
	ADXL375_BW_3_13HZ,
	ADXL375_BW_6_25HZ,
	ADXL375_BW_12_5HZ,
	ADXL375_BW_25HZ,
	ADXL375_BW_50HZ,
	ADXL375_BW_100HZ,
	ADXL375_BW_200HZ,
	ADXL375_BW_400HZ,
	ADXL375_BW_800HZ,
	ADXL375_BW_1600HZ
};

enum adxl375_odr {
	ADXL375_ODR_0_10HZ = 0,
	ADXL375_ODR_0_20HZ,
	ADXL375_ODR_0_39HZ,
	ADXL375_ODR_0_78HZ,
	ADXL375_ODR_1_56HZ,
	ADXL375_ODR_3_13HZ,
	ADXL375_ODR_6_25HZ,
	ADXL375_ODR_12_5HZ,
	ADXL375_ODR_25HZ,
	ADXL375_ODR_50HZ,
	ADXL375_ODR_100HZ,
	ADXL375_ODR_200HZ,
	ADXL375_ODR_400HZ,
	ADXL375_ODR_800HZ,
	ADXL375_ODR_1600HZ,
	ADXL375_ODR_3200HZ
};

enum adxl375_fifo_format {
	ADXL375_XYZ_FIFO,
	ADXL375_X_FIFO,
	ADXL375_Y_FIFO,
	ADXL375_XY_FIFO,
	ADXL375_Z_FIFO,
	ADXL375_XZ_FIFO,
	ADXL375_YZ_FIFO,
	ADXL375_XYZ_PEAK_FIFO,
};

enum adxl375_fifo_mode {
	ADXL375_FIFO_BYPASS,
	ADXL375_FIFO_STREAM,
	ADXL375_FIFO_TRIGGER
};

struct adxl375_fifo_config {
	enum adxl375_fifo_mode fifo_mode;
	enum adxl375_fifo_format fifo_format;
	uint16_t fifo_samples;
};

struct adxl375_activity_threshold {
	uint16_t thresh;
	bool referenced;
	bool enable;
};

struct adxl375_xyz_accel_data {
	int16_t x;
	int16_t y;
	int16_t z;
};

struct adxl375_transfer_function {
	int (*read_reg_multiple)(const struct device *dev, uint8_t reg_addr, uint8_t *value,
				 uint16_t len);
	int (*write_reg)(const struct device *dev, uint8_t reg_addr, uint8_t value);
	int (*read_reg)(const struct device *dev, uint8_t reg_addr, uint8_t *value);
	int (*write_reg_mask)(const struct device *dev, uint8_t reg_addr, uint32_t mask,
			      uint8_t value);
};

struct adxl375_data {
	struct adxl375_xyz_accel_data sample;
	const struct adxl375_transfer_function *hw_tf;
	struct adxl375_fifo_config fifo_config;
};

struct adxl375_dev_config {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	struct i2c_dt_spec i2c;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	struct spi_dt_spec spi;
#endif
	int (*bus_init)(const struct device *dev);

	enum adxl375_odr odr;

	/* Device Settings */
	bool autosleep;

	bool lp;

	enum adxl375_op_mode op_mode;
};

int adxl375_spi_init(const struct device *dev);
int adxl375_i2c_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SENSOR_ADXL375_ADXL375_H_ */

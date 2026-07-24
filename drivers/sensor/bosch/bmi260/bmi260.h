/*
 * Copyright (c) 2021 Bosch Sensortec GmbH
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMI260_BMI260_H_
#define ZEPHYR_DRIVERS_SENSOR_BMI260_BMI260_H_

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#define BMI260_WR_LEN                           32
#define BMI260_CONFIG_FILE_RETRIES              15
#define BMI260_CONFIG_FILE_POLL_PERIOD_US       10000
#define BMI260_INTER_WRITE_DELAY_US             1000

#define BMI260_REG_CHIP_ID         0x00
#define BMI260_REG_ERROR           0x02
#define BMI260_REG_STATUS          0x03
#define BMI260_REG_AUX_X_LSB       0x04
#define BMI260_REG_ACC_X_LSB       0x0C
#define BMI260_REG_GYR_X_LSB       0x12
#define BMI260_REG_SENSORTIME_0    0x18
#define BMI260_REG_EVENT           0x1B
#define BMI260_REG_INT_STATUS_0    0x1C
#define BMI260_REG_SC_OUT_0        0x1E
#define BMI260_REG_WR_GEST_ACT     0x20
#define BMI260_REG_INTERNAL_STATUS 0x21
#define BMI260_REG_TEMPERATURE_0   0x22
#define BMI260_REG_FIFO_LENGTH_0   0x24
#define BMI260_REG_FIFO_DATA       0x26
#define BMI260_REG_FEAT_PAGE       0x2F
#define BMI260_REG_FEATURES_0      0x30
#define BMI260_REG_ACC_CONF        0x40
#define BMI260_REG_ACC_RANGE       0x41
#define BMI260_REG_GYR_CONF        0x42
#define BMI260_REG_GYR_RANGE       0x43
#define BMI260_REG_AUX_CONF        0x44
#define BMI260_REG_FIFO_DOWNS      0x45
#define BMI260_REG_FIFO_WTM_0      0x46
#define BMI260_REG_FIFO_CONFIG_0   0x48
#define BMI260_REG_SATURATION      0x4A
#define BMI260_REG_AUX_DEV_ID      0x4B
#define BMI260_REG_AUX_IF_CONF     0x4C
#define BMI260_REG_AUX_RD_ADDR     0x4D
#define BMI260_REG_AUX_WR_ADDR     0x4E
#define BMI260_REG_AUX_WR_DATA     0x4F
#define BMI260_REG_ERR_REG_MSK     0x52
#define BMI260_REG_INT1_IO_CTRL    0x53
#define BMI260_REG_INT2_IO_CTRL    0x54
#define BMI260_REG_INT_LATCH       0x55
#define BMI260_REG_INT1_MAP_FEAT   0x56
#define BMI260_REG_INT2_MAP_FEAT   0x57
#define BMI260_REG_INT_MAP_DATA    0x58
#define BMI260_REG_INIT_CTRL       0x59
#define BMI260_REG_INIT_ADDR_0     0x5B
#define BMI260_REG_INIT_DATA       0x5E
#define BMI260_REG_INTERNAL_ERROR  0x5F
#define BMI260_REG_AUX_IF_TRIM     0x68
#define BMI260_REG_GYR_CRT_CONF    0x69
#define BMI260_REG_NVM_CONF        0x6A
#define BMI260_REG_IF_CONF         0x6B
#define BMI260_REG_DRV             0x6C
#define BMI260_REG_ACC_SELF_TEST   0x6D
#define BMI260_REG_GYR_SELF_TEST   0x6E
#define BMI260_REG_NV_CONF         0x70
#define BMI260_REG_OFFSET_0        0x71
#define BMI260_REG_PWR_CONF        0x7C
#define BMI260_REG_PWR_CTRL        0x7D
#define BMI260_REG_CMD             0x7E
#define BMI260_REG_MASK            GENMASK(6, 0)

#define BMI260_ANYMO_1_DURATION_POS	0
#define BMI260_ANYMO_1_DURATION_MASK	BIT_MASK(12)
#define BMI260_ANYMO_1_DURATION(n)	((n) << BMI260_ANYMO_1_DURATION_POS)
#define BMI260_ANYMO_1_SELECT_X		BIT(13)
#define BMI260_ANYMO_1_SELECT_Y		BIT(14)
#define BMI260_ANYMO_1_SELECT_Z		BIT(15)
#define BMI260_ANYMO_1_SELECT_XYZ	(BMI260_ANYMO_1_SELECT_X | \
					 BMI260_ANYMO_1_SELECT_Y | \
					 BMI260_ANYMO_1_SELECT_Z)
#define BMI260_ANYMO_2_THRESHOLD_POS	0
#define BMI260_ANYMO_2_THRESHOLD_MASK	BIT_MASK(10)
#define BMI260_ANYMO_2_THRESHOLD(n)	((n) << BMI260_ANYMO_2_THRESHOLD_POS)
#define BMI260_ANYMO_2_OUT_CONF_POS	11
#define BMI260_ANYMO_2_OUT_CONF_MASK	(BIT(11) | BIT(12) | BIT(13) | BIT(14))
#define BMI260_ANYMO_2_ENABLE		BIT(15)
#define BMI260_ANYMO_2_OUT_CONF_OFF	(0x00 << BMI260_ANYMO_2_OUT_CONF_POS)
#define BMI260_ANYMO_2_OUT_CONF_BIT_0	(0x01 << BMI260_ANYMO_2_OUT_CONF_POS)
#define BMI260_ANYMO_2_OUT_CONF_BIT_1	(0x02 << BMI260_ANYMO_2_OUT_CONF_POS)
#define BMI260_ANYMO_2_OUT_CONF_BIT_2	(0x03 << BMI260_ANYMO_2_OUT_CONF_POS)
#define BMI260_ANYMO_2_OUT_CONF_BIT_3	(0x04 << BMI260_ANYMO_2_OUT_CONF_POS)
#define BMI260_ANYMO_2_OUT_CONF_BIT_4	(0x05 << BMI260_ANYMO_2_OUT_CONF_POS)
#define BMI260_ANYMO_2_OUT_CONF_BIT_5	(0x06 << BMI260_ANYMO_2_OUT_CONF_POS)
#define BMI260_ANYMO_2_OUT_CONF_BIT_6	(0x07 << BMI260_ANYMO_2_OUT_CONF_POS)
#define BMI260_ANYMO_2_OUT_CONF_BIT_8	(0x08 << BMI260_ANYMO_2_OUT_CONF_POS)

#define BMI260_INT_IO_CTRL_LVL		BIT(1) /* Output level (0 = active low, 1 = active high) */
#define BMI260_INT_IO_CTRL_OD		BIT(2) /* Open-drain (0 = push-pull, 1 = open-drain)*/
#define BMI260_INT_IO_CTRL_OUTPUT_EN	BIT(3) /* Output enabled */
#define BMI260_INT_IO_CTRL_INPUT_EN	BIT(4) /* Input enabled */

/* Applies to INT1_MAP_FEAT, INT2_MAP_FEAT, INT_STATUS_0 */
#define BMI260_INT_MAP_SIG_MOTION        BIT(0)
#define BMI260_INT_MAP_STEP_COUNTER      BIT(1)
#define BMI260_INT_MAP_ACTIVITY          BIT(2)
#define BMI260_INT_MAP_WRIST_WEAR_WAKEUP BIT(3)
#define BMI260_INT_MAP_WRIST_GESTURE     BIT(4)
#define BMI260_INT_MAP_NO_MOTION         BIT(5)
#define BMI260_INT_MAP_ANY_MOTION        BIT(6)

#define BMI260_INT_MAP_DATA_FFULL_INT1		BIT(0)
#define BMI260_INT_MAP_DATA_FWM_INT1		BIT(1)
#define BMI260_INT_MAP_DATA_DRDY_INT1		BIT(2)
#define BMI260_INT_MAP_DATA_ERR_INT1		BIT(3)
#define BMI260_INT_MAP_DATA_FFULL_INT2		BIT(4)
#define BMI260_INT_MAP_DATA_FWM_INT2		BIT(5)
#define BMI260_INT_MAP_DATA_DRDY_INT2		BIT(6)
#define BMI260_INT_MAP_DATA_ERR_INT2		BIT(7)

#define BMI260_INT_STATUS_ANY_MOTION		BIT(6)

#define BMI260_CHIP_ID 0x27

#define BMI260_CMD_G_TRIGGER  0x02
#define BMI260_CMD_USR_GAIN   0x03
#define BMI260_CMD_NVM_PROG   0xA0
#define BMI260_CMD_FIFO_FLUSH OxB0
#define BMI260_CMD_SOFT_RESET 0xB6

#define BMI260_POWER_ON_TIME                500
#define BMI260_SOFT_RESET_TIME              2000
#define BMI260_ACC_SUS_TO_NOR_START_UP_TIME 2000
#define BMI260_GYR_SUS_TO_NOR_START_UP_TIME 45000
#define BMI260_GYR_FAST_START_UP_TIME       2000
#define BMI260_TRANSC_DELAY_SUSPEND         450
#define BMI260_TRANSC_DELAY_NORMAL          2

#define BMI260_PREPARE_CONFIG_LOAD  0x00
#define BMI260_COMPLETE_CONFIG_LOAD 0x01

#define BMI260_INST_MESSAGE_MSK        0x0F
#define BMI260_INST_MESSAGE_NOT_INIT   0x00
#define BMI260_INST_MESSAGE_INIT_OK    0x01
#define BMI260_INST_MESSAGE_INIT_ERR   0x02
#define BMI260_INST_MESSAGE_DRV_ERR    0x03
#define BMI260_INST_MESSAGE_SNS_STOP   0x04
#define BMI260_INST_MESSAGE_NVM_ERR    0x05
#define BMI260_INST_MESSAGE_STRTUP_ERR 0x06
#define BMI260_INST_MESSAGE_COMPAT_ERR 0x07

#define BMI260_INST_AXES_REMAP_ERROR 0x20
#define BMI260_INST_ODR_50HZ_ERROR   0x40

#define BMI260_PWR_CONF_ADV_PWR_SAVE_MSK 0x01
#define BMI260_PWR_CONF_ADV_PWR_SAVE_EN  0x01
#define BMI260_PWR_CONF_ADV_PWR_SAVE_DIS 0x00

#define BMI260_PWR_CONF_FIFO_SELF_WKUP_MSK 0x02
#define BMI260_PWR_CONF_FIFO_SELF_WKUP_POS 0x01
#define BMI260_PWR_CONF_FIFO_SELF_WKUP_EN  0x01
#define BMI260_PWR_CONF_FIFO_SELF_WKUP_DIS 0x00

#define BMI260_PWR_CONF_FUP_EN_MSK 0x04
#define BMI260_PWR_CONF_FUP_EN_POS 0x02
#define BMI260_PWR_CONF_FUP_EN     0x01
#define BMI260_PWR_CONF_FUP_DIS    0x00

#define BMI260_PWR_CTRL_MSK     0x0F
#define BMI260_PWR_CTRL_AUX_EN  0x01
#define BMI260_PWR_CTRL_GYR_EN  0x02
#define BMI260_PWR_CTRL_ACC_EN  0x04
#define BMI260_PWR_CTRL_TEMP_EN 0x08

#define BMI260_ACC_ODR_MSK      0x0F
#define BMI260_ACC_ODR_25D32_HZ 0x01
#define BMI260_ACC_ODR_25D16_HZ 0x02
#define BMI260_ACC_ODR_25D8_HZ  0x03
#define BMI260_ACC_ODR_25D4_HZ  0x04
#define BMI260_ACC_ODR_25D2_HZ  0x05
#define BMI260_ACC_ODR_25_HZ    0x06
#define BMI260_ACC_ODR_50_HZ    0x07
#define BMI260_ACC_ODR_100_HZ   0x08
#define BMI260_ACC_ODR_200_HZ   0x09
#define BMI260_ACC_ODR_400_HZ   0x0A
#define BMI260_ACC_ODR_800_HZ   0x0B
#define BMI260_ACC_ODR_1600_HZ  0x0C

#define BMI260_ACC_BWP_MSK        0x30
#define BMI260_ACC_BWP_POS        4
#define BMI260_ACC_BWP_OSR4_AVG1  0x00
#define BMI260_ACC_BWP_OSR2_AVG2  0x01
#define BMI260_ACC_BWP_NORM_AVG4  0x02
#define BMI260_ACC_BWP_CIC_AVG8   0x03
#define BMI260_ACC_BWP_RES_AVG16  0x04
#define BMI260_ACC_BWP_RES_AVG32  0x05
#define BMI260_ACC_BWP_RES_AVG64  0x06
#define BMI260_ACC_BWP_RES_AVG128 0x07

#define BMI260_ACC_FILT_MSK      0x80
#define BMI260_ACC_FILT_POS      7
#define BMI260_ACC_FILT_PWR_OPT  0x00
#define BMI260_ACC_FILT_PERF_OPT 0x01

#define BMI260_ACC_RANGE_MSK 0x03
#define BMI260_ACC_RANGE_2G  0x00
#define BMI260_ACC_RANGE_4G  0x01
#define BMI260_ACC_RANGE_8G  0x02
#define BMI260_ACC_RANGE_16G 0x03

#define BMI260_GYR_ODR_MSK     0x0F
#define BMI260_GYR_ODR_25_HZ   0x06
#define BMI260_GYR_ODR_50_HZ   0x07
#define BMI260_GYR_ODR_100_HZ  0x08
#define BMI260_GYR_ODR_200_HZ  0x09
#define BMI260_GYR_ODR_400_HZ  0x0A
#define BMI260_GYR_ODR_800_HZ  0x0B
#define BMI260_GYR_ODR_1600_HZ 0x0C
#define BMI260_GYR_ODR_3200_HZ 0x0D

#define BMI260_GYR_BWP_MSK  0x30
#define BMI260_GYR_BWP_POS  4
#define BMI260_GYR_BWP_OSR4 0x00
#define BMI260_GYR_BWP_OSR2 0x01
#define BMI260_GYR_BWP_NORM 0x02

#define BMI260_GYR_FILT_NOISE_MSK      0x40
#define BMI260_GYR_FILT_NOISE_POS      6
#define BMI260_GYR_FILT_NOISE_PWR      0x00
#define BMI260_GYR_FILT_NOISE_PERF     0x01

#define BMI260_GYR_FILT_MSK      0x80
#define BMI260_GYR_FILT_POS      7
#define BMI260_GYR_FILT_PWR_OPT  0x00
#define BMI260_GYR_FILT_PERF_OPT 0x01

#define BMI260_GYR_RANGE_MSK     0x07
#define BMI260_GYR_RANGE_2000DPS 0x00
#define BMI260_GYR_RANGE_1000DPS 0x01
#define BMI260_GYR_RANGE_500DPS  0x02
#define BMI260_GYR_RANGE_250DPS  0x03
#define BMI260_GYR_RANGE_125DPS  0x04

#define BMI260_GYR_OIS_RANGE_MSK     0x80
#define BMI260_GYR_OIS_RANGE_POS     3
#define BMI260_GYR_OIS_RANGE_250DPS  0x00
#define BMI260_GYR_OIS_RANGE_2000DPS 0x01

#define BMI260_SET_BITS(reg_data, bitname, data)		  \
	((reg_data & ~(bitname##_MSK)) | ((data << bitname##_POS) \
					  & bitname##_MSK))
#define BMI260_SET_BITS_POS_0(reg_data, bitname, data) \
	((reg_data & ~(bitname##_MSK)) | (data & bitname##_MSK))

struct bmi260_data {
	int16_t ax, ay, az, gx, gy, gz;
	uint8_t acc_range, acc_odr, gyr_odr;
	uint16_t gyr_range;

#if CONFIG_BMI260_TRIGGER
	const struct device *dev;
	struct k_mutex trigger_mutex;
	sensor_trigger_handler_t motion_handler;
	const struct sensor_trigger *motion_trigger;
	sensor_trigger_handler_t drdy_handler;
	const struct sensor_trigger *drdy_trigger;
	struct gpio_callback int1_cb;
	struct gpio_callback int2_cb;
	atomic_t int_flags;
	uint16_t anymo_1;
	uint16_t anymo_2;

#if CONFIG_BMI260_TRIGGER_OWN_THREAD
	struct k_sem trig_sem;

	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_BMI260_THREAD_STACK_SIZE);
	struct k_thread thread;

#elif CONFIG_BMI260_TRIGGER_GLOBAL_THREAD
	struct k_work trig_work;
#endif
#endif /* CONFIG_BMI260_TRIGGER */
};

struct bmi260_feature_reg {
	/* Which feature page the register resides in */
	uint8_t page;
	uint8_t addr;
};

struct bmi260_feature_config {
	const char *name;
	const uint8_t *config_file;
	size_t config_file_len;
	struct bmi260_feature_reg *anymo_1;
	struct bmi260_feature_reg *anymo_2;
};

union bmi260_bus {
#if CONFIG_BMI260_BUS_SPI
	struct spi_dt_spec spi;
#endif
#if CONFIG_BMI260_BUS_I2C
	struct i2c_dt_spec i2c;
#endif
};

typedef int (*bmi260_bus_check_fn)(const union bmi260_bus *bus);
typedef int (*bmi260_bus_init_fn)(const union bmi260_bus *bus);
typedef int (*bmi260_reg_read_fn)(const union bmi260_bus *bus,
				  uint8_t start,
				  uint8_t *data,
				  uint16_t len);
typedef int (*bmi260_reg_write_fn)(const union bmi260_bus *bus,
				   uint8_t start,
				   const uint8_t *data,
				   uint16_t len);

struct bmi260_bus_io {
	bmi260_bus_check_fn check;
	bmi260_reg_read_fn read;
	bmi260_reg_write_fn write;
	bmi260_bus_init_fn init;
};

struct bmi260_config {
	union bmi260_bus bus;
	const struct bmi260_bus_io *bus_io;
	const struct bmi260_feature_config *feature;
#if CONFIG_BMI260_TRIGGER
	struct gpio_dt_spec int1;
	struct gpio_dt_spec int2;
#endif
};

#if CONFIG_BMI260_BUS_SPI
#define BMI260_SPI_OPERATION (SPI_WORD_SET(8) | SPI_TRANSFER_MSB)
#define BMI260_SPI_ACC_DELAY_US 2
extern const struct bmi260_bus_io bmi260_bus_io_spi;
#endif

#if CONFIG_BMI260_BUS_I2C
extern const struct bmi260_bus_io bmi260_bus_io_i2c;
#endif

int bmi260_reg_read(const struct device *dev, uint8_t reg, uint8_t *data, uint16_t length);

int bmi260_reg_write(const struct device *dev, uint8_t reg,
		     const uint8_t *data, uint16_t length);

int bmi260_reg_write_with_delay(const struct device *dev,
				uint8_t reg,
				const uint8_t *data,
				uint16_t length,
				uint32_t delay_us);

#ifdef CONFIG_BMI260_TRIGGER
int bmi260_trigger_set(const struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler);

int bmi260_init_interrupts(const struct device *dev);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_BMI260_BMI260_H_ */

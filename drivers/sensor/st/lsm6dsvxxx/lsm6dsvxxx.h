/* ST Microelectronics LSM6DSVXXX family IMU sensor
 *
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lsm6dsv320x.pdf
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LSM6DSVXXX_LSM6DSVXXX_H_
#define ZEPHYR_DRIVERS_SENSOR_LSM6DSVXXX_LSM6DSVXXX_H_

#include <stdint.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/sys/util.h>
#include <stmemsc.h>
#include <zephyr/rtio/regmap.h>
#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/pm/device.h>

#define LSM6DSVXXX_ANY_INST_ON_BUS_STATUS_OKAY(bus) \
	(DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(st_lsm6dsv320x, bus) ||\
	 DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(st_lsm6dsv80x, bus)  ||\
	 DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(st_ism6hg256x, bus))

#if DT_HAS_COMPAT_STATUS_OKAY(st_lsm6dsv320x)
#include "lsm6dsv320x_reg.h"
#include <zephyr/dt-bindings/sensor/lsm6dsv320x.h>
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(st_lsm6dsv80x)
#include "lsm6dsv80x_reg.h"
#include <zephyr/dt-bindings/sensor/lsm6dsv80x.h>
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(st_ism6hg256x)
#include "ism6hg256x_reg.h"
#include <zephyr/dt-bindings/sensor/ism6hg256x.h>
#endif

#if LSM6DSVXXX_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif /* LSM6DSVXXX_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

#if LSM6DSVXXX_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif /* LSM6DSVXXX_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */

#if LSM6DSVXXX_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
#include <zephyr/drivers/i3c.h>
#endif /* LSM6DSVXXX_ANY_INST_ON_BUS_STATUS_OKAY(i3c) */

#if (LSM6DSVXXX_ANY_INST_ON_BUS_STATUS_OKAY(i3c))
#define ON_I3C_BUS(cfg) (cfg->i3c.bus != NULL)
#define I3C_INT_PIN(cfg) (cfg->int_en_i3c)
#else
#define ON_I3C_BUS(cfg) (false)
#define I3C_INT_PIN(cfg) (false)
#endif

struct trigger_config {
	uint8_t int_fifo_th : 1;
	uint8_t int_fifo_full : 1;
	uint8_t int_drdy : 1;
};

typedef int32_t (*api_lsm6dsvxxx_init_chip)(const struct device *dev);
#if defined(CONFIG_LSM6DSVXXX_TRIGGER)
typedef int32_t (*api_lsm6dsvxxx_drdy_mode_set)(const struct device *dev);
#endif /* CONFIG_LSM6DSVXXX_TRIGGER */
#if defined(CONFIG_PM_DEVICE)
typedef int32_t (*api_lsm6dsvxxx_pm_action)(const struct device *dev, enum pm_device_action action);
#endif /* CONFIG_PM_DEVICE */
typedef int32_t (*api_lsm6dsvxxx_accel_set_fs)(const struct device *dev, int32_t range);
typedef int32_t (*api_lsm6dsvxxx_accel_set_odr)(const struct device *dev, int32_t freq);
typedef int32_t (*api_lsm6dsvxxx_accel_set_mode)(const struct device *dev, int32_t mode);
typedef int32_t (*api_lsm6dsvxxx_accel_get_fs)(const struct device *dev, int32_t *range);
typedef int32_t (*api_lsm6dsvxxx_accel_get_odr)(const struct device *dev, int32_t *freq);
typedef int32_t (*api_lsm6dsvxxx_accel_get_mode)(const struct device *dev, int32_t *mode);
typedef int32_t (*api_lsm6dsvxxx_gyro_set_fs)(const struct device *dev, int32_t range);
typedef int32_t (*api_lsm6dsvxxx_gyro_set_odr)(const struct device *dev, int32_t freq);
typedef int32_t (*api_lsm6dsvxxx_gyro_set_mode)(const struct device *dev, int32_t mode);
typedef int32_t (*api_lsm6dsvxxx_gyro_get_fs)(const struct device *dev, int32_t *range);
typedef int32_t (*api_lsm6dsvxxx_gyro_get_odr)(const struct device *dev, int32_t *freq);
typedef int32_t (*api_lsm6dsvxxx_gyro_get_mode)(const struct device *dev, int32_t *mode);
#if defined(CONFIG_LSM6DSVXXX_STREAM)
typedef void (*api_lsm6dsvxxx_config_fifo)(const struct device *dev,
					   struct trigger_config trig_cfg);
typedef void (*api_lsm6dsvxxx_config_drdy)(const struct device *dev,
					   struct trigger_config trig_cfg);
typedef uint32_t (*api_lsm6dsvxxx_from_f16_to_f32)(uint16_t val);
typedef float (*api_lsm6dsvxxx_from_sflp_to_mg)(int16_t lsb);
#endif /* CONFIG_LSM6DSVXXX_STREAM */

struct lsm6dsvxxx_chip_api {
	api_lsm6dsvxxx_init_chip init_chip;
#if defined(CONFIG_LSM6DSVXXX_TRIGGER)
	api_lsm6dsvxxx_drdy_mode_set drdy_mode_set;
#endif /* CONFIG_LSM6DSVXXX_TRIGGER */
#if defined(CONFIG_PM_DEVICE)
	api_lsm6dsvxxx_pm_action pm_action;
#endif /* CONFIG_PM_DEVICE */
	api_lsm6dsvxxx_accel_set_fs accel_fs_set;
	api_lsm6dsvxxx_accel_set_odr accel_odr_set;
	api_lsm6dsvxxx_accel_set_mode accel_mode_set;
	api_lsm6dsvxxx_accel_get_fs accel_fs_get;
	api_lsm6dsvxxx_accel_get_odr accel_odr_get;
	api_lsm6dsvxxx_accel_get_mode accel_mode_get;
	api_lsm6dsvxxx_gyro_set_fs gyro_fs_set;
	api_lsm6dsvxxx_gyro_set_odr gyro_odr_set;
	api_lsm6dsvxxx_gyro_set_mode gyro_mode_set;
	api_lsm6dsvxxx_gyro_get_fs gyro_fs_get;
	api_lsm6dsvxxx_gyro_get_odr gyro_odr_get;
	api_lsm6dsvxxx_gyro_get_mode gyro_mode_get;
#if defined(CONFIG_LSM6DSVXXX_STREAM)
	api_lsm6dsvxxx_config_fifo config_fifo;
	api_lsm6dsvxxx_config_drdy config_drdy;
	api_lsm6dsvxxx_from_f16_to_f32 from_f16_to_f32;
	api_lsm6dsvxxx_from_sflp_to_mg from_sflp_to_mg;
#endif /* CONFIG_LSM6DSVXXX_STREAM */
};

struct lsm6dsvxxx_config {
	stmdev_ctx_t ctx;
	union {
#if LSM6DSVXXX_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
		const struct i2c_dt_spec i2c;
#endif
#if LSM6DSVXXX_ANY_INST_ON_BUS_STATUS_OKAY(spi)
		const struct spi_dt_spec spi;
#endif
#if LSM6DSVXXX_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
		struct i3c_device_desc **i3c;
#endif
	} stmemsc_cfg;
	uint8_t accel_pm;
	uint8_t accel_odr;
	uint8_t accel_hg_odr;
	uint8_t accel_range;
	uint8_t gyro_pm;
	uint8_t gyro_odr;
	uint8_t gyro_range;
	uint8_t drdy_pulsed;
	const int8_t *accel_bit_shift;
	const int32_t *accel_scaler;

#ifdef CONFIG_LSM6DSVXXX_STREAM
	uint8_t fifo_wtm;
	uint8_t accel_batch : 4;
	uint8_t gyro_batch : 4;
	uint8_t temp_batch : 2;
	uint8_t sflp_odr : 3;
	uint8_t sflp_fifo_en : 3;
#endif
#ifdef CONFIG_LSM6DSVXXX_TRIGGER
	const struct gpio_dt_spec int1_gpio;
	const struct gpio_dt_spec int2_gpio;
	uint8_t drdy_pin;
	bool trig_enabled;
#if LSM6DSVXXX_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
	bool int_en_i3c;
	lsm6dsvxxx_i3c_ibi_time_t bus_act_sel;
#endif /* LSM6DSVXXX_ANY_INST_ON_BUS_STATUS_OKAY(i3c) */
#endif /* CONFIG_LSM6DSVXXX_TRIGGER */

#if LSM6DSVXXX_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
	struct {
		const struct device *bus;
		const struct i3c_device_id dev_id;
	} i3c;
#endif
	const struct lsm6dsvxxx_chip_api *chip_api;
};

struct lsm6dsvxxx_ibi_payload {
	uint8_t mdb;
	uint8_t fifo_status1;
	uint8_t fifo_status2;
	uint8_t all_int_src;
	uint8_t status_reg;
	uint8_t status_reg_ois;
	uint8_t status_master_main;
	uint8_t emb_func_status;
	uint8_t fsm_status;
	uint8_t mlc_status;
} __packed;

struct lsm6dsvxxx_data {
	const struct device *dev;
	int16_t acc[3];
	uint32_t acc_gain;
	int16_t gyro[3];
	uint32_t gyro_gain;
#if defined(CONFIG_LSM6DSVXXX_ENABLE_TEMP)
	int16_t temp_sample;
#endif

	uint8_t accel_freq;
	uint8_t accel_fs;
	uint8_t gyro_freq;
	uint8_t gyro_fs;
	uint8_t out_xl;
	uint8_t out_tp;

	struct rtio_iodev_sqe *streaming_sqe;
	struct rtio *rtio_ctx;
	struct rtio_iodev *iodev;

	rtio_bus_type bus_type; /* I2C is 0, SPI is 1, I3C is 2 */

#ifdef CONFIG_LSM6DSVXXX_STREAM
	uint64_t timestamp;
	uint8_t status;
	uint8_t fifo_status[2];
	uint16_t fifo_count;
	struct trigger_config trig_cfg;
	uint8_t accel_batch_odr : 4;
	uint8_t gyro_batch_odr : 4;
	uint8_t temp_batch_odr : 2;
	uint8_t sflp_batch_odr : 3;
	uint8_t reserved : 3;
	int32_t gbias_x_udps;
	int32_t gbias_y_udps;
	int32_t gbias_z_udps;
#endif
#ifdef CONFIG_LSM6DSVXXX_TRIGGER
	const struct gpio_dt_spec *drdy_gpio;
	struct gpio_callback gpio_cb;
#endif /* CONFIG_LSM6DSVXXX_TRIGGER */

#if LSM6DSVXXX_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
	struct i3c_device_desc *i3c_dev;
	struct lsm6dsvxxx_ibi_payload ibi_payload;
#endif
};

static inline uint8_t lsm6dsvxxx_bus_reg(rtio_bus_type bus, uint8_t addr)
{
	return (rtio_is_spi(bus)) ? addr | 0x80 : addr;
}

int lsm6dsvxxx_spi_init(const struct device *dev);

#define LSM6DSVXXX_ACCEL_FS_VAL_TO_FS_IDX(x) (__builtin_clz(x) - 1)

/* decoder */
struct lsm6dsvxxx_decoder_header {
	const struct lsm6dsvxxx_config *cfg;
	uint64_t timestamp;
	uint8_t is_fifo: 1;
	uint8_t accel_fs: 4;
	uint8_t gyro_fs: 3;
	uint8_t int_status;
} __attribute__((__packed__));

struct lsm6dsvxxx_fifo_data {
	struct lsm6dsvxxx_decoder_header header;
	uint8_t gyro_odr: 4;
	uint8_t accel_odr: 4;
	uint16_t fifo_count: 11;
	uint16_t reserved_1: 5;
	uint16_t gyro_batch_odr: 4;
	uint16_t accel_batch_odr: 4;
	uint16_t temp_batch_odr: 4;
	uint16_t sflp_batch_odr: 3;
	uint16_t reserved_2: 1;
} __attribute__((__packed__));

struct lsm6dsvxxx_rtio_data {
	struct lsm6dsvxxx_decoder_header header;
	struct {
		uint8_t has_accel: 1; /* set if accel channel has data */
		uint8_t has_temp: 1;  /* set if temp channel has data */
		uint8_t reserved: 6;
	}  __attribute__((__packed__));
	int16_t accel[3];
	int16_t temp;
};

/* Calculate scaling factor to transform micro-g/LSB unit into micro-ms2/LSB */
#define SENSOR_SCALE_UG_TO_UMS2(ug_lsb)	\
	(int32_t)((ug_lsb) * SENSOR_G / 1000000LL)

/* Calculate scaling factor to transform micro-dps/LSB unit into micro-rads/LSB */
#define SENSOR_SCALE_UDPS_TO_URADS(udps_lsb) \
	(int32_t)(((udps_lsb) * SENSOR_PI / 180LL) / 1000000LL)

#ifdef CONFIG_LSM6DSVXXX_STREAM
#define LSM6DSVXXX_FIFO_ITEM_LEN 7
#define LSM6DSVXXX_FIFO_SIZE(x) (x * LSM6DSVXXX_FIFO_ITEM_LEN)
#endif

int lsm6dsvxxx_encode(const struct device *dev, const struct sensor_chan_spec *const channels,
		      const size_t num_channels, uint8_t *buf);

int lsm6dsvxxx_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder);

void lsm6dsvxxx_rtio_rd_transaction(const struct device *dev,
				 uint8_t *regs, uint8_t regs_num,
				 struct spi_buf *buf,
				 struct rtio_iodev_sqe *iodev_sqe,
				 rtio_callback_t complete_op_cb);
#ifdef CONFIG_LSM6DSVXXX_TRIGGER
int lsm6dsvxxx_init_interrupt(const struct device *dev);
#endif /* CONFIG_LSM6DSVXXX_TRIGGER */

#endif /* ZEPHYR_DRIVERS_SENSOR_LSM6DSVXXX_LSM6DSVXXX_H_ */

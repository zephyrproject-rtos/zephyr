/*
 * Copyright (c) 2020 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ADX345_ADX345_H_
#define ZEPHYR_DRIVERS_SENSOR_ADX345_ADX345_H_

#include <zephyr/drivers/sensor.h>
#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#ifdef CONFIG_ADXL345_STREAM
#include <zephyr/rtio/rtio.h>
#endif /* CONFIG_ADXL345_STREAM */

#define DT_DRV_COMPAT adi_adxl345

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif
#include <zephyr/sys/util.h>

/* ADXL345 communication commands */
#define ADXL345_WRITE_CMD          0x00
#define ADXL345_READ_CMD           0x80
#define ADXL345_MULTIBYTE_FLAG     0x40

#define ADXL345_REG_READ(x)	((x & 0xFF) | ADXL345_READ_CMD)

#define SAMPLE_SIZE 6
#define SAMPLE_MASK 0x3F
#define SAMPLE_NUM  0x1F

/* Registers */
#define ADXL345_DEVICE_ID_REG           0x00
#define ADXL345_RATE_REG                0x2c
#define ADXL345_POWER_CTL_REG           0x2d
#define ADXL345_DATA_FORMAT_REG         0x31
#define ADXL345_DATA_FORMAT_FULL_RES    0x08
#define ADXL345_X_AXIS_DATA_0_REG       0x32
#define ADXL345_FIFO_CTL_REG            0x38
#define ADXL345_FIFO_STATUS_REG         0x39

#define ADXL345_PART_ID            0xe5

#define ADXL345_RANGE_2G           0x0
#define ADXL345_RANGE_4G           0x1
#define ADXL345_RANGE_8G           0x2
#define ADXL345_RANGE_16G          0x3
#define ADXL345_RATE_25HZ          0x8
#define ADXL345_ENABLE_MEASURE_BIT (1 << 3)
#define ADXL345_FIFO_STREAM_MODE   (1 << 7)
#define ADXL345_FIFO_COUNT_MASK    0x3f
#define ADXL345_COMPLEMENT_MASK(x) GENMASK(15, (x))
#define ADXL345_COMPLEMENT         0xfc00

#define ADXL345_MAX_FIFO_SIZE      32

#define ADXL345_INT_ENABLE 0x2Eu
#define ADXL345_INT_MAP    0x2Fu
#define ADXL345_INT_SOURCE 0x30u

/* ADXL345_STATUS_1 */
#define ADXL345_STATUS_DOUBLE_TAP(x) (((x) >> 5) & 0x1)
#define ADXL345_STATUS_SINGLE_TAP(x) (((x) >> 6) & 0x1)
#define ADXL345_STATUS_DATA_RDY(x)   (((x) >> 7) & 0x1)

/* ADXL345_INT_MAP */
#define ADXL345_INT_MAP_OVERRUN_MSK        BIT(0)
#define ADXL345_INT_MAP_OVERRUN_MODE(x)    (((x) & 0x1) << 0)
#define ADXL345_INT_MAP_WATERMARK_MSK      BIT(1)
#define ADXL345_INT_MAP_WATERMARK_MODE(x)  (((x) & 0x1) << 1)
#define ADXL345_INT_MAP_FREE_FALL_MSK      BIT(2)
#define ADXL345_INT_MAP_FREE_FALL_MODE(x)  (((x) & 0x1) << 2)
#define ADXL345_INT_MAP_INACT_MSK          BIT(3)
#define ADXL345_INT_MAP_INACT_MODE(x)      (((x) & 0x1) << 3)
#define ADXL345_INT_MAP_ACT_MSK            BIT(4)
#define ADXL345_INT_MAP_ACT_MODE(x)        (((x) & 0x1) << 4)
#define ADXL345_INT_MAP_DOUBLE_TAP_MSK     BIT(5)
#define ADXL345_INT_MAP_DOUBLE_TAP_MODE(x) (((x) & 0x1) << 5)
#define ADXL345_INT_MAP_SINGLE_TAP_MSK     BIT(6)
#define ADXL345_INT_MAP_SINGLE_TAP_MODE(x) (((x) & 0x1) << 6)
#define ADXL345_INT_MAP_DATA_RDY_MSK       BIT(7)
#define ADXL345_INT_MAP_DATA_RDY_MODE(x)   (((x) & 0x1) << 7)

/* POWER_CTL */
#define ADXL345_POWER_CTL_WAKEUP_4HZ         BIT(0)
#define ADXL345_POWER_CTL_WAKEUP_4HZ_MODE(x) (((x) & 0x1) << 0)
#define ADXL345_POWER_CTL_WAKEUP_2HZ         BIT(1)
#define ADXL345_POWER_CTL_WAKEUP_2HZ_MODE(x) (((x) & 0x1) << 1)
#define ADXL345_POWER_CTL_SLEEP              BIT(2)
#define ADXL345_POWER_CTL_SLEEP_MODE(x)      (((x) & 0x1) << 2)
#define ADXL345_POWER_CTL_MEASURE_MSK        GENMASK(3, 3)
#define ADXL345_POWER_CTL_MEASURE_MODE(x)    (((x) & 0x1) << 3)
#define ADXL345_POWER_CTL_STANDBY_MODE(x)    (((x) & 0x0) << 3)

/* ADXL345_FIFO_CTL */
#define ADXL345_FIFO_CTL_MODE_MSK        GENMASK(7, 6)
#define ADXL345_FIFO_CTL_MODE_MODE(x)    (((x) & 0x3) << 6)
#define ADXL345_FIFO_CTL_TRIGGER_MSK     BIT(5)
#define ADXL345_FIFO_CTL_TRIGGER_MODE(x) (((x) & 0x1) << 5)
#define ADXL345_FIFO_CTL_SAMPLES_MSK     BIT(0)
#define ADXL345_FIFO_CTL_SAMPLES_MODE(x) ((x) & 0x1F)

#define ADXL345_ODR_MSK     GENMASK(3, 0)
#define ADXL345_ODR_MODE(x) ((x) & 0xF)

enum adxl345_odr {
	ADXL345_ODR_12HZ = 0x7,
	ADXL345_ODR_25HZ,
	ADXL345_ODR_50HZ,
	ADXL345_ODR_100HZ,
	ADXL345_ODR_200HZ,
	ADXL345_ODR_400HZ
};

enum adxl345_fifo_trigger {
	ADXL345_INT1,
	ADXL345_INT2
};

enum adxl345_fifo_mode {
	ADXL345_FIFO_BYPASSED,
	ADXL345_FIFO_OLD_SAVED,
	ADXL345_FIFO_STREAMED,
	ADXL345_FIFO_TRIGGERED
};

struct adxl345_fifo_config {
	enum adxl345_fifo_mode fifo_mode;
	enum adxl345_fifo_trigger fifo_trigger;
	uint16_t fifo_samples;
};

enum adxl345_op_mode {
	ADXL345_STANDBY,
	ADXL345_MEASURE
};

struct adxl345_dev_data {
	unsigned int sample_number;
	int16_t bufx[ADXL345_MAX_FIFO_SIZE];
	int16_t bufy[ADXL345_MAX_FIFO_SIZE];
	int16_t bufz[ADXL345_MAX_FIFO_SIZE];
	struct adxl345_fifo_config fifo_config;
	uint8_t is_full_res;
	uint8_t selected_range;
#ifdef CONFIG_ADXL345_TRIGGER
	struct gpio_callback gpio_cb;

	sensor_trigger_handler_t th_handler;
	const struct sensor_trigger *th_trigger;
	sensor_trigger_handler_t drdy_handler;
	const struct sensor_trigger *drdy_trigger;
	const struct device *dev;

#if defined(CONFIG_ADXL345_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ADXL345_THREAD_STACK_SIZE);
	struct k_sem gpio_sem;
	struct k_thread thread;
#elif defined(CONFIG_ADXL345_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif
#endif /* CONFIG_ADXL345_TRIGGER */
#ifdef CONFIG_ADXL345_STREAM
	struct rtio_iodev_sqe *sqe;
	struct rtio *rtio_ctx;
	struct rtio_iodev *iodev;
	uint8_t status1;
	uint8_t fifo_ent[1];
	uint64_t timestamp;
	struct rtio *r_cb;
	uint8_t fifo_watermark_irq;
	uint8_t fifo_samples;
	uint16_t fifo_total_bytes;
#endif /* CONFIG_ADXL345_STREAM */
};

struct adxl345_fifo_data {
	uint8_t is_fifo: 1;
	uint8_t is_full_res: 1;
	uint8_t selected_range: 2;
	uint8_t sample_set_size: 4;
	uint8_t int_status;
	uint16_t accel_odr: 4;
	uint16_t fifo_byte_count: 12;
	uint64_t timestamp;
} __attribute__((__packed__));

struct adxl345_sample {
#ifdef CONFIG_ADXL345_STREAM
	uint8_t is_fifo: 1;
	uint8_t res: 7;
#endif /* CONFIG_ADXL345_STREAM */
	uint8_t selected_range;
	int16_t x;
	int16_t y;
	int16_t z;
};

union adxl345_bus {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	struct i2c_dt_spec i2c;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	struct spi_dt_spec spi;
#endif
};

typedef bool (*adxl345_bus_is_ready_fn)(const union adxl345_bus *bus);
typedef int (*adxl345_reg_access_fn)(const struct device *dev, uint8_t cmd,
				     uint8_t reg_addr, uint8_t *data, size_t length);

struct adxl345_dev_config {
	const union adxl345_bus bus;
	adxl345_bus_is_ready_fn bus_is_ready;
	adxl345_reg_access_fn reg_access;
	enum adxl345_odr odr;
	bool op_mode;
	struct adxl345_fifo_config fifo_config;
#ifdef CONFIG_ADXL345_TRIGGER
	struct gpio_dt_spec interrupt;
#endif
};

void adxl345_submit_stream(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);
void adxl345_stream_irq_handler(const struct device *dev);

#ifdef CONFIG_ADXL345_TRIGGER
int adxl345_get_status(const struct device *dev,
		       uint8_t *status, uint16_t *fifo_entries);

int adxl345_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

int adxl345_init_interrupt(const struct device *dev);

#endif /* CONFIG_ADXL345_TRIGGER */

int adxl345_reg_write_mask(const struct device *dev,
			       uint8_t reg_addr,
			       uint8_t mask,
			       uint8_t data);

int adxl345_reg_access(const struct device *dev, uint8_t cmd, uint8_t addr,
				     uint8_t *data, size_t len);

int adxl345_reg_write(const struct device *dev, uint8_t addr, uint8_t *data,
				    uint8_t len);

int adxl345_reg_read(const struct device *dev, uint8_t addr, uint8_t *data,
				   uint8_t len);

int adxl345_reg_write_byte(const struct device *dev, uint8_t addr, uint8_t val);

int adxl345_reg_read_byte(const struct device *dev, uint8_t addr, uint8_t *buf);

int adxl345_set_op_mode(const struct device *dev, enum adxl345_op_mode op_mode);
#ifdef CONFIG_SENSOR_ASYNC_API
int adxl345_read_sample(const struct device *dev, struct adxl345_sample *sample);
void adxl345_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);
int adxl345_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder);
void adxl345_accel_convert(struct sensor_value *val, int16_t sample);
#endif /* CONFIG_SENSOR_ASYNC_API */

#ifdef CONFIG_ADXL345_STREAM
int adxl345_configure_fifo(const struct device *dev, enum adxl345_fifo_mode mode,
		enum adxl345_fifo_trigger trigger, uint16_t fifo_samples);
size_t adxl345_get_packet_size(const struct adxl345_dev_config *cfg);
#endif /* CONFIG_ADXL345_STREAM */
#endif /* ZEPHYR_DRIVERS_SENSOR_ADX345_ADX345_H_ */

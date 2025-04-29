/*
 * Copyright (c) 2017 IpTronix S.r.l.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ADXL362_ADXL362_H_
#define ZEPHYR_DRIVERS_SENSOR_ADXL362_ADXL362_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/dt-bindings/sensor/adxl362.h>

#define ADXL362_SLAVE_ID    1

/* ADXL362 communication commands */
#define ADXL362_WRITE_REG           0x0A
#define ADXL362_READ_REG            0x0B
#define ADXL362_READ_FIFO           0x0D

/* Registers */
#define ADXL362_REG_DEVID_AD            0x00
#define ADXL362_REG_DEVID_MST           0x01
#define ADXL362_REG_PARTID              0x02
#define ADXL362_REG_REVID               0x03
#define ADXL362_REG_XDATA               0x08
#define ADXL362_REG_YDATA               0x09
#define ADXL362_REG_ZDATA               0x0A
#define ADXL362_REG_STATUS              0x0B
#define ADXL362_REG_FIFO_L              0x0C
#define ADXL362_REG_FIFO_H              0x0D
#define ADXL362_REG_XDATA_L             0x0E
#define ADXL362_REG_XDATA_H             0x0F
#define ADXL362_REG_YDATA_L             0x10
#define ADXL362_REG_YDATA_H             0x11
#define ADXL362_REG_ZDATA_L             0x12
#define ADXL362_REG_ZDATA_H             0x13
#define ADXL362_REG_TEMP_L              0x14
#define ADXL362_REG_TEMP_H              0x15
#define ADXL362_REG_SOFT_RESET          0x1F
#define ADXL362_REG_THRESH_ACT_L        0x20
#define ADXL362_REG_THRESH_ACT_H        0x21
#define ADXL362_REG_TIME_ACT            0x22
#define ADXL362_REG_THRESH_INACT_L      0x23
#define ADXL362_REG_THRESH_INACT_H      0x24
#define ADXL362_REG_TIME_INACT_L        0x25
#define ADXL362_REG_TIME_INACT_H        0x26
#define ADXL362_REG_ACT_INACT_CTL       0x27
#define ADXL362_REG_FIFO_CTL            0x28
#define ADXL362_REG_FIFO_SAMPLES        0x29
#define ADXL362_REG_INTMAP1             0x2A
#define ADXL362_REG_INTMAP2             0x2B
#define ADXL362_REG_FILTER_CTL          0x2C
#define ADXL362_REG_POWER_CTL           0x2D
#define ADXL362_REG_SELF_TEST           0x2E

/* ADXL362_REG_STATUS definitions */
#define ADXL362_STATUS_ERR_USER_REGS        (1 << 7)
#define ADXL362_STATUS_AWAKE                (1 << 6)
#define ADXL362_STATUS_INACT                (1 << 5)
#define ADXL362_STATUS_ACT                  (1 << 4)
#define ADXL362_STATUS_FIFO_OVERRUN         (1 << 3)
#define ADXL362_STATUS_FIFO_WATERMARK       (1 << 2)
#define ADXL362_STATUS_FIFO_RDY             (1 << 1)
#define ADXL362_STATUS_DATA_RDY             (1 << 0)

/* ADXL362_REG_ACT_INACT_CTL definitions */
#define ADXL362_ACT_INACT_CTL_LINKLOOP(x)   (((x) & 0x3) << 4)
#define ADXL362_ACT_INACT_CTL_INACT_REF     (1 << 3)
#define ADXL362_ACT_INACT_CTL_INACT_EN      (1 << 2)
#define ADXL362_ACT_INACT_CTL_ACT_REF       (1 << 1)
#define ADXL362_ACT_INACT_CTL_ACT_EN        (1 << 0)

/* ADXL362_ACT_INACT_CTL_LINKLOOP(x) options */
#define ADXL362_MODE_DEFAULT        0
#define ADXL362_MODE_LINK           1
#define ADXL362_MODE_LOOP           3

/* ADXL362_REG_FIFO_CTL */
#define ADXL362_FIFO_CTL_AH                 (1 << 3)
#define ADXL362_FIFO_CTL_FIFO_TEMP          (1 << 2)
#define ADXL362_FIFO_CTL_FIFO_MODE(x)       (((x) & 0x3) << 0)

/* ADXL362_FIFO_CTL_FIFO_MODE(x) options */
#define ADXL362_FIFO_DISABLE              ADXL362_FIFO_MODE_DISABLED
#define ADXL362_FIFO_OLDEST_SAVED         ADXL362_FIFO_MODE_OLDEST_SAVED
#define ADXL362_FIFO_STREAM               ADXL362_FIFO_MODE_STREAM
#define ADXL362_FIFO_TRIGGERED            ADXL362_FIFO_MODE_TRIGGERED

/* ADXL362_REG_INTMAP1 */
#define ADXL362_INTMAP1_INT_LOW             (1 << 7)
#define ADXL362_INTMAP1_AWAKE               (1 << 6)
#define ADXL362_INTMAP1_INACT               (1 << 5)
#define ADXL362_INTMAP1_ACT                 (1 << 4)
#define ADXL362_INTMAP1_FIFO_OVERRUN        (1 << 3)
#define ADXL362_INTMAP1_FIFO_WATERMARK      (1 << 2)
#define ADXL362_INTMAP1_FIFO_READY          (1 << 1)
#define ADXL362_INTMAP1_DATA_READY          (1 << 0)

/* ADXL362_REG_INTMAP2 definitions */
#define ADXL362_INTMAP2_INT_LOW             (1 << 7)
#define ADXL362_INTMAP2_AWAKE               (1 << 6)
#define ADXL362_INTMAP2_INACT               (1 << 5)
#define ADXL362_INTMAP2_ACT                 (1 << 4)
#define ADXL362_INTMAP2_FIFO_OVERRUN        (1 << 3)
#define ADXL362_INTMAP2_FIFO_WATERMARK      (1 << 2)
#define ADXL362_INTMAP2_FIFO_READY          (1 << 1)
#define ADXL362_INTMAP2_DATA_READY          (1 << 0)

/* ADXL362_REG_FILTER_CTL definitions */
#define ADXL362_FILTER_CTL_RANGE(x)         (((x) & 0x3) << 6)
#define ADXL362_FILTER_CTL_RES              (1 << 5)
#define ADXL362_FILTER_CTL_HALF_BW          (1 << 4)
#define ADXL362_FILTER_CTL_EXT_SAMPLE       (1 << 3)
#define ADXL362_FILTER_CTL_ODR(x)           (((x) & 0x7) << 0)

/* ADXL362_FILTER_CTL_RANGE(x) options */
#define ADXL362_RANGE_2G                0       /* +/-2 g */
#define ADXL362_RANGE_4G                1       /* +/-4 g */
#define ADXL362_RANGE_8G                2       /* +/-8 g */

/* ADXL362_FILTER_CTL_ODR(x) options */
#define ADXL362_ODR_12_5_HZ             0       /* 12.5 Hz */
#define ADXL362_ODR_25_HZ               1       /* 25 Hz */
#define ADXL362_ODR_50_HZ               2       /* 50 Hz */
#define ADXL362_ODR_100_HZ              3       /* 100 Hz */
#define ADXL362_ODR_200_HZ              4       /* 200 Hz */
#define ADXL362_ODR_400_HZ              5       /* 400 Hz */

/* ADXL362_REG_POWER_CTL definitions */
#define ADXL362_POWER_CTL_RES               (1 << 7)
#define ADXL362_POWER_CTL_EXT_CLK           (1 << 6)
#define ADXL362_POWER_CTL_LOW_NOISE(x)      (((x) & 0x3) << 4)
#define ADXL362_POWER_CTL_WAKEUP            (1 << 3)
#define ADXL362_POWER_CTL_AUTOSLEEP         (1 << 2)
#define ADXL362_POWER_CTL_MEASURE(x)        (((x) & 0x3) << 0)

/* ADXL362_POWER_CTL_LOW_NOISE(x) options */
#define ADXL362_NOISE_MODE_NORMAL           0
#define ADXL362_NOISE_MODE_LOW              1
#define ADXL362_NOISE_MODE_ULTRALOW         2

/* ADXL362_POWER_CTL_MEASURE(x) options */
#define ADXL362_MEASURE_STANDBY         0
#define ADXL362_MEASURE_ON              2

/* ADXL362_REG_SELF_TEST */
#define ADXL362_SELF_TEST_ST            (1 << 0)

/* ADXL362 device information */
#define ADXL362_DEVICE_AD               0xAD
#define ADXL362_DEVICE_MST              0x1D
#define ADXL362_PART_ID                 0xF2

/* ADXL362 Reset settings */
#define ADXL362_RESET_KEY               0x52

/* ADXL362 Status check */
#define ADXL362_STATUS_CHECK_DATA_READY(x)	(((x) >> 0) & 0x1)
#define ADXL362_STATUS_CHECK_INACT(x)		(((x) >> 5) & 0x1)
#define ADXL362_STATUS_CHECK_ACTIVITY(x)	(((x) >> 4) & 0x1)
#define ADXL362_STATUS_CHECK_FIFO_OVR(x)	(((x) >> 3) & 0x1)
#define ADXL362_STATUS_CHECK_FIFO_WTR(x)	(((x) >> 2) & 0x1)

/* ADXL362 scale factors from specifications */
#define ADXL362_ACCEL_2G_LSB_PER_G	1000
#define ADXL362_ACCEL_4G_LSB_PER_G	500
#define ADXL362_ACCEL_8G_LSB_PER_G	235

/* ADXL362 temperature sensor specifications */
#define ADXL362_TEMP_MC_PER_LSB 65
#define ADXL362_TEMP_BIAS_LSB 350
#define ADXL362_TEMP_BIAS_TEST_CONDITION 25

/* ADXL362 check fifo sample header */
#define ADXL362_FIFO_HDR_CHECK_ACCEL_X(x)	((((x) & 0xC000) >> 14) == 0x00)
#define ADXL362_FIFO_HDR_CHECK_ACCEL_Y(x)	((((x) & 0xC000) >> 14) == 0x01)
#define ADXL362_FIFO_HDR_CHECK_ACCEL_Z(x)	((((x) & 0xC000) >> 14) == 0x02)
#define ADXL362_FIFO_HDR_CHECK_TEMP(x)	((((x) & 0xC000) >> 14) == 0x03)

struct adxl362_config {
	struct spi_dt_spec bus;
#if defined(CONFIG_ADXL362_TRIGGER)
	struct gpio_dt_spec interrupt;
	uint8_t int1_config;
	uint8_t int2_config;
#endif
	uint8_t power_ctl;
	uint8_t fifo_mode;
	uint16_t water_mark_lvl;
};

struct adxl362_data {
	union {
		int16_t acc_xyz[3];
		struct {
			int16_t acc_x;
			int16_t acc_y;
			int16_t acc_z;
		};
	} __packed;
	int16_t temp;
	uint8_t selected_range;
	uint8_t accel_odr;

	uint8_t fifo_mode;
	uint8_t en_temp_read;
	uint16_t water_mark_lvl;

#if defined(CONFIG_ADXL362_TRIGGER)
	const struct device *dev;
	struct gpio_callback gpio_cb;
	struct k_mutex trigger_mutex;

	sensor_trigger_handler_t inact_handler;
	const struct sensor_trigger *inact_trigger;
	sensor_trigger_handler_t act_handler;
	const struct sensor_trigger *act_trigger;
	sensor_trigger_handler_t drdy_handler;
	const struct sensor_trigger *drdy_trigger;

#if defined(CONFIG_ADXL362_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ADXL362_THREAD_STACK_SIZE);
	struct k_sem gpio_sem;
	struct k_thread thread;
#elif defined(CONFIG_ADXL362_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif
#endif /* CONFIG_ADXL362_TRIGGER */
#ifdef CONFIG_ADXL362_STREAM
	uint8_t status;
	uint8_t fifo_ent[2];
	struct rtio_iodev_sqe *sqe;
	struct rtio *rtio_ctx;
	struct rtio_iodev *iodev;
	uint64_t timestamp;
	struct rtio *r_cb;
	uint8_t fifo_full_irq: 1;
	uint8_t fifo_wmark_irq: 1;
	uint8_t res: 6;
#endif /* CONFIG_ADXL362_STREAM */
};

struct adxl362_sample_data {
#ifdef CONFIG_ADXL362_STREAM
	uint8_t is_fifo: 1;
	uint8_t res: 7;
#endif /*CONFIG_ADXL362_STREAM*/
	uint8_t selected_range;
	int16_t acc_x;
	int16_t acc_y;
	int16_t acc_z;
	int16_t temp;
};

struct adxl362_fifo_data {
	uint8_t is_fifo: 1;
	uint8_t has_tmp: 1;
	uint8_t selected_range: 3;
	uint8_t accel_odr: 3;
	uint8_t int_status;
	uint16_t fifo_byte_count;
	uint64_t timestamp;
} __attribute__((__packed__));

BUILD_ASSERT(sizeof(struct adxl362_fifo_data) % 4 == 0,
		"adxl362_fifo_data struct should be word aligned");

#if defined(CONFIG_ADXL362_ACCEL_RANGE_RUNTIME) ||\
		defined(CONFIG_ADXL362_ACCEL_RANGE_2G)
#	define ADXL362_DEFAULT_RANGE_ACC		ADXL362_RANGE_2G
#elif defined(CONFIG_ADXL362_ACCEL_RANGE_4G)
#	define ADXL362_DEFAULT_RANGE_ACC		ADXL362_RANGE_4G
#else
#	define ADXL362_DEFAULT_RANGE_ACC		ADXL362_RANGE_8G
#endif

#if defined(CONFIG_ADXL362_ACCEL_ODR_RUNTIME) ||\
		defined(CONFIG_ADXL362_ACCEL_ODR_12_5)
#	define ADXL362_DEFAULT_ODR_ACC		ADXL362_ODR_12_5_HZ
#elif defined(CONFIG_ADXL362_ACCEL_ODR_25)
#	define ADXL362_DEFAULT_ODR_ACC		ADXL362_ODR_25_HZ
#elif defined(CONFIG_ADXL362_ACCEL_ODR_50)
#	define ADXL362_DEFAULT_ODR_ACC		ADXL362_ODR_50_HZ
#elif defined(CONFIG_ADXL362_ACCEL_ODR_100)
#	define ADXL362_DEFAULT_ODR_ACC		ADXL362_ODR_100_HZ
#elif defined(CONFIG_ADXL362_ACCEL_ODR_200)
#	define ADXL362_DEFAULT_ODR_ACC		ADXL362_ODR_200_HZ
#else
#	define ADXL362_DEFAULT_ODR_ACC		ADXL362_ODR_400_HZ
#endif

void adxl362_submit_stream(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);
void adxl362_stream_irq_handler(const struct device *dev);
int adxl362_fifo_read(const struct device *dev, void *buff, size_t length);

#ifdef CONFIG_ADXL362_TRIGGER
int adxl362_reg_write_mask(const struct device *dev,
			   uint8_t reg_addr, uint8_t mask, uint8_t data);

int adxl362_get_status(const struct device *dev, uint8_t *status);

int adxl362_interrupt_activity_enable(const struct device *dev);

int adxl362_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

int adxl362_init_interrupt(const struct device *dev);

int adxl362_set_interrupt_mode(const struct device *dev, uint8_t mode);

int adxl362_clear_data_ready(const struct device *dev);
#endif /* CONFIG_ADT7420_TRIGGER */

#ifdef CONFIG_SENSOR_ASYNC_API
int adxl362_rtio_fetch(const struct device *dev,
				struct adxl362_sample_data *sample_data);
void adxl362_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);
int adxl362_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder);
void adxl362_accel_convert(struct sensor_value *val, int accel,
				  int range);
void adxl362_temp_convert(struct sensor_value *val, int temp);
#endif /* CONFIG_SENSOR_ASYNC_API */

#ifdef CONFIG_ADXL362_STREAM
int adxl362_fifo_setup(const struct device *dev, uint8_t mode,
			      uint16_t water_mark_lvl, uint8_t en_temp_read);
#endif /* CONFIG_ADXL362_STREAM */

int adxl362_reg_access(const struct device *dev, uint8_t cmd,
			      uint8_t reg_addr, void *data, size_t length);
#endif /* ZEPHYR_DRIVERS_SENSOR_ADXL362_ADXL362_H_ */

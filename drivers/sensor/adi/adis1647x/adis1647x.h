/*
 * Copyright (c) 2026 Analog Devices, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ADIS1647X_ADIS1647X_H_
#define ZEPHYR_DRIVERS_SENSOR_ADIS1647X_ADIS1647X_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/rtio/rtio.h>

/* SPI stall times */
#define ADIS1647X_SPI_STALL_US      16
#define ADIS1647X_HW_RESET_PULSE_MS 10
#define ADIS1647X_HW_RESET_WAIT_MS  310
#define ADIS1647X_SW_RESET_MS       310

#define ADIS1647X_WRITE_CMD 0x80
#define ADIS1647X_READ_CMD  0x00
#define ADIS1647X_BURST_CMD 0x68

#define ADIS1647X_REG_PROD_ID  0x72
#define ADIS1647X_REG_RANG_MDL 0x5E

#define ADIS1647X_REG_MSC_CTRL          0x60
#define ADIS1647X_REG_MSC_CTRL_BURST32  BIT(9)
#define ADIS1647X_REG_MSC_CTRL_BURSTSEL BIT(8)
#define ADIS1647X_REG_MSC_CTRL_GCOMP    BIT(7)
#define ADIS1647X_REG_MSC_CTRL_PCOMP    BIT(6)
#define ADIS1647X_REG_MSC_CTRL_DR_POL   BIT(0)
#define ADIS1647X_REG_MSC_CTRL_DEFAULT                                                             \
	(ADIS1647X_REG_MSC_CTRL_PCOMP | ADIS1647X_REG_MSC_CTRL_DR_POL)

#define ADIS1647X_REG_FIRM_REV            0x6C
#define ADIS1647X_REG_GLOBAL_CMD          0x68
#define ADIS1647X_REG_GLOBAL_CMD_SW_RESET BIT(7)

#define ADIS1647X_REG_DEC_RATE 0x64

#define ADIS1647X_DIAG_Z_ACCEL_FAILURE      BIT(13)
#define ADIS1647X_DIAG_Y_ACCEL_FAILURE      BIT(12)
#define ADIS1647X_DIAG_X_ACCEL_FAILURE      BIT(11)
#define ADIS1647X_DIAG_Z_GYRO_FAILURE       BIT(10)
#define ADIS1647X_DIAG_Y_GYRO_FAILURE       BIT(9)
#define ADIS1647X_DIAG_X_GYRO_FAILURE       BIT(8)
#define ADIS1647X_DIAG_SELF_TEST_ERR        BIT(5)
#define ADIS1647X_DIAG_POWER_SUPPLY         BIT(4)
#define ADIS1647X_DIAG_SPI_COMM_ERROR       BIT(3)
#define ADIS1647X_DIAG_FLASH_UPDATE_FAILURE BIT(2)
#define ADIS1647X_DIAG_DATAPATH_OVERRUN     BIT(1)
#define ADIS1647X_DIAG_SENSOR_INIT_FAILURE  BIT(0)

/* Model index order must match the enum in adi,adis1647x.yaml */
enum adis1647x_model_idx {
	ADIS1647X_MODEL_ADIS16470 = 0,
	ADIS1647X_MODEL_ADIS16475_1,
	ADIS1647X_MODEL_ADIS16475_2,
	ADIS1647X_MODEL_ADIS16475_3,
	ADIS1647X_MODEL_ADIS16477_1,
	ADIS1647X_MODEL_ADIS16477_2,
	ADIS1647X_MODEL_ADIS16477_3,
};

struct adis1647x_model_cfg {
	uint8_t accel_scale_num; /* mg/LSB * 100, e.g. 125 = 1.25 mg/LSB */
	uint16_t gyro_scale_num; /* (max_dps / 20000) * 100000, e.g. 625 = 125 dps */
	uint16_t prod_id;        /* expected PROD_ID register value */
	uint8_t rang_mdl;        /* expected RANG_MDL bits [3:2] value */
};

struct adis1647x_config {
	uint8_t model_idx;
	int dec_rate;
	struct spi_dt_spec spi;
	struct gpio_dt_spec reset;
#if defined(CONFIG_ADIS1647X_TRIGGER) || defined(CONFIG_ADIS1647X_STREAM)
	struct gpio_dt_spec interrupt;
#endif
};

struct adis1647x_data {
	uint8_t accel_scale_num;
	uint16_t gyro_scale_num;
	int16_t accel_x, accel_y, accel_z;
	int16_t gyro_x, gyro_y, gyro_z;
	int16_t temp;

#ifdef CONFIG_ADIS1647X_TRIGGER
	const struct device *dev;
	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t drdy_handler;
	const struct sensor_trigger *drdy_trigger;
#if defined(CONFIG_ADIS1647X_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ADIS1647X_THREAD_STACK_SIZE);
	struct k_sem gpio_sem;
	struct k_thread thread;
#elif defined(CONFIG_ADIS1647X_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif
#endif /*CONFIG_ADIS1647X_TRIGGER*/

#ifdef CONFIG_SENSOR_ASYNC_API
	struct rtio *rtio_ctx;
	struct rtio_iodev *iodev;
#endif /* CONFIG_SENSOR_ASYNC_API */

#ifdef CONFIG_ADIS1647X_STREAM
	struct rtio_iodev_sqe *sqe;
	uint64_t timestamp;
#endif /* CONFIG_ADIS1647X_STREAM */
};

struct adis1647x_burst_data {
	uint8_t cmd[2];
	uint16_t diag_stat;
	uint16_t x_gyro_out;
	uint16_t y_gyro_out;
	uint16_t z_gyro_out;
	uint16_t x_accel_out;
	uint16_t y_accel_out;
	uint16_t z_accel_out;
	uint16_t temp_out;
	uint16_t data_cntr;
	uint16_t spi_chksum;
} __packed;

struct adis1647x_sample_data {
	uint8_t accel_scale_num;
	uint16_t gyro_scale_num;
	uint64_t timestamp;
	struct adis1647x_burst_data burst_data;
};

#ifdef CONFIG_ADIS1647X_TRIGGER
int adis1647x_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler);
int adis1647x_init_interrupt(const struct device *dev);
#endif

/* SPI functions */
int adis1647x_reg_read(const struct device *dev, uint8_t addr, uint16_t *val);
int adis1647x_reg_write(const struct device *dev, uint8_t addr, uint16_t val);
int adis1647x_reg_write_no_verify(const struct device *dev, uint8_t addr, uint16_t val);
int adis1647x_burst_read(const struct device *dev, struct adis1647x_burst_data *data);

/* API functions */
#ifdef CONFIG_SENSOR_ASYNC_API
void adis1647x_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);
#endif
int adis1647x_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder);

/* Conversion helpers */
void adis1647x_accel_convert(struct sensor_value *val, int16_t raw, uint8_t accel_scale_num);
void adis1647x_gyro_convert(struct sensor_value *val, int16_t raw, uint16_t gyro_scale_num);

/* Data helpers */
int adis1647x_get_data(const struct device *dev, struct adis1647x_sample_data *sample_data);
int adis1647x_verify_burst(const struct adis1647x_burst_data *b);
#ifdef CONFIG_ADIS1647X_STREAM
void adis1647x_submit_stream(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);
void adis1647x_stream_irq_handler(const struct device *dev);
#endif
#endif

/*
 * Copyright The Zephyr Project Contributors
 * Copyright (c) 2026 Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_MS5637_MS5637_H_
#define ZEPHYR_DRIVERS_SENSOR_MS5637_MS5637_H_

#include <zephyr/drivers/sensor_data_types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/mpsc_lockfree.h>
#include <zephyr/kernel.h>
#ifdef CONFIG_SENSOR_ASYNC_API
#include <zephyr/rtio/rtio.h>
#endif

/* I2C commands (MS5637-02BA03 datasheet) */
#define MS5637_CMD_RESET    0x1EU
#define MS5637_CMD_READ_ADC 0x00U

/* Pressure conversion commands: base 0x40 + OSR offset */
#define MS5637_CMD_CONV_P_256  0x40U
#define MS5637_CMD_CONV_P_512  0x42U
#define MS5637_CMD_CONV_P_1024 0x44U
#define MS5637_CMD_CONV_P_2048 0x46U
#define MS5637_CMD_CONV_P_4096 0x48U
#define MS5637_CMD_CONV_P_8192 0x4AU

/* Temperature conversion commands: base 0x50 + OSR offset */
#define MS5637_CMD_CONV_T_256  0x50U
#define MS5637_CMD_CONV_T_512  0x52U
#define MS5637_CMD_CONV_T_1024 0x54U
#define MS5637_CMD_CONV_T_2048 0x56U
#define MS5637_CMD_CONV_T_4096 0x58U
#define MS5637_CMD_CONV_T_8192 0x5AU

/* PROM read commands: base 0xA0 + word_index * 2 */
#define MS5637_PROM_ADDR_BASE  0xA0U
#define MS5637_PROM_WORD_COUNT 8U

/* PROM coefficient indices */
#define MS5637_PROM_CRC_IDX 0U
#define MS5637_PROM_C1_IDX  1U /* Pressure sensitivity | SENS_T1 */
#define MS5637_PROM_C2_IDX  2U /* Pressure offset | OFF_T1 */
#define MS5637_PROM_C3_IDX  3U /* Temperature coefficient of pressure sensitivity | TCS */
#define MS5637_PROM_C4_IDX  4U /* Temperature coefficient of pressure offset | TCO */
#define MS5637_PROM_C5_IDX  5U /* Reference temperature | T_REF */
#define MS5637_PROM_C6_IDX  6U /* Temperature coefficient of temperature | TEMPSENS */

/* Reset recovery time per datasheet: 2.8 ms typical */
#define MS5637_RESET_TIME_MS 3U

/* ADC read byte count (24-bit result) */
#define MS5637_ADC_READ_LEN 3U

/* CRC-4 parameters */
#define MS5637_CRC4_POLY        0x3000U
#define MS5637_CRC_NIBBLE_SHIFT 12U
#define MS5637_CRC_NIBBLE_MASK  0x000FU

/* Micro-degrees per centidegree for sensor_value_from_micro() */
#define MS5637_MICRO_PER_CENTIDEG 10000

/*
 * ADC conversion delays in ms per OSR level (datasheet Table 2).
 * Values are max conversion times rounded up.
 */
#define MS5637_CONV_DELAY_256  1U
#define MS5637_CONV_DELAY_512  2U
#define MS5637_CONV_DELAY_1024 3U
#define MS5637_CONV_DELAY_2048 5U
#define MS5637_CONV_DELAY_4096 9U
#define MS5637_CONV_DELAY_8192 17U

struct ms5637_config {
	struct i2c_dt_spec bus;
	uint16_t osr_pressure;
	uint16_t osr_temperature;
#ifdef CONFIG_SENSOR_ASYNC_API
	struct rtio *r;
	struct rtio_iodev *bus_iodev;
#endif /* CONFIG_SENSOR_ASYNC_API */
};

struct ms5637_calibration {
	uint16_t sens_t1;
	uint16_t off_t1;
	uint16_t tcs;
	uint16_t tco;
	uint16_t t_ref;
	uint16_t tempsens;
};

struct ms5637_reading {
	int32_t pressure;     /* mbar * 100 (i.e. Pascals) */
	int32_t temperature;  /* centidegrees Celsius */
};

struct ms5637_data {
	/* PROM calibration coefficients */
	struct ms5637_calibration cal;

	/* Compensated measurement results */
	struct ms5637_reading reading;

	/* Active conversion commands */
	uint8_t pressure_conv_cmd;
	uint8_t temperature_conv_cmd;

	/* Conversion delays in ms */
	uint8_t pressure_conv_delay;
	uint8_t temperature_conv_delay;

#ifdef CONFIG_SENSOR_ASYNC_API
	const struct device *dev;
	struct k_spinlock mpsc_lock;
	struct mpsc io_q;
	struct rtio_iodev_sqe *pending_sqe;
	/*
	 * DMA-safe read buffers (must stay valid until callback fires):
	 *   [0..2] = pressure ADC result (24-bit big-endian)
	 *   [3..5] = temperature ADC result (24-bit big-endian)
	 */
	uint8_t raw_buffer[6];
#endif /* CONFIG_SENSOR_ASYNC_API */
};

void ms5637_compensate(const struct ms5637_calibration *calibration,
		       uint32_t adc_temperature,
		       uint32_t adc_pressure,
		       struct ms5637_reading *reading);

#ifdef CONFIG_SENSOR_ASYNC_API

struct ms5637_encoded_data {
	struct sensor_data_header header;
	uint32_t adc_pressure;
	uint32_t adc_temperature;
	struct ms5637_calibration calibration;
};

void ms5637_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);
int ms5637_get_decoder(const struct device *dev,
			const struct sensor_decoder_api **decoder);
#endif /* CONFIG_SENSOR_ASYNC_API */

#endif /* ZEPHYR_DRIVERS_SENSOR_MS5637_MS5637_H_ */

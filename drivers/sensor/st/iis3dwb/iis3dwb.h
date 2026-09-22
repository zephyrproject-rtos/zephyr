/* ST Microelectronics IIS3DWB accelerometer sensor
 *
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/iis3dwb.pdf
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_IIS3DWB_IIS3DWB_H_
#define ZEPHYR_DRIVERS_SENSOR_IIS3DWB_IIS3DWB_H_

#include <stdint.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/sys/util.h>
#include <stmemsc.h>
#include <zephyr/rtio/regmap.h>

#include "iis3dwb_reg.h"
#include <zephyr/dt-bindings/sensor/iis3dwb.h>
#include <zephyr/drivers/sensor_clock.h>

#define GAIN_UNIT (61LL)

struct trigger_config {
	uint8_t int_fifo_th : 1;
	uint8_t int_fifo_full : 1;
	uint8_t int_drdy : 1;
};

struct iis3dwb_config {
	stmdev_ctx_t ctx;
	union {
#if DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(st_iis3dwb, spi)
		const struct spi_dt_spec spi;
#endif
	} stmemsc_cfg;

	uint8_t range;
	uint8_t filter;
	uint8_t odr;

#ifdef CONFIG_IIS3DWB_STREAM
	uint16_t fifo_wtm;
	uint8_t accel_batch : 4;
	uint8_t temp_batch : 2;
	uint8_t ts_batch : 2;
#endif
#ifdef CONFIG_IIS3DWB_TRIGGER
	const struct gpio_dt_spec int1_gpio;
	const struct gpio_dt_spec int2_gpio;
	uint8_t drdy_pulsed;
	uint8_t drdy_pin;
	bool trig_enabled;
#endif
};

struct iis3dwb_data {
	int16_t acc[3];

	uint8_t range;
	uint8_t odr;

	struct rtio *rtio_ctx;
	struct rtio_iodev *iodev;
	struct rtio_iodev_sqe *streaming_sqe;

#ifdef CONFIG_IIS3DWB_STREAM
	uint64_t timestamp;
	uint8_t status;
	uint8_t fifo_status[2];
	uint16_t fifo_count;
	struct trigger_config trig_cfg;
	uint8_t accel_batch_odr : 4;
	uint8_t temp_batch_odr : 2;
	uint8_t ts_batch_odr : 2;
#endif

#ifdef CONFIG_IIS3DWB_TRIGGER
	const struct gpio_dt_spec *drdy_gpio;
	struct gpio_callback gpio_cb;

	const struct device *dev;
#endif /* CONFIG_IIS3DWB_TRIGGER */
};

int iis3dwb_spi_init(const struct device *dev);

#define IIS3DWB_FIFO_ITEM_LEN 7
#define IIS3DWB_FIFO_SIZE(x)  (x * IIS3DWB_FIFO_ITEM_LEN)

#ifdef CONFIG_IIS3DWB_TRIGGER
int iis3dwb_init_interrupt(const struct device *dev);
#endif

/* decoder */
struct iis3dwb_decoder_header {
	uint64_t timestamp;
	uint8_t is_fifo: 1;
	uint8_t range: 2;
	uint8_t reserved: 5;
	uint8_t int_status;
} __attribute__((__packed__));

struct iis3dwb_fifo_data {
	struct iis3dwb_decoder_header header;
	uint32_t accel_odr: 4;
	uint32_t fifo_mode_sel: 2;
	uint32_t fifo_count: 10;
	uint32_t reserved_1: 5;
	uint32_t accel_batch_odr: 4;
	uint32_t temp_batch_odr: 2;
	uint32_t ts_batch_odr: 2;
	uint32_t reserved: 3;
} __attribute__((__packed__));

struct iis3dwb_rtio_data {
	struct iis3dwb_decoder_header header;
	struct {
		uint8_t has_accel: 1; /* set if accel channel has data */
		uint8_t has_temp: 1;  /* set if temp channel has data */
		uint8_t reserved: 6;
	} __attribute__((__packed__));
	int16_t accel[3];
	int16_t temp;
};

int iis3dwb_encode(const struct device *dev, const struct sensor_chan_spec *const channels,
		   const size_t num_channels, uint8_t *buf);
int iis3dwb_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder);

#ifdef CONFIG_IIS3DWB_TRIGGER
int iis3dwb_route_int1(const struct device *dev, iis3dwb_pin_int1_route_t pin_int);
int iis3dwb_route_int2(const struct device *dev, iis3dwb_pin_int2_route_t pin_int);
void iis3dwb_stream_irq_handler(const struct device *dev);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_IIS3DWB_IIS3DWB_H_ */

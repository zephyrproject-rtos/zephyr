/* ST Microelectronics HTS221 humidity sensor
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/hts221.pdf
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ZIO_HTS221_H_
#define ZEPHYR_INCLUDE_DRIVERS_ZIO_HTS221_H_

#include <gpio.h>
#include <zio.h>
#include "hts221_reg.h"

/* device attributes */
#define HTS221_SAMPLE_RATE_IDX 0

/* datum type */
struct hts221_datum {
	s32_t hum_perc;
	s32_t temp_degC;
};

struct hts221_config {
	char *master_dev_name;
	int (*bus_init)(struct device *dev);
#ifdef DT_ST_HTS221_BUS_I2C
	u16_t i2c_slv_addr;
#endif
#ifdef CONFIG_HTS221_TRIGGER
	const char *drdy_port;
	u8_t drdy_pin;
#endif
};

typedef struct {
	s16_t x0;
	u8_t  y0;
	s16_t x1;
	u8_t  y1;
} lin_t;

struct hts221_data {
	struct device *bus;
	hts221_ctx_t *ctx;

#ifdef DT_ST_HTS221_BUS_I2C
	hts221_ctx_t ctx_i2c;
#elif DT_ST_HTS221_BUS_SPI
	hts221_ctx_t ctx_spi;
#endif

#ifdef CONFIG_HTS221_TRIGGER
	struct device *gpio;
	u32_t pin;
	struct gpio_callback gpio_cb;
	//struct sensor_trigger data_ready_trigger;
	//sensor_trigger_handler_t handler_drdy;

#if defined(CONFIG_HTS221_TRIGGER_OWN_THREAD)
	K_THREAD_STACK_MEMBER(thread_stack, CONFIG_HTS221_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_HTS221_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
	struct device *dev;
#endif
#endif /* CONFIG_HTS221_TRIGGER */

	lin_t lin_hum;
	lin_t lin_temp;
	u8_t sample_rate;
	struct k_work work;
	struct device *dev;
	ZIO_FIFO_BUF_DECLARE(fifo, struct hts221_datum, 1 /*CONFIG_FT5336_FIFO_SIZE*/);
};

int hts221_i2c_init(struct device *dev);
#ifdef CONFIG_HTS221_TRIGGER
int hts221_init_interrupt(struct device *dev);
#endif /* CONFIG_HTS221_TRIGGER */
#endif /* ZEPHYR_INCLUDE_DRIVERS_ZIO_HTS221_H_ */

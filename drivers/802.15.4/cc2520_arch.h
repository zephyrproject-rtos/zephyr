/* cc2520_arch.h - TI CC2520 arch/driver model specific header */

/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* You should not include this file directly and it should only be
 * included by board.h file.
 */

#ifndef __CC2520_ARCH_H__
#define __CC2520_ARCH_H__

#include <nanokernel.h>
#include <stdbool.h>
#include <string.h>

#include <spi.h>
#include <gpio.h>

#include "cc2520.h"

#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define CC2520_PRINT          printf
#else
#include <misc/printk.h>
#define CC2520_PRINT          printk
#endif

#if CONFIG_TI_CC2520_DEBUG
#define CC2520_PRINTF(...) CC2520_PRINT(__VA_ARGS__)
#else
#define CC2520_PRINTF(...)
#endif

#ifndef CLOCK_CYCLE_LT
#define CLOCK_CYCLE_LT(a, b)	((signed)((a)-(b)) < 0)
#endif

#ifndef CLOCK_MSEC_TO_CYCLES
#define CLOCK_MSEC_TO_CYCLES(msec)			\
	((msec) * sys_clock_hw_cycles_per_tick *	\
	sys_clock_us_per_tick / 1000)
#endif

#define CONFIG_CC2520_DRV_NAME "CC2520"

struct cc2520_gpio_config {
	struct device *gpio;
};

struct cc2520_config {
	struct cc2520_gpio_config **gpios;
	struct device *spi;
};

extern struct cc2520_gpio_config cc2520_gpio_config[CC2520_GPIO_IDX_LAST_ENTRY];
extern struct cc2520_config cc2520_config;

typedef void (*cc2520_gpio_int_handler_t)(struct device *port, uint32_t pin);

struct device *cc2520_spi_configure(void);
struct cc2520_gpio_config **cc2520_gpio_configure(void);

#define CC2520_GPIO(p)							\
	(((struct device *)&						\
	  ((struct cc2520_config *)					\
	   device_get_binding(CONFIG_CC2520_DRV_NAME)->			\
	   config->config_info)->gpios[CC2520_GPIO_IDX_ ## p]->gpio))

#define CC2520_SPI() \
	(((struct cc2520_config *)			\
	  device_get_binding(CONFIG_CC2520_DRV_NAME)->	\
	  config->config_info)->spi)

static inline bool cc2520_read_fifo_buf(uint8_t *buffer, uint32_t count)
{
	uint8_t data[128 + 1] = { 0xff };
	int ret;

	data[0] = CC2520_INS_RXBUF;

	ret = spi_transceive(CC2520_SPI(), data, sizeof(data),
			     data, sizeof(data));
	memcpy(buffer, data + 1, count);

	return (!ret);
}

static inline bool cc2520_write_fifo_buf(uint8_t *buffer, int count)
{
	uint8_t data[128 + 1];

	if (count > (sizeof(data) - 1)) {
		CC2520_PRINTF("%s: too long data %d, max is %d\n", __func__,
		       count, sizeof(data) - 1);
		return false;
	}

	data[0] = CC2520_INS_TXBUF;
	memcpy(&data[1], buffer, count);

	return (!spi_write(CC2520_SPI(), data, count + 1));
}

static inline bool cc2520_write_reg(uint16_t addr, uint16_t value)
{
	uint8_t data[3];

	data[0] = CC2520_INS_MEMWR | ((addr >> 8) & 0x0f);
	data[1] = addr;
	data[2] = value;

	return (!spi_write(CC2520_SPI(), data, 3));
}

static inline bool cc2520_read_reg(uint16_t addr, uint16_t *value)
{
	uint8_t data[3];
	int ret;

	data[0] = CC2520_INS_MEMRD | ((addr >> 8) & 0x0f);
	data[1] = addr;
	data[2] = 0;

	ret = spi_transceive(CC2520_SPI(), data, 3, data, 3);
	*value = data[2];

	return (!ret);
}

static inline bool cc2520_write_ram(uint8_t *buffer, int addr, int count)
{
	uint8_t data[128 + 1 + 1];

	if (count > (sizeof(data) - 2)) {
		CC2520_PRINTF("%s: too long data %d, max is %d\n",
			      __func__, count, sizeof(data) - 2);
		return false;
	}

	data[0] = CC2520_INS_MEMWR | (addr >> 8);
	data[1] = addr;
	memcpy(&data[2], buffer, count);

	return (!spi_write(CC2520_SPI(), data, count + 2));
}

static inline bool cc2520_read_ram(uint8_t *buffer, int addr, uint32_t count)
{
	uint8_t data[128 + 1 + 1] = { 0 };
	int ret;

	data[0] = CC2520_INS_MEMRD | (addr >> 8);
	data[1] = addr;

	ret = spi_transceive(CC2520_SPI(), data, sizeof(data),
			     data, sizeof(data));
	memcpy(buffer, data + 2, count);

	return (!ret);
}

static inline bool cc2520_get_status(uint8_t *status)
{
	uint8_t data[1];

	data[0] = CC2520_INS_SNOP;

	return (!spi_transceive(CC2520_SPI(), data, 1, status, 1));
}

static inline bool cc2520_strobe(uint8_t strobe)
{
	uint8_t data[1];

	data[0] = strobe;

	return (!spi_write(CC2520_SPI(), data, 1));
}

static inline bool cc2520_strobe_plus_nop(uint8_t strobe)
{
	uint8_t data[2];

	data[0] = strobe;
	data[1] = CC2520_INS_SNOP;

	return (!spi_write(CC2520_SPI(), data, 2));
}

static inline int cc2520_get_fifop(void)
{
	uint32_t value;

	gpio_pin_read(CC2520_GPIO(FIFOP), CONFIG_CC2520_GPIO_FIFOP, &value);

	return value;
}

static inline int cc2520_get_fifo(void)
{
	uint32_t value;

	gpio_pin_read(CC2520_GPIO(FIFO), CONFIG_CC2520_GPIO_FIFO, &value);

	return value;
}

static inline int cc2520_get_sfd(void)
{
	uint32_t value;

	gpio_pin_read(CC2520_GPIO(SFD), CONFIG_CC2520_GPIO_SFD, &value);

	return value;
}

static inline int cc2520_get_cca(void)
{
	uint32_t value;

	gpio_pin_read(CC2520_GPIO(CCA), CONFIG_CC2520_GPIO_CCA, &value);

	return value;
}

static inline void cc2520_set_vreg(int enable)
{
	/* If VREG_EN is connected to VDD, then set the gpio pointer
	 * to NULL and do not configure.
	 */
	if (!CC2520_GPIO(VREG)) {
		return;
	}

	gpio_pin_write(CC2520_GPIO(VREG), CONFIG_CC2520_GPIO_VREG, enable);
}

static inline void cc2520_set_reset(int enable)
{
	gpio_pin_write(CC2520_GPIO(RESET), CONFIG_CC2520_GPIO_RESET, enable);
}

static inline void cc2520_enable_fifop_int(int enable)
{
	CC2520_PRINTF("%s FIFOP\n", enable ? "enable" : "disable");

	if (enable) {
		gpio_pin_enable_callback(CC2520_GPIO(FIFOP),
					 CONFIG_CC2520_GPIO_FIFOP);
	} else {
		gpio_pin_disable_callback(CC2520_GPIO(FIFOP),
					  CONFIG_CC2520_GPIO_FIFOP);
	}
}

static inline void cc2520_clear_fifop_int(void)
{
	/* Do nothing */
}

static inline void cc2520_init_fifop_int(cc2520_gpio_int_handler_t handler)
{
	gpio_set_callback(CC2520_GPIO(FIFOP), handler);
}

#endif /* __CC2520_ARCH_H__ */

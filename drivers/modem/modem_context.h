/** @file
 * @brief Modem context header file.
 *
 * A modem context driver allowing application to handle all
 * aspects of received protocol data.
 */

/*
 * Copyright (c) 2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MODEM_MODEM_CONTEXT_H_
#define ZEPHYR_INCLUDE_DRIVERS_MODEM_MODEM_CONTEXT_H_

#include <kernel.h>
#include <net/buf.h>
#include <net/net_ip.h>
#include <sys/ring_buffer.h>
#include <drivers/gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MODEM_PIN(name_, pin_, flags_) { \
	.dev_name = name_, \
	.pin = pin_, \
	.init_flags = flags_ \
}

struct modem_iface {
	struct device *dev;

	int (*read)(struct modem_iface *iface, uint8_t *buf, size_t size,
		    size_t *bytes_read);
	int (*write)(struct modem_iface *iface, const uint8_t *buf, size_t size);

	/* implementation data */
	void *iface_data;
};

struct modem_cmd_handler {
	void (*process)(struct modem_cmd_handler *cmd_handler,
			struct modem_iface *iface);

	/* implementation data */
	void *cmd_handler_data;
};

struct modem_pin {
	struct device *gpio_port_dev;
	char *dev_name;
	gpio_pin_t pin;
	gpio_flags_t init_flags;
};

struct modem_context {
	/* modem data */
	char *data_manufacturer;
	char *data_model;
	char *data_revision;
	char *data_imei;
#if defined(CONFIG_MODEM_SIM_NUMBERS)
	char *data_imsi;
	char *data_iccid;
#endif
	int   data_rssi;

	/* pin config */
	struct modem_pin *pins;
	size_t pins_len;

	/* interface config */
	struct modem_iface iface;

	/* command handler config */
	struct modem_cmd_handler cmd_handler;

	/* driver data */
	void *driver_data;
};

/**
 * @brief  IP address to string
 *
 * @param  addr: sockaddr to be converted
 *
 * @retval Buffer with IP in string form
 */
char *modem_context_sprint_ip_addr(const struct sockaddr *addr);

/**
 * @brief  Get port from IP address
 *
 * @param  addr: sockaddr
 * @param  port: store port
 *
 * @retval 0 if ok, < 0 if error.
 */
int modem_context_get_addr_port(const struct sockaddr *addr, uint16_t *port);

/**
 * @brief  Gets modem context by id.
 *
 * @param  id: modem context id.
 *
 * @retval modem context or NULL.
 */
struct modem_context *modem_context_from_id(int id);

/**
 * @brief  Finds modem context which owns the iface device.
 *
 * @param  *dev: device used by the modem iface.
 *
 * @retval Modem context or NULL.
 */
struct modem_context *modem_context_from_iface_dev(struct device *dev);

/**
 * @brief  Registers modem context.
 *
 * @note   Prepares modem context to be used.
 *
 * @param  *ctx: modem context to register.
 *
 * @retval 0 if ok, < 0 if error.
 */
int modem_context_register(struct modem_context *ctx);

/* pin config functions */
int modem_pin_read(struct modem_context *ctx, uint32_t pin);
int modem_pin_write(struct modem_context *ctx, uint32_t pin, uint32_t value);
int modem_pin_config(struct modem_context *ctx, uint32_t pin, bool enable);
int modem_pin_init(struct modem_context *ctx);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MODEM_MODEM_CONTEXT_H_ */

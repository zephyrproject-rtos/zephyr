/** @file
 * @brief Modem receiver header file.
 *
 * A modem receiver driver allowing application to handle all
 * aspects of received protocol data.
 */

/*
 * Copyright (c) 2018 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MODEM_MODEM_RECEIVER_H_
#define ZEPHYR_INCLUDE_DRIVERS_MODEM_MODEM_RECEIVER_H_

#include <kernel.h>
#include <sys/ring_buffer.h>

#ifdef __cplusplus
extern "C" {
#endif

struct mdm_receiver_context {
	struct device *uart_dev;

	/* rx data */
	struct ring_buf rx_rb;
	struct k_sem rx_sem;

	/* modem data */
	char *data_manufacturer;
	char *data_model;
	char *data_revision;
#if defined(CONFIG_MODEM_SIM_NUMBERS)
	char *data_imei;
	char *data_imsi;
#endif
	char *data_iccid;
	int   data_rssi;
};

/**
 * @brief  Gets receiver context by id.
 *
 * @param  id: receiver context id.
 *
 * @retval Receiver context or NULL.
 */
struct mdm_receiver_context *mdm_receiver_context_from_id(int id);

/**
 * @brief  Get received data.
 *
 * @param  *ctx: receiver context.
 * @param  *buf: buffer to copy the received data to.
 * @param  size: buffer size.
 * @param  *bytes_read: amount of received bytes
 *
 * @retval 0 if ok, < 0 if error.
 */
int mdm_receiver_recv(struct mdm_receiver_context *ctx,
		      uint8_t *buf, size_t size, size_t *bytes_read);

/**
 * @brief  Sends the data over specified receiver context.
 *
 * @param  *ctx: receiver context.
 * @param  *buf: buffer with the data to send.
 * @param  size: the amount of data to send.
 *
 * @retval 0 if ok, < 0 if error.
 */
int mdm_receiver_send(struct mdm_receiver_context *ctx,
		      const uint8_t *buf, size_t size);

/**
 * @brief  Registers receiver context.
 *
 * @note   Acquires receivers device, and prepares the context to be used.
 *
 * @param  *ctx: receiver context to register.
 * @param  *uart_dev_name: communication device for the receiver context.
 * @param  *buf: rx buffer to use for received data.
 * @param  size: rx buffer size.
 *
 * @retval 0 if ok, < 0 if error.
 */
int mdm_receiver_register(struct mdm_receiver_context *ctx,
			  const char *uart_dev_name,
			  uint8_t *buf, size_t size);

int mdm_receiver_sleep(struct mdm_receiver_context *ctx);

int mdm_receiver_wake(struct mdm_receiver_context *ctx);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MODEM_MODEM_RECEIVER_H_ */

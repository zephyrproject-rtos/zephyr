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

#include <stdlib.h>
#include <kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

struct mdm_receiver_context {
	struct device *uart_dev;

	/* rx data */
	u8_t *uart_pipe_buf;
	size_t uart_pipe_size;
	struct k_pipe uart_pipe;
	struct k_sem rx_sem;

	/* modem data */
	char *data_manufacturer;
	char *data_model;
	char *data_revision;
	char *data_imei;
	int   data_rssi;
};

struct mdm_receiver_context *mdm_receiver_context_from_id(int id);

int mdm_receiver_recv(struct mdm_receiver_context *ctx,
		      u8_t *buf, size_t size, size_t *bytes_read);
int mdm_receiver_send(struct mdm_receiver_context *ctx,
		      const u8_t *buf, size_t size);
int mdm_receiver_register(struct mdm_receiver_context *ctx,
			  const char *uart_dev_name,
			  u8_t *buf, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MODEM_MODEM_RECEIVER_H_ */

/*
 * Copyright (c) 2020 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/ec_host_cmd_periph.h>
#include <syscall_handler.h>

static inline void
z_vrfy_ec_host_cmd_periph_init(const struct device *dev,
			       struct ec_host_cmd_periph_rx_ctx *rx_ctx)
{
	struct ec_host_cmd_periph_rx_ctx local_rx_ctx;

	Z_OOPS(Z_SYSCALL_OBJ_INIT(dev, K_OBJ_DRIVER_EC_HOST_CMD_PERIPH_API));

	z_impl_host_cmd_periph_init(dev, &local_rx_ctx);

	Z_OOPS(z_user_to_copy(&local_rx_ctx, rx_ctx, sizeof(*rx_ctx)));
}
#include <syscalls/ec_host_cmd_periph_init_mrsh.c>

static inline void
z_vrfy_ec_host_cmd_periph_send(const struct device *dev,
			       const struct ec_host_cmd_periph_tx_buf *tx_buf)
{
	struct ec_host_cmd_periph_tx_buf local_tx_buf;

	Z_OOPS(Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_EC_HOST_CMD_PERIPH_API));
	Z_OOPS(z_user_from_copy(&local_tx_buf, tx_buf, sizeof(*tx_buf)));

	/* Ensure that user thread has acces to read buffer since
	 * device will read from this memory location.
	 */
	Z_OOPS(Z_SYSCALL_MEMORY_READ(local_tx_buf.buf, local_tx_buf.size));

	z_impl_host_cmd_periph_send(dev, &local_tx_buf);
}
#include <syscalls/ec_host_cmd_periph_init_mrsh.c>

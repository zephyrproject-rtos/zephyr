/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/internal/syscall_handler.h>
#include <zephyr/drivers/pulse_io.h>
#include <zephyr/sys/util.h>

/* The tx/rx request union aliases pulse_symbol and pulse_cell; validate
 * the user buffer against the larger element so either mode is covered.
 */
#define PULSE_IO_ELEM_SIZE MAX(sizeof(struct pulse_symbol), sizeof(struct pulse_cell))

static inline int z_vrfy_pulse_io_get_capabilities(const struct device *dev,
						   struct pulse_io_caps *caps)
{
	K_OOPS(K_SYSCALL_DRIVER_PULSE_IO(dev, get_capabilities));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(caps, sizeof(*caps)));
	return z_impl_pulse_io_get_capabilities((const struct device *)dev, caps);
}
#include <zephyr/syscalls/pulse_io_get_capabilities_mrsh.c>

static inline int z_vrfy_pulse_io_channel_get(const struct device *dev, uint8_t channel_idx,
					      struct pulse_io_channel **chan)
{
	K_OOPS(K_SYSCALL_DRIVER_PULSE_IO(dev, channel_get));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(chan, sizeof(*chan)));
	return z_impl_pulse_io_channel_get((const struct device *)dev, channel_idx, chan);
}
#include <zephyr/syscalls/pulse_io_channel_get_mrsh.c>

static inline int z_vrfy_pulse_io_channel_release(const struct device *dev,
						  struct pulse_io_channel *chan)
{
	K_OOPS(K_SYSCALL_DRIVER_PULSE_IO(dev, channel_release));
	return z_impl_pulse_io_channel_release((const struct device *)dev, chan);
}
#include <zephyr/syscalls/pulse_io_channel_release_mrsh.c>

static inline int z_vrfy_pulse_io_channel_configure(const struct device *dev,
						    struct pulse_io_channel *chan,
						    const struct pulse_io_config *cfg)
{
	struct pulse_io_config cfg_copy;

	K_OOPS(K_SYSCALL_DRIVER_PULSE_IO(dev, channel_configure));
	K_OOPS(k_usermode_from_copy(&cfg_copy, cfg, sizeof(cfg_copy)));
	return z_impl_pulse_io_channel_configure((const struct device *)dev, chan,
						 (const struct pulse_io_config *)&cfg_copy);
}
#include <zephyr/syscalls/pulse_io_channel_configure_mrsh.c>

static inline int z_vrfy_pulse_io_transmit_sync(const struct device *dev,
						struct pulse_io_channel *chan,
						const struct pulse_io_tx_req *req,
						k_timeout_t timeout)
{
	struct pulse_io_tx_req req_copy;

	K_OOPS(K_SYSCALL_DRIVER_PULSE_IO(dev, transmit_sync));
	K_OOPS(k_usermode_from_copy(&req_copy, req, sizeof(req_copy)));
	K_OOPS(K_SYSCALL_MEMORY_ARRAY_READ(req_copy.symbols, req_copy.count, PULSE_IO_ELEM_SIZE));
	return z_impl_pulse_io_transmit_sync((const struct device *)dev, chan,
					     (const struct pulse_io_tx_req *)&req_copy, timeout);
}
#include <zephyr/syscalls/pulse_io_transmit_sync_mrsh.c>

static inline int z_vrfy_pulse_io_receive_sync(const struct device *dev,
					       struct pulse_io_channel *chan,
					       const struct pulse_io_rx_req *req, size_t *count,
					       k_timeout_t timeout)
{
	struct pulse_io_rx_req req_copy;

	K_OOPS(K_SYSCALL_DRIVER_PULSE_IO(dev, receive_sync));
	K_OOPS(k_usermode_from_copy(&req_copy, req, sizeof(req_copy)));
	K_OOPS(K_SYSCALL_MEMORY_ARRAY_WRITE(req_copy.symbols, req_copy.capacity,
					    PULSE_IO_ELEM_SIZE));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(count, sizeof(*count)));
	return z_impl_pulse_io_receive_sync((const struct device *)dev, chan,
					    (const struct pulse_io_rx_req *)&req_copy, count,
					    timeout);
}
#include <zephyr/syscalls/pulse_io_receive_sync_mrsh.c>

static inline int z_vrfy_pulse_io_stop(const struct device *dev, struct pulse_io_channel *chan)
{
	K_OOPS(K_SYSCALL_DRIVER_PULSE_IO(dev, stop));
	return z_impl_pulse_io_stop((const struct device *)dev, chan);
}
#include <zephyr/syscalls/pulse_io_stop_mrsh.c>

static inline int z_vrfy_pulse_io_get_encoder(const struct device *dev,
					      const struct pulse_io_encoder_api **api)
{
	K_OOPS(K_SYSCALL_DRIVER_PULSE_IO(dev, get_encoder));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(api, sizeof(*api)));
	return z_impl_pulse_io_get_encoder((const struct device *)dev, api);
}
#include <zephyr/syscalls/pulse_io_get_encoder_mrsh.c>

static inline int z_vrfy_pulse_io_get_decoder(const struct device *dev,
					      const struct pulse_io_decoder_api **api)
{
	K_OOPS(K_SYSCALL_DRIVER_PULSE_IO(dev, get_decoder));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(api, sizeof(*api)));
	return z_impl_pulse_io_get_decoder((const struct device *)dev, api);
}
#include <zephyr/syscalls/pulse_io_get_decoder_mrsh.c>

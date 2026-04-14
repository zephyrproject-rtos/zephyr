/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZBUS_PROXY_AGENT_UART_H_
#define ZEPHYR_INCLUDE_ZBUS_PROXY_AGENT_UART_H_

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/util.h>
#include <zephyr/zbus/proxy_agent/zbus_proxy_agent.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file zbus_proxy_agent_uart.h
 * @brief Zbus UART proxy backend API
 * @defgroup zbus_proxy_agent_uart_backend Zbus Proxy Agent UART Backend
 * @ingroup zbus_proxy_agent
 *
 * @details The UART backend serializes proxy messages onto a byte stream using
 * a compact wire format and wraps each record in a COBS frame terminated by
 * ``0x00``. Each frame contains:
 *
 * - ``message_size`` encoded as little-endian ``uint32_t``
 * - ``channel_name_len`` encoded as little-endian ``uint32_t``
 * - ``message_size`` bytes of payload
 * - ``channel_name_len`` bytes of channel name including the NUL terminator
 *
 * Communicating domains must keep channel names and message structure layouts
 * compatible. The current wire format uses little-endian length fields.
 * @{
 */

#define ZBUS_PROXY_AGENT_UART_WIRE_HEADER_SIZE (sizeof(uint32_t) * 2U)
#define ZBUS_PROXY_AGENT_UART_MAX_DECODED_FRAME_SIZE                                            \
	(ZBUS_PROXY_AGENT_UART_WIRE_HEADER_SIZE + CONFIG_ZBUS_PROXY_AGENT_MAX_MESSAGE_SIZE +    \
	 CONFIG_ZBUS_PROXY_AGENT_MAX_CHANNEL_NAME_SIZE)
#define ZBUS_PROXY_AGENT_UART_MAX_ENCODED_FRAME_SIZE                                            \
	(ZBUS_PROXY_AGENT_UART_MAX_DECODED_FRAME_SIZE +                                         \
	 (ZBUS_PROXY_AGENT_UART_MAX_DECODED_FRAME_SIZE / 254U) + 2U)
#define ZBUS_PROXY_AGENT_UART_RX_RING_BUF_SIZE (ZBUS_PROXY_AGENT_UART_MAX_ENCODED_FRAME_SIZE * 4U)

/**
 * @brief Mutable runtime data for the UART backend of a Zbus proxy agent.
 */
struct zbus_proxy_agent_uart_data {
	/** Work item that drains the ISR ring buffer on the agent's configured workqueue */
	struct zbus_proxy_agent_work_ctx rx_work;
	/** Ring buffer used to absorb UART bytes from the ISR */
	struct ring_buf rx_ringbuf;
	/** TX lock used to serialize framed writes */
	struct k_mutex tx_lock;
	/** Callback function for received data */
	zbus_proxy_agent_recv_cb_t recv_cb;
	/** Pointer to proxy agent config, used as user data for the receive callback */
	const struct zbus_proxy_agent *recv_cb_config_ptr;
	/** Proxy agent instance owning this UART backend */
	const struct zbus_proxy_agent *agent;
	/** Number of bytes currently buffered for the in-progress encoded frame */
	size_t rx_frame_len;
	/** Drop incoming bytes until the next delimiter */
	bool rx_drop_until_delimiter;
	/** Set when ISR bytes were dropped and the current frame must be discarded */
	atomic_t rx_overrun;
	/** 1 once backend init completes successfully, otherwise 0 */
	uint8_t initialized;
	/** Storage backing the ISR receive ring buffer */
	uint8_t rx_ringbuf_storage[ZBUS_PROXY_AGENT_UART_RX_RING_BUF_SIZE];
	/** Temporary storage for one encoded COBS frame without the trailing delimiter */
	uint8_t rx_frame[ZBUS_PROXY_AGENT_UART_MAX_ENCODED_FRAME_SIZE];
	/** Temporary storage for one encoded COBS frame including the trailing delimiter */
	uint8_t tx_frame[ZBUS_PROXY_AGENT_UART_MAX_ENCODED_FRAME_SIZE];
	/** Temporary storage for one decoded wire-format frame */
	uint8_t decoded_frame[ZBUS_PROXY_AGENT_UART_MAX_DECODED_FRAME_SIZE];
};

/**
 * @brief Configuration structure for the UART backend of a Zbus proxy agent.
 */
struct zbus_proxy_agent_uart_config {
	/** UART device */
	const struct device *dev;
	/** Pointer to mutable UART backend runtime data */
	struct zbus_proxy_agent_uart_data *data;
};

/** @cond INTERNAL_HIDDEN */

#define _ZBUS_PROXY_AGENT_UART_CONFIG_DEFINE(_name, _dev)                                          \
	static struct zbus_proxy_agent_uart_data _name##_uart_data;                              \
	static const struct zbus_proxy_agent_uart_config _name##_uart_config = {                 \
		.dev = (_dev),                                                                    \
		.data = &_name##_uart_data,                                                      \
	};

#define _ZBUS_PROXY_AGENT_UART_CONFIG(_name) (&_name##_uart_config)

extern const struct zbus_proxy_agent_backend_api zbus_proxy_agent_uart_backend_api;

#define _ZBUS_PROXY_AGENT_UART_API (&zbus_proxy_agent_uart_backend_api)

/**
 * @brief Define a UART backend instance and its proxy agent in code.
 *
 * The proxy agent itself is a software construct and is therefore instantiated in
 * code. Devicetree may still be used to select the underlying UART hardware, for
 * example with ``DEVICE_DT_GET(DT_CHOSEN(zbus_proxy_agent_uart))`` or a phandle
 * stored under ``zephyr,user``. Queued proxy-agent work runs on Zephyr's system
 * workqueue by default.
 *
 * @param _name Proxy agent instance name
 * @param _dev Device pointer expression for the UART used by this backend
 */
#define ZBUS_PROXY_AGENT_UART_DEFINE(_name, _dev)                                                 \
	_ZBUS_PROXY_AGENT_UART_CONFIG_DEFINE(_name, _dev)                                     \
	_ZBUS_PROXY_AGENT_INSTANCE_DEFINE(_name, _ZBUS_PROXY_AGENT_UART_CONFIG(_name),        \
					  _ZBUS_PROXY_AGENT_UART_API)

/**
 * @brief Define a UART backend instance and its proxy agent on a specific workqueue.
 *
 * @param _name Proxy agent instance name
 * @param _dev Device pointer expression for the UART used by this backend
 * @param _work_q Workqueue used for queued proxy-agent RX/TX processing
 */
#define ZBUS_PROXY_AGENT_UART_DEFINE_WITH_WORKQ(_name, _dev, _work_q)                       \
	_ZBUS_PROXY_AGENT_UART_CONFIG_DEFINE(_name, _dev)                                   \
	_ZBUS_PROXY_AGENT_INSTANCE_DEFINE_WITH_WORKQ(                                       \
		_name, _ZBUS_PROXY_AGENT_UART_CONFIG(_name), _ZBUS_PROXY_AGENT_UART_API,    \
		_work_q)

/** @endcond */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZBUS_PROXY_AGENT_UART_H_ */

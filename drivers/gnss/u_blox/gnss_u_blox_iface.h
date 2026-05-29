/*
 * Copyright (c) 2024 NXP
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _GNSS_U_BLOX_IFACE_H_
#define _GNSS_U_BLOX_IFACE_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gnss.h>
#include <zephyr/modem/ubx.h>
#include <zephyr/modem/backend/uart.h>
#include <zephyr/drivers/gpio.h>

#include "gnss_ubx_common.h"

struct u_blox_iface_config {
	const struct device *bus;
	struct gpio_dt_spec reset_gpio;
	uint16_t fix_rate_ms;
	struct {
		uint32_t initial;
		uint32_t desired;
	} baudrate;
};

struct u_blox_iface_data {
	struct gnss_ubx_common_data common_data;
	struct {
		struct modem_pipe *pipe;
		struct modem_backend_uart uart_backend;
		uint8_t receive_buf[1024];
		uint8_t transmit_buf[256];
	} backend;
	struct {
		struct modem_ubx inst;
		uint8_t receive_buf[1024];
	} ubx;
	struct {
		struct modem_ubx_script inst;
		uint8_t response_buf[512];
		uint8_t request_buf[256];
		struct k_sem req_buf_lock;
		struct k_sem lock;
	} script;
#if CONFIG_GNSS_SATELLITES
	struct gnss_satellite satellites[CONFIG_GNSS_U_BLOX_SATELLITES_COUNT];
#endif
};

/**
 * @brief Initialize the UBX interface, modem backend, and register unsollicited messages.
 *
 * Must be called before any other APIs can be used.
 *
 * @param[in] dev		GNSS device instance.
 * @param[in] unsol		Unsolicited message array, used to initialize the
 *				backend.
 * @param[in] unsol_size	Number of entries in @a unsol.
 * @param[in] valset_supported	Use UBX-CFG-VALSET when true to configure the baudrate,
 *				otherwise use the legacy UBX-CFG-PRT.
 *
 * @return 0 on success, negative errno on failure.
 */
int u_blox_iface_init(const struct device *dev, const struct modem_ubx_match *unsol,
		      size_t unsol_size, bool valset_supported);

/**
 * @brief Send a UBX formated request and retrieve the response.
 *
 * @param[in] dev		GNSS device instance.
 * @param[in] req		UBX request frame.
 * @param[in] len		Request frame length.
 * @param[out] rsp		Response payload buffer.
 * @param[in] min_rsp_size	Size of the @a rsp buffer.
 *
 * @return 0 on success, negative errno on failure.
 */
int u_blox_iface_msg_get(const struct device *dev, const struct ubx_frame *req,
			 size_t len, void *rsp, size_t min_rsp_size);

/**
 * @brief Send a UBX formated message
 *
 * @param[in] dev		GNSS device instance.
 * @param[in] req		UBX request frame.
 * @param[in] len		Request frame length.
 * @param[in] wait_for_ack	When set to true, will wait for ACK.
 *
 * @return 0 on success, negative errno on failure.
 */
int u_blox_iface_msg_send(const struct device *dev, const struct ubx_frame *req,
			  size_t len, bool wait_for_ack);

/**
 * @brief Format a payload in a UBX request and send.
 *
 * @param[in] dev		GNSS device instance.
 * @param[in] class_id		UBX message class.
 * @param[in] msg_id		UBX message ID.
 * @param[in] payload		Payload buffer.
 * @param[in] payload_size	Payload size in bytes.
 * @param[in] wait_for_ack	When set to true, will wait for ACK
 *
 * @return 0 on success, negative errno on failure.
 */
int u_blox_iface_msg_payload_send(const struct device *dev, uint8_t class_id, uint8_t msg_id,
				  const uint8_t *payload, size_t payload_size, bool wait_for_ack);

#endif /* _GNSS_U_BLOX_IFACE_H_ */

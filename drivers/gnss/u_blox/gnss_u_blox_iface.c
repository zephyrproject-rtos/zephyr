/*
 * Copyright (c) 2024 NXP
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <zephyr/modem/ubx/protocol.h>
#include <zephyr/modem/ubx/keys.h>

#include "gnss_u_blox_iface.h"

LOG_MODULE_REGISTER(u_blox_iface, CONFIG_GNSS_LOG_LEVEL);

#define RESET_PULSE_MS 100

static void init_match(struct u_blox_iface_data *data, const struct device *gnss)
{
	struct gnss_ubx_common_config match_config = {
		.gnss = gnss,
	};

#if CONFIG_GNSS_SATELLITES
	match_config.satellites.buf = data->satellites;
	match_config.satellites.size = ARRAY_SIZE(data->satellites);
#endif

	gnss_ubx_common_init(&data->common_data, &match_config);
}

static void set_baudrate_with_valset(const struct device *dev)
{
	const struct u_blox_iface_config *cfg = dev->config;

	struct ubx_cfg_val_u32 baudrate = {
		.hdr = {
			.ver = UBX_CFG_VAL_VER_SIMPLE,
			.layer = 1,
		},
		.key = UBX_KEY_CFG_UART1_BAUDRATE,
		.value = cfg->baudrate.desired,
	};

	(void)u_blox_iface_msg_payload_send(dev, UBX_CLASS_ID_CFG, UBX_MSG_ID_CFG_VAL_SET,
					(const uint8_t *)&baudrate, sizeof(baudrate), true);

}

static void set_baudrate_with_cfg_prt(const struct device *dev)
{
	const struct u_blox_iface_config *cfg = dev->config;

	struct ubx_cfg_prt port_config = {
		.port_id = UBX_CFG_PORT_ID_UART,
		.baudrate = cfg->baudrate.desired,
		.mode = UBX_CFG_PRT_MODE_CHAR_LEN(UBX_CFG_PRT_PORT_MODE_CHAR_LEN_8) |
			UBX_CFG_PRT_MODE_PARITY(UBX_CFG_PRT_PORT_MODE_PARITY_NONE) |
			UBX_CFG_PRT_MODE_STOP_BITS(UBX_CFG_PRT_PORT_MODE_STOP_BITS_1),
		.in_proto_mask = UBX_CFG_PRT_PROTO_MASK_UBX,
		.out_proto_mask = UBX_CFG_PRT_PROTO_MASK_UBX,
	};

	(void)u_blox_iface_msg_payload_send(dev, UBX_CLASS_ID_CFG, UBX_MSG_ID_CFG_PRT,
					    (const uint8_t *)&port_config,
					    sizeof(port_config), true);
}

static int configure_baudrate(const struct device *dev, bool valset_supported)
{
	int err = 0;
	const struct u_blox_iface_config *cfg = dev->config;
	struct uart_config uart_cfg;

	err = uart_config_get(cfg->bus, &uart_cfg);
	if (err < 0) {
		LOG_ERR("Failed to get UART config: %d", err);
		return err;
	}

	uart_cfg.baudrate = cfg->baudrate.initial;
	err = uart_configure(cfg->bus, &uart_cfg);
	if (err < 0) {
		LOG_ERR("Failed to configure UART: %d", err);
	}

	if (valset_supported) {
		set_baudrate_with_valset(dev);
	} else {
		set_baudrate_with_cfg_prt(dev);
	}

	uart_cfg.baudrate = cfg->baudrate.desired;

	err = uart_configure(cfg->bus, &uart_cfg);
	if (err < 0) {
		LOG_ERR("Failed to configure UART: %d", err);
	}

	return err;
}

static int init_modem(struct u_blox_iface_data *data, const struct u_blox_iface_config *cfg,
			 const struct modem_ubx_match *unsol, size_t unsol_size)
{
	int err;

	const struct modem_ubx_config ubx_config = {
		.user_data = (void *)&data->common_data,
		.receive_buf = data->ubx.receive_buf,
		.receive_buf_size = sizeof(data->ubx.receive_buf),
		.unsol_matches = {
			.array = unsol,
			.size = unsol_size,
		},
	};

	const struct modem_backend_uart_config uart_backend_config = {
		.uart = cfg->bus,
		.receive_buf = data->backend.receive_buf,
		.receive_buf_size = sizeof(data->backend.receive_buf),
		.transmit_buf = data->backend.transmit_buf,
		.transmit_buf_size = sizeof(data->backend.transmit_buf),
	};

	(void)modem_ubx_init(&data->ubx.inst, &ubx_config);

	data->backend.pipe = modem_backend_uart_init(&data->backend.uart_backend,
						     &uart_backend_config);
	err = modem_pipe_open(data->backend.pipe, K_SECONDS(1));
	if (err != 0) {
		LOG_ERR("Failed to open Modem pipe: %d", err);
		return err;
	}

	err = modem_ubx_attach(&data->ubx.inst, data->backend.pipe);
	if (err != 0) {
		LOG_ERR("Failed to attach UBX inst to modem pipe: %d", err);
		return err;
	}

	(void)k_sem_init(&data->script.lock, 1, 1);
	(void)k_sem_init(&data->script.req_buf_lock, 1, 1);

	data->script.inst.response.buf = data->script.response_buf;
	data->script.inst.response.buf_len = sizeof(data->script.response_buf);

	return err;
}

static int reattach_modem(struct u_blox_iface_data *data)
{
	int err;

	(void)modem_ubx_release(&data->ubx.inst);
	(void)modem_pipe_close(data->backend.pipe, K_SECONDS(1));

	err = modem_pipe_open(data->backend.pipe, K_SECONDS(1));
	if (err != 0) {
		LOG_ERR("Failed to re-open modem pipe: %d", err);
		return err;
	}

	err = modem_ubx_attach(&data->ubx.inst, data->backend.pipe);
	if (err != 0) {
		LOG_ERR("Failed to re-attach modem pipe to UBX inst: %d", err);
	}

	return 0;
}

static int msg_get(const struct device *dev, const struct ubx_frame *req,
		   size_t len, void *rsp, size_t min_rsp_size,
		   k_timeout_t timeout, uint16_t retry_count)
{
	struct u_blox_iface_data *data = dev->data;
	struct ubx_frame *rsp_frame = (struct ubx_frame *)data->script.inst.response.buf;
	int err;

	err = k_sem_take(&data->script.lock, K_SECONDS(3));
	if (err != 0) {
		LOG_ERR("Failed to take script lock: %d", err);
		return err;
	}

	data->script.inst.timeout = timeout;
	data->script.inst.retry_count = retry_count;
	data->script.inst.match.filter.class = req->class;
	data->script.inst.match.filter.id = req->id;
	data->script.inst.request.buf = req;
	data->script.inst.request.len = len;

	err = modem_ubx_run_script(&data->ubx.inst, &data->script.inst);
	if (err != 0 || (data->script.inst.response.buf_len < UBX_FRAME_SZ(min_rsp_size))) {
		err = -EIO;
	} else {
		memcpy(rsp, rsp_frame->payload_and_checksum, min_rsp_size);
	}

	k_sem_give(&data->script.lock);

	return err;
}

#ifdef CONFIG_GNSS_U_BLOX_RESET_ON_INIT
static int reset_modem(const struct device *dev)
{
	const struct u_blox_iface_config *cfg = dev->config;

	if (cfg->reset_gpio.port == NULL) {
		return 0;
	}

	(void)gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_ACTIVE);
	k_sleep(K_MSEC(RESET_PULSE_MS));
	(void)gpio_pin_set_dt(&cfg->reset_gpio, 0);

	return 0;
}
#endif

int u_blox_iface_init(const struct device *dev, const struct modem_ubx_match *unsol,
		      size_t unsol_size, bool valset_supported)
{
	int err;
	struct u_blox_iface_data *data = dev->data;
	const struct u_blox_iface_config *cfg = dev->config;
	uint8_t mon_ver_buf[UBX_FRAME_SZ(0)];
	int mon_ver_len;
	struct ubx_mon_ver ver;

	init_match(data, dev);

#ifdef CONFIG_GNSS_U_BLOX_RESET_ON_INIT
	reset_modem(dev);
#endif

	err = init_modem(data, cfg, unsol, unsol_size);
	if (err < 0) {
		LOG_ERR("Failed to initialize modem: %d", err);
		return err;
	}

	mon_ver_len = ubx_frame_encode(UBX_CLASS_ID_MON, UBX_MSG_ID_MON_VER,
			NULL, 0, mon_ver_buf, sizeof(mon_ver_buf));
	if (mon_ver_len < 0) {
		return mon_ver_len;
	}

	/* Poll the receiver for the version, if it is successful then the baudrate is
	 * already the desired one and there is nothing more to do.
	 */
	err = msg_get(dev, (struct ubx_frame *)mon_ver_buf, mon_ver_len,
		      (void *)&ver, sizeof(ver), K_MSEC(200), 0);
	if (err == 0) {
		return 0;
	}

	err = configure_baudrate(dev, valset_supported);
	if (err < 0) {
		LOG_ERR("Failed to configure baud-rate: %d", err);
		return err;
	}

	err = reattach_modem(data);
	if (err < 0) {
		LOG_ERR("Failed to re-attach modem: %d", err);
		return err;
	}

	return 0;
}

int u_blox_iface_msg_get(const struct device *dev, const struct ubx_frame *req,
			 size_t len, void *rsp, size_t min_rsp_size)
{
	return msg_get(dev, req, len, rsp, min_rsp_size, K_SECONDS(3), 2);
}

int u_blox_iface_msg_send(const struct device *dev, const struct ubx_frame *req,
			  size_t len, bool wait_for_ack)
{
	struct u_blox_iface_data *data = dev->data;
	int err;

	err = k_sem_take(&data->script.lock, K_SECONDS(3));
	if (err != 0) {
		LOG_ERR("Failed to take script lock: %d", err);
		return err;
	}

	data->script.inst.timeout = K_SECONDS(3);
	data->script.inst.retry_count = wait_for_ack ? 2 : 0;
	data->script.inst.match.filter.class = wait_for_ack ? UBX_CLASS_ID_ACK : 0;
	data->script.inst.match.filter.id = UBX_MSG_ID_ACK;
	data->script.inst.request.buf = (const void *)req;
	data->script.inst.request.len = len;

	err = modem_ubx_run_script(&data->ubx.inst, &data->script.inst);

	k_sem_give(&data->script.lock);

	return err;
}

int u_blox_iface_msg_payload_send(const struct device *dev, uint8_t class_id, uint8_t msg_id,
			       const uint8_t *payload, size_t payload_size, bool wait_for_ack)
{
	struct u_blox_iface_data *data = dev->data;
	struct ubx_frame *frame = (struct ubx_frame *)data->script.request_buf;
	int err;

	err = k_sem_take(&data->script.req_buf_lock, K_SECONDS(3));
	if (err != 0) {
		LOG_ERR("Failed to take script lock: %d", err);
		return err;
	}

	err = ubx_frame_encode(class_id, msg_id, payload, payload_size,
			       (uint8_t *)frame, sizeof(data->script.request_buf));
	if (err > 0) {
		err = u_blox_iface_msg_send(dev, frame, err, wait_for_ack);
	}

	k_sem_give(&data->script.req_buf_lock);

	return err;
}

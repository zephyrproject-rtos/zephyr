/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This file is based on mb.c and mb_util.c from uC/Modbus Stack.
 *
 *                                uC/Modbus
 *                         The Embedded Modbus Stack
 *
 *      Copyright 2003-2020 Silicon Laboratories Inc. www.silabs.com
 *
 *                   SPDX-License-Identifier: Apache-2.0
 *
 * This software is subject to an open source license and is distributed by
 *  Silicon Laboratories Inc. pursuant to the terms of the Apache License,
 *      Version 2.0 available at www.apache.org/licenses/LICENSE-2.0.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(modbus_serial, CONFIG_MODBUS_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include <modbus_internal.h>

static void modbus_serial_tx_on(struct modbus_context *ctx)
{
	struct modbus_serial_config *cfg = ctx->cfg;

	if (cfg->de != NULL) {
		gpio_pin_set(cfg->de->port, cfg->de->pin, 1);
	}

	uart_irq_tx_enable(cfg->dev);
}

static void modbus_serial_tx_off(struct modbus_context *ctx)
{
	struct modbus_serial_config *cfg = ctx->cfg;

	uart_irq_tx_disable(cfg->dev);
	if (cfg->de != NULL) {
		gpio_pin_set(cfg->de->port, cfg->de->pin, 0);
	}
}

static void modbus_serial_rx_fifo_drain(struct modbus_context *ctx)
{
	struct modbus_serial_config *cfg = ctx->cfg;
	uint8_t buf[8];
	int n;

	do {
		n = uart_fifo_read(cfg->dev, buf, sizeof(buf));
	} while (n == sizeof(buf));
}

static void modbus_serial_rx_on(struct modbus_context *ctx)
{
	struct modbus_serial_config *cfg = ctx->cfg;

	if (cfg->re != NULL) {
		gpio_pin_set(cfg->re->port, cfg->re->pin, 1);
	}

	atomic_set_bit(&ctx->state, MODBUS_STATE_RX_ENABLED);
	uart_irq_rx_enable(cfg->dev);
}

static void modbus_serial_rx_off(struct modbus_context *ctx)
{
	struct modbus_serial_config *cfg = ctx->cfg;

	uart_irq_rx_disable(cfg->dev);
	atomic_clear_bit(&ctx->state, MODBUS_STATE_RX_ENABLED);

	if (cfg->re != NULL) {
		gpio_pin_set(cfg->re->port, cfg->re->pin, 0);
	}
}

#ifdef CONFIG_MODBUS_ASCII_MODE
/* The function calculates an 8-bit Longitudinal Redundancy Check. */
static uint8_t modbus_ascii_get_lrc(uint8_t *src, size_t length)
{
	uint8_t lrc = 0;
	uint8_t tmp;
	uint8_t *pblock = src;

	while (length-- > 0) {
		/* Add the data byte to LRC, increment data pointer. */
		if (hex2bin(pblock, 2, &tmp, sizeof(tmp)) != sizeof(tmp)) {
			return 0;
		}

		lrc += tmp;
		pblock += 2;
	}

	/* Two complement the binary sum */
	lrc = ~lrc + 1;

	return lrc;
}

/* Parses and converts an ASCII mode frame into a Modbus RTU frame. */
static int modbus_ascii_rx_adu(struct modbus_context *ctx)
{
	struct modbus_serial_config *cfg = ctx->cfg;
	uint8_t *pmsg;
	uint8_t *prx_data;
	uint16_t rx_size;
	uint8_t frame_lrc;
	uint8_t calc_lrc;

	rx_size =  cfg->uart_buf_ctr;
	prx_data = &ctx->rx_adu.data[0];

	if (!(rx_size & 0x01)) {
		LOG_WRN("Message should have an odd number of bytes");
		return -EMSGSIZE;
	}

	if (rx_size < MODBUS_ASCII_MIN_MSG_SIZE) {
		LOG_WRN("Frame length error");
		return -EMSGSIZE;
	}

	if ((cfg->uart_buf[0] != MODBUS_ASCII_START_FRAME_CHAR) ||
	    (cfg->uart_buf[rx_size - 2] != MODBUS_ASCII_END_FRAME_CHAR1) ||
	    (cfg->uart_buf[rx_size - 1] != MODBUS_ASCII_END_FRAME_CHAR2)) {
		LOG_WRN("Frame character error");
		return -EMSGSIZE;
	}

	/* Take away for the ':', CR, and LF */
	rx_size -= 3;
	/* Point past the ':' to the address. */
	pmsg = &cfg->uart_buf[1];

	hex2bin(pmsg, 2, &ctx->rx_adu.unit_id, 1);
	pmsg += 2;
	rx_size -= 2;
	hex2bin(pmsg, 2, &ctx->rx_adu.fc, 1);
	pmsg += 2;
	rx_size -= 2;

	/* Get the data from the message */
	ctx->rx_adu.length = 0;
	while (rx_size > 2) {
		hex2bin(pmsg, 2, prx_data, 1);
		prx_data++;
		pmsg += 2;
		rx_size -= 2;
		/* Increment the number of Modbus packets received */
		ctx->rx_adu.length++;
	}

	/* Extract the message's LRC */
	hex2bin(pmsg, 2, &frame_lrc, 1);
	ctx->rx_adu.crc = frame_lrc;

	/*
	 * The LRC is calculated on the ADDR, FC and Data fields,
	 * not the ':', CR/LF and LRC placed in the message
	 * by the sender. We thus need to subtract 5 'ASCII' characters
	 * from the received message to exclude these.
	 */
	calc_lrc = modbus_ascii_get_lrc(&cfg->uart_buf[1],
					(cfg->uart_buf_ctr - 5) / 2);

	if (calc_lrc != frame_lrc) {
		LOG_ERR("Calculated LRC does not match received LRC");
		return -EIO;
	}

	return 0;
}

static uint8_t *modbus_ascii_bin2hex(uint8_t value, uint8_t *pbuf)
{
	uint8_t u_nibble = (value >> 4) & 0x0F;
	uint8_t l_nibble = value & 0x0F;

	hex2char(u_nibble, pbuf);
	pbuf++;
	hex2char(l_nibble, pbuf);
	pbuf++;

	return pbuf;
}

static void modbus_ascii_tx_adu(struct modbus_context *ctx)
{
	struct modbus_serial_config *cfg = ctx->cfg;
	uint16_t tx_bytes = 0;
	uint8_t lrc;
	uint8_t *pbuf;

	/* Place the start-of-frame character into output buffer */
	cfg->uart_buf[0] = MODBUS_ASCII_START_FRAME_CHAR;
	tx_bytes = 1;

	pbuf = &cfg->uart_buf[1];
	pbuf = modbus_ascii_bin2hex(ctx->tx_adu.unit_id, pbuf);
	tx_bytes += 2;
	pbuf = modbus_ascii_bin2hex(ctx->tx_adu.fc, pbuf);
	tx_bytes += 2;

	for (int i = 0; i < ctx->tx_adu.length; i++) {
		pbuf = modbus_ascii_bin2hex(ctx->tx_adu.data[i], pbuf);
		tx_bytes += 2;
	}

	/*
	 * Add the LRC checksum in the packet.
	 *
	 * The LRC is calculated on the ADDR, FC and Data fields,
	 * not the ':' which was inserted in the uart_buf[].
	 * Thus we subtract 1 ASCII character from the LRC.
	 * The LRC and CR/LF bytes are not YET in the .uart_buf[].
	 */
	lrc = modbus_ascii_get_lrc(&cfg->uart_buf[1], (tx_bytes - 1) / 2);
	pbuf = modbus_ascii_bin2hex(lrc, pbuf);
	tx_bytes += 2;

	*pbuf++ = MODBUS_ASCII_END_FRAME_CHAR1;
	*pbuf++ = MODBUS_ASCII_END_FRAME_CHAR2;
	tx_bytes += 2;

	/* Update the total number of bytes to send */
	cfg->uart_buf_ctr = tx_bytes;
	cfg->uart_buf_ptr = &cfg->uart_buf[0];

	LOG_DBG("Start frame transmission");
	modbus_serial_rx_off(ctx);
	modbus_serial_tx_on(ctx);
}
#else
static int modbus_ascii_rx_adu(struct modbus_context *ctx)
{
	return 0;
}

static void modbus_ascii_tx_adu(struct modbus_context *ctx)
{
}
#endif

/* Copy Modbus RTU frame and check if the CRC is valid. */
static int modbus_rtu_rx_adu(struct modbus_context *ctx)
{
	struct modbus_serial_config *cfg = ctx->cfg;
	uint16_t calc_crc;
	uint16_t crc_idx;
	uint8_t *data_ptr;

	/* Is the message long enough? */
	if ((cfg->uart_buf_ctr < MODBUS_RTU_MIN_MSG_SIZE) ||
	    (cfg->uart_buf_ctr > CONFIG_MODBUS_BUFFER_SIZE)) {
		LOG_WRN("Frame length error");
		return -EMSGSIZE;
	}

	ctx->rx_adu.unit_id = cfg->uart_buf[0];
	ctx->rx_adu.fc = cfg->uart_buf[1];
	data_ptr = &cfg->uart_buf[2];
	/* Payload length without node address, function code, and CRC */
	ctx->rx_adu.length = cfg->uart_buf_ctr - 4;
	/* CRC index */
	crc_idx = cfg->uart_buf_ctr - sizeof(uint16_t);

	memcpy(ctx->rx_adu.data, data_ptr, ctx->rx_adu.length);

	ctx->rx_adu.crc = sys_get_le16(&cfg->uart_buf[crc_idx]);
	/* Calculate CRC over address, function code, and payload */
	calc_crc = crc16_ansi(&cfg->uart_buf[0],
			      cfg->uart_buf_ctr - sizeof(ctx->rx_adu.crc));

	if (ctx->rx_adu.crc != calc_crc) {
		LOG_WRN("Calculated CRC does not match received CRC");
		return -EIO;
	}

	return 0;
}

static void modbus_rtu_tx_adu(struct modbus_context *ctx)
{
	struct modbus_serial_config *cfg = ctx->cfg;
	uint16_t tx_bytes = 0;
	uint8_t *data_ptr;

	cfg->uart_buf[0] = ctx->tx_adu.unit_id;
	cfg->uart_buf[1] = ctx->tx_adu.fc;
	tx_bytes = 2 + ctx->tx_adu.length;
	data_ptr = &cfg->uart_buf[2];

	memcpy(data_ptr, ctx->tx_adu.data, ctx->tx_adu.length);

	ctx->tx_adu.crc = crc16_ansi(&cfg->uart_buf[0], ctx->tx_adu.length + 2);
	sys_put_le16(ctx->tx_adu.crc,
		     &cfg->uart_buf[ctx->tx_adu.length + 2]);
	tx_bytes += 2;

	cfg->uart_buf_ctr = tx_bytes;
	cfg->uart_buf_ptr = &cfg->uart_buf[0];

	LOG_HEXDUMP_DBG(cfg->uart_buf, cfg->uart_buf_ctr, "uart_buf");
	LOG_DBG("Start frame transmission");
	modbus_serial_rx_off(ctx);
	modbus_serial_tx_on(ctx);
}

/*
 * A byte has been received from a serial port. We just store it in the buffer
 * for processing when a complete packet has been received.
 */
static void cb_handler_rx(struct modbus_context *ctx)
{
	struct modbus_serial_config *cfg = ctx->cfg;

	if (!atomic_test_bit(&ctx->state, MODBUS_STATE_RX_ENABLED)) {
		modbus_serial_rx_fifo_drain(ctx);
		return;
	}

	if ((ctx->mode == MODBUS_MODE_ASCII) &&
	    IS_ENABLED(CONFIG_MODBUS_ASCII_MODE)) {
		uint8_t c;

		if (uart_fifo_read(cfg->dev, &c, 1) != 1) {
			LOG_ERR("Failed to read UART");
			return;
		}

		if (c == MODBUS_ASCII_START_FRAME_CHAR) {
			/* Restart a new frame */
			cfg->uart_buf_ptr = &cfg->uart_buf[0];
			cfg->uart_buf_ctr = 0;
		}

		if (cfg->uart_buf_ctr < CONFIG_MODBUS_BUFFER_SIZE) {
			*cfg->uart_buf_ptr++ = c;
			cfg->uart_buf_ctr++;
		}

		if (c == MODBUS_ASCII_END_FRAME_CHAR2) {
			k_work_submit(&ctx->server_work);
		}

	} else {
		int n;

		if (cfg->uart_buf_ctr == CONFIG_MODBUS_BUFFER_SIZE) {
			/* Buffer full. Disable interrupt until timeout. */
			modbus_serial_rx_disable(ctx);
			return;
		}

		/* Restart timer on a new character */
		k_timer_start(&cfg->rtu_timer,
			      K_USEC(cfg->rtu_timeout), K_NO_WAIT);

		n = uart_fifo_read(cfg->dev, cfg->uart_buf_ptr,
				   (CONFIG_MODBUS_BUFFER_SIZE -
				    cfg->uart_buf_ctr));

		cfg->uart_buf_ptr += n;
		cfg->uart_buf_ctr += n;
	}
}

static void cb_handler_tx(struct modbus_context *ctx)
{
	struct modbus_serial_config *cfg = ctx->cfg;
	int n;

	if (cfg->uart_buf_ctr > 0) {
		n = uart_fifo_fill(cfg->dev, cfg->uart_buf_ptr,
				   cfg->uart_buf_ctr);
		cfg->uart_buf_ctr -= n;
		cfg->uart_buf_ptr += n;
		return;
	}

	/* Must wait till the transmission is complete or
	 * RS-485 transceiver could be disabled before all data has
	 * been transmitted and message will be corrupted.
	 */
	if (uart_irq_tx_complete(cfg->dev)) {
		/* Disable transmission */
		cfg->uart_buf_ptr = &cfg->uart_buf[0];
		modbus_serial_tx_off(ctx);
		modbus_serial_rx_fifo_drain(ctx);
		modbus_serial_rx_on(ctx);
	}
}

static void uart_cb_handler(const struct device *dev, void *app_data)
{
	struct modbus_context *ctx = (struct modbus_context *)app_data;

	if (ctx == NULL) {
		LOG_ERR("Modbus hardware is not properly initialized");
		return;
	}

	if (uart_irq_update(dev) && uart_irq_is_pending(dev)) {

		if (uart_irq_rx_ready(dev)) {
			cb_handler_rx(ctx);
		}

		if (uart_irq_tx_ready(dev)) {
			cb_handler_tx(ctx);
		}
	}
}

/* This function is called when the RTU framing timer expires. */
static void rtu_tmr_handler(struct k_timer *t_id)
{
	struct modbus_context *ctx;

	ctx = (struct modbus_context *)k_timer_user_data_get(t_id);

	if (ctx == NULL) {
		LOG_ERR("Failed to get Modbus context");
		return;
	}

	k_work_submit(&ctx->server_work);
}

static int configure_gpio(struct modbus_context *ctx)
{
	struct modbus_serial_config *cfg = ctx->cfg;

	if (cfg->de != NULL) {
		if (!device_is_ready(cfg->de->port)) {
			return -ENODEV;
		}

		if (gpio_pin_configure_dt(cfg->de, GPIO_OUTPUT_INACTIVE)) {
			return -EIO;
		}
	}


	if (cfg->re != NULL) {
		if (!device_is_ready(cfg->re->port)) {
			return -ENODEV;
		}

		if (gpio_pin_configure_dt(cfg->re, GPIO_OUTPUT_INACTIVE)) {
			return -EIO;
		}
	}

	return 0;
}

static inline int configure_uart(struct modbus_context *ctx,
				 struct modbus_iface_param *param)
{
	struct modbus_serial_config *cfg = ctx->cfg;
	struct uart_config uart_cfg = {
		.baudrate = param->serial.baud,
		.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,
	};

	if (ctx->mode == MODBUS_MODE_ASCII) {
		uart_cfg.data_bits = UART_CFG_DATA_BITS_7;
	} else {
		uart_cfg.data_bits = UART_CFG_DATA_BITS_8;
	}

	switch (param->serial.parity) {
	case UART_CFG_PARITY_ODD:
	case UART_CFG_PARITY_EVEN:
		uart_cfg.parity = param->serial.parity;
		uart_cfg.stop_bits = UART_CFG_STOP_BITS_1;
		break;
	case UART_CFG_PARITY_NONE:
		/* Use of no parity requires 2 stop bits */
		uart_cfg.parity = param->serial.parity;
		uart_cfg.stop_bits = UART_CFG_STOP_BITS_2;
		break;
	default:
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_MODBUS_NONCOMPLIANT_SERIAL_MODE)) {
		/* Allow custom stop bit settings only in non-compliant mode */
		switch (param->serial.stop_bits) {
		case UART_CFG_STOP_BITS_0_5:
		case UART_CFG_STOP_BITS_1:
		case UART_CFG_STOP_BITS_1_5:
		case UART_CFG_STOP_BITS_2:
			uart_cfg.stop_bits = param->serial.stop_bits;
			break;
		default:
			return -EINVAL;
		}
	}

	if (uart_configure(cfg->dev, &uart_cfg) != 0) {
		LOG_ERR("Failed to configure UART");
		return -EINVAL;
	}

	return 0;
}

void modbus_serial_rx_disable(struct modbus_context *ctx)
{
	modbus_serial_rx_off(ctx);
}

void modbus_serial_rx_enable(struct modbus_context *ctx)
{
	modbus_serial_rx_on(ctx);
}

int modbus_serial_rx_adu(struct modbus_context *ctx)
{
	struct modbus_serial_config *cfg = ctx->cfg;
	int rc = 0;

	switch (ctx->mode) {
	case MODBUS_MODE_RTU:
		rc = modbus_rtu_rx_adu(ctx);
		break;
	case MODBUS_MODE_ASCII:
		if (!IS_ENABLED(CONFIG_MODBUS_ASCII_MODE)) {
			return -ENOTSUP;
		}

		rc = modbus_ascii_rx_adu(ctx);
		break;
	default:
		LOG_ERR("Unsupported MODBUS mode");
		return -ENOTSUP;
	}

	cfg->uart_buf_ctr = 0;
	cfg->uart_buf_ptr = &cfg->uart_buf[0];

	return rc;
}

int modbus_serial_tx_adu(struct modbus_context *ctx)
{
	switch (ctx->mode) {
	case MODBUS_MODE_RTU:
		modbus_rtu_tx_adu(ctx);
		return 0;
	case MODBUS_MODE_ASCII:
		if (IS_ENABLED(CONFIG_MODBUS_ASCII_MODE)) {
			modbus_ascii_tx_adu(ctx);
			return 0;
		}
	default:
		break;
	}

	return -ENOTSUP;
}

int modbus_serial_init(struct modbus_context *ctx,
		       struct modbus_iface_param param)
{
	struct modbus_serial_config *cfg = ctx->cfg;
	const uint32_t if_delay_max = 3500000;
	const uint32_t numof_bits = 11;
	int err;

	switch (param.mode) {
	case MODBUS_MODE_RTU:
	case MODBUS_MODE_ASCII:
		ctx->mode = param.mode;
		break;
	default:
		return -ENOTSUP;
	}

	if (!device_is_ready(cfg->dev)) {
		LOG_ERR("Bus device %s is not ready", cfg->dev->name);
		return -ENODEV;
	}

	if (IS_ENABLED(CONFIG_UART_USE_RUNTIME_CONFIGURE)) {
		if (configure_uart(ctx, &param) != 0) {
			return -EINVAL;
		}
	}

	if (param.serial.baud == 0) {
		LOG_ERR("Baudrate is 0");
		return -EINVAL;
	}

	if (param.serial.baud <= 38400) {
		cfg->rtu_timeout = (numof_bits * if_delay_max) /
				   param.serial.baud;
	} else {
		cfg->rtu_timeout = (numof_bits * if_delay_max) / 38400;
	}

	if (configure_gpio(ctx) != 0) {
		return -EIO;
	}

	cfg->uart_buf_ctr = 0;
	cfg->uart_buf_ptr = &cfg->uart_buf[0];

	err = uart_irq_callback_user_data_set(cfg->dev, uart_cb_handler, ctx);
	if (err != 0) {
		return err;
	};

	k_timer_init(&cfg->rtu_timer, rtu_tmr_handler, NULL);
	k_timer_user_data_set(&cfg->rtu_timer, ctx);

	modbus_serial_rx_on(ctx);
	LOG_INF("RTU timeout %u us", cfg->rtu_timeout);

	return 0;
}

void modbus_serial_disable(struct modbus_context *ctx)
{
	modbus_serial_tx_off(ctx);
	modbus_serial_rx_off(ctx);
	k_timer_stop(&ctx->cfg->rtu_timer);
}

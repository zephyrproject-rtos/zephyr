/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
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
 *                   SPDX-License-Identifier: APACHE-2.0
 *
 * This software is subject to an open source license and is distributed by
 *  Silicon Laboratories Inc. pursuant to the terms of the Apache License,
 *      Version 2.0 available at www.apache.org/licenses/LICENSE-2.0.
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(mb_rtu, CONFIG_MODBUS_RTU_LOG_LEVEL);

#include <kernel.h>
#include <string.h>
#include <sys/byteorder.h>
#include <mb_rtu_internal.h>

#define DT_DRV_COMPAT zephyr_modbus_serial

#define MB_RTU_DEFINE_GPIO_CFG(n, d)				\
	static struct mb_rtu_gpio_config d##_cfg_##n = {	\
		.name = DT_INST_GPIO_LABEL(n, d),		\
		.pin = DT_INST_GPIO_PIN(n, d),			\
		.flags = DT_INST_GPIO_FLAGS(n,  d),		\
	};

#define MB_RTU_DEFINE_GPIO_CFGS(n)				\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, de_gpios),		\
		    (MB_RTU_DEFINE_GPIO_CFG(n, de_gpios)), ())	\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, re_gpios),		\
		    (MB_RTU_DEFINE_GPIO_CFG(n, re_gpios)), ())

DT_INST_FOREACH_STATUS_OKAY(MB_RTU_DEFINE_GPIO_CFGS)

#define MB_RTU_ASSIGN_GPIO_CFG(n, d)				\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, d),		\
		    (&d##_cfg_##n), (NULL))

#define MODBUS_DT_GET_DEV(n) {					\
		.dev_name = DT_INST_BUS_LABEL(n),		\
		.de = MB_RTU_ASSIGN_GPIO_CFG(n, de_gpios),	\
		.re = MB_RTU_ASSIGN_GPIO_CFG(n, re_gpios),	\
	},

static struct mb_rtu_context mb_ctx_tbl[] = {
	DT_INST_FOREACH_STATUS_OKAY(MODBUS_DT_GET_DEV)
};

static void mb_tx_enable(struct mb_rtu_context *ctx)
{
	if (ctx->de != NULL) {
		gpio_pin_set(ctx->de->dev, ctx->de->pin, 1);
	}

	uart_irq_tx_enable(ctx->dev);
}

static void mb_tx_disable(struct mb_rtu_context *ctx)
{
	uart_irq_tx_disable(ctx->dev);
	if (ctx->de != NULL) {
		gpio_pin_set(ctx->de->dev, ctx->de->pin, 0);
	}
}

static void mb_rx_enable(struct mb_rtu_context *ctx)
{
	if (ctx->re != NULL) {
		gpio_pin_set(ctx->re->dev, ctx->re->pin, 1);
	}

	uart_irq_rx_enable(ctx->dev);
}

static void mb_rx_disable(struct mb_rtu_context *ctx)
{
	uart_irq_rx_disable(ctx->dev);
	if (ctx->re != NULL) {
		gpio_pin_set(ctx->re->dev, ctx->re->pin, 0);
	}
}

#ifdef CONFIG_MODBUS_RTU_ASCII_MODE
/* The function calculates an 8-bit Longitudinal Redundancy Check. */
static uint8_t mb_ascii_get_lrc(uint8_t *src, size_t length)
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
static int mb_rx_ascii_frame(struct mb_rtu_context *ctx)
{
	uint8_t *pmsg;
	uint8_t *prx_data;
	uint16_t rx_size;
	uint8_t frame_lrc;
	uint8_t calc_lrc;

	rx_size =  ctx->uart_buf_ctr;
	prx_data = &ctx->rx_frame.data[0];

	if (!(rx_size & 0x01)) {
		LOG_WRN("Message should have an odd number of bytes");
		return -EMSGSIZE;
	}

	if (rx_size > MODBUS_ASCII_MIN_MSG_SIZE) {
		LOG_WRN("Frame length error");
		return -EMSGSIZE;
	}

	if ((ctx->uart_buf[0] != MODBUS_ASCII_START_FRAME_CHAR) ||
	    (ctx->uart_buf[rx_size - 2] != MODBUS_ASCII_END_FRAME_CHAR1) ||
	    (ctx->uart_buf[rx_size - 1] != MODBUS_ASCII_END_FRAME_CHAR2)) {
		LOG_WRN("Frame character error");
		return -EMSGSIZE;
	}

	/* Take away for the ':', CR, and LF */
	rx_size -= 3;
	/* Point past the ':' to the address. */
	pmsg = &ctx->uart_buf[1];

	hex2bin(pmsg, 2, &ctx->rx_frame.addr, 1);
	pmsg += 2;
	rx_size -= 2;
	hex2bin(pmsg, 2, &ctx->rx_frame.fc, 1);
	pmsg += 2;
	rx_size -= 2;

	/* Get the data from the message */
	ctx->rx_frame.length = 0;
	while (rx_size > 2) {
		hex2bin(pmsg, 2, prx_data, 1);
		prx_data++;
		pmsg += 2;
		rx_size -= 2;
		/* Increment the number of Modbus packets received */
		ctx->rx_frame.length++;
	}

	/* Subtract the Address and function code */
	ctx->rx_frame.length -= 2;
	/* Extract the message's LRC */
	hex2bin(pmsg, 2, &frame_lrc, 1);
	ctx->rx_frame.crc = frame_lrc;

	/*
	 * The LRC is calculated on the ADDR, FC and Data fields,
	 * not the ':', CR/LF and LRC placed in the message
	 * by the sender. We thus need to subtract 5 'ASCII' characters
	 * from the received message to exclude these.
	 */
	calc_lrc = mb_ascii_get_lrc(&ctx->uart_buf[1],
				    (ctx->uart_buf_ctr - 5) / 2);

	ctx->uart_buf_ctr = 0;
	ctx->uart_buf_ptr = &ctx->uart_buf[0];

	if (calc_lrc != frame_lrc) {
		LOG_ERR("Calculated LRC does not match received LRC");
		return -EIO;
	}

	return 0;
}

static uint8_t *mb_bin2hex(uint8_t value, uint8_t *pbuf)
{
	uint8_t u_nibble = (value >> 4) & 0x0F;
	uint8_t l_nibble = value & 0x0F;

	hex2char(u_nibble, pbuf);
	pbuf++;
	hex2char(l_nibble, pbuf);
	pbuf++;

	return pbuf;
}

static void mb_tx_ascii_frame(struct mb_rtu_context *ctx)
{
	uint16_t tx_bytes = 0;
	uint8_t lrc;
	uint8_t *pbuf;

	/* Place the start-of-frame character into output buffer */
	ctx->uart_buf[0] = MODBUS_ASCII_START_FRAME_CHAR;
	tx_bytes = 1;

	pbuf = &ctx->uart_buf[1];
	pbuf = mb_bin2hex(ctx->tx_frame.addr, pbuf);
	tx_bytes += 2;
	pbuf = mb_bin2hex(ctx->tx_frame.fc, pbuf);
	tx_bytes += 2;

	for (int i = 0; i < ctx->tx_frame.length; i++) {
		pbuf = mb_bin2hex(ctx->tx_frame.data[i], pbuf);
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
	lrc = mb_ascii_get_lrc(&ctx->uart_buf[1], (tx_bytes - 1) / 2);
	pbuf = mb_bin2hex(lrc, pbuf);
	tx_bytes += 2;

	*pbuf++ = MODBUS_ASCII_END_FRAME_CHAR1;
	*pbuf++ = MODBUS_ASCII_END_FRAME_CHAR2;
	tx_bytes += 2;

	/* Update the total number of bytes to send */
	ctx->uart_buf_ctr = tx_bytes;
	ctx->uart_buf_ptr = &ctx->uart_buf[0];

	LOG_DBG("Start frame transmission");
	mb_rx_disable(ctx);
	mb_tx_enable(ctx);
}
#else
static int mb_rx_ascii_frame(struct mb_rtu_context *ctx)
{
	return 0;
}

static void mb_tx_ascii_frame(struct mb_rtu_context *ctx)
{
}
#endif

static uint16_t mb_rtu_crc16(uint8_t *src, size_t length)
{
	uint16_t crc = 0xFFFF;
	uint8_t shiftctr;
	bool flag;
	uint8_t *pblock = src;

	while (length > 0) {
		length--;
		crc ^= (uint16_t)*pblock++;
		shiftctr = 8;
		do {
			/* Determine if the shift out of rightmost bit is 1 */
			flag = (crc & 0x0001) ? true : false;
			/* Shift CRC to the right one bit. */
			crc >>= 1;
			/*
			 * If bit shifted out of rightmost bit was a 1
			 * exclusive OR the CRC with the generating polynomial.
			 */
			if (flag == true) {
				crc ^= MODBUS_CRC16_POLY;
			}

			shiftctr--;
		} while (shiftctr > 0);
	}

	return crc;
}

/* Copy Modbus RTU frame and check if the CRC is valid. */
static int mb_rx_rtu_frame(struct mb_rtu_context *ctx)
{
	uint16_t calc_crc;
	uint16_t crc_idx;
	uint8_t *data_ptr;

	/* Is the message long enough? */
	if ((ctx->uart_buf_ctr < MODBUS_RTU_MIN_MSG_SIZE) ||
	    (ctx->uart_buf_ctr > CONFIG_MODBUS_RTU_BUFFER_SIZE)) {
		LOG_WRN("Frame length error");
		return -EMSGSIZE;
	}

	ctx->rx_frame.addr = ctx->uart_buf[0];
	ctx->rx_frame.fc = ctx->uart_buf[1];
	data_ptr = &ctx->uart_buf[2];
	/* Payload length without node address, function code, and CRC */
	ctx->rx_frame.length = ctx->uart_buf_ctr - 4;
	/* CRC index */
	crc_idx = ctx->uart_buf_ctr - sizeof(uint16_t);

	memcpy(ctx->rx_frame.data, data_ptr, ctx->rx_frame.length);

	ctx->rx_frame.crc = sys_get_le16(&ctx->uart_buf[crc_idx]);
	/* Calculate CRC over address, function code, and payload */
	calc_crc = mb_rtu_crc16(&ctx->uart_buf[0],
				ctx->uart_buf_ctr - sizeof(ctx->rx_frame.crc));

	ctx->uart_buf_ctr = 0;
	ctx->uart_buf_ptr = &ctx->uart_buf[0];

	if (ctx->rx_frame.crc != calc_crc) {
		LOG_WRN("Calculated CRC does not match received CRC");
		return -EIO;
	}

	return 0;
}

static void mb_tx_rtu_frame(struct mb_rtu_context *ctx)
{
	uint16_t tx_bytes = 0;
	uint8_t *data_ptr;

	ctx->uart_buf[0] = ctx->tx_frame.addr;
	ctx->uart_buf[1] = ctx->tx_frame.fc;
	tx_bytes = 2 + ctx->tx_frame.length;
	data_ptr = &ctx->uart_buf[2];

	memcpy(data_ptr, ctx->tx_frame.data, ctx->tx_frame.length);

	ctx->tx_frame.crc = mb_rtu_crc16(&ctx->uart_buf[0],
					 ctx->tx_frame.length + 2);
	sys_put_le16(ctx->tx_frame.crc,
		     &ctx->uart_buf[ctx->tx_frame.length + 2]);
	tx_bytes += 2;

	ctx->uart_buf_ctr = tx_bytes;
	ctx->uart_buf_ptr = &ctx->uart_buf[0];

	LOG_HEXDUMP_DBG(ctx->uart_buf, ctx->uart_buf_ctr, "uart_buf");
	LOG_DBG("Start frame transmission");
	mb_rx_disable(ctx);
	mb_tx_enable(ctx);
}

void mb_tx_frame(struct mb_rtu_context *ctx)
{
	if (IS_ENABLED(CONFIG_MODBUS_RTU_ASCII_MODE) &&
	    (ctx->ascii_mode == true)) {
		mb_tx_ascii_frame(ctx);
	} else {
		mb_tx_rtu_frame(ctx);
	}
}

/*
 * A byte has been received from a serial port. We just store it in the buffer
 * for processing when a complete packet has been received.
 */
static void mb_cb_handler_rx(struct mb_rtu_context *ctx)
{
	if ((ctx->ascii_mode == true) &&
	    IS_ENABLED(CONFIG_MODBUS_RTU_ASCII_MODE)) {
		uint8_t c;

		if (uart_fifo_read(ctx->dev, &c, 1) != 1) {
			LOG_ERR("Failed to read UART");
			return;
		}

		if (c == MODBUS_ASCII_START_FRAME_CHAR) {
			/* Restart a new frame */
			ctx->uart_buf_ptr = &ctx->uart_buf[0];
			ctx->uart_buf_ctr = 0;
		}

		if (ctx->uart_buf_ctr < CONFIG_MODBUS_RTU_BUFFER_SIZE) {
			*ctx->uart_buf_ptr++ = c;
			ctx->uart_buf_ctr++;
		}

		if (c == MODBUS_ASCII_END_FRAME_CHAR2) {
			k_work_submit(&ctx->server_work);
		}

	} else {
		int n;

		/* Restart timer on a new character */
		k_timer_start(&ctx->rtu_timer,
			      K_USEC(ctx->rtu_timeout), K_NO_WAIT);

		n = uart_fifo_read(ctx->dev, ctx->uart_buf_ptr,
				   (CONFIG_MODBUS_RTU_BUFFER_SIZE -
				    ctx->uart_buf_ctr));

		ctx->uart_buf_ptr += n;
		ctx->uart_buf_ctr += n;
	}
}

static void mb_cb_handler_tx(struct mb_rtu_context *ctx)
{
	int n;

	if (ctx->uart_buf_ctr > 0) {
		n = uart_fifo_fill(ctx->dev, ctx->uart_buf_ptr,
				   ctx->uart_buf_ctr);
		ctx->uart_buf_ctr -= n;
		ctx->uart_buf_ptr += n;
	} else {
		/* Disable transmission */
		ctx->uart_buf_ptr = &ctx->uart_buf[0];
		mb_tx_disable(ctx);
		mb_rx_enable(ctx);
	}
}

static void mb_uart_cb_handler(const struct device *dev, void *app_data)
{
	struct mb_rtu_context *ctx = (struct mb_rtu_context *)app_data;

	if (ctx == NULL) {
		LOG_ERR("Modbus hardware is not properly initialized");
		return;
	}

	while (uart_irq_update(ctx->dev) && uart_irq_is_pending(ctx->dev)) {

		if (uart_irq_rx_ready(ctx->dev)) {
			mb_cb_handler_rx(ctx);
		}

		if (uart_irq_tx_ready(ctx->dev)) {
			mb_cb_handler_tx(ctx);
		}
	}
}

static void mb_rx_handler(struct k_work *item)
{
	struct mb_rtu_context *ctx =
		CONTAINER_OF(item, struct mb_rtu_context, server_work);

	if (ctx == NULL) {
		LOG_ERR("Where is my pointer?");
		return;
	}

	mb_rx_disable(ctx);

	if (IS_ENABLED(CONFIG_MODBUS_RTU_ASCII_MODE) &&
	    (ctx->ascii_mode == true)) {
		ctx->rx_frame_err = mb_rx_ascii_frame(ctx);
	} else {
		ctx->rx_frame_err = mb_rx_rtu_frame(ctx);
	}

	if (ctx->client == true) {
		k_sem_give(&ctx->client_wait_sem);
	} else if (IS_ENABLED(CONFIG_MODBUS_RTU_SERVER)) {
		if (mbs_rx_handler(ctx) == false) {
			/* Server does not send response, re-enable RX */
			mb_rx_enable(ctx);
		}
	}
}

/* This function is called when the RTU framing timer expires. */
static void mb_rtu_tmr_handler(struct k_timer *t_id)
{
	struct mb_rtu_context *ctx;

	ctx = (struct mb_rtu_context *)k_timer_user_data_get(t_id);

	if (ctx == NULL) {
		LOG_ERR("Failed to get Modbus context");
		return;
	}

	k_work_submit(&ctx->server_work);
}

static int mb_configure_uart(struct mb_rtu_context *ctx,
			     uint32_t baudrate,
			     enum uart_config_parity parity)
{
	const uint32_t if_delay_max = 3500000;
	const uint32_t numof_bits = 11;
	struct uart_config uart_cfg;

	ctx->dev = device_get_binding(ctx->dev_name);
	if (ctx->dev == NULL) {
		LOG_ERR("Failed to get UART device %s",
			log_strdup(ctx->dev_name));
		return -ENODEV;
	}

	uart_cfg.baudrate = baudrate,
	uart_cfg.flow_ctrl = UART_CFG_FLOW_CTRL_NONE;

	if (ctx->ascii_mode == true) {
		uart_cfg.data_bits = UART_CFG_DATA_BITS_7;
	} else {
		uart_cfg.data_bits = UART_CFG_DATA_BITS_8;
	}

	switch (parity) {
	case UART_CFG_PARITY_ODD:
	case UART_CFG_PARITY_EVEN:
		uart_cfg.parity = parity;
		uart_cfg.stop_bits = UART_CFG_STOP_BITS_1;
		break;
	case UART_CFG_PARITY_NONE:
		/* Use of no parity requires 2 stop bits */
		uart_cfg.parity = parity;
		uart_cfg.stop_bits = UART_CFG_STOP_BITS_2;
		break;
	default:
		return -EINVAL;
	}

	if (uart_configure(ctx->dev, &uart_cfg) != 0) {
		LOG_ERR("Failed to configure UART");
		return -EINVAL;
	}

	uart_irq_callback_user_data_set(ctx->dev, mb_uart_cb_handler, ctx);
	mb_rx_enable(ctx);

	if (baudrate <= 38400) {
		ctx->rtu_timeout = (numof_bits * if_delay_max) / baudrate;
	} else {
		ctx->rtu_timeout = (numof_bits * if_delay_max) / 38400;
	}

	LOG_INF("RTU timeout %u us", ctx->rtu_timeout);

	return 0;
}

struct mb_rtu_context *mb_get_context(const uint8_t iface)
{
	struct mb_rtu_context *ctx;

	if (iface >= ARRAY_SIZE(mb_ctx_tbl)) {
		LOG_ERR("Interface %u not available", iface);
		return NULL;
	}

	ctx = &mb_ctx_tbl[iface];

	if (!atomic_test_bit(&ctx->state, MB_RTU_STATE_CONFIGURED)) {
		LOG_ERR("Interface not configured");
		return NULL;
	}

	return ctx;
}

static int mb_configure_gpio(struct mb_rtu_context *ctx)
{
	if (ctx->de != NULL) {
		ctx->de->dev = device_get_binding(ctx->de->name);
		if (ctx->de->dev == NULL) {
			return -ENODEV;
		}

		if (gpio_pin_configure(ctx->de->dev, ctx->de->pin,
				       GPIO_OUTPUT_INACTIVE | ctx->de->flags)) {
			return -EIO;
		}
	}


	if (ctx->re != NULL) {
		ctx->re->dev = device_get_binding(ctx->re->name);
		if (ctx->re->dev == NULL) {
			return -ENODEV;
		}

		if (gpio_pin_configure(ctx->re->dev, ctx->re->pin,
				       GPIO_OUTPUT_INACTIVE | ctx->re->flags)) {
			return -EIO;
		}
	}

	return 0;
}

static struct mb_rtu_context *mb_cfg_iface(const uint8_t iface,
					   const uint8_t node_addr,
					   const uint32_t baud,
					   const enum uart_config_parity parity,
					   const uint32_t rx_timeout,
					   const bool client,
					   const bool ascii_mode)
{
	struct mb_rtu_context *ctx;

	if (iface >= ARRAY_SIZE(mb_ctx_tbl)) {
		LOG_ERR("Interface %u not available", iface);
		return NULL;
	}

	ctx = &mb_ctx_tbl[iface];

	if (atomic_test_and_set_bit(&ctx->state, MB_RTU_STATE_CONFIGURED)) {
		LOG_ERR("Interface allready used");
		return NULL;
	}

	if ((client == true) &&
	    !IS_ENABLED(CONFIG_MODBUS_RTU_CLIENT)) {
		LOG_ERR("Modbus client support is not enabled");
		ctx->client = false;
		return NULL;
	}

	ctx->rxwait_to = rx_timeout;
	ctx->node_addr = node_addr;
	ctx->client = client;
	ctx->ascii_mode = ascii_mode;
	ctx->mbs_user_cb = NULL;
	k_mutex_init(&ctx->iface_lock);

	ctx->uart_buf_ctr = 0;
	ctx->uart_buf_ptr = &ctx->uart_buf[0];

	k_sem_init(&ctx->client_wait_sem, 0, 1);
	k_work_init(&ctx->server_work, mb_rx_handler);

	if (IS_ENABLED(CONFIG_MODBUS_RTU_FC08_DIAGNOSTIC)) {
		mbs_reset_statistics(ctx);
	}

	if (mb_configure_gpio(ctx) != 0) {
		return NULL;
	}

	if (mb_configure_uart(ctx, baud, parity) != 0) {
		LOG_ERR("Failed to configure UART");
		return NULL;
	}

	k_timer_init(&ctx->rtu_timer, mb_rtu_tmr_handler, NULL);
	k_timer_user_data_set(&ctx->rtu_timer, ctx);

	return ctx;
}

int mb_rtu_cfg_server(const uint8_t iface, const uint8_t node_addr,
		      const uint32_t baud, const enum uart_config_parity parity,
		      struct mbs_rtu_user_callbacks *const cb,
		      const bool ascii_mode)
{
	struct mb_rtu_context *ctx;

	if (!IS_ENABLED(CONFIG_MODBUS_RTU_SERVER)) {
		LOG_ERR("Modbus server support is not enabled");
		return -ENOTSUP;
	}

	if (cb == NULL) {
		LOG_ERR("User callbacks should be available");
		return -EINVAL;
	}

	ctx = mb_cfg_iface(iface, node_addr, baud,
			   parity, 0, false, ascii_mode);

	if (ctx == NULL) {
		return -EINVAL;
	}

	ctx->mbs_user_cb = cb;

	return 0;
}

int mb_rtu_cfg_client(const uint8_t iface,
		      const uint32_t baud, const enum uart_config_parity parity,
		      const uint32_t rx_timeout,
		      const bool ascii_mode)
{
	struct mb_rtu_context *ctx;

	if (!IS_ENABLED(CONFIG_MODBUS_RTU_CLIENT)) {
		LOG_ERR("Modbus client support is not enabled");
		return -ENOTSUP;
	}

	ctx = mb_cfg_iface(iface, 0, baud,
			   parity, rx_timeout, true, ascii_mode);

	if (ctx == NULL) {
		return -EINVAL;
	}

	return 0;
}

int mb_rtu_disable_iface(const uint8_t iface)
{
	struct mb_rtu_context *ctx;

	if (iface >= ARRAY_SIZE(mb_ctx_tbl)) {
		LOG_ERR("Interface %u not available", iface);
		return -EINVAL;
	}

	ctx = &mb_ctx_tbl[iface];

	mb_tx_disable(ctx);
	mb_rx_disable(ctx);
	k_timer_stop(&ctx->rtu_timer);

	ctx->rxwait_to = 0;
	ctx->node_addr = 0;
	ctx->ascii_mode = false;
	ctx->mbs_user_cb = NULL;
	atomic_clear_bit(&ctx->state, MB_RTU_STATE_CONFIGURED);

	LOG_INF("Disable Modbus interface");

	return 0;
}

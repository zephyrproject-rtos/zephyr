/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(modbus, CONFIG_MODBUS_LOG_LEVEL);

#include <kernel.h>
#include <string.h>
#include <sys/byteorder.h>
#include <modbus_internal.h>

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
		.iface_name = DT_INST_LABEL(n),			\
		.dev_name = DT_INST_BUS_LABEL(n),		\
		.de = MB_RTU_ASSIGN_GPIO_CFG(n, de_gpios),	\
		.re = MB_RTU_ASSIGN_GPIO_CFG(n, re_gpios),	\
	},

static struct modbus_context mb_ctx_tbl[] = {
	DT_INST_FOREACH_STATUS_OKAY(MODBUS_DT_GET_DEV)
};

static void modbus_rx_handler(struct k_work *item)
{
	struct modbus_context *ctx;

	ctx = CONTAINER_OF(item, struct modbus_context, server_work);
	if (ctx == NULL) {
		LOG_ERR("Failed to obtain context pointer?");
		return;
	}

	switch (ctx->mode) {
	case MODBUS_MODE_RTU:
	case MODBUS_MODE_ASCII:
		modbus_serial_rx_disable(ctx);
		ctx->rx_frame_err = modbus_serial_rx_frame(ctx);
		break;
	default:
		LOG_ERR("Unknown MODBUS mode");
		return;
	}

	if (ctx->client == true) {
		k_sem_give(&ctx->client_wait_sem);
		return;
	}

	if (IS_ENABLED(CONFIG_MODBUS_SERVER)) {
		bool respond = mbs_rx_handler(ctx);

		switch (ctx->mode) {
		case MODBUS_MODE_RTU:
		case MODBUS_MODE_ASCII:
			if (respond == false) {
				LOG_DBG("Server has dropped frame");
				modbus_serial_rx_enable(ctx);
			}
			break;
		default:
			break;
		}
	}
}

void mb_tx_frame(struct modbus_context *ctx)
{
	switch (ctx->mode) {
	case MODBUS_MODE_RTU:
	case MODBUS_MODE_ASCII:
		if (modbus_serial_tx_frame(ctx)) {
			LOG_ERR("Unsupported MODBUS serial mode");
		}
		break;
	default:
		LOG_ERR("Unknown MODBUS mode");
	}
}

struct modbus_context *mb_get_context(const uint8_t iface)
{
	struct modbus_context *ctx;

	if (iface >= ARRAY_SIZE(mb_ctx_tbl)) {
		LOG_ERR("Interface %u not available", iface);
		return NULL;
	}

	ctx = &mb_ctx_tbl[iface];

	if (!atomic_test_bit(&ctx->state, MODBUS_STATE_CONFIGURED)) {
		LOG_ERR("Interface not configured");
		return NULL;
	}

	return ctx;
}

static struct modbus_context *mb_cfg_iface(const uint8_t iface,
					   const uint8_t node_addr,
					   const uint32_t baud,
					   const enum uart_config_parity parity,
					   const uint32_t rx_timeout,
					   const bool client,
					   const bool ascii_mode)
{
	struct modbus_context *ctx;

	if (iface >= ARRAY_SIZE(mb_ctx_tbl)) {
		LOG_ERR("Interface %u not available", iface);
		return NULL;
	}

	ctx = &mb_ctx_tbl[iface];

	if (atomic_test_and_set_bit(&ctx->state, MODBUS_STATE_CONFIGURED)) {
		LOG_ERR("Interface allready used");
		return NULL;
	}

	if ((client == true) &&
	    !IS_ENABLED(CONFIG_MODBUS_CLIENT)) {
		LOG_ERR("Modbus client support is not enabled");
		ctx->client = false;
		return NULL;
	}

	ctx->rxwait_to = rx_timeout;
	ctx->node_addr = node_addr;
	ctx->client = client;
	ctx->mbs_user_cb = NULL;
	k_mutex_init(&ctx->iface_lock);

	ctx->uart_buf_ctr = 0;
	ctx->uart_buf_ptr = &ctx->uart_buf[0];

	k_sem_init(&ctx->client_wait_sem, 0, 1);
	k_work_init(&ctx->server_work, modbus_rx_handler);

	if (IS_ENABLED(CONFIG_MODBUS_FC08_DIAGNOSTIC)) {
		mbs_reset_statistics(ctx);
	}

	if (modbus_serial_init(ctx, baud, parity, ascii_mode) != 0) {
		LOG_ERR("Failed to init MODBUS over serial line");
		return NULL;
	}

	LOG_DBG("Modbus interface %s initialized", ctx->iface_name);

	return ctx;
}

int modbus_init_server(const uint8_t iface, const uint8_t node_addr,
		       const uint32_t baud, const enum uart_config_parity parity,
		       struct modbus_user_callbacks *const cb,
		       const bool ascii_mode)
{
	struct modbus_context *ctx;

	if (!IS_ENABLED(CONFIG_MODBUS_SERVER)) {
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

int modbus_iface_get_by_name(const char *iface_name)
{
	for (int i = 0; i < ARRAY_SIZE(mb_ctx_tbl); i++) {
		if (strcmp(iface_name, mb_ctx_tbl[i].iface_name) == 0) {
			return i;
		}
	}

	return -ENODEV;
}

int modbus_init_client(const uint8_t iface,
		       const uint32_t baud, const enum uart_config_parity parity,
		       const uint32_t rx_timeout,
		       const bool ascii_mode)
{
	struct modbus_context *ctx;

	if (!IS_ENABLED(CONFIG_MODBUS_CLIENT)) {
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

int modbus_disable(const uint8_t iface)
{
	struct modbus_context *ctx;

	if (iface >= ARRAY_SIZE(mb_ctx_tbl)) {
		LOG_ERR("Interface %u not available", iface);
		return -EINVAL;
	}

	ctx = &mb_ctx_tbl[iface];

	modbus_serial_disable(ctx);
	ctx->rxwait_to = 0;
	ctx->node_addr = 0;
	ctx->mode = MODBUS_MODE_RTU;
	ctx->mbs_user_cb = NULL;
	atomic_clear_bit(&ctx->state, MODBUS_STATE_CONFIGURED);

	LOG_INF("Disable Modbus interface");

	return 0;
}

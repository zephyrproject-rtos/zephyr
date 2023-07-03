/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(modbus, CONFIG_MODBUS_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <modbus_internal.h>

#define DT_DRV_COMPAT zephyr_modbus_serial

#define MB_RTU_DEFINE_GPIO_CFG(inst, prop)				\
	static struct gpio_dt_spec prop##_cfg_##inst = {		\
		.port = DEVICE_DT_GET(DT_INST_PHANDLE(inst, prop)),	\
		.pin = DT_INST_GPIO_PIN(inst, prop),			\
		.dt_flags = DT_INST_GPIO_FLAGS(inst,  prop),		\
	};

#define MB_RTU_DEFINE_GPIO_CFGS(inst)					\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, de_gpios),		\
		    (MB_RTU_DEFINE_GPIO_CFG(inst, de_gpios)), ())	\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, re_gpios),		\
		    (MB_RTU_DEFINE_GPIO_CFG(inst, re_gpios)), ())

DT_INST_FOREACH_STATUS_OKAY(MB_RTU_DEFINE_GPIO_CFGS)

#define MB_RTU_ASSIGN_GPIO_CFG(inst, prop)			\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, prop),		\
		    (&prop##_cfg_##inst), (NULL))

#define MODBUS_DT_GET_SERIAL_DEV(inst) {			\
		.dev = DEVICE_DT_GET(DT_INST_PARENT(inst)),	\
		.de = MB_RTU_ASSIGN_GPIO_CFG(inst, de_gpios),	\
		.re = MB_RTU_ASSIGN_GPIO_CFG(inst, re_gpios),	\
	},

#ifdef CONFIG_MODBUS_SERIAL
static struct modbus_serial_config modbus_serial_cfg[] = {
	DT_INST_FOREACH_STATUS_OKAY(MODBUS_DT_GET_SERIAL_DEV)
};
#endif

#define MODBUS_DT_GET_DEV(inst) {				\
		.iface_name = DEVICE_DT_NAME(DT_DRV_INST(inst)),\
		.cfg = &modbus_serial_cfg[inst],		\
	},

#define DEFINE_MODBUS_RAW_ADU(x, _) {				\
		.iface_name = "RAW_"#x,				\
		.rawcb.raw_tx_cb = NULL,				\
		.mode = MODBUS_MODE_RAW,			\
	}


static struct modbus_context mb_ctx_tbl[] = {
	DT_INST_FOREACH_STATUS_OKAY(MODBUS_DT_GET_DEV)
#ifdef CONFIG_MODBUS_RAW_ADU
	LISTIFY(CONFIG_MODBUS_NUMOF_RAW_ADU, DEFINE_MODBUS_RAW_ADU, (,), _)
#endif
};

static void modbus_rx_handler(struct k_work *item)
{
	struct modbus_context *ctx;

	ctx = CONTAINER_OF(item, struct modbus_context, server_work);

	switch (ctx->mode) {
	case MODBUS_MODE_RTU:
	case MODBUS_MODE_ASCII:
		if (IS_ENABLED(CONFIG_MODBUS_SERIAL)) {
			modbus_serial_rx_disable(ctx);
			ctx->rx_adu_err = modbus_serial_rx_adu(ctx);
		}
		break;
	case MODBUS_MODE_RAW:
		if (IS_ENABLED(CONFIG_MODBUS_RAW_ADU)) {
			ctx->rx_adu_err = modbus_raw_rx_adu(ctx);
		}
		break;
	default:
		LOG_ERR("Unknown MODBUS mode");
		return;
	}

	if (ctx->client == true) {
		k_sem_give(&ctx->client_wait_sem);
	} else if (IS_ENABLED(CONFIG_MODBUS_SERVER)) {
		bool respond = modbus_server_handler(ctx);

		if (respond) {
			modbus_tx_adu(ctx);
		} else {
			LOG_DBG("Server has dropped frame");
		}

		switch (ctx->mode) {
		case MODBUS_MODE_RTU:
		case MODBUS_MODE_ASCII:
			if (IS_ENABLED(CONFIG_MODBUS_SERIAL) &&
			    respond == false) {
				modbus_serial_rx_enable(ctx);
			}
			break;
		default:
			break;
		}
	}
}

void modbus_tx_adu(struct modbus_context *ctx)
{
	switch (ctx->mode) {
	case MODBUS_MODE_RTU:
	case MODBUS_MODE_ASCII:
		if (IS_ENABLED(CONFIG_MODBUS_SERIAL) &&
		    modbus_serial_tx_adu(ctx)) {
			LOG_ERR("Unsupported MODBUS serial mode");
		}
		break;
	case MODBUS_MODE_RAW:
		if (IS_ENABLED(CONFIG_MODBUS_RAW_ADU) &&
		    modbus_raw_tx_adu(ctx)) {
			LOG_ERR("Unsupported MODBUS raw mode");
		}
		break;
	default:
		LOG_ERR("Unknown MODBUS mode");
	}
}

int modbus_tx_wait_rx_adu(struct modbus_context *ctx)
{
	modbus_tx_adu(ctx);

	if (k_sem_take(&ctx->client_wait_sem, K_USEC(ctx->rxwait_to)) != 0) {
		LOG_WRN("Client wait-for-RX timeout");
		return -ETIMEDOUT;
	}

	return ctx->rx_adu_err;
}

struct modbus_context *modbus_get_context(const uint8_t iface)
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

int modbus_iface_get_by_ctx(const struct modbus_context *ctx)
{
	for (int i = 0; i < ARRAY_SIZE(mb_ctx_tbl); i++) {
		if (&mb_ctx_tbl[i] == ctx) {
			return i;
		}
	}

	return -ENODEV;
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

static struct modbus_context *modbus_init_iface(const uint8_t iface)
{
	struct modbus_context *ctx;

	if (iface >= ARRAY_SIZE(mb_ctx_tbl)) {
		LOG_ERR("Interface %u not available", iface);
		return NULL;
	}

	ctx = &mb_ctx_tbl[iface];

	if (atomic_test_and_set_bit(&ctx->state, MODBUS_STATE_CONFIGURED)) {
		LOG_ERR("Interface already used");
		return NULL;
	}

	k_mutex_init(&ctx->iface_lock);
	k_sem_init(&ctx->client_wait_sem, 0, 1);
	k_work_init(&ctx->server_work, modbus_rx_handler);

	return ctx;
}

int modbus_init_server(const int iface, struct modbus_iface_param param)
{
	struct modbus_context *ctx = NULL;
	int rc = 0;

	if (!IS_ENABLED(CONFIG_MODBUS_SERVER)) {
		LOG_ERR("Modbus server support is not enabled");
		rc = -ENOTSUP;
		goto init_server_error;
	}

	if (param.server.user_cb == NULL) {
		LOG_ERR("User callbacks should be available");
		rc = -EINVAL;
		goto init_server_error;
	}

	ctx = modbus_init_iface(iface);
	if (ctx == NULL) {
		rc = -EINVAL;
		goto init_server_error;
	}

	ctx->client = false;

	switch (param.mode) {
	case MODBUS_MODE_RTU:
	case MODBUS_MODE_ASCII:
		if (IS_ENABLED(CONFIG_MODBUS_SERIAL) &&
		    modbus_serial_init(ctx, param) != 0) {
			LOG_ERR("Failed to init MODBUS over serial line");
			rc = -EINVAL;
			goto init_server_error;
		}
		break;
	case MODBUS_MODE_RAW:
		if (IS_ENABLED(CONFIG_MODBUS_RAW_ADU) &&
		    modbus_raw_init(ctx, param) != 0) {
			LOG_ERR("Failed to init MODBUS raw ADU support");
			rc = -EINVAL;
			goto init_server_error;
		}
		break;
	default:
		LOG_ERR("Unknown MODBUS mode");
		rc = -ENOTSUP;
		goto init_server_error;
	}

	ctx->unit_id = param.server.unit_id;
	ctx->mbs_user_cb = param.server.user_cb;
	if (IS_ENABLED(CONFIG_MODBUS_FC08_DIAGNOSTIC)) {
		modbus_reset_stats(ctx);
	}

	LOG_DBG("Modbus interface %s initialized", ctx->iface_name);

	return 0;

init_server_error:
	if (ctx != NULL) {
		atomic_clear_bit(&ctx->state, MODBUS_STATE_CONFIGURED);
	}

	return rc;
}

int modbus_init_client(const int iface, struct modbus_iface_param param)
{
	struct modbus_context *ctx = NULL;
	int rc = 0;

	if (!IS_ENABLED(CONFIG_MODBUS_CLIENT)) {
		LOG_ERR("Modbus client support is not enabled");
		rc = -ENOTSUP;
		goto init_client_error;
	}

	ctx = modbus_init_iface(iface);
	if (ctx == NULL) {
		rc = -EINVAL;
		goto init_client_error;
	}

	ctx->client = true;

	switch (param.mode) {
	case MODBUS_MODE_RTU:
	case MODBUS_MODE_ASCII:
		if (IS_ENABLED(CONFIG_MODBUS_SERIAL) &&
		    modbus_serial_init(ctx, param) != 0) {
			LOG_ERR("Failed to init MODBUS over serial line");
			rc = -EINVAL;
			goto init_client_error;
		}
		break;
	case MODBUS_MODE_RAW:
		if (IS_ENABLED(CONFIG_MODBUS_RAW_ADU) &&
		    modbus_raw_init(ctx, param) != 0) {
			LOG_ERR("Failed to init MODBUS raw ADU support");
			rc = -EINVAL;
			goto init_client_error;
		}
		break;
	default:
		LOG_ERR("Unknown MODBUS mode");
		rc = -ENOTSUP;
		goto init_client_error;
	}

	ctx->unit_id = 0;
	ctx->mbs_user_cb = NULL;
	ctx->rxwait_to = param.rx_timeout;

	return 0;

init_client_error:
	if (ctx != NULL) {
		atomic_clear_bit(&ctx->state, MODBUS_STATE_CONFIGURED);
	}

	return rc;
}

int modbus_disable(const uint8_t iface)
{
	struct modbus_context *ctx;
	struct k_work_sync work_sync;

	ctx = modbus_get_context(iface);
	if (ctx == NULL) {
		LOG_ERR("Interface %u not initialized", iface);
		return -EINVAL;
	}

	switch (ctx->mode) {
	case MODBUS_MODE_RTU:
	case MODBUS_MODE_ASCII:
		if (IS_ENABLED(CONFIG_MODBUS_SERIAL)) {
			modbus_serial_disable(ctx);
		}
		break;
	case MODBUS_MODE_RAW:
		break;
	default:
		LOG_ERR("Unknown MODBUS mode");
	}

	k_work_cancel_sync(&ctx->server_work, &work_sync);
	ctx->rxwait_to = 0;
	ctx->unit_id = 0;
	ctx->mode = MODBUS_MODE_RTU;
	ctx->mbs_user_cb = NULL;
	atomic_clear_bit(&ctx->state, MODBUS_STATE_CONFIGURED);

	LOG_INF("Modbus interface %u disabled", iface);

	return 0;
}

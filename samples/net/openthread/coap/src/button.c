/*
 * Copyright (c) 2024 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(coap);

#include "coap_utils.h"
#include "button.h"

struct btn_rsc_data {
	const struct gpio_dt_spec gpio;
};

struct btn_rsc_ctx {
	struct btn_rsc_data *btn;
	int count;
};

static const struct json_obj_descr json_btn_state_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_btn_state, btn_id, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_btn_state, state, JSON_TOK_NUMBER),
};

static const struct json_obj_descr json_btn_get_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_btn_get, device_id, JSON_TOK_STRING),
	JSON_OBJ_DESCR_OBJ_ARRAY(struct json_btn_get, btns, JSON_MAX_BTN, count,
				 json_btn_state_descr, ARRAY_SIZE(json_btn_state_descr)),
};

static K_SEM_DEFINE(btn_get_sem, 0, 1);

#ifdef CONFIG_OT_COAP_SAMPLE_SERVER
static int btn_handler_get(void *ctx, otMessage *msg, const otMessageInfo *msg_info)
{
	uint8_t buf[COAP_MAX_BUF_SIZE];
	struct btn_rsc_ctx *btn_ctx = ctx;

	struct json_btn_get btn_data = {
		.device_id = coap_device_id(),
	};

	for (int i = 0; i < btn_ctx->count; i++) {
		btn_data.btns[i].btn_id = i;
		btn_data.btns[i].state = gpio_pin_get_dt(&btn_ctx->btn[i].gpio);
	}
	btn_data.count = btn_ctx->count;

	json_obj_encode_buf(json_btn_get_descr, ARRAY_SIZE(json_btn_get_descr), &btn_data, buf,
			    COAP_MAX_BUF_SIZE);

	return coap_resp_send(msg, msg_info, buf, strlen(buf) + 1);
}

static void btn_handler(void *ctx, otMessage *msg, const otMessageInfo *msg_info)
{
	coap_req_handler(ctx, msg, msg_info, NULL, btn_handler_get);
}

#define DEFINE_BTN_CTX(node_id)                                                                    \
	{                                                                                          \
		.gpio = GPIO_DT_SPEC_GET(node_id, gpios),                                          \
	},

#define DEFINE_BTNS_CTX(inst, compat, ...) DT_FOREACH_CHILD(DT_INST(inst, compat), DEFINE_BTN_CTX)

static struct btn_rsc_data btn_rsc_data[] = {
	DT_COMPAT_FOREACH_STATUS_OKAY_VARGS(gpio_keys, DEFINE_BTNS_CTX)};

static struct btn_rsc_ctx btn_rsc_ctx = {
	.btn = btn_rsc_data,
	.count = ARRAY_SIZE(btn_rsc_data),
};

static otCoapResource btn_rsc = {
	.mUriPath = BTN_URI,
	.mHandler = btn_handler,
	.mContext = &btn_rsc_ctx,
	.mNext = NULL,
};

static int button_init_rsc(otCoapResource *rsc)
{
	int ret = 0;

	struct btn_rsc_ctx *btn_ctx = rsc->mContext;
	const struct gpio_dt_spec *gpio;

	LOG_INF("Initializing the buttons");
	for (int i = 0; i < btn_ctx->count; i++) {
		gpio = &btn_ctx->btn[i].gpio;
		ret = button_init(gpio);
		if (ret) {
			break;
		}
	}

	return ret;
}

void coap_btn_reg_rsc(void)
{
	otInstance *ot = openthread_get_default_instance();

	button_init_rsc(&btn_rsc);
	LOG_INF("Registering button rsc");
	otCoapAddResource(ot, &btn_rsc);
}
#endif /* CONFIG_OT_COAP_SAMPLE_SERVER */

int button_init(const struct gpio_dt_spec *gpio)
{
	int ret;

	if (!gpio_is_ready_dt(gpio)) {
		LOG_ERR("Error: button device %s is not ready\n", gpio->port->name);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(gpio, GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure %s pin %d\n", ret, gpio->port->name,
			gpio->pin);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure interrupt on %s pin %d\n", ret,
			gpio->port->name, gpio->pin);
		return ret;
	}

	return 0;
}

static void coap_btn_get_state_cb(void *ctx, otMessage *msg, const otMessageInfo *msg_info,
				  otError error)
{
	uint8_t buf[COAP_MAX_BUF_SIZE];
	int len = COAP_MAX_BUF_SIZE;
	struct json_btn_get *btn = (struct json_btn_get *)ctx;
	int ret;

	ret = coap_get_data(msg, buf, &len);
	if (ret) {
		btn->count = 0;
		goto exit;
	}

	json_obj_parse(buf, len, json_btn_get_descr, ARRAY_SIZE(json_btn_get_descr), btn);
exit:
	k_sem_give(&btn_get_sem);
}

int coap_btn_get_state(const char *addr, int btn_id, int *state)
{
	struct json_btn_get btn;
	int ret;

	ret = coap_get_req_send(addr, BTN_URI, NULL, 0, coap_btn_get_state_cb, &btn);
	if (ret) {
		return ret;
	}

	ret = k_sem_take(&btn_get_sem, K_FOREVER);
	if (ret) {
		return ret;
	}

	if (btn_id >= btn.count) {
		return -ENODEV;
	}

	*state = btn.btns[btn_id].state;
	return ret;
}

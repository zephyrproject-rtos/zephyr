/*
 * Copyright (c) 2024 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(coap);

#include "coap_utils.h"
#include "led.h"

#ifdef CONFIG_OT_COAP_SAMPLE_SERVER
struct led_rsc_data {
	const struct gpio_dt_spec gpio;
	int state;
};

struct led_rsc_ctx {
	struct led_rsc_data *led;
	int count;
};
#endif

static const struct json_obj_descr json_led_state_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_led_state, led_id, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_led_state, state, JSON_TOK_NUMBER),
};

static const struct json_obj_descr json_led_get_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_led_get, device_id, JSON_TOK_STRING),
	JSON_OBJ_DESCR_OBJ_ARRAY(struct json_led_get, leds, JSON_MAX_LED, count,
				 json_led_state_descr, ARRAY_SIZE(json_led_state_descr)),
};

K_SEM_DEFINE(led_get_sem, 0, 1);

#ifdef CONFIG_OT_COAP_SAMPLE_SERVER
static int led_init(otCoapResource *rsc)
{
	struct led_rsc_ctx *led_ctx = rsc->mContext;
	int ret;

	LOG_INF("Initializing the LED");
	for (int i = 0; i < led_ctx->count; i++) {
		struct led_rsc_data *led = &led_ctx->led[i];

		if (!gpio_is_ready_dt(&led->gpio)) {
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&led->gpio, GPIO_OUTPUT);
		if (ret) {
			LOG_ERR("Failed to configure the GPIO");
			return ret;
		}
	}

	return 0;
}

static int led_handler_put(void *ctx, uint8_t *buf, int size)
{
	struct json_led_state led_data;
	struct led_rsc_ctx *led_ctx = ctx;
	struct led_rsc_data *led;
	int ret = -EINVAL;

	json_obj_parse(buf, size, json_led_state_descr, ARRAY_SIZE(json_led_state_descr),
		       &led_data);

	if (led_data.led_id >= led_ctx->count) {
		LOG_ERR("Invalid led id: %x", led_data.led_id);
		return -EINVAL;
	}
	led = &led_ctx->led[led_data.led_id];

	switch (led_data.state) {
	case LED_MSG_STATE_ON:
		ret = gpio_pin_set_dt(&led->gpio, 1);
		led->state = 1;
		break;
	case LED_MSG_STATE_OFF:
		ret = gpio_pin_set_dt(&led->gpio, 0);
		led->state = 0;
		break;
	case LED_MSG_STATE_TOGGLE:
		led->state = 1 - led->state;
		ret = gpio_pin_set_dt(&led->gpio, led->state);
		break;
	default:
		LOG_ERR("Set an unsupported LED state: %x", led_data.state);
	}

	return ret;
}

static int led_handler_get(void *ctx, otMessage *msg, const otMessageInfo *msg_info)
{
	uint8_t buf[COAP_MAX_BUF_SIZE];
	struct led_rsc_ctx *led_ctx = ctx;

	struct json_led_get led_data = {
		.device_id = coap_device_id(),
	};

	for (int i = 0; i < led_ctx->count; i++) {
		led_data.leds[i].led_id = i;
		led_data.leds[i].state = led_ctx->led[i].state;
	}
	led_data.count = led_ctx->count;

	json_obj_encode_buf(json_led_get_descr, ARRAY_SIZE(json_led_get_descr), &led_data, buf,
			    COAP_MAX_BUF_SIZE);

	return coap_resp_send(msg, msg_info, buf, strlen(buf) + 1);
}

static void led_handler(void *ctx, otMessage *msg, const otMessageInfo *msg_info)
{
	coap_req_handler(ctx, msg, msg_info, led_handler_put, led_handler_get);
}

#define DEFINE_LED_CTX(node_id)                                                                    \
	{                                                                                          \
		.gpio = GPIO_DT_SPEC_GET(node_id, gpios),                                          \
		.state = 0,                                                                        \
	},

#define DEFINE_LEDS_CTX(inst, compat, ...) DT_FOREACH_CHILD(DT_INST(inst, compat), DEFINE_LED_CTX)

static struct led_rsc_data led_rsc_data[] = {
	DT_COMPAT_FOREACH_STATUS_OKAY_VARGS(gpio_leds, DEFINE_LEDS_CTX)};

static struct led_rsc_ctx led_rsc_ctx = {
	.led = led_rsc_data,
	.count = ARRAY_SIZE(led_rsc_data),
};

static otCoapResource led_rsc = {
	.mUriPath = LED_URI,
	.mHandler = led_handler,
	.mContext = &led_rsc_ctx,
	.mNext = NULL,
};

void coap_led_reg_rsc(void)
{
	otInstance *ot = openthread_get_default_instance();

	LOG_INF("Registering LED rsc");
	led_init(&led_rsc);
	otCoapAddResource(ot, &led_rsc);
}
#endif /* CONFIG_OT_COAP_SAMPLE_SERVER */

static void coap_led_send_req_cb(void *ctx, otMessage *msg, const otMessageInfo *msg_info,
				 otError error)
{
}

int coap_led_set_state(const char *addr, int led_id, int state)
{
	uint8_t buf[COAP_MAX_BUF_SIZE];

	struct json_led_state led_data = {
		.led_id = led_id,
		.state = state,
	};

	json_obj_encode_buf(json_led_state_descr, ARRAY_SIZE(json_led_state_descr), &led_data, buf,
			    COAP_MAX_BUF_SIZE);

	return coap_put_req_send(addr, LED_URI, buf, strlen(buf) + 1, coap_led_send_req_cb, NULL);
}

static void coap_led_get_state_cb(void *ctx, otMessage *msg, const otMessageInfo *msg_info,
				  otError error)
{
	uint8_t buf[COAP_MAX_BUF_SIZE];
	int len = COAP_MAX_BUF_SIZE;
	struct json_led_get *led = (struct json_led_get *)ctx;
	int ret;

	ret = coap_get_data(msg, buf, &len);
	if (ret) {
		led->count = 0;
		goto exit;
	}

	json_obj_parse(buf, len, json_led_get_descr, ARRAY_SIZE(json_led_get_descr), led);
exit:
	k_sem_give(&led_get_sem);
}

int coap_led_get_state(const char *addr, int led_id, int *state)
{
	struct json_led_get led;
	int ret;

	ret = coap_get_req_send(addr, LED_URI, NULL, 0, coap_led_get_state_cb, &led);
	if (ret) {
		return ret;
	}

	ret = k_sem_take(&led_get_sem, K_FOREVER);
	if (ret) {
		return ret;
	}

	if (led_id >= led.count) {
		return -ENODEV;
	}

	*state = led.leds[led_id].state;
	return ret;
}

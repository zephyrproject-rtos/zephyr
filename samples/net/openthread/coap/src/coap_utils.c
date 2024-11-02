/*
 * Copyright (c) 2024 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(coap);

#include "coap_utils.h"

static uint8_t coap_buf[COAP_MAX_BUF_SIZE];
static uint8_t coap_dev_id[COAP_DEVICE_ID_SIZE];

#ifdef CONFIG_OT_COAP_SAMPLE_SERVER
static void coap_default_handler(void *context, otMessage *message,
				 const otMessageInfo *message_info)
{
	ARG_UNUSED(context);
	ARG_UNUSED(message);
	ARG_UNUSED(message_info);

	LOG_INF("Received CoAP message that does not match any request "
		"or resource");
}
#endif /* CONFIG_OT_COAP_SAMPLE_SERVER */

static int coap_req_send(const char *addr, const char *uri, uint8_t *buf, int len,
			 otCoapResponseHandler handler, void *ctx, otCoapCode code)
{
	otInstance *ot;
	otMessage *msg;
	otMessageInfo msg_info;
	otError err;
	int ret;

	ot = openthread_get_default_instance();
	if (!ot) {
		LOG_ERR("Failed to get an OpenThread instance");
		return -ENODEV;
	}

	memset(&msg_info, 0, sizeof(msg_info));
	otIp6AddressFromString(addr, &msg_info.mPeerAddr);
	msg_info.mPeerPort = OT_DEFAULT_COAP_PORT;

	msg = otCoapNewMessage(ot, NULL);
	if (!msg) {
		LOG_ERR("Failed to allocate a new CoAP message");
		return -ENOMEM;
	}

	otCoapMessageInit(msg, OT_COAP_TYPE_CONFIRMABLE, code);

	err = otCoapMessageAppendUriPathOptions(msg, uri);
	if (err != OT_ERROR_NONE) {
		LOG_ERR("Failed to append uri-path: %s", otThreadErrorToString(err));
		ret = -EBADMSG;
		goto err;
	}

	err = otCoapMessageSetPayloadMarker(msg);
	if (err != OT_ERROR_NONE) {
		LOG_ERR("Failed to set payload marker: %s", otThreadErrorToString(err));
		ret = -EBADMSG;
		goto err;
	}

	err = otMessageAppend(msg, buf, len);
	if (err != OT_ERROR_NONE) {
		LOG_ERR("Failed to set append payload to response: %s", otThreadErrorToString(err));
		ret = -EBADMSG;
		goto err;
	}

	err = otCoapSendRequest(ot, msg, &msg_info, handler, ctx);
	if (err != OT_ERROR_NONE) {
		LOG_ERR("Failed to send the request: %s", otThreadErrorToString(err));
		ret = -EIO; /* Find a better error code */
		goto err;
	}

	return 0;

err:
	otMessageFree(msg);
	return ret;
}

int coap_put_req_send(const char *addr, const char *uri, uint8_t *buf, int len,
		      otCoapResponseHandler handler, void *ctx)
{
	return coap_req_send(addr, uri, buf, len, handler, ctx, OT_COAP_CODE_PUT);
}

int coap_get_req_send(const char *addr, const char *uri, uint8_t *buf, int len,
		      otCoapResponseHandler handler, void *ctx)
{
	return coap_req_send(addr, uri, buf, len, handler, ctx, OT_COAP_CODE_GET);
}

int coap_resp_send(otMessage *req, const otMessageInfo *req_info, uint8_t *buf, int len)
{
	otInstance *ot;
	otMessage *resp;
	otCoapCode resp_code;
	otCoapType resp_type;
	otError err;
	int ret;

	ot = openthread_get_default_instance();
	if (!ot) {
		LOG_ERR("Failed to get an OpenThread instance");
		return -ENODEV;
	}

	resp = otCoapNewMessage(ot, NULL);
	if (!resp) {
		LOG_ERR("Failed to allocate a new CoAP message");
		return -ENOMEM;
	}

	switch (otCoapMessageGetType(req)) {
	case OT_COAP_TYPE_CONFIRMABLE:
		resp_type = OT_COAP_TYPE_ACKNOWLEDGMENT;
		break;
	case OT_COAP_TYPE_NON_CONFIRMABLE:
		resp_type = OT_COAP_TYPE_NON_CONFIRMABLE;
		break;
	default:
		LOG_ERR("Invalid message type");
		ret = -EINVAL;
		goto err;
	}

	switch (otCoapMessageGetCode(req)) {
	case OT_COAP_CODE_GET:
		resp_code = OT_COAP_CODE_CONTENT;
		break;
	case OT_COAP_CODE_PUT:
		resp_code = OT_COAP_CODE_CHANGED;
		break;
	default:
		LOG_ERR("Invalid message code");
		ret = -EINVAL;
		goto err;
	}

	err = otCoapMessageInitResponse(resp, req, resp_type, resp_code);
	if (err != OT_ERROR_NONE) {
		LOG_ERR("Failed to initialize the response: %s", otThreadErrorToString(err));
		ret = -EBADMSG;
		goto err;
	}

	err = otCoapMessageSetPayloadMarker(resp);
	if (err != OT_ERROR_NONE) {
		LOG_ERR("Failed to set payload marker: %s", otThreadErrorToString(err));
		ret = -EBADMSG;
		goto err;
	}

	err = otMessageAppend(resp, buf, len);
	if (err != OT_ERROR_NONE) {
		LOG_ERR("Failed to set append payload to response: %s", otThreadErrorToString(err));
		ret = -EBADMSG;
		goto err;
	}

	err = otCoapSendResponse(ot, resp, req_info);
	if (err != OT_ERROR_NONE) {
		LOG_ERR("Failed to send the response: %s", otThreadErrorToString(err));
		ret = -EIO;
		goto err;
	}

	return 0;

err:
	otMessageFree(resp);
	return ret;
}

int coap_req_handler(void *ctx, otMessage *msg, const otMessageInfo *msg_info,
		     coap_req_handler_put put_fn, coap_req_handler_get get_fn)
{
	otCoapCode msg_code = otCoapMessageGetCode(msg);
	otCoapType msg_type = otCoapMessageGetType(msg);
	int ret;

	if (msg_type != OT_COAP_TYPE_CONFIRMABLE && msg_type != OT_COAP_TYPE_NON_CONFIRMABLE) {
		return -EINVAL;
	}

	if (msg_code == OT_COAP_CODE_PUT && put_fn) {
		int len = otMessageGetLength(msg) - otMessageGetOffset(msg);

		otMessageRead(msg, otMessageGetOffset(msg), coap_buf, len);
		ret = put_fn(ctx, coap_buf, len);
		if (ret) {
			return ret;
		}

		if (msg_type == OT_COAP_TYPE_CONFIRMABLE) {
			ret = get_fn(ctx, msg, msg_info);
		}

		return ret;
	}

	if (msg_code == OT_COAP_CODE_GET) {
		return get_fn(ctx, msg, msg_info);
	}

	return -EINVAL;
}

const char *coap_device_id(void)
{
	otInstance *ot = openthread_get_default_instance();
	otExtAddress eui64;
	int i;

	if (coap_dev_id[0] != '\0') {
		return coap_dev_id;
	}

	otPlatRadioGetIeeeEui64(ot, eui64.m8);
	for (i = 0; i < 8; i++) {
		if (i * 2 >= COAP_DEVICE_ID_SIZE) {
			i = COAP_DEVICE_ID_SIZE - 1;
			break;
		}
		sprintf(coap_dev_id + i * 2, "%02x", eui64.m8[i]);
	}
	coap_dev_id[i * 2] = '\0';

	return coap_dev_id;
}

int coap_get_data(otMessage *msg, void *buf, int *len)
{
	int coap_len = otMessageGetLength(msg) - otMessageGetOffset(msg);

	if (coap_len > *len) {
		return -ENOMEM;
	}

	*len = coap_len;
	otMessageRead(msg, otMessageGetOffset(msg), buf, coap_len);

	return 0;
}

int coap_init(void)
{
	otError err;
	otInstance *ot;

#ifdef CONFIG_OT_COAP_SAMPLE_SERVER
	LOG_INF("Initializing OpenThread CoAP server");
#else  /* CONFIG_OT_COAP_SAMPLE_SERVER */
	LOG_INF("Initializing OpenThread CoAP client");
#endif /* CONFIG_OT_COAP_SAMPLE_SERVER */
	ot = openthread_get_default_instance();
	if (!ot) {
		LOG_ERR("Failed to get an OpenThread instance");
		return -ENODEV;
	}

#ifdef CONFIG_OT_COAP_SAMPLE_SERVER
	otCoapSetDefaultHandler(ot, coap_default_handler, NULL);
#endif /* CONFIG_OT_COAP_SAMPLE_SERVER */

	err = otCoapStart(ot, OT_DEFAULT_COAP_PORT);
	if (err != OT_ERROR_NONE) {
		LOG_ERR("Cannot start CoAP: %s", otThreadErrorToString(err));
		return -EBADMSG;
	}

	return 0;
}

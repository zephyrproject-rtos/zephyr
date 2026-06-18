/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/ipc/ipc_service.h>

#include <test_context.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(remote, LOG_LEVEL_INF);

static struct test_context test_ctx[TEST_EP_COUNT];
static const char *ep_name[TEST_EP_COUNT] = {"ep0", "ep1"};

static const struct device *ipc_instance = DEVICE_DT_GET(DT_ALIAS(dut_ipc));
static volatile bool close_after_unbound;

static volatile bool reregister_request;

static struct test_context *ep0(void)
{
	return &test_ctx[TEST_EP0];
}

static void reregister_action(struct test_context *ctx)
{
	int ret;

	ret = ipc_service_deregister_endpoint(&ctx->ep);
	if (ret < 0 && ret != -ENOENT) {
		LOG_ERR("ipc_service_deregister_endpoint() failed: %d", ret);
	}

	k_msleep(10);

	ret = ipc_service_register_endpoint(ipc_instance, &ctx->ep, &ctx->ep_cfg);
	if (ret < 0) {
		LOG_ERR("ipc_service_register_endpoint() after deregister failed: %d", ret);
	}
}

static void ep_bound(void *priv)
{
	struct test_context *ctx = priv;

	ctx->bound = true;
	LOG_INF("Endpoint bound");
}

static void ep_unbound(void *priv)
{
	struct test_context *ctx = priv;

	ctx->bound = false;
	ctx->unbound = true;
	LOG_INF("Endpoint unbound");

	reregister_request = true;
}

static void ep_error(const char *message, void *priv)
{
	ARG_UNUSED(priv);
	LOG_ERR("Endpoint error: %s", message);
}

static void ep_received(const void *data, size_t len, void *priv)
{
	const struct api_test_msg *msg = data;
	struct test_context *ctx = priv;
	struct api_test_msg rsp;
	int ret;

	if (len < sizeof(msg->cmd)) {
		LOG_ERR("Unexpected message size: %u", len);
		return;
	}

	switch (msg->cmd) {
	case API_TEST_CMD_PING:
		rsp.cmd = API_TEST_CMD_PONG;
		memset(rsp.data, 0, sizeof(rsp.data));
		ret = ipc_service_send(&ctx->ep, &rsp, sizeof(rsp));
		if (ret < 0) {
			LOG_ERR("PONG send failed: %d", ret);
		}
		break;
	case API_TEST_CMD_ECHO:
		if (len < sizeof(struct api_test_msg)) {
			LOG_ERR("ECHO message too short: %u", len);
			break;
		}
		rsp.cmd = API_TEST_CMD_ECHO_RSP;
		memcpy(rsp.data, msg->data, sizeof(rsp.data));
		ret = ipc_service_send(&ctx->ep, &rsp, sizeof(rsp));
		if (ret < 0) {
			LOG_ERR("ECHO_RSP send failed: %d", ret);
		}
		break;
	case API_TEST_CMD_DEREGISTER:
		reregister_request = true;
		break;
	default:
		LOG_ERR("Unhandled command: %u", msg->cmd);
		break;
	}
}

static int register_endpoint(struct test_context *ctx, const char *name, bool with_unbound)
{
	ctx->ep_cfg.name = name;
	ctx->ep_cfg.priv = ctx;
	ctx->ep_cfg.cb.bound = ep_bound;
	ctx->ep_cfg.cb.unbound = with_unbound ? ep_unbound : NULL;
	ctx->ep_cfg.cb.received = ep_received;
	ctx->ep_cfg.cb.error = ep_error;

	return ipc_service_register_endpoint(ipc_instance, &ctx->ep, &ctx->ep_cfg);
}

static void wait_for_bound(struct test_context *ctx)
{
	while (!wait_for_flag(&ctx->bound, 1000)) {
	}
}

int main(void)
{
	int ret;
	size_t ep_cnt = IS_ENABLED(CONFIG_IPC_SERVICE_API_TEST_SECOND_ENDPOINT) ? 2 : 1;

	ret = ipc_service_open_instance(ipc_instance);
	if (ret < 0 && ret != -EALREADY) {
		LOG_ERR("ipc_service_open_instance() failed: %d", ret);
		return ret;
	}

	for (size_t i = 0; i < ep_cnt; i++) {
		ret = register_endpoint(&test_ctx[i], ep_name[i], i == TEST_EP0);
		if (ret < 0) {
			LOG_ERR("ipc_service_register_endpoint() failed: %d", ret);
			return ret;
		}

		wait_for_bound(&test_ctx[i]);
	}

	LOG_INF("IPC service API remote ready");

	while (1) {
		k_msleep(1);

		if (reregister_request) {
			reregister_request = false;
			reregister_action(ep0());
		}
	}

	return 0;
}

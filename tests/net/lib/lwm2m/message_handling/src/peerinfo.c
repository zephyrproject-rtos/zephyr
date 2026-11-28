/*
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 * SPDX-License-Identifier: Apache-2.0
 */

#include "lwm2m_message_handling.h"

#include <zephyr/net/lwm2m.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/ztest.h>

static int parse(const char *url, bool is_firmware_uri, struct lwm2m_ctx *ctx)
{
	char buf[64];
	int ret;

	strcpy(buf, url);
	memset(ctx, 0, sizeof(*ctx));

	ret = lwm2m_parse_peerinfo(buf, ctx, is_firmware_uri);

	zassert_str_equal(buf, url, "URL not restored after parsing");

	return ret;
}

static uint16_t parsed_port(const struct lwm2m_ctx *ctx)
{
	zassert_equal(ctx->remote_addr_storage.ss_family, NET_AF_INET);

	return net_ntohs(net_sin(net_sad(&ctx->remote_addr_storage))->sin_port);
}

ZTEST(lwm2m_peerinfo, test_coap_with_port)
{
	struct lwm2m_ctx ctx;

	zassert_ok(parse("coap://192.0.2.1:5700", false, &ctx));
	zassert_false(ctx.use_dtls);
	zassert_equal(parsed_port(&ctx), 5700);
}

ZTEST(lwm2m_peerinfo, test_default_ports)
{
	struct lwm2m_ctx ctx;

	zassert_ok(parse("coap://192.0.2.1", false, &ctx));
	zassert_equal(parsed_port(&ctx), CONFIG_LWM2M_PEER_PORT);

	zassert_ok(parse("coap://192.0.2.1/fw", true, &ctx));
	zassert_equal(parsed_port(&ctx), CONFIG_LWM2M_FIRMWARE_PORT_NONSECURE);
}

ZTEST(lwm2m_peerinfo, test_coaps)
{
	struct lwm2m_ctx ctx;
	int ret = parse("coaps://192.0.2.1", false, &ctx);

	if (IS_ENABLED(CONFIG_LWM2M_DTLS_SUPPORT)) {
		zassert_ok(ret);
		zassert_true(ctx.use_dtls);
	} else {
		zassert_equal(ret, -EPROTONOSUPPORT);
	}
}

ZTEST(lwm2m_peerinfo, test_unsupported_scheme)
{
	struct lwm2m_ctx ctx;

	zassert_equal(parse("http://192.0.2.1", false, &ctx), -EPROTONOSUPPORT);
	zassert_equal(parse("coa://192.0.2.1", false, &ctx), -EPROTONOSUPPORT);
	zassert_equal(parse("coapss://192.0.2.1", false, &ctx), -EPROTONOSUPPORT);
}

ZTEST(lwm2m_peerinfo, test_invalid_url)
{
	struct lwm2m_ctx ctx;

	zassert_equal(parse("not a url", false, &ctx), -ENOTSUP);
}

ZTEST_SUITE(lwm2m_peerinfo, NULL, NULL, NULL, NULL, NULL);

/*
 * Copyright (c) 2018 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Test Custom Log Header
 */

#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>
#include <zephyr/ztest.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(test, LOG_LEVEL_DBG);

static uint32_t count;
static char output[512];

static int cbprintf_callback(int c, void *ctx)
{
	char **p = ctx;
	**p = c;
	(*p)++;
	return c;
}

static void backend_process(const struct log_backend *const backend, union log_msg_generic *msg)
{
	char *pstr = output;
	size_t len;
	uint8_t *p = log_msg_get_package(&msg->log, &len);

	int slen = cbpprintf(cbprintf_callback, &pstr, p);

	output[slen] = '\0';

	count += 1;
}

static const struct log_backend_api backend_api = {
	.process = backend_process,
};

LOG_BACKEND_DEFINE(backend, backend_api, false);

ZTEST(log_custom_header, test_macro_prefix)
{
	zassert_equal(count, 0);

	LOG_DBG("DBG %d", 0);
	LOG_PROCESS();
	zassert_equal(count, 1);
	zassert_mem_equal(output, CUSTOM_LOG_PREFIX "DBG 0", strlen(output));

	LOG_INF("INF %s", "foo");
	LOG_PROCESS();
	zassert_equal(count, 2);
	zassert_mem_equal(output, CUSTOM_LOG_PREFIX "INF foo", strlen(output));

	LOG_WRN("WRN %x", 0xff);
	LOG_PROCESS();
	zassert_equal(count, 3);
	zassert_mem_equal(output, CUSTOM_LOG_PREFIX "WRN ff", strlen(output));

	LOG_ERR("ERR %d %d %d", 1, 2, 3);
	LOG_PROCESS();
	zassert_equal(count, 4);
	zassert_mem_equal(output, CUSTOM_LOG_PREFIX "ERR 1 2 3", strlen(output));
}

static void *setup(void)
{
	log_init();
	return NULL;
}

static void before(void *notused)
{
	count = 0;
	output[0] = '\0';

	log_backend_enable(&backend, NULL, LOG_LEVEL_DBG);
}

static void after(void *notused)
{
	log_backend_disable(&backend);
}

ZTEST_SUITE(log_custom_header, NULL, setup, before, after, NULL);

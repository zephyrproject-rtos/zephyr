/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Run the "net qbu" shell commands on the dummy shell backend against the
 * eth_fake driver of this suite, and check the resulting driver state
 * through the Ethernet management API.
 */

#include <zephyr/kernel.h>

#if defined(CONFIG_NET_SHELL_QBU_SUPPORTED) && defined(CONFIG_SHELL_BACKEND_DUMMY)

#include <stdarg.h>
#include <string.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/ethernet_mgmt.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>
#include <zephyr/ztest.h>

BUILD_ASSERT(NET_TC_TX_COUNT == 2, "test vectors assume CONFIG_NET_TC_TX_COUNT=2");

static const struct shell *sh;
static struct net_if *fake_iface;
static int fake_idx;

static void find_fake_iface(struct net_if *iface, void *user_data)
{
	const struct device *dev = net_if_get_device(iface);

	ARG_UNUSED(user_data);

	if (fake_iface == NULL && dev != NULL && strcmp(dev->name, "eth_fake") == 0) {
		fake_iface = iface;
	}
}

static int run(const char *fmt, ...)
{
	char cmd[64];
	va_list ap;

	va_start(ap, fmt);
	vsnprintk(cmd, sizeof(cmd), fmt, ap);
	va_end(ap);

	shell_backend_dummy_clear_output(sh);

	return shell_execute_cmd(sh, cmd);
}

static const char *output(void)
{
	size_t len;

	return shell_backend_dummy_get_output(sh, &len);
}

static int qbu_get(enum ethernet_qbu_param_type type, struct ethernet_req_params *params)
{
	memset(params, 0, sizeof(*params));
	params->qbu_param.type = type;

	return net_mgmt(NET_REQUEST_ETHERNET_GET_QBU_PARAM, fake_iface, params,
			sizeof(*params));
}

static void qbu_set(struct ethernet_req_params *params, enum ethernet_qbu_param_type type)
{
	int ret;

	params->qbu_param.port_id = 0;
	params->qbu_param.type = type;

	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_QBU_PARAM, fake_iface, params,
		       sizeof(*params));
	zassert_equal(ret, 0, "direct qbu set failed (%d)", ret);
}

static void get_table(enum ethernet_qbu_preempt_status *out)
{
	struct ethernet_req_params params;
	int ret;

	ret = qbu_get(ETHERNET_QBU_PARAM_TYPE_PREEMPTION_STATUS_TABLE, &params);
	zassert_equal(ret, 0, "cannot read preemption table (%d)", ret);

	memcpy(out, params.qbu_param.frame_preempt_statuses,
	       sizeof(params.qbu_param.frame_preempt_statuses));
}

static void set_table_all(enum ethernet_qbu_preempt_status status)
{
	struct ethernet_req_params params = {0};
	int i;

	for (i = 0; i < NET_TC_TX_COUNT; i++) {
		params.qbu_param.frame_preempt_statuses[i] = status;
	}

	qbu_set(&params, ETHERNET_QBU_PARAM_TYPE_PREEMPTION_STATUS_TABLE);
}

static void *qbu_shell_setup(void)
{
	sh = shell_backend_dummy_get_ptr();
	WAIT_FOR(shell_ready(sh), 20000, k_msleep(1));
	zassert_true(shell_ready(sh), "timed out waiting for dummy shell backend");

	net_if_foreach(find_fake_iface, NULL);
	zassert_not_null(fake_iface, "cannot find eth_fake interface");

	fake_idx = net_if_get_by_iface(fake_iface);
	zassert_true(fake_idx > 0, "invalid interface index");

	return NULL;
}

ZTEST(net_qbu_shell, test_enable)
{
	struct ethernet_req_params params;

	zassert_equal(run("net qbu enable %d on", fake_idx), 0, "enable on failed");
	zassert_equal(qbu_get(ETHERNET_QBU_PARAM_TYPE_STATUS, &params), 0);
	zassert_true(params.qbu_param.enabled, "qbu should be enabled");

	zassert_equal(run("net qbu enable %d off", fake_idx), 0, "enable off failed");
	zassert_equal(qbu_get(ETHERNET_QBU_PARAM_TYPE_STATUS, &params), 0);
	zassert_false(params.qbu_param.enabled, "qbu should be disabled");

	zassert_not_equal(run("net qbu enable %d maybe", fake_idx), 0,
			  "invalid on/off value accepted");
}

ZTEST(net_qbu_shell, test_set_preempt_mapping)
{
	enum ethernet_qbu_preempt_status tbl[NET_TC_TX_COUNT];

	set_table_all(ETHERNET_QBU_STATUS_EXPRESS);

	zassert_equal(run("net qbu set_preempt %d 0 1", fake_idx), 0, "set_preempt failed");
	get_table(tbl);
	zassert_equal(tbl[0], ETHERNET_QBU_STATUS_PREEMPTABLE, "0 must map to preemptable");
	zassert_equal(tbl[1], ETHERNET_QBU_STATUS_EXPRESS, "1 must map to express");

	zassert_equal(run("net qbu set_preempt %d 1 0", fake_idx), 0, "set_preempt failed");
	get_table(tbl);
	zassert_equal(tbl[0], ETHERNET_QBU_STATUS_EXPRESS, "1 must map to express");
	zassert_equal(tbl[1], ETHERNET_QBU_STATUS_PREEMPTABLE, "0 must map to preemptable");
}

ZTEST(net_qbu_shell, test_set_preempt_unlisted_express)
{
	enum ethernet_qbu_preempt_status tbl[NET_TC_TX_COUNT];

	set_table_all(ETHERNET_QBU_STATUS_PREEMPTABLE);

	zassert_equal(run("net qbu set_preempt %d 0", fake_idx), 0, "set_preempt failed");
	get_table(tbl);
	zassert_equal(tbl[0], ETHERNET_QBU_STATUS_PREEMPTABLE, "tc0 should be preemptable");
	zassert_equal(tbl[1], ETHERNET_QBU_STATUS_EXPRESS, "unlisted tc1 should be express");
}

ZTEST(net_qbu_shell, test_set_preempt_invalid)
{
	enum ethernet_qbu_preempt_status tbl[NET_TC_TX_COUNT];

	set_table_all(ETHERNET_QBU_STATUS_PREEMPTABLE);

	/* Out of range value, non-numeric value, missing TC, too many TCs */
	zassert_not_equal(run("net qbu set_preempt %d 2", fake_idx), 0, "value 2 accepted");
	zassert_not_equal(run("net qbu set_preempt %d x", fake_idx), 0, "value x accepted");
	zassert_not_equal(run("net qbu set_preempt %d", fake_idx), 0, "no TC accepted");
	zassert_not_equal(run("net qbu set_preempt %d 0 0 0", fake_idx), 0,
			  "too many TCs accepted");

	/* None of the rejected commands may have touched the driver state */
	get_table(tbl);
	zassert_equal(tbl[0], ETHERNET_QBU_STATUS_PREEMPTABLE, "tc0 modified");
	zassert_equal(tbl[1], ETHERNET_QBU_STATUS_PREEMPTABLE, "tc1 modified");
}

ZTEST(net_qbu_shell, test_set_frag_size)
{
	struct ethernet_req_params params;

	zassert_equal(run("net qbu set_frag_size %d 3", fake_idx), 0, "set_frag_size failed");
	zassert_equal(qbu_get(ETHERNET_QBR_PARAM_TYPE_ADDITIONAL_FRAGMENT_SIZE, &params), 0);
	zassert_equal(params.qbu_param.additional_fragment_size, 3, "wrong fragment size");

	zassert_equal(run("net qbu set_frag_size %d 1", fake_idx), 0, "set_frag_size failed");
	zassert_equal(qbu_get(ETHERNET_QBR_PARAM_TYPE_ADDITIONAL_FRAGMENT_SIZE, &params), 0);
	zassert_equal(params.qbu_param.additional_fragment_size, 1, "wrong fragment size");

	zassert_not_equal(run("net qbu set_frag_size %d 4", fake_idx), 0, "size 4 accepted");
	zassert_equal(qbu_get(ETHERNET_QBR_PARAM_TYPE_ADDITIONAL_FRAGMENT_SIZE, &params), 0);
	zassert_equal(params.qbu_param.additional_fragment_size, 1,
		      "rejected size changed the driver state");
}

ZTEST(net_qbu_shell, test_invalid_iface)
{
	zassert_not_equal(run("net qbu enable 99 on"), 0, "unknown iface accepted");
	zassert_not_equal(run("net qbu enable abc on"), 0, "non-numeric iface accepted");
	zassert_not_equal(run("net qbu get_info 99"), 0, "unknown iface accepted");
}

ZTEST(net_qbu_shell, test_get_info)
{
	struct ethernet_req_params params = {0};
	const char *out;

	params.qbu_param.enabled = true;
	qbu_set(&params, ETHERNET_QBU_PARAM_TYPE_STATUS);

	params.qbu_param.hold_advance = 20;
	qbu_set(&params, ETHERNET_QBU_PARAM_TYPE_HOLD_ADVANCE);

	params.qbu_param.release_advance = 10;
	qbu_set(&params, ETHERNET_QBU_PARAM_TYPE_RELEASE_ADVANCE);

	params.qbu_param.additional_fragment_size = 3;
	qbu_set(&params, ETHERNET_QBR_PARAM_TYPE_ADDITIONAL_FRAGMENT_SIZE);

	memset(&params, 0, sizeof(params));
	params.qbu_param.frame_preempt_statuses[0] = ETHERNET_QBU_STATUS_PREEMPTABLE;
	params.qbu_param.frame_preempt_statuses[1] = ETHERNET_QBU_STATUS_EXPRESS;
	qbu_set(&params, ETHERNET_QBU_PARAM_TYPE_PREEMPTION_STATUS_TABLE);

	zassert_equal(run("net qbu get_info %d", fake_idx), 0, "get_info failed");
	out = output();

	zassert_not_null(strstr(out, "preemption enabled: on"), "status missing:\n%s", out);
	zassert_not_null(strstr(out, "hold_advance(ns): 20"), "hold missing:\n%s", out);
	zassert_not_null(strstr(out, "release_advance(ns): 10"), "release missing:\n%s", out);
	zassert_not_null(strstr(out, "min fragment = 256 bytes"), "frag missing:\n%s", out);
	zassert_not_null(strstr(out, "tc0: preemptable"), "tc0 missing:\n%s", out);
	zassert_not_null(strstr(out, "tc1: express"), "tc1 missing:\n%s", out);
}

ZTEST_SUITE(net_qbu_shell, NULL, qbu_shell_setup, NULL, NULL, NULL);

#endif /* CONFIG_NET_SHELL_QBU_SUPPORTED && CONFIG_SHELL_BACKEND_DUMMY */

/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/wifi_nm.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>
#include <zephyr/ztest.h>

#if defined(CONFIG_WIFI_HWSIM_NAN_SHARED_IFACE)
/* Private driver header: see target_include_directories() in CMakeLists.txt. */
#include "wifi_hwsim.h"
#endif

/*
 * Exercise real net_mgmt and HostAP paths with one STA and one NAN interface.
 * hwsim supplies a delayed cluster event; no NAN frames or peers are simulated.
 */
#define TEST_TIMEOUT_MS 5000

#if defined(CONFIG_WIFI_HWSIM_NAN_SHARED_IFACE)
static struct net_if *sta_iface(void)
{
	return net_if_lookup_by_dev(DEVICE_DT_GET(DT_NODELABEL(radio0)));
}

static struct net_if *nan_iface(void)
{
	return hwsim_get_secondary_iface(DEVICE_DT_GET(DT_NODELABEL(radio0)));
}
#else
static struct net_if *nan_iface(void)
{
	return net_if_lookup_by_dev(DEVICE_DT_GET(DT_NODELABEL(nan)));
}

static struct net_if *sta_iface(void)
{
	return net_if_lookup_by_dev(DEVICE_DT_GET(DT_NODELABEL(sta)));
}
#endif

static int nan_request(enum wifi_nan_operation op, struct wifi_nan_params *params)
{
	params->op = op;
	return net_mgmt(NET_REQUEST_WIFI_NAN, nan_iface(), params, sizeof(*params));
}

static void nan_command(enum wifi_nan_operation op)
{
	struct wifi_nan_params params = {0};

	zassert_ok(nan_request(op, &params), "NAN operation %d failed", op);
	zassert_equal(strncmp(params.resp, "OK", 2), 0, "NAN response: %s", params.resp);
}

static int nan_set_request(const char *name, const char *value, struct wifi_nan_params *params)
{
	snprintk(params->set.param, sizeof(params->set.param), "%s", name);
	snprintk(params->set.value, sizeof(params->set.value), "%s", value);
	return nan_request(WIFI_NAN_OP_SET, params);
}

static void nan_set(const char *name, const char *value)
{
	struct wifi_nan_params params = {0};

	zassert_ok(nan_set_request(name, value, &params), "NAN_SET %s failed", name);
	zassert_equal(strncmp(params.resp, "OK", 2), 0, "NAN response: %s", params.resp);
}

/* Run a shell command on the dummy backend and return its captured output. */
static const char *shell_run(int *ret, const char *fmt, ...)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	static char cmd[128];
	size_t size;
	va_list ap;

	va_start(ap, fmt);
	vsnprintk(cmd, sizeof(cmd), fmt, ap);
	va_end(ap);

	shell_backend_dummy_clear_output(sh);
	*ret = shell_execute_cmd(sh, cmd);

	return shell_backend_dummy_get_output(sh, &size);
}

static void wait_registered(struct net_if *iface)
{
	int64_t deadline = k_uptime_get() + TEST_TIMEOUT_MS;

	zassert_not_null(iface);
	while (wifi_nm_get_instance_iface(iface) == NULL && k_uptime_get() < deadline) {
		k_sleep(K_MSEC(10));
	}
	zassert_not_null(wifi_nm_get_instance_iface(iface), "Interface registration timed out");
}

static void wait_unregistered(struct net_if *iface)
{
	int64_t deadline = k_uptime_get() + TEST_TIMEOUT_MS;

	while (wifi_nm_get_instance_iface(iface) != NULL && k_uptime_get() < deadline) {
		k_sleep(K_MSEC(10));
	}
	zassert_is_null(wifi_nm_get_instance_iface(iface), "Interface removal timed out");
}

static void expect_status(const char *field)
{
	struct wifi_nan_params params = {0};

	zassert_ok(nan_request(WIFI_NAN_OP_STATUS, &params));
	zassert_not_null(strstr(params.resp, field), "Expected %s in %s", field, params.resp);
}

static void wait_cluster(const char *cluster)
{
	struct wifi_nan_params params = {0};
	int64_t deadline = k_uptime_get() + TEST_TIMEOUT_MS;

	do {
		zassert_ok(nan_request(WIFI_NAN_OP_STATUS, &params));
		if (strstr(params.resp, cluster) != NULL) {
			return;
		}
		k_sleep(K_MSEC(10));
	} while (k_uptime_get() < deadline);

	zassert_unreachable("Cluster event not consumed: %s", params.resp);
}

static void test_before(void *data)
{
	ARG_UNUSED(data);

	wait_registered(sta_iface());
	wait_registered(nan_iface());
	nan_command(WIFI_NAN_OP_STOP);
	nan_set("dual_band", "0");
	nan_set("master_pref", "10");
}

static void test_after(void *data)
{
	ARG_UNUSED(data);

	if (net_if_is_admin_up(nan_iface())) {
		nan_command(WIFI_NAN_OP_STOP);
	}
}

/* Check separate roles and NAN identity, then verify NAN leaves STA status unchanged. */
ZTEST(nan_hwsim, test_registration_and_sta_isolation)
{
	struct wifi_nan_params params = {.op = WIFI_NAN_OP_START};
	struct wifi_iface_status sta_before = {0};
	struct wifi_iface_status sta_after = {0};
	int ret;

	zassert_not_equal(nan_iface(), sta_iface());
	zassert_equal(wifi_nm_get_instance_iface(nan_iface()),
		      wifi_nm_get_instance_iface(sta_iface()));
	zassert_equal(wifi_nm_get_type_iface(nan_iface()), BIT(WIFI_TYPE_NAN));
	zassert_equal(wifi_nm_get_type_iface(sta_iface()), BIT(WIFI_TYPE_STA));
	expect_status("nan_started=0\n");
	expect_status("nmi=02:00:00:00:00:02\n");
	zassert_ok(net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, sta_iface(), &sta_before,
			    sizeof(sta_before)));

	/* Synchronized NAN must not be enabled on the ordinary station interface. */
	ret = net_mgmt(NET_REQUEST_WIFI_NAN, sta_iface(), &params, sizeof(params));

	zassert_true(ret != 0 || strncmp(params.resp, "FAIL", 4) == 0,
		     "Station unexpectedly accepted NAN start");
	nan_set("cluster_id", "50:6f:9a:01:10:01");
	nan_command(WIFI_NAN_OP_START);
	wait_cluster("cluster_id=50:6f:9a:01:10:01\n");
	nan_command(WIFI_NAN_OP_STOP);

	zassert_ok(net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, sta_iface(), &sta_after,
			    sizeof(sta_after)));
	zassert_equal(sta_before.state, sta_after.state);
	zassert_equal(sta_before.iface_mode, sta_after.iface_mode);
	zassert_equal(wifi_nm_get_type_iface(sta_iface()), BIT(WIFI_TYPE_STA));
}

/* Reject duplicate start, tolerate repeated stop, and consume a fresh event after restart. */
ZTEST(nan_hwsim, test_start_cluster_stop_restart)
{
	struct wifi_nan_params params = {0};
	int ret;

	nan_set("cluster_id", "50:6f:9a:01:20:01");
	nan_command(WIFI_NAN_OP_START);
	expect_status("nan_started=1\n");
	wait_cluster("cluster_id=50:6f:9a:01:20:01\n");

	ret = nan_request(WIFI_NAN_OP_START, &params);

	zassert_true(ret != 0 || strncmp(params.resp, "FAIL", 4) == 0,
		     "Duplicate NAN start accepted");
	nan_command(WIFI_NAN_OP_STOP);
	expect_status("nan_started=0\n");
	nan_command(WIFI_NAN_OP_STOP);

	/* A new ID makes a stale status value insufficient to pass the restart. */
	nan_set("cluster_id", "50:6f:9a:01:20:02");
	nan_command(WIFI_NAN_OP_START);
	wait_cluster("cluster_id=50:6f:9a:01:20:02\n");
	nan_command(WIFI_NAN_OP_STOP);
	expect_status("nan_started=0\n");
}

/* Stop before the delayed notification and check that its cluster never reaches status. */
ZTEST(nan_hwsim, test_stop_cancels_pending_cluster)
{
	struct wifi_nan_params params = {0};

	nan_set("cluster_id", "50:6f:9a:01:30:01");
	nan_command(WIFI_NAN_OP_START);
	nan_command(WIFI_NAN_OP_STOP);

	/* Observe the entire notification window after stopping. */
	k_sleep(K_MSEC(2 * CONFIG_WIFI_HWSIM_NAN_CLUSTER_EVENT_DELAY_MS));
	expect_status("nan_started=0\n");
	zassert_ok(nan_request(WIFI_NAN_OP_STATUS, &params));
	zassert_is_null(strstr(params.resp, "cluster_id=50:6f:9a:01:30:01\n"),
			"Stopped operation delivered a cluster event");
}

/* Remove NAN with a pending event, reject requests while absent, then register and start again. */
ZTEST(nan_hwsim, test_remove_cancels_pending_cluster)
{
	struct wifi_nan_params params = {0};

	nan_set("cluster_id", "50:6f:9a:01:40:01");
	nan_command(WIFI_NAN_OP_START);
	zassert_ok(net_if_down(nan_iface()));
	wait_unregistered(nan_iface());
	/* Without an NM registration, the NAN device has no nan_cfg provider. */
	zassert_equal(nan_request(WIFI_NAN_OP_START, &params), -ENOTSUP);
	zassert_equal(nan_request(WIFI_NAN_OP_STATUS, &params), -ENOTSUP);
	k_sleep(K_MSEC(2 * CONFIG_WIFI_HWSIM_NAN_CLUSTER_EVENT_DELAY_MS));
	zassert_equal(wifi_nm_get_type_iface(sta_iface()), BIT(WIFI_TYPE_STA));

	zassert_ok(net_if_up(nan_iface()));
	wait_registered(nan_iface());
	expect_status("nan_started=0\n");
	expect_status("cluster_id=00:00:00:00:00:00\n");
	nan_set("dual_band", "0");
	nan_command(WIFI_NAN_OP_START);
	wait_cluster("cluster_id=50:6f:9a:01:00:02\n");
	nan_command(WIFI_NAN_OP_STOP);
	nan_set("cluster_id", "50:6f:9a:01:40:02");
	nan_command(WIFI_NAN_OP_START);
	wait_cluster("cluster_id=50:6f:9a:01:40:02\n");
}

/* Reject malformed management requests without preventing a subsequent valid start. */
ZTEST(nan_hwsim, test_invalid_management_requests)
{
	struct wifi_nan_params params = {0};

	zassert_equal(net_mgmt(NET_REQUEST_WIFI_NAN, nan_iface(), NULL, 0), -EINVAL);
	zassert_equal(nan_set_request("", "10", &params), -EINVAL);
	zassert_equal(nan_set_request("master_pref", "", &params), -EINVAL);
	zassert_equal(nan_request((enum wifi_nan_operation)-1, &params), -EINVAL);
	expect_status("nan_started=0\n");

	/* Rejected requests must leave the command path usable. */
	nan_set("cluster_id", "50:6f:9a:01:50:01");
	nan_command(WIFI_NAN_OP_START);
	wait_cluster("cluster_id=50:6f:9a:01:50:01\n");
}

/* HostAP must reject invalid settings while retaining the previously accepted configuration. */
ZTEST(nan_hwsim, test_rejected_configuration_preserves_valid_config)
{
	static const struct {
		const char *name;
		const char *value;
	} invalid[] = {
		{"master_pref", "0"},
		{"master_pref", "255"},
		{"cluster_id", "not-a-mac"},
		{"cluster_id", "02:00:00:00:00:02"},
	};

	nan_set("cluster_id", "50:6f:9a:01:60:01");
	for (size_t i = 0; i < ARRAY_SIZE(invalid); i++) {
		struct wifi_nan_params params = {0};
		int ret = nan_set_request(invalid[i].name, invalid[i].value, &params);

		zassert_true(ret != 0 || strncmp(params.resp, "FAIL", 4) == 0,
			     "Accepted invalid %s=%s", invalid[i].name, invalid[i].value);
	}
	expect_status("nan_started=0\n");
	/* Use the previous valid values without setting them again. */
	nan_command(WIFI_NAN_OP_START);
	wait_cluster("cluster_id=50:6f:9a:01:60:01\n");
}

/* Exercise a driver refusal, check for stray events, and retry with supported settings. */
ZTEST(nan_hwsim, test_failed_start_recovery)
{
	struct wifi_nan_params params = {0};
	int ret;

	nan_set("cluster_id", "50:6f:9a:01:70:01");
	/* HostAP accepts this setting, but the hwsim model supports one band. */
	nan_set("dual_band", "1");
	ret = nan_request(WIFI_NAN_OP_START, &params);
	zassert_true(ret != 0 || strncmp(params.resp, "FAIL", 4) == 0,
		     "Unsupported dual-band start accepted");
	expect_status("nan_started=0\n");
	k_sleep(K_MSEC(2 * CONFIG_WIFI_HWSIM_NAN_CLUSTER_EVENT_DELAY_MS));
	zassert_ok(nan_request(WIFI_NAN_OP_STATUS, &params));
	zassert_is_null(strstr(params.resp, "cluster_id=50:6f:9a:01:70:01\n"),
			"Failed start delivered a cluster event");
	nan_command(WIFI_NAN_OP_STOP);
	nan_set("dual_band", "0");
	nan_command(WIFI_NAN_OP_START);
	wait_cluster("cluster_id=50:6f:9a:01:70:01\n");
	expect_status("nan_started=1\n");
}

/* Removing and re-registering STA must preserve the active NAN context and cluster. */
ZTEST(nan_hwsim, test_sta_reregistration_preserves_nan)
{
	struct wifi_iface_status sta_status = {0};

	nan_set("cluster_id", "50:6f:9a:01:80:01");
	nan_command(WIFI_NAN_OP_START);
	wait_cluster("cluster_id=50:6f:9a:01:80:01\n");
	zassert_ok(net_if_down(sta_iface()));
	wait_unregistered(sta_iface());
	zassert_equal(wifi_nm_get_type_iface(nan_iface()), BIT(WIFI_TYPE_NAN));
	expect_status("nan_started=1\n");
	expect_status("cluster_id=50:6f:9a:01:80:01\n");

	zassert_ok(net_if_up(sta_iface()));
	wait_registered(sta_iface());
	zassert_equal(wifi_nm_get_instance_iface(sta_iface()),
		      wifi_nm_get_instance_iface(nan_iface()));
	zassert_equal(wifi_nm_get_type_iface(sta_iface()), BIT(WIFI_TYPE_STA));
	zassert_ok(net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, sta_iface(), &sta_status,
			    sizeof(sta_status)));
	zassert_equal(sta_status.iface_mode, WIFI_MODE_INFRA);
	expect_status("nan_started=1\n");
	expect_status("cluster_id=50:6f:9a:01:80:01\n");
	expect_status("nmi=02:00:00:00:00:02\n");
}

/*
 * This test covers below:
 * "wifi nan ..." shell commands must resolve to the dedicated NAN interface, not silently
 * fall back to the station interface (which rejects NAN operations, see
 * test_registration_and_sta_isolation above).
 */
ZTEST(nan_hwsim, test_shell_nan_start_stop_status)
{
	const char *out;
	int ret;

	shell_run(&ret, "wifi nan set cluster_id 50:6f:9a:01:90:01");
	zassert_ok(ret, "shell: NAN set cluster_id failed");
	shell_run(&ret, "wifi nan set dual_band 0");
	zassert_ok(ret, "shell: NAN set dual_band failed");
	shell_run(&ret, "wifi nan set master_pref 10");
	zassert_ok(ret, "shell: NAN set master_pref failed");

	shell_run(&ret, "wifi nan start");
	zassert_ok(ret, "shell: NAN start failed");

	out = shell_run(&ret, "wifi nan status");
	zassert_ok(ret, "shell: NAN status failed");
	zassert_not_null(strstr(out, "nan_started=1"),
			  "Shell NAN start did not reach the NAN interface: %s", out);

	wait_cluster("cluster_id=50:6f:9a:01:90:01\n");

	out = shell_run(&ret, "wifi nan status");
	zassert_ok(ret, "shell: NAN status failed");
	zassert_not_null(strstr(out, "cluster_id=50:6f:9a:01:90:01"),
			  "Unexpected cluster in shell NAN status: %s", out);

	shell_run(&ret, "wifi nan stop");
	zassert_ok(ret, "shell: NAN stop failed");

	out = shell_run(&ret, "wifi nan status");
	zassert_ok(ret, "shell: NAN status failed");
	zassert_not_null(strstr(out, "nan_started=0"),
			  "Shell NAN stop did not reach the NAN interface: %s", out);
}

ZTEST_SUITE(nan_hwsim, NULL, NULL, test_before, test_after, NULL);

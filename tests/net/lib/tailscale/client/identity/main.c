/*
 * SPDX-FileCopyrightText: FoBE Studio
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

/* Reinitialize the singleton in this fixture while retaining real NVS Settings. */
#include "client.c"

static struct net_if *underlay;
static enum tailscale_watch_state mock_watch_state;
static int enroll_result;
static int expire_result;
static bool fail_save;
static bool ephemeral;
static unsigned int expire_calls;
static unsigned int login_calls;
static int ingress_validation_result;

int __real_settings_save_one(const char *name, const void *value, size_t len);

int __wrap_settings_save_one(const char *name, const void *value, size_t len)
{
	return fail_save ? -EIO : __real_settings_save_one(name, value, len);
}

bool tailscale_control_registration_valid(const char *hostname, const char *auth_key,
					uint32_t timeout_ms)
{
	ARG_UNUSED(auth_key);
	return hostname != NULL && timeout_ms > 0;
}

int tailscale_control_enroll(const struct tailscale_control_params *params)
{
	zassert_equal(mock_watch_state, TAILSCALE_WATCH_STOPPED);
	zassert_equal_ptr(params->underlay, underlay);
	zassert_equal(params->ephemeral, ephemeral);
	if (enroll_result == -EINPROGRESS && params->login_required != NULL) {
		params->login_required("https://fixture.invalid/authorize", params->login_context);
	}
	return enroll_result;
}

int tailscale_control_logout(const struct tailscale_control_params *params)
{
	struct identity_record record;

	zassert_equal(mock_watch_state, TAILSCALE_WATCH_STOPPED);
	zassert_is_null(params->auth_key);
	zassert_not_null(params->node_private);
	if (!params->ephemeral) {
		zassert_equal(settings_load_one(IDENTITY_SETTINGS_KEY, &record, sizeof(record)),
			      sizeof(record));
		zassert_equal(record.state, IDENTITY_RECORD_RETIRED);
	}
	expire_calls++;
	return expire_result;
}

int tailscale_control_watch_start(const struct tailscale_control_params *params, struct net_if *lan)
{
	zassert_equal_ptr(lan, underlay);
	zassert_is_null(params->auth_key);
	zassert_is_null(params->login_required);
	mock_watch_state = TAILSCALE_WATCH_RECEIVING;
	return 0;
}

int tailscale_control_watch_stop(void)
{
	mock_watch_state = TAILSCALE_WATCH_STOPPED;
	return 0;
}

void tailscale_control_watch_status(struct tailscale_watch_status *status)
{
	*status = (struct tailscale_watch_status){
		.state = mock_watch_state,
		.valid = mock_watch_state == TAILSCALE_WATCH_RECEIVING,
	};
}

int tailscale_ingress_validate(const struct tailscale_service_rule *rules, size_t count)
{
	ARG_UNUSED(rules);
	ARG_UNUSED(count);
	return ingress_validation_result;
}

int tailscale_ingress_replace(const struct tailscale_service_rule *rules, size_t count)
{
	return tailscale_ingress_validate(rules, count);
}

int tailscale_routes_set(const struct tailscale_routes *routes)
{
	zassert_not_null(routes);
	return 0;
}

static void login(const char *url, void *context)
{
	struct tailscale_watch_status status;

	zassert_equal_ptr(context, &login_calls);
	zassert_equal(strcmp(url, "https://fixture.invalid/authorize"), 0);
	zassert_ok(tailscale_client_status(&status));
	zassert_false(status.valid);
	zassert_equal(tailscale_client_start(), -EBUSY);
	zassert_equal(tailscale_client_logout(NULL), -EBUSY);
	login_calls++;
}

static int init_client(enum tailscale_identity_mode mode)
{
	const struct tailscale_client_config config = {
		.underlay = underlay,
		.hostname = "fixture",
		.ca_tag = 1,
		.timeout_ms = 45000,
		.identity_mode = mode,
	};

	mbedtls_platform_zeroize(&client, sizeof(client));
	ephemeral = mode == TAILSCALE_IDENTITY_EPHEMERAL;
	mock_watch_state = TAILSCALE_WATCH_STOPPED;
	return tailscale_client_init(&config);
}

static void *setup(void)
{
	struct net_in_addr address = {.s_addr = net_htonl(0xc0000201)};

	zassert_ok(settings_subsys_init());
	underlay = net_if_get_default();
	zassert_not_null(underlay);
	zassert_not_null(net_if_ipv4_addr_add(underlay, &address, NET_ADDR_MANUAL, 0));
	return NULL;
}

static void before(void *unused)
{
	const struct timespec realtime = {.tv_sec = TAILSCALE_MIN_VALID_EPOCH + 60};

	ARG_UNUSED(unused);
	zassert_ok(sys_clock_settime(SYS_CLOCK_REALTIME, &realtime));
	fail_save = false;
	enroll_result = 0;
	expire_result = 0;
	expire_calls = 0;
	login_calls = 0;
	ingress_validation_result = 0;
	zassert_ok(settings_delete(IDENTITY_SETTINGS_KEY));
}

ZTEST(tailscale_identity, test_invalid_start_configuration_does_not_claim_client)
{
	struct tailscale_client_config config = {
		.underlay = underlay,
		.hostname = "fixture",
		.ca_tag = 1,
		.timeout_ms = 45000,
		.identity_mode = TAILSCALE_IDENTITY_EPHEMERAL,
		.auth_key = "fixture-key",
	};
	struct net_if *iface;

	mbedtls_platform_zeroize(&client, sizeof(client));
	ephemeral = true;
	mock_watch_state = TAILSCALE_WATCH_STOPPED;
	ingress_validation_result = -EINVAL;
	zassert_equal(tailscale_start(&config, &iface), -EINVAL);

	ingress_validation_result = 0;
	config.timeout_ms += 1000;
	zassert_ok(tailscale_start(&config, &iface));
	zassert_not_null(iface);
	zassert_ok(tailscale_stop(TAILSCALE_STOP_KEEP_IDENTITY));
}

ZTEST(tailscale_identity, test_migrate_and_reject_corrupt_records)
{
	struct tailscale_identity legacy = {0};
	struct identity_record record;
	const struct timespec realtime = {0};
	uint8_t expected[TAILSCALE_NODE_KEY_SIZE], actual[TAILSCALE_NODE_KEY_SIZE];

	zassert_ok(settings_save_one(IDENTITY_SETTINGS_KEY, &legacy, sizeof(legacy)));
	zassert_equal(init_client(TAILSCALE_IDENTITY_PERSISTENT), -EBADMSG);
	memset(legacy.machine, 0x25, sizeof(legacy.machine));
	memset(legacy.node, 0x41, sizeof(legacy.node));
	memset(legacy.disco, 0x83, sizeof(legacy.disco));
	zassert_ok(node_public_key(&legacy, expected));
	zassert_ok(settings_save_one(IDENTITY_SETTINGS_KEY, &legacy, sizeof(legacy)));
	fail_save = true;
	zassert_equal(init_client(TAILSCALE_IDENTITY_PERSISTENT), -EIO);
	fail_save = false;
	zassert_ok(init_client(TAILSCALE_IDENTITY_PERSISTENT));
	zassert_ok(tailscale_client_node_public_key(actual));
	zassert_mem_equal(actual, expected, sizeof(actual));
	zassert_ok(sys_clock_settime(SYS_CLOCK_REALTIME, &realtime));
	zassert_equal(tailscale_client_start(), -ENODATA);
	zassert_equal(tailscale_client_enroll(NULL, NULL, NULL), -ENODATA);
	zassert_equal(settings_load_one(IDENTITY_SETTINGS_KEY, &record, sizeof(record)),
		      sizeof(record));
	record.keys.node[12] ^= 1;
	zassert_ok(settings_save_one(IDENTITY_SETTINGS_KEY, &record, sizeof(record)));
	zassert_equal(init_client(TAILSCALE_IDENTITY_PERSISTENT), -EBADMSG);
	zassert_equal(tailscale_client_start(), -EAGAIN);
}

ZTEST(tailscale_identity, test_stop_and_offline_logout)
{
	uint8_t pending[TAILSCALE_NODE_KEY_SIZE], actual[TAILSCALE_NODE_KEY_SIZE];
	int remote_error;

	zassert_ok(init_client(TAILSCALE_IDENTITY_PERSISTENT));
	enroll_result = -EINPROGRESS;
	int rc = tailscale_client_enroll(NULL, login, &login_calls);

	zassert_equal(rc, -EINPROGRESS, "enrollment returned %d", rc);
	zassert_equal(login_calls, 1);
	zassert_ok(tailscale_client_node_public_key(pending));
	zassert_ok(init_client(TAILSCALE_IDENTITY_PERSISTENT));
	zassert_equal(tailscale_client_start(), -ENOENT);
	enroll_result = 0;
	zassert_ok(tailscale_client_enroll(NULL, NULL, NULL));
	zassert_ok(tailscale_client_start());
	zassert_ok(tailscale_client_stop());
	zassert_equal(expire_calls, 0);
	zassert_ok(init_client(TAILSCALE_IDENTITY_PERSISTENT));
	zassert_ok(tailscale_client_node_public_key(actual));
	zassert_mem_equal(actual, pending, sizeof(actual));
	zassert_ok(tailscale_client_start());

	fail_save = true;
	zassert_equal(tailscale_client_logout(&remote_error), -EIO);
	zassert_equal(mock_watch_state, TAILSCALE_WATCH_STOPPED);
	zassert_equal(expire_calls, 0);
	zassert_equal(tailscale_client_start(), -EBUSY);
	fail_save = false;
	expire_result = -ENETUNREACH;
	zassert_ok(tailscale_client_logout(&remote_error));
	zassert_equal(remote_error, -ENETUNREACH);
	zassert_equal(expire_calls, 1);
	zassert_ok(init_client(TAILSCALE_IDENTITY_PERSISTENT));
	zassert_equal(tailscale_client_start(), -ENOENT);
	zassert_ok(tailscale_client_enroll(NULL, NULL, NULL));
	zassert_ok(tailscale_client_node_public_key(actual));
	zassert_not_equal(memcmp(actual, pending, sizeof(actual)), 0);
}

ZTEST(tailscale_identity, test_ephemeral_identity_leaves_persistent_state_unchanged)
{
	uint8_t node[TAILSCALE_NODE_KEY_SIZE], persisted[TAILSCALE_NODE_KEY_SIZE];

	zassert_ok(init_client(TAILSCALE_IDENTITY_PERSISTENT));
	zassert_ok(tailscale_client_enroll(NULL, NULL, NULL));
	zassert_ok(tailscale_client_node_public_key(persisted));
	zassert_ok(init_client(TAILSCALE_IDENTITY_EPHEMERAL));
	zassert_equal(tailscale_client_start(), -ENOENT);
	zassert_ok(tailscale_client_enroll("fixture-key", NULL, NULL));
	zassert_ok(tailscale_client_node_public_key(node));
	zassert_not_equal(memcmp(node, persisted, sizeof(node)), 0);
	zassert_ok(tailscale_client_logout(NULL));
	zassert_ok(init_client(TAILSCALE_IDENTITY_PERSISTENT));
	zassert_ok(tailscale_client_node_public_key(node));
	zassert_mem_equal(node, persisted, sizeof(node));
}

ZTEST_SUITE(tailscale_identity, NULL, setup, before, NULL, NULL);

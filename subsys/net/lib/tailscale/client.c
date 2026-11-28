/*
 * SPDX-FileCopyrightText: FoBE Studio
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <mbedtls/platform_util.h>
#include <psa/crypto.h>
#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/clock.h>

#include "client_internal.h"

#include <zephyr/net/net_if.h>
#include <zephyr/sys/atomic.h>

#include "control.h"
#include "ingress.h"
#include "net_stack.h"
#include "policy.h"

#define IDENTITY_SETTINGS_KEY "tailscale/identity_v1"

enum identity_record_state {
	IDENTITY_RECORD_READY = 1,
	IDENTITY_RECORD_RETIRED,
	IDENTITY_RECORD_PROVISIONING,
};

struct identity_record {
	uint8_t magic[3];
	uint8_t state;
	struct tailscale_identity keys;
	uint8_t checksum[PSA_HASH_LENGTH(PSA_ALG_SHA_256)];
};

struct identity_load {
	struct tailscale_identity *keys;
	enum identity_record_state state;
	int result;
	bool migrate;
};

static const uint8_t identity_magic[] = {'T', 'N', 2};

BUILD_ASSERT(sizeof(struct tailscale_identity) == 96);
BUILD_ASSERT(sizeof(struct identity_record) == 132);

int tailscale_realtime_now(struct timespec *now)
{
	struct timespec realtime;
	int rc = sys_clock_gettime(SYS_CLOCK_REALTIME, &realtime);

	if (rc < 0 || realtime.tv_sec < TAILSCALE_MIN_VALID_EPOCH) {
		return -ENODATA;
	}
	if (now != NULL) {
		*now = realtime;
	}
	return 0;
}

static bool all_bytes_are(const void *data, size_t len, uint8_t value)
{
	const uint8_t *bytes = data;
	uint8_t bits = 0;

	for (size_t i = 0; i < len; i++) {
		bits |= bytes[i] ^ value;
	}
	return bits == 0;
}

static bool same_scalar(const uint8_t *a, const uint8_t *b)
{
	uint8_t bits = (a[0] ^ b[0]) & 0xf8;

	for (size_t i = 1; i < TAILSCALE_NODE_KEY_SIZE - 1; i++) {
		bits |= a[i] ^ b[i];
	}
	bits |= (a[TAILSCALE_NODE_KEY_SIZE - 1] ^ b[TAILSCALE_NODE_KEY_SIZE - 1]) & 0x3f;
	return bits == 0;
}

static bool valid_identity(const struct tailscale_identity *identity)
{
	const uint8_t *const keys[] = {identity->machine, identity->node, identity->disco};

	for (size_t i = 0; i < ARRAY_SIZE(keys); i++) {
		if (all_bytes_are(keys[i], TAILSCALE_NODE_KEY_SIZE, 0) ||
		    all_bytes_are(keys[i], TAILSCALE_NODE_KEY_SIZE, UINT8_MAX)) {
			return false;
		}
		for (size_t j = 0; j < i; j++) {
			if (same_scalar(keys[i], keys[j])) {
				return false;
			}
		}
	}
	return true;
}

static int save_identity(enum identity_record_state state,
			 const struct tailscale_identity *identity)
{
	struct identity_record record = {.state = state};
	size_t len;
	int rc = -EIO;

	memcpy(record.magic, identity_magic, sizeof(record.magic));
	if (identity != NULL) {
		record.keys = *identity;
	}
	if (psa_hash_compute(PSA_ALG_SHA_256, (const uint8_t *)&record,
			     offsetof(struct identity_record, checksum), record.checksum,
			     sizeof(record.checksum), &len) == PSA_SUCCESS &&
	    len == sizeof(record.checksum)) {
		rc = settings_save_one(IDENTITY_SETTINGS_KEY, &record, sizeof(record));
	}
	mbedtls_platform_zeroize(&record, sizeof(record));
	return rc;
}

static int decode_identity(const struct identity_record *record, struct identity_load *load)
{
	if (memcmp(record->magic, identity_magic, sizeof(record->magic)) != 0 ||
	    psa_hash_compare(PSA_ALG_SHA_256, (const uint8_t *)record,
			     offsetof(struct identity_record, checksum), record->checksum,
			     sizeof(record->checksum)) != PSA_SUCCESS) {
		return -EBADMSG;
	}
	if (record->state == IDENTITY_RECORD_RETIRED &&
	    all_bytes_are(&record->keys, sizeof(record->keys), 0)) {
		load->state = IDENTITY_RECORD_RETIRED;
		return 0;
	}
	if ((record->state != IDENTITY_RECORD_READY &&
	     record->state != IDENTITY_RECORD_PROVISIONING) ||
	    !valid_identity(&record->keys)) {
		return -EBADMSG;
	}
	load->state = record->state;
	*load->keys = record->keys;
	return 0;
}

static int read_identity(const char *key, size_t len, settings_read_cb read_cb, void *cb_arg,
			 void *param)
{
	struct identity_load *load = param;
	struct identity_record record;
	bool legacy = len == sizeof(record.keys);

	if (key != NULL) {
		return 0;
	}
	if (len != sizeof(record) && !legacy) {
		load->result = -EBADMSG;
		return 1;
	}
	ssize_t count = read_cb(cb_arg, legacy ? (void *)&record.keys : (void *)&record, len);

	if (count != (ssize_t)len) {
		load->result = count < 0 ? count : -EBADMSG;
	} else if (legacy) {
		load->result = -EBADMSG;
		if (valid_identity(&record.keys)) {
			*load->keys = record.keys;
			load->state = IDENTITY_RECORD_READY;
			load->result = 0;
			load->migrate = true;
		}
	} else {
		load->result = decode_identity(&record, load);
	}
	mbedtls_platform_zeroize(&record, sizeof(record));
	return 1;
}

static int load_identity(struct tailscale_identity *identity, enum identity_record_state *state)
{
	struct identity_load load = {.keys = identity, .state = IDENTITY_RECORD_RETIRED};
	int rc = settings_subsys_init();

	if (rc == 0) {
		rc = settings_load_subtree_direct(IDENTITY_SETTINGS_KEY, read_identity, &load);
	}
	if (rc == 0) {
		rc = load.migrate ? save_identity(load.state, identity) : load.result;
	}
	*state = load.state;
	if (rc != 0 || *state == IDENTITY_RECORD_RETIRED) {
		mbedtls_platform_zeroize(identity, sizeof(*identity));
	}
	return rc;
}

static int new_identity(struct tailscale_identity *identity)
{
	int rc = sys_csrand_get(identity, sizeof(*identity));

	if (rc == 0 && !valid_identity(identity)) {
		rc = -EIO;
	}
	if (rc != 0) {
		mbedtls_platform_zeroize(identity, sizeof(*identity));
	}
	return rc;
}

static int node_public_key(const struct tailscale_identity *identity, uint8_t *public_key)
{
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key = PSA_KEY_ID_NULL;
	size_t len;

	psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY));
	psa_set_key_bits(&attributes, 255);
	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_DERIVE | PSA_KEY_USAGE_EXPORT);
	psa_set_key_algorithm(&attributes, PSA_ALG_ECDH);
	psa_status_t status =
		psa_import_key(&attributes, identity->node, sizeof(identity->node), &key);

	psa_reset_key_attributes(&attributes);
	if (status != PSA_SUCCESS) {
		return -EIO;
	}
	status = psa_export_public_key(key, public_key, TAILSCALE_NODE_KEY_SIZE, &len);
	(void)psa_destroy_key(key);
	return status == PSA_SUCCESS && len == TAILSCALE_NODE_KEY_SIZE ? 0 : -EIO;
}

enum client_identity_state {
	CLIENT_IDENTITY_EMPTY,
	CLIENT_IDENTITY_PROVISIONING,
	CLIENT_IDENTITY_AVAILABLE,
	CLIENT_IDENTITY_RETIRING,
};

static struct {
	struct tailscale_control_params control;
	struct tailscale_identity keys;
	char hostname[TAILSCALE_HOSTNAME_MAX + 1];
	enum client_identity_state identity;
	atomic_t initialized;
	bool enrolling;
	bool retirement_saved;
} client;

K_MUTEX_DEFINE(client_lock);

static int enter_client(bool initializing)
{
	if (k_mutex_lock(&client_lock, K_NO_WAIT) != 0) {
		return -EBUSY;
	}
	int rc = 0;

	if (client.enrolling) {
		rc = -EBUSY;
	} else if (initializing == !!atomic_get(&client.initialized)) {
		rc = initializing ? -EALREADY : -EAGAIN;
	}
	if (rc != 0) {
		k_mutex_unlock(&client_lock);
	}
	return rc;
}

int tailscale_client_init(const struct tailscale_client_config *config)
{
	if (config == NULL || config->underlay == NULL || config->ca_tag < 0 ||
	    (config->identity_mode != TAILSCALE_IDENTITY_PERSISTENT &&
	     config->identity_mode != TAILSCALE_IDENTITY_EPHEMERAL) ||
	    !tailscale_control_registration_valid(config->hostname, NULL, config->timeout_ms)) {
		return -EINVAL;
	}
	int rc = enter_client(true);

	if (rc != 0) {
		return rc;
	}
	bool ephemeral = config->identity_mode == TAILSCALE_IDENTITY_EPHEMERAL;

	if (psa_crypto_init() != PSA_SUCCESS) {
		rc = -EIO;
	} else if (!ephemeral) {
		enum identity_record_state state;

		rc = load_identity(&client.keys, &state);
		if (rc == 0) {
			if (state == IDENTITY_RECORD_READY) {
				client.identity = CLIENT_IDENTITY_AVAILABLE;
			} else if (state == IDENTITY_RECORD_PROVISIONING) {
				client.identity = CLIENT_IDENTITY_PROVISIONING;
			}
		}
	}
	if (rc == 0) {
		strcpy(client.hostname, config->hostname);
		client.control = (struct tailscale_control_params){
			.machine_private = client.keys.machine,
			.node_private = client.keys.node,
			.disco_private = client.keys.disco,
			.underlay = config->underlay,
			.hostname = client.hostname,
			.ca_tag = config->ca_tag,
			.timeout_ms = config->timeout_ms,
			.ephemeral = ephemeral,
		};
		atomic_set(&client.initialized, 1);
	}
	k_mutex_unlock(&client_lock);
	return rc;
}

int tailscale_client_enroll(const char *auth_key, tailscale_login_cb_t login, void *context)
{
	int rc = enter_client(false);

	if (rc != 0) {
		return rc;
	}
	struct tailscale_control_params params = client.control;
	struct tailscale_watch_status status;

	tailscale_control_watch_status(&status);
	if (client.identity == CLIENT_IDENTITY_RETIRING ||
	    (status.state != TAILSCALE_WATCH_STOPPED && status.state != TAILSCALE_WATCH_FAILED)) {
		rc = -EBUSY;
	} else if (!tailscale_control_registration_valid(params.hostname, auth_key,
						       params.timeout_ms)) {
		rc = -EINVAL;
	} else {
		rc = tailscale_realtime_now(NULL);
	}
	if (rc == 0 && client.identity == CLIENT_IDENTITY_EMPTY) {
		rc = new_identity(&client.keys);
		if (rc == 0 && !params.ephemeral) {
			rc = save_identity(IDENTITY_RECORD_PROVISIONING, &client.keys);
		}
		if (rc == 0) {
			client.identity = CLIENT_IDENTITY_PROVISIONING;
		} else {
			mbedtls_platform_zeroize(&client.keys, sizeof(client.keys));
		}
	}
	if (rc == 0) {
		params.auth_key = auth_key;
		params.login_required = login;
		params.login_context = context;
		client.enrolling = true;
		rc = tailscale_control_enroll(&params);
		client.enrolling = false;
		if (rc == 0 && client.identity == CLIENT_IDENTITY_PROVISIONING) {
			if (!params.ephemeral) {
				rc = save_identity(IDENTITY_RECORD_READY, &client.keys);
			}
			if (rc == 0) {
				client.identity = CLIENT_IDENTITY_AVAILABLE;
			}
		}
	}
	k_mutex_unlock(&client_lock);
	return rc;
}

int tailscale_client_start(void)
{
	int rc = enter_client(false);

	if (rc != 0) {
		return rc;
	}
	struct tailscale_control_params params = client.control;
	struct net_if *underlay = params.underlay;
	const struct net_in_addr *address =
		net_if_ipv4_get_global_addr(underlay, NET_ADDR_PREFERRED);

	if (client.identity == CLIENT_IDENTITY_RETIRING) {
		rc = -EBUSY;
	} else if (client.identity != CLIENT_IDENTITY_AVAILABLE) {
		rc = -ENOENT;
	} else if (address == NULL) {
		rc = -ENETDOWN;
	} else {
		rc = tailscale_realtime_now(NULL);
	}
	if (rc == 0) {
		struct net_sockaddr_in *endpoint = net_sin(net_sad(&params.local_endpoint));

		endpoint->sin_family = NET_AF_INET;
		endpoint->sin_addr = *address;
#ifdef CONFIG_TAILSCALE_DISCO
		endpoint->sin_port = net_htons(CONFIG_TAILSCALE_DERP_UDP_PORT);
#else
		endpoint->sin_port = net_htons(CONFIG_WIREGUARD_PORT);
#endif
		rc = tailscale_control_watch_start(&params, underlay);
	}
	k_mutex_unlock(&client_lock);
	return rc;
}

int tailscale_client_status(struct tailscale_watch_status *status)
{
	if (status == NULL) {
		return -EINVAL;
	}
	if (!atomic_get(&client.initialized)) {
		return -EAGAIN;
	}
	tailscale_control_watch_status(status);
	return 0;
}

int tailscale_client_stop(void)
{
	int rc = enter_client(false);

	if (rc == 0) {
		rc = tailscale_control_watch_stop();
		k_mutex_unlock(&client_lock);
	}
	return rc;
}

int tailscale_client_logout(int *remote_error)
{
	int rc = enter_client(false);

	if (rc != 0) {
		return rc;
	}
	if (remote_error != NULL) {
		*remote_error = 0;
	}
	if (client.identity == CLIENT_IDENTITY_EMPTY) {
		goto out;
	}
	client.identity = CLIENT_IDENTITY_RETIRING;
	int stop_error = tailscale_control_watch_stop();

	if (!client.control.ephemeral && !client.retirement_saved) {
		rc = save_identity(IDENTITY_RECORD_RETIRED, NULL);
		client.retirement_saved = rc == 0;
	}
	if (rc == 0) {
		rc = stop_error;
	}
	if (rc == 0) {
		int expire_error = tailscale_control_logout(&client.control);

		mbedtls_platform_zeroize(&client.keys, sizeof(client.keys));
		client.identity = CLIENT_IDENTITY_EMPTY;
		client.retirement_saved = false;
		if (remote_error != NULL) {
			*remote_error = expire_error;
		}
	}
out:
	k_mutex_unlock(&client_lock);
	return rc;
}

int tailscale_client_node_public_key(uint8_t public_key[TAILSCALE_NODE_KEY_SIZE])
{
	if (public_key == NULL) {
		return -EINVAL;
	}
	if (k_mutex_lock(&client_lock, K_NO_WAIT) != 0) {
		return -EBUSY;
	}
	int rc = -ENOENT;

	if (!atomic_get(&client.initialized)) {
		rc = -EAGAIN;
	} else if (client.identity == CLIENT_IDENTITY_AVAILABLE ||
		   client.identity == CLIENT_IDENTITY_PROVISIONING) {
		rc = node_public_key(&client.keys, public_key);
	}
	k_mutex_unlock(&client_lock);
	return rc;
}

static K_MUTEX_DEFINE(api_lock);

static int validate_configuration(const struct tailscale_routes *routes,
				  const struct tailscale_service_rule *services,
				  size_t service_count)
{
	if (routes != NULL && memchr(routes->exit_node, '\0', sizeof(routes->exit_node)) == NULL) {
		return -EINVAL;
	}

	return tailscale_ingress_validate(services, service_count);
}

static int configure(const struct tailscale_routes *routes,
		     const struct tailscale_service_rule *services, size_t service_count)
{
	static const struct tailscale_routes defaults;
	int rc = tailscale_routes_set(routes != NULL ? routes : &defaults);

	if (rc == 0) {
		rc = tailscale_ingress_replace(services, service_count);
	}
	return rc;
}

int tailscale_start(const struct tailscale_client_config *config, struct net_if **iface)
{
	struct tailscale_watch_status status;
	struct net_if *overlay;
	int rc;

	if (config == NULL || config->underlay == NULL || config->hostname == NULL ||
	    !tailscale_control_registration_valid(config->hostname, config->auth_key,
						  config->timeout_ms)) {
		return -EINVAL;
	}
	int index = net_if_get_by_name("wg0");

	if (index < 0) {
		return index;
	}
	overlay = net_if_get_by_index(index);
	if (k_mutex_lock(&api_lock, K_NO_WAIT) != 0) {
		return -EBUSY;
	}

	tailscale_control_watch_status(&status);
	if (status.state != TAILSCALE_WATCH_STOPPED && status.state != TAILSCALE_WATCH_FAILED) {
		rc = -EALREADY;
		goto out;
	}
	rc = validate_configuration(config->routes, config->services, config->service_count);
	if (rc != 0) {
		goto out;
	}
	rc = tailscale_client_init(config);
	if (rc == -EALREADY) {
		/* One stored identity and underlay are owned for the lifetime of the image. */
		if (config->underlay != client.control.underlay ||
		    strcmp(config->hostname, client.hostname) != 0 ||
		    config->ca_tag != client.control.ca_tag ||
		    config->timeout_ms != client.control.timeout_ms ||
		    config->identity_mode != (client.control.ephemeral
						      ? TAILSCALE_IDENTITY_EPHEMERAL
						      : TAILSCALE_IDENTITY_PERSISTENT)) {
			rc = -EINVAL;
			goto out;
		}
		rc = 0;
	}
	if (rc == 0) {
		rc = configure(config->routes, config->services, config->service_count);
	}
	if (rc != 0) {
		goto out;
	}

	rc = tailscale_client_start();
	if (rc == -ENOENT && (config->auth_key != NULL || config->login != NULL ||
			      client.identity == CLIENT_IDENTITY_PROVISIONING)) {
		rc = tailscale_client_enroll(config->auth_key, config->login,
					     config->login_context);
		if (rc == 0) {
			rc = tailscale_client_start();
		}
	}
	if (rc == 0 && iface != NULL) {
		*iface = overlay;
	}
out:
	k_mutex_unlock(&api_lock);
	return rc;
}

int tailscale_configure(const struct tailscale_routes *routes,
			const struct tailscale_service_rule *services, size_t service_count)
{
	int rc;

	if (k_mutex_lock(&api_lock, K_NO_WAIT) != 0) {
		return -EBUSY;
	}
	rc = tailscale_net_active() ? validate_configuration(routes, services, service_count)
				   : -ENETDOWN;
	if (rc == 0) {
		rc = configure(routes, services, service_count);
	}
	k_mutex_unlock(&api_lock);
	return rc;
}

int tailscale_stop(enum tailscale_stop_mode mode)
{
	int rc;
	int remote_error = 0;

	if (mode != TAILSCALE_STOP_KEEP_IDENTITY && mode != TAILSCALE_STOP_LOGOUT) {
		return -EINVAL;
	}
	if (k_mutex_lock(&api_lock, K_NO_WAIT) != 0) {
		return -EBUSY;
	}
	rc = mode == TAILSCALE_STOP_LOGOUT ? tailscale_client_logout(&remote_error) :
		tailscale_client_stop();
	k_mutex_unlock(&api_lock);
	return rc != 0 ? rc : remote_error;
}

/*
 * Copyright (c) 2026 Siratul Islam <email@sirat.me>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/logging/log.h>
#include <zephyr/fido2/fido2_types.h>
#include <zephyr/fido2/fido2_storage.h>
#include <zephyr/fido2/fido2_transport.h>

#include "fido2_cbor.h"

LOG_MODULE_REGISTER(fido2, CONFIG_FIDO2_LOG_LEVEL);

struct fido2_msg {
	const struct fido2_transport *transport;
	uint32_t cid;
	size_t len;
	/* data[0] = cmd, data[1..n] = CBOR */
	uint8_t data[CONFIG_FIDO2_CBOR_MAX_SIZE + 1];
};

/*
 * The transport callback may run on the system workqueue where stack can
 * be tight. Keep the queue item out.
 */
K_MUTEX_DEFINE(rx_enqueue_mutex);
static struct fido2_msg rx_enqueue_msg;
/* Reused to minimize thread stack usage. */
static struct fido2_msg rx_dequeue_msg;

K_MSGQ_DEFINE(fido2_msgq, sizeof(struct fido2_msg), 2, 4);

static K_THREAD_STACK_DEFINE(fido2_stack, CONFIG_FIDO2_THREAD_STACK_SIZE);
static struct k_thread fido2_thread;

static uint8_t ctap_tx_frame[CONFIG_FIDO2_CBOR_MAX_SIZE];

static atomic_t fido2_running;

static k_timepoint_t reset_timeout;

static enum fido2_status handle_get_info(uint8_t *cbor_out, size_t cbor_out_cap,
					 size_t *cbor_out_len)
{
	struct fido2_device_info info = {0};
	int ret;

	info.versions[0] = "FIDO_2_0";
	info.versions[1] = "FIDO_2_1";
	info.num_versions = 2;
	info.num_pin_uv_auth_protocols = 0;
	info.max_credential_count = CONFIG_FIDO2_MAX_CREDENTIALS;
	info.max_msg_size = CONFIG_FIDO2_CBOR_MAX_SIZE;
	info.max_credential_id_length = FIDO2_CREDENTIAL_ID_MAX_SIZE;
	info.transports = 0;

	info.firmware_version = 0x00010000;

	if (IS_ENABLED(CONFIG_FIDO2_TRANSPORT_USB_HID)) {
		info.transports |= FIDO2_TRANSPORT_USB;
	}

	info.options.rk = !IS_ENABLED(CONFIG_FIDO2_STORAGE_NONE);
	info.options.up = true;
	info.options.plat = false;
	/* These will depend on config */
	info.options.uv = false;
	/* Allow non-UV non-discoverable credential creation */
	info.options.make_cred_uv_not_rqd = !IS_ENABLED(CONFIG_FIDO2_ALWAYS_UV);
	/* Force UV even when RP sets userVerification=discouraged */
	info.options.always_uv = IS_ENABLED(CONFIG_FIDO2_ALWAYS_UV);
	info.options.no_mc_ga_permissions_with_client_pin = false;

#if defined(CONFIG_FIDO2_CREDENTIAL_MANAGEMENT)
	info.options.cred_mgmt = true;
#endif
#if defined(CONFIG_FIDO2_EXT_CRED_PROTECT)
	info.extensions[info.num_extensions++] = "credProtect";
#endif
#if defined(CONFIG_FIDO2_EXT_HMAC_SECRET)
	info.extensions[info.num_extensions++] = "hmac-secret";
#endif
#if defined(CONFIG_FIDO2_EXT_LARGE_BLOB_KEY)
	info.extensions[info.num_extensions++] = "largeBlobKey";
#endif
#if defined(CONFIG_FIDO2_EXT_CRED_BLOB)
	info.extensions[info.num_extensions++] = "credBlob";
#endif
#if defined(CONFIG_FIDO2_EXT_THIRD_PARTY_PAYMENT)
	info.extensions[info.num_extensions++] = "thirdPartyPayment";
#endif

	/* Convert HEX AAGUID to binary and load it in info */
	size_t written = hex2bin(CONFIG_FIDO2_AAGUID, strlen(CONFIG_FIDO2_AAGUID), info.aaguid,
				 FIDO2_AAGUID_SIZE);
	if (written != FIDO2_AAGUID_SIZE) {
		LOG_ERR("Invalid FIDO2_AAGUID");
		return FIDO2_ERR_OTHER;
	}

	ret = fido2_cbor_encode_get_info(&info, cbor_out, cbor_out_cap, cbor_out_len);
	if (ret) {
		return FIDO2_ERR_OTHER;
	}

	return FIDO2_OK;
}

static enum fido2_status process_command(uint8_t cmd, uint8_t *cbor_in, size_t cbor_in_len,
					 uint8_t *cbor_out, size_t cbor_out_cap,
					 size_t *cbor_out_len,
					 const struct fido2_transport *transport, uint32_t cid)
{
	switch (cmd) {
	case FIDO2_CMD_GET_INFO:
		return handle_get_info(cbor_out, cbor_out_cap, cbor_out_len);
	default:
		*cbor_out_len = 0;
		return FIDO2_ERR_INVALID_COMMAND;
	}
}

static void transport_recv_cb(const struct fido2_transport *transport, uint32_t cid,
			      const uint8_t *buf, size_t len, void *user_data)
{
	int ret;

	ARG_UNUSED(user_data);

	if (len > sizeof(rx_enqueue_msg.data)) {
		LOG_WRN("Message too large, dropping");
		return;
	}

	k_mutex_lock(&rx_enqueue_mutex, K_FOREVER);

	rx_enqueue_msg.transport = transport;
	rx_enqueue_msg.cid = cid;
	rx_enqueue_msg.len = len;
	memcpy(rx_enqueue_msg.data, buf, len);

	ret = k_msgq_put(&fido2_msgq, &rx_enqueue_msg, K_NO_WAIT);

	k_mutex_unlock(&rx_enqueue_mutex);

	if (ret) {
		LOG_WRN("Message queue full, dropping cid=0x%08x", cid);
	}
}

static void fido2_thread_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (atomic_get(&fido2_running)) {
		if (k_msgq_get(&fido2_msgq, &rx_dequeue_msg, K_MSEC(100)) != 0) {
			continue;
		}

		if (rx_dequeue_msg.len < 1) {
			continue;
		}

		uint8_t cmd = rx_dequeue_msg.data[0];
		size_t cbor_out_len = 0;
		enum fido2_status status;
		int ret;

		status =
			process_command(cmd, rx_dequeue_msg.data + 1, rx_dequeue_msg.len - 1,
					ctap_tx_frame + 1, sizeof(ctap_tx_frame) - 1, &cbor_out_len,
					rx_dequeue_msg.transport, rx_dequeue_msg.cid);
		ctap_tx_frame[0] = (uint8_t)status;

		if (rx_dequeue_msg.transport && rx_dequeue_msg.transport->api) {
			ret = rx_dequeue_msg.transport->api->send(rx_dequeue_msg.cid, ctap_tx_frame,
								  cbor_out_len + 1);
		} else {
			LOG_ERR("No send path for cid=0x%08x", rx_dequeue_msg.cid);
			ret = -ENODEV;
		}

		if (ret) {
			LOG_WRN("Response send failed: %d", ret);
		} else {
			LOG_INF("CTAP cmd=0x%02x status=0x%02x wire_len=%zu cid=0x%08x", cmd,
				ctap_tx_frame[0], cbor_out_len + 1, rx_dequeue_msg.cid);
		}
	}
}

int fido2_init(void)
{
	int ret;

	reset_timeout = sys_timepoint_calc(K_SECONDS(10)); /* Reset allowed for 10s post-boot */

	ret = fido2_transport_init_all(transport_recv_cb, NULL);
	if (ret) {
		LOG_ERR("Transport init failed");
		return ret;
	}

	LOG_INF("FIDO2 subsystem initialized");

	return 0;
}

int fido2_start(void)
{
	if (atomic_get(&fido2_running)) {
		LOG_ERR("FIDO2 authenticator already started");
		return -EALREADY;
	}

	atomic_set(&fido2_running, 1);

	k_thread_create(&fido2_thread, fido2_stack, K_THREAD_STACK_SIZEOF(fido2_stack),
			fido2_thread_fn, NULL, NULL, NULL, CONFIG_FIDO2_THREAD_PRIORITY, 0,
			K_NO_WAIT);
	k_thread_name_set(&fido2_thread, "fido2");

	LOG_INF("FIDO2 authenticator started");
	return 0;
}

int fido2_stop(void)
{
	int ret;

	if (!atomic_get(&fido2_running)) {
		LOG_ERR("FIDO2 authenticator already stopped");
		return -EALREADY;
	}

	atomic_set(&fido2_running, 0);

	ret = k_thread_join(&fido2_thread, K_SECONDS(5));
	if (ret) {
		LOG_ERR("FIDO2 thread stop failed: %d", ret);
		return ret;
	}

	fido2_transport_shutdown_all();

	LOG_INF("FIDO2 authenticator stopped");
	return 0;
}

int fido2_reset(void)
{
	if (sys_timepoint_expired(reset_timeout)) {
		return -EACCES;
	}

	/* TODO: Reset */
	return 0;
}

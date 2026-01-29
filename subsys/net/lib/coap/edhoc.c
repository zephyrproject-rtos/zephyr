/*
 * Copyright (c) 2026 Siemens AG
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/net/coap.h>
#include <zephyr/net/edhoc-oscore.h>
#include <edhoc.h>

#include <edhoc_test_vectors_p256_v16.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_coap, CONFIG_COAP_LOG_LEVEL);

#if defined(CONFIG_NET_TC_THREAD_COOPERATIVE)
/* Lowest priority cooperative thread */
#define THREAD_PRIORITY K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1)
#else
#define THREAD_PRIORITY K_PRIO_PREEMPT(CONFIG_NUM_PREEMPT_PRIORITIES - 1)
#endif

/* We could use send and receive queues to exchange data with the CoAP server,
 * but there should only be ever one packet in flight, so a single variable
 * should be enough.
 */
uint8_t edhoc_tx_buf[CONFIG_COAP_SERVER_MESSAGE_SIZE];
uint16_t edhoc_tx_buf_len;
uint8_t edhoc_rx_buf[CONFIG_COAP_SERVER_MESSAGE_SIZE];
uint16_t edhoc_rx_buf_len;
static K_MUTEX_DEFINE(edhoc_tx_lock);
static K_MUTEX_DEFINE(edhoc_rx_lock);

/* Only one global EDHOC instance is supported */
static struct edhoc_oscore_ctx *edhoc_oscore_ctx;

static void edhoc_init_creds(struct other_party_cred *cred_i, struct edhoc_responder_context *c_r)
{
    const uint8_t vec_num_i = 0;

    /* Our state (EDHOC responder/server)*/
    /* connection id */
	c_r->c_r.ptr = (uint8_t *)test_vectors[vec_num_i].c_r;
	c_r->c_r.len = test_vectors[vec_num_i].c_r_len;

	c_r->suites_r.len = test_vectors[vec_num_i].SUITES_R_len;
	c_r->suites_r.ptr = (uint8_t *)test_vectors[vec_num_i].SUITES_R;
	c_r->ead_2.len = test_vectors[vec_num_i].ead_2_len;
	c_r->ead_2.ptr = (uint8_t *)test_vectors[vec_num_i].ead_2;
	c_r->ead_4.len = test_vectors[vec_num_i].ead_4_len;
	c_r->ead_4.ptr = (uint8_t *)test_vectors[vec_num_i].ead_4;

    /* encoded credentials (e.g., encoded x5chain) */
	c_r->id_cred_r.len = test_vectors[vec_num_i].id_cred_r_len;
	c_r->id_cred_r.ptr = (uint8_t *)test_vectors[vec_num_i].id_cred_r;

    /* credentials (e.g., CBOR encoded x.509 cert) */
	c_r->cred_r.len = test_vectors[vec_num_i].cred_r_len;
	c_r->cred_r.ptr = (uint8_t *)test_vectors[vec_num_i].cred_r;

    /* ephemeral keys */
	c_r->g_y.len = test_vectors[vec_num_i].g_y_raw_len;
	c_r->g_y.ptr = (uint8_t *)test_vectors[vec_num_i].g_y_raw;
	c_r->y.len = test_vectors[vec_num_i].y_raw_len;
	c_r->y.ptr = (uint8_t *)test_vectors[vec_num_i].y_raw;

    /* server private keys */
	c_r->g_r.len = test_vectors[vec_num_i].g_r_raw_len;
	c_r->g_r.ptr = (uint8_t *)test_vectors[vec_num_i].g_r_raw;
    /* server public keys */
	c_r->r.len = test_vectors[vec_num_i].r_raw_len;
	c_r->r.ptr = (uint8_t *)test_vectors[vec_num_i].r_raw;

    /* certificate secret + public key */
	c_r->sk_r.len = test_vectors[vec_num_i].sk_r_raw_len;
	c_r->sk_r.ptr = (uint8_t *)test_vectors[vec_num_i].sk_r_raw;
	c_r->pk_r.len = test_vectors[vec_num_i].pk_r_raw_len;
	c_r->pk_r.ptr = (uint8_t *)test_vectors[vec_num_i].pk_r_raw;

    /* Other party (EDHOC initiator/client) */
	cred_i->id_cred.len = test_vectors[vec_num_i].id_cred_i_len;
	cred_i->id_cred.ptr = (uint8_t *)test_vectors[vec_num_i].id_cred_i;
	cred_i->cred.len = test_vectors[vec_num_i].cred_i_len;
	cred_i->cred.ptr = (uint8_t *)test_vectors[vec_num_i].cred_i;
	cred_i->g.len = test_vectors[vec_num_i].g_i_raw_len;
	cred_i->g.ptr = (uint8_t *)test_vectors[vec_num_i].g_i_raw;
	cred_i->pk.len = test_vectors[vec_num_i].pk_i_raw_len;
	cred_i->pk.ptr = (uint8_t *)test_vectors[vec_num_i].pk_i_raw;
	cred_i->ca.len = test_vectors[vec_num_i].ca_i_len;
	cred_i->ca.ptr = (uint8_t *)test_vectors[vec_num_i].ca_i;
	cred_i->ca_pk.len = test_vectors[vec_num_i].ca_i_pk_len;
	cred_i->ca_pk.ptr = (uint8_t *)test_vectors[vec_num_i].ca_i_pk;
}

enum err ead_process(void *params, struct byte_array *ead13)
{
	/* For this sample we don't use EAD */
	return ok;
}

enum err rx(void *sock, struct byte_array *data)
{
	int ret = ok;
	while(1) {
		if (edhoc_rx_buf_len > 0) {
			ret = k_mutex_lock(&edhoc_rx_lock, K_FOREVER);
			if (ret != 0) {
				continue;
			}
			if (data->len < edhoc_rx_buf_len) {
				k_mutex_unlock(&edhoc_rx_lock);
				LOG_ERR("EDHOC RX buffer too small");
				continue;
			}
			LOG_DBG("EDHOC received %u bytes", edhoc_rx_buf_len);
			memcpy(data->ptr, edhoc_rx_buf, edhoc_rx_buf_len);
			data->len = edhoc_rx_buf_len;
			edhoc_rx_buf_len = 0;
			k_mutex_unlock(&edhoc_rx_lock);
			return ok;
		}
		k_sleep(K_MSEC(100));
	}
	return ret;
}

enum err tx(void *sock, struct byte_array *data)
{
	int ret = ok;

	ret = k_mutex_lock(&edhoc_tx_lock, K_FOREVER);
	if (ret != 0) {
		return ret;
	}
	if (data->len > sizeof(edhoc_tx_buf)) {
		ret = buffer_to_small;
		goto unlock;
	}
	LOG_DBG("EDHOC sending %u bytes", data->len);
	memcpy(edhoc_tx_buf, data->ptr, data->len);
	edhoc_tx_buf_len = data->len;
unlock:
	k_mutex_unlock(&edhoc_tx_lock);
	return ret;
}

int edhoc_server_thread(void)
{
	struct other_party_cred cred_i;
	struct edhoc_responder_context c_r;
	struct cred_array cred_i_array = { .len = 1, .ptr = &cred_i };
	BYTE_ARRAY_NEW(err_msg, 0, 0);
	BYTE_ARRAY_NEW(PRK_out, 32, 32);
	BYTE_ARRAY_NEW(prk_exporter, 32, 32);
	BYTE_ARRAY_NEW(oscore_master_secret, EDHOC_OSCORE_MASTER_SECRET_MAX_LEN, EDHOC_OSCORE_MASTER_SECRET_MAX_LEN);
	BYTE_ARRAY_NEW(oscore_master_salt, EDHOC_OSCORE_MASTER_SALT_MAX_LEN, EDHOC_OSCORE_MASTER_SALT_MAX_LEN);

	edhoc_init_creds(&cred_i, &c_r);
	c_r.sock = NULL;
	LOG_INF("EDHOC server thread started");

	while (1) {
		edhoc_responder_run(&c_r, &cred_i_array, &err_msg, &PRK_out, tx,
					rx, ead_process);

    	prk_out2exporter(SHA_256, &PRK_out, &prk_exporter);

		edhoc_exporter(SHA_256, OSCORE_MASTER_SECRET, &prk_exporter,
				&oscore_master_secret);
		LOG_HEXDUMP_DBG(oscore_master_secret.ptr,
			oscore_master_secret.len, "OSCORE Master Secret");

		edhoc_exporter(SHA_256, OSCORE_MASTER_SALT, &prk_exporter,
				&oscore_master_salt);
		LOG_HEXDUMP_DBG(oscore_master_salt.ptr,
			oscore_master_salt.len, "OSCORE Master Salt");

		struct oscore_init_params params = {
		.master_secret = {.ptr = oscore_master_secret.ptr,
			.len = oscore_master_secret.len},
		.master_salt = {.ptr = oscore_master_salt.ptr,
				.len = oscore_master_salt.len},
		.sender_id = {.ptr = test_vectors[0].c_i,
				.len = test_vectors[0].c_i_len},
		.recipient_id = {.ptr = test_vectors[0].c_r,
				.len = test_vectors[0].c_r_len},
		.id_context = {.ptr = NULL, .len = 0},
		.aead_alg = OSCORE_AES_CCM_16_64_128, /* only supported algo */
		.hkdf = OSCORE_SHA_256, /* only supported algo */
		.fresh_master_secret_salt = true
		};
		int ret = oscore_context_init(&params, &(edhoc_oscore_ctx->oscore_ctx));
		if (ret != 0){
			LOG_ERR("Failed to init OSCORE context from EDHOC (%d)", ret);
		} else {
			LOG_INF("OSCORE credentials updated from EDHOC");
		}
    }
    return 0;
}

int edhoc_rx_enqueue(struct edhoc_oscore_ctx *ctx, const uint8_t *data, const uint16_t data_len)
{
	int ret = 0;

	if (!ctx || !data) {
		return -EINVAL;
	}
	if (data_len == 0){
		return -EINVAL;
	}
	ret = k_mutex_lock(&edhoc_rx_lock, K_FOREVER);
	if (ret != 0) {
		return ret;
	}
	if (ctx != edhoc_oscore_ctx) {
		LOG_ERR("Invalid EDHOC context");
		ret = -EINVAL;
		goto unlock;
	}
	if (data_len > sizeof(edhoc_rx_buf)){
		ret = -EINVAL;
		goto unlock;
	}
	if (edhoc_rx_buf_len != 0){
		ret = -EBUSY;
		goto unlock;
	}
	memcpy(edhoc_rx_buf, data, data_len);
	edhoc_rx_buf_len = data_len;
unlock:
	k_mutex_unlock(&edhoc_rx_lock);
	return ret;
}

int edhoc_tx_dequeue(struct edhoc_oscore_ctx *ctx, uint8_t *data, uint16_t *data_len)
{
	int ret = 0;

	if (!ctx || !data || !data_len) {
		return -EINVAL;
	}

	ret = k_mutex_lock(&edhoc_tx_lock, K_FOREVER);
	if (ret != 0) {
		return ret;
	}
	if (ctx != edhoc_oscore_ctx) {
		LOG_ERR("Invalid EDHOC context");
		ret = -EINVAL;
		goto unlock;
	}
	if (edhoc_tx_buf_len == 0){
		ret = -ENODATA;
		goto unlock;
	}
	if (*data_len < edhoc_tx_buf_len){
		ret = -EINVAL;
		goto unlock;
	}
	memcpy(data, edhoc_tx_buf, edhoc_tx_buf_len);
	*data_len = edhoc_tx_buf_len;
	edhoc_tx_buf_len = 0;
unlock:
	k_mutex_unlock(&edhoc_tx_lock);
	return ret;
}

int edhoc_register_ctx(struct edhoc_oscore_ctx *ctx)
{
	int ret = 0;

	if (ctx == NULL){
		return -EINVAL;
	}

	ret = k_mutex_lock(&edhoc_tx_lock, K_FOREVER);
	if (ret != 0) {
		return ret;
	}
	edhoc_oscore_ctx = ctx;
	edhoc_tx_buf_len = 0;
	edhoc_rx_buf_len = 0;
	k_mutex_unlock(&edhoc_tx_lock);
	return ret;
}


K_THREAD_DEFINE(edhoc_server_id, CONFIG_EDHOC_SERVER_STACK_SIZE,
		edhoc_server_thread, NULL, NULL, NULL,
		THREAD_PRIORITY, 0, 0);

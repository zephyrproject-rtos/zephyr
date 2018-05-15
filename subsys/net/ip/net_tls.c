/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/param.h>

#include "net_private.h"
#include "net_tls_internal.h"
#include "tcp_internal.h"

#define TIMEOUT_TLS_RX_MS       100
#define TIMEOUT_TLS_TX_MS       100

#if defined(CONFIG_NET_PRECONFIGURE_TLS_CREDENTIALS)
#include CONFIG_NET_TLS_CREDENTIALS_FILE
#endif

static mbedtls_ctr_drbg_context tls_ctr_drbg;

struct net_tls_credential {
	enum net_tls_credential_type type;
	sec_tag_t tag;
	const void *buf;
	size_t len;
};

/* Global poll of credentials shared among TLS contexts. */
static struct net_tls_credential credentials[CONFIG_NET_MAX_CREDENTIALS_NUMBER];

static struct net_tls_credential *unused_credential_get(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(credentials); i++) {
		if (credentials[i].type == NET_TLS_CREDENTIAL_UNUSED) {
			return &credentials[i];
		}
	}

	return NULL;
}

static struct net_tls_credential *credential_get(
		sec_tag_t tag, enum net_tls_credential_type type)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(credentials); i++) {
		if (credentials[i].type == type && credentials[i].tag == tag) {
			return &credentials[i];
		}
	}

	return NULL;
}

static struct net_tls_credential *credential_next_get(
		sec_tag_t tag, struct net_tls_credential *iter)
{
	int i;

	if (!iter) {
		iter = credentials;
	} else {
		iter++;
	}

	for (i = iter - credentials; i < ARRAY_SIZE(credentials); i++) {
		if (credentials[i].type != NET_TLS_CREDENTIAL_UNUSED &&
		    credentials[i].tag == tag) {
			return &credentials[i];
		}
	}

	return NULL;
}

static int tls_tx(void *ctx, const unsigned char *buf, size_t len)
{
	struct net_context *context = ctx;
	struct net_pkt *pkt;
	int bytes, ret;

	pkt = net_pkt_get_tx(context, TIMEOUT_TLS_TX_MS);

	if (!pkt) {
		return -EIO;
	}

	bytes = net_pkt_append(pkt, len, (u8_t *)buf, TIMEOUT_TLS_TX_MS);

	ret = net_context_output(context, pkt, &context->remote);
	if (ret < 0) {
		net_pkt_unref(pkt);
		return ret;
	}

	return bytes;
}

static int tls_rx(void *ctx, unsigned char *buf, size_t len)
{
	struct net_context *context = ctx;
	int ret;
	u16_t len_left;

	if (!context->mbedtls.rx_pkt) {
		context->mbedtls.rx_pkt =
			k_fifo_get(&context->mbedtls.rx_fifo, K_NO_WAIT);
		if (context->mbedtls.rx_pkt) {
			context->mbedtls.rx_offset =
				net_pkt_appdata(context->mbedtls.rx_pkt) -
				context->mbedtls.rx_pkt->frags->data;
		} else {
			return MBEDTLS_ERR_SSL_WANT_READ;
		}
	}

	len_left = net_pkt_get_len(context->mbedtls.rx_pkt) -
		   context->mbedtls.rx_offset;
	ret = net_frag_linearize(buf, len, context->mbedtls.rx_pkt,
				 context->mbedtls.rx_offset,
				 MIN(len, len_left));
	if (ret > 0) {
		context->mbedtls.rx_offset += ret;
	}

	if (context->mbedtls.rx_offset >=
	    net_pkt_get_len(context->mbedtls.rx_pkt)) {
		net_pkt_unref(context->mbedtls.rx_pkt);
		context->mbedtls.rx_pkt = NULL;
		context->mbedtls.rx_offset = 0;
	}

	return ret;
}

static int tls_mbedtls_ctr_drbg_random(void *p_rng, unsigned char *output,
				       size_t output_len)
{
	static K_MUTEX_DEFINE(mutex);

	ARG_UNUSED(mutex);

	return 0;
}

static void tls_recv_cb(struct net_context *context, struct net_pkt *pkt,
			int status, void *user_data)
{
	struct net_pkt *decrypted_pkt;
	int len, ret = 0;

	if (!pkt) {
		/* Forward EOF */
		if (context->tls_cb) {
			context->tls_cb(context, pkt, status, user_data);
		}
		return;
	}

	k_fifo_put(&context->mbedtls.rx_fifo, pkt);

	/* Process it as application data only after handshake is over,
	 * otherwise packet will be consumed by handshake.
	 */
	if (context->mbedtls.ssl.state == MBEDTLS_SSL_HANDSHAKE_OVER) {
		len = net_pkt_appdatalen(pkt);

		decrypted_pkt = net_pkt_clone(pkt, 0, NET_PKT_CLONE_HDR);
		if (!decrypted_pkt) {
			return;
		}

		/* Copy IP and transport header */
		u16_t header_len = net_pkt_appdata(pkt) - pkt->frags->data;

		ret = net_frag_linear_copy(decrypted_pkt->frags, pkt->frags,
					   0, header_len);
		if (ret < 0) {
			goto err;
		}

		/* Set appdata and appdatalen */
		net_pkt_set_appdata(decrypted_pkt,
				    decrypted_pkt->frags->data + header_len);
		net_pkt_set_appdatalen(decrypted_pkt, 0);

		while (1) {
			ret = mbedtls_ssl_read(&context->mbedtls.ssl,
					       context->mbedtls.rx_ssl_buf,
					       sizeof(context->mbedtls.rx_ssl_buf));
			if (ret == 0 || ret == MBEDTLS_ERR_SSL_WANT_READ ||
				ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
				ret = 0;

				break;
			}

			if (ret > 0) {
				ret = net_pkt_append(decrypted_pkt, ret,
						     context->mbedtls.rx_ssl_buf,
						     TIMEOUT_TLS_RX_MS);
				/* Update appdatalen */
				net_pkt_set_appdatalen(decrypted_pkt,
						       net_pkt_appdatalen(decrypted_pkt) + ret);
				continue;
			}

			ret = -EBADF;
			break;
		}

		if (ret >= 0 && context->tls_cb) {
			context->tls_cb(context, decrypted_pkt, status, user_data);
		} else {
			goto err;
		}
	}

	return;

err:
	net_pkt_unref(decrypted_pkt);
}

static int tls_add_ca_certificate(struct net_context *context,
				  struct net_tls_credential *ca_cert)
{
#if defined(MBEDTLS_X509_CRT_PARSE_C)
	int err = mbedtls_x509_crt_parse(&context->mbedtls.ca_chain,
					 ca_cert->buf, ca_cert->len);
	if (err != 0) {
		return -EINVAL;
	}
#endif /* MBEDTLS_X509_CRT_PARSE_C */

	return 0;
}

static void tls_set_ca_chain(struct net_context *context)
{
#if defined(MBEDTLS_X509_CRT_PARSE_C)
	mbedtls_ssl_conf_ca_chain(&context->mbedtls.config,
				  &context->mbedtls.ca_chain, NULL);

	mbedtls_ssl_conf_authmode(&context->mbedtls.config,
				  MBEDTLS_SSL_VERIFY_REQUIRED);

	mbedtls_ssl_conf_cert_profile(&context->mbedtls.config,
				      &mbedtls_x509_crt_profile_default);
#endif /* MBEDTLS_X509_CRT_PARSE_C */
}

static int tls_set_psk(struct net_context *context,
		       struct net_tls_credential *psk,
		       struct net_tls_credential *psk_id)
{
#if defined(MBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED)
	mbedtls_ssl_conf_psk(&context->mbedtls.config,
			     psk->buf, psk->len,
			     (const unsigned char *)psk_id->buf,
			     psk_id->len - 1);
#endif

	return 0;
}

static int tls_mbedtls_set_credentials(struct net_context *context)
{
	struct net_tls_credential *credential;
	sec_tag_t tag;
	int i, err;
	bool ca_cert_present = false;

	for (i = 0; i < context->options.sec_tag_list.sec_tag_count; i++) {
		tag = context->options.sec_tag_list.sec_tags[i];
		credential = NULL;

		while ((credential = credential_next_get(tag, credential))
				!= NULL) {
			switch (credential->type) {
			case NET_TLS_CREDENTIAL_CA_CERTIFICATE:
			{
				err = tls_add_ca_certificate(context,
							     credential);
				if (err < 0) {
					return err;
				}

				ca_cert_present = true;

				break;
			}

			case NET_TLS_CREDENTIAL_PSK:
			{
				struct net_tls_credential *psk_id =
					credential_get(
						tag, NET_TLS_CREDENTIAL_PSK_ID);

				if (!psk_id) {
					return -ENOENT;
				}

				err = tls_set_psk(context, credential, psk_id);
				if (err < 0) {
					return err;
				}

				break;
			}

			case NET_TLS_CREDENTIAL_PSK_ID:
				/* Ignore PSK ID - it will be used together with PSK */
				break;

			default:
				return -EINVAL;
			}
		}
	}

	if (ca_cert_present) {
		tls_set_ca_chain(context);
	}

	return 0;
}

void net_tls_init(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(credentials); i++) {
		credentials[i].type = NET_TLS_CREDENTIAL_UNUSED;
	}

#if defined(CONFIG_NET_PRECONFIGURE_TLS_CREDENTIALS)
#if defined(MBEDTLS_X509_CRT_PARSE_C)
	net_tls_credential_add(NET_TLS_DEFAULT_CA_CERTIFICATE_TAG,
			       NET_TLS_CREDENTIAL_CA_CERTIFICATE,
			       ca_certificate, sizeof(ca_certificate));
#endif /* MBEDTLS_X509_CRT_PARSE_C */
#if defined(MBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED)
	net_tls_credential_add(NET_TLS_DEFAULT_PSK_TAG,
			       NET_TLS_CREDENTIAL_PSK,
			       client_psk, sizeof(client_psk));
	net_tls_credential_add(NET_TLS_DEFAULT_PSK_TAG,
			       NET_TLS_CREDENTIAL_PSK_ID,
			       client_psk_id, sizeof(client_psk_id));
#endif /* MBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED */
#endif /* CONFIG_NET_PRECONFIGURE_TLS_CREDENTIALS */
}

int net_tls_enable(struct net_context *context, bool enabled)
{
	int state;

	if (!context) {
		return -EINVAL;
	}

	if (context->options.tls == enabled) {
		return 0;
	}

	state = net_context_get_state(context);
	if (state != NET_CONTEXT_IDLE && state != NET_CONTEXT_UNCONNECTED) {
		return -EBUSY;
	}

	if (enabled) {
		k_fifo_init(&context->mbedtls.rx_fifo);
		mbedtls_ssl_init(&context->mbedtls.ssl);
		mbedtls_ssl_config_init(&context->mbedtls.config);
		mbedtls_ssl_set_bio(&context->mbedtls.ssl, context,
				    tls_tx, tls_rx, NULL);
#if defined(MBEDTLS_X509_CRT_PARSE_C)
		mbedtls_x509_crt_init(&context->mbedtls.ca_chain);
#endif
	} else {
#if defined(MBEDTLS_X509_CRT_PARSE_C)
		mbedtls_x509_crt_free(&context->mbedtls.ca_chain);
#endif
		mbedtls_ssl_set_bio(&context->mbedtls.ssl,
				    NULL, NULL, NULL, NULL);
		mbedtls_ssl_config_free(&context->mbedtls.config);
		mbedtls_ssl_free(&context->mbedtls.ssl);
	}

	context->options.tls = enabled;

	return 0;
}

int net_tls_connect(struct net_context *context, bool listening)
{
	enum net_context_state state;
	int err, role, type;

	if (!context) {
		return -EINVAL;
	}

	if (!context->options.tls) {
		return 0;
	}

	state = net_context_get_state(context);
	if (state != NET_CONTEXT_READY) {
		return -EBUSY;
	}

	context->recv_cb = tls_recv_cb;

	role = listening ? MBEDTLS_SSL_IS_SERVER : MBEDTLS_SSL_IS_CLIENT;

	type = (net_context_get_type(context) == SOCK_STREAM) ?
		MBEDTLS_SSL_TRANSPORT_STREAM :
		MBEDTLS_SSL_TRANSPORT_DATAGRAM;

	err = mbedtls_ssl_config_defaults(&context->mbedtls.config, role, type,
					  MBEDTLS_SSL_PRESET_DEFAULT);
	if (err) {
		err = -ENOMEM;
		goto mbed_err_conf;
	}

	mbedtls_ctr_drbg_init(&tls_ctr_drbg);

	mbedtls_ssl_conf_rng(&context->mbedtls.config,
			     tls_mbedtls_ctr_drbg_random, &tls_ctr_drbg);

	err = tls_mbedtls_set_credentials(context);
	if (err) {
		goto mbed_err_conf;
	}

	err = mbedtls_ssl_setup(&context->mbedtls.ssl,
				&context->mbedtls.config);
	if (err) {
		err = -EINVAL;
		goto mbed_err_conf;
	}

	while ((err = mbedtls_ssl_handshake(&context->mbedtls.ssl)) != 0) {
		if (err != MBEDTLS_ERR_SSL_WANT_READ &&
		    err != MBEDTLS_ERR_SSL_WANT_WRITE) {
			err = -ENOPROTOOPT;
			goto mbed_err_handshake;
		}
	}

	return 0;

mbed_err_handshake:
mbed_err_conf:
	mbedtls_ssl_session_reset(&context->mbedtls.ssl);
	mbedtls_ssl_config_free(&context->mbedtls.config);

	return err;
}

int net_tls_send(struct net_pkt *pkt)
{
	struct net_context *context = net_pkt_context(pkt);
	struct net_buf *frag = pkt->frags;

	while (frag) {
		int ret = 0;
		u8_t *data = frag->data;
		u16_t len = frag->len;

		while (ret < len) {
			ret = mbedtls_ssl_write(&context->mbedtls.ssl,
						data, len);
			if (ret == len) {
				break;
			}

			if (ret > 0 && ret < len) {
				data += ret;
				len -= ret;
				continue;
			}

			if (ret != MBEDTLS_ERR_SSL_WANT_WRITE &&
			    ret != MBEDTLS_ERR_SSL_WANT_READ) {
				return -EBADF;
			}
		}

		frag = frag->frags;
	}

	net_pkt_unref(pkt);

	return 0;
}

int net_tls_recv(struct net_context *context, net_context_recv_cb_t cb,
		 void *user_data)
{
	context->tls_cb = cb;

#if defined(CONFIG_NET_TCP)
	context->tcp->recv_user_data = user_data;
#endif

	return 0;
}

int net_tls_sec_tag_list_get(struct net_context *context,
			     sec_tag_t *sec_tags, int *sec_tag_cnt)
{
	int tags_to_copy;

	if (!context || !sec_tags || !sec_tag_cnt) {
		return -EINVAL;
	}

	if (!context->options.tls) {
		return -EPERM;
	}

	tags_to_copy = MIN(context->options.sec_tag_list.sec_tag_count,
			   *sec_tag_cnt);
	memcpy(sec_tags, context->options.sec_tag_list.sec_tags,
	       tags_to_copy * sizeof(sec_tag_t));
	*sec_tag_cnt = tags_to_copy;

	return 0;
}

int net_tls_sec_tag_list_set(struct net_context *context,
			     const sec_tag_t *sec_tags, int sec_tag_cnt)
{
	enum net_context_state state;
	int i;

	if (!context || !sec_tags) {
		return -EINVAL;
	}

	if (!context->options.tls) {
		return -EPERM;
	}

	state = net_context_get_state(context);
	if (state != NET_CONTEXT_IDLE && state != NET_CONTEXT_UNCONNECTED) {
		return -EPERM;
	}

	if (sec_tag_cnt > ARRAY_SIZE(context->options.sec_tag_list.sec_tags)) {
		return -ENOMEM;
	}

	for (i = 0; i < sec_tag_cnt; i++) {
		if (credential_next_get(sec_tags[i], NULL) == NULL) {
			return -ENOENT;
		}
	}

	memcpy(context->options.sec_tag_list.sec_tags, sec_tags,
	       sec_tag_cnt * sizeof(sec_tag_t));
	context->options.sec_tag_list.sec_tag_count = sec_tag_cnt;

	return 0;
}

int net_tls_credential_add(sec_tag_t tag, enum net_tls_credential_type type,
			   const void *cred, size_t credlen)
{
	struct net_tls_credential *credential = credential_get(tag, type);

	if (credential != NULL) {
		return -EEXIST;
	}

	credential = unused_credential_get();
	if (credential == NULL) {
		return -ENOMEM;
	}

	credential->tag = tag;
	credential->type = type;
	credential->buf = cred;
	credential->len = credlen;

	return 0;
}

int net_tls_credential_get(sec_tag_t tag, enum net_tls_credential_type type,
			   void *cred, size_t *credlen)
{
	struct net_tls_credential *credential = credential_get(tag, type);

	if (credential == NULL) {
		return -ENOENT;
	}

	*credlen = credential->len;
	memcpy(cred, credential->buf, credential->len);

	return 0;
}

int net_tls_credential_delete(sec_tag_t tag, enum net_tls_credential_type type)
{
	struct net_tls_credential *credential = NULL;

	credential = credential_get(tag, type);

	if (!credential) {
		return -ENOENT;
	}

	memset(credential, 0, sizeof(struct net_tls_credential));
	credential->type = NET_TLS_CREDENTIAL_UNUSED;

	return 0;
}

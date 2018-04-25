/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "net_tls.h"
#include "net_private.h"

#define TIMEOUT_TLS_RX_MS       100
#define TIMEOUT_TLS_TX_MS       100

static mbedtls_ctr_drbg_context tls_ctr_drbg;

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

	ARG_UNUSED(context);

	return -EOPNOTSUPP;
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
	} else {
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

	return 0;
}

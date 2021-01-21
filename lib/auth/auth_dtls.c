/**
 *  Copyright (c) 2021 Golden Bits Software, Inc.
 *
 *  @file  BLE Authentication using DTLS
 *
 *  @brief  DTLS authentication code using Mbed DTLS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <sys/byteorder.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr.h>
#include <init.h>
#include <random/rand32.h>


#if defined(CONFIG_MBEDTLS)
#if !defined(CONFIG_MBEDTLS_CFG_FILE)
#include "mbedtls/config.h"
#else
#include CONFIG_MBEDTLS_CFG_FILE
#endif
#endif /* CONFIG_MBEDTLS_CFG_FILE */


#include <mbedtls/ctr_drbg.h>
#include <mbedtls/net_sockets.h>
#include <mbedtls/x509.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/ssl.h>
#include <mbedtls/ssl_cookie.h>
#include <mbedtls/error.h>
#include <mbedtls/debug.h>
#include <mbedtls/entropy.h>
#include <mbedtls/timing.h>

#define LOG_LEVEL CONFIG_AUTH_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_DECLARE(auth_lib, CONFIG_AUTH_LOG_LEVEL);

#include <auth/auth_lib.h>
#include "auth_internal.h"

#define MBED_ERROR_BUFLEN           (150u)
#define MAX_MBEDTLS_CONTEXT         (CONFIG_NUM_AUTH_INSTANCES)
#define AUTH_DTLS_COOKIE_LEN        (32u)

#define DTLS_PACKET_SYNC_BYTES      (0x45B8)
#define DTLS_HEADER_BYTES           (sizeof(struct dtls_packet_hdr))

#define AUTH_DTLS_MIN_TIMEOUT       (10000u)
#define ATUH_DTLS_MAX_TIMEOUT       (30000u)
#define AUTH_DTLS_HELLO_WAIT_MSEC   (15000u)


/**
 * Header identifying a DTLS packet (aka datagram).  Unlike TLS, DTLS packets
 * must be forwarded to Mbedtls as one or more complete packets.  TLS is
 * design to handle an incoming byte stream.
 */
#pragma pack(push, 1)
struct dtls_packet_hdr {
	uint16_t sync_bytes;    /* use magic number to identify header */
	uint16_t packet_len;    /* size of DTLS datagram */
};
#pragma pack(pop)



/**
 * Mbed context, one context per DTLS connection.
 */
struct mbed_tls_context {
	bool in_use;

	mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context ctr_drbg;
	mbedtls_ssl_context ssl;
	mbedtls_ssl_config conf;
	mbedtls_x509_crt cacert;
	mbedtls_x509_crt device_cert;
	mbedtls_pk_context device_private_key;
	mbedtls_timing_delay_context timer;
	mbedtls_ssl_cookie_ctx cookie_ctx;

	/* Temp buffer used to assemble full frame when sending. */
	uint8_t temp_dtlsbuf[CONFIG_MBEDTLS_SSL_MAX_CONTENT_LEN];

	/* cookie used for DTLS */
	uint8_t cookie[AUTH_DTLS_COOKIE_LEN];
};

/**
 * List of Mbed instances
 */
static struct mbed_tls_context tlscontext[MAX_MBEDTLS_CONTEXT];


/* ===================== local functions =========================== */


/**
 * Get a free Mbed context to use with an authentication instance.
 *
 * @return Pointer to context, NULL if no free context available.
 */
static struct mbed_tls_context *auth_get_mbedcontext(void)
{
	// use mutex lock to protect accessing list
	for (int cnt = 0; cnt < MAX_MBEDTLS_CONTEXT; cnt++) {

		if (!tlscontext[cnt].in_use) {
			tlscontext[cnt].in_use = true;
			return &tlscontext[cnt];
		}
	}

	return NULL;
}

/**
 * Free a Mbed context.
 *
 * @param mbed_ctx Pointer to context.
 */
static void auth_free_mbedcontext(struct mbed_tls_context *mbed_ctx)
{
	mbed_ctx->in_use = false;

	/* Free any MBed tls resources */
	mbedtls_x509_crt_free(&mbed_ctx->cacert);
	mbedtls_x509_crt_free(&mbed_ctx->device_cert);
	mbedtls_pk_free(&mbed_ctx->device_private_key);
	mbedtls_ssl_free(&mbed_ctx->ssl);
	mbedtls_ssl_config_free(&mbed_ctx->conf);
	mbedtls_ctr_drbg_free(&mbed_ctx->ctr_drbg);
	mbedtls_entropy_free(&mbed_ctx->entropy);
}

/**
 * Initialize Mbed context.
 *
 * @param mbed_ctx Pointer to context.
 */
static void auth_init_context(struct mbed_tls_context *mbed_ctx)
{
	mbedtls_ssl_init(&mbed_ctx->ssl);
	mbedtls_ssl_config_init(&mbed_ctx->conf);
	mbedtls_x509_crt_init(&mbed_ctx->cacert);
	mbedtls_x509_crt_init(&mbed_ctx->device_cert);
	mbedtls_pk_init(&mbed_ctx->device_private_key);
	mbedtls_ssl_cookie_init(&mbed_ctx->cookie_ctx);
	mbedtls_ctr_drbg_init(&mbed_ctx->ctr_drbg);
	mbedtls_entropy_init(&mbed_ctx->entropy);

	auth_get_random(mbed_ctx->cookie, sizeof(mbed_ctx->cookie));
}

/**
 * Return the handshake state name, helpful for debug purposes.
 *
 * @param state  The state enumeration.
 *
 * @return  Pointer to handshake string name.
 */
static const char *auth_tls_handshake_state(const mbedtls_ssl_states state)
{
	switch (state) {

	case MBEDTLS_SSL_HELLO_REQUEST:
		return "MBEDTLS_SSL_HELLO_REQUEST";
		break;

	case MBEDTLS_SSL_CLIENT_HELLO:
		return "MBEDTLS_SSL_CLIENT_HELLO";
		break;

	case MBEDTLS_SSL_SERVER_HELLO:
		return "MBEDTLS_SSL_SERVER_HELLO";
		break;

	case MBEDTLS_SSL_SERVER_CERTIFICATE:
		return "MBEDTLS_SSL_SERVER_CERTIFICATE";
		break;

	case MBEDTLS_SSL_SERVER_KEY_EXCHANGE:
		return "MBEDTLS_SSL_SERVER_KEY_EXCHANGE";
		break;

	case MBEDTLS_SSL_CERTIFICATE_REQUEST:
		return "MBEDTLS_SSL_CERTIFICATE_REQUEST";
		break;

	case MBEDTLS_SSL_SERVER_HELLO_DONE:
		return "MBEDTLS_SSL_SERVER_HELLO_DONE";
		break;

	case MBEDTLS_SSL_CLIENT_CERTIFICATE:
		return "MBEDTLS_SSL_CLIENT_CERTIFICATE";
		break;

	case MBEDTLS_SSL_CLIENT_KEY_EXCHANGE:
		return "MBEDTLS_SSL_CLIENT_KEY_EXCHANGE";
		break;

	case MBEDTLS_SSL_CERTIFICATE_VERIFY:
		return "MBEDTLS_SSL_CERTIFICATE_VERIFY";
		break;

	case MBEDTLS_SSL_CLIENT_CHANGE_CIPHER_SPEC:
		return "MBEDTLS_SSL_CLIENT_CHANGE_CIPHER_SPEC";
		break;

	case MBEDTLS_SSL_CLIENT_FINISHED:
		return "MBEDTLS_SSL_CLIENT_FINISHED";
		break;

	case MBEDTLS_SSL_SERVER_CHANGE_CIPHER_SPEC:
		return "MBEDTLS_SSL_SERVER_CHANGE_CIPHER_SPEC";
		break;

	case MBEDTLS_SSL_SERVER_FINISHED:
		return "MBEDTLS_SSL_SERVER_FINISHED";
		break;

	case MBEDTLS_SSL_FLUSH_BUFFERS:
		return "MBEDTLS_SSL_FLUSH_BUFFERS";
		break;

	case MBEDTLS_SSL_HANDSHAKE_WRAPUP:
		return "MBEDTLS_SSL_HANDSHAKE_WRAPUP";
		break;

	case MBEDTLS_SSL_HANDSHAKE_OVER:
		return "MBEDTLS_SSL_HANDSHAKE_OVER";
		break;

	case MBEDTLS_SSL_SERVER_NEW_SESSION_TICKET:
		return "MBEDTLS_SSL_SERVER_NEW_SESSION_TICKET";
		break;

	case MBEDTLS_SSL_SERVER_HELLO_VERIFY_REQUEST_SENT:
		return "MBEDTLS_SSL_SERVER_HELLO_VERIFY_REQUEST_SENT";
		break;

	default:
		break;
	}

	return "unknown";
}


/**
 * Gets the time since boot in Milliseconds, used for DTLS retry timer.  The absolute
 * time (wall clock time) isn't necessary, just the relative time in milliseconds.
 *
 * @param val     Timer value
 * @param reset   If non-zero then set current milliseconds into the time var.
 *
 * @return 0 if resetting the time, else the time difference in milliseconds.
 */
static unsigned long auth_tls_timing_get_timer(struct mbedtls_timing_hr_time *val, int reset)
{
	int64_t delta;
	int64_t *mssec = (int64_t *) val;
	int64_t cur_msg = k_uptime_get();

	if (reset) {
		*mssec = cur_msg;
		return 0;
	}

	delta = cur_msg - *mssec;

	return (unsigned long )delta;
}

/**
 * Set delays to watch, final and intermediate delays.
 *
 * @param data    Timing delay context.
 * @param int_ms  Intermediate delay in milliseconds.
 * @param fin_ms  Final delay in milliseconds.
 *
 */
static void auth_tls_timing_set_delay(void *data, uint32_t int_ms, uint32_t fin_ms)
{
	mbedtls_timing_delay_context *ctx = (mbedtls_timing_delay_context *) data;

	ctx->int_ms = int_ms;
	ctx->fin_ms = fin_ms;

	if (fin_ms != 0) {
		(void) auth_tls_timing_get_timer(&ctx->timer, 1);
	}
}

/**
 *  Determine the delay result.
 *
 *  @param data   Pointer to delay context.
 *
 *  @return -1 if cancelled, 0 if none of the delays is expired,
 *           1 if the intermediate delay only is expired,
 *           2 if the final delay is expired
 */
static int auth_tls_timing_get_delay(void *data)
{
	mbedtls_timing_delay_context *ctx = (mbedtls_timing_delay_context *)data;
	int64_t elapsed_ms;

	if (ctx->fin_ms == 0) {
		return -1;
	}

	elapsed_ms = auth_tls_timing_get_timer(&ctx->timer, 0);

	if (elapsed_ms >= (int64_t)ctx->fin_ms) {
		return 2;
	}

	if (elapsed_ms >= (int64_t)ctx->int_ms) {
		return 1;
	}

	return 0;
}


/**
 * Function called by Mbed stack to print debug messages.
 *
 * @param ctx     Context
 * @param level   Debug level
 * @param file    Source filename of debug log entry.
 * @param line    Line number of debug log entry.
 * @param str     Debug/Log message.
 */
static void auth_mbed_debug(void *ctx, int level, const char *file,
			    int line, const char *str)
{
	const char *p, *basename;

	/**
	 * @brief   Need to define const string here vs. const char *fmt = "[%s:%d] %s"
	 *          because the LOG_ERR(), LOG_* macros can't handle a pointer.
	 */
#define LOG_FMT  "[%s:%d] %s"

	(void)ctx;

	if (!file || !str) {
		return;
	}

	/* Extract basename from file */
	for (p = basename = file; *p != '\0'; p++) {
		if (*p == '/' || *p == '\\') {
			basename = p + 1;
		}
	}


	switch (level) {

	case 0:
	{
		LOG_ERR(LOG_FMT, log_strdup(basename), line, log_strdup(str));
		break;
	}

	case 1:
	{
		LOG_WRN(LOG_FMT, log_strdup(basename), line, log_strdup(str));
		break;
	}

	case 2:
	{
		LOG_INF(LOG_FMT,  log_strdup(basename), line, log_strdup(str));
		break;
	}

	case 3:
		__attribute__((fallthrough));
	default:
	{
		LOG_DBG(LOG_FMT, log_strdup(basename), line, log_strdup(str));
		break;
	}
	}
}



/**
 * Mbed routine to send data, called by Mbed TLS library.
 *
 * @param ctx   Context pointer, pointer to struct authenticate_conn
 * @param buf   Buffer to send.
 * @param len   Number of bytes to send.
 *
 * @return  Number of bytes sent, else Mbed tls error.
 */
static int auth_mbedtls_tx(void *ctx, const uint8_t *buf, size_t len)
{
	int send_cnt;
	struct authenticate_conn *auth_conn = (struct authenticate_conn *)ctx;

	if (auth_conn == NULL) {
		return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
	}

	struct dtls_packet_hdr *dtls_hdr;
	struct mbed_tls_context *mbedctx = (struct mbed_tls_context *)auth_conn->internal_obj;

	if (mbedctx == NULL) {
		LOG_ERR("Missing Mbed context.");
		return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
	}

	dtls_hdr = (struct dtls_packet_hdr *)mbedctx->temp_dtlsbuf;

	/**
	 * DTLS is targeted for the UDP datagram protocol, as such the Mbed stack
	 * expects a full DTLS packet (ie datagram) to be receive vs. a partial
	 * packet. When sending, add a header to enable the receiving side
	 * to determine when a full DTLS packet has been recevid.
	 */

	/* Check the temp buffer is large enough */
	if ((sizeof(mbedctx->temp_dtlsbuf) - DTLS_HEADER_BYTES) < len) {
		return MBEDTLS_ERR_SSL_BUFFER_TOO_SMALL;
	}

	/* set byte order to Big Endian when sending over lower transport. */
	dtls_hdr->sync_bytes = sys_cpu_to_be16(DTLS_PACKET_SYNC_BYTES);

	/* does not include header */
	dtls_hdr->packet_len = sys_cpu_to_be16((uint16_t)len);

	/* Combine the header with the payload.  This maximizes the lower
	 * transport throughput vs. sending the DTLS header separately then
	 * sending the body. */
	memcpy(&mbedctx->temp_dtlsbuf[DTLS_HEADER_BYTES], buf, len);

	/* send to peripheral */
	send_cnt = auth_xport_send(auth_conn->xport_hdl, mbedctx->temp_dtlsbuf,
				   len + DTLS_HEADER_BYTES);


	if (send_cnt < 0) {
		LOG_ERR("Failed to send, err: %d", send_cnt);
		return MBEDTLS_ERR_NET_SEND_FAILED;
	}

	LOG_INF("Send %d byes.", send_cnt);

	/* return number bytes sent, do not include the DTLS header */
	send_cnt -= DTLS_HEADER_BYTES;

	return send_cnt;
}

/**
 * Mbed receive function, called by Mbed code.
 *
 * @param ctx     Context.
 * @param buffer  Buffer to copy received bytes into.
 * @param len     Buffer byte size.
 *
 * @return   Number of bytes copied or negative number on error.
 */
static int auth_mbedtls_rx(void *ctx, uint8_t *buffer, size_t len)
{
	struct authenticate_conn *auth_conn = (struct authenticate_conn *)ctx;
	int rx_bytes = 0;
	struct dtls_packet_hdr dtls_hdr;
	uint16_t packet_len = 0;

	/**
	 * DTLS is targeted for the UDP datagram protocol, as such the Mbed stack
	 * expects a full DTLS packet (ie datagram) to be receive vs. a partial
	 * packet.  For the lower transports, a full datagram packet maybe broken
	 * up into multiple fragments.  The receive queue may contain a partial
	 * DTLS frame.  The code here waits until a full DTLS packet is received.
	 */

	/* Will wait until a full DTLS packet is received. */
	while (true) {

		rx_bytes = auth_xport_getnum_recvqueue_bytes_wait(auth_conn->xport_hdl, 1000u);

		/* Check for canceled flag */
		if (auth_conn->cancel_auth) {
			return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
		}

		/* no bytes or timed out */
		if (rx_bytes == 0 || rx_bytes == -EAGAIN) {
			continue;
		}

		/* an error */
		if (rx_bytes < 0) {
			/* an error occurred */
			return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
		}

		if (rx_bytes < DTLS_HEADER_BYTES) {
			continue;
		}

		/* peek into receive queue */
		auth_xport_recv_peek(auth_conn->xport_hdl, (uint8_t *)&dtls_hdr,
				     sizeof(struct dtls_packet_hdr));

		/* check for sync bytes */
		if (sys_be16_to_cpu(dtls_hdr.sync_bytes) != DTLS_PACKET_SYNC_BYTES) {
			// read bytes and try to peek again
			auth_xport_recv(auth_conn->xport_hdl, (uint8_t *)&dtls_hdr,
					sizeof(struct dtls_packet_hdr), 1000u);
			continue;
		}

		// have valid DTLS packet header, check packet length
		dtls_hdr.packet_len = sys_be16_to_cpu(dtls_hdr.packet_len);

		/* Is there enough room to copy into Mbedtls buffer? */
		if (dtls_hdr.packet_len > len) {
			return MBEDTLS_ERR_SSL_BUFFER_TOO_SMALL;
		}

		/* Zero length packet, ignore */
		if (dtls_hdr.packet_len == 0u) {
			LOG_ERR("Empty DTLS packet.");
			/* Read the DTLS header and return */
			auth_xport_recv(auth_conn->xport_hdl, (uint8_t *)&dtls_hdr,
					sizeof(struct dtls_packet_hdr), 1000u);
			return 0;
		}

		/* rx_bytes must be at least DTLS_HEADER_BYTES in length here.  Enough
		 * to fill a complete DTLS packet. */
		if ((int)dtls_hdr.packet_len <= (rx_bytes - (int)DTLS_HEADER_BYTES)) {

			packet_len = dtls_hdr.packet_len;

			/* copy packet into mbed buffers */
			/* read header, do not forward to Mbed */
			auth_xport_recv(auth_conn->xport_hdl, (uint8_t *)&dtls_hdr,
					sizeof(struct dtls_packet_hdr), 1000u);

			/* read packet into mbed buffer*/
			rx_bytes = auth_xport_recv(auth_conn->xport_hdl, buffer,
						   packet_len, 1000u);

			if (rx_bytes <= 0) {
				/* an error occurred */
				return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
			}

			len -= rx_bytes;
			buffer += rx_bytes;

			/* we're done with one DTLS packet, return */
			return rx_bytes;
		}

		/**
		 * If we're here it means we have a partial DTLS packet,
		 * wait for more data until there is enough to fill a
		 * complete DTLS packet.
		 */
		LOG_DBG("Waiting for more bytes to fill DTLS packet.");
	}

	/* Didn't send any bytes */
	return 0;
}


/**
 * Set the DTLS cookie.
 *
 * @param auth_conn   Pointer to auth connection
 *
 * @return 0 on success, else error code.
 */
static int auth_tls_set_cookie(struct authenticate_conn *auth_conn)
{
	int ret;
	struct mbed_tls_context *mbed_ctx = (struct mbed_tls_context *)auth_conn->internal_obj;

	/* should not be NULL!!  */
	if (!mbed_ctx) {
		LOG_ERR("No MBED context.");
		return AUTH_ERROR_INVALID_PARAM;
	}

	ret = mbedtls_ssl_set_client_transport_id(&mbed_ctx->ssl, mbed_ctx->cookie,
						  sizeof(mbed_ctx->cookie));

	return ret;
}

/**
 * Pseudo random source for Mbed
 *
 * @param data    Optional entropy source.
 * @param output  Buffer to receive random numbers.
 * @param len     Number of bytes requested.
 * @param olen    Number of bytes actually returned.
 *
 * @return Always zero.
 */
static int auth_tls_entropy(void *data, unsigned char *output, size_t len,
			    size_t *olen)
{
	(void) data;

	auth_get_random(output, len);
	*olen = len;

	return 0;
}


/**
 * If performing a DTLS handshake
 *
 * @param auth_conn  The auth connection/instance.
 *
 */
static void auth_dtls_thead(struct authenticate_conn *auth_conn)
{
	char err_buf[MBED_ERROR_BUFLEN];
	int bytecount = 0;
	struct mbed_tls_context *mbed_ctx = (struct mbed_tls_context *) auth_conn->internal_obj;

	/* Set status */
	auth_lib_set_status(auth_conn, AUTH_STATUS_STARTED);

	/**
	 * For the server we can noty start the handshake, the code will continue
	 * to read looking for a "Client Hello".  So we'll just stay at the
	 * MBEDTLS_SSL_CLIENT_HELLO state until the central sends the "Client Hello"
	 *
	 * For the client, a client hello will be sent immediately.
	 */
	if (!auth_conn->is_client) {

		/**
		 * For the the DTLS server, use the auth connection handle as the cookie.
		 */
		int ret = auth_tls_set_cookie(auth_conn);

		if (ret) {
			LOG_ERR("Failed to get connection info for DTLS cookie, auth failed, error: 0x%x", ret);
			auth_lib_set_status(auth_conn, AUTH_STATUS_FAILED);
			return;
		}

		/* Sit in a loop waiting for the initial Client Hello message
		 * from the client. */
		while (bytecount == 0) {

			if (auth_conn->cancel_auth) {
				LOG_INF("DTLS authentication canceled.");
				return;
			}

			/* Server, wait for client hello */
			bytecount = auth_xport_getnum_recvqueue_bytes_wait(auth_conn->xport_hdl,
									   AUTH_DTLS_HELLO_WAIT_MSEC);

			if (bytecount == -EAGAIN) {
				/* simply timed out waiting for client hello, try again */
				LOG_INF("Timeout waiting for Client Hello");
				continue;
			}

			if (bytecount < 0) {
				LOG_ERR("Server, error when waiting for client hello, error: %d", bytecount);
				auth_lib_set_status(auth_conn, AUTH_STATUS_FAILED);
				return;
			}
		}

		LOG_INF("Server received initial Client Hello from client.");
	}

	/* Set status */
	auth_lib_set_status(auth_conn, AUTH_STATUS_IN_PROCESS);

	int ret = 0;

	/* start handshake */
	do {

		while (mbed_ctx->ssl.state != MBEDTLS_SSL_HANDSHAKE_OVER &&
		       !auth_conn->cancel_auth) {

			LOG_INF("Handshake state: %s", auth_tls_handshake_state(mbed_ctx->ssl.state));

			/* do handshake step */
			ret = mbedtls_ssl_handshake_step(&mbed_ctx->ssl);

			if (ret != 0) {
				break;
			}
		}


		if (auth_conn->cancel_auth) {
			LOG_INF("Authentication canceled.");
			break;
		}

		if (ret == MBEDTLS_ERR_SSL_HELLO_VERIFY_REQUIRED) {

			/* restart handshake to process client cookie */
			LOG_INF("Restarting handshake, need client cookie.");

			/* reset session and cookie info */
			mbedtls_ssl_session_reset(&mbed_ctx->ssl);

			ret = auth_tls_set_cookie(auth_conn);

			if (ret) {
				LOG_ERR("Failed to reset cookie information, error: 0x%x", ret);
				ret = MBEDTLS_ERR_SSL_INTERNAL_ERROR;
			} else {
				ret = MBEDTLS_ERR_SSL_WANT_READ;
			}

		}

		if (auth_conn->cancel_auth) {
			LOG_INF("Authentication canceled.");
			break;
		}

	} while (ret == MBEDTLS_ERR_SSL_WANT_READ ||
		 ret == MBEDTLS_ERR_SSL_WANT_WRITE);


	if (mbed_ctx->ssl.state == MBEDTLS_SSL_HANDSHAKE_OVER) {
		LOG_INF("DTLS Handshake success.");
		ret = AUTH_SUCCESS;
	} else {
		/* All of the MBed error are of the form -0xXXXX.  To display
		 * correctly negate the error value, thus -ret */
		LOG_ERR("DTLS Handshake failed, error: -0x%x", -ret);
		mbedtls_strerror(ret, err_buf, sizeof(err_buf));
		LOG_ERR("%s", log_strdup(err_buf));
	}


	enum auth_status auth_status;

	switch (ret) {
	case MBEDTLS_ERR_SSL_BAD_HS_CERTIFICATE:
	case MBEDTLS_ERR_SSL_BAD_HS_CERTIFICATE_VERIFY:
		auth_status = AUTH_STATUS_AUTHENTICATION_FAILED;
		break;

	case AUTH_SUCCESS:
		auth_status = AUTH_STATUS_SUCCESSFUL;
		break;

	default:
		auth_status = AUTH_STATUS_FAILED;
		break;
	}

	/* now check if cancel occurred */
	if (auth_conn->cancel_auth) {
		auth_status = AUTH_STATUS_CANCELED;
	}

	/* Call status */
	auth_lib_set_status(auth_conn, auth_status);

	return;
}


/* ================= external/internal funcs ==================== */

/**
 * @see auth_internal.h
 *
 */
int auth_init_dtls_method(struct authenticate_conn *auth_conn, struct auth_optional_param *opt_params)
{
	struct mbed_tls_context *mbed_ctx;
	struct auth_dtls_certs *certs;
	int ret = AUTH_ERROR_DTLS_INIT_FAILED;

	LOG_DBG("Initializing Mbed DTLS");

	/* Check parameters for DTLS */
	if((auth_conn == NULL) || (opt_params == NULL) || (opt_params->param_id != AUTH_DTLS_PARAM)) {
		LOG_ERR("Missing certificates for TLS/DTLS authentication.");
		return AUTH_ERROR_INVALID_PARAM;
	}

	/* get pointer to certs */
	certs = &opt_params->param_body.dtls_certs;

	/* get, init, and set context pointer */
	mbed_ctx = auth_get_mbedcontext();

	if (mbed_ctx == NULL) {
		LOG_ERR("Unable to allocate Mbed TLS context.");
		return AUTH_ERROR_NO_RESOURCE;
	}

	/* Set the DTLS authentication thread */
	auth_conn->auth_func = auth_dtls_thead;

	/* Init mbed context */
	auth_init_context(mbed_ctx);


	/* Save MBED tls context as internal object. The intent of using a void
	 * 'internal_obj' is to provide a var in the struct authentication_conn to
	 * store different authentication methods.  Instead of Mbed, it maybe a
	 * Challenge-Response.*/
	auth_conn->internal_obj = mbed_ctx;

	int endpoint = auth_conn->is_client ? MBEDTLS_SSL_IS_CLIENT : MBEDTLS_SSL_IS_SERVER;

	mbedtls_ssl_config_defaults(&mbed_ctx->conf,
				    endpoint,
				    MBEDTLS_SSL_TRANSPORT_DATAGRAM,
				    MBEDTLS_SSL_PRESET_DEFAULT);


	/* set the lower layer transport functions */
	mbedtls_ssl_set_bio(&mbed_ctx->ssl, auth_conn, auth_mbedtls_tx, auth_mbedtls_rx, NULL);

	/* set max record len to 512, as small as possible */
	mbedtls_ssl_conf_max_frag_len(&mbed_ctx->conf, MBEDTLS_SSL_MAX_FRAG_LEN_512);

	/* Set the DTLS time out */
	mbedtls_ssl_conf_handshake_timeout(&mbed_ctx->conf, AUTH_DTLS_MIN_TIMEOUT,
					   ATUH_DTLS_MAX_TIMEOUT);

	/*  Force verification.  */
	mbedtls_ssl_conf_authmode(&mbed_ctx->conf, MBEDTLS_SSL_VERIFY_REQUIRED);


	if ((certs->device_cert_pem.priv_key == NULL) || (certs->device_cert_pem.priv_key_size == 0)) {
		LOG_ERR("Failed to get device private key");
		goto init_error;
	}


	ret = mbedtls_pk_parse_key(&mbed_ctx->device_private_key, certs->device_cert_pem.priv_key,
				   certs->device_cert_pem.priv_key_size, NULL, 0);

	if (ret) {
		LOG_ERR("Failed to parse device private key, error: 0x%x", ret);
		goto init_error;
	}

	/* Setup certs, the CA chain followed by the end device cert. */
	if ((certs->server_ca_chain_pem.cert == NULL) || (certs->server_ca_chain_pem.cert_size == 0)) {
		LOG_ERR("Failed to get CA cert chain");
		goto init_error;
	}

	/* Parse and set the CA certs */
	ret = mbedtls_x509_crt_parse(&mbed_ctx->cacert, certs->server_ca_chain_pem.cert,
				     certs->server_ca_chain_pem.cert_size);

	if (ret) {
		LOG_ERR("Failed to parse CA cert, error: 0x%x", ret);
		goto init_error;
	}

	/* set CA certs into context */
	mbedtls_ssl_conf_ca_chain(&mbed_ctx->conf, &mbed_ctx->cacert, NULL);

	/* Get and parse the device cert */
	if ((certs->device_cert_pem.cert == NULL) || (certs->device_cert_pem.cert_size == 0)) {
		LOG_ERR("Failed to get device cert");
		goto init_error;
	}

	ret = mbedtls_x509_crt_parse(&mbed_ctx->device_cert, (const unsigned char *)certs->device_cert_pem.cert,
				     certs->device_cert_pem.cert_size);

	if (ret) {
		LOG_ERR("Failed to parse device cert, error: 0x%x", ret);
		goto init_error;
	}

	/* Parse and set the device cert */
	ret = mbedtls_ssl_conf_own_cert(&mbed_ctx->conf, &mbed_ctx->device_cert,
					&mbed_ctx->device_private_key);

	if (ret) {
		LOG_ERR("Failed to set device cert and key, error: 0x%x", ret);
		goto init_error;
	}

	/* set entropy source */
	mbedtls_entropy_add_source(&mbed_ctx->entropy, auth_tls_entropy, NULL,
				   32,  MBEDTLS_ENTROPY_SOURCE_STRONG);

	mbedtls_ctr_drbg_seed(&mbed_ctx->ctr_drbg, mbedtls_entropy_func,
			      &mbed_ctx->entropy, NULL, 0);

	/* setup call to Zephyr random API */
	mbedtls_ssl_conf_rng(&mbed_ctx->conf, mbedtls_ctr_drbg_random, &mbed_ctx->ctr_drbg);

	mbedtls_ssl_conf_dbg(&mbed_ctx->conf, auth_mbed_debug, auth_conn);

#if defined(MBEDTLS_DEBUG_C)
	mbedtls_debug_set_threshold(CONFIG_MBEDTLS_DEBUG_LEVEL);
#endif


	if (!auth_conn->is_client) {

		auth_tls_set_cookie(auth_conn);

		ret = mbedtls_ssl_cookie_setup(&mbed_ctx->cookie_ctx,
					       mbedtls_ctr_drbg_random,
					       &mbed_ctx->ctr_drbg);

		if (ret) {
			LOG_ERR("Failed to setup dtls cookies, error: 0x%x", ret);
			goto init_error;
		}

		mbedtls_ssl_conf_dtls_cookies(&mbed_ctx->conf,
					      mbedtls_ssl_cookie_write,
					      mbedtls_ssl_cookie_check,
					      &mbed_ctx->cookie_ctx);
	}

	ret = mbedtls_ssl_setup(&mbed_ctx->ssl, &mbed_ctx->conf);

	if (ret) {
		LOG_ERR("mbedtls_ssl_setup returned %d", ret);
		goto init_error;
	}

	/* Setup timers */
	mbedtls_ssl_set_timer_cb(&mbed_ctx->ssl, &mbed_ctx->timer,
				 auth_tls_timing_set_delay,
				 auth_tls_timing_get_delay);

	return AUTH_SUCCESS;


init_error:
	LOG_ERR("Failed to initialize DTLS auth method.");
	auth_free_mbedcontext(mbed_ctx);
	return ret;
}

/**
 * @see auth_internal.h
 */
int auth_deinit_dtls(struct authenticate_conn *auth_conn)
{
	/* if present, free Mbed context */
	if(auth_conn->internal_obj) {
		auth_free_mbedcontext((struct mbed_tls_context *)auth_conn->internal_obj);
		auth_conn->internal_obj = NULL;
	}

	return AUTH_SUCCESS;
}


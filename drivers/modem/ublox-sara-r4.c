/*
 * Copyright (c) 2019-2020 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT u_blox_sara_r4

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(modem_ublox_sara_r4, CONFIG_MODEM_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <ctype.h>
#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/init.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_offload.h>
#include <zephyr/net/offloaded_netdev.h>
#include <zephyr/net/socket_offload.h>

#if defined(CONFIG_MODEM_UBLOX_SARA_AUTODETECT_APN)
#include <stdio.h>
#endif

#include "modem_context.h"
#include "modem_socket.h"
#include "modem_cmd_handler.h"
#include "modem_iface_uart.h"

#if !defined(CONFIG_MODEM_UBLOX_SARA_R4_MANUAL_MCCMNO)
#define CONFIG_MODEM_UBLOX_SARA_R4_MANUAL_MCCMNO ""
#endif


#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
#include "tls_internal.h"
#include <zephyr/net/tls_credentials.h>
#endif

/* pin settings */
static const struct gpio_dt_spec power_gpio = GPIO_DT_SPEC_INST_GET(0, mdm_power_gpios);
#if DT_INST_NODE_HAS_PROP(0, mdm_reset_gpios)
static const struct gpio_dt_spec reset_gpio = GPIO_DT_SPEC_INST_GET(0, mdm_reset_gpios);
#endif
#if DT_INST_NODE_HAS_PROP(0, mdm_vint_gpios)
static const struct gpio_dt_spec vint_gpio = GPIO_DT_SPEC_INST_GET(0, mdm_vint_gpios);
#endif

#define MDM_UART_NODE			DT_INST_BUS(0)
#define MDM_UART_DEV			DEVICE_DT_GET(MDM_UART_NODE)

#define MDM_RESET_NOT_ASSERTED		1
#define MDM_RESET_ASSERTED		0

#define MDM_CMD_TIMEOUT			K_SECONDS(10)
#define MDM_DNS_TIMEOUT			K_SECONDS(70)
#define MDM_CMD_CONN_TIMEOUT		K_SECONDS(120)
#define MDM_REGISTRATION_TIMEOUT	K_SECONDS(180)
#define MDM_PROMPT_CMD_DELAY		K_MSEC(50)

#define MDM_MAX_DATA_LENGTH		1024
#define MDM_RECV_MAX_BUF		30
#define MDM_RECV_BUF_SIZE		128

#define MDM_MAX_SOCKETS			6
#define MDM_BASE_SOCKET_NUM		0

#define MDM_NETWORK_RETRY_COUNT		3
#define MDM_WAIT_FOR_RSSI_COUNT		10
#define MDM_WAIT_FOR_RSSI_DELAY		K_SECONDS(2)

#define MDM_MANUFACTURER_LENGTH		10
#define MDM_MODEL_LENGTH		16
#define MDM_REVISION_LENGTH		64
#define MDM_IMEI_LENGTH			16
#define MDM_IMSI_LENGTH			16
#define MDM_APN_LENGTH			32
#define MDM_MAX_CERT_LENGTH		8192
#if defined(CONFIG_MODEM_UBLOX_SARA_AUTODETECT_VARIANT)
#define MDM_VARIANT_UBLOX_R4 4
#define MDM_VARIANT_UBLOX_U2 2
#endif

NET_BUF_POOL_DEFINE(mdm_recv_pool, MDM_RECV_MAX_BUF, MDM_RECV_BUF_SIZE,
		    0, NULL);

/* RX thread structures */
K_KERNEL_STACK_DEFINE(modem_rx_stack,
		      CONFIG_MODEM_UBLOX_SARA_R4_RX_STACK_SIZE);
struct k_thread modem_rx_thread;

#if defined(CONFIG_MODEM_UBLOX_SARA_RSSI_WORK)
/* RX thread work queue */
K_KERNEL_STACK_DEFINE(modem_workq_stack,
		      CONFIG_MODEM_UBLOX_SARA_R4_RX_WORKQ_STACK_SIZE);
static struct k_work_q modem_workq;
#endif

/* socket read callback data */
struct socket_read_data {
	char *recv_buf;
	size_t recv_buf_len;
	struct sockaddr *recv_addr;
	uint16_t recv_read_len;
};

/* driver data */
struct modem_data {
	struct net_if *net_iface;
	uint8_t mac_addr[6];

	/* modem interface */
	struct modem_iface_uart_data iface_data;
	uint8_t iface_rb_buf[MDM_MAX_DATA_LENGTH];

	/* modem cmds */
	struct modem_cmd_handler_data cmd_handler_data;
	uint8_t cmd_match_buf[MDM_RECV_BUF_SIZE + 1];

	/* socket data */
	struct modem_socket_config socket_config;
	struct modem_socket sockets[MDM_MAX_SOCKETS];

#if defined(CONFIG_MODEM_UBLOX_SARA_RSSI_WORK)
	/* RSSI work */
	struct k_work_delayable rssi_query_work;
#endif

	/* modem data */
	char mdm_manufacturer[MDM_MANUFACTURER_LENGTH];
	char mdm_model[MDM_MODEL_LENGTH];
	char mdm_revision[MDM_REVISION_LENGTH];
	char mdm_imei[MDM_IMEI_LENGTH];
	char mdm_imsi[MDM_IMSI_LENGTH];
	int mdm_rssi;

#if defined(CONFIG_MODEM_UBLOX_SARA_AUTODETECT_VARIANT)
	/* modem variant */
	int mdm_variant;
#endif
#if defined(CONFIG_MODEM_UBLOX_SARA_AUTODETECT_APN)
	/* APN */
	char mdm_apn[MDM_APN_LENGTH];
#endif

	/* modem state */
	int ev_creg;

	/* bytes written to socket in last transaction */
	int sock_written;

	/* response semaphore */
	struct k_sem sem_response;

	/* prompt semaphore */
	struct k_sem sem_prompt;
};

static struct modem_data mdata;
static struct modem_context mctx;

#if defined(CONFIG_DNS_RESOLVER)
static struct zsock_addrinfo result;
static struct sockaddr result_addr;
static char result_canonname[DNS_MAX_NAME_SIZE + 1];
#endif

/* helper macro to keep readability */
#define ATOI(s_, value_, desc_) modem_atoi(s_, value_, desc_, __func__)

/**
 * @brief  Convert string to long integer, but handle errors
 *
 * @param  s: string with representation of integer number
 * @param  err_value: on error return this value instead
 * @param  desc: name the string being converted
 * @param  func: function where this is called (typically __func__)
 *
 * @retval return integer conversion on success, or err_value on error
 */
static int modem_atoi(const char *s, const int err_value,
		      const char *desc, const char *func)
{
	int ret;
	char *endptr;

	ret = (int)strtol(s, &endptr, 10);
	if (!endptr || *endptr != '\0') {
		LOG_ERR("bad %s '%s' in %s", s, desc,
			func);
		return err_value;
	}

	return ret;
}

#if defined(CONFIG_MODEM_UBLOX_SARA_AUTODETECT_APN)

/* the list of SIM profiles. Global scope, so the app can change it */
const char *modem_sim_profiles =
	CONFIG_MODEM_UBLOX_SARA_AUTODETECT_APN_PROFILES;

int find_apn(char *apn, int apnlen, const char *profiles, const char *imsi)
{
	int rc = -1;

	/* try to find a match */
	char *s = strstr(profiles, imsi);

	if (s) {
		char *eos;

		/* find the assignment operator preceding the match */
		while (s >= profiles && !strchr("=", *s)) {
			s--;
		}
		/* find the apn preceding the assignment operator */
		while (s >= profiles && strchr(" =", *s)) {
			s--;
		}

		/* mark end of apn string */
		eos = s+1;

		/* find first character of the apn */
		while (s >= profiles && !strchr(" ,", *s)) {
			s--;
		}
		s++;

		/* copy the key */
		if (s >= profiles) {
			int len = eos - s;

			if (len < apnlen) {
				memcpy(apn, s, len);
				apn[len] = '\0';
				rc = 0;
			} else {
				LOG_ERR("buffer overflow");
			}
		}
	}

	return rc;
}

/* try to detect APN automatically, based on IMSI */
int modem_detect_apn(const char *imsi)
{
	int rc = -1;

	if (imsi != NULL && strlen(imsi) >= 5) {

		/* extract MMC and MNC from IMSI */
		char mmcmnc[6];
		*mmcmnc = 0;
		strncat(mmcmnc, imsi, sizeof(mmcmnc)-1);

		/* try to find a matching IMSI, and assign the APN */
		rc = find_apn(mdata.mdm_apn,
			sizeof(mdata.mdm_apn),
			modem_sim_profiles,
			mmcmnc);
		if (rc < 0) {
			rc = find_apn(mdata.mdm_apn,
				sizeof(mdata.mdm_apn),
				modem_sim_profiles,
				"*");
		}
	}

	if (rc == 0) {
		LOG_INF("Assign APN: \"%s\"", mdata.mdm_apn);
	}

	return rc;
}
#endif

/* Forward declaration */
MODEM_CMD_DEFINE(on_cmd_sockwrite);

/* send binary data via the +USO[ST/WR] commands */
static k_ssize_t send_socket_data(void *obj, const struct msghdr *msg, k_timeout_t timeout)
{
	int ret;
	char send_buf[sizeof("AT+USO**=###,"
			     "!####.####.####.####.####.####.####.####!,"
			     "#####,#########\r\n")];
	uint16_t dst_port = 0U;
	struct modem_socket *sock = (struct modem_socket *)obj;
	const struct modem_cmd handler_cmds[] = {
		MODEM_CMD("+USOST: ", on_cmd_sockwrite, 2U, ","),
		MODEM_CMD("+USOWR: ", on_cmd_sockwrite, 2U, ","),
	};
	struct sockaddr *dst_addr = msg->msg_name;
	size_t buf_len = 0;

	if (!sock) {
		return -EINVAL;
	}

	for (int i = 0; i < msg->msg_iovlen; i++) {
		if (!msg->msg_iov[i].iov_base || msg->msg_iov[i].iov_len == 0) {
			errno = EINVAL;
			return -1;
		}
		buf_len += msg->msg_iov[i].iov_len;
	}

	if (!sock->is_connected && sock->ip_proto != IPPROTO_UDP) {
		errno = ENOTCONN;
		return -1;
	}

	if (!dst_addr && sock->ip_proto == IPPROTO_UDP) {
		dst_addr = &sock->dst;
	}

	/*
	 * Binary and ASCII mode allows sending MDM_MAX_DATA_LENGTH bytes to
	 * the socket in one command
	 */
	if (buf_len > MDM_MAX_DATA_LENGTH) {
		if (sock->type == SOCK_DGRAM) {
			errno = EMSGSIZE;
			return -1;
		}

		buf_len = MDM_MAX_DATA_LENGTH;
	}

	/* The number of bytes written will be reported by the modem */
	mdata.sock_written = 0;

	if (sock->ip_proto == IPPROTO_UDP) {
		char ip_str[NET_IPV6_ADDR_LEN];

		ret = modem_context_sprint_ip_addr(dst_addr, ip_str, sizeof(ip_str));
		if (ret != 0) {
			LOG_ERR("Error formatting IP string %d", ret);
			goto exit;
		}

		ret = modem_context_get_addr_port(dst_addr, &dst_port);
		if (ret != 0) {
			LOG_ERR("Error getting port from IP address %d", ret);
			goto exit;
		}

		snprintk(send_buf, sizeof(send_buf),
			 "AT+USOST=%d,\"%s\",%u,%zu", sock->id,
			 ip_str,
			 dst_port, buf_len);
	} else {
		snprintk(send_buf, sizeof(send_buf), "AT+USOWR=%d,%zu",
			 sock->id, buf_len);
	}

	k_sem_take(&mdata.cmd_handler_data.sem_tx_lock, K_FOREVER);

	/* Reset prompt '@' semaphore */
	k_sem_reset(&mdata.sem_prompt);

	ret = modem_cmd_send_nolock(&mctx.iface, &mctx.cmd_handler,
				    NULL, 0U, send_buf, NULL, K_NO_WAIT);
	if (ret < 0) {
		goto exit;
	}

	/* set command handlers */
	ret = modem_cmd_handler_update_cmds(&mdata.cmd_handler_data,
					    handler_cmds,
					    ARRAY_SIZE(handler_cmds),
					    true);
	if (ret < 0) {
		goto exit;
	}

	/* Wait for prompt '@' */
	ret = k_sem_take(&mdata.sem_prompt, K_SECONDS(1));
	if (ret != 0) {
		ret = -ETIMEDOUT;
		LOG_ERR("No @ prompt received");
		goto exit;
	}

	/*
	 * The AT commands manual requires a 50 ms wait
	 * after '@' prompt if using AT+USOWR, but not
	 * if using AT+USOST. This if condition is matched with
	 * the command selection above.
	 */
	if (sock->ip_proto != IPPROTO_UDP) {
		k_sleep(MDM_PROMPT_CMD_DELAY);
	}

	/* Reset response semaphore before sending data
	 * So that we are sure that we won't use a previously pending one
	 * And we won't miss the one that is going to be freed
	 */
	k_sem_reset(&mdata.sem_response);

	/* Send data directly on modem iface */
	for (int i = 0; i < msg->msg_iovlen; i++) {
		int len = MIN(buf_len, msg->msg_iov[i].iov_len);

		if (len == 0) {
			break;
		}
		mctx.iface.write(&mctx.iface, msg->msg_iov[i].iov_base, len);
		buf_len -= len;
	}

	if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		ret = 0;
		goto exit;
	}
	ret = k_sem_take(&mdata.sem_response, timeout);

	if (ret == 0) {
		ret = modem_cmd_handler_get_error(&mdata.cmd_handler_data);
	} else if (ret == -EAGAIN) {
		ret = -ETIMEDOUT;
	}

exit:
	/* unset handler commands and ignore any errors */
	(void)modem_cmd_handler_update_cmds(&mdata.cmd_handler_data,
					    NULL, 0U, false);
	k_sem_give(&mdata.cmd_handler_data.sem_tx_lock);

	if (ret < 0) {
		return ret;
	}

	return mdata.sock_written;
}

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
/* send binary data via the +USO[ST/WR] commands */
static k_ssize_t send_cert(struct modem_socket *sock, struct modem_cmd *handler_cmds,
			      size_t handler_cmds_len, const char *cert_data, size_t cert_len,
			      int cert_type)
{
	int ret;
	char *filename = "ca";
	char send_buf[sizeof("AT+USECMNG=#,#,!####!,####\r\n")];

	/* TODO support other cert types as well */
	if (cert_type != 0) {
		return -EINVAL;
	}

	if (!sock) {
		return -EINVAL;
	}

	__ASSERT_NO_MSG(cert_len <= MDM_MAX_CERT_LENGTH);

	snprintk(send_buf, sizeof(send_buf),
		 "AT+USECMNG=0,%d,\"%s\",%d", cert_type, filename, cert_len);

	k_sem_take(&mdata.cmd_handler_data.sem_tx_lock, K_FOREVER);

	ret = modem_cmd_send_nolock(&mctx.iface, &mctx.cmd_handler,
				    NULL, 0U, send_buf, NULL, K_NO_WAIT);
	if (ret < 0) {
		goto exit;
	}

	/* set command handlers */
	ret = modem_cmd_handler_update_cmds(&mdata.cmd_handler_data,
					    handler_cmds, handler_cmds_len,
					    true);
	if (ret < 0) {
		goto exit;
	}

	/* slight pause per spec so that @ prompt is received */
	k_sleep(MDM_PROMPT_CMD_DELAY);
	mctx.iface.write(&mctx.iface, cert_data, cert_len);

	k_sem_reset(&mdata.sem_response);
	ret = k_sem_take(&mdata.sem_response, K_MSEC(1000));

	if (ret == 0) {
		ret = modem_cmd_handler_get_error(&mdata.cmd_handler_data);
	} else if (ret == -EAGAIN) {
		ret = -ETIMEDOUT;
	}

exit:
	/* unset handler commands and ignore any errors */
	(void)modem_cmd_handler_update_cmds(&mdata.cmd_handler_data,
					    NULL, 0U, false);
	k_sem_give(&mdata.cmd_handler_data.sem_tx_lock);

	return ret;
}
#endif

/*
 * Modem Response Command Handlers
 */

/* Handler: OK */
MODEM_CMD_DEFINE(on_cmd_ok)
{
	modem_cmd_handler_set_error(data, 0);
	k_sem_give(&mdata.sem_response);
	return 0;
}

/* Handler: @ */
MODEM_CMD_DEFINE(on_prompt)
{
	k_sem_give(&mdata.sem_prompt);

	/* A direct cmd should return the number of byte processed.
	 * Therefore, here we always return 1
	 */
	return 1;
}

/* Handler: ERROR */
MODEM_CMD_DEFINE(on_cmd_error)
{
	modem_cmd_handler_set_error(data, -EIO);
	k_sem_give(&mdata.sem_response);
	return 0;
}

/* Handler: +CME Error: <err>[0] */
MODEM_CMD_DEFINE(on_cmd_exterror)
{
	/* TODO: map extended error codes to values */
	modem_cmd_handler_set_error(data, -EIO);
	k_sem_give(&mdata.sem_response);
	return 0;
}

/*
 * Modem Info Command Handlers
 */

/* Handler: <manufacturer> */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_manufacturer)
{
	size_t out_len;

	out_len = net_buf_linearize(mdata.mdm_manufacturer,
				    sizeof(mdata.mdm_manufacturer) - 1,
				    data->rx_buf, 0, len);
	mdata.mdm_manufacturer[out_len] = '\0';
	LOG_INF("Manufacturer: %s", mdata.mdm_manufacturer);
	return 0;
}

/* Handler: <model> */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_model)
{
	size_t out_len;

	out_len = net_buf_linearize(mdata.mdm_model,
				    sizeof(mdata.mdm_model) - 1,
				    data->rx_buf, 0, len);
	mdata.mdm_model[out_len] = '\0';
	LOG_INF("Model: %s", mdata.mdm_model);

#if defined(CONFIG_MODEM_UBLOX_SARA_AUTODETECT_VARIANT)
	/* Set modem type */
	if (strstr(mdata.mdm_model, "R4")) {
		mdata.mdm_variant = MDM_VARIANT_UBLOX_R4;
	} else {
		if (strstr(mdata.mdm_model, "U2")) {
			mdata.mdm_variant = MDM_VARIANT_UBLOX_U2;
		}
	}
	LOG_INF("Variant: %d", mdata.mdm_variant);
#endif

	return 0;
}

/* Handler: <rev> */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_revision)
{
	size_t out_len;

	out_len = net_buf_linearize(mdata.mdm_revision,
				    sizeof(mdata.mdm_revision) - 1,
				    data->rx_buf, 0, len);
	mdata.mdm_revision[out_len] = '\0';
	LOG_INF("Revision: %s", mdata.mdm_revision);
	return 0;
}

/* Handler: <IMEI> */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_imei)
{
	size_t out_len;

	out_len = net_buf_linearize(mdata.mdm_imei, sizeof(mdata.mdm_imei) - 1,
				    data->rx_buf, 0, len);
	mdata.mdm_imei[out_len] = '\0';
	LOG_INF("IMEI: %s", mdata.mdm_imei);
	return 0;
}

/* Handler: <IMSI> */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_imsi)
{
	size_t out_len;

	out_len = net_buf_linearize(mdata.mdm_imsi, sizeof(mdata.mdm_imsi) - 1,
				    data->rx_buf, 0, len);
	mdata.mdm_imsi[out_len] = '\0';
	LOG_INF("IMSI: %s", mdata.mdm_imsi);

#if defined(CONFIG_MODEM_UBLOX_SARA_AUTODETECT_APN)
	/* set the APN automatically */
	modem_detect_apn(mdata.mdm_imsi);
#endif

	return 0;
}

#if !defined(CONFIG_MODEM_UBLOX_SARA_U2)
/*
 * Handler: +CESQ: <rxlev>[0],<ber>[1],<rscp>[2],<ecn0>[3],<rsrq>[4],<rsrp>[5]
 */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_rssi_cesq)
{
	int rsrp, rxlev;

	rsrp = ATOI(argv[5], 0, "rsrp");
	rxlev = ATOI(argv[0], 0, "rxlev");
	if (rsrp >= 0 && rsrp <= 97) {
		mdata.mdm_rssi = -140 + (rsrp - 1);
		LOG_INF("RSRP: %d", mdata.mdm_rssi);
	} else if (rxlev >= 0 && rxlev <= 63) {
		mdata.mdm_rssi = -110 + (rxlev - 1);
		LOG_INF("RSSI: %d", mdata.mdm_rssi);
	} else {
		mdata.mdm_rssi = -1000;
		LOG_INF("RSRP/RSSI not known");
	}

	return 0;
}
#endif

#if defined(CONFIG_MODEM_UBLOX_SARA_U2) \
	|| defined(CONFIG_MODEM_UBLOX_SARA_AUTODETECT_VARIANT)
/* Handler: +CSQ: <signal_power>[0],<qual>[1] */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_rssi_csq)
{
	int rssi;

	rssi = ATOI(argv[0], 0, "signal_power");
	if (rssi == 31) {
		mdata.mdm_rssi = -46;
	} else if (rssi >= 0 && rssi <= 31) {
		/* FIXME: This value depends on the RAT */
		mdata.mdm_rssi = -110 + ((rssi * 2) + 1);
	} else {
		mdata.mdm_rssi = -1000;
	}

	LOG_INF("RSSI: %d", mdata.mdm_rssi);
	return 0;
}
#endif

#if defined(CONFIG_MODEM_CELL_INFO)
static int unquoted_atoi(const char *s, int base)
{
	if (*s == '"') {
		s++;
	}

	return strtol(s, NULL, base);
}

/*
 * Handler: +COPS: <mode>[0],<format>[1],<oper>[2]
 */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_cops)
{
	if (argc >= 3) {
		mctx.data_operator = unquoted_atoi(argv[2], 10);
		LOG_INF("operator: %u",
			mctx.data_operator);
	}

	return 0;
}

/*
 * Handler: +CEREG: <n>[0],<stat>[1],<tac>[2],<ci>[3],<AcT>[4]
 */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_cereg)
{
	if (argc >= 4) {
		mctx.data_lac = unquoted_atoi(argv[2], 16);
		mctx.data_cellid = unquoted_atoi(argv[3], 16);
		LOG_INF("lac: %u, cellid: %u",
			mctx.data_lac,
			mctx.data_cellid);
	}

	return 0;
}

static const struct setup_cmd query_cellinfo_cmds[] = {
	SETUP_CMD_NOHANDLE("AT+CEREG=2"),
	SETUP_CMD("AT+CEREG?", "", on_cmd_atcmdinfo_cereg, 5U, ","),
	SETUP_CMD_NOHANDLE("AT+COPS=3,2"),
	SETUP_CMD("AT+COPS?", "", on_cmd_atcmdinfo_cops, 3U, ","),
};
#endif /* CONFIG_MODEM_CELL_INFO */

/*
 * Modem Socket Command Handlers
 */

/* Handler: +USOCR: <socket_id>[0] */
MODEM_CMD_DEFINE(on_cmd_sockcreate)
{
	struct modem_socket *sock = NULL;
	int id;

	/* look up new socket by special id */
	sock = modem_socket_from_newid(&mdata.socket_config);
	if (sock) {
		id = ATOI(argv[0], -1, "socket_id");

		/* on error give up modem socket */
		if (modem_socket_id_assign(&mdata.socket_config, sock, id) < 0) {
			modem_socket_put(&mdata.socket_config, sock->sock_fd);
		}
	}

	/* don't give back semaphore -- OK to follow */
	return 0;
}

/* Handler: +USO[WR|ST]: <socket_id>[0],<length>[1] */
MODEM_CMD_DEFINE(on_cmd_sockwrite)
{
	mdata.sock_written = ATOI(argv[1], 0, "length");
	LOG_DBG("bytes written: %d", mdata.sock_written);
	return 0;
}

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
/* Handler: +USECMNG: 0,<type>[0],<internal_name>[1],<md5_string>[2] */
MODEM_CMD_DEFINE(on_cmd_cert_write)
{
	LOG_DBG("cert md5: %s", argv[2]);
	return 0;
}
#endif

/* Common code for +USOR[D|F]: "<data>" */
static int on_cmd_sockread_common(int socket_id,
				  struct modem_cmd_handler_data *data,
				  int socket_data_length, uint16_t len)
{
	struct modem_socket *sock = NULL;
	struct socket_read_data *sock_data;
	int ret;

	if (!len) {
		LOG_ERR("Short +USOR[D|F] value.  Aborting!");
		return -EAGAIN;
	}

	/*
	 * make sure we still have buf data and next char in the buffer is a
	 * quote.
	 */
	if (!data->rx_buf || *data->rx_buf->data != '\"') {
		LOG_ERR("Incorrect format! Ignoring data!");
		return -EINVAL;
	}

	/* zero length */
	if (socket_data_length <= 0) {
		LOG_ERR("Length problem (%d).  Aborting!", socket_data_length);
		return -EAGAIN;
	}

	/* check to make sure we have all of the data (minus quotes) */
	if ((net_buf_frags_len(data->rx_buf) - 2) < socket_data_length) {
		LOG_DBG("Not enough data -- wait!");
		return -EAGAIN;
	}

	/* skip quote */
	len--;
	net_buf_pull_u8(data->rx_buf);
	if (!data->rx_buf->len) {
		data->rx_buf = net_buf_frag_del(NULL, data->rx_buf);
	}

	sock = modem_socket_from_id(&mdata.socket_config, socket_id);
	if (!sock) {
		LOG_ERR("Socket not found! (%d)", socket_id);
		ret = -EINVAL;
		goto exit;
	}

	sock_data = (struct socket_read_data *)sock->data;
	if (!sock_data) {
		LOG_ERR("Socket data not found! Skip handling (%d)", socket_id);
		ret = -EINVAL;
		goto exit;
	}

	ret = net_buf_linearize(sock_data->recv_buf, sock_data->recv_buf_len,
				data->rx_buf, 0, (uint16_t)socket_data_length);
	data->rx_buf = net_buf_skip(data->rx_buf, ret);
	sock_data->recv_read_len = ret;
	if (ret != socket_data_length) {
		LOG_ERR("Total copied data is different then received data!"
			" copied:%d vs. received:%d", ret, socket_data_length);
		ret = -EINVAL;
	}

exit:
	/* remove packet from list (ignore errors) */
	(void)modem_socket_packet_size_update(&mdata.socket_config, sock,
					      -socket_data_length);

	/* don't give back semaphore -- OK to follow */
	return ret;
}

/*
 * Handler: +USORF: <socket_id>[0],<remote_ip_addr>[1],<remote_port>[2],
 *          <length>[3],"<data>"
*/
MODEM_CMD_DEFINE(on_cmd_sockreadfrom)
{
	/* TODO: handle remote_ip_addr */

	return on_cmd_sockread_common(ATOI(argv[0], 0, "socket_id"), data,
				      ATOI(argv[3], 0, "length"), len);
}

/* Handler: +USORD: <socket_id>[0],<length>[1],"<data>" */
MODEM_CMD_DEFINE(on_cmd_sockread)
{
	return on_cmd_sockread_common(ATOI(argv[0], 0, "socket_id"), data,
				      ATOI(argv[1], 0, "length"), len);
}

#if defined(CONFIG_DNS_RESOLVER)
/* Handler: +UDNSRN: "<resolved_ip_address>"[0], "<resolved_ip_address>"[1] */
MODEM_CMD_DEFINE(on_cmd_dns)
{
	/* chop off end quote */
	argv[0][strlen(argv[0]) - 1] = '\0';

	/* FIXME: Hard-code DNS on SARA-R4 to return IPv4 */
	result_addr.sa_family = AF_INET;
	/* skip beginning quote when parsing */
	(void)net_addr_pton(result.ai_family, &argv[0][1],
			    &((struct sockaddr_in *)&result_addr)->sin_addr);
	return 0;
}
#endif

/*
 * MODEM UNSOLICITED NOTIFICATION HANDLERS
 */

/* Handler: +UUSOCL: <socket_id>[0] */
MODEM_CMD_DEFINE(on_cmd_socknotifyclose)
{
	struct modem_socket *sock;

	sock = modem_socket_from_id(&mdata.socket_config,
				    ATOI(argv[0], 0, "socket_id"));
	if (sock) {
		sock->is_connected = false;
	}

	return 0;
}

/* Handler: +UUSOR[D|F]: <socket_id>[0],<length>[1] */
MODEM_CMD_DEFINE(on_cmd_socknotifydata)
{
	int ret, socket_id, new_total;
	struct modem_socket *sock;

	socket_id = ATOI(argv[0], 0, "socket_id");
	new_total = ATOI(argv[1], 0, "length");
	sock = modem_socket_from_id(&mdata.socket_config, socket_id);
	if (!sock) {
		return 0;
	}

	ret = modem_socket_packet_size_update(&mdata.socket_config, sock,
					      new_total);
	if (ret < 0) {
		LOG_ERR("socket_id:%d left_bytes:%d err: %d", socket_id,
			new_total, ret);
	}

	if (new_total > 0) {
		modem_socket_data_ready(&mdata.socket_config, sock);
	}

	return 0;
}

/* Handler: +CREG: <stat>[0] */
MODEM_CMD_DEFINE(on_cmd_socknotifycreg)
{
	mdata.ev_creg = ATOI(argv[0], 0, "stat");
	LOG_DBG("CREG:%d", mdata.ev_creg);
	return 0;
}

/* RX thread */
static void modem_rx(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		/* wait for incoming data */
		modem_iface_uart_rx_wait(&mctx.iface, K_FOREVER);

		modem_cmd_handler_process(&mctx.cmd_handler, &mctx.iface);

		/* give up time if we have a solid stream of data */
		k_yield();
	}
}

static int pin_init(void)
{
	LOG_INF("Setting Modem Pins");

#if DT_INST_NODE_HAS_PROP(0, mdm_reset_gpios)
	LOG_DBG("MDM_RESET_PIN -> NOT_ASSERTED");
	gpio_pin_set_dt(&reset_gpio, MDM_RESET_NOT_ASSERTED);
#endif

	LOG_DBG("MDM_POWER_PIN -> ENABLE");
	gpio_pin_set_dt(&power_gpio, 1);
	k_sleep(K_SECONDS(4));

	LOG_DBG("MDM_POWER_PIN -> DISABLE");
	gpio_pin_set_dt(&power_gpio, 0);
#if defined(CONFIG_MODEM_UBLOX_SARA_U2)
	k_sleep(K_SECONDS(1));
#else
	k_sleep(K_SECONDS(4));
#endif
	LOG_DBG("MDM_POWER_PIN -> ENABLE");
	gpio_pin_set_dt(&power_gpio, 1);
	k_sleep(K_SECONDS(1));

	/* make sure module is powered off */
#if DT_INST_NODE_HAS_PROP(0, mdm_vint_gpios)
	LOG_DBG("Waiting for MDM_VINT_PIN = 0");

	while (gpio_pin_get_dt(&vint_gpio) > 0) {
#if defined(CONFIG_MODEM_UBLOX_SARA_U2)
		/* try to power off again */
		LOG_DBG("MDM_POWER_PIN -> DISABLE");
		gpio_pin_set_dt(&power_gpio, 0);
		k_sleep(K_SECONDS(1));
		LOG_DBG("MDM_POWER_PIN -> ENABLE");
		gpio_pin_set_dt(&power_gpio, 1);
#endif
		k_sleep(K_MSEC(100));
	}
#else
	k_sleep(K_SECONDS(8));
#endif

	LOG_DBG("MDM_POWER_PIN -> DISABLE");

	unsigned int irq_lock_key = irq_lock();

	gpio_pin_set_dt(&power_gpio, 0);
#if defined(CONFIG_MODEM_UBLOX_SARA_U2)
	k_usleep(50);		/* 50-80 microseconds */
#else
	k_sleep(K_SECONDS(1));
#endif
	gpio_pin_set_dt(&power_gpio, 1);

	irq_unlock(irq_lock_key);

	LOG_DBG("MDM_POWER_PIN -> ENABLE");

#if DT_INST_NODE_HAS_PROP(0, mdm_vint_gpios)
	LOG_DBG("Waiting for MDM_VINT_PIN = 1");
	do {
		k_sleep(K_MSEC(100));
	} while (gpio_pin_get_dt(&vint_gpio) == 0);
#else
	k_sleep(K_SECONDS(10));
#endif

	gpio_pin_configure_dt(&power_gpio, GPIO_INPUT);

	LOG_INF("... Done!");

	return 0;
}

#if defined(CONFIG_MODEM_UBLOX_SARA_AUTODETECT_VARIANT)
static void modem_rssi_query_work(struct k_work *work)
{
	static const struct modem_cmd cmds[] = {
		  MODEM_CMD("+CSQ: ", on_cmd_atcmdinfo_rssi_csq, 2U, ","),
		  MODEM_CMD("+CESQ: ", on_cmd_atcmdinfo_rssi_cesq, 6U, ","),
	};
	const char *send_cmd_u2 = "AT+CSQ";
	const char *send_cmd_r4 = "AT+CESQ";
	int ret;

	/* choose cmd according to variant */
	const char *send_cmd = send_cmd_r4;

	if (mdata.mdm_variant == MDM_VARIANT_UBLOX_U2) {
		send_cmd = send_cmd_u2;
	}

	/* query modem RSSI */
	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler,
		cmds, ARRAY_SIZE(cmds),
		send_cmd,
		&mdata.sem_response,
		MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("AT+C[E]SQ ret:%d", ret);
	}

#if defined(CONFIG_MODEM_CELL_INFO)
	/* query cell info */
	ret = modem_cmd_handler_setup_cmds_nolock(&mctx.iface,
						  &mctx.cmd_handler,
						  query_cellinfo_cmds,
						  ARRAY_SIZE(query_cellinfo_cmds),
						  &mdata.sem_response,
						  MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_WRN("modem query for cell info returned %d", ret);
	}
#endif

#if defined(CONFIG_MODEM_UBLOX_SARA_RSSI_WORK)
	/* re-start RSSI query work */
	if (work) {
		k_work_reschedule_for_queue(
			&modem_workq, &mdata.rssi_query_work,
			K_SECONDS(CONFIG_MODEM_UBLOX_SARA_RSSI_WORK_PERIOD));
	}
#endif
}
#else
static void modem_rssi_query_work(struct k_work *work)
{
	static const struct modem_cmd cmd =
#if defined(CONFIG_MODEM_UBLOX_SARA_U2)
		MODEM_CMD("+CSQ: ", on_cmd_atcmdinfo_rssi_csq, 2U, ",");
	static char *send_cmd = "AT+CSQ";
#else
		MODEM_CMD("+CESQ: ", on_cmd_atcmdinfo_rssi_cesq, 6U, ",");
	static char *send_cmd = "AT+CESQ";
#endif
	int ret;

	/* query modem RSSI */
	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler,
			     &cmd, 1U, send_cmd, &mdata.sem_response,
			     MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("AT+C[E]SQ ret:%d", ret);
	}

#if defined(CONFIG_MODEM_CELL_INFO)
	/* query cell info */
	ret = modem_cmd_handler_setup_cmds_nolock(&mctx.iface,
						  &mctx.cmd_handler,
						  query_cellinfo_cmds,
						  ARRAY_SIZE(query_cellinfo_cmds),
						  &mdata.sem_response,
						  MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_WRN("modem query for cell info returned %d", ret);
	}
#endif

#if defined(CONFIG_MODEM_UBLOX_SARA_RSSI_WORK)
	/* re-start RSSI query work */
	if (work) {
		k_work_reschedule_for_queue(
			&modem_workq, &mdata.rssi_query_work,
			K_SECONDS(CONFIG_MODEM_UBLOX_SARA_RSSI_WORK_PERIOD));
	}
#endif
}
#endif

static void modem_reset(void)
{
	int ret = 0, retry_count = 0, counter = 0;
	static const struct setup_cmd setup_cmds[] = {
		/* turn off echo */
		SETUP_CMD_NOHANDLE("ATE0"),
		/* stop functionality */
		SETUP_CMD_NOHANDLE("AT+CFUN=0"),
		/* extended error numbers */
		SETUP_CMD_NOHANDLE("AT+CMEE=1"),
#if defined(CONFIG_BOARD_PARTICLE_BORON)
		/* use external SIM */
		SETUP_CMD_NOHANDLE("AT+UGPIOC=23,0,0"),
#endif
#if defined(CONFIG_MODEM_UBLOX_SARA_R4_NET_STATUS_PIN)
		/* enable the network status indication */
		SETUP_CMD_NOHANDLE("AT+UGPIOC="
			STRINGIFY(CONFIG_MODEM_UBLOX_SARA_R4_NET_STATUS_PIN)
			",2"),
#endif
		/* UNC messages for registration */
		SETUP_CMD_NOHANDLE("AT+CREG=1"),
		/* query modem info */
		SETUP_CMD("AT+CGMI", "", on_cmd_atcmdinfo_manufacturer, 0U, ""),
		SETUP_CMD("AT+CGMM", "", on_cmd_atcmdinfo_model, 0U, ""),
		SETUP_CMD("AT+CGMR", "", on_cmd_atcmdinfo_revision, 0U, ""),
		SETUP_CMD("AT+CGSN", "", on_cmd_atcmdinfo_imei, 0U, ""),
		SETUP_CMD("AT+CIMI", "", on_cmd_atcmdinfo_imsi, 0U, ""),
#if !defined(CONFIG_MODEM_UBLOX_SARA_AUTODETECT_APN)
		/* setup PDP context definition */
		SETUP_CMD_NOHANDLE("AT+CGDCONT=1,\"IP\",\""
					CONFIG_MODEM_UBLOX_SARA_R4_APN "\""),
		/* start functionality */
		SETUP_CMD_NOHANDLE("AT+CFUN=1"),
#endif
	};

#if defined(CONFIG_MODEM_UBLOX_SARA_AUTODETECT_VARIANT)
	static const struct setup_cmd post_setup_cmds_u2[] = {
#if !defined(CONFIG_MODEM_UBLOX_SARA_AUTODETECT_APN)
		/* set the APN */
		SETUP_CMD_NOHANDLE("AT+UPSD=0,1,\""
				CONFIG_MODEM_UBLOX_SARA_R4_APN "\""),
#endif
		/* set dynamic IP */
		SETUP_CMD_NOHANDLE("AT+UPSD=0,7,\"0.0.0.0\""),
		/* activate the GPRS connection */
		SETUP_CMD_NOHANDLE("AT+UPSDA=0,3"),
	};
#endif

	static const struct setup_cmd post_setup_cmds[] = {
#if defined(CONFIG_MODEM_UBLOX_SARA_U2)
		/* set the APN */
		SETUP_CMD_NOHANDLE("AT+UPSD=0,1,\""
				CONFIG_MODEM_UBLOX_SARA_R4_APN "\""),
		/* set dynamic IP */
		SETUP_CMD_NOHANDLE("AT+UPSD=0,7,\"0.0.0.0\""),
		/* activate the GPRS connection */
		SETUP_CMD_NOHANDLE("AT+UPSDA=0,3"),
#else
		/* activate the PDP context */
		SETUP_CMD_NOHANDLE("AT+CGACT=1,1"),
#endif
	};

restart:

#if defined(CONFIG_MODEM_UBLOX_SARA_AUTODETECT_APN)
	mdata.mdm_apn[0] = '\0';
	strncat(mdata.mdm_apn,
		CONFIG_MODEM_UBLOX_SARA_R4_APN,
		sizeof(mdata.mdm_apn)-1);
#endif

#if defined(CONFIG_MODEM_UBLOX_SARA_RSSI_WORK)
	/* stop RSSI delay work */
	k_work_cancel_delayable(&mdata.rssi_query_work);
#endif

	pin_init();

	LOG_INF("Waiting for modem to respond");

	/* Give the modem a while to start responding to simple 'AT' commands.
	 * Also wait for CSPS=1 or RRCSTATE=1 notification
	 */
	ret = -1;
	while (counter++ < 50 && ret < 0) {
		k_sleep(K_SECONDS(2));
		ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler,
				     NULL, 0, "AT", &mdata.sem_response,
				     MDM_CMD_TIMEOUT);
		if (ret < 0 && ret != -ETIMEDOUT) {
			break;
		}
	}

	if (ret < 0) {
		LOG_ERR("MODEM WAIT LOOP ERROR: %d", ret);
		goto error;
	}

	ret = modem_cmd_handler_setup_cmds(&mctx.iface, &mctx.cmd_handler,
					   setup_cmds, ARRAY_SIZE(setup_cmds),
					   &mdata.sem_response,
					   MDM_REGISTRATION_TIMEOUT);
	if (ret < 0) {
		goto error;
	}

#if defined(CONFIG_MODEM_UBLOX_SARA_AUTODETECT_APN)
	/* autodetect APN from IMSI */
	char cmd[sizeof("AT+CGDCONT=1,\"IP\",\"\"")+MDM_APN_LENGTH];

	snprintk(cmd, sizeof(cmd), "AT+CGDCONT=1,\"IP\",\"%s\"", mdata.mdm_apn);

	/* setup PDP context definition */
	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler,
		NULL, 0,
		(const char *)cmd,
		&mdata.sem_response,
		MDM_REGISTRATION_TIMEOUT);

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler,
		NULL, 0,
		"AT+CFUN=1",
		&mdata.sem_response,
		MDM_REGISTRATION_TIMEOUT);
#endif

	if (strlen(CONFIG_MODEM_UBLOX_SARA_R4_MANUAL_MCCMNO) > 0) {
		/* use manual MCC/MNO entry */
		ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler,
				     NULL, 0,
				     "AT+COPS=1,2,\""
					CONFIG_MODEM_UBLOX_SARA_R4_MANUAL_MCCMNO
					"\"",
				     &mdata.sem_response,
				     MDM_REGISTRATION_TIMEOUT);
	} else {
		/* register operator automatically */
		ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler,
				     NULL, 0, "AT+COPS=0,0",
				     &mdata.sem_response,
				     MDM_REGISTRATION_TIMEOUT);
	}

	if (ret < 0) {
		LOG_ERR("AT+COPS ret:%d", ret);
		goto error;
	}

	LOG_INF("Waiting for network");

	/*
	 * TODO: A lot of this should be setup as a 3GPP module to handle
	 * basic connection to the network commands / polling
	 */

	/* wait for +CREG: 1(normal) or 5(roaming) */
	counter = 0;
	while (counter++ < 40 && mdata.ev_creg != 1 && mdata.ev_creg != 5) {
		if (counter == 20) {
			LOG_WRN("Force restart of RF functionality");

			/* Disable RF temporarily */
			ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler,
				NULL, 0, "AT+CFUN=0", &mdata.sem_response,
				MDM_CMD_TIMEOUT);

			k_sleep(K_SECONDS(1));

			/* Enable RF */
			ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler,
				NULL, 0, "AT+CFUN=1", &mdata.sem_response,
				MDM_CMD_TIMEOUT);
		}

		k_sleep(K_SECONDS(1));
	}

	/* query modem RSSI */
	modem_rssi_query_work(NULL);
	k_sleep(MDM_WAIT_FOR_RSSI_DELAY);

	counter = 0;
	/* wait for RSSI < 0 and > -1000 */
	while (counter++ < MDM_WAIT_FOR_RSSI_COUNT &&
	       (mdata.mdm_rssi >= 0 ||
		mdata.mdm_rssi <= -1000)) {
		modem_rssi_query_work(NULL);
		k_sleep(MDM_WAIT_FOR_RSSI_DELAY);
	}

	if (mdata.mdm_rssi >= 0 || mdata.mdm_rssi <= -1000) {
		retry_count++;
		if (retry_count >= MDM_NETWORK_RETRY_COUNT) {
			LOG_ERR("Failed network init.  Too many attempts!");
			ret = -ENETUNREACH;
			goto error;
		}

		LOG_ERR("Failed network init.  Restarting process.");
		goto restart;
	}

#if defined(CONFIG_MODEM_UBLOX_SARA_AUTODETECT_VARIANT)
	if (mdata.mdm_variant == MDM_VARIANT_UBLOX_U2) {

#if defined(CONFIG_MODEM_UBLOX_SARA_AUTODETECT_APN)
		/* setup PDP context definition */
		char cmd[sizeof("AT+UPSD=0,1,\"%s\"")+MDM_APN_LENGTH];

		snprintk(cmd, sizeof(cmd), "AT+UPSD=0,1,\"%s\"", mdata.mdm_apn);
		ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler,
			NULL, 0,
			(const char *)cmd,
			&mdata.sem_response,
			MDM_REGISTRATION_TIMEOUT);
#endif
		ret = modem_cmd_handler_setup_cmds(&mctx.iface,
			&mctx.cmd_handler,
			post_setup_cmds_u2,
			ARRAY_SIZE(post_setup_cmds_u2),
			&mdata.sem_response,
			MDM_REGISTRATION_TIMEOUT);
	} else {
#endif
		ret = modem_cmd_handler_setup_cmds(&mctx.iface,
					   &mctx.cmd_handler,
					   post_setup_cmds,
					   ARRAY_SIZE(post_setup_cmds),
					   &mdata.sem_response,
					   MDM_REGISTRATION_TIMEOUT);
#if defined(CONFIG_MODEM_UBLOX_SARA_AUTODETECT_VARIANT)
	}
#endif
	if (ret < 0) {
		goto error;
	}

	LOG_INF("Network is ready.");

#if defined(CONFIG_MODEM_UBLOX_SARA_RSSI_WORK)
	/* start RSSI query */
	k_work_reschedule_for_queue(
		&modem_workq, &mdata.rssi_query_work,
		K_SECONDS(CONFIG_MODEM_UBLOX_SARA_RSSI_WORK_PERIOD));
#endif

error:
	return;
}

/*
 * generic socket creation function
 * which can be called in bind() or connect()
 */
static int create_socket(struct modem_socket *sock, const struct sockaddr *addr)
{
	int ret;
	static const struct modem_cmd cmd =
		MODEM_CMD("+USOCR: ", on_cmd_sockcreate, 1U, "");
	char buf[sizeof("AT+USOCR=#,#####\r")];
	uint16_t local_port = 0U, proto = 6U;

	if (addr) {
		if (addr->sa_family == AF_INET6) {
			local_port = ntohs(net_sin6(addr)->sin6_port);
		} else if (addr->sa_family == AF_INET) {
			local_port = ntohs(net_sin(addr)->sin_port);
		}
	}

	if (sock->ip_proto == IPPROTO_UDP) {
		proto = 17U;
	}

	if (local_port > 0U) {
		snprintk(buf, sizeof(buf), "AT+USOCR=%d,%u", proto, local_port);
	} else {
		snprintk(buf, sizeof(buf), "AT+USOCR=%d", proto);
	}

	/* create socket */
	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler,
			     &cmd, 1U, buf,
			     &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		goto error;
	}

	if (sock->ip_proto == IPPROTO_TLS_1_2) {
		char atbuf[sizeof("AT+USECPRF=#,#,#######\r")];

		/* Enable socket security */
		snprintk(atbuf, sizeof(atbuf), "AT+USOSEC=%d,1,%d", sock->id, sock->id);
		ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, atbuf,
				     &mdata.sem_response, MDM_CMD_TIMEOUT);
		if (ret < 0) {
			goto error;
		}
		/* Reset the security profile */
		snprintk(atbuf, sizeof(atbuf), "AT+USECPRF=%d", sock->id);
		ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, atbuf,
				     &mdata.sem_response, MDM_CMD_TIMEOUT);
		if (ret < 0) {
			goto error;
		}
		/* Validate server cert against the CA.  */
		snprintk(atbuf, sizeof(atbuf), "AT+USECPRF=%d,0,1", sock->id);
		ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, atbuf,
				     &mdata.sem_response, MDM_CMD_TIMEOUT);
		if (ret < 0) {
			goto error;
		}
		/* Use TLSv1.2 only */
		snprintk(atbuf, sizeof(atbuf), "AT+USECPRF=%d,1,3", sock->id);
		ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, atbuf,
				     &mdata.sem_response, MDM_CMD_TIMEOUT);
		if (ret < 0) {
			goto error;
		}
		/* Set root CA filename */
		snprintk(atbuf, sizeof(atbuf), "AT+USECPRF=%d,3,\"ca\"", sock->id);
		ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, atbuf,
				     &mdata.sem_response, MDM_CMD_TIMEOUT);
		if (ret < 0) {
			goto error;
		}
	}

	errno = 0;
	return 0;

error:
	LOG_ERR("%s ret:%d", buf, ret);
	modem_socket_put(&mdata.socket_config, sock->sock_fd);
	errno = -ret;
	return -1;
}

/*
 * Socket Offload OPS
 */

static const struct socket_op_vtable offload_socket_fd_op_vtable;

static int offload_socket(int family, int type, int proto)
{
	int ret;

	/* defer modem's socket create call to bind() */
	ret = modem_socket_get(&mdata.socket_config, family, type, proto);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	errno = 0;
	return ret;
}

static int offload_close(void *obj)
{
	struct modem_socket *sock = (struct modem_socket *)obj;
	char buf[sizeof("AT+USOCL=#\r")];
	int ret;

	/* make sure socket is allocated and assigned an id */
	if (modem_socket_id_is_assigned(&mdata.socket_config, sock) == false) {
		return 0;
	}

	if (sock->is_connected || sock->ip_proto == IPPROTO_UDP) {
		snprintk(buf, sizeof(buf), "AT+USOCL=%d", sock->id);

		ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler,
				     NULL, 0U, buf,
				     &mdata.sem_response, MDM_CMD_TIMEOUT);
		if (ret < 0) {
			LOG_ERR("%s ret:%d", buf, ret);
		}
	}

	modem_socket_put(&mdata.socket_config, sock->sock_fd);
	return 0;
}

static int offload_bind(void *obj, const struct sockaddr *addr,
			socklen_t addrlen)
{
	struct modem_socket *sock = (struct modem_socket *)obj;

	/* save bind address information */
	memcpy(&sock->src, addr, sizeof(*addr));

	/* make sure we've created the socket */
	if (modem_socket_is_allocated(&mdata.socket_config, sock) == true) {
		if (create_socket(sock, addr) < 0) {
			return -1;
		}
	}

	return 0;
}

static int offload_connect(void *obj, const struct sockaddr *addr,
			   socklen_t addrlen)
{
	struct modem_socket *sock = (struct modem_socket *)obj;
	int ret;
	char buf[sizeof("AT+USOCO=###,!####.####.####.####.####.####.####.####!,#####,#\r")];
	uint16_t dst_port = 0U;
	char ip_str[NET_IPV6_ADDR_LEN];

	if (!addr) {
		errno = EINVAL;
		return -1;
	}

	/* make sure socket has been allocated */
	if (modem_socket_is_allocated(&mdata.socket_config, sock) == false) {
		LOG_ERR("Invalid socket_id(%d) from fd:%d",
			sock->id, sock->sock_fd);
		errno = EINVAL;
		return -1;
	}

	/* make sure we've created the socket */
	if (modem_socket_id_is_assigned(&mdata.socket_config, sock) == false) {
		if (create_socket(sock, NULL) < 0) {
			return -1;
		}
	}

	memcpy(&sock->dst, addr, sizeof(*addr));
	if (addr->sa_family == AF_INET6) {
		dst_port = ntohs(net_sin6(addr)->sin6_port);
	} else if (addr->sa_family == AF_INET) {
		dst_port = ntohs(net_sin(addr)->sin_port);
	} else {
		errno = EAFNOSUPPORT;
		return -1;
	}

	/* skip socket connect if UDP */
	if (sock->ip_proto == IPPROTO_UDP) {
		errno = 0;
		return 0;
	}

	ret = modem_context_sprint_ip_addr(addr, ip_str, sizeof(ip_str));
	if (ret != 0) {
		errno = -ret;
		LOG_ERR("Error formatting IP string %d", ret);
		return -1;
	}

	snprintk(buf, sizeof(buf), "AT+USOCO=%d,\"%s\",%d", sock->id,
		ip_str, dst_port);
	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler,
			     NULL, 0U, buf,
			     &mdata.sem_response, MDM_CMD_CONN_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", buf, ret);
		errno = -ret;
		return -1;
	}

	sock->is_connected = true;
	errno = 0;
	return 0;
}

static k_ssize_t offload_recvfrom(void *obj, void *buf, size_t len, int flags,
				     struct sockaddr *from, socklen_t *fromlen)
{
	struct modem_socket *sock = (struct modem_socket *)obj;
	int ret, next_packet_size;
	static const struct modem_cmd cmd[] = {
		MODEM_CMD("+USORF: ", on_cmd_sockreadfrom, 4U, ","),
		MODEM_CMD("+USORD: ", on_cmd_sockread, 2U, ","),
	};
	char sendbuf[sizeof("AT+USORF=#,#####\r")];
	struct socket_read_data sock_data;

	if (!buf || len == 0) {
		errno = EINVAL;
		return -1;
	}

	if (flags & ZSOCK_MSG_PEEK) {
		errno = ENOTSUP;
		return -1;
	}

	next_packet_size = modem_socket_next_packet_size(&mdata.socket_config,
							 sock);
	if (!next_packet_size) {
		if (flags & ZSOCK_MSG_DONTWAIT) {
			errno = EAGAIN;
			return -1;
		}

		if (!sock->is_connected && sock->ip_proto != IPPROTO_UDP) {
			errno = 0;
			return 0;
		}

		modem_socket_wait_data(&mdata.socket_config, sock);
		next_packet_size = modem_socket_next_packet_size(
			&mdata.socket_config, sock);
	}

	/*
	 * Binary and ASCII mode allows sending MDM_MAX_DATA_LENGTH bytes to
	 * the socket in one command
	 */
	if (next_packet_size > MDM_MAX_DATA_LENGTH) {
		next_packet_size = MDM_MAX_DATA_LENGTH;
	}

	snprintk(sendbuf, sizeof(sendbuf), "AT+USO%s=%d,%zd",
		 sock->ip_proto == IPPROTO_UDP ? "RF" : "RD", sock->id,
		 len < next_packet_size ? len : next_packet_size);

	/* socket read settings */
	(void)memset(&sock_data, 0, sizeof(sock_data));
	sock_data.recv_buf = buf;
	sock_data.recv_buf_len = len;
	sock_data.recv_addr = from;
	sock->data = &sock_data;

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler,
			     cmd, ARRAY_SIZE(cmd), sendbuf, &mdata.sem_response,
			     MDM_CMD_TIMEOUT);
	if (ret < 0) {
		errno = -ret;
		ret = -1;
		goto exit;
	}

	/* HACK: use dst address as from */
	if (from && fromlen) {
		*fromlen = sizeof(sock->dst);
		memcpy(from, &sock->dst, *fromlen);
	}

	/* return length of received data */
	errno = 0;
	ret = sock_data.recv_read_len;

exit:
	/* clear socket data */
	sock->data = NULL;
	return ret;
}

static k_ssize_t offload_sendto(void *obj, const void *buf, size_t len, int flags,
				   const struct sockaddr *to, socklen_t tolen)
{
	struct iovec msg_iov = {
		.iov_base = (void *)buf,
		.iov_len = len,
	};
	struct msghdr msg = {
		.msg_iovlen = 1,
		.msg_name = (struct sockaddr *)to,
		.msg_namelen = tolen,
		.msg_iov = &msg_iov,
	};

	int ret = send_socket_data(obj, &msg, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	errno = 0;
	return ret;
}

static int offload_ioctl(void *obj, unsigned int request, va_list args)
{
	switch (request) {
	case ZFD_IOCTL_POLL_PREPARE: {
		struct zsock_pollfd *pfd;
		struct k_poll_event **pev;
		struct k_poll_event *pev_end;

		pfd = va_arg(args, struct zsock_pollfd *);
		pev = va_arg(args, struct k_poll_event **);
		pev_end = va_arg(args, struct k_poll_event *);

		return modem_socket_poll_prepare(&mdata.socket_config, obj, pfd, pev, pev_end);
	}
	case ZFD_IOCTL_POLL_UPDATE: {
		struct zsock_pollfd *pfd;
		struct k_poll_event **pev;

		pfd = va_arg(args, struct zsock_pollfd *);
		pev = va_arg(args, struct k_poll_event **);

		return modem_socket_poll_update(obj, pfd, pev);
	}

	case ZVFS_F_GETFL:
		return 0;

	default:
		errno = EINVAL;
		return -1;
	}
}

static k_ssize_t offload_read(void *obj, void *buffer, size_t count)
{
	return offload_recvfrom(obj, buffer, count, 0, NULL, 0);
}

static k_ssize_t offload_write(void *obj, const void *buffer, size_t count)
{
	return offload_sendto(obj, buffer, count, 0, NULL, 0);
}

static k_ssize_t offload_sendmsg(void *obj, const struct msghdr *msg, int flags)
{
	k_ssize_t sent = 0;
	int bkp_iovec_idx;
	struct iovec bkp_iovec = {0};
	struct msghdr crafted_msg = {
		.msg_name = msg->msg_name,
		.msg_namelen = msg->msg_namelen,
	};
	size_t full_len = 0;
	int ret;

	/* Compute the full length to be send and check for invalid values */
	for (int i = 0; i < msg->msg_iovlen; i++) {
		if (!msg->msg_iov[i].iov_base || msg->msg_iov[i].iov_len == 0) {
			errno = EINVAL;
			return -1;
		}
		full_len += msg->msg_iov[i].iov_len;
	}

	LOG_DBG("msg_iovlen:%zd flags:%d, full_len:%zd",
		msg->msg_iovlen, flags, full_len);

	while (full_len > sent) {
		int removed = 0;
		int i = 0;

		crafted_msg.msg_iovlen = msg->msg_iovlen;
		crafted_msg.msg_iov = &msg->msg_iov[0];

		bkp_iovec_idx = -1;
		/*  Iterate on iovec to remove the bytes already sent */
		while (removed < sent) {
			int to_removed = sent - removed;

			if (to_removed >= msg->msg_iov[i].iov_len) {
				crafted_msg.msg_iovlen -= 1;
				crafted_msg.msg_iov = &msg->msg_iov[i + 1];

				removed += msg->msg_iov[i].iov_len;
			} else {
				/* Backup msg->msg_iov[i] before "removing"
				 * starting bytes already send.
				 */
				bkp_iovec_idx = i;
				bkp_iovec.iov_len = msg->msg_iov[i].iov_len;
				bkp_iovec.iov_base = msg->msg_iov[i].iov_base;

				/* Update msg->msg_iov[i] to "remove"
				 * starting bytes already send.
				 */
				msg->msg_iov[i].iov_len -= to_removed;
				msg->msg_iov[i].iov_base = &(((uint8_t *)msg->msg_iov[i].iov_base)[to_removed]);

				removed += to_removed;
			}

			i++;
		}

		ret = send_socket_data(obj, &crafted_msg, MDM_CMD_TIMEOUT);

		/* Restore backup iovec when necessary */
		if (bkp_iovec_idx != -1) {
			msg->msg_iov[bkp_iovec_idx].iov_len = bkp_iovec.iov_len;
			msg->msg_iov[bkp_iovec_idx].iov_base = bkp_iovec.iov_base;
		}

		/* Handle send_socket_data() returned value */
		if (ret < 0) {
			errno = -ret;
			return -1;
		}

		sent += ret;
	}

	return (k_ssize_t)sent;
}

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
static int map_credentials(struct modem_socket *sock, const void *optval, socklen_t optlen)
{
	sec_tag_t *sec_tags = (sec_tag_t *)optval;
	int ret = 0;
	int tags_len;
	sec_tag_t tag;
	int id;
	int i;
	struct tls_credential *cert;

	if ((optlen % sizeof(sec_tag_t)) != 0 || (optlen == 0)) {
		return -EINVAL;
	}

	tags_len = optlen / sizeof(sec_tag_t);
	/* For each tag, retrieve the credentials value and type: */
	for (i = 0; i < tags_len; i++) {
		tag = sec_tags[i];
		cert = credential_next_get(tag, NULL);
		while (cert != NULL) {
			switch (cert->type) {
			case TLS_CREDENTIAL_CA_CERTIFICATE:
				id = 0;
				break;
			case TLS_CREDENTIAL_NONE:
			case TLS_CREDENTIAL_PSK:
			case TLS_CREDENTIAL_PSK_ID:
			default:
				/* Not handled */
				return -EINVAL;
			}
			struct modem_cmd cmd[] = {
				MODEM_CMD("+USECMNG: ", on_cmd_cert_write, 3U, ","),
			};
			ret = send_cert(sock, cmd, 1, cert->buf, cert->len, id);
			if (ret < 0) {
				return ret;
			}

			cert = credential_next_get(tag, cert);
		}
	}

	return 0;
}
#else
static int map_credentials(struct modem_socket *sock, const void *optval, socklen_t optlen)
{
	return -EINVAL;
}
#endif

static int offload_setsockopt(void *obj, int level, int optname,
			      const void *optval, socklen_t optlen)
{
	struct modem_socket *sock = (struct modem_socket *)obj;

	int ret;

	if (IS_ENABLED(CONFIG_NET_SOCKETS_SOCKOPT_TLS) && level == SOL_TLS) {
		switch (optname) {
		case TLS_SEC_TAG_LIST:
			ret = map_credentials(sock, optval, optlen);
			break;
		case TLS_HOSTNAME:
			LOG_WRN("TLS_HOSTNAME option is not supported");
			return -EINVAL;
		case TLS_PEER_VERIFY:
			if (*(uint32_t *)optval != TLS_PEER_VERIFY_REQUIRED) {
				LOG_WRN("Disabling peer verification is not supported");
				return -EINVAL;
			}
			ret = 0;
			break;
		default:
			return -EINVAL;
		}
	} else {
		return -EINVAL;
	}

	return ret;
}


static const struct socket_op_vtable offload_socket_fd_op_vtable = {
	.fd_vtable = {
		.read = offload_read,
		.write = offload_write,
		.close = offload_close,
		.ioctl = offload_ioctl,
	},
	.bind = offload_bind,
	.connect = offload_connect,
	.sendto = offload_sendto,
	.recvfrom = offload_recvfrom,
	.listen = NULL,
	.accept = NULL,
	.sendmsg = offload_sendmsg,
	.getsockopt = NULL,
	.setsockopt = offload_setsockopt,
};

static bool offload_is_supported(int family, int type, int proto)
{
	if (family != AF_INET &&
	    family != AF_INET6) {
		return false;
	}

	if (type != SOCK_DGRAM &&
	    type != SOCK_STREAM) {
		return false;
	}

	if (proto != IPPROTO_TCP &&
	    proto != IPPROTO_UDP &&
	    proto != IPPROTO_TLS_1_2) {
		return false;
	}

	return true;
}

NET_SOCKET_OFFLOAD_REGISTER(ublox_sara_r4, CONFIG_NET_SOCKETS_OFFLOAD_PRIORITY,
			    AF_UNSPEC, offload_is_supported, offload_socket);

#if defined(CONFIG_DNS_RESOLVER)
/* TODO: This is a bare-bones implementation of DNS handling
 * We ignore most of the hints like ai_family, ai_protocol and ai_socktype.
 * Later, we can add additional handling if it makes sense.
 */
static int offload_getaddrinfo(const char *node, const char *service,
			       const struct zsock_addrinfo *hints,
			       struct zsock_addrinfo **res)
{
	static const struct modem_cmd cmd =
		MODEM_CMD("+UDNSRN: ", on_cmd_dns, 1U, ",");
	uint32_t port = 0U;
	int ret;
	/* DNS command + 128 bytes for domain name parameter */
	char sendbuf[sizeof("AT+UDNSRN=#,'[]'\r") + 128];

	/* init result */
	(void)memset(&result, 0, sizeof(result));
	(void)memset(&result_addr, 0, sizeof(result_addr));
	/* FIXME: Hard-code DNS to return only IPv4 */
	result.ai_family = AF_INET;
	result_addr.sa_family = AF_INET;
	result.ai_addr = &result_addr;
	result.ai_addrlen = sizeof(result_addr);
	result.ai_canonname = result_canonname;
	result_canonname[0] = '\0';

	if (service) {
		port = ATOI(service, 0U, "port");
		if (port < 1 || port > USHRT_MAX) {
			return DNS_EAI_SERVICE;
		}
	}

	if (port > 0U) {
		/* FIXME: DNS is hard-coded to return only IPv4 */
		if (result.ai_family == AF_INET) {
			net_sin(&result_addr)->sin_port = htons(port);
		}
	}

	/* check to see if node is an IP address */
	if (net_addr_pton(result.ai_family, node,
			  &((struct sockaddr_in *)&result_addr)->sin_addr)
	    == 0) {
		*res = &result;
		return 0;
	}

	/* user flagged node as numeric host, but we failed net_addr_pton */
	if (hints && hints->ai_flags & AI_NUMERICHOST) {
		return DNS_EAI_NONAME;
	}

	snprintk(sendbuf, sizeof(sendbuf), "AT+UDNSRN=0,\"%s\"", node);
	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler,
			     &cmd, 1U, sendbuf, &mdata.sem_response,
			     MDM_DNS_TIMEOUT);
	if (ret < 0) {
		return ret;
	}

	LOG_DBG("DNS RESULT: %s",
		net_addr_ntop(result.ai_family,
					 &net_sin(&result_addr)->sin_addr,
					 sendbuf, NET_IPV4_ADDR_LEN));

	*res = (struct zsock_addrinfo *)&result;
	return 0;
}

static void offload_freeaddrinfo(struct zsock_addrinfo *res)
{
	/* using static result from offload_getaddrinfo() -- no need to free */
	ARG_UNUSED(res);
}

static const struct socket_dns_offload offload_dns_ops = {
	.getaddrinfo = offload_getaddrinfo,
	.freeaddrinfo = offload_freeaddrinfo,
};
#endif

static int net_offload_dummy_get(sa_family_t family,
				 enum net_sock_type type,
				 enum net_ip_protocol ip_proto,
				 struct net_context **context)
{

	LOG_ERR("CONFIG_NET_SOCKETS_OFFLOAD must be enabled for this driver");

	return -ENOTSUP;
}

/* placeholders, until Zephyr IP stack updated to handle a NULL net_offload */
static struct net_offload modem_net_offload = {
	.get = net_offload_dummy_get,
};

#define HASH_MULTIPLIER		37
static uint32_t hash32(char *str, int len)
{
	uint32_t h = 0;
	int i;

	for (i = 0; i < len; ++i) {
		h = (h * HASH_MULTIPLIER) + str[i];
	}

	return h;
}

static inline uint8_t *modem_get_mac(const struct device *dev)
{
	struct modem_data *data = dev->data;
	uint32_t hash_value;

	data->mac_addr[0] = 0x00;
	data->mac_addr[1] = 0x10;

	/* use IMEI for mac_addr */
	hash_value = hash32(mdata.mdm_imei, strlen(mdata.mdm_imei));

	UNALIGNED_PUT(hash_value, (uint32_t *)(data->mac_addr + 2));

	return data->mac_addr;
}

static int offload_socket(int family, int type, int proto);

static void modem_net_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct modem_data *data = dev->data;

	/* Direct socket offload used instead of net offload: */
	iface->if_dev->offload = &modem_net_offload;
	net_if_set_link_addr(iface, modem_get_mac(dev),
			     sizeof(data->mac_addr),
			     NET_LINK_ETHERNET);
	data->net_iface = iface;
#ifdef CONFIG_DNS_RESOLVER
	socket_offload_dns_register(&offload_dns_ops);
#endif

	net_if_socket_offload_set(iface, offload_socket);
}

static struct offloaded_if_api api_funcs = {
	.iface_api.init = modem_net_iface_init,
};

static const struct modem_cmd response_cmds[] = {
	MODEM_CMD("OK", on_cmd_ok, 0U, ""), /* 3GPP */
	MODEM_CMD("ERROR", on_cmd_error, 0U, ""), /* 3GPP */
	MODEM_CMD("+CME ERROR: ", on_cmd_exterror, 1U, ""),
	MODEM_CMD_DIRECT("@", on_prompt),
};

static const struct modem_cmd unsol_cmds[] = {
	MODEM_CMD("+UUSOCL: ", on_cmd_socknotifyclose, 1U, ""),
	MODEM_CMD("+UUSORD: ", on_cmd_socknotifydata, 2U, ","),
	MODEM_CMD("+UUSORF: ", on_cmd_socknotifydata, 2U, ","),
	MODEM_CMD("+CREG: ", on_cmd_socknotifycreg, 1U, ""),
};

static int modem_init(const struct device *dev)
{
	int ret = 0;

	ARG_UNUSED(dev);

	k_sem_init(&mdata.sem_response, 0, 1);
	k_sem_init(&mdata.sem_prompt, 0, 1);

#if defined(CONFIG_MODEM_UBLOX_SARA_RSSI_WORK)
	/* initialize the work queue */
	k_work_queue_start(&modem_workq, modem_workq_stack,
			   K_KERNEL_STACK_SIZEOF(modem_workq_stack),
			   K_PRIO_COOP(7), NULL);
#endif

	/* socket config */
	ret = modem_socket_init(&mdata.socket_config, &mdata.sockets[0], ARRAY_SIZE(mdata.sockets),
				MDM_BASE_SOCKET_NUM, false, &offload_socket_fd_op_vtable);
	if (ret < 0) {
		goto error;
	}

	/* cmd handler */
	const struct modem_cmd_handler_config cmd_handler_config = {
		.match_buf = &mdata.cmd_match_buf[0],
		.match_buf_len = sizeof(mdata.cmd_match_buf),
		.buf_pool = &mdm_recv_pool,
		.alloc_timeout = K_NO_WAIT,
		.eol = "\r",
		.user_data = NULL,
		.response_cmds = response_cmds,
		.response_cmds_len = ARRAY_SIZE(response_cmds),
		.unsol_cmds = unsol_cmds,
		.unsol_cmds_len = ARRAY_SIZE(unsol_cmds),
	};

	ret = modem_cmd_handler_init(&mctx.cmd_handler, &mdata.cmd_handler_data,
				     &cmd_handler_config);
	if (ret < 0) {
		goto error;
	}

	/* modem interface */
	const struct modem_iface_uart_config uart_config = {
		.rx_rb_buf = &mdata.iface_rb_buf[0],
		.rx_rb_buf_len = sizeof(mdata.iface_rb_buf),
		.dev = MDM_UART_DEV,
		.hw_flow_control = DT_PROP(MDM_UART_NODE, hw_flow_control),
	};

	ret = modem_iface_uart_init(&mctx.iface, &mdata.iface_data, &uart_config);
	if (ret < 0) {
		goto error;
	}

	/* modem data storage */
	mctx.data_manufacturer = mdata.mdm_manufacturer;
	mctx.data_model = mdata.mdm_model;
	mctx.data_revision = mdata.mdm_revision;
	mctx.data_imei = mdata.mdm_imei;
	mctx.data_rssi = &mdata.mdm_rssi;

	/* pin setup */
	ret = gpio_pin_configure_dt(&power_gpio, GPIO_OUTPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure %s pin", "power");
		goto error;
	}

#if DT_INST_NODE_HAS_PROP(0, mdm_reset_gpios)
	ret = gpio_pin_configure_dt(&reset_gpio, GPIO_OUTPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure %s pin", "reset");
		goto error;
	}
#endif

#if DT_INST_NODE_HAS_PROP(0, mdm_vint_gpios)
	ret = gpio_pin_configure_dt(&vint_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure %s pin", "vint");
		goto error;
	}
#endif

	mctx.driver_data = &mdata;

	ret = modem_context_register(&mctx);
	if (ret < 0) {
		LOG_ERR("Error registering modem context: %d", ret);
		goto error;
	}

	/* start RX thread */
	k_thread_create(&modem_rx_thread, modem_rx_stack,
			K_KERNEL_STACK_SIZEOF(modem_rx_stack),
			modem_rx,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);

#if defined(CONFIG_MODEM_UBLOX_SARA_RSSI_WORK)
	/* init RSSI query */
	k_work_init_delayable(&mdata.rssi_query_work, modem_rssi_query_work);
#endif

	modem_reset();

error:
	return ret;
}

NET_DEVICE_DT_INST_OFFLOAD_DEFINE(0, modem_init, NULL,
				  &mdata, NULL,
				  CONFIG_MODEM_UBLOX_SARA_R4_INIT_PRIORITY,
				  &api_funcs,
				  MDM_MAX_DATA_LENGTH);

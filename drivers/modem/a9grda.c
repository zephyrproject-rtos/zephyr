/*
 * Copyright (c) 2019 ZedBlox Logitech
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT rda_a9g

#include <logging/log.h>
LOG_MODULE_REGISTER(modem_a9grda, CONFIG_MODEM_LOG_LEVEL);

#include <drivers/modem/a9grda.h>
#include <drivers/gpio.h>
#include <init.h>

#include "modem_context.h"
#include "modem_socket.h"
#include "modem_cmd_handler.h"
#include "modem_iface_uart.h"

/* pin settings */
enum mdm_control_pins {
	MDM_POWER = 0,
	MDM_RESET,
};

static struct modem_pin modem_pins[] = {
	/* MDM_POWER */
	MODEM_PIN(DT_INST_GPIO_LABEL(0, mdm_power_gpios),
		  DT_INST_GPIO_PIN(0, mdm_power_gpios),
		  DT_INST_GPIO_FLAGS(0, mdm_power_gpios) | GPIO_OUTPUT),

	/* MDM_RESET */
	MODEM_PIN(DT_INST_GPIO_LABEL(0, mdm_reset_gpios),
		  DT_INST_GPIO_PIN(0, mdm_reset_gpios),
		  DT_INST_GPIO_FLAGS(0, mdm_reset_gpios) | GPIO_OUTPUT),

#if DT_INST_NODE_HAS_PROP(0, mdm_vint_gpios)
	/* MDM_VINT */
	MODEM_PIN(DT_INST_GPIO_LABEL(0, mdm_vint_gpios),
		  DT_INST_GPIO_PIN(0, mdm_vint_gpios),
		  DT_INST_GPIO_FLAGS(0, mdm_vint_gpios) | GPIO_INPUT),
#endif
};

#define MDM_UART_DEV_NAME DT_INST_BUS_LABEL(0)

#define MDM_POWER_ENABLE 1
#define MDM_POWER_DISABLE 0
#define MDM_RESET_NOT_ASSERTED 0
#define MDM_RESET_ASSERTED 1

#define MDM_CMD_TIMEOUT K_SECONDS(20)
#define MDM_REGISTRATION_TIMEOUT K_SECONDS(180)
#define MDM_PROMPT_CMD_DELAY K_MSEC(75)
#define MDM_LOCK_TIMEOUT K_SECONDS(1)

#define MDM_MAX_DATA_LENGTH 2048
#define MDM_RECV_MAX_BUF 30
#define MDM_RECV_BUF_SIZE 256

#define MDM_MAX_SOCKETS 6
#define MDM_BASE_SOCKET_NUM 0

#define MDM_NETWORK_RETRY_COUNT 3
#define MDM_WAIT_FOR_RSSI_COUNT 10
#define MDM_WAIT_FOR_RSSI_DELAY K_SECONDS(2)

#define BUF_ALLOC_TIMEOUT K_SECONDS(1)

#define MDM_CD_LEN 16
#define MDM_GPS_DATA_LEN 256

#define RSSI_TIMEOUT_SECS 30
#define MDM_SOCKET_MAX_BUF_SIZE 4096U

RING_BUF_ITEM_DECLARE_SIZE(mdm_socket_ring_buf_0, MDM_SOCKET_MAX_BUF_SIZE);

NET_BUF_POOL_DEFINE(mdm_recv_pool, MDM_RECV_MAX_BUF, MDM_RECV_BUF_SIZE, 0,
		    NULL);

/* RX thread structures */
K_THREAD_STACK_DEFINE(modem_rx_stack, CONFIG_MODEM_A9GRDA_RX_STACK_SIZE);
struct k_thread modem_rx_thread;

/* RX thread work queue */
K_THREAD_STACK_DEFINE(modem_workq_stack,
		      CONFIG_MODEM_A9GRDA_RX_WORKQ_STACK_SIZE);
static struct k_work_q modem_workq;

struct http_config {
	u8_t http_pending;
	u16_t http_id;
	size_t http_resp_len;
};

struct recv_data_config {
	char *recv_buf;
	size_t recv_buf_len;
	size_t expected_len;
	size_t recv_read_len;
	union {
		/* http config */
		struct http_config http_cfg;
	};
};

/* driver data */
struct modem_data {
	struct net_if *net_iface;
	u8_t mac_addr[6];

	/* modem interface */
	struct modem_iface_uart_data iface_data;
	u8_t iface_isr_buf[MDM_RECV_BUF_SIZE];
	u8_t iface_rb_buf[MDM_MAX_DATA_LENGTH];

	/* modem cmds */
	struct modem_cmd_handler_data cmd_handler_data;
	u8_t cmd_read_buf[MDM_RECV_BUF_SIZE];
	u8_t cmd_match_buf[MDM_RECV_BUF_SIZE + 1];

	/* socket data */
	struct modem_socket_config socket_config;
	struct modem_socket sockets[MDM_MAX_SOCKETS];

	/* FIXME Host name. Move to struct modem_socket? */
	char hst_name[CONFIG_MODEM_SOCKET_MAX_HST_LEN];

	/* command buffer send */
	char send_buf[CONFIG_MODEM_MAXIMUM_TR_RC_SIZE];

	/* RSSI work */
	struct k_delayed_work rssi_query_work;

	/* modem data */
	char mdm_manufacturer[MDM_MANUFACTURER_LENGTH];
	char mdm_model[MDM_MODEL_LENGTH];
	char mdm_revision[MDM_REVISION_LENGTH];
	char mdm_imei[MDM_IMEI_LENGTH];
	char mdm_timeval[MDM_TIME_LENGTH];

	/* modem state */
	int ev_creg;

	/* response semaphore */
	struct k_sem sem_response;

	/* connect semaphore */
	struct k_sem sem_connect;

	/* lock semaphore */
	struct k_sem mdm_lock;

	/* Config to receive more data */
	struct recv_data_config recv_cfg;

    /* Reference location */
    int agps_status;
    char gps_data[MDM_GPS_DATA_LEN];
    char ref_lat[MDM_CD_LEN];
    char ref_lon[MDM_CD_LEN];
    /*
	char lat_cur[MDM_CD_LEN];
	char lon_cur[MDM_CD_LEN];
    */

    char time_data[MDM_TIME_LENGTH];
};

enum CONNECT_STATUS {
	MDM_CONNECT_SUCCESS = 0,
	MDM_CONNECT_FAIL,
};

static struct modem_data mdata;
static struct modem_context mctx;
static u8_t connect_status = MDM_CONNECT_FAIL;

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
static int modem_atoi(const char *s, const int err_value, const char *desc,
		      const char *func)
{
	int ret;
	char *endptr;

	ret = (int)strtol(s, &endptr, 10);
	if (!endptr || *endptr != '\0') {
		LOG_ERR("bad %s '%s' in %s", log_strdup(s), log_strdup(desc),
			log_strdup(func));
		return err_value;
	}

	return ret;
}

#if 0
/* convert a hex-encoded buffer back into a binary buffer */
static int hex_to_binary(struct modem_cmd_handler_data *data,
			 u16_t data_length,
			 u8_t *bin_buf, u16_t bin_buf_len)
{
	int i;
	u8_t c = 0U, c2;

	/* make sure we have room for a NUL char at the end of bin_buf */
	if (data_length > bin_buf_len - 1) {
		return -ENOMEM;
	}

	for (i = 0; i < data_length * 2; i++) {
		if (!data->rx_buf) {
			return -ENOMEM;
		}

		c2 = *data->rx_buf->data;
		if (isdigit(c2)) {
			c += c2 - '0';
		} else if (isalpha(c2)) {
			c += c2 - (isupper(c2) ? 'A' - 10 : 'a' - 10);
		} else {
			return -EINVAL;
		}

		if (i % 2) {
			bin_buf[i / 2] = c;
			c = 0U;
		} else {
			c = c << 4;
		}

		/* pull data from buf and advance to the next frag if needed */
		net_buf_pull_u8(data->rx_buf);
		if (!data->rx_buf->len) {
			data->rx_buf = net_buf_frag_del(NULL, data->rx_buf);
		}
	}

	/* end with a NUL char */
	bin_buf[i / 2] = '\0';
	return 0;
}
#endif

/*
 * Modem Response Command Handlers
 */

/* Handler: OK */
MODEM_CMD_DEFINE(on_cmd_ok)
{
	LOG_DBG("%s", __func__);
	modem_cmd_handler_set_error(data, 0);
	k_sem_give(&mdata.sem_response);
    return 0;
}

/* Handler: CONNECT OK */
MODEM_CMD_DEFINE(on_cmd_connect_ok)
{
	LOG_DBG("%s", __func__);
	modem_cmd_handler_set_error(data, 0);
	connect_status = MDM_CONNECT_SUCCESS;
	k_sem_give(&mdata.sem_connect);
	/* CONNECT OK comes before/after OK */
    return 0;
}

/* Handler: CONNECT FAIL */
MODEM_CMD_DEFINE(on_cmd_connect_fail)
{
	LOG_DBG("%s", __func__);
	modem_cmd_handler_set_error(data, -EIO);
	connect_status = MDM_CONNECT_FAIL;
	k_sem_give(&mdata.sem_connect);
	/* CONNECT FAIL comes before/after OK */
    return 0;
}

/* Handler: SHUT OK */
MODEM_CMD_DEFINE(on_cmd_shut_ok)
{
	LOG_DBG("%s", __func__);
	modem_cmd_handler_set_error(data, 0);
	/* SHUT OK comes before OK */
    return 0;
}

/* Handler: ERROR */
MODEM_CMD_DEFINE(on_cmd_error)
{
	LOG_DBG("%s", __func__);
	modem_cmd_handler_set_error(data, -EIO);
	k_sem_give(&mdata.sem_response);
    return 0;
}

/* Handler: +CME Error: <err>[0] */
MODEM_CMD_DEFINE(on_cmd_exterror)
{
	LOG_DBG("%s err %d", __func__, *argv[0]);
	/* TODO: map extended error codes to values */
	modem_cmd_handler_set_error(data, -EIO);
	k_sem_give(&mdata.sem_response);
    return 0;
}

/*
 * GNSS response command handlers
 */

/* Handler: +AGPS: <err> */
MODEM_CMD_DEFINE(on_cmd_gps_agps)
{
	int agps;

	agps = ATOI(argv[0], 0, "agps");
	LOG_DBG("agps: %d", agps);

    mdata.agps_status = agps;

    if (agps == 0)
    {
        mdata.agps_status = 0;

        modem_cmd_handler_set_error(data, 0);
        connect_status = MDM_CONNECT_SUCCESS;
        k_sem_give(&mdata.sem_connect);
    }
    else
    {
        modem_cmd_handler_set_error(data, -EIO);
        connect_status = MDM_CONNECT_FAIL;
        k_sem_give(&mdata.sem_connect);
    }
    return 0;
}

/* Handler: +GETREFLOC: <lat>,<long> */
MODEM_CMD_DEFINE(on_cmd_gps_getrefloc)
{
    int lat,lon;

	lat = ATOI(argv[0], 0, "lat");
    lon = ATOI(argv[1], 0, "lon");

	LOG_DBG("lat: %d, lon: %d", lat, lon);

    if (lat == -1 || lon == -1)
    {
        modem_cmd_handler_set_error(data, -EIO);
        connect_status = MDM_CONNECT_FAIL;
        k_sem_give(&mdata.sem_connect);
    }
    else
    {
        /* FIXME make lat and lon into double */
        strcpy(mdata.ref_lat, argv[0]);
        strcpy(mdata.ref_lon, argv[1]);

        modem_cmd_handler_set_error(data, 0);
        connect_status = MDM_CONNECT_SUCCESS;
        k_sem_give(&mdata.sem_connect);
    }
    return 0;
}

/* Handler: %GNGGA, ... */
MODEM_CMD_DEFINE(on_cmd_gps_read)
{
#if 0
    if (!data->rx_buf)
    {
        LOG_DBG("no rx");
    }
    else
    {
        LOG_DBG("rx len: %d", len);
    }

    LOG_DBG("%s***, %s***, %s***, %s***, %s***, %s***!", argv[0], argv[1],
            argv[2], argv[3], argv[4], argv[5]);
#endif
	size_t out_len;

	out_len = net_buf_linearize(mdata.gps_data,
				    sizeof(mdata.gps_data) - 1,
				    data->rx_buf, 0, len);
	mdata.gps_data[out_len] = '\0';

    LOG_INF("GPS data: %s", log_strdup(mdata.gps_data));
	k_sem_give(&mdata.sem_response);
    return 0;
}

/* Handler: +CCLK: "..." */
MODEM_CMD_DEFINE(on_cmd_gettime)
{
	size_t out_len;

	out_len = strlen(argv[0]);

    if (argv[0][0] != '\"')
    {
        LOG_ERR("Time format +CCLK wrong %s, %c", argv[0], argv[0][0]);
        return -1;
    }

    /* argv[0] + 1 to ensure not to cpy preceding "
     * out_len -1 to ensure not to cpy trailing " */
	memcpy(mdata.mdm_timeval, argv[0] + 1, out_len - 1);
	mdata.mdm_timeval[out_len] = '\0';
	mctx.data_sys_timeval = k_uptime_get_32();

	memcpy(mdata.time_data, argv[0] + 1, out_len - 1);
	mdata.time_data[out_len] = '\0';

    LOG_INF("TIME CCLK: %s", log_strdup(mdata.time_data));
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
	LOG_INF("Manufacturer: %s", log_strdup(mdata.mdm_manufacturer));
    return 0;
}

/* Handler: <model> */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_model)
{
	size_t out_len;

	out_len =
		net_buf_linearize(mdata.mdm_model, sizeof(mdata.mdm_model) - 1,
				  data->rx_buf, 0, len);
	mdata.mdm_model[out_len] = '\0';
	LOG_INF("Model: %s", log_strdup(mdata.mdm_model));
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
	LOG_INF("Revision: %s", log_strdup(mdata.mdm_revision));
    return 0;
}

/* Handler: +EGMR:<IMEI> */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_imei)
{
	size_t out_len;

	out_len = strlen(argv[0]);
	memcpy(mdata.mdm_imei, argv[0], out_len);
	mdata.mdm_imei[out_len] = '\0';
	LOG_INF("IMEI: %s", log_strdup(mdata.mdm_imei));
    return 0;
}

/* Handler: +CTZV:<Time> */
MODEM_CMD_DEFINE(on_cmd_timezoneval)
{
	size_t out_len;

	out_len = strlen(argv[0]);
	memcpy(mdata.mdm_timeval, argv[0], out_len);
	mdata.mdm_timeval[out_len] = '\0';
	mctx.data_sys_timeval = k_uptime_get_32();
	LOG_INF("TIME: %s, %u", log_strdup(mdata.mdm_timeval), mctx.data_sys_timeval);
    return 0;
}

/* Handler: +CSQ: <signal_power>[0],<qual>[1] */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_rssi_csq)
{
	int rssi;

	rssi = ATOI(argv[0], 0, "qual");
	LOG_INF("rssi: %d", rssi);
	if (rssi == 31) {
		mctx.data_rssi = -46;
	} else if (rssi >= 0 && rssi <= 31) {
		/* FIXME: This value depends on the RAT */
		mctx.data_rssi = -110 + ((rssi * 2) + 1);
	} else {
		mctx.data_rssi = -1000;
	}

	LOG_INF("QUAL: %d", mctx.data_rssi);
    return 0;
}

#if 0
/*
 * Modem Socket Command Handlers
 */

/* Common code for +CIPRECV: <binary_data> */
static void on_cmd_sockread_common(int socket_id,
				   struct modem_cmd_handler_data *data,
				   int socket_data_length, u16_t len)
{
	struct modem_socket *sock = NULL;
	uint8_t *sock_data;
	uint32_t rb_size = 0;
	int ret;

	if (!len) {
		LOG_ERR("No TCP data received,  Aborting!");
		return;
	}

	/* zero length */
	if (socket_data_length <= 0) {
		LOG_ERR("Length problem (%d).  Aborting!", socket_data_length);
		return;
	}

	/* check that we have enough data */
	if (!data->rx_buf) {
		LOG_ERR("Incorrect format! Ignoring data!");
		return;
	}

	sock = modem_socket_from_id(&mdata.socket_config, socket_id);
	if (!sock) {
		LOG_ERR("Socket not found! (%d)", socket_id);
		return;
	}

	rb_size = ring_buf_put_claim(sock->socket_buf, &sock_data, len);
	if (rb_size != len) {
		LOG_ERR("socket ring buf claim failed! (%d), ret sz: %d",
			socket_id, rb_size);
		return;
	}

	ret = net_buf_linearize(
		sock_data, MDM_SOCKET_MAX_BUF_SIZE - socket_data_length + len,
		data->rx_buf, 0, len);
	if (ret < 0) {
		LOG_ERR("socket data copy failed! %d", ret);
		return;
	} else {
		if (ring_buf_put_finish(sock->socket_buf, ret) != 0) {
			LOG_ERR("socket no memory!");
			return;
		}
	}

	/* unblock sockets waiting on recv() */
	k_sem_give(&sock->sem_data_ready);
	if (sock->is_polled) {
		/* unblock poll() */
		k_sem_give(&mdata.socket_config.sem_poll);
	}

	/* don't give back semaphore -- unsol cmd */
}
#endif

static size_t on_cmd_http_common(void *data_v, u16_t len)
{
	struct modem_cmd_handler_data *data =
		(struct modem_cmd_handler_data *)data_v;
	size_t ret = 0;
	size_t skipped = 0;
	u16_t cp_len = 0;

	/*
	 * make sure we still have buf data and next char in the buffer is a
	 * quote.
	 */
	if (mdata.recv_cfg.recv_read_len == 0) {
		if (!data->rx_buf || *data->rx_buf->data != '\"') {
			LOG_ERR("Incorrect format! Ignoring data! Missing \"");
			goto exit;
		}

		/* skip quote */
		len--;
		net_buf_pull_u8(data->rx_buf);
		if (!data->rx_buf->len) {
			data->rx_buf = net_buf_frag_del(NULL, data->rx_buf);
		}
		skipped += 1;
	}

	/* length to be copied */
	cp_len = MIN(mdata.recv_cfg.expected_len - mdata.recv_cfg.recv_read_len,
		     len);

	ret = net_buf_linearize(
		mdata.recv_cfg.recv_buf + mdata.recv_cfg.recv_read_len,
		mdata.recv_cfg.recv_buf_len - mdata.recv_cfg.recv_read_len,
		data->rx_buf, 0, cp_len);
	if (ret < 0) {
		LOG_ERR("Http response copy failed! %d", ret);
		goto exit;
	} else {
		mdata.recv_cfg.recv_read_len += (u16_t)ret;
		if (mdata.recv_cfg.recv_buf_len <=
		    mdata.recv_cfg.recv_read_len) {
			LOG_INF("recv_buf_len less than recv_read_len");
			goto exit;
		}
	}

	if (mdata.recv_cfg.expected_len != mdata.recv_cfg.recv_read_len) {
		return ret;
	} else {
		char rcv;
		ret = net_buf_linearize(&rcv, 1, data->rx_buf, cp_len, 1);
		if (rcv != '\"') {
			LOG_ERR("Incorrect format! Missing \"");
			goto exit;
		}
	}

exit:
	mdata.recv_cfg.http_cfg.http_pending = 0;
	k_sem_give(&mdata.sem_connect);

	/* clear callback for more data */
	mdata.cmd_handler_data.process_data = NULL;

	return ret;
}

#if 0
/*
 * MODEM UNSOLICITED NOTIFICATION HANDLERS
 */

/* Handler: CLOSED */
MODEM_CMD_DEFINE(on_cmd_socknotifyclose)
{
	struct modem_socket *sock;

	/* FIXME Check what to do for multi sockets */
	sock = modem_socket_from_id(&mdata.socket_config, 0);
	if (!sock) {
		return;
	}

	modem_socket_put(&mdata.socket_config, sock->sock_fd);
}

/* Handler: +CIPRECV: <length>,<data> */
MODEM_CMD_DEFINE(on_cmd_socknotifydata)
{
	int ret, socket_id, new_total;
	int i;
	struct modem_socket *sock;
	u16_t total = 0U;

	socket_id = 0; //ATOI(argv[0], 0, "socket_id");
	new_total = ATOI(argv[0], 0, "length");
	if (len != new_total) {
		LOG_ERR("socket_id:%d len_parsed:%d len_recv: %d", socket_id,
			len, new_total);
	}

	sock = modem_socket_from_id(&mdata.socket_config, socket_id);
	if (!sock) {
		return;
	}

	for (i = 0; i < sock->packet_count; i++) {
		total += sock->packet_sizes[i];
	}

	new_total += total;
	if (new_total > MDM_SOCKET_MAX_BUF_SIZE) {
		LOG_ERR("Socket data packet drop. Not enough memory for %d! (%d)",
			new_total, socket_id);
		len -= (new_total - MDM_SOCKET_MAX_BUF_SIZE);
		new_total = MDM_SOCKET_MAX_BUF_SIZE;
	}

	ret = modem_socket_packet_size_update(&mdata.socket_config, sock,
					      new_total);
	if (ret < 0) {
		LOG_ERR("socket_id:%d left_bytes:%d err: %d", socket_id,
			new_total, ret);
	}

	/* read data */
	on_cmd_sockread_common(socket_id, data, new_total, len);
}
#endif

/* Handler: +HTTP: <method>,<length>,"data" */
MODEM_CMD_DEFINE(on_cmd_http_response)
{
	u16_t http_resp_len = (u16_t)ATOI(argv[1], 0, "length");

	LOG_INF("rcv %u.", len);
	LOG_INF("HTTP m:%d len:%d", ATOI(argv[0], 0, "method_id"),
		http_resp_len);

	if (len < http_resp_len + 2) {
		LOG_INF("Short http value %u. Request more!", len);

		/* Register callback for more data */
		mdata.cmd_handler_data.process_data = on_cmd_http_common;
	}

	/* 
     * let the process command know that more data is required 
     * need more will be 2 + resp_len
     */
	mdata.recv_cfg.http_cfg.http_pending = 1;
	mdata.recv_cfg.http_cfg.http_id = ATOI(argv[0], 0, "method_id");
	mdata.recv_cfg.http_cfg.http_resp_len = http_resp_len;
	mdata.recv_cfg.expected_len = http_resp_len;

	if (on_cmd_http_common(data, len) == 0) {
        return -EAGAIN;
	}
    return 0;
}

/* Handler: +CREG: <stat>[0] */
MODEM_CMD_DEFINE(on_cmd_socknotifycreg)
{
	mdata.ev_creg = ATOI(argv[0], 0, "stat");
	LOG_INF("CREG:%d", mdata.ev_creg);
    return 0;
}

/* RX thread */
static void modem_rx(void)
{
	while (true) {
		/* wait for incoming data */
		k_sem_take(&mdata.iface_data.rx_sem, K_FOREVER);

		mctx.cmd_handler.process(&mctx.cmd_handler, &mctx.iface);

		/* give up time if we have a solid stream of data */
		k_yield();
	}
}

static int pin_init(void)
{
	LOG_DBG("Setting Modem Pins");

	LOG_DBG("MDM_RESET_PIN -> NOT_ASSERTED");
	modem_pin_write(&mctx, MDM_RESET, MDM_RESET_NOT_ASSERTED);

	LOG_DBG("MDM_POWER_PIN -> ENABLE");
	modem_pin_write(&mctx, MDM_POWER, MDM_POWER_ENABLE);
	k_sleep(K_SECONDS(2));

	LOG_DBG("MDM_POWER_PIN -> DISABLE");
	modem_pin_write(&mctx, MDM_POWER, MDM_POWER_DISABLE);
	k_sleep(K_SECONDS(2));
	LOG_DBG("MDM_POWER_PIN -> ENABLE");
	modem_pin_write(&mctx, MDM_POWER, MDM_POWER_ENABLE);
	k_sleep(K_SECONDS(1));

	/* make sure module is powered off */
#if defined(DT_RDA_A9G_0_MDM_VINT_GPIOS_CONTROLLER)
	LOG_DBG("Waiting for MDM_VINT_PIN = 0");

	do {
		k_sleep(K_MSEC(100));
	} while (modem_pin_read(&mctx, MDM_VINT) != MDM_VINT_DISABLE);
#else
	k_sleep(K_SECONDS(1));
#endif

	LOG_DBG("MDM_POWER_PIN -> DISABLE");

	unsigned int irq_lock_key = irq_lock();

	modem_pin_write(&mctx, MDM_POWER, MDM_POWER_DISABLE);
	k_sleep(K_SECONDS(1));
	modem_pin_write(&mctx, MDM_POWER, MDM_POWER_ENABLE);

	irq_unlock(irq_lock_key);

	LOG_DBG("MDM_POWER_PIN -> ENABLE");

#if defined(DT_RDA_A9G_0_MDM_VINT_GPIOS_CONTROLLER)
	LOG_DBG("Waiting for MDM_VINT_PIN = 1");
	do {
		k_sleep(K_MSEC(100));
	} while (modem_pin_read(&mctx, MDM_VINT) != MDM_VINT_ENABLE);
#else
	k_sleep(K_SECONDS(1));
#endif

	modem_pin_config(&mctx, MDM_POWER, false);

	LOG_DBG("... Done!");

	return 0;
}

static void modem_rssi_query_work(struct k_work *work)
{
	struct modem_cmd cmd =
		MODEM_CMD("+CSQ: ", on_cmd_atcmdinfo_rssi_csq, 2U, ",");
	static char *send_cmd = "AT+CSQ";
	int ret;

    k_sem_take(&mdata.mdm_lock, K_FOREVER);

	/* query modem RSSI */
	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, &cmd, 1U, send_cmd,
			     &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("AT+C[E]SQ ret:%d", ret);
	}

    k_sem_give(&mdata.mdm_lock);

	/* re-start RSSI query work */
	if (work) {
		k_delayed_work_submit_to_queue(&modem_workq,
					       &mdata.rssi_query_work,
					       K_SECONDS(RSSI_TIMEOUT_SECS));
	}
}

static void modem_reset(void)
{
	int ret = 0, retry_count = 0, counter = 0;
	static struct setup_cmd setup_cmds[] = {
		/* turn off echo */
		SETUP_CMD_NOHANDLE("ATE0"),
		/* stop functionality */
		SETUP_CMD_NOHANDLE("AT+CFUN=0"),
		/* extended error numbers */
		SETUP_CMD_NOHANDLE("AT+CMEE=1"),
		/* UNC messages for registration */
		SETUP_CMD_NOHANDLE("AT+CREG=1"),
		/* HEX receive data mode */
		//SETUP_CMD_NOHANDLE("AT+UDCONF=1,1"),
		/* query modem info */
		SETUP_CMD("AT+CGMI", "", on_cmd_atcmdinfo_manufacturer, 0U, ""),
		SETUP_CMD("AT+CGMM", "", on_cmd_atcmdinfo_model, 0U, ""),
		SETUP_CMD("AT+CGMR", "", on_cmd_atcmdinfo_revision, 0U, ""),
		SETUP_CMD("AT+EGMR=2,7", "+EGMR:", on_cmd_atcmdinfo_imei, 1U,
			  ""),
		/* setup PDP context definition */
		SETUP_CMD_NOHANDLE(
			"AT+CGDCONT=1,\"IP\",\"" CONFIG_MODEM_A9GRDA_APN "\""),
		/* start functionality */
		SETUP_CMD_NOHANDLE("AT+CFUN=1"),
	};

	/* bring down network interface */
	atomic_clear_bit(mdata.net_iface->if_dev->flags, NET_IF_UP);

restart:
	/* stop RSSI delay work */
	k_delayed_work_cancel(&mdata.rssi_query_work);

	pin_init();

	LOG_INF("Waiting for modem to respond");

	/* Give the modem a while to start responding to simple 'AT' commands.
	 */
	ret = -1;

    k_sem_take(&mdata.mdm_lock, K_FOREVER);

	while (counter++ < 50 && ret < 0) {
		k_sleep(K_SECONDS(2));
		ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0,
				     "AT", &mdata.sem_response,
				     MDM_CMD_TIMEOUT);
		if (ret < 0 && ret != -ETIMEDOUT) {
			break;
		}
        LOG_INF("Waiting for modem retrying.");
	}

	if (ret < 0) {
		LOG_ERR("MODEM WAIT LOOP ERROR: %d", ret);
        k_sem_give(&mdata.mdm_lock);
		goto error;
	}

	ret = modem_cmd_handler_setup_cmds(&mctx.iface, &mctx.cmd_handler,
					   setup_cmds, ARRAY_SIZE(setup_cmds),
					   &mdata.sem_response,
					   MDM_REGISTRATION_TIMEOUT);

	if (ret < 0) {
        k_sem_give(&mdata.mdm_lock);
		goto error;
	}

#if 0
    k_sleep(K_SECONDS(5));
    /* register operator automatically */
    ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler,
                 NULL, 0, "AT+COPS=0,0",
                 &mdata.sem_response,
                 MDM_REGISTRATION_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("AT+COPS ret:%d", ret);
		goto error;
	}
#endif

	LOG_INF("Waiting for network");

	/*
	 * TODO: A lot of this should be setup as a 3GPP module to handle
	 * basic connection to the network commands / polling
	 */

	/* wait for +CREG: 1(normal) or 5(roaming) */
	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0,
			     "AT+CREG?", &mdata.sem_response,
			     MDM_REGISTRATION_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("AT+CREG ret:%d", ret);
        k_sem_give(&mdata.mdm_lock);
		goto error;
	}

	counter = 0;
	while (counter++ < 20 && mdata.ev_creg != 1 && mdata.ev_creg != 5) {
		k_sleep(K_SECONDS(1));
	}

    /* give semaphore for rssi query to work */
    k_sem_give(&mdata.mdm_lock);

	/* query modem RSSI */
	modem_rssi_query_work(NULL);
	k_sleep(MDM_WAIT_FOR_RSSI_DELAY);

	counter = 0;
	/* wait for RSSI < 0 and > -1000 */
	while (counter++ < MDM_WAIT_FOR_RSSI_COUNT &&
	       (mctx.data_rssi >= 0 || mctx.data_rssi <= -1000)) {
		modem_rssi_query_work(NULL);
		k_sleep(MDM_WAIT_FOR_RSSI_DELAY);
	}

	if (mctx.data_rssi >= 0 || mctx.data_rssi <= -1000) {
		retry_count++;
		if (retry_count >= MDM_NETWORK_RETRY_COUNT) {
			LOG_ERR("Failed network init.  Too many attempts!");
			ret = -ENETUNREACH;
			goto error;
		}

		LOG_ERR("Failed network init.  Restarting process. %d", mctx.data_rssi);
		goto restart;
	}

	LOG_INF("Network is ready.");

#if 0
	/* Set iface up */
	net_if_up(mdata.net_iface);
#endif

	/* start RSSI query */
	k_delayed_work_submit_to_queue(&modem_workq, &mdata.rssi_query_work,
				       K_SECONDS(RSSI_TIMEOUT_SECS));

error:
	return;
}

#if 0
/*
 * generic socket creation function
 * which can be called in bind() or connect()
 */
static int create_socket(struct modem_socket *sock, const struct sockaddr *addr)
{
	int ret = 0;

	/* TODO Change if multi sockets, create socket */
	sock = modem_socket_from_newid(&mdata.socket_config);
	sock->id = 0;

	return ret;
}

/*
 * Socket Offload OPS
 */

static int offload_socket(int family, int type, int proto)
{
	/* defer modem's socket create call to bind() */
	return modem_socket_get(&mdata.socket_config, family, type, proto);
}

static int offload_close(int sock_fd)
{
	struct modem_socket *sock;
	char buf[sizeof("AT+CIPCLOSE\r")];
	int ret;

	sock = modem_socket_from_fd(&mdata.socket_config, sock_fd);
	if (!sock) {
		/* socket was already closed?  Exit quietly here. */
		return 0;
	}

	/* make sure we assigned an id */
	if (sock->id < mdata.socket_config.base_socket_num) {
		return 0;
	}

	snprintk(buf, sizeof(buf), "AT+CIPCLOSE");

    k_sem_take(&mdata.mdm_lock, K_FOREVER);

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buf,
			     &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
	}

    k_sem_give(&mdata.mdm_lock);

	modem_socket_put(&mdata.socket_config, sock_fd);
	return 0;
}

static int offload_bind(int sock_fd, const struct sockaddr *addr,
			socklen_t addrlen)
{
	/* Not supported */
	return -EPFNOSUPPORT;
}

static int offload_connect(int sock_fd, const struct sockaddr *addr,
			   socklen_t addrlen)
{
	struct modem_socket *sock;
	int ret;
	char buf1[sizeof("AT+CGACT=#,#\r")];
	char buf2[sizeof("AT+CIPSTART=!###!,!###.###.###.###!,#####\r") +
		  CONFIG_MODEM_SOCKET_MAX_HST_LEN];
	u16_t dst_port = 0U;

	if (!addr) {
		LOG_ERR("Invalid addr from fd:%d", sock_fd);
		return -EINVAL;
	}

	sock = modem_socket_from_fd(&mdata.socket_config, sock_fd);
	if (!sock) {
		LOG_ERR("Can't locate socket from fd:%d", sock_fd);
		return -EINVAL;
	}

	if (sock->id < mdata.socket_config.base_socket_num - 1) {
		LOG_ERR("Invalid socket_id(%d) from fd:%d", sock->id, sock_fd);
		return -EINVAL;
	}

	/* make sure we've created the socket */
	if (sock->id == mdata.socket_config.sockets_len + 1) {
		ret = create_socket(sock, NULL);
		if (ret < 0) {
			LOG_ERR("Create socket fail from fd:%d", sock_fd);
			return ret;
		}
	}

	memset(buf1, 0, sizeof(buf1));
	snprintk(buf1, sizeof(buf1), "AT+CGATT=%d", 1);

    k_sem_take(&mdata.mdm_lock, K_FOREVER);

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buf1,
			     &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf1), ret);
		goto ret;
	}

	memset(buf1, 0, sizeof(buf1));
	snprintk(buf1, sizeof(buf1), "AT+CGACT=%d,%d", 1, 1);
	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buf1,
			     &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf1), ret);
		goto ret;
	}

	memcpy(&sock->dst, addr, sizeof(*addr));
	if (addr->sa_family == AF_INET6) {
		dst_port = ntohs(net_sin6(addr)->sin6_port);
	} else if (addr->sa_family == AF_INET) {
		dst_port = ntohs(net_sin(addr)->sin_port);
	} else {
		return -EPFNOSUPPORT;
	}

	/* if UDP */
	if (sock->ip_proto == IPPROTO_UDP) {
		snprintk(buf2, sizeof(buf2), "AT+CIPSTART=%s,\"%s\",%d", "UDP",
			 mdata.hst_name, dst_port);
	} else if (sock->ip_proto == IPPROTO_TCP) {
		snprintk(buf2, sizeof(buf2), "AT+CIPSTART=%s,\"%s\",%d", "TCP",
			 mdata.hst_name, dst_port);
	}

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buf2,
			     &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf2), ret);
	}

	/* wait for connect */
	k_sem_reset(&mdata.sem_connect);

	if (k_sem_take(&mdata.sem_connect, MDM_CMD_TIMEOUT) != 0) {
		LOG_ERR("No connect resp in %d s", MDM_CMD_TIMEOUT);
		ret = -EIO;
	} else {
		if (connect_status == MDM_CONNECT_FAIL) {
			LOG_ERR("%s ret:%d", log_strdup(buf2), ret);
			ret = -EIO;
		}
	}

ret:
    k_sem_give(&mdata.mdm_lock);

	return ret;
}

/* support for POLLIN only for now. */
static int offload_poll(struct pollfd *fds, int nfds, int msecs)
{
	int i;
	void *obj;

	/* Only accept modem sockets. */
	for (i = 0; i < nfds; i++) {
		if (fds[i].fd < 0) {
			continue;
		}

		/* If vtable matches, then it's modem socket. */
		obj = z_get_fd_obj(fds[i].fd,
				   (const struct fd_op_vtable *)
						&offload_socket_fd_op_vtable,
				   EINVAL);
		if (obj == NULL) {
			return -1;
		}
	}

	return modem_socket_poll(&mdata.socket_config, fds, nfds, msecs);
}

static ssize_t offload_recvfrom(int sock_fd, void *buf, short int len,
				short int flags, struct sockaddr *from,
				socklen_t *fromlen)
{
	struct modem_socket *sock;
	ssize_t ret_len = 0;
	int ret;

	if (!buf || len == 0) {
		return -EINVAL;
	}

	sock = modem_socket_from_fd(&mdata.socket_config, sock_fd);
	if (!sock) {
		LOG_ERR("Can't locate socket from fd:%d", sock_fd);
		return -EINVAL;
	}

	if (flags & MSG_PEEK) {
		return -ENOTSUP;
	} else if (flags & MSG_DONTWAIT && !sock->packet_sizes[0]) {
		return 0;
	}

	if (!sock->packet_sizes[0]) {
		k_sem_take(&sock->sem_data_ready, K_FOREVER);
	}

	/* return length of received data */
	ret = ring_buf_get(sock->socket_buf, (u8_t *)buf, len);
	if (ret != 0) {
		LOG_ERR("err socket data recv %d", sock_fd);
	}

	ret_len = ret;

	/* remove packet from list (ignore errors) */
	ret = modem_socket_packet_size_update(&mdata.socket_config, sock,
					      -ret_len);
	if (ret < 0) {
		LOG_ERR("err socket packet size update %d", sock_fd);
		goto exit;
	}

	/* HACK: use dst address as from */
	if (from && fromlen) {
		*fromlen = sizeof(sock->dst);
		memcpy(from, &sock->dst, *fromlen);
	}

exit:
	/* clear socket data */
	sock->data = NULL;
	return ret;
}

static ssize_t offload_recv(int sock_fd, void *buf, size_t max_len, int flags)
{
	return offload_recvfrom(sock_fd, buf, (short int)max_len, flags, NULL,
				NULL);
}

static ssize_t offload_sendto(int sock_fd, const void *buf, size_t len,
			      int flags, const struct sockaddr *to,
			      socklen_t tolen)
{
	struct modem_socket *sock;
	int ret;
	size_t rem_len = len;
	char cmd_buf[sizeof("AT+CIPSEND=####,\r") + 1024];
	memset(cmd_buf, 0, sizeof(cmd_buf));

	if (!buf || len == 0) {
		return -EINVAL;
	}

	sock = modem_socket_from_fd(&mdata.socket_config, sock_fd);
	if (!sock) {
		LOG_ERR("Can't locate socket from fd:%d", sock_fd);
		return -EINVAL;
	}

	if (!to && sock->ip_proto == IPPROTO_UDP) {
		to = &sock->dst;
	}

    k_sem_take(&mdata.mdm_lock, K_FOREVER);

	while (rem_len > 0) {
		if (rem_len > 1024) {
			snprintk(cmd_buf, sizeof(cmd_buf), "AT+CIPSEND=%d,",
				 1024);
			snprintk(cmd_buf + strlen(cmd_buf), 1024, "\"%s\"",
				 (u8_t *)buf + len - rem_len);

			ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler,
					     NULL, 0U, cmd_buf,
					     &mdata.sem_response,
					     MDM_CMD_TIMEOUT);
			if (ret < 0) {
				LOG_ERR("%s ret:%d, rem_len:%d",
					log_strdup(buf), ret, rem_len);
				goto ret;
			}
			memset(cmd_buf, 0, sizeof(cmd_buf));
			rem_len -= 1024;
		} else {
			snprintk(cmd_buf, sizeof(cmd_buf),
				 "AT+CIPSEND=%d,\"%s\"", rem_len,
				 (u8_t *)buf + len - rem_len);
			ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler,
					     NULL, 0U, cmd_buf,
					     &mdata.sem_response,
					     MDM_CMD_TIMEOUT);
			if (ret < 0) {
				LOG_ERR("%s ret:%d, rem_len:%d",
					log_strdup(buf), ret, rem_len);
				goto ret;
			}
			rem_len = 0;
		}
	}

ret:
    k_sem_give(&mdata.mdm_lock);

	return ret;
}

static ssize_t offload_send(int sock_fd, const void *buf, size_t len, int flags)
{
	return offload_sendto(sock_fd, buf, len, flags, NULL, 0U);
}

int offload_getaddrinfo(const char *node, const char *service,
			const struct addrinfo *hints, struct addrinfo **res)
{
	if (strlen(node) < CONFIG_MODEM_SOCKET_MAX_HST_LEN) {
		memcpy(mdata.hst_name, node, strlen(node));
	} else {
		return -1;
	}
	return 0;
}

void offload_freeaddrinfo(struct addrinfo *res)
{
	memcpy(mdata.hst_name, 0, CONFIG_MODEM_SOCKET_MAX_HST_LEN);
}

static const struct socket_op_vtable offload_socket_fd_op_vtable = {
	.fd_vtable = {
		.read = offload_read,
		.write = offload_write,
		.ioctl = offload_ioctl,
	},
	.bind = offload_bind,
	.connect = offload_connect,
	.sendto = offload_sendto,
	.recvfrom = offload_recvfrom,
	.listen = NULL,
	.accept = NULL,
	.sendmsg = NULL,
	.getsockopt = NULL,
	.setsockopt = NULL,
};

static const struct socket_offload modem_socket_offload = {
	.socket = offload_socket,
	.close = offload_close,
	.bind = offload_bind,
	.connect = offload_connect,
	.poll = offload_poll,
	.recv = offload_recv,
	.recvfrom = offload_recvfrom,
	.send = offload_send,
	.sendto = offload_sendto,
	.getaddrinfo = offload_getaddrinfo,
	.freeaddrinfo = offload_freeaddrinfo,
};
#endif

static int net_offload_dummy_get(sa_family_t family, enum net_sock_type type,
				 enum net_ip_protocol ip_proto,
				 struct net_context **context)
{
	LOG_DBG("NET_SOCKET_OFFLOAD must be configured for this driver");

	return -ENOTSUP;
}

/* placeholders, until Zepyr IP stack updated to handle a NULL net_offload */
static struct net_offload modem_net_offload = {
	.get = net_offload_dummy_get,
};

int a9g_get_clock(struct device *dev, char *timeval)
{
	char buf[sizeof("AT+CCLK?\r")];
	int ret = 0;

	memset(buf, 0, sizeof(buf));
	snprintk(buf, sizeof(buf), "AT+CCLK?");

    /* FIXME Find a common solution for all locks */
    ret = k_sem_take(&mdata.mdm_lock, MDM_LOCK_TIMEOUT);
    if (ret != 0)
    {
        return ret;
    }

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buf,
			     &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
		goto ret;
	}

    /* copy time value */
    strcpy(timeval, mdata.time_data);

ret:
    k_sem_give(&mdata.mdm_lock);

	return ret;
}


int a9g_http_init(struct device *dev, struct usr_http_cfg *cfg)
{
	char buf[sizeof("AT+CGACT=#,#\r")];
	int ret = 0;

	memset(buf, 0, sizeof(buf));
	snprintk(buf, sizeof(buf), "AT+CGATT=%d", 1);

    k_sem_take(&mdata.mdm_lock, K_FOREVER);

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buf,
			     &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
		goto ret;
	}

	memset(buf, 0, sizeof(buf));
	snprintk(buf, sizeof(buf), "AT+CGACT=%d,%d", 1, 1);

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buf,
			     &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
		goto ret;
	}

	memset(buf, 0, sizeof(buf));
	snprintk(buf, sizeof(buf), "AT+INITHTTP");
	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buf,
			     &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
		goto ret;
	}

ret:
    k_sem_give(&mdata.mdm_lock);

	return ret;
}

int a9g_http_term(struct device *dev, struct usr_http_cfg *cfg)
{
	char buf[sizeof("AT+TERMHTTP###\r")];
	int ret = 0;

	memset(buf, 0, sizeof(buf));
	snprintk(buf, sizeof(buf), "AT+TERMHTTP");

    k_sem_take(&mdata.mdm_lock, K_FOREVER);

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buf,
			     &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
		goto ret;
	}

#if 0
    memset(buf, 0, sizeof(buf));
    snprintk(buf, sizeof(buf), "AT+CGACT=%d,%d", 0, 1);
	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler,
			     NULL, 0U, buf, &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
        return ret;
	}

    memset(buf, 0, sizeof(buf));
    snprintk(buf, sizeof(buf), "AT+CGATT=%d", 0);
	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler,
			     NULL, 0U, buf, &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
        return ret;
	}
#endif

ret:
    k_sem_give(&mdata.mdm_lock);

	return ret;
}

int a9g_http_execute(struct device *dev, struct usr_http_cfg *cfg)
{
	int ret = 0;

	switch (cfg->method) {
	case HTTP_GET:
		if (cfg->url == NULL) {
			return -EINVAL;
		}

		/* set http config data */
		mdata.recv_cfg.recv_buf = cfg->recv_buf;
		mdata.recv_cfg.recv_buf_len = cfg->recv_buf_len;

		memset(mdata.send_buf, 0, sizeof(mdata.send_buf));
		snprintk(mdata.send_buf, sizeof(mdata.send_buf),
			 "AT+HTTPGET=\"%s\"", cfg->url);
        
        k_sem_take(&mdata.mdm_lock, K_FOREVER);

		ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U,
				     mdata.send_buf, &mdata.sem_response,
				     MDM_CMD_TIMEOUT);
		if (ret < 0) {
			LOG_ERR("%s ret:%d", log_strdup(cfg->url), ret);
			goto ret;
		}

		/* wait for response */
		k_sem_reset(&mdata.sem_connect);

		if (k_sem_take(&mdata.sem_connect, K_MSEC(cfg->timeout)) != 0) {
			LOG_ERR("No http resp in %d ms", cfg->timeout);
			/* clear callback for more data */
			mdata.cmd_handler_data.process_data = NULL;

			/* received data len */
			cfg->recv_read_len = mdata.recv_cfg.recv_read_len;

			/* set http config data */
			mdata.recv_cfg.expected_len = 0;
			mdata.recv_cfg.recv_buf = NULL;
			mdata.recv_cfg.recv_buf_len = 0;
			mdata.recv_cfg.recv_read_len = 0;

			ret = -EIO;
            goto ret;
		}

		/* clear callback for more data */
		mdata.cmd_handler_data.process_data = NULL;

		/* received data len */
		cfg->recv_read_len = mdata.recv_cfg.recv_read_len;

		/* set http config data */
		mdata.recv_cfg.expected_len = 0;
		mdata.recv_cfg.recv_buf = NULL;
		mdata.recv_cfg.recv_buf_len = 0;
		mdata.recv_cfg.recv_read_len = 0;

		break;

	case HTTP_POST:
		if (cfg->url == NULL || cfg->content_type == NULL ||
		    cfg->content_body == NULL) {
			return -EINVAL;
		}

		/* set http config data */
		mdata.recv_cfg.recv_buf = cfg->recv_buf;
		mdata.recv_cfg.recv_buf_len = cfg->recv_buf_len;

		memset(mdata.send_buf, 0, sizeof(mdata.send_buf));
		snprintk(mdata.send_buf, sizeof(mdata.send_buf),
			 "AT+HTTPPOST=\"%s\",\"%s\",\"%s\"", cfg->url,
			 cfg->content_type, cfg->content_body);

        k_sem_take(&mdata.mdm_lock, K_FOREVER);

		ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U,
				     mdata.send_buf, &mdata.sem_response,
				     MDM_CMD_TIMEOUT);
		if (ret < 0) {
			LOG_ERR("%s ret:%d", log_strdup(cfg->url), ret);
			goto ret;
		}

		/* wait for response */
		k_sem_reset(&mdata.sem_connect);

		if (k_sem_take(&mdata.sem_connect, K_MSEC(cfg->timeout)) != 0) {
			LOG_ERR("No http resp in %d ms", cfg->timeout);

			/* clear callback for more data */
			mdata.cmd_handler_data.process_data = NULL;

			/* received data len */
			cfg->recv_read_len = mdata.recv_cfg.recv_read_len;

			/* set http config data */
			mdata.recv_cfg.expected_len = 0;
			mdata.recv_cfg.recv_buf = NULL;
			mdata.recv_cfg.recv_buf_len = 0;
			mdata.recv_cfg.recv_read_len = 0;

			ret = -EIO;
            goto ret;
		}

		/* clear callback for more data */
		mdata.cmd_handler_data.process_data = NULL;

		/* received data len */
		cfg->recv_read_len = mdata.recv_cfg.recv_read_len;

		/* set http config data */
		mdata.recv_cfg.expected_len = 0;
		mdata.recv_cfg.recv_buf = NULL;
		mdata.recv_cfg.recv_buf_len = 0;
		mdata.recv_cfg.recv_read_len = 0;

		break;

	default:
		LOG_ERR("Currently not supported");
		return -ENOTSUP;
	}

ret:
    k_sem_give(&mdata.mdm_lock);

	return ret;
}

int a9g_gps_init(struct device *dev, struct usr_gps_cfg *cfg)
{
	char buf[sizeof("AT+QGNSSC=1\r")];
	int ret = 0;

	memset(buf, 0, sizeof(buf));
	snprintk(buf, sizeof(buf), "AT+QGNSSC=1");

    k_sem_take(&mdata.mdm_lock, K_FOREVER);

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buf,
			     &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
		goto ret;
	}

	memset(buf, 0, sizeof(buf));
	snprintk(buf, sizeof(buf), "AT+CCED=1,2");
	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buf,
			     &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
		goto ret;
	}

ret:
    k_sem_give(&mdata.mdm_lock);

	return ret;
}

int a9g_agps(struct device *dev, struct usr_gps_cfg *cfg)
{
	char buf[sizeof("AT+QGNSSEPO=1\r")];
	int ret = 0;

	memset(buf, 0, sizeof(buf));
	snprintk(buf, sizeof(buf), "AT+QGNSSEPO=1");

    k_sem_take(&mdata.mdm_lock, K_FOREVER);

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buf,
			     &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
		goto ret;
	}

	/* wait for agps result */
	k_sem_reset(&mdata.sem_connect);

	if (k_sem_take(&mdata.sem_connect, K_MSEC(50000)) != 0) {
		LOG_ERR("No agps resp in %d ms", 50000);
		ret = -EIO;
	}
    else
    {
		if (connect_status == MDM_CONNECT_FAIL) {
			LOG_ERR("%s ret:%d", log_strdup(buf), ret);
			ret = -EIO;
		}
    }

    cfg->agps_status = mdata.agps_status;

ret:
    k_sem_give(&mdata.mdm_lock);

	return ret;
}

int a9g_gps_read(struct device *dev, struct usr_gps_cfg *cfg)
{
	char buf[sizeof("AT+QGNSSRD?\r")];
	int ret = 0;

	memset(buf, 0, sizeof(buf));
	snprintk(buf, sizeof(buf), "AT+QGNSSRD?");

    /* FIXME Find a common solution for all locks */
    ret = k_sem_take(&mdata.mdm_lock, MDM_LOCK_TIMEOUT);
    if (ret != 0)
    {
        return ret;
    }

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buf,
			     &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
		goto ret;
	}

	/* read data */
#if 0
    strcpy(cfg->lat, mdata.lat_cur);
    strcpy(cfg->lon, mdata.lon_cur);
#endif
    strcpy(cfg->gps_data, mdata.gps_data);

ret:
    k_sem_give(&mdata.mdm_lock);

	return ret;
}

int a9g_gps_close(struct device *dev, struct usr_gps_cfg *cfg)
{
	char buf[sizeof("AT+QGNSSC=0\r")];
	int ret = 0;

	memset(buf, 0, sizeof(buf));
	snprintk(buf, sizeof(buf), "AT+QGNSSC=0");

    k_sem_take(&mdata.mdm_lock, K_FOREVER);

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buf,
			     &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
		goto ret;
	}

    cfg->agps_status = 1;
    mdata.agps_status = 1;

ret:
    k_sem_give(&mdata.mdm_lock);

	return ret;
}

int a9g_get_ctx(struct device *dev, struct mdm_ctx *ctx)
{
    struct modem_context *mdm_ctx = modem_context_from_id(0);

    strcpy(ctx->data_manufacturer, mdata.mdm_manufacturer);
    strcpy(ctx->data_model, mdata.mdm_model);
    strcpy(ctx->data_revision, mdata.mdm_revision);
    strcpy(ctx->data_imei, mdata.mdm_imei);
    strcpy(ctx->data_timeval, mdata.mdm_timeval);
    ctx->data_sys_timeval = mctx.data_sys_timeval;
    ctx->data_rssi = mctx.data_rssi;

    return 0;
}

#define HASH_MULTIPLIER 37
static u32_t hash32(char *str, int len)
{
	u32_t h = 0;
	int i;

	for (i = 0; i < len; ++i) {
		h = (h * HASH_MULTIPLIER) + str[i];
	}

	return h;
}

static inline u8_t *modem_get_mac(struct device *dev)
{
	struct modem_data *data = dev->driver_data;
	u32_t hash_value;

	data->mac_addr[0] = 0x00;
	data->mac_addr[1] = 0x10;

	/* use IMEI for mac_addr */
	hash_value = hash32(mdata.mdm_imei, strlen(mdata.mdm_imei));

	UNALIGNED_PUT(hash_value, (u32_t *)(data->mac_addr + 2));

	return data->mac_addr;
}

static void modem_net_iface_init(struct net_if *iface)
{
	struct device *dev = net_if_get_device(iface);
	struct modem_data *data = dev->driver_data;

	/* Direct socket offload used instead of net offload: */
	iface->if_dev->offload = &modem_net_offload;
	net_if_set_link_addr(iface, modem_get_mac(dev), sizeof(data->mac_addr),
			     NET_LINK_ETHERNET);
	data->net_iface = iface;
}

static struct modem_a9g_rda_net_api api_funcs = {
	.net_api =
		{
			.init = modem_net_iface_init,
		},
	.get_clock = a9g_get_clock,
	.http_init = a9g_http_init,
	.http_execute = a9g_http_execute,
	.http_term = a9g_http_term,
	.gps_init = a9g_gps_init,
	.gps_agps = a9g_agps,
	.gps_read = a9g_gps_read,
	.gps_close = a9g_gps_close,
	.get_ctx = a9g_get_ctx,
    .reset = modem_reset,
};

/* TODO Using single socket mode for now. Use multi socket mode todo */
static struct modem_cmd response_cmds[] = {
	MODEM_CMD("OK", on_cmd_ok, 0U, ""), /* 3GPP */
	MODEM_CMD("CONNECT OK", on_cmd_connect_ok, 0U, ""), /* TCP connect */
	MODEM_CMD("CONNECT FAIL", on_cmd_connect_fail, 0U,
		  ""), /* TCP connect */
	MODEM_CMD("SHUT OK", on_cmd_shut_ok, 0U, ""), /* TCP shut */
	MODEM_CMD("ERROR", on_cmd_error, 0U, ""), /* 3GPP */
	MODEM_CMD("+CME ERROR: ", on_cmd_exterror, 1U, ""),
	MODEM_CMD("+AGPS: ", on_cmd_gps_agps, 1U, ","),
	MODEM_CMD("+GETREFLOC: ", on_cmd_gps_getrefloc, 2U, ","),
	MODEM_CMD("$GNGGA,", on_cmd_gps_read, 0U, ""),
	MODEM_CMD("+CCLK: ", on_cmd_gettime, 1U, ""),
};

static struct modem_cmd unsol_cmds[] = {
#if 0
	MODEM_CMD("CLOSED", on_cmd_socknotifyclose, 0U, ""), /* TCP closed */
	MODEM_CMD("+CIPRCV: ", on_cmd_socknotifydata, 1U, ","),
#endif
	MODEM_CMD("+CREG: ", on_cmd_socknotifycreg, 1U, ","),
	MODEM_CMD("+CTZV:", on_cmd_timezoneval, 1U, ""),
	MODEM_CMD("+HTTP: ", on_cmd_http_response, 2U, ","),
};

static int modem_init(struct device *dev)
{
	int ret = 0;

	ARG_UNUSED(dev);

	k_sem_init(&mdata.sem_response, 0, 1);
	k_sem_init(&mdata.sem_connect, 0, 1);
	k_sem_init(&mdata.mdm_lock, 1, 1);

	/* initialize the work queue */
	k_work_q_start(&modem_workq, modem_workq_stack,
		       K_THREAD_STACK_SIZEOF(modem_workq_stack),
		       K_PRIO_COOP(7));

#if 0
	/* socket config */
	mdata.socket_config.sockets = &mdata.sockets[0];
	/* FIXME Only socket 0 in use, HACK */
	mdata.socket_config.sockets[0].socket_buf = &mdm_socket_ring_buf_0;
	mdata.socket_config.sockets_len = ARRAY_SIZE(mdata.sockets);
	mdata.socket_config.base_socket_num = MDM_BASE_SOCKET_NUM;
	ret = modem_socket_init(&mdata.socket_config, &offload_socket_fd_op_vtable);
	if (ret < 0) {
		goto error;
	}
#endif

	/* cmd handler */
	mdata.cmd_handler_data.cmds[CMD_RESP] = response_cmds;
	mdata.cmd_handler_data.cmds_len[CMD_RESP] = ARRAY_SIZE(response_cmds);
	mdata.cmd_handler_data.cmds[CMD_UNSOL] = unsol_cmds;
	mdata.cmd_handler_data.cmds_len[CMD_UNSOL] = ARRAY_SIZE(unsol_cmds);
	mdata.cmd_handler_data.read_buf = &mdata.cmd_read_buf[0];
	mdata.cmd_handler_data.read_buf_len = sizeof(mdata.cmd_read_buf);
	mdata.cmd_handler_data.match_buf = &mdata.cmd_match_buf[0];
	mdata.cmd_handler_data.match_buf_len = sizeof(mdata.cmd_match_buf);
	mdata.cmd_handler_data.buf_pool = &mdm_recv_pool;
	mdata.cmd_handler_data.alloc_timeout = BUF_ALLOC_TIMEOUT;
	mdata.cmd_handler_data.process_data = NULL;
    mdata.cmd_handler_data.eol = "\r";
	ret = modem_cmd_handler_init(&mctx.cmd_handler,
				     &mdata.cmd_handler_data);
	if (ret < 0) {
		goto error;
	}

    /* modem gps */
    mdata.agps_status = 1;
    memset(mdata.ref_lat, 0, MDM_CD_LEN);
    memset(mdata.ref_lon, 0, MDM_CD_LEN);
    //memset(mdata.lat_cur, 0, MDM_CD_LEN);
    //memset(mdata.lon_cur, 0, MDM_CD_LEN);
    memset(mdata.gps_data, 0, MDM_GPS_DATA_LEN);

	/* modem interface */
    /* HACK FIXME */
#define HIGH_DRIVE_UART_MDM 1
#if HIGH_DRIVE_UART_MDM
    gpio_pin_configure(device_get_binding(DT_LABEL(DT_ALIAS(gpio_0))),
            DT_PROP(DT_ALIAS(uart_0), tx_pin),
            GPIO_OUTPUT | GPIO_DS_ALT_LOW | GPIO_DS_ALT_HIGH);

    gpio_pin_configure(device_get_binding(DT_LABEL(DT_ALIAS(gpio_0))),
            DT_PROP(DT_ALIAS(uart_0), rx_pin),
            GPIO_OUTPUT | GPIO_DS_ALT_LOW | GPIO_DS_ALT_HIGH);
#endif
	mdata.iface_data.isr_buf = &mdata.iface_isr_buf[0];
	mdata.iface_data.isr_buf_len = sizeof(mdata.iface_isr_buf);
	mdata.iface_data.rx_rb_buf = &mdata.iface_rb_buf[0];
	mdata.iface_data.rx_rb_buf_len = sizeof(mdata.iface_rb_buf);
	ret = modem_iface_uart_init(&mctx.iface, &mdata.iface_data,
				    MDM_UART_DEV_NAME);
	if (ret < 0) {
		goto error;
	}

	/* modem data storage */
	mctx.data_manufacturer = mdata.mdm_manufacturer;
	mctx.data_model = mdata.mdm_model;
	mctx.data_revision = mdata.mdm_revision;
	mctx.data_imei = mdata.mdm_imei;
	mctx.data_timeval = mdata.mdm_timeval;

	/* pin setup */
	mctx.pins = modem_pins;
	mctx.pins_len = ARRAY_SIZE(modem_pins);

	mctx.driver_data = &mdata;

	ret = modem_context_register(&mctx);
	if (ret < 0) {
		LOG_ERR("Error registering modem context: %d", ret);
		goto error;
	}

	/* start RX thread */
	k_thread_create(&modem_rx_thread, modem_rx_stack,
			K_THREAD_STACK_SIZEOF(modem_rx_stack),
			(k_thread_entry_t)modem_rx, NULL, NULL, NULL,
			K_PRIO_COOP(7), 0, K_NO_WAIT);

	/* init RSSI query */
	k_delayed_work_init(&mdata.rssi_query_work, modem_rssi_query_work);

	modem_reset();

error:
	return ret;
}

NET_DEVICE_OFFLOAD_INIT(modem_a9g, DT_INST_LABEL(0), modem_init,
        device_pm_control_nop, &mdata, NULL, CONFIG_MODEM_A9GRDA_INIT_PRIORITY,
        &api_funcs, MDM_MAX_DATA_LENGTH);

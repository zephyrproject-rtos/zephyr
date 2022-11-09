/*
 * Copyright (c) 2020 ZedBlox Logitech Pvt Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT quectel_bg95

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(modem_quectel_bg95, CONFIG_MODEM_LOG_LEVEL);

#include <drivers/modem/quectel-bg95.h>
#include <drivers/gpio.h>
#include <init.h>

#include "modem_context.h"
#include "modem_socket.h"
#include "modem_cmd_handler.h"
#include "modem_iface_uart.h"

/* Change to 1 if socket functionality enabled */
#define MODEM_BG95_SOCKET

#define MDM_UART_NODE			DT_INST_BUS(0)
#define MDM_UART_DEV			DEVICE_DT_GET(MDM_UART_NODE)

#define MDM_CMD_CONN_TIMEOUT K_SECONDS(180)
#define MDM_DFOTA_TIMEOUT    K_SECONDS(480)

#define MAX_HTTP_CMD_SIZE 64  //32

#define URC_SSL_RECV     1
#define URC_SSL_CLOSED   2
#define URC_PDP_DEACT    4

#define GPS_PRIORITY   0
#define WWAN_PRIORITY  1
/* Start AGPS by default */
//#define AGPS_DEFAULT
/* Uncomment if need to close gps everytime WWAN is in use */
//define GPS_CLOSE_ON_WWAN
/* Uncomment if BG96 used instead of BG95. FIXME Use config */
//define QUECTEL_BG96

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
#define MDM_DNS_TIMEOUT K_SECONDS(120)
#define MDM_REGISTRATION_TIMEOUT K_SECONDS(240)
#define MDM_NETWORK_REG_TIMEOUT K_SECONDS(30)
#define MDM_PROMPT_CMD_DELAY K_MSEC(75)
#define MDM_LOCK_TIMEOUT K_SECONDS(1)

#define MDM_MAX_DATA_LENGTH 1024U
/* From the quectel BG95 datasheet */
#define MDM_MAX_SEND_DATA_LEN  1450U
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

#define HTTP_TIMEOUT_SECS 10 /* FIXME Check this timeout */

static struct mdm_ctx q_ctx;

static uint16_t cinfo_idx = 0;

NET_BUF_POOL_DEFINE(mdm_recv_pool, MDM_RECV_MAX_BUF, MDM_RECV_BUF_SIZE, 0,
		    NULL);

/* RX thread structures */
K_THREAD_STACK_DEFINE(modem_rx_stack, CONFIG_MODEM_QUECTEL_BG95_RX_STACK_SIZE);
struct k_thread modem_rx_thread;

/* RX thread work queue */
K_THREAD_STACK_DEFINE(modem_workq_stack,
		      CONFIG_MODEM_QUECTEL_BG95_RX_WORKQ_STACK_SIZE);
static struct k_work_q modem_workq;

struct http_config {
	uint8_t http_pending;
	uint16_t http_id;
	size_t http_resp_len;
    uint16_t http_rd_err;
};

struct recv_data_config {
	char *recv_buf;
	size_t recv_buf_len;
	size_t expected_len;
	size_t recv_read_len;
	int recv_status;
	union {
		/* http config */
		struct http_config http_cfg;
	};
};

/* socket read callback data */
struct socket_read_data {
	char *recv_buf;
	size_t recv_buf_len;
	struct sockaddr *recv_addr;
	uint16_t recv_read_len;
};

/* TODO Check fields in this struct */
struct fileops_data {
    int status;
    int open_fd;
    int offset;
    uint8_t* rw_buf;
    size_t exp_wr_sz;
    size_t act_wr_sz;
    size_t fsize;
    size_t tot_sz;
    size_t rd_buf_sz;
    size_t act_rd_sz;
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
	uint8_t cmd_read_buf[MDM_RECV_BUF_SIZE];
	uint8_t cmd_match_buf[MDM_RECV_BUF_SIZE + 1];

	/* socket data */
	struct modem_socket_config socket_config;
	struct modem_socket sockets[MDM_MAX_SOCKETS];

	/* FIXME Host name. Move to struct modem_socket? */
	char hst_name[CONFIG_MODEM_SOCKET_MAX_HST_LEN];

	/* command buffer send */
	char send_buf[CONFIG_MODEM_MAXIMUM_TR_RC_SIZE];

	/* RSSI work */
	struct k_delayed_work rssi_query_work;
	struct k_work urc_handle_work;

	/* modem state */
	int ev_creg;
    int pdp_ctx;
    /*
     * 0 - "ssl-recv"
     * 1 - "ssl-closed"
     * 2 - "tcp-pdpdeact"
     * 3:8 - rsvd
     */
    uint8_t urc_status;
    int urc_close;

    /* ntp status */
    int ntp_status;

    /* file ops */
    struct fileops_data fops;

    /* bytes written to socket in last transaction */
    int sock_written;

	/* response semaphore */
	struct k_sem sem_response;

	/* connect semaphore */
	struct k_sem sem_connect;

	/* reply semaphore */
    struct k_sem sem_reply;

	/* lock semaphore */
	struct k_sem mdm_lock;

	/* Config to receive more data */
	struct recv_data_config recv_cfg;

    /* wwan status */
    int wwan_in_session;

    /* gps info */
    int gps_status;
    int agps_status;
    char gps_data[MDM_GPS_DATA_LEN];
    char ref_lat[MDM_CD_LEN];
    char ref_lon[MDM_CD_LEN];

    char time_data[MDM_TIME_LENGTH];
};

enum CONNECT_STATUS {
	MDM_CONNECT_SUCCESS = 0,
	MDM_CONNECT_FAIL,
};

/* Global static variables */
#if defined(CONFIG_DNS_RESOLVER)
static int dns_stat = 0;
static struct addrinfo result;
static struct sockaddr result_addr;
static char result_canonname[DNS_MAX_NAME_SIZE + 1];
#endif

static int open_sock_err = 0;
static int current_sock_rd_id;
static struct modem_data mdata;
static struct modem_context mctx;
static uint8_t connect_status = MDM_CONNECT_FAIL;

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

/* Handler: CONNECT */
MODEM_CMD_DEFINE(on_cmd_connect_ok)
{
	LOG_DBG("%s", __func__);
	modem_cmd_handler_set_error(data, 0);
	connect_status = MDM_CONNECT_SUCCESS;

	k_sem_give(&mdata.sem_connect);

	/* CONNECT comes before OK and data */
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

/* Handler: SEND FAIL */
MODEM_CMD_DEFINE(on_cmd_send_fail)
{
	LOG_DBG("%s", __func__);
	modem_cmd_handler_set_error(data, -EIO);
	k_sem_give(&mdata.sem_response);
    return 0;
}
/* Handler: +CME Error: <err>[0] */
MODEM_CMD_DEFINE(on_cmd_exterror)
{
	LOG_DBG("%s err %s", __func__, log_strdup(argv[0]));
	/* TODO: map extended error codes to values */
	modem_cmd_handler_set_error(data, -EIO);
	k_sem_give(&mdata.sem_response);
    return 0;
}

/*
 * GNSS response command handlers
 */

/* Handler: +QGPSLOC: */
MODEM_CMD_DEFINE(on_cmd_gps_read)
{
	size_t out_len;

	out_len = net_buf_linearize(mdata.gps_data,
				    sizeof(mdata.gps_data) - 1,
				    data->rx_buf, 0, len);
	mdata.gps_data[out_len] = '\0';

    LOG_DBG("GPS data: %s", log_strdup(mdata.gps_data));
    /* Wait for OK to follow */
	//k_sem_give(&mdata.sem_response);
    return 0;
}

/* Handler: +QNTP: <err_code>,"..." */
MODEM_CMD_DEFINE(on_cmd_ntptime)
{
    uint8_t buf[8];
    uint8_t t_off = 0;
	uint16_t ntp_err;

	memset(buf, 0, sizeof(buf));
    LOG_DBG("TIME NTP: %s", log_strdup(argv[0]));

    while ((argv[0][t_off] != ',' && argv[0][t_off] != '\0')
            && t_off < MIN(7, strlen(argv[0])+1)) {
        buf[t_off] = argv[0][t_off];
        t_off++;
    }
    buf[t_off] = '\0';

	ntp_err = ATOI(buf, 0, "ntp");

    LOG_DBG("NTP err: %d", ntp_err);

    if (ntp_err != 0) {
        LOG_ERR("ntp server time not fetched");
        goto ret; /* not returning error */
    }

    t_off++;
    if (argv[0][t_off] != '\"')
    {
        LOG_ERR("Time format +QNTP wrong %s, %c", log_strdup(argv[0]), argv[0][t_off]);
    }

ret:
    mdata.ntp_status = ntp_err;

    /* OK before this, so no OK after */
	k_sem_give(&mdata.sem_reply);

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
	memcpy(q_ctx.data_timeval, argv[0] + 1, out_len - 1);
	q_ctx.data_timeval[out_len] = '\0';
	q_ctx.data_sys_timeval = k_uptime_get_32();

	memcpy(mdata.time_data, argv[0] + 1, out_len - 1);
	mdata.time_data[out_len] = '\0';

    LOG_DBG("TIME CCLK: %s", log_strdup(mdata.time_data));

    /* OK to follow */

    return 0;
}

/*
 * Modem Info Command Handlers
 */

/* Handler: <manufacturer> */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_manufacturer)
{
	size_t out_len;

	out_len = net_buf_linearize(q_ctx.data_manufacturer,
				    sizeof(q_ctx.data_manufacturer) - 1,
				    data->rx_buf, 0, len);
	q_ctx.data_manufacturer[out_len] = '\0';
	LOG_DBG("Manufacturer: %s", log_strdup(q_ctx.data_manufacturer));
    return 0;
}

/* Handler: <qeng> */
MODEM_CMD_DEFINE(on_cmd_qeng)
{
	size_t out_len;
    uint16_t idx = 0;
    uint8_t found_ch = 0;

    if (cinfo_idx >= (MAX_CI_BUF_SIZE))
    {
        LOG_ERR("cinfo_idx cnt exceeded");
        return 0;
    }

    /* HACK FIXME strlen("\"neighbourcell\",") */
	out_len = net_buf_linearize(q_ctx.data_cellinfo + cinfo_idx,
				    MAX_CI_BUF_SIZE - cinfo_idx - 1,
				    data->rx_buf, strlen("\"neighbourcell\","),
                    len - strlen("\"neighbourcell\","));

	*(q_ctx.data_cellinfo + cinfo_idx + out_len) = ';';
	*(q_ctx.data_cellinfo + cinfo_idx + out_len + 1) = '\0';

    /* Hack FIXME Replace " with * */
    while(idx < out_len && found_ch < 2)
    {
        /* LIMITED to 2 " */
        if (*(q_ctx.data_cellinfo + cinfo_idx + idx) == '\"')
        {
            *(q_ctx.data_cellinfo + cinfo_idx + idx) = '*';
            found_ch++;
        }
        idx++;
    }

	LOG_DBG("CDBGO: %s", log_strdup(q_ctx.data_cellinfo +
                cinfo_idx));

    cinfo_idx += (out_len + 1); //+1 for ";"

    return 0;
}

/* Handler: <model> */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_model)
{
	size_t out_len;

	out_len =
		net_buf_linearize(q_ctx.data_model, sizeof(q_ctx.data_model) - 1,
				  data->rx_buf, 0, len);
	q_ctx.data_model[out_len] = '\0';
	LOG_INF("Model: %s", log_strdup(q_ctx.data_model));
    return 0;
}

/* Handler: <rev> */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_revision)
{
	size_t out_len;

	out_len = net_buf_linearize(q_ctx.data_revision,
				    sizeof(q_ctx.data_revision) - 1,
				    data->rx_buf, 0, len);
	q_ctx.data_revision[out_len] = '\0';
	LOG_INF("Revision: %s", log_strdup(q_ctx.data_revision));
    return 0;
}

/* Handler: <IMEI> */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_imei)
{
	size_t out_len;

	out_len = strlen(argv[0]);
	memcpy(q_ctx.data_imei, argv[0], out_len);
	q_ctx.data_imei[out_len] = '\0';
	LOG_INF("IMEI: %s", log_strdup(q_ctx.data_imei));
    return 0;
}

/* Handler: +CTZV:<Time> */
MODEM_CMD_DEFINE(on_cmd_timezoneval)
{
	size_t out_len;

	out_len = strlen(argv[0]);
	memcpy(q_ctx.data_timeval, argv[0], out_len);
	q_ctx.data_timeval[out_len] = '\0';
	q_ctx.data_sys_timeval = k_uptime_get_32();
	LOG_INF("TIME: %s, %u", log_strdup(q_ctx.data_timeval), q_ctx.data_sys_timeval);
    return 0;
}

/* Handler: +CSQ: <signal_power>[0],<qual>[1] */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_rssi_csq)
{
	int rssi;

	rssi = ATOI(argv[0], 0, "qual");
	LOG_DBG("rssi: %d", rssi);
	if (rssi == 31) {
		q_ctx.data_rssi = -51;
	} else if (rssi >= 0 && rssi <= 31) {
		/* FIXME: This value depends on the RAT */
		q_ctx.data_rssi = -114 + ((rssi * 2) + 1);
	} else {
		q_ctx.data_rssi = -1000;
	}

	LOG_DBG("QUAL: %d", q_ctx.data_rssi);
    return 0;
}

/* Handler: +HTTPGET: <err>,<httprspcode>,<length> */
/* Handler: +HTTPPOST: <err>,<httprspcode>,<length> */
MODEM_CMD_DEFINE(on_cmd_http_response)
{
	uint16_t http_resp_err = (uint16_t)ATOI(argv[0], 0, "err");

    LOG_DBG("http err: %d", http_resp_err);

    mdata.recv_cfg.recv_status = http_resp_err;

	k_sem_give(&mdata.sem_reply);

    return 0;
}

static size_t string_first_of(char* s, char* sub)
{
    int n;
    size_t off = 0;
    if(s == NULL) {
        return 0;
    }

    if(sub == NULL) {
        return 0;
    }
    n =  strlen(sub);
    while(*s != 0)
    {
        if(strncmp(s, sub, n) == 0)
        {
            return off;
        }
        s++;
		off++;
    }
    return -1;
}

/* Handler: CONNECT */
MODEM_CMD_DEFINE(on_cmd_http_read_con)
{
    size_t http_resp_len = net_buf_frags_len(data->rx_buf);
    int off = 0;

    http_resp_len = MIN(mdata.recv_cfg.recv_buf_len,
            http_resp_len);

    //LOG_DBG("hrl: %d", http_resp_len);
	mdata.recv_cfg.recv_read_len = net_buf_linearize(
            mdata.recv_cfg.recv_buf, mdata.recv_cfg.recv_buf_len,
            data->rx_buf, len + 2, http_resp_len);

    //LOG_DBG("rrl: %d, %d", mdata.recv_cfg.recv_read_len, mdata.recv_cfg.recv_buf_len);
    off = string_first_of(mdata.recv_cfg.recv_buf, "OK");
    if (off < 0) {
        if (mdata.recv_cfg.recv_read_len < mdata.recv_cfg.recv_buf_len)
        {
            //LOG_DBG("why not");
            return -EAGAIN;
        }
        /* TODO Check/handle buffer size not enough case */
        LOG_ERR("No matching string found or http buf not enough");
        return (mdata.recv_cfg.recv_read_len + len + 2);
    }
    else {
        mdata.recv_cfg.recv_buf[off] = '\0';
        mdata.recv_cfg.recv_read_len = off;
    }

    //LOG_DBG("rrl2: %d", off);
    return (off + len + 2);
}

/* Handler: +QHTTPREAD: <err> */
/* Handler: +QHTTPREADFILE: <err> */
MODEM_CMD_DEFINE(on_cmd_http_read)
{
	uint16_t http_err = (uint16_t)ATOI(argv[0], 0, "err");
    mdata.recv_cfg.http_cfg.http_rd_err = http_err;
	k_sem_give(&mdata.sem_reply);
    return 0;
}

#ifdef MODEM_BG95_SOCKET
/*
 * Modem Socket Command Handlers
 */

/* Common code for +QSSLRECV: <len>
 * <binary_data> */
static int on_cmd_sockread_common(int socket_id,
				   struct modem_cmd_handler_data *data,
				   int socket_data_length, uint16_t len)
{
	struct modem_socket *sock = NULL;
	struct socket_read_data *sock_data;
	int ret;

	if (!len) {
		LOG_ERR("Short +QSSLRECV value.  Aborting!");
		return -EAGAIN;
	}

	/* zero length */
	if (socket_data_length <= 0) {
		LOG_ERR("Length problem (%d).  Aborting!", socket_data_length);
		return -EAGAIN;
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
    /* skip in cmd direct match ret */
	//data->rx_buf = net_buf_skip(data->rx_buf, ret);
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
 * Handler: +QSSLRECV: <length>[3]
 *                     "<data>"
 */
MODEM_CMD_DEFINE(on_cmd_sockreadfrom)
{
    int socket_id, new_total, ret, buf_len, cur_len = 0;
    struct modem_socket *sock = NULL;
    char buf[len + 8];
    int i = len;

    cur_len = net_buf_frags_len(data->rx_buf);
    if (cur_len < (len + 7)) {
        return -EAGAIN;
    }

	buf_len = net_buf_linearize(buf,
				    len + 7, data->rx_buf, 0, len + 7);
    buf[len + 7] = '\0';

    while((buf[i] != '\r') && (i < (len + 7))) {
        i++;
    }

    if (i >= (len + 7)) {
        LOG_ERR("Wrong format in QSSLRECV");
        return -EINVAL; /* FIXME */
    }

    buf[i] = '\0';

    socket_id = current_sock_rd_id;

    /* check to make sure we have all of the data (minus parsed len and CRLF) */
	new_total = ATOI(buf + len, 0, "length");
    if (new_total == 0) {
        LOG_DBG("no more data");
        /* TODO redundant logic? No more data left */
        mdata.urc_status &= ~URC_SSL_RECV;
        return i;
    }

    cur_len = net_buf_frags_len(data->rx_buf);
	if (cur_len < (new_total + i + 2)) {
		LOG_DBG("Not enough data -- wait!");
		return -EAGAIN;
	}

    LOG_DBG("socket_id:%d len_parsed:%d len_recv: %d", socket_id,
        (i + 2), new_total);

    /* skip the matched len */
    data->rx_buf = net_buf_skip(data->rx_buf, i + 2);
    cur_len -= len;

    sock = modem_socket_from_id(&mdata.socket_config, socket_id);
	if (!sock) {
		LOG_ERR("Socket not found! (%d)", socket_id);
		ret = -EINVAL;
		return ret;
	}

	ret = modem_socket_packet_size_update(&mdata.socket_config, sock,
					      new_total);
	if (ret < 0) {
		LOG_ERR("socket_id:%d left_bytes:%d err: %d", socket_id,
			new_total, ret);
	}

	return on_cmd_sockread_common(current_sock_rd_id, data,
				      new_total, new_total);
}

/* Handler: SEND OK */
MODEM_CMD_DEFINE(on_cmd_sockwrite)
{
	LOG_DBG("%s", __func__);
	modem_cmd_handler_set_error(data, 0);
	k_sem_give(&mdata.sem_response);
    return 0;
}

/*
 * MODEM UNSOLICITED NOTIFICATION HANDLERS
 */

/* Handler: +QIURC: <type>,<ctx_id> */
/* Buffer Access Mode needs to be set for this to work */
MODEM_CMD_DEFINE(on_cmd_socknotifyurc)
{
    if (strcmp(argv[0], "\"pdpdeact\"") == 0) {
        mdata.urc_status |= URC_PDP_DEACT;
        mdata.pdp_ctx = 0;
		k_work_submit_to_queue(&modem_workq,
					       &mdata.urc_handle_work);
    }

#if defined(CONFIG_DNS_RESOLVER)
    else if (strcmp(argv[0], "\"dnsgip\"") == 0) {
        /*
         * FIXME: Following logic only considers the first IP address return
         * and ignores others.
         */
        if (dns_stat == 1) {
            /* chop off end quote */
            argv[1][strlen(argv[1]) - 1] = '\0';

            result_addr.sa_family = AF_INET;
            /* skip beginning quote when parsing */
            (void)net_addr_pton(result.ai_family, &argv[1][1],
                        &((struct sockaddr_in *)&result_addr)->sin_addr);
            dns_stat = 0;

            /* give reply semaphore */
            k_sem_give(&mdata.sem_reply);
            return 0;
        }
        else if (ATOI(argv[1], 0, "err") == 0) {
            dns_stat = 1;
        }
    }
#endif

    return 0;
}

/* Handler: +QSSLURC: <type>,<socket_id> */
/* Buffer Access Mode needs to be set for this to work */
MODEM_CMD_DEFINE(on_cmd_socknotifysslurc)
{
    int socket_id;
	struct modem_socket *sock;

	socket_id = ATOI(argv[1], 0, "socket_id");

    if (strcmp(argv[0], "\"recv\"") == 0) {
        mdata.urc_status |= URC_SSL_RECV;
        LOG_DBG("urc recv: %d, %x", socket_id, mdata.urc_status);
    } else if (strcmp(argv[0], "\"closed\"") == 0) {
        mdata.urc_status |= URC_SSL_CLOSED;
        mdata.urc_close =  socket_id;
		k_work_submit_to_queue(&modem_workq,
					       &mdata.urc_handle_work);
        return 0;
    }

	sock = modem_socket_from_id(&mdata.socket_config, socket_id);
	if (!sock) {
		return 0;
	}

    modem_socket_data_ready(&mdata.socket_config, sock);

    return 0;
}

/* Handler: +QSSLOPEN: <socket_id>,<err> */
MODEM_CMD_DEFINE(on_cmd_sockcreate)
{
    int socket_id, err;
	socket_id = ATOI(argv[0], 0, "socket_id");
	err = ATOI(argv[1], 0, "err");

    open_sock_err = err;

	k_sem_give(&mdata.sem_reply);
    return err;
}

#endif

/* Handler: +CREG: <n>,<stat>[0] */
MODEM_CMD_DEFINE(on_cmd_socknotifycreg)
{
	mdata.ev_creg = ATOI(argv[1], 0, "stat");
	LOG_DBG("CREG:%d", mdata.ev_creg);
    return 0;
}

/* Handler: +QIACT: */
MODEM_CMD_DEFINE(on_cmd_qiact)
{
    uint8_t ctx_id = ATOI(argv[0], 0, "cti");
    uint8_t ctx_state = ATOI(argv[1], 0, "cts");
    uint8_t ctx_type = ATOI(argv[2], 0, "ctt");

    LOG_DBG("qiact: %d, %d, %d", ctx_id, ctx_state,
            ctx_type);

    if (ctx_id != 1) {
        LOG_ERR("Are we using ctx other than 1?");
        return 0;
    }

    mdata.pdp_ctx = ctx_state;

    //if (strncmp(argv[3], "0.0.0.0", sizeof("0.0.0.0")))

    return 0;
}

#ifdef CONFIG_QUECTEL_BG95_FILE_OPS
/* Handler: +QFOPEN: <fd> */
MODEM_CMD_DEFINE(on_cmd_qfopen)
{
    mdata.fops.open_fd = ATOI(argv[0], 0, "fd");
    return 0;
}

/* Handler: CONNECT <rd_sz> */
MODEM_CMD_DEFINE(on_cmd_qfread)
{
    size_t cur_len = net_buf_frags_len(data->rx_buf);
    char buf[8];
    int i = 0;
    int ret = 0;
    int buf_len = 0;

    if (cur_len < (len + 3)) {
        return -EAGAIN;
    }

	buf_len = net_buf_linearize(buf, 8,
            data->rx_buf, len, cur_len - len);
    buf[7] = '\0';

    while((buf[i] != '\r') && (i < 7)) {
        i++;
    }

    if (i >= 7) {
        if (buf_len < 8) {
            return -EAGAIN;
        }
        LOG_ERR("Wrong format in QFREAD");
        return -EINVAL; /* FIXME */
    }
    buf[i] = '\0';

    mdata.fops.act_rd_sz = ATOI(buf, 0, "rd_sz");

    if (cur_len < (len + i + 2 + mdata.fops.act_rd_sz))
    {
		LOG_DBG("Not enough data -- wait!");
        return -EAGAIN;
    }

	ret = net_buf_linearize(mdata.fops.rw_buf, mdata.fops.rd_buf_sz,
				data->rx_buf, len + i + 2, mdata.fops.act_rd_sz);
    if (ret < MIN(mdata.fops.rd_buf_sz, ret < mdata.fops.act_rd_sz))
    {
        LOG_ERR("Could not fetch data");
    }

    mdata.fops.act_rd_sz = ret;

    return (ret + len + i + 2);
}

/* Handler: +QFWRITE: <wr_sz>,<tot_sz> */
MODEM_CMD_DEFINE(on_cmd_qfwrite)
{
    mdata.fops.act_wr_sz = ATOI(argv[0], 0, "wr_sz");
    mdata.fops.tot_sz = ATOI(argv[1], 0, "tot_sz");
    return 0;
}

/* Handler: +QFLST: <fname>,<f_sz> */
MODEM_CMD_DEFINE(on_cmd_qflst)
{
    /* FIXME Use fname as well */
    //mdata.fops.fname = ATOI(argv[0], 0, "wr_sz");
    LOG_DBG("**FNAME: %s, FSIZE:%s**", argv[0], argv[1]);
    mdata.fops.fsize = ATOI(argv[1], 0, "f_sz");
    return 0;
}
#endif

#ifdef CONFIG_QUECTEL_BG95_DFOTA
/* Handler: +QIND: <type>,<ind> */
MODEM_CMD_DEFINE(on_cmd_qind)
{
    /* FIXME Use fname as well */
    if (strcmp(argv[0], "\"FOTA\"") == 0)
    {
        if (strcmp(argv[1], "\"END\"") == 0)
        {
            /* give reply semaphore */
            k_sem_give(&mdata.sem_reply);
        }
    }
    return 0;
}
#endif

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

static void quectel_bg95_rx_priority(uint8_t prio)
{
    char buf[MAX_HTTP_CMD_SIZE];
    int ret = 0;

    memset(buf, 0, sizeof(buf));
    snprintk(buf, sizeof(buf), "AT+QGPSCFG=\"priority\",%d", prio);

    if (k_sem_take(&mdata.mdm_lock, MDM_CMD_TIMEOUT) != 0) {
        LOG_ERR("rx prio sem fail");
        return;
    }

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buf,
			     &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
	}

    k_sem_give(&mdata.mdm_lock);

    /* TODO Check if priority assign successful */
}

int quectel_bg95_gps_close(struct device *dev);

static int wwan_session_start(void)
{
    mdata.wwan_in_session = 1;

    //if (mdata.gps_status == 1)
    {
        /* switch to WWAN priority */
        quectel_bg95_rx_priority(WWAN_PRIORITY);

#ifdef GPS_CLOSE_ON_WWAN
        /* switch off gps? */
        (void) quectel_bg95_gps_close(NULL);
#endif
    }

    /*
     * recommended: Delay for GNNS to switch to WWAN mode
     * wait 700 msec after gps priority. Extend till 2s for safety
     */
    k_sleep(K_MSEC(2000));

    return 0;
}

/* TODO switch on gps in future? */
static int wwan_session_end(void)
{
    mdata.wwan_in_session = 0;

    /* switch to GPS priority */
    quectel_bg95_rx_priority(GPS_PRIORITY);

    return 0;
}

static int configure_ssl_ctx(void)
{
    char buf[MAX_HTTP_CMD_SIZE];
    int ret = 0;

    k_sem_take(&mdata.mdm_lock, K_FOREVER);

	memset(buf, 0, sizeof(buf));
	snprintk(buf, sizeof(buf), "AT+QSSLCFG=\"sslversion\",%d,%d", 1, 4);

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buf,
			     &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
		goto ret;
	}

	memset(buf, 0, sizeof(buf));
	snprintk(buf, sizeof(buf), "AT+QSSLCFG=\"ciphersuite\",%d,0xFFFF", 1);

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buf,
			     &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
		goto ret;
	}

    /* FIXME seclevel needs to change if certificates needs to be verified */
	memset(buf, 0, sizeof(buf));
	snprintk(buf, sizeof(buf), "AT+QSSLCFG=\"seclevel\",%d,%d", 1, 0);

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

static int check_pdp_ctx(void)
{
    struct modem_cmd cmd =
        MODEM_CMD("+QIACT: ", on_cmd_qiact, 3U, ",");
	static char* send_cmd = "AT+QIACT?";
    int ret = 0;

    if (k_sem_take(&mdata.mdm_lock, MDM_CMD_TIMEOUT) != 0) {
        LOG_ERR("check pdp sem fail");
        return -1;
    }

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, &cmd, 1U,
                send_cmd, &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(send_cmd), ret);
		goto ret;
	}

ret:
    k_sem_give(&mdata.mdm_lock);

    return ret;
}

static int configure_pdp_ctx(void)
{
	char buf[MAX_HTTP_CMD_SIZE];
    int ret = 0;

    k_sem_take(&mdata.mdm_lock, K_FOREVER);

	memset(buf, 0, sizeof(buf));
	snprintk(buf, sizeof(buf), "AT+QICSGP=%d,%d,\"%s\",\"\",\"\",1",
            1, 1, CONFIG_MODEM_QUECTEL_BG95_APN);

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

static int deactivate_pdp_ctx(void)
{
	char buf[MAX_HTTP_CMD_SIZE];
    int ret = 0;

    if (k_sem_take(&mdata.mdm_lock, MDM_CMD_TIMEOUT) != 0) {
        LOG_ERR("deactivate pdp sem fail");
        return -1;
    }

    memset(buf, 0, sizeof(buf));
	snprintk(buf, sizeof(buf), "AT+QIDEACT=%d", 1);

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buf,
			     &mdata.sem_response, MDM_REGISTRATION_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
		goto ret;
	}

    mdata.pdp_ctx = 0;
ret:
    k_sem_give(&mdata.mdm_lock);

    return ret;
}

static int activate_pdp_ctx(void)
{
	char buf[MAX_HTTP_CMD_SIZE];
    int ret = 0;

    if (k_sem_take(&mdata.mdm_lock, MDM_CMD_TIMEOUT) != 0) {
        LOG_ERR("activate pdp sem fail");
        return -1;
    }

    memset(buf, 0, sizeof(buf));
	snprintk(buf, sizeof(buf), "AT+QIACT=%d", 1);

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buf,
			     &mdata.sem_response, MDM_REGISTRATION_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
		goto ret;
	}

    mdata.pdp_ctx = 1;
ret:
    k_sem_give(&mdata.mdm_lock);

    /* HACK CHECK */
    check_pdp_ctx();

    if (ret < 0) {
        deactivate_pdp_ctx();
        check_pdp_ctx();
    }

    return ret;
}

/* ssl pdp ctx init of needed */
static int ssl_init_seq(void)
{
    int ret = 0;
    if (mdata.pdp_ctx == 0)
    {
        ret = activate_pdp_ctx();
        if (ret < 0) {
            LOG_ERR("activate pdp ctx retrying, ret:%d", ret);
            ret = activate_pdp_ctx();
            if (ret < 0) {
                LOG_ERR("activate pdp ctx, ret:%d", ret);
                errno = -ret;
                goto ret;
            }
        }

#if 0
        ret = check_pdp_ctx();
        if (ret < 0) {
            LOG_ERR("check pdp ctx, ret:%d", ret);
            goto ret;
        }
#endif

        ret = configure_ssl_ctx();
        if (ret < 0) {
            LOG_ERR("cfg ssl ctx, ret:%d", ret);
            errno = -ret;
            goto ret;
        }
    }

ret:
    return ret;
}

static int bg95_sock_close(uint8_t sock_id)
{
	char buf[sizeof("AT+QSSLCLOSE=#\r")];
    int ret = 0;
    struct modem_socket *sock;

	memset(buf, 0, sizeof(buf));

    if (k_sem_take(&mdata.mdm_lock, MDM_CMD_TIMEOUT) != 0) {
        LOG_ERR("sock close sem fail");
        return -1;
    }

    snprintk(buf, sizeof(buf), "AT+QSSLCLOSE=%d", sock_id);

    ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler,
                 NULL, 0U, buf,
                 &mdata.sem_response, MDM_CMD_TIMEOUT);
    if (ret < 0) {
        LOG_ERR("%s ret:%d", log_strdup(buf), ret);
    }

    k_sem_give(&mdata.mdm_lock);

    /* session over */
    wwan_session_end();

	sock = modem_socket_from_id(&mdata.socket_config, sock_id);
	if (!sock) {
		return -1;
	}

	modem_socket_put(&mdata.socket_config, sock->sock_fd);

    return 0;
}

static int pin_init(void)
{
	LOG_DBG("Setting Modem Pins");

	LOG_DBG("MDM_POWER_PIN -> DISABLE");
	modem_pin_write(&mctx, MDM_POWER, MDM_POWER_DISABLE);
	k_sleep(K_SECONDS(3));
	LOG_DBG("MDM_POWER_PIN -> ENABLE");
	modem_pin_write(&mctx, MDM_POWER, MDM_POWER_ENABLE);
	k_sleep(K_SECONDS(1));

	/* make sure module is powered off */
#if defined(DT_QUECTEL_BG95_0_MDM_VINT_GPIOS_CONTROLLER)
	LOG_DBG("Waiting for MDM_VINT_PIN = 0");

	do {
		k_sleep(K_MSEC(100));
	} while (modem_pin_read(&mctx, MDM_VINT) != MDM_VINT_DISABLE);
#else
	k_sleep(K_SECONDS(1));
#endif

	LOG_DBG("MDM_RESET_PIN -> DISABLE");

	unsigned int irq_lock_key = irq_lock();

	LOG_DBG("MDM_RESET_PIN -> ASSERTED");
	modem_pin_write(&mctx, MDM_RESET, MDM_RESET_ASSERTED);
	k_sleep(K_SECONDS(1));
	LOG_DBG("MDM_RESET_PIN -> NOT_ASSERTED");
	modem_pin_write(&mctx, MDM_RESET, MDM_RESET_NOT_ASSERTED);

	irq_unlock(irq_lock_key);

#if defined(DT_QUECTEL_BG95_0_MDM_VINT_GPIOS_CONTROLLER)
	LOG_DBG("Waiting for MDM_VINT_PIN = 1");
	do {
		k_sleep(K_MSEC(100));
	} while (modem_pin_read(&mctx, MDM_VINT) != MDM_VINT_ENABLE);
#else
	k_sleep(K_SECONDS(1));
#endif

    // What for?
	//modem_pin_config(&mctx, MDM_POWER, GPIO_DIR_IN);

	LOG_DBG("... Done!");

	return 0;
}

static void urc_handle_worker(struct k_work *work)
{
    int ret = 0;
    struct modem_socket* sock;

    if ((mdata.urc_status & URC_PDP_DEACT) != 0)
    {
        ret = deactivate_pdp_ctx();
        if (ret < 0) {
            LOG_ERR("deactivate pdp ctx fail");
            return;
        }
        mdata.urc_status &= ~URC_PDP_DEACT;
        ret = activate_pdp_ctx();
        if (ret < 0) {
            LOG_ERR("activate pdp ctx fail");
        }
        else {
            mdata.pdp_ctx = 1;
        }
    }
    if ((mdata.urc_status & URC_SSL_CLOSED) != 0) {
        LOG_DBG("ssl urc close");
        ret = bg95_sock_close(mdata.urc_close);
        if (ret < 0) {
            LOG_ERR("sock close fail");
            return;
        }
        mdata.urc_status &= ~URC_SSL_CLOSED;
        /* close socket coonection and reset socket */
        sock = modem_socket_from_id(&mdata.socket_config, mdata.urc_close);
        modem_socket_put(&mdata.socket_config, sock->sock_fd);
    }
}

static void modem_rssi_query_work(struct k_work *work)
{
	struct modem_cmd cmd =
		MODEM_CMD("+CSQ: ", on_cmd_atcmdinfo_rssi_csq, 2U, ",");
	static char *send_cmd = "AT+CSQ";
	int ret;

    if (k_sem_take(&mdata.mdm_lock, MDM_CMD_TIMEOUT) != 0) {
        LOG_ERR("RSSI fail");
        return;
    }

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
		//SETUP_CMD_NOHANDLE("ATH"),
		/* extended error numbers */
		SETUP_CMD_NOHANDLE("AT+CFUN=0"),
		SETUP_CMD_NOHANDLE("AT+CMEE=1"),
		SETUP_CMD_NOHANDLE("AT+QCFG=\"nwscanmode\", 1"),
		SETUP_CMD_NOHANDLE("AT+CFUN=1"),
		/* UNC messages for registration. Enable loc info as well */
		SETUP_CMD_NOHANDLE("AT+CREG=2"),
		/* HEX receive data mode */
		//SETUP_CMD_NOHANDLE("AT+UDCONF=1,1"),
		/* query modem info */
		SETUP_CMD("AT+CGMI", "", on_cmd_atcmdinfo_manufacturer, 0U, ""),
		SETUP_CMD("AT+CGMM", "", on_cmd_atcmdinfo_model, 0U, ""),
		SETUP_CMD("AT+QGMR", "", on_cmd_atcmdinfo_revision, 0U, ""),
		SETUP_CMD("AT+CGSN", "", on_cmd_atcmdinfo_imei, 1U, ""),
        /* Disable automatic NITZ updates
         * Interfering with CCLK set by QNTP?
         */
		//SETUP_CMD_NOHANDLE("AT+CTZU=0"),
	};

restart:
	/* stop RSSI delay work */
	k_delayed_work_cancel(&mdata.rssi_query_work);

	pin_init();

	LOG_DBG("Waiting for modem to respond");

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

    k_sem_give(&mdata.mdm_lock);

    /* configure pdp ctx */
    ret = configure_pdp_ctx();
    if (ret < 0) {
        LOG_ERR("failed to configure pdp ctx!");
        goto error;
    }

    k_sleep(K_SECONDS(2));

    k_sem_take(&mdata.mdm_lock, K_FOREVER);

#if 0
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

	LOG_DBG("Waiting for network");

	/*
	 * TODO: A lot of this should be setup as a 3GPP module to handle
	 * basic connection to the network commands / polling
	 */

	/* wait for +CREG: 1(normal) or 5(roaming) */
	counter = 0;
    do {
        ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0,
                     "AT+CREG?", &mdata.sem_response,
                     MDM_NETWORK_REG_TIMEOUT);
        if (ret < 0) {
            LOG_ERR("AT+CREG ret:%d", ret);
            // give semaphore
            k_sem_give(&mdata.mdm_lock);
            goto error;
        }
        k_sleep(K_SECONDS(20));
    } while (counter++ < 20 && mdata.ev_creg != 1 && mdata.ev_creg != 5);

    /* give semaphore for rssi query to work */
    k_sem_give(&mdata.mdm_lock);

	/* query modem RSSI */
	modem_rssi_query_work(NULL);
	k_sleep(MDM_WAIT_FOR_RSSI_DELAY);

	counter = 0;
	/* wait for RSSI < 0 and > -1000 */
	while (counter++ < MDM_WAIT_FOR_RSSI_COUNT &&
	       (q_ctx.data_rssi >= 0 || q_ctx.data_rssi <= -1000)) {
		modem_rssi_query_work(NULL);
		k_sleep(MDM_WAIT_FOR_RSSI_DELAY);
	}

	if (q_ctx.data_rssi >= 0 || q_ctx.data_rssi <= -1000) {
		retry_count++;
		if (retry_count >= MDM_NETWORK_RETRY_COUNT) {
			LOG_ERR("Failed network init.  Too many attempts!");
			ret = -ENETUNREACH;
			goto error;
		}

		LOG_ERR("Failed network init.  Restarting process. %d", q_ctx.data_rssi);
		goto restart;
	}

    /* **************Global variables reset**************** */
    /* reset status */
    mdata.pdp_ctx = 0;
    mdata.urc_status = 0;
    mdata.urc_close = -1;
    mdata.wwan_in_session = 0;

    /* modem gps reset */
    mdata.agps_status = 0;
    mdata.gps_status = 0;
    memset(mdata.ref_lat, 0, MDM_CD_LEN);
    memset(mdata.ref_lon, 0, MDM_CD_LEN);
    memset(mdata.gps_data, 0, MDM_GPS_DATA_LEN);

    k_sem_reset(&mdata.sem_response);
	k_sem_reset(&mdata.sem_connect);
	k_sem_reset(&mdata.sem_reply);
    /* mdm lock needs to be set to 1 */
	k_sem_reset(&mdata.mdm_lock);
    k_sem_give(&mdata.mdm_lock);


    /* **************************************************** */

	LOG_DBG("Network is ready.");

#if 0
    ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0,
                 "AT+QNVFC=\"/nv/item_files/gsm/l1/l1_debug\",01000060",
                 &mdata.sem_response,
                 MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("AT+QNVFC ret:%d", ret);
	}
#endif

	/* start RSSI query */
	k_delayed_work_submit_to_queue(&modem_workq, &mdata.rssi_query_work,
				       K_SECONDS(RSSI_TIMEOUT_SECS));

error:
	return;
}

#ifdef MODEM_BG95_SOCKET
/*
 * generic socket creation function
 * which can be called in bind() or connect()
 */
static int create_socket(struct modem_socket *sock, const struct sockaddr *addr)
{
    /* Nothing to do here */
	errno = 0;
	return 0;
}

/*
 * Socket Offload OPS
 */

static const struct socket_op_vtable offload_socket_fd_op_vtable;

static int offload_socket(int family, int type, int proto)
{
	int ret;

    /* wwan session start */
    wwan_session_start();

    /* perform modem ssl, pdp ctx initialisation */
    ret = ssl_init_seq();
    if (ret < 0) {
        goto ret;
    }

	/* defer modem's socket create call to bind() */
	ret = modem_socket_get(&mdata.socket_config, family, type, proto);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	errno = 0;
ret:
	return ret;
}

static int offload_close(void *obj)
{
    struct modem_socket *sock = (struct modem_socket *)obj;
	int ret;

	/* make sure we assigned an id */
	if (sock->id < mdata.socket_config.base_socket_num) {
		return 0;
	}

    /*
     * FIXME Sometimes in timedout cases is connected might
     * not be true, but still requires close. HACK for now
     * In future need to check for sock->is_connected flag
	 * if (sock->is_connected) {}
     */
    ret = bg95_sock_close(sock->id);
    if (ret < 0) {
        LOG_ERR("sock close, ret:%d", ret);
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
	if (sock->id == mdata.socket_config.sockets_len + 1) {
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
	char buf[sizeof("AT+QSSLOPEN=##,#,##,#################,#####,#\r")];
	uint16_t dst_port = 0U;
    char ip_str[NET_IPV6_ADDR_LEN];

	if (!addr) {
		errno = EINVAL;
		return -1;
	}

	if (sock->id < mdata.socket_config.base_socket_num - 1) {
		LOG_ERR("Invalid socket_id(%d) from fd:%d",
			sock->id, sock->sock_fd);
		errno = EINVAL;
		return -1;
	}

	/* make sure we've created the socket */
	if (sock->id == mdata.socket_config.sockets_len + 1) {
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

	/* TODO skip socket connect if UDP */
	if (sock->ip_proto == IPPROTO_UDP) {
		errno = 0;
		return 0;
	}

    k_sem_take(&mdata.mdm_lock, K_FOREVER);

    ret = modem_context_sprint_ip_addr(addr, ip_str, sizeof(ip_str));
	if (ret != 0) {
		errno = -ret;
		LOG_ERR("Error formatting IP string %d", ret);
		return -1;
	}

    /* reset sem_reply so that previous spurious gives dont affect this */
    k_sem_reset(&mdata.sem_reply);

    /* 0 - Buffer Access mode */
    /* 1 - Direct Push mode */
    snprintk(buf, sizeof(buf), "AT+QSSLOPEN=1,1,%u,\"%s\",%u,%u", sock->id,
		 ip_str, dst_port, 0);
	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler,
			     NULL, 0U, buf,
			     &mdata.sem_response, MDM_CMD_CONN_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
		errno = -ret;
        ret = -1;
		goto re;
	}

    if (k_sem_take(&mdata.sem_reply, MDM_CMD_CONN_TIMEOUT) != 0) {
        ret = -ETIMEDOUT;
        errno = -ret;
        goto re;
    }

    if (open_sock_err != 0) {
        ret = -EIO;
        errno = -ret;
        goto re;
    }

	sock->is_connected = true;
	errno = 0;
    ret = 0;

re:
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
            LOG_ERR("poll err obj NULL");
			return -1;
		}
	}

	return modem_socket_poll(&mdata.socket_config, fds, nfds, msecs);
}

/* TODO UDP not supported */
static ssize_t offload_recvfrom(void *obj, void *buf, size_t len,
				int flags, struct sockaddr *from,
				socklen_t *fromlen)
{
	struct modem_socket *sock = (struct modem_socket *)obj;
	int ret, rd_len;
	struct modem_cmd cmd[] = {
		MODEM_CMD_DIRECT("+QSSLRECV: ", on_cmd_sockreadfrom),
	};
	char sendbuf[sizeof("AT+QSSLRECV=##,#####\r")];
	struct socket_read_data sock_data;

	if (!buf || len == 0) {
		errno = EINVAL;
		return -1;
	}

    if (sock->id < 0) {
        errno = EBADF;
        return -1;
    }

	if (flags & MSG_PEEK) {
		errno = ENOTSUP;
		return -1;
	}

    LOG_DBG("urc stat: %x", mdata.urc_status);
    if ((mdata.urc_status & URC_SSL_RECV) == 0) {
        if ((flags & MSG_DONTWAIT) != 0) {
            errno = EWOULDBLOCK;
            return -1;
        }

        /* Only block when no data and MSG_DONTWAIT is not set */
        if (modem_socket_wait_data(&mdata.socket_config, sock) != 0)
        {
            LOG_ERR("RECV timeout");
            errno = ETIMEDOUT;
            return -1;
        }
    }

	/*
	 * Binary and ASCII mode allows sending MDM_MAX_DATA_LENGTH bytes to
	 * the socket in one command
	 */
    rd_len = (len > MDM_MAX_DATA_LENGTH) ?
                MDM_MAX_DATA_LENGTH : len;

	snprintk(sendbuf, sizeof(sendbuf), "AT+QSSLRECV=%d,%d",
            sock->id, rd_len);

    k_sem_take(&mdata.mdm_lock, K_FOREVER);

	/* socket read settings */
	(void)memset(&sock_data, 0, sizeof(sock_data));
	sock_data.recv_buf = buf;
	sock_data.recv_buf_len = len;
    /* important if there is no data, this serves as 0 */
	sock_data.recv_read_len = 0;
	sock_data.recv_addr = from;
	sock->data = &sock_data;
    current_sock_rd_id = sock->id;

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler,
			     cmd, ARRAY_SIZE(cmd), sendbuf, &mdata.sem_response,
			     MDM_CMD_TIMEOUT);
	if (ret < 0) {
        /* TODO Check why this is needed for error*/
        mdata.urc_status &= ~URC_SSL_RECV;
		errno = -ret;
		ret = -1;
		goto exit;
	}

	/* HACK: use dst address as from */
	if (from && fromlen) {
		*fromlen = sizeof(sock->dst);
		memcpy(from, &sock->dst, *fromlen);
	}

    /* read all the socket data, reset SSL RECV flag */
    if (sock_data.recv_read_len < rd_len) {
        mdata.urc_status &= ~URC_SSL_RECV;
    }

	/* return length of received data */
	errno = 0;
	ret = sock_data.recv_read_len;

exit:
    k_sem_give(&mdata.mdm_lock);

	/* clear socket data */
	sock->data = NULL;
	return ret;
}

/* send binary data via the +QSSLSEND commands */
static ssize_t send_socket_data(struct modem_socket *sock,
				const struct sockaddr *dst_addr,
				struct modem_cmd *handler_cmds,
				size_t handler_cmds_len,
				const char *buf, size_t buf_len,
				k_timeout_t timeout)
{
	int ret;
	char send_buf[sizeof("AT+QSSLSEND=##,#####\r\n")];

	if (!sock) {
		return -EINVAL;
	}

	/*
	 * Binary and ASCII mode allows sending MDM_MAX_SEND_DATA_LEN bytes to
	 * the socket in one command
	 */
	if (buf_len > MDM_MAX_SEND_DATA_LEN) {
		buf_len = MDM_MAX_SEND_DATA_LEN;
	}

	/* The number of bytes written will be buf_len */
	mdata.sock_written = buf_len;

	if (sock->ip_proto == IPPROTO_UDP) {
        LOG_ERR("UDP not supported yet");
        ret = -ENOTSUP;
	} else {
		snprintk(send_buf, sizeof(send_buf), "AT+QSSLSEND=%d,%zu",
			 sock->id, buf_len);
	}

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

	/* slight pause per spec so that > prompt is received */
	k_sleep(MDM_PROMPT_CMD_DELAY);
	mctx.iface.write(&mctx.iface, buf, buf_len);

	if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		ret = 0;
		goto exit;
	}

	k_sem_reset(&mdata.sem_response);
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
        LOG_ERR("ret: %d", ret);
		return ret;
	}

	return mdata.sock_written;
}

static ssize_t offload_sendto(void *obj, const void *buf, size_t len,
			      int flags, const struct sockaddr *to,
			      socklen_t tolen)
{
	int ret;
	struct modem_socket *sock = (struct modem_socket *)obj;
	struct modem_cmd cmd[] = {
		MODEM_CMD("SEND OK", on_cmd_sockwrite, 0U, ""),
	};

	if (!buf || len == 0) {
		errno = EINVAL;
		return -1;
	}

	if (!sock->is_connected && sock->ip_proto != IPPROTO_UDP) {
		errno = ENOTCONN;
		return -1;
	}

	if (!to && sock->ip_proto == IPPROTO_UDP) {
		to = &sock->dst;
	}

    k_sem_take(&mdata.mdm_lock, K_FOREVER);

	ret = send_socket_data(sock, to, cmd, ARRAY_SIZE(cmd), buf, len,
			       MDM_CMD_TIMEOUT);
	if (ret < 0) {
		errno = -ret;
        ret = -1;
		goto exit;
	}

exit:
    k_sem_give(&mdata.mdm_lock);
	errno = 0;
	return ret;
}

static int offload_ioctl(void *obj, unsigned int request, va_list args)
{
	switch (request) {
	case ZFD_IOCTL_POLL_PREPARE:
		return -EXDEV;

	case ZFD_IOCTL_POLL_UPDATE:
		return -EOPNOTSUPP;

	case ZFD_IOCTL_POLL_OFFLOAD: {
		struct zsock_pollfd *fds;
		int nfds;
		int timeout;

		fds = va_arg(args, struct zsock_pollfd *);
		nfds = va_arg(args, int);
		timeout = va_arg(args, int);

		return offload_poll(fds, nfds, timeout);
	}

	default:
		errno = EINVAL;
		return -1;
	}
}

static ssize_t offload_read(void *obj, void *buffer, size_t count)
{
	return offload_recvfrom(obj, buffer, count, 0, NULL, 0);
}

static ssize_t offload_write(void *obj, const void *buffer, size_t count)
{
	return offload_sendto(obj, buffer, count, 0, NULL, 0);
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
	.sendmsg = NULL,
	.getsockopt = NULL,
	.setsockopt = NULL,
};

static bool offload_is_supported(int family, int type, int proto)
{
	/* TODO offloading always enabled for now. */
	return true;
}

NET_SOCKET_OFFLOAD_REGISTER(quectel_bg95, CONFIG_NET_SOCKETS_OFFLOAD_PRIORITY,
        AF_UNSPEC, offload_is_supported, offload_socket);

///////////////////////////////////////////////////////////////
#if defined(CONFIG_DNS_RESOLVER)
/* TODO: This is a bare-bones implementation of DNS handling
 * We ignore most of the hints like ai_family, ai_protocol and ai_socktype.
 * Later, we can add additional handling if it makes sense.
 */
static int offload_getaddrinfo(const char *node, const char *service,
			       const struct addrinfo *hints,
			       struct addrinfo **res)
{
	uint32_t port = 0U;
	int ret;
	/* DNS command + 128 bytes for domain name parameter */
	char sendbuf[sizeof("AT+QIDNSGIP=##,'[]'\r") + 128];

    /* perform modem ssl, pdp ctx initialisation if needed */
    ret = ssl_init_seq();
    if (ret < 0) {
        return ret;
    }

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

	/* check to see if node is an IP address */
	if (net_addr_pton(result.ai_family, node,
			  &((struct sockaddr_in *)&result_addr)->sin_addr)
	    == 1) {
		*res = &result;
		return 0;
	}

	/* user flagged node as numeric host, but we failed net_addr_pton */
	if (hints && hints->ai_flags & AI_NUMERICHOST) {
		return EAI_NONAME;
	}

	if (service) {
		port = ATOI(service, 0U, "port");
		if (port < 1 || port > USHRT_MAX) {
			return EAI_SERVICE;
		}
	}

    /* reset sem_reply semapahore just in case */
    k_sem_reset(&mdata.sem_reply);

	snprintk(sendbuf, sizeof(sendbuf), "AT+QIDNSGIP=1,\"%s\"", node);
	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler,
			     NULL, 0U, sendbuf, &mdata.sem_response,
			     MDM_CMD_TIMEOUT);
	if (ret < 0) {
		return ret;
	}

    /* wait for sem reply */
    if (k_sem_take(&mdata.sem_reply, MDM_DNS_TIMEOUT) != 0) {
        LOG_ERR("DNS timeout");
        return -1;
    }

	if (port > 0U) {
		/* FIXME: DNS is hard-coded to return only IPv4 */
		if (result.ai_family == AF_INET) {
			net_sin(&result_addr)->sin_port = htons(port);
		}
	}

	LOG_DBG("DNS RESULT: %s",
		log_strdup(net_addr_ntop(result.ai_family,
					 &net_sin(&result_addr)->sin_addr,
					 sendbuf, NET_IPV4_ADDR_LEN)));

	*res = (struct addrinfo *)&result;
	return 0;
}

static void offload_freeaddrinfo(struct addrinfo *res)
{
	/* using static result from offload_getaddrinfo() -- no need to free */
	res = NULL;
}

const struct socket_dns_offload offload_dns_ops = {
	.getaddrinfo = offload_getaddrinfo,
	.freeaddrinfo = offload_freeaddrinfo,
};
#endif
//////////////////////////////////////////////////////////////
#endif

static int net_offload_dummy_get(sa_family_t family, enum net_sock_type type,
				 enum net_ip_protocol ip_proto,
				 struct net_context **context)
{
	LOG_DBG("CONFIG_NET_SOCKET_OFFLOAD must be configured for this driver");

	return -ENOTSUP;
}

/* placeholders, until Zepyr IP stack updated to handle a NULL net_offload */
static struct net_offload modem_net_offload = {
	.get = net_offload_dummy_get,
};

int quectel_bg95_get_ntp_time(struct device *dev)
{
    if (mdata.pdp_ctx == 0) {
        LOG_ERR("ctx not yet activated");
        return -1;
    }

	char buf[sizeof("AT+QNTP=1\r") + 64];
	int ret = 0;

	memset(buf, 0, sizeof(buf));
    // IP addr time.google.com : "216.239.35.0"
	snprintk(buf, sizeof(buf), "AT+QNTP=1,\"%s\",%d", "time.google.com", 123);

    /* FIXME Find a common solution for all locks */
    ret = k_sem_take(&mdata.mdm_lock, MDM_LOCK_TIMEOUT);
    if (ret != 0)
    {
        return ret;
    }

    /* reset sem_reply so that previous spurious gives dont affect this */
    k_sem_reset(&mdata.sem_reply);

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buf,
			     &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
		goto ret;
	}

    if (k_sem_take(&mdata.sem_reply, MDM_CMD_CONN_TIMEOUT) != 0) {
        LOG_ERR("sem_reply ntp timed out");
        goto ret;
    }

ret:
    k_sem_give(&mdata.mdm_lock);

    LOG_DBG("ntp stat: %d", mdata.ntp_status);
    /* Restart pdp ctx on DNS error */
    if (mdata.ntp_status == 565) {
        LOG_DBG("pdp ctx re-activate");
        deactivate_pdp_ctx();
        activate_pdp_ctx();
        /* reset ntp status */
        mdata.ntp_status = 0;
    }

	return ret;
}


int quectel_bg95_get_clock(struct device *dev, char *timeval)
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
			     &mdata.sem_response, MDM_LOCK_TIMEOUT);
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

int quectel_bg95_http_init(struct device *dev, struct usr_http_cfg *cfg)
{
	char buf[MAX_HTTP_CMD_SIZE];
	int ret = 0;

    /* perform modem ssl, pdp ctx initialisation */
    ret = ssl_init_seq();
    if (ret < 0) {
        goto ret;
    }

    k_sem_take(&mdata.mdm_lock, K_FOREVER);

    memset(buf, 0, sizeof(buf));
	snprintk(buf, sizeof(buf), "AT+QHTTPCFG=\"contextid\",%d", 1);

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buf,
			     &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
		goto ret;
	}

	memset(buf, 0, sizeof(buf));
	snprintk(buf, sizeof(buf), "AT+QHTTPCFG=\"sslctxid\",%d", 1);

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buf,
			     &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
		goto ret;
	}

ret:
    k_sem_give(&mdata.mdm_lock);

    /* session start */
    wwan_session_start();

	return ret;
}

int quectel_bg95_http_term(struct device *dev, struct usr_http_cfg *cfg)
{
	int ret = 0;

    k_sem_take(&mdata.mdm_lock, K_FOREVER);

    /* TODO */
//ret:
    k_sem_give(&mdata.mdm_lock);

	return ret;
}

int quectel_bg95_http_execute(struct device *dev, struct usr_http_cfg *cfg)
{
	int ret = 0;
    struct modem_cmd cmd = MODEM_CMD_DIRECT("CONNECT", on_cmd_http_read_con);

    if (cfg->url == NULL) {
        return -EINVAL;
    }

    if (cfg->method == HTTP_POST) {
		if (cfg->content_type == NULL ||
		    cfg->content_body == NULL) {
			return -EINVAL;
		}
    }

    /* set http config data */
    mdata.recv_cfg.recv_buf = cfg->recv_buf;
    mdata.recv_cfg.recv_buf_len = cfg->recv_buf_len;

    memset(mdata.send_buf, 0, sizeof(mdata.send_buf));
    snprintk(mdata.send_buf, sizeof(mdata.send_buf),
         "AT+QHTTPURL=%d,%d", strlen(cfg->url),
         HTTP_TIMEOUT_SECS);

    k_sem_take(&mdata.mdm_lock, K_FOREVER);

    /* Do not wait for OK, Wait for CONNECT which is the next output and then input URL */
    ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U,
                 mdata.send_buf, &mdata.sem_connect,
                 MDM_CMD_TIMEOUT);
    if (ret < 0) {
        LOG_ERR("%s ret:%d", log_strdup(cfg->url), ret);
        goto ret;
    }

    memset(mdata.send_buf, 0, sizeof(mdata.send_buf));
    snprintk(mdata.send_buf, sizeof(mdata.send_buf),
         "%s", cfg->url);

    /* Wait for OK after inputting URL */
    ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U,
                 mdata.send_buf, &mdata.sem_response,
                 MDM_CMD_TIMEOUT);
    if (ret < 0) {
        LOG_ERR("%s ret:%d", log_strdup(cfg->url), ret);
        goto ret;
    }

    /* reset sem reply semaphore */
    k_sem_reset(&mdata.sem_reply);

	switch (cfg->method) {
	case HTTP_GET:
        /* reset and send next cmd */
		memset(mdata.send_buf, 0, sizeof(mdata.send_buf));
		snprintk(mdata.send_buf, sizeof(mdata.send_buf),
			 "AT+QHTTPGET=%d", cfg->timeout);

        /* Wait for OK after HTTPGET */
		ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U,
				     mdata.send_buf, &mdata.sem_response,
				     MDM_CMD_TIMEOUT);
		if (ret < 0) {
			LOG_ERR("%s ret:%d", log_strdup(cfg->url), ret);
			goto ret;
		}
		break;

	case HTTP_POST:
        /* reset and send next cmd */
		memset(mdata.send_buf, 0, sizeof(mdata.send_buf));
		snprintk(mdata.send_buf, sizeof(mdata.send_buf),
			 "AT+QHTTPOST=%d,%d,%d", strlen(cfg->content_body),
			 HTTP_TIMEOUT_SECS, cfg->timeout);

        /*
         * Do not wait for OK, wait for CONNECT which is the next
         * output and then input data
         */
		ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U,
				     mdata.send_buf, &mdata.sem_connect,
				     MDM_CMD_TIMEOUT);
		if (ret < 0) {
			LOG_ERR("%s ret:%d", log_strdup(cfg->url), ret);
			goto ret;
		}

		memset(mdata.send_buf, 0, sizeof(mdata.send_buf));
		snprintk(mdata.send_buf, sizeof(mdata.send_buf),
			 "%s,", cfg->content_body);

        /* Wait for OK after inputting body */
		ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U,
				     mdata.send_buf, &mdata.sem_response,
				     MDM_CMD_TIMEOUT);
		if (ret < 0) {
			LOG_ERR("%s ret:%d", log_strdup(cfg->url), ret);
			goto ret;
		}

        break;

	default:
		LOG_ERR("Currently not supported");
		return -ENOTSUP;
	}

    /* wait for response */
    if (k_sem_take(&mdata.sem_reply, K_SECONDS(cfg->timeout)) != 0) {
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

    if (mdata.recv_cfg.recv_status != 0) {
        ret = -1;
        LOG_ERR("http recv, ret: %d", ret);
        goto ret;
    }

    /*
     * TODO Make sure new http req is not sent immediately after
     * and within HTTP_TIMEOUT_SECS, otherwise we shall get DNS
     * parse failed error
     */
    if (cfg->resp_filename == NULL)
    {
        memset(mdata.send_buf, 0, sizeof(mdata.send_buf));
        snprintk(mdata.send_buf, sizeof(mdata.send_buf),
             "AT+QHTTPREAD=%d", HTTP_TIMEOUT_SECS);

        mdata.recv_cfg.http_cfg.http_pending = 1;

        /* Data then wait for OK which releases sem response */
        ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, &cmd, 1U,
                     mdata.send_buf, &mdata.sem_response,
                     MDM_CMD_TIMEOUT);
        if (ret < 0) {
            LOG_ERR("%s ret:%d", log_strdup(cfg->url), ret);
            goto ret;
        }

        LOG_DBG("http resp done");
        mdata.recv_cfg.http_cfg.http_pending = 0;

        /* received data len */
        cfg->recv_read_len = mdata.recv_cfg.recv_read_len;
    }
    else
    {
        memset(mdata.send_buf, 0, sizeof(mdata.send_buf));

        snprintk(mdata.send_buf, sizeof(mdata.send_buf),
             "AT+QHTTPREADFILE=\"%s\",%d", cfg->resp_filename,
             HTTP_TIMEOUT_SECS);

        /* reset sem reply semaphore */
        k_sem_reset(&mdata.sem_reply);

        /* wait for OK */
        ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U,
                     mdata.send_buf, &mdata.sem_response,
                     MDM_CMD_TIMEOUT);
        if (ret < 0) {
            LOG_ERR("%s ret:%d", log_strdup(cfg->url), ret);
            goto ret;
        }
    }

    /* wait for +QHTTPREAD / +QHTTPREADFILE */
    if (k_sem_take(&mdata.sem_reply, K_SECONDS(cfg->timeout)) != 0) {
        LOG_ERR("No http read resp in %d ms", cfg->timeout);
        ret = -EIO;
        goto ret;
    }

    if (mdata.recv_cfg.http_cfg.http_rd_err != 0) {
        LOG_ERR("HTTP read err: %d", mdata.recv_cfg.http_cfg.http_rd_err);
    }

    /* set http config data */
    mdata.recv_cfg.expected_len = 0;
    mdata.recv_cfg.recv_buf = NULL;
    mdata.recv_cfg.recv_buf_len = 0;
    mdata.recv_cfg.recv_read_len = 0;

ret:
    k_sem_give(&mdata.mdm_lock);

    /* session over */
    wwan_session_end();

	return ret;
}

int quectel_bg95_gps_init(struct device *dev, struct usr_gps_cfg *cfg)
{
	char buf[64];
	int ret = 0;

    LOG_DBG("QGPS switching on");

    /* GPS priority */
    quectel_bg95_rx_priority(GPS_PRIORITY);

    ret = k_sem_take(&mdata.mdm_lock, MDM_LOCK_TIMEOUT);
    if (ret != 0)
    {
        /* init lock timeout */
        return ret;
    }

    memset(buf, 0, sizeof(buf));
    snprintk(buf, sizeof(buf), "AT+QGPS=1");

    ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buf,
                 &mdata.sem_response, MDM_CMD_TIMEOUT);
    if (ret < 0) {
        LOG_ERR("%s ret:%d", log_strdup(buf), ret);
        goto ret;
    }

    mdata.gps_status = 1;

ret:
    k_sem_give(&mdata.mdm_lock);

	return ret;
}

#ifdef QUECTEL_BG96
/* need to call gps_init after agps */
int quectel_bg95_agps(struct device *dev, struct usr_gps_cfg *cfg)
{
	char buf[64];
	int ret = 0;

    if (cfg->agps_filename == NULL || cfg->utc_time == NULL)
    {
        return -EINVAL;
    }

    ret = k_sem_take(&mdata.mdm_lock, MDM_LOCK_TIMEOUT);
    if (ret != 0)
    {
        return ret;
    }

	memset(buf, 0, sizeof(buf));
	snprintk(buf, sizeof(buf), "AT+QGPSXTRA=1");

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buf,
			     &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
		goto ret;
	}

    memset(buf, 0, sizeof(buf));
	snprintk(buf, sizeof(buf), "AT+QGPSXTRATIME=0,\"%s\",1,0,5", cfg->utc_time);

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buf,
			     &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
		goto ret;
	}

    memset(buf, 0, sizeof(buf));
	snprintk(buf, sizeof(buf), "AT+QGPSXTRADATA=\"%s\"", cfg->agps_filename);

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buf,
			     &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
		goto ret;
	}

    memset(buf, 0, sizeof(buf));
	snprintk(buf, sizeof(buf), "AT+QFDEL=\"%s\"", cfg->agps_filename);

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buf,
			     &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
		goto ret;
	}

    if (cfg != NULL)
    {
        cfg->agps_status = 1;
    }
    mdata.agps_status = 1;

ret:
    k_sem_give(&mdata.mdm_lock);

	return ret;
}
#else
/* need to call gps_init after agps */
int quectel_bg95_agps(struct device *dev, struct usr_gps_cfg *cfg)
{
	char buf[64];
	int ret = 0;

    ret = k_sem_take(&mdata.mdm_lock, MDM_LOCK_TIMEOUT);
    if (ret != 0)
    {
        return ret;
    }

    /* TODO use xtra_info */
    memset(buf, 0, sizeof(buf));
	snprintk(buf, sizeof(buf), "AT+QGPSCFG=\"xtra_info\"");

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buf,
			     &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
		goto ret;
	}

	memset(buf, 0, sizeof(buf));
	snprintk(buf, sizeof(buf), "AT+QGPSCFG=\"xtrafilesize\"");

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buf,
			     &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
		goto ret;
	}

	memset(buf, 0, sizeof(buf));
	snprintk(buf, sizeof(buf), "AT+QGPSXTRA=1");

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buf,
			     &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
		goto ret;
	}

    if (cfg != NULL)
    {
        cfg->agps_status = 1;
    }
    mdata.agps_status = 1;

    /* NOTE: Needs modem reboot to start agps. Application needs to care of this. */

ret:
    k_sem_give(&mdata.mdm_lock);

	return ret;
}
#endif

int quectel_bg95_gps_read(struct device *dev, struct usr_gps_cfg *cfg)
{
	char buf[sizeof("AT+QGPSLOC=#,#############################\r")];
	int ret = 0;

    /*
     * Or use AT+QGPSGNMEA=“GGA” ?
     */

    /* HACK FIXME Check +QGPS? instead of static variable mdata.gps_status */
    /* if wwan in session, the HW is busy */
    if (mdata.gps_status == 0 && mdata.wwan_in_session == 1) {
        return -EBUSY;
    }
    else if (mdata.gps_status == 0) {
        /* open gps */
        ret = quectel_bg95_gps_init(dev, cfg);
        if (ret < 0) {
            LOG_ERR("GPS init: %d", ret);
            goto ret;
        }
    }

	memset(buf, 0, sizeof(buf));
	snprintk(buf, sizeof(buf), "AT+QGPSLOC=0");
	//snprintk(buf, sizeof(buf), "AT+QGPSGNMEA=\"GSV\"");

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
    strcpy(cfg->gps_data, mdata.gps_data);

ret:
    k_sem_give(&mdata.mdm_lock);

	return ret;
}

int quectel_bg95_gps_close(struct device *dev)
{
	char buf[sizeof("AT+QGPSEND\r")];
	int ret = 0;

	memset(buf, 0, sizeof(buf));
	snprintk(buf, sizeof(buf), "AT+QGPSEND");

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

    mdata.gps_status = 0;

ret:
    k_sem_give(&mdata.mdm_lock);

	return ret;
}

int quectel_bg95_get_cell_info(struct device *dev, char **cell_info)
{
    struct modem_cmd cmd =
		MODEM_CMD("+QENG: ", on_cmd_qeng, 0U, "");
    char buf[sizeof("AT+QENG=\"neighbourcell\"****")];
	int ret = 0;

    /* AT+QENG="neighbourcell" */
	memset(buf, 0, sizeof(buf));
	snprintk(buf, sizeof(buf), "AT+QENG=\"neighbourcell\"");

    /* reset cell info idx */
    cinfo_idx = 0;

    ret = k_sem_take(&mdata.mdm_lock, MDM_LOCK_TIMEOUT);
    if (ret != 0)
    {
        return ret;
    }

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, &cmd, 1U,
                buf, &mdata.sem_response, MDM_LOCK_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
		goto ret;
	}
 
    /* q_ctx has the context */
    *cell_info = &q_ctx.data_cellinfo;
ret:
    k_sem_give(&mdata.mdm_lock);

    return ret;
}

int quectel_bg95_get_ctx(struct device *dev, struct mdm_ctx **ctx)
{
    struct modem_cmd cmd =
		MODEM_CMD("+QENG: ", on_cmd_qeng, 0U, "");
    char buf[sizeof("AT+QENG=\"neighbourcell\"****")];
	int ret = 0;

    /* AT+QENG="neighbourcell" */
	memset(buf, 0, sizeof(buf));
	snprintk(buf, sizeof(buf), "AT+QENG=\"neighbourcell\"");

    /* reset cell info idx */
    cinfo_idx = 0;

    ret = k_sem_take(&mdata.mdm_lock, MDM_LOCK_TIMEOUT);
    if (ret != 0)
    {
        return ret;
    }

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, &cmd, 1U,
                buf, &mdata.sem_response, MDM_LOCK_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
		goto ret;
	}
 
    /* q_ctx has the context */
    *ctx = &q_ctx;
ret:
    k_sem_give(&mdata.mdm_lock);

    return ret;
}

#define HASH_MULTIPLIER 37
static uint32_t hash32(char *str, int len)
{
	uint32_t h = 0;
	int i;

	for (i = 0; i < len; ++i) {
		h = (h * HASH_MULTIPLIER) + str[i];
	}

	return h;
}

static inline uint8_t *modem_get_mac(struct device *dev)
{
	struct modem_data *data = dev->data;
	uint32_t hash_value;

	data->mac_addr[0] = 0x00;
	data->mac_addr[1] = 0x10;

	/* use IMEI for mac_addr */
	hash_value = hash32(q_ctx.data_imei, strlen(q_ctx.data_imei));

	UNALIGNED_PUT(hash_value, (uint32_t *)(data->mac_addr + 2));

	return data->mac_addr;
}

static void modem_net_iface_init(struct net_if *iface)
{
	struct device *dev = net_if_get_device(iface);
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
}

#ifdef CONFIG_QUECTEL_BG95_FILE_OPS
int quectel_bg95_fopen(struct device *dev,
                const char *file)
{
    struct modem_cmd cmd =
		MODEM_CMD("+QFOPEN: ", on_cmd_qfopen, 1U, "");

    char buf[sizeof("AT+QFOPEN=") + 128];
	int ret = 0;

    k_sem_take(&mdata.mdm_lock, K_FOREVER);

	memset(buf, 0, sizeof(buf));
	snprintk(buf, sizeof(buf), "AT+QFOPEN=\"%s\"", file);

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, &cmd, 1U,
                buf, &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
		goto ret;
	}

    ret = mdata.fops.open_fd;

ret:
    k_sem_give(&mdata.mdm_lock);

	return ret;
}

int quectel_bg95_fread(struct device *dev,
                int fd, uint8_t* buf, size_t len)
{
    struct modem_cmd cmd =
		MODEM_CMD_DIRECT("CONNECT ", on_cmd_qfread);

    char send_cmd[sizeof("AT+QFREAD=##,#####")];
	int ret = 0;

	memset(send_cmd, 0, sizeof(send_cmd));
	snprintk(send_cmd, sizeof(send_cmd), "AT+QFREAD=%d,%u", fd, len);

    k_sem_take(&mdata.mdm_lock, K_FOREVER);

    /* read buf len */
    mdata.fops.rd_buf_sz = len;
    mdata.fops.rw_buf = buf;

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, &cmd, 1U,
                send_cmd, &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
		goto ret;
	}

    /* return actual read size */
    ret = mdata.fops.act_rd_sz;

ret:
    k_sem_give(&mdata.mdm_lock);

	return ret;
}

int quectel_bg95_fwrite(struct device *dev,
                int fd, uint8_t* buf, size_t len)
{
    char send_cmd[sizeof("AT+QFWRITE=##,#####")];
	int ret = 0;

	memset(send_cmd, 0, sizeof(send_cmd));
	snprintk(send_cmd, sizeof(send_cmd), "AT+QFWRITE=%d,%u",
            fd, len);

    k_sem_take(&mdata.mdm_lock, K_FOREVER);

    /* expected write len */
    mdata.fops.exp_wr_sz = len;

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U,
                send_cmd, &mdata.sem_connect, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
		goto ret;
	}

    /* write file data */
    mctx.iface.write(&mctx.iface, buf, len);

    /* reset sem_response so that previous gives dont affect this */
    k_sem_reset(&mdata.sem_response);

    if (k_sem_take(&mdata.sem_response, MDM_CMD_CONN_TIMEOUT) != 0) {
        LOG_ERR("No reponse after file write from modem");
        ret = -EIO;
        goto ret;
    }

    /* return actual written size */
    ret = mdata.fops.act_wr_sz;

ret:
    k_sem_give(&mdata.mdm_lock);

	return ret;

}

int quectel_bg95_fseek(struct device *dev,
                int fd, size_t off)
{
    char buf[sizeof("AT+QFSEEK=##,#####,#####")];
	int ret = 0;

	memset(buf, 0, sizeof(buf));
	snprintk(buf, sizeof(buf), "AT+QFSEEK=%d,%d,0", fd, off);

    k_sem_take(&mdata.mdm_lock, K_FOREVER);

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U,
                buf, &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
		goto ret;
	}

ret:
    k_sem_give(&mdata.mdm_lock);

	return ret;
}

int quectel_bg95_fclose(struct device *dev,
                int fd)
{
    char buf[sizeof("AT+QFCLOSE=##")];
	int ret = 0;

	memset(buf, 0, sizeof(buf));
	snprintk(buf, sizeof(buf), "AT+QFCLOSE=%d", fd);

    k_sem_take(&mdata.mdm_lock, K_FOREVER);

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U,
                buf, &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
		goto ret;
	}

ret:
    k_sem_give(&mdata.mdm_lock);

	return ret;
}

int quectel_bg95_fdel(struct device *dev,
                const char* fname)
{
    char buf[sizeof("AT+QFDEL=") + 128];
	int ret = 0;

	memset(buf, 0, sizeof(buf));
	snprintk(buf, sizeof(buf), "AT+QFDEL=\"%s\"", fname);

    k_sem_take(&mdata.mdm_lock, K_FOREVER);

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U,
                buf, &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
		goto ret;
	}

ret:
    k_sem_give(&mdata.mdm_lock);

	return ret;
}

/* TODO Declare a stat struct id file can have diff attributes */
int quectel_bg95_fstat(struct device *dev,
                const char* fname, size_t* f_sz)
{
    struct modem_cmd cmd =
		MODEM_CMD("+QFLST: ", on_cmd_qflst, 2U, ",");
    char buf[sizeof("AT+QFLST=") + 128];
	int ret = 0;

	memset(buf, 0, sizeof(buf));
	snprintk(buf, sizeof(buf), "AT+QFLST=\"%s\"", fname);

    k_sem_take(&mdata.mdm_lock, K_FOREVER);

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, &cmd, 1U,
                buf, &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
		goto ret;
	}

    *f_sz = mdata.fops.fsize;

ret:
    k_sem_give(&mdata.mdm_lock);

	return ret;
}

#endif

#ifdef CONFIG_QUECTEL_BG95_DFOTA
/* HTTP URL FMT: {"HTTP://<HTTP_server_URL>:<HTTP_port>/<HTTP_file_path>"} */
int quectel_bg95_dfota(struct device *dev,
        const char* url)
{
    struct modem_cmd cmd =
		MODEM_CMD("+QIND: ", on_cmd_qind, 2U, ",");
    char buf[sizeof("AT+QFOTADL=") + 256];
	int ret = 0;

	memset(buf, 0, sizeof(buf));
	snprintk(buf, sizeof(buf), "AT+CSQ;+CEREG?;+CGREG?;+COPS?");

    k_sem_take(&mdata.mdm_lock, K_FOREVER);

    ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, &cmd, 1U,
                buf, &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
		goto ret;
	}

	memset(buf, 0, sizeof(buf));
	snprintk(buf, sizeof(buf), "AT+QFOTADL=?");

    ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, &cmd, 1U,
                buf, &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
		goto ret;
	}

	memset(buf, 0, sizeof(buf));
	snprintk(buf, sizeof(buf), "AT+QFOTADL=\"%s\"", url);

    /* reset sem_reply so that previous spurious gives dont affect this */
    k_sem_reset(&mdata.sem_reply);

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, &cmd, 1U,
                buf, &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
		goto ret;
	}

    /* Wait for sem reply */
    if (k_sem_take(&mdata.sem_reply, MDM_DFOTA_TIMEOUT) != 0) {
        LOG_ERR("DFOTA update fail");
        ret = -1;
    }
    /* TODO */
ret:
    k_sem_give(&mdata.mdm_lock);

	return ret;

}
#endif

static struct modem_quectel_bg95_net_api api_funcs = {
	.net_api =
		{
			.init = modem_net_iface_init,
		},
	.get_clock = quectel_bg95_get_clock,
	.get_ntp_time = quectel_bg95_get_ntp_time,
	.http_init = quectel_bg95_http_init,
	.http_execute = quectel_bg95_http_execute,
	.http_term = quectel_bg95_http_term,
	.gps_init = quectel_bg95_gps_init,
	.gps_agps = quectel_bg95_agps,
	.gps_read = quectel_bg95_gps_read,
	.gps_close = quectel_bg95_gps_close,
	.get_ctx = quectel_bg95_get_ctx,
	.get_cell_info = quectel_bg95_get_cell_info,
#ifdef CONFIG_QUECTEL_BG95_FILE_OPS
    .fopen = quectel_bg95_fopen,
    .fread = quectel_bg95_fread,
    .fwrite = quectel_bg95_fwrite,
    .fseek = quectel_bg95_fseek,
    .fclose = quectel_bg95_fclose,
    .fstat = quectel_bg95_fstat,
    .fdel = quectel_bg95_fdel,
#endif
#ifdef CONFIG_QUECTEL_BG95_DFOTA
    .dfota = quectel_bg95_dfota,
#endif
    .reset = modem_reset,
};

static struct modem_cmd response_cmds[] = {
	MODEM_CMD("OK", on_cmd_ok, 0U, ""), /* 3GPP */
	MODEM_CMD("CONNECT", on_cmd_connect_ok, 0U, ""), /* TCP connect */
	MODEM_CMD("ERROR", on_cmd_error, 0U, ""), /* 3GPP */
	MODEM_CMD("SEND FAIL", on_cmd_send_fail, 0U, ""), /* 3GPP */
	MODEM_CMD("+CME ERROR: ", on_cmd_exterror, 1U, ""),
	MODEM_CMD("+QGPSGNMEA:", on_cmd_gps_read, 0U, ""),
	MODEM_CMD("+QGPSLOC: ", on_cmd_gps_read, 0U, ""),
	MODEM_CMD("+CCLK: ", on_cmd_gettime, 1U, ""),
	MODEM_CMD("+QNTP: ", on_cmd_ntptime, 1U, ""),
};

static struct modem_cmd unsol_cmds[] = {
#ifdef MODEM_BG95_SOCKET
    MODEM_CMD("+QSSLURC: ", on_cmd_socknotifysslurc, 2U, ","),
	MODEM_CMD("+QSSLOPEN: ", on_cmd_sockcreate, 2U, ","),
    MODEM_CMD("+QIURC: ", on_cmd_socknotifyurc, 2U, ", "),
#endif
    /* 4 parameters for CREG=2 */
	MODEM_CMD("+CREG: ", on_cmd_socknotifycreg, 2U, ","),
	MODEM_CMD("+CTZV: ", on_cmd_timezoneval, 1U, ""),
	MODEM_CMD("+QHTTPGET: ", on_cmd_http_response, 1U, ","),
	MODEM_CMD("+QHTTPPOST: ", on_cmd_http_response, 1U, ","),
	MODEM_CMD("+QHTTPREAD: ", on_cmd_http_read, 1U, ""),
	MODEM_CMD("+QHTTPREADFILE: ", on_cmd_http_read, 1U, ""),
#ifdef CONFIG_QUECTEL_BG95_FILE_OPS
    MODEM_CMD("+QFWRITE: ", on_cmd_qfwrite, 2U, ","),
#endif
};

static int modem_init(const struct device *dev)
{
	int ret = 0;

    LOG_DBG("BG95 Driver");

	k_sem_init(&mdata.sem_response, 0, 1);
	k_sem_init(&mdata.sem_connect, 0, 1);
	k_sem_init(&mdata.sem_reply, 0, 1);
	k_sem_init(&mdata.mdm_lock, 1, 1);

	/* initialize the work queue */
	k_work_q_start(&modem_workq, modem_workq_stack,
		       K_THREAD_STACK_SIZEOF(modem_workq_stack),
		       K_PRIO_COOP(7));

#ifdef MODEM_BG95_SOCKET
	/* socket config */
	mdata.socket_config.sockets = &mdata.sockets[0];
	mdata.socket_config.sockets_len = ARRAY_SIZE(mdata.sockets);
	mdata.socket_config.base_socket_num = MDM_BASE_SOCKET_NUM;
	ret = modem_socket_init(&mdata.socket_config,
            &offload_socket_fd_op_vtable);
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

	/* modem interface */
	mdata.iface_data.hw_flow_control = DT_PROP(MDM_UART_NODE,
						   hw_flow_control);
	mdata.iface_data.rx_rb_buf = &mdata.iface_rb_buf[0];
	mdata.iface_data.rx_rb_buf_len = sizeof(mdata.iface_rb_buf);
	ret = modem_iface_uart_init(&mctx.iface, &mdata.iface_data,
				    MDM_UART_DEV);
	if (ret < 0) {
		goto error;
	}

    /* modem data storage */
	mctx.data_manufacturer = q_ctx.data_manufacturer;
	mctx.data_model = q_ctx.data_model;
	mctx.data_revision = q_ctx.data_revision;
	mctx.data_imei = q_ctx.data_imei;
	mctx.data_timeval = q_ctx.data_timeval;
    mctx.data_cellinfo = q_ctx.data_cellinfo;

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
	k_work_init(&mdata.urc_handle_work, urc_handle_worker);

#ifdef AGPS_DEFAULT
    (void) quectel_bg95_agps(dev, NULL);
#else
    ARG_UNUSED(dev);
#endif

#ifndef CONFIG_MODEM_QUECTEL_BG95_APP_RESET
	modem_reset();
#endif

error:
	return ret;
}

NET_DEVICE_OFFLOAD_INIT(modem_quectel_bg95, DT_INST_LABEL(0),
            modem_init, device_pm_control_nop, &mdata, NULL,
            CONFIG_MODEM_QUECTEL_BG95_INIT_PRIORITY,
            &api_funcs, MDM_MAX_DATA_LENGTH);

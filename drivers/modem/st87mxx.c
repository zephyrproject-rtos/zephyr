/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_st87mxx

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "st87mxx.h"
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>
#include <zephyr/net/offloaded_netdev.h>
#include <zephyr/net/socket_offload.h>
#include <zephyr/drivers/gpio.h>
#include "modem_receiver.h"

LOG_MODULE_REGISTER(modem_st87mxx, CONFIG_MODEM_LOG_LEVEL);

#define MDM_BASE_SOCKET_NUM		0

/* RX thread structures */
K_KERNEL_STACK_DEFINE(modem_rx_stack, CONFIG_MODEM_ST87MXX_RX_STACK_SIZE);
NET_BUF_POOL_DEFINE(mdm_recv_pool, MDM_RECV_MAX_BUF, MDM_RECV_BUF_SIZE, 0, NULL);
struct k_thread modem_rx_thread;

static void ring_pin_cb(const struct device *dev, struct gpio_callback *cb, uint32_t pins);

static const struct gpio_dt_spec reset_gpio = GPIO_DT_SPEC_INST_GET(0, mdm_reset_gpios);
static const struct gpio_dt_spec ring_gpio = GPIO_DT_SPEC_INST_GET(0, mdm_ring_gpios);
static struct gpio_callback ring_gpio_callback_data;
static char tmp_data[128];
/**
 * ST87 Reset states
 */
typedef enum {
	RESET_PIN_OFF = 0,	/* Reset Pin off	*/
	RESET_PIN_ON = 1,	/* Reset Pin on		*/
	RESET_PIN_PULSE = 2	/* Reset Pin toggle	*/
} st87mxx_reset_pin_t;

/**
 * Static Data for ST87MXX
 */
struct st87mxx_register {
	struct mdm_receiver_context *mctx;
	struct gpio_dt_spec *reset_gpio;
	struct gpio_dt_spec *ring_gpio;
};

static const struct socket_op_vtable offload_socket_fd_op_vtable;
#if defined(CONFIG_DNS_RESOLVER)
static const struct socket_dns_offload offload_dns_ops;
static struct zsock_addrinfo dns_result;
static struct sockaddr dns_result_addr;
static char dns_result_canonname[DNS_MAX_NAME_SIZE + 1];
#endif

static struct modem_context mctx;
static struct st87mxx_data mdata;


static int offload_socket(int family, int type, int proto);

static inline int digits(int n)
{
	int count = 0;

	while (n != 0) {
		n /= 10;
		++count;
	}
	return count;
}

/* Find first occurrence of a delimiter (either ',' or '\r') in a given string. */
static const char *find_delim(const char *start)
{
	const char *comma = strchr(start, ',');
	const char *cr = strchr(start, '\r');
	const char *delim;

	/* Find first occurrence of either ',' or '\r' after start */
	if (comma && cr) {
		delim = (comma < cr) ? comma : cr;
	} else if (comma) {
		delim = comma;
	} else {
		delim = cr;
	}
	return delim;
}

/* Extract the hex value for a given tag and return as unsigned long. */
static int extract_hex_value(const char *input, const char *tag, unsigned long *output)
{
	char pattern[32];

	snprintf(pattern, sizeof(pattern), "%s=", tag);
	const char *start = strstr(input, pattern);

	if (!start) {
		return 0;
	}

	start += strlen(pattern);
	const char *end = find_delim(start);

	if (!end) {
		end = input + strlen(input);
	}

	size_t len = end - start;

	if (len == 0 || len >= 16) {
		return 0;
	}

	char hex_str[17] = {0};

	strncpy(hex_str, start, len);
	hex_str[len] = '\0';

	char *endptr;

	unsigned long val = strtoul(hex_str, &endptr, 16);

	if (*endptr != '\0') {
		return 0; /* invalid hex */
	}

	*output = val;
	return 1;
}

MODEM_CMD_DEFINE(on_cmd_cgmi)
{
	size_t out_len = net_buf_linearize(
		mdata.mdm_manufacturer, sizeof(mdata.mdm_manufacturer) - 1, data->rx_buf, 0, len);
	mdata.mdm_manufacturer[out_len] = '\0';
	LOG_INF("Manufacturer: %s", mdata.mdm_manufacturer);
	return 0;
}

MODEM_CMD_DEFINE(on_cmd_cgmm)
{
	size_t out_len = net_buf_linearize(mdata.mdm_model, sizeof(mdata.mdm_model) - 1,
					data->rx_buf, 0, len);
	mdata.mdm_model[out_len] = '\0';
	LOG_INF("Model: %s", mdata.mdm_model);
	return 0;
}

MODEM_CMD_DEFINE(on_cmd_cgmr)
{
	size_t out_len = net_buf_linearize(mdata.mdm_revision, sizeof(mdata.mdm_revision) - 1,
				data->rx_buf, 0, len);
	mdata.mdm_revision[out_len] = '\0';
	LOG_INF("Revision: %s", mdata.mdm_revision);
	return 0;
}

MODEM_CMD_DEFINE(on_cmd_cgsn)
{
	size_t out_len = net_buf_linearize(mdata.mdm_imei, sizeof(mdata.mdm_imei) - 1,
				data->rx_buf, 1, len);
	mdata.mdm_imei[out_len] = '\0';
	LOG_INF("IMEI: %s", mdata.mdm_imei);
	return 0;
}

#if defined(CONFIG_MODEM_SIM_NUMBERS)
MODEM_CMD_DEFINE(on_cmd_cimi)
{
	size_t out_len =
		net_buf_linearize(mdata.mdm_imsi, sizeof(mdata.mdm_imsi) - 1, data->rx_buf, 0, len);
	mdata.mdm_imsi[out_len] = '\0';

	/* Log the received information. */
	LOG_INF("IMSI: %s", mdata.mdm_imsi);
	return 0;
}

MODEM_CMD_DEFINE(on_cmd_iccid)
{
	size_t out_len = net_buf_linearize(mdata.mdm_iccid, sizeof(mdata.mdm_iccid) - 1,
				data->rx_buf, 1, len);
	mdata.mdm_iccid[out_len] = '\0';

	/* Log the received information. */
	LOG_INF("%s", mdata.mdm_iccid);
	return 0;
}
#endif

MODEM_CMD_DEFINE(on_cmd_steng)
{
	unsigned long output = 0;

	net_buf_linearize(tmp_data, len, data->rx_buf, 2, len);

	extract_hex_value(tmp_data, "SRV1", &output);

	mdata.mdm_rssi = output >> 16;
	mdata.mdm_rssi -= 0x10000;

	LOG_INF("RSSI: %d", mdata.mdm_rssi);
	return 0;
}

MODEM_CMD_DEFINE(on_cmd_ok)
{
	modem_cmd_handler_set_error(data, 0);
	k_sem_give(&mdata.sem_response);
	return 0;
}

MODEM_CMD_DEFINE(on_cmd_error)
{
	modem_cmd_handler_set_error(data, -EIO);
	k_sem_give(&mdata.sem_response);
	return 0;
}


MODEM_CMD_DEFINE(on_cmd_sim_status)
{
	LOG_INF("on_cmd_sim_status %s", argv[0]);
	return 0;
}



MODEM_CMD_DEFINE(on_cmd_connection_status)
{
	LOG_INF("on_cmd_connection_status: %s", argv[0]);
	return 0;
}


MODEM_CMD_DEFINE(on_cmd_registration_status)
{
	mdata.mdm_registration = atoi(argv[1]);
	LOG_INF("on_cmd_registration_status: CREG: %u", mdata.mdm_registration);
	return 0;
}

MODEM_CMD_DEFINE(on_cmd_ip_config_status)
{
	mdata.context_id = atoi(argv[0]);
	LOG_INF("on_cmd_ip_config_status: mdata.context_id: %u", mdata.context_id);
	return 0;
}

MODEM_CMD_DEFINE(on_cmd_sleep)
{
	LOG_INF("on_cmd_sleep: %s", argv[0]);
	return 0;
}

MODEM_CMD_DEFINE(on_cmd_wakeup)
{
	LOG_INF("on_cmd_wakeup: %s", argv[0]);
	k_sem_give(&mdata.sem_response);
	return 0;
}

MODEM_CMD_DEFINE(on_cmd_socket_create)
{
	LOG_INF("on_cmd_socket_create: %s", argv[0]);
	struct modem_socket *sock = NULL;

	/* Look up new socket by id. */
	sock = modem_socket_from_id(&mdata.socket_config, atoi(argv[0]));
	return 0;
}

MODEM_CMD_DEFINE(on_cmd_socket_ipread)
{
	LOG_INF("on_cmd_socket_ipread");
	int socket_data_length;
	int bytes_to_skip;
	int sock_id;
	struct socket_read_data	 *sock_data;
	int ret, i;
	struct modem_socket	 *sock = NULL;

	if (!len) {
		LOG_ERR("Invalid length, Aborting!");
		return -EAGAIN;
	}

	/* Make sure we still have buf data. */
	if (!data->rx_buf) {
		LOG_ERR("Incorrect format! Ignoring data!");
		return -EINVAL;
	}

	socket_data_length = atoi(argv[2]);

	LOG_INF("socket_data_length = %d", socket_data_length);


	/* No (or not enough) data available on the socket. */
	/* Skip context_id, socket_id, commas, len, CRLF */
	bytes_to_skip = digits(socket_data_length) + 2 + 4;
	if (socket_data_length <= 0) {
		LOG_ERR("Length problem (%d).  Aborting!", socket_data_length);
		return -EAGAIN;
	}

	/* Check to make sure we have all of the data. */
	if (net_buf_frags_len(data->rx_buf) < (socket_data_length + bytes_to_skip)) {
		LOG_DBG("Not enough data -- wait!");
		return -EAGAIN;
	}

	/* Skip "len" and CRLF. */
	bytes_to_skip = digits(socket_data_length) + 2;


	for (i = 0; i < bytes_to_skip; i++) {
		net_buf_pull_u8(data->rx_buf);
	}

	if (!data->rx_buf->len) {
		data->rx_buf = net_buf_frag_del(NULL, data->rx_buf);
	}

	sock_id = atoi(argv[1]);

	sock = modem_socket_from_id(&mdata.socket_config, sock_id);
	if (!sock) {
		LOG_ERR("Socket not found! (%d)", sock_id);
		ret = -EINVAL;
		goto exit;
	}

	sock_data = (struct socket_read_data *)sock->data;
	if (!sock_data) {
		LOG_ERR("Socket data not found! Skip handling (%d)", sock_id);
		ret = -EINVAL;
		goto exit;
	}

	ret = net_buf_linearize(sock_data->recv_buf, sock_data->recv_buf_len,
				data->rx_buf, 0, (uint16_t)socket_data_length);
	data->rx_buf = net_buf_skip(data->rx_buf, ret);

	sock_data->recv_read_len = ret;
	if (ret != socket_data_length) {
		LOG_ERR("Total copied data is different than received data!"
			" copied:%d vs. received:%d", ret, socket_data_length);
		ret = -EINVAL;
	}

exit:
	/* Remove packet from list (ignore errors). */
	(void)modem_socket_packet_size_update(&mdata.socket_config, sock,
					-socket_data_length);
	return ret;
}


MODEM_CMD_DEFINE(on_cmd_ip_recv)
{
	LOG_INF("on_cmd_ip_recv");

	struct modem_socket *sock;
	int sock_id;

	sock_id = atoi(argv[1]);

	sock = modem_socket_from_id(&mdata.socket_config, sock_id);
	if (!sock) {
		return 0;
	}

	/* Modem does not tell packet size. Set dummy for receive. */
	modem_socket_packet_size_update(&mdata.socket_config, sock, 1);

	LOG_INF("Data available on socket id: %d", sock_id);
	modem_socket_data_ready(&mdata.socket_config, sock);
	return 0;
}

#if defined(CONFIG_DNS_RESOLVER)
MODEM_CMD_DEFINE(on_cmd_dns)
{
	LOG_INF("on_cmd_dns");

	argv[0][strlen(argv[0])] = '\0';

	/* Hard-code DNS to return IPv4. */
	dns_result_addr.sa_family = AF_INET;
	(void)net_addr_pton(dns_result.ai_family, &argv[0][0],
				&((struct sockaddr_in *)&dns_result_addr)->sin_addr);

	k_sem_give(&mdata.sem_dns);
	return 0;
}
#endif

MODEM_CMD_DEFINE(on_cmd_nvmread)
{
	LOG_INF("on_cmd_nvmread");

	int ret = 0;
	int tmp = 0;
	size_t out_len;
	char nvmrd[3];

	out_len = net_buf_linearize(nvmrd, sizeof(nvmrd), data->rx_buf, 0, len);
	nvmrd[out_len] = '\0';

	ret = sscanf((const char *)nvmrd, "%x", (int *)&tmp);
	mdata.cold_init_version = (uint8_t)tmp;

	k_sem_give(&mdata.sem_nvm);
	return ret;
}

/*
 * Possible responses by the ST87MXX.
 */
static const struct modem_cmd response_cmds[] = {
	MODEM_CMD("OK", on_cmd_ok, 0U, ""),
	MODEM_CMD("ERROR", on_cmd_error, 0U, ""),
	MODEM_CMD("+CME ERROR", on_cmd_error, 1U, ""),
};

/*
 * Possible unsolicited commands.
 */
static const struct modem_cmd unsolicited_cmds[] = {
	MODEM_CMD("#SIMST", on_cmd_sim_status, 1U, ""),
	MODEM_CMD("+CSCON", on_cmd_connection_status, 1U, ""),
	MODEM_CMD("#IPCFG: ", on_cmd_ip_config_status, 3U, ","),
	MODEM_CMD("#IPRECV: ", on_cmd_ip_recv, 2U, ","),
	MODEM_CMD("#SLEEP", on_cmd_sleep, 1U, ""),
	MODEM_CMD("#WAKEUP", on_cmd_wakeup, 1U, ""),
	MODEM_CMD("#STENG", on_cmd_steng, 1U, ""),
};

/* ST87MXX one-shot NVM configuration commands (sent at boot time). */
static const struct setup_cmd init_cmds[] = {
	SETUP_CMD("AT+CMEE=1", "", NULL, 0U, ""),
	SETUP_CMD("AT+CEREG=5", "", NULL, 0U, ""),
	SETUP_CMD("AT+CSCON=1", "", NULL, 0U, ""),
	SETUP_CMD("AT#SLEEPIND=0x1F", "", NULL, 0U, ""),
	SETUP_CMD("AT#WDGMODE=0", "", NULL, 0U, ""),
	SETUP_CMD("AT#TEMPLIMIT=-40, 85, "STRINGIFY(TEMP_LOW_SHUTDOWN)", "
		STRINGIFY(TEMP_HIGH_SHUTDONW)", 0, "
		STRINGIFY(TEMP_SHUTDOWN), "", NULL, 0U, ""),
	SETUP_CMD("AT#VBATLIMIT=2200, 3000, "STRINGIFY(VBAT_LOW_SHUTDOWN)", "
		STRINGIFY(VBAT_HIGH_SHUTDOWN)", "
		STRINGIFY(VBAT_SHUTDOWN)", 0, 0", "", NULL, 0U, ""),
	SETUP_CMD("AT+CFUN=0", "", NULL, 0U, ""),
	SETUP_CMD("AT#BANDSEL="BANDLIST, "", NULL, 0U, ""),
	SETUP_CMD("AT#BANDCFG="BANDCFG, "", NULL, 0U, ""),
	SETUP_CMD("AT#BANDCFG="BANDCFG_NMO1, "", NULL, 0U, ""),
	SETUP_CMD("AT#BANDCFG="BANDCFG_NMO2, "", NULL, 0U, ""),
	SETUP_CMD("AT#BANDCFG="BANDCFG_NMO3, "", NULL, 0U, ""),
	SETUP_CMD("AT#SCAN=1,-104,1,360,1,360", "", NULL, 0U, ""),
	SETUP_CMD("AT+CEDRXS=1,5,"STRINGIFY(EDRX_VALUE), "", NULL, 0U, ""),
	SETUP_CMD("AT#PTW="STRINGIFY(PTW_VALUE), "", NULL, 0U, ""),
	SETUP_CMD("AT+CPSMS="STRINGIFY(PSM_ENABLE)",,, "STRINGIFY(PERIODIC_TAU)", "
		STRINGIFY(ACTIVE_TIME), "", NULL, 0U, ""),
	SETUP_CMD("AT#SLEEPMODE=1, "STRINGIFY(HOLD_TIME)", "
		STRINGIFY(AWAKE_TIME), "", NULL, 0U, ""),
	SETUP_CMD("AT#RINGPIN="STRINGIFY(RING_PIN_ENABLE)", "STRINGIFY(RING_PIN_GPIO)", "
		STRINGIFY(RING_PIN_POLARITY)", "STRINGIFY(RING_PIN_DELAY), "", NULL, 0U, ""),
	SETUP_CMD("AT#WAKEUPEVENT=15, 3", "", NULL, 0U, ""),
	SETUP_CMD("AT#IPPARAMS=1, 0, 65535, 60, "STRINGIFY(NB_PACKET_SENT_ENABLE)", "
		STRINGIFY(DOMAIN_NAME), "", NULL, 0U, ""),
	SETUP_CMD("AT#NVMWR="STRINGIFY(ST87MXX_COLD_VERSION_NVM_PAGE)", "
		STRINGIFY(ST87MXX_COLD_VERSION_NVM_OFFSET)", 1, "
		STRINGIFY(ST87MXX_COLD_CONFIG_VERSION), "", NULL, 0U, ""),
	SETUP_CMD("AT#RESET=1", "", NULL, 0U, ""),
};


/* Commands sent to the modem to set it up at init time. */
static const struct setup_cmd setup_cmds[] = {

	/* Commands to read info from the modem (e.g. IMEI, Manufacturer Model etc). */
	SETUP_CMD("AT+CGMI", "", on_cmd_cgmi, 0U, ""),
	SETUP_CMD("AT+CGMM", "", on_cmd_cgmm, 0U, ""),
	SETUP_CMD("AT+CGMR", "", on_cmd_cgmr, 0U, ""),
	SETUP_CMD("AT+CGSN", "", on_cmd_cgsn, 0U, ""),
#if defined(CONFIG_MODEM_SIM_NUMBERS)
	SETUP_CMD("AT+CIMI", "", on_cmd_cimi, 0U, ""),
	SETUP_CMD("AT+ICCID", "", on_cmd_iccid, 0U, ""),
#endif
	SETUP_CMD("AT#STENG=8,8", "", NULL, 0U, ""),
};

static inline uint8_t *modem_get_mac(const struct device *dev)
{
	mdata.mac_addr[0] = 0x00;
	mdata.mac_addr[1] = 0x10;

	sys_rand_get(&mdata.mac_addr[2], 4U);

	return mdata.mac_addr;
}

static void ring_pin_cb(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	LOG_INF("RING CB");
}

/* Setup the Modem NET Interface. */
static void modem_net_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct st87mxx_data *data = dev->data;

	net_if_set_link_addr(iface, modem_get_mac(dev), sizeof(data->mac_addr), NET_LINK_ETHERNET);

	data->netif = iface;
#ifdef CONFIG_DNS_RESOLVER
	socket_offload_dns_register(&offload_dns_ops);
#endif
	net_if_socket_offload_set(iface, offload_socket);
}


static bool offload_is_supported(int family, int type, int proto)
{
	LOG_INF("OFFLOAD IS SUPPORTED");

	if ((family != AF_INET) && (family != AF_INET6)) {
		return false;
	}

	if ((type != SOCK_DGRAM) && (type != SOCK_STREAM)) {
		return false;
	}

	if ((proto != IPPROTO_TCP) && (proto != IPPROTO_UDP)) {
		return false;
	}

	return true;
}

static int st87mxx_gpio_init(void)
{
	int ret = 0;

	if (!gpio_is_ready_dt(mdata.reset_gpio) || !gpio_is_ready_dt(mdata.ring_gpio)) {
		ret = -1;
	}

	/* Configure a GPO for ST87 Reset GPIO */
	if (gpio_pin_configure_dt(mdata.reset_gpio, GPIO_OUTPUT) < 0) {
		ret = -1;
	}

	/* Configure a GPI for ST87 ring pin */
	if (gpio_pin_configure_dt(mdata.ring_gpio, GPIO_INPUT) < 0) {
		ret = -1;
	} else {
		if (gpio_pin_interrupt_configure_dt(mdata.ring_gpio,
				GPIO_INT_EDGE_TO_ACTIVE) < 0) {
			ret = -1;
		} else {
			gpio_init_callback(&ring_gpio_callback_data,
				ring_pin_cb, BIT(mdata.ring_gpio->pin));
			gpio_add_callback(mdata.ring_gpio->port, &ring_gpio_callback_data);
		}
	}

	return ret;
}

static int st87mxx_drive_reset_pin(st87mxx_reset_pin_t state)
{
	uint8_t status = 0;
	int ret = 0;

	switch (state) {
	case RESET_PIN_ON:
		status = (uint8_t)gpio_pin_set_dt(mdata.reset_gpio, 1);
		break;

	case RESET_PIN_OFF:
		status = (uint8_t)gpio_pin_set_dt(mdata.reset_gpio, 0);
		break;

	case RESET_PIN_PULSE:
		status = (uint8_t)gpio_pin_set_dt(mdata.reset_gpio, 0);
		k_msleep(15);
		status += (uint8_t)gpio_pin_set_dt(mdata.reset_gpio, 1);
		break;

	default:
		break;
	}

	if (status > 0) {
		ret = -1;
	}

	return ret;
}

static int st87mxx_reset(void)
{
	uint8_t status = 0;
	int ret = 0;

	/* GPIO initialization */
	status = (uint8_t)st87mxx_gpio_init();

	/* Reset ST87 module */
	status += (uint8_t)st87mxx_drive_reset_pin(RESET_PIN_PULSE);

	k_msleep(500); /* Wait for ST87 Hw reset prior to proceeding. */

	if (status > 0) {
		ret = -1;
	}

	return ret;
}

static int st87mxx_cold_param_init(void)
{
	int ret = 0;

	struct modem_cmd cmd[] = { MODEM_CMD("#NVMRD: ", on_cmd_nvmread, 1U, "") };
	char buf[sizeof("AT#NVMRD=##,##,##")];

	ret = snprintk(buf, sizeof(buf), "AT#NVMRD=%d,%d,1", ST87MXX_COLD_VERSION_NVM_PAGE,
				ST87MXX_COLD_VERSION_NVM_OFFSET);
	if (ret < 0) {
		LOG_ERR("Failed to read the NVM");
		goto error;
	}
	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, cmd, ARRAY_SIZE(cmd), buf,
				&mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("Failed to send AT command: %s ret: %d", buf, ret);
		goto error;
	}

	/* Wait for the answer. */
	k_sem_reset(&mdata.sem_nvm);
	ret = k_sem_take(&mdata.sem_nvm, MDM_CMD_TIMEOUT);

	if (mdata.cold_init_version == ST87MXX_COLD_CONFIG_VERSION) {
		/* Cold config already up-to-date */
		LOG_DBG("ST87MXX NVM up-to-date");
		return 0;
	}
	LOG_DBG("ST87M01 NVM config version mismatch: %d, going to rewrite the config...",
		mdata.cold_init_version);

	/* Run init commands on the modem. */
	ret = modem_cmd_handler_setup_cmds(&mctx.iface, &mctx.cmd_handler,
						init_cmds, ARRAY_SIZE(init_cmds),
						&mdata.sem_response, MDM_REGISTRATION_TIMEOUT);
	if (ret < 0) {
		goto error;
	}
	return 0;

error:
	return -1;
}


static int st87mxx_init(struct st87mxx_register *reg)
{
	LOG_INF("ST87MXX Init");
	int ret = 0;
	uint8_t status = 0;
	int counter = 0;
	const char *buf = "AT+CEREG?";
	struct modem_cmd cmds[] = { MODEM_CMD("+CEREG: ", on_cmd_registration_status, 2U, ",") };

	/* Register Data to Local mdata */
	mdata.mctx = reg->mctx;
	mdata.reset_gpio = reg->reset_gpio;
	mdata.ring_gpio = reg->ring_gpio;

	/* Reset of the whole system as init */
	status += (uint8_t)st87mxx_reset();

	/* Trig cold parameter initialization sequence */
	status += (uint8_t)st87mxx_cold_param_init();

	if (status > 0) {
		ret = -1;
		goto error;
	}

	k_sleep(K_SECONDS(1));

	/* wait for +CREG: 1(normal) or 5(roaming) */
	counter = 0;
	while ((counter++ < MDM_MAX_CEREG_WAITS) && (mdata.mdm_registration != 1) &&
				(mdata.mdm_registration != 5)) {
		ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, cmds, ARRAY_SIZE(cmds), buf,
					&mdata.sem_response, MDM_CMD_TIMEOUT);
		if (ret < 0) {
			LOG_ERR("Failed to query registration!!");
			return -1;
		}

		k_sleep(K_SECONDS(1));
	}

	LOG_INF("Network registration done!");

	if (counter >= MDM_MAX_CEREG_WAITS) {
		LOG_WRN("Network registration failed!");
		ret = -1;
		goto error;
	}

error:
		return ret;
}

/*Process all messages received from the modem. */
static void modem_rx(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		/* Wait for incoming data */
		modem_iface_uart_rx_wait(&mctx.iface, K_FOREVER);

		modem_cmd_handler_process(&mctx.cmd_handler, &mctx.iface);
	}
}

/*
 * Initializes modem handlers and context.
 * After successful init this function calls
 * modem_setup.
 */
static int modem_init(const struct device *dev)
{
	int ret = 0;

	ARG_UNUSED(dev);
	LOG_INF("ST87M01 modem initialization");

	k_sem_init(&mdata.sem_response, 0, 1);
	k_sem_init(&mdata.sem_dns, 0, 1);
	k_sem_init(&mdata.sem_nvm, 0, 1);

	/* Assume the modem is not registered to the network. */
	mdata.mdm_registration = 0;

	mdata.current_sock_written = 0;

	/* Socket config. */
	ret = modem_socket_init(&mdata.socket_config, &mdata.sockets[0], ARRAY_SIZE(mdata.sockets),
				MDM_BASE_SOCKET_NUM, true, &offload_socket_fd_op_vtable);
	if (ret < 0) {
		goto error;
	}

	/* Command handler. */
	const struct modem_cmd_handler_config cmd_handler_config = {
		.match_buf = &mdata.cmd_match_buf[0],
		.match_buf_len = sizeof(mdata.cmd_match_buf),
		.buf_pool = &mdm_recv_pool,
		.alloc_timeout = BUF_ALLOC_TIMEOUT,
		.eol = "\r\n",
		.user_data = NULL,
		.response_cmds = response_cmds,
		.response_cmds_len = ARRAY_SIZE(response_cmds),
		.unsol_cmds = unsolicited_cmds,
		.unsol_cmds_len = ARRAY_SIZE(unsolicited_cmds),
	};

	ret = modem_cmd_handler_init(&mctx.cmd_handler, &mdata.cmd_handler_data,
					&cmd_handler_config);
	if (ret < 0) {
		goto error;
	}

		/* Uart handler. */
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

	mctx.data_manufacturer = mdata.mdm_manufacturer;
	mctx.data_model = mdata.mdm_model;
	mctx.data_revision = mdata.mdm_revision;
	mctx.data_imei = mdata.mdm_imei;
#if defined(CONFIG_MODEM_SIM_NUMBERS)
	mctx.data_imsi = mdata.mdm_imsi;
	mctx.data_iccid = mdata.mdm_iccid;
#endif
	mctx.data_rssi = &mdata.mdm_rssi;

	mctx.driver_data = &mdata;

	ret = modem_context_register(&mctx);
	if (ret < 0) {
		LOG_ERR("Error registering modem context: %d", ret);
		goto error;
	}

	k_thread_create(&modem_rx_thread, modem_rx_stack, K_KERNEL_STACK_SIZEOF(modem_rx_stack),
			modem_rx, NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);

	struct st87mxx_register reg;

	reg.mctx = (struct mdm_receiver_context *)&mctx;
	reg.reset_gpio = (struct gpio_dt_spec *)(&reset_gpio);
	reg.ring_gpio = (struct gpio_dt_spec *)(&ring_gpio);
	st87mxx_init(&reg);

	/* Run setup commands on the modem. */
	ret = modem_cmd_handler_setup_cmds(&mctx.iface, &mctx.cmd_handler,
					setup_cmds, ARRAY_SIZE(setup_cmds),
					&mdata.sem_response, MDM_REGISTRATION_TIMEOUT);
	if (ret < 0) {
		goto error;
	}

error:
		return ret;
}

static void socket_close(struct modem_socket *sock)
{
	int ret;
	char buf[sizeof("AT#SOCKETCLOSE=##,##")];

	ret = snprintk(buf, sizeof(buf), "AT#SOCKETCLOSE=%d,%d", mdata.context_id, sock->id);
	if (ret < 0) {
		LOG_ERR("Failed to close the socket %d", sock->id);
	}

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buf, &mdata.sem_response,
				MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret: %d", buf, ret);
	}

	modem_socket_put(&mdata.socket_config, sock->sock_fd);
}

static int st87mxx_create_socket(struct modem_socket *sock, const struct sockaddr *addr)
{
	char *protocol;
	struct modem_cmd cmd[] = { MODEM_CMD("#SOCKETCREATE: ", on_cmd_socket_create, 1U, "") };
	char buf[sizeof("AT#SOCKETCREATE: #,#,###,,##,##,#")];
	char ip_str[NET_IPV6_ADDR_LEN];
	int ret = 0;
	int ip_mode = -1;

	/* Get the IP version */
	if (addr->sa_family == AF_INET6) {
		LOG_INF("addr->sa_family: AF_INET6");
		ip_mode = 1;
	} else if (addr->sa_family == AF_INET) {
		LOG_INF("addr->sa_family: AF_INET");
		ip_mode = 0;
	} else {
	}

	/* Get protocol */
	protocol = (sock->type == SOCK_STREAM) ? "TCP" : "UDP";

	ret = modem_context_sprint_ip_addr(addr, ip_str, sizeof(ip_str));
	if (ret != 0) {
		LOG_ERR("Failed to format IP!");
		errno = ENOMEM;
		return -1;
	}

	ret = snprintk(buf, sizeof(buf), "AT#SOCKETCREATE=%d,%d,%s,,%d,%d,%d", mdata.context_id,
				ip_mode, protocol, SOCKET_SEND_TIMEOUT,
				SOCKET_RECEIVE_TIMEOUT, SOCKET_FRAME_RECEIVED_URC);
	if (ret < 0) {
		LOG_ERR("Failed to build connect command. ID: %d, FD: %d", sock->id, sock->sock_fd);
		errno = ENOMEM;
		return -1;
	}

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, cmd, ARRAY_SIZE(cmd), buf,
				&mdata.sem_response, MDM_CONNECT_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret: %d", buf, ret);
		socket_close(sock);
		goto error;
	}

	ret = modem_cmd_handler_get_error(&mdata.cmd_handler_data);
	if (ret != 0) {
		LOG_ERR("Closing the socket!");
		socket_close(sock);
		goto error;
	}

	sock->is_connected = true;
	errno = 0;
	return 0;
error:
	errno = -ret;
	return -1;

}

static int st87mxx_tcp_connect(struct modem_socket *sock, const struct sockaddr *addr)
{
	int ret;
	uint16_t dst_port = 0;
	char ip_str[NET_IPV6_ADDR_LEN];
	char send_buf[sizeof("AT#TCPCONNECT=#,#,#xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:"
			"xxx.xxx.xxx.xxx#,####")] = { 0 };

	if (sock->type == SOCK_STREAM) {
		/* Get the destination port */
		if (addr->sa_family == AF_INET6) {
			dst_port = ntohs(net_sin6(addr)->sin6_port);
		} else if (addr->sa_family == AF_INET) {
			dst_port = ntohs(net_sin(addr)->sin_port);
		} else {
		}

		ret = modem_context_sprint_ip_addr(addr, ip_str, sizeof(ip_str));
		if (ret != 0) {
			LOG_ERR("Failed to format IP!");
			errno = ENOMEM;
			return -1;
		}

		ret = snprintk(send_buf, sizeof(send_buf), "AT#TCPCONNECT=%d,%d,%s,%d",
						mdata.context_id, sock->id, ip_str, dst_port);

		if (ret < 0) {
			LOG_ERR("Failed to build send command!!");
			errno = ENOMEM;
			return -1;
		}

		ret = modem_cmd_send_nolock(&mctx.iface, &mctx.cmd_handler,
					NULL, 0U, send_buf, NULL, K_NO_WAIT);

		sock->is_connected = true;

		/* Wait for the OK */
		k_sem_reset(&mdata.sem_response);
		ret = k_sem_take(&mdata.sem_response, MDM_CMD_TIMEOUT);
	}
	return 0;
}

static int offload_socket(int family, int type, int proto)
{
	LOG_INF("OFFLOAD SOCKET");

	int ret;

	/* Defer modem's socket create call to bind(). */
	ret = modem_socket_get(&mdata.socket_config, family, type, proto);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	errno = 0;
	return ret;
}


static int offload_bind(void *obj, const struct sockaddr *addr, socklen_t addrlen)
{
	LOG_INF("OFFLOAD BIND");
	struct modem_socket *sock = (struct modem_socket *)obj;

	/* Save bind address information. */
	memcpy(&sock->src, addr, sizeof(*addr));

	/* Make sure we've created the socket. */
	if (modem_socket_is_allocated(&mdata.socket_config, sock) == true) {
		if (st87mxx_create_socket(sock, addr) != 0) {
			LOG_ERR("Socket creation failed");
			return -EOPNOTSUPP;
		}
	}
	return 0;
}

static int offload_close(void *obj)
{
	struct modem_socket *sock = (struct modem_socket *)obj;

	/* Make sure socket is allocated. */
	if (modem_socket_is_allocated(&mdata.socket_config, sock) == false) {
		return 0;
	}

	/* Close the socket only if it is connected. */
	if (sock->is_connected) {
		socket_close(sock);
	}
	return 0;
}

static int offload_ioctl(void *obj, unsigned int request, va_list args)
{
	LOG_INF("OFFLOAD IOCTL");

	switch (request) {
	case ZFD_IOCTL_POLL_PREPARE: {

		LOG_INF("OFFLOAD IOCTL ZFD_IOCTL_POLL_PREPARE");

		struct zsock_pollfd *pfd;
		struct k_poll_event **pev;
		struct k_poll_event *pev_end;

		pfd = va_arg(args, struct zsock_pollfd *);
		pev = va_arg(args, struct k_poll_event **);
		pev_end = va_arg(args, struct k_poll_event *);

		return modem_socket_poll_prepare(&mdata.socket_config, obj, pfd, pev, pev_end);
	}
	case ZFD_IOCTL_POLL_UPDATE: {
		LOG_INF("OFFLOAD IOCTL ZFD_IOCTL_POLL_UPDATE");

		struct zsock_pollfd *pfd;
		struct k_poll_event **pev;

		pfd = va_arg(args, struct zsock_pollfd *);
		pev = va_arg(args, struct k_poll_event **);

		return modem_socket_poll_update(obj, pfd, pev);
	}

	default:
		errno = EINVAL;
		return -1;
	}
}

static int offload_connect(void *obj, const struct sockaddr *addr, socklen_t addrlen)
{
	LOG_INF("OFFLOAD CONNECT");

	struct modem_socket *sock = (struct modem_socket *)obj;
	int ret;

	if (modem_socket_is_allocated(&mdata.socket_config, sock) == false) {
		LOG_ERR("Invalid socket id %d from fd %d", sock->id, sock->sock_fd);
		errno = EINVAL;
		return -1;
	}

	if (sock->is_connected == true) {
		LOG_INF("Socket is already connected! id: %d, fd: %d", sock->id, sock->sock_fd);
	} else {
		if (st87mxx_create_socket(sock, addr) != 0) {
			LOG_ERR("Socket creation failed");
			return -EOPNOTSUPP;
		}
	}

	if (sock->type == SOCK_STREAM) {
		ret = (int)st87mxx_tcp_connect(sock, addr);
	}

	ret = modem_cmd_handler_get_error(&mdata.cmd_handler_data);
	if (ret != 0) {
		LOG_ERR("Closing the socket!");
		socket_close(sock);
		goto error;
	}

	sock->is_connected = true;
	errno = 0;
	return 0;
error:
	errno = -ret;
	return -1;
}

static ssize_t offload_sendto(void *obj, const void *buf, size_t len, int flags,
				const struct sockaddr *dest_addr, socklen_t addrlen)
{
	LOG_INF("OFFLOAD SENDTO");

	int ret;
	struct modem_socket *sock = (struct modem_socket *)obj;
	char send_buf_udp[sizeof("AT#IPSENDUDP=##,##,#xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:"
			"xxx.xxx.xxx.xxx#,##,##,##,####")] = { 0 };
	char send_buf_tcp[sizeof("AT#IPSENDTCP=##,##,#,####")] = { 0 };
	char ip_str[NET_IPV6_ADDR_LEN];
	uint16_t dst_port = 0;

	/* Do some sanity checks. */
	if (!buf || len == 0) {
		errno = EINVAL;
		return -1;
	}

	/* Socket has to be connected. */
	if (!sock->is_connected) {
		errno = ENOTCONN;
		return -1;
	}

	/* Only send up to Maximum Transmission Unit bytes. */
	if (len > MDM_MAX_DATA_LENGTH) {
		len = MDM_MAX_DATA_LENGTH;
	}

	/* Make sure only one send can be done at a time. */
	k_sem_take(&mdata.cmd_handler_data.sem_tx_lock, K_FOREVER);

	switch (sock->type) {
	case SOCK_STREAM:
		ret = snprintk(send_buf_tcp, sizeof(send_buf_tcp), "AT#IPSENDTCP=%d,%d,1,%zu",
						mdata.context_id, sock->id, len);
		if (ret < 0) {
			LOG_ERR("Failed to build send command!!");
			errno = ENOMEM;
			return -1;
		}

		ret = modem_cmd_send_nolock(&mctx.iface, &mctx.cmd_handler,
					NULL, 0U, send_buf_tcp, NULL, K_NO_WAIT);
		if (ret < 0) {
			LOG_ERR("Failed to send AT#IPSENDTCP command!!");
			goto exit;
		}

		/* Wait for the OK */
		k_sem_reset(&mdata.sem_response);
		ret = k_sem_take(&mdata.sem_response, MDM_CMD_TIMEOUT);

		break;

	case SOCK_DGRAM:

	/* Get the destination port. */
	if (dest_addr->sa_family == AF_INET6) {
		dst_port = ntohs(net_sin6(dest_addr)->sin6_port);
	} else if (dest_addr->sa_family == AF_INET) {
		dst_port = ntohs(net_sin(dest_addr)->sin_port);
	} else {
	}

	ret = modem_context_sprint_ip_addr(dest_addr, ip_str, sizeof(ip_str));
	if (ret != 0) {
		LOG_ERR("Failed to format IP!");
		errno = ENOMEM;
		return -1;
	}

	ret = snprintk(send_buf_udp, sizeof(send_buf_udp), "AT#IPSENDUDP=%d,%d,%s,%d,%d,%d,%zu",
			mdata.context_id, sock->id, ip_str, dst_port, 0, 1, len);
	if (ret < 0) {
		LOG_ERR("Failed to build send command!!");
		errno = ENOMEM;
		return -1;
	}

	ret = modem_cmd_send_nolock(&mctx.iface, &mctx.cmd_handler,
				NULL, 0U, send_buf_udp, NULL, K_NO_WAIT);

	if (ret < 0) {
		LOG_ERR("Failed to send AT#IPSENDUDP command!!");
		goto exit;
	}

	/* Wait for the OK. */
	k_sem_reset(&mdata.sem_response);
	ret = k_sem_take(&mdata.sem_response, MDM_CMD_TIMEOUT);

	break;

	default:
		break;
	}

	/* Send data */
	modem_cmd_send_data_nolock(&mctx.iface, buf, len);

	/* Wait for the OK */
	k_sem_reset(&mdata.sem_response);
	ret = k_sem_take(&mdata.sem_response, MDM_CMD_TIMEOUT);


exit:
	k_sem_give(&mdata.cmd_handler_data.sem_tx_lock);

	/* Data was successfully sent. */

	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	errno = 0;
	mdata.current_sock_written = len;
	return mdata.current_sock_written;

}

static ssize_t offload_recvfrom(void *obj, void *buf, size_t max_len, int flags,
				struct sockaddr *src_addr, socklen_t *addrlen)
{
	LOG_INF("OFFLOAD RECVFROM");

	struct modem_socket *sock = (struct modem_socket *)obj;
	char sendbuf[sizeof("AT#IPREAD=#####,####")];
	int ret, packet_size;
	struct socket_read_data sock_data;

	struct modem_cmd data_cmd[] = { MODEM_CMD("#IPREAD: ", on_cmd_socket_ipread, 3U, ",") };

	if (!buf || max_len == 0) {
		errno = EINVAL;
		return -1;
	}

	if (flags & ZSOCK_MSG_PEEK) {
		errno = ENOTSUP;
		return -1;
	}

	packet_size = modem_socket_next_packet_size(&mdata.socket_config, sock);
	if (!packet_size) {
		if (flags & ZSOCK_MSG_DONTWAIT) {
			errno = EAGAIN;
			return -1;
		}

		modem_socket_wait_data(&mdata.socket_config, sock);
		packet_size = modem_socket_next_packet_size(&mdata.socket_config, sock);
	}

	max_len = (max_len > MDM_MAX_DATA_LENGTH) ? MDM_MAX_DATA_LENGTH : max_len;
	snprintk(sendbuf, sizeof(sendbuf), "AT#IPREAD=%d,%d", mdata.context_id, sock->id);

	memset(&sock_data, 0, sizeof(sock_data));
	sock_data.recv_buf = buf;
	sock_data.recv_buf_len = max_len + 2; /* margin */
	sock_data.recv_addr = src_addr;
	sock->data = &sock_data;

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, data_cmd, ARRAY_SIZE(data_cmd),
				sendbuf, &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		errno = -ret;
		ret = -1;
		goto exit;
	}

	/* Use dst address as src. */
	if (src_addr && addrlen) {
		*addrlen = sizeof(sock->dst);
		memcpy(src_addr, &sock->dst, *addrlen);
	}

	errno = 0;
	ret = sock_data.recv_read_len;

exit:
	/* Clear socket data. */
	sock->data = NULL;
	return ret;

}

static ssize_t offload_sendmsg(void *obj, const struct msghdr *msg, int flags)
{
	LOG_INF("OFFLOAD SENDMSG");

	ssize_t sent = 0;
	int rc;

	LOG_DBG("msg_iovlen:%zd flags:%d", msg->msg_iovlen, flags);

	for (int i = 0; i < msg->msg_iovlen; i++) {
		const char *buf = msg->msg_iov[i].iov_base;
		size_t len	= msg->msg_iov[i].iov_len;

		while (len > 0) {
			rc = offload_sendto(obj, buf, len, flags,
					msg->msg_name, msg->msg_namelen);
			if (rc < 0) {
				if (rc == -EAGAIN) {
					k_sleep(MDM_SENDMSG_SLEEP);
				} else {
					sent = rc;
					break;
				}
			} else {
				sent += rc;
				buf += rc;
				len -= rc;
			}
		}
	}

	return (ssize_t) sent;
}

static ssize_t offload_read(void *obj, void *buffer, size_t count)
{
	return offload_recvfrom(obj, buffer, count, 0, NULL, 0);
}

static ssize_t offload_write(void *obj, const void *buffer, size_t count)
{
	return offload_sendto(obj, buffer, count, 0, NULL, 0);
}

#if defined(CONFIG_DNS_RESOLVER)
static int offload_getaddrinfo(const char *node, const char *service,
			const struct zsock_addrinfo *hints, struct zsock_addrinfo **res)
{
	LOG_INF("OFFLOAD GETADDRINFO");

	struct modem_cmd cmd[] = { MODEM_CMD("#DNS: ", on_cmd_dns, 2U, ",") };
	/* DNS command + 128 bytes for domain name parameter */
	char sendbuf[sizeof("AT#DNS=#,#,'[]'")+128];
	uint32_t port = 0;
	int ret;

	/* Init result */
	(void)memset(&dns_result, 0, sizeof(dns_result));
	(void)memset(&dns_result_addr, 0, sizeof(dns_result_addr));

	/* Currently only supports IPv4. */
	dns_result.ai_family = AF_INET;
	dns_result_addr.sa_family = AF_INET;
	dns_result.ai_addr = &dns_result_addr;
	dns_result.ai_addrlen = sizeof(dns_result_addr);
	dns_result.ai_canonname = dns_result_canonname;
	dns_result_canonname[0] = '\0';

	if (service) {
		port = atoi(service);
		if (port < 1 || port > USHRT_MAX) {
			return DNS_EAI_SERVICE;
		}
	}

	if (port > 0U) {
		if (dns_result.ai_family == AF_INET) {
			net_sin(&dns_result_addr)->sin_port = htons(port);
		}
	}

	/* Check if node is an IP address. */
	if (net_addr_pton(dns_result.ai_family, node,
				&((struct sockaddr_in *)&dns_result_addr)->sin_addr) == 0) {
		*res = &dns_result;
		return 0;
	}

	/* User flagged node as numeric host, but we failed net_addr_pton. */
	if (hints && hints->ai_flags & AI_NUMERICHOST) {
		return DNS_EAI_NONAME;
	}

	/* Send dummy AT command to wake the modem up in case it is sleeping. */
	snprintk(sendbuf, sizeof(sendbuf), "AT");
	ret = modem_cmd_send_nolock(&mctx.iface, &mctx.cmd_handler,
				NULL, 0U, sendbuf, NULL, K_NO_WAIT);

	/* Wait for the OK. */
	k_sem_reset(&mdata.sem_response);
	ret = k_sem_take(&mdata.sem_response, MDM_CMD_TIMEOUT);

	k_sem_reset(&mdata.sem_response);

	snprintk(sendbuf, sizeof(sendbuf), "AT#DNS=%d,0,%s", mdata.context_id, node);
	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, cmd, ARRAY_SIZE(cmd), sendbuf,
				&mdata.sem_response, MDM_CMD_TIMEOUT);

	*res = (struct zsock_addrinfo *)&dns_result;
	return 0;
}

static void offload_freeaddrinfo(struct zsock_addrinfo *res)
{
	/* No need to free static memory. */
	ARG_UNUSED(res);
}
#endif


#if defined(CONFIG_DNS_RESOLVER)
static const struct socket_dns_offload offload_dns_ops = {
	.getaddrinfo = offload_getaddrinfo,
	.freeaddrinfo = offload_freeaddrinfo,
};
#endif

static struct offloaded_if_api api_funcs = {
	.iface_api.init = modem_net_iface_init,
};

static const struct socket_op_vtable offload_socket_fd_op_vtable = {
	.fd_vtable = {
		.read	= offload_read,
		.write	= offload_write,
		.close	= offload_close,
		.ioctl	= offload_ioctl,
	},
	.bind		= offload_bind,
	.connect	= offload_connect,
	.sendto		= offload_sendto,
	.recvfrom	= offload_recvfrom,
	.listen		= NULL,
	.accept		= NULL,
	.sendmsg	= offload_sendmsg,
	.getsockopt	= NULL,
	.setsockopt	= NULL,
};

/* Register device with the networking stack. */
NET_DEVICE_DT_INST_OFFLOAD_DEFINE(0, modem_init, NULL, &mdata, NULL,
				CONFIG_MODEM_ST87MXX_INIT_PRIORITY, &api_funcs,
				CONFIG_MODEM_ST87MXX_MAX_RX_DATA_LENGTH);

NET_SOCKET_OFFLOAD_REGISTER(st87mxx, CONFIG_NET_SOCKETS_OFFLOAD_PRIORITY,
				AF_UNSPEC, offload_is_supported, offload_socket);

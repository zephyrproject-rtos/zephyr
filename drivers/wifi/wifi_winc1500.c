/**
 * Copyright (c) 2017 IpTronix
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_WIFI_LEVEL
#define SYS_LOG_DOMAIN "dev/winc1500"
#include <logging/sys_log.h>

#include <zephyr.h>
#include <kernel.h>
#include <device.h>
#include <string.h>
#include <errno.h>
#include <net/net_pkt.h>
#include <net/net_if.h>
#include <net/net_l2.h>
#include <net/net_context.h>
#include <net/net_offload.h>

#include <misc/printk.h>

/* We do not need <socket/include/socket.h>
 * It seems there is a bug in ASF side: if OS is already defining sockaddr
 * and all, ASF will not need to define it. Unfortunately its socket.h does
 * but also defines some NM API functions there (??), so we need to redefine
 * those here.
 */
#define __SOCKET_H__
#define HOSTNAME_MAX_SIZE	(64)

#include <driver/include/m2m_wifi.h>
#include <socket/include/m2m_socket_host_if.h>

NMI_API void socketInit(void);
typedef void (*tpfAppSocketCb) (SOCKET sock, uint8 u8Msg, void *pvMsg);
typedef void (*tpfAppResolveCb) (uint8 *pu8DomainName, uint32 u32ServerIP);
NMI_API void registerSocketCallback(tpfAppSocketCb socket_cb,
				    tpfAppResolveCb resolve_cb);
NMI_API SOCKET socket(uint16 u16Domain, uint8 u8Type, uint8 u8Flags);
NMI_API sint8 bind(SOCKET sock, struct sockaddr *pstrAddr, uint8 u8AddrLen);
NMI_API sint8 listen(SOCKET sock, uint8 backlog);
NMI_API sint8 accept(SOCKET sock, struct sockaddr *addr, uint8 *addrlen);
NMI_API sint8 connect(SOCKET sock, struct sockaddr *pstrAddr, uint8 u8AddrLen);
NMI_API sint16 recv(SOCKET sock, void *pvRecvBuf,
		    uint16 u16BufLen, uint32 u32Timeoutmsec);
NMI_API sint16 send(SOCKET sock, void *pvSendBuffer,
		    uint16 u16SendLength, uint16 u16Flags);
NMI_API sint16 sendto(SOCKET sock, void *pvSendBuffer,
		      uint16 u16SendLength, uint16 flags,
		      struct sockaddr *pstrDestAddr, uint8 u8AddrLen);

enum socket_errors {
	SOCK_ERR_NO_ERROR = 0,
	SOCK_ERR_INVALID_ADDRESS = -1,
	SOCK_ERR_ADDR_ALREADY_IN_USE = -2,
	SOCK_ERR_MAX_TCP_SOCK = -3,
	SOCK_ERR_MAX_UDP_SOCK = -4,
	SOCK_ERR_INVALID_ARG = -6,
	SOCK_ERR_MAX_LISTEN_SOCK = -7,
	SOCK_ERR_INVALID = -9,
	SOCK_ERR_ADDR_IS_REQUIRED = -11,
	SOCK_ERR_CONN_ABORTED = -12,
	SOCK_ERR_TIMEOUT = -13,
	SOCK_ERR_BUFFER_FULL = -14,
};

enum socket_messages {
	SOCKET_MSG_BIND	= 1,
	SOCKET_MSG_LISTEN,
	SOCKET_MSG_DNS_RESOLVE,
	SOCKET_MSG_ACCEPT,
	SOCKET_MSG_CONNECT,
	SOCKET_MSG_RECV,
	SOCKET_MSG_SEND,
	SOCKET_MSG_SENDTO,
	SOCKET_MSG_RECVFROM,
};

typedef struct {
	sint8	status;
} tstrSocketBindMsg;

typedef struct {
	sint8	status;
} tstrSocketListenMsg;

typedef struct {
	SOCKET			sock;
	struct sockaddr_in	strAddr;
} tstrSocketAcceptMsg;

typedef struct {
	SOCKET	sock;
	sint8	s8Error;
} tstrSocketConnectMsg;

typedef struct {
	uint8			*pu8Buffer;
	sint16			s16BufferSize;
	uint16			u16RemainingSize;
	struct sockaddr_in	strRemoteAddr;
} tstrSocketRecvMsg;

#include <driver/include/m2m_wifi.h>
#include <socket/include/m2m_socket_host_if.h>

#ifndef CONFIG_WINC1500_THREAD_STACK_SIZE
#define CONFIG_WINC1500_THREAD_STACK_SIZE 5000
#endif
#ifndef CONFIG_WINC1500_THREAD_PRIO
#define CONFIG_WINC1500_THREAD_PRIO 2
#endif

#ifndef CONFIG_WINC1500_BUF_CTR
#define CONFIG_WINC1500_BUF_CTR 1
#endif
#ifndef CONFIG_WINC1500_MAX_PACKET_SIZE
#define CONFIG_WINC1500_MAX_PACKET_SIZE 1500
#endif
#ifndef CONFIG_OFFLOAD_MAX_SOCKETS
#define CONFIG_OFFLOAD_MAX_SOCKETS 2
#endif

#define WINC1500_BIND_TIMEOUT 500
#define WINC1500_LISTEN_TIMEOUT 500

NET_BUF_POOL_DEFINE(winc1500_tx_pool, CONFIG_WINC1500_BUF_CTR,
		    CONFIG_WINC1500_MAX_PACKET_SIZE, 0, NULL);
NET_BUF_POOL_DEFINE(winc1500_rx_pool, CONFIG_WINC1500_BUF_CTR,
		    CONFIG_WINC1500_MAX_PACKET_SIZE, 0, NULL);

K_THREAD_STACK_MEMBER(winc1500_stack, CONFIG_WINC1500_THREAD_STACK_SIZE);
struct k_thread winc1500_thread_data;

struct socket_data {
	struct net_context		*context;
	net_context_connect_cb_t	connect_cb;
	net_tcp_accept_cb_t		accept_cb;
	net_context_send_cb_t		send_cb;
	net_context_recv_cb_t		recv_cb;
	void				*connect_user_data;
	void				*send_user_data;
	void				*recv_user_data;
	void				*accept_user_data;
	struct net_pkt			*rx_pkt;
	struct net_buf			*pkt_buf;
	int				ret_code;
	struct k_sem			wait_sem;
};

struct winc1500_data {
	struct socket_data socket_data[CONFIG_OFFLOAD_MAX_SOCKETS];
	struct net_if *iface;
	unsigned char mac[6];
};

static struct winc1500_data w1500_data;

#if (SYS_LOG_LEVEL > SYS_LOG_LEVEL_OFF)

static char *socket_error_string(s8_t err)
{
	switch (err) {
	case SOCK_ERR_NO_ERROR:
		return "Successful socket operation";
	case SOCK_ERR_INVALID_ADDRESS:
		return "Socket address is invalid."
			"The socket operation cannot be completed successfully"
			" without specifying a specific address. "
			"For example: bind is called without specifying"
			" a port number";
	case SOCK_ERR_ADDR_ALREADY_IN_USE:
		return "Socket operation cannot bind on the given address."
			" With socket operations, only one IP address per "
			"socket is permitted. Any attempt for a new socket "
			"to bind with an IP address already bound to another "
			"open socket, will return the following error code. "
			"States that bind operation failed.";
	case SOCK_ERR_MAX_TCP_SOCK:
		return "Exceeded the maximum number of TCP sockets."
			"A maximum number of TCP sockets opened simultaneously"
			" is defined through TCP_SOCK_MAX. It is not permitted"
			" to exceed that number at socket creation."
			" Identifies that @ref socket operation failed.";
	case SOCK_ERR_MAX_UDP_SOCK:
		return "Exceeded the maximum number of UDP sockets."
			"A maximum number of UDP sockets opened simultaneously"
			" is defined through UDP_SOCK_MAX. It is not permitted"
			" to exceed that number at socket creation."
			" Identifies that socket operation failed";
	case SOCK_ERR_INVALID_ARG:
		return "An invalid argument is passed to a function.";
	case SOCK_ERR_MAX_LISTEN_SOCK:
		return "Exceeded the maximum number of TCP passive listening "
			"sockets. Identifies Identifies that listen operation"
			" failed.";
	case SOCK_ERR_INVALID:
		return "The requested socket operation is not valid in the "
			"current socket state. For example: @ref accept is "
			"called on a TCP socket before bind or listen.";
	case SOCK_ERR_ADDR_IS_REQUIRED:
		return "Destination address is required. Failure to provide "
			"the socket address required for the socket operation "
			"to be completed. It is generated as an error to the "
			"sendto function when the address required to send the"
			" data to is not known.";
	case SOCK_ERR_CONN_ABORTED:
		return "The socket is closed by the peer. The local socket is "
			"also closed.";
	case SOCK_ERR_TIMEOUT:
		return "The socket pending operation has  timedout.";
	case SOCK_ERR_BUFFER_FULL:
		return "No buffer space available to be used for the requested"
			" socket operation.";
	default:
		return "Unknown";
	}
}

static char *wifi_cb_msg_2_str(u8_t message_type)
{
	switch (message_type) {
	case M2M_WIFI_RESP_CURRENT_RSSI:
		return "M2M_WIFI_RESP_CURRENT_RSSI";
	case M2M_WIFI_REQ_WPS:
		return "M2M_WIFI_REQ_WPS";
	case M2M_WIFI_RESP_IP_CONFIGURED:
		return "M2M_WIFI_RESP_IP_CONFIGURED";
	case M2M_WIFI_RESP_IP_CONFLICT:
		return "M2M_WIFI_RESP_IP_CONFLICT";
	case M2M_WIFI_RESP_CLIENT_INFO:
		return "M2M_WIFI_RESP_CLIENT_INFO";

	case M2M_WIFI_RESP_GET_SYS_TIME:
		return "M2M_WIFI_RESP_GET_SYS_TIME";
	case M2M_WIFI_REQ_SEND_ETHERNET_PACKET:
		return "M2M_WIFI_REQ_SEND_ETHERNET_PACKET";
	case M2M_WIFI_RESP_ETHERNET_RX_PACKET:
		return "M2M_WIFI_RESP_ETHERNET_RX_PACKET";

	case M2M_WIFI_RESP_CON_STATE_CHANGED:
		return "M2M_WIFI_RESP_CON_STATE_CHANGED";
	case M2M_WIFI_REQ_DHCP_CONF:
		return "Response indicating that IP address was obtained.";
	case M2M_WIFI_RESP_SCAN_DONE:
		return "M2M_WIFI_RESP_SCAN_DONE";
	case M2M_WIFI_RESP_SCAN_RESULT:
		return "M2M_WIFI_RESP_SCAN_RESULT";
	case M2M_WIFI_RESP_PROVISION_INFO:
		return "M2M_WIFI_RESP_PROVISION_INFO";
	default:
		return "UNKNOWN";
	}
}

static char *socket_message_to_string(u8_t message)
{
	switch (message) {
	case SOCKET_MSG_BIND:
		return "Bind socket event.";
	case SOCKET_MSG_LISTEN:
		return "Listen socket event.";
	case SOCKET_MSG_DNS_RESOLVE:
		return "DNS Resolution event.";
	case SOCKET_MSG_ACCEPT:
		return "Accept socket event.";
	case SOCKET_MSG_CONNECT:
		return "Connect socket event.";
	case SOCKET_MSG_RECV:
		return "Receive socket event.";
	case SOCKET_MSG_SEND:
		return "Send socket event.";
	case SOCKET_MSG_SENDTO:
		return "Sendto socket event.";
	case SOCKET_MSG_RECVFROM:
		return "Recvfrom socket event.";
	default:
		return "UNKNOWN.";
	}
}

#endif /* (SYS_LOG_LEVEL > SYS_LOG_LEVEL_OFF) */

/**
 * This function is called when the socket is to be opened.
 */
static int winc1500_get(sa_family_t family,
			enum net_sock_type type,
			enum net_ip_protocol ip_proto,
			struct net_context **context)
{
	struct socket_data *sd;

	if (family != AF_INET) {
		SYS_LOG_ERR("Only AF_INET is supported!");
		return -1;
	}

	(*context)->user_data = (void *)(sint32)socket(family, type, 0);
	if ((*context)->user_data < 0) {
		SYS_LOG_ERR("socket error!");
		return -1;
	}

	sd = &w1500_data.socket_data[(int)(*context)->user_data];

	k_sem_init(&sd->wait_sem, 0, 1);

	sd->context = *context;

	return 0;
}

/**
 * This function is called when user wants to bind to local IP address.
 */
static int winc1500_bind(struct net_context *context,
			 const struct sockaddr *addr,
			 socklen_t addrlen)
{
	SOCKET socket = (int)context->user_data;
	int ret;

	/* FIXME atmel winc1500 don't support bind on null port */
	if (net_sin(addr)->sin_port == 0) {
		return 0;
	}

	ret = bind((int)context->user_data, (struct sockaddr *)addr, addrlen);
	if (ret) {
		SYS_LOG_ERR("bind error %d %s!",
			    ret, socket_message_to_string(ret));
		return ret;
	}

	if (k_sem_take(&w1500_data.socket_data[socket].wait_sem,
		       WINC1500_BIND_TIMEOUT)) {
		SYS_LOG_ERR("bind error timeout expired");
		return -ETIMEDOUT;
	}

	return w1500_data.socket_data[socket].ret_code;
}

/**
 * This function is called when user wants to mark the socket
 * to be a listening one.
 */
static int winc1500_listen(struct net_context *context, int backlog)
{
	SOCKET socket = (int)context->user_data;
	int ret;

	ret = listen((int)context->user_data, backlog);
	if (ret) {
		SYS_LOG_ERR("listen error %d %s!",
			    ret, socket_error_string(ret));
		return ret;
	}

	if (k_sem_take(&w1500_data.socket_data[socket].wait_sem,
		       WINC1500_LISTEN_TIMEOUT)) {
		return -ETIMEDOUT;
	}

	return w1500_data.socket_data[socket].ret_code;
}

/**
 * This function is called when user wants to create a connection
 * to a peer host.
 */
static int winc1500_connect(struct net_context *context,
			    const struct sockaddr *addr,
			    socklen_t addrlen,
			    net_context_connect_cb_t cb,
			    s32_t timeout,
			    void *user_data)
{
	SOCKET socket = (int)context->user_data;
	int ret;

	w1500_data.socket_data[socket].connect_cb = cb;
	w1500_data.socket_data[socket].connect_user_data = user_data;
	w1500_data.socket_data[socket].ret_code = 0;

	ret = connect(socket, (struct sockaddr *)addr, addrlen);
	if (ret) {
		SYS_LOG_ERR("connect error %d %s!",
			    ret, socket_error_string(ret));
		return ret;
	}

	if (timeout != 0 &&
	    k_sem_take(&w1500_data.socket_data[socket].wait_sem, timeout)) {
		return -ETIMEDOUT;
	}

	return w1500_data.socket_data[socket].ret_code;
}

/**
 * This function is called when user wants to accept a connection
 * being established.
 */
static int winc1500_accept(struct net_context *context,
			   net_tcp_accept_cb_t cb,
			   s32_t timeout,
			   void *user_data)
{
	SOCKET socket = (int)context->user_data;
	int ret;

	w1500_data.socket_data[socket].accept_cb = cb;

	ret = accept(socket, NULL, 0);
	if (ret) {
		SYS_LOG_ERR("accept error %d %s!",
			    ret, socket_error_string(ret));
		return ret;
	}

	if (timeout) {
		if (k_sem_take(&w1500_data.socket_data[socket].wait_sem,
			       timeout)) {
			return -ETIMEDOUT;
		}
	} else {
		k_sem_take(&w1500_data.socket_data[socket].wait_sem,
			   K_FOREVER);
	}

	return w1500_data.socket_data[socket].ret_code;
}

/**
 * This function is called when user wants to send data to peer host.
 */
static int winc1500_send(struct net_pkt *pkt,
			 net_context_send_cb_t cb,
			 s32_t timeout,
			 void *token,
			 void *user_data)
{
	struct net_context *context = pkt->context;
	SOCKET socket = (int)context->user_data;
	bool first_frag;
	struct net_buf *frag;
	int ret;

	w1500_data.socket_data[socket].send_cb = cb;
	w1500_data.socket_data[socket].send_user_data = user_data;

	first_frag = true;

	for (frag = pkt->frags; frag; frag = frag->frags) {
		u8_t *data_ptr;
		u16_t data_len;

		if (first_frag) {
			data_ptr = net_pkt_ll(pkt);
			data_len = net_pkt_ll_reserve(pkt) + frag->len;
			first_frag = false;
		} else {
			data_ptr = frag->data;
			data_len = frag->len;

		}

		ret = send(socket, data_ptr, data_len, 0);
		if (ret) {
			SYS_LOG_ERR("send error %d %s!",
				    ret, socket_error_string(ret));
			return ret;
		}
	}

	net_pkt_unref(pkt);

	return 0;
}

/**
 * This function is called when user wants to send data to peer host.
 */
static int winc1500_sendto(struct net_pkt *pkt,
			   const struct sockaddr *dst_addr,
			   socklen_t addrlen,
			   net_context_send_cb_t cb,
			   s32_t timeout,
			   void *token,
			   void *user_data)
{
	struct net_context *context = pkt->context;
	SOCKET socket = (int)context->user_data;
	bool first_frag;
	struct net_buf *frag;
	int ret;

	w1500_data.socket_data[socket].send_cb = cb;
	w1500_data.socket_data[socket].send_user_data = user_data;

	first_frag = true;

	for (frag = pkt->frags; frag; frag = frag->frags) {
		u8_t *data_ptr;
		u16_t data_len;

		if (first_frag) {
			data_ptr = net_pkt_ll(pkt);
			data_len = net_pkt_ll_reserve(pkt) + frag->len;
			first_frag = false;
		} else {
			data_ptr = frag->data;
			data_len = frag->len;
		}

		ret = sendto(socket, data_ptr, data_len, 0,
			     (struct sockaddr *)dst_addr, addrlen);
		if (ret) {
			SYS_LOG_ERR("send error %d %s!",
				    ret, socket_error_string(ret));
			return ret;
		}
	}

	net_pkt_unref(pkt);

	return 0;
}

/**
 */
static int prepare_pkt(struct socket_data *sock_data)
{
	/* Get the frame from the buffer */
	sock_data->rx_pkt = net_pkt_get_reserve_rx(0, K_NO_WAIT);
	if (!sock_data->rx_pkt) {
		SYS_LOG_ERR("Could not allocate rx packet");
		return -1;
	}

	/* Reserve a data frag to receive the frame */
	sock_data->pkt_buf = net_pkt_get_frag(sock_data->rx_pkt, K_NO_WAIT);
	if (!sock_data->pkt_buf) {
		SYS_LOG_ERR("Could not allocate data frag");
		net_pkt_unref(sock_data->rx_pkt);
		return -1;
	}

	net_pkt_frag_insert(sock_data->rx_pkt, sock_data->pkt_buf);

	return 0;
}

/**
 * This function is called when user wants to receive data from peer
 * host.
 */
static int winc1500_recv(struct net_context *context,
			 net_context_recv_cb_t cb,
			 s32_t timeout,
			 void *user_data)
{
	SOCKET socket = (int) context->user_data;
	int ret;

	ret = prepare_pkt(&w1500_data.socket_data[socket]);
	if (ret) {
		SYS_LOG_ERR("Could not reserve packet buffer");
		return -ENOMEM;
	}

	w1500_data.socket_data[socket].recv_cb = cb;
	w1500_data.socket_data[socket].recv_user_data = user_data;

	ret = recv(socket, w1500_data.socket_data[socket].pkt_buf->data,
		   CONFIG_WINC1500_MAX_PACKET_SIZE, timeout);
	if (ret) {
		SYS_LOG_ERR("recv error %d %s!",
			    ret, socket_error_string(ret));
		return ret;
	}

	return 0;
}

/**
 * This function is called when user wants to close the socket.
 */
static int winc1500_put(struct net_context *context)
{
	return 0;
}

static struct net_offload winc1500_offload = {
	.get		= winc1500_get,
	.bind		= winc1500_bind,
	.listen		= winc1500_listen,
	.connect	= winc1500_connect,
	.accept		= winc1500_accept,
	.send		= winc1500_send,
	.sendto		= winc1500_sendto,
	.recv		= winc1500_recv,
	.put		= winc1500_put,
};

static void winc1500_wifi_cb(u8_t message_type, void *pvMsg)
{
	SYS_LOG_INF("Msg Type %d %s",
		    message_type, wifi_cb_msg_2_str(message_type));

	switch (message_type) {
	case M2M_WIFI_RESP_CON_STATE_CHANGED:
	{
		tstrM2mWifiStateChanged *pstrWifiState =
			(tstrM2mWifiStateChanged *)pvMsg;

		switch (pstrWifiState->u8CurrState) {
		case M2M_WIFI_DISCONNECTED:
			/* TODO status disconnected */
			break;
		case M2M_WIFI_CONNECTED:
			/* TODO status connected */
			break;
		case M2M_WIFI_UNDEF:
			/* TODO status undefined*/
			break;
		}
	}

	break;

	case M2M_WIFI_REQ_DHCP_CONF:
	{
		u8_t *pu8IPAddress = (u8_t *)pvMsg;
		struct in_addr addr;
		u8_t i;

		/* Connected and got IP address*/
		SYS_LOG_INF("Wi-Fi connected, IP is %u.%u.%u.%u",
			    pu8IPAddress[0], pu8IPAddress[1],
			    pu8IPAddress[2], pu8IPAddress[3]);
		/* TODO at this point the standby mode should be enable
		 * status = WiFi connected IP assigned
		 */
		for (i = 0; i < 4; i++) {
			addr.s4_addr[i] = pu8IPAddress[i];
		}

		/* TODO fill in net mask, gateway and lease time */

		net_if_ipv4_addr_add(w1500_data.iface,
				     &addr, NET_ADDR_DHCP, 0);
	}

	break;

	default:
		break;
	}

}

static void winc1500_socket_cb(SOCKET sock, uint8 message, void *pvMsg)
{
	struct socket_data *sd = &w1500_data.socket_data[sock];

	if (message != 6) {
		SYS_LOG_INF(": sock %d Msg %d %s",
			    sock, message, socket_message_to_string(message));
	}

	sd->ret_code = 0;

	switch (message) {
	case SOCKET_MSG_CONNECT:
	{
		tstrSocketConnectMsg *strConnMsg =
			(tstrSocketConnectMsg *)pvMsg;

		SYS_LOG_ERR("CONNECT: socket %d error %d",
			    strConnMsg->sock, strConnMsg->s8Error);

		if (sd->connect_cb) {
			sd->connect_cb(sd->context,
				       strConnMsg->s8Error,
				       sd->connect_user_data);
		}

		sd->ret_code = strConnMsg->s8Error;
	}
		k_sem_give(&sd->wait_sem);
		break;

	case SOCKET_MSG_SEND:
		break;

	case SOCKET_MSG_RECV:
	{
		tstrSocketRecvMsg *pstrRx = (tstrSocketRecvMsg *)pvMsg;

		if ((pstrRx->pu8Buffer != NULL) &&
		    (pstrRx->s16BufferSize > 0)) {

			net_buf_add(sd->pkt_buf,
				    pstrRx->s16BufferSize);

			net_pkt_set_appdata(sd->rx_pkt,
					    sd->pkt_buf->data);
			net_pkt_set_appdatalen(sd->rx_pkt,
					       pstrRx->s16BufferSize);

			if (sd->recv_cb) {
				sd->recv_cb(sd->context,
					    sd->rx_pkt,
					    0,
					    sd->recv_user_data);
			}
		} else if (pstrRx->pu8Buffer == NULL) {
			if (pstrRx->s16BufferSize == SOCK_ERR_CONN_ABORTED) {
				net_pkt_unref(sd->rx_pkt);
				return;
			}
		}

		if (prepare_pkt(&w1500_data.socket_data[sock])) {
			SYS_LOG_ERR("Could not reserve packet buffer");
			return;
		}

		recv(sock, sd->pkt_buf->data,
		     CONFIG_WINC1500_MAX_PACKET_SIZE, K_NO_WAIT);
	}
		break;
	case SOCKET_MSG_BIND:
	{
		/* The result of the bind operation.
		 * Holding a value of ZERO for a successful
		 * bind or otherwise a negative error code
		 * corresponding to the type of error.
		 */
		tstrSocketBindMsg *bind_msg = (tstrSocketBindMsg *)pvMsg;

		if (bind_msg->status) {
			SYS_LOG_ERR("BIND: error %d %s",
				    bind_msg->status,
				    socket_message_to_string(bind_msg->status));
			sd->ret_code = bind_msg->status;
		}
	}
		k_sem_give(&sd->wait_sem);
		break;
	case SOCKET_MSG_LISTEN:
	{
		/* Holding a value of ZERO for a successful listen or
		 * otherwise a negative error code corresponding to
		 * the type of error.
		 */
		tstrSocketListenMsg *listen_msg = (tstrSocketListenMsg *)pvMsg;

		if (listen_msg->status) {
			SYS_LOG_ERR("winc1500_socket_cb:LISTEN: error %d %s\n",
				    listen_msg->status,
				    socket_message_to_string(listen_msg->status));
			sd->ret_code = listen_msg->status;
		}
	}
		k_sem_give(&sd->wait_sem);
		break;
	case SOCKET_MSG_ACCEPT:
	{
		/* sock
		 *   On a successful accept operation, the return information
		 *   is the socket ID for the accepted connection with the
		 *   remote peer.
		 *   Otherwise a negative error code is returned to indicate
		 *   failure of the accept operation.
		 * strAddr;
		 *   Socket address structure for the remote peer.
		 */
		tstrSocketAcceptMsg *accept_msg = (tstrSocketAcceptMsg *)pvMsg;

		SYS_LOG_INF("ACCEPT:"
			    "from %d.%d.%d.%d:%d, new socket is %d",
			    accept_msg->strAddr.sin_addr.s4_addr[0],
			    accept_msg->strAddr.sin_addr.s4_addr[1],
			    accept_msg->strAddr.sin_addr.s4_addr[2],
			    accept_msg->strAddr.sin_addr.s4_addr[3],
			    ntohs(accept_msg->strAddr.sin_port),
			    accept_msg->sock);

		if (accept_msg->sock < 0) {
			SYS_LOG_ERR("ACCEPT: error %d %s",
				    accept_msg->sock,
				    socket_message_to_string(accept_msg->sock));
			sd->ret_code = accept_msg->sock;
		}

		if (sd->accept_cb) {
			struct socket_data *a_sd;
			int ret;

			a_sd = &w1500_data.socket_data[accept_msg->sock];

			memcpy(a_sd, sd, sizeof(struct socket_data));

			ret = net_context_get(AF_INET, SOCK_STREAM,
					      IPPROTO_TCP, &a_sd->context);
			if (ret < 0) {
				SYS_LOG_ERR("Cannot get new network"
					    "context for ACCEPT");
			} else {
				a_sd->context->user_data =
					(void *)((int)accept_msg->sock);

				sd->accept_cb(
					a_sd->context,
					(struct sockaddr *)&accept_msg->strAddr,
					sizeof(struct sockaddr_in),
					(accept_msg->sock > 0) ?
					0 :accept_msg->sock,
					sd->accept_user_data);
			}
		}
	}
		k_sem_give(&sd->wait_sem);
		break;
	}
}

static void winc1500_thread(void)
{
	while (1) {
		while (m2m_wifi_handle_events(NULL) != 0) {
		}

		k_sleep(K_MSEC(1));
	}
}


static void winc1500_iface_init(struct net_if *iface)
{
	SYS_LOG_INF("eth_init:net_if_set_link_addr:"
		    "MAC Address %02X:%02X:%02X:%02X:%02X:%02X",
		    w1500_data.mac[0], w1500_data.mac[1], w1500_data.mac[2],
		    w1500_data.mac[3], w1500_data.mac[4], w1500_data.mac[5]);

	net_if_set_link_addr(iface, w1500_data.mac, sizeof(w1500_data.mac),
			     NET_LINK_ETHERNET);

	iface->if_dev->offload = &winc1500_offload;

	w1500_data.iface = iface;
}

static const struct net_if_api winc1500_api = {
	.init = winc1500_iface_init,
};

static int winc1500_init(struct device *dev)
{
	tstrWifiInitParam param = {
		.pfAppWifiCb = winc1500_wifi_cb,
	};
	tuniM2MWifiAuth creds = {
		.au8PSK = CONFIG_WIFI_WINC1500_PSK,
	};
	unsigned char is_valid;
	int ret;

	ARG_UNUSED(dev);

	ret = m2m_wifi_init(&param);
	if (ret) {
		SYS_LOG_ERR("m2m_wifi_init return error!(%d)", ret);
		return -EIO;
	}

	socketInit();
	registerSocketCallback(winc1500_socket_cb, NULL);

	m2m_wifi_get_otp_mac_address(w1500_data.mac, &is_valid);
	SYS_LOG_INF("WINC1500 MAC Address from OTP (%d) "
		    "%02X:%02X:%02X:%02X:%02X:%02X",
		    is_valid,
		    w1500_data.mac[0], w1500_data.mac[1], w1500_data.mac[2],
		    w1500_data.mac[3], w1500_data.mac[4], w1500_data.mac[5]);

	m2m_wifi_set_power_profile(PWR_LOW1);
	m2m_wifi_set_tx_power(TX_PWR_LOW);

	/* monitoring thread for winc wifi callbacks */
	k_thread_create(&winc1500_thread_data, winc1500_stack,
			CONFIG_WINC1500_THREAD_STACK_SIZE,
			(k_thread_entry_t)winc1500_thread, NULL, NULL, NULL,
			K_PRIO_COOP(CONFIG_WINC1500_THREAD_PRIO),
			0, K_NO_WAIT);

	SYS_LOG_INF("Connecting to %s (%u) with %s",
		    CONFIG_WIFI_WINC1500_SSID,
		    CONFIG_WIFI_WINC1500_SSID_LENGTH,
		    CONFIG_WIFI_WINC1500_PSK);

	ret = m2m_wifi_connect(CONFIG_WIFI_WINC1500_SSID,
			       CONFIG_WIFI_WINC1500_SSID_LENGTH,
			       M2M_WIFI_SEC_WPA_PSK, &creds,
			       M2M_WIFI_CH_ALL);
	if (ret) {
		SYS_LOG_WRN("Connecting to wifi does not seem to work");
	}

	SYS_LOG_INF("WINC1500 driver Initialized");

	return 0;
}

NET_DEVICE_OFFLOAD_INIT(winc1500, CONFIG_WIFI_WINC1500_NAME,
			winc1500_init, &w1500_data, NULL,
			CONFIG_WIFI_INIT_PRIORITY, &winc1500_api,
			CONFIG_WINC1500_MAX_PACKET_SIZE);

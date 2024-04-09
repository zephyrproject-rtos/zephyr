/*
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(online_checker, CONFIG_NET_CONNECTION_MANAGER_LOG_LEVEL);

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <errno.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/icmp.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/conn_mgr_monitor.h>
#include <zephyr/net/http/client.h>
#include <zephyr/net/trickle.h>
#include "conn_mgr_private.h"
#include "net_private.h"

#if defined(CONFIG_NET_CONNECTION_MANAGER_ONLINE_CHECK_PING)
#define PING_HOST CONFIG_NET_CONNECTION_MANAGER_ONLINE_CHECK_PING_HOST
#else
#define PING_HOST ""
#endif

#if defined(CONFIG_NET_CONNECTION_MANAGER_ONLINE_CHECK_HTTP)
#define ONLINE_CHECK_URL CONFIG_NET_CONNECTION_MANAGER_ONLINE_CHECK_HTTP_URL
#define MAX_RECV_BUF_LEN 512
#else
#define ONLINE_CHECK_URL ""
#define MAX_RECV_BUF_LEN 1
#endif

#define WAIT_TIMEOUT CONFIG_NET_CONNECTION_MANAGER_ONLINE_CHECK_TIMEOUT
#define MAX_HOSTNAME_LEN CONFIG_NET_CONNECTION_MANAGER_ONLINE_CHECK_MAX_HOSTNAME_LEN

#if defined(CONFIG_NET_CONNECTION_MANAGER_ONLINE_VERIFY)
#define TRICKLE_IMIN CONFIG_NET_CONNECTION_MANAGER_ONLINE_VERIFY_IMIN
#define TRICKLE_IMAX CONFIG_NET_CONNECTION_MANAGER_ONLINE_VERIFY_IMAX
#define TRICKLE_K CONFIG_NET_CONNECTION_MANAGER_ONLINE_VERIFY_K
#define VERIFY_PERIOD CONFIG_NET_CONNECTION_MANAGER_ONLINE_VERIFY_PERIOD
#define PRIVATE_ADDR_CHECK IS_ENABLED(CONFIG_NET_CONNECTION_MANAGER_ONLINE_VERIFY_PRIV_ADDR_CHECK)
#else
#define TRICKLE_IMIN 0
#define TRICKLE_IMAX 0
#define TRICKLE_K 0
#define VERIFY_PERIOD 0
#define PRIVATE_ADDR_CHECK 0
#endif

struct online_check_data {
	enum net_conn_mgr_online_check_type strategy;
	net_conn_mgr_online_checker_t cb;
	void *user_data;
	const char *url;
	char *host; /* This points to hostname_port */
	char *port; /* and same for this */
	struct sockaddr hostaddr;
	struct net_trickle verifier;
	struct {
		/* Trickle algorithm options */
		uint32_t Imin;
		uint8_t Imax;
		uint8_t k;
	} trickle;
	struct k_work_delayable verifier_work;
	char hostname_port[MAX_HOSTNAME_LEN];
	uint8_t http_recv_buf[MAX_RECV_BUF_LEN];
	bool hostaddr_valid : 1;
	bool is_tls : 1;
	bool pkts_received : 1;
	bool running : 1;
};

static struct online_check_data online_check_data_storage;
static struct online_check_data *online_check = &online_check_data_storage;

static void stop_online_check(void);

static K_MUTEX_DEFINE(check_running_lock);
static bool check_running;
static uint16_t *states;

bool conn_mgr_trigger_online_checks;

/* Enabling this can cause lot of prints so do it if really needed */
static bool debug_rx_src;

void conn_mgr_online_checker_update(struct net_pkt *pkt,
				    void *pkt_hdr,
				    sa_family_t family,
				    bool is_loopback)
{
	if (!IS_ENABLED(CONFIG_NET_CONNECTION_MANAGER_ONLINE_VERIFY)) {
		return;
	}

	/* We cannot verify online connectivity from loopback interface */
	if (is_loopback) {
		return;
	}

	if (states == NULL || states[conn_mgr_get_index_for_if(net_pkt_iface(pkt))] == 0) {
		/* We are not interested in this interface */
		return;
	}

	/* Check if the source address of the packet is not local */
	if (IS_ENABLED(CONFIG_NET_IPV6) && family == AF_INET6) {
		struct net_ipv6_hdr *hdr = pkt_hdr;

		if (net_ipv6_is_ll_addr((struct in6_addr *)hdr->src)) {
			return;
		}

		if (PRIVATE_ADDR_CHECK) {
			if (net_ipv6_is_ula_addr((struct in6_addr *)hdr->src)) {
				return;
			}

			if (net_ipv6_is_private_addr((struct in6_addr *)hdr->src)) {
				return;
			}
		}

		if (debug_rx_src) {
			NET_DBG("Recv src addr %s", net_sprint_ipv6_addr(hdr->src));
		}

		online_check->pkts_received = true;
		net_trickle_consistency(&online_check->verifier);

		return;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4) && family == AF_INET) {
		struct net_ipv4_hdr *hdr = pkt_hdr;

		if (net_ipv4_is_ll_addr((struct in_addr *)hdr->src)) {
			return;
		}

		if (PRIVATE_ADDR_CHECK) {
			if (net_ipv4_is_private_addr((struct in_addr *)hdr->src)) {
				return;
			}
		}

		if (debug_rx_src) {
			NET_DBG("Recv src addr %s", net_sprint_ipv4_addr(hdr->src));
		}

		online_check->pkts_received = true;
		net_trickle_consistency(&online_check->verifier);

		return;
	}
}

void conn_mgr_refresh_online_connectivity_check(void)
{
	/* All interfaces are down. Disable Trickle timer and
	 * generate offline event.
	 */
	net_mgmt_event_notify(NET_EVENT_CONNECTIVITY_OFFLINE, NULL);

	stop_online_check();

	conn_mgr_trigger_online_connectivity_check();
}

static void verifier_cb(struct net_trickle *trickle, bool do_suppress,
			void *user_data)
{
	ARG_UNUSED(user_data);

	NET_DBG("Verifier %p callback called (suppress %d)", trickle,
		do_suppress);

	/* Check if we have received any data between calls to this cb. */
	if (do_suppress) {
		NET_DBG("No data transfer seen, not considered online.");
		conn_mgr_refresh_online_connectivity_check();
	} else {
		NET_DBG("Data transfer seen, considering still online.");
	}
}

static void verifier_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct online_check_data *online_data =
		CONTAINER_OF(dwork, struct online_check_data, verifier_work);
	bool is_consistent = online_data->pkts_received;

	online_data->pkts_received = false;

	if (!is_consistent) {
		net_trickle_inconsistency(&online_data->verifier);
	}
}

/* We are now online but we also want to know if we lost online connectivity.
 * Start to monitor network traffic and if we do not see proper traffic,
 * then re-trigger the online check.
 */
static int enable_online_state_verifier(void)
{
	k_timeout_t timeout;
	k_timepoint_t next;
	int ret;

	if (!IS_ENABLED(CONFIG_NET_CONNECTION_MANAGER_ONLINE_VERIFY)) {
		return -ENOTSUP;
	}

	if (net_trickle_is_running(&online_check->verifier)) {
		NET_WARN("Online connectivity timer already running.");
		return -EALREADY;
	}

	ret = net_trickle_create(&online_check->verifier,
				 online_check->trickle.Imin,
				 online_check->trickle.Imax,
				 online_check->trickle.k);
	if (ret < 0) {
		NET_WARN("Cannot %s verifier (%d)", "create", ret);
		return ret;
	}

	ret = net_trickle_start(&online_check->verifier,
				verifier_cb, online_check);
	if (ret < 0) {
		NET_WARN("Cannot %s verifier (%d)", "activate", ret);
		return ret;
	}

	k_work_init_delayable(&online_check->verifier_work, verifier_timeout);

	next = sys_timepoint_calc(K_MINUTES(VERIFY_PERIOD));
	timeout = sys_timepoint_timeout(next);

	k_work_reschedule(&online_check->verifier_work, timeout);

	/* The timer is disabled if we are dropped from online state */

	return 0;
}

static enum dns_resolve_status resolve_host(const char *host,
					    const char *service,
					    int socktype,
					    struct zsock_addrinfo **res)
{
	struct zsock_addrinfo hints;
	int ret;

	if (!host || host[0] == '\0') {
		NET_WARN("Online check hostname missing.");
		return DNS_EAI_NONAME;
	}

	memset(&hints, 0, sizeof(hints));
	if (IS_ENABLED(CONFIG_NET_IPV4) && !IS_ENABLED(CONFIG_NET_IPV6)) {
		hints.ai_family = AF_INET;
	} else if (!IS_ENABLED(CONFIG_NET_IPV4) && IS_ENABLED(CONFIG_NET_IPV6)) {
		hints.ai_family = AF_INET6;
	} else {
		hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
	}

	hints.ai_socktype = socktype;

	NET_DBG("Resolving %s", host);

	ret = zsock_getaddrinfo(host, service, &hints, res);
	if (ret != 0) {
		NET_DBG("Cannot get %s (%d/%s)", host,
			ret, zsock_gai_strerror(ret));
		return ret;
	}

	return 0;
}

static int icmp_handler(struct net_icmp_ctx *ctx,
			struct net_pkt *pkt,
			struct net_icmp_ip_hdr *hdr,
			struct net_icmp_hdr *icmp_hdr,
			void *user_data)
{
	struct k_sem *wait_sem = user_data;

	if (hdr->family == AF_INET) {
		struct net_ipv4_hdr *ip_hdr = hdr->ipv4;

		NET_DBG("Received Echo reply from %s to %s",
			net_sprint_ipv4_addr(&ip_hdr->src),
			net_sprint_ipv4_addr(&ip_hdr->dst));

	} else if (hdr->family == AF_INET6) {
		struct net_ipv6_hdr *ip_hdr = hdr->ipv6;

		NET_DBG("Received Echo Reply from %s to %s",
			net_sprint_ipv6_addr(&ip_hdr->src),
			net_sprint_ipv6_addr(&ip_hdr->dst));
	} else {
		return -ENOENT;
	}

	k_sem_give(wait_sem);

	return 0;
}

static int ping_check(struct net_if *iface,
		      struct sockaddr *addr)
{
	struct net_icmp_ping_params params = {
		.identifier = sys_rand32_get(),
	};
	struct net_icmp_ctx ctx;
	struct k_sem wait_sem;
	int ret;
	int type;

	if (addr->sa_family == AF_INET) {
		type = NET_ICMPV4_ECHO_REPLY;
	} else if (addr->sa_family == AF_INET6) {
		type = NET_ICMPV6_ECHO_REPLY;
	} else {
		return -EINVAL;
	}

	ret = net_icmp_init_ctx(&ctx, type, 0, icmp_handler);
	if (ret < 0) {
		return ret;
	}

	NET_DBG("Sending ping to %s (%d)",
		addr->sa_family == AF_INET ?
		net_sprint_ipv4_addr(&net_sin(addr)->sin_addr) :
		net_sprint_ipv6_addr(&net_sin6(addr)->sin6_addr),
		net_if_get_by_iface(iface));

	k_sem_init(&wait_sem, 0, 1);

	ret = net_icmp_send_echo_request(&ctx, iface, addr,
					 &params, &wait_sem);
	if (ret < 0) {
		NET_DBG("Cannot send ping (%d)", ret);
		goto out;
	}

	ret = k_sem_take(&wait_sem, K_MSEC(WAIT_TIMEOUT));
	if (ret < 0) {
		goto out;
	}

	NET_DBG("Sending Online Connectivity event for interface %d",
		net_if_get_by_iface(iface));
	net_mgmt_event_notify(NET_EVENT_CONNECTIVITY_ONLINE, iface);

	enable_online_state_verifier();

	ret = 0;
out:
	net_icmp_cleanup_ctx(&ctx);

	return ret;
}

static int resolve_hostname(const char *hostname, const char *service,
			    int socktype)
{
	struct zsock_addrinfo *res = NULL;
	enum dns_resolve_status st;

	if (!online_check->hostaddr_valid) {
		st = resolve_host(hostname, service, socktype, &res);
		if (st != 0) {
			NET_DBG("Cannot resolve %s", hostname);
			return -EAGAIN;
		}

		for (; res; res = res->ai_next) {
			/* For multi-address hosts, we do the
			 * first address only.
			 */
			memcpy(&online_check->hostaddr,
			       res->ai_addr,
			       sizeof(online_check->hostaddr));
			online_check->hostaddr_valid = true;
			break;
		}

		zsock_freeaddrinfo(res);
	}

	return 0;
}

static int do_online_ping_check(struct net_if *iface, const char *host)
{
	int ret;

	ret = resolve_hostname(host, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	if (!online_check->hostaddr_valid) {
		NET_DBG("Invalid host address (%s)", host);
		return -EINVAL;
	}

	ret = ping_check(iface, &online_check->hostaddr);
	if (ret < 0) {
		NET_DBG("ping check failed (%d)", ret);
		return ret;
	}

	return 0;
}

static int get_hostname(const char *url, char *hostname, int maxlen,
			bool *is_tls)
{
	const char *ptr, *host;
	int len;

	ptr = strstr(url, "https://");
	if (ptr == url) {
		if (!IS_ENABLED(CONFIG_NET_SOCKETS_SOCKOPT_TLS)) {
			NET_WARN("TLS not enabled but HTTPS URL supplied!");
			*is_tls = false;
		} else {
			*is_tls = true;
		}

		host = ptr + sizeof("https://") - 1;
	} else {
		ptr = strstr(url, "http://");
		if (ptr == url) {
			*is_tls = false;
			host = ptr + sizeof("http://") - 1;
		} else {
			return -EINVAL;
		}
	}

	ptr = strstr(host, "/");
	if (ptr) {
		len = MIN(maxlen - 1, ptr - host);
	} else {
		len = strlen(host);
		len = MIN(maxlen - 1, len);
	}

	memcpy(hostname, host, len);
	hostname[len] = '\0';

	return 0;
}

static void set_host_and_port(struct online_check_data *data)
{
	data->port = strstr(data->hostname_port, ":");
	if (data->port != NULL) {
		*data->port++ = '\0';
	} else {
		if (data->is_tls) {
			data->port = "443";
		} else {
			data->port = "80";
		}
	}

	data->host = data->hostname_port;
}

static int resolve_url(const char *url)
{
	struct sockaddr addr;
	bool is_tls;
	bool status;
	int ret;

	if (url == NULL) {
		return -EINVAL;
	}

	ret = get_hostname(url, online_check->hostname_port,
			   MAX_HOSTNAME_LEN, &is_tls);
	if (ret < 0) {
		NET_DBG("Cannot find hostname from %s", url);
		return ret;
	}

	online_check->is_tls = is_tls;

	/* Try to parse the address first */
	status = net_ipaddr_parse(online_check->hostname_port,
				  strlen(online_check->hostname_port),
				  &addr);
	if (status) {
		/* We could parse the IP address */
		memcpy(&online_check->hostaddr, &addr,
		       sizeof(online_check->hostaddr));
		online_check->hostaddr_valid = true;

		set_host_and_port(online_check);
	} else {
		/* We need to do a DNS query */

		set_host_and_port(online_check);

		ret = resolve_hostname(online_check->host, online_check->port,
				       SOCK_STREAM);
		if (ret < 0) {
			return ret;
		}
	}

	if (online_check->hostaddr_valid) {
		if (net_sin(&online_check->hostaddr)->sin_port == 0) {
			if (online_check->is_tls) {
				net_sin(&online_check->hostaddr)->sin_port = htons(443);
			} else {
				net_sin(&online_check->hostaddr)->sin_port = htons(80);
			}
		}
	}

	return 0;
}

static void response_cb(struct http_response *rsp,
			enum http_final_call final_data,
			void *user_data)
{
	uint16_t *status = user_data;

	if (final_data == HTTP_DATA_MORE) {
		NET_DBG("Partial data received (%zd bytes)", rsp->data_len);
	} else if (final_data == HTTP_DATA_FINAL) {
		NET_DBG("All the data received (%zd bytes)", rsp->data_len);
	}

	NET_INFO("Response status %s", rsp->http_status);

	*status = rsp->http_status_code;
}

static int exec_http_query(struct net_if *iface, int sock)
{
	int32_t timeout = WAIT_TIMEOUT;
	struct http_request req;
	uint16_t status;
	int ret;

	memset(&req, 0, sizeof(req));

	req.method = HTTP_GET;
	req.url = online_check->url;
	req.host = online_check->host;
	req.port = online_check->port;
	req.protocol = "HTTP/1.1";
	req.response = response_cb;
	req.recv_buf = online_check->http_recv_buf;
	req.recv_buf_len = sizeof(online_check->http_recv_buf);

	/* Make sure that we cannot access recv_buf data multiple times */
	k_mutex_lock(&conn_mgr_mon_lock, K_FOREVER);

	ret = http_client_req(sock, &req, timeout, &status);
	if (ret < 0) {
		NET_DBG("HTTP request failed (%d)", ret);
		goto out;
	}

	/* Lets assume online also for 301 "Moved Permanently" */
	if (status == 200 || status == 301) {
		NET_DBG("Sending Online Connectivity event for interface %d",
			net_if_get_by_iface(iface));
		net_mgmt_event_notify(NET_EVENT_CONNECTIVITY_ONLINE, iface);

		enable_online_state_verifier();

		ret = 0;
	} else {
		NET_DBG("Received HTTP status %d, not considering online.",
			status);

		ret = -ENOTCONN;
	}

out:
	k_mutex_unlock(&conn_mgr_mon_lock);

	return ret;
}

static int do_online_http_check(struct net_if *iface, const char *host)
{
	int proto = IPPROTO_TCP;
	const char *tls_hostname = NULL;
	const sec_tag_t *tags = NULL;
	size_t tags_size = 0;
	socklen_t addrlen;
	int sock;
	int ret;

	if (!online_check->hostaddr_valid) {
		ret = resolve_url(host);
		if (ret < 0) {
			NET_DBG("Cannot parse URL \"%s\" (%d)",
				host, ret);
			return ret;
		}
	}

	if (!online_check->hostaddr_valid) {
		return -EAGAIN;
	}

	if (IS_ENABLED(CONFIG_NET_SOCKETS_SOCKOPT_TLS) && online_check->is_tls) {
		if (online_check->cb == NULL) {
			NET_DBG("HTTPS request but callback not registered. "
				"HTTP online check disabled.");
			return -ENOTSUP;
		}

		ret = online_check->cb(iface, &tags, &tags_size,
				       &tls_hostname,
				       host,
				       online_check->host,
				       online_check->port,
				       &online_check->hostaddr,
				       online_check->user_data);
		if (ret < 0) {
			NET_DBG("Setting up socket failed (%d)", ret);
			return ret;
		}

		if (tags_size == 0 || tags == NULL) {
			NET_DBG("Security tags is empty, TLS disabled");
		} else {
			proto = IPPROTO_TLS_1_2;
		}
	}

	sock = zsock_socket(online_check->hostaddr.sa_family,
			    SOCK_STREAM, proto);
	if (sock < 0) {
		NET_DBG("Socket creation failed (%d)", sock);
		return ret;
	}

	if (IS_ENABLED(CONFIG_NET_SOCKETS_SOCKOPT_TLS) && proto == IPPROTO_TLS_1_2) {
		ret = zsock_setsockopt(sock, SOL_TLS, TLS_SEC_TAG_LIST,
				       tags, tags_size);
		if (ret < 0) {
			LOG_ERR("TLS setsockopt: %d", errno);
			ret = -errno;
			goto out;
		}

		if (tls_hostname != NULL) {
			ret = zsock_setsockopt(sock, SOL_TLS, TLS_HOSTNAME,
					       tls_hostname,
					       strlen(tls_hostname));
			if (ret < 0) {
				LOG_ERR("setsockopt: %d", errno);
				ret = -errno;
				goto out;
			}
		}
	}

	addrlen = 0;

	if (IS_ENABLED(CONFIG_NET_IPV4) && online_check->hostaddr.sa_family == AF_INET) {
		addrlen = sizeof(struct sockaddr_in);
	} else if (IS_ENABLED(CONFIG_NET_IPV6) && online_check->hostaddr.sa_family == AF_INET6) {
		addrlen = sizeof(struct sockaddr_in6);
	}

	if (addrlen == 0) {
		ret = -EINVAL;
		NET_DBG("Invalid sockaddr len (%d)", ret);
		goto out;
	}

	NET_DBG("Connecting to %s:%d",
		online_check->hostaddr.sa_family == AF_INET ?
		net_sprint_ipv4_addr(&net_sin(&online_check->hostaddr)->sin_addr) :
		net_sprint_ipv6_addr(&net_sin6(&online_check->hostaddr)->sin6_addr),
		online_check->hostaddr.sa_family == AF_INET ?
		ntohs(net_sin(&online_check->hostaddr)->sin_port) :
		ntohs(net_sin6(&online_check->hostaddr)->sin6_port));
	ret = zsock_connect(sock, &online_check->hostaddr, addrlen);
	if (ret < 0) {
		ret = -errno;
		NET_DBG("Connect failed (%d)", ret);
		goto out;
	}

	NET_DBG("Sending HTTP%sGET request to %s:%d (ifindex %d)",
		online_check->is_tls ? "S " : " ",
		online_check->hostaddr.sa_family == AF_INET ?
		net_sprint_ipv4_addr(&net_sin(&online_check->hostaddr)->sin_addr) :
		net_sprint_ipv6_addr(&net_sin6(&online_check->hostaddr)->sin6_addr),
		online_check->hostaddr.sa_family == AF_INET ?
		ntohs(net_sin(&online_check->hostaddr)->sin_port) :
		ntohs(net_sin6(&online_check->hostaddr)->sin6_port),
		net_if_get_by_iface(iface));

	ret = exec_http_query(iface, sock);

out:
	zsock_close(sock);
	return ret;
}

static int do_online_check(struct net_if *iface)
{
	int ret;

	if (IS_ENABLED(CONFIG_NET_CONNECTION_MANAGER_ONLINE_CHECK_PING) &&
	    online_check->strategy == NET_CONN_MGR_ONLINE_CHECK_PING) {
		ret = do_online_ping_check(iface, PING_HOST);

	} else if (IS_ENABLED(CONFIG_NET_CONNECTION_MANAGER_ONLINE_CHECK_HTTP) &&
	    online_check->strategy == NET_CONN_MGR_ONLINE_CHECK_HTTP) {
		ret = do_online_http_check(iface, online_check->url);
	} else {
		if (online_check->strategy != NET_CONN_MGR_ONLINE_CHECK_PING &&
		    online_check->strategy != NET_CONN_MGR_ONLINE_CHECK_HTTP) {
			NET_ERR("Invalid online check strategy (%d)", online_check->strategy);
		}

		return -EINVAL;
	}

	if (ret == 0) {
		online_check->running = true;
	} else {
		online_check->running = false;
	}

	return ret;
}

static void clear_online_check_flags(void)
{
	int i, state_count;

	k_mutex_lock(&conn_mgr_mon_lock, K_FOREVER);

	state_count = conn_mgr_get_iface_states(&states);

	for (i = 0; i < state_count; i++) {
		states[i] &= ~CONN_MGR_IF_ONLINE_CHECK;
	}

	k_mutex_unlock(&conn_mgr_mon_lock);
}

static void stop_online_check(void)
{
	int ret;

	clear_online_check_flags();

	k_mutex_lock(&check_running_lock, K_FOREVER);
	check_running = false;
	k_mutex_unlock(&check_running_lock);

	if (!IS_ENABLED(CONFIG_NET_CONNECTION_MANAGER_ONLINE_VERIFY)) {
		return;
	}

	if (net_trickle_is_running(&online_check->verifier)) {
		ret = net_trickle_stop(&online_check->verifier);
		if (ret < 0) {
			NET_DBG("Cannot stop Trickle timer (%d)", ret);
		}

		k_work_cancel_delayable(&online_check->verifier_work);
	}

	/* If there is a check currently ongoing, it will eventually stop
	 * and the new check will be done using the new strategy.
	 */
}

void conn_mgr_online_connectivity_check(void)
{
	int ret, i, state_count;

	k_mutex_lock(&conn_mgr_mon_lock, K_FOREVER);
	state_count = conn_mgr_get_iface_states(&states);
	k_mutex_unlock(&conn_mgr_mon_lock);

	for (i = 0; i < state_count; i++) {
		if (states[i] & CONN_MGR_IF_READY) {
			k_mutex_lock(&conn_mgr_mon_lock, K_FOREVER);

			if (!(states[i] & CONN_MGR_IF_READY) ||
			    (states[i] & CONN_MGR_IF_ONLINE_CHECK)) {
				k_mutex_unlock(&conn_mgr_mon_lock);
				continue;
			}

			k_mutex_unlock(&conn_mgr_mon_lock);

			/* Do connectivity check and if it succeeds,
			 * generate CONNECTIVITY_ONLINE event. The online
			 * check might take some time.
			 */
			k_mutex_lock(&check_running_lock, K_FOREVER);

			if (check_running) {
				NET_DBG("Online check already running");
				k_mutex_unlock(&check_running_lock);
				break;
			}

			check_running = true;

			k_mutex_unlock(&check_running_lock);

			k_mutex_lock(&conn_mgr_mon_lock, K_FOREVER);

			/* Tells that we are doing online check for this interface */
			states[i] |= CONN_MGR_IF_ONLINE_CHECK;

			k_mutex_unlock(&conn_mgr_mon_lock);

			/* Did we get to online? This is a synchronous call and
			 * in worst case might take a long time to return.
			 */
			ret = do_online_check(conn_mgr_mon_get_if_by_index(i));
			if (ret == 0) {
				/* We are online, no need to check the other interfaces */
				break;
			}

			k_mutex_lock(&conn_mgr_mon_lock, K_FOREVER);

			states[i] &= ~CONN_MGR_IF_ONLINE_CHECK;

			k_mutex_unlock(&conn_mgr_mon_lock);
		} else {
			bool trigger_check = false;

			k_mutex_lock(&conn_mgr_mon_lock, K_FOREVER);

			/* If this interface was online, then we should re-check */
			if (states[i] & CONN_MGR_IF_ONLINE_CHECK) {
				trigger_check = true;
			}

			/* Clear the online check flag for non ready interface */
			states[i] &= ~CONN_MGR_IF_ONLINE_CHECK;

			k_mutex_unlock(&conn_mgr_mon_lock);

			if (trigger_check) {
				conn_mgr_refresh_online_connectivity_check();
				break;
			}
		}
	}
}

bool conn_mgr_trigger_online_connectivity_check(void)
{
	if (IS_ENABLED(CONFIG_NET_CONNECTION_MANAGER_ONLINE_VERIFY)) {
		online_check->trickle.Imin = TRICKLE_IMIN * MSEC_PER_SEC * SEC_PER_MIN;
		online_check->trickle.Imax = TRICKLE_IMAX;
		online_check->trickle.k = TRICKLE_K;
	}

	if (online_check->url == NULL) {
		conn_mgr_register_online_checker_url(ONLINE_CHECK_URL);
	}

	conn_mgr_trigger_online_checks = true;
	k_sem_give(&conn_mgr_mon_updated);

	return true;
}

int conn_mgr_register_online_checker_cb(net_conn_mgr_online_checker_t cb,
					void *user_data)
{
	online_check->cb = cb;
	online_check->user_data = user_data;

	return 0;
}

void conn_mgr_set_online_check_strategy(enum net_conn_mgr_online_check_type type)
{
	if (type == NET_CONN_MGR_ONLINE_CHECK_PING ||
	    type == NET_CONN_MGR_ONLINE_CHECK_HTTP) {
		if (online_check->strategy == type) {
			return;
		}

		online_check->strategy = type;

		NET_DBG("Setting online connectivity check strategy to %s",
			type == NET_CONN_MGR_ONLINE_CHECK_PING ? "ping" : "http");

		stop_online_check();
		conn_mgr_trigger_online_connectivity_check();
	} else {
		NET_ERR("Invalid value %d for online connectivity check strategy.", type);
	}
}

int conn_mgr_register_online_checker_url(const char *url)
{
	online_check->url = url;

	return 0;
}

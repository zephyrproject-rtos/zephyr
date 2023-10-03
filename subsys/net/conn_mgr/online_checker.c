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

struct online_check_data {
	net_conn_mgr_online_checker_t cb;
	void *user_data;
	char *host; /* This points to hostname_port */
	char *port; /* and same for this */
	struct sockaddr hostaddr;
	char hostname_port[MAX_HOSTNAME_LEN];
	uint8_t http_recv_buf[MAX_RECV_BUF_LEN];
	bool hostaddr_valid : 1;
	bool is_tls : 1;
};

static struct online_check_data online_check_data_storage;
static struct online_check_data *online_check = &online_check_data_storage;

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

static void do_online_ping_check(struct net_if *iface, const char *host)
{
	int ret;

	ret = resolve_hostname(host, NULL, 0);
	if (ret < 0) {
		return;
	}

	if (online_check->hostaddr_valid) {
		ret = ping_check(iface, &online_check->hostaddr);
		if (ret < 0) {
			NET_DBG("ping check failed (%d)", ret);
		}
	}
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

	ret = get_hostname(url, online_check->hostname_port,
			   MAX_HOSTNAME_LEN, &is_tls);
	if (ret < 0) {
		NET_DBG("Cannot find hostname from %s", ONLINE_CHECK_URL);
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
	req.url = ONLINE_CHECK_URL;
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
	} else {
		NET_DBG("Received HTTP status %d, not considering online.",
			status);
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
				ONLINE_CHECK_URL, ret);
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
			LOG_ERR("setsockopt: %d", errno);
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

	exec_http_query(iface, sock);
	ret = 0;

out:
	zsock_close(sock);
	return ret;
}

static void do_online_check(struct net_if *iface)
{
	if (IS_ENABLED(CONFIG_NET_CONNECTION_MANAGER_ONLINE_CHECK_PING)) {
		do_online_ping_check(iface, PING_HOST);
	} else if (IS_ENABLED(CONFIG_NET_CONNECTION_MANAGER_ONLINE_CHECK_HTTP)) {
		do_online_http_check(iface, ONLINE_CHECK_URL);
	}
}

void conn_mgr_online_connectivity_check(void)
{
	uint16_t *states;
	int i, state_count;

	k_mutex_lock(&conn_mgr_mon_lock, K_FOREVER);
	state_count = conn_mgr_get_iface_states(&states);
	k_mutex_unlock(&conn_mgr_mon_lock);

	for (i = 0; i < state_count; i++) {
		if (states[i] & CONN_MGR_IF_READY) {
			k_mutex_lock(&conn_mgr_mon_lock, K_FOREVER);
			if (!(states[i] & CONN_MGR_IF_READY)) {
				k_mutex_unlock(&conn_mgr_mon_lock);
				continue;
			}

			k_mutex_unlock(&conn_mgr_mon_lock);

			/* Do connectivity check and if it succeeds,
			 * generate CONNECTIVITY_ONLINE event. The online
			 * check might take some time.
			 */
			do_online_check(conn_mgr_mon_get_if_by_index(i));
		}
	}
}

int conn_mgr_register_online_checker_cb(net_conn_mgr_online_checker_t cb,
					void *user_data)
{
	online_check->cb = cb;
	online_check->user_data = user_data;

	return 0;
}

#define SYS_LOG_DOMAIN "ping"
#define SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_DEBUG 1

#error "Ping service"

#include <zephyr.h>

#include <net/net_pkt.h>
#include <net/net_mgmt.h>
#include <net/net_core.h>
#include <net/net_context.h>

/* Port to bind to */
#define PING_PORT 58473
/* Timeout for all timeout operations */
#define PING_TIMEOUT K_SECONDS(2)
/* Size of stack area used by each thread */
#define STACKSIZE 1024
/* Scheduling priority used by each thread */
#define PRIORITY 7
/* Delay between greetings (in ms) */
#define SLEEPTIME 500

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_EVENT_IPV4_ADDR_ADD, ping_service_start);

static int ping_service_start(u32_t mgmt_event, struct net_if *iface,
			      void *data, size_t len)
{
	int socket;

	NET_INFO("ping service started\n");

	/* Context get */
	socket = zsock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (socket < 0) {
		NET_ERR("Failed to create socket (%d)", socket);
		return;
	}

	/* Context bind */
	// net_ipaddr_copy(&addr.sin_addr, iface_addr);
	// addr.sin_family = AF_INET;
	// addr.sin_port = htons(PING_PORT);

	// ret = net_context_bind(context,
	// 		       (const struct sockaddr *)&addr, sizeof(addr));
	// if (ret < 0) {
	// 	NET_ERR("Cannot bind IPv4 UDP port %d (%d)",
	// 		ntohs(addr.sin_port), ret);
	// 	return;
	// }
}

void ping_ping()
{
	/*
	pkt = net_pkt_get_tx(context, PING_TIMEOUT);
	if (!pkt) {
		ret = -ENOMEM;
		goto quit;
	}

	ret = net_pkt_append_all(pkt, 6, "alive", PING_TIMEOUT);
	if (ret < 0) {
		ret = -ENOMEM;
		goto quit;
	}

	if (server->sa_family == AF_INET) {
		server_addr_len = sizeof(struct sockaddr_in);
	} else {
		server_addr_len = sizeof(struct sockaddr_in6);
	}

	ret = net_context_sendto(pkt, server, server_addr_len, NULL,
				 PING_TIMEOUT, NULL, NULL);
	if (ret < 0) {
		NET_DBG("Cannot send query (%d)", ret);
		net_pkt_unref(pkt);
		goto quit;
	}
	*/
}

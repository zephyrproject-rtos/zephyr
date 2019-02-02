/* Initializations */
#define SYS_LOG_DOMAIN "example-app"
#define SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_DEBUG 1

#include <zephyr.h>

#include <net/net_pkt.h>
#include <net/net_core.h>
#include <net/net_context.h>

#define MY_IP6ADDR { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, \
			 0, 0, 0, 0, 0, 0, 0, 0x1 } } }
#define MY_PORT 4242

struct in6_addr in6addr_my = MY_IP6ADDR;
struct sockaddr_in6 my_addr6 = { 0 };
struct net_context *context;
int ret;

struct k_sem waiter;

static inline void quit(void)
{
	k_sem_give(&waiter);
}

static inline void init_app(void)
{
	k_sem_init(&waiter, 0, 1);

	/* Add our address to the network interface */
	net_if_ipv6_addr_add(net_if_get_default(), &in6addr_my,
			     NET_ADDR_MANUAL, 0);
}

void main(void)
{
	NET_INFO("Run sample application");

	init_app();

	create_context();

	bind_address();

	receive_data();

	k_sem_take(&waiter, K_FOREVER);

	close_context();

	NET_INFO("Stopping sample application");
}

/* Context creation */
static int create_context(void)
{
	ret = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_UDP, &context);
	if (!ret) {
		NET_ERR("Cannot get context (%d)", ret);
		return ret;
	}

	return 0;
}

/* Context bind */
static int bind_address(void)
{
	net_ipaddr_copy(&my_addr6.sin6_addr, &in6addr_my);
	my_addr6.sin6_family = AF_INET6;
	my_addr6.sin6_port = htons(MY_PORT);

	ret = net_context_bind(context, (struct sockaddr *)&my_addr6);
	if (ret < 0) {
		NET_ERR("Cannot bind IPv6 UDP port %d (%d)",
			ntohs(my_addr6.sin6_port), ret);
		return ret;
	}

	return 0;
}

/* Receive data */
#define MAX_DBG_PRINT 64

static struct net_buf *udp_recv(const char *name,
				struct net_context *context,
				struct net_buf *buf)
{
	struct net_buf *reply_buf, *frag, *tmp;
	int header_len, recv_len, reply_len;

	NET_INFO("%s received %u bytes", name,
	      net_pkt_appdatalen(buf));

	reply_buf = net_pkt_get_tx(context, K_FOREVER);

	NET_ASSERT(reply_buf);

	recv_len = net_buf_frags_len(buf->frags);

	tmp = buf->frags;

	/* First fragment will contain IP header so move the data
	 * down in order to get rid of it.
	 */
	header_len = net_pkt_appdata(buf) - tmp->data;

	NET_ASSERT(header_len < CONFIG_NET_BUF_DATA_SIZE);

	net_buf_pull(tmp, header_len);

	while (tmp) {
		frag = net_pkt_get_data(context, K_FOREVER);

		memcpy(net_buf_add(frag, tmp->len), tmp->data, tmp->len);

		net_buf_frag_add(reply_buf, frag);

		net_buf_frag_del(buf, tmp);

		tmp = buf->frags;
	}

	reply_len = net_buf_frags_len(reply_buf->frags);

	NET_ASSERT_INFO(recv_len != reply_len,
			"Received %d bytes, sending %d bytes",
			recv_len, reply_len);

	return reply_buf;
}

static inline void udp_sent(struct net_context *context,
			    int status,
			    void *token,
			    void *user_data)
{
	if (!status) {
		NET_INFO("Sent %d bytes", POINTER_TO_UINT(token));
	}
}

static inline void set_dst_addr(sa_family_t family,
				struct net_buf *buf,
				struct sockaddr *dst_addr)
{
	if (family == AF_INET6) {
		net_ipaddr_copy(&net_sin6(dst_addr)->sin6_addr,
				&NET_IPV6_HDR(buf)->src);
		net_sin6(dst_addr)->sin6_family = AF_INET6;
		net_sin6(dst_addr)->sin6_port = NET_UDP_HDR(buf)->src_port;
	}
}

static void udp_received(struct net_context *context,
			 struct net_buf *buf,
			 int status,
			 void *user_data)
{
	struct net_buf *reply_buf;
	struct sockaddr dst_addr;
	sa_family_t family = net_pkt_family(buf);
	static char dbg[MAX_DBG_PRINT + 1];
	int ret;

	snprintf(dbg, MAX_DBG_PRINT, "UDP IPv%c",
		 family == AF_INET6 ? '6' : '4');

	set_dst_addr(family, buf, &dst_addr);

	reply_buf = udp_recv(dbg, context, buf);

	net_pkt_unref(buf);

	ret = net_context_sendto(reply_buf, &dst_addr, udp_sent, 0,
				 UINT_TO_POINTER(net_buf_frags_len(reply_buf)),
				 user_data);
	if (ret < 0) {
		NET_ERR("Cannot send data to peer (%d)", ret);

		net_pkt_unref(reply_buf);

		quit();
	}
}

static int receive_data(void)
{
	ret = net_context_recv(context, udp_received, 0, NULL);
	if (ret < 0) {
		NET_ERR("Cannot receive IPv6 UDP packets");

		quit();

		return ret;
	}

	return 0;
}

/* Context close */
static int close_context(void)
{
	ret = net_context_put(context);
	if (ret < 0) {
		NET_ERR("Cannot close IPv6 UDP context");
		return ret;
	}

	return 0;
}

/*
 * Copyright (c) 2017 Intel Corporation.
 * Copyright (c) 2017 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if 1
#define SYS_LOG_DOMAIN "irc"
#define NET_LOG_ENABLED 1
#endif

#include <board.h>
#include <drivers/rand32.h>
#include <errno.h>
#include <gpio.h>
#include <net/net_pkt.h>
#include <net/net_context.h>
#include <net/net_core.h>
#include <net/net_if.h>
#include <stdio.h>
#include <zephyr.h>
#include <net/dns_resolve.h>
#include <net/net_mgmt.h>
#include <net/net_event.h>

#if !defined(CONFIG_NET_IPV6) && !defined(CONFIG_NET_IPV4)
#error "CONFIG_NET_IPV6 or CONFIG_NET_IPV4 must be enabled for irc_bot"
#endif

#define STACK_SIZE	2048
u8_t stack[STACK_SIZE];
static struct k_thread bot_thread;

#define CMD_BUFFER_SIZE 256
static u8_t cmd_buf[CMD_BUFFER_SIZE];

/* LED */
#if defined(LED0_GPIO_PORT)
#define LED_GPIO_NAME LED0_GPIO_PORT
#define LED_PIN LED0_GPIO_PIN
#else
#define LED_GPIO_NAME "(fail)"
#define LED_PIN 0
#endif

static struct device *led0;
static bool fake_led;

/* Network Config */

#if defined(CONFIG_NET_IPV6)

#define ZIRC_AF_INET		AF_INET6
#define ZIRC_SOCKADDR_IN	sockaddr_in6

#if defined(CONFIG_NET_APP_MY_IPV6_ADDR)
#define ZIRC_LOCAL_IP_ADDR	CONFIG_NET_APP_MY_IPV6_ADDR
#else
#define ZIRC_LOCAL_IP_ADDR	"2001:db8::1"
#endif /* CONFIG_NET_APP_MY_IPV6_ADDR */

#if defined(CONFIG_NET_APP_PEER_IPV6_ADDR)
#define ZIRC_PEER_IP_ADDR	CONFIG_NET_APP_PEER_IPV6_ADDR
#else
#define ZIRC_PEER_IP_ADDR	"2001:db8::2"
#endif /* CONFIG_NET_APP_PEER_IPV6_ADDR */

#else /* CONFIG_NET_IPV4 */

#define ZIRC_AF_INET		AF_INET
#define ZIRC_SOCKADDR_IN	sockaddr_in

#if defined(CONFIG_NET_APP_MY_IPV4_ADDR)
#define ZIRC_LOCAL_IP_ADDR	CONFIG_NET_APP_MY_IPV4_ADDR
#else
#define ZIRC_LOCAL_IP_ADDR	"192.0.2.1"
#endif /* CONFIG_NET_APP_MY_IPV4_ADDR */

#if defined(CONFIG_NET_APP_PEER_IPV4_ADDR)
#define ZIRC_PEER_IP_ADDR	CONFIG_NET_APP_PEER_IPV4_ADDR
#else
#define ZIRC_PEER_IP_ADDR	"192.0.2.2"
#endif /* CONFIG_NET_APP_PEER_IPV4_ADDR */

#endif

/* DNS API */
#define DNS_QUERY_TIMEOUT K_SECONDS(4)

/* IRC API */
#define DEFAULT_SERVER	"irc.freenode.net"
#define DEFAULT_PORT	6667
#define DEFAULT_CHANNEL	"#zephyrbot"

struct zirc_chan;

typedef void (*on_privmsg_rcvd_cb_t)(void *data, struct zirc_chan *chan,
				  char *umask, char *msg);

struct zirc {
	struct sockaddr local_addr;
	struct sockaddr remote_addr;

#if defined(CONFIG_DNS_RESOLVER)
	struct k_sem wait_dns;
#endif

	struct k_sem wait_iface;

	struct net_context *conn;
	struct zirc_chan *chans;

	void *data;
};

struct zirc_chan {
	struct zirc *irc;
	struct zirc_chan *next;

	const char *chan;

	on_privmsg_rcvd_cb_t on_privmsg_rcvd;
	void *data;
};

static void on_msg_rcvd(void *data, struct zirc_chan *chan, char *umask,
			char *msg);

static void
panic(const char *msg)
{
	NET_ERR("Panic: %s", msg);
	for (;;) {
		k_sleep(K_FOREVER);
	}
}

static int
transmit(struct net_context *ctx, char buffer[], size_t len)
{
	struct net_pkt *send_pkt;

	send_pkt = net_pkt_get_tx(ctx, K_FOREVER);
	if (!send_pkt) {
		return -ENOMEM;
	}

	if (!net_pkt_append_all(send_pkt, len, buffer, K_FOREVER)) {
		return -EINVAL;
	}

	return net_context_send(send_pkt, NULL, K_NO_WAIT, NULL, NULL);
}

static void
on_cmd_ping(struct zirc *irc, char *umask, char *cmd, size_t len)
{
	char pong[32];
	int ret;

	NET_INFO("Got PING command from server: %s", cmd);

	ret = snprintk(pong, 32, "PONG :%s", cmd + 1);
	if (ret < sizeof(pong)) {
		ret = transmit(irc->conn, pong, ret);
		if (ret < 0) {
			NET_INFO("Transmit error: %d", ret);
		}
	}
}

static void
on_cmd_privmsg(struct zirc *irc, char *umask, char *cmd, size_t len)
{
	struct zirc_chan *chan;
	char *space;

	if (!umask) {
		/* Don't know how this got here, so ignore */
		return;
	}

	NET_INFO("Got message from umask %s: %s", umask, cmd);

	space = memchr(cmd, ' ', len);
	if (!space) {
		return;
	}

	*space = '\0';

	if (*(space + 1) != ':') {
		/* Just ignore messages without a ':' after the space */
		return;
	}

	space += 2; /* Jump over the ':', pointing to the message itself */
	for (chan = irc->chans; chan; chan = chan->next) {
		if (!strncmp(chan->chan, cmd, space - cmd)) {
			chan->on_privmsg_rcvd(chan->data, chan, umask, space);
			return;
		}
	}

	/* TODO: could be a private message (from another user) */
	NET_INFO("Unknown privmsg received: %.*s\n", (int)len, cmd);
}

#define CMD(cmd_, cb_) { \
	.cmd = cmd_ " ", \
	.cmd_len = sizeof(cmd_ " ") - 1, \
	.func = on_cmd_ ## cb_ \
}
static void
process_command(struct zirc *irc, char *cmd, size_t len)
{
	static const struct {
		const char *cmd;
		size_t cmd_len;
		void (*func)(struct zirc *zirc, char *umask, char *cmd,
			     size_t len);
	} commands[] = {
		CMD("PING", ping),
		CMD("PRIVMSG", privmsg),
	};
	char *umask;
	int i;

	if (*cmd == ':') {
		char *space = memchr(cmd, ' ', len);

		if (!space) {
			return;
		}

		umask = cmd + 1;
		*space = '\0';

		len -= (space - cmd) + 1;
		if (!len) {
			return;
		}

		cmd = space + 1;

		NET_INFO("Received from server, umask=%s: %s", umask, cmd);
	} else {
		umask = NULL;

		NET_INFO("Received from server (no umask): %s", cmd);
	}

	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (len < commands[i].cmd_len) {
			continue;
		}
		if (!strncmp(cmd, commands[i].cmd, commands[i].cmd_len)) {
			NET_INFO("Command has handler, executing");

			cmd += commands[i].cmd_len;
			len -= commands[i].cmd_len;
			commands[i].func(irc, umask, cmd, len);

			return;
		}
	}

	/* TODO: handle notices, CTCP, etc */
	NET_INFO("Could not find handler to handle %s, ignoring", cmd);
}
#undef CMD

static void
on_context_recv(struct net_context *ctx, struct net_pkt *pkt,
				int status, void *data)
{
	struct zirc *irc = data;
	struct net_buf *tmp;
	u8_t *end_of_line;
	size_t len;
	u16_t pos = 0, cmd_len = 0;

	if (!pkt) {
		/* TODO: notify of disconnection, maybe reconnect? */
		NET_ERR("Disconnected\n");
		return;
	}

	if (status) {
		/* TODO: handle connection error */
		NET_ERR("Connection error: %d\n", -status);
		net_pkt_unref(pkt);
		return;
	}

	/* tmp points to fragment containing IP header */
	tmp = pkt->frags;
	/* skip pos to the first TCP payload */
	pos = net_pkt_appdata(pkt) - tmp->data;

	while (tmp) {
		len = tmp->len - pos;

		end_of_line = memchr(tmp->data + pos, '\r', len);
		if (end_of_line) {
			len = end_of_line - (tmp->data + pos);
		}

		if (cmd_len + len > sizeof(cmd_buf)) {
			/* overrun cmd_buf - bail out */
			NET_ERR("CMD BUFFER OVERRUN!! %zu > %zu",
				cmd_len + len,
				sizeof(cmd_buf));
			break;
		}

		tmp = net_frag_read(tmp, pos, &pos, len, cmd_buf + cmd_len);
		cmd_len += len;

		if (end_of_line) {
			/* skip the /n char after /r */
			if (tmp) {
				tmp = net_frag_read(tmp, pos, &pos, 1, NULL);
			}

			cmd_buf[cmd_len] = '\0';
			process_command(irc, cmd_buf, cmd_len);
			cmd_len = 0;
		}
	}

	net_pkt_unref(pkt);

	/* TODO: handle messages that spans multiple packets? */
}

static int
zirc_nick_set(struct zirc *irc, const char *nick)
{
	char buffer[32];
	int ret;

	NET_INFO("Setting nickname to: %s", nick);

	ret = snprintk(buffer, sizeof(buffer), "NICK %s\r\n", nick);
	if (ret < 0 || ret >= sizeof(buffer)) {
		return -EINVAL;
	}

	return transmit(irc->conn, buffer, ret);
}

static int
zirc_user_set(struct zirc *irc, const char *user, const char *realname)
{
	char buffer[64];
	int ret;

	NET_INFO("Setting user to: %s, real name to: %s", user, realname);

	ret = snprintk(buffer, sizeof(buffer), "USER %s * * :%s\r\n",
				   user, realname);
	if (ret < 0 || ret >= sizeof(buffer)) {
		return -EINVAL;
	}

	return transmit(irc->conn, buffer, ret);
}

static int
zirc_chan_join(struct zirc *irc, struct zirc_chan *chan,
	       const char *channel,
	       on_privmsg_rcvd_cb_t on_privmsg_rcvd,
	       void *data)
{
	char buffer[32];
	int ret;

	NET_INFO("Joining channel: %s", channel);

	if (!on_privmsg_rcvd) {
		return -EINVAL;
	}

	ret = snprintk(buffer, sizeof(buffer), "JOIN %s\r\n", channel);
	if (ret < 0 || ret >= sizeof(buffer)) {
		return -EINVAL;
	}

	ret = transmit(irc->conn, buffer, ret);
	if (ret < 0) {
		return ret;
	}

	chan->irc = irc;
	chan->chan = channel;
	chan->on_privmsg_rcvd = on_privmsg_rcvd;
	chan->data = data;

	chan->next = irc->chans;
	irc->chans = chan;

	return 0;
}

static int
zirc_chan_part(struct zirc_chan *chan)
{
	struct zirc_chan **cc, *c;
	char buffer[32];
	int ret;

	NET_INFO("Leaving channel: %s", chan->chan);

	ret = snprintk(buffer, sizeof(buffer), "PART %s\r\n", chan->chan);
	if (ret < 0 || ret >= sizeof(buffer)) {
		return -EINVAL;
	}

	ret = transmit(chan->irc->conn, buffer, ret);
	if (ret < 0) {
		return ret;
	}

	for (cc = &chan->irc->chans, c = chan->irc->chans;
	     c; cc = &c->next, c = c->next) {
		if (c == chan) {
			*cc = c->next;
			return 0;
		}
	}

	return -ENOENT;
}

static int in_addr_set(sa_family_t family,
		       const char *ip_addr,
		       int port,
		       struct sockaddr *_sockaddr)
{
	int rc = 0;

	_sockaddr->family = family;

	if (ip_addr) {
		if (family == AF_INET6) {
			rc = net_addr_pton(family,
					   ip_addr,
					   &net_sin6(_sockaddr)->sin6_addr);
		} else {
			rc = net_addr_pton(family,
					   ip_addr,
					   &net_sin(_sockaddr)->sin_addr);
		}

		if (rc < 0) {
			NET_ERR("Invalid IP address: %s", ip_addr);
			return -EINVAL;
		}
	}

	if (port >= 0) {
		if (family == AF_INET6) {
			net_sin6(_sockaddr)->sin6_port = htons(port);
		} else {
			net_sin(_sockaddr)->sin_port = htons(port);
		}
	}

	return rc;
}

#if defined(CONFIG_DNS_RESOLVER)
static void resolve_cb(enum dns_resolve_status status,
			struct dns_addrinfo *info,
			void *user_data)
{
	struct zirc *irc = user_data;

	/* Note that we cannot do any connection establishment in the DNS
	 * callback as the DNS query will timeout very soon. We inform the
	 * connect function via semaphore when it can continue.
	 */
	k_sem_give(&irc->wait_dns);

	if (status != DNS_EAI_INPROGRESS || !info) {
		return;
	}

#if defined(CONFIG_NET_IPV6)
	if (info->ai_family == AF_INET6) {
		net_ipaddr_copy(&net_sin6(&irc->remote_addr)->sin6_addr,
				&net_sin6(&info->ai_addr)->sin6_addr);
		net_sin6(&irc->remote_addr)->sin6_port = htons(DEFAULT_PORT);
		net_sin6(&irc->remote_addr)->sin6_family = AF_INET6;
		irc->remote_addr.family = AF_INET6;
	} else
#endif
#if defined(CONFIG_NET_IPV4)
	if (info->ai_family == AF_INET) {
		net_ipaddr_copy(&net_sin(&irc->remote_addr)->sin_addr,
				&net_sin(&info->ai_addr)->sin_addr);
		net_sin(&irc->remote_addr)->sin_port = htons(DEFAULT_PORT);
		net_sin(&irc->remote_addr)->sin_family = AF_INET;
		irc->remote_addr.family = AF_INET;
	} else
#endif
	{
		NET_ERR("Invalid IP address family %d", info->ai_family);
		return;
	}

	return;
}

static inline int zirc_dns_lookup(const char *host, void *user_data)
{
	return dns_get_addr_info(host,
#ifdef CONFIG_NET_IPV6
				 DNS_QUERY_TYPE_AAAA,
#else
				 DNS_QUERY_TYPE_A,
#endif
				 NULL,
				 resolve_cb,
				 user_data,
				 DNS_QUERY_TIMEOUT);
}
#endif /* CONFIG_DNS_RESOLVER */

static int
zirc_connect(struct zirc *irc, const char *host, int port, void *data)
{
	struct net_if *iface;
	struct zirc_chan *chan;
	int ret;
	char name_buf[32];

	NET_INFO("Connecting to %s:%d...", host, port);

	ret = net_context_get(ZIRC_AF_INET, SOCK_STREAM, IPPROTO_TCP,
			      &irc->conn);
	if (ret < 0) {
		NET_DBG("Could not get new context: %d", ret);
		return ret;
	}

	iface = net_if_get_default();
	if (!iface) {
		NET_DBG("Could not get new context: %d", ret);
		return -EIO;
	}

#if defined(CONFIG_DNS_RESOLVER)
	irc->data = data;

	ret = zirc_dns_lookup(host, irc);
	if (ret < 0) {
		NET_ERR("Could not peform DNS lookup on host %s: %d",
			host, ret);
		goto connect_exit;
	}

	/* We continue the connection after the server name has been resolved.
	 */
	k_sem_take(&irc->wait_dns, K_FOREVER);
#else
	ret = in_addr_set(ZIRC_AF_INET, ZIRC_PEER_IP_ADDR,
			  port, &irc->remote_addr);
	if (ret < 0) {
		goto connect_exit;
	}
#endif

	ret = net_context_bind(irc->conn, &irc->local_addr,
			       sizeof(struct ZIRC_SOCKADDR_IN));
	if (ret < 0) {
		NET_DBG("Could not bind to local address: %d", ret);
		goto connect_exit;
	}

	irc->data = data;

	ret = net_context_connect(irc->conn, &irc->remote_addr,
				  sizeof(struct ZIRC_SOCKADDR_IN),
				  NULL, K_FOREVER, irc);
	if (ret < 0) {
		NET_DBG("Could not connect, errno %d", ret);
		goto connect_exit;
	}

	net_context_recv(irc->conn, on_context_recv, K_NO_WAIT, irc);

	chan = irc->data;

	ret = snprintk(name_buf, sizeof(name_buf), "zephyrbot%u",
		       sys_rand32_get());
	if (ret < 0 || ret >= sizeof(name_buf)) {
		panic("Can't fill name buffer");
	}

	if (zirc_nick_set(irc, name_buf) < 0) {
		panic("Could not set nick");
	}

	if (zirc_user_set(irc, name_buf, "Zephyr IRC Bot") < 0) {
		panic("Could not set user");
	}

	if (zirc_chan_join(irc, chan, DEFAULT_CHANNEL, on_msg_rcvd, NULL) < 0) {
		panic("Could not join channel");
	}
	return ret;

connect_exit:
	net_context_put(irc->conn);
	return ret;
}

static int
zirc_disconnect(struct zirc *irc)
{
	NET_INFO("Disconnecting");

	irc->chans = NULL;
	return net_context_put(irc->conn);
}

static int
zirc_chan_send_msg(const struct zirc_chan *chan, const char *msg)
{
	char buffer[128];

	NET_INFO("Sending to channel %s: %s", chan->chan, msg);

	while (*msg) {
		int msglen, txret;

		msglen = snprintk(buffer, sizeof(buffer), "PRIVMSG %s :%s\r\n",
				  chan->chan, msg);

		if (msglen < 0) {
			return msglen;
		}

		txret = transmit(chan->irc->conn, buffer, msglen);
		if (txret < 0) {
			return txret;
		}

		if (msglen < sizeof(buffer)) {
			return 0;
		}

		msg += msglen - sizeof("PRIVMSG  :\r\n") - 1 +
		       strlen(chan->chan);
	}

	return 0;
}

static void
on_cmd_hello(struct zirc_chan *chan, const char *nick, const char *msg)
{
	char buf[64];
	int ret;

	ret = snprintk(buf, sizeof(buf), "Hello, %s!", nick);
	if (ret < 0 || ret >= sizeof(buf)) {
		zirc_chan_send_msg(chan, "Hello, world!  (Your nick is "
				   "larger than my stack allows.)");
	} else {
		zirc_chan_send_msg(chan, buf);
	}
}

static void
on_cmd_random(struct zirc_chan *chan, const char *nick, const char *msg)
{
	char buf[128];
	s32_t num = sys_rand32_get();
	int ret;

	switch (num & 3) {
	case 0:
		ret = snprintk(buf, sizeof(buf), "Here's a fresh, random "
			       "32-bit integer, %s: %d", nick, num);
		break;
	case 1:
		ret = snprintk(buf, sizeof(buf), "Another number, fresh off "
			       "the PRNG: %d", num);
		break;
	case 2:
		ret = snprintk(buf, sizeof(buf), "Some calculations "
			       "with senseless constants yielded %d", num);
		break;
	case 3:
		ret = snprintk(buf, sizeof(buf), "I rolled a fair dice and "
			       "the result is...  %d", num);
		break;
	default:
		/* Shut up the compiler, as this condition is impossible.  */
		ret = -1;
	}

	if (ret < 0 || ret >= sizeof(buf)) {
		zirc_chan_send_msg(chan, "I rolled a fair dice and the "
				   "number is...  7.3");
	} else {
		zirc_chan_send_msg(chan, buf);
	}
}

static bool
read_led(void)
{
	u32_t led = 0;
	int r;

	if (!led0) {
		return fake_led;
	}

	r = gpio_pin_read(led0, LED_PIN, &led);
	if (r < 0) {
		return false;
	}

	return !led;
}

static void
write_led(bool led)
{
	if (!led0) {
		fake_led = led;
	} else {
		gpio_pin_write(led0, LED_PIN, !led);
	}
}

static void
on_cmd_led_off(struct zirc_chan *chan, const char *nick, const char *msg)
{
	zirc_chan_send_msg(chan, "The LED should be *off* now");
	write_led(false);
}

static void
on_cmd_led_on(struct zirc_chan *chan, const char *nick, const char *msg)
{
	zirc_chan_send_msg(chan, "The LED should be *on* now");
	write_led(true);
}

static void
on_cmd_led_toggle(struct zirc_chan *chan, const char *nick, const char *msg)
{
	if (read_led()) {
		on_cmd_led_off(chan, nick, msg);
	} else {
		on_cmd_led_on(chan, nick, msg);
	}
}

static void
on_cmd_rejoin(struct zirc_chan *chan, const char *nick, const char *msg)
{
	zirc_chan_part(chan);
	zirc_chan_join(chan->irc, chan, DEFAULT_CHANNEL, on_msg_rcvd, NULL);
}

static void
on_cmd_disconnect(struct zirc_chan *chan, const char *nick, const char *msg)
{
	zirc_disconnect(chan->irc);
}

#define CMD(c) { \
	.cmd = "!" #c, \
	.cmd_len = sizeof(#c) - 1, \
	.func = on_cmd_ ## c \
}
static void
on_msg_rcvd(void *data, struct zirc_chan *chan, char *umask, char *msg)
{
	static const struct {
		const char *cmd;
		size_t cmd_len;
		void (*func)(struct zirc_chan *chan, const char *nick,
			     const char *msg);
	} commands[] = {
		CMD(hello),
		CMD(random),
		CMD(led_toggle),
		CMD(led_off),
		CMD(led_on),
		CMD(rejoin),
		CMD(disconnect),
	};
	char *nick, *end;
	int i;

	if (!umask) {
		return;
	}

	NET_INFO("Received from umask %s: %s", umask, msg);

	end = strchr(umask, '!');
	if (!end) {
		nick = NULL;
	} else {
		*end = '\0';
		nick = umask;
	}

	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (!strncmp(msg, commands[i].cmd, commands[i].cmd_len)) {
			msg += commands[i].cmd_len;
			commands[i].func(chan, nick, msg);
			return;
		}
	}

	if (!strncmp(msg, "!help", 5)) {
		char msg[32];
		int ret;

		/* TODO: loop through commands[] and create help text */
		/* TODO: add help message to command, "!help command"
		 *       sends it back
		 */

		ret = snprintk(msg, sizeof(msg), "%s, you're a grown up, figure"
			       " it out", nick);
		if (ret < 0 || ret >= sizeof(msg)) {
			zirc_chan_send_msg(chan, "Your nick is too long, and my"
					   " stack is limited. Can't help you");
		} else {
			zirc_chan_send_msg(chan, msg);
		}
	}
}
#undef CMD

#if defined(CONFIG_NET_DHCPV4) && !defined(CONFIG_NET_IPV6)
#define DHCPV4_TIMEOUT K_SECONDS(30)

static struct net_mgmt_event_callback mgmt4_cb;
static struct k_sem dhcpv4_ok = K_SEM_INITIALIZER(dhcpv4_ok, 0, UINT_MAX);

static void ipv4_addr_add_handler(struct net_mgmt_event_callback *cb,
				  u32_t mgmt_event,
				  struct net_if *iface)
{
	char hr_addr[NET_IPV4_ADDR_LEN];
	int i;

	if (mgmt_event != NET_EVENT_IPV4_ADDR_ADD) {
		return;
	}

	for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		struct net_if_addr *if_addr = &iface->ipv4.unicast[i];

		if (if_addr->addr_type != NET_ADDR_DHCP || !if_addr->is_used) {
			continue;
		}

		NET_INFO("IPv4 address: %s",
			 net_addr_ntop(AF_INET, &if_addr->address.in_addr,
				       hr_addr, NET_IPV4_ADDR_LEN));
		NET_INFO("Lease time: %u seconds", iface->dhcpv4.lease_time);
		NET_INFO("Subnet: %s",
			 net_addr_ntop(AF_INET, &iface->ipv4.netmask,
				       hr_addr, NET_IPV4_ADDR_LEN));
		NET_INFO("Router: %s",
			 net_addr_ntop(AF_INET, &iface->ipv4.gw,
				       hr_addr, NET_IPV4_ADDR_LEN));
		break;
	}

	k_sem_give(&dhcpv4_ok);
}

static void setup_dhcpv4(struct zirc *irc, struct net_if *iface)
{
	NET_INFO("Running dhcpv4 client...");

	net_mgmt_init_event_callback(&mgmt4_cb, ipv4_addr_add_handler,
				     NET_EVENT_IPV4_ADDR_ADD);
	net_mgmt_add_event_callback(&mgmt4_cb);

	net_dhcpv4_start(iface);

	if (k_sem_take(&dhcpv4_ok, DHCPV4_TIMEOUT) < 0) {
		panic("No IPv4 address.");
	}

	net_ipaddr_copy(&net_sin(&irc->local_addr)->sin_addr,
			&iface->dhcpv4.requested_ip);
	irc->local_addr.family = AF_INET;
	net_sin(&irc->local_addr)->sin_family = AF_INET;
	net_sin(&irc->local_addr)->sin_port = 0;

	k_sem_give(&irc->wait_iface);
}
#else
#define setup_dhcpv4(...)
#endif /* CONFIG_NET_DHCPV4 */

#if defined(CONFIG_NET_IPV4) && !defined(CONFIG_NET_DHCPV4)

static void setup_ipv4(struct zirc *irc, struct net_if *iface)
{
	char hr_addr[NET_IPV4_ADDR_LEN];
	struct in_addr addr;

	if (net_addr_pton(AF_INET, ZIRC_LOCAL_IP_ADDR, &addr)) {
		NET_ERR("Invalid address: %s", ZIRC_LOCAL_IP_ADDR);
		return;
	}

	net_if_ipv4_addr_add(iface, &addr, NET_ADDR_MANUAL, 0);

	NET_INFO("IPv4 address: %s",
		 net_addr_ntop(AF_INET, &addr, hr_addr, NET_IPV4_ADDR_LEN));

	if (in_addr_set(ZIRC_AF_INET, ZIRC_LOCAL_IP_ADDR, 0,
			&irc->local_addr) < 0) {
		NET_ERR("Invalid IP address: %s", ZIRC_LOCAL_IP_ADDR);
	}

	k_sem_give(&irc->wait_iface);
}

#else
#define setup_ipv4(...)
#endif /* CONFIG_NET_IPV4 && !CONFIG_NET_DHCPV4 */

#if defined(CONFIG_NET_IPV6)

#define DAD_TIMEOUT K_SECONDS(3)

static struct net_mgmt_event_callback mgmt6_cb;
static struct k_sem dad_ok = K_SEM_INITIALIZER(dad_ok, 0, UINT_MAX);
static struct in6_addr laddr;

static void ipv6_dad_ok_handler(struct net_mgmt_event_callback *cb,
				u32_t mgmt_event,
				struct net_if *iface)
{
	struct net_if_addr *ifaddr;

	if (mgmt_event != NET_EVENT_IPV6_DAD_SUCCEED) {
		return;
	}

	ifaddr = net_if_ipv6_addr_lookup(&laddr, &iface);
	if (!ifaddr ||
	    !(net_ipv6_addr_cmp(&ifaddr->address.in6_addr, &laddr) &&
	      ifaddr->addr_state == NET_ADDR_PREFERRED)) {
		/* Address is not yet properly setup */
		return;
	}

	k_sem_give(&dad_ok);
}

static void setup_ipv6(struct zirc *irc, struct net_if *iface)
{
	char hr_addr[NET_IPV6_ADDR_LEN];

	if (net_addr_pton(AF_INET6, ZIRC_LOCAL_IP_ADDR, &laddr)) {
		NET_ERR("Invalid address: %s", ZIRC_LOCAL_IP_ADDR);
		return;
	}

	NET_INFO("IPv6 address: %s",
		 net_addr_ntop(AF_INET6, &laddr, hr_addr, NET_IPV6_ADDR_LEN));

	net_mgmt_init_event_callback(&mgmt6_cb, ipv6_dad_ok_handler,
				     NET_EVENT_IPV6_DAD_SUCCEED);
	net_mgmt_add_event_callback(&mgmt6_cb);

	net_if_ipv6_addr_add(iface, &laddr, NET_ADDR_MANUAL, 0);

	if (k_sem_take(&dad_ok, DAD_TIMEOUT) < 0) {
		panic("IPv6 address setup failed.");
	}

	if (in_addr_set(ZIRC_AF_INET, ZIRC_LOCAL_IP_ADDR, 0,
			&irc->local_addr) < 0) {
		NET_ERR("Invalid IP address: %s", ZIRC_LOCAL_IP_ADDR);
	}

	k_sem_give(&irc->wait_iface);
}

#else
#define setup_ipv6(...)
#endif /* CONFIG_NET_IPV6 */

static void
initialize_network(struct zirc *irc)
{
	struct net_if *iface;

	NET_INFO("Initializing network");

#if defined(CONFIG_DNS_RESOLVER)
	k_sem_init(&irc->wait_dns, 0, UINT_MAX);
#endif

	k_sem_init(&irc->wait_iface, 0, UINT_MAX);

	iface = net_if_get_default();
	if (!iface) {
		panic("No default network interface");
	}

#if defined(CONFIG_NET_IPV6) && defined(CONFIG_NET_DHCPV6)
	/* TODO: IPV6 DHCP */
#elif !defined(CONFIG_NET_IPV6) && defined(CONFIG_NET_DHCPV4)
	setup_dhcpv4(irc, iface);
#elif !defined(CONFIG_NET_IPV6) && defined(CONFIG_NET_IPV4)
	setup_ipv4(irc, iface);
#else
	setup_ipv6(irc, iface);
#endif

	/* Wait until we are ready to continue */
	k_sem_take(&irc->wait_iface, K_FOREVER);
}

static void
initialize_hardware(void)
{
	NET_INFO("Initializing hardware");

	led0 = device_get_binding(LED_GPIO_NAME);
	if (led0) {
		gpio_pin_configure(led0, LED_PIN, GPIO_DIR_OUT);
	}
}

static void irc_bot(void)
{
	struct zirc irc = { };
	struct zirc_chan chan = { };
	int ret;

	NET_INFO("Zephyr IRC bot sample");

	initialize_network(&irc);
	initialize_hardware();

	ret = zirc_connect(&irc, DEFAULT_SERVER, DEFAULT_PORT, &chan);
	if (ret < 0 && ret != -EINPROGRESS) {
		panic("Could not connect");
	}

	for (;;) {
		k_sleep(K_FOREVER);
	}
}

void main(void)
{
	k_thread_create(&bot_thread, stack, STACK_SIZE,
			(k_thread_entry_t)irc_bot,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, 0);
}

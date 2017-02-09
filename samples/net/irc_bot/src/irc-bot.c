/*
 * Copyright (c) 2017 Intel Corporation.
 * Copyright (c) 2017 Linaro Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	 http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if 1
#define SYS_LOG_DOMAIN "irc"
#define NET_LOG_ENABLED 1
#endif

#include <board.h>
#include <drivers/rand32.h>
#include <errno.h>
#include <gpio.h>
#include <net/nbuf.h>
#include <net/net_context.h>
#include <net/net_core.h>
#include <net/net_if.h>
#include <stdio.h>
#include <zephyr.h>
#include <net/dns_client.h>

#if !defined(CONFIG_NET_IPV6) && !defined(CONFIG_NET_IPV4)
#error "CONFIG_NET_IPV6 or CONFIG_NET_IPV4 must be enabled for irc_bot"
#endif

#define STACK_SIZE	2048
uint8_t stack[STACK_SIZE];

#define CMD_BUFFER_SIZE 256
static uint8_t cmd_buf[CMD_BUFFER_SIZE];

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

#if defined(CONFIG_NET_SAMPLES_MY_IPV6_ADDR)
#define ZIRC_LOCAL_IP_ADDR	CONFIG_NET_SAMPLES_MY_IPV6_ADDR
#else
#define ZIRC_LOCAL_IP_ADDR	"2001:db8::1"
#endif /* CONFIG_NET_SAMPLES_MY_IPV6_ADDR */

#if defined(CONFIG_NET_SAMPLES_PEER_IPV6_ADDR)
#define ZIRC_PEER_IP_ADDR	CONFIG_NET_SAMPLES_PEER_IPV6_ADDR
#else
#define ZIRC_PEER_IP_ADDR	"2001:db8::2"
#endif /* CONFIG_NET_SAMPLES_PEER_IPV6_ADDR */

/* Google DNS server for IPv6 */
#define DNS_SERVER		"2001:4860:4860::8888"

#else /* CONFIG_NET_IPV4 */

#define ZIRC_AF_INET		AF_INET
#define ZIRC_SOCKADDR_IN	sockaddr_in

#if defined(CONFIG_NET_SAMPLES_MY_IPV4_ADDR)
#define ZIRC_LOCAL_IP_ADDR	CONFIG_NET_SAMPLES_MY_IPV4_ADDR
#else
#define ZIRC_LOCAL_IP_ADDR	"192.168.0.1"
#endif /* CONFIG_NET_SAMPLES_MY_IPV4_ADDR */

#if defined(CONFIG_NET_SAMPLES_PEER_IPV4_ADDR)
#define ZIRC_PEER_IP_ADDR	CONFIG_NET_SAMPLES_PEER_IPV4_ADDR
#else
#define ZIRC_PEER_IP_ADDR	"192.168.0.2"
#endif /* CONFIG_NET_SAMPLES_PEER_IPV4_ADDR */

/* Google DNS server for IPv4 */
#define DNS_SERVER		"8.8.8.8"

#endif

/* DNS API */
#define DNS_PORT		53
#define DNS_SLEEP_MSECS		400

/* IRC API */
#define DEFAULT_SERVER	"irc.freenode.net"
#define DEFAULT_PORT	6667
#define DEFAULT_CHANNEL	"#zephyrbot"

struct zirc_chan;

typedef void (*on_privmsg_rcvd_cb_t)(void *data, struct zirc_chan *chan,
				  char *umask, char *msg);

struct zirc {
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
	struct net_buf *send_buf;

	send_buf = net_nbuf_get_tx(ctx, K_FOREVER);
	if (!send_buf) {
		return -ENOMEM;
	}

	if (!net_nbuf_append(send_buf, len, buffer, K_FOREVER)) {
		return -EINVAL;
	}

	return net_context_send(send_buf, NULL, K_NO_WAIT, NULL, NULL);
}

static void
on_cmd_ping(struct zirc *irc, char *umask, char *cmd, size_t len)
{
	char pong[32];
	int ret;

	NET_INFO("Got PING command from server: %s", cmd);

	ret = snprintk(pong, 32, "PONG :%s", cmd + 1);
	if (ret < sizeof(pong)) {
		transmit(irc->conn, pong, ret);
		/* TODO: what to do with the return value of transmit()? */
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
on_context_recv(struct net_context *ctx, struct net_buf *buf,
				int status, void *data)
{
	struct zirc *irc = data;
	struct net_buf *tmp;
	uint8_t *end_of_line;
	size_t len;
	uint16_t pos = 0, cmd_len = 0;

	if (!buf) {
		/* TODO: notify of disconnection, maybe reconnect? */
		NET_ERR("Disconnected\n");
		return;
	}

	if (status) {
		/* TODO: handle connection error */
		NET_ERR("Connection error: %d\n", -status);
		net_nbuf_unref(buf);
		return;
	}

	/* tmp points to fragment containing IP header */
	tmp = buf->frags;
	/* skip pos to the first TCP payload */
	pos = net_nbuf_appdata(buf) - tmp->data;

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

		tmp = net_nbuf_read(tmp, pos, &pos, len, cmd_buf + cmd_len);
		cmd_len += len;

		if (end_of_line) {
			/* skip the /n char after /r */
			if (tmp) {
				tmp = net_nbuf_read(tmp, pos, &pos, 1, NULL);
			}

			cmd_buf[cmd_len] = '\0';
			process_command(irc, cmd_buf, cmd_len);
			cmd_len = 0;
		}
	}

	net_nbuf_unref(buf);

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

static int
zirc_dns_lookup(const char *host,
		struct sockaddr *src_addr,
		struct sockaddr *dst_addr)
{
	struct net_context *net_ctx;
	struct dns_context dns_ctx;
	struct sockaddr dns_addr = { 0 };
	int ret;

	/* query DNS for server IP */
	dns_init(&dns_ctx);

	ret = net_context_get(ZIRC_AF_INET, SOCK_DGRAM, IPPROTO_UDP, &net_ctx);
	if (ret < 0) {
		NET_ERR("Cannot get network context for IP UDP: %d\n", -ret);
		return ret;
	}

	ret = net_context_bind(net_ctx, src_addr,
			       sizeof(struct ZIRC_SOCKADDR_IN));
	if (ret < 0) {
		NET_ERR("Cannot bind: %d\n", -ret);
		goto dns_exit;
	}

	in_addr_set(ZIRC_AF_INET, DNS_SERVER, DNS_PORT, &dns_addr);

	dns_ctx.net_ctx = net_ctx;
	dns_ctx.timeout = DNS_SLEEP_MSECS;
	dns_ctx.dns_server = &dns_addr;
	dns_ctx.elements = 1;
#ifdef CONFIG_NET_IPV6
	dns_ctx.query_type = DNS_QUERY_TYPE_AAAA;
	dns_ctx.address.ipv6 = &net_sin6(dst_addr)->sin6_addr;
#else
	dns_ctx.query_type = DNS_QUERY_TYPE_A;
	dns_ctx.address.ipv4 = &net_sin(dst_addr)->sin_addr;
#endif
	dns_ctx.name = DEFAULT_SERVER;
	ret = dns_resolve(&dns_ctx);
	if (ret != 0) {
		NET_ERR("dns_resolve ERROR %d\n", -ret);
	}

dns_exit:
	net_context_put(net_ctx);

	return ret;
}

static int
zirc_connect(struct zirc *irc, const char *host, int port, void *data)
{
	struct net_if *iface;
	struct sockaddr dst_addr, src_addr;
	struct zirc_chan *chan;
	int ret;
	char name_buf[32];

	NET_INFO("Connecting to %s:%d...", host, port);

	ret = net_context_get(ZIRC_AF_INET, SOCK_STREAM, IPPROTO_TCP,
			      &irc->conn);
	if (ret < 0) {
		NET_DBG("Could not get new context: %d", -ret);
		return ret;
	}

	iface = net_if_get_default();
	if (!iface) {
		NET_DBG("Could not get new context: %d", -ret);
		return -EIO;
	}

#if defined(CONFIG_NET_IPV6) && defined(CONFIG_NET_DHCPV6)
	/* TODO: IPV6 DHCP */
#elif defined(CONFIG_NET_IPV4) && defined(CONFIG_NET_DHCPV4)
	net_ipaddr_copy(&net_sin(&src_addr)->sin_addr,
			&iface->dhcpv4.requested_ip);
	ret = in_addr_set(ZIRC_AF_INET, NULL, 0, &src_addr);
#else
	ret = in_addr_set(ZIRC_AF_INET, ZIRC_LOCAL_IP_ADDR,
			  0, &src_addr);
	if (ret < 0) {
		goto connect_exit;
	}
#endif

#if defined(CONFIG_DNS_RESOLVER)
	in_addr_set(ZIRC_AF_INET, NULL, port, &dst_addr);

	/* set the IP address for the dst_addr via DNS */
	ret = zirc_dns_lookup(host, &src_addr, &dst_addr);
	if (ret < 0) {
		NET_ERR("Could not peform DNS lookup on host %s: %d",
			host, -ret);
		goto connect_exit;
	}
#else
	ret = in_addr_set(ZIRC_AF_INET, ZIRC_PEER_IP_ADDR,
			  port, &dst_addr);
	if (ret < 0) {
		goto connect_exit;
	}
#endif

	ret = net_context_bind(irc->conn, &src_addr,
			       sizeof(struct ZIRC_SOCKADDR_IN));
	if (ret < 0) {
		NET_DBG("Could not bind to local address: %d", -ret);
		goto connect_exit;
	}

	irc->data = data;

	ret = net_context_connect(irc->conn, &dst_addr,
				  sizeof(struct ZIRC_SOCKADDR_IN),
				  NULL, K_FOREVER, irc);
	if (ret < 0) {
		NET_DBG("Could not connect, errno %d", -ret);
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
	int32_t num = sys_rand32_get();
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
	uint32_t led = 0;
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

static void
initialize_network(void)
{
	struct net_if *iface;

	 NET_INFO("Initializing network");

	iface = net_if_get_default();
	if (!iface) {
		panic("No default network interface");
	}

#if defined(CONFIG_NET_IPV6) && defined(CONFIG_NET_DHCPV6)
	/* TODO: IPV6 DHCP */
#elif defined(CONFIG_NET_IPV4) && defined(CONFIG_NET_DHCPV4)
	net_dhcpv4_start(iface);

	/* delay so DHCPv4 can assign IP */
	/* TODO: add a timeout/retry */
	NET_INFO("Waiting for DHCP ...");
	do {
		k_sleep(K_SECONDS(1));
	} while (net_is_ipv4_addr_unspecified(&iface->dhcpv4.requested_ip));

	NET_INFO("Done!");

	/* TODO: add a timeout */
	NET_INFO("Waiting for IP assginment ...");
	do {
		k_sleep(K_SECONDS(1));
	} while (!net_is_my_ipv4_addr(&iface->dhcpv4.requested_ip));

	NET_INFO("Done!");
#else
	struct sockaddr addr;

	if (in_addr_set(ZIRC_AF_INET, ZIRC_LOCAL_IP_ADDR, 0,
			  &addr) < 0) {
		NET_ERR("Invalid IP address: %s",
			ZIRC_LOCAL_IP_ADDR);
	}

#if defined(CONFIG_NET_IPV6)
	net_if_ipv6_addr_add(iface,
			     &net_sin6(&addr)->sin6_addr,
			     NET_ADDR_MANUAL, 0);
#else
	net_if_ipv4_addr_add(iface,
			     &net_sin(&addr)->sin_addr,
			     NET_ADDR_MANUAL, 0);
#endif
#endif /* CONFIG_NET_IPV6 && CONFIG_NET_DHCPV6 */
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

	NET_INFO("Zephyr IRC bot sample");

	initialize_network();
	initialize_hardware();

	if (zirc_connect(&irc, DEFAULT_SERVER, DEFAULT_PORT, &chan) < 0) {
		panic("Could not connect");
	}
}

void main(void)
{
	k_thread_spawn(stack, STACK_SIZE, (k_thread_entry_t)irc_bot,
		       NULL, NULL, NULL, K_PRIO_COOP(7), 0, 0);
}

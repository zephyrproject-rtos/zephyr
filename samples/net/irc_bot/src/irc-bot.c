/*
 * Copyright (c) 2017 Intel Corporation.
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
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
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

#define STACK_SIZE	2048
uint8_t stack[STACK_SIZE];

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

/* Network */
#if !defined(CONFIG_NET_SAMPLES_MY_IPV6_ADDR)
#define CONFIG_NET_SAMPLES_MY_IPV6_ADDR "2001:db8::1"
#endif /* CONFIG_NET_SAMPLES_MY_IPV6_ADDR */

#if !defined(CONFIG_NET_SAMPLES_PEER_IPV6_ADDR)
#define CONFIG_NET_SAMPLES_PEER_IPV6_ADDR "2001:db8::2"
#endif /* CONFIG_NET_SAMPLES_PEER_IPV6_ADDR */

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

	void (*on_connect)(void *data, struct zirc *irc);
	void *data;
};

struct zirc_chan {
	struct zirc *irc;
	struct zirc_chan *next;

	const char *chan;

	on_privmsg_rcvd_cb_t on_privmsg_rcvd;
	void *data;
};

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

	send_buf = net_nbuf_get_tx(ctx);
	if (!send_buf) {
		return -ENOMEM;
	}

	if (!net_nbuf_append(send_buf, len, buffer)) {
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

	if (!buf && !status) {
		/* TODO: notify of disconnection, maybe reconnect? */
		NET_INFO("Disconnected\n");
		return;
	}
	if (status < 0) {
		/* TODO: handle connection error */
		NET_INFO("Connection error: %d\n", -status);
		return;
	}

	/* tmp points to fragment containing IP header */
	tmp = buf->frags;
	/* Adavance tmp fragment to appdata (TCP payload) */
	net_buf_pull(tmp, net_nbuf_appdata(buf) - tmp->data);

	while (tmp) {
		while (true) {
			char *end_of_line = memchr(tmp->data, '\r',
						      tmp->len);
			size_t cmd_len;

			if (!end_of_line) {
				break;
			}

			*end_of_line = '\0';
			cmd_len = end_of_line - (char *)tmp->data - 1;
			process_command(irc, tmp->data, cmd_len);

			cmd_len++; /* Account for \n also */
			tmp->len -= cmd_len;
			tmp->data += cmd_len;
		}

		net_buf_frag_del(buf, tmp);
		tmp = buf->frags;

		/* TODO: handle messages that spans multiple fragments */
	}
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

static void
on_context_connect(struct net_context *ctx, void *data)
{
	struct zirc *irc = data;

	irc->conn = ctx;
	net_context_recv(ctx, on_context_recv, K_NO_WAIT, data);
	irc->on_connect(irc->data, irc);
}

static int
zirc_connect(struct zirc *irc, const char *host, int port,
	void (*on_connect)(void *data, struct zirc *irc), void *data)
{
	/* TODO: DNS lookup for host */
	struct sockaddr dst_addr, src_addr;
	int ret;

	NET_INFO("Connecting to %s:%d...", host, port);

	if (!on_connect) {
		NET_DBG("Connection callback not set");
		return -EINVAL;
	}

	ret = net_context_get(AF_INET6, SOCK_STREAM, IPPROTO_TCP,
			      &irc->conn);
	if (ret < 0) {
		NET_DBG("Could not get new context: %d", -ret);
		return ret;
	}

	ret = in_addr_set(AF_INET6, CONFIG_NET_SAMPLES_PEER_IPV6_ADDR,
			  port, &dst_addr);
	if (ret < 0) {
		goto connect_exit;
	}

	ret = in_addr_set(AF_INET6, CONFIG_NET_SAMPLES_MY_IPV6_ADDR,
			  0, &src_addr);
	if (ret < 0) {
		goto connect_exit;
	}

	ret = net_context_bind(irc->conn, &src_addr,
			       sizeof(struct sockaddr_in6));
	if (ret < 0) {
		NET_DBG("Could not bind to local address: %d", -ret);
		goto connect_exit;
	}

	irc->data = data;
	irc->on_connect = on_connect;

	ret = net_context_connect(irc->conn, &dst_addr,
				  sizeof(struct sockaddr_in6),
				  on_context_connect, K_FOREVER, irc);
	if (ret < 0) {
		NET_DBG("Could not connect, errno %d", -ret);
		goto connect_exit;
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
	char buffer[32];
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

	ret = transmit(chan->irc->conn, buffer, ret);
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

	for (cc = &chan->irc->chans, c = c->irc->chans;
	     c; cc = &c->next, c = c->next) {
		if (c == chan) {
			*cc = c->next;
			return 0;
		}
	}

	return -ENOENT;
}

static int
zirc_chan_send_msg(const struct zirc_chan *chan, const char *msg)
{
	char buffer[80];

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
	char buf[3 * sizeof(int) + 40];
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

static void on_msg_rcvd(void *data, struct zirc_chan *chan, char *umask,
			char *msg);

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
on_connect(void *data, struct zirc *irc)
{
	struct zirc_chan *chan = data;

	if (zirc_nick_set(irc, "zephyrbot") < 0) {
		panic("Could not set nick");
	}

	if (zirc_user_set(irc, "zephyrbot", "Zephyr IRC Bot") < 0) {
		panic("Could not set nick");
	}

	if (zirc_chan_join(irc, chan, DEFAULT_CHANNEL, on_msg_rcvd, NULL) < 0) {
		panic("Could not join channel");
	}
}

static void
initialize_network(void)
{
	struct sockaddr addr;

	/* TODO: use DHCP here, watch NET_EVENT_IF_UP, zirc_connect() when
	 * available, etc
	 */

	 NET_INFO("Initializing network");

#if defined(CONFIG_NET_SAMPLES_MY_IPV6_ADDR)
	if (in_addr_set(AF_INET6, CONFIG_NET_SAMPLES_MY_IPV6_ADDR, 0,
			  &addr) < 0) {
		NET_ERR("Invalid IPv6 address: %s",
			CONFIG_NET_SAMPLES_MY_IPV6_ADDR);
	}
#endif /* CONFIG_NET_SAMPLES_MY_IPV6_ADDR */

	net_if_ipv6_addr_add(net_if_get_default(),
			     &net_sin6(&addr)->sin6_addr,
			     NET_ADDR_MANUAL, 0);
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

	if (zirc_connect(&irc, DEFAULT_SERVER, DEFAULT_PORT,
			 on_connect, &chan) < 0) {
		panic("Could not connect");
	}
}

void main(void)
{
	k_thread_spawn(stack, STACK_SIZE, (k_thread_entry_t)irc_bot,
		       NULL, NULL, NULL, K_PRIO_COOP(7), 0, 0);
}

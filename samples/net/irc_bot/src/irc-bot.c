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
#include <net/net_app.h>
#include <stdio.h>
#include <zephyr.h>
#include <net/dns_resolve.h>

#if !defined(CONFIG_NET_IPV6) && !defined(CONFIG_NET_IPV4)
#error "CONFIG_NET_IPV6 or CONFIG_NET_IPV4 must be enabled for irc_bot"
#endif

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
#define WAIT_TIMEOUT		K_SECONDS(10)
#define CONNECT_TIMEOUT		K_SECONDS(10)
#define BUF_ALLOC_TIMEOUT	K_SECONDS(1)

/* IRC API */
#define DEFAULT_SERVER	"irc.freenode.net"
#define DEFAULT_PORT	6667
#define DEFAULT_CHANNEL	"#zephyrbot"

struct zirc_chan;

typedef void (*on_privmsg_rcvd_cb_t)(void *data, struct zirc_chan *chan,
				  char *umask, char *msg);

struct zirc {
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

static struct net_app_ctx app_ctx;

static void
panic(const char *msg)
{
	NET_ERR("Panic: %s", msg);
	for (;;) {
		k_sleep(K_FOREVER);
	}
}

static int
transmit(char buffer[], size_t len)
{
	struct net_pkt *send_pkt;
	int ret;

	send_pkt = net_app_get_net_pkt(&app_ctx, AF_UNSPEC, BUF_ALLOC_TIMEOUT);
	if (!send_pkt) {
		NET_ERR("Unable to get TX packet, not enough memory.");
		return -ENOMEM;
	}

	if (!net_pkt_append_all(send_pkt, len, (u8_t *)buffer,
				BUF_ALLOC_TIMEOUT)) {
		net_pkt_unref(send_pkt);
		return -EINVAL;
	}

	ret = net_app_send_pkt(&app_ctx, send_pkt,
			       &app_ctx.default_ctx->remote,
			       NET_SOCKADDR_MAX_SIZE, K_NO_WAIT, NULL);
	if (ret < 0) {
		net_pkt_unref(send_pkt);
	}

	return ret;
}

static void
on_cmd_ping(struct zirc *irc, char *umask, char *cmd, size_t len)
{
	char pong[32];
	int ret;

	NET_INFO("Got PING command from server: %s", cmd);

	ret = snprintk(pong, 32, "PONG :%s", cmd + 1);
	if (ret < sizeof(pong)) {
		ret = transmit(pong, ret);
		if (ret < 0) {
			NET_ERR("Transmit error: %d", ret);
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
on_context_recv(struct net_app_ctx *ctx, struct net_pkt *pkt,
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
			process_command(irc, (char *)cmd_buf, cmd_len);
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

	return transmit(buffer, ret);
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

	return transmit(buffer, ret);
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

	ret = transmit(buffer, ret);
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

	ret = transmit(buffer, ret);
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

static int
zirc_connect(struct zirc *irc, const char *host, int port, void *data)
{
	struct zirc_chan *chan;
	int ret;
	char name_buf[32];

	NET_INFO("Connecting to %s:%d...", host, port);

	ret = net_app_init_tcp_client(&app_ctx, NULL, NULL,
				      host, port,
				      WAIT_TIMEOUT, NULL);
	if (ret < 0) {
		NET_ERR("net_app_init_tcp_client err:%d", ret);
		return ret;
	}

	/* set net_app callbacks */
	ret = net_app_set_cb(&app_ctx, NULL, on_context_recv, NULL, NULL);
	if (ret < 0) {
		NET_ERR("Could not set receive callback (err:%d)", ret);
		return ret;
	}

	irc->data = data;

	ret = net_app_connect(&app_ctx, CONNECT_TIMEOUT);
	if (ret < 0) {
		NET_ERR("Cannot connect (%d)", ret);
		panic("Can't init network");
	}

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
}

static int
zirc_disconnect(struct zirc *irc)
{
	NET_INFO("Disconnecting");

	irc->chans = NULL;
	net_app_close(&app_ctx);
	return net_app_release(&app_ctx);
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

		txret = transmit(buffer, msglen);
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

static void
initialize_hardware(void)
{
	NET_INFO("Initializing hardware");

	led0 = device_get_binding(LED_GPIO_NAME);
	if (led0) {
		gpio_pin_configure(led0, LED_PIN, GPIO_DIR_OUT);
	}
}

void main(void)
{
	struct zirc irc = { };
	struct zirc_chan chan = { };
	int ret;

	NET_INFO("Zephyr IRC bot sample");

	initialize_hardware();

	ret = zirc_connect(&irc, DEFAULT_SERVER, DEFAULT_PORT, &chan);
	if (ret < 0 && ret != -EINPROGRESS) {
		panic("Could not connect");
	}

	for (;;) {
		k_sleep(K_FOREVER);
	}
}

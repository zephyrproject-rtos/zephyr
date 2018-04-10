/*
 * Copyright (c) 2017 Intel Corporation.
 * Copyright (c) 2017 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_DOMAIN "irc"
#define SYS_LOG_LEVEL SYS_LOG_LEVEL_INFO
#include <logging/sys_log.h>

#include <board.h>
#include <random/rand32.h>
#include <errno.h>
#include <gpio.h>
#include <net/net_app.h>
#include <stdio.h>
#include <zephyr.h>
#include <net/dns_resolve.h>

#if !defined(CONFIG_NET_IPV6) && !defined(CONFIG_NET_IPV4)
#error "CONFIG_NET_IPV6 or CONFIG_NET_IPV4 must be enabled for irc_bot"
#endif

#define APP_BANNER "Zephyr IRC bot sample"

static void on_msg_rcvd(char *chan_name, char *umask, char *msg);

#define CMD_BUFFER_SIZE 256
static u8_t cmd_buf[CMD_BUFFER_SIZE];
static u16_t cmd_len;

#define NICK_BUFFER_SIZE 16
static char nick_buf[NICK_BUFFER_SIZE];

/* LED */
#ifndef LED0_GPIO_CONTROLLER
#ifdef LED0_GPIO_PORT
#define LED0_GPIO_CONTROLLER 	LED0_GPIO_PORT
#else
#define LED0_GPIO_CONTROLLER "(fail)"
#define LED0_GPIO_PIN 0
#endif
#endif

#define LED_GPIO_NAME LED0_GPIO_CONTROLLER
#define LED_PIN LED0_GPIO_PIN


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

static struct net_app_ctx app_ctx;

#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
NET_PKT_TX_SLAB_DEFINE(tx_tcp, 15);
NET_PKT_DATA_POOL_DEFINE(data_tcp, 30);

static struct k_mem_slab *tx_slab(void)
{
	return &tx_tcp;
}

static struct net_buf_pool *data_pool(void)
{
	return &data_tcp;
}
#else
#define tx_slab NULL
#define data_pool NULL
#endif /* CONFIG_NET_CONTEXT_NET_PKT_POOL */

static void
panic(const char *msg)
{
	SYS_LOG_ERR("Panic: %s", msg);
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
		SYS_LOG_ERR("Unable to get TX packet, not enough memory.");
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
on_cmd_ping(char *umask, char *cmd, size_t len)
{
	char pong[32];
	int ret;

	SYS_LOG_INF("Got PING command from server: %s", cmd);

	ret = snprintk(pong, 32, "PONG :%s", cmd + 1);
	if (ret < sizeof(pong)) {
		ret = transmit(pong, ret);
		if (ret < 0) {
			SYS_LOG_INF("Transmit error: %d", ret);
		}
	}
}

static void
on_cmd_privmsg(char *umask, char *cmd, size_t len)
{
	char *space;

	if (!umask) {
		/* Don't know how this got here, so ignore */
		return;
	}

	SYS_LOG_DBG("Got message from umask %s: %s", umask, cmd);

	space = memchr(cmd, ' ', len);
	if (!space) {
		return;
	}

	*space = '\0';

	if (*(space + 1) != ':') {
		/* Just ignore messages without a ':' after the space */
		SYS_LOG_DBG("Ignoring message umask: %s: %s", umask, space);
		return;
	}

	space += 2; /* Jump over the ':', pointing to the message itself */

	/* check for a private message from another user */
	if (!strncmp(nick_buf, cmd, NICK_BUFFER_SIZE)) {
		on_msg_rcvd(umask, umask, space);
	} else {
		on_msg_rcvd(cmd, umask, space);
	}
}

#define CMD(cmd_, cb_) { \
	.cmd = cmd_ " ", \
	.cmd_len = sizeof(cmd_ " ") - 1, \
	.func = on_cmd_ ## cb_ \
}
static void
process_command(char *cmd, size_t len)
{
	static const struct {
		const char *cmd;
		size_t cmd_len;
		void (*func)(char *umask, char *cmd, size_t len);
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

		SYS_LOG_DBG("Received from server, umask=%s: %s", umask, cmd);
	} else {
		umask = NULL;

		SYS_LOG_DBG("Received from server (no umask): %s", cmd);
	}

	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (len < commands[i].cmd_len) {
			continue;
		}
		if (!strncmp(cmd, commands[i].cmd, commands[i].cmd_len)) {
			SYS_LOG_DBG("Command has handler, executing");

			cmd += commands[i].cmd_len;
			len -= commands[i].cmd_len;
			commands[i].func(umask, cmd, len);

			return;
		}
	}

	/* TODO: handle notices, CTCP, etc */
	SYS_LOG_DBG("Could not find handler to handle %s, ignoring", cmd);
}
#undef CMD

static void
on_context_recv(struct net_app_ctx *ctx, struct net_pkt *pkt,
		int status, void *data)
{
	struct net_buf *tmp;
	u16_t pos = 0;

	if (!pkt) {
		/* TODO: notify of disconnection, maybe reconnect? */
		SYS_LOG_ERR("Disconnected\n");
		return;
	}

	if (status) {
		/* TODO: handle connection error */
		SYS_LOG_ERR("Connection error: %d\n", -status);
		net_pkt_unref(pkt);
		return;
	}

	/* tmp points to fragment containing IP header */
	tmp = pkt->frags;
	/* skip pos to the first TCP payload */
	pos = net_pkt_appdata(pkt) - tmp->data;

	while (true) {
		if (cmd_len >= CMD_BUFFER_SIZE) {
			SYS_LOG_WRN("cmd_buf overrun (>%d) Ignoring",
				    CMD_BUFFER_SIZE);
			cmd_len = 0;
		}

		tmp = net_frag_read(tmp, pos, &pos, 1, cmd_buf + cmd_len);
		if (*(cmd_buf + cmd_len) == '\r') {
			cmd_buf[cmd_len] = '\0';
			process_command((char *)cmd_buf, cmd_len);
			/* skip the \n after \r */
			tmp = net_frag_read(tmp, pos, &pos, 1, NULL);
			cmd_len = 0;
		} else {
			cmd_len++;
		}

		if (!tmp || pos == 0xFFFF) {
			break;
		}
	}

	net_pkt_unref(pkt);
}

static int
zirc_nick_set(const char *nick)
{
	char buffer[32];
	int ret;

	SYS_LOG_INF("Setting nickname to: %s", nick);

	ret = snprintk(buffer, sizeof(buffer), "NICK %s\r\n", nick);
	if (ret < 0 || ret >= sizeof(buffer)) {
		return -EINVAL;
	}

	return transmit(buffer, ret);
}

static int
zirc_user_set(const char *user, const char *realname)
{
	char buffer[64];
	int ret;

	SYS_LOG_INF("Setting user to: %s, real name to: %s", user, realname);

	ret = snprintk(buffer, sizeof(buffer), "USER %s * * :%s\r\n",
		       user, realname);
	if (ret < 0 || ret >= sizeof(buffer)) {
		return -EINVAL;
	}

	return transmit(buffer, ret);
}

static int
zirc_chan_join(const char *channel)
{
	char buffer[32];
	int ret;

	SYS_LOG_INF("Joining channel: %s", channel);

	ret = snprintk(buffer, sizeof(buffer), "JOIN %s\r\n", channel);
	if (ret < 0 || ret >= sizeof(buffer)) {
		return -EINVAL;
	}

	ret = transmit(buffer, ret);
	return ret;
}

static int
zirc_chan_part(char *chan_name)
{
	char buffer[32];
	int ret;

	SYS_LOG_INF("Leaving channel: %s", chan_name);

	ret = snprintk(buffer, sizeof(buffer), "PART %s\r\n", chan_name);
	if (ret < 0 || ret >= sizeof(buffer)) {
		return -EINVAL;
	}

	ret = transmit(buffer, ret);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int
zirc_connect(const char *host, int port)
{
	int ret;

	SYS_LOG_INF("Connecting to %s:%d...", host, port);

	ret = net_app_init_tcp_client(&app_ctx, NULL, NULL,
				      host, port,
				      WAIT_TIMEOUT, NULL);
	if (ret < 0) {
		SYS_LOG_ERR("net_app_init_tcp_client err:%d", ret);
		return ret;
	}

#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
	net_app_set_net_pkt_pool(&app_ctx, tx_slab, data_pool);
#endif

	/* set net_app callbacks */
	ret = net_app_set_cb(&app_ctx, NULL, on_context_recv, NULL, NULL);
	if (ret < 0) {
		SYS_LOG_ERR("Could not set receive callback (err:%d)", ret);
		return ret;
	}

	ret = net_app_connect(&app_ctx, CONNECT_TIMEOUT);
	if (ret < 0) {
		SYS_LOG_ERR("Cannot connect (%d)", ret);
		panic("Can't init network");
	}

	if (zirc_nick_set(nick_buf) < 0) {
		panic("Could not set nick");
	}

	if (zirc_user_set(nick_buf, "Zephyr IRC Bot") < 0) {
		panic("Could not set user");
	}

	if (zirc_chan_join(DEFAULT_CHANNEL) < 0) {
		panic("Could not join channel");
	}
	return ret;
}

static int
zirc_disconnect(void)
{
	SYS_LOG_INF("Disconnecting");
	cmd_len = 0;
	net_app_close(&app_ctx);
	return net_app_release(&app_ctx);
}

static int
zirc_chan_send_msg(const char *chan_name, const char *msg)
{
	char buffer[128];

	SYS_LOG_INF("Sending to channel/user %s: %s", chan_name, msg);

	while (*msg) {
		int msglen, txret;

		msglen = snprintk(buffer, sizeof(buffer), "PRIVMSG %s :%s\r\n",
				  chan_name, msg);

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
		       strlen(chan_name);
	}

	return 0;
}

static void
on_cmd_hello(char *chan_name, const char *nick, const char *msg)
{
	char buf[64];
	int ret;

	ret = snprintk(buf, sizeof(buf), "Hello, %s!", nick);
	if (ret < 0 || ret >= sizeof(buf)) {
		zirc_chan_send_msg(chan_name, "Hello, world!  (Your nick is "
				   "larger than my stack allows.)");
	} else {
		zirc_chan_send_msg(chan_name, buf);
	}
}

static void
on_cmd_random(char *chan_name, const char *nick, const char *msg)
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
		zirc_chan_send_msg(chan_name, "I rolled a fair dice and the "
				   "number is...  7.3");
	} else {
		zirc_chan_send_msg(chan_name, buf);
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
on_cmd_led_off(char *chan_name, const char *nick, const char *msg)
{
	zirc_chan_send_msg(chan_name, "The LED should be *off* now");
	write_led(false);
}

static void
on_cmd_led_on(char *chan_name, const char *nick, const char *msg)
{
	zirc_chan_send_msg(chan_name, "The LED should be *on* now");
	write_led(true);
}

static void
on_cmd_led_toggle(char *chan_name, const char *nick, const char *msg)
{
	if (read_led()) {
		on_cmd_led_off(chan_name, nick, msg);
	} else {
		on_cmd_led_on(chan_name, nick, msg);
	}
}

static void
on_cmd_rejoin(char *chan_name, const char *nick, const char *msg)
{
	/* make sure this isn't a private message */
	if (!strncmp(nick, chan_name, NICK_BUFFER_SIZE)) {
		zirc_chan_send_msg(chan_name,
				   "I can't rejoin a private message!");
	} else {
		zirc_chan_part(chan_name);
		zirc_chan_join(chan_name);
	}
}

static void
on_cmd_disconnect(char *chan_name, const char *nick, const char *msg)
{
	zirc_disconnect();
}

#define CMD(c) { \
	.cmd = "!" #c, \
	.cmd_len = sizeof(#c) - 1, \
	.func = on_cmd_ ## c \
}
static void
on_msg_rcvd(char *chan_name, char *umask, char *msg)
{
	static const struct {
		const char *cmd;
		size_t cmd_len;
		void (*func)(char *chan_name, const char *nick,
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

	SYS_LOG_DBG("Received from umask %s: %s", umask, msg);

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
			commands[i].func(chan_name, nick, msg);
			return;
		}
	}

	if (!strncmp(msg, "!help", 5)) {
		char msg[64];
		int ret;

		/* TODO: loop through commands[] and create help text */
		/* TODO: add help message to command, "!help command"
		 *       sends it back
		 */

		ret = snprintk(msg, sizeof(msg), "%s, you're a grown up, figure"
			       " it out", nick);
		if (ret < 0 || ret >= sizeof(msg)) {
			zirc_chan_send_msg(chan_name,
					   "Your nick is too long, and my"
					   " stack is limited. Can't help you");
		} else {
			zirc_chan_send_msg(chan_name, msg);
		}
	}
}
#undef CMD

void main(void)
{
	int ret;

	SYS_LOG_INF(APP_BANNER);

	led0 = device_get_binding(LED_GPIO_NAME);
	if (led0) {
		gpio_pin_configure(led0, LED_PIN, GPIO_DIR_OUT);
	}

	cmd_len = 0;

	/* setup IRC nick for max 16 chars */
	ret = snprintk(nick_buf, sizeof(nick_buf), "zephyrbot%05u",
		       sys_rand32_get() & 0xFFFF);
	if (ret < 0 || ret >= sizeof(nick_buf)) {
		panic("Can't fill nick buffer");
	}

	ret = zirc_connect(DEFAULT_SERVER, DEFAULT_PORT);
	if (ret < 0 && ret != -EINPROGRESS) {
		panic("Could not connect");
	}

	for (;;) {
		k_sleep(K_FOREVER);
	}
}

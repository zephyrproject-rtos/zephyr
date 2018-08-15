/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Websocket console
 *
 *
 * Websocket console driver. The console is provided over
 * a websocket connection.
 */

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_WEBSOCKET_CONSOLE_LEVEL
#define SYS_LOG_DOMAIN "ws/console"
#include <logging/sys_log.h>

#include <zephyr.h>
#include <init.h>
#include <misc/printk.h>

#include <console/console.h>
#include <net/buf.h>
#include <net/net_pkt.h>
#include <net/websocket_console.h>

#define NVT_NUL	0
#define NVT_LF	10
#define NVT_CR	13

#define WS_CONSOLE_STACK_SIZE	CONFIG_WEBSOCKET_CONSOLE_STACK_SIZE
#define WS_CONSOLE_PRIORITY	CONFIG_WEBSOCKET_CONSOLE_PRIO
#define WS_CONSOLE_TIMEOUT	K_MSEC(CONFIG_WEBSOCKET_CONSOLE_SEND_TIMEOUT)
#define WS_CONSOLE_LINES	CONFIG_WEBSOCKET_CONSOLE_LINE_BUF_NUMBERS
#define WS_CONSOLE_LINE_SIZE	CONFIG_WEBSOCKET_CONSOLE_LINE_BUF_SIZE
#define WS_CONSOLE_TIMEOUT	K_MSEC(CONFIG_WEBSOCKET_CONSOLE_SEND_TIMEOUT)
#define WS_CONSOLE_THRESHOLD	CONFIG_WEBSOCKET_CONSOLE_SEND_THRESHOLD

#define WS_CONSOLE_MIN_MSG	2

/* These 2 structures below are used to store the console output
 * before sending it to the client. This is done to keep some
 * reactivity: the ring buffer is non-protected, if first line has
 * not been sent yet, and if next line is reaching the same index in rb,
 * the first one will be replaced. In a perfect world, this should
 * not happen. However on a loaded system with a lot of debug output
 * this is bound to happen eventualy, moreover if it does not have
 * the luxury to bufferize as much as it wants to. Just raise
 * CONFIG_WEBSOCKET_CONSOLE_LINE_BUF_NUMBERS if possible.
 */
struct line_buf {
	char buf[WS_CONSOLE_LINE_SIZE];
	u16_t len;
};

struct line_buf_rb {
	struct line_buf l_bufs[WS_CONSOLE_LINES];
	u16_t line_in;
	u16_t line_out;
};

static struct line_buf_rb ws_rb;

NET_STACK_DEFINE(WS_CONSOLE, ws_console_stack,
		 WS_CONSOLE_STACK_SIZE, WS_CONSOLE_STACK_SIZE);
static struct k_thread ws_thread_data;
static K_SEM_DEFINE(send_lock, 0, UINT_MAX);

/* The timer is used to send non-lf terminated output that has
 * been around for "tool long". This will prove to be useful
 * to send the shell prompt for instance.
 * ToDo: raise the time, incrementaly, when no output is coming
 *       so the timer will kick in less and less.
 */
static void ws_send_prematurely(struct k_timer *timer);
static K_TIMER_DEFINE(send_timer, ws_send_prematurely, NULL);
static int (*orig_printk_hook)(int);

static struct k_fifo *avail_queue;
static struct k_fifo *input_queue;

/* Websocket context that this console is related to */
static struct http_ctx *ws_console;

extern void __printk_hook_install(int (*fn)(int));
extern void *__printk_get_hook(void);

void ws_register_input(struct k_fifo *avail, struct k_fifo *lines,
		       u8_t (*completion)(char *str, u8_t len))
{
	ARG_UNUSED(completion);

	avail_queue = avail;
	input_queue = lines;
}

static void ws_rb_init(void)
{
	int i;

	ws_rb.line_in = 0;
	ws_rb.line_out = 0;

	for (i = 0; i < WS_CONSOLE_LINES; i++) {
		ws_rb.l_bufs[i].len = 0;
	}
}

static void ws_end_client_connection(struct http_ctx *console)
{
	__printk_hook_install(orig_printk_hook);
	orig_printk_hook = NULL;

	k_timer_stop(&send_timer);

	ws_send_msg(console, NULL, 0, WS_OPCODE_CLOSE, false, true,
		    NULL, NULL);

	ws_rb_init();
}

static void ws_rb_switch(void)
{
	ws_rb.line_in++;

	if (ws_rb.line_in == WS_CONSOLE_LINES) {
		ws_rb.line_in = 0;
	}

	ws_rb.l_bufs[ws_rb.line_in].len = 0;

	/* Unfortunately, we don't have enough line buffer,
	 * so we eat the next to be sent.
	 */
	if (ws_rb.line_in == ws_rb.line_out) {
		ws_rb.line_out++;
		if (ws_rb.line_out == WS_CONSOLE_LINES) {
			ws_rb.line_out = 0;
		}
	}

	k_timer_start(&send_timer, WS_CONSOLE_TIMEOUT, WS_CONSOLE_TIMEOUT);
	k_sem_give(&send_lock);
}

static inline struct line_buf *ws_rb_get_line_out(void)
{
	u16_t out = ws_rb.line_out;

	ws_rb.line_out++;
	if (ws_rb.line_out == WS_CONSOLE_LINES) {
		ws_rb.line_out = 0;
	}

	if (!ws_rb.l_bufs[out].len) {
		return NULL;
	}

	return &ws_rb.l_bufs[out];
}

static inline struct line_buf *ws_rb_get_line_in(void)
{
	return &ws_rb.l_bufs[ws_rb.line_in];
}

/* The actual printk hook */
static int ws_console_out(int c)
{
	unsigned int key = irq_lock();
	struct line_buf *lb = ws_rb_get_line_in();
	bool yield = false;

	lb->buf[lb->len++] = (char)c;

	if (c == '\n' || lb->len == WS_CONSOLE_LINE_SIZE - 1) {
		lb->buf[lb->len-1] = NVT_CR;
		lb->buf[lb->len++] = NVT_LF;
		ws_rb_switch();
		yield = true;
	}

	irq_unlock(key);

#ifdef CONFIG_WEBSOCKET_CONSOLE_DEBUG_DEEP
	/* This is ugly, but if one wants to debug websocket console, it
	 * will also output the character to original console
	 */
	orig_printk_hook(c);
#endif

	if (yield) {
		k_yield();
	}

	return c;
}

static void ws_send_prematurely(struct k_timer *timer)
{
	struct line_buf *lb = ws_rb_get_line_in();

	if (lb->len >= WS_CONSOLE_THRESHOLD) {
		ws_rb_switch();
	}
}

static inline void ws_handle_input(struct net_pkt *pkt)
{
	struct console_input *input;
	u16_t len, offset, pos;

	len = net_pkt_appdatalen(pkt);
	if (len > CONSOLE_MAX_LINE_LEN || len < WS_CONSOLE_MIN_MSG) {
		return;
	}

	if (!avail_queue || !input_queue) {
		return;
	}

	input = k_fifo_get(avail_queue, K_NO_WAIT);
	if (!input) {
		return;
	}

	offset = net_pkt_get_len(pkt) - len;
	net_frag_read(pkt->frags, offset, &pos, len, (u8_t *)input->line);

	/* The data from websocket does not contain \n or NUL, so insert
	 * it here.
	 */
	input->line[len] = NVT_NUL;

	/* LF/CR will be removed if only the line is not NUL terminated */
	if (input->line[len-1] != NVT_NUL) {
		if (input->line[len-1] == NVT_LF) {
			input->line[len-1] = NVT_NUL;
		}

		if (input->line[len-2] == NVT_CR) {
			input->line[len-2] = NVT_NUL;
		}
	}

	k_fifo_put(input_queue, input);
}

/* The data is coming from outside system and going into zephyr */
int ws_console_recv(struct http_ctx *ctx, struct net_pkt *pkt)
{
	if (ctx != ws_console) {
		return -ENOENT;
	}

	ws_handle_input(pkt);

	net_pkt_unref(pkt);

	return 0;
}

/* This is for transferring data from zephyr to outside system */
static bool ws_console_send(struct http_ctx *console)
{
	struct line_buf *lb = ws_rb_get_line_out();

	if (lb) {
		(void)ws_send_msg(console, (u8_t *)lb->buf, lb->len,
				  WS_OPCODE_DATA_TEXT, false, true,
				  NULL, NULL);

		/* We reinitialize the line buffer */
		lb->len = 0;
	}

	return true;
}

/* WS console loop, used to send buffered output in the RB */
static void ws_console_run(void)
{
	while (true) {
		k_sem_take(&send_lock, K_FOREVER);

		if (!ws_console_send(ws_console)) {
			ws_end_client_connection(ws_console);
		}
	}
}

int ws_console_enable(struct http_ctx *ctx)
{
	orig_printk_hook = __printk_get_hook();
	__printk_hook_install(ws_console_out);

	k_timer_start(&send_timer, WS_CONSOLE_TIMEOUT, WS_CONSOLE_TIMEOUT);

	ws_console = ctx;

	return 0;
}

int ws_console_disable(struct http_ctx *ctx)
{
	if (!ws_console) {
		return 0;
	}

	if (ws_console != ctx) {
		return -ENOENT;
	}

	ws_end_client_connection(ws_console);

	ws_console = NULL;

	return 0;
}

static int ws_console_init(struct device *arg)
{
	k_thread_create(&ws_thread_data, ws_console_stack,
			K_THREAD_STACK_SIZEOF(ws_console_stack),
			(k_thread_entry_t)ws_console_run,
			NULL, NULL, NULL,
			K_PRIO_COOP(WS_CONSOLE_PRIORITY), 0, K_MSEC(10));

	SYS_LOG_INF("Websocket console initialized");

	return 0;
}

/* Websocket console is initialized as an application directly, as it requires
 * the whole network stack to be ready.
 */
SYS_INIT(ws_console_init, APPLICATION, CONFIG_WEBSOCKET_CONSOLE_INIT_PRIORITY);

/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Telnet (RFC 854, 855) with the COM port control option (RFC 2217), server
 * side.  The server offers WILL ECHO, WILL SUPPRESS-GO-AHEAD, DO BINARY and
 * WILL COM-PORT-OPTION, accepts these options from the client and refuses
 * all others.
 */

#include <string.h>

#include <zephyr/net/rfc2217.h>
#include <zephyr/sys/byteorder.h>

#define TELNET_IAC      255
#define TELNET_DO_NOT   254
#define TELNET_DO       253
#define TELNET_WILL_NOT 252
#define TELNET_WILL     251
#define TELNET_SB       250
#define TELNET_SE       240

#define OPT_BINARY   0
#define OPT_ECHO     1
#define OPT_SGA      3
#define OPT_COM_PORT 44

/* Client to server commands; the server answers with the command + 100 */
#define CPC_SIGNATURE           0
#define CPC_SET_BAUDRATE        1
#define CPC_SET_DATASIZE        2
#define CPC_SET_PARITY          3
#define CPC_SET_STOPSIZE        4
#define CPC_SET_CONTROL         5
#define CPC_NOTIFY_LINESTATE    6
#define CPC_NOTIFY_MODEMSTATE   7
#define CPC_FLOWCONTROL_SUSPEND 8
#define CPC_FLOWCONTROL_RESUME  9
#define CPC_SET_LINESTATE_MASK  10
#define CPC_SET_MODEMSTATE_MASK 11
#define CPC_PURGE_DATA          12
#define CPC_SERVER_OFFSET       100

enum {
	STATE_DATA,
	STATE_IAC,
	STATE_VERB,
	STATE_SB,
	STATE_SB_IAC,
};

static uint8_t opt_bit(uint8_t opt)
{
	switch (opt) {
	case OPT_BINARY:
		return BIT(0);
	case OPT_ECHO:
		return BIT(1);
	case OPT_SGA:
		return BIT(2);
	case OPT_COM_PORT:
		return BIT(3);
	default:
		return 0;
	}
}

/* Options the server enables on its side (WILL) and on the client's side (DO) */
#define LOCAL_OPTS  (BIT(0) | BIT(1) | BIT(2) | BIT(3))
#define REMOTE_OPTS (BIT(0) | BIT(2) | BIT(3))

static void send_option(struct rfc2217_session *s, uint8_t verb, uint8_t opt)
{
	const uint8_t msg[] = {TELNET_IAC, verb, opt};

	s->ops->send(s, msg, sizeof(msg));
}

static void send_command(struct rfc2217_session *s, uint8_t cmd, const uint8_t *val, size_t len)
{
	uint8_t msg[4 + 2 * MAX(sizeof(CONFIG_NET_RFC2217_SIGNATURE), 4) + 2];
	size_t n = 0;

	msg[n++] = TELNET_IAC;
	msg[n++] = TELNET_SB;
	msg[n++] = OPT_COM_PORT;
	msg[n++] = cmd + CPC_SERVER_OFFSET;
	for (size_t i = 0; i < len; i++) {
		msg[n++] = val[i];
		if (val[i] == TELNET_IAC) {
			msg[n++] = TELNET_IAC;
		}
	}
	msg[n++] = TELNET_IAC;
	msg[n++] = TELNET_SE;

	s->ops->send(s, msg, n);
}

static void send_command_u8(struct rfc2217_session *s, uint8_t cmd, uint8_t val)
{
	send_command(s, cmd, &val, 1);
}

void rfc2217_session_init(struct rfc2217_session *session, const struct rfc2217_session_ops *ops,
			  void *user_data)
{
	memset(session, 0, sizeof(*session));
	session->ops = ops;
	session->user_data = user_data;
	session->state = STATE_DATA;

	session->we_asked = opt_bit(OPT_ECHO) | opt_bit(OPT_SGA) | opt_bit(OPT_COM_PORT);
	session->they_asked = opt_bit(OPT_BINARY);
	send_option(session, TELNET_WILL, OPT_ECHO);
	send_option(session, TELNET_WILL, OPT_SGA);
	send_option(session, TELNET_DO, OPT_BINARY);
	send_option(session, TELNET_WILL, OPT_COM_PORT);
}

bool rfc2217_session_is_active(const struct rfc2217_session *session)
{
	return ((session->we | session->they) & opt_bit(OPT_COM_PORT)) != 0;
}

/* DO and DON'T are about the server's side, WILL and WON'T about the client's */
static void negotiate(struct rfc2217_session *s, uint8_t verb, uint8_t opt)
{
	bool local = verb == TELNET_DO || verb == TELNET_DO_NOT;
	bool enable = verb == TELNET_DO || verb == TELNET_WILL;
	uint8_t *state = local ? &s->we : &s->they;
	uint8_t *asked = local ? &s->we_asked : &s->they_asked;
	uint8_t allowed = local ? LOCAL_OPTS : REMOTE_OPTS;
	uint8_t bit = opt_bit(opt);

	if (enable) {
		if ((bit & allowed) == 0) {
			send_option(s, local ? TELNET_WILL_NOT : TELNET_DO_NOT, opt);
			return;
		}
		if ((*state & bit) == 0 && (*asked & bit) == 0) {
			send_option(s, local ? TELNET_WILL : TELNET_DO, opt);
		}
		*state |= bit;
	} else {
		if ((*state & bit) != 0) {
			send_option(s, local ? TELNET_WILL_NOT : TELNET_DO_NOT, opt);
		}
		*state &= ~bit;
	}
	*asked &= ~bit;
}

static void command(struct rfc2217_session *s, uint8_t cmd, const uint8_t *val, size_t len)
{
	const struct rfc2217_session_ops *ops = s->ops;
	uint8_t arg = len > 0 ? val[0] : 0;

	switch (cmd) {
	case CPC_SIGNATURE:
		/* An empty signature asks for the server's */
		if (len == 0) {
			send_command(s, cmd, (const uint8_t *)CONFIG_NET_RFC2217_SIGNATURE,
				     sizeof(CONFIG_NET_RFC2217_SIGNATURE) - 1);
		}
		break;
	case CPC_SET_BAUDRATE: {
		uint8_t out[4];

		sys_put_be32(ops->baud_rate(s, len >= 4 ? sys_get_be32(val) : 0), out);
		send_command(s, cmd, out, sizeof(out));
		break;
	}
	case CPC_SET_DATASIZE:
		send_command_u8(s, cmd, ops->data_size(s, arg));
		break;
	case CPC_SET_PARITY:
		send_command_u8(s, cmd, ops->parity(s, arg));
		break;
	case CPC_SET_STOPSIZE:
		send_command_u8(s, cmd, ops->stop_size(s, arg));
		break;
	case CPC_SET_CONTROL:
		send_command_u8(s, cmd, ops->control(s, arg));
		break;
	case CPC_NOTIFY_MODEMSTATE:
		send_command_u8(s, cmd, ops->modem_state != NULL ? ops->modem_state(s) : 0);
		break;
	case CPC_FLOWCONTROL_SUSPEND:
	case CPC_FLOWCONTROL_RESUME:
		if (ops->suspend != NULL) {
			ops->suspend(s, cmd == CPC_FLOWCONTROL_SUSPEND);
		}
		break;
	case CPC_SET_LINESTATE_MASK:
	case CPC_SET_MODEMSTATE_MASK:
		send_command_u8(s, cmd, arg);
		break;
	case CPC_PURGE_DATA:
		if (ops->purge != NULL && arg >= RFC2217_PURGE_RX && arg <= RFC2217_PURGE_BOTH) {
			ops->purge(s, arg);
		}
		send_command_u8(s, cmd, arg);
		break;
	default:
		break;
	}
}

static void sb_byte(struct rfc2217_session *s, uint8_t c)
{
	if (s->sb_len < sizeof(s->sb)) {
		s->sb[s->sb_len++] = c;
	} else {
		s->sb_overflow = true;
	}
}

static void sb_end(struct rfc2217_session *s)
{
	if (!s->sb_overflow && s->sb_len >= 2 && s->sb[0] == OPT_COM_PORT) {
		command(s, s->sb[1], &s->sb[2], s->sb_len - 2);
	}
}

void rfc2217_session_input(struct rfc2217_session *session, const uint8_t *buf, size_t len)
{
	struct rfc2217_session *s = session;
	size_t run = 0;

	for (size_t i = 0; i < len; i++) {
		uint8_t c = buf[i];

		switch (s->state) {
		case STATE_DATA:
			if (c == TELNET_IAC) {
				if (i > run) {
					s->ops->data(s, &buf[run], i - run);
				}
				s->state = STATE_IAC;
			} else if (c == '\0' && s->cr && (s->they & opt_bit(OPT_BINARY)) == 0) {
				/* NVT: CR NUL is a bare CR */
				if (i > run) {
					s->ops->data(s, &buf[run], i - run);
				}
				run = i + 1;
			}
			s->cr = c == '\r';
			continue;
		case STATE_IAC:
			switch (c) {
			case TELNET_IAC:
				s->ops->data(s, &c, 1);
				s->state = STATE_DATA;
				break;
			case TELNET_WILL:
			case TELNET_WILL_NOT:
			case TELNET_DO:
			case TELNET_DO_NOT:
				s->verb = c;
				s->state = STATE_VERB;
				break;
			case TELNET_SB:
				s->sb_len = 0;
				s->sb_overflow = false;
				s->state = STATE_SB;
				break;
			default:
				s->state = STATE_DATA;
				break;
			}
			break;
		case STATE_VERB:
			negotiate(s, s->verb, c);
			s->state = STATE_DATA;
			break;
		case STATE_SB:
			if (c == TELNET_IAC) {
				s->state = STATE_SB_IAC;
			} else {
				sb_byte(s, c);
			}
			break;
		case STATE_SB_IAC:
			if (c == TELNET_SE) {
				sb_end(s);
				s->state = STATE_DATA;
			} else {
				sb_byte(s, c);
				s->state = STATE_SB;
			}
			break;
		default:
			s->state = STATE_DATA;
			break;
		}
		run = i + 1;
	}

	if (s->state == STATE_DATA && len > run) {
		s->ops->data(s, &buf[run], len - run);
	}
}

size_t rfc2217_escape(const uint8_t *in, size_t in_len, uint8_t *out, size_t out_size,
		      size_t *consumed)
{
	size_t i;
	size_t n = 0;

	for (i = 0; i < in_len; i++) {
		size_t need = in[i] == TELNET_IAC ? 2 : 1;

		if (n + need > out_size) {
			break;
		}
		out[n++] = in[i];
		if (in[i] == TELNET_IAC) {
			out[n++] = TELNET_IAC;
		}
	}

	*consumed = i;

	return n;
}

/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(gsm_mux, CONFIG_MODEM_GSM_MUX_LOG_LEVEL);

#include <kernel.h>
#include <sys/util.h>
#include <sys/crc.h>
#include <net/buf.h>
#include <net/ppp.h>

#include "modem_context.h"
#include "gsm_mux.h"

/* Default values are from the specification 07.10 */
#define T1 10  /* 100 ms */
#define T2 34  /* 333 ms */
#define N2 3   /* retry 3 times */

#define FCS_INIT_VALUE 0xFF
#define FCS_GOOD_VALUE 0xCF

#define EA 0x01  /* Extension bit      */
#define CR 0x02  /* Command / Response */
#define PF 0x10  /* Poll / Final       */

/* Frame types */
#define FT_RR    0x01  /* Receive Ready                            */
#define FT_UI    0x03  /* Unnumbered Information                   */
#define FT_RNR   0x05  /* Receive Not Ready                        */
#define FT_REJ   0x09  /* Reject                                   */
#define FT_DM    0x0F  /* Disconnected Mode                        */
#define FT_SABM  0x2F  /* Set Asynchronous Balanced Mode           */
#define FT_DISC  0x43  /* Disconnect                               */
#define FT_UA    0x63  /* Unnumbered Acknowledgement               */
#define FT_UIH   0xEF  /* Unnumbered Information with Header check */

/* Control channel commands */
#define CMD_NSC    0x09  /* Non Supported Command Response      */
#define CMD_TEST   0x11  /* Test Command                        */
#define CMD_PSC    0x21  /* Power Saving Control                */
#define CMD_RLS    0x29  /* Remote Line Status Command          */
#define CMD_FCOFF  0x31  /* Flow Control Off Command            */
#define CMD_PN     0x41  /* DLC parameter negotiation           */
#define CMD_RPN    0x49  /* Remote Port Negotiation Command     */
#define CMD_FCON   0x51  /* Flow Control On Command             */
#define CMD_CLD    0x61  /* Multiplexer close down              */
#define CMD_SNC    0x69  /* Service Negotiation Command         */
#define CMD_MSC    0x71  /* Modem Status Command                */

/* Flag sequence field between messages (start of frame) */
#define SOF_MARKER 0xF9

/* Mux states */
enum gsm_mux_state {
	GSM_MUX_SOF,      /* Start of frame */
	GSM_MUX_ADDRESS,
	GSM_MUX_CONTROL,
	GSM_MUX_LEN_0,    /* First length byte */
	GSM_MUX_LEN_1,    /* Second length byte */
	GSM_MUX_DATA,
	GSM_MUX_FCS,      /* Frame Check Sequence */
	GSM_MUX_EOF       /* End of frame */
};

struct gsm_mux {
	struct modem_iface *iface;

	/* input/output functions */
	uart_pipe_recv_cb ppp_recv_cb;
	int (*modem_write_func)(struct modem_iface *iface, const u8_t *buf,
				size_t size);

	struct net_buf *buf;

	int mru;

	enum gsm_mux_state state;

	u16_t msg_len;     /* message length */
	u16_t received;    /* bytes so far received */
	u8_t address;      /* dlci address (only one byte supported) */
	u8_t control;      /* type of the frame */
	u8_t fcs;          /* calculated frame check sequence */
	u8_t received_fcs; /* packet fcs */

	bool in_use : 1;
	bool is_initiator : 1;
	bool refuse_service : 1; /* Do not try to talk to this modem */
};

/* DLCI states */
enum gsm_dlci_state {
	GSM_DLCI_CLOSED,
	GSM_DLCI_OPENING,
	GSM_DLCI_OPEN,
	GSM_DLCI_CLOSING
};

struct gsm_dlci;

typedef void (*dlci_process_msg_t)(struct gsm_dlci *dlci,
				   bool cmd,
				   struct net_buf *buf);
struct gsm_dlci {
	int num;
	struct gsm_mux *mux;
	dlci_process_msg_t process;
	enum gsm_dlci_state state;
	bool refuse_service : 1; /* Do not try to talk to this channel */
	bool in_use : 1;
};

/* From 07.10, Maximum Frame Size [1 – 128] in Basic mode */
#define MAX_MRU 128

/* Assume that there is two network buffers (one for RX and one for TX)
 * going on at the same time.
 */
#define MIN_BUF_COUNT (CONFIG_MODEM_GSM_MUX_MAX * 2)

NET_BUF_POOL_DEFINE(gsm_mux_pool, MIN_BUF_COUNT, MAX_MRU, 0, NULL);

#define BUF_ALLOC_TIMEOUT K_MSEC(50)

static struct gsm_mux muxes[CONFIG_MODEM_GSM_MUX_MAX];
static struct gsm_dlci dlcis[CONFIG_MODEM_GSM_MUX_DLCI_MAX];

static const char *get_frame_type_str(u8_t frame_type)
{
	switch (frame_type) {
	case FT_RR:
		return "RR";
	case FT_UI:
		return "UI";
	case FT_RNR:
		return "RNR";
	case FT_REJ:
		return "REJ";
	case FT_DM:
		return "DM";
	case FT_SABM:
		return "SABM";
	case FT_DISC:
		return "DISC";
	case FT_UA:
		return "UA";
	case FT_UIH:
		return "UIH";
	}

	return NULL;
}

static void hexdump_packet(const char *header, u8_t address, bool cmd_rsp,
			   u8_t control, u8_t *data, size_t len)
{
	const char *frame_type;
	char out[128];
	int ret;

	if (!IS_ENABLED(CONFIG_MODEM_GSM_MUX_LOG_LEVEL_DBG)) {
		return;
	}

	ret = snprintk(out, sizeof(out), "%s: addr %u %s ",
		       header, address, cmd_rsp ? "CMD" : "RSP");
	if (ret >= sizeof(out)) {
		goto print;
	}

	frame_type = get_frame_type_str(control & ~PF);
	if (frame_type) {
		ret += snprintk(out + ret, sizeof(out) - ret, "%s ",
				frame_type);
	} else if (!(control & 0x01)) {
		ret += snprintk(out + ret, sizeof(out) - ret,
				"I N(S)%d N(R)%d ",
				(control & 0x0E) >> 1,
				(control & 0xE0) >> 5);
	} else {
		frame_type = get_frame_type_str(control & 0x0F);
		if (frame_type) {
			ret += snprintk(out + ret, sizeof(out) - ret,
					"%s(%d) ", frame_type,
					(control & 0xE0) >> 5);
		} else {
			ret += snprintk(out + ret, sizeof(out) - ret,
					"[%02X] ", control);
		}
	}

	if (ret >= sizeof(out)) {
		goto print;
	}

	ret += snprintk(out + ret, sizeof(out) - ret, "%s",
			(control & PF) ? "(P)" : "(F)");
	if (ret >= sizeof(out)) {
		goto print;
	}

print:
	LOG_HEXDUMP_DBG(data, len, out);
}

static u8_t gsm_mux_fcs_add(u8_t fcs, u8_t recv_byte)
{
	/* TODO: verify that this is the correct crc func to use */
	return crc8_ccitt(fcs, &recv_byte, 1);
}

static u8_t gsm_mux_fcs_add_buf(u8_t fcs, u8_t *buf, size_t len)
{
	return crc8_ccitt(fcs, buf, len);
}

/* Read value until EA bit is set */
static bool gsm_mux_read_ea(int *value, u8_t recv_byte)
{
	/* As the value can be larger than one byte, collect the read
	 * bytes to given variable.
	 */
	*value <<= 7;
	*value |= recv_byte >> 1;

	/* When the address has been read fully, the EA bit is 1 */
	return recv_byte & EA;
}

static bool gsm_mux_read_address(struct gsm_mux *mux, u8_t recv_byte)
{
	int value = mux->address;
	bool ret;

	ret = gsm_mux_read_ea(&value, recv_byte);

	mux->address = value;

	return ret;
}

static bool gsm_mux_read_msg_len(struct gsm_mux *mux, u8_t recv_byte)
{
	int value = mux->msg_len;
	bool ret;

	ret = gsm_mux_read_ea(&value, recv_byte);

	mux->msg_len = value;

	return ret;
}

static struct net_buf *gsm_mux_alloc_buf(s32_t timeout, void *user_data)
{
	struct net_buf *buf;

	ARG_UNUSED(user_data);

	buf = net_buf_alloc(&gsm_mux_pool, timeout);
	if (!buf) {
		LOG_ERR("Cannot allocate buffer");
	}

	return buf;
}

static void hexdump_buf(const char *header, struct net_buf *buf)
{
	while (buf) {
		LOG_HEXDUMP_DBG(buf->data, buf->len, log_strdup(header));
		buf = buf->frags;
	}
}

static void gsm_dlci_process_data(struct gsm_dlci *dlci, bool cmd,
				  struct net_buf *buf)
{
	hexdump_buf("DLCI data", buf);

	// implementation missing
}

static struct gsm_dlci *gsm_dlci_get(u8_t dlci_address)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(dlcis); i++) {
		if (dlcis[i].in_use && dlcis[i].num != dlci_address) {
			return &dlcis[i];
		}
	}

	return NULL;
}

static void gsm_mux_control_message(struct gsm_mux *mux, struct net_buf *buf)
{
	u8_t *ptr = buf->data;
	struct gsm_dlci *dlci;
	u32_t command;
	u32_t len;

	while (gsm_mux_read_ea((int *)&command, *ptr++));

	if (ptr > (buf->data + buf->len)) {
		/* invalid packet */
		LOG_DBG("Invalid command");
		return;
	}

	while (gsm_mux_read_ea((int *)&len, *ptr++));

	if (ptr > (buf->data + buf->len)) {
		/* invalid packet */
		LOG_DBG("Invalid length");
		return;
	}

	switch (command) {
	case CMD_CLD:
		/* Modem closing down */
		dlci = gsm_dlci_get(0 | PF);
		if (dlci) {
			mux->refuse_service = true;
			dlci->refuse_service = true;
			gsm_dlci_begin_close(dlci);
		}

		break;

	case CMD_TEST:
		/* Send test message back */
		gsm_mux_control_reply(mux, CMD_TEST, buf);
		break;

	case CMD_FCON:
		/* Accepting data */
		gsm_mux_control_reply(mux, CMD_FCON, NULL);
		break;

	case CMD_FCOFF:
		/* Do not accept data */
		gsm_mux_control_reply(mux, CMD_FCOFF, NULL);
		break;

	case CMD_MSC:
		/* Modem status information */
		break;

	case CMD_RLS:
		/* Out of band error reception for a DLCI */
		break;

	case CMD_PSC:
		/* Modem wants to enter power saving state */
		gsm_mux_control_reply(mux, CMD_PSC, NULL);
		break;

	/* Optional unsupported commands */
	case CMD_PN:	/* Parameter negotiation */
	case CMD_RPN:	/* Remote port negotiation */
	case CMD_SNC:	/* Service negotiation command */
	default:
		/* Reply to bad commands with an NSC */
		buf->data[0] = command;
		buf->len = 1;
		gsm_mux_control_reply(mux, CMD_NSC, buf);
		break;
	}
}

static void gsm_dlci_process_command(struct gsm_dlci *dlci, bool cmd,
				     struct net_buf *buf)
{
	hexdump_buf("DLCI control", buf);

	if (cmd) {
		gsm_mux_control_message(dlci->mux, buf);
	} else {
		gsm_mux_control_response(dlci->mux, buf);
	}
}

static struct gsm_dlci *gsm_dlci_alloc(struct gsm_mux *mux, u8_t num)
{
	struct gsm_dlci *dlci = NULL;
	int i;

	for (i = 0; i < ARRAY_SIZE(dlcis); i++) {
		if (dlcis[i].in_use) {
			continue;
		}

		dlci = &dlcis[i];

		memset(dlci, 0, sizeof(*dlci));

		dlci->in_use = true;
		dlci->mux = mux;
		dlci->state = GSM_DLCI_CLOSED;
		dlci->num = num;

		/* Command channel (0) handling is separated from data */
		if (dlci->num) {
			dlci->process = gsm_dlci_process_data;
		} else {
			dlci->process = gsm_dlci_process_command;
		}

		break;
	}

	return dlci;
}

static void gsm_dlci_open(struct gsm_dlci *dlci)
{
	LOG_DBG("[%p/%d] open", dlci, dlci->num);
	dlci->state = GSM_DLCI_OPEN;
}

static void gsm_dlci_close(struct gsm_dlci *dlci)
{
	LOG_DBG("[%p/%d] close", dlci, dlci->num);
	dlci->state = GSM_DLCI_CLOSED;

	if (dlci->num == 0) {
		dlci->mux->refuse_service = true;
	}
}

static int gsm_mux_modem_send(struct gsm_mux *mux, u8_t *buf, size_t len)
{
	return mux->modem_write_func(mux->iface, buf, len);
}

static void gsm_mux_send_msg(struct gsm_mux *mux, bool cmd,
			     u8_t dlci_address, u8_t frame_type)
{
	u8_t buf[6];

	buf[0] = SOF_MARKER;
	buf[1] = (dlci_address << 2) | ((u8_t)cmd << 1) | EA;
	buf[2] = frame_type;
	buf[3] = EA;
	buf[4] = 0xFF - gsm_mux_fcs_add_buf(FCS_INIT_VALUE, buf + 1, 3);
	buf[5] = SOF_MARKER;

	hexdump_packet("Sending", dlci_address, cmd, frame_type,
		       buf, sizeof(buf));

	(void)gsm_mux_modem_send(mux, buf, sizeof(buf));
}

static void gsm_mux_send_command(struct gsm_mux *mux, u8_t dlci_address,
				 u8_t frame_type)
{
	gsm_mux_send_msg(mux, true, dlci_address, frame_type);
}

static void gsm_mux_send_response(struct gsm_mux *mux, u8_t dlci_address,
				  u8_t frame_type)
{
	gsm_mux_send_msg(mux, false, dlci_address, frame_type);
}

static void gsm_mux_process_pkt(struct gsm_mux *mux)
{
	u8_t dlci_address = mux->address >> 1; /* This contains C/R bit but
						* EA bit removed.
						*/
	struct gsm_dlci *dlci;
	bool cmd;   /* C/R bit, command (true) / response (false) */

	if (dlci_address >= ARRAY_SIZE(dlcis)) {
		goto fail;
	}

	cmd = !!((dlci_address << 1) & CR);

	hexdump_packet("Received:", dlci_address, cmd, mux->control,
		       mux->buf->data, mux->buf->len);

	dlci = gsm_dlci_get(dlci_address);

	/* Tweak the cmd from our point of view */
	if (mux->is_initiator) {
		cmd = cmd ? true : false;
	} else {
		cmd = cmd ? false : true;
	}

	/* What to do next */
	switch (mux->control) {
	case FT_SABM | PF:
		if (!cmd) {
			goto fail;
		}

		if (dlci == NULL) {
			dlci = gsm_dlci_alloc(mux, dlci_address);
			if (dlci == NULL) {
				goto fail;
			}
		}

		if (dlci->refuse_service) {
			gsm_mux_send_response(mux, dlci_address, FT_DM);
		} else {
			gsm_mux_send_response(mux, dlci_address, FT_UA);
			gsm_dlci_open(dlci);
		}

		break;

	case FT_DISC | PF:
		if (!cmd) {
			goto fail;
		}

		if (dlci == NULL || dlci->state == GSM_DLCI_CLOSED) {
			gsm_mux_send_response(mux, dlci_address, FT_DM);
			goto out;
		}

		gsm_mux_send_command(mux, dlci_address, FT_UA);
		gsm_dlci_close(dlci);
		break;

	case FT_UA | PF:
	case FT_UA:
		if (!cmd || dlci == NULL) {
			goto out;
		}

		switch (dlci->state) {
		case GSM_DLCI_CLOSING:
			gsm_dlci_close(dlci);
			break;
		case GSM_DLCI_OPENING:
			gsm_dlci_open(dlci);
			break;
		default:
			break;
		}

		break;

	case FT_DM | PF:
	case FT_DM:
		if (cmd) {
			goto fail;
		}

		if (dlci == NULL) {
			goto out;
		}

		gsm_dlci_close(dlci);
		break;

	case FT_UI | PF:
	case FT_UI:
	case FT_UIH | PF:
	case FT_UIH:
		if (dlci == NULL || dlci->state != GSM_DLCI_OPEN) {
			gsm_mux_send_command(mux, dlci_address, FT_DM | PF);
			goto out;
		}

		dlci->process(dlci, cmd, mux->buf);
		net_buf_unref(mux->buf);
		mux->buf = NULL;
		break;

	default:
		goto fail;
	}

out:
	return;

fail:
	LOG_ERR("Cannot handle command (0x%02x)", mux->control);
	return;
}

static bool is_UI(struct gsm_mux *mux)
{
	return (mux->control & ~PF) == FT_UI;
}

static bool gsm_mux_receive_data(struct gsm_mux *mux, u8_t recv_byte)
{
	size_t added;

	switch (mux->state) {
	case GSM_MUX_SOF:
		/* This is the initial state where we look for SOF char */
		if (recv_byte == SOF_MARKER) {
			mux->state = GSM_MUX_ADDRESS;
			mux->fcs = FCS_INIT_VALUE;

			/* We free all the allocated buffers at start */
			if (mux->buf) {
				net_buf_unref(mux->buf);
				mux->buf = NULL;
			}
		}

		break;

	case GSM_MUX_ADDRESS:
		/* DLCI (Data Link Connection Identifier) address we want to
		 * talk. This address field also contains C/R bit.
		 */
		if (gsm_mux_read_address(mux, recv_byte)) {
			mux->state = GSM_MUX_CONTROL;
		}

		mux->fcs = gsm_mux_fcs_add(mux->fcs, recv_byte);
		break;

	case GSM_MUX_CONTROL:
		mux->control = recv_byte;
		mux->state = GSM_MUX_LEN_0;
		mux->fcs = gsm_mux_fcs_add(mux->fcs, recv_byte);
		break;

	case GSM_MUX_LEN_0:
		mux->fcs = gsm_mux_fcs_add(mux->fcs, recv_byte);

		if (gsm_mux_read_msg_len(mux, recv_byte)) {
			if (mux->msg_len > mux->mru) {
				mux->state = GSM_MUX_SOF;
			} else if (mux->msg_len == 0) {
				mux->state = GSM_MUX_FCS;
			} else {
				mux->state = GSM_MUX_DATA;
			}
		} else {
			mux->state = GSM_MUX_LEN_1;
		}

		break;

	case GSM_MUX_LEN_1:
		mux->fcs = gsm_mux_fcs_add(mux->fcs, recv_byte);

		mux->msg_len |= recv_byte << 7;
		if (mux->msg_len > mux->mru) {
			mux->state = GSM_MUX_SOF;
		} else if (mux->msg_len == 0) {
			mux->state = GSM_MUX_FCS;
		} else {
			mux->state = GSM_MUX_DATA;
		}

		break;

	case GSM_MUX_DATA:
		added = net_buf_append_bytes(mux->buf, 1, (void *)&recv_byte,
					     BUF_ALLOC_TIMEOUT,
					     gsm_mux_alloc_buf, NULL);
		if (added != 1) {
			mux->state = GSM_MUX_SOF;
		} else if (++mux->received == mux->msg_len) {
			mux->state = GSM_MUX_FCS;
		}

		break;

	case GSM_MUX_FCS:
		mux->received_fcs = recv_byte;

		/* Update the FCS for Unnumbered Information field (UI) */
		if (is_UI(mux)) {
			mux->fcs = gsm_mux_fcs_add_buf(mux->fcs,
						       mux->buf->data,
						       mux->buf->len);
		}

		mux->fcs = gsm_mux_fcs_add(mux->fcs, mux->received_fcs);
		if (mux->fcs == FCS_GOOD_VALUE) {
			gsm_mux_process_pkt(mux);
		}

		mux->state = GSM_MUX_EOF;
		break;

	case GSM_MUX_EOF:
		if (recv_byte == GSM_MUX_SOF) {
			mux->state = GSM_MUX_SOF;
		}

		break;
	}

	return true;
}

u8_t *gsm_mux_recv(struct gsm_mux *mux, u8_t *buf, size_t *off)
{
	int i, len = *off;

	for (i = 0; i < *off; i++) {
		if (!gsm_mux_receive_data(mux, buf[i])) {
			// removing muxing and give to proper upper layer
			break;
		}
	}

	if (i == *off) {
		*off = 0;
	} else {
		*off = len - i - 1;

		memmove(&buf[0], &buf[i + 1], *off);
	}

	return buf;
}

struct gsm_mux *gsm_mux_alloc(struct modem_iface *iface,
			      uart_pipe_recv_cb cb,
			      int (*write)(struct modem_iface *iface,
					   const u8_t *buf,
					   size_t size))
{
	struct gsm_mux *mux = NULL;
	int i;

	for (i = 0; i < ARRAY_SIZE(muxes); i++) {
		if (muxes[i].in_use) {
			continue;
		}

		mux = &muxes[i];

		memset(mux, 0, sizeof(*mux));

		mux->in_use = true;
		mux->iface = iface;
		mux->ppp_recv_cb = cb;
		mux->modem_write_func = write;
		mux->mru = 31; /* Basic mode default */

		break;
	}

	return mux;
}

int gsm_mux_send(struct gsm_mux *mux, const u8_t *buf, size_t size)
{
	// mux the data to be sent to modem
}

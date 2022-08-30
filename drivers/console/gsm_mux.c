/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gsm_mux, CONFIG_GSM_MUX_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/crc.h>
#include <zephyr/net/buf.h>
#include <zephyr/net/ppp.h>

#include "uart_mux_internal.h"
#include "gsm_mux.h"

/* Default values are from the specification 07.10 */
#define T1_MSEC 100  /* 100 ms */
#define T2_MSEC 340  /* 333 ms */

#define N1 256 /* default I frame size, GSM 07.10 ch 6.2.2.1 */
#define N2 3   /* retry 3 times */

/* CRC8 is the reflected CRC8/ROHC algorithm */
#define FCS_POLYNOMIAL 0xe0 /* reversed crc8 */
#define FCS_INIT_VALUE 0xFF
#define FCS_GOOD_VALUE 0xCF

#define GSM_EA 0x01  /* Extension bit      */
#define GSM_CR 0x02  /* Command / Response */
#define GSM_PF 0x10  /* Poll / Final       */

/* Frame types */
#define FT_RR      0x01  /* Receive Ready                            */
#define FT_UI      0x03  /* Unnumbered Information                   */
#define FT_RNR     0x05  /* Receive Not Ready                        */
#define FT_REJ     0x09  /* Reject                                   */
#define FT_DM      0x0F  /* Disconnected Mode                        */
#define FT_SABM    0x2F  /* Set Asynchronous Balanced Mode           */
#define FT_DISC    0x43  /* Disconnect                               */
#define FT_UA      0x63  /* Unnumbered Acknowledgement               */
#define FT_UIH     0xEF  /* Unnumbered Information with Header check */

/* Control channel commands */
#define CMD_NSC    0x08  /* Non Supported Command Response           */
#define CMD_TEST   0x10  /* Test Command                             */
#define CMD_PSC    0x20  /* Power Saving Control                     */
#define CMD_RLS    0x28  /* Remote Line Status Command               */
#define CMD_FCOFF  0x30  /* Flow Control Off Command                 */
#define CMD_PN     0x40  /* DLC parameter negotiation                */
#define CMD_RPN    0x48  /* Remote Port Negotiation Command          */
#define CMD_FCON   0x50  /* Flow Control On Command                  */
#define CMD_CLD    0x60  /* Multiplexer close down                   */
#define CMD_SNC    0x68  /* Service Negotiation Command              */
#define CMD_MSC    0x70  /* Modem Status Command                     */

/* Flag sequence field between messages (start of frame) */
#define SOF_MARKER 0xF9

/* Mux parsing states */
enum gsm_mux_state {
	GSM_MUX_SOF,      /* Start of frame       */
	GSM_MUX_ADDRESS,  /* Address field        */
	GSM_MUX_CONTROL,  /* Control field        */
	GSM_MUX_LEN_0,    /* First length byte    */
	GSM_MUX_LEN_1,    /* Second length byte   */
	GSM_MUX_DATA,     /* Data                 */
	GSM_MUX_FCS,      /* Frame Check Sequence */
	GSM_MUX_EOF       /* End of frame         */
};

struct gsm_mux {
	/* UART device to use. This device is the real UART, not the
	 * muxed one.
	 */
	const struct device *uart;

	/* Buf to use when TX mux packet (hdr + data). For RX it only contains
	 * the data (not hdr).
	 */
	struct net_buf *buf;
	int mru;

	enum gsm_mux_state state;

	/* Control DLCI is not included in this list so -1 here */
	uint8_t dlci_to_create[CONFIG_GSM_MUX_DLCI_MAX - 1];

	uint16_t msg_len;     /* message length */
	uint16_t received;    /* bytes so far received */

	struct k_work_delayable t2_timer;
	sys_slist_t pending_ctrls;

	uint16_t t1_timeout_value; /* T1 default value */
	uint16_t t2_timeout_value; /* T2 default value */

	/* Information from currently read packet */
	uint8_t address;      /* dlci address (only one byte address supported) */
	uint8_t control;      /* type of the frame */
	uint8_t fcs;          /* calculated frame check sequence */
	uint8_t received_fcs; /* packet fcs */
	uint8_t retries;      /* N2 counter */

	bool in_use : 1;
	bool is_initiator : 1;   /* Did we initiate the connection attempt */
	bool refuse_service : 1; /* Do not try to talk to this modem */
};

/* DLCI states */
enum gsm_dlci_state {
	GSM_DLCI_CLOSED,
	GSM_DLCI_OPENING,
	GSM_DLCI_OPEN,
	GSM_DLCI_CLOSING
};

enum gsm_dlci_mode {
	GSM_DLCI_MODE_ABM = 0,  /* Normal Asynchronous Balanced Mode */
	GSM_DLCI_MODE_ADM = 1,  /* Asynchronous Disconnected Mode */
};

typedef int (*dlci_process_msg_t)(struct gsm_dlci *dlci, bool cmd,
				  struct net_buf *buf);
typedef void (*dlci_command_cb_t)(struct gsm_dlci *dlci, bool connected);

struct gsm_dlci {
	sys_snode_t node;
	struct k_sem disconnect_sem;
	struct gsm_mux *mux;
	dlci_process_msg_t handler;
	dlci_command_cb_t command_cb;
	gsm_mux_dlci_created_cb_t dlci_created_cb;
	void *user_data;
	const struct device *uart;
	enum gsm_dlci_state state;
	enum gsm_dlci_mode mode;
	int num;
	uint32_t req_start;
	uint8_t retries;
	bool refuse_service : 1; /* Do not try to talk to this channel */
	bool in_use : 1;
};

struct gsm_control_msg {
	sys_snode_t node;
	struct net_buf *buf;
	uint32_t req_start;
	uint8_t cmd;
	bool finished : 1;
};

/* From 07.10, Maximum Frame Size [1 - 128] in Basic mode */
#define MAX_MRU CONFIG_GSM_MUX_MRU_MAX_LEN

/* Assume that there are 3 network buffers (one for RX and one for TX, and one
 * extra when parsing data) going on at the same time.
 */
#define MIN_BUF_COUNT (CONFIG_GSM_MUX_MAX * 3)

NET_BUF_POOL_DEFINE(gsm_mux_pool, MIN_BUF_COUNT, MAX_MRU, 0, NULL);

#define BUF_ALLOC_TIMEOUT K_MSEC(50)

static struct gsm_mux muxes[CONFIG_GSM_MUX_MAX];

static struct gsm_dlci dlcis[CONFIG_GSM_MUX_DLCI_MAX];
static sys_slist_t dlci_free_entries;
static sys_slist_t dlci_active_t1_timers;
static struct k_work_delayable t1_timer;

static struct gsm_control_msg ctrls[CONFIG_GSM_MUX_PENDING_CMD_MAX];
static sys_slist_t ctrls_free_entries;

static bool gsm_mux_init_done;

static const char *get_frame_type_str(uint8_t frame_type)
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

static void hexdump_packet(const char *header, uint8_t address, bool cmd_rsp,
			   uint8_t control, const uint8_t *data, size_t len)
{
	const char *frame_type;
	char out[128];
	int ret;

	if (!IS_ENABLED(CONFIG_GSM_MUX_LOG_LEVEL_DBG)) {
		return;
	}

	memset(out, 0, sizeof(out));

	ret = snprintk(out, sizeof(out), "%s: DLCI %d %s ",
		       header, address, cmd_rsp ? "cmd" : "resp");
	if (ret >= sizeof(out)) {
		LOG_DBG("%d: Too long msg (%ld)", __LINE__, (long)(ret - sizeof(out)));
		goto print;
	}

	frame_type = get_frame_type_str(control & ~GSM_PF);
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
		LOG_DBG("%d: Too long msg (%ld)", __LINE__, (long)(ret - sizeof(out)));
		goto print;
	}

	ret += snprintk(out + ret, sizeof(out) - ret, "%s", (control & GSM_PF) ? "(P)" : "(F)");
	if (ret >= sizeof(out)) {
		LOG_DBG("%d: Too long msg (%ld)", __LINE__, (long)(ret - sizeof(out)));
		goto print;
	}

print:
	if (IS_ENABLED(CONFIG_GSM_MUX_VERBOSE_DEBUG)) {
		if (len > 0) {
			LOG_HEXDUMP_DBG(data, len, out);
		} else {
			LOG_DBG("%s", out);
		}
	} else {
		LOG_DBG("%s", out);
	}
}

static uint8_t gsm_mux_fcs_add_buf(uint8_t fcs, const uint8_t *buf, size_t len)
{
	return crc8(buf, len, FCS_POLYNOMIAL, fcs, true);
}

static uint8_t gsm_mux_fcs_add(uint8_t fcs, uint8_t recv_byte)
{
	return gsm_mux_fcs_add_buf(fcs, &recv_byte, 1);
}

static bool gsm_mux_read_ea(int *value, uint8_t recv_byte)
{
	/* As the value can be larger than one byte, collect the read
	 * bytes to given variable.
	 */
	*value <<= 7;
	*value |= recv_byte >> 1;

	/* When the address has been read fully, the EA bit is 1 */
	return recv_byte & GSM_EA;
}

static bool gsm_mux_read_msg_len(struct gsm_mux *mux, uint8_t recv_byte)
{
	int value = mux->msg_len;
	bool ret;

	ret = gsm_mux_read_ea(&value, recv_byte);

	mux->msg_len = value;

	return ret;
}

static struct net_buf *gsm_mux_alloc_buf(k_timeout_t timeout, void *user_data)
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
	if (IS_ENABLED(CONFIG_GSM_MUX_VERBOSE_DEBUG)) {
		while (buf) {
			LOG_HEXDUMP_DBG(buf->data, buf->len, header);
			buf = buf->frags;
		}
	}
}

static int gsm_dlci_process_data(struct gsm_dlci *dlci, bool cmd,
				 struct net_buf *buf)
{
	int len = 0;

	LOG_DBG("[%p] DLCI %d data %s", dlci->mux, dlci->num,
		cmd ? "request" : "response");
	hexdump_buf("buf", buf);

	while (buf) {
		uart_mux_recv(dlci->uart, dlci, buf->data, buf->len);
		len += buf->len;
		buf = buf->frags;
	}

	return len;
}

static struct gsm_dlci *gsm_dlci_get(struct gsm_mux *mux, uint8_t dlci_address)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(dlcis); i++) {
		if (dlcis[i].in_use &&
		    dlcis[i].mux == mux &&
		    dlcis[i].num == dlci_address) {
			return &dlcis[i];
		}
	}

	return NULL;
}

static int gsm_mux_modem_send(struct gsm_mux *mux, const uint8_t *buf, size_t size)
{
	if (mux->uart == NULL) {
		return -ENOENT;
	}

	if (size == 0) {
		return 0;
	}

	return uart_mux_send(mux->uart, buf, size);
}

static int gsm_mux_send_data_msg(struct gsm_mux *mux, bool cmd,
				 struct gsm_dlci *dlci, uint8_t frame_type,
				 const uint8_t *buf, size_t size)
{
	uint8_t hdr[7];
	int pos;
	int ret;

	hdr[0] = SOF_MARKER;
	hdr[1] = (dlci->num << 2) | ((uint8_t)cmd << 1) | GSM_EA;
	hdr[2] = frame_type;

	if (size < 128) {
		hdr[3] = (size << 1) | GSM_EA;
		pos = 4;
	} else {
		hdr[3] = (size & 127) << 1;
		hdr[4] = (size >> 7);
		pos = 5;
	}

	/* Write the header and data in smaller chunks in order to avoid
	 * allocating a big buffer.
	 */
	(void)gsm_mux_modem_send(mux, &hdr[0], pos);

	if (size > 0) {
		(void)gsm_mux_modem_send(mux, buf, size);
	}

	/* FSC is calculated only for address, type and length fields
	 * for UIH frames
	 */
	hdr[pos] = 0xFF - gsm_mux_fcs_add_buf(FCS_INIT_VALUE, &hdr[1],
					      pos - 1);
	if ((frame_type & ~GSM_PF) != FT_UIH) {
		hdr[pos] = gsm_mux_fcs_add_buf(hdr[pos], buf, size);
	}

	hdr[pos + 1] = SOF_MARKER;

	ret = gsm_mux_modem_send(mux, &hdr[pos], 2);

	hexdump_packet("Sending", dlci->num, cmd, frame_type,
		       buf, size);
	return ret;
}

static int gsm_mux_send_control_msg(struct gsm_mux *mux, bool cmd,
				    uint8_t dlci_address, uint8_t frame_type)
{
	uint8_t buf[6];

	buf[0] = SOF_MARKER;
	buf[1] = (dlci_address << 2) | ((uint8_t)cmd << 1) | GSM_EA;
	buf[2] = frame_type;
	buf[3] = GSM_EA;
	buf[4] = 0xFF - gsm_mux_fcs_add_buf(FCS_INIT_VALUE, buf + 1, 3);
	buf[5] = SOF_MARKER;

	hexdump_packet("Sending", dlci_address, cmd, frame_type,
		       buf, sizeof(buf));

	return gsm_mux_modem_send(mux, buf, sizeof(buf));
}

static int gsm_mux_send_command(struct gsm_mux *mux, uint8_t dlci_address,
				uint8_t frame_type)
{
	return gsm_mux_send_control_msg(mux, true, dlci_address, frame_type);
}

static int gsm_mux_send_response(struct gsm_mux *mux, uint8_t dlci_address,
				 uint8_t frame_type)
{
	return gsm_mux_send_control_msg(mux, false, dlci_address, frame_type);
}

static void dlci_run_timer(uint32_t current_time)
{
	struct gsm_dlci *dlci, *next;
	uint32_t new_timer = UINT_MAX;

	(void)k_work_cancel_delayable(&t1_timer);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&dlci_active_t1_timers,
					  dlci, next, node) {
		uint32_t current_timer = dlci->req_start +
			dlci->mux->t1_timeout_value - current_time;

		new_timer = MIN(current_timer, new_timer);
	}

	if (new_timer != UINT_MAX) {
		k_work_reschedule(&t1_timer, K_MSEC(new_timer));
	}
}

static void gsm_dlci_open(struct gsm_dlci *dlci)
{
	LOG_DBG("[%p/%d] DLCI id %d open", dlci, dlci->num, dlci->num);
	dlci->state = GSM_DLCI_OPEN;

	/* Remove this DLCI from pending T1 timers */
	sys_slist_remove(&dlci_active_t1_timers, NULL, &dlci->node);
	dlci_run_timer(k_uptime_get_32());

	if (dlci->command_cb) {
		dlci->command_cb(dlci, true);
	}
}

static void gsm_dlci_close(struct gsm_dlci *dlci)
{
	LOG_DBG("[%p/%d] DLCI id %d closed", dlci, dlci->num, dlci->num);
	dlci->state = GSM_DLCI_CLOSED;

	k_sem_give(&dlci->disconnect_sem);

	/* Remove this DLCI from pending T1 timers */
	sys_slist_remove(&dlci_active_t1_timers, NULL, &dlci->node);
	dlci_run_timer(k_uptime_get_32());

	if (dlci->command_cb) {
		dlci->command_cb(dlci, false);
	}

	if (dlci->num == 0) {
		dlci->mux->refuse_service = true;
	}
}

/* Return true if we need to retry, false otherwise */
static bool handle_t1_timeout(struct gsm_dlci *dlci)
{
	LOG_DBG("[%p/%d] T1 timeout", dlci, dlci->num);

	if (dlci->state == GSM_DLCI_OPENING) {
		dlci->retries--;
		if (dlci->retries) {
			dlci->req_start = k_uptime_get_32();
			(void)gsm_mux_send_command(dlci->mux, dlci->num, FT_SABM | GSM_PF);
			return true;
		}

		if (dlci->command_cb) {
			dlci->command_cb(dlci, false);
		}

		if (dlci->num == 0 && dlci->mux->control == (FT_DM | GSM_PF)) {
			LOG_DBG("DLCI %d -> ADM mode", dlci->num);
			dlci->mode = GSM_DLCI_MODE_ADM;
			gsm_dlci_open(dlci);
		} else {
			gsm_dlci_close(dlci);
		}
	} else if (dlci->state == GSM_DLCI_CLOSING) {
		dlci->retries--;
		if (dlci->retries) {
			(void)gsm_mux_send_command(dlci->mux, dlci->num, FT_DISC | GSM_PF);
			return true;
		}

		gsm_dlci_close(dlci);
	}

	return false;
}

static void dlci_t1_timeout(struct k_work *work)
{
	uint32_t current_time = k_uptime_get_32();
	struct gsm_dlci *entry, *next;
	sys_snode_t *prev_node = NULL;

	ARG_UNUSED(work);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&dlci_active_t1_timers,
					  entry, next, node) {
		if ((int32_t)(entry->req_start +
			    entry->mux->t1_timeout_value - current_time) > 0) {
			prev_node = &entry->node;
			break;
		}

		if (!handle_t1_timeout(entry)) {
			sys_slist_remove(&dlci_active_t1_timers, prev_node,
					 &entry->node);
		}
	}

	dlci_run_timer(current_time);
}

static struct gsm_control_msg *gsm_ctrl_msg_get_free(void)
{
	sys_snode_t *node;

	node = sys_slist_peek_head(&ctrls_free_entries);
	if (!node) {
		return NULL;
	}

	sys_slist_remove(&ctrls_free_entries, NULL, node);

	return CONTAINER_OF(node, struct gsm_control_msg, node);
}

static struct gsm_control_msg *gsm_mux_alloc_control_msg(struct net_buf *buf,
							 uint8_t cmd)
{
	struct gsm_control_msg *msg;

	msg = gsm_ctrl_msg_get_free();
	if (!msg) {
		return NULL;
	}

	msg->buf = buf;
	msg->cmd = cmd;

	return msg;
}

static void ctrl_msg_cleanup(struct gsm_control_msg *entry, bool pending)
{
	if (pending) {
		LOG_DBG("Releasing pending buf %p (ref %d)",
			entry->buf, entry->buf->ref - 1);
		net_buf_unref(entry->buf);
		entry->buf = NULL;
	}
}

/* T2 timeout is for control message retransmits */
static void gsm_mux_t2_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct gsm_mux *mux = CONTAINER_OF(dwork, struct gsm_mux, t2_timer);
	uint32_t current_time = k_uptime_get_32();
	struct gsm_control_msg *entry, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&mux->pending_ctrls, entry, next,
					  node) {
		if ((int32_t)(entry->req_start + T2_MSEC - current_time) > 0) {
			break;
		}

		ctrl_msg_cleanup(entry, true);

		sys_slist_remove(&mux->pending_ctrls, NULL, &entry->node);
		sys_slist_append(&ctrls_free_entries, &entry->node);

		entry = NULL;
	}

	if (entry) {
		k_work_reschedule(
			&mux->t2_timer,
			K_MSEC(entry->req_start + T2_MSEC - current_time));
	}
}

static int gsm_mux_send_control_message(struct gsm_mux *mux, uint8_t dlci_address,
					int cmd, uint8_t *data, size_t data_len)
{
	struct gsm_control_msg *ctrl;
	struct net_buf *buf;

	/* We create a net_buf for the control message so that we can
	 * resend it easily if needed.
	 */
	buf = gsm_mux_alloc_buf(BUF_ALLOC_TIMEOUT, NULL);
	if (!buf) {
		LOG_ERR("[%p] Cannot allocate header", mux);
		return -ENOMEM;
	}

	if (data && data_len > 0) {
		size_t added;

		added = net_buf_append_bytes(buf, data_len, data,
					     BUF_ALLOC_TIMEOUT,
					     gsm_mux_alloc_buf, NULL);
		if (added != data_len) {
			net_buf_unref(buf);
			return -ENOMEM;
		}
	}

	ctrl = gsm_mux_alloc_control_msg(buf, cmd);
	if (!ctrl) {
		net_buf_unref(buf);
		return -ENOMEM;
	}

	sys_slist_append(&mux->pending_ctrls, &ctrl->node);
	ctrl->req_start = k_uptime_get_32();

	/* Let's start the timer if necessary */
	if (!k_work_delayable_remaining_get(&mux->t2_timer)) {
		k_work_reschedule(&mux->t2_timer, K_MSEC(T2_MSEC));
	}

	return gsm_mux_modem_send(mux, buf->data, buf->len);
}

static int gsm_dlci_opening_or_closing(struct gsm_dlci *dlci,
				       enum gsm_dlci_state state,
				       int command,
				       dlci_command_cb_t cb)
{
	dlci->retries = dlci->mux->retries;
	dlci->req_start = k_uptime_get_32();
	dlci->state = state;
	dlci->command_cb = cb;

	/* Let's start the timer if necessary */
	if (!k_work_delayable_remaining_get(&t1_timer)) {
		k_work_reschedule(&t1_timer,
				  K_MSEC(dlci->mux->t1_timeout_value));
	}

	sys_slist_append(&dlci_active_t1_timers, &dlci->node);

	return gsm_mux_send_command(dlci->mux, dlci->num, command | GSM_PF);
}

static int gsm_dlci_closing(struct gsm_dlci *dlci, dlci_command_cb_t cb)
{
	if (dlci->state == GSM_DLCI_CLOSED ||
	    dlci->state == GSM_DLCI_CLOSING) {
		return -EALREADY;
	}

	LOG_DBG("[%p] DLCI %d closing", dlci, dlci->num);

	return gsm_dlci_opening_or_closing(dlci, GSM_DLCI_CLOSING, FT_DISC,
					   cb);
}

static int gsm_dlci_opening(struct gsm_dlci *dlci, dlci_command_cb_t cb)
{
	if (dlci->state == GSM_DLCI_OPEN || dlci->state == GSM_DLCI_OPENING) {
		return -EALREADY;
	}

	LOG_DBG("[%p] DLCI %d opening", dlci, dlci->num);

	return gsm_dlci_opening_or_closing(dlci, GSM_DLCI_OPENING, FT_SABM,
					   cb);
}

int gsm_mux_disconnect(struct gsm_mux *mux, k_timeout_t timeout)
{
	struct gsm_dlci *dlci;

	dlci = gsm_dlci_get(mux, 0);
	if (dlci == NULL) {
		return -ENOENT;
	}

	(void)gsm_mux_send_control_message(dlci->mux, dlci->num,
					   CMD_CLD, NULL, 0);

	(void)k_work_cancel_delayable(&mux->t2_timer);

	(void)gsm_dlci_closing(dlci, NULL);

	return k_sem_take(&dlci->disconnect_sem, timeout);
}

static int gsm_mux_control_reply(struct gsm_dlci *dlci, bool sub_cr,
				 uint8_t sub_cmd, const uint8_t *buf, size_t len)
{
	/* As this is a reply to received command, set the value according
	 * to initiator status. See GSM 07.10 page 17.
	 */
	bool cmd = !dlci->mux->is_initiator;

	return gsm_mux_send_data_msg(dlci->mux, cmd, dlci, FT_UIH | GSM_PF, buf, len);
}

static bool get_field(struct net_buf *buf, int *ret_value)
{
	int value = 0;
	uint8_t recv_byte;

	while (buf->len) {
		recv_byte = net_buf_pull_u8(buf);

		if (gsm_mux_read_ea(&value, recv_byte)) {
			*ret_value = value;
			return true;
		}

		if (buf->len == 0) {
			buf = net_buf_frag_del(NULL, buf);
			if (buf == NULL) {
				break;
			}
		}
	}

	return false;
}

static int gsm_mux_msc_reply(struct gsm_dlci *dlci, bool cmd,
			     struct net_buf *buf, size_t len)
{
	uint32_t modem_sig = 0, break_sig = 0;
	int ret;

	ret = get_field(buf, &modem_sig);
	if (!ret) {
		LOG_DBG("[%p] Malformed data", dlci->mux);
		return -EINVAL;
	}

	if (buf->len > 0) {
		ret = get_field(buf, &break_sig);
		if (!ret) {
			LOG_DBG("[%p] Malformed data", dlci->mux);
			return -EINVAL;
		}
	}

	LOG_DBG("Modem signal 0x%02x break signal 0x%02x", modem_sig,
		break_sig);

	/* FIXME to return proper status back */

	return gsm_mux_control_reply(dlci, cmd, CMD_MSC, buf->data, len);
}

static int gsm_mux_control_message(struct gsm_dlci *dlci, struct net_buf *buf)
{
	uint32_t command = 0, len = 0;
	int ret = 0;
	bool cr;

	__ASSERT_NO_MSG(dlci != NULL);

	/* Remove the C/R bit from sub-command */
	cr = buf->data[0] & GSM_CR;
	buf->data[0] &= ~GSM_CR;

	ret = get_field(buf, &command);
	if (!ret) {
		LOG_DBG("[%p] Malformed data", dlci->mux);
		return -EINVAL;
	}

	ret = get_field(buf, &len);
	if (!ret) {
		LOG_DBG("[%p] Malformed data", dlci->mux);
		return -EINVAL;
	}

	LOG_DBG("[%p] DLCI %d %s 0x%02x len %u", dlci->mux, dlci->num,
		cr ? "cmd" : "rsp", command, len);

	/* buf->data should now point to start of dlci command data */

	switch (command) {
	case CMD_CLD:
		/* Modem closing down */
		dlci->mux->refuse_service = true;
		dlci->refuse_service = true;
		gsm_dlci_closing(dlci, NULL);
		break;

	case CMD_FCOFF:
		/* Do not accept data */
		ret = gsm_mux_control_reply(dlci, cr, CMD_FCOFF, NULL, 0);
		break;

	case CMD_FCON:
		/* Accepting data */
		ret = gsm_mux_control_reply(dlci, cr, CMD_FCON, NULL, 0);
		break;

	case CMD_MSC:
		/* Modem status information */
		/* FIXME: WIP: MSC reply does not work */
		if (0) {
			ret = gsm_mux_msc_reply(dlci, cr, buf, len);
		}

		break;

	case CMD_PSC:
		/* Modem wants to enter power saving state */
		ret = gsm_mux_control_reply(dlci, cr, CMD_PSC, NULL, len);
		break;

	case CMD_RLS:
		/* Out of band error reception for a DLCI */
		break;

	case CMD_TEST:
		/* Send test message back */
		ret = gsm_mux_control_reply(dlci, cr, CMD_TEST,
					    buf->data, len);
		break;

	/* Optional and currently unsupported commands */
	case CMD_PN:	/* Parameter negotiation */
	case CMD_RPN:	/* Remote port negotiation */
	case CMD_SNC:	/* Service negotiation command */
	default:
		/* Reply to bad commands with an NSC */
		buf->data[0] = command | (cr ? GSM_CR : 0);
		buf->len = 1;
		ret = gsm_mux_control_reply(dlci, cr, CMD_NSC, buf->data, len);
		break;
	}

	return ret;
}

/* Handle a response to our control message */
static int gsm_mux_control_response(struct gsm_dlci *dlci, struct net_buf *buf)
{
	struct gsm_control_msg *entry, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&dlci->mux->pending_ctrls,
					  entry, next, node) {
		if (dlci->mux->control == entry->cmd) {
			sys_slist_remove(&dlci->mux->pending_ctrls, NULL,
					 &entry->node);
			sys_slist_append(&ctrls_free_entries, &entry->node);
			entry->finished = true;

			if (dlci->command_cb) {
				dlci->command_cb(dlci, true);
			}

			break;
		}
	}

	return 0;
}

static int gsm_dlci_process_command(struct gsm_dlci *dlci, bool cmd,
				    struct net_buf *buf)
{
	int ret;

	LOG_DBG("[%p] DLCI %d control %s", dlci->mux, dlci->num,
		cmd ? "request" : "response");
	hexdump_buf("buf", buf);

	if (cmd) {
		ret = gsm_mux_control_message(dlci, buf);
	} else {
		ret = gsm_mux_control_response(dlci, buf);
	}

	return ret;
}

static void gsm_dlci_free(struct gsm_mux *mux, uint8_t address)
{
	struct gsm_dlci *dlci;
	int i;

	for (i = 0; i < ARRAY_SIZE(dlcis); i++) {
		if (!dlcis[i].in_use) {
			continue;
		}

		dlci = &dlcis[i];

		if (dlci->mux == mux && dlci->num == address) {
			dlci->in_use = false;

			sys_slist_prepend(&dlci_free_entries, &dlci->node);
		}

		break;
	}
}

static struct gsm_dlci *gsm_dlci_get_free(void)
{
	sys_snode_t *node;

	node = sys_slist_peek_head(&dlci_free_entries);
	if (!node) {
		return NULL;
	}

	sys_slist_remove(&dlci_free_entries, NULL, node);

	return CONTAINER_OF(node, struct gsm_dlci, node);
}

static struct gsm_dlci *gsm_dlci_alloc(struct gsm_mux *mux, uint8_t address,
		const struct device *uart,
		gsm_mux_dlci_created_cb_t dlci_created_cb,
		void *user_data)
{
	struct gsm_dlci *dlci;

	dlci = gsm_dlci_get_free();
	if (!dlci) {
		return NULL;
	}

	k_sem_init(&dlci->disconnect_sem, 1, 1);

	dlci->mux = mux;
	dlci->num = address;
	dlci->in_use = true;
	dlci->retries = mux->retries;
	dlci->state = GSM_DLCI_CLOSED;
	dlci->uart = uart;
	dlci->user_data = user_data;
	dlci->dlci_created_cb = dlci_created_cb;

	/* Command channel (0) handling is separated from data */
	if (dlci->num) {
		dlci->handler = gsm_dlci_process_data;
	} else {
		dlci->handler = gsm_dlci_process_command;
	}

	return dlci;
}

static int gsm_mux_process_pkt(struct gsm_mux *mux)
{
	uint8_t dlci_address = mux->address >> 2;
	int ret = 0;
	bool cmd;   /* C/R bit, command (true) / response (false) */
	struct gsm_dlci *dlci;

	/* This function is only called for received packets so if the
	 * command is set, then it means a response if we are initiator.
	 */
	cmd = (mux->address >> 1) & 0x01;

	if (mux->is_initiator) {
		cmd = !cmd;
	}

	hexdump_packet("Received", dlci_address, cmd, mux->control,
		       mux->buf ? mux->buf->data : NULL,
		       mux->buf ? mux->buf->len : 0);

	dlci = gsm_dlci_get(mux, dlci_address);

	/* What to do next */
	switch (mux->control) {
	case FT_SABM | GSM_PF:
		if (cmd == false) {
			ret = -ENOENT;
			goto fail;
		}

		if (dlci == NULL) {
			const struct device *uart;

			uart = uart_mux_find(dlci_address);
			if (uart == NULL) {
				ret = -ENOENT;
				goto fail;
			}

			dlci = gsm_dlci_alloc(mux, dlci_address, uart, NULL,
					      NULL);
			if (dlci == NULL) {
				ret = -ENOENT;
				goto fail;
			}
		}

		if (dlci->refuse_service) {
			ret = gsm_mux_send_response(mux, dlci_address, FT_DM);
		} else {
			ret = gsm_mux_send_response(mux, dlci_address, FT_UA);
			gsm_dlci_open(dlci);
		}

		break;

	case FT_DISC | GSM_PF:
		if (cmd == false) {
			ret = -ENOENT;
			goto fail;
		}

		if (dlci == NULL || dlci->state == GSM_DLCI_CLOSED) {
			(void)gsm_mux_send_response(mux, dlci_address, FT_DM);
			ret = -ENOENT;
			goto out;
		}

		ret = gsm_mux_send_command(mux, dlci_address, FT_UA);
		gsm_dlci_close(dlci);
		break;

	case FT_UA | GSM_PF:
	case FT_UA:
		if (cmd == true || dlci == NULL) {
			ret = -ENOENT;
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

	case FT_DM | GSM_PF:
	case FT_DM:
		if (cmd == true || dlci == NULL) {
			ret = -ENOENT;
			goto fail;
		}

		gsm_dlci_close(dlci);
		break;

	case FT_UI | GSM_PF:
	case FT_UI:
	case FT_UIH | GSM_PF:
	case FT_UIH:
		if (dlci == NULL || dlci->state != GSM_DLCI_OPEN) {
			(void)gsm_mux_send_command(mux, dlci_address, FT_DM | GSM_PF);
			ret = -ENOENT;
			goto out;
		}

		ret = dlci->handler(dlci, cmd, mux->buf);

		if (mux->buf) {
			net_buf_unref(mux->buf);
			mux->buf = NULL;
		}

		break;

	default:
		ret = -EINVAL;
		goto fail;
	}

out:
	return ret;

fail:
	LOG_ERR("Cannot handle command (0x%02x) (%d)", mux->control, ret);
	return ret;
}

static bool is_UI(struct gsm_mux *mux)
{
	return (mux->control & ~GSM_PF) == FT_UI;
}

static const char *gsm_mux_state_str(enum gsm_mux_state state)
{
#if (CONFIG_GSM_MUX_LOG_LEVEL >= LOG_LEVEL_DBG) || defined(CONFIG_NET_SHELL)
	switch (state) {
	case GSM_MUX_SOF:
		return "Start-Of-Frame";
	case GSM_MUX_ADDRESS:
		return "Address";
	case GSM_MUX_CONTROL:
		return "Control";
	case GSM_MUX_LEN_0:
		return "Len0";
	case GSM_MUX_LEN_1:
		return "Len1";
	case GSM_MUX_DATA:
		return "Data";
	case GSM_MUX_FCS:
		return "FCS";
	case GSM_MUX_EOF:
		return "End-Of-Frame";
	}
#else
	ARG_UNUSED(state);
#endif

	return "";
}

#if CONFIG_GSM_MUX_LOG_LEVEL >= LOG_LEVEL_DBG
static void validate_state_transition(enum gsm_mux_state current,
				      enum gsm_mux_state new)
{
	static const uint8_t valid_transitions[] = {
		[GSM_MUX_SOF] = 1 << GSM_MUX_ADDRESS,
		[GSM_MUX_ADDRESS] = 1 << GSM_MUX_CONTROL,
		[GSM_MUX_CONTROL] = 1 << GSM_MUX_LEN_0,
		[GSM_MUX_LEN_0] = 1 << GSM_MUX_LEN_1 |
				1 << GSM_MUX_DATA |
				1 << GSM_MUX_FCS |
				1 << GSM_MUX_SOF,
		[GSM_MUX_LEN_1] = 1 << GSM_MUX_DATA |
				1 << GSM_MUX_FCS |
				1 << GSM_MUX_SOF,
		[GSM_MUX_DATA] = 1 << GSM_MUX_FCS |
				1 << GSM_MUX_SOF,
		[GSM_MUX_FCS] = 1 << GSM_MUX_EOF,
		[GSM_MUX_EOF] = 1 << GSM_MUX_SOF
	};

	if (!(valid_transitions[current] & 1 << new)) {
		LOG_DBG("Invalid state transition: %s (%d) => %s (%d)",
			gsm_mux_state_str(current), current,
			gsm_mux_state_str(new), new);
	}
}
#else
static inline void validate_state_transition(enum gsm_mux_state current,
					     enum gsm_mux_state new)
{
	ARG_UNUSED(current);
	ARG_UNUSED(new);
}
#endif

static inline enum gsm_mux_state gsm_mux_get_state(const struct gsm_mux *mux)
{
	return (enum gsm_mux_state)mux->state;
}

void gsm_mux_change_state(struct gsm_mux *mux, enum gsm_mux_state new_state)
{
	__ASSERT_NO_MSG(mux);

	if (gsm_mux_get_state(mux) == new_state) {
		return;
	}

	LOG_DBG("[%p] state %s (%d) => %s (%d)",
		mux, gsm_mux_state_str(mux->state), mux->state,
		gsm_mux_state_str(new_state), new_state);

	validate_state_transition(mux->state, new_state);

	mux->state = new_state;
}

static void gsm_mux_process_data(struct gsm_mux *mux, uint8_t recv_byte)
{
	size_t bytes_added;

	switch (mux->state) {
	case GSM_MUX_SOF:
		/* This is the initial state where we look for SOF char */
		if (recv_byte == SOF_MARKER) {
			gsm_mux_change_state(mux, GSM_MUX_ADDRESS);
			mux->fcs = FCS_INIT_VALUE;
			mux->received = 0;

			/* Avoid memory leak by freeing all the allocated
			 * buffers at start.
			 */
			if (mux->buf) {
				net_buf_unref(mux->buf);
				mux->buf = NULL;
			}
		}

		break;

	case GSM_MUX_ADDRESS:
		/* DLCI (Data Link Connection Identifier) address we want to
		 * talk. This address field also contains C/R bit.
		 * Currently we only support one byte addresses.
		 */
		mux->address = recv_byte;
		LOG_DBG("[%p] recv %d address %d C/R %d", mux, recv_byte,
			mux->address >> 2, !!(mux->address & GSM_CR));
		gsm_mux_change_state(mux, GSM_MUX_CONTROL);
		mux->fcs = gsm_mux_fcs_add(mux->fcs, recv_byte);
		break;

	case GSM_MUX_CONTROL:
		mux->control = recv_byte;
		LOG_DBG("[%p] recv %s (0x%02x) control 0x%02x P/F %d", mux,
			get_frame_type_str(recv_byte & ~GSM_PF), recv_byte,
			mux->control & ~GSM_PF, !!(mux->control & GSM_PF));
		gsm_mux_change_state(mux, GSM_MUX_LEN_0);
		mux->fcs = gsm_mux_fcs_add(mux->fcs, recv_byte);
		break;

	case GSM_MUX_LEN_0:
		mux->fcs = gsm_mux_fcs_add(mux->fcs, recv_byte);
		mux->msg_len = 0;

		if (gsm_mux_read_msg_len(mux, recv_byte)) {
			if (mux->msg_len > mux->mru) {
				gsm_mux_change_state(mux, GSM_MUX_SOF);
			} else if (mux->msg_len == 0) {
				gsm_mux_change_state(mux, GSM_MUX_FCS);
			} else {
				gsm_mux_change_state(mux, GSM_MUX_DATA);

				LOG_DBG("[%p] data len %d", mux, mux->msg_len);
			}
		} else {
			gsm_mux_change_state(mux, GSM_MUX_LEN_1);
		}

		break;

	case GSM_MUX_LEN_1:
		mux->fcs = gsm_mux_fcs_add(mux->fcs, recv_byte);

		mux->msg_len |= recv_byte << 7;
		if (mux->msg_len > mux->mru) {
			gsm_mux_change_state(mux, GSM_MUX_SOF);
		} else if (mux->msg_len == 0) {
			gsm_mux_change_state(mux, GSM_MUX_FCS);
		} else {
			gsm_mux_change_state(mux, GSM_MUX_DATA);

			LOG_DBG("[%p] data len %d", mux, mux->msg_len);
		}

		break;

	case GSM_MUX_DATA:
		if (mux->buf == NULL) {
			mux->buf = net_buf_alloc(&gsm_mux_pool,
						 BUF_ALLOC_TIMEOUT);
			if (mux->buf == NULL) {
				LOG_ERR("[%p] Can't allocate RX data! "
					"Skipping data!", mux);
				gsm_mux_change_state(mux, GSM_MUX_SOF);
				break;
			}
		}

		bytes_added = net_buf_append_bytes(mux->buf, 1,
						   (void *)&recv_byte,
						   BUF_ALLOC_TIMEOUT,
						   gsm_mux_alloc_buf,
						   &gsm_mux_pool);
		if (bytes_added != 1) {
			gsm_mux_change_state(mux, GSM_MUX_SOF);
		} else if (++mux->received == mux->msg_len) {
			gsm_mux_change_state(mux, GSM_MUX_FCS);
		}

		break;

	case GSM_MUX_FCS:
		mux->received_fcs = recv_byte;

		/* Update the FCS for Unnumbered Information field (UI) */
		if (is_UI(mux)) {
			struct net_buf *buf = mux->buf;

			while (buf) {
				mux->fcs = gsm_mux_fcs_add_buf(mux->fcs,
							       buf->data,
							       buf->len);
				buf = buf->frags;
			}
		}

		mux->fcs = gsm_mux_fcs_add(mux->fcs, mux->received_fcs);
		if (mux->fcs == FCS_GOOD_VALUE) {
			int ret = gsm_mux_process_pkt(mux);

			if (ret < 0) {
				LOG_DBG("[%p] Cannot process pkt (%d)", mux,
					ret);
			}
		}

		gsm_mux_change_state(mux, GSM_MUX_EOF);
		break;

	case GSM_MUX_EOF:
		if (recv_byte == SOF_MARKER) {
			gsm_mux_change_state(mux, GSM_MUX_SOF);
		}

		break;
	}
}

void gsm_mux_recv_buf(struct gsm_mux *mux, uint8_t *buf, int len)
{
	int i = 0;

	LOG_DBG("Received %d bytes", len);

	while (i < len) {
		gsm_mux_process_data(mux, buf[i++]);
	}
}

static void dlci_done(struct gsm_dlci *dlci, bool connected)
{
	LOG_DBG("[%p] DLCI id %d %screated", dlci, dlci->num,
		connected == false ? "not " : "");

	/* Let the UART mux to continue */
	if (dlci->dlci_created_cb) {
		dlci->dlci_created_cb(dlci, connected, dlci->user_data);
	}
}

int gsm_dlci_create(struct gsm_mux *mux,
		    const struct device *uart,
		    int dlci_address,
		    gsm_mux_dlci_created_cb_t dlci_created_cb,
		    void *user_data,
		    struct gsm_dlci **dlci)
{
	int ret;

	*dlci = gsm_dlci_alloc(mux, dlci_address, uart, dlci_created_cb,
			       user_data);
	if (!*dlci) {
		LOG_ERR("[%p] Cannot allocate DLCI %d", mux, dlci_address);
		ret = -ENOMEM;
		goto fail;
	}

	ret = gsm_dlci_opening(*dlci, dlci_done);
	if (ret < 0 && ret != -EALREADY) {
		LOG_ERR("[%p] Cannot open DLCI %d", mux, dlci_address);
		gsm_dlci_free(mux, dlci_address);
		*dlci = NULL;
	} else {
		ret = 0;
	}

fail:
	return ret;
}

int gsm_dlci_send(struct gsm_dlci *dlci, const uint8_t *buf, size_t size)
{
	/* Mux the data and send to UART */
	return gsm_mux_send_data_msg(dlci->mux, true, dlci, FT_UIH, buf, size);
}

int gsm_dlci_id(struct gsm_dlci *dlci)
{
	return dlci->num;
}

struct gsm_mux *gsm_mux_create(const struct device *uart)
{
	struct gsm_mux *mux = NULL;
	int i;

	if (!gsm_mux_init_done) {
		LOG_ERR("GSM mux not initialized!");
		return NULL;
	}

	for (i = 0; i < ARRAY_SIZE(muxes); i++) {
		if (muxes[i].in_use) {
			/* If the mux was already created, return it */
			if (uart && muxes[i].uart == uart) {
				return &muxes[i];
			}

			continue;
		}

		mux = &muxes[i];

		memset(mux, 0, sizeof(*mux));

		mux->in_use = true;
		mux->uart = uart;
		mux->mru = CONFIG_GSM_MUX_MRU_DEFAULT_LEN;
		mux->retries = N2;
		mux->t1_timeout_value = CONFIG_GSM_MUX_T1_TIMEOUT ?
			CONFIG_GSM_MUX_T1_TIMEOUT : T1_MSEC;
		mux->t2_timeout_value = T2_MSEC;
		mux->is_initiator = CONFIG_GSM_MUX_INITIATOR;
		mux->state = GSM_MUX_SOF;
		mux->buf = NULL;

		k_work_init_delayable(&mux->t2_timer, gsm_mux_t2_timeout);
		sys_slist_init(&mux->pending_ctrls);

		/* The system will continue after the control DLCI is
		 * created or timeout occurs.
		 */
		break;
	}

	return mux;
}

int gsm_mux_send(struct gsm_mux *mux, uint8_t dlci_address,
		 const uint8_t *buf, size_t size)
{
	struct gsm_dlci *dlci;

	dlci = gsm_dlci_get(mux, dlci_address);
	if (!dlci) {
		return -ENOENT;
	}

	/* Mux the data and send to UART */
	return gsm_mux_send_data_msg(mux, true, dlci, FT_UIH, buf, size);
}

void gsm_mux_detach(struct gsm_mux *mux)
{
	struct gsm_dlci *dlci;

	for (int i = 0; i < ARRAY_SIZE(dlcis); i++) {
		dlci = &dlcis[i];

		if (mux != dlci->mux || !dlci->in_use) {
			continue;
		}

		dlci->in_use = false;
		sys_slist_prepend(&dlci_free_entries, &dlci->node);
	}
}

void gsm_mux_init(void)
{
	int i;

	if (gsm_mux_init_done) {
		return;
	}

	gsm_mux_init_done = true;

	sys_slist_init(&ctrls_free_entries);

	for (i = 0; i < ARRAY_SIZE(ctrls); i++) {
		sys_slist_prepend(&ctrls_free_entries, &ctrls[i].node);
	}

	sys_slist_init(&dlci_free_entries);

	for (i = 0; i < ARRAY_SIZE(dlcis); i++) {
		sys_slist_prepend(&dlci_free_entries, &dlcis[i].node);
	}

	k_work_init_delayable(&t1_timer, dlci_t1_timeout);
}

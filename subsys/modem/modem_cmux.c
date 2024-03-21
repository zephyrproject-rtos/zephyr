/*
 * Copyright (c) 2022 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(modem_cmux, CONFIG_MODEM_CMUX_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/sys/crc.h>
#include <zephyr/modem/cmux.h>

#include <string.h>

#define MODEM_CMUX_FCS_POLYNOMIAL		(0xE0)
#define MODEM_CMUX_FCS_INIT_VALUE		(0xFF)
#define MODEM_CMUX_EA				(0x01)
#define MODEM_CMUX_CR				(0x02)
#define MODEM_CMUX_PF				(0x10)
#define MODEM_CMUX_FRAME_SIZE_MAX		(0x08)
#define MODEM_CMUX_DATA_SIZE_MIN		(0x08)
#define MODEM_CMUX_DATA_FRAME_SIZE_MIN		(MODEM_CMUX_FRAME_SIZE_MAX + \
						 MODEM_CMUX_DATA_SIZE_MIN)

#define MODEM_CMUX_CMD_DATA_SIZE_MAX		(0x08)
#define MODEM_CMUX_CMD_FRAME_SIZE_MAX		(MODEM_CMUX_FRAME_SIZE_MAX + \
						 MODEM_CMUX_CMD_DATA_SIZE_MAX)

#define MODEM_CMUX_T1_TIMEOUT			(K_MSEC(330))
#define MODEM_CMUX_T2_TIMEOUT			(K_MSEC(660))

#define MODEM_CMUX_EVENT_CONNECTED_BIT		(BIT(0))
#define MODEM_CMUX_EVENT_DISCONNECTED_BIT	(BIT(1))

enum modem_cmux_frame_types {
	MODEM_CMUX_FRAME_TYPE_RR = 0x01,
	MODEM_CMUX_FRAME_TYPE_UI = 0x03,
	MODEM_CMUX_FRAME_TYPE_RNR = 0x05,
	MODEM_CMUX_FRAME_TYPE_REJ = 0x09,
	MODEM_CMUX_FRAME_TYPE_DM = 0x0F,
	MODEM_CMUX_FRAME_TYPE_SABM = 0x2F,
	MODEM_CMUX_FRAME_TYPE_DISC = 0x43,
	MODEM_CMUX_FRAME_TYPE_UA = 0x63,
	MODEM_CMUX_FRAME_TYPE_UIH = 0xEF,
};

enum modem_cmux_command_types {
	MODEM_CMUX_COMMAND_NSC = 0x04,
	MODEM_CMUX_COMMAND_TEST = 0x08,
	MODEM_CMUX_COMMAND_PSC = 0x10,
	MODEM_CMUX_COMMAND_RLS = 0x14,
	MODEM_CMUX_COMMAND_FCOFF = 0x18,
	MODEM_CMUX_COMMAND_PN = 0x20,
	MODEM_CMUX_COMMAND_RPN = 0x24,
	MODEM_CMUX_COMMAND_FCON = 0x28,
	MODEM_CMUX_COMMAND_CLD = 0x30,
	MODEM_CMUX_COMMAND_SNC = 0x34,
	MODEM_CMUX_COMMAND_MSC = 0x38,
};

struct modem_cmux_command_type {
	uint8_t ea: 1;
	uint8_t cr: 1;
	uint8_t value: 6;
};

struct modem_cmux_command_length {
	uint8_t ea: 1;
	uint8_t value: 7;
};

struct modem_cmux_command {
	struct modem_cmux_command_type type;
	struct modem_cmux_command_length length;
	uint8_t value[];
};

static int modem_cmux_wrap_command(struct modem_cmux_command **command, const uint8_t *data,
				   uint16_t data_len)
{
	if ((data == NULL) || (data_len < 2)) {
		return -EINVAL;
	}

	(*command) = (struct modem_cmux_command *)data;

	if (((*command)->length.ea == 0) || ((*command)->type.ea == 0)) {
		return -EINVAL;
	}

	if ((*command)->length.value != (data_len - 2)) {
		return -EINVAL;
	}

	return 0;
}

static struct modem_cmux_command *modem_cmux_command_wrap(const uint8_t *data)
{
	return (struct modem_cmux_command *)data;
}

static const char *modem_cmux_frame_type_to_str(enum modem_cmux_frame_types frame_type)
{
	switch (frame_type) {
	case MODEM_CMUX_FRAME_TYPE_RR:
		return "RR";
	case MODEM_CMUX_FRAME_TYPE_UI:
		return "UI";
	case MODEM_CMUX_FRAME_TYPE_RNR:
		return "RNR";
	case MODEM_CMUX_FRAME_TYPE_REJ:
		return "REJ";
	case MODEM_CMUX_FRAME_TYPE_DM:
		return "DM";
	case MODEM_CMUX_FRAME_TYPE_SABM:
		return "SABM";
	case MODEM_CMUX_FRAME_TYPE_DISC:
		return "DISC";
	case MODEM_CMUX_FRAME_TYPE_UA:
		return "UA";
	case MODEM_CMUX_FRAME_TYPE_UIH:
		return "UIH";
	}
	return "";
}

static void modem_cmux_log_frame(const struct modem_cmux_frame *frame,
				 const char *action, size_t hexdump_len)
{
	LOG_DBG("%s ch:%u cr:%u pf:%u type:%s dlen:%u", action, frame->dlci_address,
		frame->cr, frame->pf, modem_cmux_frame_type_to_str(frame->type), frame->data_len);
	LOG_HEXDUMP_DBG(frame->data, hexdump_len, "data:");
}

static void modem_cmux_log_transmit_frame(const struct modem_cmux_frame *frame)
{
	modem_cmux_log_frame(frame, "tx", frame->data_len);
}

static void modem_cmux_log_received_frame(const struct modem_cmux_frame *frame)
{
	modem_cmux_log_frame(frame, "rcvd", frame->data_len);
}

static const char *modem_cmux_command_type_to_str(enum modem_cmux_command_types command_type)
{
	switch (command_type) {
	case MODEM_CMUX_COMMAND_NSC:
		return "NSC";
	case MODEM_CMUX_COMMAND_TEST:
		return "TEST";
	case MODEM_CMUX_COMMAND_PSC:
		return "PSC";
	case MODEM_CMUX_COMMAND_RLS:
		return "RLS";
	case MODEM_CMUX_COMMAND_FCOFF:
		return "FCOFF";
	case MODEM_CMUX_COMMAND_PN:
		return "PN";
	case MODEM_CMUX_COMMAND_RPN:
		return "RPN";
	case MODEM_CMUX_COMMAND_FCON:
		return "FCON";
	case MODEM_CMUX_COMMAND_CLD:
		return "CLD";
	case MODEM_CMUX_COMMAND_SNC:
		return "SNC";
	case MODEM_CMUX_COMMAND_MSC:
		return "MSC";
	}
	return "";
}

static void modem_cmux_log_transmit_command(const struct modem_cmux_command *command)
{
	LOG_DBG("ea:%u,cr:%u,type:%s", command->type.ea, command->type.cr,
		modem_cmux_command_type_to_str(command->type.value));
	LOG_HEXDUMP_DBG(command->value, command->length.value, "data:");
}

static void modem_cmux_log_received_command(const struct modem_cmux_command *command)
{
	LOG_DBG("ea:%u,cr:%u,type:%s", command->type.ea, command->type.cr,
		modem_cmux_command_type_to_str(command->type.value));
	LOG_HEXDUMP_DBG(command->value, command->length.value, "data:");
}

static void modem_cmux_raise_event(struct modem_cmux *cmux, enum modem_cmux_event event)
{
	if (cmux->callback == NULL) {
		return;
	}

	cmux->callback(cmux, event, cmux->user_data);
}

static void modem_cmux_bus_callback(struct modem_pipe *pipe, enum modem_pipe_event event,
				    void *user_data)
{
	struct modem_cmux *cmux = (struct modem_cmux *)user_data;

	switch (event) {
	case MODEM_PIPE_EVENT_RECEIVE_READY:
		k_work_schedule(&cmux->receive_work, K_NO_WAIT);
		break;

	case MODEM_PIPE_EVENT_TRANSMIT_IDLE:
		k_work_schedule(&cmux->transmit_work, K_NO_WAIT);
		break;

	default:
		break;
	}
}

static uint16_t modem_cmux_transmit_frame(struct modem_cmux *cmux,
					  const struct modem_cmux_frame *frame)
{
	uint8_t buf[MODEM_CMUX_FRAME_SIZE_MAX];
	uint8_t fcs;
	uint16_t space;
	uint16_t data_len;
	uint16_t buf_idx;

	space = ring_buf_space_get(&cmux->transmit_rb) - MODEM_CMUX_FRAME_SIZE_MAX;
	data_len = MIN(space, frame->data_len);

	/* SOF */
	buf[0] = 0xF9;

	/* DLCI Address (Max 63) */
	buf[1] = 0x01 | (frame->cr << 1) | (frame->dlci_address << 2);

	/* Frame type and poll/final */
	buf[2] = frame->type | (frame->pf << 4);

	/* Data length */
	if (data_len > 127) {
		buf[3] = data_len << 1;
		buf[4] = data_len >> 7;
		buf_idx = 5;
	} else {
		buf[3] = 0x01 | (data_len << 1);
		buf_idx = 4;
	}

	/* Compute FCS for the header (exclude SOF) */
	fcs = crc8(&buf[1], (buf_idx - 1), MODEM_CMUX_FCS_POLYNOMIAL, MODEM_CMUX_FCS_INIT_VALUE,
		   true);

	/* FCS final */
	if (frame->type == MODEM_CMUX_FRAME_TYPE_UIH) {
		fcs = 0xFF - fcs;
	} else {
		fcs = 0xFF - crc8(frame->data, data_len, MODEM_CMUX_FCS_POLYNOMIAL, fcs, true);
	}

	/* Frame header */
	ring_buf_put(&cmux->transmit_rb, buf, buf_idx);

	/* Data */
	ring_buf_put(&cmux->transmit_rb, frame->data, data_len);

	/* FCS and EOF will be put on the same call */
	buf[0] = fcs;
	buf[1] = 0xF9;
	ring_buf_put(&cmux->transmit_rb, buf, 2);
	k_work_schedule(&cmux->transmit_work, K_NO_WAIT);
	return data_len;
}

static bool modem_cmux_transmit_cmd_frame(struct modem_cmux *cmux,
					  const struct modem_cmux_frame *frame)
{
	uint16_t space;
	struct modem_cmux_command *command;

	k_mutex_lock(&cmux->transmit_rb_lock, K_FOREVER);
	space = ring_buf_space_get(&cmux->transmit_rb);

	if (space < MODEM_CMUX_CMD_FRAME_SIZE_MAX) {
		k_mutex_unlock(&cmux->transmit_rb_lock);
		return false;
	}

	modem_cmux_log_transmit_frame(frame);
	if (modem_cmux_wrap_command(&command, frame->data, frame->data_len) == 0) {
		modem_cmux_log_transmit_command(command);
	}

	modem_cmux_transmit_frame(cmux, frame);
	k_mutex_unlock(&cmux->transmit_rb_lock);
	return true;
}

static int16_t modem_cmux_transmit_data_frame(struct modem_cmux *cmux,
					      const struct modem_cmux_frame *frame)
{
	uint16_t space;
	int ret;

	k_mutex_lock(&cmux->transmit_rb_lock, K_FOREVER);

	if (cmux->flow_control_on == false) {
		k_mutex_unlock(&cmux->transmit_rb_lock);
		return 0;
	}

	space = ring_buf_space_get(&cmux->transmit_rb);

	/*
	 * Two command frames are reserved for command channel, and we shall prefer
	 * waiting for more than MODEM_CMUX_DATA_FRAME_SIZE_MIN bytes available in the
	 * transmit buffer rather than transmitting a few bytes at a time. This avoids
	 * excessive wrapping overhead, since transmitting a single byte will require 8
	 * bytes of wrapping.
	 */
	if (space < ((MODEM_CMUX_CMD_FRAME_SIZE_MAX * 2) + MODEM_CMUX_DATA_FRAME_SIZE_MIN)) {
		k_mutex_unlock(&cmux->transmit_rb_lock);
		return -ENOMEM;
	}

	modem_cmux_log_transmit_frame(frame);
	ret = modem_cmux_transmit_frame(cmux, frame);
	k_mutex_unlock(&cmux->transmit_rb_lock);
	return ret;
}

static void modem_cmux_acknowledge_received_frame(struct modem_cmux *cmux)
{
	struct modem_cmux_command *command;
	struct modem_cmux_frame frame;
	uint8_t data[MODEM_CMUX_CMD_DATA_SIZE_MAX];

	if (sizeof(data) < cmux->frame.data_len) {
		LOG_WRN("Command acknowledge buffer overrun");
		return;
	}

	memcpy(&frame, &cmux->frame, sizeof(cmux->frame));
	memcpy(data, cmux->frame.data, cmux->frame.data_len);
	modem_cmux_wrap_command(&command, data, cmux->frame.data_len);
	command->type.cr = 0;
	frame.data = data;
	frame.data_len = cmux->frame.data_len;

	if (modem_cmux_transmit_cmd_frame(cmux, &frame) == false) {
		LOG_WRN("Command acknowledge buffer overrun");
	}
}

static void modem_cmux_on_msc_command(struct modem_cmux *cmux, struct modem_cmux_command *command)
{
	if (command->type.cr) {
		modem_cmux_acknowledge_received_frame(cmux);
	}
}

static void modem_cmux_on_fcon_command(struct modem_cmux *cmux)
{
	k_mutex_lock(&cmux->transmit_rb_lock, K_FOREVER);
	cmux->flow_control_on = true;
	k_mutex_unlock(&cmux->transmit_rb_lock);
	modem_cmux_acknowledge_received_frame(cmux);
}

static void modem_cmux_on_fcoff_command(struct modem_cmux *cmux)
{
	k_mutex_lock(&cmux->transmit_rb_lock, K_FOREVER);
	cmux->flow_control_on = false;
	k_mutex_unlock(&cmux->transmit_rb_lock);
	modem_cmux_acknowledge_received_frame(cmux);
}

static void modem_cmux_on_cld_command(struct modem_cmux *cmux, struct modem_cmux_command *command)
{
	if (command->type.cr) {
		modem_cmux_acknowledge_received_frame(cmux);
	}

	if (cmux->state != MODEM_CMUX_STATE_DISCONNECTING &&
	    cmux->state != MODEM_CMUX_STATE_CONNECTED) {
		LOG_WRN("Unexpected close down");
		return;
	}

	if (cmux->state == MODEM_CMUX_STATE_DISCONNECTING) {
		k_work_cancel_delayable(&cmux->disconnect_work);
	}

	cmux->state = MODEM_CMUX_STATE_DISCONNECTED;
	k_mutex_lock(&cmux->transmit_rb_lock, K_FOREVER);
	cmux->flow_control_on = false;
	k_mutex_unlock(&cmux->transmit_rb_lock);

	modem_cmux_raise_event(cmux, MODEM_CMUX_EVENT_DISCONNECTED);
	k_event_clear(&cmux->event, MODEM_CMUX_EVENT_CONNECTED_BIT);
	k_event_post(&cmux->event, MODEM_CMUX_EVENT_DISCONNECTED_BIT);
}

static void modem_cmux_on_control_frame_ua(struct modem_cmux *cmux)
{
	if (cmux->state != MODEM_CMUX_STATE_CONNECTING) {
		LOG_DBG("Unexpected UA frame");
		return;
	}

	cmux->state = MODEM_CMUX_STATE_CONNECTED;
	k_mutex_lock(&cmux->transmit_rb_lock, K_FOREVER);
	cmux->flow_control_on = true;
	k_mutex_unlock(&cmux->transmit_rb_lock);
	k_work_cancel_delayable(&cmux->connect_work);
	modem_cmux_raise_event(cmux, MODEM_CMUX_EVENT_CONNECTED);
	k_event_clear(&cmux->event, MODEM_CMUX_EVENT_DISCONNECTED_BIT);
	k_event_post(&cmux->event, MODEM_CMUX_EVENT_CONNECTED_BIT);
}

static void modem_cmux_on_control_frame_uih(struct modem_cmux *cmux)
{
	struct modem_cmux_command *command;

	if ((cmux->state != MODEM_CMUX_STATE_CONNECTED) &&
	    (cmux->state != MODEM_CMUX_STATE_DISCONNECTING)) {
		LOG_DBG("Unexpected UIH frame");
		return;
	}

	if (modem_cmux_wrap_command(&command, cmux->frame.data, cmux->frame.data_len) < 0) {
		LOG_WRN("Invalid command");
		return;
	}

	modem_cmux_log_received_command(command);

	switch (command->type.value) {
	case MODEM_CMUX_COMMAND_CLD:
		modem_cmux_on_cld_command(cmux, command);
		break;

	case MODEM_CMUX_COMMAND_MSC:
		modem_cmux_on_msc_command(cmux, command);
		break;

	case MODEM_CMUX_COMMAND_FCON:
		modem_cmux_on_fcon_command(cmux);
		break;

	case MODEM_CMUX_COMMAND_FCOFF:
		modem_cmux_on_fcoff_command(cmux);
		break;

	default:
		LOG_DBG("Unknown control command");
		break;
	}
}

static void modem_cmux_connect_response_transmit(struct modem_cmux *cmux)
{
	if (cmux == NULL) {
		return;
	}

	struct modem_cmux_frame frame = {
		.dlci_address = cmux->frame.dlci_address,
		.cr = cmux->frame.cr,
		.pf = cmux->frame.pf,
		.type = MODEM_CMUX_FRAME_TYPE_UA,
		.data = NULL,
		.data_len = 0,
	};

	LOG_DBG("SABM/DISC request state send ack");
	modem_cmux_transmit_cmd_frame(cmux, &frame);
}

static void modem_cmux_on_control_frame_sabm(struct modem_cmux *cmux)
{
	modem_cmux_connect_response_transmit(cmux);

	if ((cmux->state == MODEM_CMUX_STATE_CONNECTED) ||
	    (cmux->state == MODEM_CMUX_STATE_DISCONNECTING)) {
		LOG_DBG("Connect request not accepted");
		return;
	}

	cmux->state = MODEM_CMUX_STATE_CONNECTED;
	k_mutex_lock(&cmux->transmit_rb_lock, K_FOREVER);
	cmux->flow_control_on = true;
	k_mutex_unlock(&cmux->transmit_rb_lock);
	modem_cmux_raise_event(cmux, MODEM_CMUX_EVENT_CONNECTED);
	k_event_clear(&cmux->event, MODEM_CMUX_EVENT_DISCONNECTED_BIT);
	k_event_post(&cmux->event, MODEM_CMUX_EVENT_CONNECTED_BIT);
}

static void modem_cmux_on_control_frame(struct modem_cmux *cmux)
{
	modem_cmux_log_received_frame(&cmux->frame);

	switch (cmux->frame.type) {
	case MODEM_CMUX_FRAME_TYPE_UA:
		modem_cmux_on_control_frame_ua(cmux);
		break;

	case MODEM_CMUX_FRAME_TYPE_UIH:
		modem_cmux_on_control_frame_uih(cmux);
		break;

	case MODEM_CMUX_FRAME_TYPE_SABM:
		modem_cmux_on_control_frame_sabm(cmux);
		break;

	default:
		LOG_WRN("Unknown %s frame type", "control");
		break;
	}
}

static struct modem_cmux_dlci *modem_cmux_find_dlci(struct modem_cmux *cmux)
{
	sys_snode_t *node;
	struct modem_cmux_dlci *dlci;

	SYS_SLIST_FOR_EACH_NODE(&cmux->dlcis, node) {
		dlci = (struct modem_cmux_dlci *)node;

		if (dlci->dlci_address == cmux->frame.dlci_address) {
			return dlci;
		}
	}

	return NULL;
}

static void modem_cmux_on_dlci_frame_ua(struct modem_cmux_dlci *dlci)
{
	switch (dlci->state) {
	case MODEM_CMUX_DLCI_STATE_OPENING:
		dlci->state = MODEM_CMUX_DLCI_STATE_OPEN;
		modem_pipe_notify_opened(&dlci->pipe);
		k_work_cancel_delayable(&dlci->open_work);
		k_mutex_lock(&dlci->receive_rb_lock, K_FOREVER);
		ring_buf_reset(&dlci->receive_rb);
		k_mutex_unlock(&dlci->receive_rb_lock);
		break;

	case MODEM_CMUX_DLCI_STATE_CLOSING:
		dlci->state = MODEM_CMUX_DLCI_STATE_CLOSED;
		modem_pipe_notify_closed(&dlci->pipe);
		k_work_cancel_delayable(&dlci->close_work);
		break;

	default:
		LOG_DBG("Unexpected UA frame");
		break;
	}
}

static void modem_cmux_on_dlci_frame_uih(struct modem_cmux_dlci *dlci)
{
	struct modem_cmux *cmux = dlci->cmux;
	uint32_t written;

	if (dlci->state != MODEM_CMUX_DLCI_STATE_OPEN) {
		LOG_DBG("Unexpected UIH frame");
		return;
	}

	k_mutex_lock(&dlci->receive_rb_lock, K_FOREVER);
	written = ring_buf_put(&dlci->receive_rb, cmux->frame.data, cmux->frame.data_len);
	k_mutex_unlock(&dlci->receive_rb_lock);
	if (written != cmux->frame.data_len) {
		LOG_WRN("DLCI %u receive buffer overrun (dropped %u out of %u bytes)",
			dlci->dlci_address, cmux->frame.data_len - written, cmux->frame.data_len);
	}
	modem_pipe_notify_receive_ready(&dlci->pipe);
}

static void modem_cmux_on_dlci_frame_sabm(struct modem_cmux_dlci *dlci)
{
	struct modem_cmux *cmux = dlci->cmux;

	modem_cmux_connect_response_transmit(cmux);

	if (dlci->state == MODEM_CMUX_DLCI_STATE_OPEN) {
		LOG_DBG("Unexpected SABM frame");
		return;
	}

	dlci->state = MODEM_CMUX_DLCI_STATE_OPEN;
	modem_pipe_notify_opened(&dlci->pipe);
	k_mutex_lock(&dlci->receive_rb_lock, K_FOREVER);
	ring_buf_reset(&dlci->receive_rb);
	k_mutex_unlock(&dlci->receive_rb_lock);
}

static void modem_cmux_on_dlci_frame_disc(struct modem_cmux_dlci *dlci)
{
	struct modem_cmux *cmux = dlci->cmux;

	modem_cmux_connect_response_transmit(cmux);

	if (dlci->state != MODEM_CMUX_DLCI_STATE_OPEN) {
		LOG_DBG("Unexpected Disc frame");
		return;
	}

	dlci->state = MODEM_CMUX_DLCI_STATE_CLOSED;
	modem_pipe_notify_closed(&dlci->pipe);
}

static void modem_cmux_on_dlci_frame(struct modem_cmux *cmux)
{
	struct modem_cmux_dlci *dlci;

	modem_cmux_log_received_frame(&cmux->frame);

	dlci = modem_cmux_find_dlci(cmux);
	if (dlci == NULL) {
		LOG_WRN("Ignoring frame intended for unconfigured DLCI %u.",
			cmux->frame.dlci_address);
		return;
	}

	switch (cmux->frame.type) {
	case MODEM_CMUX_FRAME_TYPE_UA:
		modem_cmux_on_dlci_frame_ua(dlci);
		break;

	case MODEM_CMUX_FRAME_TYPE_UIH:
		modem_cmux_on_dlci_frame_uih(dlci);
		break;

	case MODEM_CMUX_FRAME_TYPE_SABM:
		modem_cmux_on_dlci_frame_sabm(dlci);
		break;

	case MODEM_CMUX_FRAME_TYPE_DISC:
		modem_cmux_on_dlci_frame_disc(dlci);
		break;

	default:
		LOG_WRN("Unknown %s frame type", "DLCI");
		break;
	}
}

static void modem_cmux_on_frame(struct modem_cmux *cmux)
{
	if (cmux->frame.dlci_address == 0) {
		modem_cmux_on_control_frame(cmux);
	} else {
		modem_cmux_on_dlci_frame(cmux);
	}
}

static void modem_cmux_drop_frame(struct modem_cmux *cmux)
{
	LOG_WRN("Dropped frame");
	cmux->receive_state = MODEM_CMUX_RECEIVE_STATE_SOF;

#if defined(CONFIG_MODEM_CMUX_LOG_LEVEL_DBG)
	struct modem_cmux_frame *frame = &cmux->frame;

	frame->data = cmux->receive_buf;
	modem_cmux_log_frame(frame, "dropped", MIN(frame->data_len, cmux->receive_buf_size));
#endif
}

static void modem_cmux_process_received_byte(struct modem_cmux *cmux, uint8_t byte)
{
	uint8_t fcs;

	switch (cmux->receive_state) {
	case MODEM_CMUX_RECEIVE_STATE_SOF:
		if (byte == 0xF9) {
			cmux->receive_state = MODEM_CMUX_RECEIVE_STATE_RESYNC;
			break;
		}

		break;

	case MODEM_CMUX_RECEIVE_STATE_RESYNC:
		/*
		 * Allow any number of consequtive flags (0xF9).
		 * 0xF9 could also be a valid address field for DLCI 62.
		 */
		if (byte == 0xF9) {
			break;
		}

		__fallthrough;

	case MODEM_CMUX_RECEIVE_STATE_ADDRESS:
		/* Initialize */
		cmux->receive_buf_len = 0;
		cmux->frame_header_len = 0;

		/* Store header for FCS */
		cmux->frame_header[cmux->frame_header_len] = byte;
		cmux->frame_header_len++;

		/* Get CR */
		cmux->frame.cr = (byte & 0x02) ? true : false;

		/* Get DLCI address */
		cmux->frame.dlci_address = (byte >> 2) & 0x3F;

		/* Await control */
		cmux->receive_state = MODEM_CMUX_RECEIVE_STATE_CONTROL;
		break;

	case MODEM_CMUX_RECEIVE_STATE_CONTROL:
		/* Store header for FCS */
		cmux->frame_header[cmux->frame_header_len] = byte;
		cmux->frame_header_len++;

		/* Get PF */
		cmux->frame.pf = (byte & MODEM_CMUX_PF) ? true : false;

		/* Get frame type */
		cmux->frame.type = byte & (~MODEM_CMUX_PF);

		/* Await data length */
		cmux->receive_state = MODEM_CMUX_RECEIVE_STATE_LENGTH;
		break;

	case MODEM_CMUX_RECEIVE_STATE_LENGTH:
		/* Store header for FCS */
		cmux->frame_header[cmux->frame_header_len] = byte;
		cmux->frame_header_len++;

		/* Get first 7 bits of data length */
		cmux->frame.data_len = (byte >> 1);

		/* Check if length field continues */
		if ((byte & MODEM_CMUX_EA) == 0) {
			/* Await continued length field */
			cmux->receive_state = MODEM_CMUX_RECEIVE_STATE_LENGTH_CONT;
			break;
		}

		/* Check if no data field */
		if (cmux->frame.data_len == 0) {
			/* Await FCS */
			cmux->receive_state = MODEM_CMUX_RECEIVE_STATE_FCS;
			break;
		}

		/* Await data */
		cmux->receive_state = MODEM_CMUX_RECEIVE_STATE_DATA;
		break;

	case MODEM_CMUX_RECEIVE_STATE_LENGTH_CONT:
		/* Store header for FCS */
		cmux->frame_header[cmux->frame_header_len] = byte;
		cmux->frame_header_len++;

		/* Get last 8 bits of data length */
		cmux->frame.data_len |= ((uint16_t)byte) << 7;

		/* Await data */
		cmux->receive_state = MODEM_CMUX_RECEIVE_STATE_DATA;
		break;

	case MODEM_CMUX_RECEIVE_STATE_DATA:
		/* Copy byte to data */
		if (cmux->receive_buf_len < cmux->receive_buf_size) {
			cmux->receive_buf[cmux->receive_buf_len] = byte;
		}
		cmux->receive_buf_len++;

		/* Check if datalen reached */
		if (cmux->frame.data_len == cmux->receive_buf_len) {
			/* Await FCS */
			cmux->receive_state = MODEM_CMUX_RECEIVE_STATE_FCS;
		}

		break;

	case MODEM_CMUX_RECEIVE_STATE_FCS:
		if (cmux->receive_buf_len > cmux->receive_buf_size) {
			LOG_WRN("Receive buffer overrun (%u > %u)",
				cmux->receive_buf_len, cmux->receive_buf_size);
			cmux->receive_state = MODEM_CMUX_RECEIVE_STATE_DROP;
			break;
		}

		/* Compute FCS */
		if (cmux->frame.type == MODEM_CMUX_FRAME_TYPE_UIH) {
			fcs = 0xFF - crc8(cmux->frame_header, cmux->frame_header_len,
					  MODEM_CMUX_FCS_POLYNOMIAL, MODEM_CMUX_FCS_INIT_VALUE,
					  true);
		} else {
			fcs = crc8(cmux->frame_header, cmux->frame_header_len,
				   MODEM_CMUX_FCS_POLYNOMIAL, MODEM_CMUX_FCS_INIT_VALUE, true);

			fcs = 0xFF - crc8(cmux->frame.data, cmux->frame.data_len,
					  MODEM_CMUX_FCS_POLYNOMIAL, fcs, true);
		}

		/* Validate FCS */
		if (fcs != byte) {
			LOG_WRN("Frame FCS error");

			/* Drop frame */
			cmux->receive_state = MODEM_CMUX_RECEIVE_STATE_DROP;
			break;
		}

		cmux->receive_state = MODEM_CMUX_RECEIVE_STATE_EOF;
		break;

	case MODEM_CMUX_RECEIVE_STATE_DROP:
		modem_cmux_drop_frame(cmux);
		break;

	case MODEM_CMUX_RECEIVE_STATE_EOF:
		/* Validate byte is EOF */
		if (byte != 0xF9) {
			/* Unexpected byte */
			modem_cmux_drop_frame(cmux);
			break;
		}

		/* Process frame */
		cmux->frame.data = cmux->receive_buf;
		modem_cmux_on_frame(cmux);

		/* Await start of next frame */
		cmux->receive_state = MODEM_CMUX_RECEIVE_STATE_SOF;
		break;

	default:
		break;
	}
}

static void modem_cmux_receive_handler(struct k_work *item)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(item);
	struct modem_cmux *cmux = CONTAINER_OF(dwork, struct modem_cmux, receive_work);
	int ret;

	/* Receive data from pipe */
	ret = modem_pipe_receive(cmux->pipe, cmux->work_buf, sizeof(cmux->work_buf));
	if (ret < 1) {
		if (ret < 0) {
			LOG_ERR("Pipe receiving error: %d", ret);
		}
		return;
	}

	/* Process received data */
	for (int i = 0; i < ret; i++) {
		modem_cmux_process_received_byte(cmux, cmux->work_buf[i]);
	}

	/* Reschedule received work */
	k_work_schedule(&cmux->receive_work, K_NO_WAIT);
}

static void modem_cmux_dlci_notify_transmit_idle(struct modem_cmux *cmux)
{
	sys_snode_t *node;
	struct modem_cmux_dlci *dlci;

	SYS_SLIST_FOR_EACH_NODE(&cmux->dlcis, node) {
		dlci = (struct modem_cmux_dlci *)node;
		modem_pipe_notify_transmit_idle(&dlci->pipe);
	}
}

static void modem_cmux_transmit_handler(struct k_work *item)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(item);
	struct modem_cmux *cmux = CONTAINER_OF(dwork, struct modem_cmux, transmit_work);
	uint8_t *reserved;
	uint32_t reserved_size;
	int ret;
	bool transmit_rb_empty;

	k_mutex_lock(&cmux->transmit_rb_lock, K_FOREVER);

	while (true) {
		transmit_rb_empty = ring_buf_is_empty(&cmux->transmit_rb);

		if (transmit_rb_empty) {
			break;
		}

		reserved_size = ring_buf_get_claim(&cmux->transmit_rb, &reserved, UINT32_MAX);

		ret = modem_pipe_transmit(cmux->pipe, reserved, reserved_size);
		if (ret < 0) {
			ring_buf_get_finish(&cmux->transmit_rb, 0);
			if (ret != -EPERM) {
				LOG_ERR("Failed to %s %u bytes. (%d)",
					"transmit", reserved_size, ret);
			}
			break;
		}

		ring_buf_get_finish(&cmux->transmit_rb, (uint32_t)ret);

		if (ret < reserved_size) {
			LOG_DBG("Transmitted only %u out of %u bytes at once.", ret, reserved_size);
			break;
		}
	}

	k_mutex_unlock(&cmux->transmit_rb_lock);

	if (transmit_rb_empty) {
		modem_cmux_dlci_notify_transmit_idle(cmux);
	}
}

static void modem_cmux_connect_handler(struct k_work *item)
{
	struct k_work_delayable *dwork;
	struct modem_cmux *cmux;

	if (item == NULL) {
		return;
	}

	dwork = k_work_delayable_from_work(item);
	cmux = CONTAINER_OF(dwork, struct modem_cmux, connect_work);

	cmux->state = MODEM_CMUX_STATE_CONNECTING;

	struct modem_cmux_frame frame = {
		.dlci_address = 0,
		.cr = true,
		.pf = true,
		.type = MODEM_CMUX_FRAME_TYPE_SABM,
		.data = NULL,
		.data_len = 0,
	};

	modem_cmux_transmit_cmd_frame(cmux, &frame);
	k_work_schedule(&cmux->connect_work, MODEM_CMUX_T1_TIMEOUT);
}

static void modem_cmux_disconnect_handler(struct k_work *item)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(item);
	struct modem_cmux *cmux = CONTAINER_OF(dwork, struct modem_cmux, disconnect_work);
	struct modem_cmux_command *command;
	uint8_t data[2];

	cmux->state = MODEM_CMUX_STATE_DISCONNECTING;

	command = modem_cmux_command_wrap(data);
	command->type.ea = 1;
	command->type.cr = 1;
	command->type.value = MODEM_CMUX_COMMAND_CLD;
	command->length.ea = 1;
	command->length.value = 0;

	struct modem_cmux_frame frame = {
		.dlci_address = 0,
		.cr = true,
		.pf = false,
		.type = MODEM_CMUX_FRAME_TYPE_UIH,
		.data = data,
		.data_len = sizeof(data),
	};

	/* Transmit close down command */
	modem_cmux_transmit_cmd_frame(cmux, &frame);
	k_work_schedule(&cmux->disconnect_work, MODEM_CMUX_T1_TIMEOUT);
}

static int modem_cmux_dlci_pipe_api_open(void *data)
{
	struct modem_cmux_dlci *dlci = (struct modem_cmux_dlci *)data;

	if (k_work_delayable_is_pending(&dlci->open_work) == true) {
		return -EBUSY;
	}

	k_work_schedule(&dlci->open_work, K_NO_WAIT);
	return 0;
}

static int modem_cmux_dlci_pipe_api_transmit(void *data, const uint8_t *buf, size_t size)
{
	struct modem_cmux_dlci *dlci = (struct modem_cmux_dlci *)data;
	struct modem_cmux *cmux = dlci->cmux;

	struct modem_cmux_frame frame = {
		.dlci_address = dlci->dlci_address,
		.cr = true,
		.pf = false,
		.type = MODEM_CMUX_FRAME_TYPE_UIH,
		.data = buf,
		.data_len = size,
	};

	return modem_cmux_transmit_data_frame(cmux, &frame);
}

static int modem_cmux_dlci_pipe_api_receive(void *data, uint8_t *buf, size_t size)
{
	struct modem_cmux_dlci *dlci = (struct modem_cmux_dlci *)data;
	uint32_t ret;

	k_mutex_lock(&dlci->receive_rb_lock, K_FOREVER);
	ret = ring_buf_get(&dlci->receive_rb, buf, size);
	k_mutex_unlock(&dlci->receive_rb_lock);
	return ret;
}

static int modem_cmux_dlci_pipe_api_close(void *data)
{
	struct modem_cmux_dlci *dlci = (struct modem_cmux_dlci *)data;

	if (k_work_delayable_is_pending(&dlci->close_work) == true) {
		return -EBUSY;
	}

	k_work_schedule(&dlci->close_work, K_NO_WAIT);
	return 0;
}

struct modem_pipe_api modem_cmux_dlci_pipe_api = {
	.open = modem_cmux_dlci_pipe_api_open,
	.transmit = modem_cmux_dlci_pipe_api_transmit,
	.receive = modem_cmux_dlci_pipe_api_receive,
	.close = modem_cmux_dlci_pipe_api_close,
};

static void modem_cmux_dlci_open_handler(struct k_work *item)
{
	struct k_work_delayable *dwork;
	struct modem_cmux_dlci *dlci;

	if (item == NULL) {
		return;
	}

	dwork = k_work_delayable_from_work(item);
	dlci = CONTAINER_OF(dwork, struct modem_cmux_dlci, open_work);

	dlci->state = MODEM_CMUX_DLCI_STATE_OPENING;

	struct modem_cmux_frame frame = {
		.dlci_address = dlci->dlci_address,
		.cr = true,
		.pf = true,
		.type = MODEM_CMUX_FRAME_TYPE_SABM,
		.data = NULL,
		.data_len = 0,
	};

	modem_cmux_transmit_cmd_frame(dlci->cmux, &frame);
	k_work_schedule(&dlci->open_work, MODEM_CMUX_T1_TIMEOUT);
}

static void modem_cmux_dlci_close_handler(struct k_work *item)
{
	struct k_work_delayable *dwork;
	struct modem_cmux_dlci *dlci;
	struct modem_cmux *cmux;

	if (item == NULL) {
		return;
	}

	dwork = k_work_delayable_from_work(item);
	dlci = CONTAINER_OF(dwork, struct modem_cmux_dlci, close_work);
	cmux = dlci->cmux;

	dlci->state = MODEM_CMUX_DLCI_STATE_CLOSING;

	struct modem_cmux_frame frame = {
		.dlci_address = dlci->dlci_address,
		.cr = true,
		.pf = true,
		.type = MODEM_CMUX_FRAME_TYPE_DISC,
		.data = NULL,
		.data_len = 0,
	};

	modem_cmux_transmit_cmd_frame(cmux, &frame);
	k_work_schedule(&dlci->close_work, MODEM_CMUX_T1_TIMEOUT);
}

static void modem_cmux_dlci_pipes_notify_closed(struct modem_cmux *cmux)
{
	sys_snode_t *node;
	struct modem_cmux_dlci *dlci;

	SYS_SLIST_FOR_EACH_NODE(&cmux->dlcis, node) {
		dlci = (struct modem_cmux_dlci *)node;
		modem_pipe_notify_closed(&dlci->pipe);
	}
}

void modem_cmux_init(struct modem_cmux *cmux, const struct modem_cmux_config *config)
{
	__ASSERT_NO_MSG(cmux != NULL);
	__ASSERT_NO_MSG(config != NULL);
	__ASSERT_NO_MSG(config->receive_buf != NULL);
	__ASSERT_NO_MSG(config->receive_buf_size >= 126);
	__ASSERT_NO_MSG(config->transmit_buf != NULL);
	__ASSERT_NO_MSG(config->transmit_buf_size >= 148);

	memset(cmux, 0x00, sizeof(*cmux));
	cmux->callback = config->callback;
	cmux->user_data = config->user_data;
	cmux->receive_buf = config->receive_buf;
	cmux->receive_buf_size = config->receive_buf_size;
	sys_slist_init(&cmux->dlcis);
	cmux->state = MODEM_CMUX_STATE_DISCONNECTED;
	ring_buf_init(&cmux->transmit_rb, config->transmit_buf_size, config->transmit_buf);
	k_mutex_init(&cmux->transmit_rb_lock);
	k_work_init_delayable(&cmux->receive_work, modem_cmux_receive_handler);
	k_work_init_delayable(&cmux->transmit_work, modem_cmux_transmit_handler);
	k_work_init_delayable(&cmux->connect_work, modem_cmux_connect_handler);
	k_work_init_delayable(&cmux->disconnect_work, modem_cmux_disconnect_handler);
	k_event_init(&cmux->event);
	k_event_clear(&cmux->event, MODEM_CMUX_EVENT_CONNECTED_BIT);
	k_event_post(&cmux->event, MODEM_CMUX_EVENT_DISCONNECTED_BIT);
}

struct modem_pipe *modem_cmux_dlci_init(struct modem_cmux *cmux, struct modem_cmux_dlci *dlci,
					const struct modem_cmux_dlci_config *config)
{
	__ASSERT_NO_MSG(cmux != NULL);
	__ASSERT_NO_MSG(dlci != NULL);
	__ASSERT_NO_MSG(config != NULL);
	__ASSERT_NO_MSG(config->dlci_address < 64);
	__ASSERT_NO_MSG(config->receive_buf != NULL);
	__ASSERT_NO_MSG(config->receive_buf_size >= 126);

	memset(dlci, 0x00, sizeof(*dlci));
	dlci->cmux = cmux;
	dlci->dlci_address = config->dlci_address;
	ring_buf_init(&dlci->receive_rb, config->receive_buf_size, config->receive_buf);
	k_mutex_init(&dlci->receive_rb_lock);
	modem_pipe_init(&dlci->pipe, dlci, &modem_cmux_dlci_pipe_api);
	k_work_init_delayable(&dlci->open_work, modem_cmux_dlci_open_handler);
	k_work_init_delayable(&dlci->close_work, modem_cmux_dlci_close_handler);
	dlci->state = MODEM_CMUX_DLCI_STATE_CLOSED;
	sys_slist_append(&dlci->cmux->dlcis, &dlci->node);
	return &dlci->pipe;
}

int modem_cmux_attach(struct modem_cmux *cmux, struct modem_pipe *pipe)
{
	cmux->pipe = pipe;
	ring_buf_reset(&cmux->transmit_rb);
	cmux->receive_state = MODEM_CMUX_RECEIVE_STATE_SOF;
	modem_pipe_attach(cmux->pipe, modem_cmux_bus_callback, cmux);
	return 0;
}

int modem_cmux_connect(struct modem_cmux *cmux)
{
	int ret;

	ret = modem_cmux_connect_async(cmux);
	if (ret < 0) {
		return ret;
	}

	if (k_event_wait(&cmux->event, MODEM_CMUX_EVENT_CONNECTED_BIT, false,
			 MODEM_CMUX_T2_TIMEOUT) == 0) {
		return -EAGAIN;
	}

	return 0;
}

int modem_cmux_connect_async(struct modem_cmux *cmux)
{
	__ASSERT_NO_MSG(cmux->pipe != NULL);

	if (k_event_test(&cmux->event, MODEM_CMUX_EVENT_CONNECTED_BIT)) {
		return -EALREADY;
	}

	if (k_work_delayable_is_pending(&cmux->connect_work) == false) {
		k_work_schedule(&cmux->connect_work, K_NO_WAIT);
	}

	return 0;
}

int modem_cmux_disconnect(struct modem_cmux *cmux)
{
	int ret;

	ret = modem_cmux_disconnect_async(cmux);
	if (ret < 0) {
		return ret;
	}
	if (k_event_wait(&cmux->event, MODEM_CMUX_EVENT_DISCONNECTED_BIT, false,
			 MODEM_CMUX_T2_TIMEOUT) == 0) {
		return -EAGAIN;
	}

	return 0;
}

int modem_cmux_disconnect_async(struct modem_cmux *cmux)
{
	__ASSERT_NO_MSG(cmux->pipe != NULL);

	if (k_event_test(&cmux->event, MODEM_CMUX_EVENT_DISCONNECTED_BIT)) {
		return -EALREADY;
	}

	if (k_work_delayable_is_pending(&cmux->disconnect_work) == false) {
		k_work_schedule(&cmux->disconnect_work, K_NO_WAIT);
	}

	return 0;
}

void modem_cmux_release(struct modem_cmux *cmux)
{
	struct k_work_sync sync;

	/* Close DLCI pipes */
	modem_cmux_dlci_pipes_notify_closed(cmux);

	/* Release bus pipe */
	if (cmux->pipe) {
		modem_pipe_release(cmux->pipe);
	}

	/* Cancel all work */
	k_work_cancel_delayable_sync(&cmux->connect_work, &sync);
	k_work_cancel_delayable_sync(&cmux->disconnect_work, &sync);
	k_work_cancel_delayable_sync(&cmux->transmit_work, &sync);
	k_work_cancel_delayable_sync(&cmux->receive_work, &sync);

	/* Unreference pipe */
	cmux->pipe = NULL;

	/* Reset events */
	k_event_clear(&cmux->event, MODEM_CMUX_EVENT_CONNECTED_BIT);
	k_event_post(&cmux->event, MODEM_CMUX_EVENT_DISCONNECTED_BIT);
}

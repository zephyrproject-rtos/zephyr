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

#include "modem_workqueue.h"

#define MODEM_CMUX_SOF				(0xF9)
#define MODEM_CMUX_FCS_POLYNOMIAL		(0xE0)
#define MODEM_CMUX_FCS_INIT_VALUE		(0xFF)
#define MODEM_CMUX_EA				(0x01)
#define MODEM_CMUX_CR				(0x02)
#define MODEM_CMUX_PF				(0x10)
#define MODEM_CMUX_DATA_SIZE_MIN		8
#define MODEM_CMUX_DATA_FRAME_SIZE_MIN          (MODEM_CMUX_HEADER_SIZE + MODEM_CMUX_DATA_SIZE_MIN)
#define MODEM_CMUX_DATA_FRAME_SIZE_MAX		(MODEM_CMUX_HEADER_SIZE + CONFIG_MODEM_CMUX_MTU)

/* Biggest supported Multiplexer control commands in UIH frame
 * Modem Status Command (MSC) - 5 bytes when Break is included.
 *
 * PN would be 10 bytes, but that is not implemented
 */
#define MODEM_CMUX_CMD_DATA_SIZE_MAX		5 /* Max size of information field of UIH frame */
#define MODEM_CMUX_CMD_HEADER_SIZE		2 /* command type + length */
#define MODEM_CMUX_CMD_FRAME_SIZE_MAX           (MODEM_CMUX_HEADER_SIZE +  \
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
	uint8_t value[MODEM_CMUX_CMD_DATA_SIZE_MAX - 2]; /* Subtract type and length bytes */
};

struct modem_cmux_msc_signals {
	uint8_t ea: 1;    /**< Last octet, always 1 */
	uint8_t fc: 1;    /**< Flow Control */
	uint8_t rtc: 1;   /**< Ready to Communicate */
	uint8_t rtr: 1;   /**< Ready to Transmit */
	uint8_t res_0: 2; /**< Reserved, set to zero */
	uint8_t ic: 1;    /**< Incoming call indicator */
	uint8_t dv: 1;    /**< Data Valid */
};
struct modem_cmux_msc_addr {
	uint8_t ea: 1;                         /**< Last octet, always 1 */
	uint8_t pad_one: 1;                    /**< Set to 1 */
	uint8_t dlci_address: 6;               /**< DLCI channel address */
};

static struct modem_cmux_dlci *modem_cmux_find_dlci(struct modem_cmux *cmux, uint8_t dlci_address);
static void modem_cmux_dlci_notify_transmit_idle(struct modem_cmux *cmux);

static void set_state(struct modem_cmux *cmux, enum modem_cmux_state state)
{
	cmux->state = state;
	k_event_set(&cmux->event, BIT(state));
}

static bool wait_state(struct modem_cmux *cmux, enum modem_cmux_state state, k_timeout_t timeout)
{
	return k_event_wait(&cmux->event, BIT(state), false, timeout) == BIT(state);
}

static bool is_connected(struct modem_cmux *cmux)
{
	return cmux->state == MODEM_CMUX_STATE_CONNECTED;
}

static bool modem_cmux_command_type_is_valid(const struct modem_cmux_command_type type)
{
	/* All commands are only 7 bits, so EA is always set */
	if (type.ea == 0) {
		return false;
	}
	switch (type.value) {
	case MODEM_CMUX_COMMAND_NSC:
	case MODEM_CMUX_COMMAND_TEST:
	case MODEM_CMUX_COMMAND_PSC:
	case MODEM_CMUX_COMMAND_RLS:
	case MODEM_CMUX_COMMAND_FCOFF:
	case MODEM_CMUX_COMMAND_PN:
	case MODEM_CMUX_COMMAND_RPN:
	case MODEM_CMUX_COMMAND_FCON:
	case MODEM_CMUX_COMMAND_CLD:
	case MODEM_CMUX_COMMAND_SNC:
	case MODEM_CMUX_COMMAND_MSC:
		return true;
	default:
		return false;
	}
}

static bool modem_cmux_command_length_is_valid(const struct modem_cmux_command_length length)
{
	/* All commands are shorter than 127 bytes, so EA is always set */
	if (length.ea == 0) {
		return false;
	}
	if (length.value > (MODEM_CMUX_CMD_DATA_SIZE_MAX - MODEM_CMUX_CMD_HEADER_SIZE)) {
		return false;
	}
	return true;
}

static bool modem_cmux_command_is_valid(const struct modem_cmux_command *command)
{
	if (!modem_cmux_command_type_is_valid(command->type)) {
		return false;
	}
	if (!modem_cmux_command_length_is_valid(command->length)) {
		return false;
	}
	/* Verify known value sizes as specified in 3GPP TS 27.010
	 * section 5.4.6.3 Message Type and Actions
	 */
	switch (command->type.value) {
	case MODEM_CMUX_COMMAND_PN:
		return command->length.value == 8;
	case MODEM_CMUX_COMMAND_TEST:
		return (command->length.value > 0 &&
			command->length.value <= MODEM_CMUX_CMD_DATA_SIZE_MAX - 2);
	case MODEM_CMUX_COMMAND_MSC:
		return command->length.value == 2 || command->length.value == 3;
	case MODEM_CMUX_COMMAND_NSC:
		return command->length.value == 1;
	case MODEM_CMUX_COMMAND_RPN:
		return command->length.value == 1 || command->length.value == 8;
	case MODEM_CMUX_COMMAND_RLS:
		return command->length.value == 2;
	case MODEM_CMUX_COMMAND_SNC:
		return command->length.value == 1 || command->length.value == 3;
	default:
		return command->length.value == 0;
	}
}

static struct modem_cmux_command_type modem_cmux_command_type_decode(const uint8_t byte)
{
	struct modem_cmux_command_type type = {
		.ea = (byte & MODEM_CMUX_EA) ? 1 : 0,
		.cr = (byte & MODEM_CMUX_CR) ? 1 : 0,
		.value = (byte >> 2) & 0x3F,
	};

	if (type.ea == 0) {
		return (struct modem_cmux_command_type){0};
	}

	return type;
}

static uint8_t modem_cmux_command_type_encode(const struct modem_cmux_command_type type)
{
	return (type.ea ? MODEM_CMUX_EA : 0) |
	       (type.cr ? MODEM_CMUX_CR : 0) |
	       ((type.value & 0x3F) << 2);
}

static struct modem_cmux_command_length modem_cmux_command_length_decode(const uint8_t byte)
{
	struct modem_cmux_command_length length = {
		.ea = (byte & MODEM_CMUX_EA) ? 1 : 0,
		.value = (byte >> 1) & 0x7F,
	};

	if (length.ea == 0) {
		return (struct modem_cmux_command_length){0};
	}

	return length;
}

static uint8_t modem_cmux_command_length_encode(const struct modem_cmux_command_length length)
{
	return (length.ea ? MODEM_CMUX_EA : 0) |
	       ((length.value & 0x7F) << 1);
}

static struct modem_cmux_command modem_cmux_command_decode(const uint8_t *data, size_t len)
{
	if (len < 2) {
		return (struct modem_cmux_command){0};
	}

	struct modem_cmux_command command = {
		.type = modem_cmux_command_type_decode(data[0]),
		.length = modem_cmux_command_length_decode(data[1]),
	};

	if (command.type.ea == 0 || command.length.ea == 0 ||
	    command.length.value > MODEM_CMUX_CMD_DATA_SIZE_MAX ||
	    (2 + command.length.value) > len) {
		return (struct modem_cmux_command){0};
	}

	memcpy(&command.value[0], &data[2], command.length.value);

	return command;
}

/**
 * @brief Encode command into a shared buffer
 *
 * Not a thread safe, so can only be used within a workqueue context and data
 * must be copied out to a TX ringbuffer.
 *
 * @param command command to encode
 * @param len encoded length of the command is written here
 * @return pointer to encoded command buffer on success, NULL on failure
 */
static uint8_t *modem_cmux_command_encode(struct modem_cmux_command *command, uint16_t *len)
{
	static uint8_t buf[MODEM_CMUX_CMD_DATA_SIZE_MAX];

	__ASSERT_NO_MSG(len != NULL);
	__ASSERT_NO_MSG(command != NULL);
	__ASSERT_NO_MSG(modem_cmux_command_is_valid(command));

	buf[0] = modem_cmux_command_type_encode(command->type);
	buf[1] = modem_cmux_command_length_encode(command->length);
	if (command->length.value > 0) {
		memcpy(&buf[2], &command->value[0], command->length.value);
	}
	*len = 2 + command->length.value;
	return buf;
}

static struct modem_cmux_msc_signals modem_cmux_msc_signals_decode(const uint8_t byte)
{
	struct modem_cmux_msc_signals signals;

	/* 3GPP TS 27.010 MSC signals octet:
	 * |0  |1 |2  |3  |4 |5 |6 |7 |
	 * |EA |FC|RTC|RTR|0 |0 |IC|DV|
	 */
	signals.ea = (byte & BIT(0)) ? 1 : 0;
	signals.fc = (byte & BIT(1)) ? 1 : 0;
	signals.rtc = (byte & BIT(2)) ? 1 : 0;
	signals.rtr = (byte & BIT(3)) ? 1 : 0;
	signals.ic = (byte & BIT(6)) ? 1 : 0;
	signals.dv = (byte & BIT(7)) ? 1 : 0;

	return signals;
}

static uint8_t modem_cmux_msc_signals_encode(const struct modem_cmux_msc_signals signals)
{
	return (signals.ea ? BIT(0) : 0) | (signals.fc ? BIT(1) : 0) |
	       (signals.rtc ? BIT(2) : 0) | (signals.rtr ? BIT(3) : 0) |
	       (signals.ic ? BIT(6) : 0) | (signals.dv ? BIT(7) : 0);
}

static struct modem_cmux_msc_addr modem_cmux_msc_addr_decode(const uint8_t byte)
{
	struct modem_cmux_msc_addr addr;

	/* 3GPP TS 27.010 MSC address octet:
	 * |0  |1 |2 |3 |4 |5 |6 |7 |
	 * |EA |1 |      DLCI       |
	 */
	addr.ea = (byte & BIT(0)) ? 1 : 0;
	addr.pad_one = 1;
	addr.dlci_address = (byte >> 2) & 0x3F;

	return addr;
}

static uint8_t modem_cmux_msc_addr_encode(const struct modem_cmux_msc_addr a)
{
	return (a.ea ? BIT(0) : 0) | BIT(1) |
	       ((a.dlci_address & 0x3F) << 2);
}


#if CONFIG_MODEM_CMUX_LOG_FRAMES
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
	if (hexdump_len > 0) {
		LOG_HEXDUMP_DBG(frame->data, hexdump_len, "data:");
	}
}
#else
#define modem_cmux_log_frame(frame, action, hexdump_len) \
	do { ARG_UNUSED(frame); ARG_UNUSED(action); ARG_UNUSED(hexdump_len); } while (0)
#endif /* CONFIG_MODEM_CMUX_LOG_FRAMES */

static void modem_cmux_log_transmit_frame(const struct modem_cmux_frame *frame)
{
	modem_cmux_log_frame(frame, "tx", frame->data_len);
}

static void modem_cmux_log_received_frame(const struct modem_cmux_frame *frame)
{
	modem_cmux_log_frame(frame, "rcvd", frame->data_len);
}

#if CONFIG_MODEM_STATS
static uint32_t modem_cmux_get_receive_buf_length(struct modem_cmux *cmux)
{
	return cmux->receive_buf_len;
}

static uint32_t modem_cmux_get_receive_buf_size(struct modem_cmux *cmux)
{
	return cmux->receive_buf_size;
}

static uint32_t modem_cmux_get_transmit_buf_length(struct modem_cmux *cmux)
{
	return ring_buf_size_get(&cmux->transmit_rb);
}

static uint32_t modem_cmux_get_transmit_buf_size(struct modem_cmux *cmux)
{
	return ring_buf_capacity_get(&cmux->transmit_rb);
}

static void modem_cmux_init_buf_stats(struct modem_cmux *cmux)
{
	uint32_t size;

	size = modem_cmux_get_receive_buf_size(cmux);
	modem_stats_buffer_init(&cmux->receive_buf_stats, "cmux_rx", size);
	size = modem_cmux_get_transmit_buf_size(cmux);
	modem_stats_buffer_init(&cmux->transmit_buf_stats, "cmux_tx", size);
}

static void modem_cmux_advertise_transmit_buf_stats(struct modem_cmux *cmux)
{
	uint32_t length;

	length = modem_cmux_get_transmit_buf_length(cmux);
	modem_stats_buffer_advertise_length(&cmux->transmit_buf_stats, length);
}

static void modem_cmux_advertise_receive_buf_stats(struct modem_cmux *cmux)
{
	uint32_t length;

	length = modem_cmux_get_receive_buf_length(cmux);
	modem_stats_buffer_advertise_length(&cmux->receive_buf_stats, length);
}
#endif

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
}

static void modem_cmux_log_received_command(const struct modem_cmux_command *command)
{
	LOG_DBG("ea:%u,cr:%u,type:%s", command->type.ea, command->type.cr,
		modem_cmux_command_type_to_str(command->type.value));
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
		modem_work_schedule(&cmux->receive_work, K_NO_WAIT);
		break;

	case MODEM_PIPE_EVENT_TRANSMIT_IDLE:
		modem_work_schedule(&cmux->transmit_work, K_NO_WAIT);
		break;

	default:
		break;
	}
}

static uint16_t modem_cmux_transmit_frame(struct modem_cmux *cmux,
					  const struct modem_cmux_frame *frame)
{
	uint8_t buf[MODEM_CMUX_HEADER_SIZE];
	uint8_t fcs;
	uint16_t space;
	uint16_t data_len;
	uint16_t buf_idx;

	space = ring_buf_space_get(&cmux->transmit_rb) - MODEM_CMUX_HEADER_SIZE;
	data_len = MIN(space, frame->data_len);
	data_len = MIN(data_len, CONFIG_MODEM_CMUX_MTU);

	/* SOF */
	buf[0] = MODEM_CMUX_SOF;

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
	fcs = crc8_rohc(MODEM_CMUX_FCS_INIT_VALUE, &buf[1], (buf_idx - 1));

	/* FCS final */
	if (frame->type == MODEM_CMUX_FRAME_TYPE_UIH) {
		fcs = 0xFF - fcs;
	} else {
		fcs = 0xFF - crc8_rohc(fcs, frame->data, data_len);
	}

	/* Frame header */
	ring_buf_put(&cmux->transmit_rb, buf, buf_idx);

	/* Data */
	ring_buf_put(&cmux->transmit_rb, frame->data, data_len);

	/* FCS and EOF will be put on the same call */
	buf[0] = fcs;
	buf[1] = MODEM_CMUX_SOF;
	ring_buf_put(&cmux->transmit_rb, buf, 2);
	modem_work_schedule(&cmux->transmit_work, K_NO_WAIT);
	return data_len;
}

static bool modem_cmux_transmit_cmd_frame(struct modem_cmux *cmux,
					  const struct modem_cmux_frame *frame)
{
	uint16_t space;
	struct modem_cmux_command command;

	k_mutex_lock(&cmux->transmit_rb_lock, K_FOREVER);
	space = ring_buf_space_get(&cmux->transmit_rb);

	if (space < MODEM_CMUX_CMD_FRAME_SIZE_MAX) {
		k_mutex_unlock(&cmux->transmit_rb_lock);
		LOG_WRN("CMD buffer overflow");
		return false;
	}

	modem_cmux_log_transmit_frame(frame);
	command = modem_cmux_command_decode(frame->data, frame->data_len);
	if (modem_cmux_command_is_valid(&command)) {
		modem_cmux_log_transmit_command(&command);
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
	 * One command frame is reserved for command channel, and we shall prefer
	 * waiting for more than MODEM_CMUX_DATA_FRAME_SIZE_MIN bytes available in the
	 * transmit buffer rather than transmitting a few bytes at a time. This avoids
	 * excessive wrapping overhead, since transmitting a single byte will require 8
	 * bytes of wrapping.
	 */
	if (space < (MODEM_CMUX_CMD_FRAME_SIZE_MAX + MODEM_CMUX_DATA_FRAME_SIZE_MIN)) {
		k_mutex_unlock(&cmux->transmit_rb_lock);
		return 0;
	}

	modem_cmux_log_transmit_frame(frame);
	ret = modem_cmux_transmit_frame(cmux, frame);
	k_mutex_unlock(&cmux->transmit_rb_lock);
	return ret;
}

static void modem_cmux_acknowledge_received_frame(struct modem_cmux *cmux)
{
	struct modem_cmux_command_type command;
	struct modem_cmux_frame frame;
	uint8_t data[MODEM_CMUX_CMD_DATA_SIZE_MAX];

	if (sizeof(data) < cmux->frame.data_len) {
		LOG_WRN("Command acknowledge buffer overrun");
		return;
	}

	memcpy(&frame, &cmux->frame, sizeof(cmux->frame));
	memcpy(data, cmux->frame.data, cmux->frame.data_len);
	command = modem_cmux_command_type_decode(data[0]);
	command.cr = 0;
	data[0] = modem_cmux_command_type_encode(command);
	frame.data = data;
	frame.data_len = cmux->frame.data_len;

	if (modem_cmux_transmit_cmd_frame(cmux, &frame) == false) {
		LOG_WRN("Command acknowledge buffer overrun");
	}
}

static void modem_cmux_send_msc(struct modem_cmux *cmux, struct modem_cmux_dlci *dlci)
{
	if (cmux == NULL || dlci == NULL) {
		return;
	}

	struct modem_cmux_msc_addr addr = {
		.ea = 1,
		.pad_one = 1,
		.dlci_address = dlci->dlci_address,
	};
	struct modem_cmux_msc_signals signals = {
		.ea = 1,
		.fc = dlci->rx_full ? 1 : 0,
		.rtc = dlci->state == MODEM_CMUX_DLCI_STATE_OPEN ? 1 : 0,
		.rtr = dlci->state == MODEM_CMUX_DLCI_STATE_OPEN ? 1 : 0,
		.dv = 1,
	};
	struct modem_cmux_command cmd = {
		.type = {
			.ea = 1,
			.cr = 1,
			.value = MODEM_CMUX_COMMAND_MSC,
		},
		.length = {
			.ea = 1,
			.value = 2,
		},
		.value[0] = modem_cmux_msc_addr_encode(addr),
		.value[1] = modem_cmux_msc_signals_encode(signals),
	};

	uint16_t len;
	uint8_t *data = modem_cmux_command_encode(&cmd, &len);

	if (data == NULL) {
		return;
	}

	struct modem_cmux_frame frame = {
		.dlci_address = 0,
		.cr = cmux->initiator,
		.pf = false,
		.type = MODEM_CMUX_FRAME_TYPE_UIH,
		.data = data,
		.data_len = len,
	};

	LOG_DBG("Sending MSC command for DLCI %u, FC:%d RTR: %d DV: %d", addr.dlci_address,
		signals.fc, signals.rtr, signals.dv);
	modem_cmux_transmit_cmd_frame(cmux, &frame);
}

static void modem_cmux_on_msc_command(struct modem_cmux *cmux, struct modem_cmux_command *command)
{
	if (!command->type.cr) {
		return;
	}

	modem_cmux_acknowledge_received_frame(cmux);

	uint8_t len = command->length.value;

	if (len != 2 && len != 3) {
		LOG_WRN("Unexpected MSC command length %d", (int)len);
		return;
	}

	struct modem_cmux_msc_addr msc = modem_cmux_msc_addr_decode(command->value[0]);
	struct modem_cmux_msc_signals signals = modem_cmux_msc_signals_decode(command->value[1]);
	struct modem_cmux_dlci *dlci = modem_cmux_find_dlci(cmux, msc.dlci_address);

	if (dlci) {
		LOG_DBG("MSC command received for DLCI %u", msc.dlci_address);
		bool fc_signal = signals.fc || !signals.rtr;

		if (fc_signal != dlci->flow_control) {
			if (fc_signal) {
				dlci->flow_control = true;
				LOG_DBG("DLCI %u flow control ON", dlci->dlci_address);
			} else {
				dlci->flow_control = false;
				LOG_DBG("DLCI %u flow control OFF", dlci->dlci_address);
				modem_pipe_notify_transmit_idle(&dlci->pipe);
			}
		}
		/* As we have received MSC, send also our MSC */
		if (!dlci->msc_sent && dlci->state == MODEM_CMUX_DLCI_STATE_OPEN) {
			dlci->msc_sent = true;
			modem_cmux_send_msc(cmux, dlci);
		}
	}
}

static void modem_cmux_on_fcon_command(struct modem_cmux *cmux)
{
	k_mutex_lock(&cmux->transmit_rb_lock, K_FOREVER);
	cmux->flow_control_on = true;
	k_mutex_unlock(&cmux->transmit_rb_lock);
	modem_cmux_acknowledge_received_frame(cmux);
	modem_cmux_dlci_notify_transmit_idle(cmux);
}

static void modem_cmux_on_fcoff_command(struct modem_cmux *cmux)
{
	k_mutex_lock(&cmux->transmit_rb_lock, K_FOREVER);
	cmux->flow_control_on = false;
	k_mutex_unlock(&cmux->transmit_rb_lock);
	modem_cmux_acknowledge_received_frame(cmux);
}

static void disconnect(struct modem_cmux *cmux)
{
	LOG_DBG("CMUX disconnected");
	k_work_cancel_delayable(&cmux->disconnect_work);
	set_state(cmux, MODEM_CMUX_STATE_DISCONNECTED);
	k_mutex_lock(&cmux->transmit_rb_lock, K_FOREVER);
	cmux->flow_control_on = false;
	k_mutex_unlock(&cmux->transmit_rb_lock);
	modem_cmux_raise_event(cmux, MODEM_CMUX_EVENT_DISCONNECTED);
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
		disconnect(cmux);
	} else {
		set_state(cmux, MODEM_CMUX_STATE_DISCONNECTING);
		k_work_schedule(&cmux->disconnect_work, MODEM_CMUX_T1_TIMEOUT);
	}
}

static void modem_cmux_on_control_frame_ua(struct modem_cmux *cmux)
{
	if (cmux->state != MODEM_CMUX_STATE_CONNECTING) {
		LOG_DBG("Unexpected UA frame in state %d", cmux->state);
		return;
	}

	LOG_DBG("CMUX connected");
	set_state(cmux, MODEM_CMUX_STATE_CONNECTED);
	cmux->initiator = true;
	k_mutex_lock(&cmux->transmit_rb_lock, K_FOREVER);
	cmux->flow_control_on = true;
	k_mutex_unlock(&cmux->transmit_rb_lock);
	k_work_cancel_delayable(&cmux->connect_work);
	modem_cmux_raise_event(cmux, MODEM_CMUX_EVENT_CONNECTED);
}

static void modem_cmux_respond_unsupported_cmd(struct modem_cmux *cmux)
{
	struct modem_cmux_frame frame = cmux->frame;
	struct modem_cmux_command cmd = modem_cmux_command_decode(frame.data, frame.data_len);

	if (!modem_cmux_command_is_valid(&cmd)) {
		LOG_WRN("Invalid command");
		return;
	}


	/* 3GPP TS 27.010: 5.4.6.3.8 Non Supported Command Response (NSC) */
	struct modem_cmux_command nsc_cmd = {
		.type = {
			.ea = 1,
			.cr = 0,
			.value = MODEM_CMUX_COMMAND_NSC,
		},
		.length = {
			.ea = 1,
			.value = 1,
		},
		.value[0] = modem_cmux_command_type_encode(cmd.type),
	};

	uint16_t len;
	uint8_t *data = modem_cmux_command_encode(&nsc_cmd, &len);

	if (data == NULL) {
		return;
	}

	frame.data = data;
	frame.data_len = len;

	modem_cmux_transmit_cmd_frame(cmux, &frame);
}

static void modem_cmux_on_control_frame_uih(struct modem_cmux *cmux)
{
	struct modem_cmux_command command;

	if ((cmux->state != MODEM_CMUX_STATE_CONNECTED) &&
	    (cmux->state != MODEM_CMUX_STATE_DISCONNECTING)) {
		LOG_DBG("Unexpected UIH frame");
		return;
	}

	command = modem_cmux_command_decode(cmux->frame.data, cmux->frame.data_len);
	if (!modem_cmux_command_is_valid(&command)) {
		LOG_WRN("Invalid command");
		return;
	}

	modem_cmux_log_received_command(&command);

	if (!command.type.cr) {
		LOG_DBG("Received response command");
		switch (command.type.value) {
		case MODEM_CMUX_COMMAND_CLD:
			modem_cmux_on_cld_command(cmux, &command);
			break;
		default:
			/* Responses to other commands are ignored */
			break;
		}
		return;
	}

	switch (command.type.value) {
	case MODEM_CMUX_COMMAND_CLD:
		modem_cmux_on_cld_command(cmux, &command);
		break;

	case MODEM_CMUX_COMMAND_MSC:
		modem_cmux_on_msc_command(cmux, &command);
		break;

	case MODEM_CMUX_COMMAND_FCON:
		modem_cmux_on_fcon_command(cmux);
		break;

	case MODEM_CMUX_COMMAND_FCOFF:
		modem_cmux_on_fcoff_command(cmux);
		break;

	default:
		LOG_DBG("Unknown control command");
		modem_cmux_respond_unsupported_cmd(cmux);
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

static void modem_cmux_dm_response_transmit(struct modem_cmux *cmux)
{
	if (cmux == NULL) {
		return;
	}

	/* 3GPP TS 27.010: 5.3.3 Disconnected Mode (DM) response */
	struct modem_cmux_frame frame = {
		.dlci_address = cmux->frame.dlci_address,
		.cr = cmux->frame.cr,
		.pf = 1,
		.type = MODEM_CMUX_FRAME_TYPE_DM,
		.data = NULL,
		.data_len = 0,
	};

	LOG_DBG("Send DM response");
	modem_cmux_transmit_cmd_frame(cmux, &frame);
}

static void modem_cmux_on_control_frame_sabm(struct modem_cmux *cmux)
{
	if ((cmux->state == MODEM_CMUX_STATE_CONNECTED) ||
	    (cmux->state == MODEM_CMUX_STATE_DISCONNECTING)) {
		LOG_DBG("Connect request not accepted");
		return;
	}

	LOG_DBG("CMUX connection request received");
	cmux->initiator = false;
	set_state(cmux, MODEM_CMUX_STATE_CONNECTED);
	modem_cmux_connect_response_transmit(cmux);
	k_mutex_lock(&cmux->transmit_rb_lock, K_FOREVER);
	cmux->flow_control_on = true;
	k_mutex_unlock(&cmux->transmit_rb_lock);
	modem_cmux_raise_event(cmux, MODEM_CMUX_EVENT_CONNECTED);
}

static void modem_cmux_on_control_frame(struct modem_cmux *cmux)
{
	modem_cmux_log_received_frame(&cmux->frame);

	if (is_connected(cmux) && cmux->frame.cr == cmux->initiator) {
		LOG_DBG("Received a response frame");
	}

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
		LOG_WRN("Unknown %s frame type %d", "control", cmux->frame.type);
		break;
	}
}

static struct modem_cmux_dlci *modem_cmux_find_dlci(struct modem_cmux *cmux, uint8_t dlci_address)
{
	sys_snode_t *node;
	struct modem_cmux_dlci *dlci;

	SYS_SLIST_FOR_EACH_NODE(&cmux->dlcis, node) {
		dlci = (struct modem_cmux_dlci *)node;

		if (dlci->dlci_address == dlci_address) {
			return dlci;
		}
	}

	return NULL;
}

static void modem_cmux_on_dlci_frame_dm(struct modem_cmux_dlci *dlci)
{
	dlci->state = MODEM_CMUX_DLCI_STATE_CLOSED;
	modem_pipe_notify_closed(&dlci->pipe);
	k_work_cancel_delayable(&dlci->close_work);
}

static void modem_cmux_on_dlci_frame_ua(struct modem_cmux_dlci *dlci)
{
	/* Drop invalid UA frames */
	if (dlci->cmux->frame.cr != dlci->cmux->initiator) {
		LOG_DBG("Received a response frame, dropping");
		return;
	}

	switch (dlci->state) {
	case MODEM_CMUX_DLCI_STATE_OPENING:
		LOG_DBG("DLCI %u opened", dlci->dlci_address);
		dlci->state = MODEM_CMUX_DLCI_STATE_OPEN;
		modem_pipe_notify_opened(&dlci->pipe);
		k_work_cancel_delayable(&dlci->open_work);
		k_mutex_lock(&dlci->receive_rb_lock, K_FOREVER);
		ring_buf_reset(&dlci->receive_rb);
		k_mutex_unlock(&dlci->receive_rb_lock);
		if (dlci->cmux->initiator) {
			modem_cmux_send_msc(dlci->cmux, dlci);
			dlci->msc_sent = true;
		} else {
			dlci->msc_sent = false;
		}
		break;

	case MODEM_CMUX_DLCI_STATE_CLOSING:
		LOG_DBG("DLCI %u closed", dlci->dlci_address);
		modem_cmux_on_dlci_frame_dm(dlci);
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
	bool previous_state = dlci->rx_full;

	if (dlci->state != MODEM_CMUX_DLCI_STATE_OPEN) {
		LOG_DBG("Unexpected UIH frame");
		return;
	}

	k_mutex_lock(&dlci->receive_rb_lock, K_FOREVER);
	written = ring_buf_put(&dlci->receive_rb, cmux->frame.data, cmux->frame.data_len);
	k_mutex_unlock(&dlci->receive_rb_lock);
	if (written < cmux->frame.data_len) {
		LOG_ERR("DLCI %u receive buffer overrun (dropped %u out of %u bytes)",
			dlci->dlci_address, cmux->frame.data_len - written, cmux->frame.data_len);
		dlci->rx_full = true;
	}
	if ((CONFIG_MODEM_CMUX_MSC_FC_THRESHOLD > 0) &&
	    (ring_buf_space_get(&dlci->receive_rb) < CONFIG_MODEM_CMUX_MSC_FC_THRESHOLD)) {
		LOG_WRN("DLCI %u receive buffer low, set flow control", dlci->dlci_address);
		dlci->rx_full = true;
	}

	if (previous_state != dlci->rx_full) {
		modem_cmux_send_msc(cmux, dlci);
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

	LOG_DBG("DLCI %u SABM request accepted, DLCI opened", dlci->dlci_address);
	dlci->state = MODEM_CMUX_DLCI_STATE_OPEN;
	dlci->msc_sent = false;
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
		modem_cmux_dm_response_transmit(cmux);
		return;
	}

	LOG_DBG("DLCI %u disconnected", dlci->dlci_address);
	dlci->state = MODEM_CMUX_DLCI_STATE_CLOSED;
	modem_pipe_notify_closed(&dlci->pipe);
}

static void modem_cmux_on_dlci_frame(struct modem_cmux *cmux)
{
	struct modem_cmux_dlci *dlci;

	modem_cmux_log_received_frame(&cmux->frame);

	if (cmux->state != MODEM_CMUX_STATE_CONNECTED) {
		LOG_DBG("Unexpected DLCI frame in state %d", cmux->state);
		return;
	}

	dlci = modem_cmux_find_dlci(cmux, cmux->frame.dlci_address);
	if (dlci == NULL) {
		LOG_WRN("Frame intended for unconfigured DLCI %u.",
			cmux->frame.dlci_address);
		modem_cmux_dm_response_transmit(cmux);
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
	case MODEM_CMUX_FRAME_TYPE_DM:
		modem_cmux_on_dlci_frame_dm(dlci);
		break;
	default:
		LOG_WRN("Unknown %s frame type (%d, DLCI %d)", "DLCI", cmux->frame.type,
			cmux->frame.dlci_address);
		break;
	}
}

static void modem_cmux_on_frame(struct modem_cmux *cmux)
{
#if CONFIG_MODEM_STATS
	modem_cmux_advertise_receive_buf_stats(cmux);
#endif

	if (cmux->frame.dlci_address == 0) {
		modem_cmux_on_control_frame(cmux);
	} else {
		modem_cmux_on_dlci_frame(cmux);
	}
}

static void modem_cmux_drop_frame(struct modem_cmux *cmux)
{
#if CONFIG_MODEM_STATS
	modem_cmux_advertise_receive_buf_stats(cmux);
#endif

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
		if (byte == MODEM_CMUX_SOF) {
			cmux->receive_state = MODEM_CMUX_RECEIVE_STATE_RESYNC;
			break;
		}

		break;

	case MODEM_CMUX_RECEIVE_STATE_RESYNC:
		/*
		 * Allow any number of consecutive flags (0xF9).
		 * 0xF9 could also be a valid address field for DLCI 62.
		 */
		if (byte == MODEM_CMUX_SOF) {
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

		if (cmux->frame.data_len > CONFIG_MODEM_CMUX_MTU) {
			LOG_ERR("Too large frame");
			modem_cmux_drop_frame(cmux);
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

		if (cmux->frame.data_len > CONFIG_MODEM_CMUX_MTU) {
			LOG_ERR("Too large frame");
			modem_cmux_drop_frame(cmux);
			break;
		}

		if (cmux->frame.data_len > cmux->receive_buf_size) {
			LOG_ERR("Indicated frame data length %u exceeds receive buffer size %u",
				cmux->frame.data_len, cmux->receive_buf_size);

			modem_cmux_drop_frame(cmux);
			break;
		}

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
			modem_cmux_drop_frame(cmux);
			break;
		}

		/* Compute FCS */
		fcs = crc8_rohc(MODEM_CMUX_FCS_INIT_VALUE, cmux->frame_header,
				cmux->frame_header_len);
		if (cmux->frame.type == MODEM_CMUX_FRAME_TYPE_UIH) {
			fcs = 0xFF - fcs;
		} else {
			fcs = 0xFF - crc8_rohc(fcs, cmux->frame.data, cmux->frame.data_len);
		}

		/* Validate FCS */
		if (fcs != byte) {
			LOG_WRN("Frame FCS error");

			modem_cmux_drop_frame(cmux);
			break;
		}

		cmux->receive_state = MODEM_CMUX_RECEIVE_STATE_EOF;
		break;

	case MODEM_CMUX_RECEIVE_STATE_EOF:
		/* Validate byte is EOF */
		if (byte != MODEM_CMUX_SOF) {
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
	modem_work_schedule(&cmux->receive_work, K_NO_WAIT);
}

static void modem_cmux_dlci_notify_transmit_idle(struct modem_cmux *cmux)
{
	sys_snode_t *node;
	struct modem_cmux_dlci *dlci;

	SYS_SLIST_FOR_EACH_NODE(&cmux->dlcis, node) {
		dlci = (struct modem_cmux_dlci *)node;
		if (!dlci->flow_control) {
			modem_pipe_notify_transmit_idle(&dlci->pipe);
		}
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

#if CONFIG_MODEM_STATS
	modem_cmux_advertise_transmit_buf_stats(cmux);
#endif

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

	set_state(cmux, MODEM_CMUX_STATE_CONNECTING);
	cmux->initiator = true;

	static const struct modem_cmux_frame frame = {
		.dlci_address = 0,
		.cr = true,
		.pf = true,
		.type = MODEM_CMUX_FRAME_TYPE_SABM,
		.data = NULL,
		.data_len = 0,
	};

	modem_cmux_transmit_cmd_frame(cmux, &frame);
	modem_work_schedule(&cmux->connect_work, MODEM_CMUX_T1_TIMEOUT);
}

static void modem_cmux_disconnect_handler(struct k_work *item)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(item);
	struct modem_cmux *cmux = CONTAINER_OF(dwork, struct modem_cmux, disconnect_work);

	if (cmux->state == MODEM_CMUX_STATE_DISCONNECTING) {
		disconnect(cmux);
	} else {
		set_state(cmux, MODEM_CMUX_STATE_DISCONNECTING);
		k_work_schedule(&cmux->disconnect_work, MODEM_CMUX_T1_TIMEOUT);
	}

	struct modem_cmux_command command = {
		.type.ea = 1,
		.type.cr = 1,
		.type.value = MODEM_CMUX_COMMAND_CLD,
		.length.ea = 1,
		.length.value = 0,
	};

	uint16_t len;
	uint8_t *data = modem_cmux_command_encode(&command, &len);

	if (data == NULL) {
		return;
	}


	struct modem_cmux_frame frame = {
		.dlci_address = 0,
		.cr = cmux->initiator,
		.pf = false,
		.type = MODEM_CMUX_FRAME_TYPE_UIH,
		.data = data,
		.data_len = len,
	};

	/* Transmit close down command */
	modem_cmux_transmit_cmd_frame(cmux, &frame);
	modem_work_schedule(&cmux->disconnect_work, MODEM_CMUX_T1_TIMEOUT);
}

#if CONFIG_MODEM_STATS
static uint32_t modem_cmux_dlci_get_receive_buf_length(struct modem_cmux_dlci *dlci)
{
	return ring_buf_size_get(&dlci->receive_rb);
}

static uint32_t modem_cmux_dlci_get_receive_buf_size(struct modem_cmux_dlci *dlci)
{
	return ring_buf_capacity_get(&dlci->receive_rb);
}

static void modem_cmux_dlci_init_buf_stats(struct modem_cmux_dlci *dlci)
{
	uint32_t size;
	char name[sizeof("dlci_xxxxx_rx")];

	size = modem_cmux_dlci_get_receive_buf_size(dlci);
	snprintk(name, sizeof(name), "dlci_%u_rx", dlci->dlci_address);
	modem_stats_buffer_init(&dlci->receive_buf_stats, name, size);
}

static void modem_cmux_dlci_advertise_receive_buf_stat(struct modem_cmux_dlci *dlci)
{
	uint32_t length;

	length = modem_cmux_dlci_get_receive_buf_length(dlci);
	modem_stats_buffer_advertise_length(&dlci->receive_buf_stats, length);
}
#endif

static int modem_cmux_dlci_pipe_api_open(void *data)
{
	struct modem_cmux_dlci *dlci = (struct modem_cmux_dlci *)data;
	struct modem_cmux *cmux = dlci->cmux;
	int ret = 0;

	K_SPINLOCK(&cmux->work_lock) {
		if (!cmux->attached) {
			ret = -EPERM;
			K_SPINLOCK_BREAK;
		}

		if (k_work_delayable_is_pending(&dlci->open_work) == true) {
			ret = -EBUSY;
			K_SPINLOCK_BREAK;
		}

		modem_work_schedule(&dlci->open_work, K_NO_WAIT);
	}

	return ret;
}

static int modem_cmux_dlci_pipe_api_transmit(void *data, const uint8_t *buf, size_t size)
{
	struct modem_cmux_dlci *dlci = (struct modem_cmux_dlci *)data;
	struct modem_cmux *cmux = dlci->cmux;
	int ret = 0;

	if (dlci->flow_control) {
		return 0;
	}

	K_SPINLOCK(&cmux->work_lock) {
		if (!cmux->attached) {
			ret = -EPERM;
			K_SPINLOCK_BREAK;
		}

		struct modem_cmux_frame frame = {
			.dlci_address = dlci->dlci_address,
			.cr = cmux->initiator,
			.pf = false,
			.type = MODEM_CMUX_FRAME_TYPE_UIH,
			.data = buf,
			.data_len = size,
		};

		ret = modem_cmux_transmit_data_frame(cmux, &frame);
	}

	return ret;
}

static int modem_cmux_dlci_pipe_api_receive(void *data, uint8_t *buf, size_t size)
{
	struct modem_cmux_dlci *dlci = (struct modem_cmux_dlci *)data;
	uint32_t ret;

	k_mutex_lock(&dlci->receive_rb_lock, K_FOREVER);

#if CONFIG_MODEM_STATS
	modem_cmux_dlci_advertise_receive_buf_stat(dlci);
#endif

	ret = ring_buf_get(&dlci->receive_rb, buf, size);
	k_mutex_unlock(&dlci->receive_rb_lock);
	/* Release FC if set */
	if (dlci->rx_full &&
	    ring_buf_space_get(&dlci->receive_rb) >= CONFIG_MODEM_CMUX_MTU) {
		LOG_DBG("DLCI %u receive buffer is no longer full", dlci->dlci_address);
		dlci->rx_full = false;
		modem_cmux_send_msc(dlci->cmux, dlci);
	}

	return ret;
}

static int modem_cmux_dlci_pipe_api_close(void *data)
{
	struct modem_cmux_dlci *dlci = (struct modem_cmux_dlci *)data;
	struct modem_cmux *cmux = dlci->cmux;
	int ret = 0;

	K_SPINLOCK(&cmux->work_lock) {
		if (!cmux->attached) {
			ret = -EPERM;
			K_SPINLOCK_BREAK;
		}

		if (k_work_delayable_is_pending(&dlci->close_work) == true) {
			ret = -EBUSY;
			K_SPINLOCK_BREAK;
		}

		modem_work_schedule(&dlci->close_work, K_NO_WAIT);
	}

	return ret;
}

static const struct modem_pipe_api modem_cmux_dlci_pipe_api = {
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
	dlci->msc_sent = false;

	struct modem_cmux_frame frame = {
		.dlci_address = dlci->dlci_address,
		.cr = dlci->cmux->initiator,
		.pf = true,
		.type = MODEM_CMUX_FRAME_TYPE_SABM,
		.data = NULL,
		.data_len = 0,
	};

	LOG_DBG("Opening: %d", dlci->dlci_address);
	modem_cmux_transmit_cmd_frame(dlci->cmux, &frame);
	modem_work_schedule(&dlci->open_work, MODEM_CMUX_T1_TIMEOUT);
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
		.cr = dlci->cmux->initiator,
		.pf = true,
		.type = MODEM_CMUX_FRAME_TYPE_DISC,
		.data = NULL,
		.data_len = 0,
	};

	LOG_DBG("Closing: %d", dlci->dlci_address);
	modem_cmux_transmit_cmd_frame(cmux, &frame);
	modem_work_schedule(&dlci->close_work, MODEM_CMUX_T1_TIMEOUT);
}

static void modem_cmux_dlci_pipes_release(struct modem_cmux *cmux)
{
	sys_snode_t *node;
	struct modem_cmux_dlci *dlci;
	struct k_work_sync sync;

	SYS_SLIST_FOR_EACH_NODE(&cmux->dlcis, node) {
		dlci = (struct modem_cmux_dlci *)node;
		modem_pipe_notify_closed(&dlci->pipe);
		k_work_cancel_delayable_sync(&dlci->open_work, &sync);
		k_work_cancel_delayable_sync(&dlci->close_work, &sync);
	}
}

void modem_cmux_init(struct modem_cmux *cmux, const struct modem_cmux_config *config)
{
	__ASSERT_NO_MSG(cmux != NULL);
	__ASSERT_NO_MSG(config != NULL);
	__ASSERT_NO_MSG(config->receive_buf != NULL);
	__ASSERT_NO_MSG(config->receive_buf_size >= MODEM_CMUX_DATA_FRAME_SIZE_MAX);
	__ASSERT_NO_MSG(config->transmit_buf != NULL);
	__ASSERT_NO_MSG(config->transmit_buf_size >= MODEM_CMUX_DATA_FRAME_SIZE_MAX);

	memset(cmux, 0x00, sizeof(*cmux));
	cmux->callback = config->callback;
	cmux->user_data = config->user_data;
	cmux->receive_buf = config->receive_buf;
	cmux->receive_buf_size = config->receive_buf_size;
	sys_slist_init(&cmux->dlcis);
	ring_buf_init(&cmux->transmit_rb, config->transmit_buf_size, config->transmit_buf);
	k_mutex_init(&cmux->transmit_rb_lock);
	k_work_init_delayable(&cmux->receive_work, modem_cmux_receive_handler);
	k_work_init_delayable(&cmux->transmit_work, modem_cmux_transmit_handler);
	k_work_init_delayable(&cmux->connect_work, modem_cmux_connect_handler);
	k_work_init_delayable(&cmux->disconnect_work, modem_cmux_disconnect_handler);
	k_event_init(&cmux->event);
	set_state(cmux, MODEM_CMUX_STATE_DISCONNECTED);

#if CONFIG_MODEM_STATS
	modem_cmux_init_buf_stats(cmux);
#endif
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

#if CONFIG_MODEM_STATS
	modem_cmux_dlci_init_buf_stats(dlci);
#endif

	return &dlci->pipe;
}

int modem_cmux_attach(struct modem_cmux *cmux, struct modem_pipe *pipe)
{
	if (cmux->pipe != NULL) {
		return -EALREADY;
	}

	cmux->pipe = pipe;
	ring_buf_reset(&cmux->transmit_rb);
	cmux->receive_state = MODEM_CMUX_RECEIVE_STATE_SOF;
	modem_pipe_attach(cmux->pipe, modem_cmux_bus_callback, cmux);

	K_SPINLOCK(&cmux->work_lock) {
		cmux->attached = true;
	}

	return 0;
}

int modem_cmux_connect(struct modem_cmux *cmux)
{
	int ret;

	ret = modem_cmux_connect_async(cmux);
	if (ret < 0) {
		return ret;
	}

	if (!wait_state(cmux, MODEM_CMUX_STATE_CONNECTED, MODEM_CMUX_T2_TIMEOUT)) {
		return -EAGAIN;
	}

	return 0;
}

int modem_cmux_connect_async(struct modem_cmux *cmux)
{
	int ret = 0;

	if (cmux->state != MODEM_CMUX_STATE_DISCONNECTED) {
		return -EALREADY;
	}

	K_SPINLOCK(&cmux->work_lock) {
		if (!cmux->attached) {
			ret = -EPERM;
			K_SPINLOCK_BREAK;
		}

		if (k_work_delayable_is_pending(&cmux->connect_work) == false) {
			modem_work_schedule(&cmux->connect_work, K_NO_WAIT);
		}
	}

	return ret;
}

int modem_cmux_disconnect(struct modem_cmux *cmux)
{
	int ret;

	ret = modem_cmux_disconnect_async(cmux);
	if (ret < 0) {
		return ret;
	}

	if (!wait_state(cmux, MODEM_CMUX_STATE_DISCONNECTED, MODEM_CMUX_T2_TIMEOUT)) {
		return -EAGAIN;
	}

	return 0;
}

int modem_cmux_disconnect_async(struct modem_cmux *cmux)
{
	int ret = 0;

	if (cmux->state == MODEM_CMUX_STATE_DISCONNECTED) {
		return -EALREADY;
	}

	K_SPINLOCK(&cmux->work_lock) {
		if (!cmux->attached) {
			ret = -EPERM;
			K_SPINLOCK_BREAK;
		}

		if (k_work_delayable_is_pending(&cmux->disconnect_work) == false) {
			modem_work_schedule(&cmux->disconnect_work, K_NO_WAIT);
		}
	}

	return ret;
}

void modem_cmux_release(struct modem_cmux *cmux)
{
	struct k_work_sync sync;

	if (cmux->pipe == NULL) {
		return;
	}

	K_SPINLOCK(&cmux->work_lock) {
		cmux->attached = false;
	}

	/* Close DLCI pipes and cancel DLCI work */
	modem_cmux_dlci_pipes_release(cmux);

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

	/* Reset state */
	set_state(cmux, MODEM_CMUX_STATE_DISCONNECTED);
}

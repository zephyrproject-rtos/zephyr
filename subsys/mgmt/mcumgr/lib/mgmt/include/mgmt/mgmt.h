/*
 * Copyright (c) 2018-2021 mcumgr authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_MGMT_MGMT_
#define H_MGMT_MGMT_

#include <inttypes.h>
#include <zephyr/mgmt/mcumgr/buf.h>

#ifdef __cplusplus
extern "C" {
#endif

/* MTU for newtmgr responses */
#define MGMT_MAX_MTU		1024

/** Opcodes; encoded in first byte of header. */
#define MGMT_OP_READ		0
#define MGMT_OP_READ_RSP	1
#define MGMT_OP_WRITE		2
#define MGMT_OP_WRITE_RSP	3

/**
 * The first 64 groups are reserved for system level mcumgr commands.
 * Per-user commands are then defined after group 64.
 */
#define MGMT_GROUP_ID_OS	0
#define MGMT_GROUP_ID_IMAGE	1
#define MGMT_GROUP_ID_STAT	2
#define MGMT_GROUP_ID_CONFIG	3
#define MGMT_GROUP_ID_LOG	4
#define MGMT_GROUP_ID_CRASH	5
#define MGMT_GROUP_ID_SPLIT	6
#define MGMT_GROUP_ID_RUN	7
#define MGMT_GROUP_ID_FS	8
#define MGMT_GROUP_ID_SHELL	9
#define MGMT_GROUP_ID_PERUSER	64

/**
 * mcumgr error codes.
 */
#define MGMT_ERR_EOK		0
#define MGMT_ERR_EUNKNOWN	1
#define MGMT_ERR_ENOMEM		2
#define MGMT_ERR_EINVAL		3
#define MGMT_ERR_ETIMEOUT	4
#define MGMT_ERR_ENOENT		5
#define MGMT_ERR_EBADSTATE	6	/* Current state disallows command. */
#define MGMT_ERR_EMSGSIZE	7	/* Response too large. */
#define MGMT_ERR_ENOTSUP	8	/* Command not supported. */
#define MGMT_ERR_ECORRUPT	9	/* Corrupt */
#define MGMT_ERR_EBUSY		10	/* Command blocked by processing of other command */
#define MGMT_ERR_EPERUSER	256

#define MGMT_HDR_SIZE		8

/*
 * MGMT event opcodes.
 */
#define MGMT_EVT_OP_CMD_RECV	0x01
#define MGMT_EVT_OP_CMD_STATUS	0x02
#define MGMT_EVT_OP_CMD_DONE	0x03

struct mgmt_hdr {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	uint8_t  nh_op:3;		/* MGMT_OP_[...] */
	uint8_t  _res1:5;
#endif
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	uint8_t  _res1:5;
	uint8_t  nh_op:3;		/* MGMT_OP_[...] */
#endif
	uint8_t  nh_flags;		/* Reserved for future flags */
	uint16_t nh_len;		/* Length of the payload */
	uint16_t nh_group;		/* MGMT_GROUP_ID_[...] */
	uint8_t  nh_seq;		/* Sequence number */
	uint8_t  nh_id;			/* Message ID within group */
};

#define nmgr_hdr mgmt_hdr

/*
 * MGMT_EVT_OP_CMD_STATUS argument
 */
struct mgmt_evt_op_cmd_status_arg {
	int status;
};

/*
 * MGMT_EVT_OP_CMD_DONE argument
 */
struct mgmt_evt_op_cmd_done_arg {
	int err;			/* MGMT_ERR_[...] */
};

/** @typedef mgmt_on_evt_cb
 * @brief Function to be called on MGMT event.
 *
 * This callback function is used to notify application about mgmt event.
 *
 * @param opcode	MGMT_EVT_OP_[...].
 * @param group		MGMT_GROUP_ID_[...].
 * @param id		Message ID within group.
 * @param arg		Optional event argument.
 */
typedef void (*mgmt_on_evt_cb)(uint8_t opcode, uint16_t group, uint8_t id, void *arg);

/** @typedef mgmt_alloc_rsp_fn
 * @brief Allocates a buffer suitable for holding a response.
 *
 * If a source buf is provided, its user data is copied into the new buffer.
 *
 * @param src_buf	An optional source buffer to copy user data from.
 * @param arg		Optional streamer argument.
 *
 * @return Newly-allocated buffer on success NULL on failure.
 */
typedef void *(*mgmt_alloc_rsp_fn)(const void *src_buf, void *arg);

/** @typedef mgmt_reset_buf_fn
 * @brief Resets a buffer to a length of 0.
 *
 * The buffer's user data remains, but its payload is cleared.
 *
 * @param buf	The buffer to reset.
 * @param arg	Optional streamer argument.
 */
typedef void (*mgmt_reset_buf_fn)(void *buf, void *arg);

/** @typedef mgmt_write_at_fn
 * @brief Writes header at the beginning of buffer
 *
 * Overwrites beginning of buffer with header; moves buffer data pointer so that next
 * non-header writes would happen after header.
 *
 * @param writer	The encoder to write to.
 * @param hdr		Header to write (struct mgmt_hdr);
 *
 * @return 0 on success, MGMT_ERR_[...] code on failure.
 */
typedef int (*mgmt_write_hdr_fn)(struct cbor_nb_writer *writer, const struct mgmt_hdr *hdr);

/** @typedef mgmt_init_writer_fn
 * @brief Frees the specified buffer.
 *
 * @param buf	The buffer to free.
 * @param arg	Optional streamer argument.
 */
typedef void (*mgmt_free_buf_fn)(void *buf, void *arg);

/**
 * @brief Configuration for constructing a mgmt_streamer object.
 */
struct mgmt_streamer_cfg {
	mgmt_alloc_rsp_fn alloc_rsp;
	mgmt_write_hdr_fn write_hdr;
	mgmt_free_buf_fn free_buf;
};

/**
 * @brief Decodes requests and encodes responses for any mcumgr protocol.
 */
struct mgmt_streamer {
	const struct mgmt_streamer_cfg *cfg;
	void *cb_arg;
	struct cbor_nb_reader *reader;
	struct cbor_nb_writer *writer;
};

/**
 * @brief Context required by command handlers for parsing requests and writing
 *		responses.
 */
struct mgmt_ctxt {
	struct cbor_nb_writer *cnbe;
	struct cbor_nb_reader *cnbd;
#ifdef CONFIG_MGMT_VERBOSE_ERR_RESPONSE
	const char *rc_rsn;
#endif
};

#ifdef CONFIG_MGMT_VERBOSE_ERR_RESPONSE
#define MGMT_CTXT_SET_RC_RSN(mc, rsn) ((mc->rc_rsn) = (rsn))
#define MGMT_CTXT_RC_RSN(mc) ((mc)->rc_rsn)
#else
#define MGMT_CTXT_SET_RC_RSN(mc, rsn)
#define MGMT_CTXT_RC_RSN(mc) NULL
#endif

/** @typedef mgmt_handler_fn
 * @brief Processes a request and writes the corresponding response.
 *
 * A separate handler is required for each supported op-ID pair.
 *
 * @param ctxt	The mcumgr context to use.
 *
 * @return 0 if a response was successfully encoded, MGMT_ERR_[...] code on failure.
 */
typedef int (*mgmt_handler_fn)(struct mgmt_ctxt *ctxt);

/**
 * @brief Read handler and write handler for a single command ID.
 */
struct mgmt_handler {
	mgmt_handler_fn mh_read;
	mgmt_handler_fn mh_write;
};

/**
 * @brief A collection of handlers for an entire command group.
 */
struct mgmt_group {
	/** Points to the next group in the list. */
	struct mgmt_group *mg_next;

	/** Array of handlers; one entry per command ID. */
	const struct mgmt_handler *mg_handlers;
	uint16_t mg_handlers_count;

	/* The numeric ID of this group. */
	uint16_t mg_group_id;
};

/**
 * @brief Uses the specified streamer to allocates a response buffer.
 *
 * If a source buf is provided, its user data is copied into the new buffer.
 *
 * @param streamer	The streamer providing the callback.
 * @param src_buf	An optional source buffer to copy user data from.
 *
 * @return	Newly-allocated buffer on success
 *		NULL on failure.
 */
void *mgmt_streamer_alloc_rsp(struct mgmt_streamer *streamer, const void *src_buf);

/**
 * @brief Uses the specified streamer to trim data from the front of a buffer.
 *
 * If the amount to trim exceeds the size of the buffer, the buffer is
 * truncated to a length of 0.
 *
 * @param streamer	The streamer providing the callback.
 * @param buf		The buffer to trim.
 * @param len		The number of bytes to remove.
 */
void mgmt_streamer_trim_front(struct mgmt_streamer *streamer, void *buf, size_t len);

/**
 * @brief Uses the specified streamer to write header to buffer.
 *
 * Any existing data at the beginning buffer will be overwritten with header.
 * Any new data that extends past the buffer's current length is appended.
 *
 * @param streamer	The streamer providing the callback.
 * @param writer	The encoder to write to.
 * @param hdr		The mgmt_hdr struct to write.
 *
 * @return 0 on success, MGMT_ERR_[...] code on failure.
 */
int mgmt_streamer_write_hdr(struct mgmt_streamer *streamer, const struct mgmt_hdr *hdr);

/**
 * @brief Uses the specified streamer to initialize a CBOR reader.
 *
 * @param streamer	The streamer providing the callback.
 * @param reader	The reader to initialize.
 * @param buf		The buffer to configure the reader with.
 *
 * @return 0 on success, MGMT_ERR_[...] code on failure.
 */
int mgmt_streamer_init_reader(struct mgmt_streamer *streamer, void *buf);

/**
 * @brief Uses the specified streamer to free a buffer.
 *
 * @param streamer	The streamer providing the callback.
 * @param buf		The buffer to free.
 */
void mgmt_streamer_free_buf(struct mgmt_streamer *streamer, void *buf);

/**
 * @brief Registers a full command group.
 *
 * @param group The group to register.
 */
void mgmt_register_group(struct mgmt_group *group);

/**
 * @brief Unregisters a full command group.
 *
 * @param group	The group to register.
 */
void mgmt_unregister_group(struct mgmt_group *group);

/**
 * @brief Finds a registered command handler.
 *
 * @param group_id	The group of the command to find.
 * @param command_id	The ID of the command to find.
 *
 * @return	The requested command handler on success;
 *		NULL on failure.
 */
const struct mgmt_handler *mgmt_find_handler(uint16_t group_id, uint16_t command_id);

/**
 * @brief Encodes a response status into the specified management context.
 *
 * @param ctxt		The management context to encode into.
 * @param status	The response status to write.
 *
 * @return 0 on success, MGMT_ERR_[...] code on failure.
 */
int mgmt_write_rsp_status(struct mgmt_ctxt *ctxt, int status);

/**
 * @brief Byte-swaps an mcumgr header from network to host byte order.
 *
 * @param hdr The mcumgr header to byte-swap.
 */
void mgmt_ntoh_hdr(struct mgmt_hdr *hdr);

/**
 * @brief Byte-swaps an mcumgr header from host to network byte order.
 *
 * @param hdr The mcumgr header to byte-swap.
 */
void mgmt_hton_hdr(struct mgmt_hdr *hdr);

/**
 * @brief Register event callback function.
 *
 * @param cb Callback function.
 */
void mgmt_register_evt_cb(mgmt_on_evt_cb cb);

/**
 * @brief This function is called to notify about mgmt event.
 *
 * @param opcode	MGMT_EVT_OP_[...].
 * @param group		MGMT_GROUP_ID_[...].
 * @param id		Message ID within group.
 * @param arg		Optional event argument.
 */
void mgmt_evt(uint8_t opcode, uint16_t group, uint8_t id, void *arg);

#ifdef __cplusplus
}
#endif

#endif /* MGMT_MGMT_H_ */

/** @file
 *  @brief Service Discovery Protocol handling.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <sys/types.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>

#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/classic/sdp.h>

#include "common/bt_str.h"
#include "common/assert.h"

#include "host/hci_core.h"
#include "host/conn_internal.h"
#include "l2cap_br_internal.h"
#include "sdp_internal.h"

#define LOG_LEVEL CONFIG_BT_SDP_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_sdp);

#define SDP_PSM 0x0001

#define SDP_CHAN(_ch) CONTAINER_OF(_ch, struct bt_sdp, chan.chan)

#define SDP_DATA_MTU 200

#define SDP_MTU (SDP_DATA_MTU + sizeof(struct bt_sdp_hdr))

#define MAX_NUM_ATT_ID_FILTER 10

#define SDP_SERVICE_HANDLE_BASE 0x10000

#define SDP_DATA_ELEM_NEST_LEVEL_MAX 5

/* Size of Cont state length */
#define SDP_CONT_STATE_LEN_SIZE 1

/* 1 byte for the no. of services searched till this response */
/* 2 bytes for the total no. of matching records */
#define SDP_SS_CONT_STATE_SIZE 3

/* 1 byte for the no. of attributes searched till this response */
#define SDP_SA_CONT_STATE_SIZE 1

/* 1 byte for the no. of services searched till this response */
/* 1 byte for the no. of attributes searched till this response */
#define SDP_SSA_CONT_STATE_SIZE 2

#define SDP_INVALID 0xff

/* SDP record handle size */
#define SDP_RECORD_HANDLE_SIZE 4

struct bt_sdp {
	struct bt_l2cap_br_chan chan;
	/* TODO: Allow more than one pending request */
};

static struct bt_sdp_record *db;
static uint8_t num_services;

static struct bt_sdp bt_sdp_pool[CONFIG_BT_MAX_CONN];

/* Pool for outgoing SDP packets */
NET_BUF_POOL_FIXED_DEFINE(sdp_pool, CONFIG_BT_MAX_CONN, BT_L2CAP_BUF_SIZE(SDP_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

#define SDP_CLIENT_CHAN(_ch) CONTAINER_OF(_ch, struct bt_sdp_client, chan.chan)

#define SDP_CLIENT_MTU 64

enum sdp_client_state {
	SDP_CLIENT_RELEASED,
	SDP_CLIENT_CONNECTING,
	SDP_CLIENT_CONNECTED,
	SDP_CLIENT_DISCONNECTING,
};

struct bt_sdp_client {
	/* semaphore for lock/unlock */
	struct k_sem                         sem_lock;
	struct bt_l2cap_br_chan              chan;
	/* list of waiting to create sdp connection again */
	sys_slist_t                          reqs_next;
	/* list of waiting to be resolved UUID params */
	sys_slist_t                          reqs;
	/* required SDP transaction ID */
	uint16_t                                tid;
	/* UUID params holder being now resolved */
	const struct bt_sdp_discover_params *param;
	/* PDU continuation state object */
	struct bt_sdp_pdu_cstate             cstate;
	/* buffer for collecting record data */
	struct net_buf                      *rec_buf;
	/* The total length of response */
	uint32_t                             total_len;
	/* Received data length */
	uint32_t                             recv_len;
	/* client state */
	enum sdp_client_state                state;
};

static struct bt_sdp_client bt_sdp_client_pool[CONFIG_BT_MAX_CONN];

enum {
	BT_SDP_ITER_STOP,
	BT_SDP_ITER_CONTINUE,
};

struct search_state {
	uint16_t att_list_size;
	uint8_t  current_svc;
	uint8_t  last_att;
	bool     pkt_full;
};

struct select_attrs_data {
	struct bt_sdp_record        *rec;
	struct net_buf              *rsp_buf;
	struct bt_sdp               *sdp;
	struct bt_sdp_data_elem_seq *seq;
	struct search_state         *state;
	uint32_t                       *filter;
	uint16_t                        max_att_len;
	uint16_t                        att_list_len;
	uint8_t                         cont_state_size;
	size_t                          num_filters;
	bool                         new_service;
};

/* @typedef bt_sdp_attr_func_t
 *  @brief SDP attribute iterator callback.
 *
 *  @param attr Attribute found.
 *  @param att_idx Index of the found attribute in the attribute database.
 *  @param user_data Data given.
 *
 *  @return BT_SDP_ITER_CONTINUE if should continue to the next attribute
 *  or BT_SDP_ITER_STOP to stop.
 */
typedef uint8_t (*bt_sdp_attr_func_t)(struct bt_sdp_attribute *attr,
				   uint8_t att_idx, void *user_data);

/* @typedef bt_sdp_svc_func_t
 * @brief SDP service record iterator callback.
 *
 * @param rec Service record found.
 * @param user_data Data given.
 *
 * @return BT_SDP_ITER_CONTINUE if should continue to the next service record
 *  or BT_SDP_ITER_STOP to stop.
 */
typedef uint8_t (*bt_sdp_svc_func_t)(struct bt_sdp_record *rec,
				  void *user_data);

static int sdp_client_new_session(struct bt_conn *conn, struct bt_sdp_client *session);

/* @brief Callback for SDP connection
 *
 *  Gets called when an SDP connection is established
 *
 *  @param chan L2CAP channel
 *
 *  @return None
 */
static void bt_sdp_connected(struct bt_l2cap_chan *chan)
{
	struct bt_l2cap_br_chan *ch = CONTAINER_OF(chan,
						   struct bt_l2cap_br_chan,
						   chan);

	struct bt_sdp *sdp __unused = CONTAINER_OF(ch, struct bt_sdp, chan);

	LOG_DBG("chan %p cid 0x%04x", ch, ch->tx.cid);
}

/** @brief Callback for SDP disconnection
 *
 *  Gets called when an SDP connection is terminated
 *
 *  @param chan L2CAP channel
 *
 *  @return None
 */
static void bt_sdp_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_l2cap_br_chan *ch = CONTAINER_OF(chan,
						   struct bt_l2cap_br_chan,
						   chan);

	struct bt_sdp *sdp __unused = CONTAINER_OF(ch, struct bt_sdp, chan);

	LOG_DBG("chan %p cid 0x%04x", ch, ch->tx.cid);
}

/* @brief Creates an SDP PDU
 *
 *  Creates an empty SDP PDU and returns the buffer
 *
 *  @param None
 *
 *  @return Pointer to the net_buf buffer
 */
static struct net_buf *bt_sdp_create_pdu(void)
{
	return bt_l2cap_create_pdu(&sdp_pool, sizeof(struct bt_sdp_hdr));
}

/* @brief Sends out an SDP PDU
 *
 *  Sends out an SDP PDU after adding the relevant header
 *
 *  @param chan L2CAP channel
 *  @param buf Buffer to be sent out
 *  @param op Opcode to be used in the packet header
 *  @param tid Transaction ID to be used in the packet header
 *
 *  @return None
 */
static int bt_sdp_send(struct bt_l2cap_chan *chan, struct net_buf *buf,
		       uint8_t op, uint16_t tid)
{
	struct bt_sdp_hdr *hdr;
	uint16_t param_len = buf->len;
	int err;

	hdr = net_buf_push(buf, sizeof(struct bt_sdp_hdr));
	hdr->op_code = op;
	hdr->tid = sys_cpu_to_be16(tid);
	hdr->param_len = sys_cpu_to_be16(param_len);

	err = bt_l2cap_chan_send(chan, buf);
	if (err < 0) {
		net_buf_unref(buf);
	}

	return err;
}

/* @brief Sends an error response PDU
 *
 *  Creates and sends an error response PDU
 *
 *  @param chan L2CAP channel
 *  @param err Error code to be sent in the packet
 *  @param tid Transaction ID to be used in the packet header
 *
 *  @return None
 */
static void send_err_rsp(struct bt_l2cap_chan *chan, uint16_t err,
			 uint16_t tid)
{
	struct net_buf *buf;

	LOG_DBG("tid %u, error %u", tid, err);

	buf = bt_sdp_create_pdu();

	net_buf_add_be16(buf, err);

	bt_sdp_send(chan, buf, BT_SDP_ERROR_RSP, tid);
}

/* @brief Parses data elements from a net_buf
 *
 * Parses the first data element from a buffer and splits it into type, size,
 * data. Used for parsing incoming requests. Net buf is advanced to the data
 * part of the element.
 *
 * @param buf Buffer to be advanced
 * @param data_elem Pointer to the parsed data element structure
 *
 * @return 0 for success, or relevant error code
 */
static uint16_t parse_data_elem(struct net_buf *buf,
				struct bt_sdp_data_elem *data_elem)
{
	uint8_t size_field_len = 0U; /* Space used to accommodate the size */

	if (buf->len < 1) {
		LOG_WRN("Malformed packet");
		return BT_SDP_INVALID_SYNTAX;
	}

	data_elem->type = net_buf_pull_u8(buf);

	switch (data_elem->type & BT_SDP_TYPE_DESC_MASK) {
	case BT_SDP_UINT8:
	case BT_SDP_INT8:
	case BT_SDP_UUID_UNSPEC:
	case BT_SDP_BOOL:
		data_elem->data_size = BIT(data_elem->type &
					   BT_SDP_SIZE_DESC_MASK);
		break;
	case BT_SDP_TEXT_STR_UNSPEC:
	case BT_SDP_SEQ_UNSPEC:
	case BT_SDP_ALT_UNSPEC:
	case BT_SDP_URL_STR_UNSPEC:
		size_field_len = BIT((data_elem->type & BT_SDP_SIZE_DESC_MASK) -
				     BT_SDP_SIZE_INDEX_OFFSET);
		if (buf->len < size_field_len) {
			LOG_WRN("Malformed packet");
			return BT_SDP_INVALID_SYNTAX;
		}
		switch (size_field_len) {
		case 1:
			data_elem->data_size = net_buf_pull_u8(buf);
			break;
		case 2:
			data_elem->data_size = net_buf_pull_be16(buf);
			break;
		case 4:
			data_elem->data_size = net_buf_pull_be32(buf);
			break;
		default:
			LOG_WRN("Invalid size in remote request");
			return BT_SDP_INVALID_SYNTAX;
		}
		break;
	default:
		LOG_WRN("Invalid type in remote request");
		return BT_SDP_INVALID_SYNTAX;
	}

	if (buf->len < data_elem->data_size) {
		LOG_WRN("Malformed packet");
		return BT_SDP_INVALID_SYNTAX;
	}

	data_elem->total_size = data_elem->data_size + size_field_len + 1;
	data_elem->data = buf->data;

	return 0;
}

/* @brief Searches for an UUID within an attribute
 *
 * Searches for an UUID within an attribute. If the attribute has data element
 * sequences, it recursively searches within them as well. On finding a match
 * with the UUID, it sets the found flag.
 *
 * @param elem Attribute to be used as the search space (haystack)
 * @param uuid UUID to be looked for (needle)
 * @param found Flag set to true if the UUID is found (to be returned)
 * @param nest_level Used to limit the extent of recursion into nested data
 *  elements, to avoid potential stack overflows
 *
 * @return Size of the last data element that has been searched
 *  (used in recursion)
 */
static uint32_t search_uuid(struct bt_sdp_data_elem *elem, struct bt_uuid *uuid,
			 bool *found, uint8_t nest_level)
{
	const uint8_t *cur_elem;
	uint32_t seq_size, size;
	union {
		struct bt_uuid uuid;
		struct bt_uuid_16 u16;
		struct bt_uuid_32 u32;
		struct bt_uuid_128 u128;
	} u;

	if (*found) {
		return 0;
	}

	/* Limit recursion depth to avoid stack overflows */
	if (nest_level == SDP_DATA_ELEM_NEST_LEVEL_MAX) {
		return 0;
	}

	seq_size = elem->data_size;
	cur_elem = elem->data;

	if ((elem->type & BT_SDP_TYPE_DESC_MASK) == BT_SDP_UUID_UNSPEC) {
		if (seq_size == 2U) {
			u.uuid.type = BT_UUID_TYPE_16;
			u.u16.val = *((uint16_t *)cur_elem);
			if (!bt_uuid_cmp(&u.uuid, uuid)) {
				*found = true;
			}
		} else if (seq_size == 4U) {
			u.uuid.type = BT_UUID_TYPE_32;
			u.u32.val = *((uint32_t *)cur_elem);
			if (!bt_uuid_cmp(&u.uuid, uuid)) {
				*found = true;
			}
		} else if (seq_size == 16U) {
			u.uuid.type = BT_UUID_TYPE_128;
			memcpy(u.u128.val, cur_elem, seq_size);
			if (!bt_uuid_cmp(&u.uuid, uuid)) {
				*found = true;
			}
		} else {
			LOG_WRN("Invalid UUID size in local database");
			BT_ASSERT(0);
		}
	}

	if ((elem->type & BT_SDP_TYPE_DESC_MASK) == BT_SDP_SEQ_UNSPEC ||
	    (elem->type & BT_SDP_TYPE_DESC_MASK) == BT_SDP_ALT_UNSPEC) {
		do {
			/* Recursively parse data elements */
			size = search_uuid((struct bt_sdp_data_elem *)cur_elem,
					   uuid, found, nest_level + 1);
			if (*found) {
				return 0;
			}
			cur_elem += sizeof(struct bt_sdp_data_elem);
			seq_size -= size;
		} while (seq_size);
	}

	return elem->total_size;
}

/* @brief SDP service record iterator.
 *
 * Iterate over service records from a starting point.
 *
 * @param func Callback function.
 * @param user_data Data to pass to the callback.
 *
 * @return Pointer to the record where the iterator stopped, or NULL if all
 *  records are covered
 */
static struct bt_sdp_record *bt_sdp_foreach_svc(bt_sdp_svc_func_t func,
						void *user_data)
{
	struct bt_sdp_record *rec = db;

	while (rec) {
		if (func(rec, user_data) == BT_SDP_ITER_STOP) {
			break;
		}

		rec = rec->next;
	}
	return rec;
}

/* @brief Inserts a service record into a record pointer list
 *
 * Inserts a service record into a record pointer list
 *
 * @param rec The current service record.
 * @param user_data Pointer to the destination record list.
 *
 * @return BT_SDP_ITER_CONTINUE to move on to the next record.
 */
static uint8_t insert_record(struct bt_sdp_record *rec, void *user_data)
{
	struct bt_sdp_record **rec_list = user_data;

	rec_list[rec->index] = rec;

	return BT_SDP_ITER_CONTINUE;
}

/* @brief Looks for matching UUIDs in a list of service records
 *
 * Parses out a sequence of UUIDs from an input buffer, and checks if a record
 * in the list contains all the UUIDs. If it doesn't, the record is removed
 * from the list, so the list contains only the records which has all the
 * input UUIDs in them.
 *
 * @param buf Incoming buffer containing all the UUIDs to be matched
 * @param matching_recs List of service records to use for storing matching
 * records
 *
 * @return 0 for success, or relevant error code
 */
static uint16_t find_services(struct net_buf *buf,
			      struct bt_sdp_record **matching_recs)
{
	struct bt_sdp_data_elem data_elem;
	struct bt_sdp_record *record;
	uint32_t uuid_list_size;
	uint16_t res;
	uint8_t att_idx, rec_idx = 0U;
	bool found;
	union {
		struct bt_uuid uuid;
		struct bt_uuid_16 u16;
		struct bt_uuid_32 u32;
		struct bt_uuid_128 u128;
	} u;

	res = parse_data_elem(buf, &data_elem);
	if (res) {
		return res;
	}

	if (((data_elem.type & BT_SDP_TYPE_DESC_MASK) != BT_SDP_SEQ_UNSPEC) &&
	    ((data_elem.type & BT_SDP_TYPE_DESC_MASK) != BT_SDP_ALT_UNSPEC)) {
		LOG_WRN("Invalid type %x in service search pattern", data_elem.type);
		return BT_SDP_INVALID_SYNTAX;
	}

	uuid_list_size = data_elem.data_size;

	bt_sdp_foreach_svc(insert_record, matching_recs);

	/* Go over the sequence of UUIDs, and match one UUID at a time */
	while (uuid_list_size) {
		res = parse_data_elem(buf, &data_elem);
		if (res) {
			return res;
		}

		if ((data_elem.type & BT_SDP_TYPE_DESC_MASK) !=
		    BT_SDP_UUID_UNSPEC) {
			LOG_WRN("Invalid type %u in service search pattern", data_elem.type);
			return BT_SDP_INVALID_SYNTAX;
		}

		if (buf->len < data_elem.data_size) {
			LOG_WRN("Malformed packet");
			return BT_SDP_INVALID_SYNTAX;
		}

		uuid_list_size -= data_elem.total_size;

		if (data_elem.data_size == 2U) {
			u.uuid.type = BT_UUID_TYPE_16;
			u.u16.val = net_buf_pull_be16(buf);
		} else if (data_elem.data_size == 4U) {
			u.uuid.type = BT_UUID_TYPE_32;
			u.u32.val = net_buf_pull_be32(buf);
		} else if (data_elem.data_size == 16U) {
			u.uuid.type = BT_UUID_TYPE_128;
			sys_memcpy_swap(u.u128.val, buf->data,
					data_elem.data_size);
			net_buf_pull(buf, data_elem.data_size);
		} else {
			LOG_WRN("Invalid UUID len %u in service search pattern",
				data_elem.data_size);
			net_buf_pull(buf, data_elem.data_size);
			continue;
		}

		/* Go over the list of services, and look for a service which
		 * doesn't have this UUID
		 */
		for (rec_idx = 0U; rec_idx < num_services; rec_idx++) {
			record = matching_recs[rec_idx];

			if (!record) {
				continue;
			}

			found = false;

			/* Search for the UUID in all the attrs of the svc */
			for (att_idx = 0U; att_idx < record->attr_count;
			     att_idx++) {
				search_uuid(&record->attrs[att_idx].val,
					    &u.uuid, &found, 1);
				if (found) {
					break;
				}
			}

			/* Remove the record from the list if it doesn't have
			 * the UUID
			 */
			if (!found) {
				matching_recs[rec_idx] = NULL;
			}
		}
	}

	return 0;
}

/* @brief Handler for Service Search Request
 *
 * Parses, processes and responds to a Service Search Request
 *
 * @param sdp Pointer to the SDP structure
 * @param buf Request net buf
 * @param tid Transaction ID
 *
 * @return 0 for success, or relevant error code
 */
static uint16_t sdp_svc_search_req(struct bt_sdp *sdp, struct net_buf *buf,
				uint16_t tid)
{
	struct bt_sdp_svc_rsp *rsp;
	struct net_buf *resp_buf;
	struct bt_sdp_record *record;
	struct bt_sdp_record *matching_recs[BT_SDP_MAX_SERVICES];
	uint16_t max_rec_count, total_recs = 0U, current_recs = 0U, res;
	uint8_t cont_state_size, cont_state = 0U, idx = 0U, count = 0U;
	bool pkt_full = false;

	res = find_services(buf, matching_recs);
	if (res) {
		/* Error in parsing */
		return res;
	}

	if (buf->len < 3) {
		LOG_WRN("Malformed packet");
		return BT_SDP_INVALID_SYNTAX;
	}

	max_rec_count = net_buf_pull_be16(buf);
	cont_state_size = net_buf_pull_u8(buf);

	/* Zero out the matching services beyond max_rec_count */
	for (idx = 0U; idx < num_services; idx++) {
		if (count == max_rec_count) {
			matching_recs[idx] = NULL;
			continue;
		}

		if (matching_recs[idx]) {
			count++;
		}
	}

	/* We send out only SDP_SS_CONT_STATE_SIZE bytes continuation state in
	 * responses, so expect only SDP_SS_CONT_STATE_SIZE bytes in requests
	 */
	if (cont_state_size) {
		if (cont_state_size != SDP_SS_CONT_STATE_SIZE) {
			LOG_WRN("Invalid cont state size %u", cont_state_size);
			return BT_SDP_INVALID_CSTATE;
		}

		if (buf->len < cont_state_size) {
			LOG_WRN("Malformed packet");
			return BT_SDP_INVALID_SYNTAX;
		}

		cont_state = net_buf_pull_u8(buf);
		/* We include total_recs in the continuation state. We calculate
		 * it once and preserve it across all the partial responses
		 */
		total_recs = net_buf_pull_be16(buf);
	}

	LOG_DBG("max_rec_count %u, cont_state %u", max_rec_count, cont_state);

	resp_buf = bt_sdp_create_pdu();
	rsp = net_buf_add(resp_buf, sizeof(*rsp));

	for (; cont_state < num_services; cont_state++) {
		record = matching_recs[cont_state];

		if (!record) {
			continue;
		}

		/* Calculate total recs only if it is first packet */
		if (!cont_state_size) {
			total_recs++;
		}

		if (pkt_full) {
			continue;
		}

		/* 4 bytes per Service Record Handle */
		/* 4 bytes for ContinuationState */
		if ((MIN(SDP_MTU, sdp->chan.tx.mtu) - resp_buf->len) <
		    (4 + 4 + sizeof(struct bt_sdp_hdr))) {
			pkt_full = true;
		}

		if (pkt_full) {
			/* Packet exhausted: Add continuation state and break */
			LOG_DBG("Packet full, num_services_covered %u", cont_state);
			net_buf_add_u8(resp_buf, SDP_SS_CONT_STATE_SIZE);
			net_buf_add_u8(resp_buf, cont_state);

			/* If it is the first packet of a partial response,
			 * continue dry-running to calculate total_recs.
			 * Else break
			 */
			if (cont_state_size) {
				break;
			}

			continue;
		}

		/* Add the service record handle to the packet */
		net_buf_add_be32(resp_buf, record->handle);
		current_recs++;
	}

	/* Add 0 continuation state if packet is exhausted */
	if (!pkt_full) {
		net_buf_add_u8(resp_buf, 0);
	} else {
		net_buf_add_be16(resp_buf, total_recs);
	}

	rsp->total_recs = sys_cpu_to_be16(total_recs);
	rsp->current_recs = sys_cpu_to_be16(current_recs);

	LOG_DBG("Sending response, len %u", resp_buf->len);
	bt_sdp_send(&sdp->chan.chan, resp_buf, BT_SDP_SVC_SEARCH_RSP, tid);

	return 0;
}

/* @brief Copies an attribute into an outgoing buffer
 *
 *  Copies an attribute into a buffer. Recursively calls itself for complex
 *  attributes.
 *
 *  @param elem Attribute to be copied to the buffer
 *  @param buf Buffer where the attribute is to be copied
 *
 *  @return Size of the last data element that has been searched
 *  (used in recursion)
 */
static uint32_t copy_attribute(struct bt_sdp_data_elem *elem,
			    struct net_buf *buf, uint8_t nest_level)
{
	const uint8_t *cur_elem;
	uint32_t size, seq_size, total_size;

	/* Limit recursion depth to avoid stack overflows */
	if (nest_level == SDP_DATA_ELEM_NEST_LEVEL_MAX) {
		return 0;
	}

	seq_size = elem->data_size;
	total_size = elem->total_size;
	cur_elem = elem->data;

	/* Copy the header */
	net_buf_add_u8(buf, elem->type);

	switch (total_size - (seq_size + 1U)) {
	case 1:
		net_buf_add_u8(buf, elem->data_size);
		break;
	case 2:
		net_buf_add_be16(buf, elem->data_size);
		break;
	case 4:
		net_buf_add_be32(buf, elem->data_size);
		break;
	}

	/* Recursively parse (till the last element is not another data element)
	 * and then fill the elements
	 */
	if ((elem->type & BT_SDP_TYPE_DESC_MASK) == BT_SDP_SEQ_UNSPEC ||
	    (elem->type & BT_SDP_TYPE_DESC_MASK) == BT_SDP_ALT_UNSPEC) {
		do {
			size = copy_attribute((struct bt_sdp_data_elem *)
					      cur_elem, buf, nest_level + 1);
			cur_elem += sizeof(struct bt_sdp_data_elem);
			seq_size -= size;
		} while (seq_size);
	} else if ((elem->type & BT_SDP_TYPE_DESC_MASK) == BT_SDP_UINT8 ||
		   (elem->type & BT_SDP_TYPE_DESC_MASK) == BT_SDP_INT8 ||
		   (elem->type & BT_SDP_TYPE_DESC_MASK) == BT_SDP_UUID_UNSPEC) {
		if (seq_size == 1U) {
			net_buf_add_u8(buf, *((uint8_t *)elem->data));
		} else if (seq_size == 2U) {
			net_buf_add_be16(buf, *((uint16_t *)elem->data));
		} else if (seq_size == 4U) {
			net_buf_add_be32(buf, *((uint32_t *)elem->data));
		} else if (seq_size == 8U) {
			net_buf_add_be64(buf, *((uint64_t *)elem->data));
		} else {
			__ASSERT(seq_size == 0x10, "Invalid sequence size");

			uint8_t val[seq_size];

			sys_memcpy_swap(val, elem->data, sizeof(val));
			net_buf_add_mem(buf, val, seq_size);
		}
	} else {
		net_buf_add_mem(buf, elem->data, seq_size);
	}

	return total_size;
}

/* @brief SDP attribute iterator.
 *
 *  Iterate over attributes of a service record from a starting index.
 *
 *  @param record Service record whose attributes are to be iterated over.
 *  @param idx Index in the attribute list from where to start.
 *  @param func Callback function.
 *  @param user_data Data to pass to the callback.
 *
 *  @return Index of the attribute where the iterator stopped
 */
static uint8_t bt_sdp_foreach_attr(struct bt_sdp_record *record, uint8_t idx,
				bt_sdp_attr_func_t func, void *user_data)
{
	for (; idx < record->attr_count; idx++) {
		if (func(&record->attrs[idx], idx, user_data) ==
		    BT_SDP_ITER_STOP) {
			break;
		}
	}

	return idx;
}

/* @brief Check if an attribute matches a range, and include it in the response
 *
 *  Checks if an attribute matches a given attribute ID or range, and if so,
 *  includes it in the response packet
 *
 *  @param attr The current attribute
 *  @param att_idx Index of the current attribute in the database
 *  @param user_data Pointer to the structure containing response packet, byte
 *   count, states, etc
 *
 *  @return BT_SDP_ITER_CONTINUE if should continue to the next attribute
 *   or BT_SDP_ITER_STOP to stop.
 */
static uint8_t select_attrs(struct bt_sdp_attribute *attr, uint8_t att_idx,
			 void *user_data)
{
	struct select_attrs_data *sad = user_data;
	uint16_t att_id_lower, att_id_upper, att_id_cur, space;
	uint32_t attr_size, seq_size;
	size_t idx_filter;

	for (idx_filter = 0U; idx_filter < sad->num_filters; idx_filter++) {

		att_id_lower = (sad->filter[idx_filter] >> 16);
		att_id_upper = (sad->filter[idx_filter]);
		att_id_cur = attr->id;

		/* Check for range values */
		if (att_id_lower != 0xffff &&
		    (!IN_RANGE(att_id_cur, att_id_lower, att_id_upper))) {
			continue;
		}

		/* Check for match values */
		if (att_id_lower == 0xffff && att_id_cur != att_id_upper) {
			continue;
		}

		/* Attribute ID matches */

		/* 3 bytes for Attribute ID */
		attr_size = 3 + attr->val.total_size;

		/* If this is the first attribute of the service, then we need
		 * to account for the space required to add the per-service
		 * data element sequence header as well.
		 */
		if ((sad->state->current_svc != sad->rec->index) &&
		    sad->new_service) {
			/* 3 bytes for Per-Service Data Elem Seq declaration */
			seq_size = attr_size + 3;
		} else {
			seq_size = attr_size;
		}

		if (sad->rsp_buf) {
			space = MIN(SDP_MTU, sad->sdp->chan.tx.mtu) -
				sad->rsp_buf->len - sizeof(struct bt_sdp_hdr);

			if ((!sad->state->pkt_full) &&
			    ((seq_size > sad->max_att_len) ||
			     (space < seq_size + sad->cont_state_size))) {
				/* Packet exhausted */
				sad->state->pkt_full = true;
			}
		}

		/* Keep filling data only if packet is not exhausted */
		if (!sad->state->pkt_full && sad->rsp_buf) {
			/* Add Per-Service Data Element Seq declaration once
			 * only when we are starting from the first attribute
			 */
			if (!sad->seq &&
			    (sad->state->current_svc != sad->rec->index)) {
				sad->seq = net_buf_add(sad->rsp_buf,
						       sizeof(*sad->seq));
				sad->seq->type = BT_SDP_SEQ16;
				sad->seq->size = 0U;
			}

			/* Add attribute ID */
			net_buf_add_u8(sad->rsp_buf, BT_SDP_UINT16);
			net_buf_add_be16(sad->rsp_buf, att_id_cur);

			/* Add attribute value */
			copy_attribute(&attr->val, sad->rsp_buf, 1);

			sad->max_att_len -= seq_size;
			sad->att_list_len += seq_size;
			sad->state->last_att = att_idx;
			sad->state->current_svc = sad->rec->index;
		}

		if (sad->seq) {
			/* Keep adding the sequence size if this packet contains
			 * the Per-Service Data Element Seq declaration header
			 */
			sad->seq->size += attr_size;
			sad->state->att_list_size += seq_size;
		} else {
			/* Keep adding the total attr lists size if:
			 * It's a dry-run, calculating the total attr lists size
			 */
			sad->state->att_list_size += seq_size;
		}

		sad->new_service = false;
		break;
	}

	/* End the search if:
	 * 1. We have exhausted the packet
	 * AND
	 * 2. This packet doesn't contain the service element declaration header
	 * AND
	 * 3. This is not a dry-run (then we look for other attrs that match)
	 */
	if (sad->state->pkt_full && !sad->seq && sad->rsp_buf) {
		return BT_SDP_ITER_STOP;
	}

	return BT_SDP_ITER_CONTINUE;
}

/* @brief Creates attribute list in the given buffer
 *
 *  Populates the attribute list of a service record in the buffer. To be used
 *  for responding to Service Attribute and Service Search Attribute requests
 *
 *  @param sdp Pointer to the SDP structure
 *  @param record Service record whose attributes are to be included in the
 *   response
 *  @param filter Attribute values/ranges to be used as a filter
 *  @param num_filters Number of elements in the attribute filter
 *  @param max_att_len Maximum size of attributes to be included in the response
 *  @param cont_state_size No. of additional continuation state bytes to keep
 *   space for in the packet. This will vary based on the type of the request
 *  @param next_att Starting position of the search in the service's attr list
 *  @param state State of the overall search
 *  @param rsp_buf Response buffer which is filled in
 *
 *  @return len Length of the attribute list created
 */
static uint16_t create_attr_list(struct bt_sdp *sdp, struct bt_sdp_record *record,
			      uint32_t *filter, size_t num_filters,
			      uint16_t max_att_len, uint8_t cont_state_size,
			      uint8_t next_att, struct search_state *state,
			      struct net_buf *rsp_buf)
{
	struct select_attrs_data sad;
	uint8_t idx_att;

	sad.num_filters = num_filters;
	sad.rec = record;
	sad.rsp_buf = rsp_buf;
	sad.sdp = sdp;
	sad.max_att_len = max_att_len;
	sad.cont_state_size = cont_state_size;
	sad.seq = NULL;
	sad.filter = filter;
	sad.state = state;
	sad.att_list_len = 0U;
	sad.new_service = true;

	idx_att = bt_sdp_foreach_attr(sad.rec, next_att, select_attrs, &sad);

	if (sad.seq) {
		sad.seq->size = sys_cpu_to_be16(sad.seq->size);
	}

	return sad.att_list_len;
}

/* @brief Extracts the attribute search list from a buffer
 *
 *  Parses a buffer to extract the attribute search list (list of attribute IDs
 *  and ranges) which are to be used to filter attributes.
 *
 *  @param buf Buffer to be parsed for extracting the attribute search list
 *  @param filter Empty list of 4byte filters that are filled in. For attribute
 *   IDs, the lower 2 bytes contain the ID and the upper 2 bytes are set to
 *   0xFFFF. For attribute ranges, the lower 2bytes indicate the start ID and
 *   the upper 2bytes indicate the end ID
 *  @param max_filters Max element slots of filter to be filled in
 *  @param num_filters No. of filter elements filled in (to be returned)
 *
 *  @return 0 for success, or relevant error code
 */
static uint16_t get_att_search_list(struct net_buf *buf, uint32_t *filter,
				 size_t max_filters, size_t *num_filters)
{
	struct bt_sdp_data_elem data_elem;
	uint16_t res;
	uint32_t size;

	*num_filters = 0U;
	res = parse_data_elem(buf, &data_elem);
	if (res) {
		return res;
	}

	size = data_elem.data_size;

	while (size) {
		if (*num_filters >= max_filters) {
			LOG_WRN("Exceeded maximum array length %zu of %p", max_filters, filter);
			return 0;
		}

		res = parse_data_elem(buf, &data_elem);
		if (res) {
			return res;
		}

		if ((data_elem.type & BT_SDP_TYPE_DESC_MASK) != BT_SDP_UINT8) {
			LOG_WRN("Invalid type %u in attribute ID list", data_elem.type);
			return BT_SDP_INVALID_SYNTAX;
		}

		if (buf->len < data_elem.data_size) {
			LOG_WRN("Malformed packet");
			return BT_SDP_INVALID_SYNTAX;
		}

		/* This is an attribute ID */
		if (data_elem.data_size == 2U) {
			filter[(*num_filters)++] = 0xffff0000 |
							net_buf_pull_be16(buf);
		}

		/* This is an attribute ID range */
		if (data_elem.data_size == 4U) {
			filter[(*num_filters)++] = net_buf_pull_be32(buf);
		}

		size -= data_elem.total_size;
	}

	return 0;
}

/* @brief Check if a given handle matches that of the current service
 *
 *  Checks if a given handle matches that of the current service
 *
 *  @param rec The current service record
 *  @param user_data Pointer to the service record handle to be matched
 *
 *  @return BT_SDP_ITER_CONTINUE if should continue to the next record
 *   or BT_SDP_ITER_STOP to stop.
 */
static uint8_t find_handle(struct bt_sdp_record *rec, void *user_data)
{
	uint32_t *svc_rec_hdl = user_data;

	if (rec->handle == *svc_rec_hdl) {
		return BT_SDP_ITER_STOP;
	}

	return BT_SDP_ITER_CONTINUE;
}

/* @brief Handler for Service Attribute Request
 *
 *  Parses, processes and responds to a Service Attribute Request
 *
 *  @param sdp Pointer to the SDP structure
 *  @param buf Request buffer
 *  @param tid Transaction ID
 *
 *  @return 0 for success, or relevant error code
 */
static uint16_t sdp_svc_att_req(struct bt_sdp *sdp, struct net_buf *buf,
			     uint16_t tid)
{
	uint32_t filter[MAX_NUM_ATT_ID_FILTER];
	struct search_state state = {
		.current_svc = SDP_INVALID,
		.last_att = SDP_INVALID,
		.pkt_full = false
	};
	struct bt_sdp_record *record;
	struct bt_sdp_att_rsp *rsp;
	struct net_buf *rsp_buf;
	uint32_t svc_rec_hdl;
	uint16_t max_att_len, res, att_list_len;
	size_t num_filters;
	uint8_t cont_state_size, next_att = 0U;

	if (buf->len < 6) {
		LOG_WRN("Malformed packet");
		return BT_SDP_INVALID_SYNTAX;
	}

	svc_rec_hdl = net_buf_pull_be32(buf);
	max_att_len = net_buf_pull_be16(buf);

	/* Set up the filters */
	res = get_att_search_list(buf, filter, ARRAY_SIZE(filter), &num_filters);
	if (res) {
		/* Error in parsing */
		return res;
	}

	if (buf->len < 1) {
		LOG_WRN("Malformed packet");
		return BT_SDP_INVALID_SYNTAX;
	}

	cont_state_size = net_buf_pull_u8(buf);

	/* We only send out 1 byte continuation state in responses,
	 * so expect only 1 byte in requests
	 */
	if (cont_state_size) {
		if (cont_state_size != SDP_SA_CONT_STATE_SIZE) {
			LOG_WRN("Invalid cont state size %u", cont_state_size);
			return BT_SDP_INVALID_CSTATE;
		}

		if (buf->len < cont_state_size) {
			LOG_WRN("Malformed packet");
			return BT_SDP_INVALID_SYNTAX;
		}

		state.last_att = net_buf_pull_u8(buf) + 1;
		next_att = state.last_att;
	}

	LOG_DBG("svc_rec_hdl %u, max_att_len 0x%04x, cont_state %u", svc_rec_hdl, max_att_len,
		next_att);

	/* Find the service */
	record = bt_sdp_foreach_svc(find_handle, &svc_rec_hdl);

	if (!record) {
		LOG_WRN("Handle %u not found", svc_rec_hdl);
		return BT_SDP_INVALID_RECORD_HANDLE;
	}

	/* For partial responses, restore the search state */
	if (cont_state_size) {
		state.current_svc = record->index;
	}

	rsp_buf = bt_sdp_create_pdu();
	rsp = net_buf_add(rsp_buf, sizeof(*rsp));

	/* cont_state_size should include 1 byte header */
	att_list_len = create_attr_list(sdp, record, filter, num_filters,
					max_att_len, SDP_SA_CONT_STATE_SIZE + 1,
					next_att, &state, rsp_buf);

	if (!att_list_len) {
		/* For empty responses, add an empty data element sequence */
		net_buf_add_u8(rsp_buf, BT_SDP_SEQ8);
		net_buf_add_u8(rsp_buf, 0);
		att_list_len = 2U;
	}

	/* Add continuation state */
	if (state.pkt_full) {
		LOG_DBG("Packet full, state.last_att %u", state.last_att);
		net_buf_add_u8(rsp_buf, 1);
		net_buf_add_u8(rsp_buf, state.last_att);
	} else {
		net_buf_add_u8(rsp_buf, 0);
	}

	rsp->att_list_len = sys_cpu_to_be16(att_list_len);

	LOG_DBG("Sending response, len %u", rsp_buf->len);
	bt_sdp_send(&sdp->chan.chan, rsp_buf, BT_SDP_SVC_ATTR_RSP, tid);

	return 0;
}

/* @brief Handler for Service Search Attribute Request
 *
 *  Parses, processes and responds to a Service Search Attribute Request
 *
 *  @param sdp Pointer to the SDP structure
 *  @param buf Request buffer
 *  @param tid Transaction ID
 *
 *  @return 0 for success, or relevant error code
 */
static uint16_t sdp_svc_search_att_req(struct bt_sdp *sdp, struct net_buf *buf,
				    uint16_t tid)
{
	uint32_t filter[MAX_NUM_ATT_ID_FILTER];
	struct bt_sdp_record *matching_recs[BT_SDP_MAX_SERVICES];
	struct search_state state = {
		.att_list_size = 0,
		.current_svc = SDP_INVALID,
		.last_att = SDP_INVALID,
		.pkt_full = false
	};
	struct net_buf *rsp_buf, *rsp_buf_cpy;
	struct bt_sdp_record *record;
	struct bt_sdp_att_rsp *rsp;
	struct bt_sdp_data_elem_seq *seq = NULL;
	uint16_t max_att_len, res, att_list_len = 0U;
	size_t num_filters;
	uint8_t cont_state_size, next_svc = 0U, next_att = 0U;
	bool dry_run = false;

	res = find_services(buf, matching_recs);
	if (res) {
		return res;
	}

	if (buf->len < 2) {
		LOG_WRN("Malformed packet");
		return BT_SDP_INVALID_SYNTAX;
	}

	max_att_len = net_buf_pull_be16(buf);

	/* Set up the filters */
	res = get_att_search_list(buf, filter, ARRAY_SIZE(filter), &num_filters);

	if (res) {
		/* Error in parsing */
		return res;
	}

	if (buf->len < 1) {
		LOG_WRN("Malformed packet");
		return BT_SDP_INVALID_SYNTAX;
	}

	cont_state_size = net_buf_pull_u8(buf);

	/* We only send out 2 bytes continuation state in responses,
	 * so expect only 2 bytes in requests
	 */
	if (cont_state_size) {
		if (cont_state_size != SDP_SSA_CONT_STATE_SIZE) {
			LOG_WRN("Invalid cont state size %u", cont_state_size);
			return BT_SDP_INVALID_CSTATE;
		}

		if (buf->len < cont_state_size) {
			LOG_WRN("Malformed packet");
			return BT_SDP_INVALID_SYNTAX;
		}

		state.current_svc = net_buf_pull_u8(buf);
		state.last_att = net_buf_pull_u8(buf) + 1;
		next_svc = state.current_svc;
		next_att = state.last_att;
	}

	LOG_DBG("max_att_len 0x%04x, state.current_svc %u, state.last_att %u", max_att_len,
		state.current_svc, state.last_att);

	rsp_buf = bt_sdp_create_pdu();

	rsp = net_buf_add(rsp_buf, sizeof(*rsp));

	/* Add headers only if this is not a partial response */
	if (!cont_state_size) {
		seq = net_buf_add(rsp_buf, sizeof(*seq));
		seq->type = BT_SDP_SEQ16;
		seq->size = 0U;

		/* 3 bytes for Outer Data Element Sequence declaration */
		att_list_len = 3U;
	}

	rsp_buf_cpy = rsp_buf;

	for (; next_svc < num_services; next_svc++) {
		record = matching_recs[next_svc];

		if (!record) {
			continue;
		}

		att_list_len += create_attr_list(sdp, record, filter,
						 num_filters, max_att_len,
						 SDP_SSA_CONT_STATE_SIZE + 1,
						 next_att, &state, rsp_buf_cpy);

		/* Check if packet is full and not dry run */
		if (state.pkt_full && !dry_run) {
			LOG_DBG("Packet full, state.last_att %u", state.last_att);
			dry_run = true;

			/* Add continuation state */
			net_buf_add_u8(rsp_buf, 2);
			net_buf_add_u8(rsp_buf, state.current_svc);
			net_buf_add_u8(rsp_buf, state.last_att);

			/* Break if it's not a partial response, else dry-run
			 * Dry run: Look for other services that match
			 */
			if (cont_state_size) {
				break;
			}

			rsp_buf_cpy = NULL;
		}

		next_att = 0U;
	}

	if (!dry_run) {
		if (!att_list_len) {
			/* For empty responses, add an empty data elem seq */
			net_buf_add_u8(rsp_buf, BT_SDP_SEQ8);
			net_buf_add_u8(rsp_buf, 0);
			att_list_len = 2U;
		}
		/* Search exhausted */
		net_buf_add_u8(rsp_buf, 0);
	}

	rsp->att_list_len = sys_cpu_to_be16(att_list_len);
	if (seq) {
		seq->size = sys_cpu_to_be16(state.att_list_size);
	}

	LOG_DBG("Sending response, len %u", rsp_buf->len);
	bt_sdp_send(&sdp->chan.chan, rsp_buf, BT_SDP_SVC_SEARCH_ATTR_RSP,
		    tid);

	return 0;
}

static const struct {
	uint8_t  op_code;
	uint16_t  (*func)(struct bt_sdp *sdp, struct net_buf *buf, uint16_t tid);
} handlers[] = {
	{ BT_SDP_SVC_SEARCH_REQ, sdp_svc_search_req },
	{ BT_SDP_SVC_ATTR_REQ, sdp_svc_att_req },
	{ BT_SDP_SVC_SEARCH_ATTR_REQ, sdp_svc_search_att_req },
};

/* @brief Callback for SDP data receive
 *
 *  Gets called when an SDP PDU is received. Calls the corresponding handler
 *  based on the op code of the PDU.
 *
 *  @param chan L2CAP channel
 *  @param buf Received PDU
 *
 *  @return None
 */
static int bt_sdp_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_l2cap_br_chan *ch = CONTAINER_OF(chan,
			struct bt_l2cap_br_chan, chan);
	struct bt_sdp *sdp = CONTAINER_OF(ch, struct bt_sdp, chan);
	struct bt_sdp_hdr *hdr;
	uint16_t err = BT_SDP_INVALID_SYNTAX;
	size_t i;

	LOG_DBG("chan %p, ch %p, cid 0x%04x", chan, ch, ch->tx.cid);

	BT_ASSERT(sdp);

	if (buf->len < sizeof(*hdr)) {
		LOG_ERR("Too small SDP PDU received");
		return 0;
	}

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	LOG_DBG("Received SDP code 0x%02x len %u", hdr->op_code, buf->len);

	if (sys_cpu_to_be16(hdr->param_len) != buf->len) {
		err = BT_SDP_INVALID_PDU_SIZE;
	} else {
		for (i = 0; i < ARRAY_SIZE(handlers); i++) {
			if (hdr->op_code != handlers[i].op_code) {
				continue;
			}

			err = handlers[i].func(sdp, buf, sys_be16_to_cpu(hdr->tid));
			break;
		}
	}

	if (err) {
		LOG_WRN("SDP error 0x%02x", err);
		send_err_rsp(chan, err, sys_be16_to_cpu(hdr->tid));
	}

	return 0;
}

/* @brief Callback for SDP connection accept
 *
 *  Gets called when an incoming SDP connection needs to be authorized.
 *  Registers the L2CAP callbacks and allocates an SDP context to the connection
 *
 *  @param conn BT connection object
 *  @param chan L2CAP channel structure (to be returned)
 *
 *  @return 0 for success, or relevant error code
 */
static int bt_sdp_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
			 struct bt_l2cap_chan **chan)
{
	static const struct bt_l2cap_chan_ops ops = {
		.connected = bt_sdp_connected,
		.disconnected = bt_sdp_disconnected,
		.recv = bt_sdp_recv,
	};
	int i;

	LOG_DBG("conn %p", conn);

	for (i = 0; i < ARRAY_SIZE(bt_sdp_pool); i++) {
		struct bt_sdp *sdp = &bt_sdp_pool[i];

		if (sdp->chan.chan.conn) {
			continue;
		}

		sdp->chan.chan.ops = &ops;
		sdp->chan.rx.mtu = SDP_MTU;

		*chan = &sdp->chan.chan;

		return 0;
	}

	LOG_ERR("No available SDP context for conn %p", conn);

	return -ENOMEM;
}

void bt_sdp_init(void)
{
	static struct bt_l2cap_server server = {
		.psm = SDP_PSM,
		.accept = bt_sdp_accept,
		.sec_level = BT_SECURITY_L0,
	};
	int res;

	res = bt_l2cap_br_server_register(&server);
	if (res) {
		LOG_ERR("L2CAP server registration failed with error %d", res);
	}

	ARRAY_FOR_EACH(bt_sdp_client_pool, i) {
		/* Locking semaphore initialized to 1 (unlocked) */
		k_sem_init(&bt_sdp_client_pool[i].sem_lock, 1, 1);
	}
}

int bt_sdp_register_service(struct bt_sdp_record *service)
{
	uint32_t handle = SDP_SERVICE_HANDLE_BASE;

	if (!service) {
		LOG_ERR("No service record specified");
		return 0;
	}

	if (num_services == BT_SDP_MAX_SERVICES) {
		LOG_ERR("Reached max allowed registrations");
		return -ENOMEM;
	}

	if (db) {
		handle = db->handle + 1;
	}

	service->next = db;
	service->index = num_services++;
	service->handle = handle;
	*((uint32_t *)(service->attrs[0].val.data)) = handle;
	db = service;

	LOG_DBG("Service registered at %u", handle);

	return 0;
}

#define GET_PARAM(__node) \
	CONTAINER_OF(__node, struct bt_sdp_discover_params, _node)

/* ServiceSearch PDU, ref to BT Core 5.4, Vol 3, part B, 4.5.1 */
static int sdp_client_ss_search(struct bt_sdp_client *session,
				const struct bt_sdp_discover_params *param)
{
	struct net_buf *buf;
	uint8_t uuid128[BT_UUID_SIZE_128];

	/* Update context param directly. */
	session->param = param;

	buf = bt_sdp_create_pdu();

	/* BT_SDP_SEQ8 means length of sequence is on additional next byte */
	net_buf_add_u8(buf, BT_SDP_SEQ8);

	switch (param->uuid->type) {
	case BT_UUID_TYPE_16:
		/* Seq length */
		net_buf_add_u8(buf, sizeof(uint8_t) + BT_UUID_SIZE_16);
		/* Seq type */
		net_buf_add_u8(buf, BT_SDP_UUID16);
		/* Seq value */
		net_buf_add_be16(buf, BT_UUID_16(param->uuid)->val);
		break;
	case BT_UUID_TYPE_32:
		net_buf_add_u8(buf, sizeof(uint8_t) + BT_UUID_SIZE_32);
		net_buf_add_u8(buf, BT_SDP_UUID32);
		net_buf_add_be32(buf, BT_UUID_32(param->uuid)->val);
		break;
	case BT_UUID_TYPE_128:
		net_buf_add_u8(buf, sizeof(uint8_t) + BT_UUID_SIZE_128);
		net_buf_add_u8(buf, BT_SDP_UUID128);
		sys_memcpy_swap(uuid128, BT_UUID_128(param->uuid)->val, sizeof(uuid128));
		net_buf_add_mem(buf, uuid128, sizeof(uuid128));
		break;
	default:
		LOG_ERR("Unknown UUID type %u", param->uuid->type);
		net_buf_unref(buf);
		return -EINVAL;
	}

	/* Set maximum number of service record handles */
	net_buf_add_be16(buf, net_buf_tailroom(session->rec_buf) / SDP_RECORD_HANDLE_SIZE);
	/*
	 * Update and validate PDU ContinuationState. Initial SSA Request has
	 * zero length continuation state since no interaction has place with
	 * server so far, otherwise use the original state taken from remote's
	 * last response PDU that is cached by SDP client context.
	 */
	if (session->cstate.length == 0U) {
		net_buf_add_u8(buf, 0x00);
	} else {
		net_buf_add_u8(buf, session->cstate.length);
		net_buf_add_mem(buf, session->cstate.data, session->cstate.length);
	}

	session->tid++;

	return bt_sdp_send(&session->chan.chan, buf, BT_SDP_SVC_SEARCH_REQ, session->tid);
}

/* ServiceAttribute PDU, ref to BT Core 5.4, Vol 3, part B, 4.6.1 */
static int sdp_client_sa_search(struct bt_sdp_client *session,
				const struct bt_sdp_discover_params *param)
{
	struct net_buf *buf;

	/* Update context param directly. */
	session->param = param;

	buf = bt_sdp_create_pdu();

	/* Add service record handle  */
	net_buf_add_be32(buf, param->handle);

	/* Set attribute max bytes count to be returned from server */
	net_buf_add_be16(buf, net_buf_tailroom(session->rec_buf));
	/*
	 * Sequence definition where data is sequence of elements and where
	 * additional next byte points the size of elements within
	 */
	net_buf_add_u8(buf, BT_SDP_SEQ8);
	net_buf_add_u8(buf, 0x05);
	/* Data element definition for two following 16bits range elements */
	net_buf_add_u8(buf, BT_SDP_UINT32);
	/* Get all attributes. It enables filter out wanted only attributes */
	net_buf_add_be16(buf, 0x0000);
	net_buf_add_be16(buf, 0xffff);

	/*
	 * Update and validate PDU ContinuationState. Initial SSA Request has
	 * zero length continuation state since no interaction has place with
	 * server so far, otherwise use the original state taken from remote's
	 * last response PDU that is cached by SDP client context.
	 */
	if (session->cstate.length == 0U) {
		net_buf_add_u8(buf, 0x00);
	} else {
		net_buf_add_u8(buf, session->cstate.length);
		net_buf_add_mem(buf, session->cstate.data, session->cstate.length);
	}

	session->tid++;

	return bt_sdp_send(&session->chan.chan, buf, BT_SDP_SVC_ATTR_REQ, session->tid);
}

/* ServiceSearchAttribute PDU, ref to BT Core 4.2, Vol 3, part B, 4.7.1 */
static int sdp_client_ssa_search(struct bt_sdp_client *session,
				 const struct bt_sdp_discover_params *param)
{
	struct net_buf *buf;
	uint8_t uuid128[BT_UUID_SIZE_128];

	/* Update context param directly. */
	session->param = param;

	buf = bt_sdp_create_pdu();

	/* BT_SDP_SEQ8 means length of sequence is on additional next byte */
	net_buf_add_u8(buf, BT_SDP_SEQ8);

	switch (param->uuid->type) {
	case BT_UUID_TYPE_16:
		/* Seq length */
		net_buf_add_u8(buf, sizeof(uint8_t) + BT_UUID_SIZE_16);
		/* Seq type */
		net_buf_add_u8(buf, BT_SDP_UUID16);
		/* Seq value */
		net_buf_add_be16(buf, BT_UUID_16(param->uuid)->val);
		break;
	case BT_UUID_TYPE_32:
		net_buf_add_u8(buf, sizeof(uint8_t) + BT_UUID_SIZE_32);
		net_buf_add_u8(buf, BT_SDP_UUID32);
		net_buf_add_be32(buf, BT_UUID_32(param->uuid)->val);
		break;
	case BT_UUID_TYPE_128:
		net_buf_add_u8(buf, sizeof(uint8_t) + BT_UUID_SIZE_128);
		net_buf_add_u8(buf, BT_SDP_UUID128);
		sys_memcpy_swap(uuid128, BT_UUID_128(param->uuid)->val, sizeof(uuid128));
		net_buf_add_mem(buf, uuid128, sizeof(uuid128));
		break;
	default:
		LOG_ERR("Unknown UUID type %u", param->uuid->type);
		net_buf_unref(buf);
		return -EINVAL;
	}

	/* Set attribute max bytes count to be returned from server */
	net_buf_add_be16(buf, net_buf_tailroom(session->rec_buf));
	/*
	 * Sequence definition where data is sequence of elements and where
	 * additional next byte points the size of elements within
	 */
	net_buf_add_u8(buf, BT_SDP_SEQ8);
	net_buf_add_u8(buf, 0x05);
	/* Data element definition for two following 16bits range elements */
	net_buf_add_u8(buf, BT_SDP_UINT32);
	/* Get all attributes. It enables filter out wanted only attributes */
	net_buf_add_be16(buf, 0x0000);
	net_buf_add_be16(buf, 0xffff);

	/*
	 * Update and validate PDU ContinuationState. Initial SSA Request has
	 * zero length continuation state since no interaction has place with
	 * server so far, otherwise use the original state taken from remote's
	 * last response PDU that is cached by SDP client context.
	 */
	if (session->cstate.length == 0U) {
		net_buf_add_u8(buf, 0x00);
	} else {
		net_buf_add_u8(buf, session->cstate.length);
		net_buf_add_mem(buf, session->cstate.data,
				session->cstate.length);
	}

	session->tid++;

	return bt_sdp_send(&session->chan.chan, buf, BT_SDP_SVC_SEARCH_ATTR_REQ,
			   session->tid);
}

static int sdp_client_discover(struct bt_sdp_client *session);

static void sdp_client_params_iterator(struct bt_sdp_client *session)
{
	struct bt_l2cap_chan *chan = &session->chan.chan;
	struct bt_sdp_discover_params *param, *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&session->reqs, param, tmp, _node) {
		if (param != session->param) {
			continue;
		}

		LOG_DBG("");

		/* Remove already checked UUID node */
		sys_slist_remove(&session->reqs, NULL, &param->_node);
		/* Invalidate cached param in context */
		session->param = NULL;
		/* Reset continuation state in current context */
		(void)memset(&session->cstate, 0, sizeof(session->cstate));
		/* Clear total length */
		session->total_len = 0;
		/* Clear received length */
		session->recv_len = 0;

		k_sem_take(&session->sem_lock, K_FOREVER);
		/* Check if there's valid next UUID */
		if (!sys_slist_is_empty(&session->reqs)) {
			k_sem_give(&session->sem_lock);
			sdp_client_discover(session);
			return;
		}

		/* No UUID items, disconnect channel */
		session->state = SDP_CLIENT_DISCONNECTING;
		k_sem_give(&session->sem_lock);
		bt_l2cap_chan_disconnect(chan);
		break;
	}
}

static uint16_t sdp_client_get_total(struct bt_sdp_client *session,
				  struct net_buf *buf, uint16_t *total)
{
	uint16_t pulled;
	uint8_t seq;

	/*
	 * Pull value of total octets of all attributes available to be
	 * collected when response gets completed for given UUID. Such info can
	 * be get from the very first response frame after initial SSA request
	 * was sent. For subsequent calls related to the same SSA request input
	 * buf and in/out function parameters stays neutral.
	 */
	if (session->cstate.length == 0U) {
		seq = net_buf_pull_u8(buf);
		pulled = 1U;
		switch (seq) {
		case BT_SDP_SEQ8:
			*total = net_buf_pull_u8(buf);
			pulled += 1U;
			break;
		case BT_SDP_SEQ16:
			*total = net_buf_pull_be16(buf);
			pulled += 2U;
			break;
		case BT_SDP_SEQ32:
			*total = net_buf_pull_be32(buf);
			pulled += 4U;
			break;
		default:
			LOG_WRN("Sequence type 0x%02x not handled", seq);
			*total = 0U;
			break;
		}

		LOG_DBG("Total %u octets of all attributes", *total);
	} else {
		pulled = 0U;
		*total = 0U;
	}

	return pulled;
}

static uint16_t get_ss_record_len(struct net_buf *buf)
{
	if (buf->len >= SDP_RECORD_HANDLE_SIZE) {
		return SDP_RECORD_HANDLE_SIZE;
	}

	LOG_WRN("Invalid service record handle length");
	return 0;
}

static uint16_t get_ssa_record_len(struct net_buf *buf)
{
	uint16_t len;
	uint8_t seq;

	seq = net_buf_pull_u8(buf);

	switch (seq) {
	case BT_SDP_SEQ8:
		len = net_buf_pull_u8(buf);
		break;
	case BT_SDP_SEQ16:
		len = net_buf_pull_be16(buf);
		break;
	case BT_SDP_SEQ32:
		len = net_buf_pull_be32(buf);
		break;
	default:
		LOG_WRN("Sequence type 0x%02x not handled", seq);
		len = 0U;
		break;
	}

	return len;
}

static uint16_t get_record_len(struct bt_sdp_client *session)
{
	struct net_buf *buf;
	uint16_t len;

	buf = session->rec_buf;

	if (!session->param) {
		return buf->len;
	}

	switch (session->param->type) {
	case BT_SDP_DISCOVER_SERVICE_SEARCH:
		len = get_ss_record_len(buf);
		break;
	case BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR:
		len = get_ssa_record_len(buf);
		break;
	case BT_SDP_DISCOVER_SERVICE_ATTR:
	default:
		len = buf->len;
		break;
	}

	LOG_DBG("Record len %u", len);

	return len;
}

enum uuid_state {
	UUID_NOT_RESOLVED,
	UUID_RESOLVED,
};

static void sdp_client_notify_result(struct bt_sdp_client *session,
				     enum uuid_state state)
{
	struct bt_conn *conn = session->chan.chan.conn;
	struct bt_sdp_client_result result;
	uint16_t rec_len;
	uint8_t user_ret;

	if ((state == UUID_NOT_RESOLVED) || (session->rec_buf->len == 0U)) {
		result.resp_buf = NULL;
		result.next_record_hint = false;
		session->param->func(conn, &result, session->param);
		return;
	}

	while (session->rec_buf->len) {
		struct net_buf_simple_state buf_state;

		rec_len = get_record_len(session);
		/* tell the user about multi record resolution */
		if (session->rec_buf->len > rec_len) {
			result.next_record_hint = true;
		} else {
			result.next_record_hint = false;
		}

		/* save the original session buffer */
		net_buf_simple_save(&session->rec_buf->b, &buf_state);
		/* initialize internal result buffer instead of memcpy */
		result.resp_buf = session->rec_buf;
		/*
		 * Set user internal result buffer length as same as record
		 * length to fake user. User will see the individual record
		 * length as rec_len instead of whole session rec_buf length.
		 */
		result.resp_buf->len = rec_len;

		user_ret = session->param->func(conn, &result, session->param);

		/* restore original session buffer */
		net_buf_simple_restore(&session->rec_buf->b, &buf_state);
		/*
		 * sync session buffer data length with next record chunk not
		 * send to user so far
		 */
		net_buf_pull(session->rec_buf, rec_len);
		if (user_ret == BT_SDP_DISCOVER_UUID_STOP) {
			break;
		}
	}
}

static int sdp_client_discover(struct bt_sdp_client *session)
{
	const struct bt_sdp_discover_params *param;
	int err;

	/*
	 * Select proper user params, if session->param is invalid it means
	 * getting new UUID from top of to be resolved params list. Otherwise
	 * the context is in a middle of partial SDP PDU responses and cached
	 * value from context can be used.
	 */
	k_sem_take(&session->sem_lock, K_FOREVER);
	if (!session->param) {
		param = GET_PARAM(sys_slist_peek_head(&session->reqs));
	} else {
		param = session->param;
	}

	if (!param) {
		struct bt_l2cap_chan *chan = &session->chan.chan;

		session->state = SDP_CLIENT_DISCONNECTING;
		k_sem_give(&session->sem_lock);
		LOG_WRN("No more request, disconnect channel");
		/* No UUID items, disconnect channel */
		return bt_l2cap_chan_disconnect(chan);
	}
	k_sem_give(&session->sem_lock);

	switch (param->type) {
	case BT_SDP_DISCOVER_SERVICE_SEARCH:
		err = sdp_client_ss_search(session, param);
		break;
	case BT_SDP_DISCOVER_SERVICE_ATTR:
		err = sdp_client_sa_search(session, param);
		break;
	case BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR:
		err = sdp_client_ssa_search(session, param);
		break;
	default:
		err = -EINVAL;
		break;
	}

	if (err) {
		/* Notify the result */
		sdp_client_notify_result(session, UUID_NOT_RESOLVED);
		/* Get next UUID and start resolving it */
		sdp_client_params_iterator(session);
	}

	return 0;
}

static int sdp_client_receive_ss(struct bt_sdp_client *session, struct net_buf *buf)
{
	struct bt_sdp_pdu_cstate *cstate;
	uint16_t current_count;
	uint16_t total_count;
	uint32_t record_len;
	uint32_t received_count;

	/* Check the buffer len for the total_count field */
	if (buf->len < sizeof(total_count)) {
		LOG_ERR("Invalid frame payload length");
		return -EINVAL;
	}

	/* Get total service record count. */
	total_count = net_buf_pull_be16(buf);

	/* Check the buffer len for the current_count field */
	if (buf->len < sizeof(current_count)) {
		LOG_ERR("Invalid frame payload length");
		return -EINVAL;
	}

	/* Get current service record count. */
	current_count = net_buf_pull_be16(buf);
	/* Check valid of current service record count */
	if (current_count > total_count) {
		LOG_ERR("Invalid current service record count");
		return -EINVAL;
	}

	received_count = session->rec_buf->len / SDP_RECORD_HANDLE_SIZE;
	if ((received_count + current_count) > total_count) {
		LOG_ERR("Excess data received");
		return -EINVAL;
	}

	record_len = current_count * SDP_RECORD_HANDLE_SIZE;
	if (record_len >= buf->len) {
		LOG_ERR("Invalid packet");
		return -EINVAL;
	}

	/* Get PDU continuation state */
	cstate = (struct bt_sdp_pdu_cstate *)(buf->data + record_len);

	if (cstate->length > BT_SDP_MAX_PDU_CSTATE_LEN) {
		LOG_ERR("Invalid SDP PDU Continuation State length %u", cstate->length);
		return -EINVAL;
	}

	if ((record_len + SDP_CONT_STATE_LEN_SIZE + cstate->length) > buf->len) {
		LOG_ERR("Invalid payload length");
		return -EINVAL;
	}

	/*
	 * No record found for given UUID. The check catches case when
	 * current response frame has Continuation State shortest and
	 * valid and this is the first response frame as well.
	 */
	if (!current_count && cstate->length == 0U && session->cstate.length == 0U) {
		LOG_WRN("Service record handle 0x%x not found", session->param->handle);
		return -EINVAL;
	}

	if (record_len > net_buf_tailroom(session->rec_buf)) {
		LOG_WRN("Not enough room for getting records data");
		return -EINVAL;
	}

	net_buf_add_mem(session->rec_buf, buf->data, record_len);
	net_buf_pull(buf, record_len);

	/* check if current response says there's next portion to be fetched */
	if (cstate->length) {
		/* Cache original Continuation State in context */
		memcpy(&session->cstate, cstate, sizeof(struct bt_sdp_pdu_cstate));

		net_buf_pull(buf, cstate->length + sizeof(cstate->length));

		/*
		 * Request for next portion of attributes data.
		 * All failure case are handled internally in the function.
		 * Ignore the return value.
		 */
		(void)sdp_client_discover(session);

		return 0;
	}

	net_buf_pull(buf, sizeof(cstate->length));

	LOG_DBG("UUID 0x%s resolved", bt_uuid_str(session->param->uuid));
	sdp_client_notify_result(session, UUID_RESOLVED);
	/* Get next UUID and start resolving it */
	sdp_client_params_iterator(session);

	return 0;
}

static int sdp_client_receive_ssa_sa(struct bt_sdp_client *session, struct net_buf *buf)
{
	struct bt_sdp_pdu_cstate *cstate;
	uint16_t frame_len;
	uint16_t total;

	/* Check the buffer len for the frame_len field */
	if (buf->len < sizeof(frame_len)) {
		LOG_ERR("Invalid frame payload length");
		return -EINVAL;
	}

	/* Get number of attributes in this frame. */
	frame_len = net_buf_pull_be16(buf);
	/* Check valid buf len for attribute list and cont state */
	if (buf->len < frame_len + SDP_CONT_STATE_LEN_SIZE) {
		LOG_ERR("Invalid frame payload length");
		return -EINVAL;
	}
	/* Check valid range of attributes length */
	if (frame_len < 2) {
		LOG_ERR("Invalid attributes data length");
		return -EINVAL;
	}

	/* Get PDU continuation state */
	cstate = (struct bt_sdp_pdu_cstate *)(buf->data + frame_len);

	if (cstate->length > BT_SDP_MAX_PDU_CSTATE_LEN) {
		LOG_ERR("Invalid SDP PDU Continuation State length %u", cstate->length);
		return -EINVAL;
	}

	if ((frame_len + SDP_CONT_STATE_LEN_SIZE + cstate->length) > buf->len) {
		LOG_ERR("Invalid frame payload length");
		return -EINVAL;
	}

	/*
	 * No record found for given UUID. The check catches case when
	 * current response frame has Continuation State shortest and
	 * valid and this is the first response frame as well.
	 */
	if (frame_len == 2U && cstate->length == 0U && session->cstate.length == 0U) {
		LOG_WRN("Record for UUID 0x%s not found", bt_uuid_str(session->param->uuid));
		return -EINVAL;
	}

	/* Get total value of all attributes to be collected */
	frame_len -= sdp_client_get_total(session, buf, &total);
	/*
	 * If total is not 0, there are two valid cases,
	 * Case 1, the continuation state length is 0, the frame_len should equal total,
	 * Case 2, the continuation state length is not 0, it means there are more data will be
	 * received. So the frame_len is less than total.
	 */
	if (total && (frame_len > total)) {
		LOG_ERR("Invalid attribute lists");
		return -EINVAL;
	}

	if (session->cstate.length == 0U) {
		session->total_len = total;
	}

	session->recv_len += frame_len;

	if (frame_len > net_buf_tailroom(session->rec_buf)) {
		LOG_WRN("Not enough room for getting records data");
		return -EINVAL;
	}

	net_buf_add_mem(session->rec_buf, buf->data, frame_len);
	net_buf_pull(buf, frame_len);

	/* check if current response says there's next portion to be fetched */
	if (cstate->length) {
		/* Cache original Continuation State in context */
		memcpy(&session->cstate, cstate, sizeof(struct bt_sdp_pdu_cstate));

		net_buf_pull(buf, cstate->length + sizeof(cstate->length));

		/*
		 * Request for next portion of attributes data.
		 * All failure case are handled internally in the function.
		 * Ignore the return value.
		 */
		(void)sdp_client_discover(session);

		return 0;
	}

	if (session->total_len && (session->recv_len != session->total_len)) {
		LOG_WRN("The received len %d is mismatched with total len %d", session->recv_len,
			session->total_len);
		return -EINVAL;
	}

	net_buf_pull(buf, sizeof(cstate->length));

	LOG_DBG("UUID 0x%s resolved", bt_uuid_str(session->param->uuid));
	sdp_client_notify_result(session, UUID_RESOLVED);
	/* Get next UUID and start resolving it */
	sdp_client_params_iterator(session);

	return 0;
}

static int sdp_client_receive(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_sdp_client *session = SDP_CLIENT_CHAN(chan);
	struct bt_sdp_hdr *hdr;
	uint16_t len, tid;
	int err = -EINVAL;

	LOG_DBG("session %p buf %p", session, buf);

	if (buf->len < sizeof(*hdr)) {
		LOG_ERR("Too small SDP PDU");
		return 0;
	}

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	len = sys_be16_to_cpu(hdr->param_len);
	tid = sys_be16_to_cpu(hdr->tid);

	LOG_DBG("SDP PDU tid %u len %u", tid, len);

	if (buf->len != len) {
		LOG_ERR("SDP PDU length mismatch (%u != %u)", buf->len, len);
		return 0;
	}

	if (tid != session->tid) {
		LOG_ERR("Mismatch transaction ID value in SDP PDU");
		return 0;
	}

	if (session->param == NULL) {
		LOG_WRN("No request in progress");
		return 0;
	}

	switch (hdr->op_code) {
	case BT_SDP_SVC_SEARCH_RSP:
		err = sdp_client_receive_ss(session, buf);
		break;
	case BT_SDP_SVC_ATTR_RSP:
	case BT_SDP_SVC_SEARCH_ATTR_RSP:
		err = sdp_client_receive_ssa_sa(session, buf);
		break;
	case BT_SDP_ERROR_RSP:
		LOG_INF("Invalid SDP request");
		break;
	default:
		LOG_DBG("PDU 0x%0x response not handled", hdr->op_code);
		break;
	}

	if (err < 0) {
		sdp_client_notify_result(session, UUID_NOT_RESOLVED);
		sdp_client_params_iterator(session);
	}

	return 0;
}

static int sdp_client_chan_connect(struct bt_sdp_client *session)
{
	return bt_l2cap_br_chan_connect(session->chan.chan.conn, &session->chan.chan, SDP_PSM);
}

static struct net_buf *sdp_client_alloc_buf(struct bt_l2cap_chan *chan)
{
	struct bt_sdp_client *session = SDP_CLIENT_CHAN(chan);
	struct net_buf *buf;

	LOG_DBG("session %p chan %p", session, chan);

	session->param = GET_PARAM(sys_slist_peek_head(&session->reqs));

	buf = net_buf_alloc(session->param->pool, K_FOREVER);
	__ASSERT_NO_MSG(buf);

	return buf;
}

static void sdp_client_connected(struct bt_l2cap_chan *chan)
{
	struct bt_sdp_client *session = SDP_CLIENT_CHAN(chan);

	LOG_DBG("session %p chan %p connected", session, chan);

	k_sem_take(&session->sem_lock, K_FOREVER);
	session->rec_buf = chan->ops->alloc_buf(chan);
	if (!session->rec_buf) {
		session->state = SDP_CLIENT_DISCONNECTING;
		k_sem_give(&session->sem_lock);
		bt_l2cap_chan_disconnect(chan);
		return;
	}
	k_sem_give(&session->sem_lock);

	sdp_client_discover(session);
}

static void sdp_client_clean_after_disconnect(struct bt_sdp_client *session)
{
	/*
	 * keep the follow fields:
	 * sem_lock - it is always valid to protect the session, never clean it after bt_sdp_init.
	 * state - the session's state.
	 * chan - it is still used before released callback.
	 * reqs_next - the pending reqs in the disconnecting phase.
	 */
	sys_slist_init(&session->reqs);
	session->tid = 0U;
	session->param = NULL;
	memset(&session->cstate, 0, sizeof(session->cstate));
	if (session->rec_buf) {
		net_buf_unref(session->rec_buf);
		session->rec_buf = NULL;
	}
	session->total_len = 0U;
	session->recv_len = 0U;
}

static void sdp_client_clean_after_release(struct bt_sdp_client *session)
{
	/*
	 * keep the follow fields:
	 * sem_lock - it is always valid to protect the session, never clean it after bt_sdp_init.
	 * chan - it is maintained by l2cap layer.
	 */
	session->state = SDP_CLIENT_RELEASED;
	sys_slist_init(&session->reqs_next);
	sdp_client_clean_after_disconnect(session);
}

static void sdp_client_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_sdp_discover_params *param, *tmp;

	struct bt_sdp_client *session = SDP_CLIENT_CHAN(chan);

	LOG_DBG("session %p chan %p disconnected", session, chan);

	/* The disconnecting may be triggered by acl disconnection or failed sdp connecting */
	k_sem_take(&session->sem_lock, K_FOREVER);
	session->state = SDP_CLIENT_DISCONNECTING;
	k_sem_give(&session->sem_lock);

	/* callback all the sdp reqs */
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&session->reqs, param, tmp, _node) {
		session->param = param;
		sdp_client_notify_result(session, UUID_NOT_RESOLVED);
		/* Remove already callbacked UUID node */
		sys_slist_find_and_remove(&session->reqs, &param->_node);
	}

	if (session->rec_buf) {
		net_buf_unref(session->rec_buf);
		session->rec_buf = NULL;
	}

	sdp_client_clean_after_disconnect(session);
}

void sdp_client_released(struct bt_l2cap_chan *chan)
{
	struct bt_sdp_client *session = SDP_CLIENT_CHAN(chan);
	struct bt_sdp_discover_params *param, *tmp;
	struct bt_conn *conn;
	sys_slist_t cb_reqs;
	int err;

	k_sem_take(&session->sem_lock, K_FOREVER);
	if (!sys_slist_is_empty(&session->reqs_next)) {
		/* put the reqs_next to reqs */
		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&session->reqs_next, param, tmp, _node) {
			sys_slist_append(&session->reqs, &param->_node);
			/* Remove already proccessed node */
			sys_slist_remove(&session->reqs_next, NULL, &param->_node);
		}

		conn = bt_conn_lookup_index(ARRAY_INDEX(bt_sdp_client_pool, session));
		err = sdp_client_new_session(conn, session);

		if (err) {
			sys_slist_init(&cb_reqs);
			SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&session->reqs, param, tmp, _node) {
				sys_slist_append(&cb_reqs, &param->_node);
			}

			sdp_client_clean_after_release(session);
		}
		k_sem_give(&session->sem_lock);

		if (err) {
			struct bt_sdp_client_result result;

			result.resp_buf = NULL;
			result.next_record_hint = false;

			SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&cb_reqs, param, tmp, _node) {
				param->func(conn, &result, param);
			}
		}
		bt_conn_unref(conn);
	} else {
		sdp_client_clean_after_release(session);
		k_sem_give(&session->sem_lock);
	}
}

static const struct bt_l2cap_chan_ops sdp_client_chan_ops = {
		.connected = sdp_client_connected,
		.disconnected = sdp_client_disconnected,
		.released = sdp_client_released,
		.recv = sdp_client_receive,
		.alloc_buf = sdp_client_alloc_buf,
};

static int sdp_client_new_session(struct bt_conn *conn, struct bt_sdp_client *session)
{
	int err;

	session->chan.chan.ops = &sdp_client_chan_ops;
	session->chan.chan.conn = conn;
	session->chan.rx.mtu = SDP_CLIENT_MTU;

	err = sdp_client_chan_connect(session);
	if (err) {
		LOG_ERR("Cannot connect %d", err);
		return err;
	}

	session->state = SDP_CLIENT_CONNECTING;
	return err;
}

static int sdp_client_discovery_start(struct bt_conn *conn,
				      struct bt_sdp_discover_params *params)
{
	int err;
	struct bt_sdp_client *session;

	session = &bt_sdp_client_pool[bt_conn_index(conn)];
	k_sem_take(&session->sem_lock, K_FOREVER);
	if (session->state == SDP_CLIENT_CONNECTING ||
	    session->state == SDP_CLIENT_CONNECTED) {
		sys_slist_append(&session->reqs, &params->_node);
		k_sem_give(&session->sem_lock);
		return 0;
	}

	/* put in `reqs_next` for next round after disconnected */
	if (session->state == SDP_CLIENT_DISCONNECTING) {
		sys_slist_append(&session->reqs_next, &params->_node);
		k_sem_give(&session->sem_lock);
		return 0;
	}

	/*
	 * Try to allocate session context since not found in pool and attempt
	 * connect to remote SDP endpoint.
	 */
	sys_slist_init(&session->reqs);
	sys_slist_init(&session->reqs_next);
	sys_slist_append(&session->reqs, &params->_node);
	err = sdp_client_new_session(conn, session);
	if (err) {
		sdp_client_clean_after_release(session);
	}
	k_sem_give(&session->sem_lock);

	return err;
}

int bt_sdp_discover(struct bt_conn *conn,
		    struct bt_sdp_discover_params *params)
{
	if (!params || !params->uuid || !params->func || !params->pool) {
		LOG_WRN("Invalid user params");
		return -EINVAL;
	}

	return sdp_client_discovery_start(conn, params);
}

/* Helper getting length of data determined by DTD for integers */
static inline ssize_t sdp_get_int_len(const uint8_t *data, size_t len)
{
	BT_ASSERT(data);

	switch (data[0]) {
	case BT_SDP_DATA_NIL:
		return 1;
	case BT_SDP_BOOL:
	case BT_SDP_INT8:
	case BT_SDP_UINT8:
		if (len < 2) {
			break;
		}

		return 2;
	case BT_SDP_INT16:
	case BT_SDP_UINT16:
		if (len < 3) {
			break;
		}

		return 3;
	case BT_SDP_INT32:
	case BT_SDP_UINT32:
		if (len < 5) {
			break;
		}

		return 5;
	case BT_SDP_INT64:
	case BT_SDP_UINT64:
		if (len < 9) {
			break;
		}

		return 9;
	case BT_SDP_INT128:
	case BT_SDP_UINT128:
	default:
		LOG_ERR("Invalid/unhandled DTD 0x%02x", data[0]);
		return -EINVAL;
	}

	LOG_ERR("Too short buffer length %zu", len);
	return -EMSGSIZE;
}

/* Helper getting length of data determined by DTD for UUID */
static inline ssize_t sdp_get_uuid_len(const uint8_t *data, size_t len)
{
	BT_ASSERT(data);

	switch (data[0]) {
	case BT_SDP_UUID16:
		if (len < (sizeof(uint8_t) + BT_UUID_SIZE_16)) {
			break;
		}

		return sizeof(uint8_t) + BT_UUID_SIZE_16;
	case BT_SDP_UUID32:
		if (len < (sizeof(uint8_t) + BT_UUID_SIZE_32)) {
			break;
		}

		return sizeof(uint8_t) + BT_UUID_SIZE_32;
	case BT_SDP_UUID128:
		if (len < (sizeof(uint8_t) + BT_UUID_SIZE_128)) {
			break;
		}

		return sizeof(uint8_t) + BT_UUID_SIZE_128;
	default:
		LOG_ERR("Invalid/unhandled DTD 0x%02x", data[0]);
		return -EINVAL;
	}

	LOG_ERR("Too short buffer length %zu", len);
	return -EMSGSIZE;
}

/* Helper getting length of data determined by DTD for strings */
static inline ssize_t sdp_get_str_len(const uint8_t *data, size_t len)
{
	const uint8_t *pnext;

	BT_ASSERT(data);

	/* validate len for pnext safe use to read next 8bit value */
	if (len < 2) {
		goto err;
	}

	pnext = data + sizeof(uint8_t);

	switch (data[0]) {
	case BT_SDP_TEXT_STR8:
	case BT_SDP_URL_STR8:
		if (len < (2 + pnext[0])) {
			break;
		}

		return 2 + pnext[0];
	case BT_SDP_TEXT_STR16:
	case BT_SDP_URL_STR16:
		/* validate len for pnext safe use to read 16bit value */
		if (len < 3) {
			break;
		}

		if (len < (3 + sys_get_be16(pnext))) {
			break;
		}

		return 3 + sys_get_be16(pnext);
	case BT_SDP_TEXT_STR32:
	case BT_SDP_URL_STR32:
	default:
		LOG_ERR("Invalid/unhandled DTD 0x%02x", data[0]);
		return -EINVAL;
	}
err:
	LOG_ERR("Too short buffer length %zu", len);
	return -EMSGSIZE;
}

/* Helper getting length of data determined by DTD for sequences */
static inline ssize_t sdp_get_seq_len(const uint8_t *data, size_t len)
{
	const uint8_t *pnext;

	BT_ASSERT(data);

	/* validate len for pnext safe use to read 8bit bit value */
	if (len < 2) {
		goto err;
	}

	pnext = data + sizeof(uint8_t);

	switch (data[0]) {
	case BT_SDP_SEQ8:
	case BT_SDP_ALT8:
		if (len < (2 + pnext[0])) {
			break;
		}

		return 2 + pnext[0];
	case BT_SDP_SEQ16:
	case BT_SDP_ALT16:
		/* validate len for pnext safe use to read 16bit value */
		if (len < 3) {
			break;
		}

		if (len < (3 + sys_get_be16(pnext))) {
			break;
		}

		return 3 + sys_get_be16(pnext);
	case BT_SDP_SEQ32:
	case BT_SDP_ALT32:
		/* validate len for pnext safe use to read 32bit value */
		if (len < 5) {
			break;
		}

		if (len < (5 + sys_get_be32(pnext))) {
			break;
		}

		return 5 + sys_get_be32(pnext);
	default:
		LOG_ERR("Invalid/unhandled DTD 0x%02x", data[0]);
		return -EINVAL;
	}
err:
	LOG_ERR("Too short buffer length %zu", len);
	return -EMSGSIZE;
}

/* Helper getting length of attribute value data */
static ssize_t sdp_get_attr_value_len(const uint8_t *data, size_t len)
{
	BT_ASSERT(data);

	LOG_DBG("Attr val DTD 0x%02x", data[0]);

	if (len < 1) {
		goto err;
	}

	switch (data[0]) {
	case BT_SDP_DATA_NIL:
	case BT_SDP_BOOL:
	case BT_SDP_UINT8:
	case BT_SDP_UINT16:
	case BT_SDP_UINT32:
	case BT_SDP_UINT64:
	case BT_SDP_UINT128:
	case BT_SDP_INT8:
	case BT_SDP_INT16:
	case BT_SDP_INT32:
	case BT_SDP_INT64:
	case BT_SDP_INT128:
		return sdp_get_int_len(data, len);
	case BT_SDP_UUID16:
	case BT_SDP_UUID32:
	case BT_SDP_UUID128:
		return sdp_get_uuid_len(data, len);
	case BT_SDP_TEXT_STR8:
	case BT_SDP_TEXT_STR16:
	case BT_SDP_TEXT_STR32:
	case BT_SDP_URL_STR8:
	case BT_SDP_URL_STR16:
	case BT_SDP_URL_STR32:
		return sdp_get_str_len(data, len);
	case BT_SDP_SEQ8:
	case BT_SDP_SEQ16:
	case BT_SDP_SEQ32:
	case BT_SDP_ALT8:
	case BT_SDP_ALT16:
	case BT_SDP_ALT32:
		return sdp_get_seq_len(data, len);
	default:
		LOG_ERR("Unknown DTD 0x%02x", data[0]);
		return -EINVAL;
	}
err:
	LOG_ERR("Too short buffer length %zu", len);
	return -EMSGSIZE;

}

/* Type holding UUID item and related to it specific information. */
struct bt_sdp_uuid_desc {
	union {
		struct bt_uuid    uuid;
		struct bt_uuid_16 uuid16;
		struct bt_uuid_32 uuid32;
	      };
	uint16_t                     attr_id;
	uint8_t                     *params;
	uint16_t                     params_len;
};

/* Generic attribute item collector. */
struct bt_sdp_attr_item {
	/*  Attribute identifier. */
	uint16_t                  attr_id;
	/*  Address of beginning attribute value taken from original buffer
	 *  holding response from server.
	 */
	uint8_t                  *val;
	/*  Says about the length of attribute value. */
	uint16_t                  len;
};

static int bt_sdp_get_attr(const struct net_buf *buf,
			   struct bt_sdp_attr_item *attr, uint16_t attr_id)
{
	uint8_t *data;
	uint16_t id;

	data = buf->data;
	while (data - buf->data < buf->len) {
		ssize_t dlen;

		/* data need to point to attribute id descriptor field (DTD)*/
		if (data[0] != BT_SDP_UINT16) {
			LOG_ERR("Invalid descriptor 0x%02x", data[0]);
			return -EINVAL;
		}

		data += sizeof(uint8_t);
		if ((data + sizeof(id) - buf->data) > buf->len) {
			return -EINVAL;
		}
		id = sys_get_be16(data);
		LOG_DBG("Attribute ID 0x%04x", id);
		data += sizeof(uint16_t);

		dlen = sdp_get_attr_value_len(data,
					      buf->len - (data - buf->data));
		if (dlen < 0) {
			LOG_ERR("Invalid attribute value data");
			return -EINVAL;
		}

		if (id == attr_id) {
			LOG_DBG("Attribute ID 0x%04x Value found", id);
			/*
			 * Initialize attribute value buffer data using selected
			 * data slice from original buffer.
			 */
			attr->val = data;
			attr->len = dlen;
			attr->attr_id = id;
			return 0;
		}

		data += dlen;
	}

	return -ENOENT;
}

/* reads SEQ item length, moves input buffer data reader forward */
static ssize_t sdp_get_seq_len_item(uint8_t **data, size_t len)
{
	const uint8_t *pnext;

	BT_ASSERT(data);
	BT_ASSERT(*data);

	/* validate len for pnext safe use to read 8bit bit value */
	if (len < 2) {
		goto err;
	}

	pnext = *data + sizeof(uint8_t);

	switch (*data[0]) {
	case BT_SDP_SEQ8:
		if (len < (2 + pnext[0])) {
			break;
		}

		*data += 2;
		return pnext[0];
	case BT_SDP_SEQ16:
		/* validate len for pnext safe use to read 16bit value */
		if (len < 3) {
			break;
		}

		if (len < (3 + sys_get_be16(pnext))) {
			break;
		}

		*data += 3;
		return sys_get_be16(pnext);
	case BT_SDP_SEQ32:
		/* validate len for pnext safe use to read 32bit value */
		if (len < 5) {
			break;
		}

		if (len < (5 + sys_get_be32(pnext))) {
			break;
		}

		*data += 5;
		return sys_get_be32(pnext);
	default:
		LOG_ERR("Invalid/unhandled DTD 0x%02x", *data[0]);
		return -EINVAL;
	}
err:
	LOG_ERR("Too short buffer length %zu", len);
	return -EMSGSIZE;
}

static int sdp_loop_seqs(uint8_t **data, size_t len)
{
	ssize_t slen;
	ssize_t pre_slen;
	uint8_t *end;

	if (len <= 0) {
		return -EMSGSIZE;
	}

	pre_slen = -EINVAL;
	slen = -EINVAL;
	end = *data + len;
	/* loop all the SEQ */
	while (*data < end) {
		/* how long is current UUID's item data associated to */
		slen = sdp_get_seq_len_item(data, end - *data);
		if (slen < 0) {
			break;
		}
		pre_slen = slen;
	}

	/* return the last seq len */
	if (pre_slen < 0) {
		return slen;
	}

	return pre_slen;
}

static int sdp_get_uuid_data(const struct bt_sdp_attr_item *attr,
			     struct bt_sdp_uuid_desc *pd,
			     uint16_t proto_profile,
			     uint8_t proto_profile_index)
{
	/* get start address of attribute value */
	uint8_t *p = attr->val;
	ssize_t slen;

	BT_ASSERT(p);

	/* start reading stacked UUIDs in analyzed sequences tree */
	while (p - attr->val < attr->len) {
		size_t to_end, left = 0;
		uint8_t dtd;

		/* to_end tells how far to the end of input buffer */
		to_end = attr->len - (p - attr->val);
		/* loop all the SEQ, get the last SEQ len */
		slen = sdp_loop_seqs(&p, to_end);

		if (slen < 0) {
			return slen;
		}

		/* left tells how far is to the end of current UUID */
		left = slen;

		/* check if at least DTD + UUID16 can be read safely */
		if (left < (sizeof(dtd) + BT_UUID_SIZE_16)) {
			return -EMSGSIZE;
		}

		/* check DTD and get stacked UUID value */
		dtd = p[0];
		p++;
		/* include last DTD in p[0] size itself updating left */
		left -= sizeof(dtd);
		switch (dtd) {
		case BT_SDP_UUID16:
			memcpy(&pd->uuid16,
				BT_UUID_DECLARE_16(sys_get_be16(p)),
				sizeof(struct bt_uuid_16));
			p += sizeof(uint16_t);
			left -= sizeof(uint16_t);
			break;
		case BT_SDP_UUID32:
			/* check if valid UUID32 can be read safely */
			if (left < BT_UUID_SIZE_32) {
				return -EMSGSIZE;
			}

			memcpy(&pd->uuid32,
				BT_UUID_DECLARE_32(sys_get_be32(p)),
				sizeof(struct bt_uuid_32));
			p += sizeof(BT_UUID_SIZE_32);
			left -= sizeof(BT_UUID_SIZE_32);
			break;
		default:
			LOG_ERR("Invalid/unhandled DTD 0x%02x\n", dtd);
			return -EINVAL;
		}

		/*
			* Check if current UUID value matches input one given by user.
			* If found save it's location and length and return.
			*/
		if ((proto_profile == BT_UUID_16(&pd->uuid)->val) ||
			(proto_profile == BT_UUID_32(&pd->uuid)->val)) {
			pd->params = p;
			pd->params_len = left;

			LOG_DBG("UUID 0x%s found", bt_uuid_str(&pd->uuid));
			if (proto_profile_index > 0U) {
				proto_profile_index--;
				p += left;
				continue;
			} else {
				return 0;
			}
		}

		/* skip left octets to point beginning of next UUID in tree */
		p += left;
	}

	LOG_DBG("Value 0x%04x index %d not found", proto_profile, proto_profile_index);
	return -ENOENT;
}

/*
 * Helper extracting specific parameters associated with UUID node given in
 * protocol descriptor list or profile descriptor list.
 */
static int sdp_get_param_item(struct bt_sdp_uuid_desc *pd_item, uint16_t *param)
{
	const uint8_t *p = pd_item->params;
	bool len_err = false;

	BT_ASSERT(p);

	LOG_DBG("Getting UUID's 0x%s params", bt_uuid_str(&pd_item->uuid));

	switch (p[0]) {
	case BT_SDP_UINT8:
		/* check if 8bits value can be read safely */
		if (pd_item->params_len < 2) {
			len_err = true;
			break;
		}
		*param = (++p)[0];
		p += sizeof(uint8_t);
		break;
	case BT_SDP_UINT16:
		/* check if 16bits value can be read safely */
		if (pd_item->params_len < 3) {
			len_err = true;
			break;
		}
		*param = sys_get_be16(++p);
		p += sizeof(uint16_t);
		break;
	case BT_SDP_UINT32:
		/* check if 32bits value can be read safely */
		if (pd_item->params_len < 5) {
			len_err = true;
			break;
		}
		*param = sys_get_be32(++p);
		p += sizeof(uint32_t);
		break;
	default:
		LOG_ERR("Invalid/unhandled DTD 0x%02x\n", p[0]);
		return -EINVAL;
	}
	/*
	 * Check if no more data than already read is associated with UUID. In
	 * valid case after getting parameter we should reach data buf end.
	 */
	if (p - pd_item->params != pd_item->params_len || len_err) {
		LOG_DBG("Invalid param buffer length");
		return -EMSGSIZE;
	}

	return 0;
}

int bt_sdp_get_proto_param(const struct net_buf *buf, enum bt_sdp_proto proto,
			   uint16_t *param)
{
	struct bt_sdp_attr_item attr;
	struct bt_sdp_uuid_desc pd;
	int res;

	if (proto != BT_SDP_PROTO_RFCOMM && proto != BT_SDP_PROTO_L2CAP &&
	    proto != BT_SDP_PROTO_AVDTP) {
		LOG_ERR("Invalid protocol specifier");
		return -EINVAL;
	}

	res = bt_sdp_get_attr(buf, &attr, BT_SDP_ATTR_PROTO_DESC_LIST);
	if (res < 0) {
		LOG_WRN("Attribute 0x%04x not found, err %d", BT_SDP_ATTR_PROTO_DESC_LIST, res);
		return res;
	}

	res = sdp_get_uuid_data(&attr, &pd, proto, 0U);
	if (res < 0) {
		LOG_WRN("Protocol specifier 0x%04x not found, err %d", proto, res);
		return res;
	}

	return sdp_get_param_item(&pd, param);
}

int bt_sdp_get_addl_proto_param(const struct net_buf *buf, enum bt_sdp_proto proto,
				uint8_t param_index, uint16_t *param)
{
	struct bt_sdp_attr_item attr;
	struct bt_sdp_uuid_desc pd;
	int res;

	if (proto != BT_SDP_PROTO_RFCOMM && proto != BT_SDP_PROTO_L2CAP &&
	    proto != BT_SDP_PROTO_AVDTP) {
		LOG_ERR("Invalid protocol specifier");
		return -EINVAL;
	}

	res = bt_sdp_get_attr(buf, &attr, BT_SDP_ATTR_ADD_PROTO_DESC_LIST);
	if (res < 0) {
		LOG_WRN("Attribute 0x%04x not found, err %d", BT_SDP_ATTR_PROTO_DESC_LIST, res);
		return res;
	}

	res = sdp_get_uuid_data(&attr, &pd, proto, param_index);
	if (res < 0) {
		LOG_WRN("Protocol specifier 0x%04x not found, err %d", proto, res);
		return res;
	}

	return sdp_get_param_item(&pd, param);
}

int bt_sdp_get_profile_version(const struct net_buf *buf, uint16_t profile,
			       uint16_t *version)
{
	struct bt_sdp_attr_item attr;
	struct bt_sdp_uuid_desc pd;
	int res;

	res = bt_sdp_get_attr(buf, &attr, BT_SDP_ATTR_PROFILE_DESC_LIST);
	if (res < 0) {
		LOG_WRN("Attribute 0x%04x not found, err %d", BT_SDP_ATTR_PROFILE_DESC_LIST, res);
		return res;
	}

	res = sdp_get_uuid_data(&attr, &pd, profile, 0U);
	if (res < 0) {
		LOG_WRN("Profile 0x%04x not found, err %d", profile, res);
		return res;
	}

	return sdp_get_param_item(&pd, version);
}

int bt_sdp_get_features(const struct net_buf *buf, uint16_t *features)
{
	struct bt_sdp_attr_item attr;
	const uint8_t *p;
	int res;

	res = bt_sdp_get_attr(buf, &attr, BT_SDP_ATTR_SUPPORTED_FEATURES);
	if (res < 0) {
		LOG_WRN("Attribute 0x%04x not found, err %d", BT_SDP_ATTR_SUPPORTED_FEATURES, res);
		return res;
	}

	p = attr.val;
	BT_ASSERT(p);

	if (p[0] != BT_SDP_UINT16) {
		LOG_ERR("Invalid DTD 0x%02x", p[0]);
		return -EINVAL;
	}

	/* assert 16bit can be read safely */
	if (attr.len < 3) {
		LOG_ERR("Data length too short %u", attr.len);
		return -EMSGSIZE;
	}

	*features = sys_get_be16(++p);
	p += sizeof(uint16_t);

	if (p - attr.val != attr.len) {
		LOG_ERR("Invalid data length %u", attr.len);
		return -EMSGSIZE;
	}

	return 0;
}

/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_snmp, CONFIG_SNMP_LOG_LEVEL);

#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net/snmp.h>
#include <zephyr/sys/util.h>

#include "snmp_ber.h"
#include "snmp_internal.h"

/* Number of GETBULK repeaters the agent keeps state for. */
#define BULK_REPEATERS_MAX (IS_ENABLED(CONFIG_SNMP_V2C) ? CONFIG_SNMP_MAX_VARBINDS : 1)

BUILD_ASSERT(sizeof(CONFIG_SNMP_COMMUNITY_READ) - 1U <= CONFIG_SNMP_COMMUNITY_MAX_LEN,
	     "CONFIG_SNMP_COMMUNITY_READ is longer than CONFIG_SNMP_COMMUNITY_MAX_LEN");
BUILD_ASSERT(sizeof(CONFIG_SNMP_COMMUNITY_WRITE) - 1U <= CONFIG_SNMP_COMMUNITY_MAX_LEN,
	     "CONFIG_SNMP_COMMUNITY_WRITE is longer than CONFIG_SNMP_COMMUNITY_MAX_LEN");

enum snmp_access {
	SNMP_ACCESS_NONE,
	SNMP_ACCESS_READ,
	SNMP_ACCESS_WRITE,
};

/* Fields of a parsed request; the varbinds reader spans the list content. */
struct snmp_request {
	int32_t version;
	uint8_t pdu_type;
	int32_t request_id;
	int32_t error_status; /* non-repeaters for GETBULK */
	int32_t error_index;  /* max-repetitions for GETBULK */
	const uint8_t *community;
	size_t community_len;
	struct snmp_ber_reader varbinds;
};

/* Outcome of processing: error status and the 1-based index it refers to. */
struct snmp_status {
	int32_t error_status;
	int32_t error_index;
};

struct snmp_counters snmp_stats;

static const struct snmp_storage *storage;

int snmp_agent_set_storage(const struct snmp_storage *s)
{
	if (s != NULL && (s->load == NULL || s->store == NULL)) {
		return -EINVAL;
	}

	storage = s;

	return 0;
}

int snmp_storage_load(enum snmp_storage_item item, void *buf, size_t size)
{
	if (storage == NULL) {
		return -ENOENT;
	}

	return storage->load(item, buf, size);
}

int snmp_storage_store(enum snmp_storage_item item, const void *data, size_t len)
{
	if (storage == NULL) {
		return 0;
	}

	return storage->store(item, data, len);
}

/* snmpEnableAuthenTraps lives here so that it is available whether or not
 * the snmp group that exposes it is built. Stored as one byte with the
 * object's own encoding.
 */
#define AUTHEN_TRAPS_ENABLED  1U
#define AUTHEN_TRAPS_DISABLED 2U

static bool authen_traps = true;

int snmp_mib2_snmp_set_authen_traps(bool enabled)
{
	uint8_t stored = enabled ? AUTHEN_TRAPS_ENABLED : AUTHEN_TRAPS_DISABLED;

	if (snmp_storage_store(SNMP_STORAGE_AUTHEN_TRAPS, &stored, sizeof(stored)) != 0) {
		return -EIO;
	}

	authen_traps = enabled;

	return 0;
}

static void authen_traps_restore(void)
{
	uint8_t stored;
	int ret;

	ret = snmp_storage_load(SNMP_STORAGE_AUTHEN_TRAPS, &stored, sizeof(stored));
	if (ret == -ENOENT) {
		return;
	}

	if (ret != (int)sizeof(stored) ||
	    (stored != AUTHEN_TRAPS_ENABLED && stored != AUTHEN_TRAPS_DISABLED)) {
		LOG_WRN("Ignoring stored snmpEnableAuthenTraps (%d)", ret);
		return;
	}

	authen_traps = (stored == AUTHEN_TRAPS_ENABLED);
}

void snmp_storage_restore(void)
{
	authen_traps_restore();

	if (IS_ENABLED(CONFIG_SNMP_MIB2_SYSTEM)) {
		snmp_mib2_system_restore();
	}
}

bool snmp_authen_traps_enabled(void)
{
	return authen_traps;
}

static char read_community[CONFIG_SNMP_COMMUNITY_MAX_LEN + 1] = CONFIG_SNMP_COMMUNITY_READ;
static char write_community[CONFIG_SNMP_COMMUNITY_MAX_LEN + 1] = CONFIG_SNMP_COMMUNITY_WRITE;
static K_MUTEX_DEFINE(community_lock);

/* Last instance returned per GETBULK repeater, across repetitions. */
static uint32_t bulk_last[BULK_REPEATERS_MAX][SNMP_OID_MAX_LEN];
static size_t bulk_last_len[BULK_REPEATERS_MAX];
static bool bulk_done[BULK_REPEATERS_MAX];

int snmp_agent_set_community(enum snmp_community which, const char *community)
{
	char *dst;
	size_t len;

	if (community == NULL) {
		return -EINVAL;
	}

	switch (which) {
	case SNMP_COMMUNITY_READ:
		dst = read_community;
		break;
	case SNMP_COMMUNITY_WRITE:
		dst = write_community;
		break;
	default:
		return -EINVAL;
	}

	len = strlen(community);
	if (len > CONFIG_SNMP_COMMUNITY_MAX_LEN) {
		return -EMSGSIZE;
	}

	k_mutex_lock(&community_lock, K_FOREVER);
	memcpy(dst, community, len + 1U);
	k_mutex_unlock(&community_lock);

	return 0;
}

static bool community_matches(const char *expected, const uint8_t *community, size_t len)
{
	size_t expected_len = strlen(expected);

	/* An empty community grants nothing. */
	if (expected_len == 0U || expected_len != len) {
		return false;
	}

	return memcmp(expected, community, len) == 0;
}

static enum snmp_access community_access(const uint8_t *community, size_t len)
{
	enum snmp_access access = SNMP_ACCESS_NONE;

	k_mutex_lock(&community_lock, K_FOREVER);

	if (community_matches(write_community, community, len)) {
		access = SNMP_ACCESS_WRITE;
	} else if (community_matches(read_community, community, len)) {
		access = SNMP_ACCESS_READ;
	}

	k_mutex_unlock(&community_lock);

	return access;
}

static int parse_request(const uint8_t *in, size_t in_len, struct snmp_request *req)
{
	struct snmp_ber_reader outer, msg, pdu;
	size_t pdu_len;
	int ret;

	snmp_ber_reader_init(&outer, in, in_len);

	ret = snmp_ber_read_sequence(&outer, SNMP_BER_TAG_SEQUENCE, &msg);
	if (ret != 0) {
		return ret;
	}

	ret = snmp_ber_read_int32(&msg, &req->version);
	if (ret != 0) {
		return ret;
	}

	ret = snmp_ber_read_octets(&msg, SNMP_BER_TAG_OCTET_STRING, &req->community,
				   &req->community_len);
	if (ret != 0) {
		return ret;
	}

	ret = snmp_ber_read_header(&msg, &req->pdu_type, &pdu_len);
	if (ret != 0) {
		return ret;
	}

	snmp_ber_reader_init(&pdu, &msg.data[msg.pos], pdu_len);

	ret = snmp_ber_read_int32(&pdu, &req->request_id);
	if (ret != 0) {
		return ret;
	}

	ret = snmp_ber_read_int32(&pdu, &req->error_status);
	if (ret != 0) {
		return ret;
	}

	ret = snmp_ber_read_int32(&pdu, &req->error_index);
	if (ret != 0) {
		return ret;
	}

	return snmp_ber_read_sequence(&pdu, SNMP_BER_TAG_SEQUENCE, &req->varbinds);
}

/*
 * Check every binding before any of them is acted on: a request that does
 * not parse is discarded as a whole rather than answered up to the failure.
 * Names longer than SNMP_OID_MAX_LEN are rejected here as well. A version 1
 * request carrying a Counter64 value is ill-formed by RFC 3584 4.2.2.1.
 */
static int validate_varbinds(const struct snmp_request *req)
{
	struct snmp_ber_reader list = req->varbinds;

	while (!snmp_ber_reader_at_end(&list)) {
		struct snmp_ber_reader vb;
		uint32_t name[SNMP_OID_MAX_LEN];
		size_t name_len;
		uint8_t tag;
		int ret;

		ret = snmp_ber_read_sequence(&list, SNMP_BER_TAG_SEQUENCE, &vb);
		if (ret != 0) {
			return ret;
		}

		ret = snmp_ber_read_oid(&vb, name, SNMP_OID_MAX_LEN, &name_len);
		if (ret != 0) {
			return -EBADMSG;
		}

		ret = snmp_ber_peek_tag(&vb, &tag);
		if (ret != 0) {
			return ret;
		}

		if (req->version == SNMP_VERSION_1 && tag == SNMP_BER_TAG_COUNTER64) {
			return -EBADMSG;
		}

		ret = snmp_ber_skip(&vb);
		if (ret != 0) {
			return ret;
		}

		if (!snmp_ber_reader_at_end(&vb)) {
			return -EBADMSG;
		}
	}

	return 0;
}

/*
 * Parse the next variable binding. The value is decoded when val is not NULL,
 * otherwise skipped. oid_buf receives the sub-identifiers of an OID value.
 * Returns -ENOTSUP for a value type the agent cannot decode; the name is
 * valid in that case.
 */
static int parse_varbind(struct snmp_ber_reader *list, uint32_t *name, size_t *name_len,
			 struct snmp_value *val, uint32_t *oid_buf)
{
	struct snmp_ber_reader vb;
	uint8_t tag;
	int ret;

	ret = snmp_ber_read_sequence(list, SNMP_BER_TAG_SEQUENCE, &vb);
	if (ret != 0) {
		return ret;
	}

	ret = snmp_ber_read_oid(&vb, name, SNMP_OID_MAX_LEN, name_len);
	if (ret != 0) {
		return -EBADMSG;
	}

	if (val == NULL) {
		return snmp_ber_skip(&vb);
	}

	ret = snmp_ber_peek_tag(&vb, &tag);
	if (ret != 0) {
		return ret;
	}

	switch (tag) {
	case SNMP_BER_TAG_INTEGER:
		val->type = SNMP_TYPE_INTEGER;
		return snmp_ber_read_int32(&vb, &val->i32);
	case SNMP_BER_TAG_OCTET_STRING:
		val->type = SNMP_TYPE_OCTET_STRING;
		return snmp_ber_read_octets(&vb, tag, &val->octets.data, &val->octets.len);
	case SNMP_BER_TAG_NULL:
		val->type = SNMP_TYPE_NULL;
		return snmp_ber_read_empty(&vb, tag);
	case SNMP_BER_TAG_OID:
		val->type = SNMP_TYPE_OID;
		val->oid.arcs = oid_buf;
		ret = snmp_ber_read_oid(&vb, oid_buf, SNMP_OID_MAX_LEN, &val->oid.len);
		return (ret == 0) ? 0 : -EBADMSG;
	case SNMP_BER_TAG_IPADDRESS:
		val->type = SNMP_TYPE_IPADDRESS;
		ret = snmp_ber_read_octets(&vb, tag, &val->octets.data, &val->octets.len);
		if (ret == 0 && val->octets.len != 4U) {
			return -EBADMSG;
		}
		return ret;
	case SNMP_BER_TAG_COUNTER32:
		val->type = SNMP_TYPE_COUNTER32;
		return snmp_ber_read_uint32(&vb, tag, &val->u32);
	case SNMP_BER_TAG_GAUGE32:
		val->type = SNMP_TYPE_GAUGE32;
		return snmp_ber_read_uint32(&vb, tag, &val->u32);
	case SNMP_BER_TAG_TIMETICKS:
		val->type = SNMP_TYPE_TIMETICKS;
		return snmp_ber_read_uint32(&vb, tag, &val->u32);
	case SNMP_BER_TAG_COUNTER64:
		val->type = SNMP_TYPE_COUNTER64;
		return snmp_ber_read_uint64(&vb, tag, &val->u64);
	default:
		return -ENOTSUP;
	}
}

static size_t value_size(const struct snmp_value *val)
{
	switch (val->type) {
	case SNMP_TYPE_INTEGER:
		return snmp_ber_sizeof_int32(val->i32);
	case SNMP_TYPE_OCTET_STRING:
	case SNMP_TYPE_IPADDRESS:
		return snmp_ber_sizeof_octets(val->octets.len);
	case SNMP_TYPE_NULL:
		return snmp_ber_sizeof_empty();
	case SNMP_TYPE_OID:
		return snmp_ber_sizeof_oid(val->oid.arcs, val->oid.len);
	case SNMP_TYPE_COUNTER32:
	case SNMP_TYPE_GAUGE32:
	case SNMP_TYPE_TIMETICKS:
		return snmp_ber_sizeof_uint32(val->u32);
	case SNMP_TYPE_COUNTER64:
		return snmp_ber_sizeof_uint64(val->u64);
	default:
		return 0U;
	}
}

static int write_value(struct snmp_ber_writer *w, const struct snmp_value *val)
{
	switch (val->type) {
	case SNMP_TYPE_INTEGER:
		return snmp_ber_write_int32(w, val->i32);
	case SNMP_TYPE_OCTET_STRING:
		return snmp_ber_write_octets(w, SNMP_BER_TAG_OCTET_STRING, val->octets.data,
					     val->octets.len);
	case SNMP_TYPE_IPADDRESS:
		return snmp_ber_write_octets(w, SNMP_BER_TAG_IPADDRESS, val->octets.data,
					     val->octets.len);
	case SNMP_TYPE_NULL:
		return snmp_ber_write_empty(w, SNMP_BER_TAG_NULL);
	case SNMP_TYPE_OID:
		return snmp_ber_write_oid(w, val->oid.arcs, val->oid.len);
	case SNMP_TYPE_COUNTER32:
		return snmp_ber_write_uint32(w, SNMP_BER_TAG_COUNTER32, val->u32);
	case SNMP_TYPE_GAUGE32:
		return snmp_ber_write_uint32(w, SNMP_BER_TAG_GAUGE32, val->u32);
	case SNMP_TYPE_TIMETICKS:
		return snmp_ber_write_uint32(w, SNMP_BER_TAG_TIMETICKS, val->u32);
	case SNMP_TYPE_COUNTER64:
		return snmp_ber_write_uint64(w, SNMP_BER_TAG_COUNTER64, val->u64);
	default:
		return -EINVAL;
	}
}

size_t snmp_varbind_size(const uint32_t *name, size_t name_len, const struct snmp_value *val)
{
	size_t name_size = snmp_ber_sizeof_oid(name, name_len);
	size_t val_size = value_size(val);

	if (name_size == 0U || val_size == 0U) {
		return 0U;
	}

	return snmp_ber_sizeof_element(name_size + val_size);
}

int snmp_write_varbind(struct snmp_ber_writer *w, const uint32_t *name, size_t name_len,
		       const struct snmp_value *val, uint8_t exception)
{
	size_t saved = w->pos;
	size_t name_size = snmp_ber_sizeof_oid(name, name_len);
	size_t val_size = (exception != 0U) ? snmp_ber_sizeof_empty() : value_size(val);
	int ret;

	if (name_size == 0U || val_size == 0U) {
		return -EINVAL;
	}

	ret = snmp_ber_write_header(w, SNMP_BER_TAG_SEQUENCE, name_size + val_size);
	if (ret == 0) {
		ret = snmp_ber_write_oid(w, name, name_len);
	}

	if (ret == 0) {
		if (exception != 0U) {
			ret = snmp_ber_write_empty(w, exception);
		} else {
			ret = write_value(w, val);
		}
	}

	if (ret != 0) {
		w->pos = saved;
	}

	return ret;
}

/* Copy the request's variable bindings unchanged, as SNMPv1 errors and SET
 * responses require.
 */
static int write_request_varbinds(struct snmp_ber_writer *w, const struct snmp_request *req)
{
	if (w->size - w->pos < req->varbinds.len) {
		return -EMSGSIZE;
	}

	memcpy(&w->buf[w->pos], req->varbinds.data, req->varbinds.len);
	w->pos += req->varbinds.len;

	return 0;
}

/* Map an SNMPv2c error status to the SNMPv1 one, RFC 3584 section 4.3. */
static int32_t v1_error_status(int32_t status)
{
	switch (status) {
	case SNMP_ERR_WRONG_VALUE:
	case SNMP_ERR_WRONG_ENCODING:
	case SNMP_ERR_WRONG_TYPE:
	case SNMP_ERR_WRONG_LENGTH:
	case SNMP_ERR_INCONSISTENT_VALUE:
		return SNMP_ERR_BAD_VALUE;
	case SNMP_ERR_NO_ACCESS:
	case SNMP_ERR_NO_CREATION:
	case SNMP_ERR_NOT_WRITABLE:
	case SNMP_ERR_AUTHORIZATION_ERROR:
	case SNMP_ERR_INCONSISTENT_NAME:
		return SNMP_ERR_NO_SUCH_NAME;
	case SNMP_ERR_RESOURCE_UNAVAILABLE:
	case SNMP_ERR_COMMIT_FAILED:
	case SNMP_ERR_UNDO_FAILED:
		return SNMP_ERR_GEN_ERR;
	default:
		return status;
	}
}

/* Error status for a getter that failed. */
static int32_t get_error_status(int ret)
{
	return (ret == -ENOENT) ? SNMP_ERR_NO_SUCH_NAME : SNMP_ERR_GEN_ERR;
}

/* Error status for a setter that failed its check phase. */
static int32_t check_error_status(int ret)
{
	switch (ret) {
	case -EINVAL:
		return SNMP_ERR_WRONG_TYPE;
	case -EMSGSIZE:
		return SNMP_ERR_WRONG_LENGTH;
	case -ERANGE:
		return SNMP_ERR_WRONG_VALUE;
	case -EACCES:
		return SNMP_ERR_NOT_WRITABLE;
	case -EBUSY:
		return SNMP_ERR_INCONSISTENT_VALUE;
	default:
		return SNMP_ERR_GEN_ERR;
	}
}

/* Error status for the first setter that failed its commit phase. */
static int32_t commit_error_status(int ret)
{
	return (ret == -EIO) ? SNMP_ERR_COMMIT_FAILED : SNMP_ERR_GEN_ERR;
}

/*
 * Find the instance after oid whose getter succeeds and leave it in oid.
 * An instance whose getter reports -ENOENT vanished between enumeration and
 * access and is skipped; for a version 1 request Counter64 instances are not
 * in view, RFC 3584 4.2.2.1. oid is untouched unless an instance was found.
 *
 * Returns 0, -ENOENT at the end of the view, or the getter's error.
 */
static int fetch_next(uint32_t *oid, size_t *oid_len, struct snmp_value *val, bool v1)
{
	uint32_t cur[SNMP_OID_MAX_LEN];
	size_t cur_len = *oid_len;
	uint32_t inst[SNMP_OID_MAX_LEN];
	size_t inst_len;

	memcpy(cur, oid, cur_len * sizeof(uint32_t));

	while (true) {
		const struct snmp_object *obj;
		void *row;
		int ret;

		obj = snmp_registry_next(cur, cur_len, inst, &inst_len, &row);
		if (obj == NULL) {
			return -ENOENT;
		}

		memcpy(cur, inst, inst_len * sizeof(uint32_t));
		cur_len = inst_len;

		ret = obj->get(obj, row, val);
		if (ret == -ENOENT) {
			continue;
		}

		if (ret != 0) {
			return ret;
		}

		if (v1 && val->type == SNMP_TYPE_COUNTER64) {
			continue;
		}

		memcpy(oid, cur, cur_len * sizeof(uint32_t));
		*oid_len = cur_len;

		return 0;
	}
}

static struct snmp_status process_get(const struct snmp_request *req, struct snmp_ber_writer *w)
{
	struct snmp_ber_reader list = req->varbinds;
	struct snmp_status status = {SNMP_ERR_NO_ERROR, 0};
	bool v1 = (req->version == SNMP_VERSION_1);
	int32_t index = 0;

	while (!snmp_ber_reader_at_end(&list)) {
		const struct snmp_object *obj;
		uint32_t name[SNMP_OID_MAX_LEN];
		size_t name_len;
		struct snmp_value val;
		uint8_t exception = 0U;
		void *row;
		int ret;

		index++;

		if (parse_varbind(&list, name, &name_len, NULL, NULL) != 0) {
			return (struct snmp_status){SNMP_ERR_GEN_ERR, index};
		}

		switch (snmp_registry_lookup(name, name_len, &obj, &row)) {
		case SNMP_LOOKUP_FOUND:
			ret = obj->get(obj, row, &val);
			if (ret == 0) {
				/* Counter64 is not in view for version 1, RFC 3584 4.2.2.1. */
				if (v1 && val.type == SNMP_TYPE_COUNTER64) {
					return (struct snmp_status){SNMP_ERR_NO_SUCH_NAME, index};
				}

				break;
			}

			if (v1 || ret != -ENOENT) {
				return (struct snmp_status){get_error_status(ret), index};
			}

			exception = SNMP_BER_TAG_NO_SUCH_INSTANCE;
			break;
		case SNMP_LOOKUP_NO_SUCH_OBJECT:
			if (v1) {
				return (struct snmp_status){SNMP_ERR_NO_SUCH_NAME, index};
			}

			exception = SNMP_BER_TAG_NO_SUCH_OBJECT;
			break;
		case SNMP_LOOKUP_NO_SUCH_INSTANCE:
		default:
			if (v1) {
				return (struct snmp_status){SNMP_ERR_NO_SUCH_NAME, index};
			}

			exception = SNMP_BER_TAG_NO_SUCH_INSTANCE;
			break;
		}

		ret = snmp_write_varbind(w, name, name_len, &val, exception);
		if (ret == -EMSGSIZE) {
			return (struct snmp_status){SNMP_ERR_TOO_BIG, 0};
		}

		if (ret != 0) {
			return (struct snmp_status){SNMP_ERR_GEN_ERR, index};
		}
	}

	return status;
}

/*
 * Answer the first limit bindings of the request with their lexicographic
 * successors and ignore the rest. With partial_ok a response that runs out
 * of space is returned as it is instead of failing with tooBig.
 */
static struct snmp_status getnext_bindings(const struct snmp_request *req,
					   struct snmp_ber_writer *w, size_t limit, bool partial_ok)
{
	struct snmp_ber_reader list = req->varbinds;
	bool v1 = (req->version == SNMP_VERSION_1);
	int32_t index = 0;

	while (!snmp_ber_reader_at_end(&list) && (size_t)index < limit) {
		uint32_t name[SNMP_OID_MAX_LEN];
		size_t name_len;
		struct snmp_value val;
		uint8_t exception = 0U;
		int ret;

		index++;

		if (parse_varbind(&list, name, &name_len, NULL, NULL) != 0) {
			return (struct snmp_status){SNMP_ERR_GEN_ERR, index};
		}

		ret = fetch_next(name, &name_len, &val, v1);
		if (ret == -ENOENT) {
			if (v1) {
				return (struct snmp_status){SNMP_ERR_NO_SUCH_NAME, index};
			}

			exception = SNMP_BER_TAG_END_OF_MIB_VIEW;
		} else if (ret != 0) {
			return (struct snmp_status){SNMP_ERR_GEN_ERR, index};
		}

		ret = snmp_write_varbind(w, name, name_len, &val, exception);
		if (ret == -EMSGSIZE) {
			return (struct snmp_status){
				partial_ok ? SNMP_ERR_NO_ERROR : SNMP_ERR_TOO_BIG, 0};
		}

		if (ret != 0) {
			return (struct snmp_status){SNMP_ERR_GEN_ERR, index};
		}
	}

	return (struct snmp_status){SNMP_ERR_NO_ERROR, 0};
}

static struct snmp_status process_getnext(const struct snmp_request *req, struct snmp_ber_writer *w)
{
	return getnext_bindings(req, w, SIZE_MAX, false);
}

static size_t count_varbinds(const struct snmp_request *req)
{
	struct snmp_ber_reader list = req->varbinds;
	size_t count = 0U;

	while (!snmp_ber_reader_at_end(&list) && snmp_ber_skip(&list) == 0) {
		count++;
	}

	return count;
}

static struct snmp_status process_getbulk(const struct snmp_request *req, struct snmp_ber_writer *w)
{
	struct snmp_ber_reader list = req->varbinds;
	int32_t non_repeaters = MAX(req->error_status, 0);
	int32_t max_repetitions = MAX(req->error_index, 0);
	size_t count = count_varbinds(req);
	size_t repeaters = 0U;
	int32_t index = 0;
	int ret;

	/* With more repeaters than there is state for, answer a single
	 * repetition, which needs none: RFC 3416 4.2.3 allows fewer
	 * repetitions than requested.
	 */
	if (count > (size_t)non_repeaters && count - (size_t)non_repeaters > BULK_REPEATERS_MAX) {
		size_t limit = (max_repetitions > 0) ? count : (size_t)non_repeaters;

		return getnext_bindings(req, w, limit, true);
	}

	/* The non-repeaters are answered like a GETNEXT request. */
	while (!snmp_ber_reader_at_end(&list)) {
		uint32_t name[SNMP_OID_MAX_LEN];
		size_t name_len;
		struct snmp_value val;
		uint8_t exception = 0U;

		index++;

		if (parse_varbind(&list, name, &name_len, NULL, NULL) != 0) {
			return (struct snmp_status){SNMP_ERR_GEN_ERR, index};
		}

		if (index > non_repeaters) {
			memcpy(bulk_last[repeaters], name, name_len * sizeof(uint32_t));
			bulk_last_len[repeaters] = name_len;
			bulk_done[repeaters] = false;
			repeaters++;
			continue;
		}

		ret = fetch_next(name, &name_len, &val, false);
		if (ret == -ENOENT) {
			exception = SNMP_BER_TAG_END_OF_MIB_VIEW;
		} else if (ret != 0) {
			return (struct snmp_status){SNMP_ERR_GEN_ERR, index};
		}

		ret = snmp_write_varbind(w, name, name_len, &val, exception);
		if (ret == -EMSGSIZE) {
			/* A partial answer is still a valid one. */
			return (struct snmp_status){SNMP_ERR_NO_ERROR, 0};
		}

		if (ret != 0) {
			return (struct snmp_status){SNMP_ERR_GEN_ERR, index};
		}
	}

	for (int32_t rep = 0; rep < max_repetitions; rep++) {
		bool all_done = true;

		for (size_t i = 0U; i < repeaters; i++) {
			struct snmp_value val;
			uint8_t exception = 0U;

			/* Index of this binding in the request, for error reports */
			index = non_repeaters + (int32_t)i + 1;

			if (!bulk_done[i]) {
				ret = fetch_next(bulk_last[i], &bulk_last_len[i], &val, false);
				if (ret == -ENOENT) {
					bulk_done[i] = true;
				} else if (ret != 0) {
					return (struct snmp_status){SNMP_ERR_GEN_ERR, index};
				}
			}

			if (bulk_done[i]) {
				exception = SNMP_BER_TAG_END_OF_MIB_VIEW;
			} else {
				all_done = false;
			}

			ret = snmp_write_varbind(w, bulk_last[i], bulk_last_len[i], &val,
						 exception);
			if (ret == -EMSGSIZE) {
				return (struct snmp_status){SNMP_ERR_NO_ERROR, 0};
			}

			if (ret != 0) {
				return (struct snmp_status){SNMP_ERR_GEN_ERR, index};
			}
		}

		if (all_done) {
			break;
		}
	}

	return (struct snmp_status){SNMP_ERR_NO_ERROR, 0};
}

/*
 * SET runs in two phases over the request, RFC 3416 4.2.5: every binding is
 * resolved, type checked and validated by its setter first, and only then are
 * the values committed in order. Nothing is rolled back: a commit that fails
 * after another binding was committed is reported as undoFailed.
 */
static struct snmp_status process_set(const struct snmp_request *req, struct snmp_ber_writer *w)
{
	uint32_t oid_buf[SNMP_OID_MAX_LEN];
	bool v1 = (req->version == SNMP_VERSION_1);

	/* The response repeats the request's bindings; RFC 3416 4.2.5 asks for
	 * this to be checked before anything is changed.
	 */
	if (w->size - w->pos < req->varbinds.len) {
		return (struct snmp_status){SNMP_ERR_TOO_BIG, 0};
	}

	for (enum snmp_set_phase phase = SNMP_SET_CHECK; phase <= SNMP_SET_COMMIT; phase++) {
		struct snmp_ber_reader list = req->varbinds;
		int32_t index = 0;

		while (!snmp_ber_reader_at_end(&list)) {
			const struct snmp_object *obj;
			uint32_t name[SNMP_OID_MAX_LEN];
			size_t name_len;
			struct snmp_value val, current;
			void *row;
			int ret;

			index++;

			ret = parse_varbind(&list, name, &name_len, &val, oid_buf);
			if (ret == -ENOTSUP) {
				return (struct snmp_status){SNMP_ERR_WRONG_TYPE, index};
			}

			/* RFC 3416 4.2.5 (4): the value does not decode as its tag
			 * demands, such as an INTEGER wider than 32 bits.
			 */
			if (ret != 0) {
				return (struct snmp_status){SNMP_ERR_WRONG_ENCODING, index};
			}

			switch (snmp_registry_lookup(name, name_len, &obj, &row)) {
			case SNMP_LOOKUP_FOUND:
				break;
			case SNMP_LOOKUP_NO_SUCH_INSTANCE:
				/* RFC 3416 4.2.5 (2) and (7): a row can never be
				 * created here, so noCreation only when a writable
				 * column could otherwise have held the value.
				 */
				return (struct snmp_status){(obj->set == NULL)
								    ? SNMP_ERR_NOT_WRITABLE
								    : SNMP_ERR_NO_CREATION,
							    index};
			case SNMP_LOOKUP_NO_SUCH_OBJECT:
			default:
				return (struct snmp_status){SNMP_ERR_NOT_WRITABLE, index};
			}

			if (obj->set == NULL) {
				return (struct snmp_status){SNMP_ERR_NOT_WRITABLE, index};
			}

			if (phase == SNMP_SET_CHECK) {
				ret = obj->get(obj, row, &current);
				if (ret == -ENOENT) {
					return (struct snmp_status){SNMP_ERR_NO_CREATION, index};
				}

				if (ret != 0) {
					return (struct snmp_status){SNMP_ERR_GEN_ERR, index};
				}

				/* Counter64 is not in view for version 1, RFC 3584 4.2.2.1. */
				if (v1 && current.type == SNMP_TYPE_COUNTER64) {
					return (struct snmp_status){SNMP_ERR_NO_SUCH_NAME, index};
				}

				if (current.type != val.type) {
					return (struct snmp_status){SNMP_ERR_WRONG_TYPE, index};
				}

				ret = obj->set(obj, row, &val, SNMP_SET_CHECK);
				if (ret != 0) {
					return (struct snmp_status){check_error_status(ret), index};
				}

				continue;
			}

			ret = obj->set(obj, row, &val, SNMP_SET_COMMIT);
			if (ret != 0) {
				if (index == 1) {
					return (struct snmp_status){commit_error_status(ret),
								    index};
				}

				return (struct snmp_status){SNMP_ERR_UNDO_FAILED, 0};
			}
		}
	}

	if (write_request_varbinds(w, req) != 0) {
		return (struct snmp_status){SNMP_ERR_TOO_BIG, 0};
	}

	return (struct snmp_status){SNMP_ERR_NO_ERROR, 0};
}

static bool version_supported(int32_t version)
{
	switch (version) {
	case SNMP_VERSION_1:
		return IS_ENABLED(CONFIG_SNMP_V1);
	case SNMP_VERSION_2C:
		return IS_ENABLED(CONFIG_SNMP_V2C);
	default:
		return false;
	}
}

/* Space reserved in front of the variable bindings for the headers that are
 * prepended once their content length is known: every content is bounded by
 * the output buffer, the error status by its largest value and the error
 * index by the number of bindings in the request.
 */
static size_t response_head_size(const struct snmp_request *req, size_t out_size)
{
	return 3U * snmp_ber_sizeof_header(out_size) + snmp_ber_sizeof_int32(req->version) +
	       snmp_ber_sizeof_octets(req->community_len) + snmp_ber_sizeof_int32(req->request_id) +
	       snmp_ber_sizeof_int32(SNMP_ERR_INCONSISTENT_NAME) +
	       snmp_ber_sizeof_int32((int32_t)count_varbinds(req));
}

static size_t finish_response(const struct snmp_request *req, const struct snmp_status *status,
			      uint8_t *out, size_t head, size_t end, size_t *out_start)
{
	size_t start = head;
	int ret;

	ret = snmp_ber_prepend_header(out, &start, SNMP_BER_TAG_SEQUENCE, end - start);
	ret |= snmp_ber_prepend_int32(out, &start, status->error_index);
	ret |= snmp_ber_prepend_int32(out, &start, status->error_status);
	ret |= snmp_ber_prepend_int32(out, &start, req->request_id);
	ret |= snmp_ber_prepend_header(out, &start, SNMP_BER_TAG_RESPONSE, end - start);
	ret |= snmp_ber_prepend_octets(out, &start, SNMP_BER_TAG_OCTET_STRING, req->community,
				       req->community_len);
	ret |= snmp_ber_prepend_int32(out, &start, req->version);
	ret |= snmp_ber_prepend_header(out, &start, SNMP_BER_TAG_SEQUENCE, end - start);
	if (ret != 0) {
		/* The head was sized for these headers. */
		LOG_ERR("Response header does not fit");
		return 0U;
	}

	*out_start = start;

	return end - start;
}

size_t snmp_process_message(const uint8_t *in, size_t in_len, uint8_t *out, size_t out_size,
			    size_t *out_start)
{
	struct snmp_request req;
	struct snmp_status status;
	struct snmp_ber_writer w;
	enum snmp_access access;
	size_t head;

	snmp_stats.in_pkts++;

	if (parse_request(in, in_len, &req) != 0 || validate_varbinds(&req) != 0) {
		snmp_stats.in_asn_parse_errs++;
		LOG_DBG("Dropping malformed message");
		return 0U;
	}

	if (!version_supported(req.version)) {
		snmp_stats.in_bad_versions++;
		LOG_DBG("Dropping version %d message", req.version);
		return 0U;
	}

	access = community_access(req.community, req.community_len);
	if (access == SNMP_ACCESS_NONE) {
		snmp_stats.in_bad_community_names++;
		LOG_DBG("Dropping message with unknown community");
		if (IS_ENABLED(CONFIG_SNMP_NOTIFICATIONS)) {
			snmp_notify_authentication_failure();
		}
		return 0U;
	}

	head = response_head_size(&req, out_size);
	if (head >= out_size) {
		snmp_stats.silent_drops++;
		return 0U;
	}

	snmp_ber_writer_init(&w, out, out_size);
	w.pos = head;

	switch (req.pdu_type) {
	case SNMP_BER_TAG_GET_REQUEST:
		status = process_get(&req, &w);
		break;
	case SNMP_BER_TAG_GET_NEXT_REQUEST:
		status = process_getnext(&req, &w);
		break;
	case SNMP_BER_TAG_GET_BULK_REQUEST:
		if (req.version == SNMP_VERSION_1) {
			LOG_DBG("Dropping GETBULK in a version 1 message");
			return 0U;
		}

		status = process_getbulk(&req, &w);
		break;
	case SNMP_BER_TAG_SET_REQUEST:
		if (access != SNMP_ACCESS_WRITE || !IS_ENABLED(CONFIG_SNMP_SET_ENABLE)) {
			/* RFC 3413 3.2 step 5: the community has no write view. */
			snmp_stats.in_bad_community_uses++;
			status = (struct snmp_status){SNMP_ERR_AUTHORIZATION_ERROR, 0};
		} else {
			status = process_set(&req, &w);
		}
		break;
	default:
		LOG_DBG("Dropping PDU type 0x%02x", req.pdu_type);
		return 0U;
	}

	if (status.error_status != SNMP_ERR_NO_ERROR) {
		if (req.version == SNMP_VERSION_1) {
			status.error_status = v1_error_status(status.error_status);
		}

		/* An error response repeats the request's bindings, except a
		 * SNMPv2c tooBig, which carries none.
		 */
		w.pos = head;
		if (status.error_status != SNMP_ERR_TOO_BIG || req.version == SNMP_VERSION_1) {
			if (write_request_varbinds(&w, &req) != 0) {
				snmp_stats.silent_drops++;
				LOG_DBG("Dropping request whose bindings do not fit");
				return 0U;
			}
		}
	}

	return finish_response(&req, &status, out, head, w.pos, out_start);
}

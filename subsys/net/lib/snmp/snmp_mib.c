/*
 * Copyright (c) 2026 StructCore Embedded
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/snmp/snmp_mib.h>
#include <zephyr/net/snmp/snmp_core.h>
#if defined(__ZEPHYR__)
#include <zephyr/sys/util.h>
#else
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif
#include <string.h>
#include <stddef.h>
#include <errno.h>

#if defined(CONFIG_SNMP) || defined(CONFIG_SNMP_AGENT)

/* MIB-II system group base OID: 1.3.6.1.2.1.1 */
static const uint32_t s_sys_base[7] = {1, 3, 6, 1, 2, 1, 1};

void snmp_mib_lock(struct snmp_agent *inst)
{
	if (inst != NULL) {
		k_mutex_lock(&inst->lock, K_FOREVER);
	}
}

void snmp_mib_unlock(struct snmp_agent *inst)
{
	if (inst != NULL) {
		k_mutex_unlock(&inst->lock);
	}
}

/*
 * MIB-II snmp group (1.3.6.1.2.1.11).
 *
 * The counter nodes carry no user_data: the sub-identifier selects the field of the
 * *receiving agent's* struct snmp_mib2_counters through this read-only table, so every
 * instance reports its own statistics (design contract 6.1). Only subid 30
 * (snmpEnableAuthenTraps) is writable and is an INTEGER, not a Counter32.
 *
 * OID layout of a registered node: 1.3.6.1.2.1.11.<subid>.0, i.e. ids[6] == 11 and
 * ids[7] == subid. Sub-identifiers 7 and 23 do not exist in RFC 3418/RFC 1213.
 */
struct counter_map_ent {
	uint8_t subid;
	uint8_t tag;
	uint16_t off;
};

#define COUNTER_MAP(sub, field, t) {sub, t, (uint16_t)offsetof(struct snmp_mib2_counters, field)}

static const struct counter_map_ent s_counter_map[] = {
	COUNTER_MAP(1, snmpInPkts, SNMP_TAG_COUNTER32),
	COUNTER_MAP(2, snmpOutPkts, SNMP_TAG_COUNTER32),
	COUNTER_MAP(3, snmpInBadVersions, SNMP_TAG_COUNTER32),
	COUNTER_MAP(4, snmpInBadCommunityNames, SNMP_TAG_COUNTER32),
	COUNTER_MAP(5, snmpInBadCommunityUses, SNMP_TAG_COUNTER32),
	COUNTER_MAP(6, snmpInASNParseErrs, SNMP_TAG_COUNTER32),
	COUNTER_MAP(8, snmpInTooBigs, SNMP_TAG_COUNTER32),
	COUNTER_MAP(9, snmpInNoSuchNames, SNMP_TAG_COUNTER32),
	COUNTER_MAP(10, snmpInBadValues, SNMP_TAG_COUNTER32),
	COUNTER_MAP(11, snmpInReadOnlys, SNMP_TAG_COUNTER32),
	COUNTER_MAP(12, snmpInGenErrs, SNMP_TAG_COUNTER32),
	COUNTER_MAP(13, snmpInTotalReqVars, SNMP_TAG_COUNTER32),
	COUNTER_MAP(14, snmpInTotalSetVars, SNMP_TAG_COUNTER32),
	COUNTER_MAP(15, snmpInGetRequests, SNMP_TAG_COUNTER32),
	COUNTER_MAP(16, snmpInGetNexts, SNMP_TAG_COUNTER32),
	COUNTER_MAP(17, snmpInSetRequests, SNMP_TAG_COUNTER32),
	COUNTER_MAP(18, snmpInGetResponses, SNMP_TAG_COUNTER32),
	COUNTER_MAP(19, snmpInTraps, SNMP_TAG_COUNTER32),
	COUNTER_MAP(20, snmpOutTooBigs, SNMP_TAG_COUNTER32),
	COUNTER_MAP(21, snmpOutNoSuchNames, SNMP_TAG_COUNTER32),
	COUNTER_MAP(22, snmpOutBadValues, SNMP_TAG_COUNTER32),
	COUNTER_MAP(24, snmpOutGenErrs, SNMP_TAG_COUNTER32),
	COUNTER_MAP(25, snmpOutGetRequests, SNMP_TAG_COUNTER32),
	COUNTER_MAP(26, snmpOutGetNexts, SNMP_TAG_COUNTER32),
	COUNTER_MAP(27, snmpOutSetRequests, SNMP_TAG_COUNTER32),
	COUNTER_MAP(28, snmpOutGetResponses, SNMP_TAG_COUNTER32),
	COUNTER_MAP(29, snmpOutTraps, SNMP_TAG_COUNTER32),
	COUNTER_MAP(30, snmpEnableAuthenTraps, ASN1_TAG_INTEGER),
	COUNTER_MAP(31, snmpSilentDrops, SNMP_TAG_COUNTER32),
	COUNTER_MAP(32, snmpProxyDrops, SNMP_TAG_COUNTER32),
};

#define SNMP_COUNTER_SUBID_AUTHEN_TRAPS 30

static const struct counter_map_ent *counter_map_lookup(uint32_t subid)
{
	for (size_t i = 0; i < ARRAY_SIZE(s_counter_map); i++) {
		if (s_counter_map[i].subid == subid) {
			return &s_counter_map[i];
		}
	}
	return NULL;
}

/* Callbacks for snmp MIB-II group counters */
static int mib2_counter_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
			    struct snmp_varbind *vb)
{
	if (!agent || !node || !vb) {
		return -EINVAL;
	}

	const struct counter_map_ent *e = counter_map_lookup(node->oid.ids[7]);
	if (!e) {

		return -EINVAL;
	}

	const uint32_t *counter = (const uint32_t *)((const uint8_t *)&agent->counters + e->off);
	vb->type = e->tag;

	vb->val.uint_val = *counter;
	return 0;
}

static int mib2_enable_authen_traps_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
					struct snmp_varbind *vb)
{
	(void)node;
	if (!agent || !vb) {
		return -EINVAL;
	}
	vb->type = ASN1_TAG_INTEGER;
	vb->val.int_val = agent->counters.snmpEnableAuthenTraps;
	return 0;
}

static int mib2_enable_authen_traps_set(struct snmp_agent *agent, struct snmp_mib_node *node,
					const struct snmp_varbind *vb)
{
	(void)node;
	if (!agent || !vb) {
		return -EINVAL;
	}
	if (vb->type != ASN1_TAG_INTEGER) {
		return -EINVAL;
	}
	if (vb->val.int_val != 1 && vb->val.int_val != 2) {
		return -EINVAL;
	}
	agent->counters.snmpEnableAuthenTraps = vb->val.int_val;
	return 0;
}

int snmp_set_device_enterprise_oid(struct snmp_agent *inst, const struct snmp_oid *oid)
{
	if (!inst || !oid || oid->len == 0 || oid->len > CONFIG_SNMP_MAX_OID_LEN) {
		return -EINVAL;
	}
	snmp_mib_lock(inst);
	inst->sys_object_id = *oid;
	snmp_mib_unlock(inst);
	return 0;
}

int snmp_mib2_set_sysdescr(struct snmp_agent *inst, const char *descr)
{
	if (!inst || !descr || strlen(descr) >= sizeof(inst->sys_descr)) {
		return -EINVAL;
	}
	snmp_mib_lock(inst);
	strncpy(inst->sys_descr, descr, sizeof(inst->sys_descr) - 1);
	inst->sys_descr[sizeof(inst->sys_descr) - 1] = '\0';
	snmp_mib_unlock(inst);
	return 0;
}

int snmp_mib2_set_syscontact(struct snmp_agent *inst, const char *contact)
{
	if (!inst || !contact || strlen(contact) >= sizeof(inst->sys_contact)) {
		return -EINVAL;
	}
	snmp_mib_lock(inst);
	strncpy(inst->sys_contact, contact, sizeof(inst->sys_contact) - 1);
	inst->sys_contact[sizeof(inst->sys_contact) - 1] = '\0';
	snmp_mib_unlock(inst);
	return 0;
}

int snmp_mib2_set_sysname(struct snmp_agent *inst, const char *name)
{
	if (!inst || !name || strlen(name) >= sizeof(inst->sys_name)) {
		return -EINVAL;
	}
	snmp_mib_lock(inst);
	strncpy(inst->sys_name, name, sizeof(inst->sys_name) - 1);
	inst->sys_name[sizeof(inst->sys_name) - 1] = '\0';
	snmp_mib_unlock(inst);
	return 0;
}

int snmp_mib2_set_syslocation(struct snmp_agent *inst, const char *location)
{
	if (!inst || !location || strlen(location) >= sizeof(inst->sys_location)) {
		return -EINVAL;
	}
	snmp_mib_lock(inst);
	strncpy(inst->sys_location, location, sizeof(inst->sys_location) - 1);
	inst->sys_location[sizeof(inst->sys_location) - 1] = '\0';
	snmp_mib_unlock(inst);
	return 0;
}

/* Maximum octet string a varbind can carry (union snmp_val::octet_str::data). */
#define SYS_STRING_MAX 128

static int sys_copy_string(const char *src, struct snmp_varbind *vb)
{
	if (!src) {
		return -EINVAL;
	}
	size_t len = strlen(src);
	if (len > SYS_STRING_MAX) {

		len = SYS_STRING_MAX;
	}
	vb->type = ASN1_TAG_OCTET_STRING;
	vb->val.octet_str.len = (uint16_t)len;
	memcpy(vb->val.octet_str.data, src, len);
	return 0;
}

/*
 * Resolve the agent-owned system string for a system-group sub-identifier.
 * @param cap Optional out-parameter receiving the writable capacity (characters,
 *            excluding the NUL terminator).
 * @return Pointer into @p agent, or NULL for a sub-identifier that is not a string.
 */
static char *sys_string_field(struct snmp_agent *agent, uint32_t subid, size_t *cap)
{
	char *p;
	size_t c;

	switch (subid) {
	case 1: /* sysDescr (read-only on the wire, capacity 127) */
		p = agent->sys_descr;
		c = sizeof(agent->sys_descr) - 1;
		break;
	case 4: /* sysContact */
		p = agent->sys_contact;
		c = sizeof(agent->sys_contact) - 1;
		break;
	case 5: /* sysName */
		p = agent->sys_name;
		c = sizeof(agent->sys_name) - 1;
		break;
	case 6: /* sysLocation */
		p = agent->sys_location;
		c = sizeof(agent->sys_location) - 1;
		break;
	default:
		if (cap) {
			*cap = 0;
		}
		return NULL;
	}

	if (cap) {
		*cap = c;
	}
	return p;
}

/*
 * System Group Callbacks (1.3.6.1.2.1.1.<subid>.0).
 *
 * The identity strings live in the receiving agent, so these callbacks select the field
 * from node->oid.ids[7] instead of dereferencing user_data (design contract 6.1).
 */
static int sys_descr_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
			 struct snmp_varbind *vb)
{
	if (!agent || !node || !vb) {
		return -EINVAL;
	}
	if (node->oid.ids[7] != 1) {
		return -EINVAL;
	}
	return sys_copy_string(agent->sys_descr, vb);
}

static int sys_object_id_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
			     struct snmp_varbind *vb)
{
	if (!agent || !node || !vb) {
		return -EINVAL;
	}
	if (node->oid.ids[7] != 2) {
		return -EINVAL;
	}
	vb->type = ASN1_TAG_OID;
	vb->val.oid = agent->sys_object_id;
	return 0;
}

static int sys_uptime_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
			  struct snmp_varbind *vb)
{
	(void)node;
	(void)agent;
	if (!vb) {
		return -EINVAL;
	}
	vb->type = SNMP_TAG_TIMETICKS;
	vb->val.uint_val = (uint32_t)(k_uptime_get() / 10);
	return 0;
}

static int sys_string_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
			  struct snmp_varbind *vb)
{
	if (!agent || !node || !vb) {
		return -EINVAL;
	}
	return sys_copy_string(sys_string_field(agent, node->oid.ids[7], NULL), vb);
}

/*
 * Writable system strings: sysContact.0, sysName.0, sysLocation.0. Capacity is taken
 * from the actual destination array (sysDescr is 128 bytes, the rest 64) instead of the
 * previous "assume at least 64" guess, so a long SET cannot run past a field.
 */
static int sys_string_set(struct snmp_agent *agent, struct snmp_mib_node *node,
			  const struct snmp_varbind *vb)
{
	if (!agent || !node || !vb) {
		return -EINVAL;
	}
	if (vb->type != ASN1_TAG_OCTET_STRING) {
		return -EINVAL;
	}

	size_t cap = 0;
	char *dst = sys_string_field(agent, node->oid.ids[7], &cap);
	if (!dst) {

		return -EINVAL;
	}
	if (vb->val.octet_str.len >= cap) {
		return -EINVAL;
	}

	memcpy(dst, vb->val.octet_str.data, vb->val.octet_str.len);
	dst[vb->val.octet_str.len] = '\0';
	return 0;
}

static int sys_services_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
			    struct snmp_varbind *vb)
{
	(void)node;
	(void)agent;
	if (!vb) {
		return -EINVAL;
	}
	vb->type = ASN1_TAG_INTEGER;
	vb->val.int_val = 72; /* Application + Link layer service */
	return 0;
}

/* Register one 1.3.6.1.2.1.11.<subid>.0 node; user_data stays NULL by design (6.1). */
static int register_counter_node(struct snmp_agent *inst, const uint32_t *base_oid,
				 const struct counter_map_ent *e)
{
	struct snmp_mib_node node;
	memset(&node, 0, sizeof(node));

	node.oid.len = 9;
	memcpy(node.oid.ids, base_oid, 8 * sizeof(uint32_t));
	node.oid.ids[7] = e->subid;
	node.oid.ids[8] = 0;
	node.type = e->tag;
	node.get_cb = (e->subid == SNMP_COUNTER_SUBID_AUTHEN_TRAPS) ? mib2_enable_authen_traps_get
								    : mib2_counter_get;
	node.set_cb =
		(e->subid == SNMP_COUNTER_SUBID_AUTHEN_TRAPS) ? mib2_enable_authen_traps_set : NULL;
	node.user_data = NULL;
	return snmp_mib_register(inst, &node);
}

int snmp_mib_init(struct snmp_agent *inst)
{
	if (!inst) {
		return -EINVAL;
	}

	snmp_mib_lock(inst);
	inst->mib_node_cnt = 0;
	inst->mib_table_cnt = 0;
	memset(inst->mib_nodes, 0, sizeof(inst->mib_nodes));
	memset(inst->mib_tables, 0, sizeof(inst->mib_tables));
	snmp_mib_unlock(inst);
	return 0;
}

static int reg_sys_node_inst(struct snmp_agent *inst, uint8_t subid, uint8_t tag_val,
			     snmp_mib_get_cb_t g_cb, snmp_mib_set_cb_t s_cb)
{
	struct snmp_mib_node n = {0};

	n.oid.len = 9;
	memcpy(n.oid.ids, s_sys_base, sizeof(s_sys_base));
	n.oid.ids[7] = subid;
	n.oid.ids[8] = 0;
	n.type = tag_val;
	n.get_cb = g_cb;
	n.set_cb = s_cb;
	n.user_data = NULL;

	return snmp_mib_register(inst, &n);
}

int snmp_mib_register_defaults(struct snmp_agent *inst)
{
	if (!inst) {
		return -EINVAL;
	}

	/* Register the snmp group (1.3.6.1.2.1.11.x.0) from the read-only map. */
	static const uint32_t base_oid[9] = {1, 3, 6, 1, 2, 1, 11, 0, 0};

	for (size_t i = 0; i < ARRAY_SIZE(s_counter_map); i++) {
		int ret = register_counter_node(inst, base_oid, &s_counter_map[i]);
		if (ret < 0) {

			return ret;
		}
	}

	if (reg_sys_node_inst(inst, 1, ASN1_TAG_OCTET_STRING, sys_descr_get, NULL) < 0 ||
	    reg_sys_node_inst(inst, 2, ASN1_TAG_OID, sys_object_id_get, NULL) < 0 ||
	    reg_sys_node_inst(inst, 3, SNMP_TAG_TIMETICKS, sys_uptime_get, NULL) < 0 ||
	    reg_sys_node_inst(inst, 4, ASN1_TAG_OCTET_STRING, sys_string_get, sys_string_set) < 0 ||
	    reg_sys_node_inst(inst, 5, ASN1_TAG_OCTET_STRING, sys_string_get, sys_string_set) < 0 ||
	    reg_sys_node_inst(inst, 6, ASN1_TAG_OCTET_STRING, sys_string_get, sys_string_set) < 0 ||
	    reg_sys_node_inst(inst, 7, ASN1_TAG_INTEGER, sys_services_get, NULL) < 0) {
		return -1;
	}

	return 0;
}

static int snmp_mib_find_node_idx(const struct snmp_agent *inst, const struct snmp_oid *oid)
{
	if (!inst || !oid || inst->mib_node_cnt == 0) {
		return -1;
	}

	int low = 0;
	int high = (int)inst->mib_node_cnt - 1;

	while (low <= high) {
		int mid = low + (high - low) / 2;
		int cmp = snmp_oid_compare(oid, &inst->mib_nodes[mid].oid);

		if (cmp == 0) {
			return mid;
		} else if (cmp < 0) {
			high = mid - 1;
		} else {
			low = mid + 1;
		}
	}

	return -1;
}

static bool snmp_oid_prefix_match(const struct snmp_oid *oid, const struct snmp_oid *prefix,
				  uint8_t prefix_len)
{
	if (!oid || !prefix || oid->len < prefix_len || prefix->len < prefix_len) {
		return false;
	}
	for (uint8_t i = 0; i < prefix_len; i++) {
		if (oid->ids[i] != prefix->ids[i]) {
			return false;
		}
	}
	return true;
}

int snmp_mib_register(struct snmp_agent *inst, const struct snmp_mib_node *node)
{
	if (!inst || !node || node->oid.len == 0 || node->oid.len > CONFIG_SNMP_MAX_OID_LEN) {
		return -EINVAL;
	}

	snmp_mib_lock(inst);

	int existing_idx = snmp_mib_find_node_idx(inst, &node->oid);
	if (existing_idx >= 0) {

		inst->mib_nodes[existing_idx] = *node;
		snmp_mib_unlock(inst);
		return 0;
	}

	if (inst->mib_node_cnt >= CONFIG_SNMP_MAX_MIB_NODES) {
		snmp_mib_unlock(inst);
		return -ENOMEM;
	}

	/* Insertion sort keeping inst->mib_nodes ordered lexicographically */
	size_t i = inst->mib_node_cnt;
	while (i > 0 && snmp_oid_compare(&inst->mib_nodes[i - 1].oid, &node->oid) > 0) {

		inst->mib_nodes[i] = inst->mib_nodes[i - 1];
		i--;
	}

	inst->mib_nodes[i] = *node;
	inst->mib_node_cnt++;

	snmp_mib_unlock(inst);
	return 0;
}

int snmp_mib_register_nodes(struct snmp_agent *inst, const struct snmp_mib_node *nodes,
			    size_t count)
{
	if (!inst || !nodes || count == 0) {
		return -EINVAL;
	}

	snmp_mib_lock(inst);

	if (inst->mib_node_cnt + count > CONFIG_SNMP_MAX_MIB_NODES) {
		snmp_mib_unlock(inst);
		return -ENOMEM;
	}

	for (size_t j = 0; j < count; j++) {
		inst->mib_nodes[inst->mib_node_cnt + j] = nodes[j];
	}
	inst->mib_node_cnt += count;

	/* Insertion sort to re-establish lexicographic order */
	for (size_t i = 0; i < inst->mib_node_cnt; i++) {
		for (size_t j = i + 1; j < inst->mib_node_cnt; j++) {
			if (snmp_oid_compare(&inst->mib_nodes[i].oid, &inst->mib_nodes[j].oid) >
			    0) {
				struct snmp_mib_node tmp = inst->mib_nodes[i];
				inst->mib_nodes[i] = inst->mib_nodes[j];

				inst->mib_nodes[j] = tmp;
			}
		}
	}

	snmp_mib_unlock(inst);
	return 0;
}

static void get_row_index_suffix(const struct snmp_table_row *r, struct snmp_table_index *out)
{
	if (r->index_oid.len > 0) {
		*out = r->index_oid;
	} else {
		memset(out, 0, sizeof(*out));
		out->len = 1;
		out->subids[0] = r->index;
	}
}

static bool row_matches_index(const struct snmp_table_row *r, uint8_t req_len,
			      const uint32_t *req_subids)
{
	if (r->index_oid.len > 0) {
		return r->index_oid.len == req_len &&
		       memcmp(r->index_oid.subids, req_subids, req_len * sizeof(uint32_t)) == 0;
	}
	return req_len == 1 && r->index == req_subids[0];
}

static bool snmp_table_find_next_entry(const struct snmp_mib_table *tbl, const struct snmp_oid *oid,
				       uint8_t *best_col_idx, struct snmp_table_row **best_row_ptr,
				       struct snmp_oid *best_entry_oid)
{
	if (!tbl || !tbl->rows || tbl->column_cnt == 0) {
		return false;
	}

	bool found = false;
	struct snmp_oid best_oid;
	uint8_t best_c = 0;
	struct snmp_table_row *best_r = NULL;

	for (uint8_t c = 0; c < tbl->column_cnt; c++) {
		uint8_t col_subid = tbl->column_subids[c];
		for (struct snmp_table_row *r = tbl->rows; r != NULL; r = r->next) {

			struct snmp_table_index idx;
			get_row_index_suffix(r, &idx);

			if (tbl->table_oid.len + 1 + idx.len > CONFIG_SNMP_MAX_OID_LEN) {
				continue;
			}

			struct snmp_oid entry_oid;
			entry_oid.len = tbl->table_oid.len + 1 + idx.len;

			for (uint8_t k = 0; k < tbl->table_oid.len; k++) {
				entry_oid.ids[k] = tbl->table_oid.ids[k];
			}
			entry_oid.ids[tbl->table_oid.len] = col_subid;
			for (uint8_t k = 0; k < idx.len; k++) {
				entry_oid.ids[tbl->table_oid.len + 1 + k] = idx.subids[k];
			}

			if (snmp_oid_compare(&entry_oid, oid) > 0) {
				if (!found || snmp_oid_compare(&entry_oid, &best_oid) < 0) {
					best_oid = entry_oid;
					best_c = c;
					best_r = r;
					found = true;
				}
			}
		}
	}

	if (found) {
		*best_col_idx = best_c;
		*best_row_ptr = best_r;
		*best_entry_oid = best_oid;
		return true;
	}
	return false;
}

int snmp_mib_get(struct snmp_agent *inst, const struct snmp_oid *oid, struct snmp_varbind *vb)
{
	if (!inst || !oid || !vb) {
		return -EINVAL;
	}

	snmp_mib_lock(inst);

	/* 1. Check Tables */
	for (size_t t = 0; t < inst->mib_table_cnt; t++) {
		struct snmp_mib_table *tbl = inst->mib_tables[t];

		if (snmp_oid_prefix_match(oid, &tbl->table_oid, tbl->table_oid.len)) {
			if (oid->len > tbl->table_oid.len) {
				uint32_t col_subid = oid->ids[tbl->table_oid.len];
				if (col_subid > 255U) {
					continue;
				}
				bool col_found = false;
				uint8_t col_idx = 0;

				for (uint8_t c = 0; c < tbl->column_cnt; c++) {
					if (tbl->column_subids[c] == (uint8_t)col_subid) {
						col_found = true;
						col_idx = c;
						break;
					}
				}

				if (col_found) {
					if (oid->len > tbl->table_oid.len + 1) {
						uint8_t req_idx_len =
							oid->len - (tbl->table_oid.len + 1);
						const uint32_t *req_idx_subids =
							&oid->ids[tbl->table_oid.len + 1];

						for (struct snmp_table_row *r = tbl->rows;
						     r != NULL; r = r->next) {
							struct snmp_table_index r_idx;
							get_row_index_suffix(r, &r_idx);

							if (r_idx.len == req_idx_len &&
							    memcmp(r_idx.subids, req_idx_subids,
								   req_idx_len *
									   sizeof(uint32_t)) == 0) {
								vb->oid = *oid;
								if (tbl->get_cbs == NULL ||
								    tbl->get_cbs[col_idx] == NULL) {
									snmp_mib_unlock(inst);
									return -EACCES;
								}
								int ret = tbl->get_cbs[col_idx](
									inst, r, col_idx, vb);
								snmp_mib_unlock(inst);
								return ret;
							}
						}
					}
					vb->oid = *oid;
					vb->type = SNMP_TAG_EXCEPT_NO_SUCH_INST;
					snmp_mib_unlock(inst);
					return 0;
				}

				vb->oid = *oid;
				vb->type = SNMP_TAG_EXCEPT_NO_SUCH_OBJ;
				snmp_mib_unlock(inst);
				return 0;
			}

			vb->oid = *oid;
			vb->type = SNMP_TAG_EXCEPT_NO_SUCH_OBJ;
			snmp_mib_unlock(inst);
			return 0;
		}
	}

	/* 2. Check Scalar Exact Match via Binary Search */
	int idx = snmp_mib_find_node_idx(inst, oid);
	if (idx >= 0) {

		vb->oid = *oid;
		int ret = 0;
		if (inst->mib_nodes[idx].get_cb) {

			ret = inst->mib_nodes[idx].get_cb(inst, &inst->mib_nodes[idx], vb);
		} else {
			vb->type = inst->mib_nodes[idx].type;
		}
		snmp_mib_unlock(inst);
		return ret;
	}

	/* 3. Check if queried OID matches the parent object prefix of any registered scalar node */
	for (size_t i = 0; i < inst->mib_node_cnt; i++) {
		const struct snmp_mib_node *node = &inst->mib_nodes[i];
		if (node->oid.len > 1 && oid->len >= node->oid.len - 1) {

			uint8_t parent_len = node->oid.len - 1;
			if (snmp_oid_prefix_match(oid, &node->oid, parent_len)) {

				vb->oid = *oid;
				vb->type = SNMP_TAG_EXCEPT_NO_SUCH_INST;
				snmp_mib_unlock(inst);
				return 0;
			}
		}
	}

	/* 4. No matching object type found */
	snmp_mib_unlock(inst);
	vb->oid = *oid;
	vb->type = SNMP_TAG_EXCEPT_NO_SUCH_OBJ;
	return 0;
}

int snmp_mib_get_next(struct snmp_agent *inst, const struct snmp_oid *oid, struct snmp_varbind *vb)
{
	if (!inst || !oid || !vb) {
		return -EINVAL;
	}

	snmp_mib_lock(inst);

	/* Find best table candidate */
	bool has_table_cand = false;
	struct snmp_oid best_table_oid;
	struct snmp_mib_table *best_tbl = NULL;
	uint8_t best_col_idx = 0;
	struct snmp_table_row *best_row = NULL;

	for (size_t t = 0; t < inst->mib_table_cnt; t++) {
		uint8_t c_idx = 0;
		struct snmp_table_row *r_ptr = NULL;
		struct snmp_oid cand_oid;

		if (snmp_table_find_next_entry(inst->mib_tables[t], oid, &c_idx, &r_ptr,
					       &cand_oid)) {
			if (!has_table_cand || snmp_oid_compare(&cand_oid, &best_table_oid) < 0) {
				has_table_cand = true;
				best_table_oid = cand_oid;
				best_tbl = inst->mib_tables[t];
				best_col_idx = c_idx;
				best_row = r_ptr;
			}
		}
	}

	/* Find best scalar candidate via binary search */
	bool has_scalar_cand = false;
	size_t best_scalar_idx = 0;

	int low = 0;
	int high = (int)inst->mib_node_cnt - 1;
	int cand = -1;
	while (low <= high) {

		int mid = low + (high - low) / 2;
		if (snmp_oid_compare(&inst->mib_nodes[mid].oid, oid) > 0) {

			cand = mid;
			high = mid - 1;
		} else {
			low = mid + 1;
		}
	}
	if (cand >= 0) {
		has_scalar_cand = true;
		best_scalar_idx = (size_t)cand;
	}

	/* Decide between table candidate and scalar candidate */
	bool pick_table = false;
	if (has_table_cand && has_scalar_cand) {

		if (snmp_oid_compare(&best_table_oid, &inst->mib_nodes[best_scalar_idx].oid) < 0) {
			pick_table = true;
		} else {
			pick_table = false;
		}
	} else if (has_table_cand) {
		pick_table = true;
	} else if (has_scalar_cand) {
		pick_table = false;
	} else {
		snmp_mib_unlock(inst);
		vb->oid = *oid;
		vb->type = SNMP_TAG_EXCEPT_END_MIB_VIEW;
		return 0;
	}

	if (pick_table) {
		vb->oid = best_table_oid;
		if (best_row && best_tbl->get_cbs && best_tbl->get_cbs[best_col_idx]) {
			int ret = best_tbl->get_cbs[best_col_idx](inst, best_row, best_col_idx, vb);
			snmp_mib_unlock(inst);

			return ret;
		}
		vb->type = best_tbl->column_types[best_col_idx];
		snmp_mib_unlock(inst);
		return 0;
	}

	vb->oid = inst->mib_nodes[best_scalar_idx].oid;
	int ret = 0;
	if (inst->mib_nodes[best_scalar_idx].get_cb) {

		ret = inst->mib_nodes[best_scalar_idx].get_cb(
			inst, &inst->mib_nodes[best_scalar_idx], vb);
	} else {
		vb->type = inst->mib_nodes[best_scalar_idx].type;
	}
	snmp_mib_unlock(inst);
	return ret;
}

/*
 * Every protocol event increments exactly one counter, on the agent that received the
 * message (design contract 6.2). No global mirror exists any more.
 */
#define MIB_COUNTER_INC(inst, field) snmp_counter_inc(&(inst)->counters.field)

static int snmp_map_error_to_version(int err, int version)
{
	int status;

	if (err == 0) {
		return SNMP_ERR_NO_ERROR;
	} else if (err > 0) {
		status = err;
	} else {
		switch (err) {
		case -EINVAL:
			status = (version == 0) ? SNMP_ERR_BAD_VALUE : SNMP_ERR_WRONG_VALUE;
			break;
		case -EACCES:
		case -EPERM:
			status = (version == 0) ? SNMP_ERR_READ_ONLY : SNMP_ERR_NOT_WRITABLE;
			break;
		case -ENOENT:
			status = (version == 0) ? SNMP_ERR_NO_SUCH_NAME : SNMP_ERR_NOT_WRITABLE;
			break;
		case -ENOMEM:
		case -ENOSPC:
			status = (version == 0) ? SNMP_ERR_GEN_ERR : SNMP_ERR_RESOURCE_UNAVAILABLE;
			break;
		case -ERANGE:
			status = (version == 0) ? SNMP_ERR_BAD_VALUE : SNMP_ERR_WRONG_VALUE;
			break;
		default:
			status = SNMP_ERR_GEN_ERR;
			break;
		}
	}

	if (version == 0) {
		switch (status) {
		case SNMP_ERR_NO_ERROR:
		case SNMP_ERR_TOO_BIG:
		case SNMP_ERR_NO_SUCH_NAME:
		case SNMP_ERR_BAD_VALUE:
		case SNMP_ERR_READ_ONLY:
		case SNMP_ERR_GEN_ERR:
			return status;
		case SNMP_ERR_NO_ACCESS:
		case SNMP_ERR_NOT_WRITABLE:
			return SNMP_ERR_READ_ONLY;
		case SNMP_ERR_WRONG_TYPE:
		case SNMP_ERR_WRONG_LENGTH:
		case SNMP_ERR_WRONG_ENCODING:
		case SNMP_ERR_WRONG_VALUE:
		case SNMP_ERR_INCONSISTENT_VALUE:
			return SNMP_ERR_BAD_VALUE;
		case SNMP_ERR_NO_CREATION:
		case SNMP_ERR_INCONSISTENT_NAME:
			return SNMP_ERR_NO_SUCH_NAME;
		case SNMP_ERR_RESOURCE_UNAVAILABLE:
		case SNMP_ERR_COMMIT_FAILED:
		case SNMP_ERR_UNDO_FAILED:
		case SNMP_ERR_AUTHORIZATION_ERR:
		default:
			return SNMP_ERR_GEN_ERR;
		}
	}

	return status;
}

int snmp_mib_set(struct snmp_agent *inst, const struct snmp_oid *oid, const struct snmp_varbind *vb)
{
	if (!inst || !oid || !vb) {
		return -EINVAL;
	}

	snmp_mib_lock(inst);

	/* Check tables first */
	for (size_t t = 0; t < inst->mib_table_cnt; t++) {
		struct snmp_mib_table *tbl = inst->mib_tables[t];
		if (oid->len <= tbl->table_oid.len + 1) {

			continue;
		}
		if (!snmp_oid_prefix_match(oid, &tbl->table_oid, tbl->table_oid.len)) {
			continue;
		}
		uint32_t col_subid = oid->ids[tbl->table_oid.len];
		if (col_subid > 255U) {
			continue;
		}
		uint8_t req_idx_len = oid->len - (tbl->table_oid.len + 1);
		const uint32_t *req_idx_subids = &oid->ids[tbl->table_oid.len + 1];
		for (uint8_t c = 0; c < tbl->column_cnt; c++) {

			if (tbl->column_subids[c] != (uint8_t)col_subid) {
				continue;
			}
			if (tbl->set_cbs == NULL || tbl->set_cbs[c] == NULL) {
				snmp_mib_unlock(inst);
				MIB_COUNTER_INC(inst, snmpInReadOnlys);
				return -EACCES;
			}
			for (struct snmp_table_row *r = tbl->rows; r != NULL; r = r->next) {
				struct snmp_table_index r_idx;
				get_row_index_suffix(r, &r_idx);

				if (r_idx.len == req_idx_len &&
				    memcmp(r_idx.subids, req_idx_subids,
					   req_idx_len * sizeof(uint32_t)) == 0) {
					int ret = tbl->set_cbs[c](inst, r, c, vb);
					snmp_mib_unlock(inst);

					return ret;
				}
			}
			snmp_mib_unlock(inst);
			MIB_COUNTER_INC(inst, snmpInNoSuchNames);
			return -ENOENT;
		}
	}

	/* Check scalar nodes */
	int s_idx = snmp_mib_find_node_idx(inst, oid);
	if (s_idx >= 0) {

		struct snmp_mib_node *node = &inst->mib_nodes[s_idx];
		if (node->set_tx_cb != NULL) {

			void *tx_ctx = NULL;
			int ret = node->set_tx_cb(inst, node, vb, SNMP_SET_RESERVE, &tx_ctx);
			if (ret == 0) {

				ret = node->set_tx_cb(inst, node, vb, SNMP_SET_COMMIT, &tx_ctx);
				if (ret != 0) {
					(void)node->set_tx_cb(inst, node, vb, SNMP_SET_UNDO,
							      &tx_ctx);
				}
				(void)node->set_tx_cb(inst, node, vb, SNMP_SET_CLEANUP, &tx_ctx);
			}
			snmp_mib_unlock(inst);
			return ret;
		}
		if (node->set_cb == NULL) {
			snmp_mib_unlock(inst);
			MIB_COUNTER_INC(inst, snmpInReadOnlys);
			return -EACCES;
		}
		int ret = node->set_cb(inst, node, vb);
		snmp_mib_unlock(inst);

		return ret;
	}

	snmp_mib_unlock(inst);
	MIB_COUNTER_INC(inst, snmpInNoSuchNames);
	return -ENOENT;
}

int snmp_mib_dispatch(struct snmp_agent *inst, struct snmp_pdu *pdu)
{
	if (!inst || !pdu) {
		return -EINVAL;
	}

	/*
	 * Only the four retrieval/update requests reach the engine. Anything else
	 * (Response, Trap, Report, ...) must never be re-encoded as a response.
	 */
	switch (pdu->type) {
	case SNMP_PDU_GET_REQ:
	case SNMP_PDU_GET_NEXT_REQ:
	case SNMP_PDU_GET_BULK_REQ:
	case SNMP_PDU_SET_REQ:
		break;
	default:
		return -EINVAL;
	}

	pdu->error_status = 0;
	pdu->error_index = 0;

	if (pdu->type == SNMP_PDU_GET_REQ) {
		MIB_COUNTER_INC(inst, snmpInGetRequests);
		for (uint8_t i = 0; i < pdu->varbind_cnt; i++) {
			MIB_COUNTER_INC(inst, snmpInTotalReqVars);
			int ret = snmp_mib_get(inst, &pdu->varbinds[i].oid, &pdu->varbinds[i]);
			if (ret < 0 && pdu->varbinds[i].type != SNMP_TAG_EXCEPT_NO_SUCH_INST) {

				pdu->error_status = 5; /* genErr */
				pdu->error_index = i + 1;
				MIB_COUNTER_INC(inst, snmpOutGenErrs);
				break;
			}
			if (pdu->version == 0 &&
			    (pdu->varbinds[i].type == SNMP_TAG_EXCEPT_NO_SUCH_INST ||
			     pdu->varbinds[i].type == SNMP_TAG_EXCEPT_NO_SUCH_OBJ)) {
				pdu->error_status = 2; /* noSuchName per RFC 1157 */
				pdu->error_index = i + 1;
				pdu->varbinds[i].type = ASN1_TAG_NULL;
				MIB_COUNTER_INC(inst, snmpOutNoSuchNames);
				break;
			}
		}
		pdu->type = SNMP_PDU_GET_RESP;
		MIB_COUNTER_INC(inst, snmpOutGetResponses);
	} else if (pdu->type == SNMP_PDU_GET_NEXT_REQ) {
		MIB_COUNTER_INC(inst, snmpInGetNexts);
		for (uint8_t i = 0; i < pdu->varbind_cnt; i++) {
			MIB_COUNTER_INC(inst, snmpInTotalReqVars);
			int gret =
				snmp_mib_get_next(inst, &pdu->varbinds[i].oid, &pdu->varbinds[i]);
			if (gret < 0) {
				pdu->error_status = SNMP_ERR_GEN_ERR;
				pdu->error_index = i + 1;
				MIB_COUNTER_INC(inst, snmpOutGenErrs);
				break;
			}
			if (pdu->version == 0 &&
			    pdu->varbinds[i].type == SNMP_TAG_EXCEPT_END_MIB_VIEW) {
				pdu->error_status = 2; /* noSuchName per RFC 1157 */
				pdu->error_index = i + 1;
				pdu->varbinds[i].type = ASN1_TAG_NULL;
				MIB_COUNTER_INC(inst, snmpOutNoSuchNames);
				break;
			}
		}
		pdu->type = SNMP_PDU_GET_RESP;
		MIB_COUNTER_INC(inst, snmpOutGetResponses);
	} else if (pdu->type == SNMP_PDU_GET_BULK_REQ) {
		MIB_COUNTER_INC(inst, snmpInGetNexts);
		uint8_t orig_cnt = pdu->varbind_cnt;
		int32_t non_rep = pdu->non_repeaters < 0 ? 0 : pdu->non_repeaters;
		int32_t max_rep = pdu->max_repetitions < 0 ? 0 : pdu->max_repetitions;

		if (non_rep > orig_cnt) {
			non_rep = orig_cnt;
		}
		uint8_t new_cnt = 0;
		int gret = 0;

		snmp_mib_lock(inst);
		struct snmp_varbind *res_vb = inst->scratch.bulk.res_vb;
		struct snmp_oid *current_oids = inst->scratch.bulk.current_oids;

		for (int i = 0; i < non_rep && new_cnt < CONFIG_SNMP_MAX_VARBINDS; i++) {
			res_vb[new_cnt] = pdu->varbinds[i];
			gret = snmp_mib_get_next(inst, &pdu->varbinds[i].oid, &res_vb[new_cnt]);
			new_cnt++;
			if (gret < 0) {
				break;
			}
		}

		uint8_t num_repeaters = orig_cnt - non_rep;
		if (gret == 0 && num_repeaters > 0 && max_rep > 0) {

			for (uint8_t r = 0; r < num_repeaters; r++) {
				current_oids[r] = pdu->varbinds[non_rep + r].oid;
			}

			for (int rep = 0; rep < max_rep; rep++) {
				bool all_end = true;
				for (uint8_t r = 0;

				     r < num_repeaters && new_cnt < CONFIG_SNMP_MAX_VARBINDS; r++) {
					struct snmp_varbind *vb = &res_vb[new_cnt];
					gret = snmp_mib_get_next(inst, &current_oids[r], vb);

					if (gret < 0) {
						break;
					}
					if (vb->type != SNMP_TAG_EXCEPT_END_MIB_VIEW) {
						all_end = false;
						current_oids[r] = vb->oid;
					}
					new_cnt++;
				}
				if (gret < 0 || all_end) {
					break;
				}
			}
		}

		pdu->varbind_cnt = new_cnt;
		memcpy(pdu->varbinds, res_vb, new_cnt * sizeof(struct snmp_varbind));
		snmp_mib_unlock(inst);

		/* RFC 1907: count total variables processed by GetBulk, once, here. */
		snmp_counter_add(&inst->counters.snmpInTotalReqVars, new_cnt);

		pdu->type = SNMP_PDU_GET_RESP;
		MIB_COUNTER_INC(inst, snmpOutGetResponses);

		if (gret < 0) {
			/* A read callback failed mid-sweep: report genErr, keep the prefix. */
			pdu->error_status = SNMP_ERR_GEN_ERR;
			pdu->error_index = new_cnt > 0 ? new_cnt : 1;
			MIB_COUNTER_INC(inst, snmpOutGenErrs);
		}
	} else if (pdu->type == SNMP_PDU_SET_REQ) {
		MIB_COUNTER_INC(inst, snmpInSetRequests);

		snmp_mib_lock(inst);

		struct snmp_set_tx_item *tx_items = inst->scratch.tx_items;
		memset(tx_items, 0, sizeof(inst->scratch.tx_items));

		int error_status = SNMP_ERR_NO_ERROR;
		int error_index = 0;

		/* Phase 1: Reserve & Validation */
		for (uint8_t i = 0; i < pdu->varbind_cnt; i++) {
			MIB_COUNTER_INC(inst, snmpInTotalSetVars);
			struct snmp_varbind *vb = &pdu->varbinds[i];
			struct snmp_set_tx_item *item = &tx_items[i];
			bool found = false;

			/* 1. Check table */
			for (size_t t = 0; t < inst->mib_table_cnt; t++) {
				struct snmp_mib_table *tbl = inst->mib_tables[t];
				if (vb->oid.len <= tbl->table_oid.len + 1) {

					continue;
				}
				if (!snmp_oid_prefix_match(&vb->oid, &tbl->table_oid,
							   tbl->table_oid.len)) {
					continue;
				}
				uint32_t col_subid = vb->oid.ids[tbl->table_oid.len];
				if (col_subid > 255U) {
					continue;
				}
				uint8_t req_idx_len = vb->oid.len - (tbl->table_oid.len + 1);
				const uint32_t *req_idx_subids =
					&vb->oid.ids[tbl->table_oid.len + 1];
				for (uint8_t c = 0; c < tbl->column_cnt; c++) {
					if (tbl->column_subids[c] != (uint8_t)col_subid) {
						continue;
					}
					found = true;
					item->is_table = true;
					item->tbl = tbl;
					item->tbl_col = c;
					if (tbl->set_cbs == NULL || tbl->set_cbs[c] == NULL) {
						error_status = (pdu->version == 0)
								       ? SNMP_ERR_READ_ONLY
								       : SNMP_ERR_NOT_WRITABLE;
						error_index = i + 1;
						break;
					}
					if (tbl->column_types[c] != vb->type) {
						error_status = (pdu->version == 0)
								       ? SNMP_ERR_BAD_VALUE
								       : SNMP_ERR_WRONG_TYPE;
						error_index = i + 1;
						break;
					}
					for (struct snmp_table_row *r = tbl->rows; r != NULL;
					     r = r->next) {
						if (row_matches_index(r, req_idx_len,
								      req_idx_subids)) {
							item->tbl_row = r;
							break;
						}
					}
					if (item->tbl_row == NULL) {
						error_status = (pdu->version == 0)
								       ? SNMP_ERR_NO_SUCH_NAME
								       : SNMP_ERR_NOT_WRITABLE;
						error_index = i + 1;
						break;
					}
					if (tbl->get_cbs && tbl->get_cbs[c]) {
						item->old_vb.oid = vb->oid;
						if (tbl->get_cbs[c](inst, item->tbl_row, c,
								    &item->old_vb) == 0) {
							item->has_old_vb = true;
						}
					}
					item->reserved = true;
					break;
				}
				if (found) {
					break;
				}
			}

			if (error_status != SNMP_ERR_NO_ERROR) {
				break;
			}

			/* 2. Check scalar nodes if not a table column */
			if (!found) {
				int k = snmp_mib_find_node_idx(inst, &vb->oid);
				if (k >= 0) {

					found = true;
					item->node = &inst->mib_nodes[k];
				}

				if (!found) {
					error_status = (pdu->version == 0) ? SNMP_ERR_NO_SUCH_NAME
									   : SNMP_ERR_NOT_WRITABLE;
					error_index = i + 1;
					break;
				}

				if (!item->node->set_tx_cb && !item->node->set_cb) {
					error_status = (pdu->version == 0) ? SNMP_ERR_READ_ONLY
									   : SNMP_ERR_NOT_WRITABLE;
					error_index = i + 1;
					break;
				}

				if (item->node->type != vb->type) {
					error_status = (pdu->version == 0) ? SNMP_ERR_BAD_VALUE
									   : SNMP_ERR_WRONG_TYPE;
					error_index = i + 1;
					break;
				}

				if (item->node->set_tx_cb) {
					int rc = item->node->set_tx_cb(inst, item->node, vb,
								       SNMP_SET_RESERVE,
								       &item->tx_ctx);
					if (rc != 0) {
						error_status =
							snmp_map_error_to_version(rc, pdu->version);
						error_index = i + 1;
						break;
					}
					item->reserved = true;
				} else {
					if (item->node->get_cb) {
						item->old_vb.oid = item->node->oid;
						if (item->node->get_cb(inst, item->node,
								       &item->old_vb) == 0) {
							item->has_old_vb = true;
						}
					}
					item->reserved = true;
				}
			}
		}

		/* If Phase 1 failed, cleanup reserved resources and abort */
		if (error_status != SNMP_ERR_NO_ERROR) {
			for (uint8_t j = 0; j < pdu->varbind_cnt; j++) {
				if (tx_items[j].reserved && tx_items[j].node &&
				    tx_items[j].node->set_tx_cb) {
					tx_items[j].node->set_tx_cb(
						inst, tx_items[j].node, &pdu->varbinds[j],
						SNMP_SET_CLEANUP, &tx_items[j].tx_ctx);
				}
			}
			snmp_mib_unlock(inst);

			pdu->error_status = error_status;
			pdu->error_index = error_index;
			pdu->type = SNMP_PDU_GET_RESP;

			if (error_status == SNMP_ERR_BAD_VALUE ||
			    error_status == SNMP_ERR_WRONG_VALUE) {
				MIB_COUNTER_INC(inst, snmpOutBadValues);
			} else if (error_status == SNMP_ERR_READ_ONLY ||
				   error_status == SNMP_ERR_NOT_WRITABLE) {
				MIB_COUNTER_INC(inst, snmpInReadOnlys);
			} else if (error_status == SNMP_ERR_NO_SUCH_NAME) {
				MIB_COUNTER_INC(inst, snmpOutNoSuchNames);
			} else {
				MIB_COUNTER_INC(inst, snmpOutGenErrs);
			}
			MIB_COUNTER_INC(inst, snmpOutGetResponses);
			return 0;
		}

		/* Phase 2: Commit */
		uint8_t commit_fail_idx = 0;
		bool commit_failed = false;

		for (uint8_t k = 0; k < pdu->varbind_cnt; k++) {
			struct snmp_set_tx_item *item = &tx_items[k];
			struct snmp_varbind *vb = &pdu->varbinds[k];
			int rc = 0;

			if (item->node && item->node->set_tx_cb) {
				rc = item->node->set_tx_cb(inst, item->node, vb, SNMP_SET_COMMIT,
							   &item->tx_ctx);
			} else if (item->node && item->node->set_cb) {
				rc = item->node->set_cb(inst, item->node, vb);
			} else if (item->is_table && item->tbl &&
				   item->tbl->set_cbs[item->tbl_col]) {
				rc = item->tbl->set_cbs[item->tbl_col](inst, item->tbl_row,
								       item->tbl_col, vb);
			}

			if (rc != 0) {
				commit_failed = true;
				commit_fail_idx = k;
				break;
			}
		}

		if (commit_failed) {
			/* Phase 3: Rollback / Undo */
			for (int u = (int)commit_fail_idx - 1; u >= 0; u--) {
				struct snmp_set_tx_item *item = &tx_items[u];
				if (item->node && item->node->set_tx_cb) {

					item->node->set_tx_cb(inst, item->node, &pdu->varbinds[u],
							      SNMP_SET_UNDO, &item->tx_ctx);
				} else if (item->has_old_vb) {
					if (item->node && item->node->set_cb) {
						item->node->set_cb(inst, item->node, &item->old_vb);
					} else if (item->is_table && item->tbl &&
						   item->tbl->set_cbs[item->tbl_col]) {
						item->tbl->set_cbs[item->tbl_col](
							inst, item->tbl_row, item->tbl_col,
							&item->old_vb);
					}
				}
			}

			/* Cleanup all */
			for (uint8_t j = 0; j < pdu->varbind_cnt; j++) {
				if (tx_items[j].reserved && tx_items[j].node &&
				    tx_items[j].node->set_tx_cb) {
					tx_items[j].node->set_tx_cb(
						inst, tx_items[j].node, &pdu->varbinds[j],
						SNMP_SET_CLEANUP, &tx_items[j].tx_ctx);
				}
			}
			snmp_mib_unlock(inst);

			pdu->error_status =
				(pdu->version == 0) ? SNMP_ERR_GEN_ERR : SNMP_ERR_COMMIT_FAILED;
			pdu->error_index = commit_fail_idx + 1;
			pdu->type = SNMP_PDU_GET_RESP;

			MIB_COUNTER_INC(inst, snmpOutGenErrs);
			MIB_COUNTER_INC(inst, snmpOutGetResponses);
			return 0;
		}

		/* Phase 4: Commit Succeeded -> Cleanup */
		for (uint8_t j = 0; j < pdu->varbind_cnt; j++) {
			if (tx_items[j].reserved && tx_items[j].node &&
			    tx_items[j].node->set_tx_cb) {
				tx_items[j].node->set_tx_cb(inst, tx_items[j].node,
							    &pdu->varbinds[j], SNMP_SET_CLEANUP,
							    &tx_items[j].tx_ctx);
			}
		}

		/*
		 * RFC 1157 §4.1.5 and RFC 3416 §4.2.5: The value of the variable-bindings
		 * field of the generated response is that of the received message.
		 * No redundant or error-prone read-back is needed.
		 */

		snmp_mib_unlock(inst);

		pdu->error_status = SNMP_ERR_NO_ERROR;
		pdu->error_index = 0;
		pdu->type = SNMP_PDU_GET_RESP;
		MIB_COUNTER_INC(inst, snmpOutGetResponses);
		return 0;
	}

	/*
	 * Reached by the GET and GETNEXT branches above (GetBulk and Set return from
	 * inside their own branch); the entry guard rejected every other PDU type.
	 */
	return 0;
}

/* --- MIB Table Support --- */

int snmp_mib_register_table(struct snmp_agent *inst, struct snmp_mib_table *table)
{
	if (!inst || !table || table->column_cnt == 0 || !table->column_subids ||
	    !table->column_types || !table->get_cbs) {
		return -EINVAL;
	}

	snmp_mib_lock(inst);

	for (size_t t = 0; t < inst->mib_table_cnt; t++) {
		if (snmp_oid_equal(&inst->mib_tables[t]->table_oid, &table->table_oid)) {
			inst->mib_tables[t] = table;
			snmp_mib_unlock(inst);
			return 0;
		}
	}

	if (inst->mib_table_cnt >= CONFIG_SNMP_MAX_TABLES) {
		snmp_mib_unlock(inst);
		return -ENOMEM;
	}

	inst->mib_tables[inst->mib_table_cnt++] = table;

	snmp_mib_unlock(inst);
	return 0;
}

int snmp_mib_table_add_row(struct snmp_agent *inst, struct snmp_mib_table *table,
			   struct snmp_table_row *row)
{
	if (!table || !row) {
		return -EINVAL;
	}

	snmp_mib_lock(inst);

	row->next = table->rows;
	table->rows = row;

	snmp_mib_unlock(inst);
	return 0;
}

int snmp_mib_table_remove_row_by_index(struct snmp_agent *inst, struct snmp_mib_table *table,
				       const struct snmp_table_index *idx)
{
	if (!table || !idx) {
		return -EINVAL;
	}

	snmp_mib_lock(inst);

	struct snmp_table_row *prev = NULL;
	struct snmp_table_row *cur = table->rows;

	while (cur) {
		struct snmp_table_index cur_idx;
		get_row_index_suffix(cur, &cur_idx);

		if (cur_idx.len == idx->len &&
		    memcmp(cur_idx.subids, idx->subids, idx->len * sizeof(uint32_t)) == 0) {
			if (prev) {
				prev->next = cur->next;
			} else {
				table->rows = cur->next;
			}
			snmp_mib_unlock(inst);
			return 0;
		}
		prev = cur;
		cur = cur->next;
	}

	snmp_mib_unlock(inst);
	return -ENOENT;
}

int snmp_mib_table_remove_row(struct snmp_agent *inst, struct snmp_mib_table *table, uint32_t index)
{
	struct snmp_table_index idx;
	memset(&idx, 0, sizeof(idx));

	idx.len = 1;
	idx.subids[0] = index;
	return snmp_mib_table_remove_row_by_index(inst, table, &idx);
}

/* --- Statistics snapshot (design contract 3.5) --- */

void snmp_agent_get_counters(const struct snmp_agent *agent, struct snmp_mib2_counters *out)
{
	if (agent == NULL || out == NULL) {
		return;
	}
	snmp_mib_lock((struct snmp_agent *)agent);
	*out = agent->counters;
	snmp_mib_unlock((struct snmp_agent *)agent);
}

void snmp_agent_zero_counters(struct snmp_agent *agent)
{
	if (agent == NULL) {
		return;
	}
	snmp_mib_lock(agent);
	memset(&agent->counters, 0, sizeof(agent->counters));
	/* RFC 1907: snmpEnableAuthenTraps is enabled by default (1). */
	agent->counters.snmpEnableAuthenTraps = 1;
	snmp_mib_unlock(agent);
}

#endif /* CONFIG_SNMP_AGENT */

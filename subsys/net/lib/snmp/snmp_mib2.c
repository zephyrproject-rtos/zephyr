/*
 * Copyright (c) 2026 StructCore Embedded
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/snmp/snmp_mib2.h>
#if defined(__ZEPHYR__)
#include <zephyr/sys/util.h>
#else
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif
#include <string.h>
#include <errno.h>

#if defined(__ZEPHYR__) && !defined(CONFIG_ZTEST)
#define SNMP_MIB2_ZEPHYR_NET 1
#endif

#if defined(SNMP_MIB2_ZEPHYR_NET)
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_stats.h>
#if defined(CONFIG_NET_STATISTICS)
extern struct net_stats net_stats;
#endif
#endif

#if !defined(__ZEPHYR__) || defined(CONFIG_ZTEST)
#if defined(__ZEPHYR__)
struct snmp_mock_iface {
	uint32_t index;
	const char *descr;
	uint32_t type;
	uint32_t mtu;
	uint32_t speed;
	uint8_t mac[6];
	uint32_t admin_status;
	uint32_t oper_status;
	uint32_t in_octets;
	uint32_t in_ucast;
	uint32_t in_errors;
	uint32_t out_octets;
	uint32_t out_ucast;
	uint32_t out_errors;
};

void snmp_mib2_mock_set_ifaces(const struct snmp_mock_iface *ifaces, size_t count);
#endif

static struct snmp_mock_iface s_mock_ifaces[CONFIG_SNMP_MAX_IFACES];
static size_t s_mock_iface_cnt;

void snmp_mib2_mock_set_ifaces(const struct snmp_mock_iface *ifaces, size_t count)
{
	if (!ifaces || count == 0) {
		s_mock_iface_cnt = 0;
		return;
	}
	size_t to_copy = count;
	if (to_copy > CONFIG_SNMP_MAX_IFACES) {

		to_copy = CONFIG_SNMP_MAX_IFACES;
	}
	memcpy(s_mock_ifaces, ifaces, to_copy * sizeof(struct snmp_mock_iface));
	s_mock_iface_cnt = to_copy;
}
#endif

/* -------------------------------------------------------------------------- */
/* Interfaces Group (1.3.6.1.2.1.2)                                           */
/* -------------------------------------------------------------------------- */

#if defined(SNMP_MIB2_ZEPHYR_NET)
static void zephyr_count_cb(struct net_if *iface, void *user_data)
{
	(void)iface;
	size_t *cnt = (size_t *)user_data;
	(*cnt)++;
}

static size_t zephyr_get_iface_count(void)
{
	size_t cnt = 0;
	net_if_foreach(zephyr_count_cb, &cnt);

	return cnt;
}

struct iface_collect_ctx {
	struct net_if *ifaces[CONFIG_SNMP_MAX_IFACES];
	size_t count;
};

static void zephyr_iface_collect_cb(struct net_if *iface, void *user_data)
{
	struct iface_collect_ctx *ctx = (struct iface_collect_ctx *)user_data;
	if (ctx->count < CONFIG_SNMP_MAX_IFACES) {

		ctx->ifaces[ctx->count++] = iface;
	}
}
#endif

static int if_number_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
			 struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	if (!vb) {
		return -EINVAL;
	}
	vb->type = ASN1_TAG_INTEGER;
#if defined(SNMP_MIB2_ZEPHYR_NET)
	vb->val.int_val = (int32_t)zephyr_get_iface_count();
#else
	vb->val.int_val = (int32_t)s_mock_iface_cnt;
#endif
	return 0;
}

static int if_col_index_get(struct snmp_agent *agent, const struct snmp_table_row *row,
			    uint8_t column, struct snmp_varbind *vb)
{
	(void)agent;
	(void)column;
	if (!row || !vb) {
		return -EINVAL;
	}
	vb->type = ASN1_TAG_INTEGER;
	vb->val.int_val = (int32_t)row->index;
	return 0;
}

static int if_col_descr_get(struct snmp_agent *agent, const struct snmp_table_row *row,
			    uint8_t column, struct snmp_varbind *vb)
{
	(void)agent;
	(void)column;
	if (!row || !vb) {
		return -EINVAL;
	}
	vb->type = ASN1_TAG_OCTET_STRING;
#if defined(SNMP_MIB2_ZEPHYR_NET)
	struct net_if *iface = (struct net_if *)row->user_data;
	const char *name = NULL;
#if defined(CONFIG_NET_INTERFACE_NAME)
	char ifname[IFNAMSIZ];
	if (net_if_get_name(iface, ifname, sizeof(ifname)) > 0) {

		name = ifname;
	}
#endif
	if (!name && iface && net_if_get_device(iface)) {
		name = net_if_get_device(iface)->name;
	}
	if (!name) {
		name = "eth";
	}
	size_t len = strlen(name);
	if (len > sizeof(vb->val.octet_str.data)) {

		len = sizeof(vb->val.octet_str.data);
	}
	memcpy(vb->val.octet_str.data, name, len);
	vb->val.octet_str.len = (uint16_t)len;
#else
	const struct snmp_mock_iface *m = (const struct snmp_mock_iface *)row->user_data;
	if (!m || !m->descr) {

		vb->val.octet_str.len = 0;
		return 0;
	}
	size_t len = strlen(m->descr);
	if (len > sizeof(vb->val.octet_str.data)) {

		len = sizeof(vb->val.octet_str.data);
	}
	memcpy(vb->val.octet_str.data, m->descr, len);
	vb->val.octet_str.len = (uint16_t)len;
#endif
	return 0;
}

static int if_col_type_get(struct snmp_agent *agent, const struct snmp_table_row *row,
			   uint8_t column, struct snmp_varbind *vb)
{
	(void)agent;
	(void)column;
	if (!row || !vb) {
		return -EINVAL;
	}
	vb->type = ASN1_TAG_INTEGER;
#if defined(SNMP_MIB2_ZEPHYR_NET)
	vb->val.int_val = 6; /* ethernetCsmacd */
#else
	const struct snmp_mock_iface *m = (const struct snmp_mock_iface *)row->user_data;
	vb->val.int_val = m ? (int32_t)m->type : 1;

#endif
	return 0;
}

static int if_col_mtu_get(struct snmp_agent *agent, const struct snmp_table_row *row,
			  uint8_t column, struct snmp_varbind *vb)
{
	(void)agent;
	(void)column;
	if (!row || !vb) {
		return -EINVAL;
	}
	vb->type = ASN1_TAG_INTEGER;
#if defined(SNMP_MIB2_ZEPHYR_NET)
	struct net_if *iface = (struct net_if *)row->user_data;
	vb->val.int_val = iface ? (int32_t)net_if_get_mtu(iface) : 1500;

#else
	const struct snmp_mock_iface *m = (const struct snmp_mock_iface *)row->user_data;
	vb->val.int_val = m ? (int32_t)m->mtu : 1500;

#endif
	return 0;
}

static int if_col_speed_get(struct snmp_agent *agent, const struct snmp_table_row *row,
			    uint8_t column, struct snmp_varbind *vb)
{
	(void)agent;
	(void)column;
	if (!row || !vb) {
		return -EINVAL;
	}
	vb->type = SNMP_TAG_GAUGE32;
#if defined(SNMP_MIB2_ZEPHYR_NET)
	vb->val.uint_val = 10000000;
#else
	const struct snmp_mock_iface *m = (const struct snmp_mock_iface *)row->user_data;
	vb->val.uint_val = m ? m->speed : 10000000;

#endif
	return 0;
}

static int if_col_phys_address_get(struct snmp_agent *agent, const struct snmp_table_row *row,
				   uint8_t column, struct snmp_varbind *vb)
{
	(void)agent;
	(void)column;
	if (!row || !vb) {
		return -EINVAL;
	}
	vb->type = ASN1_TAG_OCTET_STRING;
#if defined(SNMP_MIB2_ZEPHYR_NET)
	struct net_if *iface = (struct net_if *)row->user_data;
	struct net_linkaddr *ll = iface ? net_if_get_link_addr(iface) : NULL;
	if (ll && ll->len > 0) {

		size_t len = ll->len > sizeof(vb->val.octet_str.data)
				     ? sizeof(vb->val.octet_str.data)
				     : ll->len;
		memcpy(vb->val.octet_str.data, ll->addr, len);
		vb->val.octet_str.len = (uint16_t)len;
	} else {
		vb->val.octet_str.len = 0;
	}
#else
	const struct snmp_mock_iface *m = (const struct snmp_mock_iface *)row->user_data;
	if (m) {

		memcpy(vb->val.octet_str.data, m->mac, 6);
		vb->val.octet_str.len = 6;
	} else {
		vb->val.octet_str.len = 0;
	}
#endif
	return 0;
}

static int if_col_admin_status_get(struct snmp_agent *agent, const struct snmp_table_row *row,
				   uint8_t column, struct snmp_varbind *vb)
{
	(void)agent;
	(void)column;
	if (!row || !vb) {
		return -EINVAL;
	}
	vb->type = ASN1_TAG_INTEGER;
#if defined(SNMP_MIB2_ZEPHYR_NET)
	struct net_if *iface = (struct net_if *)row->user_data;
	vb->val.int_val = (iface && net_if_is_admin_up(iface)) ? 1 : 2;

#else
	const struct snmp_mock_iface *m = (const struct snmp_mock_iface *)row->user_data;
	vb->val.int_val = m ? (int32_t)m->admin_status : 1;

#endif
	return 0;
}

static int if_col_oper_status_get(struct snmp_agent *agent, const struct snmp_table_row *row,
				  uint8_t column, struct snmp_varbind *vb)
{
	(void)agent;
	(void)column;
	if (!row || !vb) {
		return -EINVAL;
	}
	vb->type = ASN1_TAG_INTEGER;
#if defined(SNMP_MIB2_ZEPHYR_NET)
	struct net_if *iface = (struct net_if *)row->user_data;
	vb->val.int_val = (iface && net_if_is_up(iface)) ? 1 : 2;

#else
	const struct snmp_mock_iface *m = (const struct snmp_mock_iface *)row->user_data;
	vb->val.int_val = m ? (int32_t)m->oper_status : 1;

#endif
	return 0;
}

static int if_col_last_change_get(struct snmp_agent *agent, const struct snmp_table_row *row,
				  uint8_t column, struct snmp_varbind *vb)
{
	(void)agent;
	(void)row;
	(void)column;
	if (!vb) {
		return -EINVAL;
	}
	vb->type = SNMP_TAG_TIMETICKS;
	vb->val.uint_val = 0;
	return 0;
}

static int if_col_in_octets_get(struct snmp_agent *agent, const struct snmp_table_row *row,
				uint8_t column, struct snmp_varbind *vb)
{
	(void)agent;
	(void)column;
	if (!row || !vb) {
		return -EINVAL;
	}
	vb->type = SNMP_TAG_COUNTER32;
#if defined(SNMP_MIB2_ZEPHYR_NET)
#if defined(CONFIG_NET_STATISTICS_PER_INTERFACE)
	struct net_if *iface = (struct net_if *)row->user_data;
	vb->val.uint_val = iface ? (uint32_t)iface->stats.bytes.received : 0;

#else
	vb->val.uint_val = 0;
#endif
#else
	const struct snmp_mock_iface *m = (const struct snmp_mock_iface *)row->user_data;
	vb->val.uint_val = m ? m->in_octets : 0;

#endif
	return 0;
}

static int if_col_in_ucast_get(struct snmp_agent *agent, const struct snmp_table_row *row,
			       uint8_t column, struct snmp_varbind *vb)
{
	(void)agent;
	(void)column;
	if (!row || !vb) {
		return -EINVAL;
	}
	vb->type = SNMP_TAG_COUNTER32;
#if defined(SNMP_MIB2_ZEPHYR_NET)
#if defined(CONFIG_NET_STATISTICS_PER_INTERFACE)
	struct net_if *iface = (struct net_if *)row->user_data;
	vb->val.uint_val = iface ? (uint32_t)iface->stats.unicast.rx : 0;

#else
	vb->val.uint_val = 0;
#endif
#else
	const struct snmp_mock_iface *m = (const struct snmp_mock_iface *)row->user_data;
	vb->val.uint_val = m ? m->in_ucast : 0;

#endif
	return 0;
}

static int if_col_in_errors_get(struct snmp_agent *agent, const struct snmp_table_row *row,
				uint8_t column, struct snmp_varbind *vb)
{
	(void)agent;
	(void)column;
	if (!row || !vb) {
		return -EINVAL;
	}
	vb->type = SNMP_TAG_COUNTER32;
#if defined(SNMP_MIB2_ZEPHYR_NET)
#if defined(CONFIG_NET_STATISTICS_PER_INTERFACE)
	struct net_if *iface = (struct net_if *)row->user_data;
	vb->val.uint_val = iface ? (uint32_t)iface->stats.errors.rx : 0;

#else
	vb->val.uint_val = 0;
#endif
#else
	const struct snmp_mock_iface *m = (const struct snmp_mock_iface *)row->user_data;
	vb->val.uint_val = m ? m->in_errors : 0;

#endif
	return 0;
}

static int if_col_out_octets_get(struct snmp_agent *agent, const struct snmp_table_row *row,
				 uint8_t column, struct snmp_varbind *vb)
{
	(void)agent;
	(void)column;
	if (!row || !vb) {
		return -EINVAL;
	}
	vb->type = SNMP_TAG_COUNTER32;
#if defined(SNMP_MIB2_ZEPHYR_NET)
#if defined(CONFIG_NET_STATISTICS_PER_INTERFACE)
	struct net_if *iface = (struct net_if *)row->user_data;
	vb->val.uint_val = iface ? (uint32_t)iface->stats.bytes.sent : 0;

#else
	vb->val.uint_val = 0;
#endif
#else
	const struct snmp_mock_iface *m = (const struct snmp_mock_iface *)row->user_data;
	vb->val.uint_val = m ? m->out_octets : 0;

#endif
	return 0;
}

static int if_col_out_ucast_get(struct snmp_agent *agent, const struct snmp_table_row *row,
				uint8_t column, struct snmp_varbind *vb)
{
	(void)agent;
	(void)column;
	if (!row || !vb) {
		return -EINVAL;
	}
	vb->type = SNMP_TAG_COUNTER32;
#if defined(SNMP_MIB2_ZEPHYR_NET)
#if defined(CONFIG_NET_STATISTICS_PER_INTERFACE)
	struct net_if *iface = (struct net_if *)row->user_data;
	vb->val.uint_val = iface ? (uint32_t)iface->stats.unicast.tx : 0;

#else
	vb->val.uint_val = 0;
#endif
#else
	const struct snmp_mock_iface *m = (const struct snmp_mock_iface *)row->user_data;
	vb->val.uint_val = m ? m->out_ucast : 0;

#endif
	return 0;
}

static int if_col_out_errors_get(struct snmp_agent *agent, const struct snmp_table_row *row,
				 uint8_t column, struct snmp_varbind *vb)
{
	(void)agent;
	(void)column;
	if (!row || !vb) {
		return -EINVAL;
	}
	vb->type = SNMP_TAG_COUNTER32;
#if defined(SNMP_MIB2_ZEPHYR_NET)
#if defined(CONFIG_NET_STATISTICS_PER_INTERFACE)
	struct net_if *iface = (struct net_if *)row->user_data;
	vb->val.uint_val = iface ? (uint32_t)iface->stats.errors.tx : 0;

#else
	vb->val.uint_val = 0;
#endif
#else
	const struct snmp_mock_iface *m = (const struct snmp_mock_iface *)row->user_data;
	vb->val.uint_val = m ? m->out_errors : 0;

#endif
	return 0;
}

static const uint8_t s_if_cols[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 14, 16, 17, 20};
static const uint8_t s_if_types[] = {
	ASN1_TAG_INTEGER,      /* 1: ifIndex */
	ASN1_TAG_OCTET_STRING, /* 2: ifDescr */
	ASN1_TAG_INTEGER,      /* 3: ifType */
	ASN1_TAG_INTEGER,      /* 4: ifMtu */
	SNMP_TAG_GAUGE32,      /* 5: ifSpeed */
	ASN1_TAG_OCTET_STRING, /* 6: ifPhysAddress */
	ASN1_TAG_INTEGER,      /* 7: ifAdminStatus */
	ASN1_TAG_INTEGER,      /* 8: ifOperStatus */
	SNMP_TAG_TIMETICKS,    /* 9: ifLastChange */
	SNMP_TAG_COUNTER32,    /* 10: ifInOctets */
	SNMP_TAG_COUNTER32,    /* 11: ifInUcastPkts */
	SNMP_TAG_COUNTER32,    /* 14: ifInErrors */
	SNMP_TAG_COUNTER32,    /* 16: ifOutOctets */
	SNMP_TAG_COUNTER32,    /* 17: ifOutUcastPkts */
	SNMP_TAG_COUNTER32,    /* 20: ifOutErrors */
};
static const snmp_table_get_cb_t s_if_get_cbs[] = {
	if_col_index_get,        if_col_descr_get,       if_col_type_get,
	if_col_mtu_get,          if_col_speed_get,       if_col_phys_address_get,
	if_col_admin_status_get, if_col_oper_status_get, if_col_last_change_get,
	if_col_in_octets_get,    if_col_in_ucast_get,    if_col_in_errors_get,
	if_col_out_octets_get,   if_col_out_ucast_get,   if_col_out_errors_get,
};

int snmp_mib2_register_interfaces(struct snmp_agent *agent)
{
	if (!agent) {
		return -EINVAL;
	}

	/* 1. Register ifNumber.0 */
	static const uint32_t if_base[7] = {1, 3, 6, 1, 2, 1, 2};
	struct snmp_mib_node ifnum_node;
	memset(&ifnum_node, 0, sizeof(ifnum_node));

	ifnum_node.oid.len = 9;
	memcpy(ifnum_node.oid.ids, if_base, 7 * sizeof(uint32_t));
	ifnum_node.oid.ids[7] = 1;
	ifnum_node.oid.ids[8] = 0;
	ifnum_node.type = ASN1_TAG_INTEGER;
	ifnum_node.get_cb = if_number_get;
	ifnum_node.set_cb = NULL;
	ifnum_node.user_data = NULL;
	int ret = snmp_mib_register(agent, &ifnum_node);
	if (ret < 0) {

		return ret;
	}

	/* 2. Configure ifTable */
	memset(&agent->if_table, 0, sizeof(agent->if_table));
	agent->if_table.table_oid.len = 9;
	memcpy(agent->if_table.table_oid.ids, if_base, 7 * sizeof(uint32_t));
	agent->if_table.table_oid.ids[7] = 2;
	agent->if_table.table_oid.ids[8] = 1;
	agent->if_table.column_cnt = ARRAY_SIZE(s_if_cols);
	agent->if_table.column_subids = s_if_cols;
	agent->if_table.column_types = s_if_types;
	agent->if_table.get_cbs = (snmp_table_get_cb_t *)s_if_get_cbs;
	agent->if_table.set_cbs = NULL;
	agent->if_table.rows = NULL;
	agent->if_table.user_data = NULL;

	ret = snmp_mib_register_table(agent, &agent->if_table);
	if (ret < 0) {
		return ret;
	}

	/* 3. Populate rows in reverse order to keep ascending order */
#if defined(SNMP_MIB2_ZEPHYR_NET)
	struct iface_collect_ctx ctx = {0};
	net_if_foreach(zephyr_iface_collect_cb, &ctx);

	for (size_t i = ctx.count; i > 0; i--) {
		size_t idx = i - 1;
		struct snmp_table_row *row = &agent->if_rows[idx];
		memset(row, 0, sizeof(*row));

		row->index = (uint32_t)net_if_get_by_iface(ctx.ifaces[idx]);
		row->user_data = ctx.ifaces[idx];
		ret = snmp_mib_table_add_row(agent, &agent->if_table, row);
		if (ret < 0) {
			return ret;
		}
	}
#else
	for (size_t i = s_mock_iface_cnt; i > 0; i--) {
		size_t idx = i - 1;
		if (idx < CONFIG_SNMP_MAX_IFACES) {

			struct snmp_table_row *row = &agent->if_rows[idx];
			memset(row, 0, sizeof(*row));

			row->index = s_mock_ifaces[idx].index;
			row->user_data = (void *)&s_mock_ifaces[idx];
			ret = snmp_mib_table_add_row(agent, &agent->if_table, row);
			if (ret < 0) {
				return ret;
			}
		}
	}
#endif

	return 0;
}

/* -------------------------------------------------------------------------- */
/* Generic Scalar Helper                                                      */
/* -------------------------------------------------------------------------- */

struct scalar_node_def {
	uint32_t subid;
	uint8_t tag;
	snmp_mib_get_cb_t get_cb;
};

static int register_scalar_group(struct snmp_agent *agent, const uint32_t *base_oid,
				 size_t base_len, const struct scalar_node_def *defs, size_t count)
{
	for (size_t i = 0; i < count; i++) {
		struct snmp_mib_node node;
		memset(&node, 0, sizeof(node));

		node.oid.len = (uint8_t)(base_len + 2);
		memcpy(node.oid.ids, base_oid, base_len * sizeof(uint32_t));
		node.oid.ids[base_len] = defs[i].subid;
		node.oid.ids[base_len + 1] = 0;
		node.type = defs[i].tag;
		node.get_cb = defs[i].get_cb;
		node.set_cb = NULL;
		node.user_data = NULL;
		int ret = snmp_mib_register(agent, &node);
		if (ret < 0) {

			return ret;
		}
	}
	return 0;
}

/* -------------------------------------------------------------------------- */
/* IP Group (1.3.6.1.2.1.4)                                                   */
/* -------------------------------------------------------------------------- */

static int ip_forwarding_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
			     struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	vb->type = ASN1_TAG_INTEGER;
#if defined(SNMP_MIB2_ZEPHYR_NET) && defined(CONFIG_NET_IP_FORWARDING)
	vb->val.int_val = 1;
#else
	vb->val.int_val = 2; /* not-forwarding */
#endif
	return 0;
}

static int ip_default_ttl_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
			      struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	vb->type = ASN1_TAG_INTEGER;
#if defined(SNMP_MIB2_ZEPHYR_NET) && defined(CONFIG_NET_INITIAL_TTL)
	vb->val.int_val = CONFIG_NET_INITIAL_TTL;
#else
	vb->val.int_val = 64;
#endif
	return 0;
}

static int ip_in_receives_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
			      struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	vb->type = SNMP_TAG_COUNTER32;
#if defined(SNMP_MIB2_ZEPHYR_NET)
#if defined(CONFIG_NET_STATISTICS) && defined(CONFIG_NET_STATISTICS_IPV4)
	vb->val.uint_val = (uint32_t)net_stats.ipv4.recv;
#else
	vb->val.uint_val = 0;
#endif
#else
	vb->val.uint_val = 100;
#endif
	return 0;
}

static int ip_in_hdr_errors_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
				struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	vb->type = SNMP_TAG_COUNTER32;
#if defined(SNMP_MIB2_ZEPHYR_NET)
#if defined(CONFIG_NET_STATISTICS)
	vb->val.uint_val = (uint32_t)(net_stats.ip_errors.vhlerr + net_stats.ip_errors.hblenerr +
				      net_stats.ip_errors.lblenerr + net_stats.ip_errors.chkerr);
#else
	vb->val.uint_val = 0;
#endif
#else
	vb->val.uint_val = 0;
#endif
	return 0;
}

static int ip_in_addr_errors_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
				 struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	vb->type = SNMP_TAG_COUNTER32;
	vb->val.uint_val = 0;
	return 0;
}

static int ip_in_discards_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
			      struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	vb->type = SNMP_TAG_COUNTER32;
#if defined(SNMP_MIB2_ZEPHYR_NET)
#if defined(CONFIG_NET_STATISTICS) && defined(CONFIG_NET_STATISTICS_IPV4)
	vb->val.uint_val = (uint32_t)net_stats.ipv4.drop;
#else
	vb->val.uint_val = 0;
#endif
#else
	vb->val.uint_val = 0;
#endif
	return 0;
}

static int ip_in_delivers_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
			      struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	vb->type = SNMP_TAG_COUNTER32;
#if defined(SNMP_MIB2_ZEPHYR_NET)
#if defined(CONFIG_NET_STATISTICS) && defined(CONFIG_NET_STATISTICS_IPV4)
	vb->val.uint_val = (uint32_t)net_stats.ipv4.recv;
#else
	vb->val.uint_val = 0;
#endif
#else
	vb->val.uint_val = 100;
#endif
	return 0;
}

static int ip_out_requests_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
			       struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	vb->type = SNMP_TAG_COUNTER32;
#if defined(SNMP_MIB2_ZEPHYR_NET)
#if defined(CONFIG_NET_STATISTICS) && defined(CONFIG_NET_STATISTICS_IPV4)
	vb->val.uint_val = (uint32_t)net_stats.ipv4.sent;
#else
	vb->val.uint_val = 0;
#endif
#else
	vb->val.uint_val = 50;
#endif
	return 0;
}

static int ip_out_discards_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
			       struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	vb->type = SNMP_TAG_COUNTER32;
	vb->val.uint_val = 0;
	return 0;
}

static const struct scalar_node_def s_ip_defs[] = {
	{1, ASN1_TAG_INTEGER, ip_forwarding_get},
	{2, ASN1_TAG_INTEGER, ip_default_ttl_get},
	{3, SNMP_TAG_COUNTER32, ip_in_receives_get},
	{4, SNMP_TAG_COUNTER32, ip_in_hdr_errors_get},
	{5, SNMP_TAG_COUNTER32, ip_in_addr_errors_get},
	{8, SNMP_TAG_COUNTER32, ip_in_discards_get},
	{9, SNMP_TAG_COUNTER32, ip_in_delivers_get},
	{10, SNMP_TAG_COUNTER32, ip_out_requests_get},
	{11, SNMP_TAG_COUNTER32, ip_out_discards_get},
};

int snmp_mib2_register_ip(struct snmp_agent *agent)
{
	if (!agent) {
		return -EINVAL;
	}
	static const uint32_t ip_base[7] = {1, 3, 6, 1, 2, 1, 4};
	return register_scalar_group(agent, ip_base, 7, s_ip_defs,

				     ARRAY_SIZE(s_ip_defs));
}

/* -------------------------------------------------------------------------- */
/* ICMP Group (1.3.6.1.2.1.5)                                                 */
/* -------------------------------------------------------------------------- */

static int icmp_in_msgs_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
			    struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	vb->type = SNMP_TAG_COUNTER32;
#if defined(SNMP_MIB2_ZEPHYR_NET)
#if defined(CONFIG_NET_STATISTICS) && defined(CONFIG_NET_STATISTICS_ICMP)
	vb->val.uint_val = (uint32_t)net_stats.icmp.recv;
#else
	vb->val.uint_val = 0;
#endif
#else
	vb->val.uint_val = 10;
#endif
	return 0;
}

static int icmp_in_errors_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
			      struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	vb->type = SNMP_TAG_COUNTER32;
#if defined(SNMP_MIB2_ZEPHYR_NET)
#if defined(CONFIG_NET_STATISTICS) && defined(CONFIG_NET_STATISTICS_ICMP)
	vb->val.uint_val =
		(uint32_t)(net_stats.icmp.drop + net_stats.icmp.chkerr + net_stats.icmp.typeerr);
#else
	vb->val.uint_val = 0;
#endif
#else
	vb->val.uint_val = 0;
#endif
	return 0;
}

static int icmp_in_dest_unreachs_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
				     struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	vb->type = SNMP_TAG_COUNTER32;
#if defined(SNMP_MIB2_ZEPHYR_NET)
	vb->val.uint_val = 0;
#else
	vb->val.uint_val = 1;
#endif
	return 0;
}

static int icmp_in_time_excds_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
				  struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	vb->type = SNMP_TAG_COUNTER32;
	vb->val.uint_val = 0;
	return 0;
}

static int icmp_in_echos_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
			     struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	vb->type = SNMP_TAG_COUNTER32;
#if defined(SNMP_MIB2_ZEPHYR_NET)
	vb->val.uint_val = 0;
#else
	vb->val.uint_val = 8;
#endif
	return 0;
}

static int icmp_in_echo_reps_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
				 struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	vb->type = SNMP_TAG_COUNTER32;
#if defined(SNMP_MIB2_ZEPHYR_NET)
	vb->val.uint_val = 0;
#else
	vb->val.uint_val = 1;
#endif
	return 0;
}

static int icmp_out_msgs_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
			     struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	vb->type = SNMP_TAG_COUNTER32;
#if defined(SNMP_MIB2_ZEPHYR_NET)
#if defined(CONFIG_NET_STATISTICS) && defined(CONFIG_NET_STATISTICS_ICMP)
	vb->val.uint_val = (uint32_t)net_stats.icmp.sent;
#else
	vb->val.uint_val = 0;
#endif
#else
	vb->val.uint_val = 12;
#endif
	return 0;
}

static int icmp_out_errors_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
			       struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	vb->type = SNMP_TAG_COUNTER32;
	vb->val.uint_val = 0;
	return 0;
}

static int icmp_out_dest_unreachs_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
				      struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	vb->type = SNMP_TAG_COUNTER32;
#if defined(SNMP_MIB2_ZEPHYR_NET)
	vb->val.uint_val = 0;
#else
	vb->val.uint_val = 2;
#endif
	return 0;
}

static int icmp_out_echo_reps_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
				  struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	vb->type = SNMP_TAG_COUNTER32;
#if defined(SNMP_MIB2_ZEPHYR_NET)
	vb->val.uint_val = 0;
#else
	vb->val.uint_val = 8;
#endif
	return 0;
}

static const struct scalar_node_def s_icmp_defs[] = {
	{1, SNMP_TAG_COUNTER32, icmp_in_msgs_get},
	{2, SNMP_TAG_COUNTER32, icmp_in_errors_get},
	{3, SNMP_TAG_COUNTER32, icmp_in_dest_unreachs_get},
	{4, SNMP_TAG_COUNTER32, icmp_in_time_excds_get},
	{8, SNMP_TAG_COUNTER32, icmp_in_echos_get},
	{9, SNMP_TAG_COUNTER32, icmp_in_echo_reps_get},
	{14, SNMP_TAG_COUNTER32, icmp_out_msgs_get},
	{15, SNMP_TAG_COUNTER32, icmp_out_errors_get},
	{16, SNMP_TAG_COUNTER32, icmp_out_dest_unreachs_get},
	{21, SNMP_TAG_COUNTER32, icmp_out_echo_reps_get},
};

int snmp_mib2_register_icmp(struct snmp_agent *agent)
{
	if (!agent) {
		return -EINVAL;
	}
	static const uint32_t icmp_base[7] = {1, 3, 6, 1, 2, 1, 5};
	return register_scalar_group(agent, icmp_base, 7, s_icmp_defs,

				     ARRAY_SIZE(s_icmp_defs));
}

/* -------------------------------------------------------------------------- */
/* TCP Group (1.3.6.1.2.1.6)                                                  */
/* -------------------------------------------------------------------------- */

static int tcp_rto_algorithm_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
				 struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	vb->type = ASN1_TAG_INTEGER;
	vb->val.int_val = 4; /* Van Jacobson's algorithm */
	return 0;
}

static int tcp_rto_min_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
			   struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	vb->type = ASN1_TAG_INTEGER;
	vb->val.int_val = 1000; /* 1000 ms */
	return 0;
}

static int tcp_rto_max_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
			   struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	vb->type = ASN1_TAG_INTEGER;
	vb->val.int_val = 60000; /* 60000 ms */
	return 0;
}

static int tcp_max_conn_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
			    struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	vb->type = ASN1_TAG_INTEGER;
	vb->val.int_val = -1; /* dynamic */
	return 0;
}

static int tcp_active_opens_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
				struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	vb->type = SNMP_TAG_COUNTER32;
#if defined(SNMP_MIB2_ZEPHYR_NET)
	vb->val.uint_val = 0;
#else
	vb->val.uint_val = 5;
#endif
	return 0;
}

static int tcp_passive_opens_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
				 struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	vb->type = SNMP_TAG_COUNTER32;
#if defined(SNMP_MIB2_ZEPHYR_NET)
	vb->val.uint_val = 0;
#else
	vb->val.uint_val = 2;
#endif
	return 0;
}

static int tcp_in_segs_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
			   struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	vb->type = SNMP_TAG_COUNTER32;
#if defined(SNMP_MIB2_ZEPHYR_NET)
#if defined(CONFIG_NET_STATISTICS) && defined(CONFIG_NET_STATISTICS_TCP)
	vb->val.uint_val = (uint32_t)net_stats.tcp.recv;
#else
	vb->val.uint_val = 0;
#endif
#else
	vb->val.uint_val = 500;
#endif
	return 0;
}

static int tcp_out_segs_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
			    struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	vb->type = SNMP_TAG_COUNTER32;
#if defined(SNMP_MIB2_ZEPHYR_NET)
#if defined(CONFIG_NET_STATISTICS) && defined(CONFIG_NET_STATISTICS_TCP)
	vb->val.uint_val = (uint32_t)net_stats.tcp.sent;
#else
	vb->val.uint_val = 0;
#endif
#else
	vb->val.uint_val = 450;
#endif
	return 0;
}

static const struct scalar_node_def s_tcp_defs[] = {
	{1, ASN1_TAG_INTEGER, tcp_rto_algorithm_get},
	{2, ASN1_TAG_INTEGER, tcp_rto_min_get},
	{3, ASN1_TAG_INTEGER, tcp_rto_max_get},
	{4, ASN1_TAG_INTEGER, tcp_max_conn_get},
	{5, SNMP_TAG_COUNTER32, tcp_active_opens_get},
	{6, SNMP_TAG_COUNTER32, tcp_passive_opens_get},
	{10, SNMP_TAG_COUNTER32, tcp_in_segs_get},
	{11, SNMP_TAG_COUNTER32, tcp_out_segs_get},
};

int snmp_mib2_register_tcp(struct snmp_agent *agent)
{
	if (!agent) {
		return -EINVAL;
	}
	static const uint32_t tcp_base[7] = {1, 3, 6, 1, 2, 1, 6};
	return register_scalar_group(agent, tcp_base, 7, s_tcp_defs,

				     ARRAY_SIZE(s_tcp_defs));
}

/* -------------------------------------------------------------------------- */
/* UDP Group (1.3.6.1.2.1.7)                                                  */
/* -------------------------------------------------------------------------- */

static int udp_in_datagrams_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
				struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	vb->type = SNMP_TAG_COUNTER32;
#if defined(SNMP_MIB2_ZEPHYR_NET)
#if defined(CONFIG_NET_STATISTICS) && defined(CONFIG_NET_STATISTICS_UDP)
	vb->val.uint_val = (uint32_t)net_stats.udp.recv;
#else
	vb->val.uint_val = 0;
#endif
#else
	vb->val.uint_val = 200;
#endif
	return 0;
}

static int udp_no_ports_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
			    struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	vb->type = SNMP_TAG_COUNTER32;
#if defined(SNMP_MIB2_ZEPHYR_NET)
	vb->val.uint_val = 0;
#else
	vb->val.uint_val = 3;
#endif
	return 0;
}

static int udp_in_errors_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
			     struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	vb->type = SNMP_TAG_COUNTER32;
#if defined(SNMP_MIB2_ZEPHYR_NET)
#if defined(CONFIG_NET_STATISTICS) && defined(CONFIG_NET_STATISTICS_UDP)
	vb->val.uint_val = (uint32_t)(net_stats.udp.drop + net_stats.udp.chkerr);
#else
	vb->val.uint_val = 0;
#endif
#else
	vb->val.uint_val = 1;
#endif
	return 0;
}

static int udp_out_datagrams_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
				 struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	vb->type = SNMP_TAG_COUNTER32;
#if defined(SNMP_MIB2_ZEPHYR_NET)
#if defined(CONFIG_NET_STATISTICS) && defined(CONFIG_NET_STATISTICS_UDP)
	vb->val.uint_val = (uint32_t)net_stats.udp.sent;
#else
	vb->val.uint_val = 0;
#endif
#else
	vb->val.uint_val = 150;
#endif
	return 0;
}

static const struct scalar_node_def s_udp_defs[] = {
	{1, SNMP_TAG_COUNTER32, udp_in_datagrams_get},
	{2, SNMP_TAG_COUNTER32, udp_no_ports_get},
	{3, SNMP_TAG_COUNTER32, udp_in_errors_get},
	{4, SNMP_TAG_COUNTER32, udp_out_datagrams_get},
};

int snmp_mib2_register_udp(struct snmp_agent *agent)
{
	if (!agent) {
		return -EINVAL;
	}
	static const uint32_t udp_base[7] = {1, 3, 6, 1, 2, 1, 7};
	return register_scalar_group(agent, udp_base, 7, s_udp_defs,

				     ARRAY_SIZE(s_udp_defs));
}

/* -------------------------------------------------------------------------- */
/* Master Registration Helper                                                 */
/* -------------------------------------------------------------------------- */

int snmp_mib2_register_all(struct snmp_agent *agent)
{
	int ret;

	ret = snmp_mib2_register_interfaces(agent);
	if (ret < 0) {
		return ret;
	}

	ret = snmp_mib2_register_ip(agent);
	if (ret < 0) {
		return ret;
	}

	ret = snmp_mib2_register_icmp(agent);
	if (ret < 0) {
		return ret;
	}

	ret = snmp_mib2_register_tcp(agent);
	if (ret < 0) {
		return ret;
	}

	ret = snmp_mib2_register_udp(agent);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

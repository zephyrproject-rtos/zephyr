/* @file
 * @brief Bluetooth PACS
 */

/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/byteorder.h>

#include <device.h>
#include <init.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/audio.h>
#include "../conn_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_PACS)
#define LOG_MODULE_NAME bt_pacs
#include "common/log.h"

#include "pacs_internal.h"

#define PAC_INDICATE_TIMEOUT	K_MSEC(10)

#if defined(CONFIG_BT_PAC_SNK) || defined(CONFIG_BT_PAC_SRC)
NET_BUF_SIMPLE_DEFINE_STATIC(read_buf, CONFIG_BT_L2CAP_TX_MTU);

static void pac_data_add(struct net_buf_simple *buf, uint8_t num,
			 struct bt_codec_data *data)
{
	struct bt_pac_codec_capability *cc;
	int i;

	for (i = 0; i < num; i++) {
		struct bt_data *d = &data[i].data;

		cc = net_buf_simple_add(buf, sizeof(*cc));
		cc->len = d->data_len + sizeof(cc->type);
		cc->type = d->type;
		net_buf_simple_add_mem(buf, d->data, d->data_len);
	}
}

static ssize_t pac_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, uint16_t len, uint16_t offset)
{
	sys_slist_t *lst = attr->user_data;
	struct bt_audio_cap *cap;
	struct bt_pacs_read_rsp *rsp;

	/* Reset if buffer before using */
	net_buf_simple_reset(&read_buf);

	rsp = net_buf_simple_add(&read_buf, sizeof(*rsp));
	rsp->num_pac = 0;

	SYS_SLIST_FOR_EACH_CONTAINER(lst, cap, node) {
		struct bt_pac *pac;
		struct bt_pac_meta *meta;

		pac = net_buf_simple_add(&read_buf, sizeof(*pac));

		pac->codec.id = cap->codec->id;
		pac->codec.cid = sys_cpu_to_le16(cap->codec->cid);
		pac->codec.vid = sys_cpu_to_le16(cap->codec->vid);
		pac->cc_len = read_buf.len;

		pac_data_add(&read_buf, cap->codec->data_count,
			     cap->codec->data);

		/* Buffer size shall never be bellow PAC len since we are just
		 * append data.
		 */
		__ASSERT_NO_MSG(read_buf.len >= pac->cc_len);

		pac->cc_len = read_buf.len - pac->cc_len;

		meta = net_buf_simple_add(&read_buf, sizeof(*meta));
		meta->len = read_buf.len;
		pac_data_add(&read_buf, cap->codec->meta_count,
			     cap->codec->meta);
		meta->len = read_buf.len - meta->len;

		BT_DBG("pac #%u: codec capability len %u metadata len %u",
		       rsp->num_pac, pac->cc_len, meta->len);

		rsp->num_pac++;
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, read_buf.data,
				 read_buf.len);
}

static void context_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	BT_DBG("attr %p value 0x%04x", attr, value);
}

static ssize_t context_read(struct bt_conn *conn,
			    const struct bt_gatt_attr *attr, void *buf,
			    uint16_t len, uint16_t offset)
{
	struct bt_pacs_context context = {
		/* TODO: This should reflect the ongoing channel contexts */
		.snk = 0x0001,
		.src = 0x0001,
	};

	BT_DBG("conn %p attr %p buf %p len %u offset %u", conn, attr, buf, len,
	       offset);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &context,
				 sizeof(context));
}

static void supported_context_cfg_changed(const struct bt_gatt_attr *attr,
					  uint16_t value)
{
	BT_DBG("attr %p value 0x%04x", attr, value);
}

static ssize_t supported_context_read(struct bt_conn *conn,
				      const struct bt_gatt_attr *attr,
				      void *buf, uint16_t len, uint16_t offset)
{
	struct bt_pacs_context context = {
#if defined(CONFIG_BT_PAC_SNK)
		.snk = sys_cpu_to_le16(CONFIG_BT_PACS_SNK_CONTEXT),
#endif
#if defined(CONFIG_BT_PAC_SRC)
		.src = sys_cpu_to_le16(CONFIG_BT_PACS_SRC_CONTEXT),
#endif
	};

	BT_DBG("conn %p attr %p buf %p len %u offset %u", conn, attr, buf, len,
	       offset);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &context,
				 sizeof(context));
}
#endif /* CONFIG_BT_PAC_SNK || CONFIG_BT_PAC_SRC */

#if defined(CONFIG_BT_PAC_SNK)
static sys_slist_t snks;
static struct k_work_delayable snks_work;
static uint32_t snk_loc = CONFIG_BT_PAC_SNK_LOC;

static ssize_t snk_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, uint16_t len, uint16_t offset)
{
	BT_DBG("conn %p attr %p buf %p len %u offset %u", conn, attr, buf, len,
	       offset);

	return pac_read(conn, attr, buf, len, offset);
}

static ssize_t snk_loc_read(struct bt_conn *conn,
			    const struct bt_gatt_attr *attr, void *buf,
			    uint16_t len, uint16_t offset)
{
	BT_DBG("conn %p attr %p buf %p len %u offset %u", conn, attr, buf, len,
	       offset);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &snk_loc,
				 sizeof(snk_loc));
}

static ssize_t snk_loc_write(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, const void *data,
			     uint16_t len, uint16_t offset, uint8_t flags)
{
	if (offset) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != sizeof(snk_loc)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	snk_loc = sys_get_le32(data);

	return len;
}

static void snk_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	BT_DBG("attr %p value 0x%04x", attr, value);
}

static void snk_loc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	BT_DBG("attr %p value 0x%04x", attr, value);
}
#endif /* CONFIG_BT_PAC_SNK */

#if defined(CONFIG_BT_PAC_SRC)
static sys_slist_t srcs;
static struct k_work_delayable srcs_work;
static uint32_t src_loc = CONFIG_BT_PAC_SRC_LOC;

static ssize_t src_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, uint16_t len, uint16_t offset)
{
	BT_DBG("conn %p attr %p buf %p len %u offset %u", conn, attr, buf, len,
	       offset);

	return pac_read(conn, attr, buf, len, offset);
}

static ssize_t src_loc_read(struct bt_conn *conn,
			    const struct bt_gatt_attr *attr, void *buf,
			    uint16_t len, uint16_t offset)
{
	BT_DBG("conn %p attr %p buf %p len %u offset %u", conn, attr, buf, len,
	       offset);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &src_loc,
				 sizeof(src_loc));
}

static ssize_t src_loc_write(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, const void *data,
			     uint16_t len, uint16_t offset, uint8_t flags)
{
	if (offset) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != sizeof(src_loc)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	src_loc = sys_get_le32(data);

	return len;
}

static void src_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	BT_DBG("attr %p value 0x%04x", attr, value);
}

static void src_loc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	BT_DBG("attr %p value 0x%04x", attr, value);
}
#endif /* CONFIG_BT_PAC_SRC */

#if defined(CONFIG_BT_PAC_SNK) || defined(CONFIG_BT_PAC_SRC)
BT_GATT_SERVICE_DEFINE(pacs_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_PACS),
#if defined(CONFIG_BT_PAC_SNK)
	BT_GATT_CHARACTERISTIC(BT_UUID_PACS_SNK,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ_ENCRYPT,
			       snk_read, NULL, &snks),
	BT_GATT_CCC(snk_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),
	BT_GATT_CHARACTERISTIC(BT_UUID_PACS_SNK_LOC,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE |
			       BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ_ENCRYPT |
			       BT_GATT_PERM_WRITE_ENCRYPT,
			       snk_loc_read, snk_loc_write, NULL),
	BT_GATT_CCC(snk_loc_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),
#endif /* CONFIG_BT_PAC_SNK */
#if defined(CONFIG_BT_PAC_SRC)
	BT_GATT_CHARACTERISTIC(BT_UUID_PACS_SRC,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_INDICATE,
			       BT_GATT_PERM_READ_ENCRYPT,
			       src_read, NULL, &srcs),
	BT_GATT_CCC(src_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),
	BT_GATT_CHARACTERISTIC(BT_UUID_PACS_SRC_LOC,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE |
			       BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ_ENCRYPT |
			       BT_GATT_PERM_WRITE_ENCRYPT,
			       src_loc_read, src_loc_write, NULL),
	BT_GATT_CCC(src_loc_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),
#endif /* CONFIG_BT_PAC_SNK */
	BT_GATT_CHARACTERISTIC(BT_UUID_PACS_CONTEXT,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ_ENCRYPT,
			       context_read, NULL, NULL),
	BT_GATT_CCC(context_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),
	BT_GATT_CHARACTERISTIC(BT_UUID_PACS_SUPPORTED_CONTEXT,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ_ENCRYPT,
			       supported_context_read, NULL, NULL),
	BT_GATT_CCC(supported_context_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),
);
#endif /* CONFIG_BT_PAC_SNK || CONFIG_BT_PAC_SRC */

int bt_audio_cap_get(uint8_t type, sys_slist_t **lst,
		     struct k_work_delayable **work)
{
	switch (type) {
#if defined(CONFIG_BT_PAC_SNK)
	case BT_AUDIO_SINK:
		if (lst) {
			*lst = &snks;
		}

		if (work) {
			*work = &snks_work;
		}

		return 0;
#endif /* CONFIG_BT_PAC_SNK */
#if defined(CONFIG_BT_PAC_SRC)
	case BT_AUDIO_SOURCE:
		if (lst) {
			*lst = &srcs;
		}

		if (work) {
			*work = &srcs_work;
		}

		return 0;
#endif /* CONFIG_BT_PAC_SNK */
	}

	return -ENOTSUP;
}

static void pac_indicate(struct k_work *work)
{
#if defined(CONFIG_BT_PAC_SNK) || defined(CONFIG_BT_PAC_SRC)
	struct bt_gatt_indicate_params params;

#if defined(CONFIG_BT_PAC_SNK)
	if (work == &snks_work.work) {
		params.uuid = BT_UUID_PACS_SNK;
	}
#endif /* CONFIG_BT_PAC_SNK */

#if defined(CONFIG_BT_PAC_SRC)
	if (work == &srcs_work.work) {
		params.uuid = BT_UUID_PACS_SRC;
	}
#endif /* CONFIG_BT_PAC_SRC */

	params.attr = pacs_svc.attrs;

	bt_gatt_indicate(NULL, &params);
#endif /* CONFIG_BT_PAC_SNK || CONFIG_BT_PAC_SRC */
}

/* Register Audio Capability */
int bt_audio_register(struct bt_audio_cap *cap)
{
	sys_slist_t *lst;
	struct k_work_delayable *work;
	int err;

	if (!cap || !cap->codec) {
		return -EINVAL;
	}

	BT_DBG("cap %p type 0x%02x codec 0x%02x codec cid 0x%04x "
	       "codec vid 0x%04x", cap, cap->type, cap->codec->id,
	       cap->codec->cid, cap->codec->vid);

	err = bt_audio_cap_get(cap->type, &lst, &work);
	if (err < 0) {
		return err;
	}

	sys_slist_append(lst, &cap->node);

	/* Initialize handler if it hasn't been initialized */
	if (!work->work.handler) {
		k_work_init_delayable(work, pac_indicate);
	}

	k_work_reschedule(work, PAC_INDICATE_TIMEOUT);

	return 0;
}

/* Unregister Audio Capability */
int bt_audio_unregister(struct bt_audio_cap *cap)
{
	sys_slist_t *lst;
	struct k_work_delayable *work;
	int err;

	if (!cap) {
		return -EINVAL;
	}

	BT_DBG("cap %p type 0x%02x", cap, cap->type);

	err = bt_audio_cap_get(cap->type, &lst, &work);
	if (err < 0) {
		return err;
	}

	if (!sys_slist_find_and_remove(lst, &cap->node)) {
		return -ENOENT;
	}

	k_work_reschedule(work, PAC_INDICATE_TIMEOUT);

	return 0;
}

/* @file
 * @brief Bluetooth PACS
 */

/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2022 Nordic Semiconductor ASA
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
#include <bluetooth/audio/audio.h>
#include <bluetooth/audio/capabilities.h>
#include "../host/conn_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_PACS)
#define LOG_MODULE_NAME bt_pacs
#include "common/log.h"

#include "pacs_internal.h"
#include "unicast_server.h"

#define PAC_NOTIFY_TIMEOUT	K_MSEC(10)

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

static void get_pac_records(struct bt_conn *conn, uint8_t type,
			    struct net_buf_simple *buf)
{
	struct bt_pacs_read_rsp *rsp;

	/* Reset if buffer before using */
	net_buf_simple_reset(buf);

	rsp = net_buf_simple_add(&read_buf, sizeof(*rsp));
	rsp->num_pac = 0;

	if (unicast_server_cb == NULL ||
	    unicast_server_cb->publish_capability == NULL) {
		return;
	}

	while (true) {
		struct bt_pac_meta *meta;
		struct bt_codec codec;
		struct bt_pac *pac;
		int err;

		err = unicast_server_cb->publish_capability(conn, type,
							    rsp->num_pac,
							    &codec);
		if (err != 0) {
			break;
		}

		pac = net_buf_simple_add(&read_buf, sizeof(*pac));

		pac->codec.id = codec.id;
		pac->codec.cid = sys_cpu_to_le16(codec.cid);
		pac->codec.vid = sys_cpu_to_le16(codec.vid);
		pac->cc_len = read_buf.len;

		pac_data_add(&read_buf, codec.data_count, codec.data);

		/* Buffer size shall never be below PAC len since we are just
		 * append data.
		 */
		__ASSERT_NO_MSG(read_buf.len >= pac->cc_len);

		pac->cc_len = read_buf.len - pac->cc_len;

		meta = net_buf_simple_add(&read_buf, sizeof(*meta));
		meta->len = read_buf.len;
		pac_data_add(&read_buf, codec.meta_count, codec.meta);
		meta->len = read_buf.len - meta->len;

		BT_DBG("pac #%u: codec capability len %u metadata len %u",
		       rsp->num_pac, pac->cc_len, meta->len);

		rsp->num_pac++;
	}
}

static ssize_t pac_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, uint16_t len, uint16_t offset)
{
	uint8_t type;

	if (!bt_uuid_cmp(attr->uuid, BT_UUID_PACS_SNK)) {
		type = BT_AUDIO_SINK;
	} else {
		type = BT_AUDIO_SOURCE;
	}

	get_pac_records(conn, type, &read_buf);

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

#if defined(CONFIG_BT_PAC_SNK_LOC) || defined(CONFIG_BT_PAC_SRC_LOC)
static int get_pac_loc(struct bt_conn *conn, enum bt_audio_pac_type type,
		       enum bt_audio_location *location)
{
	int err;

	if (unicast_server_cb == NULL ||
	    unicast_server_cb->publish_location == NULL) {
		BT_WARN("No callback for publish_location");
		return -ENODATA;
	}

	err = unicast_server_cb->publish_location(conn, type, location);
	if (err != 0 || *location == 0) {
		BT_DBG("err (%d) or invalid location value (%u)",
		       err, *location);
		return -ENODATA;
	}

	return 0;
}
#endif /* CONFIG_BT_PAC_SNK_LOC || CONFIG_BT_PAC_SRC_LOC */
#endif /* CONFIG_BT_PAC_SNK || CONFIG_BT_PAC_SRC */

#if defined(CONFIG_BT_PAC_SNK)
static struct k_work_delayable snks_work;

static ssize_t snk_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, uint16_t len, uint16_t offset)
{
	BT_DBG("conn %p attr %p buf %p len %u offset %u", conn, attr, buf, len,
	       offset);

	return pac_read(conn, attr, buf, len, offset);
}

static void snk_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	BT_DBG("attr %p value 0x%04x", attr, value);
}

#if defined(CONFIG_BT_PAC_SNK_LOC)
static struct k_work_delayable snks_loc_work;

static ssize_t snk_loc_read(struct bt_conn *conn,
			    const struct bt_gatt_attr *attr, void *buf,
			    uint16_t len, uint16_t offset)
{
	int err;
	enum bt_audio_location location;
	uint32_t location_32;
	uint32_t location_32_le;

	BT_DBG("conn %p attr %p buf %p len %u offset %u", conn, attr, buf, len,
	       offset);

	err = get_pac_loc(NULL, BT_AUDIO_SINK, &location);
	if (err != 0) {
		BT_DBG("get_pac_loc returned %d", err);
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	location_32 = (uint32_t)location;
	if (location_32 > BT_AUDIO_LOCATION_MASK || location_32 == 0) {
		BT_ERR("Invalid location value: 0x%08X", location_32);
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	location_32_le = sys_cpu_to_le32(location_32);

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &location_32_le, sizeof(location_32_le));
}

#if defined(CONFIG_BT_PAC_SNK_LOC_WRITEABLE)
static ssize_t snk_loc_write(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, const void *data,
			     uint16_t len, uint16_t offset, uint8_t flags)
{
	int err;
	enum bt_audio_location location;

	if (offset) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != sizeof(location)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (unicast_server_cb == NULL ||
	    unicast_server_cb->write_location == NULL) {
		BT_WARN("No callback for write_location");
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	location = (enum bt_audio_location)sys_get_le32(data);
	if (location > BT_AUDIO_LOCATION_MASK || location == 0) {
		BT_DBG("Invalid location value: 0x%08X", location);
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	err = unicast_server_cb->write_location(conn, BT_AUDIO_SINK, location);
	if (err != 0) {
		BT_DBG("write_location returned %d", err);
		return BT_GATT_ERR(BT_ATT_ERR_AUTHORIZATION);
	}

	return len;
}
#endif /* CONFIG_BT_PAC_SNK_LOC_WRITEABLE */

static void snk_loc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	BT_DBG("attr %p value 0x%04x", attr, value);
}

#endif /* CONFIG_BT_PAC_SNK_LOC */
#endif /* CONFIG_BT_PAC_SNK */

#if defined(CONFIG_BT_PAC_SRC)
static struct k_work_delayable srcs_work;

static ssize_t src_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, uint16_t len, uint16_t offset)
{
	BT_DBG("conn %p attr %p buf %p len %u offset %u", conn, attr, buf, len,
	       offset);

	return pac_read(conn, attr, buf, len, offset);
}

static void src_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	BT_DBG("attr %p value 0x%04x", attr, value);
}

#if defined(CONFIG_BT_PAC_SRC_LOC)
static struct k_work_delayable srcs_loc_work;

static ssize_t src_loc_read(struct bt_conn *conn,
			    const struct bt_gatt_attr *attr, void *buf,
			    uint16_t len, uint16_t offset)
{
	int err;
	enum bt_audio_location location;
	uint32_t location_32;
	uint32_t location_32_le;

	BT_DBG("conn %p attr %p buf %p len %u offset %u", conn, attr, buf, len,
	       offset);

	err = get_pac_loc(NULL, BT_AUDIO_SOURCE, &location);
	if (err != 0) {
		BT_DBG("get_pac_loc returned %d", err);
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	location_32 = (uint32_t)location;
	if (location_32 > BT_AUDIO_LOCATION_MASK || location_32 == 0) {
		BT_ERR("Invalid location value: 0x%08X", location_32);
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	location_32_le = sys_cpu_to_le32(location_32);

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &location_32_le, sizeof(location_32_le));
}

#if defined(BT_PAC_SRC_LOC_WRITEABLE)
static ssize_t src_loc_write(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, const void *data,
			     uint16_t len, uint16_t offset, uint8_t flags)
{
	int err;
	uint32_t location;

	if (offset) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != sizeof(location)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (unicast_server_cb == NULL ||
	    unicast_server_cb->write_location == NULL) {
		BT_WARN("No callback for write_location");
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	location = (enum bt_audio_location)sys_get_le32(data);
	if (location > BT_AUDIO_LOCATION_MASK || location == 0) {
		BT_DBG("Invalid location value: 0x%08X", location);
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	err = unicast_server_cb->write_location(conn, BT_AUDIO_SOURCE,
						location);
	if (err != 0) {
		BT_DBG("write_location returned %d", err);
		return BT_GATT_ERR(BT_ATT_ERR_AUTHORIZATION);
	}

	return len;
}
#endif /* BT_PAC_SRC_LOC_WRITEABLE */

static void src_loc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	BT_DBG("attr %p value 0x%04x", attr, value);
}
#endif /* CONFIG_BT_PAC_SRC_LOC */
#endif /* CONFIG_BT_PAC_SRC */

#if defined(CONFIG_BT_PAC_SNK) || defined(CONFIG_BT_PAC_SRC)
BT_GATT_SERVICE_DEFINE(pacs_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_PACS),
#if defined(CONFIG_BT_PAC_SNK)
	BT_GATT_CHARACTERISTIC(BT_UUID_PACS_SNK,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ_ENCRYPT,
			       snk_read, NULL, NULL),
	BT_GATT_CCC(snk_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),
#if defined(CONFIG_BT_PAC_SNK_LOC)
	BT_GATT_CHARACTERISTIC(BT_UUID_PACS_SNK_LOC,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE |
			       BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ_ENCRYPT |
			       BT_GATT_PERM_WRITE_ENCRYPT,
			       snk_loc_read,
#if defined(CONFIG_BT_PAC_SNK_LOC_WRITEABLE)
			       snk_loc_write,
#else
			       NULL,
#endif /* BT_PAC_SRC_LOC_WRITEABLE */
			       NULL),
	BT_GATT_CCC(snk_loc_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),
#endif /* CONFIG_BT_PAC_SNK_LOC */
#endif /* CONFIG_BT_PAC_SNK */
#if defined(CONFIG_BT_PAC_SRC)
	BT_GATT_CHARACTERISTIC(BT_UUID_PACS_SRC,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ_ENCRYPT,
			       src_read, NULL, NULL),
	BT_GATT_CCC(src_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),
#if defined(CONFIG_BT_PAC_SRC_LOC)
	BT_GATT_CHARACTERISTIC(BT_UUID_PACS_SRC_LOC,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE |
			       BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ_ENCRYPT |
			       BT_GATT_PERM_WRITE_ENCRYPT,
			       src_loc_read,
#if defined(BT_PAC_SRC_LOC_WRITEABLE)
			       src_loc_write,
#else
			       NULL,
#endif /* BT_PAC_SRC_LOC_WRITEABLE */
			       NULL),
	BT_GATT_CCC(src_loc_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),
#endif /* CONFIG_BT_PAC_SRC_LOC */
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
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT)
);
#endif /* CONFIG_BT_PAC_SNK || CONFIG_BT_PAC_SRC */

static struct k_work_delayable *bt_pacs_get_work(uint8_t type)
{
	switch (type) {
#if defined(CONFIG_BT_PAC_SNK)
	case BT_AUDIO_SINK:
		return &snks_work;
#endif /* CONFIG_BT_PAC_SNK */
#if defined(CONFIG_BT_PAC_SRC)
	case BT_AUDIO_SOURCE:
		return &srcs_work;
#endif /* CONFIG_BT_PAC_SNK */
	}

	return NULL;
}

#if defined(CONFIG_BT_PAC_SNK_LOC) || defined(CONFIG_BT_PAC_SRC_LOC)
static struct k_work_delayable *bt_pacs_get_loc_work(uint8_t type)
{
	switch (type) {
#if defined(CONFIG_BT_PAC_SNK)
	case BT_AUDIO_SINK:
		return &snks_loc_work;
#endif /* CONFIG_BT_PAC_SNK */
#if defined(CONFIG_BT_PAC_SRC)
	case BT_AUDIO_SOURCE:
		return &srcs_loc_work;
#endif /* CONFIG_BT_PAC_SNK */
	}

	return NULL;
}

static void pac_notify_loc(struct k_work *work)
{
#if defined(CONFIG_BT_PAC_SNK) || defined(CONFIG_BT_PAC_SRC)
	uint32_t location;
	struct bt_uuid *uuid;
	enum bt_audio_pac_type type;
	int err;

#if defined(CONFIG_BT_PAC_SNK)
	if (work == &snks_loc_work.work) {
		type = BT_AUDIO_SINK;
		uuid = BT_UUID_PACS_SNK_LOC;
	}
#endif /* CONFIG_BT_PAC_SNK */

#if defined(CONFIG_BT_PAC_SRC)
	if (work == &srcs_loc_work.work) {
		type = BT_AUDIO_SOURCE;
		uuid = BT_UUID_PACS_SRC_LOC;
	}
#endif /* CONFIG_BT_PAC_SRC */

	/* TODO: We can skip this if we are not connected to any devices */
	err = get_pac_loc(NULL, type, &location);
	if (err != 0) {
		BT_DBG("get_pac_loc returned %d, won't notify", err);
		return;
	}

	err = bt_gatt_notify_uuid(NULL, uuid, pacs_svc.attrs,
				  &sys_cpu_to_le32(location),
				  sizeof(location));
	if (err != 0) {
		BT_WARN("PACS notify failed: %d", err);
	}
#endif /* CONFIG_BT_PAC_SNK || CONFIG_BT_PAC_SRC */
}
#endif /* CONFIG_BT_PAC_SNK_LOC || CONFIG_BT_PAC_SRC_LOC */

static void pac_notify(struct k_work *work)
{
#if defined(CONFIG_BT_PAC_SNK) || defined(CONFIG_BT_PAC_SRC)
	struct bt_uuid *uuid;
	uint8_t type;
	int err;

#if defined(CONFIG_BT_PAC_SNK)
	if (work == &snks_work.work) {
		type = BT_AUDIO_SINK;
		uuid = BT_UUID_PACS_SNK;
	}
#endif /* CONFIG_BT_PAC_SNK */

#if defined(CONFIG_BT_PAC_SRC)
	if (work == &srcs_work.work) {
		type = BT_AUDIO_SOURCE;
		uuid = BT_UUID_PACS_SRC;
	}
#endif /* CONFIG_BT_PAC_SRC */

	/* TODO: We can skip this if we are not connected to any devices */
	get_pac_records(NULL, type, &read_buf);

	err = bt_gatt_notify_uuid(NULL, uuid, pacs_svc.attrs, read_buf.data,
				  read_buf.len);
	if (err != 0) {
		BT_WARN("PACS notify failed: %d", err);
	}
#endif /* CONFIG_BT_PAC_SNK || CONFIG_BT_PAC_SRC */
}

void bt_pacs_add_capability(uint8_t type)
{
	struct k_work_delayable *work;

	work = bt_pacs_get_work(type);
	if (!work) {
		return;
	}

	/* Initialize handler if it hasn't been initialized */
	if (!work->work.handler) {
		k_work_init_delayable(work, pac_notify);
	}

	k_work_reschedule(work, PAC_NOTIFY_TIMEOUT);
}

void bt_pacs_remove_capability(uint8_t type)
{
	struct k_work_delayable *work;

	work = bt_pacs_get_work(type);
	if (!work) {
		return;
	}

	k_work_reschedule(work, PAC_NOTIFY_TIMEOUT);
}

#if defined(CONFIG_BT_PAC_SNK_LOC) || defined(CONFIG_BT_PAC_SRC_LOC)
int bt_pacs_location_changed(enum bt_audio_pac_type type)
{
	struct k_work_delayable *work;

	work = bt_pacs_get_loc_work(type);
	if (!work) {
		return -EINVAL;
	}

	/* Initialize handler if it hasn't been initialized */
	if (!work->work.handler) {
		k_work_init_delayable(work, pac_notify_loc);
	}

	k_work_reschedule(work, PAC_NOTIFY_TIMEOUT);

	return 0;
}
#endif /* CONFIG_BT_PAC_SNK_LOC || CONFIG_BT_PAC_SRC_LOC */

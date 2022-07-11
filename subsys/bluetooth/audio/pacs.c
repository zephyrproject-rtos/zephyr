/* @file
 * @brief Bluetooth PACS
 */

/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/check.h>

#include <zephyr/device.h>
#include <zephyr/init.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/capabilities.h>
#include "../host/conn_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_PACS)
#define LOG_MODULE_NAME bt_pacs
#include "common/log.h"

#include "pacs_internal.h"
#include "unicast_server.h"

#define PAC_NOTIFY_TIMEOUT	K_MSEC(10)

NET_BUF_SIMPLE_DEFINE_STATIC(read_buf, CONFIG_BT_L2CAP_TX_MTU);

static const struct bt_audio_pacs_cb *pacs_cb;

static struct bt_pacs_context available_context = {
	/* TODO: This should reflect the ongoing channel contexts */
	.snk = BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED,
	.src = BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED,
};

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

static void get_pac_records(struct bt_conn *conn, enum bt_audio_dir dir,
			    struct net_buf_simple *buf)
{
	struct bt_pacs_read_rsp *rsp;

	/* Reset if buffer before using */
	net_buf_simple_reset(buf);

	rsp = net_buf_simple_add(buf, sizeof(*rsp));
	rsp->num_pac = 0;

	if (pacs_cb == NULL ||
	    pacs_cb->publish_capability == NULL) {
		return;
	}

	while (true) {
		struct bt_pac_meta *meta;
		struct bt_codec codec;
		struct bt_pac *pac;
		int err;

		err = pacs_cb->publish_capability(conn, dir, rsp->num_pac,
						  &codec);
		if (err != 0) {
			break;
		}

		pac = net_buf_simple_add(buf, sizeof(*pac));

		pac->codec.id = codec.id;
		pac->codec.cid = sys_cpu_to_le16(codec.cid);
		pac->codec.vid = sys_cpu_to_le16(codec.vid);
		pac->cc_len = buf->len;

		pac_data_add(buf, codec.data_count, codec.data);

		/* Buffer size shall never be below PAC len since we are just
		 * append data.
		 */
		__ASSERT_NO_MSG(buf->len >= pac->cc_len);

		pac->cc_len = buf->len - pac->cc_len;

		meta = net_buf_simple_add(buf, sizeof(*meta));
		meta->len = buf->len;
		pac_data_add(buf, codec.meta_count, codec.meta);
		meta->len = buf->len - meta->len;

		BT_DBG("pac #%u: codec capability len %u metadata len %u",
		       rsp->num_pac, pac->cc_len, meta->len);

		rsp->num_pac++;
	}
}

static ssize_t pac_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, uint16_t len, uint16_t offset)
{
	enum bt_audio_dir dir;

	if (!bt_uuid_cmp(attr->uuid, BT_UUID_PACS_SNK)) {
		dir = BT_AUDIO_DIR_SINK;
	} else {
		dir = BT_AUDIO_DIR_SOURCE;
	}

	get_pac_records(conn, dir, &read_buf);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, read_buf.data,
				 read_buf.len);
}

static void available_context_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	BT_DBG("attr %p value 0x%04x", attr, value);
}

static int available_contexts_get(struct bt_conn *conn, struct bt_pacs_context *context)
{
	enum bt_audio_context context_snk, context_src;
	int err;

	if (pacs_cb == NULL ||
	    pacs_cb->get_available_contexts == NULL) {
		BT_WARN("No callback for get_available_contexts");
		return -ENODATA;
	}

	if (IS_ENABLED(CONFIG_BT_PAC_SNK)) {
		err = pacs_cb->get_available_contexts(conn, BT_AUDIO_DIR_SINK,
						      &context_snk);
		if (err) {
			return err;
		}
	} else {
		/* Server is not available to receive audio for any Context */
		context_snk = BT_AUDIO_CONTEXT_TYPE_PROHIBITED;
	}

	if (IS_ENABLED(CONFIG_BT_PAC_SRC)) {
		err = pacs_cb->get_available_contexts(conn, BT_AUDIO_DIR_SOURCE,
						      &context_src);
		if (err) {
			return err;
		}
	} else {
		/* Server is not available to send audio for any Context */
		context_src = BT_AUDIO_CONTEXT_TYPE_PROHIBITED;
	}

	context->snk = sys_cpu_to_le16((uint16_t)context_snk);
	context->src = sys_cpu_to_le16((uint16_t)context_src);

	return 0;
}

static ssize_t available_contexts_read(struct bt_conn *conn,
				       const struct bt_gatt_attr *attr, void *buf,
				       uint16_t len, uint16_t offset)
{
	struct bt_pacs_context context;
	int err;

	BT_DBG("conn %p attr %p buf %p len %u offset %u", conn, attr, buf, len,
	       offset);

	err = available_contexts_get(NULL, &context);
	if (err) {
		BT_DBG("get_available_contexts returned %d", err);
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

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
	struct bt_pacs_context context;

#if defined(CONFIG_BT_PAC_SNK)
	context.snk = sys_cpu_to_le16(CONFIG_BT_PACS_SNK_CONTEXT);
#else
	context.snk = BT_AUDIO_CONTEXT_TYPE_PROHIBITED;
#endif

#if defined(CONFIG_BT_PAC_SRC)
	context.src = sys_cpu_to_le16(CONFIG_BT_PACS_SRC_CONTEXT);
#else
	context.src = BT_AUDIO_CONTEXT_TYPE_PROHIBITED;
#endif

	BT_DBG("conn %p attr %p buf %p len %u offset %u", conn, attr, buf, len,
	       offset);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &context,
				 sizeof(context));
}

#if defined(CONFIG_BT_PAC_SNK_LOC) || defined(CONFIG_BT_PAC_SRC_LOC)
static int get_pac_loc(struct bt_conn *conn, enum bt_audio_dir dir,
		       enum bt_audio_location *location)
{
	int err;

	if (pacs_cb == NULL ||
	    pacs_cb->publish_location == NULL) {
		BT_WARN("No callback for publish_location");
		return -ENODATA;
	}

	err = pacs_cb->publish_location(conn, dir, location);
	if (err != 0 || *location == 0) {
		BT_DBG("err (%d) or invalid location value (%u)",
		       err, *location);
		return -ENODATA;
	}

	return 0;
}
#endif /* CONFIG_BT_PAC_SNK_LOC || CONFIG_BT_PAC_SRC_LOC */

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

	err = get_pac_loc(NULL, BT_AUDIO_DIR_SINK, &location);
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

	if (pacs_cb == NULL ||
	    pacs_cb->write_location == NULL) {
		BT_WARN("No callback for write_location");
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	location = (enum bt_audio_location)sys_get_le32(data);
	if (location > BT_AUDIO_LOCATION_MASK || location == 0) {
		BT_DBG("Invalid location value: 0x%08X", location);
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	err = pacs_cb->write_location(conn, BT_AUDIO_DIR_SINK, location);
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

	err = get_pac_loc(NULL, BT_AUDIO_DIR_SOURCE, &location);
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

	if (pacs_cb == NULL ||
	    pacs_cb->write_location == NULL) {
		BT_WARN("No callback for write_location");
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	location = (enum bt_audio_location)sys_get_le32(data);
	if (location > BT_AUDIO_LOCATION_MASK || location == 0) {
		BT_DBG("Invalid location value: 0x%08X", location);
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	err = pacs_cb->write_location(conn, BT_AUDIO_DIR_SOURCE, location);
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

BT_GATT_SERVICE_DEFINE(pacs_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_PACS),
#if defined(CONFIG_BT_PAC_SNK)
	BT_GATT_CHARACTERISTIC(BT_UUID_PACS_SNK,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ_ENCRYPT,
			       snk_read, NULL, NULL),
	BT_GATT_CCC(snk_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),
#if defined(CONFIG_BT_PAC_SNK_LOC_WRITEABLE)
	BT_GATT_CHARACTERISTIC(BT_UUID_PACS_SNK_LOC,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE |
			       BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ_ENCRYPT |
			       BT_GATT_PERM_WRITE_ENCRYPT,
			       snk_loc_read, snk_loc_write, NULL),
#elif defined(CONFIG_BT_PAC_SNK_LOC)
	BT_GATT_CHARACTERISTIC(BT_UUID_PACS_SNK_LOC,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ_ENCRYPT,
			       snk_loc_read, NULL, NULL),
#endif /* CONFIG_BT_PAC_SNK_LOC_WRITEABLE */
	BT_GATT_CCC(snk_loc_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),
#endif /* CONFIG_BT_PAC_SNK */
#if defined(CONFIG_BT_PAC_SRC)
	BT_GATT_CHARACTERISTIC(BT_UUID_PACS_SRC,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ_ENCRYPT,
			       src_read, NULL, NULL),
	BT_GATT_CCC(src_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),
#if defined(CONFIG_BT_PAC_SRC_LOC_WRITEABLE)
	BT_GATT_CHARACTERISTIC(BT_UUID_PACS_SRC_LOC,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE |
			       BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ_ENCRYPT |
			       BT_GATT_PERM_WRITE_ENCRYPT,
			       src_loc_read, src_loc_write, NULL),
#elif defined(CONFIG_BT_PAC_SRC_LOC)
	BT_GATT_CHARACTERISTIC(BT_UUID_PACS_SRC_LOC,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ_ENCRYPT,
			       src_loc_read, NULL, NULL),
#endif /* CONFIG_BT_PAC_SRC_LOC_WRITEABLE */
	BT_GATT_CCC(src_loc_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),
#endif /* CONFIG_BT_PAC_SRC */
	BT_GATT_CHARACTERISTIC(BT_UUID_PACS_AVAILABLE_CONTEXT,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ_ENCRYPT,
			       available_contexts_read, NULL, NULL),
	BT_GATT_CCC(available_context_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),
	BT_GATT_CHARACTERISTIC(BT_UUID_PACS_SUPPORTED_CONTEXT,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ_ENCRYPT,
			       supported_context_read, NULL, NULL),
	BT_GATT_CCC(supported_context_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT)
);

static struct k_work_delayable *bt_pacs_get_work(enum bt_audio_dir dir)
{
	switch (dir) {
#if defined(CONFIG_BT_PAC_SNK)
	case BT_AUDIO_DIR_SINK:
		return &snks_work;
#endif /* CONFIG_BT_PAC_SNK */
#if defined(CONFIG_BT_PAC_SRC)
	case BT_AUDIO_DIR_SOURCE:
		return &srcs_work;
#endif /* CONFIG_BT_PAC_SRC */
	default:
		return NULL;
	}
}

#if defined(CONFIG_BT_PAC_SNK_LOC) || defined(CONFIG_BT_PAC_SRC_LOC)
static struct k_work_delayable *bt_pacs_get_loc_work(enum bt_audio_dir dir)
{
	switch (dir) {
#if defined(CONFIG_BT_PAC_SNK)
	case BT_AUDIO_DIR_SINK:
		return &snks_loc_work;
#endif /* CONFIG_BT_PAC_SNK */
#if defined(CONFIG_BT_PAC_SRC)
	case BT_AUDIO_DIR_SOURCE:
		return &srcs_loc_work;
#endif /* CONFIG_BT_PAC_SRC */
	default:
		return NULL;
	}
}

static void pac_notify_loc(struct k_work *work)
{
	uint32_t location;
	enum bt_audio_dir dir;
	int err;

#if defined(CONFIG_BT_PAC_SNK)
	if (work == &snks_loc_work.work) {
		dir = BT_AUDIO_DIR_SINK;
	}
#endif /* CONFIG_BT_PAC_SNK */

#if defined(CONFIG_BT_PAC_SRC)
	if (work == &srcs_loc_work.work) {
		dir = BT_AUDIO_DIR_SOURCE;
	}
#endif /* CONFIG_BT_PAC_SRC */

	/* TODO: We can skip this if we are not connected to any devices */
	err = get_pac_loc(NULL, dir, &location);
	if (err != 0) {
		BT_DBG("get_pac_loc returned %d, won't notify", err);
		return;
	}

	if (dir == BT_AUDIO_DIR_SINK) {
		err = bt_gatt_notify_uuid(NULL, BT_UUID_PACS_SNK_LOC,
					  pacs_svc.attrs,
					  &sys_cpu_to_le32(location),
					  sizeof(location));
	} else {
		err = bt_gatt_notify_uuid(NULL, BT_UUID_PACS_SRC_LOC,
					  pacs_svc.attrs,
					  &sys_cpu_to_le32(location),
					  sizeof(location));
	}

	if (err != 0 && err != -ENOTCONN) {
		BT_WARN("PACS notify_loc failed: %d", err);
	}
}
#endif /* CONFIG_BT_PAC_SNK_LOC || CONFIG_BT_PAC_SRC_LOC */

static void pac_notify(struct k_work *work)
{
	int err = 0;

#if defined(CONFIG_BT_PAC_SNK)
	if (work == &snks_work.work) {
		err = bt_gatt_notify_uuid(NULL, BT_UUID_PACS_SNK,
					  pacs_svc.attrs, read_buf.data,
					  read_buf.len);
	}
#endif /* CONFIG_BT_PAC_SNK */

#if defined(CONFIG_BT_PAC_SRC)
	if (work == &srcs_work.work) {
		err = bt_gatt_notify_uuid(NULL, BT_UUID_PACS_SRC,
					  pacs_svc.attrs, read_buf.data,
					  read_buf.len);
	}
#endif /* CONFIG_BT_PAC_SRC */

	if (err != 0 && err != -ENOTCONN) {
		BT_WARN("PACS notify failed: %d", err);
	}
}

void bt_pacs_add_capability(enum bt_audio_dir dir)
{
	struct k_work_delayable *work;

	work = bt_pacs_get_work(dir);
	if (!work) {
		return;
	}

	/* Initialize handler if it hasn't been initialized */
	if (!work->work.handler) {
		k_work_init_delayable(work, pac_notify);
	}

	k_work_reschedule(work, PAC_NOTIFY_TIMEOUT);
}

void bt_pacs_remove_capability(enum bt_audio_dir dir)
{
	struct k_work_delayable *work;

	work = bt_pacs_get_work(dir);
	if (!work) {
		return;
	}

	k_work_reschedule(work, PAC_NOTIFY_TIMEOUT);
}

#if defined(CONFIG_BT_PAC_SNK_LOC) || defined(CONFIG_BT_PAC_SRC_LOC)
/******* PUBLIC API *******/
int bt_audio_pacs_location_changed(enum bt_audio_dir dir)
{
	struct k_work_delayable *work;

	work = bt_pacs_get_loc_work(dir);
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

static void available_contexts_notify(struct k_work *work)
{
	struct bt_pacs_context context;
	int err;

	err = available_contexts_get(NULL, &context);
	if (err) {
		BT_DBG("get_available_contexts returned %d, won't notify", err);
		return;
	}

	err = bt_gatt_notify_uuid(NULL, BT_UUID_PACS_AVAILABLE_CONTEXT, pacs_svc.attrs,
				  &context, sizeof(context));
	if (err != 0 && err != -ENOTCONN) {
		BT_WARN("Available Audio Contexts notify failed: %d", err);
	}
}

static K_WORK_DELAYABLE_DEFINE(available_contexts_work, available_contexts_notify);

int bt_pacs_available_contexts_changed(void)
{
	int err;

	err = k_work_reschedule(&available_contexts_work, PAC_NOTIFY_TIMEOUT);
	if (err < 0) {
		return err;
	}

	return 0;
}

int bt_audio_pacs_register_cb(const struct bt_audio_pacs_cb *cb)
{
	CHECKIF(cb == NULL) {
		return -EINVAL;
	}

	if (pacs_cb != NULL) {
		return -EALREADY;
	}

	pacs_cb = cb;

	return 0;
}

bool bt_pacs_context_available(enum bt_audio_dir dir, uint16_t context)
{
	if (IS_ENABLED(CONFIG_BT_PAC_SRC) && dir == BT_AUDIO_DIR_SOURCE) {
		return (context & available_context.src) == context;
	} else if (IS_ENABLED(CONFIG_BT_PAC_SNK) && dir == BT_AUDIO_DIR_SINK) {
		return (context & available_context.snk) == context;
	}

	return false;
}

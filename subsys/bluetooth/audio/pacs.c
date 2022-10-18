/* @file
 * @brief Bluetooth PACS
 */

/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
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

#include "audio_internal.h"
#include "pacs_internal.h"
#include "unicast_server.h"

#define PAC_NOTIFY_TIMEOUT	K_MSEC(10)

#define PACS_LOCATION(_name, _work_handler) \
	struct pacs_location _name = { \
		.work = Z_WORK_DELAYABLE_INITIALIZER(_work_handler), \
	};

struct pacs_location {
	struct k_work_delayable work;
	uint32_t location;
};

static sys_slist_t snks;
static sys_slist_t srcs;

#if defined(CONFIG_BT_PAC_SNK)
static uint16_t snk_available_contexts;
static const uint16_t snk_supported_contexts = CONFIG_BT_PACS_SNK_CONTEXT;
#else
static const uint16_t snk_available_contexts = BT_AUDIO_CONTEXT_TYPE_PROHIBITED;
static const uint16_t snk_supported_contexts = BT_AUDIO_CONTEXT_TYPE_PROHIBITED;
#endif /* CONFIG_BT_PAC_SNK */

#if defined(CONFIG_BT_PAC_SRC)
static uint16_t src_available_contexts;
static const uint16_t src_supported_contexts = CONFIG_BT_PACS_SRC_CONTEXT;
#else
static const uint16_t src_available_contexts = BT_AUDIO_CONTEXT_TYPE_PROHIBITED;
static const uint16_t src_supported_contexts = BT_AUDIO_CONTEXT_TYPE_PROHIBITED;
#endif /* CONFIG_BT_PAC_SRC */

NET_BUF_SIMPLE_DEFINE_STATIC(read_buf, CONFIG_BT_L2CAP_TX_MTU);

static void pac_data_add(struct net_buf_simple *buf, uint8_t num,
			 struct bt_codec_data *data)
{
	for (uint8_t i = 0; i < num; i++) {
		struct bt_pac_codec_capability *cc;
		struct bt_data *d = &data[i].data;

		cc = net_buf_simple_add(buf, sizeof(*cc));
		cc->len = d->data_len + sizeof(cc->type);
		cc->type = d->type;
		net_buf_simple_add_mem(buf, d->data, d->data_len);

		BT_DBG("  %u: type %u: %s",
		       i, d->type, bt_hex(d->data, d->data_len));
	}
}

struct pac_records_build_data {
	struct net_buf_simple *buf;
	uint8_t num_pac;
};

static bool build_pac_records(const struct bt_audio_capability *capability, void *user_data)
{
	struct pac_records_build_data *data = user_data;
	struct net_buf_simple *buf = data->buf;
	struct bt_pac_meta *meta;
	struct bt_codec *codec;
	struct bt_pac *pac;

	codec = capability->codec;

	pac = net_buf_simple_add(buf, sizeof(*pac));
	pac->codec.id = codec->id;
	pac->codec.cid = sys_cpu_to_le16(codec->cid);
	pac->codec.vid = sys_cpu_to_le16(codec->vid);
	pac->cc_len = buf->len;

	BT_DBG("Parsing codec config data");
	pac_data_add(buf, codec->data_count, codec->data);

	/* Buffer size shall never be below PAC len since we are just
	 * append data.
	 */
	__ASSERT_NO_MSG(buf->len >= pac->cc_len);

	pac->cc_len = buf->len - pac->cc_len;

	meta = net_buf_simple_add(buf, sizeof(*meta));
	meta->len = buf->len;
	BT_DBG("Parsing metadata");
	pac_data_add(buf, codec->meta_count, codec->meta);
	meta->len = buf->len - meta->len;

	BT_DBG("pac #%u: codec capability len %u metadata len %u",
		data->num_pac, pac->cc_len, meta->len);

	data->num_pac++;

	return true;
}

static void get_pac_records(struct bt_conn *conn, enum bt_audio_dir dir,
			    struct net_buf_simple *buf)
{
	struct pac_records_build_data data = {
		.buf = buf,
	};
	struct bt_pacs_read_rsp *rsp;

	/* Reset if buffer before using */
	net_buf_simple_reset(buf);

	rsp = net_buf_simple_add(buf, sizeof(*rsp));

	bt_audio_foreach_capability(dir, build_pac_records, &data);

	rsp->num_pac = data.num_pac;
}

static void available_context_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	BT_DBG("attr %p value 0x%04x", attr, value);
}


static ssize_t available_contexts_read(struct bt_conn *conn,
				       const struct bt_gatt_attr *attr, void *buf,
				       uint16_t len, uint16_t offset)
{
	struct bt_pacs_context context = {
		.snk = sys_cpu_to_le16(snk_available_contexts),
		.src = sys_cpu_to_le16(src_available_contexts),
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
		.snk = sys_cpu_to_le16(snk_supported_contexts),
		.src = sys_cpu_to_le16(src_supported_contexts),
	};

	BT_DBG("conn %p attr %p buf %p len %u offset %u", conn, attr, buf, len,
	       offset);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &context,
				 sizeof(context));
}

#if defined(CONFIG_BT_PAC_SNK)
static void pac_notify(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(snks_work, pac_notify);

static ssize_t snk_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, uint16_t len, uint16_t offset)
{
	BT_DBG("conn %p attr %p buf %p len %u offset %u", conn, attr, buf, len,
	       offset);

	get_pac_records(conn, BT_AUDIO_DIR_SINK, &read_buf);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, read_buf.data,
				 read_buf.len);
}

static void snk_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	BT_DBG("attr %p value 0x%04x", attr, value);
}
#endif /* CONFIG_BT_PAC_SNK */

#if defined(CONFIG_BT_PAC_SNK_LOC)
static void pac_notify_snk_loc(struct k_work *work);
static PACS_LOCATION(snk_location, pac_notify_snk_loc);

static ssize_t snk_loc_read(struct bt_conn *conn,
			    const struct bt_gatt_attr *attr, void *buf,
			    uint16_t len, uint16_t offset)
{
	uint32_t location = sys_cpu_to_le32(snk_location.location);

	BT_DBG("conn %p attr %p buf %p len %u offset %u", conn, attr, buf, len,
	       offset);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &location, sizeof(location));
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
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	location = (enum bt_audio_location)sys_get_le32(data);
	if (location > BT_AUDIO_LOCATION_MASK || location == 0) {
		BT_DBG("Invalid location value: 0x%08X", location);
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	err = bt_audio_capability_set_location(BT_AUDIO_DIR_SINK, location);
	if (err != 0) {
		BT_DBG("write_location returned %d", err);
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	return len;
}
#endif /* CONFIG_BT_PAC_SNK_LOC_WRITEABLE */

static void snk_loc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	BT_DBG("attr %p value 0x%04x", attr, value);
}

static int set_snk_location(enum bt_audio_location audio_location)
{
	if (audio_location == snk_location.location) {
		return 0;
	}

	snk_location.location = audio_location;

	k_work_reschedule(&snk_location.work, PAC_NOTIFY_TIMEOUT);

	return 0;
}
#else
static int update_snk_location(enum bt_audio_location location)
{
	return -ENOTSUP;
}
#endif /* CONFIG_BT_PAC_SNK_LOC */

#if defined(CONFIG_BT_PAC_SRC)
static K_WORK_DELAYABLE_DEFINE(srcs_work, pac_notify);

static ssize_t src_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, uint16_t len, uint16_t offset)
{
	BT_DBG("conn %p attr %p buf %p len %u offset %u", conn, attr, buf, len,
	       offset);

	get_pac_records(conn, BT_AUDIO_DIR_SOURCE, &read_buf);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, read_buf.data,
				 read_buf.len);
}

static void src_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	BT_DBG("attr %p value 0x%04x", attr, value);
}
#endif /* CONFIG_BT_PAC_SRC */

#if defined(CONFIG_BT_PAC_SRC_LOC)
static void pac_notify_src_loc(struct k_work *work);
static PACS_LOCATION(src_location, pac_notify_src_loc);

static ssize_t src_loc_read(struct bt_conn *conn,
			    const struct bt_gatt_attr *attr, void *buf,
			    uint16_t len, uint16_t offset)
{
	uint32_t location = sys_cpu_to_le32(src_location.location);

	BT_DBG("conn %p attr %p buf %p len %u offset %u", conn, attr, buf, len,
	       offset);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &location, sizeof(location));
}

#if defined(CONFIG_BT_PAC_SRC_LOC_WRITEABLE)
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
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	location = (enum bt_audio_location)sys_get_le32(data);
	if (location > BT_AUDIO_LOCATION_MASK || location == 0) {
		BT_DBG("Invalid location value: 0x%08X", location);
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	err = bt_audio_capability_set_location(BT_AUDIO_DIR_SOURCE, location);
	if (err != 0) {
		BT_DBG("write_location returned %d", err);
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	return len;
}
#endif /* CONFIG_BT_PAC_SRC_LOC_WRITEABLE */

static void src_loc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	BT_DBG("attr %p value 0x%04x", attr, value);
}

static int set_src_location(enum bt_audio_location audio_location)
{
	if (audio_location == src_location.location) {
		return 0;
	}

	src_location.location = audio_location;

	k_work_reschedule(&src_location.work, PAC_NOTIFY_TIMEOUT);

	return 0;
}
#else
static int update_src_location(enum bt_audio_location location)
{
	return -ENOTSUP;
}
#endif /* CONFIG_BT_PAC_SRC_LOC */

BT_GATT_SERVICE_DEFINE(pacs_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_PACS),
#if defined(CONFIG_BT_PAC_SNK)
	BT_AUDIO_CHRC(BT_UUID_PACS_SNK,
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
		      BT_GATT_PERM_READ_ENCRYPT,
		      snk_read, NULL, NULL),
	BT_AUDIO_CCC(snk_cfg_changed),
#if defined(CONFIG_BT_PAC_SNK_LOC_WRITEABLE)
	BT_AUDIO_CHRC(BT_UUID_PACS_SNK_LOC,
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY,
		      BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT,
		      snk_loc_read, snk_loc_write, NULL),
#elif defined(CONFIG_BT_PAC_SNK_LOC)
	BT_AUDIO_CHRC(BT_UUID_PACS_SNK_LOC,
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
		      BT_GATT_PERM_READ_ENCRYPT,
		      snk_loc_read, NULL, NULL),
#endif /* CONFIG_BT_PAC_SNK_LOC_WRITEABLE */
	BT_AUDIO_CCC(snk_loc_cfg_changed),
#endif /* CONFIG_BT_PAC_SNK */
#if defined(CONFIG_BT_PAC_SRC)
	BT_AUDIO_CHRC(BT_UUID_PACS_SRC,
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
		      BT_GATT_PERM_READ_ENCRYPT,
		      src_read, NULL, NULL),
	BT_AUDIO_CCC(src_cfg_changed),
#if defined(CONFIG_BT_PAC_SRC_LOC_WRITEABLE)
	BT_AUDIO_CHRC(BT_UUID_PACS_SRC_LOC,
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY,
		      BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT,
		      src_loc_read, src_loc_write, NULL),
#elif defined(CONFIG_BT_PAC_SRC_LOC)
	BT_AUDIO_CHRC(BT_UUID_PACS_SRC_LOC,
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
		      BT_GATT_PERM_READ_ENCRYPT,
		      src_loc_read, NULL, NULL),
#endif /* CONFIG_BT_PAC_SRC_LOC_WRITEABLE */
	BT_AUDIO_CCC(src_loc_cfg_changed),
#endif /* CONFIG_BT_PAC_SRC */
	BT_AUDIO_CHRC(BT_UUID_PACS_AVAILABLE_CONTEXT,
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
		      BT_GATT_PERM_READ_ENCRYPT,
		      available_contexts_read, NULL, NULL),
	BT_AUDIO_CCC(available_context_cfg_changed),
	BT_AUDIO_CHRC(BT_UUID_PACS_SUPPORTED_CONTEXT,
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
		      BT_GATT_PERM_READ_ENCRYPT,
		      supported_context_read, NULL, NULL),
	BT_AUDIO_CCC(supported_context_cfg_changed)
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

#if defined(CONFIG_BT_PAC_SNK_LOC)
static void pac_notify_snk_loc(struct k_work *work)
{
	struct pacs_location *location = CONTAINER_OF(work, struct pacs_location, work);
	uint32_t location_le = sys_cpu_to_le32(location->location);
	int err;

	err = bt_gatt_notify_uuid(NULL, BT_UUID_PACS_SNK_LOC, pacs_svc.attrs, &location_le,
				  sizeof(location_le));
	if (err != 0 && err != -ENOTCONN) {
		BT_WARN("PACS notify_loc failed: %d", err);
	}
}
#endif /* CONFIG_BT_PAC_SNK_LOC */

#if defined(CONFIG_BT_PAC_SRC_LOC)
static void pac_notify_src_loc(struct k_work *work)
{
	struct pacs_location *location = CONTAINER_OF(work, struct pacs_location, work);
	uint32_t location_le = sys_cpu_to_le32(location->location);
	int err;

	err = bt_gatt_notify_uuid(NULL, BT_UUID_PACS_SRC_LOC, pacs_svc.attrs, &location_le,
				  sizeof(location_le));
	if (err != 0 && err != -ENOTCONN) {
		BT_WARN("PACS notify_loc failed: %d", err);
	}
}
#endif /* CONFIG_BT_PAC_SRC_LOC */

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

static void bt_pacs_capabilities_changed(enum bt_audio_dir dir)
{
	struct k_work_delayable *work;

	work = bt_pacs_get_work(dir);
	if (!work) {
		return;
	}

	k_work_reschedule(work, PAC_NOTIFY_TIMEOUT);
}

static void available_contexts_notify(struct k_work *work)
{
	struct bt_pacs_context context = {
		.snk = sys_cpu_to_le16(snk_available_contexts),
		.src = sys_cpu_to_le16(src_available_contexts),
	};
	int err;

	err = bt_gatt_notify_uuid(NULL, BT_UUID_PACS_AVAILABLE_CONTEXT, pacs_svc.attrs,
				  &context, sizeof(context));
	if (err != 0 && err != -ENOTCONN) {
		BT_WARN("Available Audio Contexts notify failed: %d", err);
	}
}

static K_WORK_DELAYABLE_DEFINE(available_contexts_work, available_contexts_notify);

bool bt_pacs_context_available(enum bt_audio_dir dir, uint16_t context)
{
	struct bt_pacs_context available_context = {
		.snk = snk_available_contexts,
		.src = src_available_contexts,
	};

	if (dir == BT_AUDIO_DIR_SOURCE) {
		return (context & available_context.src) == context;
	}

	if (dir == BT_AUDIO_DIR_SINK) {
		return (context & available_context.snk) == context;
	}

	return false;
}

static sys_slist_t *bt_audio_capability_get(enum bt_audio_dir dir)
{
	switch (dir) {
	case BT_AUDIO_DIR_SINK:
		return &snks;
	case BT_AUDIO_DIR_SOURCE:
		return &srcs;
	}

	return NULL;
}

void bt_audio_foreach_capability(enum bt_audio_dir dir, bt_audio_foreach_capability_func_t func,
				 void *user_data)
{
	struct bt_audio_capability *cap;
	sys_slist_t *lst;

	CHECKIF(func == NULL) {
		BT_ERR("func is NULL");
		return;
	}

	lst = bt_audio_capability_get(dir);
	if (!lst) {
		return;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(lst, cap, _node) {
		if (!func(cap, user_data)) {
			break;
		}
	}
}

/* Register Audio Capability */
int bt_audio_capability_register(enum bt_audio_dir dir, struct bt_audio_capability *cap)
{
	sys_slist_t *lst;

	if (!cap || !cap->codec) {
		return -EINVAL;
	}

	lst = bt_audio_capability_get(dir);
	if (!lst) {
		return -EINVAL;
	}

	BT_DBG("cap %p dir 0x%02x codec 0x%02x codec cid 0x%04x "
	       "codec vid 0x%04x", cap, dir, cap->codec->id,
	       cap->codec->cid, cap->codec->vid);

	sys_slist_append(lst, &cap->_node);

	bt_pacs_capabilities_changed(dir);

	return 0;
}

/* Unregister Audio Capability */
int bt_audio_capability_unregister(enum bt_audio_dir dir, struct bt_audio_capability *cap)
{
	sys_slist_t *lst;

	if (!cap) {
		return -EINVAL;
	}

	lst = bt_audio_capability_get(dir);
	if (!lst) {
		return -EINVAL;
	}

	BT_DBG("cap %p dir 0x%02x", cap, dir);

	if (!sys_slist_find_and_remove(lst, &cap->_node)) {
		return -ENOENT;
	}

	bt_pacs_capabilities_changed(dir);

	return 0;
}

int bt_audio_capability_set_location(enum bt_audio_dir dir, enum bt_audio_location location)
{
	switch (dir) {
	case BT_AUDIO_DIR_SINK:
		return set_snk_location(location);
	case BT_AUDIO_DIR_SOURCE:
		return set_src_location(location);
	}

	return -EINVAL;
}

static int set_available_contexts(uint16_t contexts, uint16_t *available, const uint16_t supported)
{
	int err;

	if (contexts & ~supported) {
		return -ENOTSUP;
	}

	if (contexts == *available) {
		return 0;
	}

	*available = contexts;

	err = k_work_reschedule(&available_contexts_work, PAC_NOTIFY_TIMEOUT);
	if (err < 0) {
		return err;
	}

	return 0;
}

int bt_audio_capability_set_available_contexts(enum bt_audio_dir dir,
					       enum bt_audio_context contexts)
{
	switch (dir) {
	case BT_AUDIO_DIR_SINK:
		if (IS_ENABLED(CONFIG_BT_PAC_SNK)) {
			return set_available_contexts(contexts, &snk_available_contexts,
						      snk_supported_contexts);
		} else {
			return -ENOTSUP;
		}
	case BT_AUDIO_DIR_SOURCE:
		if (IS_ENABLED(CONFIG_BT_PAC_SNK)) {
			return set_available_contexts(contexts, &src_available_contexts,
						      src_supported_contexts);
		} else {
			return -ENOTSUP;
		}
	}

	return -EINVAL;
}

enum bt_audio_context bt_audio_capability_get_available_contexts(enum bt_audio_dir dir)
{
	switch (dir) {
	case BT_AUDIO_DIR_SINK:
		return snk_available_contexts;
	case BT_AUDIO_DIR_SOURCE:
		return src_available_contexts;
	}

	return BT_AUDIO_CONTEXT_TYPE_PROHIBITED;
}

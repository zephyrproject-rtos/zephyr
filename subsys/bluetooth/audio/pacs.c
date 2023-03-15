/* @file
 * @brief Bluetooth PACS
 */

/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
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
#include <zephyr/bluetooth/audio/pacs.h>
#include <zephyr/sys/slist.h>
#include "../host/conn_internal.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_pacs, CONFIG_BT_PACS_LOG_LEVEL);

#include "common/bt_str.h"

#include "audio_internal.h"
#include "pacs_internal.h"
#include "bap_unicast_server.h"

#define PAC_NOTIFY_TIMEOUT	K_MSEC(10)
#define READ_BUF_SEM_TIMEOUT    K_MSEC(50)

#if defined(CONFIG_BT_PAC_SRC)
static uint32_t pacs_src_location;
static sys_slist_t src_pacs_list = SYS_SLIST_STATIC_INIT(&src_pacs_list);
#endif /* CONFIG_BT_PAC_SRC */

#if defined(CONFIG_BT_PAC_SNK)
static uint32_t pacs_snk_location;
static sys_slist_t snk_pacs_list = SYS_SLIST_STATIC_INIT(&snk_pacs_list);
#endif /* CONFIG_BT_PAC_SNK */

#if defined(CONFIG_BT_PAC_SNK)
static uint16_t snk_available_contexts;
static uint16_t snk_supported_contexts = BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED;
#else
static uint16_t snk_available_contexts = BT_AUDIO_CONTEXT_TYPE_PROHIBITED;
static uint16_t snk_supported_contexts = BT_AUDIO_CONTEXT_TYPE_PROHIBITED;
#endif /* CONFIG_BT_PAC_SNK */

#if defined(CONFIG_BT_PAC_SRC)
static uint16_t src_available_contexts;
static uint16_t src_supported_contexts = BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED;
#else
static uint16_t src_available_contexts = BT_AUDIO_CONTEXT_TYPE_PROHIBITED;
static uint16_t src_supported_contexts = BT_AUDIO_CONTEXT_TYPE_PROHIBITED;
#endif /* CONFIG_BT_PAC_SRC */

static K_SEM_DEFINE(read_buf_sem, 1, 1);
NET_BUF_SIMPLE_DEFINE_STATIC(read_buf, BT_ATT_MAX_ATTRIBUTE_LEN);

static ssize_t pac_data_add(struct net_buf_simple *buf, size_t count,
			    struct bt_audio_codec_data *data)
{
	size_t len = 0;

	for (size_t i = 0; i < count; i++) {
		struct bt_pac_ltv *ltv;
		struct bt_data *d = &data[i].data;
		const size_t ltv_len = sizeof(*ltv) + d->data_len;

		if (net_buf_simple_tailroom(buf) < ltv_len) {
			return -ENOMEM;
		}

		ltv = net_buf_simple_add(buf, sizeof(*ltv));
		ltv->len = d->data_len + sizeof(ltv->type);
		ltv->type = d->type;
		net_buf_simple_add_mem(buf, d->data, d->data_len);

		len += ltv_len;
	}

	return len;
}

struct pac_records_build_data {
	struct bt_pacs_read_rsp *rsp;
	struct net_buf_simple *buf;
};

static bool build_pac_records(const struct bt_pacs_cap *cap, void *user_data)
{
	struct pac_records_build_data *data = user_data;
	struct bt_audio_codec_cap *codec_cap = cap->codec_cap;
	struct net_buf_simple *buf = data->buf;
	struct net_buf_simple_state state;
	struct bt_pac_ltv_data *cc, *meta;
	struct bt_pac_codec *pac_codec;
	ssize_t len;

	net_buf_simple_save(buf, &state);

	if (net_buf_simple_tailroom(buf) < sizeof(*pac_codec)) {
		goto fail;
	}

	pac_codec = net_buf_simple_add(buf, sizeof(*pac_codec));
	pac_codec->id = codec_cap->id;
	pac_codec->cid = sys_cpu_to_le16(codec_cap->cid);
	pac_codec->vid = sys_cpu_to_le16(codec_cap->vid);

	if (net_buf_simple_tailroom(buf) < sizeof(*cc)) {
		goto fail;
	}

	cc = net_buf_simple_add(buf, sizeof(*cc));

	len = pac_data_add(buf, codec_cap->data_count, codec_cap->data);
	if (len < 0 || len > UINT8_MAX) {
		goto fail;
	}

	cc->len = len;

	if (net_buf_simple_tailroom(buf) < sizeof(*meta)) {
		goto fail;
	}

	meta = net_buf_simple_add(buf, sizeof(*meta));

	len = pac_data_add(buf, codec_cap->meta_count, codec_cap->meta);
	if (len < 0 || len > UINT8_MAX) {
		goto fail;
	}

	meta->len = len;

	data->rsp->num_pac++;

	return true;

fail:
	__ASSERT(false, "No space for %p", cap);

	net_buf_simple_restore(buf, &state);

	return false;
}

static void foreach_cap(sys_slist_t *list, bt_pacs_cap_foreach_func_t func,
			void *user_data)
{
	struct bt_pacs_cap *cap;

	SYS_SLIST_FOR_EACH_CONTAINER(list, cap, _node) {
		if (!func(cap, user_data)) {
			break;
		}
	}
}

static void get_pac_records(sys_slist_t *list, struct net_buf_simple *buf)
{
	struct pac_records_build_data data;

	/* Reset if buffer before using */
	net_buf_simple_reset(buf);

	data.rsp = net_buf_simple_add(buf, sizeof(*data.rsp));
	data.rsp->num_pac = 0;
	data.buf = buf;

	foreach_cap(list, build_pac_records, &data);
}

static void available_context_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	LOG_DBG("attr %p value 0x%04x", attr, value);
}

static ssize_t available_contexts_read(struct bt_conn *conn,
				       const struct bt_gatt_attr *attr, void *buf,
				       uint16_t len, uint16_t offset)
{
	struct bt_pacs_context context = {
		.snk = sys_cpu_to_le16(snk_available_contexts),
		.src = sys_cpu_to_le16(src_available_contexts),
	};

	LOG_DBG("conn %p attr %p buf %p len %u offset %u", conn, attr, buf, len, offset);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &context,
				 sizeof(context));
}

static void supported_context_cfg_changed(const struct bt_gatt_attr *attr,
					  uint16_t value)
{
	LOG_DBG("attr %p value 0x%04x", attr, value);
}

static ssize_t supported_context_read(struct bt_conn *conn,
				      const struct bt_gatt_attr *attr,
				      void *buf, uint16_t len, uint16_t offset)
{
	struct bt_pacs_context context = {
		.snk = sys_cpu_to_le16(snk_supported_contexts),
		.src = sys_cpu_to_le16(src_supported_contexts),
	};

	LOG_DBG("conn %p attr %p buf %p len %u offset %u", conn, attr, buf, len, offset);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &context,
				 sizeof(context));
}

static int available_contexts_notify(void);
static int supported_contexts_notify(void);

static int set_available_contexts(uint16_t contexts, uint16_t *available,
				  uint16_t supported)
{
	int err;

	if (contexts & ~supported) {
		return -ENOTSUP;
	}

	if (contexts == *available) {
		return 0;
	}

	err = available_contexts_notify();
	if (err) {
		return err;
	}

	*available = contexts;

	return 0;
}

static int set_supported_contexts(uint16_t contexts, uint16_t *supported,
				  uint16_t *available)
{
	int err;
	uint16_t tmp_supported = *supported;
	uint16_t tmp_available = *available;

	/* Ensure unspecified is always supported */
	contexts |= BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED;

	if (*supported == contexts) {
		return 0;
	}

	*supported = contexts;

	/* Update available contexts if needed*/
	if ((contexts & *available) != *available) {
		err = set_available_contexts(contexts & *available, available, contexts);
		if (err) {
			*available = tmp_available;
			*supported = tmp_supported;

			return err;
		}
	}

	err = supported_contexts_notify();
	if (err) {
		*supported = tmp_supported;
		*available = tmp_available;

		return err;
	}

	return 0;
}

#if defined(CONFIG_BT_PAC_SNK)
static int pac_notify_snk(void);
static ssize_t snk_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, uint16_t len, uint16_t offset)
{
	ssize_t ret_val;
	int err;

	LOG_DBG("conn %p attr %p buf %p len %u offset %u", conn, attr, buf, len, offset);

	err = k_sem_take(&read_buf_sem, READ_BUF_SEM_TIMEOUT);
	if (err != 0) {
		LOG_DBG("Failed to take read_buf_sem: %d", err);

		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	get_pac_records(&snk_pacs_list, &read_buf);

	ret_val = bt_gatt_attr_read(conn, attr, buf, len, offset, read_buf.data,
				    read_buf.len);

	k_sem_give(&read_buf_sem);

	return ret_val;
}

static void snk_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	LOG_DBG("attr %p value 0x%04x", attr, value);
}

static inline int set_snk_available_contexts(uint16_t contexts)
{
	return set_available_contexts(contexts, &snk_available_contexts,
				      snk_supported_contexts);
}

static inline int set_snk_supported_contexts(uint16_t contexts)
{
	return set_supported_contexts(contexts, &snk_supported_contexts,
				      &snk_available_contexts);
}
#else
static inline int set_snk_available_contexts(uint16_t contexts)
{
	return -ENOTSUP;
}

static inline int set_snk_supported_contexts(uint16_t contexts)
{
	return -ENOTSUP;
}
#endif /* CONFIG_BT_PAC_SNK */

#if defined(CONFIG_BT_PAC_SNK_LOC)
static int pac_notify_snk_loc(void);
static ssize_t snk_loc_read(struct bt_conn *conn,
			    const struct bt_gatt_attr *attr, void *buf,
			    uint16_t len, uint16_t offset)
{
	uint32_t location = sys_cpu_to_le32(pacs_snk_location);

	LOG_DBG("conn %p attr %p buf %p len %u offset %u", conn, attr, buf, len, offset);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &location,
				 sizeof(location));
}

static void snk_loc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	LOG_DBG("attr %p value 0x%04x", attr, value);
}

static int set_snk_location(enum bt_audio_location audio_location)
{
	int err;

	if (audio_location == pacs_snk_location) {
		return 0;
	}

	pacs_snk_location = audio_location;

	err = pac_notify_snk_loc();

	return err;
}
#else
static int set_snk_location(enum bt_audio_location location)
{
	return -ENOTSUP;
}
#endif /* CONFIG_BT_PAC_SNK_LOC */

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
		LOG_DBG("Invalid location value: 0x%08X", location);
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	err = set_snk_location(location);
	if (err != 0) {
		LOG_DBG("write_location returned %d", err);
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	return len;
}
#endif /* CONFIG_BT_PAC_SNK_LOC_WRITEABLE */

#if defined(CONFIG_BT_PAC_SRC)
static int pac_notify_src(void);
static ssize_t src_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, uint16_t len, uint16_t offset)
{
	ssize_t ret_val;
	int err;

	LOG_DBG("conn %p attr %p buf %p len %u offset %u", conn, attr, buf, len, offset);

	err = k_sem_take(&read_buf_sem, READ_BUF_SEM_TIMEOUT);
	if (err != 0) {
		LOG_DBG("Failed to take read_buf_sem: %d", err);

		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	get_pac_records(&src_pacs_list, &read_buf);

	ret_val = bt_gatt_attr_read(conn, attr, buf, len, offset, read_buf.data,
				    read_buf.len);

	k_sem_give(&read_buf_sem);

	return ret_val;
}

static void src_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	LOG_DBG("attr %p value 0x%04x", attr, value);
}

static inline int set_src_available_contexts(uint16_t contexts)
{
	return set_available_contexts(contexts, &src_available_contexts,
				      src_supported_contexts);
}

static inline int set_src_supported_contexts(uint16_t contexts)
{
	return set_supported_contexts(contexts, &src_supported_contexts,
				      &src_available_contexts);
}
#else
static inline int set_src_available_contexts(uint16_t contexts)
{
	return -ENOTSUP;
}

static inline int set_src_supported_contexts(uint16_t contexts)
{
	return -ENOTSUP;
}
#endif /* CONFIG_BT_PAC_SRC */

#if defined(CONFIG_BT_PAC_SRC_LOC)
static int pac_notify_src_loc(void);
static ssize_t src_loc_read(struct bt_conn *conn,
			    const struct bt_gatt_attr *attr, void *buf,
			    uint16_t len, uint16_t offset)
{
	uint32_t location = sys_cpu_to_le32(pacs_src_location);

	LOG_DBG("conn %p attr %p buf %p len %u offset %u", conn, attr, buf, len, offset);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &location,
				 sizeof(location));
}

static void src_loc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	LOG_DBG("attr %p value 0x%04x", attr, value);
}

static int set_src_location(enum bt_audio_location audio_location)
{
	int err;
	enum bt_audio_location tmp_audio_location = audio_location;


	if (audio_location == pacs_src_location) {
		return 0;
	}

	pacs_src_location = audio_location;

	err = pac_notify_src_loc();
	if (err) {
		LOG_DBG("Notify failed, previous audio location restored");
		audio_location = tmp_audio_location;

		return err;
	}

	return 0;
}
#else
static int set_src_location(enum bt_audio_location location)
{
	return -ENOTSUP;
}
#endif /* CONFIG_BT_PAC_SRC_LOC */

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
		LOG_DBG("Invalid location value: 0x%08X", location);
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	err = set_src_location(location);
	if (err != 0) {
		LOG_DBG("write_location returned %d", err);
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	return len;
}
#endif /* CONFIG_BT_PAC_SRC_LOC_WRITEABLE */

BT_GATT_SERVICE_DEFINE(pacs_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_PACS),
#if defined(CONFIG_BT_PAC_SNK)
	BT_AUDIO_CHRC(BT_UUID_PACS_SNK,
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
		      BT_GATT_PERM_READ_ENCRYPT,
		      snk_read, NULL, NULL),
	BT_AUDIO_CCC(snk_cfg_changed),
#if defined(CONFIG_BT_PAC_SNK_LOC)
#if defined(CONFIG_BT_PAC_SNK_LOC_WRITEABLE)
	BT_AUDIO_CHRC(BT_UUID_PACS_SNK_LOC,
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY,
		      BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT,
		      snk_loc_read, snk_loc_write, NULL),
#else
	BT_AUDIO_CHRC(BT_UUID_PACS_SNK_LOC,
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
		      BT_GATT_PERM_READ_ENCRYPT,
		      snk_loc_read, NULL, NULL),
#endif /* CONFIG_BT_PAC_SNK_LOC_WRITEABLE */
	BT_AUDIO_CCC(snk_loc_cfg_changed),
#endif /* CONFIG_BT_PAC_SNK_LOC */
#endif /* CONFIG_BT_PAC_SNK */
#if defined(CONFIG_BT_PAC_SRC)
	BT_AUDIO_CHRC(BT_UUID_PACS_SRC,
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
		      BT_GATT_PERM_READ_ENCRYPT,
		      src_read, NULL, NULL),
	BT_AUDIO_CCC(src_cfg_changed),
#if defined(CONFIG_BT_PAC_SRC_LOC)
#if defined(CONFIG_BT_PAC_SRC_LOC_WRITEABLE)
	BT_AUDIO_CHRC(BT_UUID_PACS_SRC_LOC,
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY,
		      BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT,
		      src_loc_read, src_loc_write, NULL),
#else
	BT_AUDIO_CHRC(BT_UUID_PACS_SRC_LOC,
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
		      BT_GATT_PERM_READ_ENCRYPT,
		      src_loc_read, NULL, NULL),
#endif /* CONFIG_BT_PAC_SRC_LOC_WRITEABLE */
	BT_AUDIO_CCC(src_loc_cfg_changed),
#endif /* CONFIG_BT_PAC_SRC_LOC */
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

#if defined(CONFIG_BT_PAC_SNK_LOC)
static int pac_notify_snk_loc(void)
{
	uint32_t location_le = sys_cpu_to_le32(pacs_snk_location);
	int err;

	err = bt_gatt_notify_uuid(NULL, BT_UUID_PACS_SNK_LOC, pacs_svc.attrs, &location_le,
				  sizeof(location_le));
	if (err != 0 && err != -ENOTCONN) {
		LOG_WRN("PACS notify_loc failed: %d", err);
		return err;
	}

	return 0;
}
#endif /* CONFIG_BT_PAC_SNK_LOC */

#if defined(CONFIG_BT_PAC_SRC_LOC)
static int pac_notify_src_loc(void)
{
	uint32_t location_le = sys_cpu_to_le32(pacs_src_location);
	int err;

	err = bt_gatt_notify_uuid(NULL, BT_UUID_PACS_SRC_LOC, pacs_svc.attrs, &location_le,
				  sizeof(location_le));
	if (err != 0 && err != -ENOTCONN) {
		LOG_WRN("PACS notify_loc failed: %d", err);
		return err;
	}

	return 0;
}
#endif /* CONFIG_BT_PAC_SRC_LOC */

#if defined(CONFIG_BT_PAC_SNK)
static int pac_notify_snk(void)
{
	int err = 0;

	err = k_sem_take(&read_buf_sem, K_NO_WAIT);
	if (err != 0) {
		LOG_DBG("Failed to take read_buf_sem: %d", err);

		return err;
	}

	get_pac_records(&snk_pacs_list, &read_buf);

	err = bt_gatt_notify_uuid(NULL, BT_UUID_PACS_SNK, pacs_svc.attrs,
				  read_buf.data, read_buf.len);
	if (err != 0 && err != -ENOTCONN) {
		LOG_WRN("PACS notify failed: %d", err);
	}

	k_sem_give(&read_buf_sem);

	if (err == -ENOTCONN) {
		return 0;
	} else {
		return 0;
	}
}
#endif /* CONFIG_BT_PAC_SNK */

#if defined(CONFIG_BT_PAC_SRC)
static int pac_notify_src(void)
{
	int err = 0;

	err = k_sem_take(&read_buf_sem, K_NO_WAIT);
	if (err != 0) {
		LOG_DBG("Failed to take read_buf_sem: %d", err);

		return err;
	}

	get_pac_records(&src_pacs_list, &read_buf);

	err = bt_gatt_notify_uuid(NULL, BT_UUID_PACS_SRC, pacs_svc.attrs,
				  read_buf.data, read_buf.len);
	if (err != 0 && err != -ENOTCONN) {
		LOG_WRN("PACS notify failed: %d", err);
	}

	k_sem_give(&read_buf_sem);

	if (err == -ENOTCONN) {
		return 0;
	} else {
		return 0;
	}
}
#endif /* CONFIG_BT_PAC_SRC */

static int pacs_changed(enum bt_audio_dir dir)
{
	int err = 0;

	if (dir == BT_AUDIO_DIR_SINK) {
#if defined(CONFIG_BT_PAC_SNK)
		err = pac_notify_snk();
#endif /* CONFIG_BT_PAC_SNK */
	} else {
#if defined(CONFIG_BT_PAC_SRC)
		err = pac_notify_src();
#endif /* CONFIG_BT_PAC_SRC */
	}

	return err;
}

static int available_contexts_notify(void)
{
	struct bt_pacs_context context = {
		.snk = sys_cpu_to_le16(snk_available_contexts),
		.src = sys_cpu_to_le16(src_available_contexts),
	};
	int err;

	err = bt_gatt_notify_uuid(NULL, BT_UUID_PACS_AVAILABLE_CONTEXT, pacs_svc.attrs,
				  &context, sizeof(context));
	if (err != 0 && err != -ENOTCONN) {
		LOG_WRN("Available Audio Contexts notify failed: %d", err);
		return err;
	}

	return 0;
}

static int supported_contexts_notify(void)
{
	struct bt_pacs_context context = {
		.snk = sys_cpu_to_le16(snk_supported_contexts),
		.src = sys_cpu_to_le16(src_supported_contexts),
	};
	int err;

	err = bt_gatt_notify_uuid(NULL, BT_UUID_PACS_SUPPORTED_CONTEXT, pacs_svc.attrs,
				  &context, sizeof(context));
	if (err != 0 && err != -ENOTCONN) {
		LOG_WRN("Supported Audio Contexts notify failed: %d", err);

		return err;
	}

	return 0;
}

bool bt_pacs_context_available(enum bt_audio_dir dir, uint16_t context)
{
	if (dir == BT_AUDIO_DIR_SOURCE) {
		return (context & src_available_contexts) == context;
	}

	if (dir == BT_AUDIO_DIR_SINK) {
		return (context & snk_available_contexts) == context;
	}

	return false;
}

static sys_slist_t *pacs_get(enum bt_audio_dir dir)
{
	switch (dir) {
#if defined(CONFIG_BT_PAC_SNK)
	case BT_AUDIO_DIR_SINK:
		return &snk_pacs_list;
#endif /* CONFIG_BT_PAC_SNK */
#if defined(CONFIG_BT_PAC_SRC)
	case BT_AUDIO_DIR_SOURCE:
		return &src_pacs_list;
#endif /* CONFIG_BT_PAC_SRC */
	default:
		return NULL;
	}
}

void bt_pacs_cap_foreach(enum bt_audio_dir dir, bt_pacs_cap_foreach_func_t func, void *user_data)
{
	sys_slist_t *pac;

	CHECKIF(func == NULL) {
		LOG_ERR("func is NULL");
		return;
	}

	pac = pacs_get(dir);
	if (!pac) {
		return;
	}

	foreach_cap(pac, func, user_data);
}

/* Register Audio Capability */
int bt_pacs_cap_register(enum bt_audio_dir dir, struct bt_pacs_cap *cap)
{
	const struct bt_audio_codec_cap *codec_cap;
	sys_slist_t *pac;
	int err;

	if (!cap || !cap->codec_cap) {
		return -EINVAL;
	}

	codec_cap = cap->codec_cap;

	pac = pacs_get(dir);
	if (!pac) {
		return -EINVAL;
	}

	LOG_DBG("cap %p dir %s codec_cap id 0x%02x codec_cap cid 0x%04x codec_cap vid 0x%04x", cap,
		bt_audio_dir_str(dir), codec_cap->id, codec_cap->cid, codec_cap->vid);

	sys_slist_append(pac, &cap->_node);

	err = pacs_changed(dir);
	if (err) {
		return -EINVAL;
	}

	return 0;
}

/* Unregister Audio Capability */
int bt_pacs_cap_unregister(enum bt_audio_dir dir, struct bt_pacs_cap *cap)
{
	sys_slist_t *pac;

	if (!cap) {
		return -EINVAL;
	}

	pac = pacs_get(dir);
	if (!pac) {
		return -EINVAL;
	}

	LOG_DBG("cap %p dir %s", cap, bt_audio_dir_str(dir));

	if (!sys_slist_find_and_remove(pac, &cap->_node)) {
		return -ENOENT;
	}

	pacs_changed(dir);

	return 0;
}

int bt_pacs_set_location(enum bt_audio_dir dir, enum bt_audio_location location)
{
	switch (dir) {
	case BT_AUDIO_DIR_SINK:
		return set_snk_location(location);
	case BT_AUDIO_DIR_SOURCE:
		return set_src_location(location);
	}

	return -EINVAL;
}

int bt_pacs_set_available_contexts(enum bt_audio_dir dir, enum bt_audio_context contexts)
{
	switch (dir) {
	case BT_AUDIO_DIR_SINK:
		return set_snk_available_contexts(contexts);
	case BT_AUDIO_DIR_SOURCE:
		return set_src_available_contexts(contexts);
	}

	return -EINVAL;
}

int bt_pacs_set_supported_contexts(enum bt_audio_dir dir, enum bt_audio_context contexts)
{
	switch (dir) {
	case BT_AUDIO_DIR_SINK:
		return set_snk_supported_contexts(contexts);
	case BT_AUDIO_DIR_SOURCE:
		return set_src_supported_contexts(contexts);
	}

	return -EINVAL;
}

enum bt_audio_context bt_pacs_get_available_contexts(enum bt_audio_dir dir)
{
	switch (dir) {
	case BT_AUDIO_DIR_SINK:
		return snk_available_contexts;
	case BT_AUDIO_DIR_SOURCE:
		return src_available_contexts;
	}

	return BT_AUDIO_CONTEXT_TYPE_PROHIBITED;
}

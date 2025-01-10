/* @file
 * @brief Bluetooth PACS
 */

/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/pacs.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

#include "../host/conn_internal.h"
#include "../host/hci_core.h"
#include "common/bt_str.h"

#include "audio_internal.h"
#include "bap_unicast_server.h"
#include "pacs_internal.h"

LOG_MODULE_REGISTER(bt_pacs, CONFIG_BT_PACS_LOG_LEVEL);

#define PAC_NOTIFY_TIMEOUT	K_MSEC(10)
#define READ_BUF_SEM_TIMEOUT    K_MSEC(50)

#if defined(CONFIG_BT_PAC_SRC)
static uint32_t pacs_src_location;
static sys_slist_t src_pac_list = SYS_SLIST_STATIC_INIT(&src_pac_list);
static uint16_t src_supported_contexts;
#endif /* CONFIG_BT_PAC_SRC */

#if defined(CONFIG_BT_PAC_SNK)
static uint32_t pacs_snk_location;
static sys_slist_t snk_pac_list = SYS_SLIST_STATIC_INIT(&snk_pac_list);
static uint16_t snk_supported_contexts;
#endif /* CONFIG_BT_PAC_SNK */

static uint16_t src_available_contexts = BT_AUDIO_CONTEXT_TYPE_PROHIBITED;
static uint16_t snk_available_contexts = BT_AUDIO_CONTEXT_TYPE_PROHIBITED;

enum {
	FLAG_ACTIVE,
	FLAG_SINK_PAC_CHANGED,
	FLAG_SINK_AUDIO_LOCATIONS_CHANGED,
	FLAG_SOURCE_PAC_CHANGED,
	FLAG_SOURCE_AUDIO_LOCATIONS_CHANGED,
	FLAG_AVAILABLE_AUDIO_CONTEXT_CHANGED,
	FLAG_SUPPORTED_AUDIO_CONTEXT_CHANGED,
	FLAG_NUM,
};

enum {
	PACS_FLAG_REGISTERED,
	PACS_FLAG_SVC_CHANGING,
	PACS_FLAG_NOTIFY_RDY,
	PACS_FLAG_SNK_PAC,
	PACS_FLAG_SNK_LOC,
	PACS_FLAG_SRC_PAC,
	PACS_FLAG_SRC_LOC,
	PACS_FLAG_NUM,
};

struct pacs_client {
	bt_addr_le_t addr;

#if defined(CONFIG_BT_PAC_SNK)
	/* Sink Available Contexts override value */
	uint16_t *snk_available_contexts;
#endif /* CONFIG_BT_PAC_SNK */

#if defined(CONFIG_BT_PAC_SRC)
	/* Source Available Contexts override value */
	uint16_t *src_available_contexts;
#endif /* CONFIG_BT_PAC_SRC */

	/* Pending notification flags */
	ATOMIC_DEFINE(flags, FLAG_NUM);
};

static struct pacs {
	atomic_t flags;

	struct pacs_client clients[CONFIG_BT_MAX_PAIRED];
} pacs;


static K_SEM_DEFINE(read_buf_sem, 1, 1);
NET_BUF_SIMPLE_DEFINE_STATIC(read_buf, BT_ATT_MAX_ATTRIBUTE_LEN);

static int pacs_gatt_notify(struct bt_conn *conn,
			    const struct bt_uuid *uuid,
			    const struct bt_gatt_attr *attr,
			    const void *data,
			    uint16_t len);
static void deferred_nfy_work_handler(struct k_work *work);

static K_WORK_DEFINE(deferred_nfy_work, deferred_nfy_work_handler);

struct pac_records_build_data {
	struct bt_pacs_read_rsp *rsp;
	struct net_buf_simple *buf;
};

static struct pacs_client *client_lookup_conn(const struct bt_conn *conn)
{
	__ASSERT_NO_MSG(conn != NULL);

	for (size_t i = 0; i < ARRAY_SIZE(pacs.clients); i++) {
		if (atomic_test_bit(pacs.clients[i].flags, FLAG_ACTIVE) &&
		    bt_addr_le_eq(&pacs.clients[i].addr, bt_conn_get_dst(conn))) {
			return &pacs.clients[i];
		}
	}

	return NULL;
}

static void pacs_set_notify_bit(int bit)
{
	for (size_t i = 0U; i < ARRAY_SIZE(pacs.clients); i++) {
		if (atomic_test_bit(pacs.clients[i].flags, FLAG_ACTIVE)) {
			atomic_set_bit(pacs.clients[i].flags, bit);
		}
	}
}

static bool build_pac_records(const struct bt_pacs_cap *cap, void *user_data)
{
	struct pac_records_build_data *data = user_data;
	const struct bt_audio_codec_cap *codec_cap = cap->codec_cap;
	struct net_buf_simple *buf = data->buf;
	struct net_buf_simple_state state;
	struct bt_pac_codec *pac_codec;

	net_buf_simple_save(buf, &state);

	if (net_buf_simple_tailroom(buf) < sizeof(*pac_codec)) {
		goto fail;
	}

	pac_codec = net_buf_simple_add(buf, sizeof(*pac_codec));
	pac_codec->id = codec_cap->id;
	pac_codec->cid = sys_cpu_to_le16(codec_cap->cid);
	pac_codec->vid = sys_cpu_to_le16(codec_cap->vid);

	if (net_buf_simple_tailroom(buf) < (sizeof(struct bt_pac_ltv_data) + codec_cap->data_len)) {
		goto fail;
	}

	net_buf_simple_add_u8(buf, codec_cap->data_len);
	net_buf_simple_add_mem(buf, codec_cap->data, codec_cap->data_len);

	if (net_buf_simple_tailroom(buf) < (sizeof(struct bt_pac_ltv_data) + codec_cap->meta_len)) {
		goto fail;
	}

	net_buf_simple_add_u8(buf, codec_cap->meta_len);
	net_buf_simple_add_mem(buf, codec_cap->meta, codec_cap->meta_len);

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

static enum bt_audio_context pacs_get_available_contexts_for_conn(struct bt_conn *conn,
								  enum bt_audio_dir dir)
{
	const struct pacs_client *client;

	client = client_lookup_conn(conn);
	if (client == NULL) {
		LOG_DBG("No client context for conn %p", (void *)conn);
		return bt_pacs_get_available_contexts(dir);
	}

	switch (dir) {
	case BT_AUDIO_DIR_SINK:
#if defined(CONFIG_BT_PAC_SNK)
		if (atomic_test_bit(&pacs.flags, PACS_FLAG_SNK_PAC) &&
		    client->snk_available_contexts != NULL) {
			return POINTER_TO_UINT(client->snk_available_contexts);
		}
#endif /* CONFIG_BT_PAC_SNK */
		break;
	case BT_AUDIO_DIR_SOURCE:
#if defined(CONFIG_BT_PAC_SRC)
		if (atomic_test_bit(&pacs.flags, PACS_FLAG_SRC_PAC) &&
		    client->src_available_contexts != NULL) {
			return POINTER_TO_UINT(client->src_available_contexts);
		}
#endif /* CONFIG_BT_PAC_SRC */
		break;
	}

	return bt_pacs_get_available_contexts(dir);
}

static ssize_t available_contexts_read(struct bt_conn *conn,
				       const struct bt_gatt_attr *attr, void *buf,
				       uint16_t len, uint16_t offset)
{
	struct bt_pacs_context context = {
		.snk = sys_cpu_to_le16(
			pacs_get_available_contexts_for_conn(conn, BT_AUDIO_DIR_SINK)),
		.src = sys_cpu_to_le16(
			pacs_get_available_contexts_for_conn(conn, BT_AUDIO_DIR_SOURCE)),
	};

	LOG_DBG("conn %p attr %p buf %p len %u offset %u", conn, attr, buf, len, offset);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &context,
				 sizeof(context));
}

#if defined(CONFIG_BT_PACS_SUPPORTED_CONTEXT_NOTIFIABLE)
static void supported_context_cfg_changed(const struct bt_gatt_attr *attr,
					  uint16_t value)
{
	LOG_DBG("attr %p value 0x%04x", attr, value);
}
#endif /* CONFIG_BT_PACS_SUPPORTED_CONTEXT_NOTIFIABLE */

static uint16_t supported_context_get(enum bt_audio_dir dir)
{
	switch (dir) {
#if defined(CONFIG_BT_PAC_SNK)
	case BT_AUDIO_DIR_SINK:
		if (atomic_test_bit(&pacs.flags, PACS_FLAG_SNK_PAC)) {
			return snk_supported_contexts | BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED;
		}
#endif /* CONFIG_BT_PAC_SNK */
#if defined(CONFIG_BT_PAC_SRC)
	case BT_AUDIO_DIR_SOURCE:
		if (atomic_test_bit(&pacs.flags, PACS_FLAG_SRC_PAC)) {
			return src_supported_contexts | BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED;
		}
#endif /* CONFIG_BT_PAC_SRC */
	default:
		break;
	}

	return BT_AUDIO_CONTEXT_TYPE_PROHIBITED;
}

static ssize_t supported_context_read(struct bt_conn *conn,
				      const struct bt_gatt_attr *attr,
				      void *buf, uint16_t len, uint16_t offset)
{
	struct bt_pacs_context context = {
		.snk = sys_cpu_to_le16(supported_context_get(BT_AUDIO_DIR_SINK)),
		.src = sys_cpu_to_le16(supported_context_get(BT_AUDIO_DIR_SOURCE)),
	};

	LOG_DBG("conn %p attr %p buf %p len %u offset %u", conn, attr, buf, len, offset);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &context,
				 sizeof(context));
}

static int set_available_contexts(uint16_t contexts, uint16_t *available,
				  uint16_t supported)
{
	if (contexts & ~supported) {
		return -ENOTSUP;
	}

	if (contexts == *available) {
		return 0;
	}

	*available = contexts;

	pacs_set_notify_bit(FLAG_AVAILABLE_AUDIO_CONTEXT_CHANGED);
	k_work_submit(&deferred_nfy_work);

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

	if (IS_ENABLED(CONFIG_BT_PACS_SUPPORTED_CONTEXT_NOTIFIABLE)) {
		pacs_set_notify_bit(FLAG_SUPPORTED_AUDIO_CONTEXT_CHANGED);
		k_work_submit(&deferred_nfy_work);
	}

	return 0;
}

#if defined(CONFIG_BT_PAC_SNK)
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

	get_pac_records(&snk_pac_list, &read_buf);

	ret_val = bt_gatt_attr_read(conn, attr, buf, len, offset, read_buf.data,
				    read_buf.len);

	k_sem_give(&read_buf_sem);

	return ret_val;
}

#if defined(CONFIG_BT_PAC_SNK_NOTIFIABLE)
static const struct bt_uuid *pacs_snk_uuid = BT_UUID_PACS_SNK;

static void snk_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	LOG_DBG("attr %p value 0x%04x", attr, value);
}
#endif /* CONFIG_BT_PAC_SNK_NOTIFIABLE */
#endif /* CONFIG_BT_PAC_SNK */

#if defined(CONFIG_BT_PAC_SNK_LOC)
static ssize_t snk_loc_read(struct bt_conn *conn,
			    const struct bt_gatt_attr *attr, void *buf,
			    uint16_t len, uint16_t offset)
{
	uint32_t location = sys_cpu_to_le32(pacs_snk_location);

	LOG_DBG("conn %p attr %p buf %p len %u offset %u", conn, attr, buf, len, offset);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &location,
				 sizeof(location));
}

#if defined(CONFIG_BT_PAC_SNK_LOC_NOTIFIABLE)
static const struct bt_uuid *pacs_snk_loc_uuid = BT_UUID_PACS_SNK_LOC;

static void snk_loc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	LOG_DBG("attr %p value 0x%04x", attr, value);
}
#endif /* CONFIG_BT_PAC_SNK_LOC_NOTIFIABLE */

static void set_snk_location(enum bt_audio_location audio_location)
{
	if (atomic_test_bit(&pacs.flags, PACS_FLAG_SNK_LOC)) {
		if (audio_location == pacs_snk_location) {
			return;
		}

		pacs_snk_location = audio_location;

		if (IS_ENABLED(CONFIG_BT_PAC_SNK_LOC_NOTIFIABLE)) {
			pacs_set_notify_bit(FLAG_SINK_AUDIO_LOCATIONS_CHANGED);
			k_work_submit(&deferred_nfy_work);
		}
	}
}
#else
static void set_snk_location(enum bt_audio_location location)
{
	return;
}
#endif /* CONFIG_BT_PAC_SNK_LOC */

#if defined(CONFIG_BT_PAC_SNK_LOC_WRITEABLE)
static ssize_t snk_loc_write(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, const void *data,
			     uint16_t len, uint16_t offset, uint8_t flags)
{
	enum bt_audio_location location;

	if (offset) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != sizeof(location)) {
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	location = (enum bt_audio_location)sys_get_le32(data);
	if (location > BT_AUDIO_LOCATION_MASK) {
		LOG_DBG("Invalid location value: 0x%08X", location);
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	set_snk_location(location);

	return len;
}
#endif /* CONFIG_BT_PAC_SNK_LOC_WRITEABLE */

#if defined(CONFIG_BT_PAC_SRC)
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

	get_pac_records(&src_pac_list, &read_buf);

	ret_val = bt_gatt_attr_read(conn, attr, buf, len, offset, read_buf.data,
				    read_buf.len);

	k_sem_give(&read_buf_sem);

	return ret_val;
}

#if defined(CONFIG_BT_PAC_SRC_NOTIFIABLE)
static const struct bt_uuid *pacs_src_uuid = BT_UUID_PACS_SRC;

static void src_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	LOG_DBG("attr %p value 0x%04x", attr, value);
}
#endif /* CONFIG_BT_PAC_SRC_NOTIFIABLE */
#endif /* CONFIG_BT_PAC_SRC */

#if defined(CONFIG_BT_PAC_SRC_LOC)
static ssize_t src_loc_read(struct bt_conn *conn,
			    const struct bt_gatt_attr *attr, void *buf,
			    uint16_t len, uint16_t offset)
{
	uint32_t location = sys_cpu_to_le32(pacs_src_location);

	LOG_DBG("conn %p attr %p buf %p len %u offset %u", conn, attr, buf, len, offset);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &location,
				 sizeof(location));
}

#if defined(CONFIG_BT_PAC_SRC_LOC_NOTIFIABLE)
static const struct bt_uuid *pacs_src_loc_uuid = BT_UUID_PACS_SRC_LOC;

static void src_loc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	LOG_DBG("attr %p value 0x%04x", attr, value);
}
#endif /* CONFIG_BT_PAC_SRC_LOC_NOTIFIABLE */

static void set_src_location(enum bt_audio_location audio_location)
{
	if (atomic_test_bit(&pacs.flags, PACS_FLAG_SRC_LOC)) {
		if (audio_location == pacs_src_location) {
			return;
		}

		pacs_src_location = audio_location;

		if (IS_ENABLED(CONFIG_BT_PAC_SRC_LOC_NOTIFIABLE)) {
			pacs_set_notify_bit(FLAG_SOURCE_AUDIO_LOCATIONS_CHANGED);
			k_work_submit(&deferred_nfy_work);
		}
	}
}
#else
static void set_src_location(enum bt_audio_location location)
{
	return;
}
#endif /* CONFIG_BT_PAC_SRC_LOC */

#if defined(CONFIG_BT_PAC_SRC_LOC_WRITEABLE)
static ssize_t src_loc_write(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, const void *data,
			     uint16_t len, uint16_t offset, uint8_t flags)
{
	uint32_t location;

	if (offset) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != sizeof(location)) {
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	location = (enum bt_audio_location)sys_get_le32(data);
	if (location > BT_AUDIO_LOCATION_MASK) {
		LOG_DBG("Invalid location value: 0x%08X", location);
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	set_src_location(location);

	return len;
}
#endif /* CONFIG_BT_PAC_SRC_LOC_WRITEABLE */


static sys_slist_t *pacs_get_pac(enum bt_audio_dir dir)
{
	switch (dir) {
#if defined(CONFIG_BT_PAC_SNK)
	case BT_AUDIO_DIR_SINK:
		if (atomic_test_bit(&pacs.flags, PACS_FLAG_SNK_PAC)) {
			return &snk_pac_list;
		}
		return NULL;
#endif /* CONFIG_BT_PAC_SNK */
#if defined(CONFIG_BT_PAC_SRC)
	case BT_AUDIO_DIR_SOURCE:
		if (atomic_test_bit(&pacs.flags, PACS_FLAG_SRC_PAC)) {
			return &src_pac_list;
		}
		return NULL;
#endif /* CONFIG_BT_PAC_SRC */
	default:
		return NULL;
	}
}

#if defined(CONFIG_BT_PAC_SNK)
#define BT_PACS_SNK_PROP \
	BT_GATT_CHRC_READ \
	IF_ENABLED(CONFIG_BT_PAC_SNK_LOC_NOTIFIABLE, (|BT_GATT_CHRC_NOTIFY))
#define BT_PAC_SNK(_read) \
	BT_AUDIO_CHRC(BT_UUID_PACS_SNK, \
		      BT_PACS_SNK_PROP, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      _read, NULL, NULL), \
	IF_ENABLED(CONFIG_BT_PAC_SNK_NOTIFIABLE, (BT_AUDIO_CCC(snk_cfg_changed),))

#define BT_PACS_SNK_LOC_PROP \
	BT_GATT_CHRC_READ \
	IF_ENABLED(CONFIG_BT_PAC_SNK_LOC_WRITEABLE, (|BT_GATT_CHRC_WRITE)) \
	IF_ENABLED(CONFIG_BT_PAC_SNK_LOC_NOTIFIABLE, (|BT_GATT_CHRC_NOTIFY))

#define BT_PACS_SNK_LOC_PERM \
	BT_GATT_PERM_READ_ENCRYPT \
	IF_ENABLED(CONFIG_BT_PAC_SNK_LOC_WRITEABLE, (|BT_GATT_PERM_WRITE_ENCRYPT))

#define BT_PACS_SNK_LOC(_read) \
	BT_AUDIO_CHRC(BT_UUID_PACS_SNK_LOC, \
		      BT_PACS_SNK_LOC_PROP, \
		      BT_PACS_SNK_LOC_PERM, \
		      _read, \
		      COND_CODE_1(CONFIG_BT_PAC_SNK_LOC_WRITEABLE, (snk_loc_write), (NULL)), \
		      NULL), \
	IF_ENABLED(CONFIG_BT_PAC_SNK_LOC_NOTIFIABLE, (BT_AUDIO_CCC(snk_loc_cfg_changed),))
#else
#define BT_PAC_SNK(_read)
#define BT_PACS_SNK_LOC(_read)
#endif

#if defined(CONFIG_BT_PAC_SRC)
#define BT_PACS_SRC_PROP \
	BT_GATT_CHRC_READ \
	IF_ENABLED(CONFIG_BT_PAC_SRC_LOC_NOTIFIABLE, (|BT_GATT_CHRC_NOTIFY))
#define BT_PAC_SRC(_read) \
	BT_AUDIO_CHRC(BT_UUID_PACS_SRC, \
		      BT_PACS_SRC_PROP, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      _read, NULL, NULL), \
	IF_ENABLED(CONFIG_BT_PAC_SRC_NOTIFIABLE, (BT_AUDIO_CCC(src_cfg_changed),))

#define BT_PACS_SRC_LOC_PROP \
	BT_GATT_CHRC_READ \
	IF_ENABLED(CONFIG_BT_PAC_SRC_LOC_WRITEABLE, (|BT_GATT_CHRC_WRITE)) \
	IF_ENABLED(CONFIG_BT_PAC_SRC_LOC_NOTIFIABLE, (|BT_GATT_CHRC_NOTIFY))

#define BT_PACS_SRC_LOC_PERM \
	BT_GATT_PERM_READ_ENCRYPT \
	IF_ENABLED(CONFIG_BT_PAC_SRC_LOC_WRITEABLE, (|BT_GATT_PERM_WRITE_ENCRYPT))

#define BT_PACS_SRC_LOC(_read) \
	BT_AUDIO_CHRC(BT_UUID_PACS_SRC_LOC, \
		      BT_PACS_SRC_LOC_PROP, \
		      BT_PACS_SRC_LOC_PERM, \
		      _read, \
		      COND_CODE_1(CONFIG_BT_PAC_SRC_LOC_WRITEABLE, (src_loc_write), (NULL)), \
		      NULL), \
	IF_ENABLED(CONFIG_BT_PAC_SRC_LOC_NOTIFIABLE, (BT_AUDIO_CCC(src_loc_cfg_changed),))
#else
#define BT_PAC_SRC(_read)
#define BT_PACS_SRC_LOC(_read)
#endif

#define BT_PAC_AVAILABLE_CONTEXT(_read) \
	BT_AUDIO_CHRC(BT_UUID_PACS_AVAILABLE_CONTEXT, \
		      BT_GATT_CHRC_READ|BT_GATT_CHRC_NOTIFY, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      _read, NULL, NULL), \
	BT_AUDIO_CCC(available_context_cfg_changed),

#define BT_PACS_SUPPORTED_CONTEXT_PROP \
	BT_GATT_CHRC_READ \
	IF_ENABLED(CONFIG_BT_PACS_SUPPORTED_CONTEXT_NOTIFIABLE, (|BT_GATT_CHRC_NOTIFY))

#define BT_PAC_SUPPORTED_CONTEXT(_read) \
	BT_AUDIO_CHRC(BT_UUID_PACS_SUPPORTED_CONTEXT, \
		      BT_PACS_SUPPORTED_CONTEXT_PROP, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      _read, NULL, NULL), \
	IF_ENABLED(CONFIG_BT_PACS_SUPPORTED_CONTEXT_NOTIFIABLE, \
		   (BT_AUDIO_CCC(supported_context_cfg_changed),))

#define BT_PACS_SERVICE_DEFINITION() { \
	BT_GATT_PRIMARY_SERVICE(BT_UUID_PACS), \
	BT_PAC_SNK(snk_read) \
	BT_PACS_SNK_LOC(snk_loc_read) \
	BT_PAC_SRC(src_read) \
	BT_PACS_SRC_LOC(src_loc_read) \
	BT_PAC_AVAILABLE_CONTEXT(available_contexts_read) \
	BT_PAC_SUPPORTED_CONTEXT(supported_context_read) \
}

static struct bt_gatt_attr pacs_attrs[] = BT_PACS_SERVICE_DEFINITION();
static struct bt_gatt_service pacs_svc = (struct bt_gatt_service)BT_GATT_SERVICE(pacs_attrs);




#if defined(BT_PAC_SNK_NOTIFIABLE)
#define PACS_SINK_PAC_CHAR_ATTR_COUNT 3  /* declaration + value + cccd */
#else
#define PACS_SINK_PAC_CHAR_ATTR_COUNT 2  /* declaration + value */
#endif /* BT_PAC_SNK_NOTIFIABLE */

#if defined(BT_PAC_SNK_LOC_NOTIFIABLE)
#define PACS_SINK_PAC_LOC_CHAR_ATTR_COUNT 3  /* declaration + value + cccd */
#else
#define PACS_SINK_PAC_LOC_CHAR_ATTR_COUNT 2  /* declaration + value*/
#endif /* BT_PAC_SNK_LOC_NOTIFIABLE */

#if defined(BT_PAC_SRC_NOTIFIABLE)
#define PACS_SOURCE_PAC_CHAR_ATTR_COUNT 3  /* declaration + value + cccd */
#else
#define PACS_SOURCE_PAC_CHAR_ATTR_COUNT 2  /* declaration + value */
#endif /* BT_PAC_SRC_NOTIFIABLE */

#if defined(BT_PAC_SRC_LOC_NOTIFIABLE)
#define PACS_SOURCE_PAC_LOC_CHAR_ATTR_COUNT 3  /* declaration + value + cccd */
#else
#define PACS_SOURCE_PAC_LOC_CHAR_ATTR_COUNT 2  /* declaration + value*/
#endif /* BT_PAC_SRC_LOC_NOTIFIABLE */

static void configure_pacs_char(const struct bt_bap_pacs_register_param *param)
{
	size_t attrs_to_rem;
	uint8_t first_to_rem;

	/* Remove the Sink PAC and Location */
	if (!param->snk_pac) {
		first_to_rem = 0;
		attrs_to_rem = PACS_SINK_PAC_CHAR_ATTR_COUNT + PACS_SINK_PAC_LOC_CHAR_ATTR_COUNT;
	} else if (!param->snk_loc) {
		first_to_rem = PACS_SINK_PAC_CHAR_ATTR_COUNT;
		attrs_to_rem = PACS_SINK_PAC_LOC_CHAR_ATTR_COUNT;
	} else {
		first_to_rem = pacs_svc.attr_count;
		attrs_to_rem = 0;
	}

	for (size_t i = first_to_rem + attrs_to_rem; i < pacs_svc.attr_count; i++) {
		pacs_svc.attrs[i - attrs_to_rem] = pacs_svc.attrs[i];
	}
	pacs_svc.attr_count -= attrs_to_rem;

	/* Set first_to_rem to the start of Source PAC Char, for cleaner offset calc */
	first_to_rem = PACS_SINK_PAC_CHAR_ATTR_COUNT + PACS_SINK_PAC_LOC_CHAR_ATTR_COUNT;

	/* Remove the Source PAC and Location */
	if (!param->snk_pac) {
		first_to_rem -= attrs_to_rem;
		attrs_to_rem = PACS_SOURCE_PAC_CHAR_ATTR_COUNT +
			       PACS_SOURCE_PAC_LOC_CHAR_ATTR_COUNT;
	} else if (!param->snk_loc) {
		first_to_rem = first_to_rem + PACS_SOURCE_PAC_CHAR_ATTR_COUNT - attrs_to_rem;
		attrs_to_rem = PACS_SINK_PAC_LOC_CHAR_ATTR_COUNT;
	} else {
		return;
	}

	for (size_t i = first_to_rem; i < pacs_svc.attr_count; i++) {
		pacs_svc.attrs[i - attrs_to_rem] = pacs_svc.attrs[i];
	}
	pacs_svc.attr_count -= attrs_to_rem;
}

int bt_pacs_register(const struct bt_bap_pacs_register_param *param)
{
	int err = 0;

	if (atomic_test_and_set_bit(&pacs.flags, PACS_FLAG_REGISTERED)) {
		LOG_DBG("PACS already registered");

		return -EALREADY;
	}

	/* Save registration param so we can guard functions accordingly */
#if defined(CONFIG_BT_PAC_SNK)
	atomic_set_bit_to(&pacs.flags, PACS_FLAG_SNK_PAC, param->snk_pac);
#endif /* CONFIG_BT_PAC_SNK */
#if defined(CONFIG_BT_PAC_SNK_LOC)
	atomic_set_bit_to(&pacs.flags, PACS_FLAG_SNK_LOC, param->snk_loc);
#endif /* CONFIG_BT_PAC_SNK_LOC */
#if defined(CONFIG_BT_PAC_SRC)
	atomic_set_bit_to(&pacs.flags, PACS_FLAG_SRC_PAC, param->src_pac);
#endif /* CONFIG_BT_PAC_SRC */
#if defined(CONFIG_BT_PAC_SRC_LOC)
	atomic_set_bit_to(&pacs.flags, PACS_FLAG_SRC_LOC, param->src_loc);
#endif /* CONFIG_BT_PAC_SRC_LOC */

	/* Remove characteristics if necessary */
	configure_pacs_char(param);

	err = bt_gatt_service_register(&pacs_svc);
	if (err != 0) {
		LOG_DBG("Failed to register ASCS in gatt DB");
		atomic_clear_bit(&pacs.flags, PACS_FLAG_REGISTERED);

		return err;
	}

	return 0;
}

int bt_pacs_unregister(void)
{
	int err;
	struct bt_gatt_attr _pacs_attrs[] = BT_PACS_SERVICE_DEFINITION();

	if (!atomic_test_bit(&pacs.flags, PACS_FLAG_REGISTERED)) {
		LOG_DBG("No pacs instance registered");

		return -EALREADY;
	}

	if (atomic_test_and_set_bit(&pacs.flags, PACS_FLAG_SVC_CHANGING)) {
		LOG_DBG("Service change already in progress");
		atomic_clear_bit(&pacs.flags, PACS_FLAG_SVC_CHANGING);

		return -EBUSY;
	}

	err = bt_gatt_service_unregister(&pacs_svc);

	/* If unregistration was successful, make sure to reset pacs_attrs so it can be used for
	 * new registrations
	 */
	if (err != 0) {
		LOG_DBG("Failed to unregister PACS");
		atomic_clear_bit(&pacs.flags, PACS_FLAG_SVC_CHANGING);

		return err;
	}

	memcpy(pacs_svc.attrs, &_pacs_attrs, sizeof(struct bt_gatt_attr));
	pacs_svc.attr_count = ARRAY_SIZE(pacs_attrs);

	atomic_clear_bit(&pacs.flags, PACS_FLAG_REGISTERED);
	atomic_clear_bit(&pacs.flags, PACS_FLAG_SVC_CHANGING);

	return err;
}

#if defined(CONFIG_BT_PAC_SNK_LOC_NOTIFIABLE) || defined(CONFIG_BT_PAC_SRC_LOC_NOTIFIABLE)
static int pac_notify_loc(struct bt_conn *conn, enum bt_audio_dir dir)
{
	uint32_t location_le;
	int err;
	const struct bt_uuid *uuid;

	switch (dir) {
#if defined(CONFIG_BT_PAC_SNK_LOC_NOTIFIABLE)
	case BT_AUDIO_DIR_SINK:
		location_le = sys_cpu_to_le32(pacs_snk_location);
		uuid = pacs_snk_loc_uuid;
		break;
#endif /* CONFIG_BT_PAC_SNK_LOC_NOTIFIABLE */
#if defined(CONFIG_BT_PAC_SRC_LOC_NOTIFIABLE)
	case BT_AUDIO_DIR_SOURCE:
		location_le = sys_cpu_to_le32(pacs_src_location);
		uuid = pacs_src_loc_uuid;
		break;
#endif /* CONFIG_BT_PAC_SRC_LOC_NOTIFIABLE */
	default:
		return -EINVAL;
	}

	err = pacs_gatt_notify(conn, uuid, pacs_svc.attrs, &location_le, sizeof(location_le));
	if (err != 0 && err != -ENOTCONN) {
		LOG_WRN("PACS notify_loc failed: %d", err);
		return err;
	}

	return 0;
}
#endif /* CONFIG_BT_PAC_SNK_LOC_NOTIFIABLE || CONFIG_BT_PAC_SRC_LOC_NOTIFIABLE */

#if defined(CONFIG_BT_PAC_SNK_NOTIFIABLE) || defined(CONFIG_BT_PAC_SRC_NOTIFIABLE)
static int pac_notify(struct bt_conn *conn, enum bt_audio_dir dir)
{
	int err = 0;
	sys_slist_t *pac;
	const struct bt_uuid *uuid;

	switch (dir) {
#if defined(CONFIG_BT_PAC_SNK_NOTIFIABLE)
	case BT_AUDIO_DIR_SINK:
		uuid = pacs_snk_uuid;
		break;
#endif /* CONFIG_BT_PAC_SNK_NOTIFIABLE */
#if defined(CONFIG_BT_PAC_SRC_NOTIFIABLE)
	case BT_AUDIO_DIR_SOURCE:
		uuid = pacs_src_uuid;
		break;
#endif /* CONFIG_BT_PAC_SRC_NOTIFIABLE */
	default:
		return -EINVAL;
	}

	err = k_sem_take(&read_buf_sem, K_NO_WAIT);
	if (err != 0) {
		LOG_DBG("Failed to take read_buf_sem: %d", err);

		return err;
	}

	pac = pacs_get_pac(dir);
	__ASSERT(pac, "Failed to get pacs.\n");
	get_pac_records(pac, &read_buf);

	err = pacs_gatt_notify(conn, uuid, pacs_svc.attrs,
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
#endif /* CONFIG_BT_PAC_SNK_NOTIFIABLE || CONFIG_BT_PAC_SRC_NOTIFIABLE */

static int available_contexts_notify(struct bt_conn *conn)
{
	struct bt_pacs_context context = {
		.snk = sys_cpu_to_le16(
			pacs_get_available_contexts_for_conn(conn, BT_AUDIO_DIR_SINK)),
		.src = sys_cpu_to_le16(
			pacs_get_available_contexts_for_conn(conn, BT_AUDIO_DIR_SOURCE)),
	};
	int err;

	err = pacs_gatt_notify(conn, BT_UUID_PACS_AVAILABLE_CONTEXT, pacs_svc.attrs,
				  &context, sizeof(context));
	if (err != 0 && err != -ENOTCONN) {
		LOG_WRN("Available Audio Contexts notify failed: %d", err);
		return err;
	}

	return 0;
}

static int supported_contexts_notify(struct bt_conn *conn)
{
	struct bt_pacs_context context = {
		.snk = sys_cpu_to_le16(supported_context_get(BT_AUDIO_DIR_SINK)),
		.src = sys_cpu_to_le16(supported_context_get(BT_AUDIO_DIR_SOURCE)),
	};
	int err;

	err = pacs_gatt_notify(conn, BT_UUID_PACS_SUPPORTED_CONTEXT, pacs_svc.attrs,
				  &context, sizeof(context));
	if (err != 0 && err != -ENOTCONN) {
		LOG_WRN("Supported Audio Contexts notify failed: %d", err);

		return err;
	}
	return 0;
}

void pacs_gatt_notify_complete_cb(struct bt_conn *conn, void *user_data)
{
	/* Notification done, clear bit and reschedule work */
	atomic_clear_bit(&pacs.flags, PACS_FLAG_NOTIFY_RDY);
	k_work_submit(&deferred_nfy_work);
}

static int pacs_gatt_notify(struct bt_conn *conn,
			    const struct bt_uuid *uuid,
			    const struct bt_gatt_attr *attr,
			    const void *data,
			    uint16_t len)
{
	int err;
	struct bt_gatt_notify_params params;

	memset(&params, 0, sizeof(params));
	params.uuid = uuid;
	params.attr = attr;
	params.data = data;
	params.len  = len;
	params.func = pacs_gatt_notify_complete_cb;

	/* Mark notification in progress */
	if (atomic_test_bit(&pacs.flags, PACS_FLAG_SVC_CHANGING) ||
	    !atomic_test_bit(&pacs.flags, PACS_FLAG_REGISTERED)) {
		return 0;
	}

	atomic_set_bit(&pacs.flags, PACS_FLAG_NOTIFY_RDY);

	err = bt_gatt_notify_cb(conn, &params);
	if (err != 0) {
		atomic_clear_bit(&pacs.flags, PACS_FLAG_NOTIFY_RDY);
	}

	if (err && err != -ENOTCONN) {
		return err;
	}

	return 0;
}

static void notify_cb(struct bt_conn *conn, void *data)
{
	struct pacs_client *client;
	struct bt_conn_info info;
	int err = 0;

	LOG_DBG("");

	err = bt_conn_get_info(conn, &info);
	if (err != 0) {
		LOG_ERR("Failed to get conn info: %d", err);
		return;
	}

	if (info.state != BT_CONN_STATE_CONNECTED) {
		/* Not connected */
		return;
	}

	client = client_lookup_conn(conn);
	if (client == NULL) {
		return;
	}

	/* Check if we have unverified notifications in progress */
	if (atomic_test_bit(&pacs.flags, PACS_FLAG_NOTIFY_RDY)) {
		return;
	}

#if defined(CONFIG_BT_PAC_SNK_NOTIFIABLE)
	if (atomic_test_bit(client->flags, FLAG_SINK_PAC_CHANGED)) {
		LOG_DBG("Notifying Sink PAC");
		err = pac_notify(conn, BT_AUDIO_DIR_SINK);
		if (!err) {
			atomic_clear_bit(client->flags, FLAG_SINK_PAC_CHANGED);
		}
	}
#endif /* CONFIG_BT_PAC_SNK_NOTIFIABLE */

#if defined(CONFIG_BT_PAC_SNK_LOC_NOTIFIABLE)
	if (atomic_test_bit(client->flags, FLAG_SINK_AUDIO_LOCATIONS_CHANGED)) {
		LOG_DBG("Notifying Sink Audio Location");
		err = pac_notify_loc(conn, BT_AUDIO_DIR_SINK);
		if (!err) {
			atomic_clear_bit(client->flags, FLAG_SINK_AUDIO_LOCATIONS_CHANGED);
		}
	}
#endif /* CONFIG_BT_PAC_SNK_LOC_NOTIFIABLE */

#if defined(CONFIG_BT_PAC_SRC_NOTIFIABLE)
	if (atomic_test_bit(client->flags, FLAG_SOURCE_PAC_CHANGED)) {
		LOG_DBG("Notifying Source PAC");
		err = pac_notify(conn, BT_AUDIO_DIR_SOURCE);
		if (!err) {
			atomic_clear_bit(client->flags, FLAG_SOURCE_PAC_CHANGED);
		}
	}
#endif /* CONFIG_BT_PAC_SRC_NOTIFIABLE */

#if defined(CONFIG_BT_PAC_SRC_LOC_NOTIFIABLE)
	if (atomic_test_and_clear_bit(client->flags, FLAG_SOURCE_AUDIO_LOCATIONS_CHANGED)) {
		LOG_DBG("Notifying Source Audio Location");
		err = pac_notify_loc(conn, BT_AUDIO_DIR_SOURCE);
		if (!err) {
			atomic_clear_bit(client->flags, FLAG_SOURCE_AUDIO_LOCATIONS_CHANGED);
		}
	}
#endif /* CONFIG_BT_PAC_SRC_LOC_NOTIFIABLE */

	if (atomic_test_bit(client->flags, FLAG_AVAILABLE_AUDIO_CONTEXT_CHANGED)) {
		LOG_DBG("Notifying Available Contexts");
		err = available_contexts_notify(conn);
		if (!err) {
			atomic_clear_bit(client->flags, FLAG_AVAILABLE_AUDIO_CONTEXT_CHANGED);
		}
	}

	if (IS_ENABLED(CONFIG_BT_PACS_SUPPORTED_CONTEXT_NOTIFIABLE) &&
	    atomic_test_bit(client->flags, FLAG_SUPPORTED_AUDIO_CONTEXT_CHANGED)) {
		LOG_DBG("Notifying Supported Contexts");
		err = supported_contexts_notify(conn);
		if (!err) {
			atomic_clear_bit(client->flags, FLAG_SUPPORTED_AUDIO_CONTEXT_CHANGED);
		}
	}
}

static void deferred_nfy_work_handler(struct k_work *work)
{
	bt_conn_foreach(BT_CONN_TYPE_LE, notify_cb, NULL);
}

static void pacs_auth_pairing_complete(struct bt_conn *conn, bool bonded)
{
	LOG_DBG("%s paired (%sbonded)", bt_addr_le_str(bt_conn_get_dst(conn)),
		bonded ? "" : "not ");

	if (!bonded) {
		return;
	}

	/* Check if already in list, and do nothing if it is */
	for (size_t i = 0U; i < ARRAY_SIZE(pacs.clients); i++) {
		if (atomic_test_bit(pacs.clients[i].flags, FLAG_ACTIVE) &&
		    bt_addr_le_eq(bt_conn_get_dst(conn), &pacs.clients[i].addr)) {
			return;
		}
	}

	/* Else add the device */
	for (size_t i = 0U; i < ARRAY_SIZE(pacs.clients); i++) {
		if (!atomic_test_bit(pacs.clients[i].flags, FLAG_ACTIVE)) {
			atomic_set_bit(pacs.clients[i].flags, FLAG_ACTIVE);
			memcpy(&pacs.clients[i].addr, bt_conn_get_dst(conn), sizeof(bt_addr_le_t));

			/* Send out all pending notifications */
			k_work_submit(&deferred_nfy_work);
			return;
		}
	}
}

static void pacs_bond_deleted(uint8_t id, const bt_addr_le_t *peer)
{
	/* Find the device entry to delete */
	for (size_t i = 0U; i < ARRAY_SIZE(pacs.clients); i++) {
		/* Check if match, and if active, if so, reset */
		if (atomic_test_bit(pacs.clients[i].flags, FLAG_ACTIVE) &&
		    bt_addr_le_eq(peer, &pacs.clients[i].addr)) {
			for (size_t j = 0U; j < FLAG_NUM; j++) {
				atomic_clear_bit(pacs.clients[i].flags, j);
			}
			(void)memset(&pacs.clients[i].addr, 0, sizeof(bt_addr_le_t));
			return;
		}
	}
}

static void pacs_security_changed(struct bt_conn *conn, bt_security_t level,
				  enum bt_security_err err)
{
	LOG_DBG("%s changed security level to %d", bt_addr_le_str(bt_conn_get_dst(conn)), level);

	if (err != 0 || conn->encrypt == 0) {
		return;
	}

	if (!bt_addr_le_is_bonded(conn->id, &conn->le.dst)) {
		return;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(pacs.clients); i++) {
		for (size_t j = 0U; j < FLAG_NUM; j++) {
			if (atomic_test_bit(pacs.clients[i].flags, j)) {

				/**
				 *  It's enough that one flag is set, as the defer work will go
				 * through all notifiable characteristics
				 */
				k_work_submit(&deferred_nfy_work);
				return;
			}
		}
	}
}

static void pacs_disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct pacs_client *client;

	client = client_lookup_conn(conn);
	if (client == NULL) {
		return;
	}

#if defined(CONFIG_BT_PAC_SNK)
	if (atomic_test_bit(&pacs.flags, PACS_FLAG_SNK_PAC) &&
	    client->snk_available_contexts != NULL) {
		uint16_t old = POINTER_TO_UINT(client->snk_available_contexts);
		uint16_t new;

		client->snk_available_contexts = NULL;
		new = pacs_get_available_contexts_for_conn(conn, BT_AUDIO_DIR_SINK);

		atomic_set_bit_to(client->flags, FLAG_AVAILABLE_AUDIO_CONTEXT_CHANGED, old != new);
	}
#endif /* CONFIG_BT_PAC_SNK */

#if defined(CONFIG_BT_PAC_SRC)
	if (atomic_test_bit(&pacs.flags, PACS_FLAG_SRC_PAC) &&
	    client->src_available_contexts != NULL) {
		uint16_t old = POINTER_TO_UINT(client->src_available_contexts);
		uint16_t new;

		client->src_available_contexts = NULL;
		new = pacs_get_available_contexts_for_conn(conn, BT_AUDIO_DIR_SOURCE);

		atomic_set_bit_to(client->flags, FLAG_AVAILABLE_AUDIO_CONTEXT_CHANGED, old != new);
	}
#endif /* CONFIG_BT_PAC_SRC */
}

static struct bt_conn_cb conn_callbacks = {
	.security_changed = pacs_security_changed,
	.disconnected = pacs_disconnected,
};

static struct bt_conn_auth_info_cb auth_callbacks = {
	.pairing_complete = pacs_auth_pairing_complete,
	.bond_deleted = pacs_bond_deleted
};

void bt_pacs_cap_foreach(enum bt_audio_dir dir, bt_pacs_cap_foreach_func_t func, void *user_data)
{
	sys_slist_t *pac;

	CHECKIF(func == NULL) {
		LOG_ERR("func is NULL");
		return;
	}

	pac = pacs_get_pac(dir);
	if (!pac) {
		return;
	}

	foreach_cap(pac, func, user_data);
}

static void add_bonded_addr_to_client_list(const struct bt_bond_info *info, void *data)
{
	for (uint8_t i = 0; i < ARRAY_SIZE(pacs.clients); i++) {
		/* Check if device is registered, it not, add it */
		if (!atomic_test_bit(pacs.clients[i].flags, FLAG_ACTIVE)) {
			char addr_str[BT_ADDR_LE_STR_LEN];

			atomic_set_bit(pacs.clients[i].flags, FLAG_ACTIVE);
			memcpy(&pacs.clients[i].addr, &info->addr, sizeof(bt_addr_le_t));
			bt_addr_le_to_str(&pacs.clients[i].addr, addr_str, sizeof(addr_str));
			LOG_DBG("Added %s to bonded list\n", addr_str);
			return;
		}
	}
}

/* Register Audio Capability */
int bt_pacs_cap_register(enum bt_audio_dir dir, struct bt_pacs_cap *cap)
{
	const struct bt_audio_codec_cap *codec_cap;
	static bool callbacks_registered;
	sys_slist_t *pac;

	if (!cap || !cap->codec_cap) {
		return -EINVAL;
	}

	codec_cap = cap->codec_cap;

	pac = pacs_get_pac(dir);
	if (!pac) {
		return -EINVAL;
	}

	/* Restore bonding list */
	bt_foreach_bond(BT_ID_DEFAULT, add_bonded_addr_to_client_list, NULL);

	LOG_DBG("cap %p dir %s codec_cap id 0x%02x codec_cap cid 0x%04x codec_cap vid 0x%04x", cap,
		bt_audio_dir_str(dir), codec_cap->id, codec_cap->cid, codec_cap->vid);

	sys_slist_append(pac, &cap->_node);

	if (!callbacks_registered) {
		bt_conn_cb_register(&conn_callbacks);
		bt_conn_auth_info_cb_register(&auth_callbacks);

		callbacks_registered = true;
	}

	if (IS_ENABLED(CONFIG_BT_PAC_SNK_NOTIFIABLE) && dir == BT_AUDIO_DIR_SINK) {
		pacs_set_notify_bit(FLAG_SINK_PAC_CHANGED);
		k_work_submit(&deferred_nfy_work);
	}

	if (IS_ENABLED(CONFIG_BT_PAC_SRC_NOTIFIABLE) && dir == BT_AUDIO_DIR_SOURCE) {
		pacs_set_notify_bit(FLAG_SOURCE_PAC_CHANGED);
		k_work_submit(&deferred_nfy_work);
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

	pac = pacs_get_pac(dir);
	if (!pac) {
		return -EINVAL;
	}

	LOG_DBG("cap %p dir %s", cap, bt_audio_dir_str(dir));

	if (!sys_slist_find_and_remove(pac, &cap->_node)) {
		return -ENOENT;
	}

	switch (dir) {
#if defined(CONFIG_BT_PAC_SNK_NOTIFIABLE)
	case BT_AUDIO_DIR_SINK:
		pacs_set_notify_bit(FLAG_SINK_PAC_CHANGED);
		k_work_submit(&deferred_nfy_work);
		break;
#endif /* CONFIG_BT_PAC_SNK_NOTIFIABLE) */
#if defined(CONFIG_BT_PAC_SRC_NOTIFIABLE)
	case BT_AUDIO_DIR_SOURCE:
		pacs_set_notify_bit(FLAG_SOURCE_PAC_CHANGED);
		k_work_submit(&deferred_nfy_work);
		break;
#endif /* CONFIG_BT_PAC_SRC_NOTIFIABLE */
	default:
		return -EINVAL;
	}

	return 0;
}

int bt_pacs_set_location(enum bt_audio_dir dir, enum bt_audio_location location)
{
	switch (dir) {
	case BT_AUDIO_DIR_SINK:
		set_snk_location(location);
		break;
	case BT_AUDIO_DIR_SOURCE:
		set_src_location(location);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

int bt_pacs_set_available_contexts(enum bt_audio_dir dir, enum bt_audio_context contexts)
{
	if (!atomic_test_bit(&pacs.flags, PACS_FLAG_REGISTERED)) {
		return -EINVAL;
	}
	switch (dir) {
	case BT_AUDIO_DIR_SINK:
		if (atomic_test_bit(&pacs.flags, PACS_FLAG_SNK_PAC)) {
			return set_available_contexts(contexts, &snk_available_contexts,
						      supported_context_get(dir));
		}
		return -EINVAL;
	case BT_AUDIO_DIR_SOURCE:
		if (atomic_test_bit(&pacs.flags, PACS_FLAG_SRC_PAC)) {
			return set_available_contexts(contexts, &src_available_contexts,
						      supported_context_get(dir));
		}
		return -EINVAL;
	}

	return -EINVAL;
}

int bt_pacs_conn_set_available_contexts_for_conn(struct bt_conn *conn, enum bt_audio_dir dir,
						 enum bt_audio_context *contexts)
{
	enum bt_audio_context old = pacs_get_available_contexts_for_conn(conn, dir);
	struct bt_conn_info info = { 0 };
	struct pacs_client *client;
	int err;

	client = client_lookup_conn(conn);
	if (client == NULL) {
		return -ENOENT;
	}

	err = bt_conn_get_info(conn, &info);
	if (err < 0) {
		LOG_ERR("Could not get conn info: %d", err);
		return err;
	}

	switch (dir) {
#if defined(CONFIG_BT_PAC_SNK)
	case BT_AUDIO_DIR_SINK:
		if (atomic_test_bit(&pacs.flags, PACS_FLAG_SNK_PAC)) {
			if (contexts != NULL) {
				client->snk_available_contexts = UINT_TO_POINTER(*contexts);
			} else {
				client->snk_available_contexts = NULL;
			}
			break;
		}

		return -EINVAL;
#endif /* CONFIG_BT_PAC_SNK */
#if defined(CONFIG_BT_PAC_SRC)
	case BT_AUDIO_DIR_SOURCE:
		if (atomic_test_bit(&pacs.flags, PACS_FLAG_SRC_PAC)) {
			if (contexts != NULL) {
				client->src_available_contexts = UINT_TO_POINTER(*contexts);
			} else {
				client->src_available_contexts = NULL;
			}
			break;
		}

		return -EINVAL;
#endif /* CONFIG_BT_PAC_SRC */
	default:
		return -EINVAL;
	}

	if (pacs_get_available_contexts_for_conn(conn, dir) == old) {
		/* No change. Skip notification */
		return 0;
	}

	atomic_set_bit(client->flags, FLAG_AVAILABLE_AUDIO_CONTEXT_CHANGED);

	/* Send notification on encrypted link only */
	if (info.security.level > BT_SECURITY_L1) {
		k_work_submit(&deferred_nfy_work);
	}

	return 0;
}

int bt_pacs_set_supported_contexts(enum bt_audio_dir dir, enum bt_audio_context contexts)
{
	uint16_t *supported_contexts = NULL;
	uint16_t *available_contexts = NULL;

	switch (dir) {
	case BT_AUDIO_DIR_SINK:
#if defined(CONFIG_BT_PAC_SNK)
		if (atomic_test_bit(&pacs.flags, PACS_FLAG_SNK_PAC)) {
			supported_contexts = &snk_supported_contexts;
			available_contexts = &snk_available_contexts;
			break;
		}
		return -EINVAL;
#endif /* CONFIG_BT_PAC_SNK */
		return -ENOTSUP;
	case BT_AUDIO_DIR_SOURCE:
#if defined(CONFIG_BT_PAC_SRC)
		if (atomic_test_bit(&pacs.flags, PACS_FLAG_SRC_PAC)) {
			supported_contexts = &src_supported_contexts;
			available_contexts = &src_available_contexts;
			break;
		}
		return -EINVAL;
#endif /* CONFIG_BT_PAC_SRC */
		return -ENOTSUP;
	default:
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_PACS_SUPPORTED_CONTEXT_NOTIFIABLE) || *supported_contexts == 0) {
		return set_supported_contexts(contexts, supported_contexts, available_contexts);
	}

	return -EALREADY;
}

enum bt_audio_context bt_pacs_get_available_contexts(enum bt_audio_dir dir)
{
	switch (dir) {
	case BT_AUDIO_DIR_SINK:
		if (atomic_test_bit(&pacs.flags, PACS_FLAG_SNK_PAC)) {
			return snk_available_contexts;
		}
		return -EINVAL;
	case BT_AUDIO_DIR_SOURCE:
		if (atomic_test_bit(&pacs.flags, PACS_FLAG_SRC_PAC)) {
			return src_available_contexts;
		}
		return -EINVAL;
	}

	return BT_AUDIO_CONTEXT_TYPE_PROHIBITED;
}

enum bt_audio_context bt_pacs_get_available_contexts_for_conn(struct bt_conn *conn,
							      enum bt_audio_dir dir)
{
	CHECKIF(conn == NULL) {
		LOG_ERR("NULL conn");
		return BT_AUDIO_CONTEXT_TYPE_PROHIBITED;
	}

	return pacs_get_available_contexts_for_conn(conn, dir);
}

struct codec_cap_lookup_id_data {
	const struct bt_pac_codec *codec_id;
	const struct bt_audio_codec_cap *codec_cap;
};

static bool codec_lookup_id(const struct bt_pacs_cap *cap, void *user_data)
{
	struct codec_cap_lookup_id_data *data = user_data;

	if (cap->codec_cap->id == data->codec_id->id &&
	    cap->codec_cap->cid == data->codec_id->cid &&
	    cap->codec_cap->vid == data->codec_id->vid) {
		data->codec_cap = cap->codec_cap;

		return false;
	}

	return true;
}

const struct bt_audio_codec_cap *bt_pacs_get_codec_cap(enum bt_audio_dir dir,
						       const struct bt_pac_codec *codec_id)
{
	struct codec_cap_lookup_id_data lookup_data = {
		.codec_id = codec_id,
		.codec_cap = NULL,
	};

	bt_pacs_cap_foreach(dir, codec_lookup_id, &lookup_data);

	return lookup_data.codec_cap;
}

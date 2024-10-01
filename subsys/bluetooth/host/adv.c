/*
 * Copyright (c) 2017-2021 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <sys/types.h>

#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/buf.h>

#include "addr_internal.h"
#include "hci_core.h"
#include "conn_internal.h"
#include "id.h"
#include "scan.h"

#include "common/bt_str.h"

#define LOG_LEVEL CONFIG_BT_HCI_CORE_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_adv);

enum adv_name_type {
	ADV_NAME_TYPE_NONE,
	ADV_NAME_TYPE_AD,
	ADV_NAME_TYPE_SD,
};

struct bt_ad {
	/* Pointer to an LTV structure */
	const struct bt_data *data;
	/* Number of elements in @p data */
	size_t len;
};

struct ad_stream {
	/* ad is a two dimensional array of struct bt_data elements. */
	const struct bt_ad *ad;
	/* The number of struct bt_ad elements. */
	size_t ad_len;

	/* The current index in the array of struct bt_ad elements */
	size_t ad_index;
	/* The current index in the array of ad.data elements */
	size_t data_index;

	/* Current LTV offset contains the data offset in the ad[x].data[y].data value array
	 * The length and type are included in this offset.
	 */
	uint16_t current_ltv_offset;

	/* The remaining size of total ad[i].data[j].data_len + 2 for LTV header */
	size_t remaining_size;
};

static int ad_stream_new(struct ad_stream *stream,
			 const struct bt_ad *ad, size_t ad_len)
{
	(void)memset(stream, 0, sizeof(*stream));
	stream->ad = ad;
	stream->ad_len = ad_len;

	for (size_t i = 0; i < ad_len; i++) {
		for (size_t j = 0; j < ad[i].len; j++) {
			/* LTV length + type + value */
			stream->remaining_size += ad[i].data[j].data_len + 2;

			if (stream->remaining_size > BT_GAP_ADV_MAX_EXT_ADV_DATA_LEN) {
				return -EINVAL;
			}
		}
	}

	return 0;
}

/**
 * @brief Returns true if the current stream is empty.
 *
 * @param stream AD stream, @ref ad_stream_new
 *
 * @returns true if the stream is now empty.
 */
static bool ad_stream_is_empty(const struct ad_stream *stream)
{
	return stream->remaining_size == 0;
}

/**
 * @brief Returns the bt_data structure that is currently being read
 *
 * If the structure has been fully read, the function iterates to the next
 *
 * @param stream AD stream, @ref ad_stream_new
 *
 * @returns The current LTV structure or NULL if there are no left.
 */
static const struct bt_data *ad_stream_current_ltv_update(struct ad_stream *stream)
{
	const struct bt_data *current_ltv = &stream->ad[stream->ad_index].data[stream->data_index];
	const bool done_reading_ltv = (stream->current_ltv_offset == current_ltv->data_len + 2);

	if (done_reading_ltv) {
		stream->current_ltv_offset = 0;

		if (stream->data_index + 1 == stream->ad[stream->ad_index].len) {
			stream->data_index = 0;
			stream->ad_index++;
		} else {
			stream->data_index++;
		}
	}

	if (stream->ad_index == stream->ad_len) {
		return NULL;
	} else {
		return &stream->ad[stream->ad_index].data[stream->data_index];
	}
}

/**
 * @brief Read at max buf_len data from the flattened AD stream.
 *
 * The read data can contain multiple LTV AD structures.
 *
 * @param stream  AD stream, @ref ad_stream_new
 * @param buf     Buffer where the data will be put
 * @param buf_len Buffer length
 *
 * @returns The number of bytes read from the stream written to the provided buffer
 */
static uint8_t ad_stream_read(struct ad_stream *stream, uint8_t *buf, uint8_t buf_len)
{
	uint8_t read_len = 0;

	while (read_len < buf_len) {
		const struct bt_data *current_ltv = ad_stream_current_ltv_update(stream);

		if (!current_ltv) {
			break;
		}

		if (stream->current_ltv_offset == 0) {
			buf[read_len] = current_ltv->data_len + 1;
			stream->current_ltv_offset++;
			read_len++;
		} else if (stream->current_ltv_offset == 1) {
			buf[read_len] = current_ltv->type;
			stream->current_ltv_offset++;
			read_len++;
		} else {
			const size_t remaining_data_len =
					current_ltv->data_len - stream->current_ltv_offset + 2;
			const size_t size_to_copy = MIN(buf_len - read_len, remaining_data_len);

			(void)memcpy(&buf[read_len],
				&current_ltv->data[stream->current_ltv_offset - 2],
				size_to_copy);
			stream->current_ltv_offset += size_to_copy;
			read_len += size_to_copy;
		}
	}

	__ASSERT_NO_MSG(stream->remaining_size >= read_len);
	stream->remaining_size -= read_len;

	return read_len;
}

enum adv_name_type get_adv_name_type(const struct bt_le_ext_adv *adv)
{
	if (atomic_test_bit(adv->flags, BT_ADV_INCLUDE_NAME_SD)) {
		return ADV_NAME_TYPE_SD;
	}

	if (atomic_test_bit(adv->flags, BT_ADV_INCLUDE_NAME_AD)) {
		return ADV_NAME_TYPE_AD;
	}

	return ADV_NAME_TYPE_NONE;
}

enum adv_name_type get_adv_name_type_param(const struct bt_le_adv_param *param)
{
	if (param->options & BT_LE_ADV_OPT_USE_NAME) {
		if (param->options & BT_LE_ADV_OPT_FORCE_NAME_IN_AD) {
			return ADV_NAME_TYPE_AD;
		}

		if ((param->options & BT_LE_ADV_OPT_EXT_ADV) &&
		    !(param->options & BT_LE_ADV_OPT_SCANNABLE)) {
			return ADV_NAME_TYPE_AD;
		}

		return ADV_NAME_TYPE_SD;
	}

	return ADV_NAME_TYPE_NONE;
}

#if defined(CONFIG_BT_EXT_ADV)
static struct bt_le_ext_adv adv_pool[CONFIG_BT_EXT_ADV_MAX_ADV_SET];
#endif /* defined(CONFIG_BT_EXT_ADV) */


#if defined(CONFIG_BT_EXT_ADV)
uint8_t bt_le_ext_adv_get_index(struct bt_le_ext_adv *adv)
{
	ptrdiff_t index = adv - adv_pool;

	__ASSERT(index >= 0 && index < ARRAY_SIZE(adv_pool),
		 "Invalid bt_adv pointer");
	return (uint8_t)index;
}

static struct bt_le_ext_adv *adv_new(void)
{
	struct bt_le_ext_adv *adv = NULL;
	int i;

	for (i = 0; i < ARRAY_SIZE(adv_pool); i++) {
		if (!atomic_test_bit(adv_pool[i].flags, BT_ADV_CREATED)) {
			adv = &adv_pool[i];
			break;
		}
	}

	if (!adv) {
		return NULL;
	}

	(void)memset(adv, 0, sizeof(*adv));
	atomic_set_bit(adv_pool[i].flags, BT_ADV_CREATED);
	adv->handle = i;

	return adv;
}

static void adv_delete(struct bt_le_ext_adv *adv)
{
	atomic_clear_bit(adv->flags, BT_ADV_CREATED);
}

#if defined(CONFIG_BT_BROADCASTER)
static struct bt_le_ext_adv *bt_adv_lookup_handle(uint8_t handle)
{
	if (handle < ARRAY_SIZE(adv_pool) &&
	    atomic_test_bit(adv_pool[handle].flags, BT_ADV_CREATED)) {
		return &adv_pool[handle];
	}

	return NULL;
}
#endif /* CONFIG_BT_BROADCASTER */
#endif /* defined(CONFIG_BT_EXT_ADV) */

void bt_le_ext_adv_foreach(void (*func)(struct bt_le_ext_adv *adv, void *data),
			   void *data)
{
#if defined(CONFIG_BT_EXT_ADV)
	for (size_t i = 0; i < ARRAY_SIZE(adv_pool); i++) {
		if (atomic_test_bit(adv_pool[i].flags, BT_ADV_CREATED)) {
			func(&adv_pool[i], data);
		}
	}
#else
	func(&bt_dev.adv, data);
#endif /* defined(CONFIG_BT_EXT_ADV) */
}

void bt_adv_reset_adv_pool(void)
{
#if defined(CONFIG_BT_EXT_ADV)
	(void)memset(&adv_pool, 0, sizeof(adv_pool));
#endif /* defined(CONFIG_BT_EXT_ADV) */

	(void)memset(&bt_dev.adv, 0, sizeof(bt_dev.adv));
}

static struct bt_le_ext_adv *adv_get_legacy(void)
{
#if defined(CONFIG_BT_EXT_ADV)
	if (bt_dev.adv) {
		return bt_dev.adv;
	}

	bt_dev.adv = adv_new();
	return bt_dev.adv;
#else
	return &bt_dev.adv;
#endif
}

void bt_le_adv_delete_legacy(void)
{
#if defined(CONFIG_BT_EXT_ADV)
	if (bt_dev.adv) {
		atomic_clear_bit(bt_dev.adv->flags, BT_ADV_CREATED);
		bt_dev.adv = NULL;
	}
#endif
}

struct bt_le_ext_adv *bt_le_adv_lookup_legacy(void)
{
#if defined(CONFIG_BT_EXT_ADV)
	return bt_dev.adv;
#else
	return &bt_dev.adv;
#endif
}

int bt_le_adv_set_enable_legacy(struct bt_le_ext_adv *adv, bool enable)
{
	struct net_buf *buf;
	struct bt_hci_cmd_state_set state;
	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_ADV_ENABLE, 1);
	if (!buf) {
		return -ENOBUFS;
	}

	if (enable) {
		net_buf_add_u8(buf, BT_HCI_LE_ADV_ENABLE);
	} else {
		net_buf_add_u8(buf, BT_HCI_LE_ADV_DISABLE);
	}

	bt_hci_cmd_state_set_init(buf, &state, adv->flags, BT_ADV_ENABLED, enable);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_ADV_ENABLE, buf, NULL);
	if (err) {
		return err;
	}

	return 0;
}

int bt_le_adv_set_enable_ext(struct bt_le_ext_adv *adv,
			 bool enable,
			 const struct bt_le_ext_adv_start_param *param)
{
	struct net_buf *buf;
	struct bt_hci_cmd_state_set state;
	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_EXT_ADV_ENABLE, 6);
	if (!buf) {
		return -ENOBUFS;
	}

	if (enable) {
		net_buf_add_u8(buf, BT_HCI_LE_ADV_ENABLE);
	} else {
		net_buf_add_u8(buf, BT_HCI_LE_ADV_DISABLE);
	}

	net_buf_add_u8(buf, 1);

	net_buf_add_u8(buf, adv->handle);
	net_buf_add_le16(buf, param ? param->timeout : 0);
	net_buf_add_u8(buf, param ? param->num_events : 0);

	bt_hci_cmd_state_set_init(buf, &state, adv->flags, BT_ADV_ENABLED, enable);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_EXT_ADV_ENABLE, buf, NULL);
	if (err) {
		return err;
	}

	return 0;
}

int bt_le_adv_set_enable(struct bt_le_ext_adv *adv, bool enable)
{
	if (IS_ENABLED(CONFIG_BT_EXT_ADV) &&
	    BT_DEV_FEAT_LE_EXT_ADV(bt_dev.le.features)) {
		return bt_le_adv_set_enable_ext(adv, enable, NULL);
	}

	return bt_le_adv_set_enable_legacy(adv, enable);
}

static bool valid_adv_ext_param(const struct bt_le_adv_param *param)
{
	if (IS_ENABLED(CONFIG_BT_EXT_ADV) &&
	    BT_DEV_FEAT_LE_EXT_ADV(bt_dev.le.features)) {
		if (param->peer &&
		    !(param->options & BT_LE_ADV_OPT_EXT_ADV) &&
		    !(param->options & BT_LE_ADV_OPT_CONNECTABLE)) {
			/* Cannot do directed non-connectable advertising
			 * without extended advertising.
			 */
			return false;
		}

		if (param->peer &&
		    (param->options & BT_LE_ADV_OPT_EXT_ADV) &&
		    !(param->options & BT_LE_ADV_OPT_DIR_MODE_LOW_DUTY)) {
			/* High duty cycle directed connectable advertising
			 * shall not be used with Extended Advertising.
			 */
			return false;
		}

		if (!(param->options & BT_LE_ADV_OPT_EXT_ADV) &&
		    param->options & (BT_LE_ADV_OPT_EXT_ADV |
				      BT_LE_ADV_OPT_NO_2M |
				      BT_LE_ADV_OPT_CODED |
				      BT_LE_ADV_OPT_ANONYMOUS |
				      BT_LE_ADV_OPT_USE_TX_POWER)) {
			/* Extended options require extended advertising. */
			return false;
		}

		if ((param->options & BT_LE_ADV_OPT_EXT_ADV) &&
		    (param->options & BT_LE_ADV_OPT_SCANNABLE) &&
		    (param->options & BT_LE_ADV_OPT_FORCE_NAME_IN_AD)) {
			/* Advertising data is not permitted for an extended
			 * scannable advertiser.
			 */
			return false;
		}
	}

	if (IS_ENABLED(CONFIG_BT_PRIVACY) &&
	    param->peer &&
	    (param->options & BT_LE_ADV_OPT_USE_IDENTITY) &&
	    (param->options & BT_LE_ADV_OPT_DIR_ADDR_RPA)) {
		/* own addr type used for both RPAs in directed advertising. */
		return false;
	}

	if (param->id >= bt_dev.id_count ||
	    bt_addr_le_eq(&bt_dev.id_addr[param->id], BT_ADDR_LE_ANY)) {
		return false;
	}

	if (!(param->options & BT_LE_ADV_OPT_CONNECTABLE)) {
		/*
		 * BT Core 4.2 [Vol 2, Part E, 7.8.5]
		 * The Advertising_Interval_Min and Advertising_Interval_Max
		 * shall not be set to less than 0x00A0 (100 ms) if the
		 * Advertising_Type is set to ADV_SCAN_IND or ADV_NONCONN_IND.
		 */
		if (bt_dev.hci_version < BT_HCI_VERSION_5_0 &&
		    param->interval_min < 0x00a0) {
			return false;
		}
	}

	if ((param->options & (BT_LE_ADV_OPT_DIR_MODE_LOW_DUTY |
			       BT_LE_ADV_OPT_DIR_ADDR_RPA)) &&
	    !param->peer) {
		return false;
	}

	if ((param->options & BT_LE_ADV_OPT_DIR_MODE_LOW_DUTY) ||
	    !param->peer) {
		if (param->interval_min > param->interval_max ||
		    param->interval_min < 0x0020 ||
		    param->interval_max > 0x4000) {
			return false;
		}
	}

	if ((param->options & BT_LE_ADV_OPT_DISABLE_CHAN_37) &&
	    (param->options & BT_LE_ADV_OPT_DISABLE_CHAN_38) &&
	    (param->options & BT_LE_ADV_OPT_DISABLE_CHAN_39)) {
		return false;
	}

	return true;
}

static bool valid_adv_param(const struct bt_le_adv_param *param)
{
	if (param->options & BT_LE_ADV_OPT_EXT_ADV) {
		return false;
	}

	if (param->peer && !(param->options & BT_LE_ADV_OPT_CONNECTABLE)) {
		return false;
	}

	return valid_adv_ext_param(param);
}

static int set_data_add_complete(uint8_t *set_data, uint8_t set_data_len_max,
			const struct bt_ad *ad, size_t ad_len, uint8_t *data_len)
{
	uint8_t set_data_len = 0;

	for (size_t i = 0; i < ad_len; i++) {
		const struct bt_data *data = ad[i].data;

		for (size_t j = 0; j < ad[i].len; j++) {
			size_t len = data[j].data_len;
			uint8_t type = data[j].type;

			/* Check if ad fit in the remaining buffer */
			if ((set_data_len + len + 2) > set_data_len_max) {
				ssize_t shortened_len = set_data_len_max -
							(set_data_len + 2);

				if (!(type == BT_DATA_NAME_COMPLETE &&
				      shortened_len > 0)) {
					LOG_ERR("Too big advertising data");
					return -EINVAL;
				}

				type = BT_DATA_NAME_SHORTENED;
				len = shortened_len;
			}

			set_data[set_data_len++] = len + 1;
			set_data[set_data_len++] = type;

			memcpy(&set_data[set_data_len], data[j].data, len);
			set_data_len += len;
		}
	}

	*data_len = set_data_len;
	return 0;
}

static int hci_set_ad(uint16_t hci_op, const struct bt_ad *ad, size_t ad_len)
{
	struct bt_hci_cp_le_set_adv_data *set_data;
	struct net_buf *buf;
	int err;

	buf = bt_hci_cmd_create(hci_op, sizeof(*set_data));
	if (!buf) {
		return -ENOBUFS;
	}

	set_data = net_buf_add(buf, sizeof(*set_data));
	(void)memset(set_data, 0, sizeof(*set_data));

	err = set_data_add_complete(set_data->data, BT_GAP_ADV_MAX_ADV_DATA_LEN,
				    ad, ad_len, &set_data->len);
	if (err) {
		net_buf_unref(buf);
		return err;
	}

	return bt_hci_cmd_send_sync(hci_op, buf, NULL);
}

static int hci_set_adv_ext_complete(struct bt_le_ext_adv *adv, uint16_t hci_op,
				    size_t total_data_len, const struct bt_ad *ad, size_t ad_len)
{
	struct bt_hci_cp_le_set_ext_adv_data *set_data;
	struct net_buf *buf;
	size_t cmd_size;
	int err;

	/* Provide the opportunity to truncate the complete name */
	if (!atomic_test_bit(adv->flags, BT_ADV_EXT_ADV) &&
	    total_data_len > BT_GAP_ADV_MAX_ADV_DATA_LEN) {
		total_data_len = BT_GAP_ADV_MAX_ADV_DATA_LEN;
	}

	cmd_size = sizeof(*set_data) + total_data_len;

	buf = bt_hci_cmd_create(hci_op, cmd_size);
	if (!buf) {
		return -ENOBUFS;
	}

	set_data = net_buf_add(buf, cmd_size);
	(void)memset(set_data, 0, cmd_size);

	err = set_data_add_complete(set_data->data, total_data_len,
				    ad, ad_len, &set_data->len);
	if (err) {
		net_buf_unref(buf);
		return err;
	}

	set_data->handle = adv->handle;
	set_data->op = BT_HCI_LE_EXT_ADV_OP_COMPLETE_DATA;
	set_data->frag_pref = BT_HCI_LE_EXT_ADV_FRAG_DISABLED;

	return bt_hci_cmd_send_sync(hci_op, buf, NULL);
}

static int hci_set_adv_ext_fragmented(struct bt_le_ext_adv *adv, uint16_t hci_op,
				      const struct bt_ad *ad, size_t ad_len)
{
	int err;
	struct ad_stream stream;
	bool is_first_iteration = true;

	err = ad_stream_new(&stream, ad, ad_len);
	if (err) {
		return err;
	}

	while (!ad_stream_is_empty(&stream)) {
		struct bt_hci_cp_le_set_ext_adv_data *set_data;
		struct net_buf *buf;
		const size_t data_len = MIN(BT_HCI_LE_EXT_ADV_FRAG_MAX_LEN, stream.remaining_size);
		const size_t cmd_size = sizeof(*set_data) + data_len;

		buf = bt_hci_cmd_create(hci_op, cmd_size);
		if (!buf) {
			return -ENOBUFS;
		}

		set_data = net_buf_add(buf, cmd_size);

		set_data->handle = adv->handle;
		set_data->frag_pref = BT_HCI_LE_EXT_ADV_FRAG_ENABLED;
		set_data->len = ad_stream_read(&stream, set_data->data, data_len);

		if (is_first_iteration && ad_stream_is_empty(&stream)) {
			set_data->op = BT_HCI_LE_EXT_ADV_OP_COMPLETE_DATA;
		} else if (is_first_iteration) {
			set_data->op = BT_HCI_LE_EXT_ADV_OP_FIRST_FRAG;
		} else if (ad_stream_is_empty(&stream)) {
			set_data->op = BT_HCI_LE_EXT_ADV_OP_LAST_FRAG;
		} else {
			set_data->op = BT_HCI_LE_EXT_ADV_OP_INTERM_FRAG;
		}

		err = bt_hci_cmd_send_sync(hci_op, buf, NULL);
		if (err) {
			return err;
		}

		is_first_iteration = false;
	}

	return 0;
}

static int hci_set_ad_ext(struct bt_le_ext_adv *adv, uint16_t hci_op,
			  const struct bt_ad *ad, size_t ad_len)
{
	size_t total_len_bytes = 0;

	for (size_t i = 0; i < ad_len; i++) {
		for (size_t j = 0; j < ad[i].len; j++) {
			total_len_bytes += ad[i].data[j].data_len + 2;
		}
	}

	if ((total_len_bytes > BT_HCI_LE_EXT_ADV_FRAG_MAX_LEN) &&
	    atomic_test_bit(adv->flags, BT_ADV_ENABLED)) {
		/* It is not allowed to set advertising data in multiple
		 * operations while the advertiser is running.
		 */
		return -EAGAIN;
	}

	if (total_len_bytes > bt_dev.le.max_adv_data_len) {
		LOG_WRN("adv or scan rsp data too large (%d > max %d)", total_len_bytes,
			bt_dev.le.max_adv_data_len);
		return -EDOM;
	}

	if (total_len_bytes <= BT_HCI_LE_EXT_ADV_FRAG_MAX_LEN) {
		/* If possible, set all data at once.
		 * This allows us to update advertising data while advertising.
		 */
		return hci_set_adv_ext_complete(adv, hci_op, total_len_bytes, ad, ad_len);
	} else {
		return hci_set_adv_ext_fragmented(adv, hci_op, ad, ad_len);
	}

	return 0;
}

static int set_ad(struct bt_le_ext_adv *adv, const struct bt_ad *ad,
		  size_t ad_len)
{
	if (IS_ENABLED(CONFIG_BT_EXT_ADV) &&
	    BT_DEV_FEAT_LE_EXT_ADV(bt_dev.le.features)) {
		return hci_set_ad_ext(adv, BT_HCI_OP_LE_SET_EXT_ADV_DATA,
				      ad, ad_len);
	}

	return hci_set_ad(BT_HCI_OP_LE_SET_ADV_DATA, ad, ad_len);
}

static int set_sd(struct bt_le_ext_adv *adv, const struct bt_ad *sd,
		  size_t sd_len)
{
	if (IS_ENABLED(CONFIG_BT_EXT_ADV) &&
	    BT_DEV_FEAT_LE_EXT_ADV(bt_dev.le.features)) {
		return hci_set_ad_ext(adv, BT_HCI_OP_LE_SET_EXT_SCAN_RSP_DATA,
				      sd, sd_len);
	}

	return hci_set_ad(BT_HCI_OP_LE_SET_SCAN_RSP_DATA, sd, sd_len);
}

#if defined(CONFIG_BT_PER_ADV)
static int hci_set_per_adv_data(const struct bt_le_ext_adv *adv,
				const struct bt_data *ad, size_t ad_len)
{
	int err;
	struct ad_stream stream;
	struct bt_ad d = { .data = ad, .len = ad_len };
	bool is_first_iteration = true;

	err = ad_stream_new(&stream, &d, 1);
	if (err) {
		return err;
	}

	while (!ad_stream_is_empty(&stream)) {
		struct bt_hci_cp_le_set_per_adv_data *set_data;
		struct net_buf *buf;
		const size_t data_len = MIN(BT_HCI_LE_PER_ADV_FRAG_MAX_LEN, stream.remaining_size);
		const size_t cmd_size = sizeof(*set_data) + data_len;

		buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_PER_ADV_DATA, cmd_size);
		if (!buf) {
			return -ENOBUFS;
		}

		set_data = net_buf_add(buf, cmd_size);
		(void)memset(set_data, 0, cmd_size);

		set_data->handle = adv->handle;
		set_data->len = ad_stream_read(&stream, set_data->data, data_len);

		if (is_first_iteration && ad_stream_is_empty(&stream)) {
			set_data->op = BT_HCI_LE_EXT_ADV_OP_COMPLETE_DATA;
		} else if (is_first_iteration) {
			set_data->op = BT_HCI_LE_EXT_ADV_OP_FIRST_FRAG;
		} else if (ad_stream_is_empty(&stream)) {
			set_data->op = BT_HCI_LE_EXT_ADV_OP_LAST_FRAG;
		} else {
			set_data->op = BT_HCI_LE_EXT_ADV_OP_INTERM_FRAG;
		}

		err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_PER_ADV_DATA, buf, NULL);
		if (err) {
			return err;
		}

		is_first_iteration = false;
	}

	return 0;
}
#endif /* CONFIG_BT_PER_ADV */

static inline bool ad_has_name(const struct bt_data *ad, size_t ad_len)
{
	size_t i;

	for (i = 0; i < ad_len; i++) {
		if (ad[i].type == BT_DATA_NAME_COMPLETE ||
		    ad[i].type == BT_DATA_NAME_SHORTENED) {
			return true;
		}
	}

	return false;
}

static bool ad_is_limited(const struct bt_data *ad, size_t ad_len)
{
	size_t i;

	for (i = 0; i < ad_len; i++) {
		if (ad[i].type == BT_DATA_FLAGS &&
		    ad[i].data_len == sizeof(uint8_t) &&
		    ad[i].data != NULL) {
			if (ad[i].data[0] & BT_LE_AD_LIMITED) {
				return true;
			}
		}
	}

	return false;
}

static int le_adv_update(struct bt_le_ext_adv *adv,
			 const struct bt_data *ad, size_t ad_len,
			 const struct bt_data *sd, size_t sd_len,
			 bool ext_adv, bool scannable,
			 enum adv_name_type name_type)
{
	struct bt_ad d[2] = {};
	struct bt_data data;
	size_t d_len;
	int err;

	if (name_type != ADV_NAME_TYPE_NONE) {
		const char *name = bt_get_name();

		if ((ad && ad_has_name(ad, ad_len)) ||
		    (sd && ad_has_name(sd, sd_len))) {
			/* Cannot use name if name is already set */
			return -EINVAL;
		}

		data = (struct bt_data)BT_DATA(
			BT_DATA_NAME_COMPLETE,
			name, strlen(name));
	}

	if (!(ext_adv && scannable)) {
		d_len = 1;
		d[0].data = ad;
		d[0].len = ad_len;

		if (name_type == ADV_NAME_TYPE_AD) {
			d[1].data = &data;
			d[1].len = 1;
			d_len = 2;
		}

		err = set_ad(adv, d, d_len);
		if (err) {
			return err;
		}
	}

	if (scannable) {
		d_len = 1;
		d[0].data = sd;
		d[0].len = sd_len;

		if (name_type == ADV_NAME_TYPE_SD) {
			d[1].data = &data;
			d[1].len = 1;
			d_len = 2;
		}

		err = set_sd(adv, d, d_len);
		if (err) {
			return err;
		}
	}

	atomic_set_bit(adv->flags, BT_ADV_DATA_SET);
	return 0;
}

int bt_le_adv_update_data(const struct bt_data *ad, size_t ad_len,
			  const struct bt_data *sd, size_t sd_len)
{
	struct bt_le_ext_adv *adv = bt_le_adv_lookup_legacy();
	bool scannable;

	if (!adv) {
		return -EINVAL;
	}

	if (!atomic_test_bit(adv->flags, BT_ADV_ENABLED)) {
		return -EAGAIN;
	}

	scannable = atomic_test_bit(adv->flags, BT_ADV_SCANNABLE);

	return le_adv_update(adv, ad, ad_len, sd, sd_len, false, scannable,
			     get_adv_name_type(adv));
}

static uint8_t get_filter_policy(uint32_t options)
{
	if (!IS_ENABLED(CONFIG_BT_FILTER_ACCEPT_LIST)) {
		return BT_LE_ADV_FP_NO_FILTER;
	} else if ((options & BT_LE_ADV_OPT_FILTER_SCAN_REQ) &&
		   (options & BT_LE_ADV_OPT_FILTER_CONN)) {
		return BT_LE_ADV_FP_FILTER_BOTH;
	} else if (options & BT_LE_ADV_OPT_FILTER_SCAN_REQ) {
		return BT_LE_ADV_FP_FILTER_SCAN_REQ;
	} else if (options & BT_LE_ADV_OPT_FILTER_CONN) {
		return BT_LE_ADV_FP_FILTER_CONN_IND;
	} else {
		return BT_LE_ADV_FP_NO_FILTER;
	}
}

static uint8_t get_adv_channel_map(uint32_t options)
{
	uint8_t channel_map = 0x07;

	if (options & BT_LE_ADV_OPT_DISABLE_CHAN_37) {
		channel_map &= ~0x01;
	}

	if (options & BT_LE_ADV_OPT_DISABLE_CHAN_38) {
		channel_map &= ~0x02;
	}

	if (options & BT_LE_ADV_OPT_DISABLE_CHAN_39) {
		channel_map &= ~0x04;
	}

	return channel_map;
}

static inline bool adv_is_directed(const struct bt_le_ext_adv *adv)
{
	/* The advertiser is assumed to be directed when the peer address has
	 * been set.
	 */
	return !bt_addr_le_eq(&adv->target_addr, BT_ADDR_LE_ANY);
}

static int le_adv_start_add_conn(const struct bt_le_ext_adv *adv,
				 struct bt_conn **out_conn)
{
	struct bt_conn *conn;

	bt_dev.adv_conn_id = adv->id;

	if (!adv_is_directed(adv)) {
		/* Undirected advertising */
		conn = bt_conn_add_le(adv->id, BT_ADDR_LE_NONE);
		if (!conn) {
			return -ENOMEM;
		}

		bt_conn_set_state(conn, BT_CONN_ADV_CONNECTABLE);
		*out_conn = conn;
		return 0;
	}

	if (bt_conn_exists_le(adv->id, &adv->target_addr)) {
		return -EINVAL;
	}

	conn = bt_conn_add_le(adv->id, &adv->target_addr);
	if (!conn) {
		return -ENOMEM;
	}

	bt_conn_set_state(conn, BT_CONN_ADV_DIR_CONNECTABLE);
	*out_conn = conn;
	return 0;
}

static void le_adv_stop_free_conn(const struct bt_le_ext_adv *adv, uint8_t status)
{
	struct bt_conn *conn;

	if (!adv_is_directed(adv)) {
		conn = bt_conn_lookup_state_le(adv->id, BT_ADDR_LE_NONE,
					       BT_CONN_ADV_CONNECTABLE);
	} else {
		conn = bt_conn_lookup_state_le(adv->id, &adv->target_addr,
					       BT_CONN_ADV_DIR_CONNECTABLE);
	}

	if (conn) {
		conn->err = status;
		bt_conn_set_state(conn, BT_CONN_DISCONNECTED);
		bt_conn_unref(conn);
	}
}

int bt_le_adv_start_legacy(struct bt_le_ext_adv *adv,
			   const struct bt_le_adv_param *param,
			   const struct bt_data *ad, size_t ad_len,
			   const struct bt_data *sd, size_t sd_len)
{
	struct bt_hci_cp_le_set_adv_param set_param;
	struct bt_conn *conn = NULL;
	struct net_buf *buf;
	bool dir_adv = (param->peer != NULL), scannable = false;
	enum adv_name_type name_type;

	int err;

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_READY)) {
		return -EAGAIN;
	}

	if (!valid_adv_param(param)) {
		return -EINVAL;
	}

	if (!bt_id_adv_random_addr_check(param)) {
		return -EINVAL;
	}

	if (atomic_test_bit(adv->flags, BT_ADV_ENABLED)) {
		return -EALREADY;
	}

	(void)memset(&set_param, 0, sizeof(set_param));

	set_param.min_interval = sys_cpu_to_le16(param->interval_min);
	set_param.max_interval = sys_cpu_to_le16(param->interval_max);
	set_param.channel_map  = get_adv_channel_map(param->options);
	set_param.filter_policy = get_filter_policy(param->options);

	atomic_clear_bit(bt_dev.flags, BT_DEV_RPA_VALID);

	adv->id = param->id;
	bt_dev.adv_conn_id = adv->id;

	err = bt_id_set_adv_own_addr(adv, param->options, dir_adv,
				     &set_param.own_addr_type);
	if (err) {
		return err;
	}

	if (dir_adv) {
		bt_addr_le_copy(&adv->target_addr, param->peer);
	} else {
		bt_addr_le_copy(&adv->target_addr, BT_ADDR_LE_ANY);
	}

	name_type = get_adv_name_type_param(param);

	if (param->options & BT_LE_ADV_OPT_CONNECTABLE) {
		if (dir_adv) {
			if (param->options & BT_LE_ADV_OPT_DIR_MODE_LOW_DUTY) {
				set_param.type = BT_HCI_ADV_DIRECT_IND_LOW_DUTY;
			} else {
				set_param.type = BT_HCI_ADV_DIRECT_IND;
			}

			bt_addr_le_copy(&set_param.direct_addr, param->peer);
		} else {
			scannable = true;
			set_param.type = BT_HCI_ADV_IND;
		}
	} else if ((param->options & BT_LE_ADV_OPT_SCANNABLE) || sd ||
		   (name_type == ADV_NAME_TYPE_SD)) {
		scannable = true;
		set_param.type = BT_HCI_ADV_SCAN_IND;
	} else {
		set_param.type = BT_HCI_ADV_NONCONN_IND;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_ADV_PARAM, sizeof(set_param));
	if (!buf) {
		return -ENOBUFS;
	}

	net_buf_add_mem(buf, &set_param, sizeof(set_param));

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_ADV_PARAM, buf, NULL);
	if (err) {
		return err;
	}

	if (!dir_adv) {
		err = le_adv_update(adv, ad, ad_len, sd, sd_len, false,
				    scannable, name_type);
		if (err) {
			return err;
		}
	}

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL) &&
	    (param->options & BT_LE_ADV_OPT_CONNECTABLE)) {
		err = le_adv_start_add_conn(adv, &conn);
		if (err) {
			if (err == -ENOMEM && !dir_adv &&
			    !(param->options & BT_LE_ADV_OPT_ONE_TIME)) {
				goto set_adv_state;
			}

			return err;
		}
	}

	err = bt_le_adv_set_enable(adv, true);
	if (err) {
		LOG_ERR("Failed to start advertiser");
		if (IS_ENABLED(CONFIG_BT_PERIPHERAL) && conn) {
			bt_conn_set_state(conn, BT_CONN_DISCONNECTED);
			bt_conn_unref(conn);
		}

		return err;
	}

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL) && conn) {
		/* If undirected connectable advertiser we have created a
		 * connection object that we don't yet give to the application.
		 * Since we don't give the application a reference to manage in
		 * this case, we need to release this reference here
		 */
		bt_conn_unref(conn);
	}

set_adv_state:
	atomic_set_bit_to(adv->flags, BT_ADV_PERSIST, !dir_adv &&
			  !(param->options & BT_LE_ADV_OPT_ONE_TIME));

	atomic_set_bit_to(adv->flags, BT_ADV_INCLUDE_NAME_AD,
			  name_type == ADV_NAME_TYPE_AD);

	atomic_set_bit_to(adv->flags, BT_ADV_INCLUDE_NAME_SD,
			  name_type == ADV_NAME_TYPE_SD);

	atomic_set_bit_to(adv->flags, BT_ADV_CONNECTABLE,
			  param->options & BT_LE_ADV_OPT_CONNECTABLE);

	atomic_set_bit_to(adv->flags, BT_ADV_SCANNABLE, scannable);

	atomic_set_bit_to(adv->flags, BT_ADV_USE_IDENTITY,
			  param->options & BT_LE_ADV_OPT_USE_IDENTITY);

	return 0;
}

static int le_ext_adv_param_set(struct bt_le_ext_adv *adv,
				const struct bt_le_adv_param *param,
				bool  has_scan_data)
{
	struct bt_hci_cp_le_set_ext_adv_param *cp;
	bool dir_adv = param->peer != NULL, scannable;
	struct net_buf *buf, *rsp;
	int err;
	enum adv_name_type name_type;
	uint16_t props = 0;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_EXT_ADV_PARAM, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	(void)memset(cp, 0, sizeof(*cp));

	adv->options = param->options;

	err = bt_id_set_adv_own_addr(adv, param->options, dir_adv,
				     &cp->own_addr_type);
	if (err) {
		return err;
	}

	if (dir_adv) {
		bt_addr_le_copy(&adv->target_addr, param->peer);
	} else {
		bt_addr_le_copy(&adv->target_addr, BT_ADDR_LE_ANY);
	}

	name_type = get_adv_name_type_param(param);

	cp->handle = adv->handle;
	sys_put_le24(param->interval_min, cp->prim_min_interval);
	sys_put_le24(param->interval_max, cp->prim_max_interval);
	cp->prim_channel_map = get_adv_channel_map(param->options);
	cp->filter_policy = get_filter_policy(param->options);
	cp->tx_power = BT_HCI_LE_ADV_TX_POWER_NO_PREF;

	cp->prim_adv_phy = BT_HCI_LE_PHY_1M;
	if ((param->options & BT_LE_ADV_OPT_EXT_ADV) &&
	    !(param->options & BT_LE_ADV_OPT_NO_2M)) {
		cp->sec_adv_phy = BT_HCI_LE_PHY_2M;
	} else {
		cp->sec_adv_phy = BT_HCI_LE_PHY_1M;
	}

	if (param->options & BT_LE_ADV_OPT_CODED) {
		cp->prim_adv_phy = BT_HCI_LE_PHY_CODED;
		cp->sec_adv_phy = BT_HCI_LE_PHY_CODED;
	}

	if (!(param->options & BT_LE_ADV_OPT_EXT_ADV)) {
		props |= BT_HCI_LE_ADV_PROP_LEGACY;
	}

	if (param->options & BT_LE_ADV_OPT_USE_TX_POWER) {
		props |= BT_HCI_LE_ADV_PROP_TX_POWER;
	}

	if (param->options & BT_LE_ADV_OPT_ANONYMOUS) {
		props |= BT_HCI_LE_ADV_PROP_ANON;
	}

	if (param->options & BT_LE_ADV_OPT_NOTIFY_SCAN_REQ) {
		cp->scan_req_notify_enable = BT_HCI_LE_ADV_SCAN_REQ_ENABLE;
	}

	if (param->options & BT_LE_ADV_OPT_CONNECTABLE) {
		props |= BT_HCI_LE_ADV_PROP_CONN;
		if (!dir_adv && !(param->options & BT_LE_ADV_OPT_EXT_ADV)) {
			/* When using non-extended adv packets then undirected
			 * advertising has to be scannable as well.
			 * We didn't require this option to be set before, so
			 * it is implicitly set instead in this case.
			 */
			props |= BT_HCI_LE_ADV_PROP_SCAN;
		}
	}

	if ((param->options & BT_LE_ADV_OPT_SCANNABLE) || has_scan_data ||
	    (name_type == ADV_NAME_TYPE_SD)) {
		props |= BT_HCI_LE_ADV_PROP_SCAN;
	}

	scannable = !!(props & BT_HCI_LE_ADV_PROP_SCAN);

	if (dir_adv) {
		props |= BT_HCI_LE_ADV_PROP_DIRECT;
		if (!(param->options & BT_LE_ADV_OPT_DIR_MODE_LOW_DUTY)) {
			props |= BT_HCI_LE_ADV_PROP_HI_DC_CONN;
		}

		bt_addr_le_copy(&cp->peer_addr, param->peer);
	}

	cp->sid = param->sid;

	cp->props = sys_cpu_to_le16(props);
	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_EXT_ADV_PARAM, buf, &rsp);
	if (err) {
		return err;
	}

#if defined(CONFIG_BT_EXT_ADV)
	struct bt_hci_rp_le_set_ext_adv_param *rp = (void *)rsp->data;

	adv->tx_power = rp->tx_power;
#endif /* defined(CONFIG_BT_EXT_ADV) */

	net_buf_unref(rsp);

	atomic_set_bit(adv->flags, BT_ADV_PARAMS_SET);

	if (atomic_test_and_clear_bit(adv->flags, BT_ADV_RANDOM_ADDR_PENDING)) {
		err = bt_id_set_adv_random_addr(adv, &adv->random_addr.a);
		if (err) {
			return err;
		}
	}

	/* Flag only used by bt_le_adv_start API. */
	atomic_set_bit_to(adv->flags, BT_ADV_PERSIST, false);

	atomic_set_bit_to(adv->flags, BT_ADV_INCLUDE_NAME_AD,
			  name_type == ADV_NAME_TYPE_AD);

	atomic_set_bit_to(adv->flags, BT_ADV_INCLUDE_NAME_SD,
			  name_type == ADV_NAME_TYPE_SD);

	atomic_set_bit_to(adv->flags, BT_ADV_CONNECTABLE,
			  param->options & BT_LE_ADV_OPT_CONNECTABLE);

	atomic_set_bit_to(adv->flags, BT_ADV_SCANNABLE, scannable);

	atomic_set_bit_to(adv->flags, BT_ADV_USE_IDENTITY,
			  param->options & BT_LE_ADV_OPT_USE_IDENTITY);

	atomic_set_bit_to(adv->flags, BT_ADV_EXT_ADV,
			  param->options & BT_LE_ADV_OPT_EXT_ADV);

	return 0;
}

int bt_le_adv_start_ext(struct bt_le_ext_adv *adv,
			const struct bt_le_adv_param *param,
			const struct bt_data *ad, size_t ad_len,
			const struct bt_data *sd, size_t sd_len)
{
	struct bt_le_ext_adv_start_param start_param = {
		.timeout = 0,
		.num_events = 0,
	};
	bool dir_adv = (param->peer != NULL);
	struct bt_conn *conn = NULL;
	int err;

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_READY)) {
		return -EAGAIN;
	}

	if (!valid_adv_param(param)) {
		return -EINVAL;
	}

	if (atomic_test_bit(adv->flags, BT_ADV_ENABLED)) {
		return -EALREADY;
	}

	adv->id = param->id;
	err = le_ext_adv_param_set(adv, param, sd != NULL);
	if (err) {
		return err;
	}

	if (!dir_adv) {
		if (IS_ENABLED(CONFIG_BT_EXT_ADV)) {
			err = bt_le_ext_adv_set_data(adv, ad, ad_len, sd, sd_len);
			if (err) {
				return err;
			}
		}
	} else {
		if (!(param->options & BT_LE_ADV_OPT_DIR_MODE_LOW_DUTY)) {
			start_param.timeout =
				BT_GAP_ADV_HIGH_DUTY_CYCLE_MAX_TIMEOUT;
			atomic_set_bit(adv->flags, BT_ADV_LIMITED);
		}
	}

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL) &&
	    (param->options & BT_LE_ADV_OPT_CONNECTABLE)) {
		err = le_adv_start_add_conn(adv, &conn);
		if (err) {
			if (err == -ENOMEM && !dir_adv &&
			    !(param->options & BT_LE_ADV_OPT_ONE_TIME)) {
				goto set_adv_state;
			}

			return err;
		}
	}

	err = bt_le_adv_set_enable_ext(adv, true, &start_param);
	if (err) {
		LOG_ERR("Failed to start advertiser");
		if (IS_ENABLED(CONFIG_BT_PERIPHERAL) && conn) {
			bt_conn_set_state(conn, BT_CONN_DISCONNECTED);
			bt_conn_unref(conn);
		}

		return err;
	}

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL) && conn) {
		/* If undirected connectable advertiser we have created a
		 * connection object that we don't yet give to the application.
		 * Since we don't give the application a reference to manage in
		 * this case, we need to release this reference here
		 */
		bt_conn_unref(conn);
	}

set_adv_state:
	/* Flag always set to false by le_ext_adv_param_set */
	atomic_set_bit_to(adv->flags, BT_ADV_PERSIST, !dir_adv &&
			  !(param->options & BT_LE_ADV_OPT_ONE_TIME));

	return 0;
}

static void adv_timeout(struct k_work *work);

int bt_le_lim_adv_cancel_timeout(struct bt_le_ext_adv *adv)
{
	return k_work_cancel_delayable(&adv->lim_adv_timeout_work);
}

int bt_le_adv_start(const struct bt_le_adv_param *param,
		    const struct bt_data *ad, size_t ad_len,
		    const struct bt_data *sd, size_t sd_len)
{
	struct bt_le_ext_adv *adv = adv_get_legacy();
	int err;

	if (!adv) {
		return -ENOMEM;
	}

	if (IS_ENABLED(CONFIG_BT_EXT_ADV) &&
	    BT_DEV_FEAT_LE_EXT_ADV(bt_dev.le.features)) {
		err = bt_le_adv_start_ext(adv, param, ad, ad_len, sd, sd_len);
	} else {
		err = bt_le_adv_start_legacy(adv, param, ad, ad_len, sd, sd_len);
	}

	if (err) {
		bt_le_adv_delete_legacy();
	}

	if (ad_is_limited(ad, ad_len)) {
		k_work_init_delayable(&adv->lim_adv_timeout_work, adv_timeout);
		k_work_reschedule(&adv->lim_adv_timeout_work,
				  K_SECONDS(CONFIG_BT_LIM_ADV_TIMEOUT));
	}

	return err;
}

int bt_le_adv_stop(void)
{
	struct bt_le_ext_adv *adv = bt_le_adv_lookup_legacy();
	int err;

	if (!adv) {
		LOG_ERR("No valid legacy adv");
		return 0;
	}

	(void)bt_le_lim_adv_cancel_timeout(adv);

	/* Make sure advertising is not re-enabled later even if it's not
	 * currently enabled (i.e. BT_DEV_ADVERTISING is not set).
	 */
	atomic_clear_bit(adv->flags, BT_ADV_PERSIST);

	if (!atomic_test_bit(adv->flags, BT_ADV_ENABLED)) {
		/* Legacy advertiser exists, but is not currently advertising.
		 * This happens when keep advertising behavior is active but
		 * no conn object is available to do connectable advertising.
		 */
		bt_le_adv_delete_legacy();
		return 0;
	}

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL) &&
	    atomic_test_bit(adv->flags, BT_ADV_CONNECTABLE)) {
		le_adv_stop_free_conn(adv, 0);
	}

	if (IS_ENABLED(CONFIG_BT_EXT_ADV) &&
	    BT_DEV_FEAT_LE_EXT_ADV(bt_dev.le.features)) {
		err = bt_le_adv_set_enable_ext(adv, false, NULL);
		if (err) {
			return err;
		}
	} else {
		err = bt_le_adv_set_enable_legacy(adv, false);
		if (err) {
			return err;
		}
	}

	bt_le_adv_delete_legacy();

#if defined(CONFIG_BT_OBSERVER)
	if (!(IS_ENABLED(CONFIG_BT_EXT_ADV) &&
	      BT_DEV_FEAT_LE_EXT_ADV(bt_dev.le.features)) &&
	    !IS_ENABLED(CONFIG_BT_PRIVACY) &&
	    !IS_ENABLED(CONFIG_BT_SCAN_WITH_IDENTITY)) {
		/* If scan is ongoing set back NRPA */
		if (atomic_test_bit(bt_dev.flags, BT_DEV_SCANNING)) {
			bt_le_scan_set_enable(BT_HCI_LE_SCAN_DISABLE);
			bt_id_set_private_addr(BT_ID_DEFAULT);
			bt_le_scan_set_enable(BT_HCI_LE_SCAN_ENABLE);
		}
	}
#endif /* defined(CONFIG_BT_OBSERVER) */

	return 0;
}

#if defined(CONFIG_BT_PERIPHERAL)
static uint32_t adv_get_options(const struct bt_le_ext_adv *adv)
{
	uint32_t options = 0;

	if (!atomic_test_bit(adv->flags, BT_ADV_PERSIST)) {
		options |= BT_LE_ADV_OPT_ONE_TIME;
	}

	if (atomic_test_bit(adv->flags, BT_ADV_CONNECTABLE)) {
		options |= BT_LE_ADV_OPT_CONNECTABLE;
	}

	if (atomic_test_bit(adv->flags, BT_ADV_USE_IDENTITY)) {
		options |= BT_LE_ADV_OPT_USE_IDENTITY;
	}

	return options;
}

void bt_le_adv_resume(void)
{
	struct bt_le_ext_adv *adv = bt_le_adv_lookup_legacy();
	struct bt_conn *conn;
	bool persist_paused = false;
	int err;

	if (!adv) {
		LOG_DBG("No valid legacy adv");
		return;
	}

	if (!(atomic_test_bit(adv->flags, BT_ADV_PERSIST) &&
	      !atomic_test_bit(adv->flags, BT_ADV_ENABLED))) {
		return;
	}

	if (!atomic_test_bit(adv->flags, BT_ADV_CONNECTABLE)) {
		return;
	}

	err = le_adv_start_add_conn(adv, &conn);
	if (err) {
		LOG_DBG("Host cannot resume connectable advertising (%d)", err);
		return;
	}

	LOG_DBG("Resuming connectable advertising");

	if (IS_ENABLED(CONFIG_BT_PRIVACY) &&
	    !atomic_test_bit(adv->flags, BT_ADV_USE_IDENTITY)) {
		bt_id_set_adv_private_addr(adv);
	} else {
		uint8_t own_addr_type;
		bool dir_adv = adv_is_directed(adv);
		uint32_t options = adv_get_options(adv);

		/* Always set the address. Don't assume it has not changed. */
		err = bt_id_set_adv_own_addr(adv, options, dir_adv, &own_addr_type);
		if (err) {
			LOG_ERR("Controller cannot resume connectable advertising (%d)", err);
			return;
		}
	}

	err = bt_le_adv_set_enable(adv, true);
	if (err) {
		LOG_DBG("Controller cannot resume connectable advertising (%d)", err);
		bt_conn_set_state(conn, BT_CONN_DISCONNECTED);

		/* Temporarily clear persist flag to avoid recursion in
		 * bt_conn_unref if the flag is still set.
		 */
		persist_paused = atomic_test_and_clear_bit(adv->flags,
							   BT_ADV_PERSIST);
	}

	/* Since we don't give the application a reference to manage in
	 * this case, we need to release this reference here.
	 */
	bt_conn_unref(conn);
	if (persist_paused) {
		atomic_set_bit(adv->flags, BT_ADV_PERSIST);
	}
}
#endif /* defined(CONFIG_BT_PERIPHERAL) */

#if defined(CONFIG_BT_EXT_ADV)
int bt_le_ext_adv_get_info(const struct bt_le_ext_adv *adv,
			   struct bt_le_ext_adv_info *info)
{
	info->id = adv->id;
	info->tx_power = adv->tx_power;
	info->addr = &adv->random_addr;

	return 0;
}

int bt_le_ext_adv_create(const struct bt_le_adv_param *param,
			 const struct bt_le_ext_adv_cb *cb,
			 struct bt_le_ext_adv **out_adv)
{
	struct bt_le_ext_adv *adv;
	int err;

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_READY)) {
		return -EAGAIN;
	}

	if (!valid_adv_ext_param(param)) {
		return -EINVAL;
	}

	adv = adv_new();
	if (!adv) {
		return -ENOMEM;
	}

	adv->id = param->id;
	adv->cb = cb;

	err = le_ext_adv_param_set(adv, param, false);
	if (err) {
		adv_delete(adv);
		return err;
	}

	*out_adv = adv;
	return 0;
}

int bt_le_ext_adv_update_param(struct bt_le_ext_adv *adv,
			       const struct bt_le_adv_param *param)
{
	if (!valid_adv_ext_param(param)) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_PER_ADV) &&
	    atomic_test_bit(adv->flags, BT_PER_ADV_PARAMS_SET)) {
		/* If params for per adv has been set, do not allow setting
		 * connectable, scanable or use legacy adv
		 */
		if (param->options & BT_LE_ADV_OPT_CONNECTABLE ||
		    param->options & BT_LE_ADV_OPT_SCANNABLE ||
		    !(param->options & BT_LE_ADV_OPT_EXT_ADV) ||
		    param->options & BT_LE_ADV_OPT_ANONYMOUS) {
			return -EINVAL;
		}
	}

	if (atomic_test_bit(adv->flags, BT_ADV_ENABLED)) {
		return -EINVAL;
	}

	if (param->id != adv->id) {
		atomic_clear_bit(adv->flags, BT_ADV_RPA_VALID);
	}

	return le_ext_adv_param_set(adv, param, false);
}

int bt_le_ext_adv_start(struct bt_le_ext_adv *adv,
			const struct bt_le_ext_adv_start_param *param)
{
	struct bt_conn *conn = NULL;
	int err;

	if (atomic_test_bit(adv->flags, BT_ADV_ENABLED)) {
		return -EALREADY;
	}

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL) &&
	    atomic_test_bit(adv->flags, BT_ADV_CONNECTABLE)) {
		err = le_adv_start_add_conn(adv, &conn);
		if (err) {
			return err;
		}
	}

	atomic_set_bit_to(adv->flags, BT_ADV_LIMITED, param &&
			  (param->timeout > 0 || param->num_events > 0));

	if (atomic_test_bit(adv->flags, BT_ADV_CONNECTABLE)) {
		if (IS_ENABLED(CONFIG_BT_PRIVACY) &&
		    !atomic_test_bit(adv->flags, BT_ADV_USE_IDENTITY)) {
			bt_id_set_adv_private_addr(adv);
		}
	} else {
		if (!atomic_test_bit(adv->flags, BT_ADV_USE_IDENTITY)) {
			bt_id_set_adv_private_addr(adv);
		}
	}

	if (get_adv_name_type(adv) != ADV_NAME_TYPE_NONE &&
	    !atomic_test_bit(adv->flags, BT_ADV_DATA_SET)) {
		/* Set the advertiser name */
		bt_le_ext_adv_set_data(adv, NULL, 0, NULL, 0);
	}

	err = bt_le_adv_set_enable_ext(adv, true, param);
	if (err) {
		LOG_ERR("Failed to start advertiser");
		if (IS_ENABLED(CONFIG_BT_PERIPHERAL) && conn) {
			bt_conn_set_state(conn, BT_CONN_DISCONNECTED);
			bt_conn_unref(conn);
		}

		return err;
	}

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL) && conn) {
		/* If undirected connectable advertiser we have created a
		 * connection object that we don't yet give to the application.
		 * Since we don't give the application a reference to manage in
		 * this case, we need to release this reference here
		 */
		bt_conn_unref(conn);
	}

	return 0;
}

int bt_le_ext_adv_stop(struct bt_le_ext_adv *adv)
{
	(void)bt_le_lim_adv_cancel_timeout(adv);

	atomic_clear_bit(adv->flags, BT_ADV_PERSIST);

	if (!atomic_test_bit(adv->flags, BT_ADV_ENABLED)) {
		return 0;
	}

	if (atomic_test_and_clear_bit(adv->flags, BT_ADV_LIMITED)) {
		bt_id_adv_limited_stopped(adv);

#if defined(CONFIG_BT_SMP)
		bt_id_pending_keys_update();
#endif
	}

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL) &&
	    atomic_test_bit(adv->flags, BT_ADV_CONNECTABLE)) {
		le_adv_stop_free_conn(adv, 0);
	}

	return bt_le_adv_set_enable_ext(adv, false, NULL);
}

int bt_le_ext_adv_set_data(struct bt_le_ext_adv *adv,
			   const struct bt_data *ad, size_t ad_len,
			   const struct bt_data *sd, size_t sd_len)
{
	bool ext_adv, scannable;

	ext_adv = atomic_test_bit(adv->flags, BT_ADV_EXT_ADV);
	scannable = atomic_test_bit(adv->flags, BT_ADV_SCANNABLE);

	if (ext_adv) {
		if ((scannable && ad_len) ||
		    (!scannable && sd_len)) {
			return -ENOTSUP;
		}
	}

	return le_adv_update(adv, ad, ad_len, sd, sd_len, ext_adv, scannable,
			     get_adv_name_type(adv));
}

int bt_le_ext_adv_delete(struct bt_le_ext_adv *adv)
{
	struct bt_hci_cp_le_remove_adv_set *cp;
	struct net_buf *buf;
	int err;

	if (!BT_DEV_FEAT_LE_EXT_ADV(bt_dev.le.features)) {
		return -ENOTSUP;
	}

	/* Advertising set should be stopped first */
	if (atomic_test_bit(adv->flags, BT_ADV_ENABLED)) {
		return -EINVAL;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_REMOVE_ADV_SET, sizeof(*cp));
	if (!buf) {
		LOG_WRN("No HCI buffers");
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = adv->handle;

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_REMOVE_ADV_SET, buf, NULL);
	if (err) {
		return err;
	}

	adv_delete(adv);

	return 0;
}
#endif /* defined(CONFIG_BT_EXT_ADV) */


static void adv_timeout(struct k_work *work)
{
	int err = 0;
	struct k_work_delayable *dwork;
	struct bt_le_ext_adv *adv;

	dwork = k_work_delayable_from_work(work);
	adv = CONTAINER_OF(dwork, struct bt_le_ext_adv, lim_adv_timeout_work);

#if defined(CONFIG_BT_EXT_ADV)
	if (adv == bt_dev.adv) {
		err = bt_le_adv_stop();
	} else {
		err = bt_le_ext_adv_stop(adv);
	}
#else
	err = bt_le_adv_stop();
#endif
	if (err) {
		LOG_WRN("Failed to stop advertising: %d", err);
	}
}

#if defined(CONFIG_BT_PER_ADV)
int bt_le_per_adv_set_param(struct bt_le_ext_adv *adv,
			    const struct bt_le_per_adv_param *param)
{
#if defined(CONFIG_BT_PER_ADV_RSP)
	/* The v2 struct can be used even if we end up sending a v1 command
	 * because they have the same layout for the common fields.
	 * V2 simply adds fields at the end of the v1 command.
	 */
	struct bt_hci_cp_le_set_per_adv_param_v2 *cp;
#else
	struct bt_hci_cp_le_set_per_adv_param *cp;
#endif /* CONFIG_BT_PER_ADV_RSP */

	uint16_t opcode;
	uint16_t size;
	struct net_buf *buf;
	int err;
	uint16_t props = 0;

	if (IS_ENABLED(CONFIG_BT_PER_ADV_RSP) && BT_FEAT_LE_PAWR_ADVERTISER(bt_dev.le.features)) {
		opcode = BT_HCI_OP_LE_SET_PER_ADV_PARAM_V2;
		size = sizeof(struct bt_hci_cp_le_set_per_adv_param_v2);
	} else if (BT_FEAT_LE_EXT_PER_ADV(bt_dev.le.features)) {
		opcode = BT_HCI_OP_LE_SET_PER_ADV_PARAM;
		size = sizeof(struct bt_hci_cp_le_set_per_adv_param);
	} else {
		return -ENOTSUP;
	}

	if (atomic_test_bit(adv->flags, BT_ADV_SCANNABLE)) {
		return -EINVAL;
	} else if (atomic_test_bit(adv->flags, BT_ADV_CONNECTABLE)) {
		return -EINVAL;
	} else if (!atomic_test_bit(adv->flags, BT_ADV_EXT_ADV)) {
		return -EINVAL;
	}

	if (param->interval_min < BT_GAP_PER_ADV_MIN_INTERVAL ||
	    param->interval_max > BT_GAP_PER_ADV_MAX_INTERVAL ||
	    param->interval_min > param->interval_max) {
		return -EINVAL;
	}

	if (!BT_FEAT_LE_PER_ADV_ADI_SUPP(bt_dev.le.features) &&
	    (param->options & BT_LE_PER_ADV_OPT_INCLUDE_ADI)) {
		return -ENOTSUP;
	}

	buf = bt_hci_cmd_create(opcode, size);
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, size);
	(void)memset(cp, 0, size);

	cp->handle = adv->handle;
	cp->min_interval = sys_cpu_to_le16(param->interval_min);
	cp->max_interval = sys_cpu_to_le16(param->interval_max);

	if (param->options & BT_LE_PER_ADV_OPT_USE_TX_POWER) {
		props |= BT_HCI_LE_ADV_PROP_TX_POWER;
	}

	cp->props = sys_cpu_to_le16(props);

#if defined(CONFIG_BT_PER_ADV_RSP)
	if (opcode == BT_HCI_OP_LE_SET_PER_ADV_PARAM_V2) {
		cp->num_subevents = param->num_subevents;
		cp->subevent_interval = param->subevent_interval;
		cp->response_slot_delay = param->response_slot_delay;
		cp->response_slot_spacing = param->response_slot_spacing;
		cp->num_response_slots = param->num_response_slots;
	}
#endif /* CONFIG_BT_PER_ADV_RSP */

	err = bt_hci_cmd_send_sync(opcode, buf, NULL);
	if (err) {
		return err;
	}

	if (param->options & BT_LE_PER_ADV_OPT_INCLUDE_ADI) {
		atomic_set_bit(adv->flags, BT_PER_ADV_INCLUDE_ADI);
	} else {
		atomic_clear_bit(adv->flags, BT_PER_ADV_INCLUDE_ADI);
	}

	atomic_set_bit(adv->flags, BT_PER_ADV_PARAMS_SET);

	return 0;
}

int bt_le_per_adv_set_data(const struct bt_le_ext_adv *adv,
			   const struct bt_data *ad, size_t ad_len)
{
	size_t total_len_bytes = 0;

	if (!BT_FEAT_LE_EXT_PER_ADV(bt_dev.le.features)) {
		return -ENOTSUP;
	}

	if (!atomic_test_bit(adv->flags, BT_PER_ADV_PARAMS_SET)) {
		return -EINVAL;
	}

	if (ad_len != 0 && ad == NULL) {
		return -EINVAL;
	}

	for (size_t i = 0; i < ad_len; i++) {
		total_len_bytes += ad[i].data_len + 2;
	}

	if ((total_len_bytes > BT_HCI_LE_PER_ADV_FRAG_MAX_LEN) &&
	    atomic_test_bit(adv->flags, BT_PER_ADV_ENABLED)) {
		/* It is not allowed to set periodic advertising data
		 * in multiple operations while it is running.
		 */
		return -EINVAL;
	}

	return hci_set_per_adv_data(adv, ad, ad_len);
}

int bt_le_per_adv_set_subevent_data(const struct bt_le_ext_adv *adv, uint8_t num_subevents,
				    const struct bt_le_per_adv_subevent_data_params *params)
{
	struct bt_hci_cp_le_set_pawr_subevent_data *cp;
	struct bt_hci_cp_le_set_pawr_subevent_data_element *element;
	struct net_buf *buf;
	size_t cmd_length = sizeof(*cp);

	if (!BT_FEAT_LE_PAWR_ADVERTISER(bt_dev.le.features)) {
		return -ENOTSUP;
	}

	for (size_t i = 0; i < num_subevents; i++) {
		cmd_length += sizeof(struct bt_hci_cp_le_set_pawr_subevent_data_element);
		cmd_length += params[i].data->len;
	}

	if (cmd_length > 0xFF) {
		return -EINVAL;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_PER_ADV_SUBEVENT_DATA, (uint8_t)cmd_length);
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->adv_handle = adv->handle;
	cp->num_subevents = num_subevents;

	for (size_t i = 0; i < num_subevents; i++) {
		element = net_buf_add(buf, sizeof(*element));
		element->subevent = params[i].subevent;
		element->response_slot_start = params[i].response_slot_start;
		element->response_slot_count = params[i].response_slot_count;
		element->subevent_data_length = params[i].data->len;
		net_buf_add_mem(buf, params[i].data->data, params[i].data->len);
	}

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_PER_ADV_SUBEVENT_DATA, buf, NULL);
}

static int bt_le_per_adv_enable(struct bt_le_ext_adv *adv, bool enable)
{
	struct bt_hci_cp_le_set_per_adv_enable *cp;
	struct net_buf *buf;
	struct bt_hci_cmd_state_set state;
	int err;

	if (!BT_FEAT_LE_EXT_PER_ADV(bt_dev.le.features)) {
		return -ENOTSUP;
	}

	/* TODO: We could setup some default ext adv params if not already set*/
	if (!atomic_test_bit(adv->flags, BT_PER_ADV_PARAMS_SET)) {
		return -EINVAL;
	}

	if (atomic_test_bit(adv->flags, BT_PER_ADV_ENABLED) == enable) {
		return -EALREADY;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_PER_ADV_ENABLE, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	(void)memset(cp, 0, sizeof(*cp));

	cp->handle = adv->handle;

	if (enable) {
		cp->enable = BT_HCI_LE_SET_PER_ADV_ENABLE_ENABLE;

		if (atomic_test_bit(adv->flags, BT_PER_ADV_INCLUDE_ADI)) {
			cp->enable |= BT_HCI_LE_SET_PER_ADV_ENABLE_ADI;
		}
	} else {
		cp->enable = 0U;
	}

	bt_hci_cmd_state_set_init(buf, &state, adv->flags,
				  BT_PER_ADV_ENABLED, enable);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_PER_ADV_ENABLE, buf, NULL);
	if (err) {
		return err;
	}

	return 0;
}

int bt_le_per_adv_start(struct bt_le_ext_adv *adv)
{
	return bt_le_per_adv_enable(adv, true);
}

int bt_le_per_adv_stop(struct bt_le_ext_adv *adv)
{
	return bt_le_per_adv_enable(adv, false);
}

#if defined(CONFIG_BT_PER_ADV_RSP)
void bt_hci_le_per_adv_subevent_data_request(struct net_buf *buf)
{
	struct bt_hci_evt_le_per_adv_subevent_data_request *evt;
	struct bt_le_per_adv_data_request request;
	struct bt_le_ext_adv *adv;

	if (buf->len < sizeof(struct bt_hci_evt_le_per_adv_subevent_data_request)) {
		LOG_ERR("Invalid data request");

		return;
	}

	evt = net_buf_pull_mem(buf, sizeof(struct bt_hci_evt_le_per_adv_subevent_data_request));
	adv = bt_adv_lookup_handle(evt->adv_handle);
	if (!adv) {
		LOG_ERR("Unknown advertising handle %d", evt->adv_handle);

		return;
	}

	request.start = evt->subevent_start;
	request.count = evt->subevent_data_count;

	if (adv->cb && adv->cb->pawr_data_request) {
		adv->cb->pawr_data_request(adv, &request);
	}
}

void bt_hci_le_per_adv_response_report(struct net_buf *buf)
{
	struct bt_hci_evt_le_per_adv_response_report *evt;
	struct bt_hci_evt_le_per_adv_response *response;
	struct bt_le_ext_adv *adv;
	struct bt_le_per_adv_response_info info;
	struct net_buf_simple data;

	if (buf->len < sizeof(struct bt_hci_evt_le_per_adv_response_report)) {
		LOG_ERR("Invalid response report");

		return;
	}

	evt = net_buf_pull_mem(buf, sizeof(struct bt_hci_evt_le_per_adv_response_report));
	adv = bt_adv_lookup_handle(evt->adv_handle);
	if (!adv) {
		LOG_ERR("Unknown advertising handle %d", evt->adv_handle);

		return;
	}

	info.subevent = evt->subevent;
	info.tx_status = evt->tx_status;

	for (uint8_t i = 0; i < evt->num_responses; i++) {
		if (buf->len < sizeof(struct bt_hci_evt_le_per_adv_response)) {
			LOG_ERR("Invalid response report");

			return;
		}

		response = net_buf_pull_mem(buf, sizeof(struct bt_hci_evt_le_per_adv_response));
		info.tx_power = response->tx_power;
		info.rssi = response->rssi;
		info.cte_type = bt_get_df_cte_type(response->cte_type);
		info.response_slot = response->response_slot;

		if (buf->len < response->data_length) {
			LOG_ERR("Invalid response report");

			return;
		}

		if (response->data_status == BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL) {
			LOG_WRN("Incomplete response report received, discarding");
			(void)net_buf_pull_mem(buf, response->data_length);
		} else if (response->data_status == BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_RX_FAILED) {
			(void)net_buf_pull_mem(buf, response->data_length);

			if (adv->cb && adv->cb->pawr_response) {
				adv->cb->pawr_response(adv, &info, NULL);
			}
		} else if (response->data_status == BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE) {
			net_buf_simple_init_with_data(&data,
						      net_buf_pull_mem(buf, response->data_length),
						      response->data_length);

			if (adv->cb && adv->cb->pawr_response) {
				adv->cb->pawr_response(adv, &info, &data);
			}
		} else {
			LOG_ERR("Invalid data status %d", response->data_status);
			(void)net_buf_pull_mem(buf, response->data_length);
		}
	}
}
#endif /* CONFIG_BT_PER_ADV_RSP */

#if defined(CONFIG_BT_PER_ADV_SYNC_TRANSFER_SENDER)
int bt_le_per_adv_set_info_transfer(const struct bt_le_ext_adv *adv,
				    const struct bt_conn *conn,
				    uint16_t service_data)
{
	struct bt_hci_cp_le_per_adv_set_info_transfer *cp;
	struct net_buf *buf;


	if (!BT_FEAT_LE_EXT_PER_ADV(bt_dev.le.features)) {
		return -ENOTSUP;
	} else if (!BT_FEAT_LE_PAST_SEND(bt_dev.le.features)) {
		return -ENOTSUP;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_PER_ADV_SET_INFO_TRANSFER,
				sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	(void)memset(cp, 0, sizeof(*cp));

	cp->conn_handle = sys_cpu_to_le16(conn->handle);
	cp->adv_handle = adv->handle;
	cp->service_data = sys_cpu_to_le16(service_data);

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_PER_ADV_SET_INFO_TRANSFER, buf,
				    NULL);
}
#endif /* CONFIG_BT_PER_ADV_SYNC_TRANSFER_SENDER */
#endif /* CONFIG_BT_PER_ADV */

#if defined(CONFIG_BT_EXT_ADV)
#if defined(CONFIG_BT_BROADCASTER)
void bt_hci_le_adv_set_terminated(struct net_buf *buf)
{
	struct bt_hci_evt_le_adv_set_terminated *evt;
	struct bt_le_ext_adv *adv;
	uint16_t conn_handle;
#if defined(CONFIG_BT_CONN) && (CONFIG_BT_EXT_ADV_MAX_ADV_SET > 1)
	bool was_adv_enabled;
#endif

	evt = (void *)buf->data;
	adv = bt_adv_lookup_handle(evt->adv_handle);
	conn_handle = sys_le16_to_cpu(evt->conn_handle);

	LOG_DBG("status 0x%02x adv_handle %u conn_handle 0x%02x num %u", evt->status,
		evt->adv_handle, conn_handle, evt->num_completed_ext_adv_evts);

	if (!adv) {
		LOG_ERR("No valid adv");
		return;
	}

	(void)bt_le_lim_adv_cancel_timeout(adv);

#if defined(CONFIG_BT_CONN) && (CONFIG_BT_EXT_ADV_MAX_ADV_SET > 1)
	was_adv_enabled = atomic_test_bit(adv->flags, BT_ADV_ENABLED);
#endif

	atomic_clear_bit(adv->flags, BT_ADV_ENABLED);

#if defined(CONFIG_BT_CONN) && (CONFIG_BT_EXT_ADV_MAX_ADV_SET > 1)
	bt_dev.adv_conn_id = adv->id;
	for (int i = 0; i < ARRAY_SIZE(bt_dev.cached_conn_complete); i++) {
		if (bt_dev.cached_conn_complete[i].valid &&
		    bt_dev.cached_conn_complete[i].evt.handle == evt->conn_handle) {
			if (was_adv_enabled) {
				/* Process the cached connection complete event
				 * now that the corresponding advertising set is known.
				 *
				 * If the advertiser has been stopped before the connection
				 * complete event has been raised to the application, we
				 * discard the event.
				 */
				bt_hci_le_enh_conn_complete(&bt_dev.cached_conn_complete[i].evt);
			}
			bt_dev.cached_conn_complete[i].valid = false;
		}
	}
#endif

	if (evt->status && IS_ENABLED(CONFIG_BT_PERIPHERAL) &&
	    atomic_test_bit(adv->flags, BT_ADV_CONNECTABLE)) {
		/* This will call connected callback for high duty cycle
		 * directed advertiser timeout.
		 */
		le_adv_stop_free_conn(adv, evt->status);
	}

	if (IS_ENABLED(CONFIG_BT_CONN) && !evt->status) {
		struct bt_conn *conn = bt_conn_lookup_handle(conn_handle, BT_CONN_TYPE_LE);

		if (conn) {
			if (IS_ENABLED(CONFIG_BT_PRIVACY) &&
			    !atomic_test_bit(adv->flags, BT_ADV_USE_IDENTITY)) {
				/* Set Responder address unless already set */
				conn->le.resp_addr.type = BT_ADDR_LE_RANDOM;
				if (bt_addr_eq(&conn->le.resp_addr.a, BT_ADDR_ANY)) {
					bt_addr_copy(&conn->le.resp_addr.a,
						     &adv->random_addr.a);
				}
			} else if (adv->options & BT_LE_ADV_OPT_USE_NRPA) {
				bt_addr_le_copy(&conn->le.resp_addr,
						&adv->random_addr);
			} else {
				bt_addr_le_copy(&conn->le.resp_addr,
					&bt_dev.id_addr[conn->id]);
			}

			if (adv->cb && adv->cb->connected) {
				struct bt_le_ext_adv_connected_info info = {
					.conn = conn,
				};

				adv->cb->connected(adv, &info);
			}

			bt_conn_unref(conn);
		}
	}

	if (atomic_test_and_clear_bit(adv->flags, BT_ADV_LIMITED)) {
		bt_id_adv_limited_stopped(adv);

#if defined(CONFIG_BT_SMP)
		bt_id_pending_keys_update();
#endif

		if (adv->cb && adv->cb->sent) {
			struct bt_le_ext_adv_sent_info info = {
				.num_sent = evt->num_completed_ext_adv_evts,
			};

			adv->cb->sent(adv, &info);
		}
	}

	if (adv == bt_dev.adv) {
		if (atomic_test_bit(adv->flags, BT_ADV_PERSIST)) {
#if defined(CONFIG_BT_PERIPHERAL)
			bt_le_adv_resume();
#endif
		} else {
			bt_le_adv_delete_legacy();
		}
	}
}

void bt_hci_le_scan_req_received(struct net_buf *buf)
{
	struct bt_hci_evt_le_scan_req_received *evt;
	struct bt_le_ext_adv *adv;

	evt = (void *)buf->data;
	adv = bt_adv_lookup_handle(evt->handle);

	LOG_DBG("handle %u peer %s", evt->handle, bt_addr_le_str(&evt->addr));

	if (!adv) {
		LOG_ERR("No valid adv");
		return;
	}

	if (adv->cb && adv->cb->scanned) {
		struct bt_le_ext_adv_scanned_info info;
		bt_addr_le_t id_addr;

		if (bt_addr_le_is_resolved(&evt->addr)) {
			bt_addr_le_copy_resolved(&id_addr, &evt->addr);
		} else {
			bt_addr_le_copy(&id_addr,
					bt_lookup_id_addr(adv->id, &evt->addr));
		}

		info.addr = &id_addr;
		adv->cb->scanned(adv, &info);
	}
}
#endif /* defined(CONFIG_BT_BROADCASTER) */
#endif /* defined(CONFIG_BT_EXT_ADV) */

/* Common functions for LE Audio services */

/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/check.h>

#include "audio_internal.h"

LOG_MODULE_REGISTER(bt_audio, CONFIG_BT_AUDIO_LOG_LEVEL);

int bt_audio_data_parse(const uint8_t ltv[], size_t size,
			bool (*func)(struct bt_data *data, void *user_data), void *user_data)
{
	CHECKIF(ltv == NULL) {
		LOG_DBG("ltv is NULL");

		return -EINVAL;
	}

	CHECKIF(func == NULL) {
		LOG_DBG("func is NULL");

		return -EINVAL;
	}

	for (size_t i = 0; i < size;) {
		const uint8_t len = ltv[i];
		struct bt_data data;

		if (i + len > size || len < sizeof(data.type)) {
			LOG_DBG("Invalid len %u at i = %zu", len, i);

			return -EINVAL;
		}

		i++; /* Increment as we have parsed the len field */

		data.type = ltv[i++];
		data.data_len = len - sizeof(data.type);

		if (data.data_len > 0) {
			data.data = &ltv[i];
		} else {
			data.data = NULL;
		}

		if (!func(&data, user_data)) {
			return -ECANCELED;
		}

		/* Since we are incrementing i by the value_len, we don't need to increment it
		 * further in the `for` statement
		 */
		i += data.data_len;
	}

	return 0;
}

uint8_t bt_audio_get_chan_count(enum bt_audio_location chan_allocation)
{
	if (chan_allocation == BT_AUDIO_LOCATION_MONO_AUDIO) {
		return 1;
	}

#ifdef POPCOUNT
	return POPCOUNT(chan_allocation);
#else
	uint8_t cnt = 0U;

	while (chan_allocation != 0U) {
		cnt += chan_allocation & 1U;
		chan_allocation >>= 1U;
	}

	return cnt;
#endif
}

#if defined(CONFIG_BT_CONN)

static uint8_t bt_audio_security_check(const struct bt_conn *conn)
{
	struct bt_conn_info info;
	int err;

	err = bt_conn_get_info(conn, &info);
	if (err < 0) {
		return BT_ATT_ERR_UNLIKELY;
	}

	/* Require an encryption key with at least 128 bits of entropy, derived from SC or OOB
	 * method.
	 */
	if ((info.security.flags & (BT_SECURITY_FLAG_OOB | BT_SECURITY_FLAG_SC)) == 0) {
		/* If the client has insufficient security to read/write the requested attribute
		 * then an ATT_ERROR_RSP PDU shall be sent with the Error Code parameter set to
		 * Insufficient Authentication (0x05).
		 */
		return BT_ATT_ERR_AUTHENTICATION;
	}

	if (info.security.enc_key_size < BT_ENC_KEY_SIZE_MAX) {
		return BT_ATT_ERR_ENCRYPTION_KEY_SIZE;
	}

	return BT_ATT_ERR_SUCCESS;
}

ssize_t bt_audio_read_chrc(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			   void *buf, uint16_t len, uint16_t offset)
{
	const struct bt_audio_attr_user_data *user_data = attr->user_data;

	if (user_data->read == NULL) {
		return BT_GATT_ERR(BT_ATT_ERR_READ_NOT_PERMITTED);
	}

	if (conn != NULL) {
		uint8_t err;

		err = bt_audio_security_check(conn);
		if (err != 0) {
			return BT_GATT_ERR(err);
		}
	}

	return user_data->read(conn, attr, buf, len, offset);
}

ssize_t bt_audio_write_chrc(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			    const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	const struct bt_audio_attr_user_data *user_data = attr->user_data;

	if (user_data->write == NULL) {
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_NOT_PERMITTED);
	}

	if (conn != NULL) {
		uint8_t err;

		err = bt_audio_security_check(conn);
		if (err != 0) {
			return BT_GATT_ERR(err);
		}
	}

	return user_data->write(conn, attr, buf, len, offset, flags);
}

ssize_t bt_audio_ccc_cfg_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			       uint16_t value)
{
	if (conn != NULL) {
		uint8_t err;

		err = bt_audio_security_check(conn);
		if (err != 0) {
			return BT_GATT_ERR(err);
		}
	}

	return sizeof(value);
}
#endif /* CONFIG_BT_CONN */

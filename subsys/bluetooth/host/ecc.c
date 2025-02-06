/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/check.h>
#include <zephyr/bluetooth/hci.h>

#include <psa/crypto.h>

#include "long_wq.h"
#include "ecc.h"
#include "hci_core.h"

#define LOG_LEVEL CONFIG_BT_HCI_CORE_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_ecc);

static uint8_t pub_key[BT_PUB_KEY_LEN];
static sys_slist_t pub_key_cb_slist;
static bt_dh_key_cb_t dh_key_cb;

static void generate_pub_key(struct k_work *work);
static void generate_dh_key(struct k_work *work);
K_WORK_DEFINE(pub_key_work, generate_pub_key);
K_WORK_DEFINE(dh_key_work, generate_dh_key);

enum {
	PENDING_PUB_KEY,
	PENDING_DHKEY,

	/* Total number of flags - must be at the end of the enum */
	NUM_FLAGS,
};

static ATOMIC_DEFINE(flags, NUM_FLAGS);

static struct {
	uint8_t private_key_be[BT_PRIV_KEY_LEN];

	union {
		uint8_t public_key_be[BT_PUB_KEY_LEN];
		uint8_t dhkey_be[BT_DH_KEY_LEN];
	};
} ecc;

/* based on Core Specification 4.2 Vol 3. Part H 2.3.5.6.1 */
static const uint8_t debug_private_key_be[BT_PRIV_KEY_LEN] = {
	0x3f, 0x49, 0xf6, 0xd4, 0xa3, 0xc5, 0x5f, 0x38,
	0x74, 0xc9, 0xb3, 0xe3, 0xd2, 0x10, 0x3f, 0x50,
	0x4a, 0xff, 0x60, 0x7b, 0xeb, 0x40, 0xb7, 0x99,
	0x58, 0x99, 0xb8, 0xa6, 0xcd, 0x3c, 0x1a, 0xbd,
};

static const uint8_t debug_public_key[BT_PUB_KEY_LEN] = {
	/* X */
	0xe6, 0x9d, 0x35, 0x0e, 0x48, 0x01, 0x03, 0xcc,
	0xdb, 0xfd, 0xf4, 0xac, 0x11, 0x91, 0xf4, 0xef,
	0xb9, 0xa5, 0xf9, 0xe9, 0xa7, 0x83, 0x2c, 0x5e,
	0x2c, 0xbe, 0x97, 0xf2, 0xd2, 0x03, 0xb0, 0x20,
	/* Y */
	0x8b, 0xd2, 0x89, 0x15, 0xd0, 0x8e, 0x1c, 0x74,
	0x24, 0x30, 0xed, 0x8f, 0xc2, 0x45, 0x63, 0x76,
	0x5c, 0x15, 0x52, 0x5a, 0xbf, 0x9a, 0x32, 0x63,
	0x6d, 0xeb, 0x2a, 0x65, 0x49, 0x9c, 0x80, 0xdc
};

bool bt_pub_key_is_debug(uint8_t *cmp_pub_key)
{
	return memcmp(cmp_pub_key, debug_public_key, BT_PUB_KEY_LEN) == 0;
}

bool bt_pub_key_is_valid(const uint8_t key[BT_PUB_KEY_LEN])
{
	uint8_t key_be[BT_PUB_KEY_LEN + 1];
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t ret;
	psa_key_id_t handle;

	psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&attr, 256);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&attr, PSA_ALG_ECDH);

	/* PSA expects secp256r1 public key to start with a predefined 0x04 byte */
	key_be[0] = 0x04;
	sys_memcpy_swap(&key_be[1], key, BT_PUB_KEY_COORD_LEN);
	sys_memcpy_swap(&key_be[1 + BT_PUB_KEY_COORD_LEN], &key[BT_PUB_KEY_COORD_LEN],
			BT_PUB_KEY_COORD_LEN);

	ret = psa_import_key(&attr, key_be, sizeof(key_be), &handle);
	psa_reset_key_attributes(&attr);

	if (ret == PSA_SUCCESS) {
		psa_destroy_key(handle);
		return true;
	}

	LOG_ERR("psa_import_key() returned status %d", ret);
	return false;
}

static void set_key_attributes(psa_key_attributes_t *attr)
{
	psa_set_key_type(attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(attr, 256);
	psa_set_key_usage_flags(attr, PSA_KEY_USAGE_EXPORT | PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(attr, PSA_ALG_ECDH);
}

static void generate_pub_key(struct k_work *work)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	struct bt_pub_key_cb *cb;
	psa_key_id_t key_id;
	uint8_t tmp_pub_key_buf[BT_PUB_KEY_LEN + 1];
	size_t tmp_len;
	int err;
	psa_status_t ret;

	set_key_attributes(&attr);

	ret = psa_generate_key(&attr, &key_id);
	if (ret != PSA_SUCCESS) {
		LOG_ERR("Failed to generate ECC key %d", ret);
		err = BT_HCI_ERR_UNSPECIFIED;
		goto done;
	}

	ret = psa_export_public_key(key_id, tmp_pub_key_buf, sizeof(tmp_pub_key_buf), &tmp_len);
	if (ret != PSA_SUCCESS) {
		LOG_ERR("Failed to export ECC public key %d", ret);
		err = BT_HCI_ERR_UNSPECIFIED;
		goto done;
	}
	/* secp256r1 PSA exported public key has an extra 0x04 predefined byte at
	 * the beginning of the buffer which is not part of the coordinate so
	 * we remove that.
	 */
	memcpy(ecc.public_key_be, &tmp_pub_key_buf[1], BT_PUB_KEY_LEN);

	ret = psa_export_key(key_id, ecc.private_key_be, BT_PRIV_KEY_LEN, &tmp_len);
	if (ret != PSA_SUCCESS) {
		LOG_ERR("Failed to export ECC private key %d", ret);
		err = BT_HCI_ERR_UNSPECIFIED;
		goto done;
	}

	ret = psa_destroy_key(key_id);
	if (ret != PSA_SUCCESS) {
		LOG_ERR("Failed to destroy ECC key ID %d", ret);
		err = BT_HCI_ERR_UNSPECIFIED;
		goto done;
	}

	sys_memcpy_swap(pub_key, ecc.public_key_be, BT_PUB_KEY_COORD_LEN);
	sys_memcpy_swap(&pub_key[BT_PUB_KEY_COORD_LEN],
			&ecc.public_key_be[BT_PUB_KEY_COORD_LEN], BT_PUB_KEY_COORD_LEN);

	atomic_set_bit(bt_dev.flags, BT_DEV_HAS_PUB_KEY);
	err = 0;

done:
	atomic_clear_bit(flags, PENDING_PUB_KEY);

	/* Change to cooperative priority while we do the callbacks */
	k_sched_lock();

	SYS_SLIST_FOR_EACH_CONTAINER(&pub_key_cb_slist, cb, node) {
		if (cb->func) {
			cb->func(err ? NULL : pub_key);
		}
	}

	sys_slist_init(&pub_key_cb_slist);

	k_sched_unlock();
}

static void generate_dh_key(struct k_work *work)
{
	int err;

	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id;
	psa_status_t ret;
	/* PSA expects secp256r1 public key to start with a predefined 0x04 byte
	 * at the beginning the buffer.
	 */
	uint8_t tmp_pub_key_buf[BT_PUB_KEY_LEN + 1] = { 0x04 };
	size_t tmp_len;

	set_key_attributes(&attr);

	const uint8_t *priv_key = (IS_ENABLED(CONFIG_BT_USE_DEBUG_KEYS) ?
				   debug_private_key_be :
				   ecc.private_key_be);
	ret = psa_import_key(&attr, priv_key, BT_PRIV_KEY_LEN, &key_id);
	if (ret != PSA_SUCCESS) {
		err = -EIO;
		LOG_ERR("Failed to import the private key for key agreement %d", ret);
		goto exit;
	}

	memcpy(&tmp_pub_key_buf[1], ecc.public_key_be, BT_PUB_KEY_LEN);
	ret = psa_raw_key_agreement(PSA_ALG_ECDH, key_id, tmp_pub_key_buf, sizeof(tmp_pub_key_buf),
				    ecc.dhkey_be, BT_DH_KEY_LEN, &tmp_len);
	if (ret != PSA_SUCCESS) {
		err = -EIO;
		LOG_ERR("Raw key agreement failed %d", ret);
		goto exit;
	}

	ret = psa_destroy_key(key_id);
	if (ret != PSA_SUCCESS) {
		LOG_ERR("Failed to destroy the key %d", ret);
		err = -EIO;
		goto exit;
	}

	err = 0;

exit:
	/* Change to cooperative priority while we do the callback */
	k_sched_lock();

	if (dh_key_cb) {
		bt_dh_key_cb_t cb = dh_key_cb;

		dh_key_cb = NULL;
		atomic_clear_bit(flags, PENDING_DHKEY);

		if (err) {
			cb(NULL);
		} else {
			uint8_t dhkey[BT_DH_KEY_LEN];

			sys_memcpy_swap(dhkey, ecc.dhkey_be, sizeof(ecc.dhkey_be));
			cb(dhkey);
		}
	}

	k_sched_unlock();
}

int bt_pub_key_gen(struct bt_pub_key_cb *new_cb)
{
	struct bt_pub_key_cb *cb;

	if (IS_ENABLED(CONFIG_BT_USE_DEBUG_KEYS)) {
		atomic_set_bit(bt_dev.flags, BT_DEV_HAS_PUB_KEY);
		__ASSERT_NO_MSG(new_cb->func != NULL);
		new_cb->func(debug_public_key);
		return 0;
	}

	if (!new_cb) {
		return -EINVAL;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&pub_key_cb_slist, cb, node) {
		if (cb == new_cb) {
			LOG_WRN("Callback already registered");
			return -EALREADY;
		}
	}

	if (atomic_test_bit(flags, PENDING_DHKEY)) {
		LOG_WRN("Busy performing another ECDH operation");
		return -EBUSY;
	}

	sys_slist_prepend(&pub_key_cb_slist, &new_cb->node);

	if (atomic_test_and_set_bit(flags, PENDING_PUB_KEY)) {
		return 0;
	}

	atomic_clear_bit(bt_dev.flags, BT_DEV_HAS_PUB_KEY);

	bt_long_wq_submit(&pub_key_work);

	return 0;
}

void bt_pub_key_hci_disrupted(void)
{
	struct bt_pub_key_cb *cb;

	atomic_clear_bit(flags, PENDING_PUB_KEY);

	SYS_SLIST_FOR_EACH_CONTAINER(&pub_key_cb_slist, cb, node) {
		if (cb->func) {
			cb->func(NULL);
		}
	}

	sys_slist_init(&pub_key_cb_slist);
}

const uint8_t *bt_pub_key_get(void)
{
	if (IS_ENABLED(CONFIG_BT_USE_DEBUG_KEYS)) {
		return debug_public_key;
	}

	if (atomic_test_bit(bt_dev.flags, BT_DEV_HAS_PUB_KEY)) {
		return pub_key;
	}

	return NULL;
}

int bt_dh_key_gen(const uint8_t remote_pk[BT_PUB_KEY_LEN], bt_dh_key_cb_t cb)
{
	if (dh_key_cb == cb) {
		return -EALREADY;
	}

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_HAS_PUB_KEY)) {
		return -EADDRNOTAVAIL;
	}

	if (dh_key_cb ||
	    atomic_test_bit(flags, PENDING_PUB_KEY) ||
	    atomic_test_and_set_bit(flags, PENDING_DHKEY)) {
		return -EBUSY;
	}

	dh_key_cb = cb;

	/* Convert X and Y coordinates from little-endian to
	 * big-endian (expected by the crypto API).
	 */
	sys_memcpy_swap(ecc.public_key_be, remote_pk, BT_PUB_KEY_COORD_LEN);
	sys_memcpy_swap(&ecc.public_key_be[BT_PUB_KEY_COORD_LEN],
			&remote_pk[BT_PUB_KEY_COORD_LEN], BT_PUB_KEY_COORD_LEN);

	bt_long_wq_submit(&dh_key_work);

	return 0;
}

#ifdef ZTEST_UNITTEST
uint8_t const *bt_ecc_get_public_key(void)
{
	return pub_key;
}

uint8_t const *bt_ecc_get_internal_debug_public_key(void)
{
	return debug_public_key;
}

sys_slist_t *bt_ecc_get_pub_key_cb_slist(void)
{
	return &pub_key_cb_slist;
}

bt_dh_key_cb_t *bt_ecc_get_dh_key_cb(void)
{
	return &dh_key_cb;
}
#endif /* ZTEST_UNITTEST */

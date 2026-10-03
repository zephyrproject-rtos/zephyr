/* Copyright (c) 2026 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/types.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <psa/protected_storage.h>

/* The flash must be erased after this test suite is run for the write-once entry test to pass.
 * Both the ITS partition (storage_partition) and the PS one (storage_partition_2) are erased,
 * so that no PS entry is left without its replay protection value in ITS.
 */
#if !defined(CONFIG_BUILD_WITH_TFM) && defined(CONFIG_FLASH_PAGE_LAYOUT) &&                        \
	PARTITION_EXISTS(storage_partition)
static int erase_flash(void)
{
	int rc;

	rc = flash_flatten(PARTITION_DEVICE(storage_partition),
			   PARTITION_OFFSET(storage_partition),
			   PARTITION_SIZE(storage_partition));
	if (rc < 0) {
		TC_PRINT("Failed to flatten the storage partition (%d) !", rc);
		return rc;
	}

	rc = flash_flatten(PARTITION_DEVICE(storage_partition_2),
			   PARTITION_OFFSET(storage_partition_2),
			   PARTITION_SIZE(storage_partition_2));
	if (rc < 0) {
		TC_PRINT("Failed to flatten the storage partition 2 (%d) !", rc);
		return rc;
	}

	return 0;
}

/* Low priority to ensure we run after any flash drivers are initialized */
SYS_INIT(erase_flash, POST_KERNEL, 100);

#endif /* !CONFIG_BUILD_WITH_TFM && CONFIG_FLASH_PAGE_LAYOUT && storage_partition */

ZTEST_SUITE(secure_storage_psa_ps, NULL, NULL, NULL, NULL, NULL);

#ifdef CONFIG_SECURE_STORAGE
#define MAX_DATA_SIZE CONFIG_SECURE_STORAGE_PS_MAX_DATA_SIZE
#else
#define MAX_DATA_SIZE CONFIG_TFM_PS_MAX_ASSET_SIZE
#endif

#define UID (psa_storage_uid_t)1

static void fill_data_buffer(uint8_t data[static MAX_DATA_SIZE])
{
	for (unsigned int i = 0; i != MAX_DATA_SIZE; ++i) {
		data[i] = i;
	}
}

ZTEST(secure_storage_psa_ps, test_all_sizes)
{
	psa_status_t ret;
	uint8_t written_data[MAX_DATA_SIZE];
	struct psa_storage_info_t info;
	uint8_t read_data[MAX_DATA_SIZE];
	size_t data_length;

	fill_data_buffer(written_data);

	for (unsigned int i = 0; i <= sizeof(written_data); ++i) {

		ret = psa_ps_set(UID, i, written_data, PSA_STORAGE_FLAG_NONE);
		zassert_equal(ret, PSA_SUCCESS);

		ret = psa_ps_get_info(UID, &info);
		zassert_equal(ret, PSA_SUCCESS);
		zassert_equal(info.flags, PSA_STORAGE_FLAG_NONE);
		zassert_equal(info.size, i);
		zassert_equal(info.capacity, i);

		ret = psa_ps_get(UID, 0, sizeof(read_data), read_data, &data_length);
		zassert_equal(ret, PSA_SUCCESS);
		zassert_equal(data_length, i);
		zassert_mem_equal(read_data, written_data, data_length);

		ret = psa_ps_remove(UID);
		zassert_equal(ret, PSA_SUCCESS);
		ret = psa_ps_get_info(UID, &info);
		zassert_equal(ret, PSA_ERROR_DOES_NOT_EXIST);
	}
}

ZTEST(secure_storage_psa_ps, test_all_offsets)
{
	psa_status_t ret;
	uint8_t written_data[MAX_DATA_SIZE];
	uint8_t read_data[MAX_DATA_SIZE];
	size_t data_length;

	fill_data_buffer(written_data);
	ret = psa_ps_set(UID, sizeof(written_data), written_data, PSA_STORAGE_FLAG_NONE);
	zassert_equal(ret, PSA_SUCCESS);

	for (unsigned int i = 0; i <= sizeof(read_data); ++i) {

		ret = psa_ps_get(UID, i, sizeof(read_data) - i, read_data, &data_length);
		zassert_equal(ret, PSA_SUCCESS);
		zassert_equal(data_length, sizeof(read_data) - i);

		zassert_mem_equal(read_data, written_data + i, data_length);
	}
}

ZTEST(secure_storage_psa_ps, test_max_num_entries)
{
	psa_status_t ret = PSA_SUCCESS;
	unsigned int i;
	struct psa_storage_info_t info;

	for (i = 0; ret == PSA_SUCCESS; ++i) {
		ret = psa_ps_set(UID + i, sizeof(i), &i, PSA_STORAGE_FLAG_NONE);
	}
	const unsigned int max_num_entries = i - 1;

	zassert_true(max_num_entries > 1);
	printk("Successfully wrote %u entries.\n", max_num_entries);
	zassert_equal(ret, PSA_ERROR_INSUFFICIENT_STORAGE);

	for (i = 0; i != max_num_entries; ++i) {
		unsigned int data;
		size_t data_length;

		ret = psa_ps_get(UID + i, 0, sizeof(data), &data, &data_length);
		zassert_equal(ret, PSA_SUCCESS);
		zassert_equal(data, i);
	}
	for (i = 0; i != max_num_entries; ++i) {
		ret = psa_ps_remove(UID + i);
		zassert_equal(ret, PSA_SUCCESS);
	}
	for (i = 0; i != max_num_entries; ++i) {
		ret = psa_ps_get_info(UID + i, &info);
		zassert_equal(ret, PSA_ERROR_DOES_NOT_EXIST);
	}
}

/* The flash must be erased between runs of this test for it to pass. */
ZTEST(secure_storage_psa_ps, test_write_once_flag)
{
	psa_status_t ret;
	/* Use a UID that isn't used in the other tests for the write-once entry. */
	const psa_storage_uid_t uid = 1 << 16;
	const uint8_t data[MAX_DATA_SIZE] = {};
	struct psa_storage_info_t info;

	ret = psa_ps_set(uid, sizeof(data), data, PSA_STORAGE_FLAG_WRITE_ONCE);
	zassert_equal(ret, PSA_SUCCESS, "%s%d",
		      (ret == PSA_ERROR_NOT_PERMITTED)
			      ? "Has the flash been erased since this test ran? "
			      : "",
		      ret);

	ret = psa_ps_get_info(uid, &info);
	zassert_equal(ret, PSA_SUCCESS);
	zassert_equal(info.flags, PSA_STORAGE_FLAG_WRITE_ONCE);

	ret = psa_ps_set(uid, sizeof(data), data, PSA_STORAGE_FLAG_NONE);
	zassert_equal(ret, PSA_ERROR_NOT_PERMITTED);

	ret = psa_ps_remove(uid);
	zassert_equal(ret, PSA_ERROR_NOT_PERMITTED);
}

/* The optional psa_ps_create() and psa_ps_set_extended() functions are not supported. */
ZTEST(secure_storage_psa_ps, test_set_extended_not_supported)
{
	psa_status_t ret;
	const psa_storage_uid_t uid = 1;
	const uint8_t data[MAX_DATA_SIZE] = {};
	struct psa_storage_info_t info;

	zassert_equal(psa_ps_get_support(), 0);

	ret = psa_ps_create(uid, sizeof(data), PSA_STORAGE_FLAG_NONE);
	zassert_equal(ret, PSA_ERROR_NOT_SUPPORTED);
	ret = psa_ps_get_info(uid, &info);
	zassert_equal(ret, PSA_ERROR_DOES_NOT_EXIST);

	ret = psa_ps_set(uid, sizeof(data), data, PSA_STORAGE_FLAG_NONE);
	zassert_equal(ret, PSA_SUCCESS);
	ret = psa_ps_set_extended(uid, 0, sizeof(data), data);
	zassert_equal(ret, PSA_ERROR_NOT_SUPPORTED);

	ret = psa_ps_remove(uid);
	zassert_equal(ret, PSA_SUCCESS);
}

#ifndef CONFIG_BUILD_WITH_TFM
/*
 * The following tests access the ITS entries and the PS storage medium directly to verify
 * the replay protection of PS. This is only possible when Secure Storage is used as
 * backend for both ITS and PS, not when TF-M is being used.
 */

#include "internal/zephyr/secure_storage/its.h"
#include "internal/zephyr/secure_storage/ps/store.h"

/* Tweak the replay protection values stored in ITS. */
ZTEST(secure_storage_psa_ps, test_its_replay_protection_tampering)
{
	psa_status_t ret;
	psa_storage_uid_t uid = 1;
	uint8_t data[MAX_DATA_SIZE];
	uint8_t check_data[MAX_DATA_SIZE];
	size_t check_data_length;
	uint8_t bad_replay_prot[CONFIG_SECURE_STORAGE_PS_REPLAY_PROTECTION_SIZE];
	uint8_t good_replay_protection[CONFIG_SECURE_STORAGE_PS_REPLAY_PROTECTION_SIZE];
	size_t replay_prot_length;

	/* First run: fill data with 0x1. */
	memset(data, 0x1, sizeof(data));
	ret = psa_ps_set(uid, sizeof(data), data, PSA_STORAGE_FLAG_NONE);
	zassert_equal(ret, PSA_SUCCESS);

	/* Dump the replay protection. This will be used as the "bad" one. */
	ret = secure_storage_its_get(SECURE_STORAGE_CALLER_PSA_PS, uid, 0,
				     sizeof(bad_replay_prot), bad_replay_prot, &replay_prot_length);
	zassert_equal(ret, PSA_SUCCESS);

	/* Write a new version with data buffer filled by 0x2. */
	memset(data, 0x2, sizeof(data));
	ret = psa_ps_set(uid, sizeof(data), data, PSA_STORAGE_FLAG_NONE);
	zassert_equal(ret, PSA_SUCCESS);

	/* Dump the replay protection value. This will be used as the "good" one. */
	ret = secure_storage_its_get(SECURE_STORAGE_CALLER_PSA_PS, uid, 0,
				     sizeof(good_replay_protection), good_replay_protection,
				     &replay_prot_length);
	zassert_equal(ret, PSA_SUCCESS);

	/* Forcedly update the replay protection value to a wrong value and read
	 * data back from PS. It will fail.
	 */
	ret = secure_storage_its_set(SECURE_STORAGE_CALLER_PSA_PS, uid, replay_prot_length,
				     bad_replay_prot, PSA_STORAGE_FLAG_NONE);
	zassert_equal(ret, PSA_SUCCESS);
	ret = psa_ps_get(uid, 0, sizeof(check_data), check_data, &check_data_length);
	zassert_equal(ret, PSA_ERROR_INVALID_SIGNATURE);

	/* Completely remove the replay protection field from ITS and read data back
	 * from PS. It will fail.
	 */
	ret = secure_storage_its_remove(SECURE_STORAGE_CALLER_PSA_PS, uid);
	zassert_equal(ret, PSA_SUCCESS);
	ret = psa_ps_get(uid, 0, sizeof(check_data), check_data, &check_data_length);
	zassert_equal(ret, PSA_ERROR_INVALID_SIGNATURE);

	/* Restore replay protection value in ITS and read data back from PS. It will
	 * succeed again.
	 */
	ret = secure_storage_its_set(SECURE_STORAGE_CALLER_PSA_PS, uid, replay_prot_length,
				     good_replay_protection, PSA_STORAGE_FLAG_NONE);
	zassert_equal(ret, PSA_SUCCESS);
	ret = psa_ps_get(uid, 0, sizeof(check_data), check_data, &check_data_length);
	zassert_equal(ret, PSA_SUCCESS);

	/* Remove the replay protection value from ITS again. The entry can still be
	 * removed and it no longer exists afterwards.
	 */
	ret = secure_storage_its_remove(SECURE_STORAGE_CALLER_PSA_PS, uid);
	zassert_equal(ret, PSA_SUCCESS);
	ret = psa_ps_remove(uid);
	zassert_equal(ret, PSA_SUCCESS);
	ret = psa_ps_get(uid, 0, sizeof(check_data), check_data, &check_data_length);
	zassert_equal(ret, PSA_ERROR_DOES_NOT_EXIST);
}

#define STORED_DATA_SIZE SECURE_STORAGE_PS_TRANSFORM_MAX_STORED_DATA_SIZE

/* Helper: get the data of a PS entry directly from the storage medium, bypassing the PS
 * implementation. This simulates an attacker that has access to the PS storage medium.
 */
static psa_status_t raw_ps_store_get(psa_storage_uid_t uid, size_t data_size, void *data,
				     size_t *data_length)
{
	secure_storage_uid_t ps_uid;

	zassert_equal(secure_storage_make_uid(SECURE_STORAGE_CALLER_PSA_PS, uid, &ps_uid),
		      PSA_SUCCESS);
	return secure_storage_ps_store_get(ps_uid, data_size, data, data_length);
}

/* Helper: set the data of a PS entry directly from the storage medium, bypassing the PS
 * implementation. This simulates an attacker that has access to the PS storage medium.
 */

static psa_status_t raw_ps_store_set(psa_storage_uid_t uid, size_t data_length,
				     const void *data)
{
	secure_storage_uid_t ps_uid;

	zassert_equal(secure_storage_make_uid(SECURE_STORAGE_CALLER_PSA_PS, uid, &ps_uid),
		      PSA_SUCCESS);
	return secure_storage_ps_store_set(ps_uid, data_length, data);
}

/* Helper: check that the provided PS entry has an invalid signature. */
static void assert_entry_tampered(psa_storage_uid_t uid)
{
	psa_status_t ret;
	uint8_t data[MAX_DATA_SIZE];
	size_t data_length;
	struct psa_storage_info_t info;

	ret = psa_ps_get(uid, 0, sizeof(data), data, &data_length);
	zassert_equal(ret, PSA_ERROR_INVALID_SIGNATURE);
	ret = psa_ps_get_info(uid, &info);
	zassert_equal(ret, PSA_ERROR_INVALID_SIGNATURE);
}

/* Helper: remove a PS entry and check that the entry is really removed from both PS and
 * ITS storages.
 */
static void assert_entry_removed(psa_storage_uid_t uid)
{
	psa_status_t ret;
	uint8_t data[MAX_DATA_SIZE];
	size_t data_length;
	uint8_t replay_prot[CONFIG_SECURE_STORAGE_PS_REPLAY_PROTECTION_SIZE];
	size_t replay_prot_length;

	ret = psa_ps_remove(uid);
	zassert_equal(ret, PSA_SUCCESS);
	ret = psa_ps_get(uid, 0, sizeof(data), data, &data_length);
	zassert_equal(ret, PSA_ERROR_DOES_NOT_EXIST);
	ret = raw_ps_store_get(uid, sizeof(data), data, &data_length);
	zassert_equal(ret, PSA_ERROR_DOES_NOT_EXIST);
	ret = secure_storage_its_get(SECURE_STORAGE_CALLER_PSA_PS, uid, 0, sizeof(replay_prot),
				     replay_prot, &replay_prot_length);
	zassert_equal(ret, PSA_ERROR_DOES_NOT_EXIST);
}

/* Restore an older version of an entry on the PS storage medium and check that:
 * - it correctly shows up as tampered;
 * - it can be removed from both PS and ITS.
 */
ZTEST(secure_storage_psa_ps, test_ps_rollback)
{
	psa_status_t ret;
	const psa_storage_uid_t uid = 1;
	uint8_t data[MAX_DATA_SIZE];
	uint8_t old_stored_data[STORED_DATA_SIZE];
	size_t old_stored_data_length;

	memset(data, 0x1, sizeof(data));
	ret = psa_ps_set(uid, sizeof(data), data, PSA_STORAGE_FLAG_NONE);
	zassert_equal(ret, PSA_SUCCESS);
	ret = raw_ps_store_get(uid, sizeof(old_stored_data), old_stored_data,
			       &old_stored_data_length);
	zassert_equal(ret, PSA_SUCCESS);

	memset(data, 0x2, sizeof(data));
	ret = psa_ps_set(uid, sizeof(data), data, PSA_STORAGE_FLAG_NONE);
	zassert_equal(ret, PSA_SUCCESS);

	ret = raw_ps_store_set(uid, old_stored_data_length, old_stored_data);
	zassert_equal(ret, PSA_SUCCESS);

	assert_entry_tampered(uid);
	assert_entry_removed(uid);
}

/* Corrupt a single bit of an entry on the PS storage medium and check that:
 * - it correctly shows up as tampered;
 * - it can be removed from both PS and ITS.
 */
ZTEST(secure_storage_psa_ps, test_ps_bit_flip)
{
	psa_status_t ret;
	const psa_storage_uid_t uid = 1;
	uint8_t data[MAX_DATA_SIZE];
	uint8_t stored_data[STORED_DATA_SIZE];
	size_t stored_data_length;

	memset(data, 0x1, sizeof(data));
	ret = psa_ps_set(uid, sizeof(data), data, PSA_STORAGE_FLAG_NONE);
	zassert_equal(ret, PSA_SUCCESS);

	ret = raw_ps_store_get(uid, sizeof(stored_data), stored_data, &stored_data_length);
	zassert_equal(ret, PSA_SUCCESS);
	stored_data[stored_data_length / 2] ^= 0x1;
	ret = raw_ps_store_set(uid, stored_data_length, stored_data);
	zassert_equal(ret, PSA_SUCCESS);

	assert_entry_tampered(uid);
	assert_entry_removed(uid);
}

/* Swap two entries on the PS storage medium and check that both:
 * - show up as tampered;
 * - can be removed from both PS and ITS.
 */
ZTEST(secure_storage_psa_ps, test_ps_swap)
{
	psa_status_t ret;
	const psa_storage_uid_t uid_a = 1;
	const psa_storage_uid_t uid_b = 2;
	uint8_t data[MAX_DATA_SIZE];
	uint8_t stored_data_a[STORED_DATA_SIZE];
	size_t stored_data_a_length;
	uint8_t stored_data_b[STORED_DATA_SIZE];
	size_t stored_data_b_length;

	memset(data, 0x1, sizeof(data));
	ret = psa_ps_set(uid_a, sizeof(data), data, PSA_STORAGE_FLAG_NONE);
	zassert_equal(ret, PSA_SUCCESS);
	memset(data, 0x2, sizeof(data));
	ret = psa_ps_set(uid_b, sizeof(data), data, PSA_STORAGE_FLAG_NONE);
	zassert_equal(ret, PSA_SUCCESS);

	ret = raw_ps_store_get(uid_a, sizeof(stored_data_a), stored_data_a,
			       &stored_data_a_length);
	zassert_equal(ret, PSA_SUCCESS);
	ret = raw_ps_store_get(uid_b, sizeof(stored_data_b), stored_data_b,
			       &stored_data_b_length);
	zassert_equal(ret, PSA_SUCCESS);
	ret = raw_ps_store_set(uid_a, stored_data_b_length, stored_data_b);
	zassert_equal(ret, PSA_SUCCESS);
	ret = raw_ps_store_set(uid_b, stored_data_a_length, stored_data_a);
	zassert_equal(ret, PSA_SUCCESS);

	assert_entry_tampered(uid_a);
	assert_entry_tampered(uid_b);
	assert_entry_removed(uid_a);
	assert_entry_removed(uid_b);
}
#endif /* CONFIG_BUILD_WITH_TFM */

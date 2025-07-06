/* Copyright (c) 2024 Nordic Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <psa/internal_trusted_storage.h>

/* The flash must be erased after this test suite is run for the write-once entry test to pass. */
ZTEST_SUITE(secure_storage_psa_its, NULL, NULL, NULL, NULL, NULL);

#ifdef CONFIG_SECURE_STORAGE
#define MAX_DATA_SIZE CONFIG_SECURE_STORAGE_ITS_MAX_DATA_SIZE
#else
#define MAX_DATA_SIZE CONFIG_TFM_ITS_MAX_ASSET_SIZE
#endif

#define UID (psa_storage_uid_t)1

static void fill_data_buffer(uint8_t data[static MAX_DATA_SIZE])
{
	for (unsigned int i = 0; i != MAX_DATA_SIZE; ++i) {
		data[i] = i;
	}
}

ZTEST(secure_storage_psa_its, test_all_sizes)
{
	psa_status_t ret;
	uint8_t written_data[MAX_DATA_SIZE];
	struct psa_storage_info_t info;
	uint8_t read_data[MAX_DATA_SIZE];
	size_t data_length;

	fill_data_buffer(written_data);

	for (unsigned int i = 0; i <= sizeof(written_data); ++i) {

		ret = psa_its_set(UID, i, written_data, PSA_STORAGE_FLAG_NONE);
		zassert_equal(ret, PSA_SUCCESS);

		ret = psa_its_get_info(UID, &info);
		zassert_equal(ret, PSA_SUCCESS);
		zassert_equal(info.flags, PSA_STORAGE_FLAG_NONE);
		zassert_equal(info.size, i);
		zassert_equal(info.capacity, i);

		ret = psa_its_get(UID, 0, sizeof(read_data), read_data, &data_length);
		zassert_equal(ret, PSA_SUCCESS);
		zassert_equal(data_length, i);
		zassert_mem_equal(read_data, written_data, data_length);

		ret = psa_its_remove(UID);
		zassert_equal(ret, PSA_SUCCESS);
		ret = psa_its_get_info(UID, &info);
		zassert_equal(ret, PSA_ERROR_DOES_NOT_EXIST);
	}
}

ZTEST(secure_storage_psa_its, test_all_offsets)
{
	psa_status_t ret;
	uint8_t written_data[MAX_DATA_SIZE];
	uint8_t read_data[MAX_DATA_SIZE];
	size_t data_length;

	fill_data_buffer(written_data);
	ret = psa_its_set(UID, sizeof(written_data), written_data, PSA_STORAGE_FLAG_NONE);
	zassert_equal(ret, PSA_SUCCESS);

	for (unsigned int i = 0; i <= sizeof(read_data); ++i) {

		ret = psa_its_get(UID, i, sizeof(read_data) - i, read_data, &data_length);
		zassert_equal(ret, PSA_SUCCESS);
		zassert_equal(data_length, sizeof(read_data) - i);

		zassert_mem_equal(read_data, written_data + i, data_length);
	}
}

ZTEST(secure_storage_psa_its, test_max_num_entries)
{
	psa_status_t ret = PSA_SUCCESS;
	unsigned int i;
	struct psa_storage_info_t info;

	for (i = 0; ret == PSA_SUCCESS; ++i) {
		ret = psa_its_set(UID + i, sizeof(i), &i, PSA_STORAGE_FLAG_NONE);
	}
	const unsigned int max_num_entries = i - 1;

	zassert_true(max_num_entries > 1);
	printk("Successfully wrote %u entries.\n", max_num_entries);
	zassert_equal(ret, PSA_ERROR_INSUFFICIENT_STORAGE);

	for (i = 0; i != max_num_entries; ++i) {
		unsigned int data;
		size_t data_length;

		ret = psa_its_get(UID + i, 0, sizeof(data), &data, &data_length);
		zassert_equal(ret, PSA_SUCCESS);
		zassert_equal(data, i);
	}
	for (i = 0; i != max_num_entries; ++i) {
		ret = psa_its_remove(UID + i);
		zassert_equal(ret, PSA_SUCCESS);
	}
	for (i = 0; i != max_num_entries; ++i) {
		ret = psa_its_get_info(UID + i, &info);
		zassert_equal(ret, PSA_ERROR_DOES_NOT_EXIST);
	}
}

/* The flash must be erased between runs of this test for it to pass. */
ZTEST(secure_storage_psa_its, test_write_once_flag)
{
	psa_status_t ret;
	/* Use a UID that isn't used in the other tests for the write-once entry. */
	const psa_storage_uid_t uid = 1 << 16;
	const uint8_t data[MAX_DATA_SIZE] = {};
	struct psa_storage_info_t info;

	ret = psa_its_set(uid, sizeof(data), data, PSA_STORAGE_FLAG_WRITE_ONCE);
	zassert_equal(ret, PSA_SUCCESS, "%s%d", (ret == PSA_ERROR_NOT_PERMITTED) ?
		      "Has the flash been erased since this test ran? " : "", ret);

	ret = psa_its_get_info(uid, &info);
	zassert_equal(ret, PSA_SUCCESS);
	zassert_equal(info.flags, PSA_STORAGE_FLAG_WRITE_ONCE);

	ret = psa_its_set(uid, sizeof(data), data, PSA_STORAGE_FLAG_NONE);
	zassert_equal(ret, PSA_ERROR_NOT_PERMITTED);

	ret = psa_its_remove(uid);
	zassert_equal(ret, PSA_ERROR_NOT_PERMITTED);
}

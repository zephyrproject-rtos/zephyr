/*
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>
#include <sys/printk.h>
#include <psa/storage_common.h>
#include <psa/protected_storage.h>

#define TEST_STRING_1 "The quick brown fox jumps over the lazy dog"
#define TEST_STRING_2 "Lorem ipsum dolor sit amet"

static psa_status_t sample_ps_try_remove(psa_storage_uid_t uid)
{
	psa_status_t status = PSA_SUCCESS;

	printk("Removing UID%lld\n", uid);

	status = psa_ps_remove(uid);
	if (status != PSA_SUCCESS) {
		printk("Failed to remove data! (%d)\n", status);
	}

	return status;
}

static psa_status_t sample_ps_try_get_info(psa_storage_uid_t uid)
{
	psa_status_t status = PSA_SUCCESS;
	struct psa_storage_info_t uid_info = { 0 };

	printk("Get Info UID%lld\n", uid);

	status = psa_ps_get_info(uid, &uid_info);
	if (status != PSA_SUCCESS) {
		printk("Failed to get info! (%d)\n", status);
	} else {
		printk("Info on data stored in UID%08llx\n", uid);
		printk("- Size: %d\n", uid_info.size);
		printk("- Capacity: 0x%2x\n", uid_info.capacity);
		printk("- Flags: 0x%2x\n", uid_info.flags);
	}

	return status;
}

static psa_status_t sample_ps_try_write(psa_storage_uid_t uid,
					psa_storage_create_flags_t uid_flags,
					uint8_t* buffer, size_t size)
{
	psa_status_t status = PSA_SUCCESS;

	printk("Writing data to UID%lld, Flags: %04x, Size: %d\n",
		uid, uid_flags, size);

	status = psa_ps_set(uid, size, buffer, uid_flags);
	if (status != PSA_SUCCESS) {
		printk("Failed to store data! (%d)\n", status);
	}

	return status;
}

static psa_status_t sample_ps_try_get_data(psa_storage_uid_t uid, uint8_t* data,
					   size_t size, size_t* bytes_read)
{
	psa_status_t status = PSA_SUCCESS;

	if (!data || !bytes_read || size == 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	printk("Get Data UID%lld\n", uid);

	status = psa_ps_get(uid, 0, size, data, bytes_read);
	if (status != PSA_SUCCESS) {
		printk("Failed to get data stored in UID1! (%d)\n", status);
	}

	return status;
}

void main(void)
{
	psa_status_t status = 0;
	size_t bytes_read = 0;
	uint8_t stored_data[sizeof(TEST_STRING_1)] = { 0 };

	printk("Protected Storage sample started.\n");
	printk("PSA Protected Storage API Version %d.%d\n",
			PSA_PS_API_VERSION_MAJOR, PSA_PS_API_VERSION_MINOR);

	if (sample_ps_try_remove(1) == PSA_ERROR_DOES_NOT_EXIST) {
		printk("Got expected error (PSA_ERROR_DOES_NOT_EXIST) when try remove UID\n");
	}
	printk("Writing data to UID1: %s\n", TEST_STRING_1);
	(void)sample_ps_try_write(1, PSA_STORAGE_FLAG_NONE, TEST_STRING_1, sizeof(TEST_STRING_1));
	(void)sample_ps_try_get_info(1);

	printk("Read and compare data stored in UID1\n");
	(void)sample_ps_try_get_data(1, stored_data, sizeof(stored_data), &bytes_read);
	printk("Data stored in UID1: %s\n", stored_data);
	(void)sample_ps_try_get_info(1);

	printk("Overwriting data stored in UID1: %s\n", TEST_STRING_2);
	(void)sample_ps_try_write(1, PSA_STORAGE_FLAG_NONE, TEST_STRING_2, sizeof(TEST_STRING_2));
	(void)sample_ps_try_get_info(1);

	printk("Writing data to UID2 with overwrite protection: %s\n", TEST_STRING_1);
	(void)sample_ps_try_write(2, PSA_STORAGE_FLAG_WRITE_ONCE, TEST_STRING_1, sizeof(TEST_STRING_1));
	(void)sample_ps_try_get_info(2);

	printk("Attempting to write '%s' to UID2\n", TEST_STRING_2);
	status = sample_ps_try_write(2, PSA_STORAGE_FLAG_WRITE_ONCE, TEST_STRING_2, sizeof(TEST_STRING_2));
	if (status != PSA_ERROR_NOT_PERMITTED) {
		printk("Got unexpected status when overwriting! (%d)\n", status);
		return;
	}
	(void)sample_ps_try_get_info(2);
	printk("Got expected error (PSA_ERROR_NOT_PERMITTED) when writing to protected UID\n");
	if (sample_ps_try_remove(2) != PSA_ERROR_NOT_PERMITTED) {
		printk("Got unexpected status when removing! (%d)\n", status);
		return;
	}
	printk("Got expected error (PSA_ERROR_NOT_PERMITTED) when try remove UID\n");

	(void)sample_ps_try_get_info(2);
	(void)sample_ps_try_remove(1);
}

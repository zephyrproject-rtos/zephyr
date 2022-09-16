/*
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <string.h>
#include <zephyr/sys/printk.h>
#include <psa/storage_common.h>
#include <psa/protected_storage.h>

#define TEST_STRING_1 "The quick brown fox jumps over the lazy dog"
#define TEST_STRING_2 "Lorem ipsum dolor sit amet"

void main(void)
{
	psa_status_t status = 0;

	printk("Protected Storage sample started.\n");
	printk("PSA Protected Storage API Version %d.%d\n",
			PSA_PS_API_VERSION_MAJOR, PSA_PS_API_VERSION_MINOR);

	printk("Writing data to UID1: %s\n", TEST_STRING_1);
	psa_storage_uid_t uid1 = 1;
	psa_storage_create_flags_t uid1_flag = PSA_STORAGE_FLAG_NONE;

	status = psa_ps_set(uid1, sizeof(TEST_STRING_1), TEST_STRING_1, uid1_flag);
	if (status != PSA_SUCCESS) {
		printk("Failed to store data! (%d)\n", status);
		return;
	}

	/* Get info on UID1 */
	struct psa_storage_info_t uid1_info;

	status = psa_ps_get_info(uid1, &uid1_info);
	if (status != PSA_SUCCESS) {
		printk("Failed to get info! (%d)\n", status);
		return;
	}
	printk("Info on data stored in UID1:\n");
	printk("- Size: %d\n", uid1_info.size);
	printk("- Capacity: 0x%2x\n", uid1_info.capacity);
	printk("- Flags: 0x%2x\n", uid1_info.flags);

	printk("Read and compare data stored in UID1\n");
	size_t bytes_read;
	char stored_data[sizeof(TEST_STRING_1)];

	status = psa_ps_get(uid1, 0, sizeof(TEST_STRING_1), &stored_data, &bytes_read);
	if (status != PSA_SUCCESS) {
		printk("Failed to get data stored in UID1! (%d)\n", status);
		return;
	}
	printk("Data stored in UID1: %s\n", stored_data);

	printk("Overwriting data stored in UID1: %s\n", TEST_STRING_2);
	status = psa_ps_set(uid1, sizeof(TEST_STRING_2), TEST_STRING_2, uid1_flag);
	if (status != PSA_SUCCESS) {
		printk("Failed to overwrite UID1! (%d)\n", status);
		return;
	}

	printk("Writing data to UID2 with overwrite protection: %s\n", TEST_STRING_1);
	psa_storage_uid_t uid2 = 2;
	psa_storage_create_flags_t uid2_flag = PSA_STORAGE_FLAG_WRITE_ONCE;

	status = psa_ps_set(uid2, sizeof(TEST_STRING_1), TEST_STRING_1, uid2_flag);
	if (status != PSA_SUCCESS) {
		printk("Failed to set write once flag! (%d)\n", status);
		return;
	}

	printk("Attempting to write '%s' to UID2\n", TEST_STRING_2);
	status = psa_ps_set(uid2, sizeof(TEST_STRING_2), TEST_STRING_2, uid2_flag);
	if (status != PSA_ERROR_NOT_PERMITTED) {
		printk("Got unexpected status when overwriting! (%d)\n", status);
		return;
	}
	printk("Got expected error (PSA_ERROR_NOT_PERMITTED) when writing to protected UID\n");

	printk("Removing UID1\n");
	status = psa_ps_remove(uid1);
	if (status != PSA_SUCCESS) {
		printk("Failed to remove UID1! (%d)\n", status);
		return;
	}
}

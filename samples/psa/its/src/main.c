/* Copyright (c) 2024 Nordic Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */
#include <psa/internal_trusted_storage.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(psa_its);

#define SAMPLE_DATA_UID (psa_storage_uid_t)1
#define SAMPLE_DATA_FLAGS PSA_STORAGE_FLAG_NONE

#ifdef CONFIG_SECURE_STORAGE
#define SAMPLE_DATA_SIZE CONFIG_SECURE_STORAGE_ITS_MAX_DATA_SIZE
#else
#define SAMPLE_DATA_SIZE 128
#endif

static int write_and_read_data(void)
{
	LOG_INF("Writing to and reading back from ITS...");
	psa_status_t ret;

	/* Data to be written to ITS. */
	uint8_t p_data_write[SAMPLE_DATA_SIZE];

	for (unsigned int i = 0; i != sizeof(p_data_write); ++i) {
		p_data_write[i] = i;
	}

	ret = psa_its_set(SAMPLE_DATA_UID, sizeof(p_data_write), p_data_write, SAMPLE_DATA_FLAGS);
	if (ret != PSA_SUCCESS) {
		LOG_ERR("Writing the data to ITS failed. (%d)", ret);
		return -1;
	}

	/* Data to be read from ITS. */
	uint8_t p_data_read[SAMPLE_DATA_SIZE];

	/* Read back the data starting from an offset. */
	const size_t data_offset = SAMPLE_DATA_SIZE / 2;

	/* Number of bytes read. */
	size_t p_data_length = 0;

	ret = psa_its_get(SAMPLE_DATA_UID, data_offset, sizeof(p_data_read), p_data_read,
			  &p_data_length);
	if (ret != PSA_SUCCESS) {
		LOG_ERR("Reading back the data from ITS failed. (%d).", ret);
		return -1;
	}

	if (p_data_length != SAMPLE_DATA_SIZE - data_offset) {
		LOG_ERR("Unexpected amount of bytes read back. (%zu != %zu)",
			p_data_length, SAMPLE_DATA_SIZE - data_offset);
		return -1;
	}

	if (memcmp(p_data_write + data_offset, p_data_read, p_data_length)) {
		LOG_HEXDUMP_INF(p_data_write + data_offset, p_data_length, "Data written:");
		LOG_HEXDUMP_INF(p_data_read, p_data_length, "Data read back:");
		LOG_ERR("The data read back doesn't match the data written.");
		return -1;
	}

	LOG_INF("Successfully wrote to ITS and read back what was written.");
	return 0;
}

static int read_info(void)
{
	LOG_INF("Reading the written entry's metadata...");
	psa_status_t ret;

	/* The entry's metadata. */
	struct psa_storage_info_t p_info;

	ret = psa_its_get_info(SAMPLE_DATA_UID, &p_info);
	if (ret != PSA_SUCCESS) {
		LOG_ERR("Failed to retrieve the entry's metadata. (%d)", ret);
		return -1;
	}

	if (p_info.capacity != SAMPLE_DATA_SIZE
	 || p_info.size != SAMPLE_DATA_SIZE
	 || p_info.flags != SAMPLE_DATA_FLAGS) {
		LOG_ERR("Entry metadata unexpected. (capacity:%zu size:%zu flags:0x%x)",
			p_info.capacity, p_info.size, p_info.flags);
		return -1;
	}

	LOG_INF("Successfully read the entry's metadata.");
	return 0;
}

static int remove_entry(void)
{
	LOG_INF("Removing the entry from ITS...");
	psa_status_t ret;

	ret = psa_its_remove(SAMPLE_DATA_UID);
	if (ret != PSA_SUCCESS) {
		LOG_ERR("Failed to remove the entry. (%d)", ret);
		return -1;
	}

	LOG_INF("Entry removed from ITS.");
	return 0;
}

int main(void)
{
	LOG_INF("PSA ITS sample started.");

	if (write_and_read_data()) {
		return -1;
	}

	if (read_info()) {
		return -1;
	}

	if (remove_entry()) {
		return -1;
	}

	LOG_INF("Sample finished successfully.");
	return 0;
}

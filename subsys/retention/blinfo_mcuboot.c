/*
 * Copyright (c) 2023, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <limits.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include <zephyr/retention/retention.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zephyr/retention/blinfo.h>
#include <bootutil/boot_status.h>

LOG_MODULE_REGISTER(blinfo_mcuboot, CONFIG_RETENTION_LOG_LEVEL);

static const struct device *bootloader_info_dev =
					DEVICE_DT_GET(DT_CHOSEN(zephyr_bootloader_info));

#if !defined(CONFIG_RETENTION_BOOTLOADER_INFO_OUTPUT_FUNCTION)
static
#endif
int blinfo_lookup(uint16_t key, char *val, int val_len_max)
{
	struct shared_data_tlv_header header;
	struct shared_data_tlv_entry tlv_entry = {0};
	uintptr_t offset = SHARED_DATA_HEADER_SIZE;
	int rc;

	rc = retention_read(bootloader_info_dev, 0, (void *)&header, sizeof(header));

	if (rc != 0) {
		return rc;
	}

	/* Iterate over the whole shared MCUboot data section and look for a TLV with
	 * the required tag.
	 */
	while (offset < header.tlv_tot_len) {
		rc = retention_read(bootloader_info_dev, offset, (void *)&tlv_entry,
				    sizeof(tlv_entry));

		if (rc != 0) {
			return rc;
		}

		if (GET_MAJOR(tlv_entry.tlv_type) == TLV_MAJOR_BLINFO &&
		    GET_MINOR(tlv_entry.tlv_type) == key) {
			/* Return an error if the provided buffer is too small to fit the
			 * value in it, bootloader values are small and concise and should
			 * be able to fit in a single buffer.
			 */
			if (tlv_entry.tlv_len > val_len_max) {
				return -EOVERFLOW;
			}

			offset += SHARED_DATA_ENTRY_HEADER_SIZE;
			rc = retention_read(bootloader_info_dev, offset, val,
					    tlv_entry.tlv_len);

			if (rc != 0) {
				return rc;
			}

			return tlv_entry.tlv_len;
		}

		offset += SHARED_DATA_ENTRY_SIZE(tlv_entry.tlv_len);
	}

	/* Return IO error as a valid key name was provided but the TLV was not found in
	 * the shared data section.
	 */
	return -EIO;
}

#if defined(CONFIG_RETENTION_BOOTLOADER_INFO_OUTPUT_SETTINGS)
static int blinfo_handle_get(const char *name, char *val, int val_len_max);
static int blinfo_handle_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg);

static struct settings_handler blinfo_handler = {
	.name = "blinfo",
	.h_get = blinfo_handle_get,
	.h_set = blinfo_handle_set,
};

static int blinfo_handle_get(const char *name, char *val, int val_len_max)
{
	const char *next;
	uint16_t index;

	/* Allowed keys are mode, signature_type, recovery, running_slot, bootloader_version
	 * and max_application_size which cannot contain any additional entries
	 */
	if (settings_name_steq(name, "mode", &next) && !next) {
		index = BLINFO_MODE;
	} else if (settings_name_steq(name, "signature_type", &next) && !next) {
		index = BLINFO_SIGNATURE_TYPE;
	} else if (settings_name_steq(name, "recovery", &next) && !next) {
		index = BLINFO_RECOVERY;
	} else if (settings_name_steq(name, "running_slot", &next) && !next) {
		index = BLINFO_RUNNING_SLOT;
	} else if (settings_name_steq(name, "bootloader_version", &next) && !next) {
		index = BLINFO_BOOTLOADER_VERSION;
	} else if (settings_name_steq(name, "max_application_size", &next) && !next) {
		index = BLINFO_MAX_APPLICATION_SIZE;
	} else {
		return -ENOENT;
	}

	return blinfo_lookup(index, val, val_len_max);
}

static int blinfo_handle_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	return -ENOTSUP;
}
#endif

static int blinfo_init(void)
{
	int rc;

	rc = retention_is_valid(bootloader_info_dev);

	if (rc == 1 || rc == -ENOTSUP) {
		struct shared_data_tlv_header header;

		rc = retention_read(bootloader_info_dev, 0, (void *)&header, sizeof(header));

		if (rc == 0 && header.tlv_magic != SHARED_DATA_TLV_INFO_MAGIC) {
			/* Unknown data present */
			LOG_ERR("MCUboot data load failed, expected magic value: 0x%x, got: 0x%x",
				SHARED_DATA_TLV_INFO_MAGIC, header.tlv_magic);
			rc = -EINVAL;
		}
	}

#if defined(CONFIG_RETENTION_BOOTLOADER_INFO_OUTPUT_SETTINGS)
	if (rc == 0) {
		settings_register(&blinfo_handler);
	}
#endif

	return rc;
}

SYS_INIT(blinfo_init, APPLICATION, CONFIG_RETENTION_BOOTLOADER_INFO_INIT_PRIORITY);

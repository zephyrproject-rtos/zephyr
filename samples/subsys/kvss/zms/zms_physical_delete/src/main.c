/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * ZMS physical delete sample.
 * Stores several versions of a secret, deletes it with zms_delete() and shows that the data is
 * still present in flash, then deletes it with zms_delete_physical() and shows that it is gone.
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/kvss/zms.h>

#define ZMS_PARTITION        storage_partition
#define ZMS_PARTITION_DEVICE PARTITION_DEVICE(ZMS_PARTITION)
#define ZMS_PARTITION_OFFSET PARTITION_OFFSET(ZMS_PARTITION)

#define SECTOR_COUNT 3
#define SECRET_ID    1
#define OTHER_ID     2
#define VERSIONS     3

static struct zms_fs fs;

/* Count how many times a byte pattern appears in the ZMS partition */
static int count_in_flash(const uint8_t *pattern, size_t plen, uint32_t *count)
{
	uint8_t buf[64];
	off_t end = fs.offset + (off_t)fs.sector_size * fs.sector_count;
	int rc;

	*count = 0;
	for (off_t off = fs.offset; off + plen <= end; off++) {
		rc = flash_read(fs.flash_device, off, buf, plen);
		if (rc) {
			return rc;
		}
		if (memcmp(buf, pattern, plen) == 0) {
			(*count)++;
		}
	}

	return 0;
}

static int count_secrets(uint32_t *total)
{
	uint8_t secret[32];
	uint32_t count;
	int rc;

	*total = 0;
	for (int i = 1; i <= VERSIONS; i++) {
		memset(secret, 0xA0 + i, sizeof(secret));
		rc = count_in_flash(secret, 16, &count);
		if (rc) {
			return rc;
		}
		*total += count;
	}

	return 0;
}

int main(void)
{
	struct flash_pages_info info;
	uint8_t secret[32];
	uint8_t other[32];
	uint8_t rd[32];
	uint32_t found;
	int rc;

	fs.flash_device = ZMS_PARTITION_DEVICE;
	if (!device_is_ready(fs.flash_device)) {
		printk("Flash device not ready\n");
		return 0;
	}

	fs.offset = ZMS_PARTITION_OFFSET;
	rc = flash_get_page_info_by_offs(fs.flash_device, fs.offset, &info);
	if (rc) {
		printk("Unable to get page info: %d\n", rc);
		return 0;
	}
	fs.sector_size = info.size;
	fs.sector_count = SECTOR_COUNT;

	rc = zms_mount(&fs);
	if (rc) {
		printk("FAIL: mount failed: %d\n", rc);
		return 0;
	}
	rc = zms_clear(&fs);
	if (rc) {
		printk("FAIL: clear failed: %d\n", rc);
		return 0;
	}
	rc = zms_mount(&fs);
	if (rc) {
		printk("FAIL: re-mount failed: %d\n", rc);
		return 0;
	}

	/* Several versions of the secret and one unrelated entry */
	memset(other, 0x55, sizeof(other));
	rc = zms_write(&fs, OTHER_ID, other, sizeof(other));
	if (rc != sizeof(other)) {
		printk("FAIL: write failed: %d\n", rc);
		return 0;
	}
	for (int i = 1; i <= VERSIONS; i++) {
		memset(secret, 0xA0 + i, sizeof(secret));
		rc = zms_write(&fs, SECRET_ID, secret, sizeof(secret));
		if (rc != sizeof(secret)) {
			printk("FAIL: write failed: %d\n", rc);
			return 0;
		}
	}
	rc = count_secrets(&found);
	if (rc || found == 0) {
		printk("FAIL: secret not found in flash: %d\n", rc);
		return 0;
	}
	printk("%d versions written, %u copies of the secret in flash\n", VERSIONS, found);

	/* A regular delete only hides the entry */
	rc = zms_delete(&fs, SECRET_ID);
	if (rc) {
		printk("FAIL: delete failed: %d\n", rc);
		return 0;
	}
	rc = count_secrets(&found);
	if (rc || found == 0) {
		printk("FAIL: regular delete is expected to leave the data: %d\n", rc);
		return 0;
	}
	printk("After zms_delete(): %u copies of the secret still in flash\n", found);

	/* The physical delete overwrites every version */
	rc = zms_delete_physical(&fs, SECRET_ID);
	if (rc) {
		printk("FAIL: physical delete failed: %d\n", rc);
		return 0;
	}
	rc = count_secrets(&found);
	if (rc || found != 0) {
		printk("FAIL: %u copies of the secret left: %d\n", found, rc);
		return 0;
	}
	printk("After zms_delete_physical(): %u copies of the secret in flash\n", found);

	rc = zms_read(&fs, SECRET_ID, rd, sizeof(rd));
	if (rc != -ENOENT) {
		printk("FAIL: deleted entry can still be read: %d\n", rc);
		return 0;
	}
	rc = zms_read(&fs, OTHER_ID, rd, sizeof(rd));
	if (rc != sizeof(other) || memcmp(rd, other, sizeof(other)) != 0) {
		printk("FAIL: unrelated entry was damaged: %d\n", rc);
		return 0;
	}

	printk("PASS: physical delete succeeded\n");

	return 0;
}

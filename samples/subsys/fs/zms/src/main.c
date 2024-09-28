/*
 * Copyright (c) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * ZMS Sample for Zephyr using high level API.
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/device.h>
#include <string.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/zms.h>

static struct zms_fs fs;

#define ZMS_PARTITION        storage_partition
#define ZMS_PARTITION_DEVICE FIXED_PARTITION_DEVICE(ZMS_PARTITION)
#define ZMS_PARTITION_OFFSET FIXED_PARTITION_OFFSET(ZMS_PARTITION)

#define IP_ADDRESS_ID 1
#define KEY_VALUE_ID  0xbeefdead
#define CNT_ID        2
#define LONG_DATA_ID  3

#define MAX_ITERATIONS   300
#define DELETE_ITERATION 10

static int delete_and_verify_items(struct zms_fs *fs, uint32_t id)
{
	int rc = 0;

	rc = zms_delete(fs, id);
	if (rc) {
		goto error1;
	}
	rc = zms_get_data_length(fs, id);
	if (rc > 0) {
		goto error2;
	}

	return 0;
error1:
	printk("Error while deleting item rc=%d\n", rc);
	return rc;
error2:
	printk("Error, Delete failed item should not be present\n");
	return -1;
}

static int delete_basic_items(struct zms_fs *fs)
{
	int rc = 0;

	rc = delete_and_verify_items(fs, IP_ADDRESS_ID);
	if (rc) {
		printk("Error while deleting item %x rc=%d\n", IP_ADDRESS_ID, rc);
		return rc;
	}
	rc = delete_and_verify_items(fs, KEY_VALUE_ID);
	if (rc) {
		printk("Error while deleting item %x rc=%d\n", KEY_VALUE_ID, rc);
		return rc;
	}
	rc = delete_and_verify_items(fs, CNT_ID);
	if (rc) {
		printk("Error while deleting item %x rc=%d\n", CNT_ID, rc);
		return rc;
	}
	rc = delete_and_verify_items(fs, LONG_DATA_ID);
	if (rc) {
		printk("Error while deleting item %x rc=%d\n", LONG_DATA_ID, rc);
	}

	return rc;
}

int main(void)
{
	int rc = 0;
	char buf[16];
	uint8_t key[8] = {0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF}, longarray[128];
	uint32_t i_cnt = 0U, i;
	uint32_t id = 0;
	ssize_t free_space = 0;
	struct flash_pages_info info;

	for (int n = 0; n < sizeof(longarray); n++) {
		longarray[n] = n;
	}

	/* define the zms file system by settings with:
	 *	sector_size equal to the pagesize,
	 *	3 sectors
	 *	starting at ZMS_PARTITION_OFFSET
	 */
	fs.flash_device = ZMS_PARTITION_DEVICE;
	if (!device_is_ready(fs.flash_device)) {
		printk("Storage device %s is not ready\n", fs.flash_device->name);
		return 0;
	}
	fs.offset = ZMS_PARTITION_OFFSET;
	rc = flash_get_page_info_by_offs(fs.flash_device, fs.offset, &info);
	if (rc) {
		printk("Unable to get page info, rc=%d\n", rc);
		return 0;
	}
	fs.sector_size = info.size;
	fs.sector_count = 3U;

	for (i = 0; i < MAX_ITERATIONS; i++) {
		rc = zms_mount(&fs);
		if (rc) {
			printk("Storage Init failed, rc=%d\n", rc);
			return 0;
		}

		printk("ITERATION: %u\n", i);
		/* IP_ADDRESS_ID is used to store an address, lets see if we can
		 * read it from flash, since we don't know the size read the
		 * maximum possible
		 */
		rc = zms_read(&fs, IP_ADDRESS_ID, &buf, sizeof(buf));
		if (rc > 0) {
			/* item was found, show it */
			buf[rc] = '\0';
			printk("ID: %u, IP Address: %s\n", IP_ADDRESS_ID, buf);
		}
		/* Rewriting ADDRESS IP even if we found it */
		strncpy(buf, "172.16.254.1", sizeof(buf) - 1);
		printk("Adding IP_ADDRESS %s at id %u\n", buf, IP_ADDRESS_ID);
		rc = zms_write(&fs, IP_ADDRESS_ID, &buf, strlen(buf));
		if (rc < 0) {
			printk("Error while writing Entry rc=%d\n", rc);
			break;
		}

		/* KEY_VALUE_ID is used to store a key/value pair , lets see if we can read
		 * it from storage.
		 */
		rc = zms_read(&fs, KEY_VALUE_ID, &key, sizeof(key));
		if (rc > 0) { /* item was found, show it */
			printk("Id: %x, Key: ", KEY_VALUE_ID);
			for (int n = 0; n < 8; n++) {
				printk("%x ", key[n]);
			}
			printk("\n");
		}
		/* Rewriting KEY_VALUE even if we found it */
		printk("Adding key/value at id %x\n", KEY_VALUE_ID);
		rc = zms_write(&fs, KEY_VALUE_ID, &key, sizeof(key));
		if (rc < 0) {
			printk("Error while writing Entry rc=%d\n", rc);
			break;
		}

		/* CNT_ID is used to store the loop counter, lets see
		 * if we can read it from storage
		 */
		rc = zms_read(&fs, CNT_ID, &i_cnt, sizeof(i_cnt));
		if (rc > 0) { /* item was found, show it */
			printk("Id: %d, loop_cnt: %u\n", CNT_ID, i_cnt);
			if (i_cnt != (i - 1)) {
				break;
			}
		}
		printk("Adding counter at id %u\n", CNT_ID);
		rc = zms_write(&fs, CNT_ID, &i, sizeof(i));
		if (rc < 0) {
			printk("Error while writing Entry rc=%d\n", rc);
			break;
		}

		/* LONG_DATA_ID is used to store a larger dataset ,lets see if we can read
		 * it from flash
		 */
		rc = zms_read(&fs, LONG_DATA_ID, &longarray, sizeof(longarray));
		if (rc > 0) {
			/* item was found, show it */
			printk("Id: %d, Longarray: ", LONG_DATA_ID);
			for (int n = 0; n < sizeof(longarray); n++) {
				printk("%x ", longarray[n]);
			}
			printk("\n");
		}
		/* Rewrite the entry even if we found it */
		printk("Adding Longarray at id %d\n", LONG_DATA_ID);
		rc = zms_write(&fs, LONG_DATA_ID, &longarray, sizeof(longarray));
		if (rc < 0) {
			printk("Error while writing Entry rc=%d\n", rc);
			break;
		}

		/* Each DELETE_ITERATION delete all basic items */
		if (!(i % DELETE_ITERATION) && (i)) {
			rc = delete_basic_items(&fs);
			if (rc) {
				break;
			}
		}
	}

	if (i != MAX_ITERATIONS) {
		printk("Error: Something went wrong at iteration %u rc=%d\n", i - 1, rc);
		return 0;
	}

	while (1) {
		/* fill all storage */
		rc = zms_write(&fs, id, &id, sizeof(uint32_t));
		if (rc < 0) {
			break;
		}
		id++;
	}

	if (rc == -ENOSPC) {
		/* Calculate free space and verify that it is 0 */
		free_space = zms_calc_free_space(&fs);
		if (free_space < 0) {
			printk("Error while computing free space, rc=%d\n", free_space);
			return 0;
		}
		if (free_space > 0) {
			printk("Error: free_space should be 0, computed %u\n", free_space);
			return 0;
		}
		printk("Memory is full let's delete all items\n");

		/* Now delete all previously written items */
		for (uint32_t n = 0; n < id; n++) {
			rc = delete_and_verify_items(&fs, n);
			if (rc) {
				printk("Error deleting at id %u\n", n);
				return 0;
			}
		}
		rc = delete_basic_items(&fs);
		if (rc) {
			printk("Error deleting basic items\n");
			return 0;
		}
	}

	/*
	 * Let's compute free space in storage. But before doing that let's Garbage collect
	 * all sectors where we deleted all entries and then compute the free space
	 */
	for (uint32_t i = 0; i < fs.sector_count; i++) {
		rc = zms_sector_use_next(&fs);
		if (rc) {
			printk("Error while changing sector rc=%d\n", rc);
		}
	}
	free_space = zms_calc_free_space(&fs);
	if (free_space < 0) {
		printk("Error while computing free space, rc=%d\n", free_space);
		return 0;
	}
	printk("Free space in storage is %u bytes\n", free_space);
	printk("Sample code finished Successfully\n");

	return 0;
}

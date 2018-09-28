/*
 * NVS Sample for Zephyr using high level API, the sample illustrates the usage
 * of NVS for storing data of different kind (strings, binary blobs, unsigned
 * 32 bit integer) and also how to read them back from flash. The reading of
 * data is illustrated for both a basic read (latest added value) as well as
 * reading back the history of data (previously added values). Next to reading
 * and writing data it also shows how data can be deleted from flash.
 *
 * The sample stores the following items:
 * 1. A string representing an IP-address: stored at id=1, data="192.168.1.1"
 * 2. A binary blob representing a key: stored at id=2, data=FF FE FD FC FB FA
 *    F9 F8
 * 3. A reboot counter (32bit): stored at id=3, data=reboot_counter
 * 4. A string: stored at id=4, data="DATA" (used to illustrate deletion of
 * items)
 *
 * At first boot the sample checks if the data is available in flash and adds
 * the items if they are not in flash.
 *
 * Every reboot increases the values of the reboot_counter and updates it in
 * flash.
 *
 * At the 10th reboot the string item with id=4 is deleted (or marked for
 * deletion).
 *
 * At the 11th reboot the istring tem with id=4 can no longer be read with the
 * basic nvs_read() function as it has been deleted. It is possible to read the
 * value with nvs_read_hist()
 *
 * At the 78th reboot the first sector is full and a new sector is taken into
 * use. The data with id=1, id=2 and id=3 is copied to the new sector. As a
 * result of this the history of the reboot_counter will be removed but the
 * latest values of address, key and reboot_counter is kept.
 *
 * Copyright (c) 2018 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr.h>
#include <misc/reboot.h>
#include <board.h>
#include <device.h>
#include <string.h>
#include <nvs/nvs.h>

#define NVS_SECTOR_SIZE FLASH_ERASE_BLOCK_SIZE /* Multiple of FLASH_PAGE_SIZE */
#define NVS_SECTOR_COUNT 3 /* At least 2 sectors */
#define NVS_STORAGE_OFFSET FLASH_AREA_STORAGE_OFFSET /* Start address of the
						      * filesystem in flash
						      */
static struct nvs_fs fs = {
	.sector_size = NVS_SECTOR_SIZE,
	.sector_count = NVS_SECTOR_COUNT,
	.offset = NVS_STORAGE_OFFSET,
};

/* 1000 msec = 1 sec */
#define SLEEP_TIME      100
/* maximum reboot counts, make high enough to trigger sector change (buffer */
/* rotation). */
#define MAX_REBOOT 400

#define ADDRESS_ID 1
#define KEY_ID 2
#define RBT_CNT_ID 3
#define STRING_ID 4
#define LONG_ID 5


void main(void)
{
	int rc = 0, cnt = 0, cnt_his = 0;
	char buf[16];
	u8_t key[8], longarray[128];
	u32_t reboot_counter = 0, reboot_counter_his;

	rc = nvs_init(&fs, FLASH_DEV_NAME);
	if (rc) {
		printk("Flash Init failed\n");
	}

	/* ADDRESS_ID is used to store an address, lets see if we can
	 * read it from flash, since we don't know the size read the
	 * maximum possible
	 */
	rc = nvs_read(&fs, ADDRESS_ID, &buf, sizeof(buf));
	if (rc > 0) { /* item was found, show it */
		printk("Id: %d, Address: %s\n", ADDRESS_ID, buf);
	} else   {/* item was not found, add it */
		strcpy(buf, "192.168.1.1");
		printk("No address found, adding %s at id %d\n", buf,
		       ADDRESS_ID);
		nvs_write(&fs, ADDRESS_ID, &buf, strlen(buf)+1);
	}
	/* KEY_ID is used to store a key, lets see if we can read it from flash
	 */
	rc = nvs_read(&fs, KEY_ID, &key, sizeof(key));
	if (rc > 0) { /* item was found, show it */
		printk("Id: %d, Key: ", KEY_ID);
		for (int n = 0; n < 8; n++) {
			printk("%x ", key[n]);
		}
		printk("\n");
	} else   {/* item was not found, add it */
		printk("No key found, adding it at id %d\n", KEY_ID);
		key[0] = 0xFF;
		key[1] = 0xFE;
		key[2] = 0xFD;
		key[3] = 0xFC;
		key[4] = 0xFB;
		key[5] = 0xFA;
		key[6] = 0xF9;
		key[7] = 0xF8;
		nvs_write(&fs, KEY_ID, &key, sizeof(key));
	}
	/* RBT_CNT_ID is used to store the reboot counter, lets see
	 * if we can read it from flash
	 */
	rc = nvs_read(&fs, RBT_CNT_ID, &reboot_counter, sizeof(reboot_counter));
	if (rc > 0) { /* item was found, show it */
		printk("Id: %d, Reboot_counter: %d\n",
			RBT_CNT_ID, reboot_counter);
	} else   {/* item was not found, add it */
		printk("No Reboot counter found, adding it at id %d\n",
		       RBT_CNT_ID);
		nvs_write(&fs, RBT_CNT_ID, &reboot_counter,
			  sizeof(reboot_counter));
	}
	/* STRING_ID is used to store data that will be deleted,lets see
	 * if we can read it from flash, since we don't know the size read the
	 * maximum possible
	 */
	rc = nvs_read(&fs, STRING_ID, &buf, sizeof(buf));
	if (rc > 0) {
		/* item was found, show it */
		printk("Id: %d, Data: %s\n",
			STRING_ID, buf);
		/* remove the item if reboot_counter = 10 */
		if (reboot_counter == 10) {
			nvs_delete(&fs, STRING_ID);
		}
	} else   {
		/* entry was not found, add it if reboot_counter = 0*/
		if (reboot_counter == 0) {
			printk("Id: %d not found, adding it\n",
			STRING_ID);
			strcpy(buf, "DATA");
			nvs_write(&fs, STRING_ID, &buf, strlen(buf) + 1);
		}
	}

	/* LONG_ID is used to store a larger dataset ,lets see if we can read
	 * it from flash
	 */
	rc = nvs_read(&fs, LONG_ID, &longarray, sizeof(longarray));
	if (rc > 0) {
		/* item was found, show it */
		printk("Id: %d, Longarray: ", LONG_ID);
		for (int n = 0; n < sizeof(longarray); n++) {
			printk("%x ", longarray[n]);
		}
		printk("\n");
	} else   {
		/* entry was not found, add it if reboot_counter = 0*/
		if (reboot_counter == 0) {
			printk("Longarray not found, adding it as id %d\n",
			       LONG_ID);
			for (int n = 0; n < sizeof(longarray); n++) {
				longarray[n] = n;
			}
			nvs_write(&fs, LONG_ID, &longarray, sizeof(longarray));
		}
	}

	cnt = 5;
	while (1) {
		k_sleep(SLEEP_TIME);
		if (reboot_counter < MAX_REBOOT) {
			if (cnt == 5) {
				/* print some history information about
				 * the reboot counter
				 * Check the counter history in flash
				 */
				printk("Reboot counter history: ");
				while (1) {
					rc = nvs_read_hist(
						&fs, RBT_CNT_ID,
						&reboot_counter_his,
						sizeof(reboot_counter_his),
						cnt_his);
					if (rc < 0) {
						break;
					}
					printk("...%d", reboot_counter_his);
					cnt_his++;
				}
				if (cnt_his == 0) {
					printk("\n Error, no Reboot counter");
				} else {
					printk("\nOldest reboot counter: %d",
					       reboot_counter_his);
				}
				printk("\nRebooting in ");
			}
			printk("...%d", cnt);
			cnt--;
			if (cnt == 0) {
				printk("\n");
				reboot_counter++;
				nvs_write(
					&fs, RBT_CNT_ID, &reboot_counter,
					sizeof(reboot_counter));
				if (reboot_counter == MAX_REBOOT) {
					printk("Doing last reboot...\n");
				}
				sys_reboot(0);
			}
		}
	}
}

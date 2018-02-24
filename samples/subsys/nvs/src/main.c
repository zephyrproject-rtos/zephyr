/*
 * NVS Sample for zephyr using high level API, this sample stores a string (address), a binary blob (key) and a long integer
 * (reboot counter) in flash the samples reboots every 5s to show the data that was stored in flash. It stops after 82 reboots
 * (when reboot counter is 80 the write of reboot counter triggers a sector change which copies the data of address, key
 * and reboot counter before erasing the flash). As a result of the rotation the history data of reboot counter is reduced in
 * size but the address and key data is kept.
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

#define STORAGE_OFFSET 0x3F000
#define STORAGE_MAGIC 0x4d455348 /* hex for "MESH" */
#define SECTOR_SIZE 0x400
#define SECTOR_COUNT 2

static struct nvs_fs fs = {
  .sector_size = SECTOR_SIZE,
  .sector_count = SECTOR_COUNT,
  .offset = STORAGE_OFFSET,
};

/* 1000 msec = 1 sec */
#define SLEEP_TIME 	1000
#define MAX_REBOOT 82

void main(void)
{
	int rc=0,cnt=0;
  struct nvs_entry entry;
	char buf[]="192.168.1.1";
  u8_t key[8]={0xFF,0xFE,0xFD,0xFC,0xFB,0xFA,0xF9,0xF8};
  u32_t reboot_counter=0;

  rc=nvs_init(&fs, FLASH_DEV_NAME, STORAGE_MAGIC);
  if (rc) {
    printk("Flash Init failed\n");
  }

  /* entry with id=1 is used to store address, lets see if we can read it from flash */
  entry.id = 1;
  rc=nvs_read(&fs, &entry,&buf);
  if (rc == NVS_OK) { /* entry was found, show it */
    printk("Entry: %d, Address: %s\n",entry.id,buf);
  }
  else { /* entry was not found, add it */
    printk("No address found, adding it as entry %d\n",entry.id);
    entry.len=strlen(buf)+1;
    rc = nvs_write(&fs, &entry, &buf);
    if (rc) {
      printk("write error\n");
    }
  }
  /* entry with id=2 is used a key, lets see if we can read it from flash */
  entry.id = 2;
  rc=nvs_read(&fs, &entry,&key);
  if (rc == NVS_OK) { /* entry was found, show it */
    printk("Entry: %d, Key: ",entry.id);
    for (int n=0;n<8;n++) {
      printk("%x ",key[n]);
    }
    printk("\n");
  }
  else { /* entry was not found, add it */
    printk("No key found, adding it as entry %d\n",entry.id);
    entry.len=8;
    rc = nvs_write(&fs, &entry, &key);
    if (rc) {
      printk("write error\n");
    }
  }
  /* entry with id=3 is used to store the reboot counter, lets see if we can read it from flash */
  entry.id = 3;
  rc=nvs_read(&fs, &entry,&reboot_counter);
  if (rc == NVS_OK) { /* entry was found, show it */
    printk("Entry: %d, Reboot counter: %d\n",entry.id,reboot_counter);
  }
  else { /* entry was not found, add it */
    printk("No Reboot counter found, adding it as entry %d\n",entry.id);
    entry.len=sizeof(reboot_counter);
    rc = nvs_write(&fs, &entry, &reboot_counter);
    if (rc) {
      printk("write error\n");
    }
  }

  cnt=5;
	while (1) {
		k_sleep(SLEEP_TIME);
    if (reboot_counter < MAX_REBOOT) {/* limit the amount of reboots to 85, this should just trigger buffer rotation */
      if (cnt == 5) { /* print some history information about the reboot counter */
        /* Check the counter history in flash */
        entry.id=3;
        rc=nvs_read_hist(&fs, &entry,&reboot_counter,0); /* Get the first entry */
        if (rc == NVS_OK) { /* entry was found, show it */
          printk("Reboot counter history: %d",reboot_counter);
        }
        else {
          printk("Error, Reboot counter not found\n");
        }
        while (nvs_read_hist(&fs, &entry,&reboot_counter,1) == NVS_OK) { /* Get the other entries */
          printk("...%d",reboot_counter);
        }
        printk("\nRebooting in ");
      }
      printk("...%d",cnt);
      cnt--;
      if (cnt == 0) {
        printk("\n");
        reboot_counter++;
        entry.id = 3;
        entry.len=sizeof(reboot_counter);
        rc = nvs_write(&fs, &entry, &reboot_counter);
        if (rc) {
          printk("write error\n");
        }
        if (reboot_counter == (MAX_REBOOT - 1)) {
          printk("Doing last reboot...\n");
        }
        sys_reboot(0);
      }
    }
	}
}

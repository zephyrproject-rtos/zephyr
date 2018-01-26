/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <board.h>
#include <device.h>
#include <flash.h>
#include <sfcb/sfcb.h>

/*#if defined(CONFIG_SOC_FAMILY_NRF5)
#define FLASH_DEV_NAME CONFIG_SOC_FLASH_NRF5_DEV_NAME
#else
#define FLASH_DEV_NAME ""
#endif */

#define PSTORAGE_OFFSET 0x3F000
#define PSTORAGE_MAGIC 0x4d455348 /* hex for "MESH" */
#define SECTOR_SIZE 0x400
#define SECTOR_COUNT 4

static struct sfcb_fs fs = {
  .offset = PSTORAGE_OFFSET,
  .sector_size = SECTOR_SIZE,
  .sector_count = SECTOR_COUNT,
  .gc = true,
};

/* 1000 msec = 1 sec */
#define SLEEP_TIME 	1000

void main(void)
{
	int rc=0;
  struct sfcb_entry new_entry;
	u8_t buf[4];

  fs.flash_device = device_get_binding(FLASH_DEV_NAME);
  rc=sfcb_fs_init(&fs, PSTORAGE_MAGIC);
  if (rc) {
    printk("Flash Init failed\n");
  }

  /* entry with id 1, len4, data="MESH" */
	new_entry.id=1;
	new_entry.len=4;
	rc = sfcb_fs_append(&fs, &new_entry);
	if (rc == SFCB_ERR_NOSPACE) {
		printk("Doing rotate ...\n");
		rc=sfcb_fs_rotate(&fs);
		rc=sfcb_fs_append(&fs, &new_entry);
	}
	if (rc == 0) {
	  rc = sfcb_fs_flash_write(&fs, new_entry.data_addr, "MESH", new_entry.len);
	  rc = sfcb_fs_append_close(&fs,&new_entry);
	}

	/* Fill up the circular buffer with "FF" until it has to rotate with garbage collection
	   each item takes hdr + data + slt: 4 + 2 + 4 bytes = 10 bytes, the sector headers take
		 8 bytes, so with 4 sectors and one kept empty in gc mode we trigger gc after
		 (3 * 1024 - 3 * 8 - 12) / 10 = 303.6 writes. It will be somewhat sooner because sectors
		 are filled at the end */
  for (int i=1;i<304;i++) {
    new_entry.id=2;
    new_entry.len=2;
    rc=sfcb_fs_append(&fs, &new_entry);

    if (rc == SFCB_ERR_NOSPACE) {
      printk("Doing rotate ...\n");
      rc=sfcb_fs_rotate(&fs);
      rc=sfcb_fs_append(&fs, &new_entry);
    }
    if (rc == 0) {
			rc = sfcb_fs_flash_write(&fs, new_entry.data_addr, "FF", new_entry.len);
      rc=sfcb_fs_append_close(&fs,&new_entry);
    }
  }

  printk("Done writing ...\n");
  for (int i=1;i<102;i++) {
    new_entry.id=i;
    rc=sfcb_fs_get_first_entry(&fs, &new_entry);
    if (rc == SFCB_OK) {
      printk("Found first entry %d, at location: %x ",new_entry.id,new_entry.data_addr);
      if (sfcb_fs_check_crc(&fs, &new_entry)) {
        printk("CRC error\n");
      }
      printk("CRC OK - Data:");
			rc = sfcb_fs_flash_read(&fs, new_entry.data_addr, &buf, new_entry.len);
			for (int j=0;j<new_entry.len;j++) {
				printk("%c", buf[j]);
			}
			printk("\n");
    }
    if (rc == SFCB_ERR_NOVAR) {
      //printk("Entry not found\n");
    }

    rc=sfcb_fs_get_last_entry(&fs, &new_entry);
    if (rc == SFCB_OK) {
      printk("Found last entry %d, at location: %x ",new_entry.id,new_entry.data_addr);
      if (sfcb_fs_check_crc(&fs, &new_entry)) {
        printk("CRC error\n");
      }
			printk("CRC OK - Data:");
			rc = sfcb_fs_flash_read(&fs, new_entry.data_addr, &buf, new_entry.len);
			for (int j=0;j<new_entry.len;j++) {
				printk("%c", buf[j]);
			}
			printk("\n");
    }
    if (rc == SFCB_ERR_NOVAR) {
      //printk("Entry not found\n");
    }

  }

	while (1) {
		k_sleep(SLEEP_TIME);
	}
}

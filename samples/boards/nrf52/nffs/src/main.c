/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include <zephyr.h>
#include <zephyr/types.h>

#include <errno.h>
#include <misc/printk.h>
#include <misc/byteorder.h>


#include <fs.h>
#include <nrf.h>
#include <device.h>
#include <gpio.h>
#include <board.h>


#define  SETB(x,y)   (x|=(1<<y))     //for o/p
#define  CLRB(x,y)   (x&=(~(1<<y)))  //for o/p
#define  TGLB(x,y)   (x^=(1<<y))     //for o/p
#define  CHECKB(x,y) (x&(1<<y))      //for i/p

int lock=0;

struct device *button_device[1];

static struct k_work button_work;

static void button_pressed(struct device *dev, struct gpio_callback *cb,
		 uint32_t pins)
{
	k_work_submit(&button_work);
}

void press(struct k_work *work)
{
	char buf[20];
	int err=0;

	fs_file_t fp;

	err = fs_open(&fp,"/0.txt");
	if (err != 0) {
		printk("error %d while opening for read\n\r", err);
	}

	err = fs_read(&fp, buf, sizeof(buf));
	if (err < 0) {
		printk("error %d while reading\n\r", err);
	}

	printk("Data read from file is --> %s \n", buf);

	err = fs_close(&fp);
	if (err != 0) {
		printk("error %d while closing after read\n\r", err);
	}

}


static int test(int idx)
{
	int err;
	char name[MAX_FILE_NAME + 1];
	fs_file_t fp;
	static const char msg[] = "Hello World\0";
	char buf[sizeof(msg)];

	/* Note that NFFS needs an absolute path */
	sprintf(name, "/%d.txt", idx);

	/* Delete the old file, if any */
	fs_unlink(name);

	/* Open and write */
	err = fs_open(&fp, name);
	if (err != 0) {
		printk("%s: error %d while opening for write\n", name, err);
		return err;
	}

	err = fs_write(&fp, msg, sizeof(msg));
	if (err < 0) {
		printk("%s: error %d while writing\n", name, err);
		return err;
	}

	if (err != sizeof(msg)) {
		printk("%s: tried to write %d bytes, only wrote %d\n", name,
		       sizeof(msg), err);
		return err;
	}

	/* Close the file and read the data back */
	err = fs_close(&fp);
	if (err != 0) {
		printk("%s: error %d while closing after write\n", name, err);
		return err;
	}


	struct fs_dirent entry;

	if(fs_stat(name,&entry)==0)
	{


		err = fs_open(&fp, name);
		if (err != 0) {
			printk("%s: error %d while opening for read\n", name, err);
			return err;
		}

		err = fs_read(&fp, buf, sizeof(buf));
		if (err < 0) {
			printk("%s: error %d while reading\n", name, err);
			return err;
		}

		if (err != sizeof(msg)) {
			printk("%s: tried to read %d bytes, only got %d\n", name,
			       sizeof(msg), err);
			return err;
		}

		printk("%s: wrote then read \"%s\"\n", name, buf);

		/* Close the file */
		err = fs_close(&fp);
		if (err != 0) {
			printk("%s: error %d while closing after read\n", name, err);
			return err;
		}
 	}

	lock=1;

	return 0;
}

void main(void)
{

	NRF_P0->PIN_CNF[12]=0x0000000C;

	static struct gpio_callback button_cb[1];
	
	k_work_init(&button_work, press);

	button_device[0] = device_get_binding(SW0_GPIO_NAME);
	gpio_pin_configure(button_device[0], SW0_GPIO_PIN, (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE | GPIO_PUD_PULL_UP | GPIO_INT_DEBOUNCE | GPIO_INT_DOUBLE_EDGE));
	gpio_init_callback(&button_cb[0], button_pressed, BIT(SW0_GPIO_PIN));
	gpio_add_callback(button_device[0], &button_cb[0]);
	gpio_pin_enable_callback(button_device[0], SW0_GPIO_PIN);

	if (device_get_binding(CONFIG_FS_NFFS_FLASH_DEV_NAME) == NULL) 
	{
		printk("warning: flash device %s not found\n",	CONFIG_FS_NFFS_FLASH_DEV_NAME);
	}

	while(lock==0)
	{

        	if(CHECKB(NRF_P0->IN,12)==0)
		{
			test(0);
		}

		k_sleep(100);
	}	
}





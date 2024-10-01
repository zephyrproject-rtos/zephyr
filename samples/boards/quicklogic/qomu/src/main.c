/*
 * Copyright (c) 2022 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/fpga.h>
#include <stdio.h>
#include <eoss3_dev.h>

extern uint32_t axFPGABitStream[];
extern uint32_t axFPGABitStream_length;

int main(void)
{
	const struct device *fpga = device_get_binding("fpga");

	/* load usbserial bitstream */
	fpga_load(fpga, axFPGABitStream, axFPGABitStream_length);

	/* let it enumerate */
	k_msleep(2000);

	printk("####################\n");
	printk("I am Zephyr on Qomu!\n");
	printk("####################\n");
	return 0;
}

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include<zephyr/device.h>
#include<zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>

int main(void)
{
	//printf("Hello World! %s\n", CONFIG_BOARD);
	uint8_t msg_arr[1];
	const struct device * dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
	printf("Hello world");
	uint16_t addr = 0x0068;
	struct i2c_msg msg_obj;
	while(1){
	msg_arr[0] = 0x02;
	msg_obj.buf = msg_arr;
	msg_obj.len = sizeof(msg_arr);
	msg_obj.flags = I2C_MSG_WRITE;
	i2c_transfer(dev,&msg_obj,1,addr);
	msg_obj.flags = I2C_MSG_READ;
	i2c_transfer(dev,&msg_obj,1,addr);
	printk("Data read is %d",bcd2bin((msg_obj.buf)[0]));
	}
	return 0;
}

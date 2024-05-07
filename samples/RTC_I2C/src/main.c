/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include<zephyr/device.h>
#include<zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include"RTC.h"
void lcd_run(){
  const struct device * dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
  printf("Hello world");
  lcd_init(dev,0x27,2,1);
  lcd_setCursor(dev,0x27,0,0);
  lcd_printf(dev,0x27,"Hello world");
while(1);
 }

K_THREAD_DEFINE(my_tid, 512,
                lcd_run, NULL, NULL, NULL,
                1, 0, 0);





/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <gpio.h>
#include <spi.h>

#include <misc/printk.h>

#include "board.h"

struct device *spi_dev;
struct device *button_select;

struct spi_config config = { 0 };


void write_leds(u16_t val)
{
        int err = 0;

        err = spi_slave_select(spi_dev, CLICK1_SPI_SELECT);
        if (err) {
                printk("Error selecting LED slave\n");
                return;
        }

        err = spi_write(spi_dev, &val, 2);
        if (err != 0) {
                printk("Unable to perform SPI write: %d\n", err);
        } else {
                printk("Wrote LED value %d\n", val);
        }
}

u16_t read_buttons()
{
        int err = 0;
        u16_t val = 0;
        err = spi_slave_select(spi_dev, CLICK2_SPI_SELECT);
        if (err) {
                printk("Error selecting button slave\n");
        } else {
                err = spi_read(spi_dev, &val, 2);
                if (err != 0) {
                        printk("Unable to perform SPI read: %d\n", err);
                } else {
                        val = ~val;
                }
        }
        return val;

}

void init_leds()
{
        write_leds(0);
}

void init_spi()
{
        spi_dev = device_get_binding(CONFIG_SPI_0_NAME);
        config.config = SPI_WORD(8);
        config.max_sys_freq = 256;

        int ret = spi_configure(spi_dev, &config);
        if (ret) {
                printk("SPI configuration failed\n");
                return;
        } else {
                printk("SPI %s configured\n", CONFIG_SPI_0_NAME);
        }
}

char *buttonLabels = "B654A321*D#0C987";

void print_buttons_pushed(u16_t value)
{
        printk("buttons pushed: ");
        for (int i = 15; i >= 0; i -= 1) {
                if (value & 0x01) {
                        printk("%c", buttonLabels[i]);
                }

                value >>= 1;
        }
        printk("\n");

}

void main(void)
{
        printk("Hello World! %s\n", CONFIG_ARCH);

        init_spi();

        u16_t lastval = 0;
        while (true) {
                int val = read_buttons();
                if (val) {
                        if (val != lastval) {
                                print_buttons_pushed(val);
                                write_leds(val);
                                lastval = val;
                        }
                }
        }
}

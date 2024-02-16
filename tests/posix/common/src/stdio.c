/*
 * Copyright (c) 2024 Dawid Osuchowski
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/console/console.h>
#include <zephyr/drivers/serial/uart_emul.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

static const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

// ZTEST(stdio, test_fgetc_empty_stream)
// {
//     char test_char = EOF;
//     int expected_return = 0;
//     int retErr = ferror(stdin);
//     int ret = fgetc(stdin);
//     printf("ERRNO: %d\nretError: %d", errno, retErr);

//     zassert_equal(ret, test_char, "Expected return value %d, got %d", test_char, ret);
//     zassert_not_equal(feof(stdin), expected_return);
// }

ZTEST(stdio, test_fgetc_one_char)
{

    char test_char = 'A';
    int expected_return = 0;
    uart_emul_put_rx_data(dev, (uint8_t *)&test_char, sizeof(uint8_t));
    int ret = fgetc(stdin);


    zassert_equal(ret, test_char, "Expected return value %d, got %d", test_char, ret);
    zassert_not_equal(feof(stdin), expected_return);


}

static void before(void *arg)
{
    uart_emul_flush_tx_data(dev);
}


ZTEST_SUITE(stdio, NULL, NULL, before, NULL, NULL);
/*
 * Copyright (c) 2022 Trackunit A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>

#define DUMMY1_NODE_ID DT_NODELABEL(dummy1)
#define DUMMY2_NODE_ID DT_NODELABEL(dummy2)
#define DUMMY_LOW_NODE_ID DT_NODELABEL(dummy_low)
#define DUMMY_HIGH_NODE_ID DT_NODELABEL(dummy_high)

/* APIs implemented by dummy driver defined in dummy_driver.c */
extern const struct uart_driver_api dummy_uart_api;
extern const struct i2c_driver_api dummy_i2c_api;
extern const struct spi_driver_api dummy_spi_api;

/* Run counter used to verify dummy driver init has been invoked correctly */
extern int dummy_driver_init_run_cnt;

/*
 * Catch any linker errors here. If external references are generated and
 * included, and the driver is able to instanciate them, this will not cause
 * any errors.
 */
static const struct device *uart_dev1 = DEVICE_DT_API_GET(DUMMY1_NODE_ID, uart);
static const struct device *i2c_dev1 = DEVICE_DT_API_GET(DUMMY1_NODE_ID, i2c);
static const struct device *spi_dev1 = DEVICE_DT_API_GET(DUMMY1_NODE_ID, spi);

static const struct device *uart_dev2 = DEVICE_DT_API_GET(DUMMY2_NODE_ID, uart);
static const struct device *i2c_dev2 = DEVICE_DT_API_GET(DUMMY2_NODE_ID, i2c);
static const struct device *spi_dev2 = DEVICE_DT_API_GET(DUMMY2_NODE_ID, spi);

/* Test DEVICE_DT_API_SUPPORTED macro */
#if (DEVICE_DT_API_SUPPORTED(DUMMY1_NODE_ID, uart) == 0)
	#warning "DEVICE_DT_API_SUPPORTED failed for dummy1 uart API"
#endif

#if (DEVICE_DT_API_SUPPORTED(DUMMY1_NODE_ID, i2c) == 0)
	#warning "DEVICE_DT_API_SUPPORTED failed for dummy1 i2c API"
#endif

#if (DEVICE_DT_API_SUPPORTED(DUMMY1_NODE_ID, spi) == 0)
	#warning "DEVICE_DT_API_SUPPORTED failed for dummy1 spi API"
#endif

#if (DEVICE_DT_API_SUPPORTED(DUMMY1_NODE_ID, led) == 1)
	#warning "DEVICE_DT_API_SUPPORTED failed for dummy1 led API"
#endif

#if (DEVICE_DT_API_SUPPORTED_ANY(uart) == 0)
	#warning "DEVICE_DT_API_SUPPORTED_ANY failed for uart API"
#endif

#if (DEVICE_DT_API_SUPPORTED_ANY(dummy) == 1)
	#warning "DEVICE_DT_API_SUPPORTED_ANY failed for dummy API"
#endif

ZTEST(device_api, test_dummy_driver_init_has_run)
{
	zassert_true(dummy_driver_init_run_cnt == 2,
		"Init has not run correctly for dummy driver instances");
}

ZTEST(device_api, test_devs_1_share_config_and_data)
{
	zassert_true(uart_dev1->data == i2c_dev1->data, "dev1 uart and i2c data differs");
	zassert_true(i2c_dev1->data == spi_dev1->data, "dev1 i2c and spi data differs");
	zassert_true(uart_dev1->config == i2c_dev1->config, "dev1 uart and i2c config differs");
	zassert_true(i2c_dev1->config == spi_dev1->config, "dev1 i2c and spi config differs");
}

ZTEST(device_api, test_devs_2_share_config_and_data)
{
	zassert_true(uart_dev2->data == i2c_dev2->data, "dev2 uart and i2c data differs");
	zassert_true(i2c_dev2->data == spi_dev2->data, "dev2 i2c and spi data differs");
	zassert_true(uart_dev2->config == i2c_dev2->config, "dev2 uart and i2c config differs");
	zassert_true(i2c_dev2->config == spi_dev2->config, "dev2 i2c and spi config differs");
}

ZTEST(device_api, test_devs_1_api_ptr)
{
	zassert_true(uart_dev1->api == &dummy_uart_api, "dev1 uart api ptr incorrect");
	zassert_true(i2c_dev1->api == &dummy_i2c_api, "dev1 i2c api ptr incorrect");
	zassert_true(spi_dev1->api == &dummy_spi_api, "dev1 spi api ptr incorrect");
}

ZTEST(device_api, test_devs_2_api_ptr)
{
	zassert_true(uart_dev2->api == &dummy_uart_api, "dev2 uart api ptr incorrect");
	zassert_true(i2c_dev2->api == &dummy_i2c_api, "dev2 i2c api ptr incorrect");
	zassert_true(spi_dev2->api == &dummy_spi_api, "dev2 spi api ptr incorrect");
}

/* Statically defined device variables */
static int device_0_data;
static const int device_0_cfg;

static int device_1_data;
static const int device_1_cfg;

static int static_dev_init_run_cnt;

/* Statically defined device API implementations */
static const struct uart_driver_api static_dev_uart_api;
static const struct i2c_driver_api static_dev_i2c_api;
static const struct spi_driver_api static_dev_spi_api;

/* Statically defined device init function */
static int static_dev_init(const struct device *dev)
{
	static_dev_init_run_cnt++;
	return 0;
}

/* Define statically defined device named device0 */
DEVICE_NEW_DEFINE(device0, "device_0", static_dev_init, NULL,
	&device_0_data, &device_0_cfg, POST_KERNEL, 99,
	DEVICE_API(&static_dev_uart_api, uart),
	DEVICE_API(&static_dev_i2c_api, i2c),
	DEVICE_API(&static_dev_spi_api, spi));

/* Define statically defined device named device1 */
DEVICE_NEW_DEFINE(device1, "device_1", static_dev_init, NULL,
	&device_1_data, &device_1_cfg, POST_KERNEL, 99,
	DEVICE_API(&static_dev_uart_api, uart),
	DEVICE_API(&static_dev_i2c_api, i2c),
	DEVICE_API(&static_dev_spi_api, spi));

/* Catch linker errors here if devices have not been defined correctly */
static const struct device *static_device0_uart = DEVICE_API_GET(device0, uart);
static const struct device *static_device0_i2c = DEVICE_API_GET(device0, i2c);
static const struct device *static_device0_spi = DEVICE_API_GET(device0, spi);

static const struct device *static_device1_uart = DEVICE_API_GET(device1, uart);
static const struct device *static_device1_i2c = DEVICE_API_GET(device1, i2c);
static const struct device *static_device1_spi = DEVICE_API_GET(device1, spi);

ZTEST(device_api, test_static_api_dev_init_has_run)
{
	zassert_true(static_dev_init_run_cnt == 2,
		"Init has not run correctly for statically defined device device0");
}

ZTEST(device_api, test_static_api_devs_0_share_config_and_data)
{
	zassert_true(static_device0_uart->data == static_device0_i2c->data,
		"static_device0 uart and i2c data differs");
	zassert_true(static_device0_i2c->data == static_device0_spi->data,
		"static_device0 i2c and spi data differs");
	zassert_true(static_device0_uart->config == static_device0_i2c->config,
		"static_device0 uart and i2c config differs");
	zassert_true(static_device0_i2c->config == static_device0_spi->config,
		"static_device0 i2c and spi config differs");
}

ZTEST(device_api, test_static_api_devs_1_share_config_and_data)
{
	zassert_true(static_device1_uart->data == static_device1_i2c->data,
		"static_device1 uart and i2c data differs");
	zassert_true(static_device1_i2c->data == static_device1_spi->data,
		"static_device1 i2c and spi data differs");
	zassert_true(static_device1_uart->config == static_device1_i2c->config,
		"static_device1 uart and i2c config differs");
	zassert_true(static_device1_i2c->config == static_device1_spi->config,
		"static_device1 i2c and spi config differs");
}

ZTEST(device_api, test_static_api_devs_0_api_ptr)
{
	zassert_true(static_device0_uart->api == &static_dev_uart_api,
		"static_device0 uart api ptr incorrect");
	zassert_true(static_device0_i2c->api == &static_dev_i2c_api,
		"static_device0 i2c api ptr incorrect");
	zassert_true(static_device0_spi->api == &static_dev_spi_api,
		"static_device0 spi api ptr incorrect");
}

ZTEST(device_api, test_static_api_devs_1_api_ptr)
{
	zassert_true(static_device1_uart->api == &static_dev_uart_api,
		"static_device1 uart api ptr incorrect");
	zassert_true(static_device1_i2c->api == &static_dev_i2c_api,
		"static_device1 i2c api ptr incorrect");
	zassert_true(static_device1_spi->api == &static_dev_spi_api,
		"static_device1 spi api ptr incorrect");
}

ZTEST(device_api, test_api_devs_in_correct_section)
{
	extern const struct device __api_device_start[];
	extern const struct device __api_device_end[];
	uintptr_t api_dev_start_ptr = (uintptr_t)__api_device_start;
	uintptr_t api_dev_end_ptr = (uintptr_t)__api_device_end;

	zassert_true(api_dev_start_ptr < api_dev_end_ptr,
		"no devices in api device section");
}

/* Ensure DEVICE_DT_NEW_FOREACH compiles with known and unknown API */
#define TEST_DEVICE_DT_NEW_FOREACH(node_id)
DEVICE_DT_API_FOREACH(TEST_DEVICE_DT_NEW_FOREACH, uart)
DEVICE_DT_API_FOREACH(TEST_DEVICE_DT_NEW_FOREACH, dummy)

/* Ensure DEVICE_DT_NEW_FOREACH_VARGS compiles with known and unknown API */
#define TEST_DEVICE_DT_NEW_FOREACH_VARGS(node_id, ...)
DEVICE_DT_API_FOREACH_VARGS(TEST_DEVICE_DT_NEW_FOREACH_VARGS, uart)
DEVICE_DT_API_FOREACH_VARGS(TEST_DEVICE_DT_NEW_FOREACH_VARGS, dummy)

/* Test DEVICE_DT_PROP and DEVICE_DT_PROP_OR for dummy_driver */
static const char *dummy1_prop_vendor = DEVICE_DT_PROPERTY(DUMMY1_NODE_ID, vendor);
static const int dummy1_prop_serial = DEVICE_DT_PROPERTY(DUMMY1_NODE_ID, serial);
static const char *dummy1_prop_model = DEVICE_DT_PROPERTY(DUMMY1_NODE_ID, model);
static const char *dummy1_prop_maybe = DEVICE_DT_PROPERTY_OR(DUMMY1_NODE_ID, maybe, "Default");

/* Verify values of dummy1 properties */
ZTEST(device_api, test_device_dt_prop_dummy1_vendor)
{
	zassert_true(strcmp(dummy1_prop_vendor, "DummyVendor") == 0,
		"DEVICE_DT_PROPERTY expanded to incorrect value for dummy1 vendor");
}

ZTEST(device_api, test_device_dt_prop_dummy1_serial)
{
	zassert_true(dummy1_prop_serial == 1432,
		"DEVICE_DT_PROPERTY expanded to incorrect value for dummy1 serial");
}

ZTEST(device_api, test_device_dt_prop_dummy1_model)
{
	zassert_true(strcmp(dummy1_prop_model, "DefaultDummyModel") == 0,
		"DEVICE_DT_PROPERTY expanded to incorrect value for dummy1 model");
}

ZTEST(device_api, test_device_dt_prop_dummy1_maybe)
{
	zassert_true(strcmp(dummy1_prop_maybe, "Default") == 0,
		"DEVICE_DT_PROPERTY_OR expanded to incorrect value for non existent maybe");
}

ZTEST_SUITE(device_api, NULL, NULL, NULL, NULL, NULL);

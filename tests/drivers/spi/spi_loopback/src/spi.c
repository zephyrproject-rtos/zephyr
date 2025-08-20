/*
 * Copyright 2025 NXP
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* To run this loopback test, connect MOSI pin to the MISO of the SPI */

/*
 ************************
 * Include dependencies *
 ************************
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include <stdarg.h>

/*
 **********************
 * SPI configurations *
 **********************
 */

#define FRAME_SIZE COND_CODE_1(CONFIG_SPI_LOOPBACK_16BITS_FRAMES, (16), (8))
#define MODE_LOOP  COND_CODE_1(CONFIG_SPI_LOOPBACK_MODE_LOOP, (SPI_MODE_LOOP), (0))

#define SPI_OP(frame_size)                                                                         \
	SPI_OP_MODE_MASTER | SPI_MODE_CPOL | MODE_LOOP | SPI_MODE_CPHA |                           \
		SPI_WORD_SET(frame_size) | SPI_LINES_SINGLE

#define SPI_FAST_DEV DT_COMPAT_GET_ANY_STATUS_OKAY(test_spi_loopback_fast)
static struct spi_dt_spec spi_fast = SPI_DT_SPEC_GET(SPI_FAST_DEV, SPI_OP(FRAME_SIZE), 0);

#define SPI_SLOW_DEV DT_COMPAT_GET_ANY_STATUS_OKAY(test_spi_loopback_slow)
static struct spi_dt_spec spi_slow = SPI_DT_SPEC_GET(SPI_SLOW_DEV, SPI_OP(FRAME_SIZE), 0);

static struct spi_dt_spec *loopback_specs[2] = {&spi_slow, &spi_fast};
static char *spec_names[2] = {"SLOW", "FAST"};
static int spec_idx;

/* Driver may need to reconfigure due to different spi config for the
 * some of the tests. If we use the same memory location for the spec,
 * it will be treated like the same owner of the bus as previously,
 * and many drivers will think it is the same spec and skip reconfiguring.
 * Even declaring a copy of the spec on the stack will most likely have it end up at the same
 * location on the stack every time and have the same problem.
 * It's not clear from the API if this is expected but it is a common trick that is de facto
 * standardized by the use of the private spi_context header, so for now consider it
 * expected behavior in this test, and write the test accordingly. The point of those cases is
 * not to test the specifics of bus ownership in the API, so we just deal with it.
 */
struct spi_dt_spec spec_copies[5];

const struct gpio_dt_spec miso_pin = GPIO_DT_SPEC_GET_OR(DT_PATH(zephyr_user), miso_gpios, {});
const struct gpio_dt_spec mosi_pin = GPIO_DT_SPEC_GET_OR(DT_PATH(zephyr_user), mosi_gpios, {});

/*
 ********************
 * SPI test buffers *
 ********************
 */

#ifdef CONFIG_NOCACHE_MEMORY
#define __NOCACHE	__attribute__((__section__(".nocache")))
#elif defined(CONFIG_DT_DEFINED_NOCACHE)
#define __NOCACHE	__attribute__((__section__(CONFIG_DT_DEFINED_NOCACHE_NAME)))
#else /* CONFIG_NOCACHE_MEMORY */
#define __NOCACHE
#if CONFIG_DCACHE_LINE_SIZE != 0
#define __BUF_ALIGN	__aligned(CONFIG_DCACHE_LINE_SIZE)
#endif
#endif /* CONFIG_NOCACHE_MEMORY */

#ifndef __BUF_ALIGN
#define __BUF_ALIGN	__aligned(32)
#endif

#define BUF_SIZE 18
static const char tx_data[BUF_SIZE] = "0123456789abcdef-\0";
static __BUF_ALIGN char buffer_tx[BUF_SIZE] __NOCACHE;
static __BUF_ALIGN char buffer_rx[BUF_SIZE] __NOCACHE;

#define BUF2_SIZE 36
static const char tx2_data[BUF2_SIZE] = "Thequickbrownfoxjumpsoverthelazydog\0";
static __BUF_ALIGN char buffer2_tx[BUF2_SIZE] __NOCACHE;
static __BUF_ALIGN char buffer2_rx[BUF2_SIZE] __NOCACHE;

#define BUF3_SIZE CONFIG_SPI_LARGE_BUFFER_SIZE
static const char large_tx_data[BUF3_SIZE] = "Thequickbrownfoxjumpsoverthelazydog\0";
static __BUF_ALIGN char large_buffer_tx[BUF3_SIZE] __NOCACHE;
static __BUF_ALIGN char large_buffer_rx[BUF3_SIZE] __NOCACHE;

#define BUFWIDE_SIZE 12
static const uint16_t tx_data_16[] = {0x1234, 0x5678, 0x9ABC, 0xDEF0,
				      0xFF00, 0x00FF, 0xAAAA, 0x5555,
				      0xF0F0, 0x0F0F, 0xA5A5, 0x5A5A};
static __BUF_ALIGN uint16_t buffer_tx_16[BUFWIDE_SIZE] __NOCACHE;
static __BUF_ALIGN uint16_t buffer_rx_16[BUFWIDE_SIZE] __NOCACHE;
static const uint32_t tx_data_32[] = {0x12345678, 0x56781234, 0x9ABCDEF0, 0xDEF09ABC,
				      0xFFFF0000, 0x0000FFFF, 0x00FF00FF, 0xFF00FF00,
				      0xAAAA5555, 0x5555AAAA, 0xAA55AA55, 0x55AA55AA};
static __BUF_ALIGN uint32_t buffer_tx_32[BUFWIDE_SIZE] __NOCACHE;
static __BUF_ALIGN uint32_t buffer_rx_32[BUFWIDE_SIZE] __NOCACHE;

/*
 ********************
 * Helper functions *
 ********************
 */

/*
 * We need 5x(buffer size) + 1 to print a comma-separated list of each
 * byte in hex, plus a null.
 */
#define PRINT_BUF_SIZE(size) ((size * 5) + 1)

static uint8_t buffer_print_tx[PRINT_BUF_SIZE(BUF_SIZE)];
static uint8_t buffer_print_rx[PRINT_BUF_SIZE(BUF_SIZE)];

static uint8_t buffer_print_tx2[PRINT_BUF_SIZE(BUF2_SIZE)];
static uint8_t buffer_print_rx2[PRINT_BUF_SIZE(BUF2_SIZE)];

/* function for displaying the data in the buffers */
static void to_display_format(const uint8_t *src, size_t size, char *dst)
{
	size_t i;

	for (i = 0; i < size; i++) {
		sprintf(dst + 5 * i, "0x%02x,", src[i]);
	}
}

#if DT_NODE_HAS_PROP(DT_PATH(zephyr_user), cs_loopback_gpios)

static const struct gpio_dt_spec cs_loopback_gpio =
			GPIO_DT_SPEC_GET_OR(DT_PATH(zephyr_user), cs_loopback_gpios, {0});
static struct gpio_callback cs_cb_data;
atomic_t cs_count;

static void spi_loopback_gpio_cs_loopback_prepare(void)
{
	atomic_set(&cs_count, 0);
}

static int spi_loopback_gpio_cs_loopback_check(int expected_triggers)
{
	int actual_triggers = atomic_get(&cs_count);

	if (actual_triggers != expected_triggers) {
		TC_PRINT("Expected %d CS triggers, got %d", expected_triggers, actual_triggers);
		return -1;
	}
	return 0;
}

static void cs_callback(const struct device *port,
		     struct gpio_callback *cb,
		     gpio_port_pins_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	/* Give semaphore to indicate CS triggered */
	atomic_inc(&cs_count);
}

static int spi_loopback_gpio_cs_loopback_init(void)
{
	const struct gpio_dt_spec *gpio = &cs_loopback_gpio;
	int ret;

	if (!gpio_is_ready_dt(gpio)) {
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(gpio, GPIO_INPUT);
	if (ret) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(gpio, GPIO_INT_EDGE_BOTH);
	if (ret) {
		return ret;
	}

	gpio_init_callback(&cs_cb_data, cs_callback, BIT(gpio->pin));

	return gpio_add_callback(gpio->port, &cs_cb_data);
}
#else
#define spi_loopback_gpio_cs_loopback_init(...) (0)
#define spi_loopback_gpio_cs_loopback_prepare(...)
#define spi_loopback_gpio_cs_loopback_check(...) (0)
#endif

/* just a wrapper of the driver transceive call with ztest error assert */
static void spi_loopback_transceive(struct spi_dt_spec *const spec,
				    const struct spi_buf_set *const tx,
				    const struct spi_buf_set *const rx,
				    int expected_cs_count)
{
	int ret;

	zassert_ok(pm_device_runtime_get(spec->bus));
	spi_loopback_gpio_cs_loopback_prepare();
	ret = spi_transceive_dt(spec, tx, rx);
	if (ret == -EINVAL || ret == -ENOTSUP) {
		TC_PRINT("Spi config invalid for this controller\n");
		zassert_ok(pm_device_runtime_put(spec->bus));
		ztest_test_skip();
	}
	zassert_ok(ret, "SPI transceive failed, code %d", ret);
	zassert_ok(spi_loopback_gpio_cs_loopback_check(expected_cs_count));
	zassert_ok(pm_device_runtime_put(spec->bus));
}

/* The most spi buf currently used by any test case is 4, change if needed */
#define MAX_SPI_BUF_COUNT 4
struct spi_buf tx_bufs_pool[MAX_SPI_BUF_COUNT];
struct spi_buf rx_bufs_pool[MAX_SPI_BUF_COUNT];

/* A function for creating a spi_buf_set. Simply provide the spi_buf pool (either rx or tx),
 * the number of bufs that will be in the set, and then an ordered list of pairs of buf
 * pointer (void *) and buf size (size_t).
 */
static const struct spi_buf_set spi_loopback_setup_xfer(struct spi_buf *pool, size_t num_bufs, ...)
{
	struct spi_buf_set buf_set;

	zassert_true(num_bufs <= MAX_SPI_BUF_COUNT, "SPI xfer need more buf in test");
	zassert_true(pool == tx_bufs_pool || pool == rx_bufs_pool, "Invalid spi buf pool");

	va_list args;

	va_start(args, num_bufs);

	for (int i = 0; i < num_bufs; i++) {
		pool[i].buf = va_arg(args, void *);
		pool[i].len = va_arg(args, size_t);
	}

	va_end(args);

	buf_set.buffers = pool;
	buf_set.count = num_bufs;

	return buf_set;
}

/* compare two buffers and print fail if they are not the same */
static void spi_loopback_compare_bufs(const uint8_t *buf1, const uint8_t *buf2, size_t size,
				      uint8_t *printbuf1, uint8_t *printbuf2)
{
	if (memcmp(buf1, buf2, size)) {
		to_display_format(buf1, size, printbuf1);
		to_display_format(buf2, size, printbuf2);
		TC_PRINT("Buffer contents are different:\n %s\nvs:\n %s\n",
				buffer_print_tx, buffer_print_rx);
		ztest_test_fail();
	}
}

/*
 **************
 * Test cases *
 **************
 */

/* test transferring different buffers on the same dma channels */
ZTEST(spi_loopback, test_spi_complete_multiple)
{
	struct spi_dt_spec *spec = loopback_specs[spec_idx];
	const struct spi_buf_set tx = spi_loopback_setup_xfer(tx_bufs_pool, 2,
							      buffer_tx, BUF_SIZE,
							      buffer2_tx, BUF2_SIZE);
	const struct spi_buf_set rx = spi_loopback_setup_xfer(rx_bufs_pool, 2,
							      buffer_rx, BUF_SIZE,
							      buffer2_rx, BUF2_SIZE);

	spi_loopback_transceive(spec, &tx, &rx, 2);

	spi_loopback_compare_bufs(buffer_tx, buffer_rx, BUF_SIZE,
				  buffer_print_tx, buffer_print_rx);
	spi_loopback_compare_bufs(buffer2_tx, buffer2_rx, BUF2_SIZE,
				  buffer_print_tx2, buffer_print_rx2);
}

/* same as the test_spi_complete_multiple test, but seeing if there is any unreasonable latency */
ZTEST(spi_loopback, test_spi_complete_multiple_timed)
{
	/* Do not check timing when coverage is enabled */
	Z_TEST_SKIP_IFDEF(CONFIG_COVERAGE);

	struct spi_dt_spec *spec = loopback_specs[spec_idx];
	const struct spi_buf_set tx = spi_loopback_setup_xfer(tx_bufs_pool, 2,
							      buffer_tx, BUF_SIZE,
							      buffer2_tx, BUF2_SIZE);
	const struct spi_buf_set rx = spi_loopback_setup_xfer(rx_bufs_pool, 2,
							      buffer_rx, BUF_SIZE,
							      buffer2_rx, BUF2_SIZE);
	uint32_t freq = spec->config.frequency;
	uint32_t start_time, end_time, cycles_spent;
	uint64_t time_spent_us, expected_transfer_time_us, latency_measurement;

	/*
	 * spi_loopback_transceive() does an inline pm_device_runtime_get(), but since we are
	 * timing the transfer, we need to get the SPI controller before we start the measurement.
	 */
	zassert_ok(pm_device_runtime_get(spec->bus));

	/* since this is a test program, there shouldn't be much to interfere with measurement */
	start_time = k_cycle_get_32();
	spi_loopback_transceive(spec, &tx, &rx, 2);
	end_time = k_cycle_get_32();

	zassert_ok(pm_device_runtime_put(spec->bus));

	if (end_time >= start_time) {
		cycles_spent = end_time - start_time;
	} else {
		/* number of cycles from start to counter reset + rest of cycles */
		cycles_spent = (UINT32_MAX - start_time) + end_time;
	}

	time_spent_us = k_cyc_to_us_ceil64(cycles_spent);

	/* Number of bits to transfer * usec per bit */
	expected_transfer_time_us = (uint64_t)(BUF_SIZE + BUF2_SIZE) * BITS_PER_BYTE *
						USEC_PER_SEC / freq;

	TC_PRINT("Transfer took %llu us vs theoretical minimum %llu us\n",
			time_spent_us, expected_transfer_time_us);

	/* For comparing the lower bound, some kernel timer implementations
	 * do not actually update the elapsed cycles until a tick boundary,
	 * so we need to account for that by subtracting one tick from the comparison metric
	 */
	uint64_t us_per_kernel_tick = k_ticks_to_us_ceil64(1);
	uint32_t minimum_transfer_time_us;

	if (expected_transfer_time_us > us_per_kernel_tick) {
		minimum_transfer_time_us = expected_transfer_time_us - us_per_kernel_tick;
	} else {
		minimum_transfer_time_us = 0;
	}

	/* Fail if transfer is faster than theoretically possible */
	zassert_true(time_spent_us >= minimum_transfer_time_us,
		     "Transfer faster than theoretically possible");

	/* handle overflow for print statement */
	latency_measurement = time_spent_us - expected_transfer_time_us;
	if (latency_measurement > time_spent_us) {
		latency_measurement = 0;
	}
	TC_PRINT("Latency measurement: %llu us\n", latency_measurement);

	/* Allow some overhead, but not too much */
	zassert_true(time_spent_us <=
			     expected_transfer_time_us * CONFIG_SPI_IDEAL_TRANSFER_DURATION_SCALING,
		     "Very high latency");

	spi_loopback_compare_bufs(buffer_tx, buffer_rx, BUF_SIZE,
				  buffer_print_tx, buffer_print_rx);
	spi_loopback_compare_bufs(buffer2_tx, buffer2_rx, BUF2_SIZE,
				  buffer_print_tx2, buffer_print_rx2);
}

void spi_loopback_test_mode(struct spi_dt_spec *spec, bool cpol, bool cpha)
{
	const struct spi_buf_set tx = spi_loopback_setup_xfer(tx_bufs_pool, 1,
							      buffer_tx, BUF_SIZE);
	const struct spi_buf_set rx = spi_loopback_setup_xfer(rx_bufs_pool, 1,
							      buffer_rx, BUF_SIZE);
	uint32_t original_op = spec->config.operation;

	if (cpol) {
		spec->config.operation |= SPI_MODE_CPOL;
	} else {
		spec->config.operation &= ~SPI_MODE_CPOL;
	}

	if (cpha) {
		spec->config.operation |= SPI_MODE_CPHA;
	} else {
		spec->config.operation &= ~SPI_MODE_CPHA;
	}

	spi_loopback_transceive(spec, &tx, &rx, 2);

	spec->config.operation = original_op;

	spi_loopback_compare_bufs(buffer_tx, buffer_rx, BUF_SIZE,
		buffer_print_tx, buffer_print_rx);
}

ZTEST(spi_loopback, test_spi_complete_loop_mode_0)
{
	struct spi_dt_spec *spec = loopback_specs[spec_idx];
	struct spi_dt_spec *spec_copy = &spec_copies[0];
	*spec_copy = *spec;

	spi_loopback_test_mode(spec_copy, false, false);
}

ZTEST(spi_loopback, test_spi_complete_loop_mode_1)
{
	struct spi_dt_spec *spec = loopback_specs[spec_idx];
	struct spi_dt_spec *spec_copy = &spec_copies[1];
	*spec_copy = *spec;

	spi_loopback_test_mode(spec_copy, false, true);
}

ZTEST(spi_loopback, test_spi_complete_loop_mode_2)
{
	struct spi_dt_spec *spec = loopback_specs[spec_idx];
	struct spi_dt_spec *spec_copy = &spec_copies[2];
	*spec_copy = *spec;

	spi_loopback_test_mode(spec_copy, true, false);
}

ZTEST(spi_loopback, test_spi_complete_loop_mode_3)
{
	struct spi_dt_spec *spec = loopback_specs[spec_idx];
	struct spi_dt_spec *spec_copy = &spec_copies[3];
	*spec_copy = *spec;

	spi_loopback_test_mode(spec_copy, true, true);
}

ZTEST(spi_loopback, test_spi_null_tx_buf)
{
	struct spi_dt_spec *spec = loopback_specs[spec_idx];
	static const uint8_t expected_nop_return_buf[BUF_SIZE] = { 0 };
	const struct spi_buf_set tx = spi_loopback_setup_xfer(tx_bufs_pool, 1,
							      NULL, BUF_SIZE);
	const struct spi_buf_set rx = spi_loopback_setup_xfer(rx_bufs_pool, 1,
							      buffer_rx, BUF_SIZE);

	(void)memset(buffer_rx, 0x77, BUF_SIZE);

	spi_loopback_transceive(spec, &tx, &rx, 2);

	spi_loopback_compare_bufs(expected_nop_return_buf, buffer_rx, BUF_SIZE,
				  buffer_print_rx, buffer_print_rx);
}

ZTEST(spi_loopback, test_spi_rx_half_start)
{
	struct spi_dt_spec *spec = loopback_specs[spec_idx];
	const struct spi_buf_set tx = spi_loopback_setup_xfer(tx_bufs_pool, 1,
							      buffer_tx, BUF_SIZE);
	const struct spi_buf_set rx = spi_loopback_setup_xfer(rx_bufs_pool, 1,
							      buffer_rx, 8);

	(void)memset(buffer_rx, 0, BUF_SIZE);

	spi_loopback_transceive(spec, &tx, &rx, 2);

	spi_loopback_compare_bufs(buffer_tx, buffer_rx, 8,
				  buffer_print_tx, buffer_print_rx);
}

ZTEST(spi_loopback, test_spi_rx_half_end)
{
	if (IS_ENABLED(CONFIG_SPI_STM32_DMA)) {
		TC_PRINT("Skipped spi_rx_hald_end");
		return;
	}

	struct spi_dt_spec *spec = loopback_specs[spec_idx];
	const struct spi_buf_set tx = spi_loopback_setup_xfer(tx_bufs_pool, 1,
							      buffer_tx, BUF_SIZE);
	const struct spi_buf_set rx = spi_loopback_setup_xfer(rx_bufs_pool, 2,
							      NULL, 8,
							      buffer_rx, 8);

	(void)memset(buffer_rx, 0, BUF_SIZE);

	spi_loopback_transceive(spec, &tx, &rx, 2);

	spi_loopback_compare_bufs(buffer_tx+8, buffer_rx, 8,
				  buffer_print_tx, buffer_print_rx);
}

ZTEST(spi_loopback, test_spi_rx_every_4)
{
	if (IS_ENABLED(CONFIG_SPI_STM32_DMA) || IS_ENABLED(CONFIG_DSPI_MCUX_EDMA)) {
		TC_PRINT("Skipped spi_rx_every_4");
		return;
	};

	struct spi_dt_spec *spec = loopback_specs[spec_idx];
	const struct spi_buf_set tx = spi_loopback_setup_xfer(tx_bufs_pool, 1,
							      buffer_tx, BUF_SIZE);
	const struct spi_buf_set rx = spi_loopback_setup_xfer(rx_bufs_pool, 4,
							      NULL, 4,
							      buffer_rx, 4,
							      NULL, 4,
							      buffer_rx+4, 4);

	(void)memset(buffer_rx, 0, BUF_SIZE);

	spi_loopback_transceive(spec, &tx, &rx, 2);

	spi_loopback_compare_bufs(buffer_tx+4, buffer_rx, 4,
				  buffer_print_tx, buffer_print_rx);
	spi_loopback_compare_bufs(buffer_tx+12, buffer_rx+4, 4,
				  buffer_print_tx, buffer_print_rx);
}

ZTEST(spi_loopback, test_spi_rx_bigger_than_tx)
{
	if (IS_ENABLED(CONFIG_SPI_STM32_DMA) || IS_ENABLED(CONFIG_DSPI_MCUX_EDMA)) {
		TC_PRINT("Skipped spi_rx_bigger_than_tx");
		return;
	}

	struct spi_dt_spec *spec = loopback_specs[spec_idx];
	const uint32_t tx_buf_size = 8;

	BUILD_ASSERT(tx_buf_size < BUF_SIZE,
		"Transmit buffer is expected to be smaller than the receive buffer");

	const struct spi_buf_set tx = spi_loopback_setup_xfer(tx_bufs_pool, 1,
							      buffer_tx, tx_buf_size);
	const struct spi_buf_set rx = spi_loopback_setup_xfer(rx_bufs_pool, 1,
							      buffer_rx, BUF_SIZE);

	(void)memset(buffer_rx, 0xff, BUF_SIZE);

	spi_loopback_transceive(spec, &tx, &rx, 2);

	spi_loopback_compare_bufs(buffer_tx, buffer_rx, tx_buf_size,
				  buffer_print_tx, buffer_print_rx);

	static const uint8_t all_zeroes_buf[BUF_SIZE] = {0};

	spi_loopback_compare_bufs(all_zeroes_buf, buffer_rx + tx_buf_size, BUF_SIZE - tx_buf_size,
				  buffer_print_tx, buffer_print_rx);
}

/* test transferring different buffers on the same dma channels */
ZTEST(spi_loopback, test_spi_complete_large_transfers)
{
	struct spi_dt_spec *spec = loopback_specs[spec_idx];
	const struct spi_buf_set tx = spi_loopback_setup_xfer(tx_bufs_pool, 1,
							      large_buffer_tx, BUF3_SIZE);
	const struct spi_buf_set rx = spi_loopback_setup_xfer(rx_bufs_pool, 1,
							      large_buffer_rx, BUF3_SIZE);

	spi_loopback_transceive(spec, &tx, &rx, 2);

	zassert_false(memcmp(large_buffer_tx, large_buffer_rx, BUF3_SIZE),
			"Large Buffer contents are different");
}

ZTEST(spi_loopback, test_spi_null_tx_buf_set)
{
	struct spi_dt_spec *spec = loopback_specs[spec_idx];
	static const uint8_t expected_nop_return_buf[BUF_SIZE] = { 0 };
	const struct spi_buf_set rx = spi_loopback_setup_xfer(rx_bufs_pool, 1,
							      buffer_rx, BUF_SIZE);

	(void)memset(buffer_rx, 0x77, BUF_SIZE);

	spi_loopback_transceive(spec, NULL, &rx, 2);

	spi_loopback_compare_bufs(expected_nop_return_buf, buffer_rx, BUF_SIZE,
				  buffer_print_rx, buffer_print_rx);
}

ZTEST(spi_loopback, test_spi_null_rx_buf_set)
{
	struct spi_dt_spec *spec = loopback_specs[spec_idx];
	const struct spi_buf_set tx = spi_loopback_setup_xfer(tx_bufs_pool, 1,
							      buffer_tx, BUF_SIZE);

	spi_loopback_transceive(spec, &tx, NULL, 2);
}

ZTEST(spi_loopback, test_spi_null_tx_rx_buf_set)
{
	struct spi_dt_spec *spec = loopback_specs[spec_idx];

	spi_loopback_transceive(spec, NULL, NULL, 0);
}

ZTEST(spi_loopback, test_nop_nil_bufs)
{
	struct spi_dt_spec *spec = loopback_specs[spec_idx];
	const struct spi_buf_set tx = spi_loopback_setup_xfer(tx_bufs_pool, 1, NULL, 0);
	const struct spi_buf_set rx = spi_loopback_setup_xfer(rx_bufs_pool, 1, NULL, 0);

	spi_loopback_transceive(spec, &tx, &rx, 0);

	/* nothing really to check here, check is done in spi_loopback_transceive */
}

/* test using the same buffer set for RX and TX will write same data back */
ZTEST(spi_loopback, test_spi_write_back)
{
	struct spi_dt_spec *spec = loopback_specs[spec_idx];

	struct spi_buf buf = {.buf = buffer_rx, .len = BUF_SIZE};
	struct spi_buf_set set = {.buffers = &buf, .count = 1};

	memcpy(buffer_rx, tx_data, sizeof(tx_data));

	spi_loopback_transceive(spec, &set, &set, 2);

	spi_loopback_compare_bufs(tx_data, buffer_rx, BUF_SIZE,
				  buffer_print_tx, buffer_print_rx);
}

/* similar to test_spi_write_back, simulates the real common case of 1 word command */
ZTEST(spi_loopback, test_spi_same_buf_cmd)
{
	struct spi_dt_spec *spec = loopback_specs[spec_idx];

	struct spi_buf buf[2] = {
		{.buf = buffer_rx, .len = 1},
		{.buf = buffer_rx+1, .len = BUF_SIZE - 1}
	};

	const struct spi_buf_set tx = {.buffers = buf, .count = 1};
	const struct spi_buf_set rx = {.buffers = buf, .count = 2};

	memcpy(buffer_rx, tx_data, BUF_SIZE);

	spi_loopback_transceive(spec, &tx, &rx, 2);

	spi_loopback_compare_bufs(tx_data, buffer_rx, 1,
				  buffer_print_tx, buffer_print_rx);

	static const char zeros[BUF_SIZE - 1] = {0};

	zassert_ok(memcmp(buffer_rx+1, zeros, BUF_SIZE - 1));
}


static void spi_loopback_test_word_size(struct spi_dt_spec *spec,
					const void *tx_data,
					void *rx_buffer,
					const void *compare_data,
					size_t buffer_size,
					struct spi_dt_spec *spec_copy,
					uint8_t word_size)
{
	struct spi_config config_copy = spec->config;

	config_copy.operation &= ~SPI_WORD_SIZE_MASK;
	config_copy.operation |= SPI_WORD_SET(word_size);
	spec_copy->config = config_copy;
	spec_copy->bus = spec->bus;

	const struct spi_buf_set tx = spi_loopback_setup_xfer(tx_bufs_pool, 1,
							      tx_data, buffer_size);
	const struct spi_buf_set rx = spi_loopback_setup_xfer(rx_bufs_pool, 1,
							      rx_buffer, buffer_size);

	spi_loopback_transceive(spec_copy, &tx, &rx, 2);

	zassert_false(memcmp(compare_data, rx_buffer, buffer_size),
		      "%d-bit word buffer contents are different", word_size);
}

/* Test case for 7-bit word size transfers */
ZTEST(spi_loopback, test_spi_word_size_7)
{
	struct spi_dt_spec *spec = loopback_specs[spec_idx];

	spi_loopback_test_word_size(spec, tx_data, buffer_rx, tx_data,
				    sizeof(tx_data), &spec_copies[0], 7);
}

/* Test case for 9-bit word size transfers */
ZTEST(spi_loopback, test_spi_word_size_9)
{
	struct spi_dt_spec *spec = loopback_specs[spec_idx];

	static __BUF_ALIGN __NOCACHE uint16_t tx_data_9[BUFWIDE_SIZE];

	for (int i = 0; i < BUFWIDE_SIZE; i++) {
		tx_data_9[i] = tx_data_16[i] & 0x1FF;
	}

	spi_loopback_test_word_size(spec, tx_data_9, buffer_rx_16, tx_data_9,
				    sizeof(tx_data_9), &spec_copies[1], 9);
}

/* Test case for 16-bit word size transfers */
ZTEST(spi_loopback, test_spi_word_size_16)
{
	struct spi_dt_spec *spec = loopback_specs[spec_idx];

	spi_loopback_test_word_size(spec, buffer_tx_16, buffer_rx_16, tx_data_16,
				    sizeof(buffer_tx_16), &spec_copies[2], 16);
}

/* Test case for 24-bit word size transfers */
ZTEST(spi_loopback, test_spi_word_size_24)
{
	struct spi_dt_spec *spec = loopback_specs[spec_idx];

	static __BUF_ALIGN __NOCACHE uint32_t tx_data_24[BUFWIDE_SIZE];

	for (int i = 0; i < BUFWIDE_SIZE; i++) {
		tx_data_24[i] = tx_data_32[i] & 0xFFFFFF;
	}

	spi_loopback_test_word_size(spec, tx_data_24, buffer_rx_32, tx_data_24,
				    sizeof(tx_data_24), &spec_copies[3], 24);
}

/* Test case for 32-bit word size transfers */
ZTEST(spi_loopback, test_spi_word_size_32)
{
	struct spi_dt_spec *spec = loopback_specs[spec_idx];

	spi_loopback_test_word_size(spec, buffer_tx_32, buffer_rx_32, tx_data_32,
				    sizeof(buffer_tx_32), &spec_copies[4], 32);
}

static K_THREAD_STACK_DEFINE(thread_stack[3], CONFIG_ZTEST_STACK_SIZE +
					      CONFIG_TEST_EXTRA_STACK_SIZE);
static struct k_thread thread[3];

static K_SEM_DEFINE(thread_sem, 0, 3);
static K_SEM_DEFINE(sync_sem, 0, 1);

static uint8_t __BUF_ALIGN tx_buffer[3][32] __NOCACHE;
static uint8_t __BUF_ALIGN rx_buffer[3][32] __NOCACHE;

atomic_t thread_test_fails;

static void spi_transfer_thread(void *p1, void *p2, void *p3)
{
	struct spi_dt_spec *spec = (struct spi_dt_spec *)p1;
	uint8_t *tx_buf_ptr = (uint8_t *)p2;
	uint8_t *rx_buf_ptr = (uint8_t *)p3;
	int ret = 0;

	/* Wait for all threads to be ready */
	k_sem_give(&thread_sem);
	/* Perform SPI transfer */
	const struct spi_buf_set tx_bufs = {
		.buffers = &(struct spi_buf) {
			.buf = tx_buf_ptr,
			.len = 32,
		},
		.count = 1,
	};
	const struct spi_buf_set rx_bufs = {
		.buffers = &(struct spi_buf) {
			.buf = rx_buf_ptr,
			.len = 32,
		},
		.count = 1,
	};

	k_sem_take(&sync_sem, K_FOREVER);

	ret = spi_transceive_dt(spec, &tx_bufs, &rx_bufs);
	if (ret) {
		TC_PRINT("SPI concurrent transfer failed, spec %p\n", spec);
		atomic_inc(&thread_test_fails);
	}

	ret = memcmp(tx_buf_ptr, rx_buf_ptr, 32);
	if (ret) {
		TC_PRINT("SPI concurrent transfer data mismatch, spec %p\n", spec);
		atomic_inc(&thread_test_fails);
	}
}

/* Test case for concurrent SPI transfers */
static void test_spi_concurrent_transfer_helper(struct spi_dt_spec **specs)
{
	/* Create three threads */
	for (int i = 0; i < 3; i++) {
		memset(tx_buffer[i], 0xaa, sizeof(tx_buffer[i]));
		memset(rx_buffer[i], 0, sizeof(rx_buffer[i]));
		k_thread_create(&thread[i], thread_stack[i],
				K_THREAD_STACK_SIZEOF(thread_stack[i]),
				spi_transfer_thread, specs[i],
				tx_buffer[i], rx_buffer[i],
				K_PRIO_PREEMPT(10), 0, K_NO_WAIT);
	}

	/* Wait for all threads to be ready */
	for (int i = 0; i < 3; i++) {
		k_sem_take(&thread_sem, K_FOREVER);
	}

	atomic_set(&thread_test_fails, 0);

	/* Start all threads simultaneously */
	for (int i = 0; i < 3; i++) {
		k_sem_give(&sync_sem);
	}

	/* Wait for threads to complete */
	for (int i = 0; i < 3; i++) {
		k_thread_join(&thread[i], K_FOREVER);
	}

	zassert_equal(atomic_get(&thread_test_fails), 0);
}

/* test for multiple threads accessing the driver / bus with the same spi_config */
ZTEST(spi_loopback, test_spi_concurrent_transfer_same_spec)
{
	struct spi_dt_spec *specs[3] = {
		loopback_specs[spec_idx],
		loopback_specs[spec_idx],
		loopback_specs[spec_idx]
	};
	test_spi_concurrent_transfer_helper(specs);
}

/* test for multiple threads accessing the driver / bus with different spi_config
 * (different address of the config is what's important here
 */
ZTEST(spi_loopback, test_spi_concurrent_transfer_different_spec)
{
	struct spi_dt_spec *specs[3] = {
		&spec_copies[0],
		&spec_copies[1],
		&spec_copies[2]
	};
	spec_copies[0] = *loopback_specs[spec_idx];
	spec_copies[1] = *loopback_specs[spec_idx];
	spec_copies[2] = *loopback_specs[spec_idx];

	test_spi_concurrent_transfer_helper(specs);
}

ZTEST(spi_loopback, test_spi_deinit)
{
	struct spi_dt_spec *spec = loopback_specs[0];
	const struct device *dev = spec->bus;
	int ret;

	if (miso_pin.port == NULL || mosi_pin.port == NULL) {
		TC_PRINT("  zephyr,user miso-gpios or mosi-gpios are not defined\n");
		ztest_test_skip();
	}

	ret = device_deinit(dev);
	if (ret == -ENOTSUP) {
		TC_PRINT("  device deinit not supported\n");
		ztest_test_skip();
	}

	zassert_ok(ret);
	zassert_ok(gpio_pin_configure_dt(&miso_pin, GPIO_INPUT));
	zassert_ok(gpio_pin_configure_dt(&mosi_pin, GPIO_OUTPUT_INACTIVE));
	zassert_equal(gpio_pin_get_dt(&miso_pin), 0);
	zassert_ok(gpio_pin_set_dt(&mosi_pin, 1));
	zassert_equal(gpio_pin_get_dt(&miso_pin), 1);
	zassert_ok(gpio_pin_set_dt(&mosi_pin, 0));
	zassert_equal(gpio_pin_get_dt(&miso_pin), 0);
	zassert_ok(gpio_pin_configure_dt(&mosi_pin, GPIO_INPUT));
	zassert_ok(device_init(dev));
}

#if (CONFIG_SPI_ASYNC)
static struct k_poll_signal async_sig = K_POLL_SIGNAL_INITIALIZER(async_sig);
static struct k_poll_event async_evt =
	K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
				 K_POLL_MODE_NOTIFY_ONLY,
				 &async_sig);
static K_SEM_DEFINE(caller, 0, 1);
static K_SEM_DEFINE(start_async, 0, 1);
static int result = 1;

static void spi_async_call_cb(void *p1,
			      void *p2,
			      void *p3)
{
	ARG_UNUSED(p3);

	struct k_poll_event *evt = p1;
	struct k_sem *caller_sem = p2;

	TC_PRINT("Polling...");

	while (1) {
		k_sem_take(&start_async, K_FOREVER);

		zassert_false(k_poll(evt, 1, K_MSEC(2000)), "one or more events are not ready");

		result = evt->signal->result;
		k_sem_give(caller_sem);

		/* Reinitializing for next call */
		evt->signal->signaled = 0U;
		evt->state = K_POLL_STATE_NOT_READY;
	}
}

ZTEST(spi_loopback, test_spi_async_call)
{
	struct spi_dt_spec *spec = loopback_specs[spec_idx];
	const struct spi_buf_set tx = spi_loopback_setup_xfer(tx_bufs_pool, 3,
							      buffer_tx, BUF_SIZE,
							      buffer2_tx, BUF2_SIZE,
							      large_buffer_tx, BUF3_SIZE);
	const struct spi_buf_set rx = spi_loopback_setup_xfer(rx_bufs_pool, 3,
							      buffer_rx, BUF_SIZE,
							      buffer2_rx, BUF2_SIZE,
							      large_buffer_rx, BUF3_SIZE);

	memset(buffer_rx, 0, sizeof(buffer_rx));
	memset(buffer2_rx, 0, sizeof(buffer2_rx));
	memset(large_buffer_rx, 0, sizeof(large_buffer_rx));

	k_sem_give(&start_async);

	int ret = spi_transceive_signal(spec->bus, &spec->config, &tx, &rx, &async_sig);

	if (ret == -ENOTSUP) {
		TC_PRINT("Skipping ASYNC test");
		return;
	}

	zassert_false(ret, "SPI transceive failed, code %d", ret);

	k_sem_take(&caller, K_FOREVER);

	zassert_false(result, "SPI async transceive failed, result %d", result);

	spi_loopback_compare_bufs(buffer_tx, buffer_rx, BUF_SIZE,
				  buffer_print_tx, buffer_print_rx);
	spi_loopback_compare_bufs(buffer2_tx, buffer2_rx, BUF2_SIZE,
				  buffer_print_tx2, buffer_print_rx2);

	zassert_false(memcmp(large_buffer_tx, large_buffer_rx, BUF3_SIZE),
			"Large Buffer contents are different");
}
#endif

ZTEST(spi_extra_api_features, test_spi_lock_release)
{
	const struct spi_buf_set tx = spi_loopback_setup_xfer(tx_bufs_pool, 1,
							      buffer_tx, BUF_SIZE);
	const struct spi_buf_set rx = spi_loopback_setup_xfer(rx_bufs_pool, 1,
							      NULL, BUF_SIZE);
	struct spi_dt_spec *lock_spec = &spi_slow;
	struct spi_dt_spec *try_spec = &spi_fast;

	lock_spec->config.operation |= SPI_LOCK_ON;

	zassert_ok(pm_device_runtime_get(lock_spec->bus));
	spi_loopback_transceive(lock_spec, &tx, &rx, 2);
	zassert_false(spi_release_dt(lock_spec), "SPI release failed");
	zassert_ok(pm_device_runtime_put(lock_spec->bus));

	spi_loopback_transceive(try_spec, &tx, &rx, 2);

	lock_spec->config.operation &= ~SPI_LOCK_ON;
}

ZTEST(spi_extra_api_features, test_spi_hold_on_cs)
{
	const struct spi_buf_set tx = spi_loopback_setup_xfer(tx_bufs_pool, 1,
							      buffer_tx, BUF_SIZE);
	const struct spi_buf_set rx = spi_loopback_setup_xfer(rx_bufs_pool, 1,
							      NULL, BUF_SIZE);
	struct spi_dt_spec *hold_spec = &spi_slow;
	int ret = 0;

	hold_spec->config.operation |= SPI_HOLD_ON_CS;

	spi_loopback_gpio_cs_loopback_prepare();
	ret = spi_transceive_dt(hold_spec, &tx, &rx);
	if (ret == -ENOTSUP || ret == -EINVAL) {
		TC_PRINT("SPI hold on CS not supported");
		ret = 0;
		goto early_exit;
	} else if (ret) {
		goto early_exit;
	}
	/* Should get start assertion is 1 CS edge but no end */
	if (spi_loopback_gpio_cs_loopback_check(1)) {
		ret = -EIO;
		goto early_exit;
	}

	spi_loopback_gpio_cs_loopback_prepare();
	ret = spi_transceive_dt(hold_spec, &tx, &rx);
	if (ret) {
		goto early_exit;
	}
	/* CS is already asserted, and we still have hold on, so no edges */
	if (spi_loopback_gpio_cs_loopback_check(0)) {
		ret = -EIO;
		goto early_exit;
	}

	hold_spec->config.operation &= ~SPI_HOLD_ON_CS;

	spi_loopback_gpio_cs_loopback_prepare();
	ret = spi_transceive_dt(hold_spec, &tx, &rx);
	if (ret) {
		goto early_exit;
	}
	/* This time we don't have hold flag but it starts held, so only end trigger */
	if (spi_loopback_gpio_cs_loopback_check(1)) {
		ret = -EIO;
		goto early_exit;
	}

	/* now just do a normal transfer to make sure there was no leftover effects */
	spi_loopback_transceive(hold_spec, &tx, &rx, 2);

	return;

early_exit:
	hold_spec->config.operation &= ~SPI_HOLD_ON_CS;
	zassert_false(ret, "SPI transceive failed, code %d", ret);
	/* if there was no error then it was meant to be a skip at this point */
	ztest_test_skip();
}

/*
 *************************
 * Test suite definition *
 *************************
 */

static void *spi_loopback_common_setup(void)
{
	memset(buffer_tx, 0, sizeof(buffer_tx));
	memcpy(buffer_tx, tx_data, sizeof(tx_data));
	memset(buffer2_tx, 0, sizeof(buffer2_tx));
	memcpy(buffer2_tx, tx2_data, sizeof(tx2_data));
	memset(large_buffer_tx, 0, sizeof(large_buffer_tx));
	memcpy(large_buffer_tx, large_tx_data, sizeof(large_tx_data));
	memset(buffer_tx_16, 0, sizeof(buffer_tx_16));
	memcpy(buffer_tx_16, tx_data_16, sizeof(tx_data_16));
	memset(buffer_tx_32, 0, sizeof(buffer_tx_32));
	memcpy(buffer_tx_32, tx_data_32, sizeof(tx_data_32));
	return NULL;
}

static void *spi_loopback_setup(void)
{
	printf("Testing loopback spec: %s\n", spec_names[spec_idx]);
	spi_loopback_common_setup();
	return NULL;
}

static void run_after_suite(void *unused)
{
	spec_idx++;
}

static void run_after_lock(void *unused)
{
	spi_release_dt(&spi_fast);
	spi_release_dt(&spi_slow);
	spi_slow.config.operation &= ~SPI_LOCK_ON;
	spi_fast.config.operation &= ~SPI_LOCK_ON;
	spi_slow.config.operation &= ~SPI_HOLD_ON_CS;
}

ZTEST_SUITE(spi_loopback, NULL, spi_loopback_setup, NULL, NULL, run_after_suite);
ZTEST_SUITE(spi_extra_api_features, NULL, spi_loopback_common_setup, NULL, NULL, run_after_lock);

struct k_thread async_thread;
k_tid_t async_thread_id;
#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
K_THREAD_STACK_DEFINE(spi_async_stack, STACK_SIZE);

void test_main(void)
{
	printf("SPI test on buffers TX/RX %p/%p, frame size = %d"
#ifdef CONFIG_DMA
		", DMA enabled"
#ifndef CONFIG_NOCACHE_MEMORY
		" (without CONFIG_NOCACHE_MEMORY)"
#endif
#endif /* CONFIG_DMA */
			"\n",
			buffer_tx,
			buffer_rx,
			FRAME_SIZE);

#if (CONFIG_SPI_ASYNC)
	async_thread_id = k_thread_create(&async_thread,
					  spi_async_stack, STACK_SIZE,
					  spi_async_call_cb,
					  &async_evt, &caller, NULL,
					  K_PRIO_COOP(7), 0, K_NO_WAIT);
#endif

	zassert_false(spi_loopback_gpio_cs_loopback_init());

	ztest_run_all(NULL, false, ARRAY_SIZE(loopback_specs), 1);

#if (CONFIG_SPI_ASYNC)
	k_thread_abort(async_thread_id);
#endif

	ztest_verify_all_test_suites_ran();
}

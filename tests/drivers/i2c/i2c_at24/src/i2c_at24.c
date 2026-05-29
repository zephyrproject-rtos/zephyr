/*
 * Copyright (c) 2025 Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * I2C controller driver test using AT24 EEPROM as target fixture.
 * Validates raw I2C, RTIO, and PM APIs.
 * CONFIG_EEPROM=n prevents driver contention.
 */

#include <string.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#ifdef CONFIG_PM_DEVICE_RUNTIME
#include <zephyr/pm/device_runtime.h>
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_ALIAS(eeprom_0)) && \
	DT_NODE_HAS_COMPAT(DT_ALIAS(eeprom_0), atmel_at24)
#define EEPROM_NODE DT_ALIAS(eeprom_0)
#elif DT_NUM_INST_STATUS_OKAY(atmel_at24) == 1
#define EEPROM_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(atmel_at24)
#elif DT_HAS_COMPAT_STATUS_OKAY(atmel_at24)
#error "Multiple AT24 EEPROMs found; define the 'eeprom-0' alias to select one"
#else
#error "Add an enabled AT24-compatible EEPROM node or define " \
	"the 'eeprom-0' alias"
#endif

#define EEPROM_SIZE		DT_PROP(EEPROM_NODE, size)
#define PAGE_SIZE		DT_PROP(EEPROM_NODE, pagesize)
#define ADDR_WIDTH		DT_PROP(EEPROM_NODE, address_width)
#define ADDR_BYTES		(ADDR_WIDTH / 8)
#define EEPROM_WRITE_TIME_MS	DT_PROP(EEPROM_NODE, timeout)

BUILD_ASSERT((ADDR_WIDTH == 8) || (ADDR_WIDTH == 16),
	     "EEPROM address-width must be 8 or 16 bits");
BUILD_ASSERT((ADDR_WIDTH % 8) == 0,
	     "EEPROM address-width must be a whole number of bytes");
BUILD_ASSERT(PAGE_SIZE > 0, "EEPROM pagesize must be greater than zero");
BUILD_ASSERT(EEPROM_SIZE > 0, "EEPROM size must be greater than zero");
BUILD_ASSERT(PAGE_SIZE <= EEPROM_SIZE,
	     "EEPROM pagesize must not exceed the total device size");
BUILD_ASSERT(!DT_PROP_OR(EEPROM_NODE, read_only, 0),
	     "i2c_at24 test requires a writable EEPROM");
BUILD_ASSERT(IS_POWER_OF_TWO(PAGE_SIZE),
	     "EEPROM pagesize must be a power of two");

#define TEST_AREA_BASE 0x00

static const struct i2c_dt_spec eeprom = I2C_DT_SPEC_GET(EEPROM_NODE);
static uint8_t tx_buf[ADDR_BYTES + PAGE_SIZE];
static uint8_t rx_buf[PAGE_SIZE];

/* Encode EEPROM memory address into buffer (8 or 16 bit). */
static void eeprom_addr_encode(uint8_t *buf, uint16_t mem_addr)
{
	if (ADDR_BYTES == 2) {
		buf[0] = (uint8_t)(mem_addr >> 8);
		buf[1] = (uint8_t)(mem_addr & 0xFF);
	} else {
		buf[0] = (uint8_t)(mem_addr & 0xFF);
	}
}

/* Write up to one page to EEPROM (blocking). */
static void eeprom_page_write(uint16_t mem_addr, const uint8_t *data,
			      size_t len)
{
	int ret;

	zassert_true(len <= PAGE_SIZE, "Write exceeds page size");
	eeprom_addr_encode(tx_buf, mem_addr);
	memcpy(&tx_buf[ADDR_BYTES], data, len);

	ret = i2c_write(eeprom.bus, tx_buf, ADDR_BYTES + len, eeprom.addr);
	zassert_ok(ret, "EEPROM page write failed: %d", ret);
	k_msleep(EEPROM_WRITE_TIME_MS);
}

/* Read from EEPROM using write-read transaction (blocking). */
static void eeprom_read(uint16_t mem_addr, uint8_t *buf, size_t len)
{
	uint8_t addr_buf[ADDR_BYTES];
	int ret;

	eeprom_addr_encode(addr_buf, mem_addr);
	ret = i2c_write_read(eeprom.bus, eeprom.addr, addr_buf,
			     ADDR_BYTES, buf, len);
	zassert_ok(ret, "EEPROM read failed: %d", ret);
}

/* Fill buffer with incrementing pattern starting from seed. */
static void fill_pattern(uint8_t *buf, size_t len, uint8_t seed)
{
	for (size_t i = 0; i < len; i++) {
		buf[i] = (seed + (uint8_t)i) & 0xFF;
	}
}

/* Test suite setup: verify I2C bus is ready. */
static void *i2c_at24_setup(void)
{
	zassert_true(i2c_is_ready_dt(&eeprom), "I2C bus device is not ready");
	return NULL;
}

/* Pre-test hook: clear buffers, acquire PM runtime if enabled. */
static void i2c_at24_before(void *f)
{
	ARG_UNUSED(f);
	memset(rx_buf, 0, sizeof(rx_buf));
	memset(tx_buf, 0, sizeof(tx_buf));

#ifdef CONFIG_PM_DEVICE_RUNTIME
	pm_device_runtime_get(eeprom.bus);
#endif
}

/* Post-test hook: release PM runtime if enabled. */
static void i2c_at24_after(void *f)
{
	ARG_UNUSED(f);

#ifdef CONFIG_PM_DEVICE_RUNTIME
	pm_device_runtime_put(eeprom.bus);
#endif
}

/* Test basic page write and read. */
ZTEST(i2c_at24, test_write_read_basic)
{
	uint8_t pattern[PAGE_SIZE];

	fill_pattern(pattern, sizeof(pattern), 0xA0);
	eeprom_page_write(TEST_AREA_BASE, pattern, sizeof(pattern));
	eeprom_read(TEST_AREA_BASE, rx_buf, sizeof(pattern));

	zassert_mem_equal(pattern, rx_buf, sizeof(pattern),
			  "Basic write/read data mismatch");
}

/* Test repeated-start read transfer. */
ZTEST(i2c_at24, test_transfer_restart)
{
	struct i2c_msg msgs[2];
	uint8_t addr_buf[ADDR_BYTES];
	uint8_t pattern[PAGE_SIZE];
	int ret;

	fill_pattern(pattern, sizeof(pattern), 0xB0);
	eeprom_page_write(TEST_AREA_BASE, pattern, sizeof(pattern));

	eeprom_addr_encode(addr_buf, TEST_AREA_BASE);

	msgs[0].buf = addr_buf;
	msgs[0].len = ADDR_BYTES;
	msgs[0].flags = I2C_MSG_WRITE;

	msgs[1].buf = rx_buf;
	msgs[1].len = sizeof(pattern);
	msgs[1].flags = I2C_MSG_RESTART | I2C_MSG_READ | I2C_MSG_STOP;

	ret = i2c_transfer(eeprom.bus, msgs, 2, eeprom.addr);
	zassert_ok(ret, "i2c_transfer (restart read) failed: %d", ret);

	zassert_mem_equal(pattern, rx_buf, sizeof(pattern),
			  "Data mismatch on transfer restart read");
}

/* Test burst write/read helpers (8-bit address devices only). */
ZTEST(i2c_at24, test_burst_write_read)
{
	uint8_t pattern[PAGE_SIZE];
	int ret;

	if (ADDR_BYTES != 1) {
		ztest_test_skip();
		return;
	}

	fill_pattern(pattern, sizeof(pattern), 0xC0);

	ret = i2c_burst_write(eeprom.bus, eeprom.addr,
				(uint8_t)TEST_AREA_BASE, pattern,
				sizeof(pattern));
	zassert_ok(ret, "i2c_burst_write failed: %d", ret);
	k_msleep(EEPROM_WRITE_TIME_MS);

	ret = i2c_burst_read(eeprom.bus, eeprom.addr,
			     (uint8_t)TEST_AREA_BASE, rx_buf,
			     sizeof(pattern));
	zassert_ok(ret, "i2c_burst_read failed: %d", ret);

	zassert_mem_equal(pattern, rx_buf, sizeof(pattern),
			  "Burst write/read data mismatch");
}

/* Test register byte read/write (8-bit address devices only). */
ZTEST(i2c_at24, test_reg_byte_rw)
{
	uint8_t val;
	int ret;

	if (ADDR_BYTES != 1) {
		ztest_test_skip();
		return;
	}

	ret = i2c_reg_write_byte(eeprom.bus, eeprom.addr,
				 (uint8_t)TEST_AREA_BASE, 0x5A);
	zassert_ok(ret, "i2c_reg_write_byte failed: %d", ret);
	k_msleep(EEPROM_WRITE_TIME_MS);

	ret = i2c_reg_read_byte(eeprom.bus, eeprom.addr,
				(uint8_t)TEST_AREA_BASE, &val);
	zassert_ok(ret, "i2c_reg_read_byte failed: %d", ret);
	zassert_equal(val, 0x5A,
		      "reg byte mismatch: expected 0x5A got 0x%02x",
		      val);
}

/* Test register byte update (8-bit address devices only). */
ZTEST(i2c_at24, test_reg_update_byte)
{
	uint8_t val;
	int ret;

	if (ADDR_BYTES != 1) {
		ztest_test_skip();
		return;
	}

	ret = i2c_reg_write_byte(eeprom.bus, eeprom.addr,
				 (uint8_t)TEST_AREA_BASE, 0xFF);
	zassert_ok(ret, "Pre-write failed: %d", ret);
	k_msleep(EEPROM_WRITE_TIME_MS);

	ret = i2c_reg_update_byte(eeprom.bus, eeprom.addr,
				  (uint8_t)TEST_AREA_BASE, 0x0F, 0x00);
	zassert_ok(ret, "i2c_reg_update_byte failed: %d", ret);
	k_msleep(EEPROM_WRITE_TIME_MS);

	ret = i2c_reg_read_byte(eeprom.bus, eeprom.addr,
				(uint8_t)TEST_AREA_BASE, &val);
	zassert_ok(ret, "Post-update read failed: %d", ret);
	zassert_equal(val, 0xF0,
		      "reg_update_byte: expected 0xF0 got 0x%02x",
		      val);
}

/* Test NACK response from invalid address. */
ZTEST(i2c_at24, test_nack_invalid_addr)
{
	uint8_t dummy;
	int ret;
	uint16_t bad_addr = eeprom.addr ^ BIT(3);

	ret = i2c_read(eeprom.bus, &dummy, 1, bad_addr);
	zassert_equal(ret, -EIO, "Expected -EIO for invalid addr, got %d", ret);

	eeprom_read(TEST_AREA_BASE, rx_buf, 1);
}

/* Test bus recovery (skipped if not supported). */
ZTEST(i2c_at24, test_bus_recovery)
{
	int ret;

	ret = i2c_recover_bus(eeprom.bus);
	if (ret == -ENOSYS) {
		ztest_test_skip();
		return;
	}

	zassert_ok(ret, "i2c_recover_bus failed: %d", ret);

	eeprom_read(TEST_AREA_BASE, rx_buf, 1);
}

/* Test data integrity with multiple patterns. */
ZTEST(i2c_at24, test_data_integrity_patterns)
{
	static const uint8_t seeds[] = {0x00, 0x55, 0xAA, 0xFF};
	uint8_t pattern[PAGE_SIZE];

	for (size_t i = 0; i < ARRAY_SIZE(seeds); i++) {
		fill_pattern(pattern, sizeof(pattern), seeds[i]);
		eeprom_page_write(TEST_AREA_BASE, pattern, sizeof(pattern));
		eeprom_read(TEST_AREA_BASE, rx_buf, sizeof(pattern));
		zassert_mem_equal(pattern, rx_buf, sizeof(pattern),
				  "Data integrity failed for seed 0x%02x",
				  seeds[i]);
	}
}

/* Test multi-message scatter-gather write. */
ZTEST(i2c_at24, test_transfer_multi_msg_write)
{
	struct i2c_msg msgs[2];
	uint8_t addr_buf[ADDR_BYTES];
	uint8_t pattern[PAGE_SIZE];
	int ret;

	fill_pattern(pattern, sizeof(pattern), 0xD0);
	eeprom_addr_encode(addr_buf, TEST_AREA_BASE);

	msgs[0].buf = addr_buf;
	msgs[0].len = ADDR_BYTES;
	msgs[0].flags = I2C_MSG_WRITE;

	msgs[1].buf = pattern;
	msgs[1].len = sizeof(pattern);
	msgs[1].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	ret = i2c_transfer(eeprom.bus, msgs, 2, eeprom.addr);
	zassert_ok(ret, "Multi-msg write failed: %d", ret);
	k_msleep(EEPROM_WRITE_TIME_MS);

	eeprom_read(TEST_AREA_BASE, rx_buf, sizeof(pattern));
	zassert_mem_equal(pattern, rx_buf, sizeof(pattern),
			  "Data mismatch on scatter-gather write");
}

/* Test consecutive reads return identical data. */
ZTEST(i2c_at24, test_read_consistency)
{
	uint8_t rd1[PAGE_SIZE];
	uint8_t rd2[PAGE_SIZE];
	uint8_t pattern[PAGE_SIZE];

	fill_pattern(pattern, sizeof(pattern), 0xE0);
	eeprom_page_write(TEST_AREA_BASE, pattern, sizeof(pattern));

	eeprom_read(TEST_AREA_BASE, rd1, sizeof(rd1));
	eeprom_read(TEST_AREA_BASE, rd2, sizeof(rd2));

	zassert_mem_equal(rd1, rd2, sizeof(rd1),
			  "Consecutive reads returned different data");
	zassert_mem_equal(pattern, rd1, sizeof(pattern),
			  "Read data does not match written pattern");
}

#ifdef CONFIG_I2C_CALLBACK
static K_SEM_DEFINE(cb_sem, 0, 1);

/* Async transfer completion callback. */
static void eeprom_transfer_cb(const struct device *dev, int result, void *data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(data);

	zassert_ok(result, "Async transfer error: %d", result);
	k_sem_give(&cb_sem);
}

/* Test async callback transfer (skipped if not supported). */
ZTEST(i2c_at24, test_transfer_async_cb)
{
	struct i2c_msg msgs[2];
	uint8_t addr_buf[ADDR_BYTES];
	uint8_t pattern[PAGE_SIZE];
	int ret;

	fill_pattern(pattern, sizeof(pattern), 0xF0);
	eeprom_page_write(TEST_AREA_BASE, pattern, sizeof(pattern));

	eeprom_addr_encode(addr_buf, TEST_AREA_BASE);

	msgs[0].buf = addr_buf;
	msgs[0].len = ADDR_BYTES;
	msgs[0].flags = I2C_MSG_WRITE;

	msgs[1].buf = rx_buf;
	msgs[1].len = sizeof(pattern);
	msgs[1].flags = I2C_MSG_RESTART | I2C_MSG_READ | I2C_MSG_STOP;

	k_sem_reset(&cb_sem);

	ret = i2c_transfer_cb_dt(&eeprom, msgs, ARRAY_SIZE(msgs),
				 eeprom_transfer_cb, NULL);
	if (ret == -ENOSYS) {
		ztest_test_skip();
		return;
	}

	zassert_ok(ret, "i2c_transfer_cb failed: %d", ret);

	zassert_ok(k_sem_take(&cb_sem, K_MSEC(1000)),
		   "Async callback timed out");

	zassert_mem_equal(pattern, rx_buf, sizeof(pattern),
			  "Async read data mismatch");
}
#endif /* CONFIG_I2C_CALLBACK */

#ifdef CONFIG_I2C_RTIO
#include <zephyr/rtio/rtio.h>

I2C_DT_IODEV_DEFINE(eeprom_iodev, EEPROM_NODE);
RTIO_DEFINE(eeprom_rtio, 8, 8);

/* Write up to one page using RTIO. */
static void eeprom_page_write_rtio(uint16_t mem_addr, const uint8_t *data,
				     size_t len)
{
	struct rtio_sqe *sqe;
	struct rtio_cqe *cqe;
	int ret;

	zassert_true(len <= PAGE_SIZE, "Write exceeds page size");
	eeprom_addr_encode(tx_buf, mem_addr);
	memcpy(&tx_buf[ADDR_BYTES], data, len);

	sqe = rtio_sqe_acquire(&eeprom_rtio);
	zassert_not_null(sqe, "SQE pool exhausted");
	rtio_sqe_prep_write(sqe, &eeprom_iodev, RTIO_PRIO_NORM,
			    tx_buf, ADDR_BYTES + len, NULL);
	sqe->iodev_flags |= RTIO_IODEV_I2C_STOP;

	ret = rtio_submit(&eeprom_rtio, 1);
	zassert_ok(ret, "RTIO submit failed: %d", ret);

	do {
		cqe = rtio_cqe_consume(&eeprom_rtio);
	} while (cqe == NULL);
	ret = cqe->result;
	rtio_cqe_release(&eeprom_rtio, cqe);
	zassert_ok(ret, "EEPROM page write failed: %d", ret);

	k_msleep(EEPROM_WRITE_TIME_MS);
}

/* Read from EEPROM using RTIO write-read transaction. */
static void eeprom_read_rtio(uint16_t mem_addr, uint8_t *buf, size_t len)
{
	struct rtio_sqe *wr_sqe, *rd_sqe;
	struct rtio_cqe *cqe;
	uint8_t addr_buf[ADDR_BYTES];
	int ret;

	eeprom_addr_encode(addr_buf, mem_addr);

	wr_sqe = rtio_sqe_acquire(&eeprom_rtio);
	rd_sqe = rtio_sqe_acquire(&eeprom_rtio);
	zassert_not_null(wr_sqe, "SQE pool exhausted");
	zassert_not_null(rd_sqe, "SQE pool exhausted");

	rtio_sqe_prep_write(wr_sqe, &eeprom_iodev, RTIO_PRIO_NORM,
			    addr_buf, ADDR_BYTES, NULL);
	rtio_sqe_prep_read(rd_sqe, &eeprom_iodev, RTIO_PRIO_NORM,
			   buf, len, NULL);

	wr_sqe->flags |= RTIO_SQE_TRANSACTION;
	rd_sqe->iodev_flags |= RTIO_IODEV_I2C_RESTART |
			       RTIO_IODEV_I2C_STOP;

	ret = rtio_submit(&eeprom_rtio, 2);
	zassert_ok(ret, "RTIO submit failed: %d", ret);

	cqe = rtio_cqe_consume(&eeprom_rtio);
	zassert_not_null(cqe, "missing write CQE");
	ret = cqe->result;
	rtio_cqe_release(&eeprom_rtio, cqe);
	zassert_ok(ret, "EEPROM address write failed: %d", ret);

	cqe = rtio_cqe_consume(&eeprom_rtio);
	zassert_not_null(cqe, "missing read CQE");
	ret = cqe->result;
	rtio_cqe_release(&eeprom_rtio, cqe);
	zassert_ok(ret, "EEPROM read failed: %d", ret);
}

/* Test basic RTIO page write and read. */
ZTEST(i2c_at24, test_write_read_basic_rtio)
{
	uint8_t pattern[PAGE_SIZE];

	fill_pattern(pattern, sizeof(pattern), 0xA0);
	eeprom_page_write_rtio(TEST_AREA_BASE, pattern, sizeof(pattern));
	eeprom_read_rtio(TEST_AREA_BASE, rx_buf, sizeof(pattern));

	zassert_mem_equal(pattern, rx_buf, sizeof(pattern),
			  "RTIO basic write/read data mismatch");
}

/* Test RTIO repeated-start read transfer. */
ZTEST(i2c_at24, test_transfer_restart_rtio)
{
	struct rtio_sqe *wr_sqe, *rd_sqe;
	struct rtio_cqe *cqe;
	uint8_t addr_buf[ADDR_BYTES];
	uint8_t pattern[PAGE_SIZE];
	int ret;

	fill_pattern(pattern, sizeof(pattern), 0xB0);
	eeprom_page_write_rtio(TEST_AREA_BASE, pattern, sizeof(pattern));

	eeprom_addr_encode(addr_buf, TEST_AREA_BASE);

	wr_sqe = rtio_sqe_acquire(&eeprom_rtio);
	rd_sqe = rtio_sqe_acquire(&eeprom_rtio);
	zassert_not_null(wr_sqe, "SQE pool exhausted");
	zassert_not_null(rd_sqe, "SQE pool exhausted");

	rtio_sqe_prep_write(wr_sqe, &eeprom_iodev, RTIO_PRIO_NORM,
			    addr_buf, ADDR_BYTES, NULL);
	rtio_sqe_prep_read(rd_sqe, &eeprom_iodev, RTIO_PRIO_NORM,
			   rx_buf, sizeof(pattern), NULL);

	wr_sqe->flags |= RTIO_SQE_TRANSACTION;
	rd_sqe->iodev_flags |= RTIO_IODEV_I2C_RESTART |
			       RTIO_IODEV_I2C_STOP;

	ret = rtio_submit(&eeprom_rtio, 2);
	zassert_ok(ret, "RTIO submit failed: %d", ret);

	cqe = rtio_cqe_consume(&eeprom_rtio);
	zassert_not_null(cqe, "missing write CQE");
	ret = cqe->result;
	rtio_cqe_release(&eeprom_rtio, cqe);
	zassert_ok(ret, "write failed: %d", ret);

	cqe = rtio_cqe_consume(&eeprom_rtio);
	zassert_not_null(cqe, "missing read CQE");
	ret = cqe->result;
	rtio_cqe_release(&eeprom_rtio, cqe);
	zassert_ok(ret, "read failed: %d", ret);

	zassert_mem_equal(pattern, rx_buf, sizeof(pattern),
			  "RTIO data mismatch on transfer restart read");
}

/* Test RTIO multi-message scatter-gather write. */
ZTEST(i2c_at24, test_transfer_multi_msg_write_rtio)
{
	struct rtio_sqe *wr_addr, *wr_data;
	struct rtio_cqe *cqe;
	uint8_t addr_buf[ADDR_BYTES];
	uint8_t pattern[PAGE_SIZE];
	int ret;

	fill_pattern(pattern, sizeof(pattern), 0xD0);
	eeprom_addr_encode(addr_buf, TEST_AREA_BASE);

	wr_addr = rtio_sqe_acquire(&eeprom_rtio);
	wr_data = rtio_sqe_acquire(&eeprom_rtio);
	zassert_not_null(wr_addr, "SQE pool exhausted");
	zassert_not_null(wr_data, "SQE pool exhausted");

	rtio_sqe_prep_write(wr_addr, &eeprom_iodev, RTIO_PRIO_NORM,
			    addr_buf, ADDR_BYTES, NULL);
	rtio_sqe_prep_write(wr_data, &eeprom_iodev, RTIO_PRIO_NORM,
			    pattern, sizeof(pattern), NULL);

	wr_addr->flags |= RTIO_SQE_TRANSACTION;
	wr_data->iodev_flags |= RTIO_IODEV_I2C_STOP;

	ret = rtio_submit(&eeprom_rtio, 2);
	zassert_ok(ret, "RTIO submit failed: %d", ret);

	cqe = rtio_cqe_consume(&eeprom_rtio);
	zassert_not_null(cqe, "missing address CQE");
	ret = cqe->result;
	rtio_cqe_release(&eeprom_rtio, cqe);
	zassert_ok(ret, "address write failed: %d", ret);

	cqe = rtio_cqe_consume(&eeprom_rtio);
	zassert_not_null(cqe, "missing data CQE");
	ret = cqe->result;
	rtio_cqe_release(&eeprom_rtio, cqe);
	zassert_ok(ret, "data write failed: %d", ret);

	k_msleep(EEPROM_WRITE_TIME_MS);

	eeprom_read_rtio(TEST_AREA_BASE, rx_buf, sizeof(pattern));
	zassert_mem_equal(pattern, rx_buf, sizeof(pattern),
			  "RTIO data mismatch on scatter-gather write");
}

/* Test RTIO data integrity with multiple patterns. */
ZTEST(i2c_at24, test_data_integrity_patterns_rtio)
{
	static const uint8_t seeds[] = {0x00, 0x55, 0xAA, 0xFF};
	uint8_t pattern[PAGE_SIZE];

	for (size_t i = 0; i < ARRAY_SIZE(seeds); i++) {
		fill_pattern(pattern, sizeof(pattern), seeds[i]);
		eeprom_page_write_rtio(TEST_AREA_BASE, pattern, sizeof(pattern));
		eeprom_read_rtio(TEST_AREA_BASE, rx_buf, sizeof(pattern));
		zassert_mem_equal(pattern, rx_buf, sizeof(pattern),
				  "RTIO data integrity failed for seed 0x%02x",
				  seeds[i]);
	}
}
#endif /* CONFIG_I2C_RTIO */

ZTEST_SUITE(i2c_at24, NULL, i2c_at24_setup, i2c_at24_before, i2c_at24_after, NULL);

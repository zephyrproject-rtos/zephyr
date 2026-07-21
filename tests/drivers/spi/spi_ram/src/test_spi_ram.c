/*
 * Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @addtogroup t_spi_basic
 * @{
 * @defgroup t_spi_read_write test_spi_read_write
 * @brief TestPurpose: verify SPI master can read and write to FM25W256 SPI F-RAM
 * @}
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/tc_util.h>

/* ---------------------------------------------------------------------------
 * FM25W256 SPI F-RAM opcodes
 * Datasheet: https://www.infineon.com/cms/en/product/memories/f-ram/fm25w256/
 * ---------------------------------------------------------------------------
 */
#define FRAM_OPCODE_WREN  0x06U /* Write Enable */
#define FRAM_OPCODE_WRDI  0x04U /* Write Disable */
#define FRAM_OPCODE_RDSR  0x05U /* Read Status Register */
#define FRAM_OPCODE_WRSR  0x01U /* Write Status Register */
#define FRAM_OPCODE_READ  0x03U /* Read Memory Data */
#define FRAM_OPCODE_FSTRD 0x0BU /* Fast Read Memory Data */
#define FRAM_OPCODE_WRITE 0x02U /* Write Memory Data */
#define FRAM_OPCODE_RDID  0x9FU /* Read Device ID */
#define FRAM_OPCODE_SLEEP 0xB9U /* Enter Sleep Mode */

/* FM25W256 status register bit masks */
#define FRAM_SR_WEL  BIT(1) /* Write Enable Latch */
#define FRAM_SR_BP0  BIT(2) /* Block Protect 0 */
#define FRAM_SR_BP1  BIT(3) /* Block Protect 1 */
#define FRAM_SR_WPEN BIT(7) /* Write Protect Enable */

/* Number of address bytes in a FRAM command (opcode + addr_hi + addr_lo) */
#define FRAM_CMD_ADDR_LEN 3U

/* Maximum data bytes per single transfer (stack buffer limit) */
#define FRAM_MAX_XFER_LEN 64U

/* SPI frequency for the FM25W256 (max 40 MHz).
 * 8 MHz gives 6.25x oversampling at a 50 MHz analyzer sample rate.
 */
#define FRAM_SPI_FREQ_HZ 8000000U

#if DT_NODE_HAS_STATUS_OKAY(DT_ALIAS(spi_ram))
#define SPI_DEV_NODE DT_ALIAS(spi_ram)
#else
#error "Please set the correct SPI device and alias for spi_ram to be status okay"
#endif

/*
 * SPI configuration: 8-bit words, MSB first, mode 0 (CPOL=0, CPHA=0).
 * The FM25W256 also supports mode 3; mode 0 is used here.
 * CS is sourced from the cs-gpios property; the SPI framework asserts/
 * deasserts the GPIO automatically around each transfer.
 */
static struct spi_config spi_cfg = {
	.frequency = FRAM_SPI_FREQ_HZ,
	.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB,
	.slave = 0,
	.cs = {
		.gpio = GPIO_DT_SPEC_GET_BY_IDX(SPI_DEV_NODE, cs_gpios, 0),
		.delay = 10U, /* 10 µs CS setup/hold — aids analyzer capture */
		.cs_is_gpio = true,
	},
};

static const struct device *spi_dev = DEVICE_DT_GET(SPI_DEV_NODE);

/* Test payload written to / read from the FRAM */
static uint8_t tx_data[] = {'Z', 'e', 'p', 'h', 'y', 'r', '\n'};
static uint8_t rx_data[sizeof(tx_data)];

/* FRAM address cursor — incremented after each write/read test */
static uint16_t addr;

/* ---------------------------------------------------------------------------
 * Helper: send WREN command
 *
 * The Write Enable Latch (WEL) is cleared after every successful WRITE,
 * WRSR, power cycle, or WRDI.  It must be set before each WRITE/WRSR.
 * The FM25W256 requires WREN to arrive in its own CS-delimited transaction.
 * ---------------------------------------------------------------------------
 */
static int fram_write_enable(void)
{
	uint8_t wren_cmd = FRAM_OPCODE_WREN;
	struct spi_buf tx_buf = {.buf = &wren_cmd, .len = 1U};
	struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1U};

	return spi_write(spi_dev, &spi_cfg, &tx);
}

/* ---------------------------------------------------------------------------
 * Helper: write `len` bytes to the FRAM at `address`
 *
 * Flat single-buffer TX: [WRITE | addr_hi | addr_lo | data...]
 * Using a single spi_buf avoids multi-entry buf_set handling in the driver.
 * WREN is sent in a separate preceding transaction.
 * ---------------------------------------------------------------------------
 */
static int fram_write(uint16_t address, const uint8_t *data, size_t len)
{
	/* Combined: opcode + 2-byte address + data */
	uint8_t tx_buf[FRAM_CMD_ADDR_LEN + FRAM_MAX_XFER_LEN];
	int ret;

	__ASSERT_NO_MSG(len <= FRAM_MAX_XFER_LEN);

	tx_buf[0] = FRAM_OPCODE_WRITE;
	tx_buf[1] = (uint8_t)((address >> 8) & 0xFFU);
	tx_buf[2] = (uint8_t)(address & 0xFFU);
	memcpy(&tx_buf[FRAM_CMD_ADDR_LEN], data, len);

	struct spi_buf tx = {.buf = tx_buf, .len = FRAM_CMD_ADDR_LEN + len};
	struct spi_buf_set tx_set = {.buffers = &tx, .count = 1U};

	ret = fram_write_enable();
	if (ret) {
		return ret;
	}

	return spi_write(spi_dev, &spi_cfg, &tx_set);
}

/* ---------------------------------------------------------------------------
 * Helper: read `len` bytes from the FRAM starting at `address`
 *
 * Flat single-buffer full-duplex transceive:
 *   TX: [READ | addr_hi | addr_lo | 0x00 x len]
 *   RX: [ignored x 3              | data x len]
 * Using single equal-length spi_bufs avoids multi-entry and NULL-buf issues.
 * ---------------------------------------------------------------------------
 */
static int fram_read(uint16_t address, uint8_t *data, size_t len)
{
	const size_t total = FRAM_CMD_ADDR_LEN + len;
	uint8_t tx_buf[FRAM_CMD_ADDR_LEN + FRAM_MAX_XFER_LEN];
	uint8_t rx_buf[FRAM_CMD_ADDR_LEN + FRAM_MAX_XFER_LEN];
	int ret;

	__ASSERT_NO_MSG(len <= FRAM_MAX_XFER_LEN);

	memset(tx_buf, 0, total);
	tx_buf[0] = FRAM_OPCODE_READ;
	tx_buf[1] = (uint8_t)((address >> 8) & 0xFFU);
	tx_buf[2] = (uint8_t)(address & 0xFFU);

	struct spi_buf tx = {.buf = tx_buf, .len = total};
	struct spi_buf rx = {.buf = rx_buf, .len = total};
	struct spi_buf_set tx_set = {.buffers = &tx, .count = 1U};
	struct spi_buf_set rx_set = {.buffers = &rx, .count = 1U};

	ret = spi_transceive(spi_dev, &spi_cfg, &tx_set, &rx_set);
	if (ret == 0) {
		/* Data starts after the 3-byte command header in the RX buffer */
		memcpy(data, &rx_buf[FRAM_CMD_ADDR_LEN], len);
	}
	return ret;
}

/* ---------------------------------------------------------------------------
 * Helper: read the FRAM status register via RDSR
 *
 * Flat 2-byte full-duplex transceive:
 *   TX: [RDSR | 0x00]   RX: [ignored | status]
 * ---------------------------------------------------------------------------
 */
static int fram_read_status(uint8_t *status)
{
	uint8_t tx_buf[2] = {FRAM_OPCODE_RDSR, 0x00U};
	uint8_t rx_buf[2] = {0U, 0U};
	int ret;

	struct spi_buf tx = {.buf = tx_buf, .len = sizeof(tx_buf)};
	struct spi_buf rx = {.buf = rx_buf, .len = sizeof(rx_buf)};
	struct spi_buf_set tx_set = {.buffers = &tx, .count = 1U};
	struct spi_buf_set rx_set = {.buffers = &rx, .count = 1U};

	ret = spi_transceive(spi_dev, &spi_cfg, &tx_set, &rx_set);
	if (ret == 0) {
		*status = rx_buf[1];
	}
	return ret;
}

/* ---------------------------------------------------------------------------
 * Suite setup — runs once before all tests
 * ---------------------------------------------------------------------------
 */
static void *spi_ram_setup(void)
{
	zassert_true(device_is_ready(spi_dev), "SPI device is not ready");

	return NULL;
}

/* ---------------------------------------------------------------------------
 * Per-test setup — clears the receive buffer before each test
 * ---------------------------------------------------------------------------
 */
static void spi_ram_before(void *f)
{
	memset(rx_data, 0, sizeof(rx_data));
}

/* ---------------------------------------------------------------------------
 * test_ram_write_read
 *
 * Write a known payload to the FRAM, read it back, verify they match.
 * The address cursor advances after each write so successive test runs
 * (e.g. via twister) hit different FRAM cells.
 * ---------------------------------------------------------------------------
 */
ZTEST(spi_ram, test_ram_write_read)
{
	uint16_t test_addr = addr;

	TC_PRINT("SPI FRAM write/read from thread %p addr 0x%04x\n", k_current_get(), test_addr);

	zassert_ok(fram_write(test_addr, tx_data, sizeof(tx_data)), "SPI write to FRAM failed");

	zassert_ok(fram_read(test_addr, rx_data, sizeof(rx_data)), "SPI read from FRAM failed");

	zassert_equal(memcmp(tx_data, rx_data, sizeof(tx_data)), 0,
		      "Written and read data should match");

	addr += (uint16_t)sizeof(tx_data);
}

/* ---------------------------------------------------------------------------
 * test_ram_read_status
 *
 * Read the FRAM status register via RDSR and confirm the Write Enable
 * Latch (WEL) is clear.  After power-on or a completed WRITE, WEL should
 * be 0.
 * ---------------------------------------------------------------------------
 */
ZTEST(spi_ram, test_ram_read_status)
{
	uint8_t status = 0U;

	TC_PRINT("Reading FRAM status register\n");

	zassert_ok(fram_read_status(&status), "SPI RDSR command failed");

	TC_PRINT("FRAM status register: 0x%02x\n", status);

	zassert_equal(status & FRAM_SR_WEL, 0U, "WEL bit should be cleared at test start");
}

/* ---------------------------------------------------------------------------
 * test_ram_write_enable_disable
 *
 * Verify the Write Enable Latch (WEL) is set after WREN and cleared
 * after WRDI, using RDSR to sample the status register.
 * ---------------------------------------------------------------------------
 */
ZTEST(spi_ram, test_ram_write_enable_disable)
{
	uint8_t status = 0U;

	TC_PRINT("Testing WREN / WRDI latch behaviour\n");

	/* WREN: WEL should become set */
	zassert_ok(fram_write_enable(), "WREN command failed");

	status = 0U;
	zassert_ok(fram_read_status(&status), "RDSR after WREN failed");
	zassert_not_equal(status & FRAM_SR_WEL, 0U, "WEL bit should be set after WREN");

	/* WRDI: WEL should become clear */
	uint8_t wrdi_cmd = FRAM_OPCODE_WRDI;
	struct spi_buf wrdi_buf = {.buf = &wrdi_cmd, .len = 1U};
	struct spi_buf_set wrdi_tx = {.buffers = &wrdi_buf, .count = 1U};

	zassert_ok(spi_write(spi_dev, &spi_cfg, &wrdi_tx), "WRDI command failed");

	status = 0U;
	zassert_ok(fram_read_status(&status), "RDSR after WRDI failed");
	zassert_equal(status & FRAM_SR_WEL, 0U, "WEL bit should be cleared after WRDI");
}

/* ---------------------------------------------------------------------------
 * test_ram_sequential_write_read
 *
 * Writes 256 ordered 16-bit values (0x0000, 0x1111, ..., 0xFFFF) to the
 * FRAM and reads them back, verifying each byte matches.
 *
 * Total payload: 512 bytes, written in FRAM_MAX_XFER_LEN-byte chunks to
 * stay within the flat-buffer stack limit.
 * ---------------------------------------------------------------------------
 */
ZTEST(spi_ram, test_ram_sequential_write_read)
{
	/* Build expected pattern: 0x0000, 0x1111, 0x2222, ..., 0xFFFF
	 * Stored big-endian (high byte first) so the pattern is visible on
	 * the wire as 00 00  11 11  22 22 ... FF FF.
	 */
	static const uint16_t PATTERN_START;
	static const uint16_t PATTERN_STEP = 0x1111U;
	static const uint16_t PATTERN_COUNT = 256U;                  /* 0x0000..0xFFFF */
	const size_t total_bytes = PATTERN_COUNT * sizeof(uint16_t); /* 512 */
	const uint16_t base_addr = 0x0100U; /* write after the basic test area */

	uint8_t expected[PATTERN_COUNT * sizeof(uint16_t)];
	uint8_t actual[PATTERN_COUNT * sizeof(uint16_t)];

	/* Fill expected buffer */
	for (uint16_t i = 0; i < PATTERN_COUNT; i++) {
		uint16_t val = (uint16_t)(PATTERN_START + i * PATTERN_STEP);

		expected[i * 2U] = (uint8_t)(val >> 8);
		expected[i * 2U + 1U] = (uint8_t)(val & 0xFFU);
	}

	TC_PRINT("Writing %zu bytes sequential pattern to FRAM addr 0x%04x\n", total_bytes,
		 base_addr);

	/* Write in FRAM_MAX_XFER_LEN-byte chunks */
	for (size_t offset = 0; offset < total_bytes; offset += FRAM_MAX_XFER_LEN) {
		size_t chunk = MIN(FRAM_MAX_XFER_LEN, total_bytes - offset);
		uint16_t waddr = (uint16_t)(base_addr + offset);

		zassert_ok(fram_write(waddr, &expected[offset], chunk),
			   "Chunked FRAM write failed at offset %zu", offset);
	}

	TC_PRINT("Reading back %zu bytes\n", total_bytes);

	/* Read back in FRAM_MAX_XFER_LEN-byte chunks */
	memset(actual, 0, sizeof(actual));
	for (size_t offset = 0; offset < total_bytes; offset += FRAM_MAX_XFER_LEN) {
		size_t chunk = MIN(FRAM_MAX_XFER_LEN, total_bytes - offset);
		uint16_t raddr = (uint16_t)(base_addr + offset);

		zassert_ok(fram_read(raddr, &actual[offset], chunk),
			   "Chunked FRAM read failed at offset %zu", offset);
	}

	/* Verify every byte */
	for (size_t i = 0; i < total_bytes; i++) {
		zassert_equal(actual[i], expected[i],
			      "Mismatch at byte %zu: expected 0x%02x got 0x%02x", i, expected[i],
			      actual[i]);
	}

	TC_PRINT("Sequential pattern verified OK (%zu bytes)\n", total_bytes);
}

ZTEST_SUITE(spi_ram, NULL, spi_ram_setup, spi_ram_before, NULL, NULL);

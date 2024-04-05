/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 * Copyright (c) 2024 Microchip Technology, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <soc.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/sys_io.h>

#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app, CONFIG_LOG_DEFAULT_LEVEL);

#include <mec_rom_api.h>

#define MEC540X_ROM_OTP_92_IDX 92u
#define MEC540X_ROM_OTP_94_IDX 94u

#define FLASH_TEST_PAGE_ADDR 0x1000u
#define LED0_NODE DT_ALIAS(led0)
#define FLASH_SIZE_MBIT DT_PROP(DT_ALIAS(spi_flash), size)
#define FLASH_SIZE_BYTES (FLASH_SIZE_MBIT / 8u)
#define SPI0_LINES DT_PROP(DT_ALIAS(spi0), lines)

static const struct gpio_dt_spec led0_gpio_spec = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

static const struct device *spi_dev = DEVICE_DT_GET(DT_ALIAS(spi0));
static const struct device *spi_flash_dev = DEVICE_DT_GET(DT_ALIAS(spi_flash));

static const uint8_t spi_flash_page0_data[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
	0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
	0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
	0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
	0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
	0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
	0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
	0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
	0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
	0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
	0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
	0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
	0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
	0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,
};

const struct spi_config spi_cfg_fd_12m = {
	.frequency = MHZ(12),
	.operation = SPI_WORD_SET(8) | SPI_LINES_SINGLE,
};

#ifdef CONFIG_SPI_EXTENDED_MODES
const struct spi_config spi_cfg_dual_12m = {
	.frequency = MHZ(12),
	.operation = SPI_WORD_SET(8) | SPI_LINES_DUAL,
};

const struct spi_config spi_cfg_quad_12m = {
	.frequency = MHZ(12),
	.operation = SPI_WORD_SET(8) | SPI_LINES_QUAD,
};
#endif

static struct spi_config spi_cfg;
static volatile uint32_t spin_val;
static volatile int ret_val;

static void spin_on(uint32_t id, int rval);
static int spi_flash_read_status(int status_reg, uint8_t *status);
static int spi_flash_write_status(int status_reg, uint8_t status);
int buffer_filled_with(const uint8_t *buffer, size_t buffersz, uint8_t val);

struct spi_buf_set txbs;
struct spi_buf_set rxbs;
struct spi_buf txb[4];
struct spi_buf rxb[4];

uint8_t buf1[256];
uint8_t buf2[256];
uint8_t sector_buf[4096];

int main(void)
{
	size_t nflash_pages = 0;
	size_t flash_min_wr_block_size = 0;
	int ret, erased;
	uint32_t jedec_id, spi_addr;
#ifdef CONFIG_SPI_EXTENDED_MODES
	int ret2, ret3, ret4;
#endif
	uint16_t otp_index;
	uint8_t otp_data, spi_status;

	LOG_INF("MEC5 QSPI sample: board: %s", DT_N_P_compatible_IDX_0);

	jedec_id = mec_rom_version();
	LOG_INF("ROM version = 0x%08x", jedec_id);

	otp_index = MEC540X_ROM_OTP_92_IDX;
	otp_data = 0xffu;
	ret = mec_rom_otp_read_byte(otp_index, &otp_data);
	if (ret) {
		LOG_ERR("ROM Read OTP[%u] error: %d", otp_index, ret);
	} else {
		LOG_INF("ROM OTP[%u] = 0x%02x", otp_index, otp_data);
	}

	otp_index = MEC540X_ROM_OTP_94_IDX;
	otp_data = 0xffu;
	ret = mec_rom_otp_read_byte(otp_index, &otp_data);
	if (ret) {
		LOG_ERR("ROM Read OTP[%u] error: %d", otp_index, ret);
	} else {
		LOG_INF("ROM OTP[%u] = 0x%02x", otp_index, otp_data);
	}

	erased = 0;
	memset(txb, 0, sizeof(txb));
	memset(rxb, 0, sizeof(rxb));

	if (!gpio_is_ready_dt(&led0_gpio_spec)) {
		LOG_ERR("GPIO LED device is not ready! (%d)", -2);
		spin_on(1u, -2);
	}

	if (!device_is_ready(spi_dev)) {
		LOG_ERR("SPI device is not ready! (%d)", -3);
		spin_on(2u, -3);
	}

	if (!device_is_ready(spi_flash_dev)) {
		LOG_ERR("Flash device is not ready! (%d)", -3);
		spin_on(3u, -3);
	}

	ret = gpio_pin_configure_dt(&led0_gpio_spec, GPIO_OUTPUT_ACTIVE);
	if (ret) {
		LOG_ERR("LED GPIO configuration error (%d)", ret);
		spin_on(4u, ret);
	}

	gpio_pin_toggle_dt(&led0_gpio_spec);
	k_sleep(K_MSEC(1000));
	gpio_pin_toggle_dt(&led0_gpio_spec);
	k_sleep(K_MSEC(1000));
	gpio_pin_toggle_dt(&led0_gpio_spec);

	nflash_pages = flash_get_page_count(spi_flash_dev);
	LOG_INF("Number of flash pages = %u", nflash_pages);

	flash_min_wr_block_size = flash_get_write_block_size(spi_flash_dev);
	LOG_INF("Flash minimum write block size = %u", flash_min_wr_block_size);

	const struct flash_parameters *flash_params = flash_get_parameters(spi_flash_dev);

	if (!flash_params) {
		LOG_ERR("Flash get parameters failed %d", ret);
		spin_on(5u, ret);
	}

	LOG_INF("Flash write block size = 0x%0x  erase value = 0x%02x",
		flash_params->write_block_size, flash_params->erase_value);

	LOG_INF("Read JEDEC ID from flash");
	jedec_id = 0;
	ret = flash_read_jedec_id(spi_flash_dev, (uint8_t *)&jedec_id);
	if (ret) {
		LOG_ERR("Flash JEDEC ID read failed: %d", ret);
		spin_on(6u, ret);
	}
	LOG_HEXDUMP_INF(&jedec_id, 3, "JEDEC-ID");

#ifndef CONFIG_SPI_NOR_SFDP_DEVICETREE
	memset(buf1, 0x55, sizeof(buf1));
	LOG_INF("Read first 64-bytes of SFDP data from flash");
	ret = flash_sfdp_read(spi_flash_dev, 0, buf1, 64u);
	if (ret) {
		LOG_ERR("Flash SFDP read failed: %d", ret);
		spin_on(7u, ret);
	}

	LOG_HEXDUMP_INF(&buf1, 64, "SFDP data");
#endif

	spi_status = 0x80u;
	ret = spi_flash_read_status(1, &spi_status);
	if (ret) {
		LOG_ERR("Read SPI flash status using SPI driver error: %d", ret);
		goto app_exit;
	}
	LOG_INF("SPI Flash STATUS1 = 0x%02x", spi_status);

	if (jedec_id == 0x001840efu) {
		ret = spi_flash_read_status(2, &spi_status);
		if (ret) {
			LOG_ERR("Read SPI flash status 2 using SPI driver error: %d", ret);
			goto app_exit;
		}
		LOG_INF("SPI Flash STATUS2 = 0x%02x", spi_status);

		ret = spi_flash_read_status(3, &spi_status);
		if (ret) {
			LOG_ERR("Read SPI flash status 3 using SPI driver error: %d", ret);
			goto app_exit;
		}
		LOG_INF("SPI Flash STATUS3 = 0x%02x", spi_status);

		if (spi_status & BIT(2)) { /* WPS set? */
			/* clear it to factory default */
			LOG_INF("WPS(bit[2]) is set. Clear it to factory default");
			ret = spi_flash_write_status(3, spi_status & ~BIT(2));
			if (ret) {
				goto app_exit;
			}
		}
	}

	spi_addr = 0;
	for (size_t n = 0; n < (FLASH_SIZE_BYTES / 4096); n++) {
		LOG_INF("Check sector at 0x%0x", spi_addr);
		memset(sector_buf, 0x55, 4096u);
		spi_addr &= 0xfffff000u;
		ret = flash_read(spi_flash_dev, spi_addr, sector_buf, 4096u);
		if (ret) {
			LOG_ERR("Flash sector read failed %d", ret);
			goto app_exit;
		}
		erased = 1;
		for (size_t m = 0; m < 4096; m++) {
			if (sector_buf[m] != 0xffu) {
				erased = 0;
				break;
			}
		}
		if (erased) {
			LOG_INF("  Sector erased");
		} else {
			LOG_INF("  Sector NOT in erased state");
		}
		spi_addr += 4096u;
	}

	memset(buf2, 0x55, sizeof(buf2));

	spi_addr = 0u;
	LOG_INF("Read Page 0 using Flash driver and check if in erased state");
	ret = flash_read(spi_flash_dev, spi_addr, buf2, 256u);
	if (ret) {
		LOG_ERR("Flash read error %d", ret);
		goto app_exit;
	}

	ret = buffer_filled_with((const uint8_t *)buf2, 256u, 0xffu);
	if (ret) {
		LOG_ERR("Page 0 not erased!");
	} else {
		LOG_INF("Page 0 is erased");
	}

	if (ret) {
		spi_addr = 0; /* Erase Sector 0 to eliminate Boot-ROM TAGs */
		LOG_INF("Erase 4KB sector 0");
		ret = flash_erase(spi_flash_dev, spi_addr, 4096u);
		if (ret) {
			LOG_ERR("Flash erase sector 0 error %d", ret);
			goto app_exit;
		}

		memset(buf2, 0x55, sizeof(buf2));
		LOG_INF("Read Page 0 using Flash driver and check if in erased state");
		ret = flash_read(spi_flash_dev, spi_addr, buf2, 256u);
		if (ret) {
			LOG_ERR("Flash read error %d", ret);
			goto app_exit;
		}

		ret = buffer_filled_with((const uint8_t *)buf2, 256u, 0xffu);
		if (ret) {
			LOG_ERR("Page 0 not erased!");
			goto app_exit;
		} else {
			LOG_INF("Page 0 is erased");
		}
	}

	spi_addr = FLASH_TEST_PAGE_ADDR;
	/* Erase sector containing test page */
	LOG_INF("Erase 4KB sector containing test page at 0x%0x", spi_addr);
	ret = flash_erase(spi_flash_dev, spi_addr, 4096u);
	if (ret) {
		LOG_ERR("Flash erase page error %d", ret);
		goto app_exit;
	}

	memset(buf2, 0x55, sizeof(buf2));
	LOG_INF("Read test page using Flash driver and check if in erased state");
	ret = flash_read(spi_flash_dev, spi_addr, buf2, 256u);
	if (ret) {
		LOG_ERR("Flash read error %d", ret);
		goto app_exit;
	}

	ret = buffer_filled_with((const uint8_t *)buf2, 256u, 0xffu);
	if (ret) {
		LOG_ERR("Page not in erased state!");
		goto app_exit;
	}

	/* Write test page with known pattern */
	LOG_INF("Write known pattern to page at 0x%0x", spi_addr);
	ret = flash_write(spi_flash_dev, spi_addr, spi_flash_page0_data, 256u);
	if (ret) {
		LOG_ERR("Flash write page error %d", ret);
		goto app_exit;
	}

	memset(buf2, 0x55, sizeof(buf2));
	LOG_INF("Read test page using Flash driver and check validity");
	ret = flash_read(spi_flash_dev, spi_addr, buf2, 256u);
	if (ret) {
		LOG_ERR("Flash read error %d", ret);
		goto app_exit;
	}

	ret = memcmp(spi_flash_page0_data, buf2, 256);
	if (ret) {
		LOG_ERR("!!! Flash driver read back of page data mismatch !!!");
		goto app_exit;
	} else {
		LOG_INF("Flash driver read back of page data matches");
	}

	LOG_INF("Read test page using SPI driver with full-duplex style buffers");
	spi_cfg.frequency = MHZ(12);
	spi_cfg.operation = SPI_WORD_SET(8) | SPI_LINES_SINGLE;

	/* read 256 bytes: this method requires the second transmit buffer
	 * to be 256 bytes in size. We re-use the rx buffer.
	 */
	memset(buf2, 0x55, sizeof(buf2));
	buf1[0] = 0x03u; /* SPI Read 1-1-1-0 */
	buf1[1] = (spi_addr >> 16) & 0xffu;
	buf1[2] = (spi_addr >> 8) & 0xffu;
	buf1[3] = spi_addr & 0xffu;

	txb[0].buf = buf1;
	txb[0].len = 4u;
	txb[1].buf = buf2;
	txb[1].len = 256u;

	rxb[0].buf = buf2;
	rxb[0].len = 4u;
	rxb[1].buf = buf2;
	rxb[1].len = 256u;

	txbs.buffers = &txb[0];
	txbs.count = 2;
	rxbs.buffers = &rxb[0];
	rxbs.count = 2;

	ret = spi_transceive(spi_dev, (const struct spi_config *)&spi_cfg, &txbs, &rxbs);
	LOG_INF("SPI xfr 1 returned (%d)", ret);

	ret = memcmp(spi_flash_page0_data, buf2, 256u);
	LOG_INF("Compare to expected (%d)", ret);

	/* read 256 bytes: Requires modified SPI driver which interprets buffers
	 * with NULL ptr and non-zero length as: TX don't drive TX lines (0)
	 * RX discard data
	 */
	LOG_INF("Read test page using SPI driver with full-duplex NULL buffer technique");

	memset(buf2, 0x55, sizeof(buf2));
	buf1[0] = 0x03u; /* SPI Read 1-1-1-0 */
	buf1[1] = (spi_addr >> 16) & 0xffu;
	buf1[2] = (spi_addr >> 8) & 0xffu;
	buf1[3] = spi_addr & 0xffu;

	txb[0].buf = buf1;
	txb[0].len = 4u;
	txb[1].buf = NULL;
	txb[1].len = 256u;

	rxb[0].buf = NULL;
	rxb[0].len = 4u;
	rxb[1].buf = buf2;
	rxb[1].len = 256u;

	txbs.buffers = &txb[0];
	txbs.count = 2;
	rxbs.buffers = &rxb[0];
	rxbs.count = 2;

	ret = spi_transceive(spi_dev, (const struct spi_config *)&spi_cfg, &txbs, &rxbs);
	LOG_INF("SPI xfr 2 returned (%d)", ret);

	/* Passed */
	ret = memcmp(spi_flash_page0_data, buf2, 256u);
	LOG_INF("Compare to expected (%d)", ret);

#ifdef CONFIG_SPI_EXTENDED_MODES
	/* Read 8 bytes using SPI flash dual. SPI flash read opcode = 0x3B.
	 * Requires opcode, address, and 8 clocks with I/O lines tri-stated.
	 * Opcode and address transmitted in full-duplex mode
	 * 8 tri-state clocks require I/O lines tri-stated.
	 * Data is received on both I/O lines (half-duplex operation).
	 * SPI driver requires a new struct spi_config with dual lines.
	 * Driver must be built with CONFIG_SPI_EXTENDED_MODES=y
	 */
	LOG_INF("Read first 8 bytes of test page using SPI driver using 1-1-2-8 (dual) opcode");

	memset(buf2, 0x55, sizeof(buf2));
	buf1[0] = 0x3bu; /* SPI Read 1-1-2-8 */
	buf1[1] = (spi_addr >> 16) & 0xffu;
	buf1[2] = (spi_addr >> 8) & 0xffu;
	buf1[3] = spi_addr & 0xffu;

	txb[0].buf = buf1;
	txb[0].len = 4u;
	txb[1].buf = NULL;
	txb[1].len = 8; /* Tri-state clocks. len = number of clocks */

	rxb[0].buf = NULL;
	rxb[0].len = 4u;
	rxb[1].buf = NULL;
	rxb[1].len = 8u;

	txbs.buffers = &txb[0];
	txbs.count = 2;
	rxbs.buffers = &rxb[0];
	rxbs.count = 2;

	/* struct spi_config must be writeable.
	 * We use SPI_HOLD_ON_CS to instruct the driver to keep CS# asserted.
	 * We will clear SPI_HOLD_ON_CS on the last driver call.
	 * We use SPI_LOCK_ON to keep the SPI context semaphore owned by this
	 * spi_context to prevent other thread from being able to use the controller.
	 * We use API spi_release() to release the lock after transaction is completed.
	 */
	spi_cfg.operation |= SPI_HOLD_ON_CS | SPI_LOCK_ON;

	ret = spi_transceive(spi_dev, (const struct spi_config *)&spi_cfg, &txbs, &rxbs);

	rxb[0].buf = buf2;
	rxb[0].len = 8u;

	rxbs.buffers = &rxb[0];
	rxbs.count = 1;

	spi_cfg.operation &= ~(SPI_HOLD_ON_CS | SPI_LINES_MASK);
	spi_cfg.operation |= SPI_LINES_DUAL;

	ret2 = spi_transceive(spi_dev, (const struct spi_config *)&spi_cfg, NULL, &rxbs);

	ret3 = spi_release(spi_dev, (const struct spi_config *)&spi_cfg);

	ret4 = memcmp(spi_flash_page0_data, buf2, 8u);
	LOG_INF("Dual read of 8 bytes returns: (%d) (%d) (%d) (%d)", ret, ret2, ret3, ret4);

	/* Dual read 256 bytes */
	LOG_INF("Read test page using SPI driver using 1-1-2-8 (dual) opcode");
	spi_cfg.operation = SPI_WORD_SET(8) | SPI_LINES_SINGLE;

	memset(buf2, 0x55, sizeof(buf2));
	buf1[0] = 0x3bu; /* SPI Read 1-1-2-8 */
	buf1[1] = (spi_addr >> 16) & 0xffu;
	buf1[2] = (spi_addr >> 8) & 0xffu;
	buf1[3] = spi_addr & 0xffu;

	txb[0].buf = buf1;
	txb[0].len = 4u;
	txb[1].buf = NULL;
	txb[1].len = 8; /* number of tri-state clocks */

	rxb[0].buf = NULL;
	rxb[0].len = 4u;
	rxb[1].buf = NULL;
	rxb[1].len = 8; /* number of tri-state clocks */

	txbs.buffers = &txb[0];
	txbs.count = 2;
	rxbs.buffers = &rxb[0];
	rxbs.count = 2;

	spi_cfg.operation |= SPI_HOLD_ON_CS | SPI_LOCK_ON;
	ret = spi_transceive(spi_dev, (const struct spi_config *)&spi_cfg, &txbs, &rxbs);

	rxb[0].buf = buf2;
	rxb[0].len = 256u;

	rxbs.buffers = &rxb[0];
	rxbs.count = 1;

	spi_cfg.operation &= ~(SPI_HOLD_ON_CS | SPI_LINES_MASK);
	spi_cfg.operation |= SPI_LINES_DUAL;

	ret2 = spi_transceive(spi_dev, (const struct spi_config *)&spi_cfg, NULL, &rxbs);

	ret3 = spi_release(spi_dev, (const struct spi_config *)&spi_cfg);

	ret4 = memcmp(spi_flash_page0_data, buf2, 256u);
	LOG_INF("Dual read of 256 bytes returns: (%d) (%d) (%d) (%d)", ret, ret2, ret3, ret4);

#if SPI0_LINES == 4
	LOG_INF("Test Quad read. Will fail if SPI flash's QE bit is not set!");
	/* Quad read 256 bytes */
	spi_cfg.operation = SPI_WORD_SET(8) | SPI_LINES_SINGLE;

	memset(buf2, 0x55, sizeof(buf2));
	buf1[0] = 0x6bu; /* SPI Read 1-1-4-8 */
	buf1[1] = (spi_addr >> 16) & 0xffu;
	buf1[2] = (spi_addr >> 8) & 0xffu;
	buf1[3] = spi_addr & 0xffu;

	txb[0].buf = buf1;
	txb[0].len = 4u;
	txb[1].buf = NULL;
	txb[1].len = 8;

	rxb[0].buf = NULL;
	rxb[0].len = 4u;
	rxb[1].buf = NULL;
	rxb[1].len = 8u; /* number of tri-state clocks */

	txbs.buffers = &txb[0];
	txbs.count = 2;
	rxbs.buffers = &rxb[0];
	rxbs.count = 2;

	spi_cfg.operation |= SPI_HOLD_ON_CS | SPI_LOCK_ON;
	ret = spi_transceive(spi_dev, (const struct spi_config *)&spi_cfg, &txbs, &rxbs);

	rxb[0].buf = buf2;
	rxb[0].len = 256u;

	rxbs.buffers = &rxb[0];
	rxbs.count = 1;

	spi_cfg.operation &= ~(SPI_HOLD_ON_CS | SPI_LINES_MASK);
	spi_cfg.operation |= SPI_LINES_QUAD;

	ret2 = spi_transceive(spi_dev, (const struct spi_config *)&spi_cfg, NULL, &rxbs);
	ret3 = spi_release(spi_dev, (const struct spi_config *)&spi_cfg);

	ret4 = memcmp(spi_flash_page0_data, buf2, 256u);
	LOG_INF("Quad read of 256 bytes returns: (%d) (%d) (%d) (%d)",
		ret, ret2, ret3, ret4);
#endif /* #if SPI0_LINES == 4 */
#endif /* CONFIG_SPI_EXTENDED_MODES */

app_exit:
	LOG_INF("Application Done (%d)", ret);
	spin_on(256, 0);

	return 0;
}

static void spin_on(uint32_t id, int rval)
{
	spin_val = id;
	ret_val = rval;

	log_panic(); /* flush log buffers */

	while (spin_val) {
		;
	}
}

static int spi_flash_read_status(int status_reg, uint8_t *status)
{
	uint8_t txdata[4] = {0};
	uint8_t rxdata[4] = {0};
	int ret = 0;

	if (!status) {
		return -EINVAL;
	}

	if (status_reg == 1) {
		txdata[0] = 0x05u;
	} else if (status_reg == 2) {
		txdata[0] = 0x35u;
	} else if (status_reg == 3) {
		txdata[0] = 0x15u;
	} else {
		return -EINVAL;
	}

	txb[0].buf = txdata;
	txb[0].len = 2u;
	rxb[0].buf = rxdata;
	rxb[0].len = 2u;

	txbs.buffers = &txb[0];
	txbs.count = 1;
	rxbs.buffers = &rxb[0];
	rxbs.count = 1;

	ret = spi_transceive(spi_dev, &spi_cfg_fd_12m, &txbs, &rxbs);
	if (!ret) {
		*status = rxdata[1];
	}

	return ret;
}

static int spi_flash_write_status(int status_reg, uint8_t status)
{
	uint8_t txdata[4] = {0};
	uint8_t rxdata[4] = {0};
	int ret = 0;
	uint8_t opcode;

	if (!status) {
		return -EINVAL;
	}

	/* Send Write-Enable */
	txdata[0] = 0x06u;
	txb[0].buf = txdata;
	txb[0].len = 1u;
	rxb[0].buf = rxdata;
	rxb[0].len = 1u;

	txbs.buffers = &txb[0];
	txbs.count = 1;
	rxbs.buffers = &rxb[0];
	rxbs.count = 1;

	ret = spi_transceive(spi_dev, &spi_cfg_fd_12m, &txbs, &rxbs);
	if (ret) {
		LOG_ERR("SPI flash write status send write enable error %d", ret);
		return ret;
	}

	if (status_reg == 1) {
		opcode = 0x01u;
	} else if (status_reg == 2) {
		opcode = 0x31u;
	} else if (status_reg == 3) {
		opcode = 0x11u;
	} else {
		return -EINVAL;
	}

	txdata[0] = opcode;
	txdata[1] = status;
	txb[0].buf = txdata;
	txb[0].len = 2u;
	rxb[0].buf = rxdata;
	rxb[0].len = 2u;

	txbs.buffers = &txb[0];
	txbs.count = 1;
	rxbs.buffers = &rxb[0];
	rxbs.count = 1;

	ret = spi_transceive(spi_dev, &spi_cfg_fd_12m, &txbs, &rxbs);
	if (ret) {
		LOG_ERR("SPI flash write status send 0x%02x, 0x%02x: error %d",
			opcode, status, ret);
	}

	return ret;
}

int buffer_filled_with(const uint8_t *buffer, size_t buffersz, uint8_t val)
{
	if (!buffer || !buffersz) {
		return -EINVAL;
	}

	for (size_t n = 0; n < buffersz; n++) {
		if (buffer[n] != val) {
			return -EIO;
		}
	}

	return 0;
}

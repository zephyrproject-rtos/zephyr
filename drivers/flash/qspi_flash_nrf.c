/*
 * Copyright (c) 2018 Findlay Feng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <device.h>
#include <flash.h>
#include <init.h>
#include <kernel.h>
#include <nrfx_qspi.h>

#define LOG_DOMAIN "qspi_flash_nrfx_qspi"
#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(qspi_flash_nrfx_qspi);

struct qspi_flash_data {
	bool write_protection_sw;
};

#define is_word_aligned(__addr) ((POINTER_TO_UINT(__addr) & 0x3) == 0)

#if defined(CONFIG_QSPI_FLASH_NRF_READOC_FASTREAD)
#define CONFIG_QSPI_FLASH_NRF_READOC NRF_QSPI_READOC_FASTREAD
#elif defined(CONFIG_QSPI_FLASH_NRF_READOC_READ2O)
#define CONFIG_QSPI_FLASH_NRF_READOC NRF_QSPI_READOC_READ2O
#elif defined(CONFIG_QSPI_FLASH_NRF_READOC_READ2IO)
#define CONFIG_QSPI_FLASH_NRF_READOC NRF_QSPI_READOC_READ2IO
#elif defined(CONFIG_QSPI_FLASH_NRF_READOC_READ4O)
#define CONFIG_QSPI_FLASH_NRF_READOC NRF_QSPI_READOC_READ4O
#elif defined(CONFIG_QSPI_FLASH_NRF_READOC_READ4IO)
#define CONFIG_QSPI_FLASH_NRF_READOC NRF_QSPI_READOC_READ4IO
#else
#define CONFIG_QSPI_FLASH_NRF_READOC NRF_QSPI_READOC_READ4IO
#endif

#if defined(CONFIG_QSPI_FLASH_NRF_WRITEOC_PP)
#define CONFIG_QSPI_FLASH_NRF_WRITEOC NRF_QSPI_WRITEOC_PP
#elif defined(CONFIG_QSPI_FLASH_NRF_WRITEOC_PP2O)
#define CONFIG_QSPI_FLASH_NRF_WRITEOC NRF_QSPI_WRITEOC_PP2O
#elif defined(CONFIG_QSPI_FLASH_NRF_WRITEOC_PP4O)
#define CONFIG_QSPI_FLASH_NRF_WRITEOC NRF_QSPI_WRITEOC_PP4O
#elif defined(CONFIG_QSPI_FLASH_NRF_WRITEOC_PP4IO)
#define CONFIG_QSPI_FLASH_NRF_WRITEOC NRF_QSPI_WRITEOC_PP4IO
#else
#define CONFIG_QSPI_FLASH_NRF_WRITEOC NRF_QSPI_WRITEOC_PP4IO
#endif

#if defined(CONFIG_QSPI_FLASH_NRF_ADDRMODE_24BIT)
#define CONFIG_QSPI_FLASH_NRF_ADDRMODE NRF_QSPI_ADDRMODE_24BIT
#elif defined(CONFIG_QSPI_FLASH_NRF_ADDRMODE_32BIT)
#define CONFIG_QSPI_FLASH_NRF_ADDRMODE NRF_QSPI_ADDRMODE_32BIT
#else
#define CONFIG_QSPI_FLASH_NRF_ADDRMODE NRF_QSPI_ADDRMODE_24BIT
#endif

#if defined(CONFIG_QSPI_NRF_MODE_0)
#define CONFIG_QSPI_NRF_MODE NRF_QSPI_MODE_0
#elif defined(CONFIG_QSPI_NRF_MODE_1)
#define CONFIG_QSPI_NRF_MODE NRF_QSPI_MODE_2
#else
#define CONFIG_QSPI_NRF_MODE NRF_QSPI_MODE_0
#endif

#ifndef CONFIG_QSPI_FLASH_NRF_DPM
#define CONFIG_QSPI_FLASH_NRF_DPM 0
#endif

#if KHZ(32000) <= DT_QSPI_FLASH_QSPI_FREQ_0
#define CONFIG_QSPI_FLASH_NRF_FREQUENCY NRF_QSPI_FREQ_32MDIV1

#elif KHZ(16000) <= DT_QSPI_FLASH_QSPI_FREQ_0
#define CONFIG_QSPI_FLASH_NRF_FREQUENCY NRF_QSPI_FREQ_32MDIV2

#elif KHZ(10600) <= DT_QSPI_FLASH_QSPI_FREQ_0
#define CONFIG_QSPI_FLASH_NRF_FREQUENCY NRF_QSPI_FREQ_32MDIV3

#elif KHZ(8000) <= DT_QSPI_FLASH_QSPI_FREQ_0
#define CONFIG_QSPI_FLASH_NRF_FREQUENCY NRF_QSPI_FREQ_32MDIV4

#elif KHZ(6400) <= DT_QSPI_FLASH_QSPI_FREQ_0
#define CONFIG_QSPI_FLASH_NRF_FREQUENCY NRF_QSPI_FREQ_32MDIV5

#elif KHZ(5330) <= DT_QSPI_FLASH_QSPI_FREQ_0
#define CONFIG_QSPI_FLASH_NRF_FREQUENCY NRF_QSPI_FREQ_32MDIV6

#elif KHZ(4570) <= DT_QSPI_FLASH_QSPI_FREQ_0
#define CONFIG_QSPI_FLASH_NRF_FREQUENCY NRF_QSPI_FREQ_32MDIV7

#elif KHZ(4000) <= DT_QSPI_FLASH_QSPI_FREQ_0
#define CONFIG_QSPI_FLASH_NRF_FREQUENCY NRF_QSPI_FREQ_32MDIV8

#elif KHZ(3550) <= DT_QSPI_FLASH_QSPI_FREQ_0
#define CONFIG_QSPI_FLASH_NRF_FREQUENCY NRF_QSPI_FREQ_32MDIV9

#elif KHZ(3200) <= DT_QSPI_FLASH_QSPI_FREQ_0
#define CONFIG_QSPI_FLASH_NRF_FREQUENCY NRF_QSPI_FREQ_32MDIV10

#elif KHZ(2900) <= DT_QSPI_FLASH_QSPI_FREQ_0
#define CONFIG_QSPI_FLASH_NRF_FREQUENCY NRF_QSPI_FREQ_32MDIV11

#elif KHZ(2660) <= DT_QSPI_FLASH_QSPI_FREQ_0
#define CONFIG_QSPI_FLASH_NRF_FREQUENCY NRF_QSPI_FREQ_32MDIV12

#elif KHZ(2460) <= DT_QSPI_FLASH_QSPI_FREQ_0
#define CONFIG_QSPI_FLASH_NRF_FREQUENCY NRF_QSPI_FREQ_32MDIV13

#elif KHZ(2290) <= DT_QSPI_FLASH_QSPI_FREQ_0
#define CONFIG_QSPI_FLASH_NRF_FREQUENCY NRF_QSPI_FREQ_32MDIV14

#elif KHZ(2130) <= DT_QSPI_FLASH_QSPI_FREQ_0
#define CONFIG_QSPI_FLASH_NRF_FREQUENCY NRF_QSPI_FREQ_32MDIV15

#elif KHZ(2000) <= DT_QSPI_FLASH_QSPI_FREQ_0
#define CONFIG_QSPI_FLASH_NRF_FREQUENCY NRF_QSPI_FREQ_32MDIV16
#else
#error "Nordic Semiconductor nRF QSPI flash driver. Incompatible clock."
#endif

#if (DT_SERIAL_FLASH_ADDRESS < 0x12000000) ||		  \
	(DT_SERIAL_FLASH_ADDRESS + DT_SERIAL_FLASH_SIZE > \
	 0x20000000)
#error  "Nordic Semiconductor nRF QSPI flash driver. " \
	"Incompatible address or size."
#endif

static int read(off_t addr, void *data, size_t len)
{
	nrfx_err_t err;

	if (!is_word_aligned(addr) || !is_word_aligned(len)) {
		return -EINVAL;
	}

	if (is_word_aligned(data)) {
		err = nrfx_qspi_read(data, len, addr);
		if (err == NRFX_ERROR_BUSY) {
			return -EBUSY;
		} else if (err != NRFX_SUCCESS) {
			return -EINVAL;
		}
	} else {
		u32_t buf[64];
		size_t read_bytes;

		read_bytes = min(sizeof(buf) - addr % sizeof(buf), len);

		while (read_bytes) {
			err = nrfx_qspi_read(buf, read_bytes, addr);
			if (err == NRFX_ERROR_BUSY) {
				return -EBUSY;
			} else if (err != NRFX_SUCCESS) {
				return -EINVAL;
			}

			memcpy(data, buf, read_bytes);
			addr += read_bytes;
			data = ((u8_t *)data) + read_bytes;
			len -= read_bytes;

			read_bytes = min(sizeof(buf), len);
		}
	}

	err = nrfx_qspi_read(data, len, addr);
	if (err == NRFX_ERROR_BUSY) {
		return -EBUSY;
	} else if (err != NRFX_SUCCESS) {
		return -EINVAL;
	}

	return 0;
}

static int write(off_t addr, const void *data, size_t len)
{
	nrfx_err_t err;

	if (!is_word_aligned(addr) || !is_word_aligned(len)) {
		return -EINVAL;
	}

	if (nrfx_is_in_ram(data) && is_word_aligned(data)) {
		err = nrfx_qspi_write(data, len, addr);
		if (err == NRFX_ERROR_BUSY) {
			return -EBUSY;
		} else if (err != NRFX_SUCCESS) {
			return -EINVAL;
		}
	} else {
		u32_t buf[64];
		size_t write_bytes;

		write_bytes = min(sizeof(buf) - addr % sizeof(buf), len);

		while (write_bytes) {
			memcpy(buf, data, write_bytes);

			err = nrfx_qspi_write(buf, write_bytes, addr);
			if (err == NRFX_ERROR_BUSY) {
				return -EBUSY;
			} else if (err != NRFX_SUCCESS) {
				return -EINVAL;
			}

			addr += write_bytes;
			data = ((u8_t *)data) + write_bytes;
			len -= write_bytes;

			write_bytes = min(sizeof(buf), len);
		}
	}

	return 0;
}

static int erase(off_t addr, size_t len)
{
	nrfx_err_t err = nrfx_qspi_erase(len, addr);

	if (err == NRFX_ERROR_BUSY) {
		return -EBUSY;
	} else if (err != NRFX_SUCCESS) {
		return -EINVAL;
	}

	return 0;
}

static inline bool is_addr_valid(off_t addr, size_t len)
{
	if (addr + len > DT_SERIAL_FLASH_SIZE || addr < 0) {
		return false;
	}

	return true;
}

static int qspi_flash_nrf_read(struct device *dev, off_t addr, void *data,
			       size_t len)
{
	if (!is_addr_valid(addr, len)) {
		return -EINVAL;
	}

	if (!len) {
		return 0;
	}

	return read(addr, data, len);
}

static int qspi_flash_nrf_write(struct device *dev, off_t addr,
				const void *data, size_t len)
{
	struct qspi_flash_data *dev_data = dev->driver_data;

	if (!is_addr_valid(addr, len)) {
		return -EINVAL;
	}

	if (dev_data->write_protection_sw || !len) {
		return 0;
	}

	return write(addr, data, len);
}

static int qspi_flash_nrf_erase(struct device *dev, off_t addr, size_t size)
{
	struct qspi_flash_data *dev_data = dev->driver_data;

	if (!is_addr_valid(addr, size)) {
		return -EINVAL;
	}

	if (dev_data->write_protection_sw || !size) {
		return 0;
	}

	return erase(addr, size);
}

static int qspi_flash_nrf_write_protection(struct device *dev, bool enable)
{
	struct qspi_flash_data *data = dev->driver_data;

	data->write_protection_sw = enable;

	return 0;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static struct flash_pages_layout dev_layout;

static void
qspi_flash_nrf_pages_layout(struct device *dev,
			    const struct flash_pages_layout **layout,
			    size_t *layout_size)
{
	*layout = &dev_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static const struct flash_driver_api qspi_flash_nrf_api = {
	.read = qspi_flash_nrf_read,
	.write = qspi_flash_nrf_write,
	.erase = qspi_flash_nrf_erase,
	.write_protection = qspi_flash_nrf_write_protection,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = qspi_flash_nrf_pages_layout,
#endif
	.write_block_size = 4,
};

static int nrf_qspi_flash_init(struct device *dev)
{
	struct qspi_flash_data *data = dev->driver_data;
	static const nrfx_qspi_config_t config = {
		.xip_offset = DT_SERIAL_FLASH_ADDRESS - 0x12000000,
		.pins = {
			.sck_pin = DT_QSPI_FLASH_SCK_PIN,
			.csn_pin = DT_QSPI_FLASH_CSN_PIN,
			.io0_pin = DT_QSPI_FLASH_IO0_PIN,
			.io1_pin = DT_QSPI_FLASH_IO1_PIN,
			.io2_pin = DT_QSPI_FLASH_IO2_PIN,
			.io3_pin = DT_QSPI_FLASH_IO3_PIN,
		},
		.irq_priority = (uint8_t)DT_QSPI_FLASH_IRQ_PRI,
		.prot_if = {
			.readoc = CONFIG_QSPI_FLASH_NRF_READOC,
			.writeoc = CONFIG_QSPI_FLASH_NRF_WRITEOC,
			.addrmode = CONFIG_QSPI_FLASH_NRF_ADDRMODE,
			.dpmconfig = CONFIG_QSPI_FLASH_NRF_DPM,
		},
		.phy_if = {
			.sck_freq = CONFIG_QSPI_FLASH_NRF_FREQUENCY,
			.sck_delay = (uint8_t)CONFIG_QSPI_FLASH_NRF_DELAY,
			.spi_mode = CONFIG_QSPI_NRF_MODE,
			.dpmen = CONFIG_QSPI_FLASH_NRF_DPM,
		},
	};
	nrfx_err_t result;

	dev->driver_api = &qspi_flash_nrf_api;
	data->write_protection_sw = true;

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	dev_layout.pages_count = DT_SERIAL_FLASH_SIZE / KB(4);
	dev_layout.pages_size = KB(4);
#endif

	result = nrfx_qspi_init(&config, NULL, NULL);
	if (result != NRFX_SUCCESS) {
		LOG_ERR("Failed to initialize device: %s", dev->config->name);
		return -EBUSY;
	}
#if defined(CONFIG_QSPI_FLASH_NRF_DEV_MX25R6435F)
	u8_t status_reg = 0x40;
	nrf_qspi_cinstr_conf_t cinstr = NRFX_QSPI_DEFAULT_CINSTR(0x06, 1);

	cinstr.io2_level = true;
	cinstr.io3_level = true;

	result = nrfx_qspi_cinstr_xfer(&cinstr, NULL, NULL);
	if (result != NRFX_SUCCESS) {
		LOG_ERR("Failed to initialize device: %s", dev->config->name);
		return -EBUSY;
	}

	cinstr.opcode = 0x01;
	cinstr.length = 2;

	result = nrfx_qspi_cinstr_xfer(&cinstr, &status_reg, NULL);
	if (result != NRFX_SUCCESS) {
		LOG_ERR("Failed to initialize device: %s", dev->config->name);
		return -EBUSY;
	}
#endif

	return 0;
}

struct qspi_flash_data qspi_flash_data_m;

DEVICE_INIT(nrf_qspi_flash, DT_SERIAL_FLASH_DEV_NAME, nrf_qspi_flash_init,
	    &qspi_flash_data_m, NULL, POST_KERNEL,
	    CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

/*
 * Copyright (c) 2017 Crypta Labs Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Generic SPI Flash driver for chips with Discoverable Parameters */

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SPI_LEVEL
#include <logging/sys_log.h>

#include <errno.h>
#include <flash.h>
#include <spi.h>
#include <misc/util.h>

#define SFDP_HEADER_SIGNATURE 0x50444653

struct sfdp_header {
	u32_t	signature;
	u8_t	minor_ver;
	u8_t	major_ver;
	u8_t	nph;
	u8_t	unused;
};

struct sfdp_parameter_header {
	u8_t	id_lsb;
	u8_t	minor_ver;
	u8_t	major_ver;
	u8_t	length;
	u8_t	addr[3];
	u8_t	id_msb;
};

static inline size_t get_phdr_addr(struct sfdp_parameter_header *phdr)
{
	return phdr->addr[0] + (phdr->addr[1] << 8) +
		(phdr->addr[2] << 16);
}

static inline u16_t get_phdr_id(struct sfdp_parameter_header *phdr)
{
	return (phdr->id_msb << 8) + phdr->id_lsb;
}

struct sfdp_basic_flash_parameters {
	u32_t block_erase_sizes : 2;
	u32_t write_granularity : 1;
	u32_t volatile_status_register : 1;
	u32_t write_enable_opcode_select : 1;
	u32_t unused1 : 3;
	u32_t opcode_erase_4k : 8;

	u32_t support_1_1_2_fast_read : 1;
	u32_t addr_bytes : 2;
	u32_t support_dtr : 1;
	u32_t support_1_2_2_fast_read : 1;
	u32_t support_1_4_4_fast_read : 1;
	u32_t support_1_1_4_fast_read : 1;
	u32_t unused2 : 9;

	u32_t density;

	struct {
		u8_t dummy_clocks : 4;
		u8_t mode_clocks : 4;
		u8_t opcode;
	} read_opcodes[4]; /* 1-4-4, 1-1-4, 1-1-2, 1-2-2 */

	u32_t support_2_2_2_fast_read : 1;
	u32_t unused3 : 3;
	u32_t support_4_4_4_fast_read : 1;
	u32_t unused4 : 27;

	struct {
		u8_t dummy_clocks : 4;
		u8_t mode_clocks : 4;
		u8_t opcode;
	} read_opcodes_2[4]; /* reserved, 2-2-2, reserved, 4-4-4 */

	struct {
		u8_t size;
		u8_t erase_opcode;
	} sector_sizes[4];

	struct {
		u8_t mult : 4, time : 4;
	} sectore_erase_times[4];

	u8_t page_mult : 4;
	u8_t page_size_shift : 4;
	u8_t page_program_time : 4;
	u8_t first_byte_program_time : 4;
	u8_t additional_byte_program_time : 4;
	u8_t chip_erase_time : 7;
	u8_t unused5 : 1;

	u32_t suspend_info;

	u8_t program_resume_opcode;
	u8_t program_suspend_opcode;
	u8_t resume_opcode;
	u8_t suspend_opcode;

	u32_t power_down_info;
};

#define SFDP_BFPT_ID		0xff00  /* Basic Flash Parameter Table */
#define SFDP_SECTOR_MAP_ID	0xff81  /* Sector Map Table */

struct spi_flash_data {
	struct spi_config config;
	struct k_sem sem;

	u32_t flash_size, page_size;
	u8_t addr_len;
	u8_t read_opcode, read_dummy;
	u8_t write_opcode, write_dummy;
	u8_t global_unprotect_opcode;

	union {
		u32_t bfpt[16];
		struct sfdp_basic_flash_parameters bfp;
	};
	u32_t sector_map[16];
};

#define CMD_IDENTIFY		0x9F
#define CMD_DISCOVER_PARAMETERS	0x5A
#define CMD_READ_STATUS		0x05
#define CMD_READ_MEMORY		0x03
#define CMD_READ_CONFIG		0x35
#define CMD_WRITE_STATUS_CONFIG	0x01
#define CMD_PROGRAM_PAGE	0x02
#define CMD_WRITE_ENABLE	0x06
#define CMD_WRITE_DISABLE	0x04

#define STATUS_BUSY		0x01

#define MAX_SFDP_HEADERS	4

static inline int spi_flash_reg_read(struct device *dev, u8_t cmd_id)
{
	struct spi_flash_data *const data = dev->driver_data;
	u8_t buf[2];
	struct spi_buf cmd = {
		.buf = buf,
		.len = sizeof(buf),
	};

	buf[0] = cmd_id;

	if (spi_transceive(&data->config, &cmd, 1, &cmd, 1) != 0) {
		return -EIO;
	}

	return buf[1];
}

static inline void wait_for_flash_idle(struct device *dev)
{
	while (spi_flash_reg_read(dev, CMD_READ_STATUS) & STATUS_BUSY)
		;
}

static inline int spi_flash_write_sr_cr(struct device *dev, u8_t sr, u8_t cr)
{
	struct spi_flash_data *const data = dev->driver_data;
	u8_t buf[3];
	struct spi_buf cmd = {
		.buf = buf,
		.len = sizeof(buf),
	};

	buf[0] = CMD_WRITE_STATUS_CONFIG;
	buf[1] = sr;
	buf[2] = cr;

	if (spi_transceive(&data->config, &cmd, 1, &cmd, 1) != 0) {
		return -EIO;
	}

	wait_for_flash_idle(dev);
	return 0;
}

static inline int spi_flash_id(struct device *dev)
{
	struct spi_flash_data *const data = dev->driver_data;
	u8_t buf[4] = { CMD_IDENTIFY };
	struct spi_buf bufs = {
		.buf = &buf,
		.len = sizeof(buf)
	};

	if (spi_transceive(&data->config, &bufs, 1, &bufs, 1) != 0) {
		return -EIO;
	}

	switch ((buf[1] << 16) + (buf[2] << 8) + buf[3]) {
	case 0xBF2641: /* SST26VF016B */
		data->global_unprotect_opcode = 0x98;
		break;
	case 0x000000:
		return -EIO;
	}

	SYS_LOG_INF("ID %02x %02x %02x", buf[1], buf[2], buf[3]);
	return 0;
}

static int spi_flash_read_sfdp(struct device *dev, uint32_t addr,
			       void *ptr, size_t len)
{
	struct spi_flash_data *const data = dev->driver_data;
	u8_t hdr[5] = {
		CMD_DISCOVER_PARAMETERS, addr >> 16, addr >> 8, addr, 0
	};
	struct spi_buf txbufs[2] = {
		{ .buf = &hdr, sizeof(hdr) },
		{ .buf = NULL, len }
	};
	struct spi_buf rxbufs[2] = {
		{ .buf = NULL, sizeof(hdr) },
		{ .buf = ptr, len },
	};

	if (spi_transceive(&data->config, txbufs, 2, rxbufs, 2) != 0) {
		return -EIO;
	}

	return 0;
}

static int spi_flash_sfdp(struct device *dev)
{
	struct spi_flash_data *data = dev->driver_data;
	struct sfdp_header sfdp;
	struct sfdp_parameter_header phdr[4];
	int r, i, nph;

	r = spi_flash_read_sfdp(dev, 0, &sfdp, sizeof(sfdp));
	if (r != 0) {
		return r;
	}

	if (sfdp.signature != SFDP_HEADER_SIGNATURE) {
		return -ENODEV;
	}

	nph = min(ARRAY_SIZE(phdr), sfdp.nph + 1UL);
	r = spi_flash_read_sfdp(
		dev, sizeof(sfdp),
		phdr, sizeof(struct sfdp_parameter_header[nph]));
	if (r != 0) {
		return r;
	}

	for (i = 0; i < nph; i++) {
		size_t len;
		void *ptr;

		SYS_LOG_DBG("[%d] id %04x v%d.%d addr %06x len %d",
				i, get_phdr_id(&phdr[i]),
				phdr[i].major_ver, phdr[i].minor_ver,
				get_phdr_addr(&phdr[i]),
				phdr[i].length);
		switch (get_phdr_id(&phdr[i])) {
		case SFDP_BFPT_ID:
			ptr = data->bfpt;
			len = min(ARRAY_SIZE(data->bfpt), phdr[i].length);
			break;
		default:
			continue;
		}

		spi_flash_read_sfdp(dev, get_phdr_addr(&phdr[i]),
				    ptr, len * 4);
	}

	if (!data->bfp.density) {
		return -ENODEV;
	}

	data->flash_size = data->bfp.density >> 3;
	data->page_size = 1U << data->bfp.page_size_shift;

	SYS_LOG_INF("SFDP v%d.%d %d KiB (%d page size)",
		sfdp.major_ver, sfdp.minor_ver,
		(data->flash_size + 1) >> 10,
		data->page_size);

	switch (data->bfp.addr_bytes) {
	case 0: /* 3-bytes */
	case 1: /* 3-bytes default (4-bytes supported) */
		data->addr_len = 3;
		break;
	case 2: /* 4-bytes */
		data->addr_len = 4;
		break;
	default:
		return -ENODEV;
	}

	data->read_opcode = CMD_READ_MEMORY;
	data->read_dummy = 0;

	data->write_opcode = CMD_PROGRAM_PAGE;
	data->write_dummy = 0;

	return 0;
}

static int spi_flash_cmd(struct device *dev, u8_t cmd_id)
{
	struct spi_flash_data *const data = dev->driver_data;
	u8_t buf[1];
	struct spi_buf cmd = {
		.buf = buf,
		.len = sizeof(buf),
	};

	buf[0] = cmd_id;

	if (spi_transceive(&data->config, &cmd, 1, &cmd, 1) != 0) {
		return -EIO;
	}

	return 0;
}

static int spi_flash_read(struct device *dev, off_t offset, void *ptr,
			  size_t len)
{
	struct spi_flash_data *const data = dev->driver_data;
	u8_t hdr[5];
	struct spi_buf txbufs[2] = {
		{ .buf = hdr,  .len = 1 + data->addr_len },
		{ .buf = NULL, .len = data->read_dummy + len },
	};
	struct spi_buf rxbufs[2] = {
		{ .buf = NULL, .len = 1 + data->addr_len + data->read_dummy },
		{ .buf = ptr,  .len = len },
	};
	int r, i;

	if (offset < 0 || offset+len >= data->flash_size) {
		return -ENODEV;
	}

	k_sem_take(&data->sem, K_FOREVER);
	wait_for_flash_idle(dev);
	i = 0;
	hdr[i++] = data->read_opcode;
	if (data->addr_len == 4) {
		hdr[i++] = offset >> 24;
	}
	hdr[i++] = offset >> 16;
	hdr[i++] = offset >> 8;
	hdr[i++] = offset;
	r = spi_transceive(&data->config, txbufs, ARRAY_SIZE(txbufs),
			   rxbufs, ARRAY_SIZE(rxbufs));
	k_sem_give(&data->sem);

	return r;
}

static int spi_flash_write(struct device *dev, off_t offset,
			      const void *ptr, size_t len)
{
	struct spi_flash_data *const data = dev->driver_data;
	u8_t hdr[5];
	struct spi_buf txbufs[] = {
		{ .buf = hdr, .len = 1 + data->addr_len },
		{ .buf = NULL, .len = data->write_dummy },
		{ .buf = (void *)ptr, .len = len },
	};
	int r, i;

	SYS_LOG_DBG("write: @%x len %x, opcode %x (page size %d)",
		offset, len,
		data->write_opcode, data->page_size);

	if (!data->write_opcode) {
		return -ENOTSUP;
	}

	if (offset < 0 || offset+len >= data->flash_size
	    || len > data->page_size) {
		return -ENODEV;
	}

	k_sem_take(&data->sem, K_FOREVER);

	wait_for_flash_idle(dev);

	r = spi_flash_cmd(dev, CMD_WRITE_ENABLE);
	if (r != 0) {
		goto err;
	}

	i = 0;
	hdr[i++] = data->write_opcode;
	if (data->addr_len == 4) {
		hdr[i++] = offset >> 24;
	}
	hdr[i++] = offset >> 16;
	hdr[i++] = offset >> 8;
	hdr[i++] = offset;

	r = spi_transceive(&data->config, txbufs, ARRAY_SIZE(txbufs), NULL, 0);
	if (r == 0) {
		wait_for_flash_idle(dev);
	}

	k_sem_give(&data->sem);

err:
	return r;
}

static int spi_flash_write_protection_set(struct device *dev, bool enable)
{
	struct spi_flash_data *const data = dev->driver_data;
	int r;

	k_sem_take(&data->sem, K_FOREVER);
	wait_for_flash_idle(dev);

	if (enable) {
		r = spi_flash_cmd(dev, CMD_WRITE_DISABLE);
	} else {
		SYS_LOG_DBG("global unblock %02x",
			    data->global_unprotect_opcode);

		r = spi_flash_cmd(dev, CMD_WRITE_ENABLE);
		if (r == 0 && data->global_unprotect_opcode) {
			wait_for_flash_idle(dev);
			r = spi_flash_cmd(dev, data->global_unprotect_opcode);
		}
	}
	k_sem_give(&data->sem);

	return r;
}

static inline int spi_flash_erase_cmd(struct device *dev,
				      u8_t opcode, off_t offset)
{
	struct spi_flash_data *const data = dev->driver_data;
	u8_t hdr[5];
	struct spi_buf txbufs[] = {
		{ .buf = hdr,  .len = 1 + data->addr_len },
	};
	int r, i;

	SYS_LOG_DBG("erase: %02x %06x", opcode, offset);

	wait_for_flash_idle(dev);
	r = spi_flash_cmd(dev, CMD_WRITE_ENABLE);
	if (r != 0) {
		return r;
	}

	i = 0;
	hdr[i++] = opcode;
	if (data->addr_len == 4) {
		hdr[i++] = offset >> 24;
	}
	hdr[i++] = offset >> 16;
	hdr[i++] = offset >> 8;
	hdr[i++] = offset;

	wait_for_flash_idle(dev);
	return spi_transceive(&data->config, txbufs, ARRAY_SIZE(txbufs), 0, 0);
}

static int spi_flash_erase(struct device *dev, off_t offset, size_t size)
{
	struct spi_flash_data *const data = dev->driver_data;
	off_t offs = offset;
	size_t left = size;
	int ret = 0;

	if (offset < 0 || offset+size >= data->flash_size ||
	    (offset & 0xfff) || (size & 0xfff)) {
		return -ENODEV;
	}

	if (!data->bfp.opcode_erase_4k) {
		return -ENOTSUP;
	}

	k_sem_take(&data->sem, K_FOREVER);

	/* FIXME: Support chip erase, and large block erase */

	/* Erase in 4K blocks */
	while (left && ret == 0) {
		ret = spi_flash_erase_cmd(dev, data->bfp.opcode_erase_4k, offs);
		offs += 0x1000;
		left -= 0x1000;
	}

	k_sem_give(&data->sem);
	return ret;
}

static const struct flash_driver_api spi_flash_api = {
	.read = spi_flash_read,
	.write = spi_flash_write,
	.erase = spi_flash_erase,
	.write_protection = spi_flash_write_protection_set,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = (flash_api_pages_layout)
		       flash_page_layout_not_implemented,
#endif
	.write_block_size = 1,
};

static int spi_flash_init(struct device *dev)
{
	struct device *spi_dev;
	struct spi_flash_data *data = dev->driver_data;
	int r;

	spi_dev = device_get_binding(CONFIG_SPI_FLASH_SFDP_SPI_NAME);
	if (!spi_dev) {
		return -EIO;
	}

	data->config = (struct spi_config) {
		.dev = spi_dev,
		.frequency = CONFIG_SPI_FLASH_SFDP_SPI_FREQ_0,
		.operation = SPI_WORD_SET(8),
	};

	r = spi_flash_id(dev);
	if (r != 0) {
		return r;
	}

	r = spi_flash_sfdp(dev);
	if (r != 0) {
		return r;
	}

	k_sem_init(&data->sem, 1, UINT_MAX);
	dev->driver_api = &spi_flash_api;
	return 0;
}

static struct spi_flash_data spi_flash_memory_data;

DEVICE_INIT(spi_flash_memory, "flash0", spi_flash_init,
	    &spi_flash_memory_data, NULL, POST_KERNEL,
	    CONFIG_SPI_FLASH_SFDP_INIT_PRIORITY);

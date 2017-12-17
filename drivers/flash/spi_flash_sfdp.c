/*
 * Copyright (c) 2017 Crypta Labs Ltd
 * Copyright (c) 2018 Findlay Feng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Generic SPI Flash driver for chips with Discoverable Parameters */
/* https://www.jedec.org/system/files/docs/JESD216B.pdf */

#define SYS_LOG_DOMAIN "SPI Flash"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SPI_FLASH_LEVEL
#include <logging/sys_log.h>

#include "spi_flash_sfdp.h"

#include <errno.h>
#include <string.h>

#define SFDP_HEADER_SIGNATURE 0x50444653
#define SFDP_SPI_OPERATION_SINGLE (SPI_WORD_SET(8) | SPI_LINES_SINGLE)
#define SFDP_SPI_OPERATION_DUAL (SPI_WORD_SET(8) | SPI_LINES_DUAL)
#define SFDP_SPI_OPERATION_QUAD (SPI_WORD_SET(8) | SPI_LINES_QUAD)

struct sfdp_spi_buf_set {
	struct spi_buf buffers[4];
	size_t count;
	u8_t buf[20];
	off_t offset;
};

static inline void sfdp_spi_buf_append(struct sfdp_spi_buf_set *buf_set,
				       void *data, size_t len)
{
	buf_set->buffers[buf_set->count].buf = data;
	buf_set->buffers[buf_set->count].len = len;
	buf_set->count++;
}

static inline void sfdp_spi_buf_copy(struct sfdp_spi_buf_set *buf_set,
				     const void *data, size_t len)
{
	void *buf = buf_set->buf + buf_set->offset;

	memcpy(buf, data, len);
	sfdp_spi_buf_append(buf_set, buf, len);
	buf_set->offset += len;
}

static inline size_t sfdp_spi_buf_adj(u8_t *in, u8_t *out, int inlines,
				      int outlines, size_t len)
{
	u8_t mask = (1 << inlines) - 1;
	int inoff, outoff;

	if (inlines == outlines) {
		memcpy(out, in, len);
		return len;
	}

	while (len--) {
		inoff = 8 - inlines;
		while (inoff) {
			outoff = 0;
			while (outoff < 8) {
				*out <<= outlines;
				*out |= (*in >> inoff) & mask;
				inoff -= inlines;
				outoff += outlines;
			}
			out++;
		}
		in++;
	}
	return len * (outlines / inlines);
}

static void sfdp_spi_buf_append_cmd(struct device *dev,
				    struct sfdp_spi_buf_set *buf_set, u8_t cmd,
				    bool read)
{
	struct spi_flash_data *const data = dev->driver_data;
	u8_t buf[4];

	if (read) {
		sfdp_spi_buf_copy(buf_set, buf,
				  sfdp_spi_buf_adj(&cmd, buf,
						   data->instruction_lines,
						   data->data_lines, 1));
	} else {
		if (data->quad_enable) {
			sfdp_spi_buf_copy(buf_set, &cmd, 1);
		} else {
			sfdp_spi_buf_copy(buf_set, buf,
					  sfdp_spi_buf_adj(&cmd, buf, 1,
							   data->data_lines,
							   1));
		}
	}
}

static void sfdp_spi_buf_append_addr(struct device *dev,
				     struct sfdp_spi_buf_set *buf_set,
				     u32_t addr, bool four_addr, bool read)
{
	struct spi_flash_data *const data = dev->driver_data;
	u8_t addr_buf[4];
	u8_t buf[16];
	int i = 0;

	if (four_addr) {
		addr_buf[i++] = addr >> 24;
	}
	addr_buf[i++] = addr >> 16;
	addr_buf[i++] = addr >> 8;
	addr_buf[i++] = addr;

	if (read) {
		sfdp_spi_buf_copy(buf_set, buf,
				  sfdp_spi_buf_adj(addr_buf, buf,
						   data->address_lines,
						   data->data_lines, i));
	} else {
		if (data->quad_enable) {
			sfdp_spi_buf_copy(buf_set, addr_buf, i);
		} else {
			sfdp_spi_buf_copy(buf_set, buf,
					  sfdp_spi_buf_adj(addr_buf, buf, 1,
							   data->data_lines,
							   i));
		}
	}
}

static int sfdp_spi_transceive(struct device *dev,
			       struct sfdp_spi_buf_set *buf_set)
{
	struct spi_flash_data *const data = dev->driver_data;
	struct spi_buf_set buf = {.buffers = buf_set->buffers,
				  .count = buf_set->count};
	struct spi_config *config = data->tem_config;

	if (!config) {
		config = &data->config;
	}

	if (spi_transceive(data->spi, config, &buf, &buf) != 0) {
		return -EIO;
	}

	return 0;
}

static int sfdp_spi_write(struct device *dev, struct sfdp_spi_buf_set *buf_set)
{
	struct spi_flash_data *const data = dev->driver_data;
	struct spi_buf_set buf = {.buffers = buf_set->buffers,
				  .count = buf_set->count};
	struct spi_config *config = data->tem_config;

	if (!config) {
		config = &data->config;
	}

	if (spi_write(data->spi, config, &buf) != 0) {
		return -EIO;
	}

	return 0;
}

static inline u16_t get_phdr_id(union sfdp_parameter_header *phdr)
{
	return (phdr->id_msb << 8) + phdr->id_lsb;
}

static inline int spi_flash_reg_read(struct device *dev, u8_t cmd_id, u8_t *out)
{
	struct sfdp_spi_buf_set buf_set = {0};

	sfdp_spi_buf_append_cmd(dev, &buf_set, cmd_id, false);
	sfdp_spi_buf_append(&buf_set, out, 1);

	return sfdp_spi_transceive(dev, &buf_set);
}

static inline void wait_for_flash_idle(struct device *dev)
{
	struct spi_flash_data *const data = dev->driver_data;
	u8_t reg;

	if (data->opcodes.read_status == SFDP_RESERVED_VALUE) {
		return;
	}
	while (!spi_flash_reg_read(dev, data->opcodes.read_status, &reg)) {
		if (((reg >> data->status_busy_bit) & 0x1) ^
		    data->status_busy) {
			return;
		}
		k_sleep(1);
	}
}

int spi_flash_cmd(struct device *dev, u8_t cmd_id)
{
	struct sfdp_spi_buf_set buf_set = {0};

	sfdp_spi_buf_append_cmd(dev, &buf_set, cmd_id, false);
	return sfdp_spi_write(dev, &buf_set);
}

static inline int spi_flash_write_enable(struct device *dev)
{
	struct spi_flash_data *const data = dev->driver_data;

	if (data->opcodes.write_enable == SFDP_RESERVED_VALUE) {
		return 0;
	}
	return spi_flash_cmd(dev, data->opcodes.write_enable);
}

int spi_flash_read_sfdp(struct device *dev, uint32_t addr, void *ptr,
			size_t len)
{
	struct sfdp_spi_buf_set buf_set = {0};
	u8_t cmd = CMD_READ_SFDP;
	u8_t addr_buf[] = {addr >> 16, addr >> 8, addr};

	sfdp_spi_buf_append(&buf_set, &cmd, 1);
	sfdp_spi_buf_append(&buf_set, addr_buf, 3);
	sfdp_spi_buf_append(&buf_set, NULL, 1);
	sfdp_spi_buf_append(&buf_set, ptr, len);
	return sfdp_spi_transceive(dev, &buf_set);
}

static int spi_flash_sfdp_bfp(struct device *dev, uint32_t addr, u8_t len)
{
	struct spi_flash_data *data = dev->driver_data;
	union sfdp_basic_flash_parameters bfp;
	int r;

	if (len != ARRAY_SIZE(bfp.dwords)) {
		SYS_LOG_ERR("Wrong basic flash parameters size");
		return -ENODEV;
	}

	r = spi_flash_read_sfdp(dev, addr, &bfp, len * 4);
	if (r != 0) {
		return r;
	}

	/* Set Erase Types */
	memcpy(data->erase_types, bfp.erase_types, sizeof(data->erase_types));
	if (bfp.block_erase_sizes == 1) {
		data->opcodes.block_4k_erase = bfp.opcode_erase_4k;
	}

	/* Set Memory Capacity */
	if (!bfp.density) {
		SYS_LOG_ERR("Wrong Memory Capacity");
		return -ENODEV;
	}

	if (bfp.density & 0x80000000) {
		dword_t n = bfp.density & 0x7fffffff;

		if (n < 32 || n > 35) {
			SYS_LOG_ERR("Wrong Memory Capacity");
			return -ENODEV;
		}
		data->flash_size = 1UL << (n - 3);
	} else {
		if ((bfp.density + 1) & bfp.density) {
			SYS_LOG_ERR("Wrong Memory Capacity");
			return -ENODEV;
		}
		data->flash_size = (bfp.density + 1) >> 3;
	}

	/* Set Program */
	if (bfp.write_granularity) {
		data->page_size = 1U << bfp.page_size;
	} else {
		SYS_LOG_ERR("Go to do about bytes program");
		return -ENODEV;
	}

	/* Set read mode */
	switch (data->lines) {
	case 4:
		if (bfp.support_4_4_4_fast_read) {
			data->opcodes.read = bfp.fast_read_4_4_4_opcode;
			data->dummy_clocks = bfp.fast_read_4_4_4_dummy_clocks;
			data->mode_clocks = bfp.fast_read_4_4_4_mode_clocks;
			data->instruction_lines = 4;
			data->address_lines = 4;
			data->data_lines = 4;
			break;
		} else if (bfp.support_1_4_4_fast_read) {
			data->opcodes.read = bfp.fast_read_1_4_4_opcode;
			data->dummy_clocks = bfp.fast_read_1_4_4_dummy_clocks;
			data->mode_clocks = bfp.fast_read_1_4_4_mode_clocks;
			data->instruction_lines = 1;
			data->address_lines = 4;
			data->data_lines = 4;
			break;
		} else if (bfp.support_1_1_4_fast_read) {
			data->opcodes.read = bfp.fast_read_1_1_4_opcode;
			data->dummy_clocks = bfp.fast_read_1_1_4_dummy_clocks;
			data->mode_clocks = bfp.fast_read_1_1_4_mode_clocks;
			data->instruction_lines = 1;
			data->address_lines = 1;
			data->data_lines = 4;
			break;
		}
	case 2:
		if (bfp.support_2_2_2_fast_read) {
			data->opcodes.read = bfp.fast_read_2_2_2_opcode;
			data->dummy_clocks = bfp.fast_read_2_2_2_dummy_clocks;
			data->mode_clocks = bfp.fast_read_2_2_2_mode_clocks;
			data->instruction_lines = 2;
			data->address_lines = 2;
			data->data_lines = 2;
			break;
		} else if (bfp.support_1_2_2_fast_read) {
			data->opcodes.read = bfp.fast_read_1_2_2_opcode;
			data->dummy_clocks = bfp.fast_read_1_2_2_dummy_clocks;
			data->mode_clocks = bfp.fast_read_1_2_2_mode_clocks;
			data->instruction_lines = 1;
			data->address_lines = 2;
			data->data_lines = 2;
			break;
		} else if (bfp.support_1_1_2_fast_read) {
			data->opcodes.read = bfp.fast_read_1_1_2_opcode;
			data->dummy_clocks = bfp.fast_read_1_1_2_dummy_clocks;
			data->mode_clocks = bfp.fast_read_1_1_2_mode_clocks;
			data->instruction_lines = 1;
			data->address_lines = 1;
			data->data_lines = 2;
			break;
		}
	case 1:
		data->instruction_lines = 1;
		data->address_lines = 1;
		data->data_lines = 1;
		break;
	}

	if (data->data_lines == 4) {
		SYS_LOG_ERR("Go to do about Quad Enable");
		return -ENODEV;
	}

	/* Set Four Bytes Addr */
	switch (bfp.addr_bytes) {
	case 0: /* 3-bytes */
		data->four_addr = false;
		break;
	case 1: /* 3-bytes default (4-bytes supported) */
		SYS_LOG_ERR("Go to do about Change Bytes Addr");
		return -ENODEV;
	case 2: /* 4-bytes */
		data->four_addr = true;
		break;
	default:
		SYS_LOG_ERR("Wrong Addr Bytes");
		return -ENODEV;
	}

	/* Set Write Enable */
	if (bfp.volatile_status_register) {
		if (bfp.write_enable_opcode_select) {
			data->opcodes.write_enable = 0x06;
		} else {
			data->opcodes.write_enable = 0x50;
		}
	} else {
		/* todo */
		return -ENODEV;
	}

	/* Set Polling Device Busy */
	if (bfp.polling_device_busy & 0x2) {
		data->opcodes.read_status = 0x70;
		data->status_busy_bit = 7;
		data->status_busy = 0;
	} else if (bfp.polling_device_busy & 0x1) {
		data->opcodes.read_status = 0x05;
		data->status_busy_bit = 0;
		data->status_busy = 1;
	} else {
		SYS_LOG_ERR("Wrong Polling Device Busy");
		return -ENODEV;
	}

	data->config = *data->tem_config;

	data->config.frequency = CONFIG_SPI_FLASH_SPI_FREQ_0;

	if (data->data_lines == 4) {
		data->config.operation = SFDP_SPI_OPERATION_QUAD;
	} else if (data->data_lines == 2) {
		data->config.operation = SFDP_SPI_OPERATION_DUAL;
	} else {
		data->config.operation = SFDP_SPI_OPERATION_SINGLE;
	}

	SYS_LOG_INF("Basic Flash Parameters Table finish !");
	return 0;
}

static int spi_flash_sfdp_rpmc(struct device *dev, uint32_t addr, u8_t len)
{
	/* todo */
	SYS_LOG_ERR("Go to do about replay protected monotonic counters");
	return -ENODEV;
}

static int spi_flash_sfdp_4bai(struct device *dev, uint32_t addr, u8_t len)
{
	/* todo */
	SYS_LOG_ERR("Go to do about 4 byte address");
	return -ENODEV;
}

static int spi_flash_sfdp(struct device *dev,
			  struct spi_flash_init_config *init_config)
{
	struct spi_flash_data *data = dev->driver_data;
	struct spi_config config = {
	    .frequency = MHZ(50),
	    .slave = CONFIG_SPI_FLASH_SPI_SLAVE,
	};
	union sfdp_header sfdphdr;
	union sfdp_parameter_header phdr;
	off_t sfdp_offset = 0;
	int r, i;
	int lines;

	memset(&data->opcodes, SFDP_RESERVED_VALUE, sizeof(data->opcodes));
	data->tem_config = &config;
#if defined(CONFIG_SPI_FLASH_GPIO_SPI_CS)
	config.cs = &data->cs;
#endif
	for (lines = 4; lines > 0; lines >>= 1) {
		if (lines == 4) {
			config.operation = SFDP_SPI_OPERATION_QUAD;
		} else if (lines == 2) {
			config.operation = SFDP_SPI_OPERATION_DUAL;
		} else {
			config.operation = SFDP_SPI_OPERATION_SINGLE;
		}
		SYS_LOG_DBG("Try SFDP Read mode: %d-%d-%d", lines, lines,
			    lines);
		r = spi_flash_read_sfdp(dev, sfdp_offset, &sfdphdr,
					sizeof(sfdphdr));
		if (r == 0) {
			if (!data->lines) {
				data->lines = lines;
			}
		} else {
			continue;
		}
		if (sfdphdr.signature == SFDP_HEADER_SIGNATURE) {
			break;
		}
	}
	if (lines == 0) {
		SYS_LOG_ERR("Not found SFDP");
		return -ENODEV;
	}
	sfdp_offset += sizeof(sfdphdr);
	SYS_LOG_DBG("USE SFDP read mode: %d-%d-%d", lines, lines, lines);

	SYS_LOG_DBG("Revision: %d.%d, Number of Parameter Headers : %d",
		    sfdphdr.major_ver, sfdphdr.minor_ver, sfdphdr.nph + 1);

	for (i = 0; i <= sfdphdr.nph; i++) {
		r = spi_flash_read_sfdp(dev, sfdp_offset, &phdr, sizeof(phdr));
		if (r != 0) {
			return r;
		}
		sfdp_offset += sizeof(phdr);

		SYS_LOG_DBG(
		    "Parameter[%d], id: %04x, v%d.%d, addr: %06x, len: %d", i,
		    get_phdr_id(&phdr), phdr.major_ver, phdr.minor_ver,
		    phdr.addr, phdr.length);

		switch (get_phdr_id(&phdr)) {
		case SFDP_BFPT_ID:
			r = spi_flash_sfdp_bfp(dev, phdr.addr, phdr.length);
			break;
		case SFDP_SECTOR_MAP_ID:
			if (phdr.length > ARRAY_SIZE(init_config->smpt)) {
				SYS_LOG_ERR(
				    "CONFIG_SPI_FLASH_SMPT_SIZE is too small");
				return -ENODEV;
			}
			r = spi_flash_read_sfdp(dev, phdr.addr,
						init_config->smpt,
						phdr.length * sizeof(dword_t));
			break;
		case SFDP_RPMC_ID:
			r = spi_flash_sfdp_rpmc(dev, phdr.addr, phdr.length);
			break;
		case SFDP_FOUR_ADDR_ID:
			r = spi_flash_sfdp_4bai(dev, phdr.addr, phdr.length);
			break;
		default:
			SYS_LOG_WRN("Parameter[%d], id: %04x, "
				    "Undefined vendor device may not work "
				    "properly",
				    i, get_phdr_id(&phdr));
			continue;
		}
		if (r != 0) {
			return r;
		}
	}

	if (!data->address_lines || !data->data_lines) {
		SYS_LOG_ERR("SPI mode is not defined");
		return -ENODEV;
	}

	if (data->opcodes.read == SFDP_RESERVED_VALUE) {
		SYS_LOG_ERR("Read instruction is not defined");
		return -ENODEV;
	}

	if (!data->flash_size) {
		SYS_LOG_ERR("Flash size is not defined");
		return -ENODEV;
	}

	if (!data->page_size) {
		SYS_LOG_ERR("Page size is not defined");
		return -ENODEV;
	}

	data->tem_config = NULL;
	return 0;
}

static int spi_flash_read(struct device *dev, off_t offset, void *ptr,
			  size_t len)
{
	struct spi_flash_data *const data = dev->driver_data;
	struct sfdp_spi_buf_set buf_set = {0};
	bool four_addr = false;
	int r;

	if (offset < 0 || offset + len > data->flash_size) {
		SYS_LOG_ERR("Bad address value");
		return -ENXIO;
	}

	if (data->four_addr) {
		/* todo */
		SYS_LOG_ERR("Go to do about 4 byte address");
		return -ENOTSUP;
	}
	if (data->mode_clocks) {
		/* todo */
		SYS_LOG_ERR("Go to do about read mode clocks");
		return -ENOTSUP;
	}

	sfdp_spi_buf_append_cmd(dev, &buf_set, data->opcodes.read, true);
	sfdp_spi_buf_append_addr(dev, &buf_set, offset, four_addr, true);
	if (data->dummy_clocks) {
		sfdp_spi_buf_append(&buf_set, NULL,
				    data->dummy_clocks /
					(8 / data->address_lines));
	}
	sfdp_spi_buf_append(&buf_set, ptr, len);

	k_sem_take(&data->sem, K_FOREVER);
	wait_for_flash_idle(dev);
	r = sfdp_spi_transceive(dev, &buf_set);
	k_sem_give(&data->sem);

	return r;
}

static int spi_flash_write(struct device *dev, off_t offset, const void *ptr,
			   size_t len)
{
	struct spi_flash_data *const data = dev->driver_data;
	struct sfdp_spi_buf_set buf_set = {0};
	bool four_addr = false;
	int r = 0;
	u32_t l;

	if (data->opcodes.program == SFDP_RESERVED_VALUE) {
		SYS_LOG_ERR("No write method available");
		return -EINVAL;
	}

	if (offset < 0 || offset + len > data->flash_size) {
		SYS_LOG_ERR("Bad address value");
		return -ENXIO;
	}

	/* In order to make the device without write protection have the */
	/* same performance */
	if (data->write_protection_sw) {
		SYS_LOG_INF("Write address: %08x, size: %08x", offset, len);
		return 0;
	}

	if (data->four_addr) {
		/* todo */
		SYS_LOG_ERR("Go to do about 4 byte address");
		return -ENOTSUP;
	}

	l = data->page_size;
	l -= offset & (l - 1);
	l = min(l, len);
	k_sem_take(&data->sem, K_FOREVER);
	while (l > 0 && r == 0) {
		buf_set.count = 0;
		buf_set.offset = 0;
		sfdp_spi_buf_append_cmd(dev, &buf_set, data->opcodes.program,
					false);
		sfdp_spi_buf_append_addr(dev, &buf_set, offset, four_addr,
					 false);
		sfdp_spi_buf_append(&buf_set, (void *)ptr, l);
		wait_for_flash_idle(dev);
		r = spi_flash_write_enable(dev);
		if (r == 0) {
			r = sfdp_spi_write(dev, &buf_set);
		}
		ptr += l;
		len -= l;
		offset += l;
		l = min(len, data->page_size);
	}
	k_sem_give(&data->sem);
	SYS_LOG_INF("Write address: %08x, size: %08x", offset, len);
	return r;
}

static int spi_flash_erase_cmd(struct device *dev, u8_t opcode, off_t offset)
{
	struct spi_flash_data *const data = dev->driver_data;
	struct sfdp_spi_buf_set buf_set = {0};
	bool four_addr = false;
	int r;

	if (data->four_addr) {
		/* todo */
		SYS_LOG_ERR("Go to do about 4 byte address");
		return -ENOTSUP;
	}

	sfdp_spi_buf_append_cmd(dev, &buf_set, opcode, false);
	sfdp_spi_buf_append_addr(dev, &buf_set, offset, four_addr, false);

	wait_for_flash_idle(dev);
	r = spi_flash_write_enable(dev);
	if (r != 0) {
		return r;
	}
	return sfdp_spi_transceive(dev, &buf_set);
}

static int spi_flash_erase(struct device *dev, off_t offset, size_t size)
{
	struct spi_flash_data *const data = dev->driver_data;
	off_t offs = offset, region_offs;
	size_t left = size, erase_size, region_size;
	int ret = 0, i, et;
	u8_t erase_opcode;

	if (offset < 0 || offset + size > data->flash_size) {
		return -EFAULT;
	}

	/* In order to make the device without write protection have the */
	/* same performance */
	if (data->write_protection_sw) {
		SYS_LOG_INF("erase: %08x-%08x", offset, offset + size - 1);
		return 0;
	}

	/* chip erase */
	if (offset == 0 && left == data->flash_size) {
		k_sem_take(&data->sem, K_FOREVER);
		wait_for_flash_idle(dev);
		ret = spi_flash_write_enable(dev);
		if (ret == 0) {
			ret = spi_flash_cmd(dev, CMD_CHIP_ERASE);
		}
		SYS_LOG_INF("chip erase");
		k_sem_give(&data->sem);
		return ret;
	}

	/* 4k erase */
	if (data->smrp_count == 0) {
		if (data->opcodes.block_4k_erase == SFDP_RESERVED_VALUE) {
			SYS_LOG_ERR("No erase method available, %08x-%08x",
				    offset, offset + size - 1);
			return -EFAULT;
		}
		if ((offs | left) & (KB(4) - 1)) {
			SYS_LOG_ERR("No 4Kbyte alignment of address or range");
			SYS_LOG_ERR("address: %08x, size: %08x", offs, left);
			return -EFAULT;
		}
		erase_opcode = data->opcodes.block_4k_erase;
		erase_size = KB(4);
		k_sem_take(&data->sem, K_FOREVER);
		while (left && ret == 0) {
			SYS_LOG_DBG("erase instruction: %02x address: %08x",
				    erase_opcode, offs);
			ret = spi_flash_erase_cmd(dev, erase_opcode, offs);
		}
		SYS_LOG_INF("erase: %08x-%08x", offset, offset + size - 1);
		k_sem_give(&data->sem);
		return ret;
	}

	/* block erase */
	i = 0;
	erase_size = 0;
	erase_opcode = 0;
	region_offs = 0;
	region_size = (data->smrp[0].region_size + 1) << 8;
	k_sem_take(&data->sem, K_FOREVER);
	while (left && ret == 0) {
		while (offs >= region_offs + region_size) {
			i += 1;
			if (i >= data->smrp_count) {
				ret = -EFAULT;
				SYS_LOG_ERR(
				    "No erase method available, %08x-%08x",
				    offs, offs + left - 1);
				goto end_erase;
			}
			region_offs += region_size;
			region_size = (data->smrp[i].region_size + 1) << 8;
		}

		for (et = 3; et >= 0 && i < data->smrp_count; et--) {
			erase_size = 1 << data->erase_types[et].size;
			if ((((dword_t *)data->smrp)[i] & (1 << et)) == 0) {
				continue;
			}
			if ((offs - region_offs) & (erase_size - 1)) {
				continue;
			}
			if (erase_size > left) {
				continue;
			}
			break;
		}
		if (et < 0) {
			ret = -EFAULT;
			SYS_LOG_ERR("No erase method available, %08x-%08x",
				    offs, offs + left - 1);
			goto end_erase;
		}

		erase_opcode = data->erase_types[et].opcode;
		SYS_LOG_DBG("erase instruction: %02x address: %08x",
			    erase_opcode, offs);
		ret = spi_flash_erase_cmd(dev, erase_opcode, offs);
		offs += erase_size;
		left -= erase_size;
	}
	SYS_LOG_INF("erase: %08x-%08x", offset, offset + size - 1);
end_erase:
	k_sem_give(&data->sem);
	return ret;
}

static int spi_flash_write_protection_set(struct device *dev, bool enable)
{
	struct spi_flash_data *const data = dev->driver_data;

	if (data->write_protection) {
		return data->write_protection(dev, enable);
	}
	data->write_protection_sw = enable;
	return 0;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void spi_flash_pages_layout(struct device *dev,
				   const struct flash_pages_layout **layout,
				   size_t *layout_size)
{
	struct spi_flash_data *const data = dev->driver_data;

	if (layout) {
		*layout = data->pages_layouts;
	}
	if (layout_size) {
		*layout_size = data->pages_layouts_count;
	}
}
#endif

static const struct flash_driver_api spi_flash_api = {
	.read = spi_flash_read,
	.write = spi_flash_write,
	.erase = spi_flash_erase,
	.write_protection = spi_flash_write_protection_set,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = spi_flash_pages_layout,
#endif
	.write_block_size = 1,
};

static int spi_flash_search_sector_map(struct device *dev, dword_t *smpt,
				       int len)
{
	u8_t id = 0;
	struct spi_flash_data *data = dev->driver_data;
	union sfdp_sector_map_parameters *smp = (void *)smpt;

	while (len > 0) {
		if (smp->type) {
			if (smp->map.id == id) {
				if (smp->map.region_count + 1 >
				    ARRAY_SIZE(data->smrp)) {
					SYS_LOG_ERR("SMRP array size is too "
						    "small");
					return -ENODEV;
				}
				data->smrp_count = smp->map.region_count + 1;
				memcpy(data->smrp, smp + 1,
				       sizeof(data->smrp[0]) *
					   data->smrp_count);
				SYS_LOG_INF("Sector Map finish !");
				return 0;
			}
			len -= smp->map.region_count + 2;
			smp += smp->map.region_count + 2;
		} else {
			/* Configuration Detection */
			/* todo */
			SYS_LOG_WRN(
			    "Go to do sector map Configuration Detection");
			len -= 2;
			smp += 2;
		}
		if (smp->sequence_end) {
			SYS_LOG_INF("Sector Map finish !");
			return 0;
		}
	}
	SYS_LOG_ERR("Wrong Sector Map Parameters");
	return -ENODEV;
}

static int spi_flash_init(struct device *dev)
{
	struct spi_flash_data *data = dev->driver_data;
	struct spi_flash_init_config init_config = {0};
	int r;

	data->spi = device_get_binding(CONFIG_SPI_FLASH_SPI_NAME);
	if (!data->spi) {
		return -EIO;
	}

#if defined(CONFIG_SPI_FLASH_GPIO_SPI_CS)
	data->cs = (struct spi_cs_control){
	    .gpio_dev = device_get_binding(
		    CONFIG_SPI_FLASH_GPIO_SPI_CS_DRV_NAME),
	    .gpio_pin = CONFIG_SPI_FLASH_GPIO_SPI_CS_PIN,
	    .delay = CONFIG_SPI_FLASH_GPIO_CS_WAIT_DELAY};
	if (!data->cs.gpio_dev) {
		return -EIO;
	}
#endif

	r = spi_flash_sfdp(dev, &init_config);
	if (r != 0) {
		return r;
	}

	/* todo reset flash chip */
	/* todo init flash chip */

	r = spi_flash_search_sector_map(dev, init_config.smpt,
					ARRAY_SIZE(init_config.smpt));
	if (r != 0) {
		return r;
	}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	if (data->opcodes.block_4k_erase != SFDP_RESERVED_VALUE) {
		/* 4 kilobyte Erase is supported throughout the device */
		data->pages_layouts[0].pages_size = KB(4);
		data->pages_layouts[0].pages_count = data->flash_size / KB(4);
		data->pages_layouts_count = 1;
	} else if (data->smrp_count == 0) {
		/* Only allow Chip Erase  */
		data->pages_layouts[0].pages_size = data->flash_size;
		data->pages_layouts[0].pages_count = 1;
		data->pages_layouts_count = 1;
	} else {
		/* 4 kilobyte Erase is unavailable. */
		int last = 0, now = 1, i;
		union sfdp_sector_map_region_parameters smrp;
		dword_t et_mask = 0;

		for (i = 0; i < ARRAY_SIZE(data->erase_types); i++) {
			if (data->erase_types[i].size) {
				et_mask |= 1 << i;
			}
		}
		for (i = 0; i < ARRAY_SIZE(data->pages_layouts) &&
			    last < data->smrp_count;
		     i++) {
			int et;

			smrp.dwords[0] = data->smrp[last].dwords[0] & et_mask;
			while (smrp.dwords[0] & data->smrp[now].dwords[0]) {
				smrp.dwords[0] &= data->smrp[now].dwords[0];
				if (++now == data->smrp_count) {
					break;
				}
			}
			if (smrp.erase_type_1) {
				et = 0;
			} else if (smrp.erase_type_2) {
				et = 1;
			} else if (smrp.erase_type_3) {
				et = 2;
			} else if (smrp.erase_type_4) {
				et = 3;
			} else {
				SYS_LOG_ERR(
				    "Wrong sector map region parameters");
				return -ENODEV;
			}
			data->pages_layouts[i].pages_size =
			    1 << data->erase_types[et].size;
			data->pages_layouts[i].pages_count = 0;
			while (last < now) {
				if (data->erase_types[et].size < 8) {
					data->pages_layouts[i].pages_count +=
					    (data->smrp[last].region_size + 1)
					    << (8 - data->erase_types[et].size);
				} else {
					data->pages_layouts[i].pages_count +=
					    (data->smrp[last].region_size +
					     1) >>
					    (data->erase_types[et].size - 8);
				}
				last++;
			}
		}
		if (last != data->smrp_count) {
			SYS_LOG_ERR(
			    "CONFIG_SPI_FLASH_LAYOUTS_ARRAY_SIZE is too small");
			return -ENODEV;
		}
		data->pages_layouts_count = i;
	}

	if (data->pages_layouts_count == 0) {
		SYS_LOG_ERR("Wrong pages layout");
		return -ENODEV;
	}
#endif

	k_sem_init(&data->sem, 1, UINT_MAX);
	dev->driver_api = &spi_flash_api;
	SYS_LOG_INF("SFDP finish !");
	return 0;
}

static struct spi_flash_data spi_flash_memory_data;

DEVICE_INIT(spi_flash_memory, CONFIG_SPI_FLASH_DRV_NAME, spi_flash_init,
	    &spi_flash_memory_data, NULL, POST_KERNEL,
	    CONFIG_SPI_FLASH_INIT_PRIORITY);

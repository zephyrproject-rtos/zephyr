/*
 * Copyright (c) 2022 Macronix International Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT jedec_spi_nand

#include <errno.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/init.h>
#include <string.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <stdlib.h>
#include "spi_nand.h"
#include "bch.h"

LOG_MODULE_REGISTER(spi_nand, CONFIG_FLASH_LOG_LEVEL);

/* Indicates that an access command includes bytes for the address.
 * If not provided the opcode is not followed by address bytes.
 */
#define NAND_ACCESS_ADDRESSED BIT(0)

/* Indicates that addressed access uses a 8-bit address regardless of
 * spi_nand_data::flag_8bit_addr.
 */
#define NAND_ACCESS_8BIT_ADDR BIT(1)

/* Indicates that addressed access uses a 16-bit address regardless of
 * spi_nand_data::flag_16bit_addr.
 */
#define NAND_ACCESS_16BIT_ADDR BIT(2)

/* Indicates that addressed access uses a 24-bit address regardless of
 * spi_nand_data::flag_32bit_addr.
 */
#define NAND_ACCESS_24BIT_ADDR BIT(3)

/* Indicates that addressed access uses a 32-bit address regardless of
 * spi_nand_data::flag_32bit_addr.
 */
#define NAND_ACCESS_32BIT_ADDR BIT(4)

/* Indicates that an access command is performing a write.  If not
 * provided access is a read.
 */
#define NAND_ACCESS_WRITE BIT(5)

#define NAND_ACCESS_DUMMY BIT(6)

/* Build-time data associated with the device. */
struct spi_nand_config {
	/* Devicetree SPI configuration */
	struct spi_dt_spec spi;

	uint8_t id[2];
	uint8_t support_conti_read;
};

/**
 * struct spi_nand_data - Structure for defining the SPI NAND access
 * @sem: The semaphore to access to the flash
 */
struct spi_nand_data {
	struct k_sem sem;

	/* Number of bytes per page */
	uint16_t page_size;

	/* Number of oob bytes per page */
	uint8_t oob_size;

	/* Number of pages per block */
	uint16_t page_num;

	/* Number of bytes per block */
	uint32_t block_size;

	/* Number of blocks per chip */
	uint16_t block_num;

	/* Size of flash, in bytes */
	uint32_t flash_size;

	uint8_t page_shift;

	uint8_t block_shift;

	bool continuous_read;

	bool read_recovery;

	uint8_t ecc_bits;

	uint8_t ecc_bytes;
	uint8_t ecc_steps;
	uint8_t ecc_layout_pos;
	uint32_t ecc_size;
	uint8_t *ecc_calc;
	uint8_t *ecc_code;
	uint8_t *page_buf;

	struct nand_bch_control {
		struct bch_code      *bch;
		unsigned int         *errloc;
		unsigned char        *eccmask;
	} nbc;
};

static const struct flash_parameters flash_nand_parameters = {
	.write_block_size = SPI_NAND_SUB_PAGE_SIZE,
	.erase_value = 0xff,
};

void bch_calculate_ecc(const struct device *dev,
		unsigned char *buf, unsigned char *code)
{
	struct spi_nand_data *data = dev->data;

	memset(code, 0, data->ecc_bytes);

	bch_encode(data->nbc.bch, buf, (unsigned int *)code);
}

void bch_correct_data(const struct device *dev,
		unsigned char *buf, unsigned char *read_ecc,
		unsigned char *calc_ecc)
{
	struct spi_nand_data *data = dev->data;

	bch_decode(data->nbc.bch, buf, (unsigned int *)calc_ecc);
}

/*
 * @brief Send an SPI command
 *
 * @param dev Device struct
 * @param opcode The command to send
 * @param access flags that determine how the command is constructed.
 *        See NAND_ACCESS_*.
 * @param addr The address to send
 * @param data The buffer to store or read the value
 * @param length The size of the buffer
 * @return 0 on success, negative errno code otherwise
 */
static int spi_nand_access(const struct device *const dev,
			  uint8_t opcode, unsigned int access,
			  off_t addr, void *data, size_t length)
{
	const struct spi_nand_config *const driver_cfg = dev->config;
	bool is_addressed = (access & NAND_ACCESS_ADDRESSED) != 0U;
	bool is_write = (access & NAND_ACCESS_WRITE) != 0U;
	uint8_t buf[5] = { 0 };
	uint8_t address_len = 0;
	struct spi_buf spi_buf[2] = {
		{
			.buf = buf,
			.len = 1,
		},
		{
			.buf = data,
			.len = length
		}
	};

	buf[0] = opcode;
	if (is_addressed) {
		union {
			uint32_t u32;
			uint8_t u8[4];
		} addr32 = {
			.u32 = sys_cpu_to_be32(addr),
		};

		if (access & NAND_ACCESS_32BIT_ADDR) {
			address_len = 4;
		} else if (access & NAND_ACCESS_24BIT_ADDR){
			address_len = 3;
		} else if (access & NAND_ACCESS_16BIT_ADDR){
			address_len = 2;
		} else if (access & NAND_ACCESS_8BIT_ADDR){
			address_len = 1;
		}
		memcpy(&buf[1], &addr32.u8[4 - address_len], address_len);
		spi_buf[0].len += address_len;
	};

	if (access & NAND_ACCESS_DUMMY) {
		spi_buf[0].len += 1;
	}

	const struct spi_buf_set tx_set = {
		.buffers = spi_buf,
		.count = (length != 0) ? 2 : 1,
	};

	const struct spi_buf_set rx_set = {
		.buffers = spi_buf,
		.count = 2,
	};

	if (is_write) {
		return spi_write_dt(&driver_cfg->spi, &tx_set);
	}

	return spi_transceive_dt(&driver_cfg->spi, &tx_set, &rx_set);
}

#define spi_nand_cmd_read(dev, opcode, dest, length) \
	spi_nand_access(dev, opcode, 0, 0, dest, length)
#define spi_nand_cmd_write(dev, opcode) \
	spi_nand_access(dev, opcode, NAND_ACCESS_WRITE, 0, NULL, 0)

/* Everything necessary to acquire owning access to the device.
 *
 * This means taking the lock .
 */
static void acquire_device(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_MULTITHREADING)) {
		struct spi_nand_data *const driver_data = dev->data;

		k_sem_take(&driver_data->sem, K_FOREVER);
	}
}

/* Everything necessary to release access to the device.
 *
 * This means releasing the lock.
 */
static void release_device(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_MULTITHREADING)) {
		struct spi_nand_data *const driver_data = dev->data;

		k_sem_give(&driver_data->sem);
	}
}


static int spi_nand_get_feature(const struct device *dev,
				uint8_t feature_addr, uint8_t *val)
{
    int ret = spi_nand_access(dev, SPI_NAND_CMD_GET_FEATURE,
			NAND_ACCESS_ADDRESSED | NAND_ACCESS_8BIT_ADDR,
			feature_addr, val, sizeof(*val));

	return ret;
}

static int spi_nand_set_feature(const struct device *dev,
				uint8_t feature_addr, uint8_t val)
{
    int ret = spi_nand_access(dev, SPI_NAND_CMD_SET_FEATURE,
			NAND_ACCESS_WRITE | NAND_ACCESS_ADDRESSED | NAND_ACCESS_8BIT_ADDR,
			feature_addr, &val, sizeof(val));

	return ret;
}

/**
 * @brief Wait until the flash is ready
 *
 * @note The device must be externally acquired before invoking this
 * function.
 *
 * This function should be invoked after every ERASE, PROGRAM, or
 * WRITE_STATUS operation before continuing.  This allows us to assume
 * that the device is ready to accept new commands at any other point
 * in the code.
 *
 * @param dev The device structure
 * @return 0 on success, negative errno code otherwise
 */
static int spi_nand_wait_until_ready(const struct device *dev)
{
	int ret;
	uint8_t reg = 0;

	do {
		ret = spi_nand_get_feature(dev, SPI_NAND_FEA_ADDR_STATUS, &reg);
	} while (!ret && (reg & SPI_NAND_WIP_BIT));

	return ret;
}

static int spi_nand_read_recovery_mode_set(const struct device *dev, uint8_t read_recovery_mode)
{
	int ret;

	acquire_device(dev);

	ret = spi_nand_set_feature(dev, SPI_NAND_FEA_ADDR_RECOVERY, read_recovery_mode);
	if (ret != 0) {
		LOG_ERR("set feature failed: %d", ret);
	}

	release_device(dev);
	return ret;
}

static int spi_nand_conti_read_enable(const struct device *dev)
{
	int ret;
	uint8_t secur_reg = 0;

	acquire_device(dev);

	ret = spi_nand_get_feature(dev, SPI_NAND_FEA_ADDR_CONF_B0, &secur_reg);
	if (ret != 0) {
		LOG_ERR("get feature failed: %d", ret);
		goto out;
	}

	secur_reg |= SPINAND_SECURE_BIT_CONT;
	ret = spi_nand_set_feature(dev, SPI_NAND_FEA_ADDR_CONF_B0, secur_reg);
	if (ret != 0) {
		LOG_ERR("set feature failed: %d", ret);
		goto out;
	}

	ret = spi_nand_get_feature(dev, SPI_NAND_FEA_ADDR_CONF_B0, &secur_reg);
	if (ret != 0) {
		LOG_ERR("get feature failed: %d", ret);
		goto out;
	}

	if (!(secur_reg & SPINAND_SECURE_BIT_CONT)) {
		LOG_ERR("Enable continuous read failed: %d\n", secur_reg);
	}

out: release_device(dev);
	return ret;
}

static int spi_nand_conti_read_disable(const struct device *dev)
{
	int ret;
	uint8_t secur_reg = 0;

	acquire_device(dev);

	ret = spi_nand_get_feature(dev, SPI_NAND_FEA_ADDR_CONF_B0, &secur_reg);
	if (ret != 0) {
		LOG_ERR("get feature failed: %d", ret);
		goto out;
	}

	secur_reg &= ~SPINAND_SECURE_BIT_CONT;
	ret = spi_nand_set_feature(dev, SPI_NAND_FEA_ADDR_CONF_B0, secur_reg);
	if (ret != 0) {
		LOG_ERR("set feature failed: %d", ret);
		goto out;
	}

	ret = spi_nand_get_feature(dev, SPI_NAND_FEA_ADDR_CONF_B0, &secur_reg);
	if (ret != 0) {
		LOG_ERR("get feature failed: %d", ret);
		goto out;
	}

	if (secur_reg & SPINAND_SECURE_BIT_CONT) {
		LOG_ERR("Disable continuous read failed: %d\n", secur_reg);
	}

out: release_device(dev);
	return ret;
}

static int spi_nand_conti_read_exit(const struct device *dev)
{
	return spi_nand_cmd_write(dev, SPI_NAND_CMD_RESET);
}

static int spi_nand_read_cont(const struct device *dev, off_t addr, void *dest,
			size_t size)
{
	struct spi_nand_data *data = dev->data;
	int ret = 0;

	acquire_device(dev);

	ret = spi_nand_access(dev, SPI_NAND_CMD_PAGE_READ,
		NAND_ACCESS_ADDRESSED | NAND_ACCESS_24BIT_ADDR,
		addr >> data->page_shift, NULL, 0);
	if (ret != 0) {
		LOG_ERR("page read failed: %d", ret);
		goto out;
	}

	ret = spi_nand_wait_until_ready(dev);

	ret = spi_nand_access(dev, SPI_NAND_CMD_READ_CACHE,
		NAND_ACCESS_ADDRESSED | NAND_ACCESS_24BIT_ADDR,
		((addr >> data->page_shift) & SPI_NAND_PAGE_MASK), dest, size);
	if (ret != 0) {
		LOG_ERR("read from cache failed: %d", ret);
		goto out;
	}

	ret = spi_nand_conti_read_exit(dev);

out: release_device(dev);

	return ret;
}

static int spi_nand_read_normal(const struct device *dev,
				off_t addr, void *dest, size_t size)
{
	struct spi_nand_data *data = dev->data;
	int ret = 0;
	uint32_t offset = 0;
	uint32_t chunk = 0;
	uint32_t read_bytes = 0;

	acquire_device(dev);

	while (size > 0) {
		/* Read on _page_size_bytes boundaries (Default 2048 bytes a page) */
		offset = addr % data->page_size;
		chunk = (offset + size < data->page_size) ?
			size : (data->page_size - offset);
		read_bytes = chunk;

		ret = spi_nand_access(dev, SPI_NAND_CMD_PAGE_READ,
			NAND_ACCESS_ADDRESSED | NAND_ACCESS_24BIT_ADDR,
			addr >> data->page_shift, NULL, 0);
		if (ret != 0) {
			LOG_ERR("page read failed: %d", ret);
			goto out;
		}

		ret = spi_nand_wait_until_ready(dev);

		ret = spi_nand_access(dev, SPI_NAND_CMD_READ_CACHE,
			NAND_ACCESS_ADDRESSED | NAND_ACCESS_16BIT_ADDR | NAND_ACCESS_DUMMY,
			(addr & SPI_NAND_PAGE_MASK), dest, read_bytes);
		if (ret != 0) {
			LOG_ERR("read from cache failed: %d", ret);
			goto out;
		}

		dest = (uint8_t *)dest + chunk;

		addr = (addr + SPI_NAND_PAGE_OFFSET) & (~SPI_NAND_PAGE_MASK);
		size -= chunk;
	}

out: release_device(dev);

	return ret;
}

static int spi_nand_read_software_ecc(const struct device *dev,
			off_t addr, void *dest, size_t size)
{
	struct spi_nand_data *data = dev->data;
	int ret = 0;
	uint32_t offset = 0;
	uint32_t chunk = 0;
	uint32_t read_bytes = 0;
	uint8_t ecc_steps = data->ecc_steps;
	uint8_t *p = (uint8_t *)data->page_buf;

	acquire_device(dev);

	while (size > 0) {
		/* Read on _page_size_bytes boundaries (Default 2048 bytes a page) */
		offset = addr % data->page_size;
		chunk = (offset + size < data->page_size) ?
			size : (data->page_size - offset);
		read_bytes = chunk;

		ret = spi_nand_access(dev, SPI_NAND_CMD_PAGE_READ,
			NAND_ACCESS_ADDRESSED | NAND_ACCESS_24BIT_ADDR,
			addr >> data->page_shift, NULL, 0);
		if (ret != 0) {
			LOG_ERR("page read failed: %d", ret);
			goto out;
		}

		ret = spi_nand_wait_until_ready(dev);

		ret = spi_nand_access(dev, SPI_NAND_CMD_READ_CACHE,
			NAND_ACCESS_ADDRESSED | NAND_ACCESS_16BIT_ADDR | NAND_ACCESS_DUMMY,
			(addr & SPI_NAND_PAGE_MASK), data->page_buf,
			data->page_size + data->oob_size);
		if (ret != 0) {
			LOG_ERR("read from cache failed: %d", ret);
			goto out;
		}

		memcpy(data->ecc_code, data->page_buf + data->page_size + data->ecc_layout_pos,
			data->ecc_bytes * data->ecc_steps);

		p = (uint8_t *)data->page_buf;
		ecc_steps = data->ecc_steps;
		for (uint8_t i = 0 ; ecc_steps; ecc_steps--, i += data->ecc_bytes, p += data->ecc_size) {
			memset(data->nbc.bch->input_data, 0x0, (1 << data->nbc.bch->m) / 8);
			memcpy(data->nbc.bch->input_data + data->ecc_bytes, p, data->ecc_size);

			int ret = bch_decode(data->nbc.bch, data->nbc.bch->input_data, (unsigned int *)(data->ecc_code + i));
			if (ret < 0) {
				LOG_ERR("Reading data failed");
				goto out;
			}
			memcpy(p, data->nbc.bch->input_data + data->ecc_bytes, data->ecc_size);
		}

		memcpy(dest, data->page_buf + offset, read_bytes);

		dest = (uint8_t *)dest + chunk;

		addr = (addr + SPI_NAND_PAGE_OFFSET) & (~SPI_NAND_PAGE_MASK);
		size -= chunk;
	}

out: release_device(dev);

	return ret;
}

static int spi_nand_read(const struct device *dev, off_t addr, void *dest,
			size_t size)
{
	int ret = 0;
	struct spi_nand_data *data = dev->data;

	if (data->ecc_bits == 0) {
		if (data->continuous_read) {
			ret = spi_nand_read_cont(dev, addr, dest, size);
		} else {
			ret = spi_nand_read_normal(dev, addr, dest, size);
		}
	} else {
		ret = spi_nand_read_software_ecc(dev, addr, dest, size);
	}

	return ret;
}

static int spi_nand_write(const struct device *dev, off_t addr,
			 const void *src,
			 size_t size)
{
	struct spi_nand_data *data = dev->data;
	int ret = 0;
	uint32_t offset = 0;
	uint32_t chunk = 0;
	uint32_t written_bytes = 0;

	/* address must be sub-page-aligned */
	if (addr & (SPI_NAND_SUB_PAGE_SIZE - 1)) {
		return -EINVAL;
	}

	/* size must be a multiple of sub-page */
	if ((size % SPI_NAND_SUB_PAGE_SIZE) != 0) {
		return -EINVAL;
	}

	acquire_device(dev);

	while (size > 0) {

		/* Don't write more than a page. */
		offset = addr % data->page_size;
		chunk = (offset + size < data->page_size) ? size : (data->page_size - offset);
		written_bytes = chunk;

		spi_nand_cmd_write(dev, SPI_NAND_CMD_WREN);

		if (data->ecc_bits == 0) {
			ret = spi_nand_access(dev, SPI_NAND_CMD_PP_LOAD,
				NAND_ACCESS_WRITE | NAND_ACCESS_ADDRESSED | NAND_ACCESS_16BIT_ADDR,
				(addr & SPI_NAND_PAGE_MASK), (void *)src, written_bytes);
			if (ret != 0) {
				LOG_ERR("program load failed: %d", ret);
				goto out;
			}
		} else {
			uint8_t *p = (uint8_t *)data->page_buf;
			uint8_t ecc_steps = data->ecc_steps;

			if (size < data->page_size) {
				LOG_ERR("Write failed");
				ret = -EINVAL;
				goto out;

			}

			/* prepare data */
			memset(data->page_buf, 0xff, data->page_size + data->oob_size);
			memcpy(data->page_buf + offset, (uint8_t *)src, written_bytes);

			/* calculate the software ECC */
			for (uint8_t i = 0; ecc_steps; ecc_steps--, i += data->ecc_bytes, p += data->ecc_size) {
				memset(data->nbc.bch->input_data, 0x0, (1 << data->nbc.bch->m) / 8);
				memcpy(data->nbc.bch->input_data + data->ecc_bytes, p, data->ecc_size);
				bch_calculate_ecc(dev, data->nbc.bch->input_data, data->ecc_calc + i);
			}

			/* prepare ECC code */
			memcpy(data->page_buf + data->page_size + data->ecc_layout_pos,
				data->ecc_calc, data->ecc_bytes * data->ecc_steps);

			written_bytes = data->page_size + data->oob_size;
			ret = spi_nand_access(dev, SPI_NAND_CMD_PP_LOAD,
				NAND_ACCESS_WRITE | NAND_ACCESS_ADDRESSED | NAND_ACCESS_16BIT_ADDR,
				(addr & SPI_NAND_PAGE_MASK), (void *)data->page_buf, written_bytes);
			if (ret != 0) {
				LOG_ERR("program load failed: %d", ret);
				goto out;
			}
		}

		ret = spi_nand_access(dev, SPI_NAND_CMD_PROGRAM_EXEC,
			NAND_ACCESS_WRITE | NAND_ACCESS_ADDRESSED | NAND_ACCESS_24BIT_ADDR,
			addr >> data->page_shift, NULL, 0);
		if (ret != 0) {
			LOG_ERR("program excute failed: %d", ret);
			goto out;
		}

		src = (const uint8_t *)(src) + chunk;
		addr = (addr + SPI_NAND_PAGE_OFFSET) & (~SPI_NAND_PAGE_MASK);
		size -= chunk;

		ret = spi_nand_wait_until_ready(dev);
	}

out: release_device(dev);
	return ret;
}

static int spi_nand_erase(const struct device *dev, off_t addr, size_t size)
{
	struct spi_nand_data *data = dev->data;
	int ret = 0;

	/* address must be block-aligned */
	if (!(addr & SPI_NAND_BLOCK_MASK) && (addr != 0)) {
		return -EINVAL;
	}

	/* size must be a multiple of blocks */
	if ((size % data->block_size) != 0) {
		return -EINVAL;
	}

	acquire_device(dev);

	while ((size > 0) && (ret == 0)) {
		spi_nand_cmd_write(dev, SPI_NAND_CMD_WREN);

		ret = spi_nand_access(dev, SPI_NAND_CMD_BE,
			NAND_ACCESS_ADDRESSED | NAND_ACCESS_24BIT_ADDR,
			addr >> data->page_shift, NULL, 0);
		if (ret != 0) {
			LOG_ERR("erase failed: %d", ret);
			goto out;
		}

		addr += SPI_NAND_BLOCK_OFFSET;
		size -= data->block_size;

		ret = spi_nand_wait_until_ready(dev);
		if (ret != 0) {
			LOG_ERR("wait ready failed: %d", ret);
			goto out;
		}
	}

out: release_device(dev);

	return ret;
}

static int spi_nand_check_id(const struct device *dev)
{
	const struct spi_nand_config *cfg = dev->config;
	uint8_t const *expected_id = cfg->id;
	uint8_t read_id[SPI_NAND_ID_LEN];
	if (read_id == NULL) {
		return -EINVAL;
	}

	acquire_device(dev);

    int ret = spi_nand_access(dev, SPI_NAND_CMD_RDID, NAND_ACCESS_DUMMY,
			0, read_id, SPI_NAND_ID_LEN);

	release_device(dev);

	if (memcmp(expected_id, read_id, sizeof(read_id)) != 0) {
		LOG_ERR("Wrong ID: %02X %02X , "
			"expected: %02X %02X ",
			read_id[0], read_id[1],
			expected_id[0], expected_id[1]);
		return -ENODEV;
	}

	return ret;
}

void bch_ecc_init(const struct device *dev, uint8_t ecc_bits)
{
	struct spi_nand_data *data = dev->data;
	unsigned int m, t, i;
	unsigned char *erased_page;
	unsigned int eccsize = 410;
	unsigned int eccbytes = 0;

	m = find_last_set(1 + 8 * eccsize);
	t = ecc_bits;

	data->ecc_bytes = eccbytes = ((m * t + 31) / 32) * 4;
	data->ecc_size = eccsize;
	data->ecc_steps = data->page_size / eccsize;
	data->ecc_layout_pos = 2; /* skip the bad block mark for Macronix spi nand */

	data->nbc.bch = bch_init(m, t);
	if (!data->nbc.bch) {
		return;
	}

	/* verify that eccbytes has the expected value */
	if (data->nbc.bch->ecc_words * 4 != eccbytes) {
		LOG_ERR("invalid eccbytes %u, should be %u\n",
			eccbytes, data->nbc.bch->ecc_words);
		return;
	}

	data->page_buf = (uint8_t *)k_malloc(data->page_size + data->oob_size);
	if (data->page_buf == NULL) {
		LOG_ERR("Not enougn heap \n");
	}
	data->ecc_calc = (uint8_t *)k_malloc(data->ecc_steps * data->ecc_bytes);
	if (data->ecc_calc == NULL) {
		LOG_ERR("Not enougn heap \n");
	}
	data->ecc_code = (uint8_t *)k_malloc(data->ecc_steps * data->ecc_bytes);
	if (data->ecc_code == NULL) {
		LOG_ERR("Not enougn heap \n");
	}
	data->nbc.eccmask = (unsigned char *)k_malloc(eccbytes);

	data->nbc.errloc = (unsigned int *)k_malloc(t * sizeof(*data->nbc.errloc));
	if (!data->nbc.eccmask || !data->nbc.errloc) {
		LOG_ERR("Not enougn heap \n");
		return;
	}
	/*
	* compute and store the inverted ecc of an erased ecc block
	*/
	erased_page = (unsigned char *)k_malloc(eccsize);
	if (!erased_page) {
		return;
	}
	memset(data->page_buf, 0xff, data->page_size + data->oob_size);
	memset(erased_page, 0xff, eccsize);
	memset(data->nbc.eccmask, 0, eccbytes);
	bch_encode(data->nbc.bch, erased_page, (unsigned int *)data->nbc.eccmask);
	k_free(erased_page);

	for (i = 0; i < eccbytes; i++) {
		data->nbc.eccmask[i] ^= 0xff;
	}
}

void bch_ecc_free(const struct device *dev)
{
	struct spi_nand_data *data = dev->data;
	bch_free(data->nbc.bch);
	k_free(data->nbc.errloc);
	k_free(data->nbc.eccmask);
	k_free(data->page_buf);
	k_free(data->ecc_calc);
	k_free(data->ecc_code);
}

static int spi_nand_read_otp_onfi(const struct device *dev)
{
	int ret;
	uint8_t secur_reg = 0, onfi_table[256];
	struct spi_nand_data *data = dev->data;

	data->page_shift = 12;
	data->page_size = 2048;

	acquire_device(dev);

	ret = spi_nand_get_feature(dev, SPI_NAND_FEA_ADDR_CONF_B0, &secur_reg);
	if (ret != 0) {
		LOG_ERR("get feature failed: %d", ret);
		goto out0;
	}

	secur_reg |= SPINAND_SECURE_BIT_OTP_EN;
	ret = spi_nand_set_feature(dev, SPI_NAND_FEA_ADDR_CONF_B0, secur_reg);
	if (ret != 0) {
		LOG_ERR("set feature failed: %d", ret);
		goto out0;
	}

	ret = spi_nand_get_feature(dev, SPI_NAND_FEA_ADDR_CONF_B0, &secur_reg);
	if (ret != 0) {
		LOG_ERR("get feature failed: %d", ret);
		goto out0;
	}

	if (!(secur_reg & SPINAND_SECURE_BIT_OTP_EN)) {
		LOG_ERR("Enable OTP failed: %d\n", secur_reg);
		ret = -EINVAL;
	}

out0: release_device(dev);
	if (ret != 0) {
		return ret;
	}

	ret = spi_nand_read(dev, 1 << data->page_shift, onfi_table, sizeof(onfi_table));
	if (ret != 0) {
		LOG_ERR("read onfi table failed: %d", ret);
		return ret;
	}

	if (onfi_table[0] == 'O' && onfi_table[1] == 'N'
		&& onfi_table[2] == 'F' && onfi_table[3] == 'I') {
		LOG_ERR("ONFI table found\n");
		data->page_size = onfi_table[80] + (onfi_table[81] << 8) + (onfi_table[82] << 16);
		data->oob_size = onfi_table[84] + (onfi_table[85] << 8);
		data->page_num = onfi_table[92] + (onfi_table[93] << 8);
		data->block_num = onfi_table[96] + (onfi_table[97] << 8);
		data->block_size = data->page_size * data->page_num;
		switch (data->page_size) {
			case 2048 :
				data->page_shift = 12;
			break;
			case 4096 :
				data->page_shift = 13;
			break;
		}
		switch (data->page_num) {
			case 64 :
				data->block_shift = data->page_shift + 6;
			break;
			case 128 :
				data->block_shift = data->page_shift + 7;
			break;
			case 256 :
				data->block_shift = data->page_shift + 8;
			break;
		}
		data->flash_size = data->block_size * data->block_num;
		data->ecc_bits = onfi_table[112];

		if (data->ecc_bits > 0) {
			bch_ecc_init(dev, data->ecc_bits);
		} else {
			acquire_device(dev);

			secur_reg |= SPINAND_SECURE_BIT_ECC_EN;
			ret = spi_nand_set_feature(dev,
				SPI_NAND_FEA_ADDR_CONF_B0, secur_reg);
			if (ret != 0) {
				LOG_ERR("set feature failed: %d", ret);
				goto out1;
			}

			release_device(dev);
		}

		if (onfi_table[167] & 0x01) {
			data->read_recovery = true;
		} else {
			data->read_recovery = false;
		}

		if (onfi_table[168] & 0x02) {
			data->continuous_read = true;

			ret = spi_nand_conti_read_enable(dev);
			if (ret != 0) {
				LOG_ERR("SPI NAND Set continuous read enable Failed: %d", ret);
				return ret;
			}
		} else {
			data->continuous_read = false;
		}
	} else {
		LOG_ERR("ONFI table not found");
		return 0;
	}

	acquire_device(dev);

	ret = spi_nand_get_feature(dev, SPI_NAND_FEA_ADDR_CONF_B0, &secur_reg);
	if (ret != 0) {
		LOG_ERR("get feature failed: %d", ret);
		goto out1;
	}

	secur_reg &= ~SPINAND_SECURE_BIT_OTP_EN;
	ret = spi_nand_set_feature(dev, SPI_NAND_FEA_ADDR_CONF_B0, secur_reg);
	if (ret != 0) {
		LOG_ERR("set feature failed: %d", ret);
		goto out1;
	}

	ret = spi_nand_get_feature(dev, SPI_NAND_FEA_ADDR_CONF_B0, &secur_reg);
	if (ret != 0) {
		LOG_ERR("get feature failed: %d", ret);
		goto out1;
	}

	if (secur_reg & SPINAND_SECURE_BIT_OTP_EN) {
		LOG_ERR("Disable OTP failed: %d\n", secur_reg);
		ret = -EINVAL;
	}

out1: release_device(dev);
	return ret;
}

/**
 * @brief Configure the flash
 *
 * @param dev The flash device structure
 * @param info The flash info structure
 * @return 0 on success, negative errno code otherwise
 */
static int spi_nand_configure(const struct device *dev)
{
	const struct spi_nand_config *cfg = dev->config;

	uint8_t reg = 0;
	int rc;

	/* Validate bus and CS is ready */
	if (!spi_is_ready_dt(&cfg->spi)) {
		return -ENODEV;
	}

	rc = spi_nand_check_id(dev);
	if (rc != 0) {
		LOG_ERR("Check ID failed: ");
		return -ENODEV;
	}

	/* Check for block protect bits that need to be cleared. */
	acquire_device(dev);

	rc = spi_nand_get_feature(dev, SPI_NAND_FEA_ADDR_BLOCK_PROT, &reg);

	if (rc != 0) {
		LOG_ERR("get feature failed: %d", rc);
		goto out;
	}

	/* Only clear if GET_FEATURE worked and something's set. */
	if (reg & SPINAND_BLOCK_PROT_BIT_BP_MASK) {
		reg = 0;
		rc = spi_nand_set_feature(dev, SPI_NAND_FEA_ADDR_BLOCK_PROT, reg);
		if (rc != 0) {
		    LOG_ERR("set feature failed: %d", rc);
	    }
	}

out: release_device(dev);

	if (rc != 0) {
		return rc;
	}

	rc = spi_nand_read_otp_onfi(dev);
	if (rc != 0) {
		LOG_ERR("SFDP read failed: %d", rc);
		return -ENODEV;
	}

	return 0;
}

/**
 * @brief Initialize and configure the flash
 *
 * @param name The flash name
 * @return 0 on success, negative errno code otherwise
 */
static int spi_nand_init(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_MULTITHREADING)) {
		struct spi_nand_data *const driver_data = dev->data;

		k_sem_init(&driver_data->sem, 1, K_SEM_MAX_LIMIT);
	}

	return spi_nand_configure(dev);
}

static const struct flash_parameters *
flash_nand_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_nand_parameters;
}

static const struct flash_driver_api spi_nand_api = {
	.read = spi_nand_read,
	.write = spi_nand_write,
	.erase = spi_nand_erase,
	.get_parameters = flash_nand_get_parameters,
};


static const struct spi_nand_config spi_nand_config_0 = {
	.spi = SPI_DT_SPEC_INST_GET(0, SPI_WORD_SET(8),
				    CONFIG_SPI_NAND_CS_WAIT_DELAY),
	.id = DT_INST_PROP(0, id),
	.support_conti_read = DT_INST_PROP(0, support_conti_read),
};

static struct spi_nand_data spi_nand_data_0;

DEVICE_DT_INST_DEFINE(0, &spi_nand_init, NULL,
		 &spi_nand_data_0, &spi_nand_config_0,
		 POST_KERNEL, CONFIG_SPI_NAND_INIT_PRIORITY,
		 &spi_nand_api);

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This driver is written based on the Altera's
 * Nios-II QSPI Controller HAL driver.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <string.h>
#include <zephyr/drivers/flash.h>
#include <errno.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/sys/util.h>
#include "flash_priv.h"
#include "altera_generic_quad_spi_controller2_regs.h"
#include "altera_generic_quad_spi_controller2.h"

#define LOG_LEVEL CONFIG_FLASH_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash_nios2_qspi);

/*
 * Remove the following macros once the Altera HAL
 * supports the QSPI Controller v2 IP.
 */
#define ALTERA_QSPI_CONTROLLER2_FLAG_STATUS_REG		0x0000001C
#define FLAG_STATUS_PROTECTION_ERROR			(1 << 1)
#define FLAG_STATUS_PROGRAM_SUSPENDED			(1 << 2)
#define FLAG_STATUS_PROGRAM_ERROR			(1 << 4)
#define FLAG_STATUS_ERASE_ERROR				(1 << 5)
#define FLAG_STATUS_ERASE_SUSPENDED			(1 << 6)
#define FLAG_STATUS_CONTROLLER_READY			(1 << 7)

/* ALTERA_QSPI_CONTROLLER2_STATUS_REG bits */
#define STATUS_PROTECTION_POS				2
#define STATUS_PROTECTION_MASK				0x1F
#define STATUS_PROTECTION_EN_VAL			0x17
#define STATUS_PROTECTION_DIS_VAL			0x0

/* ALTERA_QSPI_CONTROLLER2_MEM_OP_REG bits */
#define MEM_OP_ERASE_CMD				0x00000002
#define MEM_OP_WRITE_EN_CMD				0x00000004
#define MEM_OP_SECTOR_OFFSET_BIT_POS			8
#define MEM_OP_UNLOCK_ALL_SECTORS			0x00000003
#define MEM_OP_LOCK_ALL_SECTORS				0x00000F03

#define NIOS2_QSPI_BLANK_WORD				0xFFFFFFFF

#define NIOS2_WRITE_BLOCK_SIZE				4

#define USEC_TO_MSEC(x)					(x / 1000)

struct flash_nios2_qspi_config {
	alt_qspi_controller2_dev qspi_dev;
	struct k_sem sem_lock;
};

static const struct flash_parameters flash_nios2_qspi_parameters = {
	.write_block_size = NIOS2_WRITE_BLOCK_SIZE,
	.erase_value = 0xff,
};

static int flash_nios2_qspi_write_protection(const struct device *dev,
					     bool enable);

static int flash_nios2_qspi_erase(const struct device *dev, off_t offset,
				  size_t len)
{
	struct flash_nios2_qspi_config *flash_cfg = dev->data;
	alt_qspi_controller2_dev *qspi_dev = &flash_cfg->qspi_dev;
	uint32_t block_offset, offset_in_block, length_to_erase;
	uint32_t erase_offset = offset; /* address of next byte to erase */
	uint32_t remaining_length = len; /* length of data left to be erased */
	uint32_t flag_status;
	int32_t rc = 0, i, timeout, rc2;

	k_sem_take(&flash_cfg->sem_lock, K_FOREVER);

	rc = flash_nios2_qspi_write_protection(dev, false);
	if (rc) {
		goto qspi_erase_err;
	}
	/*
	 * check if offset is word aligned and
	 * length is with in the range
	 */
	if (((offset + len) > qspi_dev->data_end) ||
			(0 != (erase_offset &
			       (NIOS2_WRITE_BLOCK_SIZE - 1)))) {
		LOG_ERR("erase failed at offset 0x%lx", (long)offset);
		rc = -EINVAL;
		goto qspi_erase_err;
	}

	for (i = offset/qspi_dev->sector_size;
			i < qspi_dev->number_of_sectors; i++) {

		if ((remaining_length <= 0U) ||
				erase_offset >= (offset + len)) {
			break;
		}

		block_offset = 0U; /* block offset in byte addressing */
		offset_in_block = 0U; /* offset into current sector to erase */
		length_to_erase = 0U; /* length to erase in current sector */

		/* calculate current sector/block offset in byte addressing */
		block_offset = erase_offset & ~(qspi_dev->sector_size - 1);

		/* calculate offset into sector/block if there is one */
		if (block_offset != erase_offset) {
			offset_in_block = erase_offset - block_offset;
		}

		/* calculate the byte size of data to be written in a sector */
		length_to_erase = MIN(qspi_dev->sector_size - offset_in_block,
							remaining_length);

		/* Erase sector */
		IOWR_32DIRECT(qspi_dev->csr_base,
				ALTERA_QSPI_CONTROLLER2_MEM_OP_REG,
				MEM_OP_WRITE_EN_CMD);
		IOWR_32DIRECT(qspi_dev->csr_base,
				ALTERA_QSPI_CONTROLLER2_MEM_OP_REG,
				(i << MEM_OP_SECTOR_OFFSET_BIT_POS)
				| MEM_OP_ERASE_CMD);

		/*
		 * poll the status register to know the
		 * completion of the erase operation.
		 */
		timeout = ALTERA_QSPI_CONTROLLER2_1US_TIMEOUT_VALUE;
		while (timeout > 0) {
			/* wait for 1 usec */
			k_busy_wait(1);

			flag_status = IORD_32DIRECT(qspi_dev->csr_base,
				ALTERA_QSPI_CONTROLLER2_FLAG_STATUS_REG);

			if (flag_status & FLAG_STATUS_CONTROLLER_READY) {
				break;
			}

			timeout--;
		}

		if ((flag_status & FLAG_STATUS_ERASE_ERROR) ||
				(flag_status & FLAG_STATUS_PROTECTION_ERROR)) {
			LOG_ERR("erase failed, Flag Status Reg:0x%x",
								flag_status);
			rc = -EIO;
			goto qspi_erase_err;
		}

		/* update remaining length and erase_offset */
		remaining_length -= length_to_erase;
		erase_offset += length_to_erase;
	}

qspi_erase_err:
	rc2 = flash_nios2_qspi_write_protection(dev, true);

	if (!rc) {
		rc = rc2;
	}

	k_sem_give(&flash_cfg->sem_lock);
	return rc;

}

static int flash_nios2_qspi_write_block(const struct device *dev,
					int block_offset,
					int mem_offset, const void *data,
					size_t len)
{
	struct flash_nios2_qspi_config *flash_cfg = dev->data;
	alt_qspi_controller2_dev *qspi_dev = &flash_cfg->qspi_dev;
	uint32_t buffer_offset = 0U; /* offset into data buffer to get write data */
	int32_t remaining_length = len; /* length left to write */
	uint32_t write_offset = mem_offset; /* offset into flash to write too */
	uint32_t word_to_write, padding, bytes_to_copy;
	uint32_t flag_status;
	int32_t rc = 0;

	while (remaining_length > 0) {
		/* initialize word to write to blank word */
		word_to_write = NIOS2_QSPI_BLANK_WORD;

		/* bytes to pad the next word that is written */
		padding = 0U;

		/* number of bytes from source to copy */
		bytes_to_copy = NIOS2_WRITE_BLOCK_SIZE;

		/*
		 * we need to make sure the write is word aligned
		 * this should only be true at most 1 time
		 */
		if (0 != (write_offset & (NIOS2_WRITE_BLOCK_SIZE - 1))) {
			/*
			 * data is not word aligned calculate padding bytes
			 * need to add before start of a data offset
			 */
			padding = write_offset & (NIOS2_WRITE_BLOCK_SIZE - 1);

			/*
			 * update variables to account
			 * for padding being added
			 */
			bytes_to_copy -= padding;

			if (bytes_to_copy > remaining_length) {
				bytes_to_copy = remaining_length;
			}

			write_offset = write_offset - padding;

			if (0 != (write_offset &
					(NIOS2_WRITE_BLOCK_SIZE - 1))) {
				rc = -EINVAL;
				goto qspi_write_block_err;
			}
		} else {
			if (bytes_to_copy > remaining_length) {
				bytes_to_copy = remaining_length;
			}
		}

		/* Check memcpy length is within NIOS2_WRITE_BLOCK_SIZE */
		if (padding + bytes_to_copy > NIOS2_WRITE_BLOCK_SIZE) {
			rc = -EINVAL;
			goto qspi_write_block_err;
		}

		/* prepare the word to be written */
		memcpy((uint8_t *)&word_to_write + padding,
				(const uint8_t *)data + buffer_offset,
				bytes_to_copy);

		/* enable write */
		IOWR_32DIRECT(qspi_dev->csr_base,
				ALTERA_QSPI_CONTROLLER2_MEM_OP_REG,
				MEM_OP_WRITE_EN_CMD);

		/* write to flash 32 bits at a time */
		IOWR_32DIRECT(qspi_dev->data_base, write_offset, word_to_write);

		/* check whether write operation is successful */
		flag_status = IORD_32DIRECT(qspi_dev->csr_base,
				ALTERA_QSPI_CONTROLLER2_FLAG_STATUS_REG);

		if ((flag_status & FLAG_STATUS_PROGRAM_ERROR) ||
			(flag_status & FLAG_STATUS_PROTECTION_ERROR)) {
			LOG_ERR("write failed, Flag Status Reg:0x%x",
								flag_status);
			rc = -EIO; /* sector might be protected */
			goto qspi_write_block_err;
		}

		/* update offset and length variables */
		buffer_offset += bytes_to_copy;
		remaining_length -= bytes_to_copy;
		write_offset = write_offset + NIOS2_WRITE_BLOCK_SIZE;
	}

qspi_write_block_err:
	return rc;
}

static int flash_nios2_qspi_write(const struct device *dev, off_t offset,
				  const void *data, size_t len)
{
	struct flash_nios2_qspi_config *flash_cfg = dev->data;
	alt_qspi_controller2_dev *qspi_dev = &flash_cfg->qspi_dev;
	uint32_t block_offset, offset_in_block, length_to_write;
	uint32_t write_offset = offset; /* address of next byte to write */
	uint32_t buffer_offset = 0U; /* offset into source buffer */
	uint32_t remaining_length = len; /* length of data left to be written */
	int32_t rc = 0, i, rc2;

	k_sem_take(&flash_cfg->sem_lock, K_FOREVER);

	rc = flash_nios2_qspi_write_protection(dev, false);
	if (rc) {
		goto qspi_write_err;
	}
	/*
	 * check if offset is word aligned and
	 * length is with in the range
	 */
	if ((data == NULL) || ((offset + len) > qspi_dev->data_end) ||
			(0 != (write_offset &
			       (NIOS2_WRITE_BLOCK_SIZE - 1)))) {
		LOG_ERR("write failed at offset 0x%lx", (long)offset);
		rc = -EINVAL;
		goto qspi_write_err;
	}

	for (i = offset/qspi_dev->sector_size;
			i < qspi_dev->number_of_sectors; i++) {

		if (remaining_length <= 0U) {
			break;
		}

		block_offset = 0U; /* block offset in byte addressing */
		offset_in_block = 0U; /* offset into current sector to write */
		length_to_write = 0U; /* length to write to current sector */

		/* calculate current sector/block offset in byte addressing */
		block_offset = write_offset & ~(qspi_dev->sector_size - 1);

		/* calculate offset into sector/block if there is one */
		if (block_offset != write_offset) {
			offset_in_block = write_offset - block_offset;
		}

		/* calculate the byte size of data to be written in a sector */
		length_to_write = MIN(qspi_dev->sector_size - offset_in_block,
							remaining_length);

		rc = flash_nios2_qspi_write_block(dev,
				block_offset, write_offset,
				(const uint8_t *)data + buffer_offset,
				length_to_write);
		if (rc < 0) {
			goto qspi_write_err;
		}

		/* update remaining length and buffer_offset */
		remaining_length -= length_to_write;
		buffer_offset += length_to_write;
		write_offset += length_to_write;
	}

qspi_write_err:
	rc2 = flash_nios2_qspi_write_protection(dev, true);

	if (!rc) {
		rc = rc2;
	}

	k_sem_give(&flash_cfg->sem_lock);
	return rc;
}

static int flash_nios2_qspi_read(const struct device *dev, off_t offset,
				 void *data, size_t len)
{
	struct flash_nios2_qspi_config *flash_cfg = dev->data;
	alt_qspi_controller2_dev *qspi_dev = &flash_cfg->qspi_dev;
	uint32_t buffer_offset = 0U; /* offset into data buffer to get read data */
	uint32_t remaining_length = len; /* length left to read */
	uint32_t read_offset = offset; /* offset into flash to read from */
	uint32_t word_to_read, bytes_to_copy;
	int32_t rc = 0;

	/*
	 * check if offset and length are within the range
	 */
	if ((data == NULL) || (offset < qspi_dev->data_base) ||
	    ((offset + len) > qspi_dev->data_end)) {
		LOG_ERR("read failed at offset 0x%lx", (long)offset);
		return -EINVAL;
	}

	if (!len) {
		return 0;
	}

	k_sem_take(&flash_cfg->sem_lock, K_FOREVER);

	/* first unaligned start */
	read_offset &= ~(NIOS2_WRITE_BLOCK_SIZE - 1U);
	if (offset > read_offset) {
		/* number of bytes from source to copy */
		bytes_to_copy = NIOS2_WRITE_BLOCK_SIZE - (offset - read_offset);
		if (bytes_to_copy > remaining_length) {
			bytes_to_copy = remaining_length;
		}
		/* read from flash 32 bits at a time */
		word_to_read = IORD_32DIRECT(qspi_dev->data_base, read_offset);
		memcpy((uint8_t *)data, (uint8_t *)&word_to_read + offset -
		       read_offset, bytes_to_copy);
		/* update offset and length variables */
		read_offset += NIOS2_WRITE_BLOCK_SIZE;
		buffer_offset += bytes_to_copy;
		remaining_length -= bytes_to_copy;
	}

	/* aligned part, including unaligned end */
	while (remaining_length > 0) {
		/* number of bytes from source to copy */
		bytes_to_copy = NIOS2_WRITE_BLOCK_SIZE;

		if (bytes_to_copy > remaining_length) {
			bytes_to_copy = remaining_length;
		}

		/* read from flash 32 bits at a time */
		word_to_read = IORD_32DIRECT(qspi_dev->data_base, read_offset);
		memcpy((uint8_t *)data + buffer_offset, &word_to_read,
		       bytes_to_copy);
		/* update offset and length variables */
		read_offset += bytes_to_copy;
		buffer_offset += bytes_to_copy;
		remaining_length -= bytes_to_copy;
	}

	k_sem_give(&flash_cfg->sem_lock);
	return rc;
}

static int flash_nios2_qspi_write_protection(const struct device *dev,
					     bool enable)
{
	struct flash_nios2_qspi_config *flash_cfg = dev->data;
	alt_qspi_controller2_dev *qspi_dev = &flash_cfg->qspi_dev;
	uint32_t status, lock_val;
	int32_t rc = 0, timeout;

	/* set write enable */
	IOWR_32DIRECT(qspi_dev->csr_base,
			ALTERA_QSPI_CONTROLLER2_MEM_OP_REG,
			MEM_OP_WRITE_EN_CMD);
	if (enable) {
		IOWR_32DIRECT(qspi_dev->csr_base,
				ALTERA_QSPI_CONTROLLER2_MEM_OP_REG,
				MEM_OP_LOCK_ALL_SECTORS);
		lock_val = STATUS_PROTECTION_EN_VAL;
	} else {
		IOWR_32DIRECT(qspi_dev->csr_base,
				ALTERA_QSPI_CONTROLLER2_MEM_OP_REG,
				MEM_OP_UNLOCK_ALL_SECTORS);
		lock_val = STATUS_PROTECTION_DIS_VAL;
	}

	/*
	 * poll the status register to know the
	 * completion of the erase operation.
	 */
	timeout = ALTERA_QSPI_CONTROLLER2_1US_TIMEOUT_VALUE;
	while (timeout > 0) {
		/* wait for 1 usec */
		k_busy_wait(1);

		/*
		 * read flash flag status register before
		 * checking the QSPI status
		 */
		IORD_32DIRECT(qspi_dev->csr_base,
				ALTERA_QSPI_CONTROLLER2_FLAG_STATUS_REG);

		/* read QPSI status register */
		status = IORD_32DIRECT(qspi_dev->csr_base,
				ALTERA_QSPI_CONTROLLER2_STATUS_REG);
		if (((status >> STATUS_PROTECTION_POS) &
			STATUS_PROTECTION_MASK) == lock_val) {
			break;
		}

		timeout--;
	}

	if (timeout <= 0) {
		LOG_ERR("locking failed, status-reg 0x%x", status);
		rc = -EIO;
	}

	/* clear flag status register */
	IOWR_32DIRECT(qspi_dev->csr_base,
			ALTERA_QSPI_CONTROLLER2_FLAG_STATUS_REG, 0x0);
	return rc;
}

static const struct flash_parameters *
flash_nios2_qspi_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_nios2_qspi_parameters;
}

static const struct flash_driver_api flash_nios2_qspi_api = {
	.erase = flash_nios2_qspi_erase,
	.write = flash_nios2_qspi_write,
	.read = flash_nios2_qspi_read,
	.get_parameters = flash_nios2_qspi_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = (flash_api_pages_layout)
		       flash_page_layout_not_implemented,
#endif
};

static int flash_nios2_qspi_init(const struct device *dev)
{
	struct flash_nios2_qspi_config *flash_cfg = dev->data;

	k_sem_init(&flash_cfg->sem_lock, 1, 1);
	return 0;
}

struct flash_nios2_qspi_config flash_cfg = {
	.qspi_dev = {
		.data_base = EXT_FLASH_AVL_MEM_BASE,
		.data_end = EXT_FLASH_AVL_MEM_BASE + EXT_FLASH_AVL_MEM_SPAN,
		.csr_base = EXT_FLASH_AVL_CSR_BASE,
		.size_in_bytes = EXT_FLASH_AVL_MEM_SPAN,
		.is_epcs = EXT_FLASH_AVL_MEM_IS_EPCS,
		.number_of_sectors = EXT_FLASH_AVL_MEM_NUMBER_OF_SECTORS,
		.sector_size = EXT_FLASH_AVL_MEM_SECTOR_SIZE,
		.page_size = EXT_FLASH_AVL_MEM_PAGE_SIZE,
	}
};

DEVICE_DEFINE(flash_nios2_qspi,
		CONFIG_SOC_FLASH_NIOS2_QSPI_DEV_NAME,
		flash_nios2_qspi_init, NULL, &flash_cfg, NULL,
		POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY,
		&flash_nios2_qspi_api);

/*
 * Copyright (c) 2020 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This driver emulates an EEPROM device in flash.
 *
 * The emulation represents the EEPROM in flash as a region that is a direct
 * map of the EEPROM data (EEPROM data) followed by a region where changes to
 * the EEPROM data (EEPROM changes) are stored. The combination of EEPROM data
 * and EEPROM changes form a EEPROM page (see drawing below). Changes to EEPROM
 * data are written as address-data combinations. The size of such a combination
 * is determined by the flash write block size and the size of the EEPROM
 * (required address space), with a minimum of 4 byte.
 *
 * When there is no more space to store changes a new EEPROM page is taken into
 * use. This copies the existing data to the EEPROM data area of the new page.
 * During this copying the write that is performed is applied at the same time.
 * The old page is then invalidated.
 *
 * The EEPROM page needs to be a multiple of a flash page size. Multiple EEPROM
 * pages are also supported and increases the number of writes that can be
 * performed.
 *
 * The representation of the EEPROM on flash is shown in the next graph.
 *
 *  |-----------------------------------------------------------------------|
 *  ||----------------------| |----------------------| |-------------------||
 *  || EEPROM data          | |                      | |-Flash page--------||
 *  ||                      | |                      |                      |
 *  || size = EEPROM size   | |                      |                      |
 *  ||----------------------| |----------------------|    ...               |
 *  || EEPROM changes:      | |                      |                      |
 *  || (address, new data)  | |                      |                      |
 *  ||                      | |                      |                      |
 *  ||                    XX| |                    XX|                      |
 *  ||--EEPROM page 0-------| |--EEPROM page 1-------|                      |
 *  |------------------------------------------------------------Partition--|
 *  XX: page validity marker: all 0x00: page invalid
 *
 * Internally the address of an EEPROM byte is represented by a uint32_t (this
 * should be sufficient in all cases). In case the EEPROM size is smaller than
 * 64kB only a uint16_t is used to store changes. In this case the change stored
 * for a 4 byte flash write block size are a combination of 2 byte address and
 * 2 byte data.
 *
 * The EEPROM size, pagesize and the flash partition used for the EEPROM are
 * defined in the dts. The flash partition should allow at least two EEPROM
 * pages.
 *
 */

#define DT_DRV_COMPAT zephyr_emu_eeprom

#define EEPROM_EMU_VERSION 0
#define EEPROM_EMU_MAGIC 0x45454d55 /* EEMU in hex */

#include <zephyr/drivers/eeprom.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/kernel.h>
#define LOG_LEVEL CONFIG_EEPROM_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(eeprom_emulator);

struct eeprom_emu_config {
	/* EEPROM size */
	size_t size;
	/* EEPROM is read-only */
	bool readonly;
	/* Page size used to emulate the EEPROM, contains one area of EEPROM
	 * size and a area to store changes.
	 */
	size_t page_size;
	/* Offset of the flash partition used to emulate the EEPROM */
	off_t flash_offset;
	/* Size of the flash partition to emulate the EEPROM */
	size_t flash_size;
	/* Delay the erase of EEPROM pages until the complete partition is used.
	 */
	bool partitionerase;
	/* Size of a change block */
	uint8_t flash_cbs;
	uint8_t *rambuf;
	/* Device of the flash partition used to emulate the EEPROM */
	const struct device *flash_dev;
};

struct eeprom_emu_data {
	/* Offset in current (EEPROM) page where next change is written */
	off_t write_offset;
	/* Offset of the current (EEPROM) page */
	off_t page_offset;
	struct k_mutex lock;
};

/* read/write context */
struct eeprom_emu_ctx {
	const void *data; /* pointer to data */
	const size_t len; /* data length */
	const off_t address; /* eeprom address */
	size_t rlen; /* data remaining (unprocessed) length */
};

/*
 * basic flash read, only used with offset aligned to flash write block size
 */
static inline int eeprom_emu_flash_read(const struct device *dev, off_t offset,
					uint8_t *blk, size_t len)
{
	const struct eeprom_emu_config *dev_config = dev->config;

	return flash_read(dev_config->flash_dev, dev_config->flash_offset +
			  offset, blk, len);
}

/*
 * basic flash write, only used with offset aligned to flash write block size
 */
static inline int eeprom_emu_flash_write(const struct device *dev, off_t offset,
				   const uint8_t *blk, size_t len)
{
	const struct eeprom_emu_config *dev_config = dev->config;
	int rc;

	rc = flash_write(dev_config->flash_dev, dev_config->flash_offset +
			 offset, blk, len);
	return rc;
}

/*
 * basic flash erase, only used with offset aligned to flash page and len a
 * multiple of the flash page size
 */
static inline int eeprom_emu_flash_erase(const struct device *dev, off_t offset,
				   size_t len)
{
	const struct eeprom_emu_config *dev_config = dev->config;
	int rc;

	rc = flash_erase(dev_config->flash_dev, dev_config->flash_offset +
			 offset, len);
	return rc;
}

/*
 * eeprom_emu_page_invalidate: invalidate a page by writing all zeros at the end
 */
static int eeprom_emu_page_invalidate(const struct device *dev, off_t offset)
{
	const struct eeprom_emu_config *dev_config = dev->config;
	uint8_t buf[dev_config->flash_cbs];

	LOG_DBG("Invalidating page at [0x%tx]", (ptrdiff_t)offset);

	memset(buf, 0x00, sizeof(buf));

	offset += (dev_config->page_size - sizeof(buf));
	return eeprom_emu_flash_write(dev, offset, buf, sizeof(buf));
}

/*
 * eeprom_emu_get_address: read the address from a change block
 */
static uint32_t eeprom_emu_get_address(const struct device *dev,
				       const uint8_t *blk)
{
	const struct eeprom_emu_config *dev_config = dev->config;
	uint32_t address = 0U;

	blk += dev_config->flash_cbs / 2;
	for (int i = 0; i < sizeof(address); i++) {
		if (2 * i == dev_config->flash_cbs) {
			break;
		}

		address += ((uint32_t)(*blk) << (8 * i));
		blk++;
	}

	return address;
}

/*
 * eeprom_emu_set_change: create change blocks from data in blk and address
 */
static void eeprom_emu_set_change(const struct device *dev,
				  const uint32_t address, const uint8_t *data,
				  uint8_t *blk)
{
	const struct eeprom_emu_config *dev_config = dev->config;

	for (int i = 0; i < (dev_config->flash_cbs / 2); i++) {
		(*blk++) = (*data++);
	}

	for (int i = 0; i < (dev_config->flash_cbs / 2); i++) {
		if (i < sizeof(address)) {
			(*blk++) = (uint8_t)(((address >> (8 * i)) & 0xff));
		} else {
			(*blk++) = 0xff;
		}

	}

}

/*
 * eeprom_emu_is_word_used: check if word is not empty
 */
static int eeprom_emu_is_word_used(const struct device *dev, const uint8_t *blk)
{
	const struct eeprom_emu_config *dev_config = dev->config;

	for (int i = 0; i < dev_config->flash_cbs; i++) {
		if ((*blk++) != 0xff) {
			return 1;
		}

	}

	return 0;
}

/*
 * eeprom_emu_word_read: read basic word (cbs byte of data) item from
 * address directly from flash.
 */
static int eeprom_emu_word_read(const struct device *dev, off_t address,
				uint8_t *data)
{
	const struct eeprom_emu_config *dev_config = dev->config;
	const struct eeprom_emu_data *dev_data = dev->data;
	uint8_t buf[dev_config->flash_cbs];
	off_t direct_address;
	int rc;

	direct_address = dev_data->page_offset + address;

	/* Direct flash read */
	rc = eeprom_emu_flash_read(dev, direct_address, data, sizeof(buf));
	if (rc) {
		return rc;
	}

	/* Process changes written to flash */
	off_t offset, ch_address;
	bool mc1 = false, mc2 = false;

	offset = dev_data->write_offset;
	while (((!mc1) || (!mc2)) && (offset > dev_config->size)) {
		offset -= sizeof(buf);
		/* read the change */
		rc = eeprom_emu_flash_read(dev, dev_data->page_offset + offset,
					   buf, sizeof(buf));
		if (rc) {
			return rc;
		}

		/* get the address from a change block */
		ch_address = eeprom_emu_get_address(dev, buf);
		if ((!mc1) && (ch_address == address)) {
			memcpy(data, buf, sizeof(buf)/2);
			mc1 = true;
		}

		if ((!mc2) && (ch_address == (address + sizeof(buf)/2))) {
			memcpy(data + sizeof(buf)/2, buf, sizeof(buf)/2);
			mc2 = true;
		}

	}

	return rc;
}

/* Update data specified in ctx from flash */
static int eeprom_emu_flash_get(const struct device *dev,
				struct eeprom_emu_ctx *ctx)
{
	const struct eeprom_emu_config *dev_config = dev->config;
	off_t address = ctx->address + ctx->len - ctx->rlen;
	uint8_t *data8 = (uint8_t *)(ctx->data);
	uint8_t buf[dev_config->flash_cbs];
	const off_t addr_jmp = address & (sizeof(buf) - 1);
	size_t len;
	int rc;

	data8 += (ctx->len - ctx->rlen);
	len = MIN((sizeof(buf) - addr_jmp), ctx->rlen);
	rc = eeprom_emu_word_read(dev, address - addr_jmp, buf);
	if (rc) {
		return rc;
	}

	memcpy(data8, buf + addr_jmp, len);
	ctx->rlen -= len;

	return rc;
}

/*
 * eeprom_emu_compactor: start a new EEPROM page and copy existing data to the
 * new page. During copy update the data with present write data. Invalidate
 * the old page.
 */
static int eeprom_emu_compactor(const struct device *dev,
				struct eeprom_emu_ctx *ctx)
{
	const struct eeprom_emu_config *dev_config = dev->config;
	struct eeprom_emu_data *dev_data = dev->data;
	off_t next_page_offset;
	int rc = 0;

	LOG_DBG("Compactor called for page at [0x%tx]",
		(ptrdiff_t)dev_data->page_offset);

	next_page_offset = dev_data->page_offset + dev_config->page_size;
	if (next_page_offset >= dev_config->flash_size) {
		next_page_offset = 0;
	}

	if (!dev_config->partitionerase) {
		/* erase the new page */
		rc = eeprom_emu_flash_erase(dev, next_page_offset,
					    dev_config->page_size);
	} else if (next_page_offset == 0) {
		/* erase the entire partition */
		rc = eeprom_emu_flash_erase(dev, next_page_offset,
					    dev_config->flash_size);
	} else {
		rc = 0;
	}

	if (rc) {
		return rc;
	}

	if (dev_config->rambuf && (ctx != NULL)) {
		rc = eeprom_emu_flash_write(dev, next_page_offset,
					    dev_config->rambuf,
					    dev_config->size);
		if (rc) {
			return rc;
		}

		ctx->rlen = 0;
	} else {
		off_t rd_offset = 0;
		uint8_t buf[dev_config->flash_cbs];

		/* reset the context if available */
		if (ctx != NULL) {
			ctx->rlen = ctx->len;
		}

		/* copy existing data */
		while (rd_offset < dev_config->size) {

			rc = eeprom_emu_word_read(dev, rd_offset, buf);
			if (rc) {
				return rc;
			}

			if ((ctx != NULL) && (ctx->len) &&
			    (rd_offset > (ctx->address - sizeof(buf))))  {
				/* overwrite buf data with context data */
				uint8_t *data8 = (uint8_t *)(ctx->data);
				off_t address, addr_jmp;
				size_t len;

				address = ctx->address + ctx->len - ctx->rlen;
				addr_jmp = address & (sizeof(buf) - 1);
				len = MIN((sizeof(buf) - addr_jmp), ctx->rlen);
				data8 += (ctx->len - ctx->rlen);
				memcpy(buf + addr_jmp, data8, len);
				ctx->rlen -= len;
			}

			if (eeprom_emu_is_word_used(dev, buf)) {
				rc = eeprom_emu_flash_write(dev,
							    next_page_offset +
							    rd_offset, buf,
							    sizeof(buf));
				if (rc) {
					return rc;
				}

			}

			rd_offset += sizeof(buf);
		}

	}

	if ((dev_config->partitionerase) && (next_page_offset == 0)) {
		/* no need to invalidate previous page as it has been deleted */
		rc = 0;
	} else {
		/* invalidate the old page */
		rc = eeprom_emu_page_invalidate(dev, dev_data->page_offset);
	}

	if (!rc) {
		dev_data->write_offset = dev_config->size;
		dev_data->page_offset = next_page_offset;
	}
	return rc;
}

/*
 * eeprom_emu_word_write: write basic word (cbs bytes of data) item to address,
 */
static int eeprom_emu_word_write(const struct device *dev, off_t address,
				 const uint8_t *data,
				 struct eeprom_emu_ctx *ctx)
{
	const struct eeprom_emu_config *dev_config = dev->config;
	struct eeprom_emu_data *dev_data = dev->data;
	uint8_t buf[dev_config->flash_cbs], tmp[dev_config->flash_cbs];
	off_t direct_address, wraddr;
	int rc;

	direct_address = dev_data->page_offset + address;

	rc = eeprom_emu_flash_read(dev, direct_address, buf, sizeof(buf));
	if (rc) {
		return rc;
	}

	if (!eeprom_emu_is_word_used(dev, buf)) {
		if (eeprom_emu_is_word_used(dev, data)) {
			rc = eeprom_emu_flash_write(dev, direct_address, data,
						    sizeof(buf));
		}

		return rc;
	}

	rc = eeprom_emu_word_read(dev, address, buf);
	if (rc) {
		return rc;
	}

	if (!memcmp(buf, data, sizeof(buf))) {
		/* data has not changed */
		return rc;
	}

	wraddr = address;
	/* store change */
	for (uint8_t i = 0; i < 2; i++) {
		if (memcmp(&buf[i*sizeof(buf)/2], data, sizeof(buf)/2)) {
			eeprom_emu_set_change(dev, wraddr, data, tmp);
			rc = eeprom_emu_flash_write(dev, dev_data->page_offset +
						    dev_data->write_offset, tmp,
						    sizeof(buf));
			if (rc) {
				return rc;
			}

			dev_data->write_offset += sizeof(buf);
			if ((dev_data->write_offset + sizeof(buf)) >=
			    dev_config->page_size) {
				rc = eeprom_emu_compactor(dev, ctx);
				return rc;

			}

		}

		data += sizeof(buf)/2;
		wraddr += sizeof(buf)/2;
	}

	return rc;
}

/* Update flash with data specified in ctx */
static int eeprom_emu_flash_set(const struct device *dev,
				struct eeprom_emu_ctx *ctx)
{
	const struct eeprom_emu_config *dev_config = dev->config;
	off_t address = ctx->address + ctx->len - ctx->rlen;
	uint8_t *data8 = (uint8_t *)(ctx->data);
	uint8_t buf[dev_config->flash_cbs];
	const off_t addr_jmp = address & (sizeof(buf) - 1);
	size_t len;
	int rc;

	data8 += (ctx->len - ctx->rlen);
	len = MIN((sizeof(buf) - addr_jmp), ctx->rlen);
	rc = eeprom_emu_word_read(dev, address - addr_jmp, buf);
	if (rc) {
		return rc;
	}

	memcpy(buf + addr_jmp, data8, len);
	rc = eeprom_emu_word_write(dev, address - addr_jmp, buf, ctx);
	if (rc) {
		return rc;
	}

	if (ctx->rlen) {
		ctx->rlen -= len;
	}

	return rc;
}

static int eeprom_emu_range_is_valid(const struct device *dev, off_t address,
				     size_t len)
{
	const struct eeprom_emu_config *dev_config = dev->config;

	if ((address + len) <= dev_config->size) {
		return 1;
	}

	return 0;
}

static int eeprom_emu_read(const struct device *dev, off_t address, void *data,
			   size_t len)
{
	const struct eeprom_emu_config *dev_config = dev->config;
	struct eeprom_emu_data *dev_data = dev->data;
	struct eeprom_emu_ctx ctx = {
		.data = data,
		.len = len,
		.address = address,
		.rlen = len,
	};
	int rc = 0;

	/* Nothing to do */
	if (!len) {
		return 0;
	}

	/* Error checking */
	if ((!data) || (!eeprom_emu_range_is_valid(dev, address, len))) {
		return -EINVAL;
	}

	if (!device_is_ready(dev_config->flash_dev)) {
		LOG_ERR("flash device is not ready");
		return -EIO;
	}

	/* Handle normal case */
	LOG_DBG("EEPROM read at [0x%tx] length[%zu]", (ptrdiff_t)address, len);
	k_mutex_lock(&dev_data->lock, K_FOREVER);

	/* read from rambuffer if possible */
	if (dev_config->rambuf) {
		memcpy(data, dev_config->rambuf + address, len);
	} else {
		/* read from flash if no rambuffer */
		while (ctx.rlen) {
			rc = eeprom_emu_flash_get(dev, &ctx);
			if (rc) {
				break;
			}

		}
	}

	k_mutex_unlock(&dev_data->lock);

	return rc;
}

static int eeprom_emu_write(const struct device *dev, off_t address,
			    const void *data, size_t len)
{
	const struct eeprom_emu_config *dev_config = dev->config;
	struct eeprom_emu_data *dev_data = dev->data;
	struct eeprom_emu_ctx ctx = {
		.data = data,
		.len = len,
		.address = address,
		.rlen = len,
	};
	int rc = 0;

	/* Nothing to do */
	if (!len) {
		return 0;
	}

	/* Error checking */
	if ((!data) || (!eeprom_emu_range_is_valid(dev, address, len))) {
		return -EINVAL;
	}

	if (dev_config->readonly) {
		LOG_ERR("attempt to write to read-only device");
		return -EACCES;
	}

	if (!device_is_ready(dev_config->flash_dev)) {
		LOG_ERR("flash device is not ready");
		return -EIO;
	}

	/* Handle normal case */
	LOG_DBG("EEPROM write at [0x%tx] length[%zu]", (ptrdiff_t)address, len);

	k_mutex_lock(&dev_data->lock, K_FOREVER);

	/* first update the rambuffer */
	if (dev_config->rambuf) {
		memcpy(dev_config->rambuf + address, data, len);
	}

	/* second update the flash */
	while (ctx.rlen) {
		rc = eeprom_emu_flash_set(dev, &ctx);
		if (rc) {
			break;
		}

	}

	k_mutex_unlock(&dev_data->lock);

	return rc;
}

static size_t eeprom_emu_size(const struct device *dev)
{
	const struct eeprom_emu_config *dev_config = dev->config;

	return dev_config->size;
}

static int eeprom_emu_init(const struct device *dev)
{
	const struct eeprom_emu_config *dev_config = dev->config;
	struct eeprom_emu_data *dev_data = dev->data;
	off_t offset;
	uint8_t buf[dev_config->flash_cbs];
	int rc;

	k_mutex_init(&dev_data->lock);
	if (!device_is_ready(dev_config->flash_dev)) {
		__ASSERT(0, "Could not get flash device binding");
		return -ENODEV;
	}

	/* Find the page offset */
	dev_data->page_offset = 0U;
	dev_data->write_offset = dev_config->page_size - sizeof(buf);
	while (dev_data->page_offset < dev_config->flash_size) {
		offset = dev_data->page_offset + dev_data->write_offset;
		rc = eeprom_emu_flash_read(dev, offset, buf, sizeof(buf));
		if (rc) {
			return rc;
		}

		if (!eeprom_emu_is_word_used(dev, buf)) {
			break;
		}

		dev_data->page_offset += dev_config->page_size;
	}

	if (dev_data->page_offset == dev_config->flash_size) {
		__ASSERT(0, "All pages are invalid, is this a EEPROM area?");
		return -EINVAL;
	}

	dev_data->write_offset = dev_config->size;

	/* Update the write offset */
	while ((dev_data->write_offset + sizeof(buf)) < dev_config->page_size) {
		offset = dev_data->page_offset + dev_data->write_offset;
		rc = eeprom_emu_flash_read(dev, offset, buf, sizeof(buf));
		if (rc) {
			return rc;
		}

		if (!eeprom_emu_is_word_used(dev, buf)) {
			break;
		}

		dev_data->write_offset += sizeof(buf);
	}

	/* dev_data->write_offset reaches last possible location, compaction
	 * might have been interrupted: call eeprom_emu_compactor again, but
	 * only in case we are using a write-enabled eeprom
	 */
	if ((!dev_config->readonly) &&
	    ((dev_data->write_offset + sizeof(buf)) >= dev_config->page_size)) {
		rc = eeprom_emu_compactor(dev, NULL);
		if (rc) {
			return rc;
		}

	}

	/* Fill the ram buffer if enabled */
	if (dev_config->rambuf) {
		offset = 0;
		while (offset < dev_config->size) {
			rc = eeprom_emu_word_read(dev, offset, buf);
			if (rc) {
				return rc;
			}

			memcpy(dev_config->rambuf + offset, buf, sizeof(buf));
			offset += sizeof(buf);
		}

	}

	return rc;
}

static const struct eeprom_driver_api eeprom_emu_api = {
	.read = eeprom_emu_read,
	.write = eeprom_emu_write,
	.size = eeprom_emu_size,
};

#define EEPROM_PARTITION(n) DT_INST_PHANDLE_BY_IDX(n, partition, 0)

#define PART_WBS(part) \
	DT_PROP(COND_CODE_1(DT_NODE_HAS_COMPAT(DT_GPARENT(part), soc_nv_flash),\
		(DT_GPARENT(part)), (DT_PARENT(part))), write_block_size)

#define PART_CBS(part, size) (PART_WBS(part) < 4) ? \
	((size > (2^16)) ? 8 : 4) : PART_WBS(part)

#define PART_DEV_ID(part) \
	COND_CODE_1(DT_NODE_HAS_COMPAT(DT_GPARENT(part), soc_nv_flash), \
		 (DT_PARENT(DT_GPARENT(part))), (DT_GPARENT(part)))

#define PART_DEV(part) \
	DEVICE_DT_GET(PART_DEV_ID(part))

#define RECALC_SIZE(size, cbs) \
	(size % cbs) ? ((size + cbs - 1) & ~(cbs - 1)) : size

#define ASSERT_SIZE_PAGESIZE_VALID(size, pagesize, readonly) \
	BUILD_ASSERT(readonly ? (size <= pagesize) : (4*size <= 3*pagesize), \
		     "EEPROM size to big for pagesize")

#define ASSERT_PAGESIZE_PARTSIZE_VALID(pagesize, partsize) \
	BUILD_ASSERT(partsize % pagesize == 0U, \
		     "Partition size not a multiple of pagesize")

#define ASSERT_PAGESIZE_SIZE(pagesize, partsize, onepage) \
	BUILD_ASSERT(onepage ? (partsize >= pagesize) : (partsize > pagesize),\
		     "Partition size to small")

#define EEPROM_EMU_READ_ONLY(n) \
	DT_INST_PROP(n, read_only) || \
	DT_PROP(EEPROM_PARTITION(n), read_only)

#define EEPROM_EMU_ONEPAGE(n) \
	EEPROM_EMU_READ_ONLY(n) || DT_INST_PROP(n, partition_erase)

#define EEPROM_EMU_ENABLE_RAMBUF(n) \
	COND_CODE_1(DT_INST_PROP(n, rambuf), (1), \
		(COND_CODE_1(DT_INST_PROP(n, partition_erase), (1), (0))))

#define EEPROM_EMU_RAMBUF(n) \
	COND_CODE_0(EEPROM_EMU_ENABLE_RAMBUF(n), (), \
		(static uint8_t eeprom_emu_##n##_rambuf[DT_INST_PROP(n, size)];))

#define EEPROM_EMU_RAMBUF_LINK(n) \
	COND_CODE_0(EEPROM_EMU_ENABLE_RAMBUF(n), (NULL), \
		(eeprom_emu_##n##_rambuf))

#define EEPROM_EMU_INIT(n) \
	ASSERT_SIZE_PAGESIZE_VALID(DT_INST_PROP(n, size), \
		DT_INST_PROP(n, pagesize), EEPROM_EMU_ONEPAGE(n)); \
	ASSERT_PAGESIZE_PARTSIZE_VALID(DT_INST_PROP(n, pagesize), \
		DT_REG_SIZE(EEPROM_PARTITION(n))); \
	ASSERT_PAGESIZE_SIZE(DT_INST_PROP(n, pagesize), \
		DT_REG_SIZE(EEPROM_PARTITION(n)), EEPROM_EMU_ONEPAGE(n)); \
	EEPROM_EMU_RAMBUF(n) \
	static const struct eeprom_emu_config eeprom_emu_##n##_config = { \
		.size = RECALC_SIZE( \
			DT_INST_PROP(n, size), \
			(PART_CBS(EEPROM_PARTITION(n), DT_INST_PROP(n, size))) \
			), \
		.readonly = EEPROM_EMU_READ_ONLY(n), \
		.page_size = DT_INST_PROP(n, pagesize), \
		.flash_offset = DT_REG_ADDR(EEPROM_PARTITION(n)), \
		.flash_size = DT_REG_SIZE(EEPROM_PARTITION(n)), \
		.partitionerase = DT_INST_PROP(n, partition_erase), \
		.flash_cbs = PART_CBS(EEPROM_PARTITION(n), \
				      DT_INST_PROP(n, size)), \
		.flash_dev = PART_DEV(EEPROM_PARTITION(n)),\
		.rambuf = EEPROM_EMU_RAMBUF_LINK(n), \
	}; \
	static struct eeprom_emu_data eeprom_emu_##n##_data; \
	DEVICE_DT_INST_DEFINE(n, &eeprom_emu_init, \
		NULL, &eeprom_emu_##n##_data, \
		&eeprom_emu_##n##_config, POST_KERNEL, \
		CONFIG_EEPROM_INIT_PRIORITY, &eeprom_emu_api); \

DT_INST_FOREACH_STATUS_OKAY(EEPROM_EMU_INIT)

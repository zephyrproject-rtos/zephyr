/*
 * Copyright (c) 2026 Synaptics Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT jedec_mspi_nand

#include <errno.h>
#include <string.h>

#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/drivers/mspi/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/util.h>

#include "spi_nand.h"

LOG_MODULE_REGISTER(flash_mspi_nand, CONFIG_FLASH_LOG_LEVEL);

struct flash_mspi_nand_config {
	const struct device *bus;
	const struct flash_parameters *parameters;
	struct mspi_dev_id dev_id;
	struct mspi_dev_cfg mspi_cfg;
	uint32_t flash_size;
	uint32_t block_size;
	uint16_t block_erase_us;
	uint16_t page_program_us;
	uint16_t page_read_us;
	uint16_t reset_us;
	uint32_t addr_offset_mask;
	uint8_t addr_page_shift;
	const uint8_t *jedec_id;
	uint8_t jedec_id_len;
	bool jedec_id_specified;
	uint8_t read_opcode;
	uint8_t write_opcode;
	uint8_t read_addr_len;
	uint8_t prog_load_addr_len;
	uint8_t row_addr_len;
	bool read_dummy;
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	struct flash_pages_layout layout;
#endif
};

struct flash_mspi_nand_data {
	struct k_sem sem;
	struct mspi_xfer xfer;
	struct mspi_xfer_packet packet;
};

#define NAND_ACCESS_ADDRESSED BIT(0)
#define NAND_ACCESS_8BIT_ADDR BIT(1)
#define NAND_ACCESS_16BIT_ADDR BIT(2)
#define NAND_ACCESS_24BIT_ADDR BIT(3)
#define NAND_ACCESS_32BIT_ADDR BIT(4)
#define NAND_ACCESS_WRITE BIT(5)
#define NAND_ACCESS_DUMMY_BYTE BIT(6)

#define BAD_BLOCK_MARKER_OFFSET 0x00

static void acquire_device(const struct device *dev)
{
	const struct flash_mspi_nand_config *config = dev->config;
	struct flash_mspi_nand_data *const data = dev->data;

	k_sem_take(&data->sem, K_FOREVER);
	(void)pm_device_runtime_get(config->bus);
    LOG_DBG("Acquired MSPI NAND device %s on bus %s", dev->name, config->bus->name);
	while (mspi_dev_config(config->bus, &config->dev_id, MSPI_DEVICE_CONFIG_ALL,
			      &config->mspi_cfg) == -EBUSY) {
		k_sleep(K_USEC(1));
	}
}

static void release_device(const struct device *dev)
{
	const struct flash_mspi_nand_config *config = dev->config;
	struct flash_mspi_nand_data *const data = dev->data;

	(void)mspi_get_channel_status(config->bus, 0);
	(void)pm_device_runtime_put(config->bus);
	k_sem_give(&data->sem);
}

static uint8_t get_addr_len(unsigned int access)
{
	if ((access & NAND_ACCESS_32BIT_ADDR) != 0U) {
		return 4;
	}
	if ((access & NAND_ACCESS_24BIT_ADDR) != 0U) {
		return 3;
	}
	if ((access & NAND_ACCESS_16BIT_ADDR) != 0U) {
		return 2;
	}
	if ((access & NAND_ACCESS_8BIT_ADDR) != 0U) {
		return 1;
	}

	return 0;
}

static int mspi_nand_access(const struct device *dev, uint8_t opcode, unsigned int access,
			    off_t addr, void *data_buf, size_t length)
{
	struct flash_mspi_nand_data *data = dev->data;
	bool is_write = (access & NAND_ACCESS_WRITE) != 0U;
	uint8_t address_len = 0;

	memset(&data->xfer, 0, sizeof(data->xfer));
	memset(&data->packet, 0, sizeof(data->packet));

	if ((access & NAND_ACCESS_ADDRESSED) != 0U) {
		address_len = get_addr_len(access);
	}

	data->xfer.xfer_mode = MSPI_PIO;
	data->xfer.cmd_length = 1;
	data->xfer.addr_length = address_len;
	data->xfer.num_packet = 1;
	data->xfer.packets = &data->packet;
	data->xfer.rx_dummy = (access & NAND_ACCESS_DUMMY_BYTE) ? 8 : 0;
	data->xfer.tx_dummy = 0;

	data->packet.cmd = opcode;
	data->packet.address = addr;
	data->packet.data_buf = data_buf;
	data->packet.num_bytes = length;
	data->packet.dir = is_write ? MSPI_TX : MSPI_RX;
	LOG_DBG("MSPI NAND access: opcode=0x%02X, access=0x%02X, addr=0x%08X, length=%zu, dir=%s",
		opcode, access, addr, length, is_write ? "TX" : "RX");
	return mspi_transceive(((const struct flash_mspi_nand_config *)dev->config)->bus,
			      &((const struct flash_mspi_nand_config *)dev->config)->dev_id,
			      &data->xfer);
}

#define mspi_nand_cmd_read(dev, opcode, dest, length) \
	mspi_nand_access(dev, opcode, 0, 0, dest, length)

#define mspi_nand_cmd_read_dummy(dev, opcode, dest, length) \
	mspi_nand_access(dev, opcode, NAND_ACCESS_DUMMY_BYTE, 0, dest, length)

#define mspi_nand_cmd_write(dev, opcode) \
	mspi_nand_access(dev, opcode, NAND_ACCESS_WRITE, 0, NULL, 0)

struct spi_nand_feature_frame {
	uint8_t command;
	uint8_t address;
	uint8_t data;
};

static int mspi_nand_feature_op(const struct device *dev, struct spi_nand_feature_frame *to_nand,
				 struct spi_nand_feature_frame *from_nand)
{
	struct flash_mspi_nand_data *data = dev->data;

	memset(&data->xfer, 0, sizeof(data->xfer));
	memset(&data->packet, 0, sizeof(data->packet));

	data->xfer.xfer_mode = MSPI_PIO;
	data->xfer.cmd_length = 1;
	data->xfer.addr_length = 1;
	data->xfer.num_packet = 1;
	data->xfer.packets = &data->packet;

	data->packet.cmd = to_nand->command;
	data->packet.address = to_nand->address;
	data->packet.num_bytes = 1;
	data->packet.data_buf = from_nand ? &from_nand->data : &to_nand->data;
	data->packet.dir = from_nand ? MSPI_RX : MSPI_TX;

	return mspi_transceive(((const struct flash_mspi_nand_config *)dev->config)->bus,
			      &((const struct flash_mspi_nand_config *)dev->config)->dev_id,
			      &data->xfer);
}

static int mspi_nand_get_feature(const struct device *dev, uint8_t reg, uint8_t *feature)
{
	struct spi_nand_feature_frame out = {
		.command = SPI_NAND_CMD_GET_FEATURE,
		.address = reg,
	};
	struct spi_nand_feature_frame in;
	int ret;

	ret = mspi_nand_feature_op(dev, &out, &in);
	*feature = in.data;
	return ret;
}

static int mspi_nand_set_feature(const struct device *dev, uint8_t reg, uint8_t feature)
{
	struct spi_nand_feature_frame out = {
		.command = SPI_NAND_CMD_SET_FEATURE,
		.address = reg,
		.data = feature,
	};

	return mspi_nand_feature_op(dev, &out, NULL);
}

static int mspi_nand_wait_until_ready(const struct device *dev, const char *op,
				      uint32_t timeout_us, uint32_t poll_us, uint8_t *status)
{
	k_ticks_t t = k_uptime_ticks();
	k_timepoint_t timeout;
	int ret;

	timeout = sys_timepoint_calc(K_USEC(timeout_us));
	do {
		ret = mspi_nand_get_feature(dev, SPI_NAND_FEATURE_ADDR_STATUS, status);
		if (ret != 0) {
			LOG_ERR("Status read failed during %s: %d", op, ret);
			return ret;
		}

		if ((*status & SPI_NAND_FEATURE_STATUS_OIP) == 0U) {
			t = k_uptime_ticks() - t;
			LOG_DBG("Ready after %u us (Op %s, Status %02X)", k_ticks_to_us_near32(t),
				op, *status);
			return ret;
		}
		LOG_DBG("Waiting for %s: status=0x%02X, OIP still set", op, *status);
		k_sleep(K_USEC(poll_us));
	} while (!sys_timepoint_expired(timeout));

	LOG_ERR("Ready timeout (Op %s, Status %02X, timeout_us=%u, poll_us=%u)",
		op, *status, timeout_us, poll_us);
	return -ETIMEDOUT;
}

static int mspi_nand_page_read_to_cache(const struct device *dev, uint32_t page)
{
const struct flash_mspi_nand_config *config = dev->config;
uint8_t status;
uint8_t ecc_status;
unsigned int access = NAND_ACCESS_ADDRESSED;
int ret;

LOG_DBG("Page read to cache: page=0x%08x", page);

switch (config->row_addr_len) {
case 1:
access |= NAND_ACCESS_8BIT_ADDR;
break;
case 2:
access |= NAND_ACCESS_16BIT_ADDR;
break;
case 4:
access |= NAND_ACCESS_32BIT_ADDR;
break;
case 3:
default:
access |= NAND_ACCESS_24BIT_ADDR;
break;
}

ret = mspi_nand_access(dev, SPI_NAND_CMD_PAGE_READ, access, page, NULL, 0);
if (ret != 0) {
LOG_ERR("PAGE_READ failed for page 0x%08x: %d", page, ret);
return ret;
}

ret = mspi_nand_wait_until_ready(dev, "read", config->page_read_us, 0, &status);
if (ret != 0) {
LOG_ERR("Page read ready wait failed for page 0x%08x: %d", page, ret);
return ret;
}

ecc_status = status & SPI_NAND_FEATURE_ECC_MASK;
switch (ecc_status) {
case SPI_NAND_FEATURE_ECC_ERROR_NOT_CORRECTED:
LOG_WRN("ECC uncorrectable error on page %06x", page);
return -EBADMSG;
case SPI_NAND_FEATURE_ECC_ERROR_CORRECTED_REFRESH:
case SPI_NAND_FEATURE_ECC_ERROR_CORRECTED:
LOG_DBG("ECC errors corrected on page %06x", page);
break;
case SPI_NAND_FEATURE_ECC_NO_ERRORS:
break;
default:
__ASSERT(false, "Unreachable");
}

return 0;
}

static int mspi_nand_read_from_cache(const struct device *dev, uint16_t offset, void *dest,
				     size_t size)
{
	const struct flash_mspi_nand_config *config = dev->config;
	unsigned int access = NAND_ACCESS_ADDRESSED;
	int ret;

	switch (config->read_addr_len) {
	case 1:
		access |= NAND_ACCESS_8BIT_ADDR;
		break;
	case 3:
		access |= NAND_ACCESS_24BIT_ADDR;
		break;
	case 4:
		access |= NAND_ACCESS_32BIT_ADDR;
		break;
	case 2:
	default:
		access |= NAND_ACCESS_16BIT_ADDR;
		break;
	}

	if (config->read_dummy) {
		access |= NAND_ACCESS_DUMMY_BYTE;
	}

	LOG_DBG("Read cache: opcode=0x%02X, access=0x%02X, col=0x%04X, size=%zu, dummy=%d",
		config->read_opcode, access, offset, size, config->read_dummy);
	ret = mspi_nand_access(dev, config->read_opcode, access, offset, dest, size);
	if (ret == 0 && size > 0) {
		LOG_HEXDUMP_DBG(dest, MIN(size, 16U), "Read cache data");
	}
	return ret;
}

static bool valid_region(const struct device *dev, off_t addr, size_t size)
{
	const struct flash_mspi_nand_config *config = dev->config;

	if ((addr < 0) || (addr >= config->flash_size) || (size > config->flash_size) ||
	    ((config->flash_size - addr) < size)) {
		return false;
	}

	return true;
}

static int api_read(const struct device *dev, off_t addr, void *dest, size_t size)
{
	const struct flash_mspi_nand_config *config = dev->config;
	uint8_t *dest_u8 = dest;
	uint32_t page_address;
	uint16_t page_offset;
	uint16_t bytes_to_end;
	uint16_t bytes_to_read;
	int ret = 0;
    LOG_DBG("Reading %zu bytes from address 0x%08X", size, addr);
	if (size == 0) {
		return 0;
	}

	if (!valid_region(dev, addr, size)) {
		return -EINVAL;
	}

	acquire_device(dev);

	while (size > 0) {
		page_address = addr >> config->addr_page_shift;
		page_offset = addr & config->addr_offset_mask;
		bytes_to_end = config->parameters->write_block_size - page_offset;
		bytes_to_read = MIN(size, bytes_to_end);

		ret = mspi_nand_page_read_to_cache(dev, page_address);
		if (ret != 0) {
			break;
		}

		ret = mspi_nand_read_from_cache(dev, page_offset, dest_u8, bytes_to_read);
		if (ret != 0) {
			break;
		}

		dest_u8 += bytes_to_read;
		addr += bytes_to_read;
		size -= bytes_to_read;
	}

	release_device(dev);
	return ret;
}

static int api_write(const struct device *dev, off_t addr, const void *src, size_t size)
{
	const struct flash_mspi_nand_config *config = dev->config;
	uint32_t write_block = config->parameters->write_block_size;
	uint8_t *src_u8 = (void *)src;
	uint32_t page_address;
	uint8_t status;
	unsigned int access = NAND_ACCESS_WRITE | NAND_ACCESS_ADDRESSED;
	int ret = 0;
	LOG_DBG("Writing %zu bytes to address 0x%08X", size, addr);
	if (size == 0) {
		return 0;
	}

	if (!valid_region(dev, addr, size) || (addr % write_block) || (size % write_block)) {
		return -EINVAL;
	}

	switch (config->prog_load_addr_len) {
	case 1:
		access |= NAND_ACCESS_8BIT_ADDR;
		break;
	case 3:
		access |= NAND_ACCESS_24BIT_ADDR;
		break;
	case 4:
		access |= NAND_ACCESS_32BIT_ADDR;
		break;
	case 2:
	default:
		access |= NAND_ACCESS_16BIT_ADDR;
		break;
	}

	acquire_device(dev);

	while (size > 0) {
		ret = mspi_nand_cmd_write(dev, SPI_NAND_CMD_WRITE_ENABLE);
		if (ret != 0) {
			break;
		}

		page_address = addr >> config->addr_page_shift;

		ret = mspi_nand_access(dev, config->write_opcode, access, 0, src_u8, write_block);
		if (ret != 0) {
			break;
		}

		ret = mspi_nand_access(dev, SPI_NAND_CMD_PROGRAM_EXECUTE,
				      NAND_ACCESS_WRITE | NAND_ACCESS_ADDRESSED |
				      (config->row_addr_len == 4 ? NAND_ACCESS_32BIT_ADDR :
				       config->row_addr_len == 2 ? NAND_ACCESS_16BIT_ADDR :
				       config->row_addr_len == 1 ? NAND_ACCESS_8BIT_ADDR :
				       NAND_ACCESS_24BIT_ADDR),
				      page_address, NULL, 0);
		if (ret != 0) {
			break;
		}

		ret = mspi_nand_wait_until_ready(dev, "write", config->page_program_us, 100,
						&status);
		if (ret != 0) {
			break;
		}
		if (status & SPI_NAND_FEATURE_STATUS_PROGRAM_FAIL) {
			ret = -EIO;
			break;
		}

		src_u8 += write_block;
		size -= write_block;
		addr += write_block;
	}

	release_device(dev);
	return ret;
}

static int api_erase(const struct device *dev, off_t addr, size_t size)
{
	const struct flash_mspi_nand_config *config = dev->config;
	uint32_t page_address;
	uint8_t status;
	int ret = 0;

	LOG_DBG("Erase request: addr=0x%08lx, size=%zu, block_size=%u, flash_size=%u, row_addr_len=%u",
		(long)addr, size, config->block_size, config->flash_size, config->row_addr_len);
	if (size == 0) {
		LOG_DBG("Erase request size is zero, skipping");
		return 0;
	}

	if (!valid_region(dev, addr, size)) {
		LOG_ERR("Erase invalid region: addr=0x%08lx, size=%zu, flash_size=%u",
			(long)addr, size, config->flash_size);
		return -EINVAL;
	}
	if ((addr % config->block_size) != 0U) {
		LOG_ERR("Erase addr not block-aligned: addr=0x%08lx, block_size=%u",
			(long)addr, config->block_size);
		return -EINVAL;
	}
	if ((size % config->block_size) != 0U) {
		LOG_ERR("Erase size not block-multiple: size=%zu, block_size=%u",
			size, config->block_size);
		return -EINVAL;
	}

	acquire_device(dev);

	while (size > 0) {
		page_address = addr >> config->addr_page_shift;
		LOG_DBG("Erasing block at addr=0x%08lx, page_address=0x%08x, remaining=%zu",
			(long)addr, page_address, size);

		ret = mspi_nand_cmd_write(dev, SPI_NAND_CMD_WRITE_ENABLE);
		if (ret != 0) {
			LOG_ERR("WRITE_ENABLE failed for erase at 0x%08lx: %d", (long)addr, ret);
			break;
		}

		ret = mspi_nand_access(dev, SPI_NAND_CMD_BLOCK_ERASE,
				      NAND_ACCESS_ADDRESSED |
				      (config->row_addr_len == 4 ? NAND_ACCESS_32BIT_ADDR :
				       config->row_addr_len == 2 ? NAND_ACCESS_16BIT_ADDR :
				       config->row_addr_len == 1 ? NAND_ACCESS_8BIT_ADDR :
				       NAND_ACCESS_24BIT_ADDR),
				      page_address, NULL, 0);
		if (ret != 0) {
			LOG_ERR("BLOCK_ERASE command failed at page 0x%08x: %d", page_address, ret);
			break;
		}

		ret = mspi_nand_wait_until_ready(dev, "erase", config->block_erase_us, 500,
						&status);
		if (ret != 0) {
			LOG_ERR("Erase timeout / ready wait failed at page 0x%08x: %d",
				page_address, ret);
			break;
		}
		LOG_DBG("Erase status after wait: status=0x%02X at page 0x%08x", status, page_address);
		if (status & SPI_NAND_FEATURE_STATUS_ERASE_FAIL) {
			LOG_ERR("Erase fail bit set: status=0x%02X at page 0x%08x", status, page_address);
			ret = -EIO;
			break;
		}

		addr += config->block_size;
		size -= config->block_size;
	}

	release_device(dev);
	LOG_DBG("Erase complete with ret=%d, final_addr=0x%08lx, remaining=%zu",
		ret, (long)addr, size);
	return ret;
}

static int mspi_nand_reset(const struct device *dev)
{
	const struct flash_mspi_nand_config *config = dev->config;
	uint8_t status;
	int ret;

	ret = mspi_nand_cmd_write(dev, SPI_NAND_CMD_RESET);
	if (ret != 0) {
		return ret;
	}

	return mspi_nand_wait_until_ready(dev, "reset", config->reset_us, 100, &status);
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void api_page_layout(const struct device *dev,
			    const struct flash_pages_layout **layout, size_t *layout_size)
{
	const struct flash_mspi_nand_config *config = dev->config;

	*layout = &config->layout;
	*layout_size = 1;
}
#endif

#if defined(CONFIG_FLASH_EX_OP_ENABLED)
static int api_is_bad_block(const struct device *dev, off_t addr,
			    enum flash_block_status *status)
{
	const struct flash_mspi_nand_config *config = dev->config;
	const uint32_t bad_block_marker_offset =
		config->parameters->write_block_size + BAD_BLOCK_MARKER_OFFSET;
	uint32_t page_address;
	uint8_t bad_block_marker;
	int ret;

	if (!valid_region(dev, addr, 1) || (addr % config->block_size)) {
		return -EINVAL;
	}

	page_address = addr >> config->addr_page_shift;
	ret = mspi_nand_page_read_to_cache(dev, page_address);
	if ((ret != 0) && (ret != -EBADMSG)) {
		return ret;
	}

	ret = mspi_nand_read_from_cache(dev, bad_block_marker_offset, &bad_block_marker,
					 sizeof(bad_block_marker));
	if (ret != 0) {
		return ret;
	}

	*status = (bad_block_marker != 0xff) ? FLASH_BLOCK_BAD : FLASH_BLOCK_GOOD;
	return 0;
}

static int api_mark_bad_block(const struct device *dev, off_t addr)
{
	const struct flash_mspi_nand_config *config = dev->config;
	const uint32_t bad_block_marker_offset =
		config->parameters->write_block_size + BAD_BLOCK_MARKER_OFFSET;
	uint8_t bad_block_marker = 0x00;
	uint32_t page_address;
	uint8_t status;
	int ret;

	if (!valid_region(dev, addr, 1) || (addr % config->block_size)) {
		return -EINVAL;
	}

	page_address = addr >> config->addr_page_shift;
	ret = mspi_nand_cmd_write(dev, SPI_NAND_CMD_WRITE_ENABLE);
	if (ret != 0) {
		return ret;
	}

	ret = mspi_nand_access(dev, config->write_opcode,
			      NAND_ACCESS_WRITE | NAND_ACCESS_ADDRESSED |
			      (config->prog_load_addr_len == 4 ? NAND_ACCESS_32BIT_ADDR :
			       config->prog_load_addr_len == 3 ? NAND_ACCESS_24BIT_ADDR :
			       config->prog_load_addr_len == 1 ? NAND_ACCESS_8BIT_ADDR :
			       NAND_ACCESS_16BIT_ADDR),
			      bad_block_marker_offset, &bad_block_marker,
			      sizeof(bad_block_marker));
	if (ret != 0) {
		return ret;
	}

	ret = mspi_nand_access(dev, SPI_NAND_CMD_PROGRAM_EXECUTE,
			      NAND_ACCESS_WRITE | NAND_ACCESS_ADDRESSED |
			      (config->row_addr_len == 4 ? NAND_ACCESS_32BIT_ADDR :
			       config->row_addr_len == 2 ? NAND_ACCESS_16BIT_ADDR :
			       config->row_addr_len == 1 ? NAND_ACCESS_8BIT_ADDR :
			       NAND_ACCESS_24BIT_ADDR),
			      page_address, NULL, 0);
	if (ret != 0) {
		return ret;
	}

	ret = mspi_nand_wait_until_ready(dev, "write", config->page_program_us, 100, &status);
	if (ret != 0) {
		return ret;
	}

	return (status & SPI_NAND_FEATURE_STATUS_PROGRAM_FAIL) ? -EIO : 0;
}

static int api_ex_op(const struct device *dev, uint16_t code, const uintptr_t in, void *out)
{
	int ret;

	acquire_device(dev);

	switch (code) {
	case FLASH_EX_OP_RESET:
		ret = mspi_nand_reset(dev);
		break;
	case FLASH_EX_OP_IS_BAD_BLOCK:
		ret = api_is_bad_block(dev, *(const off_t *)in, (enum flash_block_status *)out);
		break;
	case FLASH_EX_OP_MARK_BAD_BLOCK:
		ret = api_mark_bad_block(dev, *(const off_t *)in);
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	release_device(dev);
	return ret;
}
#endif

static const struct flash_parameters *api_get_parameters(const struct device *dev)
{
	return ((const struct flash_mspi_nand_config *)dev->config)->parameters;
}

static int api_get_size(const struct device *dev, uint64_t *size)
{
	*size = ((const struct flash_mspi_nand_config *)dev->config)->flash_size;
	return 0;
}

static int onfi_parameters_load(const struct device *dev)
{
	const struct flash_mspi_nand_config *config = dev->config;
	struct spi_nand_onfi_parameter_page onfi;
	uint16_t computed_crc = 0;
	uint32_t total_size;
	uint32_t block_size;
	uint32_t page_size;
	uint8_t cfg;
	int ret;

	LOG_DBG("Reading ONFI parameter page from %s", dev->name);

	ret = mspi_nand_get_feature(dev, SPI_NAND_FEATURE_ADDR_CONFIG, &cfg);
	if (ret != 0) {
		LOG_ERR("Failed to read feature register 0x%02X: %d",
			SPI_NAND_FEATURE_ADDR_CONFIG, ret);
		return ret;
	}
	cfg |= SPI_NAND_FEATURE_CONFIG_OTP_EN;
	LOG_DBG("Enabling OTP access for ONFI read (config=0x%02X)", cfg);
	ret = mspi_nand_set_feature(dev, SPI_NAND_FEATURE_ADDR_CONFIG, cfg);
	if (ret != 0) {
		LOG_ERR("Failed to enable OTP access: %d", ret);
		return ret;
	}

	ret = mspi_nand_get_feature(dev, SPI_NAND_FEATURE_ADDR_CONFIG, &cfg);
	if (ret != 0) {
		LOG_ERR("Failed to re-read feature register 0x%02X: %d",
			SPI_NAND_FEATURE_ADDR_CONFIG, ret);
		return ret;
	}
	if (!(cfg & SPI_NAND_FEATURE_CONFIG_OTP_EN)) {
		LOG_ERR("OTP access did not enable on NAND %s", dev->name);
		return -EIO;
	}

	ret = mspi_nand_page_read_to_cache(dev, 1);
	if ((ret != 0) && (ret != -EBADMSG)) {
		LOG_ERR("Page-read-to-cache for ONFI page failed: %d", ret);
		return ret;
	}
	LOG_DBG("Fetched ONFI page at page 1, reading param bytes");

	onfi.integrity_crc = 1;
	page_size = config->parameters->write_block_size;
	for (int i = 0; i < page_size; i += sizeof(onfi)) {
		ret = mspi_nand_read_from_cache(dev, i, &onfi, sizeof(onfi));
		if (ret != 0) {
			LOG_ERR("Failed to read ONFI bytes at offset %d: %d", i, ret);
			return ret;
		}
		computed_crc = crc16(CRC16_POLY, 0x4F4E, (void *)&onfi,
				     sizeof(onfi) - sizeof(uint16_t));
		if (computed_crc == onfi.integrity_crc) {
			break;
		}
	}
	if (computed_crc != onfi.integrity_crc) {
		LOG_ERR("ONFI CRC mismatch: computed=0x%04X, stored=0x%04X",
			computed_crc, onfi.integrity_crc);
		return -ENOSPC;
	}

	block_size = onfi.data_bytes_per_page * onfi.pages_per_block;
	total_size = block_size * onfi.blocks_per_lun * onfi.num_lun;
	LOG_DBG("ONFI info: page=%u bytes, pages/block=%u, blocks/lun=%u, lun=%u, total=%u bytes",
		onfi.data_bytes_per_page, onfi.pages_per_block, onfi.blocks_per_lun,
		onfi.num_lun, total_size);
	if (onfi.data_bytes_per_page != config->parameters->write_block_size) {
		LOG_WRN("Devicetree page size does not match ONFI page size (%d != %d)",
			onfi.data_bytes_per_page, config->parameters->write_block_size);
	}
	if (block_size != config->block_size) {
		LOG_WRN("Devicetree block size does not match ONFI block size (%d != %d)",
			block_size, config->block_size);
	}
	if (total_size != config->flash_size) {
		LOG_WRN("Devicetree total size does not match ONFI total size (%d != %d)",
			total_size, config->flash_size);
	}

	cfg &= ~SPI_NAND_FEATURE_CONFIG_OTP_EN;
	LOG_DBG("Disabling OTP access after ONFI read");
	return mspi_nand_set_feature(dev, SPI_NAND_FEATURE_ADDR_CONFIG, cfg);
}

static int flash_chip_configure(const struct device *dev)
{
	const struct flash_mspi_nand_config *config = dev->config;
	uint8_t jedec_id[SPI_NAND_MAX_ID_LEN];
	int ret;

	LOG_DBG("Configuring MSPI NAND chip %s", dev->name);
	if (!device_is_ready(config->bus)) {
		LOG_ERR("MSPI bus %s is not ready for NAND %s", config->bus->name, dev->name);
		return -ENODEV;
	}

	acquire_device(dev);

	LOG_DBG("Sending NAND reset command to %s", dev->name);
	ret = mspi_nand_reset(dev);
	if (ret != 0) {
		LOG_ERR("NAND reset failed for %s: %d", dev->name, ret);
		release_device(dev);
		return ret;
	}
	LOG_DBG("NAND reset complete for %s", dev->name);

	if (config->jedec_id_len > 0U) {
		LOG_DBG("Reading JEDEC ID from NAND %s (len=%u)", dev->name, config->jedec_id_len);
		ret = mspi_nand_cmd_read_dummy(dev, SPI_NAND_CMD_READ_ID, jedec_id,
					       config->jedec_id_len);
		if (ret == 0) {
			LOG_DBG("RAW JEDEC ID bytes for %s: %02X %02X %02X %02X %02X %02X",
				dev->name,
				jedec_id[0], jedec_id[1], jedec_id[2], jedec_id[3],
				jedec_id[4], jedec_id[5]);
			LOG_HEXDUMP_INF(jedec_id, config->jedec_id_len, "JEDEC ID");
		} else {
			LOG_ERR("READ_ID failed for %s: %d", dev->name, ret);
		}
	}
	if ((ret == 0) && config->jedec_id_specified &&
	    memcmp(jedec_id, config->jedec_id, config->jedec_id_len) != 0) {
		LOG_HEXDUMP_ERR(config->jedec_id, config->jedec_id_len, "Expected JEDEC ID");
		LOG_HEXDUMP_ERR(jedec_id, config->jedec_id_len, "Queried JEDEC ID");
		ret = -EINVAL;
	}
	if (ret == 0) {
		LOG_DBG("Loading ONFI parameter page for %s", dev->name);
		ret = onfi_parameters_load(dev);
		if (ret == 0) {
			LOG_DBG("ONFI parameter page loaded successfully for %s", dev->name);
		} else {
			LOG_ERR("ONFI parameter page load failed for %s: %d", dev->name, ret);
		}
	}
	if (ret == 0) {
		LOG_DBG("Disabling block protection for %s", dev->name);
		ret = mspi_nand_set_feature(dev, SPI_NAND_FEATURE_ADDR_BLOCK_PROT,
					    SPI_NAND_FEATURE_BLOCK_PROT_DISABLE_ALL);
		if (ret == 0) {
			LOG_DBG("Block protection disabled for %s", dev->name);
		} else {
			LOG_ERR("Block protection disable failed for %s: %d", dev->name, ret);
		}
	}

	release_device(dev);
	LOG_DBG("NAND chip configure done for %s, rc=%d", dev->name, ret);
	return ret;
}

static int pm_action_cb(const struct device *dev, enum pm_device_action action)
{
    LOG_DBG("PM action %d for MSPI NAND device %s", action, dev->name);
	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
	case PM_DEVICE_ACTION_RESUME:
		return 0;
	case PM_DEVICE_ACTION_TURN_ON:
		return flash_chip_configure(dev);
	case PM_DEVICE_ACTION_TURN_OFF:
		return 0;
	default:
		return -ENOSYS;
	}
}

static int drv_init(const struct device *dev)
{
	struct flash_mspi_nand_data *data = dev->data;

	k_sem_init(&data->sem, 1, K_SEM_MAX_LIMIT);
    LOG_DBG("Initializing MSPI NAND device %s", dev->name);
	return pm_device_driver_init(dev, pm_action_cb);
	return 0;
}

static DEVICE_API(flash, drv_api) = {
	.read = api_read,
	.write = api_write,
	.erase = api_erase,
	.get_parameters = api_get_parameters,
	.get_size = api_get_size,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = api_page_layout,
#endif
#if defined(CONFIG_FLASH_EX_OP_ENABLED)
	.ex_op = api_ex_op,
#endif
};

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
#define DEFINE_PAGE_LAYOUT(inst) \
	.layout = { \
		.pages_count = DT_INST_PROP(inst, size_bytes) / DT_INST_PROP_OR(inst, erase_block_size, 131072), \
		.pages_size = DT_INST_PROP_OR(inst, erase_block_size, 131072), \
	},
#else
#define DEFINE_PAGE_LAYOUT(inst)
#endif

#define FLASH_MSPI_NAND_ADDR_PROP_TO_MASK(node_id, prop) \
	(DT_PROP(node_id, prop) == 4 ? NAND_ACCESS_32BIT_ADDR : \
	 DT_PROP(node_id, prop) == 3 ? NAND_ACCESS_24BIT_ADDR : \
	 DT_PROP(node_id, prop) == 1 ? NAND_ACCESS_8BIT_ADDR : NAND_ACCESS_16BIT_ADDR)

/* MSPI bus must be initialized before this device. */
#if (CONFIG_MSPI_INIT_PRIORITY < CONFIG_FLASH_INIT_PRIORITY)
#define FLASH_MSPI_NAND_INIT_PRIORITY CONFIG_FLASH_INIT_PRIORITY
#else
#define FLASH_MSPI_NAND_INIT_PRIORITY UTIL_INC(CONFIG_MSPI_INIT_PRIORITY)
#endif

#define FLASH_MSPI_NAND_INST(inst) \
	BUILD_ASSERT(IS_POWER_OF_TWO(DT_INST_PROP_OR(inst, write_block_size, 2048)), \
		"write-block-size must be a power of 2"); \
	BUILD_ASSERT(IS_POWER_OF_TWO(DT_INST_PROP_OR(inst, erase_block_size, 131072)), \
		"erase-block-size must be a power of 2"); \
	BUILD_ASSERT(DT_INST_PROP_OR(inst, erase_block_size, 131072) % \
		DT_INST_PROP_OR(inst, write_block_size, 2048) == 0, \
		"erase-block-size must be a multiple of write-block-size"); \
	BUILD_ASSERT(DT_INST_PROP(inst, size_bytes) % DT_INST_PROP_OR(inst, erase_block_size, 131072) == 0, \
		"size-bytes must be a multiple of erase-block-size"); \
	static const struct flash_parameters flash_mspi_nand_##inst##_parameters = { \
		.write_block_size = DT_INST_PROP_OR(inst, write_block_size, 2048), \
		.erase_value = 0xff, \
	}; \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, jedec_id), \
		(static const uint8_t flash_mspi_nand_##inst##_jedec_id[] = DT_INST_PROP(inst, jedec_id);), \
		(static const uint8_t flash_mspi_nand_##inst##_jedec_id[] = { 0 };)) \
	BUILD_ASSERT(!DT_INST_NODE_HAS_PROP(inst, jedec_id) || \
		ARRAY_SIZE(flash_mspi_nand_##inst##_jedec_id) <= SPI_NAND_MAX_ID_LEN); \
	static const struct flash_mspi_nand_config flash_mspi_nand_##inst##_config = { \
		.bus = DEVICE_DT_GET(DT_INST_BUS(inst)), \
		.parameters = &flash_mspi_nand_##inst##_parameters, \
		.dev_id = MSPI_DEVICE_ID_DT_INST(inst), \
		.mspi_cfg = MSPI_DEVICE_CONFIG_DT_INST(inst), \
		.flash_size = DT_INST_PROP(inst, size_bytes), \
		.block_size = DT_INST_PROP_OR(inst, erase_block_size, 131072), \
		.block_erase_us = DT_INST_PROP_OR(inst, block_erase_duration_max, 30000), \
		.page_program_us = DT_INST_PROP_OR(inst, page_program_duration_max, 40000), \
		.page_read_us = DT_INST_PROP_OR(inst, page_read_duration_max, 10000), \
		.reset_us = DT_INST_PROP_OR(inst, reset_duration_max, 20000), \
		.addr_offset_mask = DT_INST_PROP_OR(inst, write_block_size, 2048) - 1, \
		.addr_page_shift = LOG2(DT_INST_PROP_OR(inst, write_block_size, 2048)), \
		.jedec_id = flash_mspi_nand_##inst##_jedec_id, \
		.jedec_id_len = DT_INST_NODE_HAS_PROP(inst, jedec_id) ? \
			ARRAY_SIZE(flash_mspi_nand_##inst##_jedec_id) : 3, \
		.jedec_id_specified = DT_INST_NODE_HAS_PROP(inst, jedec_id), \
		.read_opcode = DT_INST_PROP(inst, read_command), \
		.write_opcode = DT_INST_PROP(inst, write_command), \
		.read_addr_len = DT_INST_PROP(inst, read_address_length), \
		.prog_load_addr_len = DT_INST_PROP(inst, program_load_address_length), \
		.row_addr_len = DT_INST_PROP(inst, row_address_length), \
		.read_dummy = DT_INST_NODE_HAS_PROP(inst, requires_read_dummy), \
		DEFINE_PAGE_LAYOUT(inst) \
	}; \
	static struct flash_mspi_nand_data flash_mspi_nand_##inst##_data; \
	PM_DEVICE_DT_INST_DEFINE(inst, pm_action_cb); \
	DEVICE_DT_INST_DEFINE(inst, drv_init, PM_DEVICE_DT_INST_GET(inst), \
			      &flash_mspi_nand_##inst##_data, &flash_mspi_nand_##inst##_config, \
			      POST_KERNEL, FLASH_MSPI_NAND_INIT_PRIORITY, &drv_api);

DT_INST_FOREACH_STATUS_OKAY(FLASH_MSPI_NAND_INST)
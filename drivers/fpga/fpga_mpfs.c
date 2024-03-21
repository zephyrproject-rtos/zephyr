/*
 * Copyright (c) 2024 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mpfs_mailbox
#include <stdio.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/fpga.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>
LOG_MODULE_REGISTER(fpga_mpfs);

#define SPI_FLASH_DIRECTORY_OFFSET    0x00000000
#define SPI_FLASH_GOLDEN_IMAGE_OFFSET 0x00100400
#define SPI_FLASH_NEW_IMAGE_OFFSET    0x01500400
#define SPI_FLASH_SECTOR_SIZE         4096
#define SPI_FLASH_PAGE_SIZE           256

#define SERVICES_CR_OFFSET 0x50u
#define SERVICES_SR_OFFSET 0x54u

#define SCBCTRL_SERVICESCR_REQ      (0u)
#define SCBCTRL_SERVICESCR_REQ_MASK BIT(SCBCTRL_SERVICESCR_REQ)

#define SCBCTRL_SERVICESSR_BUSY      (1u)
#define SCBCTRL_SERVICESSR_BUSY_MASK BIT(SCBCTRL_SERVICESSR_BUSY)

#define SCBCTRL_SERVICESSR_STATUS            (16u)
#define SCBCTRL_SERVICESSR_STATUS_MASK_WIDTH (16u)
#define SCBCTRL_SERVICESSR_STATUS_MASK                                                             \
	GENMASK(SCBCTRL_SERVICESSR_STATUS + SCBCTRL_SERVICESSR_STATUS_MASK_WIDTH - 1,              \
		SCBCTRL_SERVICESSR_STATUS)

#define MSS_DESIGN_INFO_CMD                (0x02)
#define MSS_SYS_BITSTREAM_AUTHENTICATE_CMD 0x23u
#define MSS_SYS_IAP_PROGRAM_BY_SPIIDX_CMD  0x42u

struct mpfs_fpga_config {
	mm_reg_t base;
	mm_reg_t mailbox;
};

struct mpfs_fpga_data {
	char FPGA_design_ver[30];
};

static inline uint32_t scb_read(mm_reg_t add, mm_reg_t offset)
{
	return sys_read32(add + offset);
}

static inline void scb_write(mm_reg_t add, mm_reg_t offset, uint32_t val)
{
	return sys_write32(val, add + offset);
}

/*This function add the index of new image into the spi directory at offset 0x004.
 * Note: In the Flash directory first four pages(each page of 256 Bytes) have either
 * a valid image address or zeros. The other remaining 12 pages are all filled with 0xFFs.
 *
 * |------------------------------| 0x000
 * | Golden Image Address:        |
 * | 0x0100400                    |
 * |------------------------------| 0x004
 * | Update Image Address         |
 * | 0x1500400                    |
 * |------------------------------| 0x008
 * | Empty                        |
 * | 0x000000                     |
 * |------------------------------| 0x00C
 * | Unused for re-programming    |
 * |                              |
 * |------------------------------| 0x400
 */
static uint8_t update_spi_flash_directory(const struct device *flash_dev)
{
	size_t len = SPI_FLASH_PAGE_SIZE;
	uint8_t buf[SPI_FLASH_PAGE_SIZE];
	uint8_t rc, k;

	memset(buf, 0, len);

	rc = flash_read(flash_dev, SPI_FLASH_DIRECTORY_OFFSET, buf, len);
	if (rc != 0) {
		LOG_ERR("Flash read failed! %d", rc);
		return rc;
	}

	/* New image address(0x1500400) entry at offset 0x004 */
	buf[4] = 0x00;
	buf[5] = 0x04;
	buf[6] = 0x50;
	buf[7] = 0x01;

	/* Erase SPI flash directory */

	rc = flash_erase(flash_dev, SPI_FLASH_DIRECTORY_OFFSET, SPI_FLASH_SECTOR_SIZE);
	if (rc != 0) {
		LOG_ERR("erase failed! %d", rc);
	}

	/* Write the first page with updated address entry */
	rc = flash_write(flash_dev, SPI_FLASH_DIRECTORY_OFFSET, buf, len);
	if (rc != 0) {
		LOG_ERR("Flash write failed! %d", rc);
		return rc;
	}

	/* Fill page number second, third and fourth with zeros */
	memset(buf, 0, len);
	k = 1;
	while (k < 4) {
		rc = flash_write(flash_dev, (SPI_FLASH_DIRECTORY_OFFSET + k * 0x100), buf, len);
		if (rc != 0) {
			LOG_ERR("Flash write failed! %d", rc);
			return rc;
		}
		k++;
	}

	return rc;
}

/* This function Program a new FPGA design image into the SPI Flash at location
 * 0x1500400.
 * Note: The source location of new image is _bin_start symbol value and the size of
 * new image is  _bim_size symbol value.
 */
static uint8_t program_new_image(const struct device *flash_dev, uint8_t *image_start,
				 uint32_t image_size)
{
	size_t len = SPI_FLASH_PAGE_SIZE;
	uint8_t buf[SPI_FLASH_PAGE_SIZE];
	uint8_t rc;
	uint32_t i, count, k;
	uint8_t *temp;

	temp = image_start;

	if (image_size > 0x1400000) {
		LOG_ERR("Image is larger than 20Mb");
		return 1;
	}

	/* Find the sectors to erase */
	count = (uint32_t)(image_size / SPI_FLASH_SECTOR_SIZE) + 1;

	LOG_INF("Erasing.");
	i = 0;
	while (i < count) {
		rc = flash_erase(
			flash_dev,
			((SPI_FLASH_NEW_IMAGE_OFFSET - 0x400) + (i * SPI_FLASH_SECTOR_SIZE)),
			SPI_FLASH_SECTOR_SIZE);
		if (rc != 0) {
			LOG_ERR("erase failed! %d", rc);
		}

		if (i % 0x100 == 0) {
			LOG_DBG(".");
		}

		i++;
	}
	/* Erase completed and ready to program new image */

	/* Find the pages to program */
	count = (uint32_t)(image_size / SPI_FLASH_PAGE_SIZE) + 1;

	LOG_INF("Programming.");
	i = 0;
	while (i < count) {
		temp = (image_start + i * SPI_FLASH_PAGE_SIZE);
		memset(buf, 0, len);
		for (k = 0; k < 256; k++) {
			buf[k] = *temp;
			temp = temp + 1;
		}

		rc = flash_write(flash_dev, (SPI_FLASH_NEW_IMAGE_OFFSET + i * SPI_FLASH_PAGE_SIZE),
				 buf, len);
		if (rc != 0) {
			LOG_ERR("Flash write failed! %d", rc);
			return rc;
		}

		if (i % 0x100 == 0) {
			LOG_DBG(".");
		}

		i++;
	}

	LOG_INF("Programming completed.");

	return rc;
}

static int8_t verify_image(const struct device *dev)
{
	const struct mpfs_fpga_config *cfg = dev->config;
	int8_t status = EINVAL;
	uint32_t value = 0;

	LOG_INF("Image verification started...");

	/* Once system controller starts processing command The busy bit will
	 * go 1. Make sure that service is complete i.e. BUSY bit is gone 0
	 */
	while (scb_read(cfg->base, SERVICES_SR_OFFSET) & SCBCTRL_SERVICESSR_BUSY_MASK) {
		;
	}

	/* Form the SS command: bit 0 to 6 is the opcode, bit 7 to 15 is the Mailbox
	 * offset For some services this field has another meaning.
	 * (e.g. for IAP bit-stream auth. it means spi_idx)
	 */
	scb_write(cfg->mailbox, 0, 0x1500400);

	value = (MSS_SYS_BITSTREAM_AUTHENTICATE_CMD << 16) | 0x1;
	scb_write(cfg->base, SERVICES_CR_OFFSET, value);

	/* REQ bit will remain set till the system controller starts
	 * processing command. Since DRI is slow interface, we are waiting
	 * here to make sure System controller has started processing
	 * command
	 */
	while (scb_read(cfg->base, SERVICES_CR_OFFSET) & SCBCTRL_SERVICESCR_REQ_MASK) {
		;
	}

	/* Once system controller starts processing command The busy bit will
	 * go 1. Make sure that service is complete i.e. BUSY bit is gone 0
	 */
	while (scb_read(cfg->base, SERVICES_SR_OFFSET) & SCBCTRL_SERVICESSR_BUSY_MASK) {
		;
	}

	/* Read the status returned by System Controller */
	status = ((scb_read(cfg->base, SERVICES_SR_OFFSET) & SCBCTRL_SERVICESSR_STATUS_MASK) >>
		  SCBCTRL_SERVICESSR_STATUS);
	LOG_INF("Image verification status  : %x   ", status);

	return status;
}

static void activate_image(const struct device *dev)
{
	const struct mpfs_fpga_config *cfg = dev->config;
	int8_t status = EINVAL;
	uint32_t value = 0;

	LOG_INF("Image activation started...");

	/* Once system controller starts processing command The busy bit will
	 * go 1. Make sure that service is complete i.e. BUSY bit is gone 0
	 */
	while (scb_read(cfg->base, SERVICES_SR_OFFSET) & SCBCTRL_SERVICESSR_BUSY_MASK) {
		;
	}

	/* Form the SS command: bit 0 to 6 is the opcode, bit 7 to 15 is the Mailbox
	 * offset For some services this field has another meaning.
	 * (e.g. for IAP bit-stream auth. it means spi_idx)
	 */
	value = (MSS_SYS_IAP_PROGRAM_BY_SPIIDX_CMD << 16) | BIT(23) | 0x1;
	scb_write(cfg->base, SERVICES_CR_OFFSET, value);

	/* REQ bit will remain set till the system controller starts
	 * processing command. Since DRI is slow interface, we are waiting
	 * here to make sure System controller has started processing
	 * command
	 */
	while (scb_read(cfg->base, SERVICES_CR_OFFSET) & SCBCTRL_SERVICESCR_REQ_MASK) {
		;
	}

	/* Once system controller starts processing command The busy bit will
	 * go 1. Make sure that service is complete i.e. BUSY bit is gone 0
	 */
	while (scb_read(cfg->base, SERVICES_SR_OFFSET) & SCBCTRL_SERVICESSR_BUSY_MASK) {
		;
	}

	/* Read the status returned by System Controller */
	status = ((scb_read(cfg->base, SERVICES_SR_OFFSET) & SCBCTRL_SERVICESSR_STATUS_MASK) >>
		  SCBCTRL_SERVICESSR_STATUS);
	LOG_INF("Image activation status  : %x   ", status);
}

static int mpfs_fpga_reset(const struct device *dev)
{
	int8_t status = EINVAL;

	status = verify_image(dev);
	if (status == 0) {
		activate_image(dev);
	}
	return 0;
}

static int mpfs_fpga_load(const struct device *dev, uint32_t *image_ptr, uint32_t img_size)
{
	const struct device *flash_dev = DEVICE_DT_GET_OR_NULL(DT_ALIAS(bitstream_flash));

	if (flash_dev == NULL) {
		LOG_ERR("Device not found");
		return -ENOENT;
	}

	if (!device_is_ready(flash_dev)) {
		LOG_ERR("%s: device not ready.", flash_dev->name);
		return 1;
	}

	if (img_size == 0) {
		LOG_ERR("Image size is zero.");
		return -EINVAL;
	}

	if (image_ptr == NULL) {
		LOG_ERR("Failed to read FPGA image");
		return -EINVAL;
	}

	update_spi_flash_directory(flash_dev);
	program_new_image(flash_dev, (uint8_t *)image_ptr, img_size);
	return 0;
}

static const char *mpfs_fpga_get_info(const struct device *dev)
{
	struct mpfs_fpga_data *data = dev->data;
	const struct mpfs_fpga_config *cfg = dev->config;
	uint32_t value = 0;
	uint16_t design_version = 0;

	/* Once system controller starts processing command The busy bit will
	 * go 1. Make sure that service is complete i.e. BUSY bit is gone 0
	 */
	while (scb_read(cfg->base, SERVICES_SR_OFFSET) & SCBCTRL_SERVICESSR_BUSY_MASK) {
		;
	}

	/* Form the SS command: bit 0 to 6 is the opcode, bit 7 to 15 is the Mailbox
	 * offset For some services this field has another meaning.
	 * (e.g. for IAP bit-stream auth. it means spi_idx)
	 */

	value = (MSS_DESIGN_INFO_CMD << 16) | 0x1;
	scb_write(cfg->base, SERVICES_CR_OFFSET, value);

	/* REQ bit will remain set till the system controller starts
	 * processing command. Since DRI is slow interface, we are waiting
	 * here to make sure System controller has started processing
	 * command
	 */
	while (scb_read(cfg->base, SERVICES_CR_OFFSET) & SCBCTRL_SERVICESCR_REQ_MASK) {
		;
	}

	/* Once system controller starts processing command The busy bit will
	 * go 1. Make sure that service is complete i.e. BUSY bit is gone 0
	 */
	while (scb_read(cfg->base, SERVICES_SR_OFFSET) & SCBCTRL_SERVICESSR_BUSY_MASK) {
		;
	}

	design_version = scb_read(cfg->mailbox, 32);
	sprintf(data->FPGA_design_ver, (uint8_t *)"Design Version : 0x%x", design_version);

	return data->FPGA_design_ver;
}

static enum FPGA_status mpfs_fpga_get_status(const struct device *dev)
{
	const struct mpfs_fpga_config *cfg = dev->config;

	if (scb_read(cfg->base, SERVICES_SR_OFFSET) & SCBCTRL_SERVICESSR_BUSY_MASK) {
		return FPGA_STATUS_INACTIVE;
	} else {
		return FPGA_STATUS_ACTIVE;
	}
}

static int mpfs_fpga_init(const struct device *dev)
{
	return 0;
}

static struct mpfs_fpga_data fpga_data;

static struct mpfs_fpga_config fpga_config = {
	.base = DT_INST_REG_ADDR_BY_IDX(0, 0),
	.mailbox = DT_INST_REG_ADDR_BY_IDX(0, 2),
};

static const struct fpga_driver_api mpfs_fpga_api = {
	.reset = mpfs_fpga_reset,
	.load = mpfs_fpga_load,
	.get_info = mpfs_fpga_get_info,
	.get_status = mpfs_fpga_get_status,
};

DEVICE_DT_INST_DEFINE(0, &mpfs_fpga_init, NULL, &fpga_data, &fpga_config, POST_KERNEL,
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &mpfs_fpga_api);

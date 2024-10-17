/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT jedec_mspi_nor

#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#include "jesd216.h"
#include "spi_nor.h"

LOG_MODULE_REGISTER(flash_mspi_nor, CONFIG_FLASH_LOG_LEVEL);

struct flash_mspi_nor_data {
	struct k_sem acquired;
	struct mspi_xfer_packet packet;
	struct mspi_xfer xfer;
};

struct flash_mspi_nor_config {
	const struct device *bus;
	struct mspi_dev_id mspi_id;
	struct mspi_dev_cfg mspi_cfg;
#if defined(CONFIG_MSPI_XIP)
	struct mspi_xip_cfg xip_cfg;
#endif
	struct gpio_dt_spec reset;
	uint32_t flash_size;
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	struct flash_pages_layout layout;
#endif
	uint8_t jedec_id[SPI_NOR_MAX_ID_LEN];
};

static int acquire(const struct device *dev)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	struct flash_mspi_nor_data *dev_data = dev->data;
	int rc;

	k_sem_take(&dev_data->acquired, K_FOREVER);

	rc = pm_device_runtime_get(dev_config->bus);
	if (rc < 0) {
		LOG_ERR("pm_device_runtime_get() failed: %d", rc);
		return rc;
	}

	/* This acquires the MSPI controller and configures it for the flash. */
	rc = mspi_dev_config(dev_config->bus, &dev_config->mspi_id,
			     MSPI_DEVICE_CONFIG_ALL, &dev_config->mspi_cfg);
	if (rc < 0) {
		LOG_ERR("mspi_dev_config() failed: %d", rc);
		return rc;
	}

	return 0;
}

static void release(const struct device *dev)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	struct flash_mspi_nor_data *dev_data = dev->data;

	/* This releases the MSPI controller. */
	(void)mspi_get_channel_status(dev_config->bus, 0);

	(void)pm_device_runtime_put(dev_config->bus);

	k_sem_give(&dev_data->acquired);
}

static inline uint32_t dev_flash_size(const struct device *dev)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;

	return dev_config->flash_size;
}

static inline uint16_t dev_page_size(const struct device *dev)
{
	return SPI_NOR_PAGE_SIZE;
}

static int api_read(const struct device *dev, off_t addr, void *dest,
		    size_t size)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	struct flash_mspi_nor_data *dev_data = dev->data;
	const uint32_t flash_size = dev_flash_size(dev);
	int rc;

	if (size == 0) {
		return 0;
	}

	if ((addr < 0) || ((addr + size) > flash_size)) {
		return -EINVAL;
	}

	rc = acquire(dev);
	if (rc < 0) {
		return rc;
	}

	/* TODO: get rid of all these hard-coded values for MX25Ux chips */
	dev_data->xfer.cmd_length  = 2;
	dev_data->xfer.addr_length = 4;
	dev_data->xfer.rx_dummy    = 20;
	dev_data->packet.dir       = MSPI_RX;
	dev_data->packet.cmd       = SPI_NOR_OCMD_RD;
	dev_data->packet.address   = addr;
	dev_data->packet.data_buf  = dest;
	dev_data->packet.num_bytes = size;
	rc = mspi_transceive(dev_config->bus, &dev_config->mspi_id,
			     &dev_data->xfer);

	release(dev);

	if (rc < 0) {
		LOG_ERR("SPI_NOR_OCMD_RD xfer failed: %d", rc);
		return rc;
	}

	return 0;
}

static int wait_until_ready(const struct device *dev, k_timeout_t poll_period)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	struct flash_mspi_nor_data *dev_data = dev->data;
	uint8_t status_reg;
	int rc;

	while (true) {
		dev_data->xfer.cmd_length  = 2;
		dev_data->xfer.addr_length = 4;
		dev_data->xfer.rx_dummy    = 4;
		dev_data->packet.dir       = MSPI_RX;
		dev_data->packet.cmd       = SPI_NOR_OCMD_RDSR;
		dev_data->packet.address   = 0;
		dev_data->packet.data_buf  = &status_reg;
		dev_data->packet.num_bytes = sizeof(status_reg);
		rc = mspi_transceive(dev_config->bus, &dev_config->mspi_id,
				     &dev_data->xfer);
		if (rc < 0) {
			LOG_ERR("SPI_NOR_OCMD_RDSR xfer failed: %d", rc);
			return rc;
		}
		if (!(status_reg & SPI_NOR_WIP_BIT)) {
			break;
		}

		k_sleep(poll_period);
	}

	return 0;
}

static int api_write(const struct device *dev, off_t addr, const void *src,
		     size_t size)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	struct flash_mspi_nor_data *dev_data = dev->data;
	const uint32_t flash_size = dev_flash_size(dev);
	const uint16_t page_size = dev_page_size(dev);
	int rc;

	if (size == 0) {
		return 0;
	}

	if ((addr < 0) || ((addr + size) > flash_size)) {
		return -EINVAL;
	}

	rc = acquire(dev);
	if (rc < 0) {
		return rc;
	}

	while (size > 0) {
		/* Split write into parts, each within one page only. */
		uint16_t page_offset = (uint16_t)(addr % page_size);
		uint16_t page_left = page_size - page_offset;
		uint16_t to_write = (uint16_t)MIN(size, page_left);

		dev_data->xfer.cmd_length  = 2;
		dev_data->xfer.tx_dummy    = 0;
		dev_data->packet.dir       = MSPI_TX;

		dev_data->xfer.addr_length = 0;
		dev_data->packet.cmd       = SPI_NOR_OCMD_WREN;
		dev_data->packet.num_bytes = 0;
		rc = mspi_transceive(dev_config->bus, &dev_config->mspi_id,
				     &dev_data->xfer);
		if (rc < 0) {
			LOG_ERR("SPI_NOR_OCMD_WREN xfer failed: %d", rc);
			break;
		}

		dev_data->xfer.addr_length = 4;
		dev_data->packet.cmd       = SPI_NOR_OCMD_PAGE_PRG;
		dev_data->packet.address   = addr;
		dev_data->packet.data_buf  = (uint8_t *)src;
		dev_data->packet.num_bytes = to_write;
		rc = mspi_transceive(dev_config->bus, &dev_config->mspi_id,
				     &dev_data->xfer);
		if (rc < 0) {
			LOG_ERR("SPI_NOR_OCMD_PAGE_PRG xfer failed: %d", rc);
			break;
		}

		addr += to_write;
		src   = (const uint8_t *)src + to_write;
		size -= to_write;

		rc = wait_until_ready(dev, K_MSEC(1));
		if (rc < 0) {
			break;
		}
	}

	release(dev);

	return rc;
}

static int api_erase(const struct device *dev, off_t addr, size_t size)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	struct flash_mspi_nor_data *dev_data = dev->data;
	const uint32_t flash_size = dev_flash_size(dev);
	int rc = 0;

	if ((addr < 0) || ((addr + size) > flash_size)) {
		return -EINVAL;
	}

	if (!SPI_NOR_IS_SECTOR_ALIGNED(addr)) {
		return -EINVAL;
	}

	if ((size % SPI_NOR_SECTOR_SIZE) != 0) {
		return -EINVAL;
	}

	rc = acquire(dev);
	if (rc < 0) {
		return rc;
	}

	while (size > 0) {
		dev_data->xfer.cmd_length  = 2;
		dev_data->xfer.tx_dummy    = 0;
		dev_data->packet.dir       = MSPI_TX;
		dev_data->packet.num_bytes = 0;

		dev_data->xfer.addr_length = 0;
		dev_data->packet.cmd       = SPI_NOR_OCMD_WREN;
		rc = mspi_transceive(dev_config->bus, &dev_config->mspi_id,
				     &dev_data->xfer);
		if (rc < 0) {
			LOG_ERR("SPI_NOR_OCMD_WREN xfer failed: %d", rc);
			break;
		}

		if (size == flash_size) {
			/* Chip erase. */
			dev_data->xfer.addr_length = 0;
			dev_data->packet.cmd       = SPI_NOR_OCMD_CE;

			size -= flash_size;
		} else {
			/* Sector erase. */
			dev_data->xfer.addr_length = 4;
			dev_data->packet.cmd       = SPI_NOR_OCMD_SE;
			dev_data->packet.address   = addr;

			addr += SPI_NOR_SECTOR_SIZE;
			size -= SPI_NOR_SECTOR_SIZE;
		}
		rc = mspi_transceive(dev_config->bus, &dev_config->mspi_id,
				     &dev_data->xfer);
		if (rc < 0) {
			LOG_ERR("Erase command 0x%02x xfer failed: %d",
				dev_data->packet.cmd, rc);
			break;
		}

		rc = wait_until_ready(dev, K_MSEC(1));
		if (rc < 0) {
			break;
		}
	}

	release(dev);

	return rc;
}

static const
struct flash_parameters *api_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	static const struct flash_parameters parameters = {
		.write_block_size = 1,
		.erase_value = 0xff,
	};

	return &parameters;
}

static int read_jedec_id(const struct device *dev, uint8_t *id)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	struct flash_mspi_nor_data *dev_data = dev->data;
	int rc;

	dev_data->xfer.cmd_length  = 2;
	dev_data->xfer.addr_length = 4;
	dev_data->xfer.rx_dummy    = 4;
	dev_data->packet.dir       = MSPI_RX;
	dev_data->packet.cmd       = JESD216_OCMD_READ_ID;
	dev_data->packet.address   = 0;
	dev_data->packet.data_buf  = id;
	dev_data->packet.num_bytes = JESD216_READ_ID_LEN;
	rc = mspi_transceive(dev_config->bus, &dev_config->mspi_id,
			     &dev_data->xfer);
	if (rc < 0) {
		printk("mspi_transceive() failed: %d\n", rc);
		return rc;
	}

	return rc;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void api_page_layout(const struct device *dev,
			     const struct flash_pages_layout **layout,
			     size_t *layout_size)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;

	*layout = &dev_config->layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

#if defined(CONFIG_FLASH_JESD216_API)
static int api_sfdp_read(const struct device *dev, off_t addr, void *dest,
			 size_t size)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	struct flash_mspi_nor_data *dev_data = dev->data;
	int rc;

	if (size == 0) {
		return 0;
	}

	rc = acquire(dev);
	if (rc < 0) {
		return rc;
	}

	dev_data->xfer.cmd_length  = 2;
	dev_data->xfer.addr_length = 4;
	dev_data->xfer.rx_dummy    = 20;
	dev_data->packet.dir       = MSPI_RX;
	dev_data->packet.cmd       = JESD216_OCMD_READ_SFDP;
	dev_data->packet.address   = addr;
	dev_data->packet.data_buf  = dest;
	dev_data->packet.num_bytes = size;
	rc = mspi_transceive(dev_config->bus, &dev_config->mspi_id,
			     &dev_data->xfer);
	if (rc < 0) {
		printk("JESD216_OCMD_READ_SFDP xfer failed: %d\n", rc);
		return rc;
	}

	release(dev);

	return rc;
}

static int api_read_jedec_id(const struct device *dev, uint8_t *id)
{
	int rc = 0;

	rc = acquire(dev);
	if (rc < 0) {
		return rc;
	}

	rc = read_jedec_id(dev, id);

	release(dev);

	return rc;
}
#endif /* CONFIG_FLASH_JESD216_API  */

static int dev_pm_action_cb(const struct device *dev,
			    enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		break;
	case PM_DEVICE_ACTION_RESUME:
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int flash_chip_init(const struct device *dev)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	struct flash_mspi_nor_data *dev_data = dev->data;
	struct mspi_dev_cfg init_dev_cfg = dev_config->mspi_cfg;
	uint8_t id[JESD216_READ_ID_LEN] = {0};
	int rc;

	init_dev_cfg.freq = MHZ(1);
	init_dev_cfg.io_mode = MSPI_IO_MODE_SINGLE;

	rc = mspi_dev_config(dev_config->bus, &dev_config->mspi_id,
			     MSPI_DEVICE_CONFIG_ALL, &init_dev_cfg);
	if (rc < 0) {
		LOG_ERR("Failed to set initial device config: %d", rc);
		return rc;
	}

	dev_data->xfer.xfer_mode  = MSPI_PIO;
	dev_data->xfer.packets    = &dev_data->packet;
	dev_data->xfer.num_packet = 1;
	dev_data->xfer.timeout    = 10;

	dev_data->xfer.cmd_length  = 1;
	dev_data->xfer.addr_length = 0;
	dev_data->xfer.tx_dummy    = 0;
	dev_data->xfer.rx_dummy    = 0;

	dev_data->packet.dir       = MSPI_RX;
	dev_data->packet.cmd       = JESD216_CMD_READ_ID;
	dev_data->packet.data_buf  = id;
	dev_data->packet.num_bytes = sizeof(id);
	rc = mspi_transceive(dev_config->bus, &dev_config->mspi_id,
			     &dev_data->xfer);
	if (rc < 0) {
		LOG_ERR("Failed to read JEDEC ID in single line mode: %d", rc);
		return rc;
	}

	/*
	 * If the read ID does not match the one from DTS, assume the flash
	 * is already in the Octa I/O mode, so switching it is not needed.
	 */
	if (memcmp(id, dev_config->jedec_id, sizeof(id)) == 0) {
		static const uint8_t enable_sopi[] = { 0x01 };

		dev_data->packet.dir       = MSPI_TX;
		dev_data->packet.cmd       = SPI_NOR_CMD_WREN;
		dev_data->packet.num_bytes = 0;
		rc = mspi_transceive(dev_config->bus, &dev_config->mspi_id,
				     &dev_data->xfer);
		if (rc < 0) {
			LOG_ERR("SPI_NOR_CMD_WREN xfer failed: %d", rc);
			return rc;
		}

		dev_data->xfer.addr_length = 4;
		dev_data->packet.cmd       = SPI_NOR_CMD_WR_CFGREG2;
		dev_data->packet.address   = 0;
		dev_data->packet.data_buf  = (uint8_t *)&enable_sopi;
		dev_data->packet.num_bytes = sizeof(enable_sopi);
		rc = mspi_transceive(dev_config->bus, &dev_config->mspi_id,
				     &dev_data->xfer);
		if (rc < 0) {
			printk("SPI_NOR_CMD_WR_CFGREG2 xfer failed: %d\n", rc);
			return rc;
		}
	}

	rc = mspi_dev_config(dev_config->bus, &dev_config->mspi_id,
			     MSPI_DEVICE_CONFIG_ALL, &dev_config->mspi_cfg);
	if (rc < 0) {
		LOG_ERR("Failed to set device config: %d", rc);
		return rc;
	}

	rc = read_jedec_id(dev, id);
	if (rc < 0) {
		return rc;
	}

	if (memcmp(id, dev_config->jedec_id, sizeof(id)) != 0) {
		LOG_ERR("JEDEC ID mismatch, read: %02x %02x %02x, "
			"expected: %02x %02x %02x",
			id[0], id[1], id[2],
			dev_config->jedec_id[0],
			dev_config->jedec_id[1],
			dev_config->jedec_id[2]);
		return -ENODEV;
	}

#if defined(CONFIG_MSPI_XIP)
	/* Enable XIP access for this chip if specified so in DT. */
	if (dev_config->xip_cfg.enable) {
		rc = mspi_xip_config(dev_config->bus, &dev_config->mspi_id,
				     &dev_config->xip_cfg);
		if (rc < 0) {
			return rc;
		}
	}
#endif

	return 0;
}

static int drv_init(const struct device *dev)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	struct flash_mspi_nor_data *dev_data = dev->data;
	int rc;

	if (!device_is_ready(dev_config->bus)) {
		LOG_ERR("Device %s is not ready", dev_config->bus->name);
		return -ENODEV;
	}

	if (dev_config->reset.port) {
		if (!gpio_is_ready_dt(&dev_config->reset)) {
			LOG_ERR("Device %s is not ready",
				dev_config->reset.port->name);
			return -ENODEV;
		}

		if (gpio_pin_configure_dt(&dev_config->reset,
					  GPIO_OUTPUT_ACTIVE) < 0 ||
		    gpio_pin_set_dt(&dev_config->reset, 0) < 0) {
			return -EIO;
		}
	}

	rc = pm_device_runtime_get(dev_config->bus);
	if (rc < 0) {
		LOG_ERR("pm_device_runtime_get() failed: %d", rc);
		return rc;
	}

	/* Acquire the MSPI controller. */
	rc = mspi_dev_config(dev_config->bus, &dev_config->mspi_id,
			     MSPI_DEVICE_CONFIG_NONE, NULL);
	if (rc == 0) {
		rc = flash_chip_init(dev);

		/* Release the MSPI controller. */
		(void)mspi_get_channel_status(dev_config->bus, 0);
	}

	(void)pm_device_runtime_put(dev_config->bus);

	if (rc < 0) {
		return rc;
	}

	k_sem_init(&dev_data->acquired, 1, K_SEM_MAX_LIMIT);

	return pm_device_driver_init(dev, dev_pm_action_cb);
}

static const struct flash_driver_api drv_api = {
	.read = api_read,
	.write = api_write,
	.erase = api_erase,
	.get_parameters = api_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = api_page_layout,
#endif
#if defined(CONFIG_FLASH_JESD216_API)
	.sfdp_read = api_sfdp_read,
	.read_jedec_id = api_read_jedec_id,
#endif
};

#define FLASH_SIZE_INST(inst) (DT_INST_PROP(inst, size) / 8)

#define FLASH_MSPI_NOR_INST(inst)					\
	BUILD_ASSERT(DT_INST_ENUM_IDX(inst, mspi_io_mode) ==		\
		     MSPI_IO_MODE_OCTAL,				\
		"Only Octal I/O mode is supported for now");		\
	PM_DEVICE_DT_INST_DEFINE(inst, dev_pm_action_cb);		\
	static struct flash_mspi_nor_data dev##inst##_data;		\
	static const struct flash_mspi_nor_config dev##inst##_config = {\
		.bus = DEVICE_DT_GET(DT_INST_BUS(inst)),		\
		.mspi_id = MSPI_DEVICE_ID_DT_INST(inst),		\
		.mspi_cfg = MSPI_DEVICE_CONFIG_DT_INST(inst),		\
	IF_ENABLED(CONFIG_MSPI_XIP,					\
		(.xip_cfg = MSPI_XIP_CONFIG_DT_INST(inst),))		\
		.reset = GPIO_DT_SPEC_INST_GET_OR(inst,			\
						  reset_gpios, {0}),	\
		.flash_size = FLASH_SIZE_INST(inst),			\
	IF_ENABLED(CONFIG_FLASH_PAGE_LAYOUT,				\
		(.layout = {						\
			/* TODO: make this size configurable */		\
			.pages_size = 65536,				\
			.pages_count = FLASH_SIZE_INST(inst) / 65536,	\
		},))							\
		.jedec_id = DT_INST_PROP(inst, jedec_id),		\
	};								\
	DEVICE_DT_INST_DEFINE(inst,					\
		drv_init, PM_DEVICE_DT_INST_GET(inst),			\
		&dev##inst##_data, &dev##inst##_config,			\
		POST_KERNEL, CONFIG_FLASH_MSPI_NOR_INIT_PRIORITY,	\
		&drv_api);

DT_INST_FOREACH_STATUS_OKAY(FLASH_MSPI_NOR_INST)

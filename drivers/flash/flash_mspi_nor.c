/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT jedec_mspi_nor

#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#include "flash_mspi_nor.h"
#include "flash_mspi_nor_quirks.h"

LOG_MODULE_REGISTER(flash_mspi_nor, CONFIG_FLASH_LOG_LEVEL);

void flash_mspi_command_set(const struct device *dev, const struct flash_mspi_nor_cmd *cmd)
{
	struct flash_mspi_nor_data *dev_data = dev->data;
	const struct flash_mspi_nor_config *dev_config = dev->config;

	memset(&dev_data->xfer, 0, sizeof(dev_data->xfer));
	memset(&dev_data->packet, 0, sizeof(dev_data->packet));

	dev_data->xfer.xfer_mode  = MSPI_PIO;
	dev_data->xfer.packets    = &dev_data->packet;
	dev_data->xfer.num_packet = 1;
	dev_data->xfer.timeout    = dev_config->transfer_timeout;

	dev_data->xfer.cmd_length = cmd->cmd_length;
	dev_data->xfer.addr_length = cmd->addr_length;
	dev_data->xfer.tx_dummy = (cmd->dir == MSPI_TX) ?
				  cmd->tx_dummy : dev_config->mspi_nor_cfg.tx_dummy;
	dev_data->xfer.rx_dummy = (cmd->dir == MSPI_RX) ?
				  cmd->rx_dummy : dev_config->mspi_nor_cfg.rx_dummy;

	dev_data->packet.dir = cmd->dir;
	dev_data->packet.cmd = cmd->cmd;
}

static int dev_cfg_apply(const struct device *dev, const struct mspi_dev_cfg *cfg)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	struct flash_mspi_nor_data *dev_data = dev->data;

	if (dev_data->curr_cfg == cfg) {
		return 0;
	}

	int rc = mspi_dev_config(dev_config->bus, &dev_config->mspi_id,
				 MSPI_DEVICE_CONFIG_ALL, cfg);
	if (rc < 0) {
		LOG_ERR("Failed to set device config: %p error: %d", cfg, rc);
	}
	return rc;
}

static int acquire(const struct device *dev)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	struct flash_mspi_nor_data *dev_data = dev->data;
	int rc;

	k_sem_take(&dev_data->acquired, K_FOREVER);

	rc = pm_device_runtime_get(dev_config->bus);
	if (rc < 0) {
		LOG_ERR("pm_device_runtime_get() failed: %d", rc);
	} else {
		/* This acquires the MSPI controller and reconfigures it
		 * if needed for the flash device.
		 */
		rc = mspi_dev_config(dev_config->bus, &dev_config->mspi_id,
				     dev_config->mspi_nor_cfg_mask,
				     &dev_config->mspi_nor_cfg);
		if (rc < 0) {
			LOG_ERR("mspi_dev_config() failed: %d", rc);
		} else {
			return 0;
		}

		(void)pm_device_runtime_put(dev_config->bus);
	}

	k_sem_give(&dev_data->acquired);
	return rc;
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

	if (dev_config->jedec_cmds->read.force_single) {
		rc = dev_cfg_apply(dev, &dev_config->mspi_nor_init_cfg);
	} else {
		rc = dev_cfg_apply(dev, &dev_config->mspi_nor_cfg);
	}

	if (rc < 0) {
		return rc;
	}

	/* TODO: get rid of all these hard-coded values for MX25Ux chips */
	flash_mspi_command_set(dev, &dev_config->jedec_cmds->read);
	dev_data->packet.address   = addr;
	dev_data->packet.data_buf  = dest;
	dev_data->packet.num_bytes = size;
	rc = mspi_transceive(dev_config->bus, &dev_config->mspi_id,
			     &dev_data->xfer);

	release(dev);

	if (rc < 0) {
		LOG_ERR("Read xfer failed: %d", rc);
		return rc;
	}

	return 0;
}

static int status_get(const struct device *dev, uint8_t *status)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	struct flash_mspi_nor_data *dev_data = dev->data;
	int rc;

	/* Enter command mode */
	if (dev_config->jedec_cmds->status.force_single) {
		rc = dev_cfg_apply(dev, &dev_config->mspi_nor_init_cfg);
	} else {
		rc = dev_cfg_apply(dev, &dev_config->mspi_nor_cfg);
	}

	if (rc < 0) {
		LOG_ERR("Switching to dev_cfg failed: %d", rc);
		return rc;
	}

	flash_mspi_command_set(dev, &dev_config->jedec_cmds->status);
	dev_data->packet.data_buf  = status;
	dev_data->packet.num_bytes = sizeof(uint8_t);

	rc = mspi_transceive(dev_config->bus, &dev_config->mspi_id, &dev_data->xfer);

	if (rc < 0) {
		LOG_ERR("Status xfer failed: %d", rc);
		return rc;
	}

	return rc;
}

static int wait_until_ready(const struct device *dev, k_timeout_t poll_period)
{
	int rc;
	uint8_t status_reg;

	while (true) {
		rc = status_get(dev, &status_reg);

		if (rc < 0) {
			LOG_ERR("Wait until ready - status xfer failed: %d", rc);
			return rc;
		}

		if (!(status_reg & SPI_NOR_WIP_BIT)) {
			break;
		}

		k_sleep(poll_period);
	}

	return 0;
}

static int write_enable(const struct device *dev)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	struct flash_mspi_nor_data *dev_data = dev->data;
	int rc;

	if (dev_config->jedec_cmds->write_en.force_single) {
		rc = dev_cfg_apply(dev, &dev_config->mspi_nor_init_cfg);
	} else {
		rc = dev_cfg_apply(dev, &dev_config->mspi_nor_cfg);
	}

	if (rc < 0) {
		return rc;
	}

	flash_mspi_command_set(dev, &dev_config->jedec_cmds->write_en);
	return mspi_transceive(dev_config->bus, &dev_config->mspi_id, &dev_data->xfer);
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

		if (write_enable(dev) < 0) {
			LOG_ERR("Write enable xfer failed: %d", rc);
			break;
		}

		if (dev_config->jedec_cmds->page_program.force_single) {
			rc = dev_cfg_apply(dev, &dev_config->mspi_nor_init_cfg);
		} else {
			rc = dev_cfg_apply(dev, &dev_config->mspi_nor_cfg);
		}

		if (rc < 0) {
			return rc;
		}

		flash_mspi_command_set(dev, &dev_config->jedec_cmds->page_program);
		dev_data->packet.address   = addr;
		dev_data->packet.data_buf  = (uint8_t *)src;
		dev_data->packet.num_bytes = to_write;
		rc = mspi_transceive(dev_config->bus, &dev_config->mspi_id,
				     &dev_data->xfer);
		if (rc < 0) {
			LOG_ERR("Page program xfer failed: %d", rc);
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
		rc = write_enable(dev);
		if (rc < 0) {
			LOG_ERR("Write enable failed.");
			break;
		}

		if (size == flash_size) {
			/* Chip erase. */
			if (dev_config->jedec_cmds->chip_erase.force_single) {
				rc = dev_cfg_apply(dev, &dev_config->mspi_nor_init_cfg);
			} else {
				rc = dev_cfg_apply(dev, &dev_config->mspi_nor_cfg);
			}

			if (rc < 0) {
				return rc;
			}

			flash_mspi_command_set(dev, &dev_config->jedec_cmds->chip_erase);
			size -= flash_size;
		} else {
			/* Sector erase. */
			if (dev_config->jedec_cmds->sector_erase.force_single) {
				rc = dev_cfg_apply(dev, &dev_config->mspi_nor_init_cfg);
			} else {
				rc = dev_cfg_apply(dev, &dev_config->mspi_nor_cfg);
			}

			if (rc < 0) {
				return rc;
			}

			flash_mspi_command_set(dev, &dev_config->jedec_cmds->sector_erase);
			dev_data->packet.address = addr;
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

static int api_get_size(const struct device *dev, uint64_t *size)
{
	*size = dev_flash_size(dev);
	return 0;
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

	if (dev_config->jedec_cmds->id.force_single) {
		rc = dev_cfg_apply(dev, &dev_config->mspi_nor_init_cfg);
	} else {
		rc = dev_cfg_apply(dev, &dev_config->mspi_nor_cfg);
	}

	if (rc < 0) {
		return rc;
	}

	flash_mspi_command_set(dev, &dev_config->jedec_cmds->id);
	dev_data->packet.data_buf  = id;
	dev_data->packet.num_bytes = JESD216_READ_ID_LEN;

	rc = mspi_transceive(dev_config->bus, &dev_config->mspi_id,
			     &dev_data->xfer);
	if (rc < 0) {
		LOG_ERR("Read JEDEC ID failed: %d\n", rc);
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

	if (dev_config->jedec_cmds->sfdp.force_single) {
		rc = dev_cfg_apply(dev, &dev_config->mspi_nor_init_cfg);
	} else {
		rc = dev_cfg_apply(dev, &dev_config->mspi_nor_cfg);
	}

	if (rc < 0) {
		return rc;
	}

	flash_mspi_command_set(dev, &dev_config->jedec_cmds->sfdp);
	dev_data->packet.address   = addr;
	dev_data->packet.data_buf  = dest;
	dev_data->packet.num_bytes = size;
	rc = mspi_transceive(dev_config->bus, &dev_config->mspi_id,
			     &dev_data->xfer);
	if (rc < 0) {
		printk("Read SFDP xfer failed: %d\n", rc);
		return rc;
	}

	release(dev);

	return rc;
}

static int api_read_jedec_id(const struct device *dev, uint8_t *id)
{
	int rc = acquire(dev);
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

static int quad_enable_set(const struct device *dev, bool enable)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	struct flash_mspi_nor_data *dev_data = dev->data;
	int rc;

	flash_mspi_command_set(dev, &commands_single.write_en);
	rc = mspi_transceive(dev_config->bus, &dev_config->mspi_id,
			     &dev_data->xfer);
	if (rc < 0) {
		LOG_ERR("Failed to set write enable: %d", rc);
		return rc;
	}

	if (dev_config->dw15_qer == JESD216_DW15_QER_VAL_S1B6) {
		const struct flash_mspi_nor_cmd cmd_status = {
			.dir = MSPI_TX,
			.cmd = SPI_NOR_CMD_WRSR,
			.cmd_length = 1,
		};
		uint8_t mode_payload = enable ? BIT(6) : 0;

		flash_mspi_command_set(dev, &cmd_status);
		dev_data->packet.data_buf  = &mode_payload;
		dev_data->packet.num_bytes = sizeof(mode_payload);
		rc = mspi_transceive(dev_config->bus, &dev_config->mspi_id, &dev_data->xfer);

		if (rc < 0) {
			LOG_ERR("Failed to enable/disable quad mode: %d", rc);
			return rc;
		}
	} else {
		/* TODO: handle all DW15 QER values */
		return -ENOTSUP;
	}

	rc = wait_until_ready(dev, K_USEC(1));

	if (rc < 0) {
		LOG_ERR("Failed waiting until device ready after enabling quad: %d", rc);
		return rc;
	}

	return 0;
}


static int default_io_mode(const struct device *dev)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	enum mspi_io_mode io_mode = dev_config->mspi_nor_cfg.io_mode;
	int rc = 0;

	if (dev_config->dw15_qer != JESD216_DW15_QER_VAL_NONE) {
		/* For Quad 1-1-4 and 1-4-4, entering or leaving mode is defined
		 * in JEDEC216 BFP DW15 QER
		 */
		if (io_mode == MSPI_IO_MODE_SINGLE) {
			rc = quad_enable_set(dev, false);
		} else if (io_mode == MSPI_IO_MODE_QUAD_1_1_4 ||
			   io_mode == MSPI_IO_MODE_QUAD_1_4_4) {
			rc = quad_enable_set(dev, true);
		}

		if (rc < 0) {
			LOG_ERR("Failed to modify Quad Enable bit: %d", rc);
		}
	}

	if ((dev_config->quirks != NULL) && (dev_config->quirks->post_switch_mode != NULL)) {
		rc = dev_config->quirks->post_switch_mode(dev);
	}

	if (rc < 0) {
		LOG_ERR("Failed to change IO mode: %d\n", rc);
		return rc;
	}

	return dev_cfg_apply(dev, &dev_config->mspi_nor_cfg);
}

#if defined(WITH_RESET_GPIO)
static int gpio_reset(const struct device *dev)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	int rc;

	if (dev_config->reset.port) {
		if (!gpio_is_ready_dt(&dev_config->reset)) {
			LOG_ERR("Device %s is not ready",
				dev_config->reset.port->name);
			return -ENODEV;
		}

		rc = gpio_pin_configure_dt(&dev_config->reset,
					   GPIO_OUTPUT_ACTIVE);
		if (rc < 0) {
			LOG_ERR("Failed to activate RESET: %d", rc);
			return -EIO;
		}

		if (dev_config->reset_pulse_us != 0) {
			k_busy_wait(dev_config->reset_pulse_us);
		}

		rc = gpio_pin_set_dt(&dev_config->reset, 0);
		if (rc < 0) {
			LOG_ERR("Failed to deactivate RESET: %d", rc);
			return -EIO;
		}

		if (dev_config->reset_recovery_us != 0) {
			k_busy_wait(dev_config->reset_recovery_us);
		}
	}

	return 0;
}
#endif

static int flash_chip_init(const struct device *dev)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	struct flash_mspi_nor_data *dev_data = dev->data;
	enum mspi_io_mode io_mode = dev_config->mspi_nor_cfg.io_mode;
	uint8_t id[JESD216_READ_ID_LEN] = {0};
	int rc;

	rc = dev_cfg_apply(dev, &dev_config->mspi_nor_init_cfg);

	if (rc < 0) {
		return rc;
	}

	/* Some chips reuse RESET pin for data in Quad modes:
	 * force single line mode before resetting.
	 */
	if (dev_config->dw15_qer != JESD216_DW15_QER_VAL_NONE &&
	    (io_mode == MSPI_IO_MODE_SINGLE ||
	     io_mode == MSPI_IO_MODE_QUAD_1_1_4 ||
	     io_mode == MSPI_IO_MODE_QUAD_1_4_4)) {
		rc = quad_enable_set(dev, false);

		if (rc < 0) {
			LOG_ERR("Failed to switch to single line mode: %d", rc);
			return rc;
		}

		rc = wait_until_ready(dev, K_USEC(1));

		if (rc < 0) {
			LOG_ERR("Failed waiting for device after switch to single line: %d", rc);
			return rc;
		}
	}

#if defined(WITH_RESET_GPIO)
	rc = gpio_reset(dev);

	if (rc < 0) {
		LOG_ERR("Failed to reset with GPIO: %d", rc);
		return rc;
	}
#endif

	flash_mspi_command_set(dev, &commands_single.id);
	dev_data->packet.data_buf  = id;
	dev_data->packet.num_bytes = sizeof(id);

	rc = mspi_transceive(dev_config->bus, &dev_config->mspi_id,
			     &dev_data->xfer);
	if (rc < 0) {
		LOG_ERR("Failed to read JEDEC ID in initial line mode: %d", rc);
		return rc;
	}

	rc = default_io_mode(dev);

	if (rc < 0) {
		LOG_ERR("Failed to switch to default io mode: %d", rc);
		return rc;
	}

	/* Reading JEDEC ID for mode that forces single lane would be redundant,
	 * since it switches back to single lane mode. Use ID from previous read.
	 */
	if (!dev_config->jedec_cmds->id.force_single) {
		rc = read_jedec_id(dev, id);
		if (rc < 0) {
			LOG_ERR("Failed to read JEDEC ID in final line mode: %d", rc);
			return rc;
		}
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

	rc = pm_device_runtime_get(dev_config->bus);
	if (rc < 0) {
		LOG_ERR("pm_device_runtime_get() failed: %d", rc);
		return rc;
	}

	rc = flash_chip_init(dev);

	/* Release the MSPI controller - it was acquired by the call to
	 * mspi_dev_config() in flash_chip_init().
	 */
	(void)mspi_get_channel_status(dev_config->bus, 0);

	(void)pm_device_runtime_put(dev_config->bus);

	if (rc < 0) {
		return rc;
	}

	k_sem_init(&dev_data->acquired, 1, K_SEM_MAX_LIMIT);

	return pm_device_driver_init(dev, dev_pm_action_cb);
}

static DEVICE_API(flash, drv_api) = {
	.read = api_read,
	.write = api_write,
	.erase = api_erase,
	.get_size = api_get_size,
	.get_parameters = api_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = api_page_layout,
#endif
#if defined(CONFIG_FLASH_JESD216_API)
	.sfdp_read = api_sfdp_read,
	.read_jedec_id = api_read_jedec_id,
#endif
};

#define FLASH_INITIAL_CONFIG(inst)					\
{									\
	.ce_num = DT_INST_PROP_OR(inst, mspi_hardware_ce_num, 0),	\
	.freq = MIN(DT_INST_PROP(inst, mspi_max_frequency), MHZ(50)),	\
	.io_mode = MSPI_IO_MODE_SINGLE,					\
	.data_rate = MSPI_DATA_RATE_SINGLE,				\
	.cpp = MSPI_CPP_MODE_0,						\
	.endian = MSPI_XFER_BIG_ENDIAN,					\
	.ce_polarity = MSPI_CE_ACTIVE_LOW,				\
	.dqs_enable = false,						\
}

#define FLASH_SIZE_INST(inst) (DT_INST_PROP(inst, size) / 8)

/* Define copies of mspi_io_mode enum values, so they can be used inside
 * the COND_CODE_1 macros.
 */
#define _MSPI_IO_MODE_SINGLE 0
#define _MSPI_IO_MODE_QUAD_1_4_4 6
#define _MSPI_IO_MODE_OCTAL 7
BUILD_ASSERT(_MSPI_IO_MODE_SINGLE == MSPI_IO_MODE_SINGLE,
	"Please align _MSPI_IO_MODE_SINGLE macro value");
BUILD_ASSERT(_MSPI_IO_MODE_QUAD_1_4_4 == MSPI_IO_MODE_QUAD_1_4_4,
	"Please align _MSPI_IO_MODE_QUAD_1_4_4 macro value");
BUILD_ASSERT(_MSPI_IO_MODE_OCTAL == MSPI_IO_MODE_OCTAL,
	"Please align _MSPI_IO_MODE_OCTAL macro value");

/* Define a non-existing extern symbol to get an understandable compile-time error
 * if the IO mode is not supported by the driver.
 */
extern const struct flash_mspi_nor_cmds mspi_io_mode_not_supported;

#define FLASH_CMDS(inst) COND_CODE_1( \
	IS_EQ(DT_INST_ENUM_IDX(inst, mspi_io_mode), _MSPI_IO_MODE_SINGLE), \
	(&commands_single), \
	(COND_CODE_1( \
		IS_EQ(DT_INST_ENUM_IDX(inst, mspi_io_mode), _MSPI_IO_MODE_QUAD_1_4_4), \
		(&commands_quad_1_4_4), \
		(COND_CODE_1( \
			IS_EQ(DT_INST_ENUM_IDX(inst, mspi_io_mode), _MSPI_IO_MODE_OCTAL), \
			(&commands_octal), \
			(&mspi_io_mode_not_supported) \
		)) \
	)) \
)

#define FLASH_QUIRKS(inst) FLASH_MSPI_QUIRKS_GET(DT_DRV_INST(inst))

#define FLASH_DW15_QER_VAL(inst) _CONCAT(JESD216_DW15_QER_VAL_, \
	DT_INST_STRING_TOKEN(inst, quad_enable_requirements))
#define FLASH_DW15_QER(inst) COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, quad_enable_requirements), \
	(FLASH_DW15_QER_VAL(inst)), (JESD216_DW15_QER_VAL_NONE))


#if defined(CONFIG_FLASH_PAGE_LAYOUT)
BUILD_ASSERT((CONFIG_FLASH_MSPI_NOR_LAYOUT_PAGE_SIZE % 4096) == 0,
	"MSPI_NOR_FLASH_LAYOUT_PAGE_SIZE must be multiple of 4096");
#define FLASH_PAGE_LAYOUT_DEFINE(inst) \
	.layout = { \
		.pages_size = CONFIG_FLASH_MSPI_NOR_LAYOUT_PAGE_SIZE, \
		.pages_count = FLASH_SIZE_INST(inst) \
			     / CONFIG_FLASH_MSPI_NOR_LAYOUT_PAGE_SIZE, \
	},
#define FLASH_PAGE_LAYOUT_CHECK(inst) \
BUILD_ASSERT((FLASH_SIZE_INST(inst) % CONFIG_FLASH_MSPI_NOR_LAYOUT_PAGE_SIZE) == 0, \
	"MSPI_NOR_FLASH_LAYOUT_PAGE_SIZE incompatible with flash size, instance " #inst);
#else
#define FLASH_PAGE_LAYOUT_DEFINE(inst)
#define FLASH_PAGE_LAYOUT_CHECK(inst)
#endif

/* MSPI bus must be initialized before this device. */
#if (CONFIG_MSPI_INIT_PRIORITY < CONFIG_FLASH_INIT_PRIORITY)
#define INIT_PRIORITY CONFIG_FLASH_INIT_PRIORITY
#else
#define INIT_PRIORITY UTIL_INC(CONFIG_MSPI_INIT_PRIORITY)
#endif

#define FLASH_MSPI_NOR_INST(inst)						\
	BUILD_ASSERT((DT_INST_ENUM_IDX(inst, mspi_io_mode) ==			\
		      MSPI_IO_MODE_SINGLE) ||					\
		     (DT_INST_ENUM_IDX(inst, mspi_io_mode) ==			\
		      MSPI_IO_MODE_QUAD_1_4_4) ||				\
		     (DT_INST_ENUM_IDX(inst, mspi_io_mode) ==			\
		      MSPI_IO_MODE_OCTAL),					\
		"Only 1x, 1-4-4 and 8x I/O modes are supported for now");	\
	PM_DEVICE_DT_INST_DEFINE(inst, dev_pm_action_cb);			\
	static struct flash_mspi_nor_data dev##inst##_data;			\
	static const struct flash_mspi_nor_config dev##inst##_config = {	\
		.bus = DEVICE_DT_GET(DT_INST_BUS(inst)),			\
		.flash_size = FLASH_SIZE_INST(inst),				\
		.mspi_id = MSPI_DEVICE_ID_DT_INST(inst),			\
		.mspi_nor_cfg = MSPI_DEVICE_CONFIG_DT_INST(inst),		\
		.mspi_nor_init_cfg = FLASH_INITIAL_CONFIG(inst),		\
		.mspi_nor_cfg_mask = DT_PROP(DT_INST_BUS(inst),			\
					 software_multiperipheral)		\
			       ? MSPI_DEVICE_CONFIG_ALL				\
			       : MSPI_DEVICE_CONFIG_NONE,			\
	IF_ENABLED(CONFIG_MSPI_XIP,						\
		(.xip_cfg = MSPI_XIP_CONFIG_DT_INST(inst),))			\
	IF_ENABLED(WITH_RESET_GPIO,						\
		(.reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),	\
		.reset_pulse_us = DT_INST_PROP_OR(inst, t_reset_pulse, 0)	\
				/ 1000,						\
		.reset_recovery_us = DT_INST_PROP_OR(inst, t_reset_recovery, 0)	\
				   / 1000,))					\
		.transfer_timeout = DT_INST_PROP(inst, transfer_timeout),	\
		FLASH_PAGE_LAYOUT_DEFINE(inst)					\
		.jedec_id = DT_INST_PROP(inst, jedec_id),			\
		.jedec_cmds = FLASH_CMDS(inst),					\
		.quirks = FLASH_QUIRKS(inst),					\
		.dw15_qer = FLASH_DW15_QER(inst),				\
	};									\
	FLASH_PAGE_LAYOUT_CHECK(inst)						\
	DEVICE_DT_INST_DEFINE(inst,						\
		drv_init, PM_DEVICE_DT_INST_GET(inst),				\
		&dev##inst##_data, &dev##inst##_config,				\
		POST_KERNEL, INIT_PRIORITY,					\
		&drv_api);

DT_INST_FOREACH_STATUS_OKAY(FLASH_MSPI_NOR_INST)

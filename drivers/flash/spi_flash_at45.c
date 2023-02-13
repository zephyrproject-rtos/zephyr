/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(spi_flash_at45, CONFIG_FLASH_LOG_LEVEL);

#define DT_DRV_COMPAT atmel_at45

/* AT45 commands used by this driver: */
/* - Continuous Array Read (Low Power Mode) */
#define CMD_READ		0x01
/* - Main Memory Byte/Page Program through Buffer 1 without Built-In Erase */
#define CMD_WRITE		0x02
/* - Read-Modify-Write */
#define CMD_MODIFY		0x58
/* - Manufacturer and Device ID Read */
#define CMD_READ_ID		0x9F
/* - Status Register Read */
#define CMD_READ_STATUS		0xD7
/* - Chip Erase */
#define CMD_CHIP_ERASE		{ 0xC7, 0x94, 0x80, 0x9A }
/* - Sector Erase */
#define CMD_SECTOR_ERASE	0x7C
/* - Block Erase */
#define CMD_BLOCK_ERASE		0x50
/* - Page Erase */
#define CMD_PAGE_ERASE		0x81
/* - Deep Power-Down */
#define CMD_ENTER_DPD		0xB9
/* - Resume from Deep Power-Down */
#define CMD_EXIT_DPD		0xAB
/* - Ultra-Deep Power-Down */
#define CMD_ENTER_UDPD		0x79
/* - Buffer and Page Size Configuration, "Power of 2" binary page size */
#define CMD_BINARY_PAGE_SIZE	{ 0x3D, 0x2A, 0x80, 0xA6 }

#define STATUS_REG_LSB_RDY_BUSY_BIT	0x80
#define STATUS_REG_LSB_PAGE_SIZE_BIT	0x01

#define INST_HAS_WP_OR(inst) DT_INST_NODE_HAS_PROP(inst, wp_gpios) ||
#define ANY_INST_HAS_WP_GPIOS DT_INST_FOREACH_STATUS_OKAY(INST_HAS_WP_OR) 0

#define INST_HAS_RESET_OR(inst) DT_INST_NODE_HAS_PROP(inst, reset_gpios) ||
#define ANY_INST_HAS_RESET_GPIOS DT_INST_FOREACH_STATUS_OKAY(INST_HAS_RESET_OR) 0

#define DEF_BUF_SET(_name, _buf_array) \
	const struct spi_buf_set _name = { \
		.buffers = _buf_array, \
		.count   = ARRAY_SIZE(_buf_array), \
	}

struct spi_flash_at45_data {
	struct k_sem lock;
};

struct spi_flash_at45_config {
	struct spi_dt_spec bus;
#if ANY_INST_HAS_RESET_GPIOS
	const struct gpio_dt_spec *reset;
#endif
#if ANY_INST_HAS_WP_GPIOS
	const struct gpio_dt_spec *wp;
#endif
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	struct flash_pages_layout pages_layout;
#endif
	uint32_t chip_size;
	uint32_t sector_size;
	uint16_t block_size;
	uint16_t page_size;
	uint16_t t_enter_dpd; /* in microseconds */
	uint16_t t_exit_dpd;  /* in microseconds */
	bool use_udpd;
	uint8_t jedec_id[3];
};

static const struct flash_parameters flash_at45_parameters = {
	.write_block_size = 1,
	.erase_value = 0xff,
};

static void acquire(const struct device *dev)
{
	struct spi_flash_at45_data *data = dev->data;

	k_sem_take(&data->lock, K_FOREVER);
}

static void release(const struct device *dev)
{
	struct spi_flash_at45_data *data = dev->data;

	k_sem_give(&data->lock);
}

static int check_jedec_id(const struct device *dev)
{
	const struct spi_flash_at45_config *cfg = dev->config;
	int err;
	uint8_t const *expected_id = cfg->jedec_id;
	uint8_t read_id[sizeof(cfg->jedec_id)];
	const uint8_t opcode = CMD_READ_ID;
	const struct spi_buf tx_buf[] = {
		{
			.buf = (void *)&opcode,
			.len = sizeof(opcode),
		}
	};
	const struct spi_buf rx_buf[] = {
		{
			.len = sizeof(opcode),
		},
		{
			.buf = read_id,
			.len = sizeof(read_id),
		}
	};
	DEF_BUF_SET(tx_buf_set, tx_buf);
	DEF_BUF_SET(rx_buf_set, rx_buf);

	err = spi_transceive_dt(&cfg->bus, &tx_buf_set, &rx_buf_set);
	if (err != 0) {
		LOG_ERR("SPI transaction failed with code: %d/%u",
			err, __LINE__);
		return -EIO;
	}

	if (memcmp(expected_id, read_id, sizeof(read_id)) != 0) {
		LOG_ERR("Wrong JEDEC ID: %02X %02X %02X, "
			"expected: %02X %02X %02X",
			read_id[0], read_id[1], read_id[2],
			expected_id[0], expected_id[1], expected_id[2]);
		return -ENODEV;
	}

	return 0;
}

/*
 * Reads 2-byte Status Register:
 * - Byte 0 to LSB
 * - Byte 1 to MSB
 * of the pointed parameter.
 */
static int read_status_register(const struct device *dev, uint16_t *status)
{
	const struct spi_flash_at45_config *cfg = dev->config;
	int err;
	const uint8_t opcode = CMD_READ_STATUS;
	const struct spi_buf tx_buf[] = {
		{
			.buf = (void *)&opcode,
			.len = sizeof(opcode),
		}
	};
	const struct spi_buf rx_buf[] = {
		{
			.len = sizeof(opcode),
		},
		{
			.buf = status,
			.len = sizeof(uint16_t),
		}
	};
	DEF_BUF_SET(tx_buf_set, tx_buf);
	DEF_BUF_SET(rx_buf_set, rx_buf);

	err = spi_transceive_dt(&cfg->bus, &tx_buf_set, &rx_buf_set);
	if (err != 0) {
		LOG_ERR("SPI transaction failed with code: %d/%u",
			err, __LINE__);
		return -EIO;
	}

	*status = sys_le16_to_cpu(*status);
	return 0;
}

static int wait_until_ready(const struct device *dev)
{
	int err;
	uint16_t status;

	do {
		err = read_status_register(dev, &status);
	} while (err == 0 && !(status & STATUS_REG_LSB_RDY_BUSY_BIT));

	return err;
}

static int configure_page_size(const struct device *dev)
{
	const struct spi_flash_at45_config *cfg = dev->config;
	int err;
	uint16_t status;
	uint8_t const conf_binary_page_size[] = CMD_BINARY_PAGE_SIZE;
	const struct spi_buf tx_buf[] = {
		{
			.buf = (void *)conf_binary_page_size,
			.len = sizeof(conf_binary_page_size),
		}
	};
	DEF_BUF_SET(tx_buf_set, tx_buf);

	err = read_status_register(dev, &status);
	if (err != 0) {
		return err;
	}

	/* If the device is already configured for "power of 2" binary
	 * page size, there is nothing more to do.
	 */
	if (status & STATUS_REG_LSB_PAGE_SIZE_BIT) {
		return 0;
	}

	err = spi_write_dt(&cfg->bus, &tx_buf_set);
	if (err != 0) {
		LOG_ERR("SPI transaction failed with code: %d/%u",
			err, __LINE__);
	} else {
		err = wait_until_ready(dev);
	}

	return (err != 0) ? -EIO : 0;
}

static bool is_valid_request(off_t addr, size_t size, size_t chip_size)
{
	return (addr >= 0 && (addr + size) <= chip_size);
}

static int spi_flash_at45_read(const struct device *dev, off_t offset,
			       void *data, size_t len)
{
	const struct spi_flash_at45_config *cfg = dev->config;
	int err;

	if (!is_valid_request(offset, len, cfg->chip_size)) {
		return -ENODEV;
	}

	uint8_t const op_and_addr[] = {
		CMD_READ,
		(offset >> 16) & 0xFF,
		(offset >> 8)  & 0xFF,
		(offset >> 0)  & 0xFF,
	};
	const struct spi_buf tx_buf[] = {
		{
			.buf = (void *)&op_and_addr,
			.len = sizeof(op_and_addr),
		}
	};
	const struct spi_buf rx_buf[] = {
		{
			.len = sizeof(op_and_addr),
		},
		{
			.buf = data,
			.len = len,
		}
	};
	DEF_BUF_SET(tx_buf_set, tx_buf);
	DEF_BUF_SET(rx_buf_set, rx_buf);

	acquire(dev);
	err = spi_transceive_dt(&cfg->bus, &tx_buf_set, &rx_buf_set);
	release(dev);

	if (err != 0) {
		LOG_ERR("SPI transaction failed with code: %d/%u",
			err, __LINE__);
	}

	return (err != 0) ? -EIO : 0;
}

static int perform_write(const struct device *dev, off_t offset,
			 const void *data, size_t len)
{
	const struct spi_flash_at45_config *cfg = dev->config;
	int err;
	uint8_t const op_and_addr[] = {
		IS_ENABLED(CONFIG_SPI_FLASH_AT45_USE_READ_MODIFY_WRITE)
			? CMD_MODIFY
			: CMD_WRITE,
		(offset >> 16) & 0xFF,
		(offset >> 8)  & 0xFF,
		(offset >> 0)  & 0xFF,
	};
	const struct spi_buf tx_buf[] = {
		{
			.buf = (void *)&op_and_addr,
			.len = sizeof(op_and_addr),
		},
		{
			.buf = (void *)data,
			.len = len,
		}
	};
	DEF_BUF_SET(tx_buf_set, tx_buf);

	err = spi_write_dt(&cfg->bus, &tx_buf_set);
	if (err != 0) {
		LOG_ERR("SPI transaction failed with code: %d/%u",
			err, __LINE__);
	} else {
		err = wait_until_ready(dev);
	}

	return (err != 0) ? -EIO : 0;
}

static int spi_flash_at45_write(const struct device *dev, off_t offset,
				const void *data, size_t len)
{
	const struct spi_flash_at45_config *cfg = dev->config;
	int err = 0;

	if (!is_valid_request(offset, len, cfg->chip_size)) {
		return -ENODEV;
	}

	acquire(dev);

#if ANY_INST_HAS_WP_GPIOS
	if (cfg->wp) {
		gpio_pin_set_dt(cfg->wp, 0);
	}
#endif

	while (len) {
		size_t chunk_len = len;
		off_t current_page_start =
			offset - (offset & (cfg->page_size - 1));
		off_t current_page_end = current_page_start + cfg->page_size;

		if (chunk_len > (current_page_end - offset)) {
			chunk_len = (current_page_end - offset);
		}

		err = perform_write(dev, offset, data, chunk_len);
		if (err != 0) {
			break;
		}

		data    = (uint8_t *)data + chunk_len;
		offset += chunk_len;
		len    -= chunk_len;
	}

#if ANY_INST_HAS_WP_GPIOS
	if (cfg->wp) {
		gpio_pin_set_dt(cfg->wp, 1);
	}
#endif

	release(dev);

	return err;
}

static int perform_chip_erase(const struct device *dev)
{
	const struct spi_flash_at45_config *cfg = dev->config;
	int err;
	uint8_t const chip_erase_cmd[] = CMD_CHIP_ERASE;
	const struct spi_buf tx_buf[] = {
		{
			.buf = (void *)&chip_erase_cmd,
			.len = sizeof(chip_erase_cmd),
		}
	};
	DEF_BUF_SET(tx_buf_set, tx_buf);

	err = spi_write_dt(&cfg->bus, &tx_buf_set);
	if (err != 0) {
		LOG_ERR("SPI transaction failed with code: %d/%u",
			err, __LINE__);
	} else {
		err = wait_until_ready(dev);
	}

	return (err != 0) ? -EIO : 0;
}

static bool is_erase_possible(size_t entity_size,
			      off_t offset, size_t requested_size)
{
	return (requested_size >= entity_size &&
		(offset & (entity_size - 1)) == 0);
}

static int perform_erase_op(const struct device *dev, uint8_t opcode,
			    off_t offset)
{
	const struct spi_flash_at45_config *cfg = dev->config;
	int err;
	uint8_t const op_and_addr[] = {
		opcode,
		(offset >> 16) & 0xFF,
		(offset >> 8)  & 0xFF,
		(offset >> 0)  & 0xFF,
	};
	const struct spi_buf tx_buf[] = {
		{
			.buf = (void *)&op_and_addr,
			.len = sizeof(op_and_addr),
		}
	};
	DEF_BUF_SET(tx_buf_set, tx_buf);

	err = spi_write_dt(&cfg->bus, &tx_buf_set);
	if (err != 0) {
		LOG_ERR("SPI transaction failed with code: %d/%u",
			err, __LINE__);
	} else {
		err = wait_until_ready(dev);
	}

	return (err != 0) ? -EIO : 0;
}

static int spi_flash_at45_erase(const struct device *dev, off_t offset,
				size_t size)
{
	const struct spi_flash_at45_config *cfg = dev->config;
	int err = 0;

	if (!is_valid_request(offset, size, cfg->chip_size)) {
		return -ENODEV;
	}

	/* Diagnose region errors before starting to erase. */
	if (((offset % cfg->page_size) != 0)
	    || ((size % cfg->page_size) != 0)) {
		return -EINVAL;
	}

	acquire(dev);

#if ANY_INST_HAS_WP_GPIOS
	if (cfg->wp) {
		gpio_pin_set_dt(cfg->wp, 0);
	}
#endif

	if (size == cfg->chip_size) {
		err = perform_chip_erase(dev);
	} else {
		while (size) {
			if (is_erase_possible(cfg->sector_size,
					      offset, size)) {
				err = perform_erase_op(dev, CMD_SECTOR_ERASE,
						       offset);
				offset += cfg->sector_size;
				size   -= cfg->sector_size;
			} else if (is_erase_possible(cfg->block_size,
						     offset, size)) {
				err = perform_erase_op(dev, CMD_BLOCK_ERASE,
						       offset);
				offset += cfg->block_size;
				size   -= cfg->block_size;
			} else if (is_erase_possible(cfg->page_size,
						     offset, size)) {
				err = perform_erase_op(dev, CMD_PAGE_ERASE,
						       offset);
				offset += cfg->page_size;
				size   -= cfg->page_size;
			} else {
				LOG_ERR("Unsupported erase request: "
					"size %zu at 0x%lx",
					size, (long)offset);
				err = -EINVAL;
			}

			if (err != 0) {
				break;
			}
		}
	}

#if ANY_INST_HAS_WP_GPIOS
	if (cfg->wp) {
		gpio_pin_set_dt(cfg->wp, 1);
	}
#endif

	release(dev);

	return err;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void spi_flash_at45_pages_layout(const struct device *dev,
					const struct flash_pages_layout **layout,
					size_t *layout_size)
{
	const struct spi_flash_at45_config *cfg = dev->config;

	*layout = &cfg->pages_layout;
	*layout_size = 1;
}
#endif /* IS_ENABLED(CONFIG_FLASH_PAGE_LAYOUT) */

static int power_down_op(const struct device *dev, uint8_t opcode,
			 uint32_t delay)
{
	const struct spi_flash_at45_config *cfg = dev->config;
	int err = 0;
	const struct spi_buf tx_buf[] = {
		{
			.buf = (void *)&opcode,
			.len = sizeof(opcode),
		}
	};
	DEF_BUF_SET(tx_buf_set, tx_buf);

	err = spi_write_dt(&cfg->bus, &tx_buf_set);
	if (err != 0) {
		LOG_ERR("SPI transaction failed with code: %d/%u",
			err, __LINE__);
		return -EIO;
	}


	k_busy_wait(delay);
	return 0;
}

static int spi_flash_at45_init(const struct device *dev)
{
	const struct spi_flash_at45_config *dev_config = dev->config;
	int err;

	if (!spi_is_ready_dt(&dev_config->bus)) {
		LOG_ERR("SPI bus %s not ready", dev_config->bus.bus->name);
		return -ENODEV;
	}

#if ANY_INST_HAS_RESET_GPIOS
	if (dev_config->reset) {
		if (!device_is_ready(dev_config->reset->port)) {
			LOG_ERR("Reset pin not ready");
			return -ENODEV;
		}
		if (gpio_pin_configure_dt(dev_config->reset, GPIO_OUTPUT_ACTIVE)) {
			LOG_ERR("Couldn't configure reset pin");
			return -ENODEV;
		}
		gpio_pin_set_dt(dev_config->reset, 0);
	}
#endif

#if ANY_INST_HAS_WP_GPIOS
	if (dev_config->wp) {
		if (!device_is_ready(dev_config->wp->port)) {
			LOG_ERR("Write protect pin not ready");
			return -ENODEV;
		}
		if (gpio_pin_configure_dt(dev_config->wp, GPIO_OUTPUT_ACTIVE)) {
			LOG_ERR("Couldn't configure write protect pin");
			return -ENODEV;
		}
	}
#endif

	acquire(dev);

	/* Just in case the chip was in the Deep (or Ultra-Deep) Power-Down
	 * mode, issue the command to bring it back to normal operation.
	 * Exiting from the Ultra-Deep mode requires only that the CS line
	 * is asserted for a certain time, so issuing the Resume from Deep
	 * Power-Down command will work in both cases.
	 */
	power_down_op(dev, CMD_EXIT_DPD, dev_config->t_exit_dpd);

	err = check_jedec_id(dev);
	if (err == 0) {
		err = configure_page_size(dev);
	}

	release(dev);

	return err;
}

#if defined(CONFIG_PM_DEVICE)
static int spi_flash_at45_pm_action(const struct device *dev,
				    enum pm_device_action action)
{
	const struct spi_flash_at45_config *dev_config = dev->config;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		acquire(dev);
		power_down_op(dev, CMD_EXIT_DPD, dev_config->t_exit_dpd);
		release(dev);
		break;

	case PM_DEVICE_ACTION_SUSPEND:
		acquire(dev);
		power_down_op(dev,
			dev_config->use_udpd ? CMD_ENTER_UDPD : CMD_ENTER_DPD,
			dev_config->t_enter_dpd);
		release(dev);
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /* IS_ENABLED(CONFIG_PM_DEVICE) */

static const struct flash_parameters *
flash_at45_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_at45_parameters;
}

static const struct flash_driver_api spi_flash_at45_api = {
	.read = spi_flash_at45_read,
	.write = spi_flash_at45_write,
	.erase = spi_flash_at45_erase,
	.get_parameters = flash_at45_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = spi_flash_at45_pages_layout,
#endif
};

#define INST_HAS_RESET_GPIO(idx) \
	DT_INST_NODE_HAS_PROP(idx, reset_gpios)

#define INST_RESET_GPIO_SPEC(idx)					\
	IF_ENABLED(INST_HAS_RESET_GPIO(idx),				\
		(static const struct gpio_dt_spec reset_##idx =	\
		GPIO_DT_SPEC_INST_GET(idx, reset_gpios);))

#define INST_HAS_WP_GPIO(idx) \
	DT_INST_NODE_HAS_PROP(idx, wp_gpios)

#define INST_WP_GPIO_SPEC(idx)						\
	IF_ENABLED(INST_HAS_WP_GPIO(idx),				\
		(static const struct gpio_dt_spec wp_##idx =		\
		GPIO_DT_SPEC_INST_GET(idx, wp_gpios);))

#define SPI_FLASH_AT45_INST(idx)					     \
	enum {								     \
		INST_##idx##_BYTES = (DT_INST_PROP(idx, size) / 8),	     \
		INST_##idx##_PAGES = (INST_##idx##_BYTES /		     \
				      DT_INST_PROP(idx, page_size)),	     \
	};								     \
	static struct spi_flash_at45_data inst_##idx##_data = {		     \
		.lock = Z_SEM_INITIALIZER(inst_##idx##_data.lock, 1, 1),     \
	};						\
	INST_RESET_GPIO_SPEC(idx)				\
	INST_WP_GPIO_SPEC(idx)					\
	static const struct spi_flash_at45_config inst_##idx##_config = {    \
		.bus = SPI_DT_SPEC_INST_GET(				     \
			idx, SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB |	     \
			SPI_WORD_SET(8), 0),				     \
		IF_ENABLED(INST_HAS_RESET_GPIO(idx),			\
			(.reset = &reset_##idx,))			\
		IF_ENABLED(INST_HAS_WP_GPIO(idx),			\
			(.wp = &wp_##idx,))			\
		IF_ENABLED(CONFIG_FLASH_PAGE_LAYOUT, (			     \
			.pages_layout = {				     \
				.pages_count = INST_##idx##_PAGES,	     \
				.pages_size  = DT_INST_PROP(idx, page_size), \
			},))						     \
		.chip_size   = INST_##idx##_BYTES,			     \
		.sector_size = DT_INST_PROP(idx, sector_size),		     \
		.block_size  = DT_INST_PROP(idx, block_size),		     \
		.page_size   = DT_INST_PROP(idx, page_size),		     \
		.t_enter_dpd = ceiling_fraction(			     \
					DT_INST_PROP(idx, enter_dpd_delay),  \
					NSEC_PER_USEC),			     \
		.t_exit_dpd  = ceiling_fraction(			     \
					DT_INST_PROP(idx, exit_dpd_delay),   \
					NSEC_PER_USEC),			     \
		.use_udpd    = DT_INST_PROP(idx, use_udpd),		     \
		.jedec_id    = DT_INST_PROP(idx, jedec_id),		     \
	};								     \
	IF_ENABLED(CONFIG_FLASH_PAGE_LAYOUT, (				     \
		BUILD_ASSERT(						     \
			(INST_##idx##_PAGES * DT_INST_PROP(idx, page_size))  \
			== INST_##idx##_BYTES,				     \
			"Page size specified for instance " #idx " of "	     \
			"atmel,at45 is not compatible with its "	     \
			"total size");))				     \
									     \
	PM_DEVICE_DT_INST_DEFINE(idx, spi_flash_at45_pm_action);	     \
									     \
	DEVICE_DT_INST_DEFINE(idx,					     \
		      spi_flash_at45_init, PM_DEVICE_DT_INST_GET(idx),	     \
		      &inst_##idx##_data, &inst_##idx##_config,		     \
		      POST_KERNEL, CONFIG_SPI_FLASH_AT45_INIT_PRIORITY,      \
		      &spi_flash_at45_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_FLASH_AT45_INST)

/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/flash.h>
#include <drivers/spi.h>
#include <sys/byteorder.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(spi_flash_at45, CONFIG_FLASH_LOG_LEVEL);

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

#define AT45_SECTOR_SIZE	0x10000UL

#define STATUS_REG_LSB_RDY_BUSY_BIT	0x80
#define STATUS_REG_LSB_PAGE_SIZE_BIT	0x01


#define DEF_BUF_SET(_name, _buf_array) \
	const struct spi_buf_set _name = { \
		.buffers = _buf_array, \
		.count   = ARRAY_SIZE(_buf_array), \
	}

struct spi_flash_at45_data {
	struct device *spi;
	struct spi_cs_control spi_cs;
	struct k_sem lock;
#if IS_ENABLED(CONFIG_DEVICE_POWER_MANAGEMENT)
	uint32_t pm_state;
#endif
};

struct spi_flash_at45_config {
	const char *spi_bus;
	struct spi_config spi_cfg;
	const char *cs_gpio;
	gpio_pin_t cs_pin;
	gpio_dt_flags_t cs_dt_flags;
#if IS_ENABLED(CONFIG_FLASH_PAGE_LAYOUT)
	struct flash_pages_layout pages_layout;
#endif
	uint32_t chip_size;
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

static struct spi_flash_at45_data *get_dev_data(struct device *dev)
{
	return dev->data;
}

static const struct spi_flash_at45_config *get_dev_config(struct device *dev)
{
	return dev->config;
}

static void acquire(struct device *dev)
{
	k_sem_take(&get_dev_data(dev)->lock, K_FOREVER);
}

static void release(struct device *dev)
{
	k_sem_give(&get_dev_data(dev)->lock);
}

static int check_jedec_id(struct device *dev)
{
	const struct spi_flash_at45_config *cfg = get_dev_config(dev);
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

	err = spi_transceive(get_dev_data(dev)->spi,
			     &cfg->spi_cfg,
			     &tx_buf_set, &rx_buf_set);
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
static int read_status_register(struct device *dev, uint16_t *status)
{
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

	err = spi_transceive(get_dev_data(dev)->spi,
			     &get_dev_config(dev)->spi_cfg,
			     &tx_buf_set, &rx_buf_set);
	if (err != 0) {
		LOG_ERR("SPI transaction failed with code: %d/%u",
			err, __LINE__);
		return -EIO;
	}

	*status = sys_le16_to_cpu(*status);
	return 0;
}

static int wait_until_ready(struct device *dev)
{
	int err;
	uint16_t status;

	do {
		err = read_status_register(dev, &status);
	} while (err == 0 && !(status & STATUS_REG_LSB_RDY_BUSY_BIT));

	return err;
}

static int configure_page_size(struct device *dev)
{
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

	err = spi_write(get_dev_data(dev)->spi,
			&get_dev_config(dev)->spi_cfg,
			&tx_buf_set);
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

static int spi_flash_at45_read(struct device *dev, off_t offset,
			       void *data, size_t len)
{
	const struct spi_flash_at45_config *cfg = get_dev_config(dev);
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
	err = spi_transceive(get_dev_data(dev)->spi,
			     &cfg->spi_cfg,
			     &tx_buf_set, &rx_buf_set);
	release(dev);

	if (err != 0) {
		LOG_ERR("SPI transaction failed with code: %d/%u",
			err, __LINE__);
	}

	return (err != 0) ? -EIO : 0;
}

static int perform_write(struct device *dev, off_t offset,
			 const void *data, size_t len)
{
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

	err = spi_write(get_dev_data(dev)->spi,
			&get_dev_config(dev)->spi_cfg,
			&tx_buf_set);
	if (err != 0) {
		LOG_ERR("SPI transaction failed with code: %d/%u",
			err, __LINE__);
	} else {
		err = wait_until_ready(dev);
	}

	return (err != 0) ? -EIO : 0;
}

static int spi_flash_at45_write(struct device *dev, off_t offset,
				const void *data, size_t len)
{
	const struct spi_flash_at45_config *cfg = get_dev_config(dev);
	int err = 0;

	if (!is_valid_request(offset, len, cfg->chip_size)) {
		return -ENODEV;
	}

	acquire(dev);

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

	release(dev);

	return err;
}

static int perform_chip_erase(struct device *dev)
{
	int err;
	uint8_t const chip_erase_cmd[] = CMD_CHIP_ERASE;
	const struct spi_buf tx_buf[] = {
		{
			.buf = (void *)&chip_erase_cmd,
			.len = sizeof(chip_erase_cmd),
		}
	};
	DEF_BUF_SET(tx_buf_set, tx_buf);

	err = spi_write(get_dev_data(dev)->spi,
			&get_dev_config(dev)->spi_cfg,
			&tx_buf_set);
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

static int perform_erase_op(struct device *dev, uint8_t opcode, off_t offset)
{
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

	err = spi_write(get_dev_data(dev)->spi,
			&get_dev_config(dev)->spi_cfg,
			&tx_buf_set);
	if (err != 0) {
		LOG_ERR("SPI transaction failed with code: %d/%u",
			err, __LINE__);
	} else {
		err = wait_until_ready(dev);
	}

	return (err != 0) ? -EIO : 0;
}

static int spi_flash_at45_erase(struct device *dev, off_t offset, size_t size)
{
	const struct spi_flash_at45_config *cfg = get_dev_config(dev);
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

	if (size == cfg->chip_size) {
		err = perform_chip_erase(dev);
	} else {
		while (size) {
			if (is_erase_possible(AT45_SECTOR_SIZE,
					      offset, size)) {
				err = perform_erase_op(dev, CMD_SECTOR_ERASE,
						       offset);
				offset += AT45_SECTOR_SIZE;
				size   -= AT45_SECTOR_SIZE;
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

	release(dev);

	return err;
}

static int spi_flash_at45_write_protection(struct device *dev, bool enable)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(enable);

	/* The Sector Protection mechanism that is available in AT45 family
	 * chips is more complex than what is exposed by the the flash API
	 * (particular sectors need to be earlier configured in a write to
	 * the nonvolatile Sector Protection Register), so it is not feasible
	 * to try to use it here. Since the protection is not automatically
	 * enabled after the device is power cycled, there is nothing needed
	 * to be done in this function.
	 */

	return 0;
}

#if IS_ENABLED(CONFIG_FLASH_PAGE_LAYOUT)
static void spi_flash_at45_pages_layout(
				struct device *dev,
				const struct flash_pages_layout **layout,
				size_t *layout_size)
{
	*layout = &get_dev_config(dev)->pages_layout;
	*layout_size = 1;
}
#endif /* IS_ENABLED(CONFIG_FLASH_PAGE_LAYOUT) */

static int power_down_op(struct device *dev, uint8_t opcode, uint32_t delay)
{
	int err = 0;
	const struct spi_buf tx_buf[] = {
		{
			.buf = (void *)&opcode,
			.len = sizeof(opcode),
		}
	};
	DEF_BUF_SET(tx_buf_set, tx_buf);

	err = spi_write(get_dev_data(dev)->spi,
			&get_dev_config(dev)->spi_cfg,
			&tx_buf_set);
	if (err != 0) {
		LOG_ERR("SPI transaction failed with code: %d/%u",
			err, __LINE__);
		return -EIO;
	}


	k_busy_wait(delay);
	return 0;
}

static int spi_flash_at45_init(struct device *dev)
{
	struct spi_flash_at45_data *dev_data = get_dev_data(dev);
	const struct spi_flash_at45_config *dev_config = get_dev_config(dev);
	int err;

	dev_data->spi = device_get_binding(dev_config->spi_bus);
	if (!dev_data->spi) {
		LOG_ERR("Cannot find %s", dev_config->spi_bus);
		return -ENODEV;
	}

	if (dev_config->cs_gpio) {
		dev_data->spi_cs.gpio_dev =
			device_get_binding(dev_config->cs_gpio);
		if (!dev_data->spi_cs.gpio_dev) {
			LOG_ERR("Cannot find %s", dev_config->cs_gpio);
			return -ENODEV;
		}

		dev_data->spi_cs.gpio_pin = dev_config->cs_pin;
		dev_data->spi_cs.gpio_dt_flags = dev_config->cs_dt_flags;
		dev_data->spi_cs.delay = 0;
	}

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

#if IS_ENABLED(CONFIG_DEVICE_POWER_MANAGEMENT)
static int spi_flash_at45_pm_control(struct device *dev, uint32_t ctrl_command,
				     void *context, device_pm_cb cb, void *arg)
{
	struct spi_flash_at45_data *dev_data = get_dev_data(dev);
	const struct spi_flash_at45_config *dev_config = get_dev_config(dev);
	int err = 0;

	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
		uint32_t new_state = *((const uint32_t *)context);

		if (new_state != dev_data->pm_state) {
			switch (new_state) {
			case DEVICE_PM_ACTIVE_STATE:
				acquire(dev);
				power_down_op(dev, CMD_EXIT_DPD,
					      dev_config->t_exit_dpd);
				release(dev);
				break;

			case DEVICE_PM_LOW_POWER_STATE:
			case DEVICE_PM_SUSPEND_STATE:
			case DEVICE_PM_OFF_STATE:
				acquire(dev);
				power_down_op(dev,
					dev_config->use_udpd ? CMD_ENTER_UDPD
							     : CMD_ENTER_DPD,
					dev_config->t_enter_dpd);
				release(dev);
				break;

			default:
				return -ENOTSUP;
			}

			dev_data->pm_state = new_state;
		}
	} else {
		__ASSERT_NO_MSG(ctrl_command == DEVICE_PM_GET_POWER_STATE);
		*((uint32_t *)context) = dev_data->pm_state;
	}

	if (cb) {
		cb(dev, err, context, arg);
	}

	return err;
}
#endif /* IS_ENABLED(CONFIG_DEVICE_POWER_MANAGEMENT) */

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
	.write_protection = spi_flash_at45_write_protection,
	.get_parameters = flash_at45_get_parameters,
#if IS_ENABLED(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = spi_flash_at45_pages_layout,
#endif
};

#define DT_DRV_COMPAT atmel_at45

#define SPI_FLASH_AT45_INST(idx)					     \
	enum {								     \
		INST_##idx##_BYTES = (DT_INST_PROP(idx, size) / 8),	     \
		INST_##idx##_PAGES = (INST_##idx##_BYTES /		     \
				      DT_INST_PROP(idx, page_size)),	     \
	};								     \
	static struct spi_flash_at45_data inst_##idx##_data = {		     \
		.lock = Z_SEM_INITIALIZER(inst_##idx##_data.lock, 1, 1),     \
		IF_ENABLED(CONFIG_DEVICE_POWER_MANAGEMENT, (		     \
			.pm_state = DEVICE_PM_ACTIVE_STATE))		     \
	};								     \
	static const struct spi_flash_at45_config inst_##idx##_config = {    \
		.spi_bus = DT_INST_BUS_LABEL(idx),			     \
		.spi_cfg = {						     \
			.frequency = DT_INST_PROP(idx, spi_max_frequency),   \
			.operation = SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | \
				     SPI_WORD_SET(8) | SPI_LINES_SINGLE,     \
			.slave = DT_INST_REG_ADDR(idx),			     \
			.cs = &inst_##idx##_data.spi_cs,		     \
		},							     \
		IF_ENABLED(DT_INST_SPI_DEV_HAS_CS_GPIOS(idx), (		     \
			.cs_gpio = DT_INST_SPI_DEV_CS_GPIOS_LABEL(idx),      \
			.cs_pin  = DT_INST_SPI_DEV_CS_GPIOS_PIN(idx),	     \
			.cs_dt_flags = DT_INST_SPI_DEV_CS_GPIOS_FLAGS(idx),)) \
		IF_ENABLED(CONFIG_FLASH_PAGE_LAYOUT, (			     \
			.pages_layout = {				     \
				.pages_count = INST_##idx##_PAGES,	     \
				.pages_size  = DT_INST_PROP(idx, page_size), \
			},))						     \
		.chip_size   = INST_##idx##_BYTES,			     \
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
	DEVICE_DEFINE(inst_##idx, DT_INST_LABEL(idx),			     \
		      spi_flash_at45_init, spi_flash_at45_pm_control,	     \
		      &inst_##idx##_data, &inst_##idx##_config,		     \
		      POST_KERNEL, CONFIG_SPI_FLASH_AT45_INIT_PRIORITY,      \
		      &spi_flash_at45_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_FLASH_AT45_INST)

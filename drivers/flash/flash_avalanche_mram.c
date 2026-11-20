/*
 * Copyright (c) 2026 Space Cubics Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <string.h>

LOG_MODULE_REGISTER(avalanche_mram, CONFIG_FLASH_LOG_LEVEL);

#define AVALANCHE_MRAM_CMD_WREN  0x06
#define AVALANCHE_MRAM_CMD_WRDI  0x04
#define AVALANCHE_MRAM_CMD_RDID  0x9f
#define AVALANCHE_MRAM_CMD_RDAR  0x65
#define AVALANCHE_MRAM_CMD_WRAR  0x71
#define AVALANCHE_MRAM_CMD_READ  0x03
#define AVALANCHE_MRAM_CMD_FAST_READ_3B 0x0b
#define AVALANCHE_MRAM_CMD_FAST_READ_4B 0x0c
#define AVALANCHE_MRAM_CMD_WRITE 0x02
#define AVALANCHE_MRAM_CMD_QPIE  0x38
#define AVALANCHE_MRAM_CMD_SPIE  0xff

#define AVALANCHE_MRAM_MAX_ADDRESS_SIZE 4
#define AVALANCHE_MRAM_MAX_CMD_SIZE     (1 + AVALANCHE_MRAM_MAX_ADDRESS_SIZE)
#define AVALANCHE_MRAM_ID_SIZE          4

#define AVALANCHE_MRAM_MANUFACTURER_ID 0xe6
#define AVALANCHE_MRAM_INTERFACE_ID    0x2
#define AVALANCHE_MRAM_VOLTAGE_ID      0x1
#define AVALANCHE_MRAM_ERASE_VALUE     0xff
#define AVALANCHE_MRAM_REG_CR2         0x00000003
#define AVALANCHE_MRAM_REG_SR          0x00000000
#define AVALANCHE_MRAM_SR_WREN         BIT(1)
#define AVALANCHE_MRAM_SR_BPSEL        GENMASK(4, 2)
#define AVALANCHE_MRAM_SR_TBSEL        BIT(5)
#define AVALANCHE_MRAM_REG_ID          0x00000030
#define AVALANCHE_MRAM_MAX_HZ          100000000U
#define AVALANCHE_MRAM_READ_MAX_HZ      50000000U
/* RDID limits differ by package. Use 40 MHz for identification and recovery. */
#define AVALANCHE_MRAM_PROBE_MAX_HZ     40000000U
/* Twelve cycles cover 100 MHz SDR; retain ten cycles up to 40 MHz. */
#define AVALANCHE_MRAM_QPI_LATENCY(hz)  ((hz) > AVALANCHE_MRAM_PROBE_MAX_HZ ? 12 : 10)

struct avalanche_mram_config {
	struct spi_dt_spec spi;
	size_t size;
	uint8_t expected_density;
	uint8_t address_size;
	bool use_qpi;
};

struct avalanche_mram_data {
	struct spi_dt_spec spi;
	struct spi_dt_spec qpi;
	struct spi_dt_spec spi_read;
	struct spi_dt_spec spi_probe;
	struct spi_dt_spec qpi_probe;
	struct k_sem lock;
	struct flash_pages_layout layout;
	size_t size;
	bool qpi_active;
	uint8_t latency_cycles;
};

static const struct flash_parameters avalanche_mram_parameters = {
	.write_block_size = 1,
	.erase_value = AVALANCHE_MRAM_ERASE_VALUE,
	.caps = {.no_explicit_erase = true},
};

static const struct spi_dt_spec *avalanche_mram_spi(const struct device *dev)
{
	const struct avalanche_mram_data *data = dev->data;

	return data->qpi_active ? &data->qpi : &data->spi;
}

static const struct spi_dt_spec *avalanche_mram_register_spi(const struct device *dev)
{
	const struct avalanche_mram_data *data = dev->data;

	return data->qpi_active ? &data->qpi : &data->spi_probe;
}

static bool avalanche_mram_valid_range(const struct avalanche_mram_data *data, off_t offset,
				       size_t len)
{
	return (offset >= 0) && ((size_t)offset <= data->size) &&
	       (len <= (data->size - (size_t)offset));
}

static void avalanche_mram_put_address(uint8_t *command, uint8_t address_size, uint32_t address)
{
	uint8_t bytes[sizeof(address)];

	sys_put_be32(address, bytes);
	memcpy(&command[1], &bytes[sizeof(bytes) - address_size], address_size);
}

static int avalanche_mram_command(const struct spi_dt_spec *spi, uint8_t command)
{
	const struct spi_buf tx_buf = {.buf = &command, .len = sizeof(command)};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

	return spi_write_dt(spi, &tx);
}

static int avalanche_mram_set_write_enable(const struct device *dev, bool enable)
{
	uint8_t command = enable ? AVALANCHE_MRAM_CMD_WREN : AVALANCHE_MRAM_CMD_WRDI;
	int ret = avalanche_mram_command(avalanche_mram_spi(dev), command);

	if (ret < 0) {
		LOG_ERR("Failed to %s writes", enable ? "enable" : "disable");
	}

	return ret;
}

static int avalanche_mram_read_register(const struct spi_dt_spec *spi, uint32_t address,
				       uint8_t latency, uint8_t *value);
static int avalanche_mram_read_locked(const struct device *dev, off_t offset, void *buffer,
				     const void *expected, size_t len);

static int avalanche_mram_check_protection(const struct device *dev, off_t offset, size_t len)
{
	const struct avalanche_mram_data *data = dev->data;
	uint8_t status;
	uint8_t bp;
	int ret = avalanche_mram_read_register(avalanche_mram_register_spi(dev),
					       AVALANCHE_MRAM_REG_SR,
					       data->latency_cycles, &status);

	if (ret < 0) {
		return ret;
	}
	bp = FIELD_GET(AVALANCHE_MRAM_SR_BPSEL, status);

	if (bp != 0) {
		size_t protected_size = data->size >> (7 - bp);
		bool bottom = (status & AVALANCHE_MRAM_SR_TBSEL) != 0;

		if ((bottom && (size_t)offset < protected_size) ||
		    (!bottom && (size_t)offset + len > data->size - protected_size)) {
			return -EACCES;
		}
	}
	return 0;
}

static int avalanche_mram_write_locked(const struct device *dev, off_t offset, const void *data,
				       size_t len)
{
	const struct avalanche_mram_config *config = dev->config;
	uint8_t command[AVALANCHE_MRAM_MAX_CMD_SIZE] = {AVALANCHE_MRAM_CMD_WRITE};
	size_t command_size = 1 + config->address_size;
	const struct spi_buf tx_bufs[] = {
		{.buf = command, .len = command_size},
		{.buf = (void *)data, .len = len},
	};
	const struct spi_buf_set tx = {.buffers = tx_bufs, .count = ARRAY_SIZE(tx_bufs)};
	int ret;
	int ret_disable;
	uint8_t status;
	const struct avalanche_mram_data *dev_data = dev->data;

	avalanche_mram_put_address(command, config->address_size, (uint32_t)offset);

	ret = avalanche_mram_set_write_enable(dev, true);
	if (ret == 0) {
		ret = avalanche_mram_read_register(avalanche_mram_register_spi(dev),
						  AVALANCHE_MRAM_REG_SR,
						  dev_data->latency_cycles, &status);
		if (ret == 0 && (status & AVALANCHE_MRAM_SR_WREN) == 0) {
			ret = -EACCES;
		}
	}
	if (ret == 0) {
		ret = spi_write_dt(avalanche_mram_spi(dev), &tx);
	}
	if (ret == 0) {
		/* HBP/HTBSEL pin protection is not readable through the status
		 * register. Verify the array to detect silently ignored writes.
		 */
		ret = avalanche_mram_read_locked(dev, offset, NULL, data, len);
	}
	if (ret < 0) {
		LOG_ERR("Failed to write to MRAM");
	}

	ret_disable = avalanche_mram_set_write_enable(dev, false);
	if (ret == 0) {
		ret = ret_disable;
	}

	return ret;
}

static int avalanche_mram_read_locked(const struct device *dev, off_t offset, void *buffer,
				     const void *expected, size_t len)
{
	const struct avalanche_mram_config *config = dev->config;
	struct avalanche_mram_data *dev_data = dev->data;
	uint8_t command[AVALANCHE_MRAM_MAX_CMD_SIZE] = {AVALANCHE_MRAM_CMD_READ};
	uint8_t received[CONFIG_FLASH_AVALANCHE_MRAM_TRANSFER_SIZE];
	size_t command_size = 1 + config->address_size;
	size_t chunk_limit = CONFIG_FLASH_AVALANCHE_MRAM_TRANSFER_SIZE;
	size_t dummy_bytes = dev_data->qpi_active ? dev_data->latency_cycles / 2 : 0;
	const struct spi_dt_spec *spi = dev_data->qpi_active ? &dev_data->qpi : &dev_data->spi_read;
	size_t done = 0;
	int ret = 0;

	if (dev_data->qpi_active) {
		command[0] = config->address_size == 4 ? AVALANCHE_MRAM_CMD_FAST_READ_4B
						     : AVALANCHE_MRAM_CMD_FAST_READ_3B;
		chunk_limit -= dummy_bytes;
	}
	while (done < len) {
		size_t chunk = MIN(len - done, chunk_limit);
		void *received_data = dev_data->qpi_active || expected != NULL ? received :
				      (uint8_t *)buffer + done;
		const struct spi_buf tx_buf = {.buf = command, .len = command_size};
		const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};
		const struct spi_buf rx_bufs[] = {
			{.buf = NULL, .len = command_size},
			{.buf = received_data, .len = chunk + dummy_bytes},
		};
		const struct spi_buf_set rx = {.buffers = rx_bufs, .count = ARRAY_SIZE(rx_bufs)};

		avalanche_mram_put_address(command, config->address_size, (uint32_t)offset + done);
		ret = spi_transceive_dt(spi, &tx, &rx);
		if (ret < 0) {
			break;
		}
		if (expected != NULL) {
			if (memcmp(received + dummy_bytes, (const uint8_t *)expected + done,
				   chunk) != 0) {
				return -EIO;
			}
		} else if (dev_data->qpi_active) {
			memcpy((uint8_t *)buffer + done, &received[dummy_bytes], chunk);
		}
		done += chunk;
	}
	return ret;
}

static int avalanche_mram_read(const struct device *dev, off_t offset, void *buffer, size_t len)
{
	struct avalanche_mram_data *data = dev->data;
	int ret;

	if (len == 0) {
		return 0;
	}
	if (buffer == NULL || !avalanche_mram_valid_range(data, offset, len)) {
		return -EINVAL;
	}
	k_sem_take(&data->lock, K_FOREVER);
	ret = avalanche_mram_read_locked(dev, offset, buffer, NULL, len);
	k_sem_give(&data->lock);
	return ret;
}

static int avalanche_mram_write(const struct device *dev, off_t offset, const void *data,
				size_t len)
{
	struct avalanche_mram_data *dev_data = dev->data;
	size_t done;
	int ret;

	if (len == 0) {
		return 0;
	}

	if ((data == NULL) || !avalanche_mram_valid_range(dev_data, offset, len)) {
		return -EINVAL;
	}

	k_sem_take(&dev_data->lock, K_FOREVER);
	ret = avalanche_mram_check_protection(dev, offset, len);
	if (ret < 0) {
		k_sem_give(&dev_data->lock);
		return ret;
	}
	for (done = 0; done < len;) {
		size_t chunk = MIN(len - done, CONFIG_FLASH_AVALANCHE_MRAM_TRANSFER_SIZE);

		ret = avalanche_mram_write_locked(dev, offset + done, (const uint8_t *)data + done,
						  chunk);
		if (ret < 0) {
			break;
		}
		done += chunk;
	}
	k_sem_give(&dev_data->lock);

	return ret;
}

static int avalanche_mram_fill(const struct device *dev, uint8_t value, off_t offset, size_t len)
{
	struct avalanche_mram_data *dev_data = dev->data;
	uint8_t buffer[CONFIG_FLASH_FILL_BUFFER_SIZE];
	size_t done = 0;
	int ret = 0;

	if (len == 0) {
		return 0;
	}

	if (!avalanche_mram_valid_range(dev_data, offset, len)) {
		return -EINVAL;
	}

	memset(buffer, value, sizeof(buffer));

	k_sem_take(&dev_data->lock, K_FOREVER);
	ret = avalanche_mram_check_protection(dev, offset, len);
	if (ret < 0) {
		k_sem_give(&dev_data->lock);
		return ret;
	}
	while (done < len) {
		size_t chunk = MIN(MIN(sizeof(buffer), len - done),
				   CONFIG_FLASH_AVALANCHE_MRAM_TRANSFER_SIZE);

		ret = avalanche_mram_write_locked(dev, offset + done, buffer, chunk);
		if (ret < 0) {
			break;
		}
		done += chunk;
	}
	k_sem_give(&dev_data->lock);

	return ret;
}

static int avalanche_mram_erase(const struct device *dev, off_t offset, size_t len)
{
	return avalanche_mram_fill(dev, AVALANCHE_MRAM_ERASE_VALUE, offset, len);
}

static const struct flash_parameters *avalanche_mram_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);
	return &avalanche_mram_parameters;
}

static int avalanche_mram_get_size(const struct device *dev, uint64_t *size)
{
	const struct avalanche_mram_data *data = dev->data;

	*size = data->size;
	return 0;
}

static int avalanche_mram_read_id(const struct spi_dt_spec *spi, uint8_t id[AVALANCHE_MRAM_ID_SIZE])
{
	uint8_t command = AVALANCHE_MRAM_CMD_RDID;
	const struct spi_buf tx_buf = {.buf = &command, .len = sizeof(command)};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};
	const struct spi_buf rx_bufs[] = {
		{.buf = NULL, .len = sizeof(command)},
		{.buf = id, .len = AVALANCHE_MRAM_ID_SIZE},
	};
	const struct spi_buf_set rx = {.buffers = rx_bufs, .count = ARRAY_SIZE(rx_bufs)};

	return spi_transceive_dt(spi, &tx, &rx);
}

static int avalanche_mram_validate_id(const struct device *dev,
				      const uint8_t id[AVALANCHE_MRAM_ID_SIZE], bool log_errors)
{
	const struct avalanche_mram_config *config = dev->config;

	if (id[0] != AVALANCHE_MRAM_MANUFACTURER_ID) {
		if (log_errors) {
			LOG_ERR("Unexpected manufacturer ID: 0x%02x", id[0]);
		}
		return -ENODEV;
	}
	if ((id[1] >> 4) != AVALANCHE_MRAM_INTERFACE_ID) {
		if (log_errors) {
			LOG_ERR("Unsupported interface ID: 0x%x", id[1] >> 4);
		}
		return -ENODEV;
	}
	if ((id[1] & 0x0f) != AVALANCHE_MRAM_VOLTAGE_ID) {
		if (log_errors) {
			LOG_ERR("Unsupported voltage ID: 0x%x", id[1] & 0x0f);
		}
		return -ENODEV;
	}
	if ((id[2] & 0x0f) != config->expected_density) {
		if (log_errors) {
			LOG_ERR("RDID density 0x%x does not match compatible (0x%x)", id[2] & 0x0f,
				config->expected_density);
		}
		return -ENODEV;
	}

	return 0;
}

static int avalanche_mram_read_register(const struct spi_dt_spec *spi, uint32_t address,
				       uint8_t latency, uint8_t *value)
{
	/* RDAR uses four address bytes on all parts. Receive latency as data
	 * to avoid controller-specific dummy-clock units.
	 */
	uint8_t command[5] = {AVALANCHE_MRAM_CMD_RDAR};
	uint8_t received[9]; /* Up to 15 quad latency clocks plus one data byte. */
	size_t lines = (spi->config.operation & SPI_LINES_MASK) == SPI_LINES_QUAD ? 4 : 1;
	size_t dummy_bits = latency * lines;
	size_t receive_bytes = DIV_ROUND_UP(dummy_bits + 8, 8);
	const struct spi_buf tx_buf = {.buf = command, .len = sizeof(command)};
	const struct spi_buf rx_bufs[] = {
		{.buf = NULL, .len = sizeof(command)},
		{.buf = received, .len = receive_bytes},
	};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};
	const struct spi_buf_set rx = {.buffers = rx_bufs, .count = ARRAY_SIZE(rx_bufs)};
	int ret;

	if (latency < 8 || latency > 15) {
		return -EINVAL;
	}
	sys_put_be32(address, &command[1]);
	ret = spi_transceive_dt(spi, &tx, &rx);
	if (ret == 0) {
		size_t byte = dummy_bits / 8;
		size_t bit = dummy_bits % 8;

		*value = received[byte];
		if (bit != 0) {
			*value = (received[byte] << bit) | (received[byte + 1] >> (8 - bit));
		}
	}
	return ret;
}

static int avalanche_mram_find_latency(const struct spi_dt_spec *spi, uint8_t *latency,
				      uint8_t *cr2)
{
	uint8_t candidate;

	/* Check both ID and CR2 to reject false latency matches. */
	for (candidate = 8; candidate <= 15; candidate++) {
		uint8_t manufacturer;
		int ret = avalanche_mram_read_register(spi, AVALANCHE_MRAM_REG_ID,
						      candidate, &manufacturer);

		if (ret < 0 || manufacturer != AVALANCHE_MRAM_MANUFACTURER_ID) {
			continue;
		}
		ret = avalanche_mram_read_register(spi, AVALANCHE_MRAM_REG_CR2, candidate, cr2);
		if (ret == 0 && (*cr2 & 0x0f) == candidate) {
			*latency = candidate;
			return 0;
		}
	}
	return -ENODEV;
}

static int avalanche_mram_prepare_qpi(const struct device *dev)
{
	struct avalanche_mram_data *data = dev->data;
	uint8_t latency = AVALANCHE_MRAM_QPI_LATENCY(data->qpi.config.frequency);
	uint8_t cr2;
	int ret = avalanche_mram_find_latency(&data->spi_probe, &data->latency_cycles, &cr2);

	if (ret < 0) {
		LOG_ERR("Failed to determine SPI register latency");
		return ret;
	}
	if ((cr2 & 0x0f) != latency) {
		uint8_t command[5] = {AVALANCHE_MRAM_CMD_WRAR};
		uint8_t value = (cr2 & 0xf0) | latency;
		const struct spi_buf tx_bufs[] = {
			{.buf = command, .len = sizeof(command)},
			{.buf = &value, .len = sizeof(value)},
		};
		const struct spi_buf_set tx = {.buffers = tx_bufs, .count = ARRAY_SIZE(tx_bufs)};
		int cleanup_ret;

		sys_put_be32(AVALANCHE_MRAM_REG_CR2, &command[1]);
		ret = avalanche_mram_command(&data->spi_probe, AVALANCHE_MRAM_CMD_WREN);
		if (ret == 0) {
			ret = spi_write_dt(&data->spi_probe, &tx);
		}
		cleanup_ret = avalanche_mram_command(&data->spi_probe, AVALANCHE_MRAM_CMD_WRDI);
		if (ret < 0 || cleanup_ret < 0) {
			return ret < 0 ? ret : cleanup_ret;
		}
	}
	data->latency_cycles = latency;
	ret = avalanche_mram_read_register(&data->spi_probe, AVALANCHE_MRAM_REG_CR2,
					  data->latency_cycles, &cr2);
	if (ret < 0 || (cr2 & 0x0f) != data->latency_cycles) {
		LOG_ERR("Failed to configure QPI read latency");
		return ret < 0 ? ret : -EIO;
	}
	return 0;
}

static int avalanche_mram_check_qpi(const struct device *dev)
{
	const struct avalanche_mram_data *data = dev->data;
	uint8_t manufacturer;
	uint8_t cr2;
	int ret = avalanche_mram_read_register(&data->qpi, AVALANCHE_MRAM_REG_ID,
					      data->latency_cycles, &manufacturer);

	if (ret < 0) {
		return ret;
	}
	ret = avalanche_mram_read_register(&data->qpi, AVALANCHE_MRAM_REG_CR2,
					  data->latency_cycles, &cr2);
	if (ret < 0) {
		return ret;
	}
	if (manufacturer != AVALANCHE_MRAM_MANUFACTURER_ID ||
	    (cr2 & 0x0f) != data->latency_cycles) {
		LOG_ERR("QPI RDAR verification failed: manufacturer 0x%02x, CR2 0x%02x",
			manufacturer, cr2);
		return -ENODEV;
	}
	LOG_DBG("QPI RDAR verified: manufacturer 0x%02x, latency %u", manufacturer,
		data->latency_cycles);
	return 0;
}

static int avalanche_mram_init(const struct device *dev)
{
	const struct avalanche_mram_config *config = dev->config;
	struct avalanche_mram_data *data = dev->data;
	uint8_t id[AVALANCHE_MRAM_ID_SIZE];
	int ret;

	k_sem_init(&data->lock, 1, 1);

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI bus not ready");
		return -ENODEV;
	}

	/* Keep separate configurations: SPI controllers may cache their pointers. */
	data->spi = config->spi;
	data->spi.config.frequency = MIN(config->spi.config.frequency, AVALANCHE_MRAM_MAX_HZ);
	data->qpi = data->spi;
	data->qpi.config.operation |= SPI_LINES_QUAD;
	data->spi_read = data->spi;
	data->spi_read.config.frequency = MIN(data->spi.config.frequency,
					    AVALANCHE_MRAM_READ_MAX_HZ);
	data->spi_probe = data->spi;
	data->spi_probe.config.frequency = MIN(data->spi.config.frequency,
					     AVALANCHE_MRAM_PROBE_MAX_HZ);
	data->qpi_probe = data->spi_probe;
	data->qpi_probe.config.operation |= SPI_LINES_QUAD;

	ret = avalanche_mram_read_id(&data->spi_probe, id);
	if (ret == 0) {
		LOG_DBG("Initial SPI RDID: %02x %02x %02x %02x", id[0], id[1], id[2], id[3]);
		ret = avalanche_mram_validate_id(dev, id, false);
	}
	if (ret < 0) {
		uint8_t cr2;

		LOG_DBG("SPI identification failed (%d); trying QPI recovery", ret);
		ret = avalanche_mram_find_latency(&data->qpi_probe, &data->latency_cycles, &cr2);
		if (ret < 0) {
			LOG_ERR("Failed to identify MRAM in SPI or QPI mode");
			return -ENODEV;
		}
		ret = avalanche_mram_command(&data->qpi_probe, AVALANCHE_MRAM_CMD_SPIE);
		if (ret < 0) {
			LOG_ERR("Failed to exit QPI mode: %d", ret);
			return ret;
		}
		ret = avalanche_mram_read_id(&data->spi_probe, id);
		if (ret < 0) {
			LOG_ERR("Failed to read SPI device ID after QPI exit: %d", ret);
			return ret;
		}
		ret = avalanche_mram_validate_id(dev, id, true);
		if (ret < 0) {
			return ret;
		}
		LOG_INF("Recovered MRAM from QPI mode");
	}

	data->size = config->size;
	data->layout.pages_count = config->size;
	data->layout.pages_size = 1;
	if (!config->use_qpi) {
		uint8_t cr2;

		ret = avalanche_mram_find_latency(&data->spi_probe, &data->latency_cycles, &cr2);
		if (ret < 0) {
			return ret;
		}
	}

	if (config->use_qpi) {
		uint8_t latency = AVALANCHE_MRAM_QPI_LATENCY(data->qpi.config.frequency);

		if (CONFIG_FLASH_AVALANCHE_MRAM_TRANSFER_SIZE <= latency / 2) {
			LOG_ERR("QPI transfer size must include latency and one data byte");
			return -ENOTSUP;
		}
		ret = avalanche_mram_prepare_qpi(dev);
		if (ret < 0) {
			return ret;
		}
		ret = avalanche_mram_command(&data->spi_probe, AVALANCHE_MRAM_CMD_QPIE);
		if (ret < 0) {
			return ret;
		}
		data->qpi_active = true;
		ret = avalanche_mram_check_qpi(dev);
		if (ret < 0) {
			(void)avalanche_mram_command(&data->qpi_probe, AVALANCHE_MRAM_CMD_SPIE);
			data->qpi_active = false;
			LOG_ERR("Failed to enter QPI mode");
			return -EIO;
		}
	}

	LOG_DBG("Device ID: %02x %02x %02x %02x, size %zu", id[0], id[1], id[2], id[3],
		config->size);
	return 0;
}

#ifdef CONFIG_FLASH_PAGE_LAYOUT
static void avalanche_mram_page_layout(const struct device *dev,
				       const struct flash_pages_layout **layout,
				       size_t *layout_size)
{
	const struct avalanche_mram_data *data = dev->data;

	*layout = &data->layout;
	*layout_size = 1;
}
#endif

static DEVICE_API(flash, avalanche_mram_api) = {
	.read = avalanche_mram_read,
	.write = avalanche_mram_write,
	.erase = avalanche_mram_erase,
	.fill = avalanche_mram_fill,
	.get_parameters = avalanche_mram_get_parameters,
	.get_size = avalanche_mram_get_size,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = avalanche_mram_page_layout,
#endif
};

#define AVALANCHE_MRAM_SPI_MODE(node_id, property, mode)                                           \
	COND_CODE_1(DT_PROP(node_id, property), (mode), (0))

#define AVALANCHE_MRAM_SPI_OPERATION(node_id)                                                      \
	(SPI_OP_MODE_CONTROLLER | SPI_WORD_SET(8) |                                                \
	 AVALANCHE_MRAM_SPI_MODE(node_id, spi_cpol, SPI_MODE_CPOL) |                               \
	 AVALANCHE_MRAM_SPI_MODE(node_id, spi_cpha, SPI_MODE_CPHA))

#define AVALANCHE_MRAM_DATA(node_id)   DT_CAT(node_id, _avalanche_mram_data)
#define AVALANCHE_MRAM_CONFIG(node_id) DT_CAT(node_id, _avalanche_mram_config)

#define AVALANCHE_MRAM_INIT(node_id, density_id, address_bytes, interface_size)                    \
	BUILD_ASSERT(DT_PROP(node_id, spi_cpol) == DT_PROP(node_id, spi_cpha),                    \
		     "Avalanche MRAM supports SPI modes 0 and 3 only");                            \
	BUILD_ASSERT(DT_PROP(node_id, spi_rx_bus_width) == DT_PROP(node_id, spi_tx_bus_width),     \
		     "Avalanche MRAM RX and TX bus widths must match");                            \
	static struct avalanche_mram_data AVALANCHE_MRAM_DATA(node_id);                            \
	static const struct avalanche_mram_config AVALANCHE_MRAM_CONFIG(node_id) = {               \
		.spi = SPI_DT_SPEC_GET(node_id, AVALANCHE_MRAM_SPI_OPERATION(node_id)),            \
		.size = interface_size,                                                            \
		.expected_density = density_id,                                                    \
		.address_size = address_bytes,                                                     \
		.use_qpi = DT_PROP(node_id, spi_rx_bus_width) == 4,                                \
	};                                                                                         \
	DEVICE_DT_DEFINE(node_id, avalanche_mram_init, NULL, &AVALANCHE_MRAM_DATA(node_id),        \
			 &AVALANCHE_MRAM_CONFIG(node_id), POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY, \
			 &avalanche_mram_api);

BUILD_ASSERT(CONFIG_FLASH_FILL_BUFFER_SIZE > 0, "FLASH_FILL_BUFFER_SIZE must be greater than zero");

#define AVALANCHE_MRAM_INIT_301(node_id)  AVALANCHE_MRAM_INIT(node_id, 0x8, 4, MB(64))
#define AVALANCHE_MRAM_INIT_302(node_id)  AVALANCHE_MRAM_INIT(node_id, 0x9, 4, MB(128))
#define AVALANCHE_MRAM_INIT_304(node_id)  AVALANCHE_MRAM_INIT(node_id, 0xa, 4, MB(256))
#define AVALANCHE_MRAM_INIT_3064(node_id) AVALANCHE_MRAM_INIT(node_id, 0x1, 3, MB(8))
#define AVALANCHE_MRAM_INIT_3128(node_id) AVALANCHE_MRAM_INIT(node_id, 0x2, 3, MB(8))
#define AVALANCHE_MRAM_INIT_308(node_id)  AVALANCHE_MRAM_INIT(node_id, 0xc, 4, MB(512))

DT_FOREACH_STATUS_OKAY(avalanche_as301g208, AVALANCHE_MRAM_INIT_301)
DT_FOREACH_STATUS_OKAY(avalanche_as302g208, AVALANCHE_MRAM_INIT_302)
DT_FOREACH_STATUS_OKAY(avalanche_as304g208, AVALANCHE_MRAM_INIT_304)
DT_FOREACH_STATUS_OKAY(avalanche_as3064204, AVALANCHE_MRAM_INIT_3064)
DT_FOREACH_STATUS_OKAY(avalanche_as3128208, AVALANCHE_MRAM_INIT_3128)
DT_FOREACH_STATUS_OKAY(avalanche_as308g208, AVALANCHE_MRAM_INIT_308)

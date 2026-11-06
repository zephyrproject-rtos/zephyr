/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2026 Space Cubics Inc.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <string.h>

LOG_MODULE_REGISTER(everspin_emxxlx, CONFIG_FLASH_LOG_LEVEL);

#define EMXXLX_WREN       0x06
#define EMXXLX_WRDI       0x04
#define EMXXLX_RDSR       0x05
#define EMXXLX_RDFSR      0x70
#define EMXXLX_CLFSR      0x50
#define EMXXLX_RDID_MIO   0xaf
#define EMXXLX_RDVCR      0x85
#define EMXXLX_WRVCR      0x81
#define EMXXLX_EXIT_4B    0xe9
#define EMXXLX_READ       0x03
#define EMXXLX_FAST_READ  0x0b
#define EMXXLX_WRITE      0x02
#define EMXXLX_SR_BUSY    BIT(0)
#define EMXXLX_SR_WEL     BIT(1)
#define EMXXLX_FSR_READY  BIT(7)
#define EMXXLX_FSR_ERRORS (BIT(1) | BIT(4) | BIT(5))
#define EMXXLX_VCR_IO     0x00
#define EMXXLX_VCR_DUMMY  0x01
#define EMXXLX_VCR_XIP    0x06
#define EMXXLX_VCR_WRAP   0x07
#define EMXXLX_VCR_MEMORY 0x08
#define EMXXLX_VCR_ISR    0x10
#define EMXXLX_SPI_NO_DS  0xdf
#define EMXXLX_QUAD_NO_DS 0xdb
#define EMXXLX_PERSISTENT BIT(0)
#define EMXXLX_POWER_ERR  BIT(2)
#define EMXXLX_DUMMY     16
#define EMXXLX_DUMMY_BYTES (EMXXLX_DUMMY / 2)
#define EMXXLX_SPI_DUMMY_BYTES (EMXXLX_DUMMY / 8)
#define EMXXLX_READ_MAX_HZ 66000000U
#define EMXXLX_TIMEOUT_MS  100

struct emxxlx_config {
	struct spi_dt_spec spi;
	struct spi_dt_spec quad;
	size_t size;
	uint8_t density_id;
	bool use_quad;
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	struct flash_pages_layout layout;
#endif
};

struct emxxlx_data {
	struct k_sem lock;
	bool quad_active;
};

static const struct flash_parameters emxxlx_parameters = {
	.write_block_size = 1,
	.erase_value = 0xff,
	.caps = {.no_explicit_erase = true},
};

static const struct spi_dt_spec *emxxlx_bus(const struct device *dev)
{
	const struct emxxlx_config *cfg = dev->config;
	const struct emxxlx_data *data = dev->data;

	return data->quad_active ? &cfg->quad : &cfg->spi;
}

static int emxxlx_command(const struct device *dev, uint8_t opcode)
{
	const struct spi_buf buf = {.buf = &opcode, .len = 1};
	const struct spi_buf_set tx = {.buffers = &buf, .count = 1};

	return spi_write_dt(emxxlx_bus(dev), &tx);
}

static int emxxlx_receive(const struct device *dev, uint8_t *header, size_t header_len,
			  void *buffer, size_t len)
{
	const struct spi_buf tx_buf = {.buf = header, .len = header_len};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};
	const struct spi_buf rx_bufs[] = {
		{.buf = NULL, .len = header_len},
		{.buf = buffer, .len = len},
	};
	const struct spi_buf_set rx = {.buffers = rx_bufs, .count = ARRAY_SIZE(rx_bufs)};

	return spi_transceive_dt(emxxlx_bus(dev), &tx, &rx);
}

static int emxxlx_status(const struct device *dev, uint8_t opcode, uint8_t *value)
{
	return emxxlx_receive(dev, &opcode, 1, value, 1);
}

static int emxxlx_wait_ready(const struct device *dev)
{
	int64_t deadline = k_uptime_get() + EMXXLX_TIMEOUT_MS;
	uint8_t status;
	int ret;

	do {
		ret = emxxlx_status(dev, EMXXLX_RDSR, &status);
		if (ret < 0 || (status & EMXXLX_SR_BUSY) == 0) {
			return ret;
		}
		k_sleep(K_USEC(10));
	} while (k_uptime_get() < deadline);

	return -ETIMEDOUT;
}

static int emxxlx_write_enable(const struct device *dev)
{
	uint8_t status;
	int ret = emxxlx_wait_ready(dev);

	if (ret == 0) {
		ret = emxxlx_command(dev, EMXXLX_CLFSR);
	}
	if (ret == 0) {
		ret = emxxlx_command(dev, EMXXLX_WREN);
	}
	if (ret == 0) {
		ret = emxxlx_status(dev, EMXXLX_RDSR, &status);
		if (ret == 0 && (status & EMXXLX_SR_WEL) == 0) {
			ret = -EACCES;
		}
	}
	/* WREN may have succeeded even if the subsequent status read failed. */
	if (ret < 0) {
		(void)emxxlx_command(dev, EMXXLX_WRDI);
	}
	return ret;
}

static int emxxlx_write_finish(const struct device *dev, int ret)
{
	uint8_t status;
	int cleanup;

	if (ret == 0) {
		ret = emxxlx_wait_ready(dev);
	}
	if (ret == 0) {
		ret = emxxlx_status(dev, EMXXLX_RDFSR, &status);
		if (ret == 0 && ((status & EMXXLX_FSR_READY) == 0 ||
				 (status & EMXXLX_FSR_ERRORS) != 0)) {
			LOG_ERR("Write failed, flag status 0x%02x", status);
			ret = -EIO;
		}
	}
	cleanup = emxxlx_command(dev, EMXXLX_WRDI);
	return ret == 0 ? cleanup : ret;
}

static void emxxlx_address(uint8_t *header, uint32_t address)
{
	sys_put_be24(address, &header[1]);
}

static int emxxlx_read_vcr(const struct device *dev, uint8_t address, uint8_t *value)
{
	uint8_t header[4] = {EMXXLX_RDVCR};

	emxxlx_address(header, address);
	return emxxlx_receive(dev, header, sizeof(header), value, 1);
}

static int emxxlx_write_vcr(const struct device *dev, uint8_t address, uint8_t value)
{
	struct emxxlx_data *data = dev->data;
	uint8_t command[5] = {EMXXLX_WRVCR};
	const struct spi_buf buf = {.buf = command, .len = sizeof(command)};
	const struct spi_buf_set tx = {.buffers = &buf, .count = 1};
	uint8_t readback;
	int ret = emxxlx_write_enable(dev);

	if (ret < 0) {
		return ret;
	}
	emxxlx_address(command, address);
	command[4] = value;
	ret = spi_write_dt(emxxlx_bus(dev), &tx);
	if (ret == 0 && address == EMXXLX_VCR_IO) {
		/* The new I/O width takes effect at the end of this transaction. */
		data->quad_active = value == EMXXLX_QUAD_NO_DS;
	}
	ret = emxxlx_write_finish(dev, ret);
	if (ret == 0) {
		ret = emxxlx_read_vcr(dev, address, &readback);
		if (ret == 0 && readback != value) {
			ret = -EIO;
		}
	}
	return ret;
}

static bool emxxlx_valid_range(const struct device *dev, off_t offset, size_t len)
{
	const struct emxxlx_config *cfg = dev->config;

	return offset >= 0 && (uint64_t)offset <= cfg->size &&
	       len <= cfg->size - (size_t)offset;
}

static int emxxlx_read(const struct device *dev, off_t offset, void *buffer, size_t len)
{
	const struct emxxlx_config *cfg = dev->config;
	struct emxxlx_data *data = dev->data;
	uint8_t received[CONFIG_FLASH_EVERSPIN_EMXXLX_TRANSFER_SIZE];
	uint8_t header[4] = {EMXXLX_READ};
	size_t done = 0;
	size_t dummy;
	bool fast;
	int ret = 0;

	if (!emxxlx_valid_range(dev, offset, len) || (len != 0 && buffer == NULL)) {
		return -EINVAL;
	}
	k_sem_take(&data->lock, K_FOREVER);
	fast = data->quad_active || cfg->spi.config.frequency > EMXXLX_READ_MAX_HZ;
	dummy = fast ? (data->quad_active ? EMXXLX_DUMMY_BYTES : EMXXLX_SPI_DUMMY_BYTES) : 0;

	if (fast) {
		header[0] = EMXXLX_FAST_READ;
	}
	while (done < len) {
		size_t chunk = MIN(len - done, sizeof(received) - dummy);

		emxxlx_address(header, offset + done);
		/* Receive dummy bytes as data: GQSPI counts dummy transfers in clocks. */
		ret = emxxlx_receive(dev, header, sizeof(header), received, dummy + chunk);
		if (ret < 0) {
			break;
		}
		memcpy((uint8_t *)buffer + done, received + dummy, chunk);
		done += chunk;
	}
	k_sem_give(&data->lock);
	return ret;
}

static int emxxlx_write_chunk(const struct device *dev, off_t offset,
			      const void *buffer, size_t len)
{
	uint8_t header[4] = {EMXXLX_WRITE};
	const struct spi_buf bufs[] = {
		{.buf = header, .len = sizeof(header)},
		{.buf = (void *)buffer, .len = len},
	};
	const struct spi_buf_set tx = {.buffers = bufs, .count = ARRAY_SIZE(bufs)};
	int ret = emxxlx_write_enable(dev);

	if (ret < 0) {
		return ret;
	}
	emxxlx_address(header, offset);
	ret = spi_write_dt(emxxlx_bus(dev), &tx);
	return emxxlx_write_finish(dev, ret);
}

static int emxxlx_write(const struct device *dev, off_t offset, const void *buffer, size_t len)
{
	struct emxxlx_data *data = dev->data;
	size_t done = 0;
	int ret = 0;

	if (!emxxlx_valid_range(dev, offset, len) || (len != 0 && buffer == NULL)) {
		return -EINVAL;
	}
	k_sem_take(&data->lock, K_FOREVER);
	while (done < len) {
		size_t chunk = MIN(len - done, CONFIG_FLASH_EVERSPIN_EMXXLX_TRANSFER_SIZE);

		ret = emxxlx_write_chunk(dev, offset + done, (const uint8_t *)buffer + done, chunk);
		if (ret < 0) {
			break;
		}
		done += chunk;
	}
	k_sem_give(&data->lock);
	return ret;
}

static int emxxlx_fill(const struct device *dev, uint8_t value, off_t offset, size_t len)
{
	struct emxxlx_data *data = dev->data;
	uint8_t buffer[CONFIG_FLASH_FILL_BUFFER_SIZE];
	size_t done = 0;
	int ret = 0;

	if (!emxxlx_valid_range(dev, offset, len)) {
		return -EINVAL;
	}
	memset(buffer, value, sizeof(buffer));
	k_sem_take(&data->lock, K_FOREVER);
	while (done < len) {
		size_t chunk = MIN(len - done, MIN(sizeof(buffer),
						CONFIG_FLASH_EVERSPIN_EMXXLX_TRANSFER_SIZE));

		ret = emxxlx_write_chunk(dev, offset + done, buffer, chunk);
		if (ret < 0) {
			break;
		}
		done += chunk;
	}
	k_sem_give(&data->lock);
	return ret;
}

static int emxxlx_erase(const struct device *dev, off_t offset, size_t len)
{
	return emxxlx_fill(dev, 0xff, offset, len);
}

static const struct flash_parameters *emxxlx_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);
	return &emxxlx_parameters;
}

static int emxxlx_get_size(const struct device *dev, uint64_t *size)
{
	const struct emxxlx_config *cfg = dev->config;

	*size = cfg->size;
	return 0;
}

static int emxxlx_check_id(const struct device *dev)
{
	const struct emxxlx_config *cfg = dev->config;
	const uint8_t expected_id[] = {0x6b, 0xbb, cfg->density_id};
	uint8_t opcode = EMXXLX_RDID_MIO;
	uint8_t id[sizeof(expected_id)];
	int ret = emxxlx_receive(dev, &opcode, 1, id, sizeof(id));

	if (ret == 0) {
		const struct emxxlx_data *data = dev->data;

		LOG_DBG("%s ID: %02x %02x %02x", data->quad_active ? "Quad" : "SPI",
			id[0], id[1], id[2]);
	}
	return ret < 0 ? ret : (memcmp(id, expected_id, sizeof(id)) == 0 ? 0 : -ENODEV);
}

static int emxxlx_probe(const struct device *dev)
{
	struct emxxlx_data *data = dev->data;
	int64_t deadline = k_uptime_get() + EMXXLX_TIMEOUT_MS;
	int ret = -ENODEV;

	do {
		bool busy = false;
		unsigned int mode;

		/* Poll busy state before RDID, which is ignored during program/erase. */
		for (mode = 0; mode < 2; mode++) {
			uint8_t status;

			data->quad_active = mode != 0;
			ret = emxxlx_status(dev, EMXXLX_RDSR, &status);
			if (ret < 0) {
				continue;
			}
			/* A wrong-width 0xff response does not prove the device is busy. */
			if (status != 0xff && (status & EMXXLX_SR_BUSY) != 0) {
				busy = true;
				continue;
			}
			ret = emxxlx_check_id(dev);
			if (ret == 0) {
				return 0;
			}
		}
		if (!busy) {
			return ret;
		}
		k_sleep(K_USEC(10));
	} while (k_uptime_get() < deadline);

	return -ETIMEDOUT;
}

static int emxxlx_init(const struct device *dev)
{
	const struct emxxlx_config *cfg = dev->config;
	struct emxxlx_data *data = dev->data;
	uint8_t value;
	int ret;

	k_sem_init(&data->lock, 1, 1);
	if (!spi_is_ready_dt(&cfg->spi)) {
		return -ENODEV;
	}
	ret = emxxlx_probe(dev);
	if (ret < 0) {
		LOG_ERR("MRAM not found (expected JEDEC ID 6b bb %02x)", cfg->density_id);
		return ret;
	}
	ret = emxxlx_wait_ready(dev);
	if (ret == 0) {
		/* This opcode works in either 3-byte or 4-byte address mode. */
		ret = emxxlx_command(dev, EMXXLX_EXIT_4B);
	}
	if (ret == 0) {
		ret = emxxlx_read_vcr(dev, EMXXLX_VCR_ISR, &value);
		if (ret == 0 && (value & EMXXLX_POWER_ERR) != 0) {
			LOG_ERR("Power-on error; manufacturing initialization/recovery required");
			return -EIO;
		}
	}
	if (ret == 0) {
		/* Avoid unnecessary Quad-to-SPI transitions: IO3 can act as RESET# on DFN parts. */
		ret = emxxlx_write_vcr(dev, EMXXLX_VCR_IO,
				      cfg->use_quad ? EMXXLX_QUAD_NO_DS : EMXXLX_SPI_NO_DS);
	}
	if (ret == 0) {
		ret = emxxlx_write_vcr(dev, EMXXLX_VCR_DUMMY, EMXXLX_DUMMY);
	}
	if (ret == 0) {
		ret = emxxlx_write_vcr(dev, EMXXLX_VCR_XIP, 0xff);
	}
	if (ret == 0) {
		ret = emxxlx_write_vcr(dev, EMXXLX_VCR_WRAP, 0xff);
	}
	if (ret == 0) {
		ret = emxxlx_read_vcr(dev, EMXXLX_VCR_MEMORY, &value);
	}
	if (ret == 0) {
		ret = emxxlx_write_vcr(dev, EMXXLX_VCR_MEMORY, value | EMXXLX_PERSISTENT);
	}
	if (ret == 0) {
		ret = emxxlx_check_id(dev);
	}
	if (ret < 0) {
		LOG_ERR("MRAM configuration failed: %d", ret);
		return ret;
	}
	LOG_INF("EMxxLX: %zu KiB, %s", cfg->size / KB(1),
		data->quad_active ? "Quad STR (4-4-4)" : "SPI");
	return 0;
}

#ifdef CONFIG_FLASH_PAGE_LAYOUT
static void emxxlx_page_layout(const struct device *dev, const struct flash_pages_layout **layout,
			       size_t *layout_size)
{
	const struct emxxlx_config *cfg = dev->config;

	*layout = &cfg->layout;
	*layout_size = 1;
}
#endif

static DEVICE_API(flash, emxxlx_api) = {
	.read = emxxlx_read,
	.write = emxxlx_write,
	.fill = emxxlx_fill,
	.erase = emxxlx_erase,
	.get_parameters = emxxlx_get_parameters,
	.get_size = emxxlx_get_size,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = emxxlx_page_layout,
#endif
};

#define EMXXLX_OPERATION(node_id)                                                                 \
	(SPI_OP_MODE_CONTROLLER | SPI_WORD_SET(8) |                                                \
	 COND_CODE_1(DT_PROP(node_id, spi_cpol), (SPI_MODE_CPOL), (0)) |                         \
	 COND_CODE_1(DT_PROP(node_id, spi_cpha), (SPI_MODE_CPHA), (0)))

#define EMXXLX_DATA(node_id) DT_CAT(node_id, _emxxlx_data)
#define EMXXLX_CONFIG(node_id) DT_CAT(node_id, _emxxlx_config)

#define EMXXLX_DEFINE(node_id, density, capacity)                                                  \
	BUILD_ASSERT(DT_PROP(node_id, spi_rx_bus_width) == DT_PROP(node_id, spi_tx_bus_width),     \
		     "Everspin MRAM RX and TX bus widths must match");                             \
	BUILD_ASSERT(DT_PROP(node_id, spi_cpol) == DT_PROP(node_id, spi_cpha),                     \
		     "Everspin MRAM supports SPI modes 0 and 3 only");                             \
	static struct emxxlx_data EMXXLX_DATA(node_id);                                           \
	static const struct emxxlx_config EMXXLX_CONFIG(node_id) = {                              \
		.spi = SPI_DT_SPEC_GET(node_id, EMXXLX_OPERATION(node_id)),                       \
		.quad = SPI_DT_SPEC_GET(node_id, EMXXLX_OPERATION(node_id) | SPI_LINES_QUAD),     \
		.size = capacity,                                                                  \
		.density_id = density,                                                             \
		.use_quad = DT_PROP(node_id, spi_rx_bus_width) == 4,                               \
		IF_ENABLED(CONFIG_FLASH_PAGE_LAYOUT,                                               \
			   (.layout = {.pages_count = capacity,                            \
				       .pages_size = 1},))                                 \
	};                                                                                         \
	DEVICE_DT_DEFINE(node_id, emxxlx_init, NULL, &EMXXLX_DATA(node_id),                       \
			 &EMXXLX_CONFIG(node_id), POST_KERNEL,                                     \
			 CONFIG_FLASH_INIT_PRIORITY, &emxxlx_api);

BUILD_ASSERT(CONFIG_FLASH_FILL_BUFFER_SIZE > 0, "FLASH_FILL_BUFFER_SIZE must be positive");

#define EMXXLX_DEFINE_004(node_id) EMXXLX_DEFINE(node_id, 0x13, KB(512))
#define EMXXLX_DEFINE_008(node_id) EMXXLX_DEFINE(node_id, 0x14, MB(1))
#define EMXXLX_DEFINE_016(node_id) EMXXLX_DEFINE(node_id, 0x15, MB(2))
#define EMXXLX_DEFINE_032(node_id) EMXXLX_DEFINE(node_id, 0x16, MB(4))
#define EMXXLX_DEFINE_064(node_id) EMXXLX_DEFINE(node_id, 0x17, MB(8))

DT_FOREACH_STATUS_OKAY(everspin_em004lxq, EMXXLX_DEFINE_004)
DT_FOREACH_STATUS_OKAY(everspin_em008lxq, EMXXLX_DEFINE_008)
DT_FOREACH_STATUS_OKAY(everspin_em016lxq, EMXXLX_DEFINE_016)
DT_FOREACH_STATUS_OKAY(everspin_em032lxq, EMXXLX_DEFINE_032)
DT_FOREACH_STATUS_OKAY(everspin_em064lxq, EMXXLX_DEFINE_064)

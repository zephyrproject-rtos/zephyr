/*
 * SPDX-FileCopyrightText: Copyright tinyvision.ai
 * SPDX-FileCopyrightText: Copyright Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nxp_sc18is60x_spi, CONFIG_SPI_LOG_LEVEL);

#include <stdint.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/mfd/mfd_sc18is60x.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#define SC18IS60X_LSB_MASK  GENMASK(5, 5)
#define SC18IS60X_MODE_MASK  GENMASK(3, 2)
#define SC18IS60X_FREQ_MASK  GENMASK(1, 0)

struct spi_sc18is60x_data {
	uint8_t *scratch;
	uint16_t scratch_size;
};

struct spi_sc18is60x_config {
	const struct device *bridge;
	uint8_t max_ss;
	uint16_t buf_size;
};

static uint8_t sc18is60x_freq_code(uint32_t frequency)
{
	if (frequency >= 1875000U) {
		return 0U;
	}
	if (frequency >= 455000U) {
		return 1U;
	}
	if (frequency >= 115000U) {
		return 2U;
	}

	return 3U;
}

static uint8_t sc18is60x_mode_code(spi_operation_t operation)
{
	uint8_t mode = 0U;

	if ((operation & SPI_MODE_CPOL) != 0U) {
		mode |= 2U;
	}
	if ((operation & SPI_MODE_CPHA) != 0U) {
		mode |= 1U;
	}

	return mode;
}

static size_t spi_buf_set_total_len(const struct spi_buf_set *set)
{
	size_t len = 0;

	if (set == NULL || set->buffers == NULL) {
		return 0;
	}

	for (size_t i = 0; i < set->count; i++) {
		len += set->buffers[i].len;
	}

	return len;
}

static void spi_buf_set_gather(uint8_t *dst, const struct spi_buf_set *set, size_t dst_len)
{
	size_t off = 0;

	if (dst_len == 0U) {
		return;
	}

	if (set == NULL || set->buffers == NULL) {
		memset(dst, 0, dst_len);
		return;
	}

	for (size_t i = 0; i < set->count; i++) {
		size_t n = set->buffers[i].len;
		const uint8_t *src = set->buffers[i].buf;

		if (off >= dst_len) {
			return;
		}
		if (off + n > dst_len) {
			n = dst_len - off;
		}
		if (src != NULL) {
			memcpy(dst + off, src, n);
		} else {
			memset(dst + off, 0, n);
		}
		off += n;
	}

	if (off < dst_len) {
		memset(dst + off, 0, dst_len - off);
	}
}

static void spi_buf_set_scatter(const struct spi_buf_set *set, const uint8_t *src, size_t src_len)
{
	size_t off = 0;

	if (set == NULL || set->buffers == NULL || src == NULL) {
		return;
	}

	for (size_t i = 0; i < set->count; i++) {
		size_t n = set->buffers[i].len;
		uint8_t *dst = set->buffers[i].buf;

		if (off >= src_len) {
			return;
		}
		if (off + n > src_len) {
			n = src_len - off;
		}
		if (dst != NULL && n > 0U) {
			memcpy(dst, src + off, n);
		}
		off += set->buffers[i].len;
	}
}

static int sc18is60x_spi_configure(const struct device *dev, const struct spi_config *config)
{
	const struct spi_sc18is60x_config *cfg = dev->config;
	uint32_t bits;
	uint8_t cfg_byte = 0;

	if ((config->operation & SPI_OP_MODE_SLAVE) != 0U) {
		LOG_ERR("SC18IS60x does not support Slave mode");
		return -ENOTSUP;
	}

	if ((config->operation & SPI_MODE_LOOP) != 0U) {
		return -ENOTSUP;
	}

	if ((config->operation & SPI_HALF_DUPLEX) != 0U) {
		return -ENOTSUP;
	}

	if ((config->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		LOG_ERR("Unsupported line configuration");
		return -ENOTSUP;
	}

	bits = SPI_WORD_SIZE_GET(config->operation);
	if (bits == 0U) {
		bits = 8U;
	}
	if (bits != 8U) {
		LOG_ERR("Only 8-bit words are supported");
		return -ENOTSUP;
	}

	cfg_byte |= FIELD_PREP(SC18IS60X_LSB_MASK, !!(config->operation & SPI_TRANSFER_LSB));
	cfg_byte |= FIELD_PREP(SC18IS60X_MODE_MASK, sc18is60x_mode_code(config->operation));
	cfg_byte |= FIELD_PREP(SC18IS60X_FREQ_MASK, sc18is60x_freq_code(config->frequency));

	return nxp_sc18is60x_configure_spi(cfg->bridge, cfg_byte);
}

static int sc18is60x_spi_transceive(const struct device *dev, const struct spi_config *spi_cfg,
				    const struct spi_buf_set *tx_buffer_set,
				    const struct spi_buf_set *rx_buffer_set)
{
	const struct spi_sc18is60x_config *cfg = dev->config;
	struct spi_sc18is60x_data *data = dev->data;
	size_t tx_len;
	size_t rx_len;
	size_t spi_len;
	uint8_t function_id;
	int ret = 0;

	if (tx_buffer_set == NULL && rx_buffer_set == NULL) {
		LOG_ERR("SC18IS60x at least one buffer_set should be set");
		return -EINVAL;
	}

	if (spi_cfg->slave >= cfg->max_ss) {
		LOG_ERR("SC18IS60x: Invalid SS Index (%u) must be 0-%u", spi_cfg->slave,
			cfg->max_ss - 1);
		return -EINVAL;
	}

	function_id = BIT(spi_cfg->slave);
	tx_len = spi_buf_set_total_len(tx_buffer_set);
	rx_len = spi_buf_set_total_len(rx_buffer_set);
	spi_len = MAX(tx_len, rx_len);

	if (spi_len > cfg->buf_size) {
		LOG_ERR("SC18IS60x: transfer of size %u exceeds buffer %u", (unsigned int)spi_len,
			cfg->buf_size);
		return -ENOMEM;
	}

	ret = nxp_sc18is60x_lock(cfg->bridge);
	if (ret != 0) {
		return ret;
	}

	ret = sc18is60x_spi_configure(dev, spi_cfg);
	if (ret != 0) {
		goto out;
	}

	if (spi_len == 0U) {
		ret = 0;
		goto out;
	}

	data->scratch[0] = function_id;
	spi_buf_set_gather(&data->scratch[1], tx_buffer_set, spi_len);

	if (rx_len > 0U) {
		ret = nxp_sc18is60x_transfer_unlocked(cfg->bridge, data->scratch,
						      (uint16_t)(spi_len + 1U), &data->scratch[1],
						      (uint16_t)spi_len);
		if (ret != 0) {
			LOG_ERR("SC18IS60x: TX/RX of size %u failed", (unsigned int)spi_len);
			goto out;
		}
		spi_buf_set_scatter(rx_buffer_set, &data->scratch[1], spi_len);
	} else {
		ret = nxp_sc18is60x_transfer_unlocked(cfg->bridge, data->scratch,
						      (uint16_t)(spi_len + 1U), NULL, 0);
		if (ret != 0) {
			LOG_ERR("SC18IS60x: TX of size: %u failed", (unsigned int)spi_len);
			goto out;
		}
	}

out:
	nxp_sc18is60x_unlock(cfg->bridge);
	return ret;
}

static DEVICE_API(spi, sc18is60x_api) = {
	.transceive = sc18is60x_spi_transceive,
};

static int sc18is60x_spi_init(const struct device *dev)
{
	const struct spi_sc18is60x_config *cfg = dev->config;

	if (!device_is_ready(cfg->bridge)) {
		return -ENODEV;
	}

	return 0;
}

#define SC18IS60X_SPI_BUFFER_SIZE(inst) DT_PROP(DT_INST_PARENT(inst), buffer_size)
#define SC18IS60X_SPI_SS_COUNT(inst) DT_PROP(DT_INST_PARENT(inst), ss_count)

#define SPI_SC18IS60X_DEFINE(inst, prefix)                                                         \
	static uint8_t prefix##_scratch_##inst[SC18IS60X_SPI_BUFFER_SIZE(inst) + 1];               \
	static struct spi_sc18is60x_data prefix##_data_##inst = {                                  \
		.scratch = prefix##_scratch_##inst,                                                \
		.scratch_size = SC18IS60X_SPI_BUFFER_SIZE(inst) + 1,                               \
	};                                                                                         \
	static const struct spi_sc18is60x_config prefix##_cfg_##inst = {                           \
		.bridge = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                     \
		.max_ss = SC18IS60X_SPI_SS_COUNT(inst),                                            \
		.buf_size = SC18IS60X_SPI_BUFFER_SIZE(inst),                                       \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, sc18is60x_spi_init, NULL, &prefix##_data_##inst,               \
			      &prefix##_cfg_##inst, POST_KERNEL,                                   \
			      CONFIG_SPI_SC18IS60X_INIT_PRIORITY, &sc18is60x_api);

#define DT_DRV_COMPAT nxp_sc18is60x_spi
DT_INST_FOREACH_STATUS_OKAY_VARGS(SPI_SC18IS60X_DEFINE, sc18is60x_spi)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT nxp_sc18is606_spi
DT_INST_FOREACH_STATUS_OKAY_VARGS(SPI_SC18IS60X_DEFINE, sc18is606_spi)

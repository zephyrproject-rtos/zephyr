/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Driver for the VIRTIO SPI controller device (virtio spec 1.4, section 5.21).
 *
 * The device carries one SPI transfer per request, so a transceive call is split
 * into one request per contiguous chunk of the buffer sets, and every request is
 * a round trip to the device. Chip select is driven by the device, kept asserted
 * across the chunks of a transaction through the cs_change field.
 */

#define DT_DRV_COMPAT virtio_spi

#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/virtio.h>
#include <zephyr/drivers/virtio/virtqueue.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(spi_virtio, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"
#ifdef CONFIG_SPI_RTIO
#include "spi_rtio.h"
#endif

#define VIRTIO_SPI_REQUESTQ 0

/* struct virtio_spi_transfer_head::mode */
#define VIRTIO_SPI_CPHA           BIT(0)
#define VIRTIO_SPI_CPOL           BIT(1)
#define VIRTIO_SPI_CS_HIGH        BIT(2)
#define VIRTIO_SPI_MODE_LSB_FIRST BIT(3)
#define VIRTIO_SPI_MODE_LOOP      BIT(4)

/* struct virtio_spi_config::mode_func_supported */
#define VIRTIO_SPI_MF_SUPPORT_CPHA_0    BIT(0)
#define VIRTIO_SPI_MF_SUPPORT_CPHA_1    BIT(1)
#define VIRTIO_SPI_MF_SUPPORT_CPOL_0    BIT(2)
#define VIRTIO_SPI_MF_SUPPORT_CPOL_1    BIT(3)
#define VIRTIO_SPI_MF_SUPPORT_CS_HIGH   BIT(4)
#define VIRTIO_SPI_MF_SUPPORT_LSB_FIRST BIT(5)
#define VIRTIO_SPI_MF_SUPPORT_LOOPBACK  BIT(6)

/* struct virtio_spi_config::tx_nbits_supported and ::rx_nbits_supported */
#define VIRTIO_SPI_RX_TX_SUPPORT_DUAL  BIT(0)
#define VIRTIO_SPI_RX_TX_SUPPORT_QUAD  BIT(1)
#define VIRTIO_SPI_RX_TX_SUPPORT_OCTAL BIT(2)

#define VIRTIO_SPI_TRANS_OK  0
#define VIRTIO_SPI_PARAM_ERR 1
#define VIRTIO_SPI_TRANS_ERR 2

/* head, tx buffer, rx buffer and result */
#define VIRTIO_SPI_MAX_BUFS 4

/* length clocked out per request when both directions are placeholders */
#define VIRTIO_SPI_DUMMY_LEN 32

/*
 * Left unpacked on purpose: the configuration space has to be read with accesses
 * as wide as the field they target, and the fields are naturally aligned anyway.
 */
struct virtio_spi_config {
	uint8_t cs_max_number;
	uint8_t cs_change_supported;
	uint8_t tx_nbits_supported;
	uint8_t rx_nbits_supported;
	uint32_t bits_per_word_mask;
	uint32_t mode_func_supported;
	uint32_t max_freq_hz;
	uint32_t max_word_delay_ns;
	uint32_t max_cs_setup_ns;
	uint32_t max_cs_hold_ns;
	uint32_t max_cs_inactive_ns;
};

BUILD_ASSERT(sizeof(struct virtio_spi_config) == 32);

struct virtio_spi_transfer_head {
	uint8_t chip_select_id;
	uint8_t bits_per_word;
	uint8_t cs_change;
	uint8_t tx_nbits;
	uint8_t rx_nbits;
	uint8_t reserved[3];
	uint32_t mode;
	uint32_t freq;
	uint32_t word_delay_ns;
	uint32_t cs_setup_ns;
	uint32_t cs_delay_hold_ns;
	uint32_t cs_change_delay_inactive_ns;
} __packed;

struct virtio_spi_transfer_result {
	uint8_t result;
} __packed;

struct spi_virtio_config {
	const struct device *vdev;
};

struct spi_virtio_data {
	struct spi_context ctx;
	struct virtq *requestq;
	struct k_sem done;
	uint32_t used_len;
	struct virtio_spi_transfer_head head;
	struct virtio_spi_transfer_result result;
	/* device capabilities, read once from the configuration space */
	uint32_t bits_per_word_mask;
	uint32_t mode_func_supported;
	uint32_t max_freq_hz;
	uint8_t cs_max_number;
	uint8_t cs_change_supported;
	uint8_t tx_nbits_supported;
	uint8_t rx_nbits_supported;
	/* bytes per data frame, derived from the configured word size */
	uint8_t dfs;
};

static const uint8_t spi_virtio_dummy_tx[VIRTIO_SPI_DUMMY_LEN];

static bool spi_virtio_nbits_supported(uint8_t supported, uint8_t nbits)
{
	switch (nbits) {
	case 2:
		return (supported & VIRTIO_SPI_RX_TX_SUPPORT_DUAL) != 0;
	case 4:
		return (supported & VIRTIO_SPI_RX_TX_SUPPORT_QUAD) != 0;
	case 8:
		return (supported & VIRTIO_SPI_RX_TX_SUPPORT_OCTAL) != 0;
	default:
		return true;
	}
}

/**
 * @brief Validate a SPI configuration and cache what it implies for the requests
 *
 * @param dev virtio SPI device
 * @param spi_cfg configuration to apply
 * @return 0 on success or negative error code on failure
 */
static int spi_virtio_configure(const struct device *dev, const struct spi_config *spi_cfg)
{
	struct spi_virtio_data *data = dev->data;
	spi_operation_t op = spi_cfg->operation;
	uint8_t bits = SPI_WORD_SIZE_GET(op);
	uint32_t mode = 0;
	uint8_t nbits;

	if (SPI_OP_MODE_GET(op) == SPI_OP_MODE_SLAVE) {
		LOG_ERR("peripheral mode is not supported");
		return -ENOTSUP;
	}

	/* the device owns the chip select lines, it selects them by index */
	if (spi_cs_is_gpio(spi_cfg)) {
		LOG_ERR("GPIO chip select is not supported");
		return -ENOTSUP;
	}

	if (spi_cfg->slave >= data->cs_max_number) {
		LOG_ERR("chip select %u out of range (%u available)", spi_cfg->slave,
			data->cs_max_number);
		return -EINVAL;
	}

	if (bits == 0 || bits > 32) {
		LOG_ERR("unsupported word size %u", bits);
		return -ENOTSUP;
	}

	if (data->bits_per_word_mask != 0 && (data->bits_per_word_mask & BIT(bits - 1)) == 0) {
		LOG_ERR("device does not support a word size of %u", bits);
		return -ENOTSUP;
	}

	if (data->max_freq_hz != 0 && spi_cfg->frequency > data->max_freq_hz) {
		LOG_ERR("frequency %u above the device maximum of %u", spi_cfg->frequency,
			data->max_freq_hz);
		return -EINVAL;
	}

	if ((op & SPI_MODE_CPHA) != 0) {
		mode |= VIRTIO_SPI_CPHA;
	}

	if ((op & SPI_MODE_CPOL) != 0) {
		mode |= VIRTIO_SPI_CPOL;
	}

	if ((op & SPI_CS_ACTIVE_HIGH) != 0) {
		mode |= VIRTIO_SPI_CS_HIGH;
	}

	if ((op & SPI_TRANSFER_LSB) != 0) {
		mode |= VIRTIO_SPI_MODE_LSB_FIRST;
	}

	if ((op & SPI_MODE_LOOP) != 0) {
		mode |= VIRTIO_SPI_MODE_LOOP;
	}

	if ((mode & VIRTIO_SPI_CPHA) != 0) {
		if ((data->mode_func_supported & VIRTIO_SPI_MF_SUPPORT_CPHA_1) == 0) {
			LOG_ERR("device does not support CPHA=1");
			return -ENOTSUP;
		}
	} else if ((data->mode_func_supported & VIRTIO_SPI_MF_SUPPORT_CPHA_0) == 0) {
		LOG_ERR("device does not support CPHA=0");
		return -ENOTSUP;
	}

	if ((mode & VIRTIO_SPI_CPOL) != 0) {
		if ((data->mode_func_supported & VIRTIO_SPI_MF_SUPPORT_CPOL_1) == 0) {
			LOG_ERR("device does not support CPOL=1");
			return -ENOTSUP;
		}
	} else if ((data->mode_func_supported & VIRTIO_SPI_MF_SUPPORT_CPOL_0) == 0) {
		LOG_ERR("device does not support CPOL=0");
		return -ENOTSUP;
	}

	if ((mode & VIRTIO_SPI_CS_HIGH) != 0 &&
	    (data->mode_func_supported & VIRTIO_SPI_MF_SUPPORT_CS_HIGH) == 0) {
		LOG_ERR("device does not support an active high chip select");
		return -ENOTSUP;
	}

	if ((mode & VIRTIO_SPI_MODE_LSB_FIRST) != 0 &&
	    (data->mode_func_supported & VIRTIO_SPI_MF_SUPPORT_LSB_FIRST) == 0) {
		LOG_ERR("device does not support LSB first transfers");
		return -ENOTSUP;
	}

	if ((mode & VIRTIO_SPI_MODE_LOOP) != 0 &&
	    (data->mode_func_supported & VIRTIO_SPI_MF_SUPPORT_LOOPBACK) == 0) {
		LOG_ERR("device does not support loopback mode");
		return -ENOTSUP;
	}

	switch (op & SPI_LINES_MASK) {
	case SPI_LINES_SINGLE:
		nbits = 1;
		break;
	case SPI_LINES_DUAL:
		nbits = 2;
		break;
	case SPI_LINES_QUAD:
		nbits = 4;
		break;
	default:
		nbits = 8;
		break;
	}

	/*
	 * The line count applies to the whole configuration, so both directions
	 * have to support it even if only one of them ends up being used.
	 */
	if (!spi_virtio_nbits_supported(data->tx_nbits_supported, nbits) ||
	    !spi_virtio_nbits_supported(data->rx_nbits_supported, nbits)) {
		LOG_ERR("device does not support %u-bit transfers", nbits);
		return -ENOTSUP;
	}

	/* there is no 24 bit type, words wider than 16 bits are stored in 32 bit frames */
	data->dfs = (bits <= 8) ? 1 : ((bits <= 16) ? 2 : 4);
	data->head.chip_select_id = spi_cfg->slave;
	data->head.bits_per_word = bits;
	data->head.tx_nbits = nbits;
	data->head.rx_nbits = nbits;
	data->head.mode = sys_cpu_to_le32(mode);
	data->head.freq = sys_cpu_to_le32(spi_cfg->frequency);

	data->ctx.config = spi_cfg;

	return 0;
}

static void spi_virtio_request_cb(void *opaque, uint32_t used_len)
{
	struct spi_virtio_data *data = opaque;

	data->used_len = used_len;
	k_sem_give(&data->done);
}

/**
 * @brief Run a single SPI transfer request on the request virtqueue
 *
 * @param dev virtio SPI device
 * @param len length of the chunk in bytes
 * @param last true if no further request follows in this transaction
 * @return 0 on success or negative error code on failure
 */
static int spi_virtio_transfer(const struct device *dev, size_t len, bool last)
{
	const struct spi_virtio_config *cfg = dev->config;
	struct spi_virtio_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	struct virtq_buf bufs[VIRTIO_SPI_MAX_BUFS];
	uint16_t n = 0;
	uint16_t readable;
	bool release_cs;
	int ret;

	/* a device without cs_change support keeps the chip select asserted on its own */
	release_cs = last && (ctx->config->operation & SPI_HOLD_ON_CS) == 0;
	data->head.cs_change = (release_cs && data->cs_change_supported) ? 1 : 0;

	bufs[n++] = (struct virtq_buf){.addr = &data->head, .len = sizeof(data->head)};

	if (spi_context_tx_buf_on(ctx)) {
		bufs[n++] = (struct virtq_buf){.addr = (void *)ctx->tx_buf, .len = len};
	} else if (!spi_context_rx_buf_on(ctx)) {
		bufs[n++] = (struct virtq_buf){.addr = (void *)spi_virtio_dummy_tx, .len = len};
	}

	readable = n;

	if (spi_context_rx_buf_on(ctx)) {
		bufs[n++] = (struct virtq_buf){.addr = ctx->rx_buf, .len = len};
	}

	bufs[n++] = (struct virtq_buf){.addr = &data->result, .len = sizeof(data->result)};

	data->used_len = 0;
	data->result.result = VIRTIO_SPI_TRANS_ERR;

	/*
	 * A single request is in flight at a time and the transport returns the
	 * descriptors before completing it, so the queue never runs out.
	 */
	ret = virtq_add_buffer_chain(data->requestq, bufs, n, readable, spi_virtio_request_cb, data,
				     K_NO_WAIT);
	if (ret != 0) {
		LOG_ERR("failed to queue a %zu byte transfer: %d", len, ret);
		return ret;
	}

	virtio_notify_virtqueue(cfg->vdev, VIRTIO_SPI_REQUESTQ);

	k_sem_take(&data->done, K_FOREVER);

	if (data->used_len < sizeof(data->result)) {
		LOG_ERR("device returned %u bytes, too short to hold a result", data->used_len);
		return -EIO;
	}

	switch (data->result.result) {
	case VIRTIO_SPI_TRANS_OK:
		return 0;
	case VIRTIO_SPI_PARAM_ERR:
		LOG_ERR("device rejected the transfer parameters");
		return -EINVAL;
	default:
		LOG_ERR("transfer failed with result %u", data->result.result);
		return -EIO;
	}
}

static int spi_virtio_transceive(const struct device *dev, const struct spi_config *spi_cfg,
				 const struct spi_buf_set *tx_bufs,
				 const struct spi_buf_set *rx_bufs)
{
	struct spi_virtio_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int ret;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	spi_context_lock(ctx, false, NULL, NULL, spi_cfg);

	ret = spi_virtio_configure(dev, spi_cfg);
	if (ret != 0) {
		goto out;
	}

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, data->dfs);

	while (spi_context_tx_on(ctx) || spi_context_rx_on(ctx)) {
		size_t chunk = spi_context_max_continuous_chunk(ctx);
		size_t len;
		bool last;

		/* nothing to send and nothing to keep, clock out of the dummy buffer */
		if (!spi_context_tx_buf_on(ctx) && !spi_context_rx_buf_on(ctx)) {
			chunk = MIN(chunk, VIRTIO_SPI_DUMMY_LEN / data->dfs);
		}

		len = chunk * data->dfs;
		last = spi_context_tx_len_left(ctx, data->dfs) <= len &&
		       spi_context_rx_len_left(ctx, data->dfs) <= len;

		ret = spi_virtio_transfer(dev, len, last);
		if (ret != 0) {
			break;
		}

		spi_context_update_tx(ctx, data->dfs, chunk);
		spi_context_update_rx(ctx, data->dfs, chunk);
	}

out:
	spi_context_release(ctx, ret);

	return ret;
}

#ifdef CONFIG_SPI_ASYNC
static int spi_virtio_transceive_async(const struct device *dev, const struct spi_config *spi_cfg,
				       const struct spi_buf_set *tx_bufs,
				       const struct spi_buf_set *rx_bufs, spi_callback_t cb,
				       void *userdata)
{
	return -ENOTSUP;
}
#endif /* CONFIG_SPI_ASYNC */

static int spi_virtio_release(const struct device *dev, const struct spi_config *spi_cfg)
{
	struct spi_virtio_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(spi, spi_virtio_api) = {
	.transceive = spi_virtio_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_virtio_transceive_async,
#endif
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release = spi_virtio_release,
};

static uint16_t spi_virtio_enum_queues_cb(uint16_t q_index, uint16_t q_size_max, void *opaque)
{
	if (q_index != VIRTIO_SPI_REQUESTQ) {
		return 0;
	}

	/* a single request is in flight at a time, and takes at most four descriptors */
	return MIN(VIRTIO_SPI_MAX_BUFS, q_size_max);
}

static int spi_virtio_init(const struct device *dev)
{
	const struct spi_virtio_config *cfg = dev->config;
	struct spi_virtio_data *data = dev->data;
	volatile struct virtio_spi_config *devcfg;
	int ret;

	if (!device_is_ready(cfg->vdev)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->vdev);
		return -ENODEV;
	}

	k_sem_init(&data->done, 0, 1);

	/* the device offers no feature bits, but the handshake still has to happen */
	ret = virtio_commit_feature_bits(cfg->vdev);
	if (ret != 0) {
		LOG_ERR("virtio_commit_feature_bits failed: %d", ret);
		return ret;
	}

	devcfg = virtio_get_device_specific_config(cfg->vdev);
	if (devcfg == NULL) {
		LOG_ERR("could not get device-specific config");
		return -ENODEV;
	}

	data->cs_max_number = devcfg->cs_max_number;
	data->cs_change_supported = devcfg->cs_change_supported;
	data->tx_nbits_supported = devcfg->tx_nbits_supported;
	data->rx_nbits_supported = devcfg->rx_nbits_supported;
	data->bits_per_word_mask = sys_le32_to_cpu(devcfg->bits_per_word_mask);
	data->mode_func_supported = sys_le32_to_cpu(devcfg->mode_func_supported);
	data->max_freq_hz = sys_le32_to_cpu(devcfg->max_freq_hz);

	if (data->cs_max_number == 0) {
		LOG_ERR("device exposes no chip select");
		return -ENODEV;
	}

	ret = virtio_init_virtqueues(cfg->vdev, 1, spi_virtio_enum_queues_cb, data);
	if (ret != 0) {
		LOG_ERR("virtio_init_virtqueues failed: %d", ret);
		return ret;
	}

	data->requestq = virtio_get_virtqueue(cfg->vdev, VIRTIO_SPI_REQUESTQ);
	if (data->requestq == NULL) {
		LOG_ERR("failed to get the request virtqueue");
		return -ENODEV;
	}

	if (data->requestq->num < VIRTIO_SPI_MAX_BUFS) {
		LOG_ERR("request virtqueue holds %u descriptors, %u are needed",
			data->requestq->num, VIRTIO_SPI_MAX_BUFS);
		return -ENOTSUP;
	}

	virtio_finalize_init(cfg->vdev);

	spi_context_unlock_unconditionally(&data->ctx);

	LOG_DBG("%u chip selects, max frequency %u Hz", data->cs_max_number, data->max_freq_hz);

	return 0;
}

#define SPI_VIRTIO_DEFINE(inst)                                                                    \
	static struct spi_virtio_data spi_virtio_data_##inst = {                                   \
		SPI_CONTEXT_INIT_LOCK(spi_virtio_data_##inst, ctx),                                \
		SPI_CONTEXT_INIT_SYNC(spi_virtio_data_##inst, ctx),                                \
	};                                                                                         \
	static const struct spi_virtio_config spi_virtio_config_##inst = {                         \
		.vdev = DEVICE_DT_GET(DT_PARENT(DT_DRV_INST(inst))),                               \
	};                                                                                         \
	SPI_DEVICE_DT_INST_DEFINE(inst, spi_virtio_init, NULL, &spi_virtio_data_##inst,            \
				  &spi_virtio_config_##inst, POST_KERNEL,                          \
				  CONFIG_SPI_INIT_PRIORITY, &spi_virtio_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_VIRTIO_DEFINE)

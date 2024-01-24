/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* The SPI STM32 backend implements dedicated SPI driver for Host Commands. Unfortunately, the
 * current SPI API can't be used to handle the host commands communication. The main issues are
 * unknown command size sent by the host (the SPI transaction sends/receives specific number of
 * bytes) and need to constant sending status byte (the SPI module is enabled and disabled per
 * transaction), see https://github.com/zephyrproject-rtos/zephyr/issues/56091.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(host_cmd_spi, CONFIG_EC_HC_LOG_LEVEL);

#include <stm32_ll_spi.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/dma/dma_stm32.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/mgmt/ec_host_cmd/backend.h>
#include <zephyr/mgmt/ec_host_cmd/ec_host_cmd.h>
#include <zephyr/pm/policy.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/time_units.h>

/* The default compatible string of a SPI devicetree node has to be replaced with the one
 * dedicated for Host Commands. It disabled standard SPI driver. For STM32 SPI "st,stm32-spi" has
 * to be changed to "st,stm32-spi-host-cmd". The remaining "additional" compatible strings should
 * stay the same.
 */
#define ST_STM32_SPI_HOST_CMD_COMPAT st_stm32_spi_host_cmd
BUILD_ASSERT(DT_NODE_EXISTS(DT_CHOSEN(zephyr_host_cmd_spi_backend)),
	     "The chosen backend node is obligatory for SPI STM32 backend.");
BUILD_ASSERT(DT_NODE_HAS_COMPAT_STATUS(DT_CHOSEN(zephyr_host_cmd_spi_backend),
				       ST_STM32_SPI_HOST_CMD_COMPAT, okay),
	     "Invalid compatible of the chosen spi node.");

#define RX_HEADER_SIZE (sizeof(struct ec_host_cmd_request_header))

/* Framing byte which precedes a response packet from the EC. After sending a
 * request, the host will clock in bytes until it sees the framing byte, then
 * clock in the response packet.
 */
#define EC_SPI_FRAME_START 0xec

/* Padding bytes which are clocked out after the end of a response packet.*/
#define EC_SPI_PAST_END 0xed

/* The number of the ending bytes. The number can be bigger than 1 for chip families
 * than need to bypass the DMA threshold.
 */
#define EC_SPI_PAST_END_LENGTH 1

/* EC is ready to receive.*/
#define EC_SPI_RX_READY 0x78

/* EC has started receiving the request from the host, but hasn't started
 * processing it yet.
 */
#define EC_SPI_RECEIVING 0xf9

/* EC has received the entire request from the host and is processing it. */
#define EC_SPI_PROCESSING 0xfa

/* EC received bad data from the host, such as a packet header with an invalid
 * length. EC will ignore all data until chip select deasserts.
 */
#define EC_SPI_RX_BAD_DATA 0xfb

/* EC received data from the AP before it was ready. That is, the host asserted
 * chip select and started clocking data before the EC was ready to receive it.
 * EC will ignore all data until chip select deasserts.
 */
#define EC_SPI_NOT_READY 0xfc

/* Supported version of host commands protocol. */
#define EC_HOST_REQUEST_VERSION 3

/* Timeout to wait for SPI request packet
 *
 * This affects the slowest SPI clock we can support. A delay of 8192 us
 * permits a 512-byte request at 500 KHz, assuming the master starts sending
 * bytes as soon as it asserts chip select. That's as slow as we would
 * practically want to run the SPI interface, since running it slower
 * significantly impacts firmware update times.
 */
#define EC_SPI_CMD_RX_TIMEOUT_US 8192

#if DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_host_cmd_spi_backend), st_stm32h7_spi)
#define EC_HOST_CMD_ST_STM32H7
#endif /* st_stm32h7_spi */

#if DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_host_cmd_spi_backend), st_stm32_spi_fifo)
#define EC_HOST_CMD_ST_STM32_FIFO
#endif /* st_stm32_spi_fifo */

/*
 * Max data size for a version 3 request/response packet.  This is big enough
 * to handle a request/response header, flash write offset/size, and 512 bytes
 * of flash data.
 */
#define SPI_MAX_REQ_SIZE  0x220
#define SPI_MAX_RESP_SIZE 0x220

/* Enumeration to maintain different states of incoming request from
 * host
 */
enum spi_host_command_state {
	/* SPI not enabled (initial state, and when chipset is off) */
	SPI_HOST_CMD_STATE_DISABLED = 0,

	/* SPI module enabled, but not ready to receive */
	SPI_HOST_CMD_STATE_RX_NOT_READY,

	/* Ready to receive next request */
	SPI_HOST_CMD_STATE_READY_TO_RX,

	/* Receiving request */
	SPI_HOST_CMD_STATE_RECEIVING,

	/* Processing request */
	SPI_HOST_CMD_STATE_PROCESSING,

	/* Sending response */
	SPI_HOST_CMD_STATE_SENDING,

	/* Received bad data - transaction started before we were ready, or
	 * packet header from host didn't parse properly. Ignoring received
	 * data.
	 */
	SPI_HOST_CMD_STATE_RX_BAD,
};

struct dma_stream {
	const struct device *dma_dev;
	uint32_t channel;
	struct dma_config dma_cfg;
	struct dma_block_config dma_blk_cfg;
	int fifo_threshold;
};

struct ec_host_cmd_spi_cfg {
	SPI_TypeDef *spi;
	const struct pinctrl_dev_config *pcfg;
	const struct stm32_pclken *pclken;
	size_t pclk_len;
};

struct ec_host_cmd_spi_ctx {
	struct gpio_dt_spec cs;
	struct gpio_callback cs_callback;
	struct ec_host_cmd_spi_cfg *spi_config;
	struct ec_host_cmd_rx_ctx *rx_ctx;
	struct ec_host_cmd_tx_buf *tx;
	uint8_t *tx_buf;
	struct dma_stream *dma_rx;
	struct dma_stream *dma_tx;
	enum spi_host_command_state state;
	int prepare_rx_later;
#ifdef CONFIG_PM
	ATOMIC_DEFINE(pm_policy_lock_on, 1);
#endif /* CONFIG_PM */
};

static const uint8_t out_preamble[4] = {
	EC_SPI_PROCESSING, EC_SPI_PROCESSING, EC_SPI_PROCESSING,
	EC_SPI_FRAME_START, /* This is the byte which matters */
};

static void dma_callback(const struct device *dev, void *arg, uint32_t channel, int status);
static int prepare_rx(struct ec_host_cmd_spi_ctx *hc_spi);

#define SPI_DMA_CHANNEL_INIT(id, dir, dir_cap, src_dev, dest_dev)                                  \
	.dma_dev = DEVICE_DT_GET(DT_DMAS_CTLR_BY_NAME(id, dir)),                                   \
	.channel = DT_DMAS_CELL_BY_NAME(id, dir, channel),                                         \
	.dma_cfg =                                                                                 \
		{                                                                                  \
			.dma_slot = DT_DMAS_CELL_BY_NAME(id, dir, slot),                           \
			.channel_direction = STM32_DMA_CONFIG_DIRECTION(                           \
				DT_DMAS_CELL_BY_NAME(id, dir, channel_config)),                    \
			.source_data_size = STM32_DMA_CONFIG_##src_dev##_DATA_SIZE(                \
				DT_DMAS_CELL_BY_NAME(id, dir, channel_config)),                    \
			.dest_data_size = STM32_DMA_CONFIG_##dest_dev##_DATA_SIZE(                 \
				DT_DMAS_CELL_BY_NAME(id, dir, channel_config)),                    \
			.source_burst_length = 1, /* SINGLE transfer */                            \
			.dest_burst_length = 1,   /* SINGLE transfer */                            \
			.channel_priority = STM32_DMA_CONFIG_PRIORITY(                             \
				DT_DMAS_CELL_BY_NAME(id, dir, channel_config)),                    \
			.dma_callback = dma_callback,                                              \
			.block_count = 2,                                                          \
	},                                                                                         \
	.fifo_threshold =                                                                          \
		STM32_DMA_FEATURES_FIFO_THRESHOLD(DT_DMAS_CELL_BY_NAME(id, dir, features)),

#define STM32_SPI_INIT(id)                                                                         \
	PINCTRL_DT_DEFINE(id);                                                                     \
	static const struct stm32_pclken pclken[] = STM32_DT_CLOCKS(id);                           \
                                                                                                   \
	static struct ec_host_cmd_spi_cfg ec_host_cmd_spi_cfg = {                                  \
		.spi = (SPI_TypeDef *)DT_REG_ADDR(id),                                             \
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(id),                                             \
		.pclken = pclken,                                                                  \
		.pclk_len = DT_NUM_CLOCKS(id),                                                     \
	};                                                                                         \
                                                                                                   \
	static struct dma_stream dma_rx = {SPI_DMA_CHANNEL_INIT(id, rx, RX, PERIPHERAL, MEMORY)};  \
	static struct dma_stream dma_tx = {SPI_DMA_CHANNEL_INIT(id, tx, TX, MEMORY, PERIPHERAL)}

STM32_SPI_INIT(DT_CHOSEN(zephyr_host_cmd_spi_backend));

#define EC_HOST_CMD_SPI_DEFINE(_name)                                                              \
	static struct ec_host_cmd_spi_ctx _name##_hc_spi = {                                       \
		.dma_rx = &dma_rx,                                                                 \
		.dma_tx = &dma_tx,                                                                 \
		.spi_config = &ec_host_cmd_spi_cfg,                                                \
	};                                                                                         \
	struct ec_host_cmd_backend _name = {                                                       \
		.api = &ec_host_cmd_api,                                                           \
		.ctx = (struct ec_host_cmd_spi_ctx *)&_name##_hc_spi,                              \
	}

static inline uint32_t dma_source_addr(SPI_TypeDef *spi)
{
#ifdef EC_HOST_CMD_ST_STM32H7
	return (uint32_t)(&spi->RXDR);
#else
	return (uint32_t)LL_SPI_DMA_GetRegAddr(spi);
#endif /* EC_HOST_CMD_ST_STM32H7 */
}

static inline uint32_t dma_dest_addr(SPI_TypeDef *spi)
{
#ifdef EC_HOST_CMD_ST_STM32H7
	return (uint32_t)(&spi->TXDR);
#else
	return (uint32_t)LL_SPI_DMA_GetRegAddr(spi);
#endif /* EC_HOST_CMD_ST_STM32H7 */
}

/* Set TX register to send status, while SPI module is enabled */
static inline void tx_status(SPI_TypeDef *spi, uint8_t status)
{
	/* The number of status bytes to sent can be bigger than 1 for chip
	 * families than need to bypass the DMA threshold.
	 */
	LL_SPI_TransmitData8(spi, status);
}

static int expected_size(const struct ec_host_cmd_request_header *header)
{
	/* Check host request version */
	if (header->prtcl_ver != EC_HOST_REQUEST_VERSION) {
		return 0;
	}

	/* Reserved byte should be 0 */
	if (header->reserved) {
		return 0;
	}

	return sizeof(*header) + header->data_len;
}

#ifdef CONFIG_PM
static void ec_host_cmd_pm_policy_state_lock_get(struct ec_host_cmd_spi_ctx *hc_spi)
{
	if (!atomic_test_and_set_bit(hc_spi->pm_policy_lock_on, 0)) {
		pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	}
}

static void ec_host_cmd_pm_policy_state_lock_put(struct ec_host_cmd_spi_ctx *hc_spi)
{
	if (atomic_test_and_clear_bit(hc_spi->pm_policy_lock_on, 0)) {
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	}
}
#else
static inline void ec_host_cmd_pm_policy_state_lock_get(struct ec_host_cmd_spi_ctx *hc_spi)
{
	ARG_UNUSED(hc_spi);
}

static void ec_host_cmd_pm_policy_state_lock_put(struct ec_host_cmd_spi_ctx *hc_spi)
{
	ARG_UNUSED(hc_spi);
}
#endif /* CONFIG_PM */

static void dma_callback(const struct device *dev, void *arg, uint32_t channel, int status)
{
	struct ec_host_cmd_spi_ctx *hc_spi = arg;

	/* End of sending */
	if (channel == hc_spi->dma_tx->channel) {
		if (hc_spi->prepare_rx_later) {
			int ret;

			ret = prepare_rx(hc_spi);

			if (ret) {
				LOG_ERR("Failed to prepare RX later");
			}
		} else {
			const struct ec_host_cmd_spi_cfg *cfg = hc_spi->spi_config;
			SPI_TypeDef *spi = cfg->spi;

			/* Set the status not ready. Prepare RX after CS deassertion */
			tx_status(spi, EC_SPI_NOT_READY);
			hc_spi->state = SPI_HOST_CMD_STATE_RX_NOT_READY;
		}
	}
}

static int spi_init(const struct ec_host_cmd_spi_ctx *hc_spi)
{
	int err;

	if (!device_is_ready(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE))) {
		LOG_ERR("Clock control device not ready");
		return -ENODEV;
	}

	err = clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
			       (clock_control_subsys_t)&hc_spi->spi_config->pclken[0]);
	if (err < 0) {
		LOG_ERR("Could not enable SPI clock");
		return err;
	}

	if (IS_ENABLED(STM32_SPI_DOMAIN_CLOCK_SUPPORT) && (hc_spi->spi_config->pclk_len > 1)) {
		err = clock_control_configure(
			DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
			(clock_control_subsys_t)&hc_spi->spi_config->pclken[1], NULL);
		if (err < 0) {
			LOG_ERR("Could not select SPI domain clock");
			return err;
		}
	}

	/* Configure dt provided device signals when available */
	err = pinctrl_apply_state(hc_spi->spi_config->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		LOG_ERR("SPI pinctrl setup failed (%d)", err);
		return err;
	}

	if ((hc_spi->dma_rx->dma_dev != NULL) && !device_is_ready(hc_spi->dma_rx->dma_dev)) {
		LOG_ERR("%s device not ready", hc_spi->dma_rx->dma_dev->name);
		return -ENODEV;
	}

	if ((hc_spi->dma_tx->dma_dev != NULL) && !device_is_ready(hc_spi->dma_tx->dma_dev)) {
		LOG_ERR("%s device not ready", hc_spi->dma_tx->dma_dev->name);
		return -ENODEV;
	}

	return 0;
}

static int spi_configure(const struct ec_host_cmd_spi_ctx *hc_spi)
{
	const struct ec_host_cmd_spi_cfg *cfg = hc_spi->spi_config;
	SPI_TypeDef *spi = cfg->spi;

#if defined(LL_SPI_PROTOCOL_MOTOROLA) && defined(SPI_CR2_FRF)
	LL_SPI_SetStandard(spi, LL_SPI_PROTOCOL_MOTOROLA);
#endif

	/* Disable before configuration */
	LL_SPI_Disable(spi);
	/* Set clock signal configuration */
	LL_SPI_SetClockPolarity(spi, LL_SPI_POLARITY_LOW);
	LL_SPI_SetClockPhase(spi, LL_SPI_PHASE_1EDGE);
	/* Set protocol parameters */
	LL_SPI_SetTransferDirection(spi, LL_SPI_FULL_DUPLEX);
	LL_SPI_SetTransferBitOrder(spi, LL_SPI_MSB_FIRST);
	LL_SPI_DisableCRC(spi);
	LL_SPI_SetDataWidth(spi, LL_SPI_DATAWIDTH_8BIT);
	/* Set slave options */
	LL_SPI_SetNSSMode(spi, LL_SPI_NSS_HARD_INPUT);
	LL_SPI_SetMode(spi, LL_SPI_MODE_SLAVE);

#ifdef EC_HOST_CMD_ST_STM32_FIFO
#ifdef EC_HOST_CMD_ST_STM32H7
	LL_SPI_SetFIFOThreshold(spi, LL_SPI_FIFO_TH_01DATA);
#else
	LL_SPI_SetRxFIFOThreshold(spi, LL_SPI_RX_FIFO_TH_QUARTER);
#endif /* EC_HOST_CMD_ST_STM32H7 */
#endif /* EC_HOST_CMD_ST_STM32_FIFO */

	return 0;
}

static int reload_dma_tx(struct ec_host_cmd_spi_ctx *hc_spi, size_t len)
{
	const struct ec_host_cmd_spi_cfg *cfg = hc_spi->spi_config;
	SPI_TypeDef *spi = cfg->spi;
	int ret;

	/* Set DMA at the beggining of the TX buffer and set the number of bytes to send */
	ret = dma_reload(hc_spi->dma_tx->dma_dev, hc_spi->dma_tx->channel, (uint32_t)hc_spi->tx_buf,
			 dma_dest_addr(spi), len);
	if (ret != 0) {
		return ret;
	}

	/* Start DMA transfer */
	ret = dma_start(hc_spi->dma_tx->dma_dev, hc_spi->dma_tx->channel);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

static int spi_config_dma_tx(struct ec_host_cmd_spi_ctx *hc_spi)
{
	const struct ec_host_cmd_spi_cfg *cfg = hc_spi->spi_config;
	SPI_TypeDef *spi = cfg->spi;
	struct dma_block_config *blk_cfg;
	struct dma_stream *stream = hc_spi->dma_tx;
	int ret;

	blk_cfg = &stream->dma_blk_cfg;

	/* Set configs for TX. This shouldn't be changed during communication */
	memset(blk_cfg, 0, sizeof(struct dma_block_config));
	blk_cfg->block_size = 0;

	/* The destination address is the SPI register */
	blk_cfg->dest_address = dma_dest_addr(spi);
	blk_cfg->dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;

	blk_cfg->source_address = (uint32_t)hc_spi->tx_buf;
	blk_cfg->source_addr_adj = DMA_ADDR_ADJ_INCREMENT;

	blk_cfg->fifo_mode_control = hc_spi->dma_tx->fifo_threshold;

	stream->dma_cfg.head_block = blk_cfg;
	stream->dma_cfg.user_data = hc_spi;

	/* Configure the TX the channels */
	ret = dma_config(hc_spi->dma_tx->dma_dev, hc_spi->dma_tx->channel, &stream->dma_cfg);

	if (ret != 0) {
		return ret;
	}

	return 0;
}

static int reload_dma_rx(struct ec_host_cmd_spi_ctx *hc_spi)
{
	const struct ec_host_cmd_spi_cfg *cfg = hc_spi->spi_config;
	SPI_TypeDef *spi = cfg->spi;
	int ret;

	/* Reload DMA to the beginning of the RX buffer */
	ret = dma_reload(hc_spi->dma_rx->dma_dev, hc_spi->dma_rx->channel, dma_source_addr(spi),
			 (uint32_t)hc_spi->rx_ctx->buf, CONFIG_EC_HOST_CMD_HANDLER_RX_BUFFER_SIZE);
	if (ret != 0) {
		return ret;
	}

	ret = dma_start(hc_spi->dma_rx->dma_dev, hc_spi->dma_rx->channel);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

static int spi_config_dma_rx(struct ec_host_cmd_spi_ctx *hc_spi)
{
	const struct ec_host_cmd_spi_cfg *cfg = hc_spi->spi_config;
	SPI_TypeDef *spi = cfg->spi;
	struct dma_block_config *blk_cfg;
	struct dma_stream *stream = hc_spi->dma_rx;
	int ret;

	blk_cfg = &stream->dma_blk_cfg;

	/* Set configs for RX. This shouldn't be changed during communication */
	memset(blk_cfg, 0, sizeof(struct dma_block_config));
	blk_cfg->block_size = CONFIG_EC_HOST_CMD_HANDLER_RX_BUFFER_SIZE;

	/* The destination address is our RX buffer */
	blk_cfg->dest_address = (uint32_t)hc_spi->rx_ctx->buf;
	blk_cfg->dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;

	/* The source address is the SPI register */
	blk_cfg->source_address = dma_source_addr(spi);
	blk_cfg->source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;

	blk_cfg->fifo_mode_control = hc_spi->dma_rx->fifo_threshold;

	stream->dma_cfg.head_block = blk_cfg;
	stream->dma_cfg.user_data = hc_spi;

	/* Configure the RX the channels */
	ret = dma_config(hc_spi->dma_rx->dma_dev, hc_spi->dma_rx->channel, &stream->dma_cfg);

	return ret;
}

static int prepare_rx(struct ec_host_cmd_spi_ctx *hc_spi)
{
	const struct ec_host_cmd_spi_cfg *cfg = hc_spi->spi_config;
	SPI_TypeDef *spi = cfg->spi;
	int ret;

	hc_spi->prepare_rx_later = 0;
	/* Flush RX buffer. It clears the RXNE(RX not empty) flag not to trigger
	 * the DMA transfer at the beginning of a new SPI transfer. The flag is
	 * set while sending response to host. The number of bytes to read can
	 * be bigger than 1 for chip families than need to bypass the DMA
	 * threshold.
	 */
	LL_SPI_ReceiveData8(spi);

	ret = reload_dma_rx(hc_spi);
	if (!ret) {
		tx_status(spi, EC_SPI_RX_READY);
		hc_spi->state = SPI_HOST_CMD_STATE_READY_TO_RX;
	}

	return ret;
}

static int spi_setup_dma(struct ec_host_cmd_spi_ctx *hc_spi)
{
	const struct ec_host_cmd_spi_cfg *cfg = hc_spi->spi_config;
	SPI_TypeDef *spi = cfg->spi;
	/* retrieve active RX DMA channel (used in callback) */
	int ret;

#ifdef EC_HOST_CMD_ST_STM32H7
	/* Set request before enabling (else SPI CFG1 reg is write protected) */
	LL_SPI_EnableDMAReq_RX(spi);
	LL_SPI_EnableDMAReq_TX(spi);

	LL_SPI_Enable(spi);
#else /* EC_HOST_CMD_ST_STM32H7 */
	LL_SPI_Enable(spi);
#endif /* !EC_HOST_CMD_ST_STM32H7 */

	ret = spi_config_dma_tx(hc_spi);
	if (ret != 0) {
		return ret;
	}

	ret = spi_config_dma_rx(hc_spi);
	if (ret != 0) {
		return ret;
	}

	/* Start receiving from the SPI Master */
	ret = dma_start(hc_spi->dma_rx->dma_dev, hc_spi->dma_rx->channel);
	if (ret != 0) {
		return ret;
	}

#ifndef EC_HOST_CMD_ST_STM32H7
	/* toggle the DMA request to restart the transfer */
	LL_SPI_EnableDMAReq_RX(spi);
	LL_SPI_EnableDMAReq_TX(spi);
#endif /* !EC_HOST_CMD_ST_STM32H7 */

	return 0;
}

static int wait_for_rx_bytes(struct ec_host_cmd_spi_ctx *hc_spi, int needed)
{
	int64_t deadline = k_ticks_to_us_floor64(k_uptime_ticks()) + EC_SPI_CMD_RX_TIMEOUT_US;
	int64_t current_time;
	struct dma_status stat;
	int ret;

	while (1) {
		uint32_t rx_bytes;

		current_time = k_ticks_to_us_floor64(k_uptime_ticks());

		ret = dma_get_status(hc_spi->dma_rx->dma_dev, hc_spi->dma_rx->channel, &stat);
		/* RX DMA is always programed to copy buffer size (max command size) */
		if (ret) {
			return ret;
		}

		rx_bytes = CONFIG_EC_HOST_CMD_HANDLER_RX_BUFFER_SIZE - stat.pending_length;
		if (rx_bytes >= needed) {
			return 0;
		}

		/* Make sure the SPI transfer is ongoing */
		ret = gpio_pin_get(hc_spi->cs.port, hc_spi->cs.pin);
		if (ret) {
			/* End of transfer - return instantly */
			return ret;
		}

		if (current_time >= deadline) {
			/* Timeout */
			return -EIO;
		}
	}
}

void gpio_cb_nss(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins)
{
	struct ec_host_cmd_spi_ctx *hc_spi =
		CONTAINER_OF(cb, struct ec_host_cmd_spi_ctx, cs_callback);
	const struct ec_host_cmd_spi_cfg *cfg = hc_spi->spi_config;
	SPI_TypeDef *spi = cfg->spi;
	int ret;

	/* CS deasserted. Setup for the next transaction */
	if (gpio_pin_get(hc_spi->cs.port, hc_spi->cs.pin)) {
		ec_host_cmd_pm_policy_state_lock_put(hc_spi);

		/* CS asserted during processing a command. Prepare for receiving after
		 * sending response.
		 */
		if (hc_spi->state == SPI_HOST_CMD_STATE_PROCESSING) {
			hc_spi->prepare_rx_later = 1;
			return;
		}

		ret = prepare_rx(hc_spi);
		if (ret) {
			LOG_ERR("Failed to prepare RX after CS deassertion");
		}

		return;
	}

	/* CS asserted. Receive full packet and call general handler */
	if (hc_spi->state == SPI_HOST_CMD_STATE_READY_TO_RX) {
		/* The SPI module and DMA are already configured and ready to receive data.
		 * Consider disabling the SPI module at the end of sending response and
		 * reenabling it here if there is a need to disable reset SPI module,
		 * because of unexpected states.
		 */
		int exp_size;

		hc_spi->state = SPI_HOST_CMD_STATE_RECEIVING;
		/* Don't allow system to suspend until the end of transfer. */
		ec_host_cmd_pm_policy_state_lock_get(hc_spi);

		/* Set TX register to send status */
		tx_status(spi, EC_SPI_RECEIVING);

		/* Get the header */
		if (wait_for_rx_bytes(hc_spi, RX_HEADER_SIZE)) {
			goto spi_bad_rx;
		}

		exp_size = expected_size((struct ec_host_cmd_request_header *)hc_spi->rx_ctx->buf);
		/* Get data bytes */
		if (exp_size > RX_HEADER_SIZE) {
			if (wait_for_rx_bytes(hc_spi, exp_size)) {
				goto spi_bad_rx;
			}
		}

		hc_spi->rx_ctx->len = exp_size;
		hc_spi->state = SPI_HOST_CMD_STATE_PROCESSING;
		tx_status(spi, EC_SPI_PROCESSING);
		ec_host_cmd_rx_notify();

		return;
	}

spi_bad_rx:
	tx_status(spi, EC_SPI_NOT_READY);
	hc_spi->state = SPI_HOST_CMD_STATE_RX_BAD;
}

static int ec_host_cmd_spi_init(const struct ec_host_cmd_backend *backend,
				struct ec_host_cmd_rx_ctx *rx_ctx, struct ec_host_cmd_tx_buf *tx)
{
	int ret;
	struct ec_host_cmd_spi_ctx *hc_spi = (struct ec_host_cmd_spi_ctx *)backend->ctx;
	const struct ec_host_cmd_spi_cfg *cfg = hc_spi->spi_config;
	SPI_TypeDef *spi = cfg->spi;

	hc_spi->state = SPI_HOST_CMD_STATE_DISABLED;

	/* SPI backend needs rx and tx buffers provided by the handler */
	if (!rx_ctx->buf || !tx->buf || !hc_spi->cs.port) {
		return -EIO;
	}

	gpio_init_callback(&hc_spi->cs_callback, gpio_cb_nss, BIT(hc_spi->cs.pin));
	gpio_add_callback(hc_spi->cs.port, &hc_spi->cs_callback);
	gpio_pin_interrupt_configure(hc_spi->cs.port, hc_spi->cs.pin, GPIO_INT_EDGE_BOTH);

	hc_spi->rx_ctx = rx_ctx;
	hc_spi->rx_ctx->len = 0;

	/* Buffer to transmit */
	hc_spi->tx_buf = tx->buf;
	hc_spi->tx = tx;
	/* Buffer for response from HC handler. Make space for preamble */
	hc_spi->tx->buf = (uint8_t *)hc_spi->tx->buf + sizeof(out_preamble);
	hc_spi->tx->len_max = hc_spi->tx->len_max - sizeof(out_preamble) - EC_SPI_PAST_END_LENGTH;

	/* Limit the requset/response max sizes */
	if (hc_spi->rx_ctx->len_max > SPI_MAX_REQ_SIZE) {
		hc_spi->rx_ctx->len_max = SPI_MAX_REQ_SIZE;
	}
	if (hc_spi->tx->len_max > SPI_MAX_RESP_SIZE) {
		hc_spi->tx->len_max = SPI_MAX_RESP_SIZE;
	}

	ret = spi_init(hc_spi);
	if (ret) {
		return ret;
	}

	ret = spi_configure(hc_spi);
	if (ret) {
		return ret;
	}

	ret = spi_setup_dma(hc_spi);
	if (ret) {
		return ret;
	}

	tx_status(spi, EC_SPI_RX_READY);
	hc_spi->state = SPI_HOST_CMD_STATE_READY_TO_RX;

	return ret;
}

static int ec_host_cmd_spi_send(const struct ec_host_cmd_backend *backend)
{
	struct ec_host_cmd_spi_ctx *hc_spi = (struct ec_host_cmd_spi_ctx *)backend->ctx;
	int ret = 0;
	int tx_size;

	dma_stop(hc_spi->dma_rx->dma_dev, hc_spi->dma_rx->channel);

	/* Add state bytes at the beggining and the end of the buffer to transmit */
	memcpy(hc_spi->tx_buf, out_preamble, sizeof(out_preamble));
	for (int i = 0; i < EC_SPI_PAST_END_LENGTH; i++) {
		hc_spi->tx_buf[sizeof(out_preamble) + hc_spi->tx->len + i] = EC_SPI_PAST_END;
	}
	tx_size = hc_spi->tx->len + sizeof(out_preamble) + EC_SPI_PAST_END_LENGTH;

	hc_spi->state = SPI_HOST_CMD_STATE_SENDING;

	ret = reload_dma_tx(hc_spi, tx_size);
	if (ret) {
		LOG_ERR("Failed to send response");
	}

	return ret;
}

static const struct ec_host_cmd_backend_api ec_host_cmd_api = {
	.init = &ec_host_cmd_spi_init,
	.send = &ec_host_cmd_spi_send,
};

EC_HOST_CMD_SPI_DEFINE(ec_host_cmd_spi);

struct ec_host_cmd_backend *ec_host_cmd_backend_get_spi(struct gpio_dt_spec *cs)
{
	struct ec_host_cmd_spi_ctx *hc_spi = ec_host_cmd_spi.ctx;

	hc_spi->cs = *cs;

	return &ec_host_cmd_spi;
}

#ifdef CONFIG_PM_DEVICE
static int ec_host_cmd_spi_stm32_pm_action(const struct device *dev,
					   enum pm_device_action action)
{
	const struct ec_host_cmd_backend *backend = (struct ec_host_cmd_backend *)dev->data;
	struct ec_host_cmd_spi_ctx *hc_spi = (struct ec_host_cmd_spi_ctx *)backend->ctx;
	const struct ec_host_cmd_spi_cfg *cfg = hc_spi->spi_config;
	int err;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		/* Set pins to active state */
		err = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
		if (err < 0) {
			return err;
		}

		/* Enable device clock */
		err = clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				       (clock_control_subsys_t)&cfg->pclken[0]);
		if (err < 0) {
			return err;
		}
		/* Enable CS interrupts. */
		if (hc_spi->cs.port) {
			gpio_pin_interrupt_configure_dt(&hc_spi->cs, GPIO_INT_EDGE_BOTH);
		}

		break;
	case PM_DEVICE_ACTION_SUSPEND:
#ifdef SPI_SR_BSY
		/* Wait 10ms for the end of transaction to prevent corruption of the last
		 * transfer
		 */
		WAIT_FOR((LL_SPI_IsActiveFlag_BSY(cfg->spi) == 0), 10 * USEC_PER_MSEC, NULL);
#endif
		/* Disable unnecessary interrupts. */
		if (hc_spi->cs.port) {
			gpio_pin_interrupt_configure_dt(&hc_spi->cs, GPIO_INT_DISABLE);
		}

		/* Stop device clock. */
		err = clock_control_off(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					(clock_control_subsys_t)&cfg->pclken[0]);
		if (err != 0) {
			return err;
		}

		/* Move pins to sleep state */
		err = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_SLEEP);
		if ((err < 0) && (err != -ENOENT)) {
			/* If returning -ENOENT, no pins where defined for sleep mode. */
			return err;
		}

		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

PM_DEVICE_DT_DEFINE(DT_CHOSEN(zephyr_host_cmd_spi_backend), ec_host_cmd_spi_stm32_pm_action);

DEVICE_DT_DEFINE(DT_CHOSEN(zephyr_host_cmd_spi_backend),
		 NULL,
		 PM_DEVICE_DT_GET(DT_CHOSEN(zephyr_host_cmd_spi_backend)),
		 &ec_host_cmd_spi, NULL,
		 PRE_KERNEL_1, CONFIG_EC_HOST_CMD_INIT_PRIORITY, NULL);

#ifdef CONFIG_EC_HOST_CMD_INITIALIZE_AT_BOOT
static int host_cmd_init(void)
{
	struct gpio_dt_spec cs = GPIO_DT_SPEC_GET(DT_CHOSEN(zephyr_host_cmd_spi_backend), cs_gpios);

	ec_host_cmd_init(ec_host_cmd_backend_get_spi(&cs));

	return 0;
}
SYS_INIT(host_cmd_init, POST_KERNEL, CONFIG_EC_HOST_CMD_INIT_PRIORITY);

#endif

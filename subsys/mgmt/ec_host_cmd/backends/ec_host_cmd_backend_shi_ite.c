/*
 * Copyright (c) 2022 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it8xxx2_shi

#include "ec_host_cmd_backend_shi.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/gpio/ite-it8xxx2-gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/ec_host_cmd/backend.h>
#include <zephyr/mgmt/ec_host_cmd/ec_host_cmd.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/pm/policy.h>

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1, "Invalid number of ITE SHI peripherals");

LOG_MODULE_REGISTER(host_cmd_shi_ite, CONFIG_EC_HC_LOG_LEVEL);

#define EC_SHI_PREAMBLE_LENGTH 4
#define EC_SHI_PAST_END_LENGTH 4
#define SPI_RX_MAX_FIFO_SIZE   DT_INST_PROP(0, buffer_rx_size)
#define SPI_TX_MAX_FIFO_SIZE   DT_INST_PROP(0, buffer_tx_size)

#define SHI_MAX_RESPONSE_SIZE                                                                      \
	(SPI_TX_MAX_FIFO_SIZE - EC_SHI_PREAMBLE_LENGTH - EC_SHI_PAST_END_LENGTH)

BUILD_ASSERT(CONFIG_EC_HOST_CMD_BACKEND_SHI_MAX_REQUEST <= SPI_RX_MAX_FIFO_SIZE,
	     "SHI max request size is too big");
BUILD_ASSERT(CONFIG_EC_HOST_CMD_BACKEND_SHI_MAX_RESPONSE <= SHI_MAX_RESPONSE_SIZE,
	     "SHI max response size is too big");

/* Parameters used by host protocols */
enum shi_state_machine {
	/* Interface is disabled */
	SHI_STATE_DISABLED,
	/* Ready to receive next request */
	SHI_STATE_READY_TO_RECV,
	/* Receiving request */
	SHI_STATE_RECEIVING,
	/* Processing request */
	SHI_STATE_PROCESSING,
	/* Received bad data */
	SHI_STATE_RX_BAD,

	SHI_STATE_COUNT,
};

/*
 * Structure shi_it8xxx2_cfg is about the setting of SHI,
 * this config will be used at initial time
 */
struct shi_it8xxx2_cfg {
	/* SHI alternate configuration */
	const struct pinctrl_dev_config *pcfg;
	/* Chip select pin */
	const struct gpio_dt_spec cs;
};

struct shi_it8xxx2_data {
	/* Peripheral data */
	struct ec_host_cmd_rx_ctx *rx_ctx;
	struct ec_host_cmd_tx_buf *tx;
	struct gpio_callback cs_cb;
	/* Current state */
	enum shi_state_machine shi_state;
	/* Buffers */
	uint8_t in_msg[SPI_RX_MAX_FIFO_SIZE] __aligned(4);
	uint8_t out_msg[SPI_TX_MAX_FIFO_SIZE] __aligned(4);
};

struct ec_host_cmd_shi_ite_ctx {
	/* SHI device instance */
	const struct device *dev;
};

static const uint8_t out_preamble[EC_SHI_PREAMBLE_LENGTH] = {
	EC_SHI_PROCESSING,
	EC_SHI_PROCESSING,
	EC_SHI_PROCESSING,
	/* This is the byte which matters */
	EC_SHI_FRAME_START,
};

static const int shi_ite_response_state[] = {
	[SHI_STATE_DISABLED] = EC_SHI_NOT_READY,  [SHI_STATE_READY_TO_RECV] = EC_SHI_RX_READY,
	[SHI_STATE_RECEIVING] = EC_SHI_RECEIVING, [SHI_STATE_PROCESSING] = EC_SHI_PROCESSING,
	[SHI_STATE_RX_BAD] = EC_SHI_RX_BAD_DATA,
};
BUILD_ASSERT(ARRAY_SIZE(shi_ite_response_state) == SHI_STATE_COUNT);

#define EC_HOST_CMD_SHI_ITE_DEFINE(_name)                                                          \
	static struct ec_host_cmd_shi_ite_ctx _name##_hc_shi_ite;                                  \
	struct ec_host_cmd_backend _name = {                                                       \
		.api = &ec_host_cmd_api,                                                           \
		.ctx = (struct ec_host_cmd_shi_ite_ctx *)&_name##_hc_shi_ite,                      \
	}

static void shi_ite_set_state(struct shi_it8xxx2_data *data, int state)
{
	/* SPI peripheral state machine */
	data->shi_state = state;

	/* Response spi peripheral state */
	IT83XX_SPI_SPISRDR = shi_ite_response_state[state];
}

static void shi_ite_reset_rx_fifo(void)
{
	/* End Rx FIFO access */
	IT83XX_SPI_TXRXFAR = 0x00;

	/* Rx FIFO reset and count monitor reset */
	IT83XX_SPI_FCR = IT83XX_SPI_RXFR | IT83XX_SPI_RXFCMR;
}

/* This routine handles spi received unexcepted data */
static void shi_ite_bad_received_data(const struct device *dev, int count)
{
	struct shi_it8xxx2_data *data = dev->data;

	/* State machine mismatch, timeout, or protocol we can't handle. */
	shi_ite_set_state(data, SHI_STATE_RX_BAD);

	/* End CPU access Rx FIFO, so it can clock in bytes from AP again. */
	IT83XX_SPI_TXRXFAR = 0;

	LOG_ERR("SPI rx bad data");
	LOG_HEXDUMP_DBG(data->in_msg, count, "in_msg=");
}

static void shi_ite_response_host_data(const struct device *dev, uint8_t *out_msg_addr, int tx_size)
{
	struct shi_it8xxx2_data *data = dev->data;

	/*
	 * Protect sequence of filling response packet for host.
	 * This will ensure CPU access FIFO is disabled at SPI end interrupt no
	 * matter the interrupt is triggered before or after the sequence.
	 */
	unsigned int key = irq_lock();

	if (data->shi_state == SHI_STATE_PROCESSING) {
		/* Tx FIFO reset and count monitor reset */
		IT83XX_SPI_TXFCR = IT83XX_SPI_TXFR | IT83XX_SPI_TXFCMR;

		/* CPU Tx FIFO1 and FIFO2 access */
		IT83XX_SPI_TXRXFAR = IT83XX_SPI_CPUTFA;

		for (int i = 0; i < tx_size; i += 4) {
			/* Write response data from out_msg buffer to Tx FIFO */
			IT83XX_SPI_CPUWTFDB0 = *(uint32_t *)(out_msg_addr + i);
		}

		/*
		 * After writing data to Tx FIFO is finished, this bit will
		 * be to indicate the SPI peripheral controller.
		 */
		IT83XX_SPI_TXFCR = IT83XX_SPI_TXFS;

		/* End Tx FIFO access */
		IT83XX_SPI_TXRXFAR = 0;

		/* SPI peripheral read Tx FIFO */
		IT83XX_SPI_FCR = IT83XX_SPI_SPISRTXF;
	}

	irq_unlock(key);
}

/*
 * Called to send a response back to the host.
 *
 * Some commands can continue for a while. This function is called by
 * host_command when it completes.
 */
static int shi_ite_backend_send(const struct ec_host_cmd_backend *backend)
{
	struct ec_host_cmd_shi_ite_ctx *hc_shi = (struct ec_host_cmd_shi_ite_ctx *)backend->ctx;
	struct shi_it8xxx2_data *data = hc_shi->dev->data;
	int tx_size;

	if (data->shi_state != SHI_STATE_PROCESSING) {
		LOG_ERR("The request data is not processing (state=%d)", data->shi_state);
		return -EBUSY;
	}

	/* Copy preamble */
	memcpy(data->out_msg, out_preamble, sizeof(out_preamble));

	/* Data to sent are already at "out_msg + sizeof(out_preamble)" memory address(tx buf
	 * assigned in the init function), prepared by the handler.
	 * Append our past-end byte, which we reserved space for.
	 */
	memset(&data->out_msg[sizeof(out_preamble) + data->tx->len], EC_SHI_PAST_END,
	       EC_SHI_PAST_END_LENGTH);

	tx_size = data->tx->len + EC_SHI_PREAMBLE_LENGTH + EC_SHI_PAST_END_LENGTH;

	/* Transmit the reply */
	shi_ite_response_host_data(hc_shi->dev, data->out_msg, tx_size);

	return 0;
}

/* Store request data from Rx FIFO to in_msg buffer */
static void shi_ite_host_request_data(uint8_t *in_msg_addr, int count)
{
	/* CPU Rx FIFO1 access */
	IT83XX_SPI_TXRXFAR = IT83XX_SPI_CPURXF1A;

	/*
	 * In shi_ite_parse_header, the request data will separate to write in_msg buffer so we
	 * cannot set CPU to end accessing Rx FIFO in this function. We will set
	 * IT83XX_SPI_TXRXFAR = 0 in shi_ite_reset_rx_fifo.
	 */
	for (int i = 0; i < count; i += 4) {
		/* Get data from master to buffer */
		*(uint32_t *)(in_msg_addr + i) = IT83XX_SPI_RXFRDRB0;
	}
}

static int shi_ite_host_request_expected_size(const struct ec_host_cmd_request_header *r)
{
	/* Check host request version */
	if (r->prtcl_ver != EC_HOST_REQUEST_VERSION) {
		return 0;
	}

	/* Reserved byte should be 0 */
	if (r->reserved) {
		return 0;
	}

	return sizeof(*r) + r->data_len;
}

/* Parse header for version of spi-protocol */
static void shi_ite_parse_header(const struct device *dev)
{
	struct shi_it8xxx2_data *data = dev->data;
	struct ec_host_cmd_request_header *r = (struct ec_host_cmd_request_header *)data->in_msg;

	/* Store request data from Rx FIFO to in_msg buffer (rx_ctx->buf) */
	shi_ite_host_request_data(data->in_msg, sizeof(*r));

	/* Protocol version 3 */
	if (r->prtcl_ver == EC_HOST_REQUEST_VERSION) {
		/* Check how big the packet should be */
		data->rx_ctx->len = shi_ite_host_request_expected_size(r);

		if (data->rx_ctx->len == 0 || data->rx_ctx->len > sizeof(data->in_msg)) {
			shi_ite_bad_received_data(dev, data->rx_ctx->len);
			return;
		}

		/* Store request data from Rx FIFO to in_msg buffer */
		shi_ite_host_request_data(data->rx_ctx->buf + sizeof(*r),
					  data->rx_ctx->len - sizeof(*r));

		k_sem_give(&data->rx_ctx->handler_owns);
	} else {
		/* Invalid version number */
		LOG_ERR("Invalid version number");
		shi_ite_bad_received_data(dev, 1);
	}
}

static void shi_ite_int_handler(const struct device *dev)
{
	struct shi_it8xxx2_data *data = dev->data;

	if (data->shi_state == SHI_STATE_DISABLED) {
		return;
	}

	/*
	 * The status of SPI end detection interrupt bit is set, it means that host command parse
	 * has been completed and AP has received the last byte which is EC_SHI_PAST_END from
	 * EC responded data, then AP ended the transaction.
	 */
	if (IT83XX_SPI_ISR & IT83XX_SPI_ENDDETECTINT) {
		/* Disable CPU access Rx FIFO to clock in data from AP again */
		IT83XX_SPI_TXRXFAR = 0;
		/* Ready to receive */
		shi_ite_set_state(data, SHI_STATE_READY_TO_RECV);
		/* CS# is deasserted, so write clear all slave status */
		IT83XX_SPI_ISR = 0xff;
		/* Allow the MCU to go into lower power mode */
		pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	}

	/*
	 * The status of Rx valid length interrupt bit is set that indicates reached target count
	 * (IT83XX_SPI_FTCB1R, IT83XX_SPI_FTCB0R) and the length field of the host requested data.
	 */
	if (IT83XX_SPI_RX_VLISR & IT83XX_SPI_RVLI) {
		/* write clear slave status */
		IT83XX_SPI_RX_VLISR = IT83XX_SPI_RVLI;
		/* Move to processing state */
		shi_ite_set_state(data, SHI_STATE_PROCESSING);
		/* Parse header for version of spi-protocol */
		shi_ite_parse_header(dev);
	}
}

void shi_ite_cs_callback(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins)
{
	struct shi_it8xxx2_data *data = CONTAINER_OF(cb, struct shi_it8xxx2_data, cs_cb);

	if (data->shi_state == SHI_STATE_DISABLED) {
		return;
	}

	/* Prevent the MCU from sleeping during the transmission */
	pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);

	/* Move to processing state */
	shi_ite_set_state(data, SHI_STATE_PROCESSING);
}

static int shi_ite_init_registers(const struct device *dev)
{
	const struct shi_it8xxx2_cfg *const cfg = dev->config;
	/* Set FIFO data target count */
	struct ec_host_cmd_request_header cmd_head;
	int status;

	/*
	 * Target count means the size of host request.
	 * And plus extra 4 bytes because the CPU accesses FIFO base on word. If host requested
	 * data length is one byte, we need to align the data length to 4 bytes.
	 */
	int target_count = sizeof(cmd_head) + 4;
	/* Offset of data_len member of host request. */
	int offset = (char *)&cmd_head.data_len - (char *)&cmd_head;

	IT83XX_SPI_FTCB1R = (target_count >> 8) & 0xff;
	IT83XX_SPI_FTCB0R = target_count & 0xff;

	/*
	 * The register setting can capture the length field of host
	 * request.
	 */
	IT83XX_SPI_TCCB1 = (offset >> 8) & 0xff;
	IT83XX_SPI_TCCB0 = offset & 0xff;

	/*
	 * Memory controller configuration register 3.
	 * bit6 : SPI pin function select (0b:Enable, 1b:Mask)
	 */
	IT83XX_GCTRL_MCCR3 |= IT83XX_GCTRL_SPISLVPFE;

	/* Set unused blocked byte */
	IT83XX_SPI_HPR2 = 0x00;

	/* Rx valid length interrupt enabled */
	IT83XX_SPI_RX_VLISMR &= ~IT83XX_SPI_RVLIM;

	/*
	 * General control register2
	 * bit4 : Rx FIFO2 will not be overwrited once it's full.
	 * bit3 : Rx FIFO1 will not be overwrited once it's full.
	 * bit0 : Rx FIFO1/FIFO2 will reset after each CS_N goes high.
	 */
	IT83XX_SPI_GCR2 = IT83XX_SPI_RXF2OC | IT83XX_SPI_RXF1OC | IT83XX_SPI_RXFAR;

	/*
	 * Interrupt mask register (0b:Enable, 1b:Mask)
	 * bit5 : Rx byte reach interrupt mask
	 * bit2 : SPI end detection interrupt mask
	 */
	IT83XX_SPI_IMR &= ~IT83XX_SPI_EDIM;

	/* Reset fifo and prepare to for next transaction */
	shi_ite_reset_rx_fifo();

	/* Ready to receive */
	shi_ite_set_state(dev->data, SHI_STATE_READY_TO_RECV);

	/* Interrupt status register(write one to clear) */
	IT83XX_SPI_ISR = 0xff;

	/* SPI peripheral controller enable (after settings are ready) */
	IT83XX_SPI_SPISGCR = IT83XX_SPI_SPISCEN;

	/* Set the pin to SHI alternate function. */
	status = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (status < 0) {
		LOG_ERR("Failed to configure SHI pins");
		return status;
	}

	/* Enable SPI peripheral interrupt */
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), shi_ite_int_handler,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));

	return 0;
}

static int shi_ite_init(const struct device *dev)
{
	const struct shi_it8xxx2_cfg *cfg = dev->config;
	struct shi_it8xxx2_data *data = dev->data;
	int ret;

	ret = shi_ite_init_registers(dev);
	if (ret) {
		return ret;
	}

	/* Configure the SPI chip select */
	ret = gpio_pin_configure(cfg->cs.port, cfg->cs.pin, GPIO_INPUT | cfg->cs.dt_flags);
	if (ret < 0) {
		LOG_ERR("Failed to configure SHI CS pin");
		return ret;
	}

	/* Enable SPI chip select pin interrupt */
	gpio_init_callback(&data->cs_cb, shi_ite_cs_callback, BIT(cfg->cs.pin));
	ret = gpio_add_callback(cfg->cs.port, &data->cs_cb);
	if (ret < 0) {
		return -EINVAL;
	}

	ret = gpio_pin_interrupt_configure(cfg->cs.port, cfg->cs.pin, GPIO_INT_EDGE_FALLING);
	if (ret < 0) {
		LOG_ERR("Failed to configure SHI CS interrupt");
		return -EINVAL;
	}

	pm_device_init_suspended(dev);
	return pm_device_runtime_enable(dev);
}

static int shi_ite_backend_init(const struct ec_host_cmd_backend *backend,
				struct ec_host_cmd_rx_ctx *rx_ctx, struct ec_host_cmd_tx_buf *tx)
{
	struct ec_host_cmd_shi_ite_ctx *hc_shi = (struct ec_host_cmd_shi_ite_ctx *)backend->ctx;
	struct shi_it8xxx2_data *data;

	hc_shi->dev = DEVICE_DT_INST_GET(0);
	if (!device_is_ready(hc_shi->dev)) {
		return -ENODEV;
	}

	data = hc_shi->dev->data;
	data->rx_ctx = rx_ctx;
	data->tx = tx;

	rx_ctx->buf = data->in_msg;
	tx->buf = data->out_msg + sizeof(out_preamble);
	data->tx->len_max = sizeof(data->out_msg) - EC_SHI_PREAMBLE_LENGTH - EC_SHI_PAST_END_LENGTH;

	return 0;
}

PINCTRL_DT_INST_DEFINE(0);

#ifdef CONFIG_PM_DEVICE
static int shi_ite_pm_cb(const struct device *dev, enum pm_device_action action)
{
	struct shi_it8xxx2_data *data = dev->data;
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		shi_ite_set_state(data, SHI_STATE_DISABLED);
		break;
	case PM_DEVICE_ACTION_RESUME:
		shi_ite_set_state(data, SHI_STATE_READY_TO_RECV);
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}
#endif

/* Assume only one peripheral */
PM_DEVICE_DT_INST_DEFINE(0, shi_ite_pm_cb);

static const struct ec_host_cmd_backend_api ec_host_cmd_api = {
	.init = shi_ite_backend_init,
	.send = shi_ite_backend_send,
};

static const struct shi_it8xxx2_cfg shi_cfg = {
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.cs = GPIO_DT_SPEC_GET_BY_IDX(DT_DRV_INST(0), cs_gpios, 0),
};

static struct shi_it8xxx2_data shi_data = {
	.shi_state = SHI_STATE_DISABLED,
};

DEVICE_DT_INST_DEFINE(0, shi_ite_init, PM_DEVICE_DT_INST_GET(0), &shi_data, &shi_cfg, POST_KERNEL,
		      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &ec_host_cmd_api);

EC_HOST_CMD_SHI_ITE_DEFINE(ec_host_cmd_shi_ite);

struct ec_host_cmd_backend *ec_host_cmd_backend_get_shi_ite(void)
{
	return &ec_host_cmd_shi_ite;
}

#if DT_NODE_EXISTS(DT_CHOSEN(zephyr_host_cmd_shi_backend))
static int host_cmd_init(void)
{

	ec_host_cmd_init(ec_host_cmd_backend_get_shi_ite());
	return 0;
}
SYS_INIT(host_cmd_init, POST_KERNEL, CONFIG_EC_HOST_CMD_INIT_PRIORITY);
#endif

/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Driver for the Smartbond USB device controller.
 */

#include <string.h>

#include "udc_common.h"

#include <DA1469xAB.h>
#include <da1469x_config.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/drivers/usb/usb_dc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/smartbond_clock_control.h>

#include <zephyr/logging/log.h>
#include <zephyr/pm/policy.h>

LOG_MODULE_REGISTER(udc_smartbond, CONFIG_UDC_DRIVER_LOG_LEVEL);

/* Size of hardware RX and TX FIFO. */
#define EP0_FIFO_SIZE 8
#define EP_FIFO_SIZE  64

/*
 * DA146xx register fields and bit mask are very long. Filed masks repeat register names.
 * Those convenience macros are a way to reduce complexity of register modification lines.
 */
#define GET_BIT(val, field)     (val & field##_Msk) >> field##_Pos
#define REG_GET_BIT(reg, field) (USB->reg & USB_##reg##_##field##_Msk)
#define REG_SET_BIT(reg, field) (USB->reg |= USB_##reg##_##field##_Msk)
#define REG_CLR_BIT(reg, field) (USB->reg &= ~USB_##reg##_##field##_Msk)
#define REG_SET_VAL(reg, field, val)                                                               \
	(USB->reg = (USB->reg & ~USB_##reg##_##field##_Msk) | (val << USB_##reg##_##field##_Pos))

struct usb_smartbond_dma_config {
	int tx_chan;
	int rx_chan;
	uint8_t tx_slot_mux;
	uint8_t rx_slot_mux;
	const struct device *tx_dev;
	const struct device *rx_dev;
};

struct udc_smartbond_config {
	IRQn_Type udc_irq;
	IRQn_Type vbus_irq;
	uint8_t fifo_read_threshold;
	uint8_t num_of_eps;
	uint16_t dma_min_transfer_size;
	struct usb_smartbond_dma_config dma_cfg;
};

/* Node functional states */
#define NFSR_NODE_RESET       0
#define NFSR_NODE_RESUME      1
#define NFSR_NODE_OPERATIONAL 2
#define NFSR_NODE_SUSPEND     3
/*
 * Those two following states are added to allow going out of sleep mode
 * using frame interrupt.  On remove wakeup RESUME state must be kept for
 * at least 1ms. It is accomplished by using FRAME interrupt that goes
 * through those two fake states before entering OPERATIONAL state.
 */
#define NFSR_NODE_WAKING      (0x10 | (NFSR_NODE_RESUME))
#define NFSR_NODE_WAKING2     (0x20 | (NFSR_NODE_RESUME))

struct smartbond_ep_reg_set {
	volatile uint32_t epc_in;
	volatile uint32_t txd;
	volatile uint32_t txs;
	volatile uint32_t txc;
	volatile uint32_t epc_out;
	volatile uint32_t rxd;
	volatile uint32_t rxs;
	volatile uint32_t rxc;
};

struct smartbond_ep_state {
	struct udc_ep_config config;
	struct smartbond_ep_reg_set *regs;
	struct net_buf *buf;
	/** Packet size sent or received so far. It is used to modify transferred field
	 * after ACK is received or when filling ISO endpoint with size larger then
	 * FIFO size.
	 */
	uint16_t last_packet_size;
	uint8_t iso: 1; /** ISO endpoint */
};

struct usb_smartbond_dma_data {
	struct dma_config tx_cfg;
	struct dma_config rx_cfg;
	struct dma_block_config tx_block_cfg;
	struct dma_block_config rx_block_cfg;
};

struct usb_smartbond_data {
	struct udc_data udc_data;
	struct usb_smartbond_dma_data dma_data;
	const struct device *dev;
	struct k_work ep0_setup_work;
	struct k_work ep0_tx_work;
	struct k_work ep0_rx_work;
	uint8_t setup_buffer[8];
	bool vbus_present;
	bool attached;
	atomic_t clk_requested;
	uint8_t nfsr;
	struct smartbond_ep_state ep_state[2][4];
	atomic_ptr_t dma_ep[2]; /** DMA used by channel */
};

#define EP0_OUT_STATE(data) (&data->ep_state[0][0])
#define EP0_IN_STATE(data)  (&data->ep_state[1][0])

static int usb_smartbond_dma_config(struct usb_smartbond_data *data)
{
	const struct udc_smartbond_config *config = data->dev->config;
	const struct usb_smartbond_dma_config *dma_cfg = &config->dma_cfg;
	struct dma_config *tx = &data->dma_data.tx_cfg;
	struct dma_config *rx = &data->dma_data.rx_cfg;
	struct dma_block_config *tx_block = &data->dma_data.tx_block_cfg;
	struct dma_block_config *rx_block = &data->dma_data.rx_block_cfg;

	if (dma_request_channel(dma_cfg->rx_dev, (void *)&dma_cfg->rx_chan) < 0) {
		LOG_ERR("RX DMA channel is already occupied");
		return -EIO;
	}

	if (dma_request_channel(dma_cfg->tx_dev, (void *)&dma_cfg->tx_chan) < 0) {
		LOG_ERR("TX DMA channel is already occupied");
		return -EIO;
	}

	tx->channel_direction = MEMORY_TO_PERIPHERAL;
	tx->dma_callback = NULL;
	tx->user_data = NULL;
	tx->block_count = 1;
	tx->head_block = tx_block;

	tx->error_callback_dis = 1;
	/* DMA callback is not used */
	tx->complete_callback_en = 1;

	tx->dma_slot = dma_cfg->tx_slot_mux;
	tx->channel_priority = 7;

	/* Burst mode is not using when DREQ is one */
	tx->source_burst_length = 1;
	tx->dest_burst_length = 1;
	/* USB is byte-oriented protocol */
	tx->source_data_size = 1;
	tx->dest_data_size = 1;

	/* Do not change */
	tx_block->dest_addr_adj = 0x2;
	/* Incremental */
	tx_block->source_addr_adj = 0x0;

	/* Should reflect TX buffer */
	tx_block->source_address = 0;
	/* Should reflect USB TX FIFO. Temporarily assign an SRAM location. */
	tx_block->dest_address = MCU_SYSRAM_M_BASE;
	/* Should reflect total bytes to be transmitted */
	tx_block->block_size = 0;

	rx->channel_direction = PERIPHERAL_TO_MEMORY;
	rx->dma_callback = NULL;
	rx->user_data = NULL;
	rx->block_count = 1;
	rx->head_block = rx_block;

	rx->error_callback_dis = 1;
	/* DMA callback is not used */
	rx->complete_callback_en = 1;

	rx->dma_slot = dma_cfg->rx_slot_mux;
	rx->channel_priority = 2;

	/* Burst mode is not using when DREQ is one */
	rx->source_burst_length = 1;
	rx->dest_burst_length = 1;
	/* USB is byte-oriented protocol */
	rx->source_data_size = 1;
	rx->dest_data_size = 1;

	/* Do not change */
	rx_block->source_addr_adj = 0x2;
	/* Incremental */
	rx_block->dest_addr_adj = 0x0;

	/* Should reflect USB RX FIFO */
	rx_block->source_address = 0;
	/* Should reflect RX buffer. Temporarily assign an SRAM location. */
	rx_block->dest_address = MCU_SYSRAM_M_BASE;
	/* Should reflect total bytes to be received */
	rx_block->block_size = 0;

	if (dma_config(dma_cfg->rx_dev, dma_cfg->rx_chan, rx) < 0) {
		LOG_ERR("RX DMA configuration failed");
		return -EINVAL;
	}

	if (dma_config(dma_cfg->tx_dev, dma_cfg->tx_chan, tx) < 0) {
		LOG_ERR("TX DMA configuration failed");
		return -EINVAL;
	}

	return 0;
}

static void usb_smartbond_dma_deconfig(struct usb_smartbond_data *data)
{
	const struct udc_smartbond_config *config = data->dev->config;
	const struct usb_smartbond_dma_config *dma_cfg = &config->dma_cfg;

	dma_stop(dma_cfg->tx_dev, dma_cfg->tx_chan);
	dma_stop(dma_cfg->rx_dev, dma_cfg->rx_chan);

	dma_release_channel(dma_cfg->tx_dev, dma_cfg->tx_chan);
	dma_release_channel(dma_cfg->rx_dev, dma_cfg->rx_chan);
}

static struct smartbond_ep_state *usb_dc_get_ep_state(struct usb_smartbond_data *data, uint8_t ep)
{
	const struct udc_smartbond_config *config = data->dev->config;

	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	uint8_t ep_dir = USB_EP_GET_DIR(ep) ? 1 : 0;

	return (ep_idx < config->num_of_eps) ? &data->ep_state[ep_dir][ep_idx] : NULL;
}

static struct smartbond_ep_state *usb_dc_get_ep_out_state(struct usb_smartbond_data *data,
							  uint8_t ep_idx)
{
	const struct udc_smartbond_config *config = data->dev->config;

	return ep_idx < config->num_of_eps ? &data->ep_state[0][ep_idx] : NULL;
}

static struct smartbond_ep_state *usb_dc_get_ep_in_state(struct usb_smartbond_data *data,
							 uint8_t ep_idx)
{
	const struct udc_smartbond_config *config = data->dev->config;

	return ep_idx < config->num_of_eps ? &data->ep_state[1][ep_idx] : NULL;
}

static void set_nfsr(struct usb_smartbond_data *data, uint8_t val)
{
	data->nfsr = val;
	/*
	 * Write only lower 2 bits to register, higher bits are used
	 * to count down till OPERATIONAL state can be entered when
	 * remote wakeup activated.
	 */
	USB->USB_NFSR_REG = val & 3;
}

static void fill_tx_fifo(struct smartbond_ep_state *ep_state)
{
	int remaining;
	const uint8_t *src;
	struct smartbond_ep_reg_set *regs = ep_state->regs;
	struct net_buf *buf = ep_state->buf;
	const struct udc_ep_config *const ep_cfg = &ep_state->config;
	const uint16_t mps = udc_mps_ep_size(ep_cfg);
	const uint8_t ep_idx = USB_EP_GET_IDX(ep_cfg->addr);

	src = buf->data;
	remaining = buf->len;
	if (remaining > mps - ep_state->last_packet_size) {
		remaining = mps - ep_state->last_packet_size;
	}

	/*
	 * Loop checks TCOUNT all the time since this value is saturated to 31
	 * and can't be read just once before.
	 */
	while ((regs->txs & USB_USB_TXS1_REG_USB_TCOUNT_Msk) > 0 && remaining > 0) {
		regs->txd = *src++;
		ep_state->last_packet_size++;
		remaining--;
	}

	/*
	 * Setup FIFO level warning in case whole packet could not be placed
	 * in FIFO at once. This case only applies to ISO endpoints with packet
	 * size grater then 64. All other packets will fit in corresponding
	 * FIFO and there is no need for enabling FIFO level interrupt.
	 */
	if (ep_idx == 0 || ep_cfg->mps <= EP_FIFO_SIZE) {
		return;
	}

	if (remaining > 0) {
		/*
		 * Max packet size is set to value greater then FIFO.
		 * Enable fifo level warning to handle larger packets.
		 */
		regs->txc |= (3 << USB_USB_TXC1_REG_USB_TFWL_Pos);
		USB->USB_FWMSK_REG |= BIT(ep_idx - 1 + USB_USB_FWMSK_REG_USB_M_TXWARN31_Pos);
	} else {
		regs->txc &= ~USB_USB_TXC1_REG_USB_TFWL_Msk;
		USB->USB_FWMSK_REG &= ~(BIT(ep_idx - 1 + USB_USB_FWMSK_REG_USB_M_TXWARN31_Pos));
		/* Whole packet already in fifo, no need to
		 * refill it later.  Mark last.
		 */
		regs->txc |= USB_USB_TXC1_REG_USB_LAST_Msk;
	}
}

static bool try_allocate_dma(struct usb_smartbond_data *data, struct smartbond_ep_state *ep_state)
{
	struct udc_ep_config *const ep_cfg = &ep_state->config;
	const uint8_t ep = ep_cfg->addr;
	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	uint8_t dir_ix = USB_EP_DIR_IS_OUT(ep) ? 0 : 1;

	if (atomic_ptr_cas(&data->dma_ep[dir_ix], NULL, ep_state)) {
		if (dir_ix == 0) {
			USB->USB_DMA_CTRL_REG =
				(USB->USB_DMA_CTRL_REG & ~USB_USB_DMA_CTRL_REG_USB_DMA_RX_Msk) |
				((ep_idx - 1) << USB_USB_DMA_CTRL_REG_USB_DMA_RX_Pos);
		} else {
			USB->USB_DMA_CTRL_REG =
				(USB->USB_DMA_CTRL_REG & ~USB_USB_DMA_CTRL_REG_USB_DMA_TX_Msk) |
				((ep_idx - 1) << USB_USB_DMA_CTRL_REG_USB_DMA_TX_Pos);
		}
		USB->USB_DMA_CTRL_REG |= USB_USB_DMA_CTRL_REG_USB_DMA_EN_Msk;
		return true;
	} else {
		return false;
	}
}

static void start_rx_dma(const struct usb_smartbond_dma_config *dma_cfg, uintptr_t src,
			 uintptr_t dst, uint16_t size)
{
	if (dma_reload(dma_cfg->rx_dev, dma_cfg->rx_chan, src, dst, size) < 0) {
		LOG_ERR("Failed to reload RX DMA");
	} else {
		dma_start(dma_cfg->rx_dev, dma_cfg->rx_chan);
	}
}

static void start_rx_packet(struct usb_smartbond_data *data, struct smartbond_ep_state *ep_state)
{
	struct udc_ep_config *const ep_cfg = &ep_state->config;
	const struct udc_smartbond_config *config = data->dev->config;
	const uint8_t ep = ep_cfg->addr;
	struct smartbond_ep_reg_set *regs = ep_state->regs;
	struct net_buf *buf = ep_state->buf;
	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	const uint16_t mps = udc_mps_ep_size(ep_cfg);
	uint8_t rxc = regs->rxc | USB_USB_RXC1_REG_USB_RX_EN_Msk;

	LOG_DBG("Start rx ep 0x%02x", ep);

	ep_state->last_packet_size = 0;

	if (mps > config->dma_min_transfer_size) {
		if (try_allocate_dma(data, ep_state)) {
			start_rx_dma(&config->dma_cfg, (uintptr_t)&regs->rxd,
				     (uintptr_t)net_buf_tail(buf), mps);
		} else if (mps > EP_FIFO_SIZE) {
			/*
			 * Other endpoint is using DMA in that direction,
			 * fall back to interrupts.
			 * For endpoint size greater than FIFO size,
			 * enable FIFO level warning interrupt when FIFO
			 * has less than 17 bytes free.
			 */
			rxc |= USB_USB_RXC1_REG_USB_RFWL_Msk;
			USB->USB_FWMSK_REG |=
				BIT(ep_idx - 1 + USB_USB_FWMSK_REG_USB_M_RXWARN31_Pos);
		}
	} else if (ep_idx != 0) {
		/* If max_packet_size would fit in FIFO no need
		 * for FIFO level warning interrupt.
		 */
		rxc &= ~USB_USB_RXC1_REG_USB_RFWL_Msk;
		USB->USB_FWMSK_REG &= ~(BIT(ep_idx - 1 + USB_USB_FWMSK_REG_USB_M_RXWARN31_Pos));
	}

	regs->rxc = rxc;
}

static void start_tx_dma(const struct usb_smartbond_dma_config *dma_cfg, uintptr_t src,
			 uintptr_t dst, uint16_t size)
{
	if (dma_reload(dma_cfg->tx_dev, dma_cfg->tx_chan, src, dst, size) < 0) {
		LOG_ERR("Failed to reload TX DMA");
	} else {
		dma_start(dma_cfg->tx_dev, dma_cfg->tx_chan);
	}
}

static void start_tx_packet(struct usb_smartbond_data *data, struct smartbond_ep_state *ep_state)
{
	const struct udc_smartbond_config *config = data->dev->config;
	struct smartbond_ep_reg_set *regs = ep_state->regs;
	struct udc_ep_config *const ep_cfg = &ep_state->config;
	struct net_buf *buf = ep_state->buf;
	const uint8_t ep = ep_cfg->addr;
	uint16_t remaining = buf->len;
	const uint16_t mps = udc_mps_ep_size(ep_cfg);
	uint16_t size = MIN(remaining, mps);
	uint8_t txc;

	LOG_DBG("ep 0x%02x %d/%d", ep, size, remaining);

	ep_state->last_packet_size = 0;

	regs->txc = USB_USB_TXC1_REG_USB_FLUSH_Msk;

	txc = USB_USB_TXC1_REG_USB_TX_EN_Msk | USB_USB_TXC1_REG_USB_LAST_Msk;
	if (ep_cfg->stat.data1) {
		txc |= USB_USB_TXC1_REG_USB_TOGGLE_TX_Msk;
	}

	if (ep != USB_CONTROL_EP_IN && size > config->dma_min_transfer_size &&
	    (uint32_t)(buf->data) >= CONFIG_SRAM_BASE_ADDRESS && try_allocate_dma(data, ep_state)) {
		start_tx_dma(&config->dma_cfg, (uintptr_t)buf->data, (uintptr_t)&regs->txd, size);
	} else {
		fill_tx_fifo(ep_state);
	}

	regs->txc = txc;
	if (ep == USB_CONTROL_EP_IN) {
		(void)USB->USB_EP0_NAK_REG;
		/*
		 * While driver expects upper layer to send data to the host,
		 * code should detect EP0 NAK event that could mean that
		 * host already sent ZLP without waiting for all requested
		 * data.
		 */
		REG_SET_BIT(USB_MAMSK_REG, USB_M_EP0_NAK);
	}
}

static uint16_t read_rx_fifo(struct smartbond_ep_state *ep_state, uint8_t *dst,
			     uint16_t bytes_in_fifo)
{
	struct smartbond_ep_reg_set *regs = ep_state->regs;
	struct udc_ep_config *const ep_cfg = &ep_state->config;
	const uint16_t mps = udc_mps_ep_size(ep_cfg);
	uint16_t remaining = mps - ep_state->last_packet_size;
	uint16_t receive_this_time = bytes_in_fifo;

	if (remaining < bytes_in_fifo) {
		receive_this_time = remaining;
	}

	for (int i = 0; i < receive_this_time; ++i) {
		dst[i] = regs->rxd;
	}

	ep_state->last_packet_size += receive_this_time;

	return bytes_in_fifo - receive_this_time;
}

static void handle_ep0_rx(struct usb_smartbond_data *data)
{
	int fifo_bytes;
	uint32_t rxs0 = USB->USB_RXS0_REG;
	struct smartbond_ep_state *ep0_out_state = EP0_OUT_STATE(data);
	struct udc_ep_config *ep0_out_config = &ep0_out_state->config;
	struct smartbond_ep_state *ep0_in_state;
	struct udc_ep_config *ep0_in_config;
	struct net_buf *buf = ep0_out_state->buf;

	fifo_bytes = GET_BIT(rxs0, USB_USB_RXS0_REG_USB_RCOUNT);

	if (rxs0 & USB_USB_RXS0_REG_USB_SETUP_Msk) {
		ep0_in_state = EP0_IN_STATE(data);
		ep0_in_config = &ep0_in_state->config;
		ep0_out_state->last_packet_size = 0;
		read_rx_fifo(ep0_out_state, data->setup_buffer, EP0_FIFO_SIZE);

		ep0_out_config->stat.halted = 0;
		ep0_out_config->stat.data1 = 1;
		ep0_in_config->stat.halted = 0;
		ep0_in_config->stat.data1 = 1;
		REG_SET_BIT(USB_TXC0_REG, USB_TOGGLE_TX0);
		REG_CLR_BIT(USB_EPC0_REG, USB_STALL);
		LOG_HEXDUMP_DBG(data->setup_buffer, 8, "setup");
		REG_CLR_BIT(USB_MAMSK_REG, USB_M_EP0_NAK);
		REG_CLR_BIT(USB_RXC0_REG, USB_RX_EN);
		(void)USB->USB_EP0_NAK_REG;
		k_work_submit_to_queue(udc_get_work_q(), &data->ep0_setup_work);
	} else {
		(void)USB->USB_EP0_NAK_REG;
		if (GET_BIT(rxs0, USB_USB_RXS0_REG_USB_TOGGLE_RX0) != ep0_out_config->stat.data1) {
			/* Toggle bit does not match discard packet */
			REG_SET_BIT(USB_RXC0_REG, USB_FLUSH);
			ep0_out_state->last_packet_size = 0;
			LOG_WRN("Packet with incorrect data1 bit rejected");
		} else {
			read_rx_fifo(ep0_out_state,
				     net_buf_tail(buf) + ep0_out_state->last_packet_size,
				     fifo_bytes);
			if (rxs0 & USB_USB_RXS0_REG_USB_RX_LAST_Msk) {
				ep0_out_config->stat.data1 ^= 1;
				net_buf_add(ep0_out_state->buf, ep0_out_state->last_packet_size);
				if (ep0_out_state->last_packet_size < EP0_FIFO_SIZE ||
				    ep0_out_state->buf->len == 0) {
					k_work_submit_to_queue(udc_get_work_q(),
							       &data->ep0_rx_work);
				} else {
					start_rx_packet(data, ep0_out_state);
				}
			}
		}
	}
}

static void udc_smartbond_ep_abort(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	struct smartbond_ep_state *ep_state = (struct smartbond_ep_state *)(ep_cfg);
	struct usb_smartbond_data *data = udc_get_private(dev);
	const struct udc_smartbond_config *config = data->dev->config;

	/* Stop DMA if it used by this endpoint */
	if (data->dma_ep[0] == ep_state) {
		dma_stop(config->dma_cfg.rx_dev, config->dma_cfg.rx_chan);
		data->dma_ep[0] = NULL;
	} else if (data->dma_ep[1] == ep_state) {
		dma_stop(config->dma_cfg.tx_dev, config->dma_cfg.tx_chan);
		data->dma_ep[1] = NULL;
	}
	/* Flush FIFO */
	if (USB_EP_DIR_IS_OUT(ep_cfg->addr)) {
		ep_state->regs->rxc |= USB_USB_RXC0_REG_USB_FLUSH_Msk;
		ep_state->regs->rxc &= ~USB_USB_RXC0_REG_USB_FLUSH_Msk;
	} else {
		ep_state->regs->txc |= USB_USB_TXC0_REG_USB_FLUSH_Msk;
		ep_state->regs->txc &= ~USB_USB_TXC0_REG_USB_FLUSH_Msk;
	}
}

static int udc_smartbond_ep_tx(const struct device *dev, uint8_t ep)
{
	struct usb_smartbond_data *data = dev->data;
	struct smartbond_ep_state *ep_state = usb_dc_get_ep_in_state(data, USB_EP_GET_IDX(ep));
	struct net_buf *buf;

	if (udc_ep_is_busy(dev, ep) ||
	    (ep_state->regs->epc_in & USB_USB_EPC1_REG_USB_STALL_Msk) != 0) {
		return 0;
	}

	buf = udc_buf_peek(dev, ep);
	LOG_DBG("TX ep 0x%02x len %u", ep, buf ? buf->len : -1);

	if (buf) {
		ep_state->buf = buf;
		ep_state->last_packet_size = 0;

		start_tx_packet(data, ep_state);

		udc_ep_set_busy(dev, ep, true);
	}

	return 0;
}

static int udc_smartbond_ep_rx(const struct device *dev, uint8_t ep)
{
	struct usb_smartbond_data *data = dev->data;
	struct smartbond_ep_state *ep_state = usb_dc_get_ep_out_state(data, USB_EP_GET_IDX(ep));
	struct net_buf *buf;

	if (udc_ep_is_busy(dev, ep)) {
		return 0;
	}

	buf = udc_buf_peek(dev, ep);

	if (buf) {
		LOG_DBG("RX ep 0x%02x len %u", ep, buf->size);

		ep_state->last_packet_size = 0;
		ep_state->buf = buf;

		start_rx_packet(data, ep_state);

		udc_ep_set_busy(dev, ep, true);
	}

	return 0;
}

static int udc_smartbond_ep_enqueue(const struct device *dev, struct udc_ep_config *const ep_cfg,
				    struct net_buf *buf)
{
	unsigned int lock_key;
	const uint8_t ep = ep_cfg->addr;
	int ret;

	LOG_DBG("ep 0x%02x enqueue %p", ep, buf);
	udc_buf_put(ep_cfg, buf);

	if (ep_cfg->stat.halted) {
		/*
		 * It is fine to enqueue a transfer for a halted endpoint,
		 * you need to make sure that transfers are re-triggered when
		 * the halt is cleared.
		 */
		LOG_DBG("ep 0x%02x halted", ep);
		return 0;
	}

	lock_key = irq_lock();

	if (USB_EP_DIR_IS_IN(ep)) {
		ret = udc_smartbond_ep_tx(dev, ep);
	} else {
		ret = udc_smartbond_ep_rx(dev, ep);
	}

	irq_unlock(lock_key);

	return ret;
}

static int udc_smartbond_ep_dequeue(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	const uint8_t ep = ep_cfg->addr;
	unsigned int lock_key;
	struct net_buf *buf;

	LOG_INF("ep 0x%02x dequeue all", ep);

	lock_key = irq_lock();

	udc_smartbond_ep_abort(dev, ep_cfg);

	buf = udc_buf_get_all(dev, ep);
	if (buf) {
		udc_submit_ep_event(dev, buf, -ECONNABORTED);
	}

	udc_ep_set_busy(dev, ep, false);

	irq_unlock(lock_key);

	return 0;
}

int udc_smartbond_ep_enable(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	const uint8_t ep = ep_cfg->addr;
	struct smartbond_ep_state *ep_state;
	bool iso = (ep_cfg->attributes & USB_EP_TRANSFER_TYPE_MASK) == USB_EP_TYPE_ISO;
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	ARG_UNUSED(dev);

	LOG_INF("Enable ep 0x%02x", ep);

	ep_state = (struct smartbond_ep_state *)(ep_cfg);
	if (USB_EP_DIR_IS_IN(ep)) {
		ep_state->regs->txc &= ~USB_USB_TXC0_REG_USB_IGN_IN_Msk;
		if (ep != USB_CONTROL_EP_IN) {
			ep_state->regs->epc_in |= USB_USB_EPC1_REG_USB_EP_EN_Msk |
						  USB_EP_GET_IDX(ep) |
						  (iso ? USB_USB_EPC2_REG_USB_ISO_Msk : 0);
			USB->USB_TXMSK_REG |= 0x11 << (ep_idx - 1);
			REG_SET_BIT(USB_MAMSK_REG, USB_M_TX_EV);
		}
	} else {
		ep_state->regs->rxc &= ~USB_USB_RXC0_REG_USB_IGN_OUT_Msk;
		if (ep == USB_CONTROL_EP_OUT) {
			ep_state->regs->rxc &= ~USB_USB_RXC0_REG_USB_IGN_SETUP_Msk;
		} else {
			ep_state->regs->epc_out = USB_USB_EPC2_REG_USB_EP_EN_Msk |
						  USB_EP_GET_IDX(ep) |
						  (iso ? USB_USB_EPC2_REG_USB_ISO_Msk : 0);
			USB->USB_RXMSK_REG |= 0x11 << (ep_idx - 1);
			REG_SET_BIT(USB_MAMSK_REG, USB_M_RX_EV);
		}
	}

	return 0;
}

static int udc_smartbond_ep_disable(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	struct usb_smartbond_data *data = udc_get_private(dev);
	const uint8_t ep = ep_cfg->addr;
	struct smartbond_ep_state *ep_state;

	LOG_INF("Disable ep 0x%02x", ep);

	ep_state = usb_dc_get_ep_state(data, ep);
	if (USB_EP_DIR_IS_IN(ep)) {
		ep_state->regs->txc =
			USB_USB_TXC0_REG_USB_IGN_IN_Msk | USB_USB_TXC0_REG_USB_FLUSH_Msk;
	} else {
		ep_state->regs->rxc =
			USB_USB_RXC0_REG_USB_IGN_SETUP_Msk | USB_USB_RXC0_REG_USB_IGN_OUT_Msk;
	}

	return 0;
}

static int udc_smartbond_ep_set_halt(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	struct smartbond_ep_state *ep_state = (struct smartbond_ep_state *)(ep_cfg);
	struct net_buf *buf;
	const uint8_t ep = ep_cfg->addr;

	LOG_DBG("Set halt ep 0x%02x", ep);

	ep_cfg->stat.halted = 1;
	if (ep_cfg->addr == USB_CONTROL_EP_IN) {
		/* Stall in DATA IN phase, drop status OUT packet */
		if (udc_ctrl_stage_is_data_in(dev)) {
			buf = udc_buf_get(dev, USB_CONTROL_EP_OUT);
			if (buf) {
				net_buf_unref(buf);
			}
		}
		USB->USB_RXC0_REG = USB_USB_RXC0_REG_USB_FLUSH_Msk;
		USB->USB_EPC0_REG |= USB_USB_EPC0_REG_USB_STALL_Msk;
		USB->USB_TXC0_REG |= USB_USB_TXC0_REG_USB_TX_EN_Msk;
	} else if (ep == USB_CONTROL_EP_OUT) {
		ep_state->regs->rxc |= USB_USB_RXC0_REG_USB_RX_EN_Msk;
		ep_state->regs->epc_in |= USB_USB_EPC0_REG_USB_STALL_Msk;
	} else if (USB_EP_DIR_IS_OUT(ep)) {
		ep_state->regs->epc_out = USB_USB_EPC1_REG_USB_STALL_Msk;
		ep_state->regs->rxc = USB_USB_RXC1_REG_USB_RX_EN_Msk;
	} else {
		ep_state->regs->epc_in |= USB_USB_EPC1_REG_USB_STALL_Msk;
		ep_state->regs->txc =
			USB_USB_TXC1_REG_USB_TX_EN_Msk | USB_USB_TXC1_REG_USB_LAST_Msk;
	}

	return 0;
}

static int udc_smartbond_ep_clear_halt(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	const uint8_t ep = ep_cfg->addr;
	struct smartbond_ep_state *ep_state = (struct smartbond_ep_state *)(ep_cfg);

	LOG_DBG("Clear halt ep 0x%02x", ep);

	ep_cfg->stat.data1 = 0;
	ep_cfg->stat.halted = 0;

	if (ep == USB_CONTROL_EP_OUT || ep == USB_CONTROL_EP_IN) {
		REG_CLR_BIT(USB_MAMSK_REG, USB_M_EP0_NAK);
	}

	if (USB_EP_DIR_IS_OUT(ep)) {
		ep_state->regs->epc_out &= ~USB_USB_EPC1_REG_USB_STALL_Msk;
		udc_smartbond_ep_rx(dev, ep);
	} else {
		ep_state->regs->epc_in &= ~USB_USB_EPC1_REG_USB_STALL_Msk;
		udc_smartbond_ep_tx(dev, ep);
	}

	return 0;
}

static int udc_smartbond_set_address(const struct device *dev, const uint8_t addr)
{
	ARG_UNUSED(dev);

	LOG_DBG("Set new address %u for %p", addr, dev);

	USB->USB_FAR_REG = (addr & USB_USB_FAR_REG_USB_AD_Msk) | USB_USB_FAR_REG_USB_AD_EN_Msk;

	return 0;
}

static int udc_smartbond_host_wakeup(const struct device *dev)
{
	struct usb_smartbond_data *data = udc_get_private(dev);

	LOG_DBG("Remote wakeup from %p", dev);

	if (data->nfsr == NFSR_NODE_SUSPEND) {
		/*
		 * Enter fake state that will use FRAME interrupt to wait before
		 * going operational.
		 */
		set_nfsr(data, NFSR_NODE_WAKING);
		USB->USB_MAMSK_REG |= USB_USB_MAMSK_REG_USB_M_FRAME_Msk;
	}

	return 0;
}

static enum udc_bus_speed udc_smartbond_device_speed(const struct device *dev)
{
	ARG_UNUSED(dev);

	return UDC_BUS_SPEED_FS;
}

static int udc_smartbond_shutdown(const struct device *dev)
{
	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_OUT)) {
		LOG_ERR("Failed to disable control endpoint");
		return -EIO;
	}

	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_IN)) {
		LOG_ERR("Failed to disable control endpoint");
		return -EIO;
	}

	return 0;
}

static uint32_t check_reset_end(struct usb_smartbond_data *data, uint32_t alt_ev)
{
	if (data->nfsr == NFSR_NODE_RESET) {
		if (GET_BIT(alt_ev, USB_USB_ALTEV_REG_USB_RESET)) {
			/*
			 * Could be still in reset, but since USB_M_RESET is
			 * disabled it can be also old reset state that was not
			 * cleared yet.
			 * If (after reading USB_ALTEV_REG register again)
			 * bit is cleared reset state just ended.
			 * Keep non-reset bits combined from two previous
			 * ALTEV reads and one from the next line.
			 */
			alt_ev = (alt_ev & ~USB_USB_ALTEV_REG_USB_RESET_Msk) | USB->USB_ALTEV_REG;
		}

		if (GET_BIT(alt_ev, USB_USB_ALTEV_REG_USB_RESET) == 0) {
			USB->USB_ALTMSK_REG =
				USB_USB_ALTMSK_REG_USB_M_RESET_Msk | USB_USB_ALTEV_REG_USB_SD3_Msk;
			if (data->ep_state[0][0].buf != NULL) {
				USB->USB_MAMSK_REG |= USB_USB_MAMSK_REG_USB_M_EP0_RX_Msk;
			}
			LOG_DBG("Set operational %02x", USB->USB_MAMSK_REG);
			set_nfsr(data, NFSR_NODE_OPERATIONAL);
		}
	}
	return alt_ev;
}

void handle_ep0_tx(struct usb_smartbond_data *data)
{
	uint32_t txs0;
	struct smartbond_ep_state *ep0_in_state = EP0_IN_STATE(data);
	const uint8_t ep = USB_CONTROL_EP_IN;
	struct smartbond_ep_reg_set *regs = ep0_in_state->regs;
	struct udc_ep_config *ep0_in_config = &ep0_in_state->config;
	struct net_buf *buf = ep0_in_state->buf;
	bool start_next_packet = true;

	txs0 = regs->txs;

	LOG_DBG("%02x %02x", ep, txs0);

	if (GET_BIT(txs0, USB_USB_TXS0_REG_USB_TX_DONE)) {
		/* ACK received */
		if (GET_BIT(txs0, USB_USB_TXS0_REG_USB_ACK_STAT)) {
			net_buf_pull(buf, ep0_in_state->last_packet_size);
			ep0_in_state->last_packet_size = 0;
			ep0_in_config->stat.data1 ^= 1;
			REG_SET_VAL(USB_TXC0_REG, USB_TOGGLE_TX0, ep0_in_config->stat.data1);

			/*
			 * Packet was sent to host but host already sent OUT packet
			 * that was NAK'ed. It means that no more data is needed.
			 */
			if (USB->USB_EP0_NAK_REG & USB_USB_EP0_NAK_REG_USB_EP0_OUTNAK_Msk) {
				net_buf_pull(buf, buf->len);
				udc_ep_buf_clear_zlp(buf);
			}
			if (buf->len == 0) {
				/* When everything was sent there is not need to fill new packet */
				start_next_packet = false;
				/* Send ZLP if protocol needs it */
				if (udc_ep_buf_has_zlp(buf)) {
					udc_ep_buf_clear_zlp(buf);
					/* Enable transmitter without putting anything in FIFO */
					USB->USB_TXC0_REG |= USB_USB_TXC0_REG_USB_TX_EN_Msk;
				} else {
					REG_CLR_BIT(USB_MAMSK_REG, USB_M_EP0_NAK);
					k_work_submit_to_queue(udc_get_work_q(),
							       &data->ep0_tx_work);
				}
			}
		} else {
			/* Start from the beginning */
			ep0_in_state->last_packet_size = 0;
		}
		if (start_next_packet) {
			start_tx_packet(data, ep0_in_state);
		}
	}
}

static void handle_epx_rx_ev(struct usb_smartbond_data *data, uint8_t ep_idx)
{
	uint32_t rxs;
	int fifo_bytes;
	struct smartbond_ep_state *ep_state = usb_dc_get_ep_out_state(data, ep_idx);
	const struct udc_smartbond_config *config = data->dev->config;
	struct smartbond_ep_reg_set *regs = ep_state->regs;
	struct udc_ep_config *const ep_cfg = &ep_state->config;
	struct net_buf *buf = ep_state->buf;

	do {
		rxs = regs->rxs;

		if (GET_BIT(rxs, USB_USB_RXS1_REG_USB_RX_ERR)) {
			regs->rxc |= USB_USB_RXC1_REG_USB_FLUSH_Msk;
			ep_state->last_packet_size = 0;
			if (data->dma_ep[0] == ep_state) {
				/* Stop DMA */
				dma_stop(config->dma_cfg.rx_dev, config->dma_cfg.rx_chan);
				/* Restart DMA since packet was dropped,
				 * all parameters should still work.
				 */
				dma_start(config->dma_cfg.rx_dev, config->dma_cfg.rx_chan);
			}
			break;
		}

		if (data->dma_ep[0] == ep_state) {
			struct dma_status rx_dma_status;

			dma_get_status(config->dma_cfg.rx_dev, config->dma_cfg.rx_chan,
				       &rx_dma_status);
			/*
			 * Disable DMA and update last_packet_size
			 * with what DMA reported.
			 */
			dma_stop(config->dma_cfg.rx_dev, config->dma_cfg.rx_chan);
			ep_state->last_packet_size = rx_dma_status.total_copied;

			/*
			 * When DMA did not finished (packet was smaller then MPS),
			 * dma_idx holds exact number of bytes transmitted. When DMA
			 * finished value in dma_idx is one less then actual number of
			 * transmitted bytes.
			 */
			if (ep_state->last_packet_size ==
			    (rx_dma_status.total_copied + rx_dma_status.pending_length)) {
				ep_state->last_packet_size++;
			}
			/* Release DMA to use by other endpoints. */
			data->dma_ep[0] = NULL;
		}
		fifo_bytes = GET_BIT(rxs, USB_USB_RXS1_REG_USB_RXCOUNT);
		/*
		 * FIFO maybe empty if DMA read it before or
		 * it's final iteration and function already read all
		 * that was to read.
		 */
		if (fifo_bytes > 0) {
			fifo_bytes = read_rx_fifo(ep_state,
						  net_buf_tail(buf) + ep_state->last_packet_size,
						  fifo_bytes);
		}

		if (GET_BIT(rxs, USB_USB_RXS1_REG_USB_RX_LAST)) {
			if (!ep_state->iso &&
			    GET_BIT(rxs, USB_USB_RXS1_REG_USB_TOGGLE_RX) != ep_cfg->stat.data1) {
				/* Toggle bit does not match discard packet */
				regs->rxc |= USB_USB_RXC1_REG_USB_FLUSH_Msk;
				ep_state->last_packet_size = 0;
				LOG_WRN("Packet with incorrect data1 field rejected");
				/* Re-enable reception */
				start_rx_packet(data, ep_state);
			} else {
				ep_cfg->stat.data1 ^= 1;
				REG_SET_VAL(USB_TXC0_REG, USB_TOGGLE_TX0, ep_cfg->stat.data1);
				net_buf_add(buf, ep_state->last_packet_size);

				if (net_buf_tailroom(buf) == 0 ||
				    ep_state->last_packet_size < udc_mps_ep_size(ep_cfg) ||
				    ep_state->iso) {
					buf = udc_buf_get(data->dev, ep_cfg->addr);
					if (unlikely(buf == NULL)) {
						LOG_ERR("ep 0x%02x queue is empty", ep_cfg->addr);
						break;
					}
					ep_cfg->stat.busy = 0;
					udc_submit_ep_event(data->dev, buf, 0);
					break;
				}
				start_rx_packet(data, ep_state);
			}
		}
	} while (fifo_bytes > config->fifo_read_threshold);
}

static void handle_rx_ev(struct usb_smartbond_data *data)
{
	if (USB->USB_RXEV_REG & BIT(0)) {
		handle_epx_rx_ev(data, 1);
	}

	if (USB->USB_RXEV_REG & BIT(1)) {
		handle_epx_rx_ev(data, 2);
	}

	if (USB->USB_RXEV_REG & BIT(2)) {
		handle_epx_rx_ev(data, 3);
	}
}

static void handle_epx_tx_ev(struct usb_smartbond_data *data, struct smartbond_ep_state *ep_state)
{
	uint32_t txs;
	const struct udc_smartbond_config *config = data->dev->config;
	struct smartbond_ep_reg_set *regs = ep_state->regs;
	struct udc_ep_config *const ep_cfg = &ep_state->config;
	struct net_buf *buf = ep_state->buf;
	const uint8_t ep = ep_cfg->addr;

	txs = regs->txs;

	if (GET_BIT(txs, USB_USB_TXS1_REG_USB_TX_DONE)) {
		if (data->dma_ep[1] == ep_state) {
			struct dma_status tx_dma_status;

			dma_get_status(config->dma_cfg.tx_dev, config->dma_cfg.tx_chan,
				       &tx_dma_status);
			/*
			 * Disable DMA and update last_packet_size with what
			 * DMA reported.
			 */
			dma_stop(config->dma_cfg.tx_dev, config->dma_cfg.tx_chan);
			ep_state->last_packet_size = tx_dma_status.total_copied + 1;
			/* Release DMA to used by other endpoints. */
			data->dma_ep[1] = NULL;
		}

		if (GET_BIT(txs, USB_USB_TXS1_REG_USB_ACK_STAT)) {
			/* ACK received, update transfer state and DATA0/1 bit */
			net_buf_pull(buf, ep_state->last_packet_size);
			ep_state->last_packet_size = 0;
			ep_cfg->stat.data1 ^= 1;
			REG_SET_VAL(USB_TXC0_REG, USB_TOGGLE_TX0, ep_cfg->stat.data1);

			if (buf->len == 0) {
				if (udc_ep_buf_has_zlp(buf)) {
					udc_ep_buf_clear_zlp(buf);
					/* Enable transmitter without putting anything in FIFO */
					regs->txc |= USB_USB_TXC1_REG_USB_TX_EN_Msk |
						     USB_USB_TXC1_REG_USB_LAST_Msk;
				} else {
					udc_ep_set_busy(data->dev, ep, false);
					buf = udc_buf_get(data->dev, ep);

					udc_submit_ep_event(data->dev, buf, 0);
					udc_smartbond_ep_tx(data->dev, ep);
				}
				return;
			}
		} else if (regs->epc_in & USB_USB_EPC1_REG_USB_STALL_Msk) {
			/*
			 * TX_DONE also indicates that STALL packet was just sent,
			 * there is no point to put anything into transmit FIFO.
			 * It could result in empty packet being scheduled.
			 */
			return;
		}
	}

	if (txs & USB_USB_TXS1_REG_USB_TX_URUN_Msk) {
		LOG_DBG("EP 0x%02x FIFO under-run\n", ep);
	}
	/* Start next or repeated packet. */
	start_tx_packet(data, ep_state);
}

static void handle_tx_ev(struct usb_smartbond_data *data)
{
	if (USB->USB_TXEV_REG & BIT(0)) {
		handle_epx_tx_ev(data, usb_dc_get_ep_in_state(data, 1));
	}
	if (USB->USB_TXEV_REG & BIT(1)) {
		handle_epx_tx_ev(data, usb_dc_get_ep_in_state(data, 2));
	}
	if (USB->USB_TXEV_REG & BIT(2)) {
		handle_epx_tx_ev(data, usb_dc_get_ep_in_state(data, 3));
	}
}

static void handle_epx_tx_warn_ev(struct usb_smartbond_data *data, uint8_t ep_idx)
{
	fill_tx_fifo(usb_dc_get_ep_in_state(data, ep_idx));
}

static void handle_fifo_warning(struct usb_smartbond_data *data)
{
	uint32_t fifo_warning = USB->USB_FWEV_REG;

	if (fifo_warning & BIT(0)) {
		handle_epx_tx_warn_ev(data, 1);
	}

	if (fifo_warning & BIT(1)) {
		handle_epx_tx_warn_ev(data, 2);
	}

	if (fifo_warning & BIT(2)) {
		handle_epx_tx_warn_ev(data, 3);
	}

	if (fifo_warning & BIT(4)) {
		handle_epx_rx_ev(data, 1);
	}

	if (fifo_warning & BIT(5)) {
		handle_epx_rx_ev(data, 2);
	}

	if (fifo_warning & BIT(6)) {
		handle_epx_rx_ev(data, 3);
	}
}

static void handle_ep0_nak(struct usb_smartbond_data *data)
{
	uint32_t ep0_nak = USB->USB_EP0_NAK_REG;

	if (REG_GET_BIT(USB_EPC0_REG, USB_STALL)) {
		if (GET_BIT(ep0_nak, USB_USB_EP0_NAK_REG_USB_EP0_INNAK)) {
			/*
			 * EP0 is stalled and NAK was sent, it means that
			 * RX is enabled. Disable RX for now.
			 */
			REG_CLR_BIT(USB_RXC0_REG, USB_RX_EN);
			REG_SET_BIT(USB_TXC0_REG, USB_TX_EN);
		}

		if (GET_BIT(ep0_nak, USB_USB_EP0_NAK_REG_USB_EP0_OUTNAK)) {
			REG_SET_BIT(USB_RXC0_REG, USB_RX_EN);
		}
	} else {
		REG_CLR_BIT(USB_MAMSK_REG, USB_M_EP0_NAK);
		if (REG_GET_BIT(USB_RXC0_REG, USB_RX_EN) == 0 &&
		    GET_BIT(ep0_nak, USB_USB_EP0_NAK_REG_USB_EP0_OUTNAK)) {
			(void)USB->USB_EP0_NAK_REG;
			k_work_submit_to_queue(udc_get_work_q(), &data->ep0_tx_work);
		}
	}
}

static void empty_ep0_queues(const struct device *dev)
{
	struct net_buf *buf;

	buf = udc_buf_get_all(dev, USB_CONTROL_EP_OUT);
	if (buf) {
		net_buf_unref(buf);
	}
	buf = udc_buf_get_all(dev, USB_CONTROL_EP_IN);
	if (buf) {
		net_buf_unref(buf);
	}
}

static void handle_bus_reset(struct usb_smartbond_data *data)
{
	const struct udc_smartbond_config *config = data->dev->config;
	uint32_t alt_ev;

	USB->USB_NFSR_REG = 0;
	USB->USB_FAR_REG = 0x80;
	USB->USB_ALTMSK_REG = 0;
	USB->USB_NFSR_REG = NFSR_NODE_RESET;
	USB->USB_TXMSK_REG = 0;
	USB->USB_RXMSK_REG = 0;
	set_nfsr(data, NFSR_NODE_RESET);

	for (int i = 0; i < config->num_of_eps; ++i) {
		data->ep_state[1][i].buf = NULL;
		data->ep_state[1][i].config.stat.busy = 0;
	}

	LOG_INF("send USB_DC_RESET");
	udc_submit_event(data->dev, UDC_EVT_RESET, 0);
	USB->USB_DMA_CTRL_REG = 0;

	USB->USB_MAMSK_REG = USB_USB_MAMSK_REG_USB_M_INTR_Msk | USB_USB_MAMSK_REG_USB_M_FRAME_Msk |
			     USB_USB_MAMSK_REG_USB_M_WARN_Msk | USB_USB_MAMSK_REG_USB_M_ALT_Msk |
			     USB_USB_MAMSK_REG_USB_M_EP0_RX_Msk |
			     USB_USB_MAMSK_REG_USB_M_EP0_TX_Msk;
	USB->USB_ALTMSK_REG = USB_USB_ALTMSK_REG_USB_M_RESUME_Msk;
	alt_ev = USB->USB_ALTEV_REG;
	check_reset_end(data, alt_ev);
	empty_ep0_queues(data->dev);
}

static void usb_clock_on(struct usb_smartbond_data *data)
{
	if (atomic_cas(&data->clk_requested, 0, 1)) {
		clock_control_on(DEVICE_DT_GET(DT_NODELABEL(osc)),
				 (clock_control_subsys_rate_t)SMARTBOND_CLK_USB);
	}
}

static void usb_clock_off(struct usb_smartbond_data *data)
{
	if (atomic_cas(&data->clk_requested, 1, 0)) {
		clock_control_off(DEVICE_DT_GET(DT_NODELABEL(osc)),
				  (clock_control_subsys_rate_t)SMARTBOND_CLK_USB);
	}
}

static void handle_alt_ev(struct usb_smartbond_data *data)
{
	const struct udc_smartbond_config *config = data->dev->config;
	struct smartbond_ep_state *ep_state;
	uint32_t alt_ev = USB->USB_ALTEV_REG;

	if (USB->USB_NFSR_REG == NFSR_NODE_SUSPEND) {
		usb_clock_on(data);
	}
	alt_ev = check_reset_end(data, alt_ev);
	if (GET_BIT(alt_ev, USB_USB_ALTEV_REG_USB_RESET) && data->nfsr != NFSR_NODE_RESET) {
		handle_bus_reset(data);
	} else if (GET_BIT(alt_ev, USB_USB_ALTEV_REG_USB_RESUME)) {
		if (USB->USB_NFSR_REG == NFSR_NODE_SUSPEND) {
			set_nfsr(data, NFSR_NODE_OPERATIONAL);
			if (data->ep_state[0][0].buf != NULL) {
				USB->USB_MAMSK_REG |= USB_USB_MAMSK_REG_USB_M_EP0_RX_Msk;
			}
			USB->USB_ALTMSK_REG = USB_USB_ALTMSK_REG_USB_M_RESET_Msk |
					      USB_USB_ALTMSK_REG_USB_M_SD3_Msk;
			/* Re-enable reception of endpoint with pending transfer */
			for (int ep_idx = 1; ep_idx < config->num_of_eps; ++ep_idx) {
				ep_state = usb_dc_get_ep_out_state(data, ep_idx);
				if (!ep_state->config.stat.halted) {
					start_rx_packet(data, ep_state);
				}
			}
			udc_submit_event(data->dev, UDC_EVT_RESUME, 0);
		}
	} else if (GET_BIT(alt_ev, USB_USB_ALTEV_REG_USB_SD3)) {
		set_nfsr(data, NFSR_NODE_SUSPEND);
		USB->USB_ALTMSK_REG =
			USB_USB_ALTMSK_REG_USB_M_RESET_Msk | USB_USB_ALTMSK_REG_USB_M_RESUME_Msk;
		usb_clock_off(data);
		udc_submit_event(data->dev, UDC_EVT_SUSPEND, 0);
	}
}

static void udc_smartbond_isr(const struct device *dev)
{
	struct usb_smartbond_data *data = udc_get_private(dev);
	uint32_t int_status = USB->USB_MAEV_REG & USB->USB_MAMSK_REG;

	if (GET_BIT(int_status, USB_USB_MAEV_REG_USB_WARN)) {
		handle_fifo_warning(data);
	}

	if (GET_BIT(int_status, USB_USB_MAEV_REG_USB_CH_EV)) {
		/* For now just clear interrupt */
		(void)USB->USB_CHARGER_STAT_REG;
	}

	if (GET_BIT(int_status, USB_USB_MAEV_REG_USB_EP0_TX)) {
		handle_ep0_tx(data);
	}

	if (GET_BIT(int_status, USB_USB_MAEV_REG_USB_EP0_RX)) {
		handle_ep0_rx(data);
	}

	if (GET_BIT(int_status, USB_USB_MAEV_REG_USB_EP0_NAK)) {
		handle_ep0_nak(data);
	}

	if (GET_BIT(int_status, USB_USB_MAEV_REG_USB_RX_EV)) {
		handle_rx_ev(data);
	}

	if (GET_BIT(int_status, USB_USB_MAEV_REG_USB_NAK)) {
		(void)USB->USB_NAKEV_REG;
	}

	if (GET_BIT(int_status, USB_USB_MAEV_REG_USB_FRAME)) {
		if (data->nfsr == NFSR_NODE_RESET) {
			/*
			 * During reset FRAME interrupt is enabled to periodically
			 * check when reset state ends.
			 * FRAME interrupt is generated every 1ms without host sending
			 * actual SOF.
			 */
			check_reset_end(data, USB_USB_ALTEV_REG_USB_RESET_Msk);
		} else if (data->nfsr == NFSR_NODE_WAKING) {
			/* No need to call set_nfsr, just set state */
			data->nfsr = NFSR_NODE_WAKING2;
		} else if (data->nfsr == NFSR_NODE_WAKING2) {
			/* No need to call set_nfsr, just set state */
			data->nfsr = NFSR_NODE_RESUME;
			LOG_DBG("data->nfsr = NFSR_NODE_RESUME %02x", USB->USB_MAMSK_REG);
		} else if (data->nfsr == NFSR_NODE_RESUME) {
			set_nfsr(data, NFSR_NODE_OPERATIONAL);
			if (data->ep_state[0][0].buf != NULL) {
				USB->USB_MAMSK_REG |= USB_USB_MAMSK_REG_USB_M_EP0_RX_Msk;
			}
			LOG_DBG("Set operational %02x", USB->USB_MAMSK_REG);
		} else {
			USB->USB_MAMSK_REG &= ~USB_USB_MAMSK_REG_USB_M_FRAME_Msk;
		}
	}

	if (GET_BIT(int_status, USB_USB_MAEV_REG_USB_TX_EV)) {
		handle_tx_ev(data);
	}

	if (GET_BIT(int_status, USB_USB_MAEV_REG_USB_ALT)) {
		handle_alt_ev(data);
	}
}

/**
 * USB functionality can be disabled from HOST and DEVICE side.
 * Host side is indicated by VBUS line.
 * Device side is decided by pair of calls udc_enable()/udc_disable(),
 * USB will only work when application calls udc_enable() and VBUS is present.
 * When both conditions are not met USB clock (PLL) is released, and peripheral
 * remain in reset state.
 */
static void usb_change_state(struct usb_smartbond_data *data, bool attached, bool vbus_present)
{
	if (data->attached == attached && data->vbus_present == vbus_present) {
		return;
	}

	if (vbus_present != data->vbus_present && attached) {
		udc_submit_event(data->dev,
				 vbus_present ? UDC_EVT_VBUS_READY : UDC_EVT_VBUS_REMOVED, 0);
	}
	if (attached && vbus_present) {
		data->attached = true;
		data->vbus_present = true;
		/*
		 * Prevent transition to standby, this greatly reduces
		 * IRQ response time
		 */
		pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
		usb_smartbond_dma_config(data);
		usb_clock_on(data);
		USB->USB_MCTRL_REG = USB_USB_MCTRL_REG_USBEN_Msk;
		USB->USB_NFSR_REG = 0;
		USB->USB_FAR_REG = 0x80;
		USB->USB_TXMSK_REG = 0;
		USB->USB_RXMSK_REG = 0;

		USB->USB_MAMSK_REG = USB_USB_MAMSK_REG_USB_M_INTR_Msk |
				     USB_USB_MAMSK_REG_USB_M_ALT_Msk |
				     USB_USB_MAMSK_REG_USB_M_WARN_Msk;
		USB->USB_ALTMSK_REG =
			USB_USB_ALTMSK_REG_USB_M_RESET_Msk | USB_USB_ALTEV_REG_USB_SD3_Msk;

		USB->USB_MCTRL_REG = USB_USB_MCTRL_REG_USBEN_Msk | USB_USB_MCTRL_REG_USB_NAT_Msk;
	} else if (data->attached && data->vbus_present) {
		/*
		 * USB was previously in use now either VBUS is gone or application
		 * requested detach, put it down
		 */
		data->attached = attached;
		data->vbus_present = vbus_present;
		/*
		 * It's imperative that USB_NAT bit-field is updated with the
		 * USBEN bit-field being set. As such, zeroing the control
		 * register at once will result in leaving the USB transceivers
		 * in a floating state. Such an action, will induce incorrect
		 * behavior for subsequent charger detection operations and given
		 * that the device does not enter the sleep state (thus powering off
		 * PD_SYS and resetting the controller along with its transceivers).
		 */
		REG_CLR_BIT(USB_MCTRL_REG, USB_NAT);
		USB->USB_MCTRL_REG = 0;
		usb_clock_off(data);
		usb_smartbond_dma_deconfig(data);
		/* Allow standby USB not in use or not connected */
		pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	} else {
		/* USB still not activated, keep track of what's on and off */
		data->attached = attached;
		data->vbus_present = vbus_present;
	}
}

static void usb_dc_smartbond_vbus_isr(struct usb_smartbond_data *data)
{
	LOG_DBG("VBUS_ISR");

	CRG_TOP->VBUS_IRQ_CLEAR_REG = 1;
	usb_change_state(data, data->attached,
			 (CRG_TOP->ANA_STATUS_REG & CRG_TOP_ANA_STATUS_REG_VBUS_AVAILABLE_Msk) !=
				 0);
}

static int usb_dc_smartbond_alloc_status_out(const struct device *dev)
{

	struct udc_ep_config *const ep_cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
	struct net_buf *buf;

	buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, 0);
	if (buf == NULL) {
		return -ENOMEM;
	}

	k_fifo_put(&ep_cfg->fifo, buf);

	return 0;
}

static int usbd_ctrl_feed_dout(const struct device *dev, const size_t length)
{
	struct usb_smartbond_data *data = udc_get_private(dev);
	struct smartbond_ep_state *ep_state = EP0_OUT_STATE(data);
	struct udc_ep_config *const ep_cfg = &ep_state->config;
	struct net_buf *buf;

	buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, length);
	if (buf == NULL) {
		return -ENOMEM;
	}

	k_fifo_put(&ep_cfg->fifo, buf);
	ep_state->buf = buf;
	start_rx_packet(data, ep_state);

	return 0;
}

static void handle_ep0_rx_work(struct k_work *item)
{
	struct usb_smartbond_data *data =
		CONTAINER_OF(item, struct usb_smartbond_data, ep0_rx_work);
	const uint8_t ep = USB_CONTROL_EP_OUT;
	struct net_buf *buf;
	const struct device *dev = data->dev;
	unsigned int lock_key;

	/*
	 * Lock needed here because busy is a bit field and access
	 * may result in wrong state of data1 field
	 */
	lock_key = irq_lock();

	udc_ep_set_busy(dev, ep, false);
	buf = udc_buf_get(dev, ep);

	irq_unlock(lock_key);
	if (unlikely(buf == NULL)) {
		LOG_ERR("ep 0x%02x queue is empty", ep);
		return;
	}
	/* Update packet size */
	if (udc_ctrl_stage_is_status_out(dev)) {
		udc_ctrl_update_stage(dev, buf);
		udc_ctrl_submit_status(dev, buf);
	} else {
		udc_ctrl_update_stage(dev, buf);
	}

	if (udc_ctrl_stage_is_status_in(dev)) {
		udc_ctrl_submit_s_out_status(dev, buf);
	}
}

static void handle_ep0_tx_work(struct k_work *item)
{
	struct usb_smartbond_data *data =
		CONTAINER_OF(item, struct usb_smartbond_data, ep0_tx_work);
	struct net_buf *buf;
	const struct device *dev = data->dev;
	const uint8_t ep = USB_CONTROL_EP_IN;
	unsigned int lock_key;

	buf = udc_buf_peek(dev, ep);
	__ASSERT(buf == EP0_IN_STATE(data)->buf, "TX work without buffer %p %p", buf,
		 EP0_IN_STATE(data)->buf);

	/*
	 * Lock needed here because busy is a bit field and access
	 * may result in wrong state of data1 filed
	 */
	lock_key = irq_lock();

	udc_ep_set_busy(dev, ep, false);

	/* Remove buffer from queue */
	buf = udc_buf_get(dev, ep);

	irq_unlock(lock_key);

	__ASSERT(buf == EP0_IN_STATE(data)->buf, "Internal error");

	/* For control endpoint get ready for ACK stage
	 * from host.
	 */
	if (udc_ctrl_stage_is_status_in(dev) || udc_ctrl_stage_is_no_data(dev)) {
		/* Status stage finished, notify upper layer */
		udc_ctrl_submit_status(dev, buf);
	}

	/* Update to next stage of control transfer */
	udc_ctrl_update_stage(dev, buf);

	if (udc_ctrl_stage_is_status_out(dev)) {
		/*
		 * Flush TX FIFO in case host already send status OUT packet
		 * and is not interested in reading from IN endpoint
		 */
		USB->USB_TXC0_REG = USB_USB_TXC0_REG_USB_FLUSH_Msk;
		/* Enable reception of status OUT packet */
		REG_SET_BIT(USB_RXC0_REG, USB_RX_EN);
		/*
		 * IN transfer finished, release buffer,
		 */
		net_buf_unref(buf);
	}
}

static void handle_ep0_setup_work(struct k_work *item)
{
	struct usb_smartbond_data *data =
		CONTAINER_OF(item, struct usb_smartbond_data, ep0_setup_work);
	struct net_buf *buf;
	int err;
	const struct device *dev = data->dev;
	struct smartbond_ep_state *ep0_out_state;

	buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, sizeof(struct usb_setup_packet));
	if (buf == NULL) {
		LOG_ERR("Failed to allocate for setup");
		return;
	}

	udc_ep_buf_set_setup(buf);
	net_buf_add_mem(buf, data->setup_buffer, 8);
	ep0_out_state = EP0_OUT_STATE(data);
	ep0_out_state->last_packet_size = 0;
	ep0_out_state->buf = NULL;
	udc_ctrl_update_stage(dev, buf);

	if (udc_ctrl_stage_is_data_out(dev)) {
		/*  Allocate and feed buffer for data OUT stage */
		LOG_DBG("s:%p|feed for -out-", buf);
		err = usbd_ctrl_feed_dout(dev, udc_data_stage_length(buf));
		if (err == -ENOMEM) {
			err = udc_submit_ep_event(dev, buf, err);
		}
	} else if (udc_ctrl_stage_is_data_in(dev)) {
		/* Allocate buffer for Status OUT state */
		err = usb_dc_smartbond_alloc_status_out(dev);
		if (err == -ENOMEM) {
			err = udc_submit_ep_event(dev, buf, err);
		} else {
			err = udc_ctrl_submit_s_in_status(dev);
			if (err == -ENOMEM) {
				err = udc_submit_ep_event(dev, buf, err);
			}
		}
	} else {
		err = udc_ctrl_submit_s_status(dev);
	}
}

static int udc_smartbond_enable(const struct device *dev)
{
	struct usb_smartbond_data *data = udc_get_private(dev);
	const struct udc_smartbond_config *config = dev->config;

	LOG_DBG("Enable UDC");

	usb_change_state(data, true, data->vbus_present);

	if (udc_ep_enable_internal(dev, USB_CONTROL_EP_OUT, USB_EP_TYPE_CONTROL, 8, 0)) {
		LOG_ERR("Failed to enable control endpoint");
		return -EIO;
	}

	if (udc_ep_enable_internal(dev, USB_CONTROL_EP_IN, USB_EP_TYPE_CONTROL, 8, 0)) {
		LOG_ERR("Failed to enable control endpoint");
		return -EIO;
	}

	irq_enable(config->udc_irq);

	return 0;
}

static int udc_smartbond_disable(const struct device *dev)
{
	struct usb_smartbond_data *data = udc_get_private(dev);
	const struct udc_smartbond_config *config = dev->config;

	LOG_DBG("Disable UDC");

	usb_change_state(data, false, data->vbus_present);

	irq_disable(config->udc_irq);

	return 0;
}

/*
 * Prepare and configure most of the parts, if the controller has a way
 * of detecting VBUS activity it should be enabled here.
 * Only udc_smartbond_enable() makes device visible to the host.
 */
static int udc_smartbond_init(const struct device *dev)
{
	struct usb_smartbond_data *data = udc_get_private(dev);
	struct udc_data *udc_data = &data->udc_data;
	const struct udc_smartbond_config *config = dev->config;
	struct smartbond_ep_reg_set *reg_set = (struct smartbond_ep_reg_set *)&(USB->USB_EPC0_REG);
	const uint16_t mps = 1023;
	int err;

	data->dev = dev;

	k_mutex_init(&udc_data->mutex);
	k_work_init(&data->ep0_setup_work, handle_ep0_setup_work);
	k_work_init(&data->ep0_rx_work, handle_ep0_rx_work);
	k_work_init(&data->ep0_tx_work, handle_ep0_tx_work);

	udc_data->caps.rwup = true;
	udc_data->caps.mps0 = UDC_MPS0_8;

	for (int i = 0; i < config->num_of_eps; i++) {
		data->ep_state[0][i].config.caps.out = 1;
		if (i == 0) {
			data->ep_state[0][i].config.caps.control = 1;
			data->ep_state[0][i].config.caps.mps = 8;
		} else {
			data->ep_state[0][i].config.caps.bulk = 1;
			data->ep_state[0][i].config.caps.interrupt = 1;
			data->ep_state[0][i].config.caps.iso = 1;
			data->ep_state[0][i].config.caps.mps = mps;
		}
		data->ep_state[0][i].config.addr = USB_EP_DIR_OUT | i;
		err = udc_register_ep(dev, &data->ep_state[0][i].config);
		if (err != 0) {
			LOG_ERR("Failed to register endpoint");
			return err;
		}
		data->ep_state[0][i].regs = reg_set + i;
	}

	for (int i = 0; i < config->num_of_eps; i++) {
		data->ep_state[1][i].config.caps.in = 1;
		if (i == 0) {
			data->ep_state[1][i].config.caps.control = 1;
			data->ep_state[1][i].config.caps.mps = 8;
		} else {
			data->ep_state[1][i].config.caps.bulk = 1;
			data->ep_state[1][i].config.caps.interrupt = 1;
			data->ep_state[1][i].config.caps.iso = 1;
			data->ep_state[1][i].config.caps.mps = mps;
		}

		data->ep_state[1][i].config.addr = USB_EP_DIR_IN | i;
		err = udc_register_ep(dev, &data->ep_state[1][i].config);
		if (err != 0) {
			LOG_ERR("Failed to register endpoint");
			return err;
		}
		data->ep_state[1][i].regs = reg_set + i;
	}

	CRG_TOP->VBUS_IRQ_CLEAR_REG = 1;
	/* Both connect and disconnect needs to be handled */
	CRG_TOP->VBUS_IRQ_MASK_REG = CRG_TOP_VBUS_IRQ_MASK_REG_VBUS_IRQ_EN_FALL_Msk |
				     CRG_TOP_VBUS_IRQ_MASK_REG_VBUS_IRQ_EN_RISE_Msk;
	NVIC_SetPendingIRQ(config->vbus_irq);
	irq_enable(config->vbus_irq);

	return 0;
}

static void udc_smartbond_lock(const struct device *dev)
{
	udc_lock_internal(dev, K_FOREVER);
}

static void udc_smartbond_unlock(const struct device *dev)
{
	udc_unlock_internal(dev);
}

static const struct udc_api udc_smartbond_api = {
	.lock = udc_smartbond_lock,
	.unlock = udc_smartbond_unlock,
	.device_speed = udc_smartbond_device_speed,
	.init = udc_smartbond_init,
	.enable = udc_smartbond_enable,
	.disable = udc_smartbond_disable,
	.shutdown = udc_smartbond_shutdown,
	.set_address = udc_smartbond_set_address,
	.host_wakeup = udc_smartbond_host_wakeup,
	.ep_enable = udc_smartbond_ep_enable,
	.ep_disable = udc_smartbond_ep_disable,
	.ep_set_halt = udc_smartbond_ep_set_halt,
	.ep_clear_halt = udc_smartbond_ep_clear_halt,
	.ep_enqueue = udc_smartbond_ep_enqueue,
	.ep_dequeue = udc_smartbond_ep_dequeue,
};

#define DT_DRV_COMPAT renesas_smartbond_usbd

#define UDC_IRQ(inst)      DT_INST_IRQ_BY_IDX(inst, 0, irq)
#define UDC_IRQ_PRI(inst)  DT_INST_IRQ_BY_IDX(inst, 0, priority)
#define VBUS_IRQ(inst)     DT_INST_IRQ_BY_IDX(inst, 1, irq)
#define VBUS_IRQ_PRI(inst) DT_INST_IRQ_BY_IDX(inst, 1, priority)

/*
 * Minimal transfer size needed to use DMA. For short transfers
 * it may be simpler to just fill hardware FIFO with data instead
 * of programming DMA registers.
 */
#define DMA_MIN_TRANSFER_SIZE(inst) DT_INST_PROP(inst, dma_min_transfer_size)
#define FIFO_READ_THRESHOLD(inst)   DT_INST_PROP(inst, fifo_read_threshold)

#define UDC_SMARTBOND_DEVICE_DEFINE(n)                                                             \
                                                                                                   \
	static const struct udc_smartbond_config udc_smartbond_cfg_##n = {                         \
		.udc_irq = UDC_IRQ(n),                                                             \
		.vbus_irq = VBUS_IRQ(n),                                                           \
		.dma_min_transfer_size = DMA_MIN_TRANSFER_SIZE(n),                                 \
		.fifo_read_threshold = FIFO_READ_THRESHOLD(n),                                     \
		.fifo_read_threshold = FIFO_READ_THRESHOLD(n),                                     \
		.num_of_eps = DT_INST_PROP(n, num_bidir_endpoints),                                \
		.dma_cfg = {                                                                       \
				.tx_chan = DT_INST_DMAS_CELL_BY_NAME(n, tx, channel),              \
				.tx_slot_mux = DT_INST_DMAS_CELL_BY_NAME(n, tx, config),           \
				.tx_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(n, tx)),         \
				.rx_chan = DT_INST_DMAS_CELL_BY_NAME(n, rx, channel),              \
				.rx_slot_mux = DT_INST_DMAS_CELL_BY_NAME(n, rx, config),           \
				.rx_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(n, rx)),         \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	static struct usb_smartbond_data udc_data_##n = {                                          \
		.udc_data = {                                                                      \
				.mutex = Z_MUTEX_INITIALIZER(udc_data_##n.udc_data.mutex),         \
				.priv = &udc_data_##n,                                             \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	static int udc_smartbond_driver_preinit_##n(const struct device *dev)                      \
	{                                                                                          \
		IRQ_CONNECT(VBUS_IRQ(n), VBUS_IRQ_PRI(n), usb_dc_smartbond_vbus_isr,               \
			    &udc_data_##n, 0);                                                     \
		IRQ_CONNECT(UDC_IRQ(n), UDC_IRQ_PRI(n), udc_smartbond_isr, DEVICE_DT_INST_GET(n),  \
			    0);                                                                    \
		return 0;                                                                          \
	}                                                                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, udc_smartbond_driver_preinit_##n, NULL, &udc_data_##n,            \
			      &udc_smartbond_cfg_##n, POST_KERNEL,                                 \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &udc_smartbond_api);

DT_INST_FOREACH_STATUS_OKAY(UDC_SMARTBOND_DEVICE_DEFINE)

/*
 * Copyright (c) 2022 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file  usb_dc_smartbond.c
 * @brief SmartBond USB device controller driver
 *
 */

#include <stdio.h>
#include <string.h>

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/usb/usb_dc.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/usb/usb_device.h>

#include <DA1469xAB.h>
#include <soc.h>
#include <da1469x_clock.h>

#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

LOG_MODULE_REGISTER(usb_dc_smartbond, CONFIG_USB_DRIVER_LOG_LEVEL);

/* USB device controller access from devicetree */
#define DT_DRV_COMPAT renesas_smartbond_usbd

#define USB_IRQ                        DT_INST_IRQ_BY_IDX(0, 0, irq)
#define USB_IRQ_PRI                    DT_INST_IRQ_BY_IDX(0, 0, priority)
#define VBUS_IRQ                       DT_INST_IRQ_BY_IDX(0, 1, irq)
#define VBUS_IRQ_PRI                   DT_INST_IRQ_BY_IDX(0, 1, priority)
#define DMA_CHANNEL_RX                 DT_INST_PROP(0, dma_chan_rx)
#define DMA_CHANNEL_TX                 DT_INST_PROP(0, dma_chan_tx)
/* Minimal transfer size needed to use DMA. For short transfers
 * it may be simpler to just fill hardware FIFO with data instead
 * of programming DMA registers.
 */
#define DMA_MIN_TRANSFER_SIZE          DT_INST_PROP(0, dma_min_transfer_size)
#define FIFO_READ_THRESHOLD            DT_INST_PROP(0, fifo_read_threshold)

/* Size of hardware RX and TX FIFO. */
#define EP0_FIFO_SIZE                  8
#define EP_FIFO_SIZE                   64

#define EP0_OUT_BUF_SIZE               EP0_FIFO_SIZE
#define EP1_OUT_BUF_SIZE               DT_INST_PROP_BY_IDX(0, ep_out_buf_size, 1)
#define EP2_OUT_BUF_SIZE               DT_INST_PROP_BY_IDX(0, ep_out_buf_size, 2)
#define EP3_OUT_BUF_SIZE               DT_INST_PROP_BY_IDX(0, ep_out_buf_size, 3)

#define EP0_IDX                        0
#define EP0_IN                         USB_CONTROL_EP_IN
#define EP0_OUT                        USB_CONTROL_EP_OUT
#define EP_MAX                         4

/* EP OUT buffers  */
static uint8_t ep0_out_buf[EP0_OUT_BUF_SIZE];
static uint8_t ep1_out_buf[EP1_OUT_BUF_SIZE];
static uint8_t ep2_out_buf[EP2_OUT_BUF_SIZE];
static uint8_t ep3_out_buf[EP3_OUT_BUF_SIZE];

static uint8_t *const ep_out_bufs[4] = {
	ep0_out_buf, ep1_out_buf, ep2_out_buf, ep3_out_buf,
};

static const uint16_t ep_out_buf_size[4] = {
	EP0_OUT_BUF_SIZE, EP1_OUT_BUF_SIZE, EP2_OUT_BUF_SIZE, EP3_OUT_BUF_SIZE,
};

/* Node functional states */
#define NFSR_NODE_RESET         0
#define NFSR_NODE_RESUME        1
#define NFSR_NODE_OPERATIONAL   2
#define NFSR_NODE_SUSPEND       3
/*
 * Those two following states are added to allow going out of sleep mode
 * using frame interrupt.  On remove wakeup RESUME state must be kept for
 * at least 1ms. It is accomplished by using FRAME interrupt that goes
 * through those two fake states before entering OPERATIONAL state.
 */
#define NFSR_NODE_WAKING        (0x10 | (NFSR_NODE_RESUME))
#define NFSR_NODE_WAKING2       (0x20 | (NFSR_NODE_RESUME))

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

static struct smartbond_ep_reg_set *const reg_sets[4] = {
	(struct smartbond_ep_reg_set *)&USB->USB_EPC0_REG,
	(struct smartbond_ep_reg_set *)&USB->USB_EPC1_REG,
	(struct smartbond_ep_reg_set *)&USB->USB_EPC3_REG,
	(struct smartbond_ep_reg_set *)&USB->USB_EPC5_REG,
};

struct smartbond_ep_state {
	atomic_t busy;
	uint8_t *buffer;
	uint16_t total_len;             /** Total length of current transfer */
	uint16_t transferred;           /** Bytes transferred so far */
	uint16_t mps;       /** Endpoint max packet size */
	/** Packet size sent or received so far. It is used to modify transferred field
	 * after ACK is received or when filling ISO endpoint with size larger then
	 * FIFO size.
	 */
	uint16_t last_packet_size;

	usb_dc_ep_callback cb;          /** Endpoint callback function */

	uint8_t data1 : 1;              /** DATA0/1 toggle bit 1 DATA1 is expected or transmitted */
	uint8_t stall : 1;              /** Endpoint is stalled */
	uint8_t iso : 1;                /** ISO endpoint */
	uint8_t ep_addr;                /** EP address */
	struct smartbond_ep_reg_set *regs;
};

struct usb_dc_state {
	bool vbus_present;
	bool attached;
	uint8_t nfsr;
	usb_dc_status_callback status_cb;
	struct smartbond_ep_state ep_state[2][4];
	atomic_ptr_t dma_ep[2];         /** DMA used by channel */
};

static struct usb_dc_state dev_state;

#define DA146XX_DMA_USB_MUX       (0x6 << ((DMA_CHANNEL_RX) * 2))
#define DA146XX_DMA_USB_MUX_MASK  (0xF << ((DMA_CHANNEL_RX) * 2))

struct smartbond_dma_channel {
	uint32_t dma_a_start;
	uint32_t dma_b_start;
	uint32_t dma_int;
	uint32_t dma_len;
	uint32_t dma_ctrl;
	uint32_t dma_idx;
	/* Extend structure size for array like usage, registers for each channel
	 * are 0x20 bytes apart.
	 */
	volatile uint32_t RESERVED[2];
};

#define DMA_CHANNEL_REGS(n) ((struct smartbond_dma_channel *)(DMA) + (n))
#define RX_DMA_REGS  DMA_CHANNEL_REGS(DMA_CHANNEL_RX)
#define TX_DMA_REGS  DMA_CHANNEL_REGS(DMA_CHANNEL_TX)

#define RX_DMA_START ((1 << DMA_DMA0_CTRL_REG_DMA_ON_Pos) |            \
		      (0 << DMA_DMA0_CTRL_REG_BW_Pos) |                        \
		      (1 << DMA_DMA0_CTRL_REG_DREQ_MODE_Pos) |                 \
		      (1 << DMA_DMA0_CTRL_REG_BINC_Pos) |                      \
		      (0 << DMA_DMA0_CTRL_REG_AINC_Pos) |                      \
		      (0 << DMA_DMA0_CTRL_REG_CIRCULAR_Pos) |                  \
		      (2 << DMA_DMA0_CTRL_REG_DMA_PRIO_Pos) |                  \
		      (0 << DMA_DMA0_CTRL_REG_DMA_IDLE_Pos) |                  \
		      (0 << DMA_DMA0_CTRL_REG_DMA_INIT_Pos) |                  \
		      (0 << DMA_DMA0_CTRL_REG_REQ_SENSE_Pos) |                 \
		      (0 << DMA_DMA0_CTRL_REG_BURST_MODE_Pos) |                \
		      (0 << DMA_DMA0_CTRL_REG_BUS_ERROR_DETECT_Pos))

#define TX_DMA_START ((1 << DMA_DMA0_CTRL_REG_DMA_ON_Pos) |            \
		      (0 << DMA_DMA0_CTRL_REG_BW_Pos) |                        \
		      (1 << DMA_DMA0_CTRL_REG_DREQ_MODE_Pos) |                 \
		      (0 << DMA_DMA0_CTRL_REG_BINC_Pos) |                      \
		      (1 << DMA_DMA0_CTRL_REG_AINC_Pos) |                      \
		      (0 << DMA_DMA0_CTRL_REG_CIRCULAR_Pos) |                  \
		      (7 << DMA_DMA0_CTRL_REG_DMA_PRIO_Pos) |                  \
		      (0 << DMA_DMA0_CTRL_REG_DMA_IDLE_Pos) |                  \
		      (0 << DMA_DMA0_CTRL_REG_DMA_INIT_Pos) |                  \
		      (1 << DMA_DMA0_CTRL_REG_REQ_SENSE_Pos) |                 \
		      (0 << DMA_DMA0_CTRL_REG_BURST_MODE_Pos) |                \
		      (0 << DMA_DMA0_CTRL_REG_BUS_ERROR_DETECT_Pos))

/*
 * DA146xx register fields and bit mask are very long. Filed masks repeat register names.
 * Those convenience macros are a way to reduce complexity of register modification lines.
 */
#define GET_BIT(val, field) (val & field ## _Msk) >> field ## _Pos
#define REG_GET_BIT(reg, field) (USB->reg & USB_ ## reg ## _ ## field ## _Msk)
#define REG_SET_BIT(reg, field) USB->reg |= USB_ ## reg ## _ ## field ## _Msk
#define REG_CLR_BIT(reg, field) USB->reg &= ~USB_ ## reg ## _ ## field ## _Msk
#define REG_SET_VAL(reg, field, val)                                   \
	USB->reg = (USB->reg & ~USB_##reg##_##field##_Msk) |               \
		   (val << USB_##reg##_##field##_Pos)

static struct smartbond_ep_state *usb_dc_get_ep_state(uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	uint8_t ep_dir = USB_EP_GET_DIR(ep) ? 1 : 0;

	return (ep_idx < EP_MAX) ? &dev_state.ep_state[ep_dir][ep_idx] : NULL;
}

static struct smartbond_ep_state *usb_dc_get_ep_out_state(uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	return (ep_idx < EP_MAX && USB_EP_DIR_IS_OUT(ep)) ?
		&dev_state.ep_state[0][ep_idx] : NULL;
}

static struct smartbond_ep_state *usb_dc_get_ep_in_state(uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	return ep_idx < EP_MAX || USB_EP_DIR_IS_IN(ep) ?
	       &dev_state.ep_state[1][ep_idx] : NULL;
}

static inline bool dev_attached(void)
{
	return dev_state.attached;
}

static inline bool dev_ready(void)
{
	return dev_state.vbus_present;
}

static void set_nfsr(uint8_t val)
{
	dev_state.nfsr = val;
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
	uint8_t ep_idx = USB_EP_GET_IDX(ep_state->ep_addr);
	struct smartbond_ep_reg_set *regs = ep_state->regs;

	src = &ep_state->buffer[ep_state->transferred];
	remaining = ep_state->total_len - ep_state->transferred;
	if (remaining > ep_state->mps - ep_state->last_packet_size) {
		remaining = ep_state->mps - ep_state->last_packet_size;
	}

	/*
	 * Loop checks TCOUNT all the time since this value is saturated to 31
	 * and can't be read just once before.
	 */
	while ((regs->txs & USB_USB_TXS1_REG_USB_TCOUNT_Msk) > 0 &&
		remaining > 0) {
		regs->txd = *src++;
		ep_state->last_packet_size++;
		remaining--;
	}

	if (ep_idx != 0) {
		if (remaining > 0) {
			/*
			 * Max packet size is set to value greater then FIFO.
			 * Enable fifo level warning to handle larger packets.
			 */
			regs->txc |= (3 << USB_USB_TXC1_REG_USB_TFWL_Pos);
			USB->USB_FWMSK_REG |=
				BIT(ep_idx - 1 + USB_USB_FWMSK_REG_USB_M_TXWARN31_Pos);
		} else {
			regs->txc &= ~USB_USB_TXC1_REG_USB_TFWL_Msk;
			USB->USB_FWMSK_REG &=
				~(BIT(ep_idx - 1 + USB_USB_FWMSK_REG_USB_M_TXWARN31_Pos));
			/* Whole packet already in fifo, no need to
			 * refill it later.  Mark last.
			 */
			regs->txc |= USB_USB_TXC1_REG_USB_LAST_Msk;
		}
	}
}

static bool try_allocate_dma(struct smartbond_ep_state *ep_state, uint8_t dir)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep_state->ep_addr);
	uint8_t dir_ix = dir == USB_EP_DIR_OUT ? 0 : 1;

	if (atomic_ptr_cas(&dev_state.dma_ep[dir_ix], NULL, ep_state)) {
		if (dir == USB_EP_DIR_OUT) {
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

static void start_rx_dma(volatile void *src, void *dst, uint16_t size)
{
	LOG_DBG("");
	/* Setup SRC and DST registers */
	RX_DMA_REGS->dma_a_start = (uint32_t)src;
	RX_DMA_REGS->dma_b_start = (uint32_t)dst;
	/* Don't need DMA interrupt, read end is determined by
	 * RX_LAST or RX_ERR events.
	 */
	RX_DMA_REGS->dma_int = size - 1;
	RX_DMA_REGS->dma_len = size - 1;
	RX_DMA_REGS->dma_ctrl = RX_DMA_START;
}

static void start_rx_packet(struct smartbond_ep_state *ep_state)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep_state->ep_addr);
	struct smartbond_ep_reg_set *regs = ep_state->regs;

	LOG_DBG("%02x", ep_state->ep_addr);

	ep_state->last_packet_size = 0;
	ep_state->transferred = 0;
	ep_state->total_len = 0;

	if (ep_state->mps > DMA_MIN_TRANSFER_SIZE) {
		if (try_allocate_dma(ep_state, USB_EP_DIR_OUT)) {
			start_rx_dma(&regs->rxd,
				     ep_state->buffer,
				     ep_state->mps);
		} else if (ep_state->mps > EP_FIFO_SIZE) {
			/*
			 * Other endpoint is using DMA in that direction,
			 * fall back to interrupts.
			 * For endpoint size greater than FIFO size,
			 * enable FIFO level warning interrupt when FIFO
			 * has less than 17 bytes free.
			 */
			regs->rxc |= USB_USB_RXC1_REG_USB_RFWL_Msk;
			USB->USB_FWMSK_REG |=
				BIT(ep_idx - 1 + USB_USB_FWMSK_REG_USB_M_RXWARN31_Pos);
		}
	} else if (ep_idx != 0) {
		/* If max_packet_size would fit in FIFO no need
		 * for FIFO level warning interrupt.
		 */
		regs->rxc &= ~USB_USB_RXC1_REG_USB_RFWL_Msk;
		USB->USB_FWMSK_REG &= ~(BIT(ep_idx - 1 + USB_USB_FWMSK_REG_USB_M_RXWARN31_Pos));
	}

	regs->rxc |= USB_USB_RXC1_REG_USB_RX_EN_Msk;
}

static void start_tx_dma(void *src, volatile void *dst, uint16_t size)
{
	LOG_DBG("%d", size);
	/* Setup SRC and DST registers. */
	TX_DMA_REGS->dma_a_start = (uint32_t)src;
	TX_DMA_REGS->dma_b_start = (uint32_t)dst;
	/* Interrupt not needed. */
	TX_DMA_REGS->dma_int = size;
	TX_DMA_REGS->dma_len = size - 1;
	TX_DMA_REGS->dma_ctrl = TX_DMA_START;
}

static void start_tx_packet(struct smartbond_ep_state *ep_state)
{
	struct smartbond_ep_reg_set *regs = ep_state->regs;
	uint16_t remaining = ep_state->total_len - ep_state->transferred;
	uint16_t size = MIN(remaining, ep_state->mps);

	LOG_DBG("%02x %d/%d", ep_state->ep_addr, size, remaining);

	ep_state->last_packet_size = 0;

	regs->txc = USB_USB_TXC1_REG_USB_FLUSH_Msk;
	regs->txc = USB_USB_TXC1_REG_USB_IGN_ISOMSK_Msk;
	if (ep_state->data1) {
		regs->txc |= USB_USB_TXC1_REG_USB_TOGGLE_TX_Msk;
	}

	if (ep_state->ep_addr != EP0_IN &&
	    remaining > DMA_MIN_TRANSFER_SIZE &&
	    (uint32_t)(ep_state->buffer) >= CONFIG_SRAM_BASE_ADDRESS &&
	    try_allocate_dma(ep_state, USB_EP_DIR_IN)) {
		/* Whole packet will be put in FIFO by DMA.
		 * Set LAST bit before start.
		 */
		start_tx_dma(ep_state->buffer + ep_state->transferred,
			     &regs->txd, size);
		regs->txc |= USB_USB_TXC1_REG_USB_LAST_Msk;
	} else {
		fill_tx_fifo(ep_state);
	}

	regs->txc |= USB_USB_TXC1_REG_USB_TX_EN_Msk;
}

static uint16_t read_rx_fifo(struct smartbond_ep_state *ep_state,
			     uint16_t bytes_in_fifo)
{
	struct smartbond_ep_reg_set *regs = ep_state->regs;
	uint16_t remaining = ep_state->mps - ep_state->last_packet_size;
	uint16_t receive_this_time = bytes_in_fifo;
	uint8_t *buf = ep_state->buffer + ep_state->last_packet_size;

	if (remaining < bytes_in_fifo) {
		receive_this_time = remaining;
	}

	for (int i = 0; i < receive_this_time; ++i) {
		buf[i] = regs->rxd;
	}

	ep_state->last_packet_size += receive_this_time;

	return bytes_in_fifo - receive_this_time;
}

static void handle_ep0_rx(void)
{
	int fifo_bytes;
	uint32_t rxs0 = USB->USB_RXS0_REG;
	struct smartbond_ep_state *ep0_out_state = usb_dc_get_ep_out_state(0);
	struct smartbond_ep_state *ep0_in_state;

	fifo_bytes = GET_BIT(rxs0, USB_USB_RXS0_REG_USB_RCOUNT);

	if (rxs0 & USB_USB_RXS0_REG_USB_SETUP_Msk) {
		ep0_in_state = usb_dc_get_ep_in_state(0);
		read_rx_fifo(ep0_out_state, EP0_FIFO_SIZE);

		ep0_out_state->stall = 0;
		ep0_out_state->data1 = 1;
		ep0_in_state->stall = 0;
		ep0_in_state->data1 = 1;
		REG_SET_BIT(USB_TXC0_REG, USB_TOGGLE_TX0);
		REG_CLR_BIT(USB_EPC0_REG, USB_STALL);
		LOG_DBG("Setup %02x %02x %02x %02x %02x %02x %02x %02x",
			ep0_out_state->buffer[0],
			ep0_out_state->buffer[1],
			ep0_out_state->buffer[2],
			ep0_out_state->buffer[3],
			ep0_out_state->buffer[4],
			ep0_out_state->buffer[5],
			ep0_out_state->buffer[6],
			ep0_out_state->buffer[7]);
		ep0_out_state->cb(EP0_OUT, USB_DC_EP_SETUP);
	} else {
		if (GET_BIT(rxs0, USB_USB_RXS0_REG_USB_TOGGLE_RX0) !=
			ep0_out_state->data1) {
			/* Toggle bit does not match discard packet */
			REG_SET_BIT(USB_RXC0_REG, USB_FLUSH);
			ep0_out_state->last_packet_size = 0;
		} else {
			read_rx_fifo(ep0_out_state, fifo_bytes);
			if (rxs0 & USB_USB_RXS0_REG_USB_RX_LAST_Msk) {
				ep0_out_state->data1 ^= 1;

				ep0_out_state->cb(EP0_OUT, USB_DC_EP_DATA_OUT);
			}
		}
	}
}

static void handle_ep0_tx(void)
{
	uint32_t txs0;
	struct smartbond_ep_state *ep0_in_state = usb_dc_get_ep_in_state(0);
	struct smartbond_ep_state *ep0_out_state = usb_dc_get_ep_out_state(0);
	struct smartbond_ep_reg_set *regs = ep0_in_state->regs;

	txs0 = regs->txs;

	LOG_DBG("%02x %02x", ep0_in_state->ep_addr, txs0);

	if (GET_BIT(txs0, USB_USB_TXS0_REG_USB_TX_DONE)) {
		/* ACK received */
		if (GET_BIT(txs0, USB_USB_TXS0_REG_USB_ACK_STAT)) {
			ep0_in_state->transferred +=
				ep0_in_state->last_packet_size;
			ep0_in_state->last_packet_size = 0;
			ep0_in_state->data1 ^= 1;
			REG_SET_VAL(USB_TXC0_REG, USB_TOGGLE_TX0,
				    ep0_in_state->data1);
			if (ep0_in_state->transferred == ep0_in_state->total_len) {
				/* For control endpoint get ready for ACK stage
				 * from host.
				 */
				ep0_out_state = usb_dc_get_ep_out_state(EP0_IDX);
				ep0_out_state->transferred = 0;
				ep0_out_state->total_len = 0;
				ep0_out_state->last_packet_size = 0;
				REG_SET_BIT(USB_RXC0_REG, USB_RX_EN);

				atomic_clear(&ep0_in_state->busy);
				ep0_in_state->cb(EP0_IN, USB_DC_EP_DATA_IN);
				return;
			}
		} else {
			/* Start from the beginning */
			ep0_in_state->last_packet_size = 0;
		}
		start_tx_packet(ep0_in_state);
	}
}

static void handle_epx_rx_ev(uint8_t ep_idx)
{
	uint32_t rxs;
	int fifo_bytes;
	struct smartbond_ep_state *ep_state = usb_dc_get_ep_out_state(ep_idx);
	struct smartbond_ep_reg_set *regs = ep_state->regs;

	do {
		rxs = regs->rxs;

		if (GET_BIT(rxs, USB_USB_RXS1_REG_USB_RX_ERR)) {
			regs->rxc |= USB_USB_RXC1_REG_USB_FLUSH_Msk;
			ep_state->last_packet_size = 0;
			if (dev_state.dma_ep[0] == ep_state) {
				/* Stop DMA */
				RX_DMA_REGS->dma_ctrl &= ~DMA_DMA0_CTRL_REG_DMA_ON_Msk;
				/* Restart DMA since packet was dropped,
				 * all parameters should still work.
				 */
				RX_DMA_REGS->dma_ctrl |= DMA_DMA0_CTRL_REG_DMA_ON_Msk;
			}
			break;
		}

		if (dev_state.dma_ep[0] == ep_state) {
			/* Disable DMA and update last_packet_size
			 * with what DMA reported.
			 */
			RX_DMA_REGS->dma_ctrl &= ~DMA_DMA0_CTRL_REG_DMA_ON_Msk;
			ep_state->last_packet_size = RX_DMA_REGS->dma_idx;
			/*
			 * When DMA did not finished (packet was smaller then MPS),
			 * dma_idx holds exact number of bytes transmitted. When DMA
			 * finished value in dma_idx is one less then actual number of
			 * transmitted bytes.
			 */
			if (ep_state->last_packet_size == RX_DMA_REGS->dma_len) {
				ep_state->last_packet_size++;
			}
			/* Release DMA to use by other endpoints. */
			dev_state.dma_ep[0] = NULL;
		}
		fifo_bytes = GET_BIT(rxs, USB_USB_RXS1_REG_USB_RXCOUNT);
		/*
		 * FIFO maybe empty if DMA read it before or
		 * it's final iteration and function already read all
		 * that was to read.
		 */
		if (fifo_bytes > 0) {
			fifo_bytes = read_rx_fifo(ep_state, fifo_bytes);
		}

		if (GET_BIT(rxs, USB_USB_RXS1_REG_USB_RX_LAST)) {
			if (!ep_state->iso &&
			    GET_BIT(rxs, USB_USB_RXS1_REG_USB_TOGGLE_RX) !=
			    ep_state->data1) {
				/* Toggle bit does not match discard packet */
				regs->rxc |= USB_USB_RXC1_REG_USB_FLUSH_Msk;
				ep_state->last_packet_size = 0;
				/* Re-enable reception */
				start_rx_packet(ep_state);
			} else {
				ep_state->data1 ^= 1;
				atomic_clear(&ep_state->busy);
				ep_state->cb(ep_state->ep_addr,
					     USB_DC_EP_DATA_OUT);
			}
		}
	} while (fifo_bytes > FIFO_READ_THRESHOLD);
}

static void handle_rx_ev(void)
{
	if (USB->USB_RXEV_REG & BIT(0)) {
		handle_epx_rx_ev(1);
	}

	if (USB->USB_RXEV_REG & BIT(1)) {
		handle_epx_rx_ev(2);
	}

	if (USB->USB_RXEV_REG & BIT(2)) {
		handle_epx_rx_ev(3);
	}
}

static void handle_epx_tx_ev(struct smartbond_ep_state *ep_state)
{
	uint32_t txs;
	struct smartbond_ep_reg_set *regs = ep_state->regs;

	txs = regs->txs;

	if (GET_BIT(txs, USB_USB_TXS1_REG_USB_TX_DONE)) {
		if (dev_state.dma_ep[1] == ep_state) {
			/* Disable DMA and update last_packet_size with what
			 * DMA reported.
			 */
			TX_DMA_REGS->dma_ctrl &= ~DMA_DMA1_CTRL_REG_DMA_ON_Msk;
			ep_state->last_packet_size = TX_DMA_REGS->dma_idx + 1;
			/* Release DMA to used by other endpoints. */
			dev_state.dma_ep[1] = NULL;
		}

		if (GET_BIT(txs, USB_USB_TXS1_REG_USB_ACK_STAT)) {
			/* ACK received, update transfer state and DATA0/1 bit */
			ep_state->transferred += ep_state->last_packet_size;
			ep_state->last_packet_size = 0;
			ep_state->data1 ^= 1;

			if (ep_state->transferred == ep_state->total_len) {
				atomic_clear(&ep_state->busy);
				ep_state->cb(ep_state->ep_addr, USB_DC_EP_DATA_IN);
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
		LOG_DBG("EP 0x%02x FIFO underrun\n", ep_state->ep_addr);
	}
	/* Start next or repeated packet. */
	start_tx_packet(ep_state);
}

static void handle_tx_ev(void)
{
	if (USB->USB_TXEV_REG & BIT(0)) {
		handle_epx_tx_ev(usb_dc_get_ep_in_state(1));
	}
	if (USB->USB_TXEV_REG & BIT(1)) {
		handle_epx_tx_ev(usb_dc_get_ep_in_state(2));
	}
	if (USB->USB_TXEV_REG & BIT(2)) {
		handle_epx_tx_ev(usb_dc_get_ep_in_state(3));
	}
}

static uint32_t check_reset_end(uint32_t alt_ev)
{
	if (dev_state.nfsr == NFSR_NODE_RESET) {
		if (GET_BIT(alt_ev, USB_USB_ALTEV_REG_USB_RESET)) {
			/*
			 * Could be still in reset, but since USB_M_RESET is
			 * disabled it can be also old reset state that was not
			 * cleared yet.
			 * If (after reading USB_ALTEV_REG register again)
			 * bit is cleared reset state just ended.
			 * Keep non-reset bits combined from two previous
			 * ALTEV read and one from the next line.
			 */
			alt_ev = (alt_ev & ~USB_USB_ALTEV_REG_USB_RESET_Msk) |
				 USB->USB_ALTEV_REG;
		}

		if (GET_BIT(alt_ev, USB_USB_ALTEV_REG_USB_RESET) == 0) {
			USB->USB_ALTMSK_REG = USB_USB_ALTMSK_REG_USB_M_RESET_Msk |
					      USB_USB_ALTEV_REG_USB_SD3_Msk;
			if (dev_state.ep_state[0][0].buffer != NULL) {
				USB->USB_MAMSK_REG |=
					USB_USB_MAMSK_REG_USB_M_EP0_RX_Msk;
			}
			LOG_INF("Set operational %02x", USB->USB_MAMSK_REG);
			set_nfsr(NFSR_NODE_OPERATIONAL);
		}
	}
	return alt_ev;
}

static void handle_bus_reset(void)
{
	uint32_t alt_ev;

	USB->USB_NFSR_REG = 0;
	USB->USB_FAR_REG = 0x80;
	USB->USB_ALTMSK_REG = 0;
	USB->USB_NFSR_REG = NFSR_NODE_RESET;
	USB->USB_TXMSK_REG = 0;
	USB->USB_RXMSK_REG = 0;
	set_nfsr(NFSR_NODE_RESET);

	for (int i = 0; i < EP_MAX; ++i) {
		dev_state.ep_state[1][i].buffer = NULL;
		dev_state.ep_state[1][i].transferred = 0;
		dev_state.ep_state[1][i].total_len = 0;
		atomic_clear(&dev_state.ep_state[1][i].busy);
	}

	LOG_INF("send USB_DC_RESET");
	dev_state.status_cb(USB_DC_RESET, NULL);
	USB->USB_DMA_CTRL_REG = 0;

	USB->USB_MAMSK_REG = USB_USB_MAMSK_REG_USB_M_INTR_Msk |
			     USB_USB_MAMSK_REG_USB_M_FRAME_Msk |
			     USB_USB_MAMSK_REG_USB_M_WARN_Msk |
			     USB_USB_MAMSK_REG_USB_M_ALT_Msk |
			     USB_USB_MAMSK_REG_USB_M_EP0_RX_Msk |
			     USB_USB_MAMSK_REG_USB_M_EP0_TX_Msk;
	USB->USB_ALTMSK_REG = USB_USB_ALTMSK_REG_USB_M_RESUME_Msk;
	alt_ev = USB->USB_ALTEV_REG;
	check_reset_end(alt_ev);
}

static void handle_alt_ev(void)
{
	struct smartbond_ep_state *ep_state;
	uint32_t alt_ev = USB->USB_ALTEV_REG;

	alt_ev = check_reset_end(alt_ev);
	if (GET_BIT(alt_ev, USB_USB_ALTEV_REG_USB_RESET) &&
	    dev_state.nfsr != NFSR_NODE_RESET) {
		handle_bus_reset();
	} else if (GET_BIT(alt_ev, USB_USB_ALTEV_REG_USB_RESUME)) {
		if (USB->USB_NFSR_REG == NFSR_NODE_SUSPEND) {
			set_nfsr(NFSR_NODE_OPERATIONAL);
			if (dev_state.ep_state[0][0].buffer != NULL) {
				USB->USB_MAMSK_REG |=
					USB_USB_MAMSK_REG_USB_M_EP0_RX_Msk;
			}
			USB->USB_ALTMSK_REG = USB_USB_ALTMSK_REG_USB_M_RESET_Msk |
					      USB_USB_ALTMSK_REG_USB_M_SD3_Msk;
			/* Re-enable reception of endpoint with pending transfer */
			for (int ep_num = 1; ep_num < EP_MAX; ++ep_num) {
				ep_state = usb_dc_get_ep_out_state(ep_num);
				if (ep_state->total_len > ep_state->transferred) {
					start_rx_packet(ep_state);
				}
			}
			dev_state.status_cb(USB_DC_RESUME, NULL);
		}
	} else if (GET_BIT(alt_ev, USB_USB_ALTEV_REG_USB_SD3)) {
		set_nfsr(NFSR_NODE_SUSPEND);
		USB->USB_ALTMSK_REG =
			USB_USB_ALTMSK_REG_USB_M_RESET_Msk |
			USB_USB_ALTMSK_REG_USB_M_RESUME_Msk;
		dev_state.status_cb(USB_DC_SUSPEND, NULL);
	}
}

static void handle_epx_tx_warn_ev(uint8_t ep_idx)
{
	fill_tx_fifo(usb_dc_get_ep_in_state(ep_idx));
}

static void handle_fifo_warning(void)
{
	uint32_t fifo_warning = USB->USB_FWEV_REG;

	if (fifo_warning & BIT(0)) {
		handle_epx_tx_warn_ev(1);
	}

	if (fifo_warning & BIT(1)) {
		handle_epx_tx_warn_ev(2);
	}

	if (fifo_warning & BIT(2)) {
		handle_epx_tx_warn_ev(3);
	}

	if (fifo_warning & BIT(4)) {
		handle_epx_rx_ev(1);
	}

	if (fifo_warning & BIT(5)) {
		handle_epx_rx_ev(2);
	}

	if (fifo_warning & BIT(6)) {
		handle_epx_rx_ev(3);
	}
}

static void handle_ep0_nak(void)
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
		if (REG_GET_BIT(USB_RXC0_REG, USB_RX_EN) == 0 &&
		    GET_BIT(ep0_nak, USB_USB_EP0_NAK_REG_USB_EP0_OUTNAK)) {
			/* NAK over EP0 was sent, transmit should conclude */
			USB->USB_TXC0_REG = USB_USB_TXC0_REG_USB_FLUSH_Msk;
			REG_SET_BIT(USB_RXC0_REG, USB_FLUSH);
			REG_SET_BIT(USB_RXC0_REG, USB_RX_EN);
			REG_CLR_BIT(USB_MAMSK_REG, USB_M_EP0_NAK);
		}
	}
}

static void usb_dc_smartbond_isr(void)
{
	uint32_t int_status = USB->USB_MAEV_REG & USB->USB_MAMSK_REG;

	if (GET_BIT(int_status, USB_USB_MAEV_REG_USB_WARN)) {
		handle_fifo_warning();
	}

	if (GET_BIT(int_status, USB_USB_MAEV_REG_USB_CH_EV)) {
		/* For now just clear interrupt */
		(void)USB->USB_CHARGER_STAT_REG;
	}

	if (GET_BIT(int_status, USB_USB_MAEV_REG_USB_EP0_TX)) {
		handle_ep0_tx();
	}

	if (GET_BIT(int_status, USB_USB_MAEV_REG_USB_EP0_RX)) {
		handle_ep0_rx();
	}

	if (GET_BIT(int_status, USB_USB_MAEV_REG_USB_EP0_NAK)) {
		handle_ep0_nak();
	}

	if (GET_BIT(int_status, USB_USB_MAEV_REG_USB_RX_EV)) {
		handle_rx_ev();
	}

	if (GET_BIT(int_status, USB_USB_MAEV_REG_USB_NAK)) {
		(void)USB->USB_NAKEV_REG;
	}

	if (GET_BIT(int_status, USB_USB_MAEV_REG_USB_FRAME)) {
		if (dev_state.nfsr == NFSR_NODE_RESET) {
			/*
			 * During reset FRAME interrupt is enabled to periodically
			 * check when reset state ends.
			 * FRAME interrupt is generated every 1ms without host sending
			 * actual SOF.
			 */
			check_reset_end(USB_USB_ALTEV_REG_USB_RESET_Msk);
		} else if (dev_state.nfsr == NFSR_NODE_WAKING) {
			/* No need to call set_nfsr, just set state */
			dev_state.nfsr = NFSR_NODE_WAKING2;
		} else if (dev_state.nfsr == NFSR_NODE_WAKING2) {
			/* No need to call set_nfsr, just set state */
			dev_state.nfsr = NFSR_NODE_RESUME;
			LOG_DBG("dev_state.nfsr = NFSR_NODE_RESUME %02x",
				USB->USB_MAMSK_REG);
		} else if (dev_state.nfsr == NFSR_NODE_RESUME) {
			set_nfsr(NFSR_NODE_OPERATIONAL);
			if (dev_state.ep_state[0][0].buffer != NULL) {
				USB->USB_MAMSK_REG |= USB_USB_MAMSK_REG_USB_M_EP0_RX_Msk;
			}
			LOG_DBG("Set operational %02x", USB->USB_MAMSK_REG);
		} else {
			USB->USB_MAMSK_REG &= ~USB_USB_MAMSK_REG_USB_M_FRAME_Msk;
		}
	}

	if (GET_BIT(int_status, USB_USB_MAEV_REG_USB_TX_EV)) {
		handle_tx_ev();
	}

	if (GET_BIT(int_status, USB_USB_MAEV_REG_USB_ALT)) {
		handle_alt_ev();
	}
}

static void usb_dc_smartbond_vbus_changed(bool present)
{
	if (dev_state.vbus_present == present) {
		return;
	}

	dev_state.vbus_present = present;

	if (present) {
		/* TODO: Add PD acquire when available */
		/* If power event happened before USB started, delay dcd_connect
		 * until dcd_init is called.
		 */
		dev_state.status_cb(USB_DC_CONNECTED, NULL);
	} else {
		USB->USB_MCTRL_REG = 0;
		dev_state.status_cb(USB_DC_DISCONNECTED, NULL);
		/* TODO: Add PD release when available */
	}
}

static void usb_dc_smartbond_vbus_isr(void)
{
	bool vbus_present =
		(CRG_TOP->ANA_STATUS_REG & CRG_TOP_ANA_STATUS_REG_VBUS_AVAILABLE_Msk) != 0;

	CRG_TOP->VBUS_IRQ_CLEAR_REG = 1;
	usb_dc_smartbond_vbus_changed(vbus_present);
}

static int usb_init(void)
{
	for (int i = 0; i < EP_MAX; ++i) {
		dev_state.ep_state[0][i].regs = reg_sets[i];
		dev_state.ep_state[0][i].ep_addr = i | USB_EP_DIR_OUT;
		dev_state.ep_state[0][i].buffer = ep_out_bufs[i];
		dev_state.ep_state[1][i].regs = reg_sets[i];
		dev_state.ep_state[1][i].ep_addr = i | USB_EP_DIR_IN;
	}

	/* Max packet size for EP0 is hardwired to 8 */
	dev_state.ep_state[0][0].mps = EP0_FIFO_SIZE;
	dev_state.ep_state[1][0].mps = EP0_FIFO_SIZE;

	IRQ_CONNECT(VBUS_IRQ, VBUS_IRQ_PRI, usb_dc_smartbond_vbus_isr, 0, 0);
	CRG_TOP->VBUS_IRQ_CLEAR_REG = 1;
	NVIC_ClearPendingIRQ(VBUS_IRQ);
	/* Both connect and disconnect needs to be handled */
	CRG_TOP->VBUS_IRQ_MASK_REG = CRG_TOP_VBUS_IRQ_MASK_REG_VBUS_IRQ_EN_FALL_Msk |
				     CRG_TOP_VBUS_IRQ_MASK_REG_VBUS_IRQ_EN_RISE_Msk;
	irq_enable(USB_IRQn);

	IRQ_CONNECT(USB_IRQ, USB_IRQ_PRI, usb_dc_smartbond_isr, 0, 0);
	irq_enable(USB_IRQ);

	return 0;
}

int usb_dc_ep_disable(const uint8_t ep)
{
	struct smartbond_ep_state *ep_state = usb_dc_get_ep_state(ep);

	LOG_DBG("%02x", ep);

	if (ep_state == NULL) {
		LOG_ERR("Not valid endpoint: %02x", ep);
		return -EINVAL;
	}

	if (ep_state->ep_addr == EP0_IN) {
		REG_SET_BIT(USB_TXC0_REG, USB_IGN_IN);
	} else if (ep_state->ep_addr == EP0_OUT) {
		USB->USB_RXC0_REG = USB_USB_RXC0_REG_USB_IGN_SETUP_Msk |
				    USB_USB_RXC0_REG_USB_IGN_OUT_Msk;
	} else if (USB_EP_DIR_IS_OUT(ep)) {
		ep_state->regs->epc_out &= ~USB_USB_EPC2_REG_USB_EP_EN_Msk;
	} else {
		ep_state->regs->epc_in &= ~USB_USB_EPC1_REG_USB_EP_EN_Msk;
	}

	return 0;
}

int usb_dc_ep_mps(const uint8_t ep)
{
	struct smartbond_ep_state *ep_state = usb_dc_get_ep_state(ep);

	if (ep_state == NULL) {
		LOG_ERR("Not valid endpoint: %02x", ep);
		return -EINVAL;
	}

	return ep_state->mps;
}

int usb_dc_ep_read_continue(uint8_t ep)
{
	struct smartbond_ep_state *ep_state = usb_dc_get_ep_out_state(ep);

	if (ep_state == NULL) {
		LOG_ERR("Not valid endpoint: %02x", ep);
		return -EINVAL;
	}

	LOG_DBG("ep 0x%02x", ep);

	/* If no more data in the buffer, start a new read transaction.
	 * DataOutStageCallback will called on transaction complete.
	 */
	if (ep_state->transferred >= ep_state->last_packet_size) {
		start_rx_packet(ep_state);
	}

	return 0;
}

int usb_dc_ep_read_wait(uint8_t ep, uint8_t *data, uint32_t max_data_len,
			uint32_t *read_bytes)
{
	struct smartbond_ep_state *ep_state = usb_dc_get_ep_out_state(ep);
	uint16_t read_count;

	if (ep_state == NULL) {
		LOG_ERR("Invalid Endpoint %x", ep);
		return -EINVAL;
	}

	LOG_DBG("ep 0x%02x, %u bytes, %p", ep, max_data_len, (void *)data);

	read_count = ep_state->last_packet_size - ep_state->transferred;

	/* When both buffer and max data to read are zero, just ignore reading
	 * and return available data in buffer. Otherwise, return data
	 * previously stored in the buffer.
	 */
	if (data) {
		read_count = MIN(read_count, max_data_len);
		memcpy(data, ep_state->buffer + ep_state->transferred, read_count);
		ep_state->transferred += read_count;
	} else if (max_data_len) {
		LOG_ERR("Wrong arguments");
	}

	if (read_bytes) {
		*read_bytes = read_count;
	}

	return 0;
}

int usb_dc_ep_read(const uint8_t ep, uint8_t *const data,
		   const uint32_t max_data_len, uint32_t *const read_bytes)
{
	if (usb_dc_ep_read_wait(ep, data, max_data_len, read_bytes) != 0) {
		return -EINVAL;
	}

	if (usb_dc_ep_read_continue(ep) != 0) {
		return -EINVAL;
	}

	return 0;
}

int usb_dc_ep_check_cap(const struct usb_dc_ep_cfg_data *const cfg)
{
	uint8_t ep_idx = USB_EP_GET_IDX(cfg->ep_addr);

	LOG_DBG("ep %x, mps %d, type %d", cfg->ep_addr, cfg->ep_mps, cfg->ep_type);

	if ((cfg->ep_type == USB_DC_EP_CONTROL && ep_idx != 0) ||
	    (cfg->ep_type != USB_DC_EP_CONTROL && ep_idx == 0)) {
		LOG_ERR("invalid endpoint configuration");
		return -EINVAL;
	}

	if (ep_idx > 3) {
		LOG_ERR("endpoint address out of range");
		return -EINVAL;
	}

	if (ep_out_buf_size[ep_idx] < cfg->ep_mps) {
		LOG_ERR("endpoint size too big");
		return -EINVAL;
	}

	return 0;
}

int usb_dc_ep_set_callback(const uint8_t ep, const usb_dc_ep_callback cb)
{
	struct smartbond_ep_state *ep_state = usb_dc_get_ep_state(ep);

	LOG_DBG("%02x %p", ep, (void *)cb);

	if (ep_state == NULL) {
		LOG_ERR("Not valid endpoint: %02x", ep);
		return -EINVAL;
	}

	ep_state->cb = cb;

	return 0;
}

void usb_dc_set_status_callback(const usb_dc_status_callback cb)
{
	dev_state.status_cb = cb;

	LOG_DBG("%p", cb);

	/* Manually call IRQ handler in case when VBUS is already present */
	usb_dc_smartbond_vbus_isr();
}

int usb_dc_reset(void)
{
	int ret;

	LOG_DBG("");

	if (!dev_attached() || !dev_ready()) {
		return -ENODEV;
	}

	ret = usb_dc_detach();
	if (ret) {
		return ret;
	}

	ret = usb_dc_attach();
	if (ret) {
		return ret;
	}

	return 0;
}

int usb_dc_set_address(const uint8_t addr)
{
	LOG_DBG("%d", addr);

	/* Set default address for one ZLP */
	USB->USB_EPC0_REG = USB_USB_EPC0_REG_USB_DEF_Msk;
	USB->USB_FAR_REG = (addr & USB_USB_FAR_REG_USB_AD_Msk) |
			   USB_USB_FAR_REG_USB_AD_EN_Msk;

	return 0;
}

int usb_dc_ep_clear_stall(const uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	uint8_t ep_dir = USB_EP_GET_DIR(ep);
	struct smartbond_ep_state *ep_state = usb_dc_get_ep_state(ep);
	struct smartbond_ep_reg_set *regs;

	LOG_DBG("%02x", ep);

	if (ep_state == NULL) {
		LOG_ERR("Not valid endpoint: %02x", ep);
		return -EINVAL;
	}
	regs =  ep_state->regs;

	/* Clear stall is called in response to Clear Feature ENDPOINT_HALT,
	 * reset toggle
	 */
	ep_state->stall = false;
	ep_state->data1 = 0;

	if (ep_dir == USB_EP_DIR_OUT) {
		regs->epc_out &= ~USB_USB_EPC1_REG_USB_STALL_Msk;
	} else {
		regs->epc_in &= ~USB_USB_EPC1_REG_USB_STALL_Msk;
	}

	if (ep_idx == 0) {
		REG_CLR_BIT(USB_MAMSK_REG, USB_M_EP0_NAK);
	}
	return 0;
}

int usb_dc_ep_enable(const uint8_t ep)
{
	struct smartbond_ep_state *ep_state = usb_dc_get_ep_state(ep);
	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	uint8_t ep_dir = USB_EP_GET_DIR(ep);

	if (ep_state == NULL) {
		LOG_ERR("Not valid endpoint: %02x", ep);
		return -EINVAL;
	}

	LOG_DBG("%02x", ep);

	if (ep_state->ep_addr == EP0_IN) {
		USB->USB_MAMSK_REG |= USB_USB_MAMSK_REG_USB_M_EP0_TX_Msk;
	} else if (ep_state->ep_addr == EP0_OUT) {
		USB->USB_MAMSK_REG |= USB_USB_MAMSK_REG_USB_M_EP0_RX_Msk;
		ep_state->last_packet_size = 0;
		ep_state->transferred = 0;
		ep_state->total_len = 0;
	} else if (ep_dir == USB_EP_DIR_OUT) {
		USB->USB_RXMSK_REG |= 0x11 << (ep_idx - 1);
		REG_SET_BIT(USB_MAMSK_REG, USB_M_RX_EV);
		ep_state->regs->epc_out |= USB_USB_EPC1_REG_USB_EP_EN_Msk;

		if (ep_state->busy) {
			return 0;
		}

		start_rx_packet(ep_state);
	} else {
		USB->USB_TXMSK_REG |= 0x11 << (ep_idx - 1);
		REG_SET_BIT(USB_MAMSK_REG, USB_M_TX_EV);
		ep_state->regs->epc_in |= USB_USB_EPC2_REG_USB_EP_EN_Msk;
	}

	return 0;
}

int usb_dc_ep_configure(const struct usb_dc_ep_cfg_data *const ep_cfg)
{
	struct smartbond_ep_state *ep_state = usb_dc_get_ep_state(ep_cfg->ep_addr);
	uint8_t ep_idx = USB_EP_GET_IDX(ep_cfg->ep_addr);
	uint8_t ep_dir = USB_EP_GET_DIR(ep_cfg->ep_addr);
	uint8_t iso_mask;

	if (ep_state == NULL) {
		return -EINVAL;
	}

	LOG_DBG("%02x", ep_cfg->ep_addr);

	ep_state->iso = ep_cfg->ep_type == USB_DC_EP_ISOCHRONOUS;
	iso_mask = (ep_state->iso ? USB_USB_EPC2_REG_USB_ISO_Msk : 0);

	if (ep_cfg->ep_type == USB_DC_EP_CONTROL) {
		ep_state->mps = EP0_FIFO_SIZE;
	} else {
		ep_state->mps = ep_cfg->ep_mps;
	}

	ep_state->data1 = 0;

	if (ep_dir == USB_EP_DIR_OUT) {
		if (ep_cfg->ep_mps > ep_out_buf_size[ep_idx]) {
			return -EINVAL;
		}

		ep_state->regs->epc_out = ep_idx | iso_mask;
	} else {
		ep_state->regs->epc_in = ep_idx | iso_mask;
	}

	return 0;
}

int usb_dc_detach(void)
{
	LOG_DBG("");

	REG_CLR_BIT(USB_MCTRL_REG, USB_NAT);

	dev_state.attached = false;

	return 0;
}

int usb_dc_attach(void)
{
	LOG_DBG("");
	if (GET_BIT(USB->USB_MCTRL_REG, USB_USB_MCTRL_REG_USB_NAT) == 0) {
		USB->USB_MCTRL_REG = USB_USB_MCTRL_REG_USBEN_Msk;
		USB->USB_NFSR_REG = 0;
		USB->USB_FAR_REG = 0x80;
		USB->USB_TXMSK_REG = 0;
		USB->USB_RXMSK_REG = 0;

		USB->USB_MAMSK_REG = USB_USB_MAMSK_REG_USB_M_INTR_Msk |
				     USB_USB_MAMSK_REG_USB_M_ALT_Msk |
				     USB_USB_MAMSK_REG_USB_M_WARN_Msk;
		USB->USB_ALTMSK_REG = USB_USB_ALTMSK_REG_USB_M_RESET_Msk |
				      USB_USB_ALTEV_REG_USB_SD3_Msk;

		USB->USB_MCTRL_REG = USB_USB_MCTRL_REG_USBEN_Msk |
				     USB_USB_MCTRL_REG_USB_NAT_Msk;

		/* Select chosen DMA to be triggered by USB. */
		DMA->DMA_REQ_MUX_REG =
			(DMA->DMA_REQ_MUX_REG & ~DA146XX_DMA_USB_MUX_MASK) |
			DA146XX_DMA_USB_MUX;
	}
	dev_state.attached = true;

	return 0;
}

int usb_dc_ep_write(const uint8_t ep, const uint8_t *const data, const uint32_t data_len,
		    uint32_t *const ret_bytes)
{
	struct smartbond_ep_state *ep_state = usb_dc_get_ep_state(ep);

	if (ep_state == NULL) {
		LOG_ERR("%02x no ep_state", ep);
		return -EINVAL;
	}

	LOG_DBG("%02x %d bytes", ep, (int)data_len);
	if (!atomic_cas(&ep_state->busy, 0, 1)) {
		LOG_DBG("%02x transfer already in progress", ep);
		return -EAGAIN;
	}

	ep_state->buffer = (uint8_t *)data;
	ep_state->transferred = 0;
	ep_state->total_len = data_len;
	ep_state->last_packet_size = 0;

	if (ep == EP0_IN) {
		/* RX has priority over TX to send packet RX needs to be off */
		REG_CLR_BIT(USB_RXC0_REG, USB_RX_EN);
		/* Handle case when device expect to send more data and
		 * host already send ZLP to confirm reception (that means
		 * that it will no longer try to read).
		 * Enable EP0_NAK.
		 */
		(void)USB->USB_EP0_NAK_REG;
		REG_SET_BIT(USB_MAMSK_REG, USB_M_EP0_NAK);
	}
	start_tx_packet(ep_state);

	if (ret_bytes) {
		*ret_bytes = data_len;
	}

	return 0;
}

int usb_dc_ep_set_stall(const uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	uint8_t ep_dir = USB_EP_GET_DIR(ep);
	struct smartbond_ep_state *ep_state = usb_dc_get_ep_state(ep);
	struct smartbond_ep_reg_set *regs;

	LOG_DBG("%02x", ep);

	if (!dev_attached() || !dev_ready()) {
		return -ENODEV;
	}

	if (ep_state == NULL) {
		LOG_ERR("Not valid endpoint: %02x", ep);
		return -EINVAL;
	}

	regs = ep_state->regs;
	ep_state->stall = 1;

	if (ep_idx == 0) {
		/* EP0 has just one registers to control stall for IN and OUT */
		if (ep_dir == USB_EP_DIR_OUT) {
			regs->rxc = USB_USB_RXC0_REG_USB_RX_EN_Msk;
			REG_SET_BIT(USB_EPC0_REG, USB_STALL);
		} else {
			regs->rxc = 0;
			regs->txc = USB_USB_TXC0_REG_USB_TX_EN_Msk;
			REG_SET_BIT(USB_EPC0_REG, USB_STALL);
		}
	} else {
		if (ep_dir == USB_EP_DIR_OUT) {
			regs->epc_out |= USB_USB_EPC1_REG_USB_STALL_Msk;
			regs->rxc |= USB_USB_RXC1_REG_USB_RX_EN_Msk;
		} else {
			regs->epc_in |= USB_USB_EPC1_REG_USB_STALL_Msk;
			regs->txc |= USB_USB_TXC1_REG_USB_TX_EN_Msk | USB_USB_TXC1_REG_USB_LAST_Msk;
		}
	}
	return 0;
}

int usb_dc_ep_is_stalled(const uint8_t ep, uint8_t *const stalled)
{
	struct smartbond_ep_state *ep_state = usb_dc_get_ep_state(ep);

	if (!dev_attached() || !dev_ready()) {
		return -ENODEV;
	}

	if (NULL == ep_state || NULL == stalled) {
		return -EINVAL;
	}

	*stalled = ep_state->stall;

	return 0;
}

int usb_dc_ep_halt(const uint8_t ep)
{
	return usb_dc_ep_set_stall(ep);
}

int usb_dc_ep_flush(const uint8_t ep)
{
	struct smartbond_ep_state *ep_state = usb_dc_get_ep_state(ep);

	if (ep_state == NULL) {
		LOG_ERR("Not valid endpoint: %02x", ep);
		return -EINVAL;
	}

	LOG_ERR("Not implemented");

	return 0;
}

SYS_INIT(usb_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

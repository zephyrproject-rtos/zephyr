/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_usbfs

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/usb/usb_ch9.h>

#include <cy_sysclk.h>
#ifdef CONFIG_UDC_INFINEON_USBFS_DMA
#include <cy_dma.h>
#include <cy_trigmux.h>
#endif

#include "udc_common.h"
#include "udc_infineon_usbfs_regs.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(udc_ifx_usbfs, CONFIG_UDC_DRIVER_LOG_LEVEL);

/* EP0 has its own 8 byte hardware buffer, the data endpoints share 512 bytes */
#define USBFS_EP0_MPS		8U
#define USBFS_EP_BUF_SIZE	512U
#define USBFS_NUM_DATA_EPS	8U
/* The shared endpoint buffer is split evenly so a slot never has to be resized */
#define USBFS_EP_SLOT_SIZE	(USBFS_EP_BUF_SIZE / USBFS_NUM_DATA_EPS)

/* A bulk endpoint ping-pongs between two slots: the host uses one while the other is copied */
#define USBFS_EP_SLOTS		2U

/* The bus reset detection logic expects a 100 kHz clock */
#define USBFS_BRS_CLOCK_HZ	100000U

/* Bus reset detection period, in cycles of the 100 kHz clock */
#define USBFS_BUS_RST_PERIOD	10U

/* OUT and SETUP byte counts include the two CRC bytes */
#define USBFS_CRC_LEN		2U

/* The PHY needs this long in suspend before it is released at power up */
#define USBFS_SUSPEND_US	2U

/* Bus reset and endpoint 0 on the low line, the data endpoints on the medium line */
#define USBFS_INTR_LVL_EP_MED(i, _)							\
	FIELD_PREP(IFX_USBFS_INTR_LVL_SEL_EP((i) + 1U), IFX_USBFS_INTR_LVL_MED)
#define USBFS_INTR_LVL_SEL								\
	(FIELD_PREP(IFX_USBFS_INTR_LVL_SEL_BUS_RESET, IFX_USBFS_INTR_LVL_LO) |		\
	 FIELD_PREP(IFX_USBFS_INTR_LVL_SEL_EP0, IFX_USBFS_INTR_LVL_LO) |		\
	 (LISTIFY(8, USBFS_INTR_LVL_EP_MED, (|))))

#ifdef CONFIG_UDC_INFINEON_USBFS_DMA
/*
 * usb.dma_req[i] and usb.dma_burstend[i] are dedicated 1-to-1 trigger lines,
 * not a routable mux, so data endpoint i + 1 is reachable only from DW0
 * channel 8 + i. The constants are absent on psoc6_01, which routes these
 * through the multiplexed group and would need Cy_TrigMux_Connect() instead
 */
#define USBFS_DMA_CHANNEL(i)	(8U + (i))
#define USBFS_TRIG_DMA_REQ(i)	((uint32_t)TRIG_OUT_1TO1_5_USB_DMA0_TO_PDMA0_TR_IN8 + (i))
#define USBFS_TRIG_BURSTEND(i)	((uint32_t)TRIG_OUT_1TO1_6_PDMA0_TR_OUT8_TO_USB_ACK0 + (i))
#endif

struct udc_ifx_usbfs_dma_cfg;

struct udc_ifx_usbfs_config {
	mem_addr_t base;
	en_clk_dst_t clk_dst;
	cy_en_divider_types_t brs_div_type;
	uint8_t brs_div_num;
	/* Indexed by data endpoint number minus one, NULL in CPU mode */
	const struct udc_ifx_usbfs_dma_cfg *dma_cfg;
	uint8_t num_dma_eps;
	struct udc_ep_config *ep_cfg_in;
	struct udc_ep_config *ep_cfg_out;
	void (*irq_connect)(void);
	void (*irq_enable_func)(void);
	void (*irq_disable_func)(void);
	void (*make_thread)(const struct device *dev);
};

#ifdef CONFIG_UDC_INFINEON_USBFS_DMA
struct udc_ifx_usbfs_dma_cfg {
	DW_Type *dw;
	uint32_t ch;
	cy_stc_dma_descriptor_t *descr[2];
};
#endif

enum usbfs_ep_state {
	USBFS_EP_DISABLED,
	USBFS_EP_IDLE,
	USBFS_EP_PENDING,
	USBFS_EP_COMPLETED,
	USBFS_EP_STALLED,
};

/* Hardware view of one data endpoint */
struct udc_ifx_usbfs_hw_ep {
	uint16_t mps;
	uint8_t addr;
	/* Mode that lets the endpoint respond to the host */
	uint8_t sie_mode;
	/* DATA toggle of the next packet */
	uint8_t toggle;
	uint8_t state;
	/* Armed when stalled, so clearing the halt re-arms it */
	bool stall_pending;
	/* The hardware buffer slots the endpoint owns - with two it ping-pongs */
	uint8_t slot_idx[USBFS_EP_SLOTS];
	uint8_t nslots;
	/* Slot the hardware is sending from or receiving into */
	uint8_t slot;
	/* IN: length of the packet staged in the other slot, or -1 */
	int16_t staged;
	/* OUT, early re-arm: the received packet's slot and byte count */
	uint8_t rd_slot;
	uint16_t rd_count;
	/* Re-armed straight from the interrupt entry, ahead of the completion handling */
	bool early;
};

enum usbfs_ep0_state {
	USBFS_EP0_IDLE,
	USBFS_EP0_SETUP,
	USBFS_EP0_DATA,
	USBFS_EP0_STATUS_IN,
	USBFS_EP0_STATUS_OUT,
};

struct udc_ifx_usbfs_data {
	struct k_thread thread_data;
	struct k_event events;
	struct usb_setup_packet setup;
	/* Bit per endpoint, IN endpoints are offset by 16 */
	atomic_t xfer_finished;
	/* Completions outrun the thread, so they are counted rather than flagged:
	 * a second one collapsing into the first leaves its buffer in the queue
	 * and the transfer is never handed to the stack
	 */
	atomic_t xfer_done[32];
	/* Offset of the next chunk to push into the 8 byte EP0 buffer */
	uint16_t ep0_in_off;
	bool ep0_in_zlp;
	/* Control transfer stage, advanced by the EP0 interrupt */
	uint8_t ep0_state;
	uint8_t ep0_toggle;
	uint8_t *ep0_out_dst;
	uint8_t ep0_out_len;
	/* Applied once the status stage of SET_ADDRESS completes */
	uint8_t address;
	bool set_address;
	/* Bit per hardware buffer slot in use */
	uint8_t slots_used;
	/* Indexed by data endpoint number minus one */
	struct udc_ifx_usbfs_hw_ep hw_ep[USBFS_NUM_DATA_EPS];
};

enum udc_ifx_usbfs_event {
	USBFS_EVT_SETUP,
	USBFS_EVT_XFER_FINISHED,
};

/* Endpoints 1-8 are single direction, with all even numbers being OUT and odds being
 * IN.  Endpoint 0 is bidrectional
 */
static inline bool ep_num_is_in(const unsigned int num)
{
	return (num % 2U) == 1U;
}

/* RESET_PTR rewinds the pointers when a transaction ends, even part way through a
 * copy, so it is only set where it is needed: manual DMA of a short OUT packet
 * reads zeros without it
 */
static inline uint32_t usbfs_arb_ep_cfg(const uint32_t ep)
{
	return IFX_USBFS_ARB_EP_CFG_CRC_BYPASS |
	       ((IS_ENABLED(CONFIG_UDC_INFINEON_USBFS_DMA) && !ep_num_is_in(ep)) ?
			IFX_USBFS_ARB_EP_CFG_RESET_PTR : 0U);
}

/* Write, then read back so the write has reached the block before returning */
static inline void usbfs_wr_flush(const struct device *dev, const mem_addr_t off,
				  const uint32_t val)
{
	const struct udc_ifx_usbfs_config *config = dev->config;

	sys_write32(val, config->base + off);
	(void)sys_read32(config->base + off);
}

static void usbfs_sie_mode_set(const struct device *dev, const uint32_t ep, const uint32_t mode)
{
	const struct udc_ifx_usbfs_config *config = dev->config;
	uint32_t cr0 = sys_read32(config->base + IFX_USBFS_SIE_EP_CR0(ep));

	cr0 &= ~IFX_USBFS_SIE_EP_CR0_MODE;
	cr0 |= FIELD_PREP(IFX_USBFS_SIE_EP_CR0_MODE, mode);
	usbfs_wr_flush(dev, IFX_USBFS_SIE_EP_CR0(ep), cr0);
}

/* Byte count of the last packet; the hardware count includes the CRC */
static uint32_t usbfs_sie_count(const struct device *dev, const uint32_t ep)
{
	const struct udc_ifx_usbfs_config *config = dev->config;
	uint32_t count;

	count = FIELD_GET(IFX_USBFS_SIE_EP_CNT0_DATA_COUNT_MSB,
			  sys_read32(config->base + IFX_USBFS_SIE_EP_CNT0(ep))) << 8;
	count |= FIELD_GET(IFX_USBFS_SIE_EP_CNT1_DATA_COUNT,
			   sys_read32(config->base + IFX_USBFS_SIE_EP_CNT1(ep)));

	return count - USBFS_CRC_LEN;
}

static void usbfs_sie_count_set(const struct device *dev, const uint32_t ep,
				const uint32_t count, const uint32_t toggle)
{
	const struct udc_ifx_usbfs_config *config = dev->config;

	sys_write32(count & 0xFFU, config->base + IFX_USBFS_SIE_EP_CNT1(ep));
	sys_write32(((count >> 8) & 0xFFU) | toggle, config->base + IFX_USBFS_SIE_EP_CNT0(ep));
}

/* Hot path arm: CR0's status bits are cleared by any write, so a plain write loses
 * nothing. STALL and NAK_INT_EN are clear on every path that re-arms a streaming endpoint
 */
static inline void usbfs_sie_arm(const struct device *dev, const uint32_t ep, const uint32_t mode)
{
	const struct udc_ifx_usbfs_config *config = dev->config;

	sys_write32(FIELD_PREP(IFX_USBFS_SIE_EP_CR0_MODE, mode),
		    config->base + IFX_USBFS_SIE_EP_CR0(ep));
}

static void usbfs_ep0_count_set(const struct device *dev, const uint32_t count,
				const uint32_t toggle)
{
	const struct udc_ifx_usbfs_config *config = dev->config;

	sys_write32(FIELD_PREP(IFX_USBFS_EP0_CNT_BYTE_COUNT, count) | toggle,
		    config->base + IFX_USBFS_EP0_CNT);
}

/* Mode that holds an endpoint off until a transfer is queued */
static uint32_t usbfs_inactive_mode(const uint32_t mode)
{
	switch (mode) {
	case IFX_USBFS_MODE_ACK_IN:
		return IFX_USBFS_MODE_NAK_IN;
	case IFX_USBFS_MODE_ACK_OUT:
		return IFX_USBFS_MODE_NAK_OUT;
	default:
		return IFX_USBFS_MODE_DISABLE;
	}
}

static void xfer_finished_submit(const struct device *dev, const uint8_t ep)
{
	struct udc_ifx_usbfs_data *priv = udc_get_private(dev);
	const unsigned int bnum = USB_EP_GET_IDX(ep) + (USB_EP_DIR_IS_IN(ep) ? 16U : 0U);

	atomic_inc(&priv->xfer_done[bnum]);
	atomic_set_bit(&priv->xfer_finished, bnum);
	k_event_post(&priv->events, BIT(USBFS_EVT_XFER_FINISHED));
}

/* Copy out of the 8 byte EP0 buffer what the last OUT or SETUP packet carried */
static uint32_t ep0_buf_read(const struct device *dev, uint8_t *const dst, const uint32_t size)
{
	const struct udc_ifx_usbfs_config *config = dev->config;
	uint32_t count;

	count = FIELD_GET(IFX_USBFS_EP0_CNT_BYTE_COUNT,
			  sys_read32(config->base + IFX_USBFS_EP0_CNT));
	count = MIN(count - USBFS_CRC_LEN, size);

	for (uint32_t i = 0U; i < count; i++) {
		dst[i] = (uint8_t)sys_read32(config->base + IFX_USBFS_EP0_DR(i));
	}

	return count;
}

/* Load an IN data stage packet of up to 8 bytes and arm EP0 for it */
static void ep0_write(const struct device *dev, const uint8_t *const src, const uint32_t len)
{
	const struct udc_ifx_usbfs_config *config = dev->config;
	struct udc_ifx_usbfs_data *priv = udc_get_private(dev);
	const uint32_t count = MIN(len, USBFS_EP0_MPS);

	for (uint32_t i = 0U; i < count; i++) {
		sys_write32(src[i], config->base + IFX_USBFS_EP0_DR(i));
	}

	priv->ep0_toggle ^= IFX_USBFS_EP0_CNT_DATA_TOGGLE;
	usbfs_ep0_count_set(dev, count, priv->ep0_toggle);
	usbfs_wr_flush(dev, IFX_USBFS_EP0_CR, IFX_USBFS_MODE_ACK_IN_STATUS_OUT);
	priv->ep0_state = USBFS_EP0_DATA;
}

/* Arm EP0 for an OUT data stage packet, landing up to len bytes at dst */
static void ep0_read_start(const struct device *dev, uint8_t *const dst, const uint32_t len)
{
	struct udc_ifx_usbfs_data *priv = udc_get_private(dev);

	if (len == 0U) {
		priv->ep0_state = USBFS_EP0_STATUS_OUT;
		return;
	}

	priv->ep0_out_dst = dst;
	priv->ep0_out_len = (uint8_t)len;
	usbfs_ep0_count_set(dev, 0U, 0U);
	usbfs_wr_flush(dev, IFX_USBFS_EP0_CR, IFX_USBFS_MODE_ACK_OUT_STATUS_IN);
	priv->ep0_state = USBFS_EP0_DATA;
}

/* Push the next chunk of a control IN transfer. The hardware buffer holds
 * eight bytes, so a descriptor is handed over a packet at a time
 */
static void ep0_in_xfer(const struct device *dev)
{
	struct udc_ifx_usbfs_data *priv = udc_get_private(dev);
	struct udc_ep_config *cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_IN);
	struct net_buf *buf = udc_buf_peek(cfg);
	size_t chunk;

	if (buf == NULL) {
		return;
	}

	chunk = MIN(buf->len - priv->ep0_in_off, USBFS_EP0_MPS);
	ep0_write(dev, buf->data + priv->ep0_in_off, chunk);
	priv->ep0_in_off += chunk;
}

/* The stack parks the buffer for the next SETUP packet on the same queue as
 * the data and status buffers, and it can sit in front of them, so it has to
 * be stepped over when servicing a transfer
 */
static struct net_buf *ep0_out_claim(const struct device *dev, const bool remove)
{
	struct udc_ep_config *cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
	struct net_buf *setup = NULL;
	struct net_buf *buf;

	while ((buf = udc_buf_get(cfg)) != NULL) {
		if (udc_get_buf_info(buf)->setup) {
			setup = buf;
			continue;
		}

		break;
	}

	if (!remove && buf != NULL) {
		udc_buf_put(cfg, buf);
	}

	if (setup != NULL) {
		udc_buf_put(cfg, setup);
	}

	return buf;
}

static void ep0_out_xfer(const struct device *dev)
{
	struct net_buf *buf = ep0_out_claim(dev, false);

	if (buf == NULL) {
		return;
	}

	ep0_read_start(dev, net_buf_tail(buf), MIN(net_buf_tailroom(buf), USBFS_EP0_MPS));
}

static void usbfs_bus_reset(const struct device *dev)
{
	const struct udc_ifx_usbfs_config *config = dev->config;
	struct udc_ifx_usbfs_data *priv = udc_get_private(dev);

	if ((sys_read32(config->base + IFX_USBFS_POWER_CTL) & IFX_USBFS_POWER_CTL_DP_UP_EN) == 0U) {
		return;
	}

	udc_submit_event(dev, UDC_EVT_RESET, 0);

	sys_write32(0U, config->base + IFX_USBFS_EP0_CNT);
	sys_write32(IFX_USBFS_MODE_NAK_INOUT, config->base + IFX_USBFS_EP0_CR);

	priv->set_address = false;
	priv->ep0_state = USBFS_EP0_IDLE;
	priv->slots_used = 0U;
	for (uint32_t ep = 1U; ep <= USBFS_NUM_DATA_EPS; ep++) {
		priv->hw_ep[ep - 1U].nslots = 0U;
	}

	/* Respond to traffic again, at address 0 */
	usbfs_wr_flush(dev, IFX_USBFS_CR0, IFX_USBFS_CR0_USB_ENABLE);
}

static void usbfs_ep0_in(const struct device *dev)
{
	struct udc_ifx_usbfs_data *priv = udc_get_private(dev);
	struct udc_ep_config *cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_IN);
	struct net_buf *buf = udc_buf_peek(cfg);

	if (buf == NULL) {
		return;
	}

	if (priv->ep0_in_off < buf->len) {
		ep0_in_xfer(dev);
		return;
	}

	if (priv->ep0_in_zlp) {
		priv->ep0_in_zlp = false;
		ep0_write(dev, buf->data, 0U);
		return;
	}

	/* The mode left behind by the data stage does not acknowledge the
	 * status packet, so the status OUT stage is armed here explicitly
	 */
	usbfs_ep0_count_set(dev, 0U, IFX_USBFS_EP0_CNT_DATA_TOGGLE);
	usbfs_wr_flush(dev, IFX_USBFS_EP0_CR, IFX_USBFS_MODE_STATUS_OUT_ONLY);

	xfer_finished_submit(dev, USB_CONTROL_EP_IN);
}

static void usbfs_ep0_out(const struct device *dev)
{
	struct udc_ifx_usbfs_data *priv = udc_get_private(dev);
	struct net_buf *buf = ep0_out_claim(dev, false);
	uint32_t received;

	if (buf == NULL) {
		return;
	}

	received = ep0_buf_read(dev, priv->ep0_out_dst, priv->ep0_out_len);
	net_buf_add(buf, received);

	if (received == USBFS_EP0_MPS && net_buf_tailroom(buf) != 0U) {
		ep0_out_xfer(dev);
		return;
	}

	xfer_finished_submit(dev, USB_CONTROL_EP_OUT);
}

static void usbfs_ep0_intr(const struct device *dev)
{
	const struct udc_ifx_usbfs_config *config = dev->config;
	struct udc_ifx_usbfs_data *priv = udc_get_private(dev);
	uint32_t cr = sys_read32(config->base + IFX_USBFS_EP0_CR);

	if ((cr & IFX_USBFS_EP0_CR_ACKED_TXN) == 0U) {
		return;
	}

	if ((cr & IFX_USBFS_EP0_CR_SETUP_RCVD) != 0U) {
		priv->ep0_state = USBFS_EP0_SETUP;

		if (FIELD_GET(IFX_USBFS_EP0_CR_MODE, cr) != IFX_USBFS_MODE_NAK_INOUT) {
			return;
		}

		/* EP0_CR stays locked while SETUP_RCVD is set so that a new SETUP cannot
		 * overwrite the buffer before it is read, and a write that is not locked
		 * out clears the status bits.
		 */
		usbfs_wr_flush(dev, IFX_USBFS_EP0_CR, cr);
		cr = sys_read32(config->base + IFX_USBFS_EP0_CR);
		if ((cr & IFX_USBFS_EP0_CR_SETUP_RCVD) == 0U) {
			priv->ep0_toggle = 0U;
			priv->ep0_in_off = 0U;
			priv->ep0_in_zlp = false;
			(void)ep0_buf_read(dev, (uint8_t *)&priv->setup, sizeof(priv->setup));
			k_event_post(&priv->events, BIT(USBFS_EVT_SETUP));
		} else {
			/* Still locked: come back through the interrupt */
			sys_write32(IFX_USBFS_INTR_SIE_EP0, config->base + IFX_USBFS_INTR_SIE_SET);
		}
	} else if ((cr & IFX_USBFS_EP0_CR_IN_RCVD) != 0U) {
		if (priv->ep0_state == USBFS_EP0_DATA) {
			usbfs_ep0_in(dev);
		} else if (priv->ep0_state == USBFS_EP0_STATUS_IN) {
			if (priv->set_address) {
				uint32_t cr0 = sys_read32(config->base + IFX_USBFS_CR0);

				cr0 &= ~IFX_USBFS_CR0_DEVICE_ADDRESS;
				cr0 |= FIELD_PREP(IFX_USBFS_CR0_DEVICE_ADDRESS, priv->address);
				sys_write32(cr0, config->base + IFX_USBFS_CR0);
				priv->set_address = false;
			}

			priv->ep0_state = USBFS_EP0_IDLE;
		} else if (priv->ep0_state == USBFS_EP0_STATUS_OUT) {
			usbfs_ep0_count_set(dev, 0U, IFX_USBFS_EP0_CNT_DATA_TOGGLE);
			usbfs_wr_flush(dev, IFX_USBFS_EP0_CR, IFX_USBFS_MODE_STATUS_OUT_ONLY);
			priv->ep0_state = USBFS_EP0_IDLE;
		} else {
			/* Nothing pending in this stage */
		}
	} else if ((cr & IFX_USBFS_EP0_CR_OUT_RCVD) != 0U) {
		if (priv->ep0_state == USBFS_EP0_DATA) {
			usbfs_ep0_out(dev);
		}
	} else {
		/* ACKed without a direction flag: nothing to service */
	}
}

static void usbfs_intr_dispatch(const struct device *dev, const uint32_t cause);

static void udc_ifx_usbfs_isr_lo(const struct device *dev)
{
	const struct udc_ifx_usbfs_config *config = dev->config;

	usbfs_intr_dispatch(dev, sys_read32(config->base + IFX_USBFS_INTR_CAUSE_LO));
}

static void usbfs_early_arm(const struct device *dev, const uint32_t cause);

static void udc_ifx_usbfs_isr_med(const struct device *dev)
{
	const struct udc_ifx_usbfs_config *config = dev->config;
	const uint32_t cause = sys_read32(config->base + IFX_USBFS_INTR_CAUSE_MED);

	usbfs_early_arm(dev, cause);
	usbfs_intr_dispatch(dev, cause);
}

static int udc_ifx_usbfs_brs_clock_init(const struct device *dev)
{
	const struct udc_ifx_usbfs_config *config = dev->config;
	uint32_t peri_hz = Cy_SysClk_ClkPeriGetFrequency();
	uint32_t div;

	if (peri_hz == 0U) {
		return -EIO;
	}

	div = peri_hz / USBFS_BRS_CLOCK_HZ;
	if (div == 0U) {
		return -ERANGE;
	}

	if (Cy_SysClk_PeriphAssignDivider(config->clk_dst, config->brs_div_type,
					  config->brs_div_num) != CY_SYSCLK_SUCCESS) {
		return -EIO;
	}

	if (Cy_SysClk_PeriphSetDivider(config->brs_div_type, config->brs_div_num,
				       div - 1U) != CY_SYSCLK_SUCCESS) {
		return -EIO;
	}

	if (Cy_SysClk_PeriphEnableDivider(config->brs_div_type,
					  config->brs_div_num) != CY_SYSCLK_SUCCESS) {
		return -EIO;
	}

	LOG_DBG("bus reset clock %u Hz (peri %u Hz / %u)",
		peri_hz / div, peri_hz, div);

	return 0;
}

/* Endpoint 0 has a single mode register, so it can only be armed for one
 * direction at a time. The data IN stage is therefore not started until the
 * stack has also queued the status OUT buffer
 */
static void ep0_in_start(const struct device *dev)
{
	struct udc_ifx_usbfs_data *priv = udc_get_private(dev);
	struct udc_ep_config *cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_IN);
	struct net_buf *buf = udc_buf_peek(cfg);
	struct udc_buf_info *bi;

	if (buf == NULL) {
		return;
	}

	bi = udc_get_buf_info(buf);
	priv->ep0_in_off = 0U;
	priv->ep0_in_zlp = bi->zlp;

	ep0_in_xfer(dev);
}

/* Byte offset in the endpoint buffer of one of the endpoint's slots */
static inline uint16_t usbfs_slot_off(const struct udc_ifx_usbfs_hw_ep *const hw,
				      const uint8_t slot)
{
	return hw->slot_idx[(hw->nslots > 1U) ? slot : 0U] * USBFS_EP_SLOT_SIZE;
}

/* Take a free slot of the hardware buffer */
static int usbfs_slot_take(const struct device *dev, uint8_t *const idx)
{
	struct udc_ifx_usbfs_data *priv = udc_get_private(dev);
	unsigned int key = irq_lock();
	const uint32_t free = ~(uint32_t)priv->slots_used & BIT_MASK(USBFS_NUM_DATA_EPS);
	int err = -ENOMEM;

	if (free != 0U) {
		*idx = (uint8_t)(find_lsb_set(free) - 1U);
		priv->slots_used |= BIT(*idx);
		err = 0;
	}

	irq_unlock(key);

	return err;
}

static void usbfs_slots_release(const struct device *dev, const uint32_t ep)
{
	struct udc_ifx_usbfs_data *priv = udc_get_private(dev);
	struct udc_ifx_usbfs_hw_ep *hw = &priv->hw_ep[ep - 1U];
	unsigned int key = irq_lock();

	for (uint8_t i = 0U; i < hw->nslots; i++) {
		priv->slots_used &= (uint8_t)~BIT(hw->slot_idx[i]);
	}

	hw->nslots = 0U;
	irq_unlock(key);
}

/* Every enabled endpoint gets one slot, so any configuration of up to eight
 * endpoints fits. A bulk endpoint takes a second, which lets it ping-pong,
 * when a transfer starts and one is free: nothing is in flight to move then
 */
static void usbfs_slot_add_second(const struct device *dev, const struct udc_ep_config *const cfg)
{
	struct udc_ifx_usbfs_data *priv = udc_get_private(dev);
	struct udc_ifx_usbfs_hw_ep *hw = &priv->hw_ep[USB_EP_GET_IDX(cfg->addr) - 1U];

	if ((hw->nslots != 1U) ||
	    ((cfg->attributes & USB_EP_TRANSFER_TYPE_MASK) != USB_EP_TYPE_BULK)) {
		return;
	}

	if (usbfs_slot_take(dev, &hw->slot_idx[1]) == 0) {
		hw->nslots = 2U;
	}
}

#ifdef CONFIG_UDC_INFINEON_USBFS_DMA
/* Channel control word with ENABLED set, so a start never reads the DMA block */
static uint32_t usbfs_dma_ch_ctl[USBFS_NUM_DATA_EPS];
/* X count each channel's descriptor currently holds, in halfwords */
static uint16_t usbfs_dma_xcount[USBFS_NUM_DATA_EPS];

static const cy_stc_dma_descriptor_config_t usbfs_dma_in_cfg = {
	.retrigger = CY_DMA_RETRIG_IM,
	.interruptType = CY_DMA_DESCR,
	.triggerOutType = CY_DMA_DESCR,
	.channelState = CY_DMA_CHANNEL_DISABLED,
	.triggerInType = CY_DMA_DESCR,
	.dataSize = CY_DMA_HALFWORD,
	.srcTransferSize = CY_DMA_TRANSFER_SIZE_DATA,
	.dstTransferSize = CY_DMA_TRANSFER_SIZE_WORD,
	.descriptorType = CY_DMA_1D_TRANSFER,
	.srcXincrement = 1,
	.dstXincrement = 0,
	.xCount = 1U,
	.yCount = 1U,
};

static const cy_stc_dma_descriptor_config_t usbfs_dma_out_cfg = {
	.retrigger = CY_DMA_RETRIG_IM,
	.interruptType = CY_DMA_DESCR,
	.triggerOutType = CY_DMA_DESCR,
	.channelState = CY_DMA_CHANNEL_DISABLED,
	.triggerInType = CY_DMA_DESCR,
	.dataSize = CY_DMA_HALFWORD,
	.srcTransferSize = CY_DMA_TRANSFER_SIZE_WORD,
	.dstTransferSize = CY_DMA_TRANSFER_SIZE_DATA,
	.descriptorType = CY_DMA_1D_TRANSFER,
	.srcXincrement = 0,
	.dstXincrement = 1,
	.xCount = 1U,
	.yCount = 1U,
};

static int usbfs_dma_init(const struct device *dev)
{
	const struct udc_ifx_usbfs_config *config = dev->config;

	for (uint8_t i = 0U; i < config->num_dma_eps; i++) {
		const struct udc_ifx_usbfs_dma_cfg *dma = &config->dma_cfg[i];
		const mem_addr_t dr16 = config->base + IFX_USBFS_ARB_RW_DR16(i + 1U);
		const cy_stc_dma_channel_config_t ch_cfg = {
			.descriptor = dma->descr[0],
			.preemptable = false,
			.priority = 0U,
			.enable = false,
			.bufferable = false,
		};

		(void)Cy_DMA_Descriptor_Init(dma->descr[0], &usbfs_dma_out_cfg);
		Cy_DMA_Descriptor_SetSrcAddress(dma->descr[0], (const void *)dr16);
		Cy_DMA_Descriptor_SetNextDescriptor(dma->descr[0], dma->descr[0]);

		(void)Cy_DMA_Descriptor_Init(dma->descr[1], &usbfs_dma_in_cfg);
		Cy_DMA_Descriptor_SetDstAddress(dma->descr[1], (const void *)dr16);
		Cy_DMA_Descriptor_SetNextDescriptor(dma->descr[1], dma->descr[1]);

		if (Cy_DMA_Channel_Init(dma->dw, dma->ch, &ch_cfg) != CY_DMA_SUCCESS) {
			return -EIO;
		}

		usbfs_dma_ch_ctl[i] = DW_CH_CTL(dma->dw, dma->ch) | DW_CH_STRUCT_CH_CTL_ENABLED_Msk;
		usbfs_dma_xcount[i] = 0U;
		Cy_DMA_Enable(dma->dw);
	}

	return 0;
}

/* The descriptor disables the channel when done, so ENABLED doubles as busy */
static void usbfs_dma_wait(const struct device *dev, const uint32_t ep)
{
	const struct udc_ifx_usbfs_config *config = dev->config;
	const struct udc_ifx_usbfs_dma_cfg *dma = &config->dma_cfg[ep - 1U];

	/* A packet takes a few microseconds; a channel still busy after this is stuck */
	for (uint32_t spins = 0U; Cy_DMA_Channel_IsEnabled(dma->dw, dma->ch); spins++) {
		if (spins == 10000U) {
			Cy_DMA_Channel_Disable(dma->dw, dma->ch);
			LOG_ERR("DMA stuck on endpoint %u", ep);
			return;
		}
	}
}

/* Run descriptor d on the endpoint's channel - it moves len bytes (even) */
static inline void usbfs_dma_kick(const struct device *dev, const uint32_t ep,
				  cy_stc_dma_descriptor_t *const d, const uint32_t len)
{
	const struct udc_ifx_usbfs_config *config = dev->config;
	const struct udc_ifx_usbfs_dma_cfg *dma = &config->dma_cfg[ep - 1U];

	if (usbfs_dma_xcount[ep - 1U] != (len / 2U)) {
		Cy_DMA_Descriptor_SetXloopDataCount(d, len / 2U);
		usbfs_dma_xcount[ep - 1U] = (uint16_t)(len / 2U);
	}

	DW_CH_CURR_PTR(dma->dw, dma->ch) = (uint32_t)d;
	DW_CH_IDX(dma->dw, dma->ch) = 0U;
	DW_CH_CTL(dma->dw, dma->ch) = usbfs_dma_ch_ctl[ep - 1U];

	/* A DMA_REQ rising edge raises the endpoint's DMA request */
	usbfs_wr_flush(dev, IFX_USBFS_ARB_EP_CFG(ep),
		       usbfs_arb_ep_cfg(ep) | IFX_USBFS_ARB_EP_CFG_DMA_REQ);
	sys_write32(usbfs_arb_ep_cfg(ep), config->base + IFX_USBFS_ARB_EP_CFG(ep));
}

/* IN endpoints are the odd-numbered ones, so endpoint ep uses entry (ep - 1) / 2 */
static uint8_t __aligned(4) usbfs_dma_in_bounce[USBFS_NUM_DATA_EPS / 2U][USBFS_EP_SLOT_SIZE];

/* In manual DMA mode the DMA writes the data register. A CPU write is refused unless a DMA grant
 * (on any endpoint) has occurred since the last fill
 */
static void usbfs_dma_in_load(const struct device *dev, const uint32_t ep,
			      const uint8_t *const data, const uint32_t len, const uint16_t off)
{
	const struct udc_ifx_usbfs_config *config = dev->config;
	cy_stc_dma_descriptor_t *const d = config->dma_cfg[ep - 1U].descr[1];
	const uint8_t *src = data;

	/* Halfword transfers need an even source, and must not read past an odd packet */
	if (((POINTER_TO_UINT(data) | len) & 1U) != 0U) {
		uint8_t *const bounce = usbfs_dma_in_bounce[(ep - 1U) / 2U];

		memcpy(bounce, data, len);
		src = bounce;
	}

	sys_write32(off, config->base + IFX_USBFS_ARB_RW_WA16(ep));
	d->src = (uint32_t)src;
	usbfs_dma_kick(dev, ep, d, ROUND_UP(len, 2U));
}

/* Start the DMA of count bytes (even) from offset off of the endpoint buffer to dst */
static void usbfs_dma_out_start(const struct device *dev, const uint32_t ep, void *const dst,
				const uint32_t count, const uint16_t off)
{
	const struct udc_ifx_usbfs_config *config = dev->config;
	cy_stc_dma_descriptor_t *const d = config->dma_cfg[ep - 1U].descr[0];

	sys_write32(off, config->base + IFX_USBFS_ARB_RW_RA16(ep));
	d->dst = (uint32_t)dst;
	usbfs_dma_kick(dev, ep, d, count);
}

/* Account for an early re-armed packet, whose DMA started with the re-arm */
static bool usbfs_dma_out_take(const struct device *dev, const uint32_t ep,
			       struct net_buf *const buf)
{
	struct udc_ifx_usbfs_data *priv = udc_get_private(dev);
	struct udc_ifx_usbfs_hw_ep *hw = &priv->hw_ep[ep - 1U];

	if (!hw->early) {
		return false;
	}

	net_buf_add(buf, hw->rd_count);
	hw->state = USBFS_EP_PENDING;

	return true;
}
#endif

#ifndef CONFIG_UDC_INFINEON_USBFS_DMA
/* Endpoint buffers start on even offsets per DR16's requirements. An odd byte goes
 * out padded: mixing in DR accesses skews the pointer for later packets
 */
static void usbfs_in_copy(const struct device *dev, const uint32_t ep, const uint16_t off,
			  const uint8_t *const data, const uint32_t len)
{
	const struct udc_ifx_usbfs_config *config = dev->config;

	sys_write32(off, config->base + IFX_USBFS_ARB_RW_WA16(ep));

	for (uint32_t i = 0U; i < len; i += 2U) {
		const uint32_t hi = ((i + 1U) < len) ? data[i + 1U] : 0U;

		sys_write32(data[i] | (hi << 8), config->base + IFX_USBFS_ARB_RW_DR16(ep));
	}
}
#endif

/* Load an IN packet and arm the endpoint to send it */
static void usbfs_in_load(const struct device *dev, const uint32_t ep,
			  const uint8_t *const data, const uint32_t len)
{
	const struct udc_ifx_usbfs_config *config = dev->config;
	struct udc_ifx_usbfs_data *priv = udc_get_private(dev);
	struct udc_ifx_usbfs_hw_ep *hw = &priv->hw_ep[ep - 1U];

	hw->state = USBFS_EP_PENDING;
	hw->staged = -1;
	hw->early = false;
	usbfs_sie_count_set(dev, ep, len, hw->toggle);

	if (len == 0U) {
		usbfs_sie_mode_set(dev, ep, hw->sie_mode);
		return;
	}

	const uint16_t off = usbfs_slot_off(hw, hw->slot);

#ifdef CONFIG_UDC_INFINEON_USBFS_DMA
	usbfs_dma_wait(dev, ep);
	usbfs_dma_in_load(dev, ep, data, len, off);
	usbfs_dma_wait(dev, ep);
#else
	usbfs_in_copy(dev, ep, off, data, len);
#endif
	sys_write32(off, config->base + IFX_USBFS_ARB_RW_RA16(ep));
	usbfs_sie_mode_set(dev, ep, hw->sie_mode);
}

/* Account for the staged packet being armed, arming it here if the interrupt entry did not */
static bool usbfs_in_arm_staged(const struct device *dev, const uint32_t ep, const uint32_t len)
{
	const struct udc_ifx_usbfs_config *config = dev->config;
	struct udc_ifx_usbfs_data *priv = udc_get_private(dev);
	struct udc_ifx_usbfs_hw_ep *hw = &priv->hw_ep[ep - 1U];

	if ((hw->staged < 0) || ((uint32_t)hw->staged != len)) {
		return false;
	}

	hw->staged = -1;
	hw->slot ^= 1U;
	hw->state = USBFS_EP_PENDING;

	if (hw->early) {
		return true;
	}

#ifdef CONFIG_UDC_INFINEON_USBFS_DMA
	usbfs_dma_wait(dev, ep);
#endif

	usbfs_sie_count_set(dev, ep, len, hw->toggle);
	sys_write32(usbfs_slot_off(hw, hw->slot), config->base + IFX_USBFS_ARB_RW_RA16(ep));
	usbfs_sie_mode_set(dev, ep, hw->sie_mode);

	return true;
}

static void usbfs_in_early_arm(const struct device *dev, const uint32_t ep)
{
	const struct udc_ifx_usbfs_config *config = dev->config;
	struct udc_ifx_usbfs_data *priv = udc_get_private(dev);
	struct udc_ifx_usbfs_hw_ep *hw = &priv->hw_ep[ep - 1U];

	if ((hw->staged < 0) || (hw->sie_mode == IFX_USBFS_MODE_ISO_IN)) {
		return;
	}

#ifdef CONFIG_UDC_INFINEON_USBFS_DMA
	/* Left to the completion handler, which waits for the DMA */
	if (Cy_DMA_Channel_IsEnabled(config->dma_cfg[ep - 1U].dw, config->dma_cfg[ep - 1U].ch)) {
		return;
	}
#endif

	/* Cleared first, so a later status bit can only be the staged packet's */
	sys_write32(BIT(ep - 1U), config->base + IFX_USBFS_SIE_EP_INT_SR);
	usbfs_sie_count_set(dev, ep, (uint32_t)hw->staged,
			    hw->toggle ^ IFX_USBFS_SIE_EP_CNT0_DATA_TOGGLE);
	sys_write32(usbfs_slot_off(hw, hw->slot ^ 1U), config->base + IFX_USBFS_ARB_RW_RA16(ep));
	usbfs_sie_arm(dev, ep, hw->sie_mode);
	hw->early = true;
}

/* A full packet with room for another behind it lets the host send the next
 * one into the other slot while this one is copied out. Its byte count and
 * toggle are captured first: re-arming hands both registers to the next packet
 */
static void usbfs_out_early_arm(const struct device *dev, const uint32_t ep)
{
	const struct udc_ifx_usbfs_config *config = dev->config;
	struct udc_ifx_usbfs_data *priv = udc_get_private(dev);
	struct udc_ifx_usbfs_hw_ep *hw = &priv->hw_ep[ep - 1U];
	struct udc_ep_config *cfg = udc_get_ep_cfg(dev, hw->addr);
	struct net_buf *buf;
	uint32_t cnt0;
	uint32_t count;

	if ((hw->nslots < 2U) || (hw->sie_mode == IFX_USBFS_MODE_ISO_OUT) || cfg->stat.halted) {
		return;
	}

	cnt0 = sys_read32(config->base + IFX_USBFS_SIE_EP_CNT0(ep));
	if ((cnt0 & IFX_USBFS_SIE_EP_CNT0_DATA_TOGGLE) != hw->toggle) {
		return;
	}

	count = ((FIELD_GET(IFX_USBFS_SIE_EP_CNT0_DATA_COUNT_MSB, cnt0) << 8) |
		 FIELD_GET(IFX_USBFS_SIE_EP_CNT1_DATA_COUNT,
			   sys_read32(config->base + IFX_USBFS_SIE_EP_CNT1(ep)))) - USBFS_CRC_LEN;
	if (count != hw->mps) {
		return;
	}

	buf = udc_buf_peek(cfg);
	if ((buf == NULL) || (net_buf_tailroom(buf) < (2U * hw->mps))) {
		return;
	}

#ifdef CONFIG_UDC_INFINEON_USBFS_DMA
	/* The packet is copied straight into the buffer, in halfwords */
	if (((POINTER_TO_UINT(net_buf_tail(buf)) | count) & 1U) != 0U) {
		return;
	}
#endif

	sys_write32(BIT(ep - 1U), config->base + IFX_USBFS_SIE_EP_INT_SR);
	hw->rd_slot = hw->slot;
	hw->rd_count = (uint16_t)count;
	hw->slot ^= 1U;
	sys_write32(usbfs_slot_off(hw, hw->slot), config->base + IFX_USBFS_ARB_RW_WA16(ep));

#ifdef CONFIG_UDC_INFINEON_USBFS_DMA
	unsigned int key = irq_lock();

	usbfs_sie_arm(dev, ep, hw->sie_mode);
	usbfs_dma_wait(dev, ep);
	usbfs_dma_out_start(dev, ep, net_buf_tail(buf), count, usbfs_slot_off(hw, hw->rd_slot));
	irq_unlock(key);
#else
	usbfs_sie_arm(dev, ep, hw->sie_mode);
#endif

	hw->early = true;
}

/* The host's next token follows a completed transaction closely, so the
 * endpoint is re-armed on interrupt entry rather than after the generic
 * dispatch whenever the next packet is already known
 */
static void usbfs_early_arm(const struct device *dev, const uint32_t cause)
{
	struct udc_ifx_usbfs_data *priv = udc_get_private(dev);
	uint32_t eps = (cause >> 8) & BIT_MASK(USBFS_NUM_DATA_EPS);

	while (eps != 0U) {
		const uint32_t ep = find_lsb_set(eps);

		eps &= eps - 1U;

		if (USB_EP_DIR_IS_IN(priv->hw_ep[ep - 1U].addr)) {
			usbfs_in_early_arm(dev, ep);
		} else {
			usbfs_out_early_arm(dev, ep);
		}
	}
}

/* Copies the packet after the armed one into the other slot while the host
 * reads, so its completion only has to arm it. Called from the completion
 * interrupt only, which is what keeps it from racing another copy
 */
static void usbfs_in_stage(const struct device *dev, const struct udc_ep_config *const cfg,
			   const struct net_buf *const buf)
{
	struct udc_ifx_usbfs_data *priv = udc_get_private(dev);
	const uint32_t ep = USB_EP_GET_IDX(cfg->addr);
	struct udc_ifx_usbfs_hw_ep *hw = &priv->hw_ep[ep - 1U];
	const uint32_t armed = MIN(buf->len, cfg->mps);
	uint32_t len;

	if ((hw->nslots < 2U) || (buf->len <= armed)) {
		return;
	}

	len = MIN(buf->len - armed, cfg->mps);

#ifdef CONFIG_UDC_INFINEON_USBFS_DMA
	usbfs_dma_in_load(dev, ep, &buf->data[armed], len, usbfs_slot_off(hw, hw->slot ^ 1U));
#else
	usbfs_in_copy(dev, ep, usbfs_slot_off(hw, hw->slot ^ 1U), &buf->data[armed], len);
#endif
	hw->staged = (int16_t)len;
}

/* Copy the received OUT packet out of the hardware buffer */
static int usbfs_out_read(const struct device *dev, const uint32_t ep, uint8_t *const dst,
			  const size_t size, uint32_t *const received)
{
	struct udc_ifx_usbfs_data *priv = udc_get_private(dev);
	const struct udc_ifx_usbfs_hw_ep *hw = &priv->hw_ep[ep - 1U];
	const uint32_t count = hw->early ? hw->rd_count : usbfs_sie_count(dev, ep);
	const uint16_t off = usbfs_slot_off(hw, hw->early ? hw->rd_slot : hw->slot);

	*received = 0U;

	if (count > size) {
		return -ENOMEM;
	}

#ifdef CONFIG_UDC_INFINEON_USBFS_DMA
	/* The CPU reads zeros from DR in manual DMA mode, so this goes through a bounce buffer */
	static uint8_t __aligned(4) bounce[USBFS_EP_SLOT_SIZE];

	if (count != 0U) {
		/* The previous packet's early copy may still be running on the channel */
		usbfs_dma_wait(dev, ep);
		usbfs_dma_out_start(dev, ep, bounce, ROUND_UP(count, 2U), off);
		usbfs_dma_wait(dev, ep);
		memcpy(dst, bounce, count);
	}
#else
	const struct udc_ifx_usbfs_config *config = dev->config;

	sys_write32(off, config->base + IFX_USBFS_ARB_RW_RA16(ep));

	for (uint32_t i = 0U; i < count; i += 2U) {
		const uint32_t v = sys_read32(config->base + IFX_USBFS_ARB_RW_DR16(ep));

		dst[i] = (uint8_t)v;
		if ((i + 1U) < count) {
			dst[i + 1U] = (uint8_t)(v >> 8);
		}
	}
#endif

	*received = count;

	return 0;
}

/* The hardware buffer holds one packet, so a request longer than the maximum
 * packet size is handed over a packet at a time. The caller owns the busy
 * flag: continuations come straight here because the endpoint is already busy
 */
static void ep_data_arm(const struct device *dev, struct udc_ep_config *const cfg,
			struct net_buf *const buf)
{
	const struct udc_ifx_usbfs_config *config = dev->config;
	struct udc_ifx_usbfs_data *priv = udc_get_private(dev);
	const uint32_t ep = USB_EP_GET_IDX(cfg->addr);
	struct udc_ifx_usbfs_hw_ep *hw = &priv->hw_ep[ep - 1U];

	if (USB_EP_DIR_IS_IN(cfg->addr)) {
		usbfs_in_load(dev, ep, buf->data, MIN(buf->len, cfg->mps));
		return;
	}

	/* Let the host write the next packet into the endpoint. */
	hw->state = USBFS_EP_PENDING;
	sys_write32(usbfs_slot_off(hw, hw->slot), config->base + IFX_USBFS_ARB_RW_WA16(ep));
	usbfs_sie_mode_set(dev, ep, hw->sie_mode);
}

static void ep_data_start(const struct device *dev, struct udc_ep_config *const cfg)
{
	struct net_buf *buf;
	unsigned int key;

	key = irq_lock();

	if (cfg->stat.halted || udc_ep_is_busy(cfg)) {
		irq_unlock(key);
		return;
	}

	buf = udc_buf_peek(cfg);
	if (buf == NULL) {
		irq_unlock(key);
		return;
	}

	udc_ep_set_busy(cfg, true);
	usbfs_slot_add_second(dev, cfg);
	irq_unlock(key);

	ep_data_arm(dev, cfg, buf);
}

static void usbfs_ep_complete(const struct device *dev, const uint8_t ep_addr,
			      const bool same_toggle)
{
	struct udc_ep_config *cfg = udc_get_ep_cfg(dev, ep_addr);
	struct net_buf *buf = udc_buf_peek(cfg);

	if (buf == NULL) {
		return;
	}

	if (USB_EP_DIR_IS_IN(ep_addr)) {

		net_buf_pull(buf, MIN(buf->len, cfg->mps));

		if (buf->len != 0U) {
			if (!usbfs_in_arm_staged(dev, USB_EP_GET_IDX(ep_addr),
						 MIN(buf->len, cfg->mps))) {
				ep_data_arm(dev, cfg, buf);
			}

			usbfs_in_stage(dev, cfg, buf);
			return;
		}
	} else {
		uint32_t received;

		/* A packet the host believes was not acknowledged carries the
		 * toggle of the previous one and must not reach the stack twice.
		 */
		if (same_toggle) {
			ep_data_arm(dev, cfg, buf);
			return;
		}

#ifdef CONFIG_UDC_INFINEON_USBFS_DMA
		if (usbfs_dma_out_take(dev, USB_EP_GET_IDX(ep_addr), buf)) {
			return;
		}
#endif

		/* Nothing is copied when the packet is larger than the tailroom. */
		if (usbfs_out_read(dev, USB_EP_GET_IDX(ep_addr), net_buf_tail(buf),
				   net_buf_tailroom(buf), &received) != 0) {
			LOG_ERR("Dropped packet on endpoint 0x%02x", cfg->addr);
		}

		net_buf_add(buf, received);

		if (received == cfg->mps && net_buf_tailroom(buf) != 0U) {
			struct udc_ifx_usbfs_data *priv = udc_get_private(dev);
			struct udc_ifx_usbfs_hw_ep *hw = &priv->hw_ep[USB_EP_GET_IDX(ep_addr) - 1U];

			if (hw->early) {
				hw->state = USBFS_EP_PENDING;
			} else {
				ep_data_arm(dev, cfg, buf);
			}

			return;
		}
	}

	xfer_finished_submit(dev, ep_addr);
}

static void usbfs_sie_ep_intr(const struct device *dev, const uint32_t ep)
{
	const struct udc_ifx_usbfs_config *config = dev->config;
	struct udc_ifx_usbfs_data *priv = udc_get_private(dev);
	struct udc_ifx_usbfs_hw_ep *hw = &priv->hw_ep[ep - 1U];
	const bool iso = (hw->sie_mode == IFX_USBFS_MODE_ISO_IN) ||
			 (hw->sie_mode == IFX_USBFS_MODE_ISO_OUT);
	bool same_toggle = false;

	/* An early armed endpoint was cleared then - clearing again could drop the next one */
	if (!hw->early) {
		usbfs_wr_flush(dev, IFX_USBFS_SIE_EP_INT_SR, BIT(ep - 1U));
	}

	if (!iso) {
		hw->toggle ^= IFX_USBFS_SIE_EP_CNT0_DATA_TOGGLE;
	}

	hw->state = USBFS_EP_COMPLETED;

	/* Only an OUT packet can repeat the toggle*/
	if (!iso && USB_EP_DIR_IS_OUT(hw->addr) && !hw->early &&
	    (hw->toggle == (sys_read32(config->base + IFX_USBFS_SIE_EP_CNT0(ep)) &
			    IFX_USBFS_SIE_EP_CNT0_DATA_TOGGLE))) {
		same_toggle = true;
		hw->toggle ^= IFX_USBFS_SIE_EP_CNT0_DATA_TOGGLE;
	}

	usbfs_ep_complete(dev, hw->addr, same_toggle);
	hw->early = false;
}

static void usbfs_intr_dispatch(const struct device *dev, const uint32_t cause)
{
	usbfs_wr_flush(dev, IFX_USBFS_INTR_SIE, cause);

	for (uint32_t eps = (cause >> 8) & BIT_MASK(USBFS_NUM_DATA_EPS); eps != 0U;
	     eps &= eps - 1U) {
		usbfs_sie_ep_intr(dev, find_lsb_set(eps));
	}

	if ((cause & IFX_USBFS_INTR_CAUSE_EP0) != 0U) {
		usbfs_ep0_intr(dev);
	}

	if ((cause & IFX_USBFS_INTR_CAUSE_BUS_RESET) != 0U) {
		usbfs_bus_reset(dev);
	}
}

static int usbfs_ep_add(const struct device *dev, const struct udc_ep_config *const cfg)
{
	const struct udc_ifx_usbfs_config *config = dev->config;
	struct udc_ifx_usbfs_data *priv = udc_get_private(dev);
	const uint32_t ep = USB_EP_GET_IDX(cfg->addr);
	const bool in = USB_EP_DIR_IS_IN(cfg->addr);
	struct udc_ifx_usbfs_hw_ep *hw = &priv->hw_ep[ep - 1U];
	int err;

	err = usbfs_slot_take(dev, &hw->slot_idx[0]);
	if (err != 0) {
		return err;
	}

	hw->nslots = 1U;
	sys_write32(usbfs_slot_off(hw, 0U), config->base + IFX_USBFS_ARB_RW_WA16(ep));
	sys_write32(usbfs_slot_off(hw, 0U), config->base + IFX_USBFS_ARB_RW_RA16(ep));

	hw->state = USBFS_EP_IDLE;
	hw->addr = cfg->addr;
	hw->toggle = 0U;
	hw->mps = cfg->mps;
	if ((cfg->attributes & USB_EP_TRANSFER_TYPE_MASK) == USB_EP_TYPE_ISO) {
		hw->sie_mode = in ? IFX_USBFS_MODE_ISO_IN : IFX_USBFS_MODE_ISO_OUT;
	} else {
		hw->sie_mode = in ? IFX_USBFS_MODE_ACK_IN : IFX_USBFS_MODE_ACK_OUT;
	}
	hw->stall_pending = false;
	hw->slot = 0U;
	hw->staged = -1;
	hw->early = false;

	/* Also clears any DMA request left from before */
	sys_write32(usbfs_arb_ep_cfg(ep), config->base + IFX_USBFS_ARB_EP_CFG(ep));

	sys_set_bits(config->base + IFX_USBFS_SIE_EP_INT_EN, BIT(ep - 1U));

	usbfs_sie_mode_set(dev, ep, usbfs_inactive_mode(hw->sie_mode));

	return 0;
}

static void usbfs_ep_remove(const struct device *dev, const uint32_t ep)
{
	const struct udc_ifx_usbfs_config *config = dev->config;
	struct udc_ifx_usbfs_data *priv = udc_get_private(dev);
	/* The completion interrupt must not re-arm the endpoint or start a DMA part way through */
	unsigned int key = irq_lock();

	usbfs_sie_mode_set(dev, ep, IFX_USBFS_MODE_DISABLE);
	priv->hw_ep[ep - 1U].state = USBFS_EP_DISABLED;

	sys_clear_bits(config->base + IFX_USBFS_SIE_EP_INT_EN, BIT(ep - 1U));

	usbfs_wr_flush(dev, IFX_USBFS_SIE_EP_INT_SR, BIT(ep - 1U));

#ifdef CONFIG_UDC_INFINEON_USBFS_DMA
	usbfs_dma_wait(dev, ep);
#endif

	usbfs_slots_release(dev, ep);
	irq_unlock(key);
}

static int usbfs_ep_stall(const struct device *dev, const uint32_t ep)
{
	struct udc_ifx_usbfs_data *priv = udc_get_private(dev);
	struct udc_ifx_usbfs_hw_ep *hw = &priv->hw_ep[ep - 1U];

	if (hw->state == USBFS_EP_DISABLED) {
		return -EINVAL;
	}

	hw->stall_pending = (hw->state == USBFS_EP_PENDING);
	hw->staged = -1;
	usbfs_wr_flush(dev, IFX_USBFS_SIE_EP_CR0(ep),
		       IFX_USBFS_SIE_EP_CR0_STALL |
		       (USB_EP_DIR_IS_IN(hw->addr) ? IFX_USBFS_MODE_ACK_IN
						   : IFX_USBFS_MODE_ACK_OUT));
	hw->state = USBFS_EP_STALLED;

	return 0;
}

static int usbfs_ep_unstall(const struct device *dev, const uint32_t ep)
{
	const struct udc_ifx_usbfs_config *config = dev->config;
	struct udc_ifx_usbfs_data *priv = udc_get_private(dev);
	struct udc_ifx_usbfs_hw_ep *hw = &priv->hw_ep[ep - 1U];
	unsigned int key;
	uint32_t mode;
	uint32_t cr0;

	if (hw->state == USBFS_EP_DISABLED) {
		return -EINVAL;
	}

	key = irq_lock();

	hw->toggle = 0U;

	if (hw->stall_pending) {
		hw->state = USBFS_EP_PENDING;
		mode = hw->sie_mode;
	} else {
		hw->state = USBFS_EP_IDLE;
		mode = usbfs_inactive_mode(hw->sie_mode);
	}

	sys_clear_bits(config->base + IFX_USBFS_SIE_EP_CNT0(ep), IFX_USBFS_SIE_EP_CNT0_DATA_TOGGLE);

	cr0 = sys_read32(config->base + IFX_USBFS_SIE_EP_CR0(ep));
	cr0 &= ~(IFX_USBFS_SIE_EP_CR0_MODE | IFX_USBFS_SIE_EP_CR0_STALL);
	cr0 |= FIELD_PREP(IFX_USBFS_SIE_EP_CR0_MODE, mode);
	usbfs_wr_flush(dev, IFX_USBFS_SIE_EP_CR0(ep), cr0);

	irq_unlock(key);

	return 0;
}

static int udc_ifx_usbfs_ep_enqueue(const struct device *dev,
				    struct udc_ep_config *const cfg,
				    struct net_buf *const buf)
{
	struct udc_ifx_usbfs_data *priv = udc_get_private(dev);
	struct udc_buf_info *bi = udc_get_buf_info(buf);

	udc_buf_put(cfg, buf);

	if (cfg->addr == USB_CONTROL_EP_IN) {
		if (bi->status) {
			/* Status stage ack; its interrupt only applies a pending address. */
			usbfs_ep0_count_set(dev, 0U, IFX_USBFS_EP0_CNT_DATA_TOGGLE);
			usbfs_wr_flush(dev, IFX_USBFS_EP0_CR, IFX_USBFS_MODE_STATUS_IN_ONLY);
			priv->ep0_state = USBFS_EP0_STATUS_IN;
			xfer_finished_submit(dev, cfg->addr);
		}

		return 0;
	}

	if (cfg->addr == USB_CONTROL_EP_OUT) {
		if (bi->setup) {
			/* SETUP data is received without any action. */
			return 0;
		}

		if (bi->status) {
			ep0_in_start(dev);
			xfer_finished_submit(dev, cfg->addr);
			return 0;
		}

		ep0_out_xfer(dev);

		return 0;
	}

	ep_data_start(dev, cfg);

	return 0;
}

static int udc_ifx_usbfs_ep_dequeue(const struct device *dev,
				    struct udc_ep_config *const cfg)
{
	struct udc_ifx_usbfs_data *priv = udc_get_private(dev);
	const uint32_t ep = USB_EP_GET_IDX(cfg->addr);
	unsigned int lock_key;

	lock_key = irq_lock();
	if (ep != 0U) {
		/* A staged packet belongs to the transfer being cancelled */
		priv->hw_ep[ep - 1U].staged = -1;
#ifdef CONFIG_UDC_INFINEON_USBFS_DMA
		usbfs_dma_wait(dev, ep);
#endif
	}
	udc_ep_set_busy(cfg, false);
	udc_ep_cancel_queued(dev, cfg);
	irq_unlock(lock_key);

	return 0;
}

static int udc_ifx_usbfs_ep_set_halt(const struct device *dev,
				     struct udc_ep_config *const cfg)
{
	unsigned int key;

	if (USB_EP_GET_IDX(cfg->addr) == 0U) {
		usbfs_wr_flush(dev, IFX_USBFS_EP0_CR, IFX_USBFS_MODE_STALL_INOUT);
		return 0;
	}

	/* An early re-arm before halted is set would clear the STALL bit */
	key = irq_lock();
	if (usbfs_ep_stall(dev, USB_EP_GET_IDX(cfg->addr)) != 0) {
		irq_unlock(key);
		return -EIO;
	}

	cfg->stat.halted = true;
	irq_unlock(key);

	return 0;
}

static int udc_ifx_usbfs_ep_clear_halt(const struct device *dev,
				       struct udc_ep_config *const cfg)
{
	struct net_buf *buf;

	if (USB_EP_GET_IDX(cfg->addr) == 0U) {
		return 0;
	}

	if (usbfs_ep_unstall(dev, USB_EP_GET_IDX(cfg->addr)) != 0) {
		return -EIO;
	}

	cfg->stat.halted = false;

	/* The host clears a halt to reset the data toggle even when the
	 * endpoint was never stalled, and unstalling leaves an endpoint with no
	 * transfer pending inactive, so an already queued one has to be rearmed
	 */
	buf = udc_buf_peek(cfg);
	if (buf != NULL) {
		ep_data_arm(dev, cfg, buf);
	} else {
		udc_ep_set_busy(cfg, false);
	}

	return 0;
}

static int udc_ifx_usbfs_ep_enable(const struct device *dev,
				   struct udc_ep_config *const cfg)
{
	const uint32_t ep = USB_EP_GET_IDX(cfg->addr);
	int err;

	/* Endpoint 0 is always present, only the data endpoints are allocated */
	if (ep == 0U) {
		return 0;
	}

	err = usbfs_ep_add(dev, cfg);
	if (err != 0) {
		LOG_ERR("Failed to add endpoint 0x%02x (%d)", cfg->addr, err);
		return -EIO;
	}

#if defined(CONFIG_UDC_INFINEON_USBFS_DMA)
	const struct udc_ifx_usbfs_config *config = dev->config;

	/* Latch the arbiter's manual DMA mode */
	usbfs_wr_flush(dev, IFX_USBFS_ARB_CFG,
		       FIELD_PREP(IFX_USBFS_ARB_CFG_DMA_CFG, IFX_USBFS_ARB_CFG_DMA_CFG_MANUAL));
	usbfs_wr_flush(dev, IFX_USBFS_ARB_CFG,
		       sys_read32(config->base + IFX_USBFS_ARB_CFG) | IFX_USBFS_ARB_CFG_CFG_CMP);
#endif

	LOG_DBG("Enable endpoint 0x%02x mps %u", cfg->addr, cfg->mps);

	return 0;
}

static int udc_ifx_usbfs_ep_disable(const struct device *dev,
				    struct udc_ep_config *const cfg)
{
	if (USB_EP_GET_IDX(cfg->addr) == 0U) {
		return 0;
	}

	usbfs_ep_remove(dev, USB_EP_GET_IDX(cfg->addr));

	return 0;
}

static int udc_ifx_usbfs_host_wakeup(const struct device *dev)
{
	ARG_UNUSED(dev);

	return -ENOTSUP;
}

static int udc_ifx_usbfs_set_address(const struct device *dev, const uint8_t addr)
{
	struct udc_ifx_usbfs_data *priv = udc_get_private(dev);

	/* Latched into CR0 once the status stage of the request completes */
	priv->address = addr;
	priv->set_address = true;

	return 0;
}

static int udc_ifx_usbfs_enable(const struct device *dev)
{
	const struct udc_ifx_usbfs_config *config = dev->config;

	sys_write32(0U, config->base + IFX_USBFS_EP0_CNT);
	sys_write32(IFX_USBFS_MODE_NAK_INOUT, config->base + IFX_USBFS_EP0_CR);

	/* The D+ pull-up is what makes the host see an attach */
	usbfs_wr_flush(dev, IFX_USBFS_POWER_CTL,
		       sys_read32(config->base + IFX_USBFS_POWER_CTL) |
		       IFX_USBFS_POWER_CTL_DP_UP_EN);

	return 0;
}

static int udc_ifx_usbfs_disable(const struct device *dev)
{
	const struct udc_ifx_usbfs_config *config = dev->config;

	usbfs_wr_flush(dev, IFX_USBFS_POWER_CTL,
		       sys_read32(config->base + IFX_USBFS_POWER_CTL) &
		       ~IFX_USBFS_POWER_CTL_DP_UP_EN);
	sys_clear_bits(config->base + IFX_USBFS_CR0, IFX_USBFS_CR0_USB_ENABLE);

	return 0;
}

#ifdef CONFIG_UDC_INFINEON_USBFS_DMA
static int udc_ifx_usbfs_dma_trigger_init(const struct device *dev)
{
	const struct udc_ifx_usbfs_config *config = dev->config;

	for (uint8_t i = 0U; i < config->num_dma_eps; i++) {
		if ((Cy_TrigMux_Select(USBFS_TRIG_DMA_REQ(i), false,
				       TRIGGER_TYPE_USB_DMA_REQ) != CY_TRIGMUX_SUCCESS) ||
		    (Cy_TrigMux_Select(USBFS_TRIG_BURSTEND(i), false,
				       TRIGGER_TYPE_USB_DMA_BURSTEND) != CY_TRIGMUX_SUCCESS)) {
			LOG_ERR("Failed to route DMA triggers for endpoint %u", i + 1U);
			return -EIO;
		}
	}

	return 0;
}
#endif

/* Power the block and PHY up, route its interrupts and leave every endpoint off */
static int usbfs_hw_init(const struct device *dev)
{
	const struct udc_ifx_usbfs_config *config = dev->config;
	struct udc_ifx_usbfs_data *priv = udc_get_private(dev);

	sys_write32(IFX_USBFS_USB_CLK_EN_CSR_CLK_EN, config->base + IFX_USBFS_USB_CLK_EN);

	/* IOMODE 0 hands the pins to the USB block */
	sys_write32(sys_read32(config->base + IFX_USBFS_USBIO_CR1) & IFX_USBFS_USBIO_CR1_RESERVED_2,
		    config->base + IFX_USBFS_USBIO_CR1);
	sys_write32(USBFS_BUS_RST_PERIOD, config->base + IFX_USBFS_BUS_RST_CNT);

	/* Enable the PHY and its receivers, passing through suspend */
	usbfs_wr_flush(dev, IFX_USBFS_POWER_CTL,
		       IFX_USBFS_POWER_CTL_SUSPEND | IFX_USBFS_POWER_CTL_ENABLE_DPO |
		       IFX_USBFS_POWER_CTL_ENABLE_DMO);
	k_busy_wait(USBFS_SUSPEND_US);
	usbfs_wr_flush(dev, IFX_USBFS_POWER_CTL,
		       sys_read32(config->base + IFX_USBFS_POWER_CTL) &
		       ~IFX_USBFS_POWER_CTL_SUSPEND);

	/* Lock the IMO to USB traffic */
	sys_write32(IFX_USBFS_CR1_ENABLE_LOCK |
		    (sys_read32(config->base + IFX_USBFS_CR1) & IFX_USBFS_CR1_RESERVED_3),
		    config->base + IFX_USBFS_CR1);

	sys_write32(USBFS_INTR_LVL_SEL, config->base + IFX_USBFS_INTR_LVL_SEL);
	sys_write32(IFX_USBFS_INTR_SIE_BUS_RESET | IFX_USBFS_INTR_SIE_EP0,
		    config->base + IFX_USBFS_INTR_SIE_MASK);
	sys_write32(0U, config->base + IFX_USBFS_LPM_CTL);

	sys_write32(FIELD_PREP(IFX_USBFS_ARB_CFG_DMA_CFG,
			       IS_ENABLED(CONFIG_UDC_INFINEON_USBFS_DMA) ?
				       IFX_USBFS_ARB_CFG_DMA_CFG_MANUAL :
				       IFX_USBFS_ARB_CFG_DMA_CFG_NONE),
		    config->base + IFX_USBFS_ARB_CFG);

#if defined(CONFIG_UDC_INFINEON_USBFS_DMA)
	int err = usbfs_dma_init(dev);

	if (err != 0) {
		return err;
	}
#endif

	priv->slots_used = 0U;
	sys_write32(0U, config->base + IFX_USBFS_SIE_EP_INT_EN);

	for (uint32_t ep = 1U; ep <= USBFS_NUM_DATA_EPS; ep++) {
		usbfs_sie_mode_set(dev, ep, IFX_USBFS_MODE_DISABLE);
		priv->hw_ep[ep - 1U].state = USBFS_EP_DISABLED;
	}

	return 0;
}

static void usbfs_hw_deinit(const struct device *dev)
{
	const struct udc_ifx_usbfs_config *config = dev->config;
	struct udc_ifx_usbfs_data *priv = udc_get_private(dev);

	sys_write32(0U, config->base + IFX_USBFS_POWER_CTL);
	sys_write32(0U, config->base + IFX_USBFS_USBIO_CTL);
	sys_write32(0U, config->base + IFX_USBFS_FLOW_CTL);
	sys_write32(0U, config->base + IFX_USBFS_LPM_CTL);
	sys_write32(0U, config->base + IFX_USBFS_INTR_SIE);
	sys_write32(0U, config->base + IFX_USBFS_INTR_SIE_SET);
	sys_write32(0U, config->base + IFX_USBFS_INTR_SIE_MASK);
	sys_write32(0U, config->base + IFX_USBFS_INTR_LVL_SEL);

	sys_write32(0U, config->base + IFX_USBFS_CR0);
	sys_write32(sys_read32(config->base + IFX_USBFS_CR1) & IFX_USBFS_CR1_RESERVED_3,
		    config->base + IFX_USBFS_CR1);
	sys_write32(0U, config->base + IFX_USBFS_USBIO_CR0);
	sys_write32((sys_read32(config->base + IFX_USBFS_USBIO_CR1) &
		     IFX_USBFS_USBIO_CR1_RESERVED_2) | IFX_USBFS_USBIO_CR1_IOMODE,
		    config->base + IFX_USBFS_USBIO_CR1);
	sys_write32(sys_read32(config->base + IFX_USBFS_USBIO_CR2) & IFX_USBFS_USBIO_CR2_RESERVED_7,
		    config->base + IFX_USBFS_USBIO_CR2);

	sys_write32(USBFS_BUS_RST_PERIOD, config->base + IFX_USBFS_BUS_RST_CNT);
	sys_write32(0U, config->base + IFX_USBFS_USB_CLK_EN);

	sys_write32(0U, config->base + IFX_USBFS_EP0_CR);
	sys_write32(0U, config->base + IFX_USBFS_EP0_CNT);

	sys_write32(0U, config->base + IFX_USBFS_ARB_CFG);
	sys_write32(0U, config->base + IFX_USBFS_ARB_INT_EN);
	sys_write32(0U, config->base + IFX_USBFS_DYN_RECONFIG);
	sys_write32(0U, config->base + IFX_USBFS_BUF_SIZE);
	sys_write32(0U, config->base + IFX_USBFS_DMA_THRES16);
	sys_write32(0U, config->base + IFX_USBFS_EP_ACTIVE);
	sys_write32(0U, config->base + IFX_USBFS_EP_TYPE);
	sys_write32(0U, config->base + IFX_USBFS_CWA16);

	sys_write32(0U, config->base + IFX_USBFS_SIE_EP_INT_EN);
	sys_write32(0U, config->base + IFX_USBFS_SIE_EP_INT_SR);

	for (uint32_t ep = 1U; ep <= USBFS_NUM_DATA_EPS; ep++) {
		sys_write32(0U, config->base + IFX_USBFS_SIE_EP_CR0(ep));
		sys_write32(0U, config->base + IFX_USBFS_SIE_EP_CNT0(ep));
		sys_write32(0U, config->base + IFX_USBFS_SIE_EP_CNT1(ep));
		sys_write32(0U, config->base + IFX_USBFS_ARB_EP_CFG(ep));
		sys_write32(0U, config->base + IFX_USBFS_ARB_EP_INT_EN(ep));
		sys_write32(0U, config->base + IFX_USBFS_ARB_RW_WA16(ep));
		sys_write32(0U, config->base + IFX_USBFS_ARB_RW_RA16(ep));
		priv->hw_ep[ep - 1U] = (struct udc_ifx_usbfs_hw_ep){0};
	}
}

static int udc_ifx_usbfs_init(const struct device *dev)
{
	const struct udc_ifx_usbfs_config *config = dev->config;
	int err;

#ifdef CONFIG_UDC_INFINEON_USBFS_DMA
	err = udc_ifx_usbfs_dma_trigger_init(dev);
	if (err != 0) {
		return err;
	}
#endif

	err = udc_ifx_usbfs_brs_clock_init(dev);
	if (err != 0) {
		LOG_ERR("Failed to configure the bus reset clock (%d)", err);
		return err;
	}

	err = usbfs_hw_init(dev);
	if (err != 0) {
		LOG_ERR("Failed to initialize the controller (%d)", err);
		return err;
	}

	config->irq_connect();
	config->irq_enable_func();

	/* The stack never enables the control endpoints itself. */
	err = udc_ep_enable_internal(dev, USB_CONTROL_EP_OUT, USB_EP_TYPE_CONTROL,
				     USBFS_EP0_MPS, 0);
	if (err != 0) {
		LOG_ERR("Failed to enable control OUT endpoint (%d)", err);
		return err;
	}

	err = udc_ep_enable_internal(dev, USB_CONTROL_EP_IN, USB_EP_TYPE_CONTROL,
				     USBFS_EP0_MPS, 0);
	if (err != 0) {
		LOG_ERR("Failed to enable control IN endpoint (%d)", err);
		return err;
	}

	return 0;
}

static int udc_ifx_usbfs_shutdown(const struct device *dev)
{
	const struct udc_ifx_usbfs_config *config = dev->config;

	(void)udc_ep_disable_internal(dev, USB_CONTROL_EP_OUT);
	(void)udc_ep_disable_internal(dev, USB_CONTROL_EP_IN);

	config->irq_disable_func();
	usbfs_hw_deinit(dev);

	return 0;
}

static void udc_ifx_usbfs_lock(const struct device *dev)
{
	k_sched_lock();
	udc_lock_internal(dev, K_FOREVER);
}

static void udc_ifx_usbfs_unlock(const struct device *dev)
{
	udc_unlock_internal(dev);
	k_sched_unlock();
}

static ALWAYS_INLINE void udc_ifx_usbfs_thread_handler(const struct device *dev)
{
	struct udc_ifx_usbfs_data *priv = udc_get_private(dev);
	uint32_t evt;

	evt = k_event_wait(&priv->events, UINT32_MAX, false, K_FOREVER);

	if (evt & BIT(USBFS_EVT_XFER_FINISHED)) {
		uint32_t eps;

		k_event_clear(&priv->events, BIT(USBFS_EVT_XFER_FINISHED));
		eps = atomic_clear(&priv->xfer_finished);

		while (eps != 0U) {
			unsigned int bnum = find_lsb_set(eps) - 1;
			uint8_t ep = (bnum >= 16U) ? (USB_EP_DIR_IN | (bnum - 16U)) :
						     (USB_EP_DIR_OUT | bnum);
			struct udc_ep_config *cfg = udc_get_ep_cfg(dev, ep);
			struct net_buf *buf;

			eps &= ~BIT(bnum);

			while (atomic_get(&priv->xfer_done[bnum]) > 0) {
				atomic_dec(&priv->xfer_done[bnum]);

				if (ep == USB_CONTROL_EP_OUT) {
					buf = ep0_out_claim(dev, true);
				} else {
					buf = udc_buf_get(cfg);
				}

				udc_ep_set_busy(cfg, false);

				if (buf != NULL) {
					udc_submit_ep_event(dev, buf, 0);
				}
			}

			if (USB_EP_GET_IDX(ep) != 0U) {
				ep_data_start(dev, cfg);
			}
		}
	}

	if (evt & BIT(USBFS_EVT_SETUP)) {
		k_event_clear(&priv->events, BIT(USBFS_EVT_SETUP));
		udc_setup_received(dev, &priv->setup);
	}

}

static int udc_ifx_usbfs_driver_preinit(const struct device *dev)
{
	const struct udc_ifx_usbfs_config *config = dev->config;
	struct udc_data *data = dev->data;
	struct udc_ifx_usbfs_data *priv = data->priv;
	int err;

	k_mutex_init(&data->mutex);
	k_event_init(&priv->events);

	for (uint8_t i = 0U; i <= USBFS_NUM_DATA_EPS; i++) {
		for (uint8_t d = 0U; d < 2U; d++) {
			const bool in = (d != 0U);
			struct udc_ep_config *cfg = in ? &config->ep_cfg_in[i] :
							 &config->ep_cfg_out[i];

			/* Numbers owned by the other direction, or with no DMA channel,
			 * get no caps.
			 */
			if ((i == 0U) ||
			    ((ep_num_is_in(i) == in) &&
			     (!IS_ENABLED(CONFIG_UDC_INFINEON_USBFS_DMA) ||
			      (i <= config->num_dma_eps)))) {
				cfg->caps.in = in;
				cfg->caps.out = !in;
				cfg->caps.control = (i == 0U);
				cfg->caps.bulk = (i != 0U);
				cfg->caps.interrupt = (i != 0U);
				cfg->caps.iso = (i != 0U);
				cfg->caps.mps = (i == 0U) ? USBFS_EP0_MPS : USBFS_EP_SLOT_SIZE;
			}

			cfg->addr = (in ? USB_EP_DIR_IN : USB_EP_DIR_OUT) | i;
			err = udc_register_ep(dev, cfg);
			if (err != 0) {
				LOG_ERR("Failed to register endpoint");
				return err;
			}
		}
	}

	data->caps.rwup = false;
	data->caps.mps0 = UDC_MPS0_8;
	/* The address is applied when the status stage completes */
	data->caps.addr_before_status = true;

	config->make_thread(dev);

	return 0;
}

static const struct udc_api udc_ifx_usbfs_api = {
	.ep_enqueue = udc_ifx_usbfs_ep_enqueue,
	.ep_dequeue = udc_ifx_usbfs_ep_dequeue,
	.ep_set_halt = udc_ifx_usbfs_ep_set_halt,
	.ep_clear_halt = udc_ifx_usbfs_ep_clear_halt,
	.ep_try_config = NULL,
	.ep_enable = udc_ifx_usbfs_ep_enable,
	.ep_disable = udc_ifx_usbfs_ep_disable,
	.host_wakeup = udc_ifx_usbfs_host_wakeup,
	.set_address = udc_ifx_usbfs_set_address,
	.enable = udc_ifx_usbfs_enable,
	.disable = udc_ifx_usbfs_disable,
	.init = udc_ifx_usbfs_init,
	.shutdown = udc_ifx_usbfs_shutdown,
	.lock = udc_ifx_usbfs_lock,
	.unlock = udc_ifx_usbfs_unlock,
};

#ifdef CONFIG_UDC_INFINEON_USBFS_DMA
#define UDC_IFX_USBFS_DMA_EP_DEFINE(i, n)						\
	static cy_stc_dma_descriptor_t __aligned(32) udc_ifx_usbfs_descr_##n##_##i[2];	\
											\
	BUILD_ASSERT(DT_INST_DMAS_CELL_BY_IDX(n, i, channel) == USBFS_DMA_CHANNEL(i),	\
		     "Data endpoint N is wired to DW0 channel 7 + N");

#define UDC_IFX_USBFS_DMA_EP_CFG(i, n)							\
	{										\
		.dw = (DW_Type *)DT_REG_ADDR(DT_INST_DMAS_CTLR_BY_IDX(n, i)),		\
		.ch = DT_INST_DMAS_CELL_BY_IDX(n, i, channel),				\
		.descr = {								\
			&udc_ifx_usbfs_descr_##n##_##i[0],				\
			&udc_ifx_usbfs_descr_##n##_##i[1],				\
		},									\
	}

#define UDC_IFX_USBFS_DMA_DEFINE(n)							\
	LISTIFY(DT_INST_PROP_LEN(n, dmas), UDC_IFX_USBFS_DMA_EP_DEFINE, (), n)		\
	static const struct udc_ifx_usbfs_dma_cfg udc_ifx_usbfs_dma_cfg_##n[] = {	\
		LISTIFY(DT_INST_PROP_LEN(n, dmas), UDC_IFX_USBFS_DMA_EP_CFG, (,), n)	\
	};

#define UDC_IFX_USBFS_DMA_CFG_INIT(n)							\
	.dma_cfg = udc_ifx_usbfs_dma_cfg_##n,						\
	.num_dma_eps = DT_INST_PROP_LEN(n, dmas),
#else
#define UDC_IFX_USBFS_DMA_DEFINE(n)
#define UDC_IFX_USBFS_DMA_CFG_INIT(n)
#endif

#define UDC_IFX_USBFS_DEVICE_DEFINE(n)							\
	K_THREAD_STACK_DEFINE(udc_ifx_usbfs_stack_##n,					\
			      CONFIG_UDC_INFINEON_USBFS_STACK_SIZE);			\
											\
	static void udc_ifx_usbfs_thread_##n(void *dev, void *arg1, void *arg2)		\
	{										\
		ARG_UNUSED(arg1);							\
		ARG_UNUSED(arg2);							\
											\
		while (true) {								\
			udc_ifx_usbfs_thread_handler(dev);				\
		}									\
	}										\
											\
	static void udc_ifx_usbfs_make_thread_##n(const struct device *dev)		\
	{										\
		struct udc_ifx_usbfs_data *priv = udc_get_private(dev);			\
											\
		k_thread_create(&priv->thread_data,					\
				udc_ifx_usbfs_stack_##n,				\
				K_THREAD_STACK_SIZEOF(udc_ifx_usbfs_stack_##n),		\
				udc_ifx_usbfs_thread_##n,				\
				(void *)dev, NULL, NULL,				\
				K_PRIO_COOP(CONFIG_UDC_INFINEON_USBFS_THREAD_PRIORITY),	\
				K_ESSENTIAL,						\
				K_NO_WAIT);						\
		k_thread_name_set(&priv->thread_data, dev->name);			\
	}										\
											\
	static void udc_ifx_usbfs_irq_connect_##n(void)					\
	{										\
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, lo, irq),				\
			    DT_INST_IRQ_BY_NAME(n, lo, priority),			\
			    udc_ifx_usbfs_isr_lo, DEVICE_DT_INST_GET(n), 0);		\
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, med, irq),				\
			    DT_INST_IRQ_BY_NAME(n, med, priority),			\
			    udc_ifx_usbfs_isr_med, DEVICE_DT_INST_GET(n), 0);		\
	}										\
											\
	static void udc_ifx_usbfs_irq_enable_##n(void)					\
	{										\
		irq_enable(DT_INST_IRQ_BY_NAME(n, lo, irq));				\
		irq_enable(DT_INST_IRQ_BY_NAME(n, med, irq));				\
	}										\
											\
	static void udc_ifx_usbfs_irq_disable_##n(void)					\
	{										\
		irq_disable(DT_INST_IRQ_BY_NAME(n, lo, irq));				\
		irq_disable(DT_INST_IRQ_BY_NAME(n, med, irq));				\
	}										\
											\
	static struct udc_ep_config							\
		ep_cfg_out_##n[DT_INST_PROP(n, num_bidir_endpoints)];			\
	static struct udc_ep_config							\
		ep_cfg_in_##n[DT_INST_PROP(n, num_bidir_endpoints)];			\
											\
	UDC_IFX_USBFS_DMA_DEFINE(n)							\
											\
	static const struct udc_ifx_usbfs_config udc_ifx_usbfs_config_##n = {		\
		.base = (mem_addr_t)DT_INST_REG_ADDR(n),				\
		.clk_dst = (en_clk_dst_t)DT_INST_PROP(n, clk_dst),			\
		.brs_div_type = (cy_en_divider_types_t)DT_INST_PROP(n, brs_div_type),	\
		.brs_div_num = DT_INST_PROP(n, brs_div_num),				\
		UDC_IFX_USBFS_DMA_CFG_INIT(n)						\
		.ep_cfg_in = ep_cfg_in_##n,						\
		.ep_cfg_out = ep_cfg_out_##n,						\
		.irq_connect = udc_ifx_usbfs_irq_connect_##n,				\
		.irq_enable_func = udc_ifx_usbfs_irq_enable_##n,			\
		.irq_disable_func = udc_ifx_usbfs_irq_disable_##n,			\
		.make_thread = udc_ifx_usbfs_make_thread_##n,				\
	};										\
											\
	static struct udc_ifx_usbfs_data udc_priv_##n;					\
											\
	static struct udc_data udc_data_##n = {						\
		.mutex = Z_MUTEX_INITIALIZER(udc_data_##n.mutex),			\
		.priv = &udc_priv_##n,							\
	};										\
											\
	DEVICE_DT_INST_DEFINE(n, udc_ifx_usbfs_driver_preinit, NULL,			\
			      &udc_data_##n, &udc_ifx_usbfs_config_##n,			\
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			      &udc_ifx_usbfs_api);

DT_INST_FOREACH_STATUS_OKAY(UDC_IFX_USBFS_DEVICE_DEFINE)

BUILD_ASSERT(DT_INST_PROP(0, num_bidir_endpoints) == USBFS_NUM_DATA_EPS + 1U,
	     "MXUSBFS provides USBFS_NUM_DATA_EPS data endpoints plus endpoint 0");

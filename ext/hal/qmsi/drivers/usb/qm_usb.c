/*
 * Copyright (c) 2016, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL CORPORATION OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "qm_usb.h"
#include "clk.h"
#include <string.h>

/** USB registers masks */
#define QM_USB_GRSTCTL_AHB_IDLE BIT(31)
#define QM_USB_GRSTCTL_TX_FNUM_OFFSET (6)
#define QM_USB_GRSTCTL_TX_FFLSH BIT(5)
#define QM_USB_GRSTCTL_C_SFT_RST BIT(0)
#define QM_USB_GAHBCFG_DMA_EN BIT(5)
#define QM_USB_GAHBCFG_GLB_INTR_MASK BIT(0)
#define QM_USB_DCTL_SFT_DISCON BIT(1)
#define QM_USB_GINTSTS_WK_UP_INT BIT(31)
#define QM_USB_GINTSTS_OEP_INT BIT(19)
#define QM_USB_GINTSTS_IEP_INT BIT(18)
#define QM_USB_GINTSTS_ENUM_DONE BIT(13)
#define QM_USB_GINTSTS_USB_RST BIT(12)
#define QM_USB_GINTSTS_USB_SUSP BIT(11)
#define QM_USB_GINTSTS_RX_FLVL BIT(4)
#define QM_USB_GINTSTS_OTG_INT BIT(2)
#define QM_USB_DCFG_DEV_SPD_LS (0x2)
#define QM_USB_DCFG_DEV_SPD_FS (0x3)
#define QM_USB_DCFG_DEV_ADDR_MASK (0x7F << 4)
#define QM_USB_DCFG_DEV_ADDR_OFFSET (4)
#define QM_USB_DAINT_IN_EP_INT(ep) (1 << (ep))
#define QM_USB_DAINT_OUT_EP_INT(ep) (0x10000 << (ep))
#define QM_USB_DEPCTL_EP_ENA BIT(31)
#define QM_USB_DEPCTL_EP_DIS BIT(30)
#define QM_USB_DEPCTL_SETDOPID BIT(28)
#define QM_USB_DEPCTL_SNAK BIT(27)
#define QM_USB_DEPCTL_CNAK BIT(26)
#define QM_USB_DEPCTL_STALL BIT(21)
#define QM_USB_DEPCTL_EP_TYPE_MASK (0x3 << 18)
#define QM_USB_DEPCTL_EP_TYPE_OFFSET (18)
#define QM_USB_DEPCTL_EP_TYPE_CONTROL (0)
#define QM_USB_DEPCTL_EP_TYPE_ISO (0x1)
#define QM_USB_DEPCTL_EP_TYPE_BULK (0x2)
#define QM_USB_DEPCTL_EP_TYPE_INTERRUPT (0x3)
#define QM_USB_DEPCTL_USB_ACT_EP BIT(15)
#define QM_USB_DEPCTL0_MSP_MASK (0x3)
#define QM_USB_DEPCTL0_MSP_8 (0x3)
#define QM_USB_DEPCTL0_MSP_16 (0x2)
#define QM_USB_DEPCTL0_MSP_32 (0x1)
#define QM_USB_DEPCTL0_MSP_64 (0)
#define QM_USB_DEPCTLn_MSP_MASK (0x3FF)
#define QM_USB_DEPCTL_MSP_OFFSET (0)
#define QM_USB_DOEPTSIZ_SUP_CNT_MASK (0x3 << 29)
#define QM_USB_DOEPTSIZ_SUP_CNT_OFFSET (29)
#define QM_USB_DOEPTSIZ0_PKT_CNT_MASK (0x1 << 19)
#define QM_USB_DOEPTSIZn_PKT_CNT_MASK (0x3FF << 19)
#define QM_USB_DIEPTSIZ0_PKT_CNT_MASK (0x3 << 19)
#define QM_USB_DIEPTSIZn_PKT_CNT_MASK (0x3FF << 19)
#define QM_USB_DEPTSIZ_PKT_CNT_OFFSET (19)
#define QM_USB_DEPTSIZ0_XFER_SIZE_MASK (0x7F)
#define QM_USB_DEPTSIZn_XFER_SIZE_MASK (0x7FFFF)
#define QM_USB_DEPTSIZ_XFER_SIZE_OFFSET (0)
#define QM_USB_DIEPINT_XFER_COMPL BIT(0)
#define QM_USB_DIEPINT_TX_FEMP BIT(7)
#define QM_USB_DIEPINT_XFER_COMPL BIT(0)
#define QM_USB_DOEPINT_SET_UP BIT(3)
#define QM_USB_DOEPINT_XFER_COMPL BIT(0)
#define QM_USB_DSTS_ENUM_SPD_MASK (0x3)
#define QM_USB_DSTS_ENUM_SPD_OFFSET (1)
#define QM_USB_DSTS_ENUM_LS (2)
#define QM_USB_DSTS_ENUM_FS (3)
#define QM_USB_GRXSTSR_EP_NUM_MASK (0xF << 0)
#define QM_USB_GRXSTSR_PKT_STS_MASK (0xF << 17)
#define QM_USB_GRXSTSR_PKT_STS_OFFSET (17)
#define QM_USB_GRXSTSR_PKT_CNT_MASK (0x7FF << 4)
#define QM_USB_GRXSTSR_PKT_CNT_OFFSET (4)
#define QM_USB_GRXSTSR_PKT_STS_OUT_DATA (2)
#define QM_USB_GRXSTSR_PKT_STS_OUT_DATA_DONE (3)
#define QM_USB_GRXSTSR_PKT_STS_SETUP_DONE (4)
#define QM_USB_GRXSTSR_PKT_STS_SETUP (6)
#define QM_USB_DTXFSTS_TXF_SPC_AVAIL_MASK (0xFFFF)

#define QM_USB_CORE_RST_TIMEOUT_US (10000)
#define QM_USB_PLL_TIMEOUT_US (100)

/** Check if an Endpoint is of direction IN. */
#define IS_IN_EP(ep) (ep < QM_USB_IN_EP_NUM)

/** Number of SETUP back-to-back packets */
#define QM_USB_SUP_CNT (1)

#if (UNIT_TEST)
#define QM_USB_EP_FIFO(ep) (ep)
#else
#define QM_USB_EP_FIFO(ep) (*(uint32_t *)(QM_USB_0_BASE + 0x1000 * (ep + 1)))
#endif

/*
 * Endpoint instance data.
 */
typedef struct {
	bool enabled;
	uint32_t data_len;
	const qm_usb_ep_config_t *ep_config;
} usb_ep_priv_t;

/*
 * USB controller instance data.
 */
typedef struct {
	qm_usb_status_callback_t status_callback;
	usb_ep_priv_t ep_ctrl[QM_USB_IN_EP_NUM + QM_USB_OUT_EP_NUM];
	bool attached;
	void *user_data;
} usb_priv_t;

static usb_priv_t usb_ctrl[QM_USB_NUM];

static bool usb_dc_ep_is_valid(const qm_usb_ep_idx_t ep)
{
	return (ep < QM_USB_IN_EP_NUM + QM_USB_OUT_EP_NUM);
}

static int usb_dc_reset(const qm_usb_t usb)
{
	uint32_t cnt = 0;

	/* Wait for AHB master idle state. */
	while (!(QM_USB[usb].grstctl & QM_USB_GRSTCTL_AHB_IDLE)) {
		clk_sys_udelay(1);
		if (++cnt > QM_USB_CORE_RST_TIMEOUT_US) {
			return -EIO;
		}
	}

	/* Core Soft Reset */
	cnt = 0;
	QM_USB[usb].grstctl |= QM_USB_GRSTCTL_C_SFT_RST;
	do {
		if (++cnt > QM_USB_CORE_RST_TIMEOUT_US) {
			return -EIO;
		}
		clk_sys_udelay(1);
	} while (QM_USB[usb].grstctl & QM_USB_GRSTCTL_C_SFT_RST);

	/* Wait for 3 PHY Clocks */
	clk_sys_udelay(100);

	return 0;
}

static void usb_dc_prep_rx(const qm_usb_t usb, const qm_usb_ep_idx_t ep,
			   uint8_t setup)
{
	/* We know this is an OUT EP always. */
	const uint8_t ep_idx = ep - QM_USB_IN_EP_NUM;
	const uint16_t ep_mps =
	    usb_ctrl[usb].ep_ctrl[ep].ep_config->max_packet_size;

	/* Set max RX size to EP mps so we get an interrupt
	 * each time a packet is received
	 */

	QM_USB[usb].out_ep_reg[ep_idx].doeptsiz =
	    (QM_USB_SUP_CNT << QM_USB_DOEPTSIZ_SUP_CNT_OFFSET) |
	    (1 << QM_USB_DEPTSIZ_PKT_CNT_OFFSET) | ep_mps;

	/* Clear NAK and enable ep */
	if (!setup) {
		QM_USB[usb].out_ep_reg[ep_idx].doepctl |= QM_USB_DEPCTL_CNAK;
	}
	QM_USB[usb].out_ep_reg[ep_idx].doepctl |= QM_USB_DEPCTL_EP_ENA;
}

static int usb_dc_tx(const qm_usb_t usb, uint8_t ep, const uint8_t *const data,
		     uint32_t data_len)
{
	uint32_t max_xfer_size, max_pkt_cnt, pkt_cnt, avail_space;
	const uint16_t ep_mps =
	    usb_ctrl[usb].ep_ctrl[ep].ep_config->max_packet_size;
	uint32_t i;

	/* Check if FIFO space available */
	avail_space = QM_USB[usb].in_ep_reg[ep].dtxfsts &
		      QM_USB_DTXFSTS_TXF_SPC_AVAIL_MASK;
	avail_space *= 4;
	if (!avail_space) {
		return -EAGAIN;
	}

	if (data_len > avail_space)
		data_len = avail_space;

	if (data_len != 0) {
		/* Get max packet size and packet count for ep */
		if (ep == QM_USB_IN_EP_0) {
			max_pkt_cnt = QM_USB_DIEPTSIZ0_PKT_CNT_MASK >>
				      QM_USB_DEPTSIZ_PKT_CNT_OFFSET;
			max_xfer_size = QM_USB_DEPTSIZ0_XFER_SIZE_MASK >>
					QM_USB_DEPTSIZ_XFER_SIZE_OFFSET;
		} else {
			max_pkt_cnt = QM_USB_DIEPTSIZn_PKT_CNT_MASK >>
				      QM_USB_DEPTSIZ_PKT_CNT_OFFSET;
			max_xfer_size = QM_USB_DEPTSIZn_XFER_SIZE_MASK >>
					QM_USB_DEPTSIZ_XFER_SIZE_OFFSET;
		}

		/* Check if transfer len is too big */
		if (data_len > max_xfer_size) {
			data_len = max_xfer_size;
		}

		/*
		 * Program the transfer size and packet count as follows:
		 *
		 * transfer size = N * ep_maxpacket + short_packet
		 * pktcnt = N + (short_packet exist ? 1 : 0)
		 */
		pkt_cnt = (data_len + ep_mps - 1) / ep_mps;

		if (pkt_cnt > max_pkt_cnt) {
			pkt_cnt = max_pkt_cnt;
			data_len = pkt_cnt * ep_mps;
		}
	} else {
		/* Zero length packet */
		pkt_cnt = 1;
	}

	/* Set number of packets and transfer size */
	QM_USB[usb].in_ep_reg[ep].dieptsiz =
	    (pkt_cnt << QM_USB_DEPTSIZ_PKT_CNT_OFFSET) | data_len;

	/* Clear NAK and enable ep */
	QM_USB[usb].in_ep_reg[ep].diepctl |= QM_USB_DEPCTL_CNAK;
	QM_USB[usb].in_ep_reg[ep].diepctl |= QM_USB_DEPCTL_EP_ENA;

	/* Write data to FIFO */
	for (i = 0; i < data_len; i += 4)
		QM_USB_EP_FIFO(ep) = *(uint32_t *)(data + i);

	return data_len;
}

static void usb_dc_handle_reset(const qm_usb_t usb)
{
	if (usb_ctrl[usb].status_callback) {
		usb_ctrl[usb].status_callback(usb_ctrl[usb].user_data, 0,
					      QM_USB_RESET);
	}

	/* Set device address to zero after reset */
	QM_USB[usb].dcfg &= ~QM_USB_DCFG_DEV_ADDR_MASK;

	/* enable global EP interrupts */
	QM_USB[usb].doepmsk = 0;
	QM_USB[usb].gintmsk |= QM_USB_GINTSTS_RX_FLVL;
	QM_USB[usb].diepmsk |= QM_USB_DIEPINT_XFER_COMPL;
}

static void usb_dc_handle_enum_done(const qm_usb_t usb)
{
	if (usb_ctrl[usb].status_callback) {
		usb_ctrl[usb].status_callback(usb_ctrl[usb].user_data, 0,
					      QM_USB_CONNECTED);
	}
}

static __inline__ void handle_rx_fifo(const qm_usb_t usb)
{
	const uint32_t grxstsp = QM_USB[usb].grxstsp;

	const uint8_t ep_idx =
	    (grxstsp & QM_USB_GRXSTSR_EP_NUM_MASK) + QM_USB_IN_EP_NUM;
	const uint32_t status = (grxstsp & QM_USB_GRXSTSR_PKT_STS_MASK) >>
				QM_USB_GRXSTSR_PKT_STS_OFFSET;
	const uint32_t xfer_size = (grxstsp & QM_USB_GRXSTSR_PKT_CNT_MASK) >>
				   QM_USB_GRXSTSR_PKT_CNT_OFFSET;
	const qm_usb_ep_config_t *const ep_cfg =
	    usb_ctrl[usb].ep_ctrl[ep_idx].ep_config;

	void (*ep_cb)(void *data, int error, qm_usb_ep_idx_t ep,
		      qm_usb_ep_status_t cb_status) = ep_cfg->callback;

	usb_ctrl[usb].ep_ctrl[ep_idx].data_len = xfer_size;

	if (ep_cb) {
		const qm_usb_status_t usb_status =
		    (status == QM_USB_GRXSTSR_PKT_STS_SETUP)
			? QM_USB_EP_SETUP
			: QM_USB_EP_DATA_OUT;

		ep_cb(ep_cfg->callback_data, 0, ep_idx, usb_status);
	}
}

static __inline__ void handle_in_ep_intr(const qm_usb_t usb)
{
	uint32_t ep_intr_sts;
	uint8_t ep_idx;
	void (*ep_cb)(void *data, int error, qm_usb_ep_idx_t ep,
		      qm_usb_ep_status_t cb_status);

	for (ep_idx = 0; ep_idx < QM_USB_IN_EP_NUM; ep_idx++) {
		if (QM_USB[usb].daint & QM_USB_DAINT_IN_EP_INT(ep_idx)) {
			/* Read IN EP interrupt status. */
			ep_intr_sts = QM_USB[usb].in_ep_reg[ep_idx].diepint &
				      QM_USB[usb].diepmsk;

			/* Clear IN EP interrupts. */
			QM_USB[usb].in_ep_reg[ep_idx].diepint = ep_intr_sts;

			ep_cb =
			    usb_ctrl[usb].ep_ctrl[ep_idx].ep_config->callback;

			if (ep_intr_sts & QM_USB_DIEPINT_XFER_COMPL && ep_cb) {
				ep_cb(usb_ctrl[usb]
					  .ep_ctrl[ep_idx]
					  .ep_config->callback_data,
				      0, ep_idx, QM_USB_EP_DATA_IN);
			}
		}
	}

	/* Clear interrupt. */
	QM_USB[usb].gintsts = QM_USB_GINTSTS_IEP_INT;
}

static __inline__ void handle_out_ep_intr(const qm_usb_t usb)
{
	uint32_t ep_intr_sts;
	uint8_t ep_idx;

	/* No OUT interrupt expected in FIFO mode, just clear
	 * interrupts. */
	for (ep_idx = 0; ep_idx < QM_USB_OUT_EP_NUM; ep_idx++) {
		if (QM_USB[usb].daint & QM_USB_DAINT_OUT_EP_INT(ep_idx)) {
			/* Read OUT EP interrupt status. */
			ep_intr_sts = QM_USB[usb].out_ep_reg[ep_idx].doepint &
				      QM_USB[usb].doepmsk;

			/* Clear OUT EP interrupts. */
			QM_USB[usb].out_ep_reg[ep_idx].doepint = ep_intr_sts;
		}
	}

	/* Clear interrupt. */
	QM_USB[usb].gintsts = QM_USB_GINTSTS_OEP_INT;
}

/* USB ISR handler */
static void usb_dc_isr_handler(const qm_usb_t usb)
{
	uint32_t int_status;

	/*  Read interrupt status */
	while ((int_status = (QM_USB[usb].gintsts & QM_USB[usb].gintmsk))) {

		if (int_status & QM_USB_GINTSTS_USB_RST) {
			/* Clear interrupt. */
			QM_USB[usb].gintsts = QM_USB_GINTSTS_USB_RST;

			/* Reset detected */
			usb_dc_handle_reset(usb);
		}

		if (int_status & QM_USB_GINTSTS_ENUM_DONE) {
			/* Clear interrupt. */
			QM_USB[usb].gintsts = QM_USB_GINTSTS_ENUM_DONE;

			/* Enumeration done detected */
			usb_dc_handle_enum_done(usb);
		}

		if (int_status & QM_USB_GINTSTS_USB_SUSP) {
			/* Clear interrupt. */
			QM_USB[usb].gintsts = QM_USB_GINTSTS_USB_SUSP;

			if (usb_ctrl[usb].status_callback) {
				usb_ctrl[usb].status_callback(
				    usb_ctrl[usb].user_data, 0, QM_USB_SUSPEND);
			}
		}

		if (int_status & QM_USB_GINTSTS_WK_UP_INT) {
			/* Clear interrupt. */
			QM_USB[usb].gintsts = QM_USB_GINTSTS_WK_UP_INT;

			if (usb_ctrl[usb].status_callback) {
				usb_ctrl[usb].status_callback(
				    usb_ctrl[usb].user_data, 0, QM_USB_RESUME);
			}
		}

		if (int_status & QM_USB_GINTSTS_RX_FLVL) {
			/* Packet in RX FIFO. */
			handle_rx_fifo(usb);
		}

		if (int_status & QM_USB_GINTSTS_IEP_INT) {
			/* IN EP interrupt. */
			handle_in_ep_intr(usb);
		}

		if (int_status & QM_USB_GINTSTS_OEP_INT) {
			/* OUT EP interrupt. */
			handle_out_ep_intr(usb);
		}
	}
}

QM_ISR_DECLARE(qm_usb_0_isr)
{
	usb_dc_isr_handler(QM_USB_0);
	QM_ISR_EOI(QM_IRQ_USB_0_INT_VECTOR);
}

int qm_usb_attach(const qm_usb_t usb)
{
	QM_CHECK(usb < QM_USB_NUM, -EINVAL);

	uint8_t ep;

	if (usb_ctrl[usb].attached) {
		return 0;
	}

	clk_sys_usb_enable();

	const int ret = usb_dc_reset(usb);
	if (ret)
		return ret;

	/* Set device speed to FS */
	QM_USB[usb].dcfg |= QM_USB_DCFG_DEV_SPD_FS;

	/* No FIFO setup needed, the default values are used. */
	/* Set NAK for all OUT EPs. */
	for (ep = 0; ep < QM_USB_OUT_EP_NUM; ep++) {
		QM_USB[usb].out_ep_reg[ep].doepctl = QM_USB_DEPCTL_SNAK;
	}

	/* Enable global interrupts. */
	QM_USB[usb].gintmsk =
	    QM_USB_GINTSTS_OEP_INT | QM_USB_GINTSTS_IEP_INT |
	    QM_USB_GINTSTS_ENUM_DONE | QM_USB_GINTSTS_USB_RST |
	    QM_USB_GINTSTS_WK_UP_INT | QM_USB_GINTSTS_USB_SUSP;

	/* Enable global interrupt. */
	QM_USB[usb].gahbcfg |= QM_USB_GAHBCFG_GLB_INTR_MASK;

	/* Disable soft disconnect. */
	QM_USB[usb].dctl &= ~QM_USB_DCTL_SFT_DISCON;

	usb_ctrl[usb].attached = true;

	return 0;
}

int qm_usb_detach(const qm_usb_t usb)
{
	QM_CHECK(usb < QM_USB_NUM, -EINVAL);

	if (!usb_ctrl[usb].attached) {
		return 0;
	}

	clk_sys_usb_disable();

	/* Enable soft disconnect. */
	QM_USB[usb].dctl |= QM_USB_DCTL_SFT_DISCON;

	usb_ctrl[usb].attached = false;

	return 0;
}

int qm_usb_reset(const qm_usb_t usb)
{
	QM_CHECK(usb < QM_USB_NUM, -EINVAL);
	return usb_dc_reset(usb);
}

int qm_usb_set_address(const qm_usb_t usb, const uint8_t addr)
{
	QM_CHECK(usb < QM_USB_NUM, -EINVAL);
	QM_CHECK(addr <
		     (QM_USB_DCFG_DEV_ADDR_MASK >> QM_USB_DCFG_DEV_ADDR_OFFSET),
		 -EINVAL);

	QM_USB[usb].dcfg &= ~QM_USB_DCFG_DEV_ADDR_MASK;
	QM_USB[usb].dcfg |= addr << QM_USB_DCFG_DEV_ADDR_OFFSET;

	return 0;
}

int qm_usb_ep_set_config(const qm_usb_t usb,
			 const qm_usb_ep_config_t *const ep_cfg)
{
	QM_CHECK(usb < QM_USB_NUM, -EINVAL);
	QM_CHECK(usb_ctrl[usb].attached, -EINVAL);
	QM_CHECK(ep_cfg, -EINVAL);
	if (!usb_dc_ep_is_valid(ep_cfg->ep)) {
		return -EINVAL;
	}

	volatile uint32_t *p_depctl;
	const uint8_t ep_idx = ep_cfg->ep < QM_USB_IN_EP_NUM
				   ? ep_cfg->ep
				   : ep_cfg->ep - QM_USB_IN_EP_NUM;

	if (!IS_IN_EP(ep_cfg->ep)) {
		p_depctl = &QM_USB[usb].out_ep_reg[ep_idx].doepctl;
	} else {
		p_depctl = &QM_USB[usb].in_ep_reg[ep_idx].diepctl;
	}

	usb_ctrl[usb].ep_ctrl[ep_cfg->ep].ep_config = ep_cfg;

	if (!ep_idx) {
		/* Set max packet size for EP0 */
		*p_depctl &= ~QM_USB_DEPCTL0_MSP_MASK;
		switch (ep_cfg->max_packet_size) {
		case 8:
			*p_depctl |= QM_USB_DEPCTL0_MSP_8
				     << QM_USB_DEPCTL_MSP_OFFSET;
			break;
		case 16:
			*p_depctl |= QM_USB_DEPCTL0_MSP_16
				     << QM_USB_DEPCTL_MSP_OFFSET;
			break;
		case 32:
			*p_depctl |= QM_USB_DEPCTL0_MSP_32
				     << QM_USB_DEPCTL_MSP_OFFSET;
			break;
		case 64:
			*p_depctl |= QM_USB_DEPCTL0_MSP_64
				     << QM_USB_DEPCTL_MSP_OFFSET;
			break;
		default:
			return -EINVAL;
		}
		/* No need to set EP0 type */
	} else {
		/* Set max packet size for EP */
		if (ep_cfg->max_packet_size >
		    (QM_USB_DEPCTLn_MSP_MASK >> QM_USB_DEPCTL_MSP_OFFSET))
			return -EINVAL;

		*p_depctl &= ~QM_USB_DEPCTLn_MSP_MASK;
		*p_depctl |= ep_cfg->max_packet_size
			     << QM_USB_DEPCTL_MSP_OFFSET;

		/* Set endpoint type */
		*p_depctl &= ~QM_USB_DEPCTL_EP_TYPE_MASK;
		switch (ep_cfg->type) {
		case QM_USB_EP_CONTROL:
			*p_depctl |= QM_USB_DEPCTL_EP_TYPE_CONTROL
				     << QM_USB_DEPCTL_EP_TYPE_OFFSET;
			break;
		case QM_USB_EP_BULK:
			*p_depctl |= QM_USB_DEPCTL_EP_TYPE_BULK
				     << QM_USB_DEPCTL_EP_TYPE_OFFSET;
			break;
		case QM_USB_EP_INTERRUPT:
			*p_depctl |= QM_USB_DEPCTL_EP_TYPE_INTERRUPT
				     << QM_USB_DEPCTL_EP_TYPE_OFFSET;
			break;
		default:
			return -EINVAL;
		}

		/* sets the Endpoint Data PID to DATA0 */
		*p_depctl |= QM_USB_DEPCTL_SETDOPID;
	}

	return 0;
}

int qm_usb_ep_set_stall_state(const qm_usb_t usb, const qm_usb_ep_idx_t ep,
			      const bool is_stalled)
{
	QM_CHECK(usb < QM_USB_NUM, -EINVAL);
	QM_CHECK(usb_ctrl[usb].attached, -EINVAL);
	if (!usb_dc_ep_is_valid(ep)) {
		return -EINVAL;
	}

	const uint8_t ep_idx = IS_IN_EP(ep) ? ep : ep - QM_USB_IN_EP_NUM;

	if (!is_stalled && !ep_idx) {
		/* Not possible to clear stall for EP0 */
		return -EINVAL;
	}

	if (IS_IN_EP(ep)) {
		uint32_t reg = QM_USB[usb].in_ep_reg[ep_idx].diepctl;
		reg ^= (-is_stalled ^ reg) & QM_USB_DEPCTL_STALL;
		QM_USB[usb].in_ep_reg[ep_idx].diepctl = reg;

		return 0;
	}

	uint32_t reg = QM_USB[usb].out_ep_reg[ep_idx].doepctl;
	reg ^= (-is_stalled ^ reg) & QM_USB_DEPCTL_STALL;
	QM_USB[usb].out_ep_reg[ep_idx].doepctl = reg;

	return 0;
}

int qm_usb_ep_halt(const qm_usb_t usb, const qm_usb_ep_idx_t ep)
{
	QM_CHECK(usb < QM_USB_NUM, -EINVAL);
	QM_CHECK(usb_ctrl[usb].attached, -EINVAL);
	if (!usb_dc_ep_is_valid(ep)) {
		return -EINVAL;
	}

	const uint8_t ep_idx = IS_IN_EP(ep) ? ep : ep - QM_USB_IN_EP_NUM;
	volatile uint32_t *p_depctl;

	/* Cannot disable EP0, just set stall */
	if (!ep_idx) {
		return qm_usb_ep_set_stall_state(usb, ep, true);
	}

	if (!IS_IN_EP(ep)) {
		p_depctl = &QM_USB[usb].out_ep_reg[ep_idx].doepctl;
	} else {
		p_depctl = &QM_USB[usb].in_ep_reg[ep_idx].diepctl;
	}

	/* Set STALL and disable endpoint if enabled */
	*p_depctl |= QM_USB_DEPCTL_STALL;
	if (*p_depctl & QM_USB_DEPCTL_EP_ENA) {
		*p_depctl |= QM_USB_DEPCTL_EP_DIS;
	}

	return 0;
}

int qm_usb_ep_is_stalled(const qm_usb_t usb, const qm_usb_ep_idx_t ep,
			 bool *stalled)
{
	QM_CHECK(usb < QM_USB_NUM, -EINVAL);
	QM_CHECK(usb_ctrl[usb].attached, -EINVAL);
	QM_CHECK(stalled, -EINVAL);
	if (!usb_dc_ep_is_valid(ep)) {
		return -EINVAL;
	}

	volatile uint32_t *p_depctl;

	if (IS_IN_EP(ep)) {
		p_depctl = &QM_USB[usb].in_ep_reg[ep].diepctl;
	} else {
		const uint8_t ep_idx = ep - QM_USB_IN_EP_NUM;
		p_depctl = &QM_USB[usb].out_ep_reg[ep_idx].doepctl;
	}

	*stalled = !!(*p_depctl & QM_USB_DEPCTL_STALL);

	return 0;
}

int qm_usb_ep_enable(const qm_usb_t usb, const qm_usb_ep_idx_t ep)
{
	QM_CHECK(usb < QM_USB_NUM, -EINVAL);
	QM_CHECK(usb_ctrl[usb].attached, -EINVAL);
	if (!usb_dc_ep_is_valid(ep)) {
		return -EINVAL;
	}

	usb_ctrl[usb].ep_ctrl[ep].enabled = true;

	if (IS_IN_EP(ep)) {
		/* Enable EP interrupts. */
		QM_USB[usb].daintmsk |= QM_USB_DAINT_IN_EP_INT(ep);

		/* Activate EP. */
		QM_USB[usb].in_ep_reg[ep].diepctl |= QM_USB_DEPCTL_USB_ACT_EP;
	} else {
		const uint8_t ep_idx = ep - QM_USB_IN_EP_NUM;

		/* Enable EP interrupts. */
		QM_USB[usb].daintmsk |= QM_USB_DAINT_OUT_EP_INT(ep_idx);

		/* Activate EP. */
		QM_USB[usb].out_ep_reg[ep_idx].doepctl |=
		    QM_USB_DEPCTL_USB_ACT_EP;

		/* Prepare EP for rx. */
		usb_dc_prep_rx(usb, ep, 0);
	}

	return 0;
}

int qm_usb_ep_disable(const qm_usb_t usb, const qm_usb_ep_idx_t ep)
{
	QM_CHECK(usb < QM_USB_NUM, -EINVAL);
	if (!usb_dc_ep_is_valid(ep)) {
		return -EINVAL;
	}

	/* Disable EP intr then de-activate, disable and set NAK. */
	if (!IS_IN_EP(ep)) {
		const uint8_t ep_idx = ep - QM_USB_IN_EP_NUM;

		QM_USB[usb].daintmsk &= ~QM_USB_DAINT_OUT_EP_INT(ep_idx);
		QM_USB[usb].doepmsk &= ~QM_USB_DOEPINT_SET_UP;
		QM_USB[usb].out_ep_reg[ep_idx].doepctl &=
		    ~(QM_USB_DEPCTL_USB_ACT_EP | QM_USB_DEPCTL_EP_ENA |
		      QM_USB_DEPCTL_SNAK);
	} else {
		QM_USB[usb].daintmsk &= ~QM_USB_DAINT_IN_EP_INT(ep);
		QM_USB[usb].diepmsk &= ~QM_USB_DIEPINT_XFER_COMPL;
		QM_USB[usb].gintmsk &= ~QM_USB_GINTSTS_RX_FLVL;
		QM_USB[usb].in_ep_reg[ep].diepctl &=
		    ~(QM_USB_DEPCTL_USB_ACT_EP | QM_USB_DEPCTL_EP_ENA |
		      QM_USB_DEPCTL_SNAK);
	}

	usb_ctrl[usb].ep_ctrl[ep].enabled = false;

	return 0;
}

int qm_usb_ep_flush(const qm_usb_t usb, const qm_usb_ep_idx_t ep)
{
	QM_CHECK(usb < QM_USB_NUM, -EINVAL);
	QM_CHECK(usb_ctrl[usb].attached, -EINVAL);
	if (!usb_dc_ep_is_valid(ep)) {
		return -EINVAL;
	}
	/*
	 *
	 * RX FIFO is global and cannot be flushed per EP, but it can
	 * be through bit 4 from GRSTCTL. For now we don't flush it
	 * here since both FIFOs are always flushed during the Core
	 * Soft Reset done at usb_dc_reset(), which is called on both
	 * qm_usb_attach() and qm_usb_reset().
	 */
	if (!IS_IN_EP(ep)) {
		return -EINVAL;
	}

	/* Each IN endpoint has dedicated Tx FIFO. */
	QM_USB[usb].grstctl |= ep << QM_USB_GRSTCTL_TX_FNUM_OFFSET;
	QM_USB[usb].grstctl |= QM_USB_GRSTCTL_TX_FFLSH;

	uint32_t cnt = 0;
	do {
		if (++cnt > QM_USB_CORE_RST_TIMEOUT_US) {
			return -EIO;
		}
		clk_sys_udelay(1);
	} while (QM_USB[usb].grstctl & QM_USB_GRSTCTL_TX_FFLSH);

	return 0;
}

int qm_usb_ep_write(const qm_usb_t usb, const qm_usb_ep_idx_t ep,
		    const uint8_t *const data, const uint32_t data_len,
		    uint32_t *const ret_bytes)
{
	QM_CHECK(usb < QM_USB_NUM, -EINVAL);
	QM_CHECK(usb_ctrl[usb].attached, -EINVAL);
	if (!usb_dc_ep_is_valid(ep)) {
		return -EINVAL;
	}
	if (!IS_IN_EP(ep)) {
		return -EINVAL;
	}

	/* Check if IN EP is enabled */
	if (!usb_ctrl[usb].ep_ctrl[ep].enabled) {
		return -EINVAL;
	}

	const int ret = usb_dc_tx(usb, ep, data, data_len);
	if (ret < 0) {
		return ret;
	}

	if (ret_bytes) {
		*ret_bytes = ret;
	}

	return 0;
}

int qm_usb_ep_read(const qm_usb_t usb, const qm_usb_ep_idx_t ep,
		   uint8_t *const data, const uint32_t max_data_len,
		   uint32_t *const read_bytes)
{
	QM_CHECK(usb < QM_USB_NUM, -EINVAL);
	QM_CHECK(usb_ctrl[usb].attached, -EINVAL);
	QM_CHECK(data, -EINVAL);
	if (!usb_dc_ep_is_valid(ep)) {
		return -EINVAL;
	}
	if (IS_IN_EP(ep)) {
		return -EINVAL;
	}

	uint32_t i, j, data_len, bytes_to_copy;

	/* Check if OUT EP enabled */
	if (!usb_ctrl[usb].ep_ctrl[ep].enabled) {
		return -EINVAL;
	}

	data_len = usb_ctrl[usb].ep_ctrl[ep].data_len;

	if (data_len > max_data_len) {
		bytes_to_copy = max_data_len;
	} else {
		bytes_to_copy = data_len;
	}

	const uint8_t ep_idx = ep - QM_USB_IN_EP_NUM;
	/* Data in the FIFOs is always stored per 32-bit words. */
	for (i = 0; i < (bytes_to_copy & ~0x3); i += 4) {
		*(uint32_t *)(data + i) = QM_USB_EP_FIFO(ep_idx);
	}
	if (bytes_to_copy & 0x3) {
		/* Not multiple of 4. */
		uint32_t last_dw = QM_USB_EP_FIFO(ep_idx);

		for (j = 0; j < (bytes_to_copy & 0x3); j++) {
			*(data + i + j) = (last_dw >> (8 * j)) & 0xFF;
		}
	}

	usb_ctrl[usb].ep_ctrl[ep].data_len -= bytes_to_copy;

	if (read_bytes) {
		*read_bytes = bytes_to_copy;
	}

	/* Prepare ep for rx if all the data frames were read. */
	if (!usb_ctrl[usb].ep_ctrl[ep].data_len) {
		usb_dc_prep_rx(usb, ep, 0);
	}

	return 0;
}

int qm_usb_set_status_callback(const qm_usb_t usb,
			       const qm_usb_status_callback_t cb)
{
	QM_CHECK(usb < QM_USB_NUM, -EINVAL);

	usb_ctrl[usb].status_callback = cb;
	return 0;
}

int qm_usb_ep_get_bytes_read(const qm_usb_t usb, const qm_usb_ep_idx_t ep,
			     uint32_t *const read_bytes)
{
	QM_CHECK(usb < QM_USB_NUM, -EINVAL);
	QM_CHECK(usb_ctrl[usb].attached, -EINVAL);
	QM_CHECK(read_bytes, -EINVAL);
	if (!usb_dc_ep_is_valid(ep)) {
		return -EINVAL;
	}
	if (IS_IN_EP(ep)) {
		return -EINVAL;
	}

	/* Check if OUT EP enabled. */
	if (!usb_ctrl[usb].ep_ctrl[ep].enabled) {
		return -EINVAL;
	}

	*read_bytes = usb_ctrl[usb].ep_ctrl[ep].data_len;

	return 0;
}

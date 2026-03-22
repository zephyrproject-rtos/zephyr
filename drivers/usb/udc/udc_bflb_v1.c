/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "udc_common.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/util.h>

#include <bouffalolab/common/usb_v1_reg.h>
#include <bflb_soc.h>
#include <glb_reg.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(udc_bflb_v1, CONFIG_UDC_DRIVER_LOG_LEVEL);

#define DT_DRV_COMPAT bflb_udc_1

/* BL70x USB controller always has 8 bidirectional endpoints */
#define BFLB_USB_NUM_ENDPOINTS 8

/* EP type mapping from USB standard to BL70x hardware register values */
#define BFLB_EP_TYPE_INTERRUPT 0U
#define BFLB_EP_TYPE_ISO       2U
#define BFLB_EP_TYPE_BULK      4U

/* EP direction register values */
#define BFLB_EP_DIR_IN  1U
#define BFLB_EP_DIR_OUT 2U

/* Config register offset for EP1-7 (ep_idx must be 1..7) */
#define BFLB_EPX_CONFIG_OFFSET(ep_idx)                                          \
	(USB_EP1_CONFIG_OFFSET + 4U * ((ep_idx) - 1U))

/* Maximum packet size for non-control endpoints (USB FS isochronous limit) */
#define BFLB_EPX_MPS_MAX 1023U

/* EP0 maximum packet size (USB FS control endpoint) */
#define BFLB_EP0_MPS 64U

/* Events for ISR-to-thread communication */
#define UDC_BFLB_V1_EVT_SETUP   BIT(0)
#define UDC_BFLB_V1_EVT_XFER    BIT(1)
#define UDC_BFLB_V1_EVT_RESET   BIT(2)
#define UDC_BFLB_V1_EVT_NEWXFER BIT(3)

struct udc_bflb_v1_config {
	mm_reg_t base;
	const struct pinctrl_dev_config *pcfg;
	size_t num_of_eps;
	struct udc_ep_config *ep_cfg_in;
	struct udc_ep_config *ep_cfg_out;
	void (*make_thread)(const struct device *dev);
	void (*irq_enable_func)(const struct device *dev);
	void (*irq_disable_func)(const struct device *dev);
};

struct udc_bflb_v1_data {
	struct k_thread thread_data;
	struct k_event events;
	/* Bitmap: bits 31..16 = IN eps, bits 15..0 = OUT eps */
	atomic_t xfer_finished;
	atomic_t xfer_new;
	uint8_t setup[8];
	/* ISR-only: current control transfer direction and data presence */
	bool ep0_ctrl_in;   /* true if bmRequestType direction is IN */
	bool ep0_has_data;  /* true if wLength > 0 */
	bool setup_pending; /* true from SETUP_DONE until thread processes */
	/* Incremented by the ISR on every SETUP_DONE.  Snapshotted in
	 * handle_evt_setup as setup_gen_serviced.  ep_set_halt(EP0)
	 * compares the two to detect stale STALL requests from the
	 * USBD stack that arrive after a new SETUP has been processed.
	 */
	uint32_t setup_gen;
	uint32_t setup_gen_serviced;
	bool ep0_status_done;
};

/* Hardware handles PID sequencing, but UDC bookkeeping must stay in sync. */
static void bflb_set_ep_data1(const struct device *dev, const uint8_t ep,
			      const bool data1)
{
	struct udc_ep_config *const ep_cfg = udc_get_ep_cfg(dev, ep);

	if (ep_cfg != NULL) {
		ep_cfg->stat.data1 = data1;
	}
}

/* Read EP direction for EP1-7 from hardware config register */
static inline uint8_t epx_get_dir(const mm_reg_t base, const uint8_t ep_idx)
{
	uint32_t regval = sys_read32(base + BFLB_EPX_CONFIG_OFFSET(ep_idx));

	return (regval >> USB_CR_EP1_DIR_SHIFT) & 0x3U;
}

/* Get RX FIFO count for an endpoint */
static inline uint16_t ep_get_rxcount(const mm_reg_t base, const uint8_t ep_idx)
{
	uint32_t regval =
		sys_read32(base + USB_EP0_FIFO_STATUS_OFFSET + 0x10U * ep_idx);

	return (regval & USB_EP0_RX_FIFO_CNT_MASK) >> USB_EP0_RX_FIFO_CNT_SHIFT;
}

/* Write data to EP TX FIFO (byte-by-byte MMIO) */
static void fifo_write(const mm_reg_t base, const uint8_t ep_idx,
		       const uint8_t *data, const uint32_t len)
{
	const mm_reg_t fifo =
		base + USB_EP0_TX_FIFO_WDATA_OFFSET + ep_idx * 0x10U;

	for (uint32_t i = 0; i < len; i++) {
		sys_write8(data[i], fifo);
	}
}

/* Read data from EP RX FIFO (byte-by-byte MMIO) */
static void fifo_read(const mm_reg_t base, const uint8_t ep_idx, uint8_t *data,
		      const uint32_t len)
{
	const mm_reg_t fifo =
		base + USB_EP0_RX_FIFO_RDATA_OFFSET + ep_idx * 0x10U;

	for (uint32_t i = 0; i < len; i++) {
		data[i] = sys_read8(fifo);
	}
}

/* Clear EP TX FIFO — use direct write (not RMW) to ensure the
 * w1c clear bit is actually triggered.  Includes a readback
 * barrier to ensure the FIFO is actually empty before returning.
 */
static void fifo_tx_clear(const mm_reg_t base, const uint8_t ep_idx)
{
	const uint32_t cfg_addr =
		base + USB_EP0_FIFO_CONFIG_OFFSET + 0x10U * ep_idx;
	const uint32_t sts_addr =
		base + USB_EP0_FIFO_STATUS_OFFSET + 0x10U * ep_idx;

	/* Direct write: only set TX_FIFO_CLR, leave other bits at 0.
	 * DMA enable bits (0-1) are intentionally not preserved since
	 * we don't use DMA.
	 */
	sys_write32(USB_EP0_TX_FIFO_CLR, cfg_addr);

	/* Readback barrier: wait until TX FIFO reports empty */
	while (!(sys_read32(sts_addr) & USB_EP0_TX_FIFO_EMPTY)) {
	}
}

/* Clear EP RX FIFO — direct write, same pattern as fifo_tx_clear */
static void fifo_rx_clear(const mm_reg_t base, const uint8_t ep_idx)
{
	sys_write32(USB_EP0_RX_FIFO_CLR,
		    base + USB_EP0_FIFO_CONFIG_OFFSET + 0x10U * ep_idx);
}

/* Wait for EP busy flag to clear */
static inline void ep_wait_not_busy(const mm_reg_t base, const uint8_t ep_idx)
{
	if (ep_idx == 0) {
		while (sys_read32(base + USB_CONFIG_OFFSET) &
		       USB_STS_USB_EP0_SW_RDY) {
		}
	} else {
		mem_addr_t reg = base + BFLB_EPX_CONFIG_OFFSET(ep_idx);

		while (sys_read32(reg) & USB_STS_EP1_RDY) {
		}
	}
}

/*
 * Set EP0 to NAK both directions.
 * Must mask STS_RDY (bit 28, read-only status) before writing back —
 * writing it back corrupts the EP0 state machine (silence instead of NAK).
 *
 * NACK_OUT also blocks SETUP tokens unconditionally (hardware quirk).
 * It must be cleared before the next SETUP is expected — see
 * ep0_clear_nack_out() which is called from the ISR at the right time.
 */
static void ep0_set_nak(const mm_reg_t base)
{
	unsigned int key = irq_lock();
	uint32_t regval = sys_read32(base + USB_CONFIG_OFFSET);

	regval &= ~USB_STS_USB_EP0_SW_RDY;
	regval |= USB_CR_USB_EP0_SW_NACK_OUT | USB_CR_USB_EP0_SW_NACK_IN;
	sys_write32(regval, base + USB_CONFIG_OFFSET);
	irq_unlock(key);
}

/*
 * Set only NACK_IN (without touching NACK_OUT).
 * Used by the thread to prevent stale IN responses without blocking
 * SETUP tokens — the ISR manages NACK_OUT via ep0_set_nak/ep0_clear_nack_out.
 */
static void ep0_set_nack_in(const mm_reg_t base)
{
	unsigned int key = irq_lock();
	uint32_t regval = sys_read32(base + USB_CONFIG_OFFSET);

	regval &= ~USB_STS_USB_EP0_SW_RDY;
	regval |= USB_CR_USB_EP0_SW_NACK_IN;
	sys_write32(regval, base + USB_CONFIG_OFFSET);
	irq_unlock(key);
}

/*
 * Clear NACK_OUT to allow the next SETUP token to be accepted.
 * Called from ISR when a control transfer completes (status phase done).
 */
static void ep0_clear_nack_out(const mm_reg_t base)
{
	unsigned int key = irq_lock();
	uint32_t regval = sys_read32(base + USB_CONFIG_OFFSET);

	regval &= ~(USB_STS_USB_EP0_SW_RDY | USB_CR_USB_EP0_SW_NACK_OUT |
		    USB_CR_USB_EP0_SW_STALL);
	sys_write32(regval, base + USB_CONFIG_OFFSET);
	irq_unlock(key);
}

/*
 * Set EP0 ready for one IN or OUT data/status transaction.
 * SW_RDY is one-shot: allows exactly one packet through.
 */
static void ep0_set_ready(const mm_reg_t base)
{
	unsigned int key = irq_lock();
	uint32_t regval = sys_read32(base + USB_CONFIG_OFFSET);

	regval &= ~USB_STS_USB_EP0_SW_RDY;
	regval |= USB_CR_USB_EP0_SW_RDY;
	regval |= USB_CR_USB_EP0_SW_NACK_IN;
	regval |= USB_CR_USB_EP0_SW_NACK_OUT;
	regval &= ~USB_CR_USB_EP0_SW_STALL;
	/* Always set NACK_OUT when arming for IN/OUT packets.
	 * RDY overrides NACK for one packet, so this doesn't block
	 * the current transfer.  But it prevents the hardware from
	 * accepting unexpected OUT packets (e.g. status-OUT) before
	 * the thread is ready.  NACK_OUT is explicitly cleared by
	 * bflb_prep_ep0_rx() or by the ISR on transfer completion.
	 */
	sys_write32(regval, base + USB_CONFIG_OFFSET);
	irq_unlock(key);
}

/* Set EPx (1-7) to NAK */
static void epx_set_nak(const mm_reg_t base, const uint8_t ep_idx)
{
	uint32_t regval = sys_read32(base + BFLB_EPX_CONFIG_OFFSET(ep_idx));

	regval |= USB_CR_EP1_NACK;
	regval &= ~USB_CR_EP1_STALL;
	regval &= ~USB_CR_EP1_RDY;
	sys_write32(regval, base + BFLB_EPX_CONFIG_OFFSET(ep_idx));
}

/* Set EPx (1-7) ready */
static void epx_set_ready(const mm_reg_t base, const uint8_t ep_idx)
{
	uint32_t regval = sys_read32(base + BFLB_EPX_CONFIG_OFFSET(ep_idx));

	regval |= USB_CR_EP1_RDY;
	regval |= USB_CR_EP1_NACK;
	regval &= ~USB_CR_EP1_STALL;
	sys_write32(regval, base + BFLB_EPX_CONFIG_OFFSET(ep_idx));
}

/* Configure and power up the USB PHY transceiver */
static int xcvr_config(void)
{
	uint32_t regval;

	/* USB requires the 48MHz DLL output; verify DLL is powered up */
	regval = sys_read32(GLB_BASE + GLB_DLL_OFFSET);
	if (!(regval & GLB_PU_DLL_MSK)) {
		LOG_ERR("DLL not powered up, cannot start USB");
		return -ENODEV;
	}

	/* Enable 48MHz DLL output and USB clock gate */
	sys_set_bits(GLB_BASE + GLB_CLK_CFG1_OFFSET,
		     GLB_DLL_48M_DIV_EN_MSK | GLB_USB_CLK_EN_MSK);

	/* Power up USB PHY */
	regval = sys_read32(GLB_BASE + GLB_USB_XCVR_OFFSET);
	regval |= GLB_PU_USB_MSK;
	sys_write32(regval, GLB_BASE + GLB_USB_XCVR_OFFSET);

	/* Configure full-speed, output drivers */
	regval = sys_read32(GLB_BASE + GLB_USB_XCVR_OFFSET);
	regval &= ~GLB_USB_SUS_MSK;
	regval |= GLB_USB_SPD_MSK; /* Full-speed */
	regval &= ~GLB_USB_DATA_CONVERT_MSK;
	regval &= ~GLB_USB_OEB_SEL_MSK;
	regval &= ~GLB_USB_ROUT_PMOS_MSK;
	regval &= ~GLB_USB_ROUT_NMOS_MSK;
	regval |= (3U << GLB_USB_ROUT_PMOS_POS);
	regval |= (3U << GLB_USB_ROUT_NMOS_POS);
	sys_write32(regval, GLB_BASE + GLB_USB_XCVR_OFFSET);

	/* Configure XCVR analog parameters */
	regval = 0;
	regval |= (2U << GLB_USB_V_HYS_M_POS);
	regval |= (2U << GLB_USB_V_HYS_P_POS);
	regval |= (7U << GLB_USB_BD_VTH_POS);
	regval |= GLB_REG_USB_USE_XCVR_MSK;
	regval |= GLB_REG_USB_USE_CTRL_MSK;
	regval |= (0U << GLB_USB_STR_DRV_POS);
	regval |= (5U << GLB_USB_RES_PULLUP_TUNE_POS);
	regval |= (2U << GLB_USB_SLEWRATE_M_FALL_POS);
	regval |= (2U << GLB_USB_SLEWRATE_M_RISE_POS);
	regval |= (2U << GLB_USB_SLEWRATE_P_FALL_POS);
	regval |= (2U << GLB_USB_SLEWRATE_P_RISE_POS);
	sys_write32(regval, GLB_BASE + GLB_USB_XCVR_CONFIG_OFFSET);

	return 0;
}

/* Power down XCVR */
static void xcvr_shutdown(void)
{
	sys_clear_bits(GLB_BASE + GLB_USB_XCVR_OFFSET,
		       GLB_USB_ENUM_MSK | GLB_PU_USB_MSK);

	/* Disable USB clock gate and 48MHz DLL output */
	sys_clear_bits(GLB_BASE + GLB_CLK_CFG1_OFFSET,
		       GLB_DLL_48M_DIV_EN_MSK | GLB_USB_CLK_EN_MSK);
}

/*
 * Ensure USB PHY starts from a clean state after possible UF2 bootloader
 * activity. Without a short disconnect/power-cycle, some hosts never
 * re-enumerate the application USB device.
 */
static int xcvr_reset(void)
{
	/* Detach and power down transceiver. */
	xcvr_shutdown();
	/* Match SDK behavior: keep PHY off briefly before re-enabling. */
	k_msleep(10);
	/* Re-apply default PHY configuration and connect pull-up. */
	return xcvr_config();
}

static uint32_t get_intstatus(const mm_reg_t base)
{
	uint32_t sts = sys_read32(base + USB_INT_STS_OFFSET);
	uint32_t mask = sys_read32(base + USB_INT_MASK_OFFSET);
	uint32_t en = sys_read32(base + USB_INT_EN_OFFSET);

	return sts & ~mask & en;
}

/* Convert bitmap bit to endpoint address */
static inline uint8_t bflb_pull_ep_from_bmsk(uint32_t *bitmap)
{
	unsigned int bit;

	__ASSERT_NO_MSG(bitmap && *bitmap);

	bit = find_lsb_set(*bitmap) - 1;
	*bitmap &= ~BIT(bit);

	if (bit >= 16U) {
		return USB_EP_DIR_IN | (bit - 16U);
	}
	return USB_EP_DIR_OUT | bit;
}

static inline int ep_to_bnum(const uint8_t ep)
{
	if (USB_EP_DIR_IS_IN(ep)) {
		return 16U + USB_EP_GET_IDX(ep);
	}
	return USB_EP_GET_IDX(ep);
}

static void bflb_handle_usb_error(const struct device *dev, const mm_reg_t base,
				  const uint32_t err_sts)
{
	const uint32_t known_errs =
		USB_UTMI_RX_ERR | USB_XFER_TO_ERR | USB_IVLD_EP_ERR |
		USB_PID_SEQ_ERR | USB_PID_CKS_ERR | USB_CRC5_ERR | USB_CRC16_ERR;
	const uint32_t err = err_sts & known_errs;

	if (err == 0U) {
		sys_write32(err_sts, base + USB_ERROR_OFFSET);
		return;
	}

	LOG_DBG("USB error flags: 0x%08x%s%s%s%s%s%s%s", err,
		(err & USB_UTMI_RX_ERR) ? " UTMI_RX_ERR" : "",
		(err & USB_XFER_TO_ERR) ? " XFER_TO_ERR" : "",
		(err & USB_IVLD_EP_ERR) ? " IVLD_EP_ERR" : "",
		(err & USB_PID_SEQ_ERR) ? " PID_SEQ_ERR" : "",
		(err & USB_PID_CKS_ERR) ? " PID_CKS_ERR" : "",
		(err & USB_CRC5_ERR) ? " CRC5_ERR" : "",
		(err & USB_CRC16_ERR) ? " CRC16_ERR" : "");

	ARG_UNUSED(dev);
	sys_write32(err_sts, base + USB_ERROR_OFFSET);
}

/*
 * Prepare and start an IN transfer for EP0.
 * Writes data from net_buf to TX FIFO and sets EP0 ready.
 *
 * SIZE and RDY are set in a single atomic register write to prevent
 * an ISR from doing a concurrent RMW on USB_CONFIG that could restore
 * a stale SIZE value between the two operations.
 */
static void bflb_prep_ep0_tx(const struct device *dev, struct net_buf *const buf,
			     struct udc_ep_config *const ep_cfg)
{
	const struct udc_bflb_v1_config *const config = dev->config;
	const mm_reg_t base = config->base;
	unsigned int key;
	uint32_t regval;
	uint16_t len;

	/*
	 * RM 18.3.4: "Wait for swrdy to be 0 before writing data to e0tfw."
	 * STS_RDY must be 0 before FIFO writes and before setting SW_RDY.
	 */
	ep_wait_not_busy(base, 0);

	len = MIN(ep_cfg->mps, buf->len);

	/*
	 * Always clear TX FIFO before writing new data.
	 * Ensures no stale bytes from previous transfers are sent.
	 */
	fifo_tx_clear(base, 0);

	fifo_write(base, 0, buf->data, len);

	/*
	 * Set SIZE and arm RDY in one atomic register write.
	 *
	 * SIZE = exact byte count.  The hardware sends exactly SIZE
	 * bytes from the FIFO.  SIZE=0 produces a ZLP.
	 *
	 * Both SIZE and RDY must be written in a single irq-locked
	 * RMW to prevent an ISR (e.g. SETUP_DONE) from doing its own
	 * RMW between them that could restore a stale SIZE value.
	 *
	 * For ZLPs (len=0): this is always a status-IN packet (the
	 * final packet of a no-data or control-OUT transfer).  Clear
	 * NACK_OUT so the next SETUP can be received immediately after
	 * the hardware sends the ZLP, without waiting for the ISR to
	 * run.  The BL70x blocks SETUP tokens when NACK_OUT is set,
	 * which violates USB spec; clearing it early prevents NAKing
	 * back-to-back SETUP tokens from the host.
	 *
	 * For data packets (len>0): keep NACK_OUT set to prevent the
	 * host's status-OUT from being accepted before the thread has
	 * a chance to queue a buffer for it.
	 */
	key = irq_lock();
	regval = sys_read32(base + USB_CONFIG_OFFSET);
	regval &= ~(USB_CR_USB_EP0_SW_SIZE_MASK | USB_STS_USB_EP0_SW_RDY |
		    USB_CR_USB_EP0_SW_STALL);
	regval |= ((uint32_t)len << USB_CR_USB_EP0_SW_SIZE_SHIFT);
	regval |= USB_CR_USB_EP0_SW_RDY;
	regval |= USB_CR_USB_EP0_SW_NACK_IN;
	if (len > 0) {
		regval |= USB_CR_USB_EP0_SW_NACK_OUT;
	} else {
		regval &= ~USB_CR_USB_EP0_SW_NACK_OUT;
	}
	sys_write32(regval, base + USB_CONFIG_OFFSET);
	irq_unlock(key);
}

/*
 * Prepare and start an OUT receive for EP0.
 * Just sets EP0 ready to accept data.
 */
static void bflb_prep_ep0_rx(const struct device *dev)
{
	const struct udc_bflb_v1_config *const config = dev->config;
	const mm_reg_t base = config->base;
	unsigned int key;
	uint32_t regval;

	ep_wait_not_busy(base, 0);

	/* Arm EP0 OUT and clear NACK_OUT atomically so the host's
	 * Data OUT or Status OUT packet is accepted.
	 */
	key = irq_lock();
	regval = sys_read32(base + USB_CONFIG_OFFSET);
	regval &= ~(USB_STS_USB_EP0_SW_RDY | USB_CR_USB_EP0_SW_STALL |
		    USB_CR_USB_EP0_SW_NACK_OUT);
	regval |= USB_CR_USB_EP0_SW_RDY | USB_CR_USB_EP0_SW_NACK_IN;
	sys_write32(regval, base + USB_CONFIG_OFFSET);
	irq_unlock(key);
}

/*
 * Prepare and start an IN transfer for EP1-7.
 */
static void bflb_prep_epx_tx(const struct device *dev, struct net_buf *const buf,
			     struct udc_ep_config *const ep_cfg)
{
	const struct udc_bflb_v1_config *const config = dev->config;
	const mm_reg_t base = config->base;
	const uint8_t ep_idx = USB_EP_GET_IDX(ep_cfg->addr);
	uint32_t regval;
	uint32_t len;

	len = MIN(ep_cfg->mps, buf->len);

	/*
	 * EPx SIZE is the MPS configuration, not the per-packet byte
	 * count.  The hardware sends whatever is in the TX FIFO (up
	 * to SIZE bytes).  Always keep SIZE at MPS so ZLPs work
	 * correctly — writing 0 bytes to the FIFO with SIZE=MPS
	 * produces a ZLP; setting SIZE=0 would misconfigure the MPS.
	 */
	regval = sys_read32(base + BFLB_EPX_CONFIG_OFFSET(ep_idx));
	regval &= ~USB_CR_EP1_SIZE_MASK;
	regval |= ((uint32_t)ep_cfg->mps << USB_CR_EP1_SIZE_SHIFT);
	sys_write32(regval, base + BFLB_EPX_CONFIG_OFFSET(ep_idx));

	fifo_write(base, ep_idx, buf->data, len);
	epx_set_ready(base, ep_idx);
}

/*
 * Prepare and start an OUT receive for EP1-7.
 */
static void bflb_prep_epx_rx(const struct device *dev,
			     struct udc_ep_config *const ep_cfg)
{
	const struct udc_bflb_v1_config *const config = dev->config;
	const uint8_t ep_idx = USB_EP_GET_IDX(ep_cfg->addr);

	epx_set_ready(config->base, ep_idx);
}

/* Handle SETUP packet received (called from thread) */
static void handle_evt_setup(const struct device *dev)
{
	const struct udc_bflb_v1_config *const config = dev->config;
	const mm_reg_t base = config->base;
	struct udc_bflb_v1_data *const priv = udc_get_private(dev);
	struct udc_ep_config *const cfg_out =
		udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
	struct udc_ep_config *const cfg_in =
		udc_get_ep_cfg(dev, USB_CONTROL_EP_IN);
	unsigned int key;
	uint32_t regval;

	/* A SETUP packet starts a new control transfer and aborts the
	 * previous one.  Only set NACK_IN (not NACK_OUT) — the ISR
	 * manages NACK_OUT, and re-setting it here would block the
	 * next SETUP token (hardware NAKs SETUP when NACK_OUT=1).
	 */
	ep0_set_nack_in(base);

	key = irq_lock();
	regval = sys_read32(base + USB_CONFIG_OFFSET);
	regval &= ~(USB_STS_USB_EP0_SW_RDY | USB_CR_USB_EP0_SW_STALL);
	sys_write32(regval, base + USB_CONFIG_OFFSET);
	irq_unlock(key);

	/* Ensure EP0 OUT_DONE is unmasked (may have been masked by a
	 * spurious OUT).
	 */
	sys_clear_bits(base + USB_INT_MASK_OFFSET, USB_CR_EP0_OUT_DONE_MASK);
	fifo_tx_clear(base, 0);
	fifo_rx_clear(base, 0);
	if (cfg_out != NULL) {
		udc_ep_set_busy(cfg_out, false);
	}
	if (cfg_in != NULL) {
		udc_ep_set_busy(cfg_in, false);
	}

	bflb_set_ep_data1(dev, USB_CONTROL_EP_OUT, true);
	bflb_set_ep_data1(dev, USB_CONTROL_EP_IN, true);

	priv->setup_gen_serviced = priv->setup_gen;
	udc_setup_received(dev, priv->setup);
}

/* Start the next queued transfer for an endpoint */
static void handle_xfer_next(const struct device *dev,
			     struct udc_ep_config *const ep_cfg)
{
	struct net_buf *buf;

	buf = udc_buf_peek(ep_cfg);
	if (buf == NULL) {
		return;
	}

	if (ep_cfg->addr == USB_CONTROL_EP_OUT) {
		struct udc_buf_info *bi = udc_get_buf_info(buf);
		struct udc_ep_config *ep_in;

		if (bi->setup) {
			const struct udc_bflb_v1_config *const config =
				dev->config;

			/* Clear NACK_OUT so the next SETUP token is
			 * accepted.  Deferred from the ISR to ensure
			 * the stack finishes processing the previous
			 * transfer first (e.g. SET_ADDRESS).
			 */
			ep0_clear_nack_out(config->base);
			return;
		}

		/* EP0 shares one RDY bit for IN and OUT. Don't arm OUT
		 * while IN is busy or has a pending transfer — arming
		 * OUT sets RDY, which would block bflb_prep_ep0_tx()
		 * in ep_wait_not_busy() (deadlock) or corrupt the IN
		 * response.  The IN completion handler kicks OUT when
		 * the IN phase finishes.
		 */
		ep_in = udc_get_ep_cfg(dev, USB_CONTROL_EP_IN);
		if (udc_ep_is_busy(ep_in) || udc_buf_peek(ep_in) != NULL) {
			return;
		}
	}

	if (ep_cfg->stat.halted) {
		return;
	}

	udc_ep_set_busy(ep_cfg, true);

	if (USB_EP_DIR_IS_OUT(ep_cfg->addr)) {
		if (USB_EP_GET_IDX(ep_cfg->addr) == 0) {
			bflb_prep_ep0_rx(dev);
		} else {
			bflb_prep_epx_rx(dev, ep_cfg);
		}
	} else {
		if (USB_EP_GET_IDX(ep_cfg->addr) == 0) {
			bflb_prep_ep0_tx(dev, buf, ep_cfg);
		} else {
			bflb_prep_epx_tx(dev, buf, ep_cfg);
		}
	}
}

/*
 * Handle IN transfer completion (called from thread).
 * For EP0, the ISR has already handled intermediate packets.
 * For EPx, the ISR has handled all packet-level processing;
 * the thread just completes the transfer.
 */
static void handle_evt_din(const struct device *dev,
			   struct udc_ep_config *const ep_cfg)
{
	const struct udc_bflb_v1_config *const config = dev->config;
	const mm_reg_t base = config->base;
	struct net_buf *buf;

	if (ep_cfg->addr == USB_CONTROL_EP_IN) {
		struct udc_ep_config *ep_out;

		sys_clear_bits(base + USB_INT_MASK_OFFSET,
			       USB_CR_EP0_IN_DONE_MASK);

		buf = udc_buf_get(ep_cfg);
		if (buf == NULL) {
			LOG_ERR("No buffer for ep 0x%02x", ep_cfg->addr);
			udc_submit_event(dev, UDC_EVT_ERROR, -ENOBUFS);
			return;
		}

		udc_ep_set_busy(ep_cfg, false);
		udc_submit_ep_event(dev, buf, 0);

		/* EP0 shares one RDY bit: now that IN is done, arm
		 * any pending OUT (Status OUT) that was deferred.
		 */
		ep_out = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
		if (!udc_ep_is_busy(ep_out)) {
			handle_xfer_next(dev, ep_out);
		}
		return;
	}

	/* EPx: ISR handled packet-level work, just complete the transfer */
	buf = udc_buf_get(ep_cfg);
	if (buf == NULL) {
		LOG_WRN("No buffer for ep 0x%02x (dequeued?)", ep_cfg->addr);
		return;
	}

	udc_ep_set_busy(ep_cfg, false);

	if (ep_cfg->stat.halted) {
		/*
		 * Transfer was aborted by ep_set_halt mid-stream.
		 * Report the abort so the class driver discards this
		 * partial buffer and resubmits a fresh one.
		 */
		udc_submit_ep_event(dev, buf, -ECONNABORTED);
	} else {
		udc_submit_ep_event(dev, buf, 0);
	}
}

/*
 * Handle OUT transfer completion (called from thread).
 * For EP0, the ISR has already read data from FIFO.
 * For EPx, the ISR has handled all packet-level processing;
 * the thread just completes the transfer.
 */
static void handle_evt_dout(const struct device *dev,
			    struct udc_ep_config *const ep_cfg)
{
	const struct udc_bflb_v1_config *const config = dev->config;
	const mm_reg_t base = config->base;
	struct net_buf *buf;

	if (ep_cfg->addr == USB_CONTROL_EP_OUT) {
		sys_clear_bits(base + USB_INT_MASK_OFFSET,
			       USB_CR_EP0_OUT_DONE_MASK);

		buf = udc_buf_get(ep_cfg);
		if (buf == NULL) {
			return;
		}

		udc_ep_set_busy(ep_cfg, false);
		udc_submit_ep_event(dev, buf, 0);
		return;
	}

	/* EPx: ISR handled packet-level work, just complete the transfer */
	buf = udc_buf_get(ep_cfg);
	if (buf == NULL) {
		LOG_WRN("No buffer for ep 0x%02x (dequeued?)", ep_cfg->addr);
		return;
	}

	udc_ep_set_busy(ep_cfg, false);

	if (ep_cfg->stat.halted) {
		udc_submit_ep_event(dev, buf, -ECONNABORTED);
	} else {
		udc_submit_ep_event(dev, buf, 0);
	}
}

/* Initialize EP0 and interrupt configuration after reset */
static void reinit_ep0(const struct device *dev)
{
	const struct udc_bflb_v1_config *const config = dev->config;
	const mm_reg_t base = config->base;
	uint32_t regval;

	/* Configure EP0: SW control mode, 64-byte MPS, NAK both directions.
	 * NACK_OUT blocks SETUP tokens until the thread clears it after
	 * the USBD stack has processed the RESET event.
	 */
	regval = sys_read32(base + USB_CONFIG_OFFSET);
	regval &= ~USB_STS_USB_EP0_SW_RDY;
	regval |= USB_CR_USB_EP0_SW_CTRL;
	regval &= ~USB_CR_USB_EP0_SW_ADDR_MASK;
	regval &= ~USB_CR_USB_EP0_SW_SIZE_MASK;
	regval |= ((uint32_t)BFLB_EP0_MPS << USB_CR_USB_EP0_SW_SIZE_SHIFT);
	regval |= USB_CR_USB_EP0_SW_NACK_IN;
	regval |= USB_CR_USB_EP0_SW_NACK_OUT;
	regval &= ~USB_CR_USB_EP0_SW_STALL;
	regval &= ~USB_CR_USB_ROM_DCT_EN;
	sys_write32(regval, base + USB_CONFIG_OFFSET);

	/* Enable base interrupts: reset, setup done, EP0 in/out done, REND */
	regval = 0;
	regval |= USB_CR_USB_RESET_EN;
	regval |= USB_CR_EP0_SETUP_DONE_EN;
	regval |= USB_CR_EP0_IN_DONE_EN;
	regval |= USB_CR_EP0_OUT_DONE_EN;
	regval |= USB_CR_USB_REND_EN;
	regval |= USB_CR_USB_ERR_EN;
	if (IS_ENABLED(CONFIG_UDC_ENABLE_SOF)) {
		regval |= USB_CR_SOF_EN;
	}
	sys_write32(regval, base + USB_INT_EN_OFFSET);

	/* Unmask these interrupts */
	regval = 0xFFFFFFFFU;
	regval &= ~USB_CR_USB_RESET_MASK;
	regval &= ~USB_CR_EP0_SETUP_DONE_MASK;
	regval &= ~USB_CR_EP0_IN_DONE_MASK;
	regval &= ~USB_CR_EP0_OUT_DONE_MASK;
	regval &= ~USB_CR_USB_REND_MASK;
	regval &= ~USB_CR_USB_ERR_MASK;
	if (IS_ENABLED(CONFIG_UDC_ENABLE_SOF)) {
		regval &= ~USB_CR_SOF_MASK;
	}
	sys_write32(regval, base + USB_INT_MASK_OFFSET);

	/* Clear all pending interrupts */
	sys_write32(0xFFFFFFFFU, base + USB_INT_CLEAR_OFFSET);

	/* Clear FIFOs */
	fifo_tx_clear(base, 0);
	fifo_rx_clear(base, 0);
}

/*
 * Handle a single IN packet completion in ISR context.
 * Prepares the next packet if more data remains, otherwise signals the
 * thread that the transfer is complete.
 */
static void isr_handle_in_done(const struct device *dev, const mm_reg_t base,
			       struct udc_bflb_v1_data *const priv,
			       const uint8_t ep_idx)
{
	const uint8_t ep_addr = USB_EP_DIR_IN | ep_idx;
	struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, ep_addr);
	struct net_buf *buf;
	uint32_t len;

	if (ep_cfg == NULL) {
		return;
	}

	buf = udc_buf_peek(ep_cfg);
	if (buf == NULL) {
		return;
	}

	len = MIN(ep_cfg->mps, buf->len);
	net_buf_pull(buf, len);

	if (buf->len > 0) {
		/* More data to send, prepare next packet */
		if (ep_idx == 0) {
			bflb_prep_ep0_tx(dev, buf, ep_cfg);
		} else if (ep_cfg->stat.halted) {
			/*
			 * Endpoint was halted mid-transfer.  Don't arm the
			 * next packet (epx_set_ready would clear STALL).
			 * Signal the thread to abort the transfer.
			 */
			atomic_or(&priv->xfer_finished, BIT(16U + ep_idx));
			k_event_post(&priv->events, UDC_BFLB_V1_EVT_XFER);
		} else {
			bflb_prep_epx_tx(dev, buf, ep_cfg);
		}
		return;
	}

	if (udc_ep_buf_has_zlp(buf)) {
		udc_ep_buf_clear_zlp(buf);
		if (ep_idx == 0) {
			/* Set SIZE=0 and arm RDY atomically */
			uint32_t regval = sys_read32(base + USB_CONFIG_OFFSET);

			regval &= ~(USB_CR_USB_EP0_SW_SIZE_MASK |
				    USB_STS_USB_EP0_SW_RDY |
				    USB_CR_USB_EP0_SW_STALL);
			regval |= USB_CR_USB_EP0_SW_RDY;
			regval |= USB_CR_USB_EP0_SW_NACK_IN;
			regval |= USB_CR_USB_EP0_SW_NACK_OUT;
			sys_write32(regval, base + USB_CONFIG_OFFSET);
		} else if (ep_cfg->stat.halted) {
			atomic_or(&priv->xfer_finished, BIT(16U + ep_idx));
			k_event_post(&priv->events, UDC_BFLB_V1_EVT_XFER);
		} else {
			epx_set_ready(base, ep_idx);
		}
		return;
	}

	/* Transfer complete, defer to thread */
	if (ep_idx == 0) {
		sys_set_bits(base + USB_INT_MASK_OFFSET,
			     USB_CR_EP0_IN_DONE_MASK);

		/*
		 * Set NAK so the host receives NAK responses while
		 * the thread processes the completion and re-arms EP0.
		 * Clear NACK_OUT so the host can send Status OUT
		 * (for GET requests) or the next SETUP (for SET
		 * requests whose Status IN was already sent).
		 */
		ep0_set_nak(base);

		ep0_clear_nack_out(base);
		priv->ep0_status_done = true;
	}
	atomic_or(&priv->xfer_finished, BIT(16U + ep_idx));
	k_event_post(&priv->events, UDC_BFLB_V1_EVT_XFER);
}

/*
 * Handle a single OUT packet completion in ISR context.
 * Reads data from FIFO and re-arms the endpoint if more data is expected,
 * otherwise signals the thread that the transfer is complete.
 */
static void isr_handle_out_done(const struct device *dev, const mm_reg_t base,
				struct udc_bflb_v1_data *const priv,
				const uint8_t ep_idx)
{
	const uint8_t ep_addr = USB_EP_DIR_OUT | ep_idx;
	struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, ep_addr);
	struct net_buf *buf;
	uint16_t rx_count;

	if (ep_cfg == NULL) {
		return;
	}

	buf = udc_buf_peek(ep_cfg);
	if (buf == NULL) {
		if (ep_idx == 0) {
			fifo_rx_clear(base, 0);
		} else {
			/*
			 * Packet arrived after transfer was canceled; flush
			 * stale FIFO data to keep subsequent transfers in sync.
			 */
			fifo_rx_clear(base, ep_idx);
		}
		return;
	}

	rx_count = ep_get_rxcount(base, ep_idx);

	/*
	 * EP0: never complete a setup_buf in the OUT_DONE path.
	 * The setup_buf is reserved for udc_setup_received() which
	 * fills it with 8 bytes of SETUP data.  If we complete it
	 * here (e.g. from a Status-OUT ZLP or stale FIFO data),
	 * the USB stack sees len!=8 → "Malformed setup packet".
	 */
	if (ep_idx == 0) {
		struct udc_buf_info *bi = udc_get_buf_info(buf);

		if (bi->setup) {
			if (rx_count == 0) {
				ep0_clear_nack_out(base);
			}
			return;
		}
	}

	/* EPx OUT_DONE with empty FIFO and empty buffer: either a
	 * spurious interrupt from endpoint init/arm, or a legitimate
	 * ZLP (e.g. URB_ZERO_PACKET) on a fresh buffer.  In both
	 * cases, re-arm the endpoint to accept the next (real) packet
	 * rather than completing a 0-byte transfer that would waste a
	 * buffer round-trip through the thread.
	 *
	 * Only fall through to transfer-complete when buf already has
	 * data (buf->len > 0) — that's a true end-of-transfer ZLP.
	 */
	if (rx_count == 0 && ep_idx > 0 && buf->len == 0) {
		if (!ep_cfg->stat.halted) {
			epx_set_ready(base, ep_idx);
		}
		return;
	}

	if (rx_count > 0) {
		uint32_t room = net_buf_tailroom(buf);

		if (rx_count > room) {
			rx_count = (uint16_t)room;
		}

		fifo_read(base, ep_idx, net_buf_tail(buf), rx_count);
		net_buf_add(buf, rx_count);
	}

	if (rx_count == ep_cfg->mps && net_buf_tailroom(buf) > 0) {
		/* Full packet and more space: expect more data */
		if (ep_idx == 0) {
			ep0_set_ready(base);
		} else if (ep_cfg->stat.halted) {
			/*
			 * Endpoint halted mid-receive.  Don't re-arm
			 * (epx_set_ready would clear STALL).
			 */
			atomic_or(&priv->xfer_finished, BIT(ep_idx));
			k_event_post(&priv->events, UDC_BFLB_V1_EVT_XFER);
		} else {
			epx_set_ready(base, ep_idx);
		}
		return;
	}

	/* Transfer complete (short packet or buffer full), defer to thread */
	if (ep_idx == 0) {
		sys_set_bits(base + USB_INT_MASK_OFFSET,
			     USB_CR_EP0_OUT_DONE_MASK);
		/*
		 * NAK both directions.  Keep NACK_OUT set so the host
		 * cannot send the next SETUP until the thread clears it
		 * via handle_xfer_next() for the setup buffer — this
		 * guarantees the buffer is on the queue when
		 * udc_setup_received() runs, preventing the
		 * setup_pending bypass from cancelling Status IN ZLPs.
		 */
		ep0_set_nak(base);
		priv->ep0_status_done = true;
	}
	atomic_or(&priv->xfer_finished, BIT(ep_idx));
	k_event_post(&priv->events, UDC_BFLB_V1_EVT_XFER);
}

/* ISR handler */
static void udc_bflb_v1_isr(const struct device *dev)
{
	const struct udc_bflb_v1_config *const config = dev->config;
	struct udc_bflb_v1_data *const priv = udc_get_private(dev);
	const mm_reg_t base = config->base;
	uint32_t sts;
	uint32_t err_sts = 0U;

	sts = get_intstatus(base);
	if (sts & USB_ERR_INT) {
		err_sts = sys_read32(base + USB_ERROR_OFFSET);
	}
	sys_write32(sts, base + USB_INT_CLEAR_OFFSET);

	if (IS_ENABLED(CONFIG_UDC_ENABLE_SOF) && (sts & USB_SOF_INT)) {
		udc_submit_sof_event(dev);
	}

	if (sts & USB_RESET_INT) {
		k_event_post(&priv->events, UDC_BFLB_V1_EVT_RESET);
		/*
		 * After a bus reset, ignore any SETUP_DONE in this
		 * same ISR invocation.  The SETUP belongs to the OLD
		 * bus session.  The host will re-send SETUP after the
		 * RESET completes.  Processing the stale SETUP would
		 * race with the RESET event in the USBD thread,
		 * causing SET_ADDRESS state to be overwritten.
		 */
		sts &= ~USB_EP0_SETUP_DONE_INT;
	}

	/*
	 * Process EP0 DONE interrupts BEFORE SETUP.
	 *
	 * When a transfer's status phase completes and the host
	 * immediately sends the next SETUP, both DONE and SETUP_DONE
	 * can appear in the same interrupt status read.  If SETUP is
	 * processed first, it sets NACK_OUT=1 for the new transfer,
	 * but then the DONE handler clears NACK_OUT for the old
	 * transfer — leaving NACK_OUT=0 for the new transfer.
	 *
	 * Processing DONE first lets the old transfer's NACK_OUT
	 * clearing happen before SETUP sets it for the new transfer.
	 */
	if (sts & USB_EP0_IN_DONE_INT) {
		isr_handle_in_done(dev, base, priv, 0);
	}

	if (sts & USB_EP0_OUT_DONE_INT) {
		/*
		 * When SETUP_DONE is also pending, skip EP0 OUT_DONE.
		 * The EP0 RX FIFO is shared: SETUP data (8 bytes) may
		 * already be in the FIFO.  If OUT_DONE reads it as
		 * regular OUT data, the SETUP_DONE handler finds an
		 * empty FIFO and drops the SETUP.
		 */
		if (!(sts & USB_EP0_SETUP_DONE_INT)) {
			isr_handle_out_done(dev, base, priv, 0);
		}
	}

	if (sts & USB_EP0_SETUP_DONE_INT) {
		ep_wait_not_busy(base, 0);

		if (ep_get_rxcount(base, 0) == 8) {
			uint32_t cfg;

			uint16_t wLength;

			fifo_tx_clear(base, 0);
			fifo_read(base, 0, priv->setup, 8);

			wLength = (uint16_t)priv->setup[6] |
				  ((uint16_t)priv->setup[7] << 8U);
			priv->ep0_ctrl_in = (priv->setup[0] & 0x80U) != 0U;
			priv->ep0_has_data = wLength > 0;
			priv->setup_pending = true;
			priv->ep0_status_done = false;
			priv->setup_gen++;

			/*
			 * Break the setup_pending vicious cycle.
			 * If the UDC common layer's setup_pending is
			 * true from a previous race, clear it now so
			 * the stack's next usbd_enqueue_setup() goes
			 * through our ep_enqueue callback (placing the
			 * buffer on the queue) instead of the bypass
			 * path that submits empty buffers.
			 */
			{
				struct udc_data *ud = dev->data;

				ud->setup_pending = false;
			}

			/*
			 * Set NAK and clear STALL to hold off the host
			 * until the thread prepares a response.
			 */
			/*
			 * Set NAK and clear STALL to hold off the host
			 * until the thread prepares a response.
			 */
			cfg = sys_read32(base + USB_CONFIG_OFFSET);
			cfg &= ~(USB_STS_USB_EP0_SW_RDY |
				 USB_CR_USB_EP0_SW_STALL);
			cfg |= USB_CR_USB_EP0_SW_NACK_IN |
			       USB_CR_USB_EP0_SW_NACK_OUT;
			sys_write32(cfg, base + USB_CONFIG_OFFSET);

			k_event_post(&priv->events, UDC_BFLB_V1_EVT_SETUP);
		} else {
			fifo_rx_clear(base, 0);
		}
	}

	/* Handle EP1-7 DONE interrupts with packet-level processing */
	for (uint8_t i = 1U; i < (uint8_t)config->num_of_eps; i++) {
		const uint32_t done_bit = BIT(9U + 2U * (uint32_t)i);

		if (!(sts & done_bit)) {
			continue;
		}

		if (epx_get_dir(base, i) == BFLB_EP_DIR_IN) {
			isr_handle_in_done(dev, base, priv, i);
		} else {
			isr_handle_out_done(dev, base, priv, i);
		}
	}

	/*
	 * REND (request/transfer end) fires at control transfer boundaries.
	 *
	 * Previously, REND cleared NACK_OUT to re-arm EP0 for the next
	 * SETUP.  However, REND can fire DURING the data phase (e.g.
	 * right after SETUP ACK), which clears NACK_OUT prematurely.
	 *
	 * NACK_OUT is now managed exclusively by the DONE handlers:
	 *  - isr_handle_in_done: clears unconditionally on EP0 completion
	 *  - isr_handle_out_done: clears unconditionally on EP0 completion
	 *  - ep_set_halt: clears so next SETUP is accepted after STALL
	 * The thread clears NACK_OUT via bflb_prep_ep0_rx() when ready
	 * for OUT data or status-OUT phases.
	 */
	if (sts & USB_ERR_INT) {
		bflb_handle_usb_error(dev, base, err_sts);
	}
}

/* Process completed IN and OUT transfers from the ISR bitmap */
static void handle_xfer_done(const struct device *dev, uint32_t eps)
{
	struct udc_ep_config *ep_cfg;
	uint32_t in_eps = eps >> 16U;
	uint32_t out_eps = eps & 0xFFFFU;
	unsigned int bit;
	uint8_t ep;

	/*
	 * Process IN endpoints (bits 16+) before OUT (bits 0-15).
	 * This ensures data-stage IN completions feed status-out
	 * buffers before status-out completions are handled.
	 */
	while (in_eps) {
		bit = find_lsb_set(in_eps) - 1;
		in_eps &= ~BIT(bit);
		ep = USB_EP_DIR_IN | bit;
		ep_cfg = udc_get_ep_cfg(dev, ep);
		if (ep_cfg == NULL) {
			LOG_ERR("No config for ep 0x%02x", ep);
			continue;
		}
		handle_evt_din(dev, ep_cfg);
		if (!udc_ep_is_busy(ep_cfg)) {
			handle_xfer_next(dev, ep_cfg);
		}
	}

	while (out_eps) {
		bit = find_lsb_set(out_eps) - 1;
		out_eps &= ~BIT(bit);
		ep = USB_EP_DIR_OUT | bit;
		ep_cfg = udc_get_ep_cfg(dev, ep);
		if (ep_cfg == NULL) {
			LOG_ERR("No config for ep 0x%02x", ep);
			continue;
		}
		handle_evt_dout(dev, ep_cfg);
		if (!udc_ep_is_busy(ep_cfg)) {
			handle_xfer_next(dev, ep_cfg);
		}
	}
}

/* Start queued transfers for newly enqueued endpoints */
static void handle_xfer_new(const struct device *dev, uint32_t eps)
{
	struct udc_ep_config *ep_cfg;
	uint8_t ep;

	while (eps) {
		ep = bflb_pull_ep_from_bmsk(&eps);
		ep_cfg = udc_get_ep_cfg(dev, ep);

		if (ep_cfg == NULL) {
			LOG_ERR("No config for ep 0x%02x", ep);
			continue;
		}

		if (!udc_ep_is_busy(ep_cfg)) {
			handle_xfer_next(dev, ep_cfg);
		}
	}
}

static void bflb_thread_handler(void *p)
{
	const struct device *dev = (const struct device *)p;
	const struct udc_bflb_v1_config *const config = dev->config;
	struct udc_bflb_v1_data *const priv = udc_get_private(dev);
	uint32_t evt;
	uint32_t eps;

	evt = k_event_wait_safe(&priv->events, UINT32_MAX, false, K_FOREVER);
	udc_lock_internal(dev, K_FOREVER);

	/*
	 * Process all pending events in a single lock acquisition.
	 * SETUP processing posts NEWXFER synchronously (via the USBD
	 * framework callback), which would normally require a second
	 * thread iteration.  Re-checking for new events here lets us
	 * arm EP0 in the same pass, cutting control-transfer latency
	 * roughly in half.
	 */
	do {
		if (evt & UDC_BFLB_V1_EVT_RESET) {
			struct udc_data *udc_data = dev->data;

			LOG_DBG("Bus reset");
			reinit_ep0(dev);

			/*
			 * Clear stale setup_pending from the UDC common
			 * layer.  Without this, the next usbd_enqueue_setup
			 * after RESET bypasses our ep_enqueue callback and
			 * directly submits a setup buffer with stale/empty
			 * cached data (len=0), causing "Malformed setup
			 * packet" errors on every enumeration cycle.
			 */
			udc_data->setup_pending = false;

			udc_submit_event(dev, UDC_EVT_RESET, 0);
			udc_unlock_internal(dev);
			k_sleep(K_MSEC(5));
			udc_lock_internal(dev, K_FOREVER);
			ep0_clear_nack_out(config->base);
		}

		if (evt & UDC_BFLB_V1_EVT_XFER) {
			eps = atomic_clear(&priv->xfer_finished);
			handle_xfer_done(dev, eps);
		}

		if (evt & UDC_BFLB_V1_EVT_SETUP) {
			handle_evt_setup(dev);
			/*
			 * The USBD thread must process the SETUP and
			 * enqueue the response before we can arm EP0.
			 * Yield to let it run, then pick up NEWXFER.
			 */
			udc_unlock_internal(dev);
			k_yield();
			udc_lock_internal(dev, K_FOREVER);
		}

		if (evt & UDC_BFLB_V1_EVT_NEWXFER) {
			eps = atomic_clear(&priv->xfer_new);
			handle_xfer_new(dev, eps);
		}

		evt = k_event_wait_safe(&priv->events, UINT32_MAX, false,
					K_NO_WAIT);
	} while (evt);

	udc_unlock_internal(dev);
}

static int udc_bflb_v1_ep_enqueue(const struct device *dev,
				  struct udc_ep_config *const ep_cfg,
				  struct net_buf *const buf)
{
	struct udc_bflb_v1_data *const priv = udc_get_private(dev);

	LOG_DBG("Enqueue ep 0x%02x len %u", ep_cfg->addr, buf->len);
	udc_buf_put(ep_cfg, buf);

	if (ep_cfg->stat.halted) {
		LOG_DBG("ep 0x%02x halted", ep_cfg->addr);
		return 0;
	}

	/* Signal the thread to start the transfer */
	atomic_or(&priv->xfer_new, BIT(ep_to_bnum(ep_cfg->addr)));
	k_event_post(&priv->events, UDC_BFLB_V1_EVT_NEWXFER);

	return 0;
}

static int udc_bflb_v1_ep_dequeue(const struct device *dev,
				  struct udc_ep_config *const ep_cfg)
{
	struct udc_bflb_v1_data *const priv = udc_get_private(dev);
	const struct udc_bflb_v1_config *const config = dev->config;
	const mm_reg_t base = config->base;
	uint8_t ep_idx = USB_EP_GET_IDX(ep_cfg->addr);
	unsigned int lock_key;

	lock_key = irq_lock();

	if (ep_idx == 0) {
		ep0_set_nak(base);
		fifo_tx_clear(base, 0);
		fifo_rx_clear(base, 0);
	} else {
		epx_set_nak(base, ep_idx);
		fifo_tx_clear(base, ep_idx);
		fifo_rx_clear(base, ep_idx);
	}

	udc_ep_cancel_queued(dev, ep_cfg);

	/* Clear pending completion signal to prevent stale processing */
	atomic_and(&priv->xfer_finished, ~BIT(ep_to_bnum(ep_cfg->addr)));

	udc_ep_set_busy(ep_cfg, false);
	irq_unlock(lock_key);

	return 0;
}

static int udc_bflb_v1_ep_enable(const struct device *dev,
				 struct udc_ep_config *const ep_cfg)
{
	const struct udc_bflb_v1_config *const config = dev->config;
	const mm_reg_t base = config->base;
	const uint8_t ep_idx = USB_EP_GET_IDX(ep_cfg->addr);
	uint8_t hw_type;
	uint8_t hw_dir;
	uint32_t regval;

	LOG_DBG("Enable ep 0x%02x", ep_cfg->addr);

	if (ep_idx == 0) {
		/* EP0 is always configured in init */
		return 0;
	}

	/* New endpoint configuration starts with DATA0 for non-control
	 * endpoints.
	 */
	ep_cfg->stat.data1 = false;

	/* Map USB standard endpoint type to BL70x hardware values */
	switch (ep_cfg->attributes & USB_EP_TRANSFER_TYPE_MASK) {
	case USB_EP_TYPE_ISO:
		hw_type = BFLB_EP_TYPE_ISO;
		break;
	case USB_EP_TYPE_BULK:
		hw_type = BFLB_EP_TYPE_BULK;
		break;
	case USB_EP_TYPE_INTERRUPT:
		hw_type = BFLB_EP_TYPE_INTERRUPT;
		break;
	default:
		hw_type = BFLB_EP_TYPE_BULK;
		break;
	}

	hw_dir = USB_EP_DIR_IS_IN(ep_cfg->addr) ? BFLB_EP_DIR_IN
						: BFLB_EP_DIR_OUT;

	/* Configure EP size, direction, type */
	regval = sys_read32(base + BFLB_EPX_CONFIG_OFFSET(ep_idx));

	regval &= ~USB_CR_EP1_SIZE_MASK;
	regval &= ~USB_CR_EP1_TYPE_MASK;
	regval &= ~USB_CR_EP1_DIR_MASK;
	regval |= ((uint32_t)ep_cfg->mps << USB_CR_EP1_SIZE_SHIFT);
	regval |= ((uint32_t)hw_dir << USB_CR_EP1_DIR_SHIFT);
	regval |= ((uint32_t)hw_type << USB_CR_EP1_TYPE_SHIFT);
	sys_write32(regval, base + BFLB_EPX_CONFIG_OFFSET(ep_idx));

	/* Start in NAK state and clear FIFOs BEFORE enabling interrupts */
	epx_set_nak(base, ep_idx);
	fifo_tx_clear(base, ep_idx);
	fifo_rx_clear(base, ep_idx);

	/* Clear any pending DONE interrupt, then unmask */
	sys_write32(BIT(9U + (uint32_t)ep_idx * 2U),
		    base + USB_INT_CLEAR_OFFSET);
	sys_set_bits(base + USB_INT_EN_OFFSET, BIT(9U + (uint32_t)ep_idx * 2U));
	sys_clear_bits(base + USB_INT_MASK_OFFSET,
		       BIT(9U + (uint32_t)ep_idx * 2U));

	return 0;
}

static int udc_bflb_v1_ep_disable(const struct device *dev,
				  struct udc_ep_config *const ep_cfg)
{
	const struct udc_bflb_v1_config *const config = dev->config;
	const mm_reg_t base = config->base;
	const uint8_t ep_idx = USB_EP_GET_IDX(ep_cfg->addr);

	LOG_DBG("Disable ep 0x%02x", ep_cfg->addr);

	if (ep_idx == 0) {
		return 0;
	}

	/* Disable DONE interrupt for this endpoint */
	sys_clear_bits(base + USB_INT_EN_OFFSET,
		       BIT(9U + (uint32_t)ep_idx * 2U));
	sys_set_bits(base + USB_INT_MASK_OFFSET,
		     BIT(9U + (uint32_t)ep_idx * 2U));

	/* NAK the endpoint */
	epx_set_nak(base, ep_idx);
	/* Flush stale data before a later re-enable */
	fifo_tx_clear(base, ep_idx);
	fifo_rx_clear(base, ep_idx);

	return 0;
}

static int udc_bflb_v1_ep_set_halt(const struct device *dev,
				   struct udc_ep_config *const ep_cfg)
{
	const struct udc_bflb_v1_config *const config = dev->config;
	const mm_reg_t base = config->base;
	const uint8_t ep_idx = USB_EP_GET_IDX(ep_cfg->addr);

	LOG_DBG("Set halt ep 0x%02x", ep_cfg->addr);

	if (ep_idx == 0) {
		struct udc_bflb_v1_data *const priv = udc_get_private(dev);
		unsigned int key;
		uint32_t regval;

		/*
		 * The USBD stack calls ep_set_halt asynchronously (via
		 * its own thread + message queue).  A new SETUP may
		 * have arrived and already been processed by the driver
		 * between the time the stack decided to STALL and now.
		 * Compare setup_gen (ISR-incremented on every SETUP_DONE)
		 * with setup_gen_serviced (snapshot at handle_evt_setup).
		 * If they differ, a new SETUP has arrived since the last
		 * one we processed — this STALL is for an old request and
		 * must be suppressed to avoid corrupting the new transfer.
		 */
		if (priv->setup_gen != priv->setup_gen_serviced) {
			LOG_DBG("ep_set_halt(EP0): suppressed stale STALL");
			return 0;
		}

		if (priv->ep0_status_done) {
			LOG_DBG("ep_set_halt(EP0): suppressed (status done)");
			return 0;
		}

		key = irq_lock();
		regval = sys_read32(base + USB_CONFIG_OFFSET);

		regval &= ~USB_STS_USB_EP0_SW_RDY;
		regval |= USB_CR_USB_EP0_SW_NACK_IN;
		regval |= USB_CR_USB_EP0_SW_RDY;
		regval |= USB_CR_USB_EP0_SW_STALL;
		/*
		 * Keep NACK_OUT set.  The STALL response consumes
		 * RDY, and NACK_OUT blocks the next SETUP until
		 * the thread is ready (via handle_xfer_next for
		 * the Setup buffer).
		 */
		sys_write32(regval, base + USB_CONFIG_OFFSET);
		irq_unlock(key);
	} else {
		uint32_t regval =
			sys_read32(base + BFLB_EPX_CONFIG_OFFSET(ep_idx));

		/*
		 * Set the persistent STALL bit.  Per the BL70x RM, when exs
		 * (STALL) is set the endpoint will always reply STALL until
		 * software clears it.  Clear NACK to avoid undefined behavior
		 * (RM warns against enabling both simultaneously on EP0;
		 * apply the same rule to EPx).  Leave RDY untouched — the
		 * hardware sends STALL regardless of RDY state.
		 */
		regval |= USB_CR_EP1_STALL;
		regval &= ~USB_CR_EP1_NACK;
		sys_write32(regval, base + BFLB_EPX_CONFIG_OFFSET(ep_idx));
		ep_cfg->stat.halted = true;
	}

	return 0;
}

static int udc_bflb_v1_ep_clear_halt(const struct device *dev,
				     struct udc_ep_config *const ep_cfg)
{
	const struct udc_bflb_v1_config *const config = dev->config;
	struct udc_bflb_v1_data *const priv = udc_get_private(dev);
	const mm_reg_t base = config->base;
	const uint8_t ep_idx = USB_EP_GET_IDX(ep_cfg->addr);

	LOG_DBG("Clear halt ep 0x%02x", ep_cfg->addr);

	if (ep_idx != 0) {
		/* Clear stall, set NAK to prevent immediate traffic */
		epx_set_nak(base, ep_idx);
		/* Clear FIFOs to remove stale data */
		fifo_tx_clear(base, ep_idx);
		fifo_rx_clear(base, ep_idx);
		ep_cfg->stat.data1 = false;
		ep_cfg->stat.halted = false;
		/* Restart queued transfers if any */
		if (udc_buf_peek(ep_cfg)) {
			atomic_or(&priv->xfer_new,
				  BIT(ep_to_bnum(ep_cfg->addr)));
			k_event_post(&priv->events, UDC_BFLB_V1_EVT_NEWXFER);
		}
	}

	return 0;
}

static int udc_bflb_v1_set_address(const struct device *dev, const uint8_t addr)
{
	const struct udc_bflb_v1_config *const config = dev->config;
	const mm_reg_t base = config->base;
	unsigned int key;
	uint32_t regval;

	key = irq_lock();
	regval = sys_read32(base + USB_CONFIG_OFFSET);
	regval &= ~USB_CR_USB_EP0_SW_ADDR_MASK;
	regval |= (uint32_t)addr << USB_CR_USB_EP0_SW_ADDR_SHIFT;
	sys_write32(regval, base + USB_CONFIG_OFFSET);
	irq_unlock(key);

	return 0;
}

static int udc_bflb_v1_host_wakeup(const struct device *dev)
{
	/* TODO: Implement remote wakeup signaling via USB XCVR registers */
	LOG_DBG("Remote wakeup");
	return -ENOTSUP;
}

static enum udc_bus_speed udc_bflb_v1_device_speed(const struct device *dev)
{
	return UDC_BUS_SPEED_FS;
}

static int udc_bflb_v1_init(const struct device *dev)
{
	const struct udc_bflb_v1_config *const config = dev->config;
	int err;

	err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (err != 0) {
		LOG_ERR("Failed to apply pinctrl (%d)", err);
		return err;
	}

	/* Reset and configure XCVR and EP0. */
	err = xcvr_reset();
	if (err != 0) {
		return err;
	}

	reinit_ep0(dev);

	err = udc_ep_enable_internal(dev, USB_CONTROL_EP_OUT,
				     USB_EP_TYPE_CONTROL, BFLB_EP0_MPS, 0);
	if (err != 0) {
		LOG_ERR("Failed to enable control OUT endpoint");
		return err;
	}

	err = udc_ep_enable_internal(dev, USB_CONTROL_EP_IN, USB_EP_TYPE_CONTROL,
				     BFLB_EP0_MPS, 0);
	if (err != 0) {
		LOG_ERR("Failed to enable control IN endpoint");
		return err;
	}

	return 0;
}

static int udc_bflb_v1_enable(const struct device *dev)
{
	const struct udc_bflb_v1_config *const config = dev->config;
	struct udc_data *data = dev->data;
	const mm_reg_t base = config->base;

	/* Ensure no stale setup_pending from a previous USB session
	 * (e.g. UF2 bootloader).
	 */
	data->setup_pending = false;

	/* Enable USB controller */
	sys_set_bits(base + USB_CONFIG_OFFSET, USB_CR_USB_EN);

	/* Connect IRQ */
	config->irq_enable_func(dev);

	/* Connect to USB bus only after controller is fully enabled */
	sys_set_bits(GLB_BASE + GLB_USB_XCVR_OFFSET, GLB_USB_ENUM_MSK);

	return 0;
}

static int udc_bflb_v1_disable(const struct device *dev)
{
	const struct udc_bflb_v1_config *const config = dev->config;
	const mm_reg_t base = config->base;
	int err;

	LOG_DBG("Disable USB device");

	config->irq_disable_func(dev);

	/* Disable USB controller */
	sys_clear_bits(base + USB_CONFIG_OFFSET, USB_CR_USB_EN);

	/* Disconnect from bus */
	sys_clear_bits(GLB_BASE + GLB_USB_XCVR_OFFSET, GLB_USB_ENUM_MSK);

	err = udc_ep_disable_internal(dev, USB_CONTROL_EP_OUT);
	if (err != 0) {
		LOG_ERR("Failed to disable control OUT endpoint");
	}

	err = udc_ep_disable_internal(dev, USB_CONTROL_EP_IN);
	if (err != 0) {
		LOG_ERR("Failed to disable control IN endpoint");
	}

	return 0;
}

static int udc_bflb_v1_shutdown(const struct device *dev)
{
	LOG_DBG("Shutdown %s", dev->name);

	xcvr_shutdown();

	return 0;
}

static void udc_bflb_v1_lock(const struct device *dev)
{
	k_sched_lock();
	udc_lock_internal(dev, K_FOREVER);
}

static void udc_bflb_v1_unlock(const struct device *dev)
{
	udc_unlock_internal(dev);
	k_sched_unlock();
}

static const struct udc_api udc_bflb_v1_api = {
	.lock = udc_bflb_v1_lock,
	.unlock = udc_bflb_v1_unlock,
	.device_speed = udc_bflb_v1_device_speed,
	.init = udc_bflb_v1_init,
	.enable = udc_bflb_v1_enable,
	.disable = udc_bflb_v1_disable,
	.shutdown = udc_bflb_v1_shutdown,
	.set_address = udc_bflb_v1_set_address,
	.host_wakeup = udc_bflb_v1_host_wakeup,
	.ep_enable = udc_bflb_v1_ep_enable,
	.ep_disable = udc_bflb_v1_ep_disable,
	.ep_set_halt = udc_bflb_v1_ep_set_halt,
	.ep_clear_halt = udc_bflb_v1_ep_clear_halt,
	.ep_enqueue = udc_bflb_v1_ep_enqueue,
	.ep_dequeue = udc_bflb_v1_ep_dequeue,
};

static int udc_bflb_v1_driver_preinit(const struct device *dev)
{
	const struct udc_bflb_v1_config *const config = dev->config;
	struct udc_bflb_v1_data *const priv = udc_get_private(dev);
	struct udc_data *const data = dev->data;
	const uint16_t mps = BFLB_EPX_MPS_MAX;
	const size_t num_out = (config->num_of_eps - 1U) / 2U;
	int err;

	k_mutex_init(&data->mutex);
	k_event_init(&priv->events);
	atomic_clear(&priv->xfer_new);
	atomic_clear(&priv->xfer_finished);

	data->caps.rwup = false;
	data->caps.addr_before_status = true;
	data->caps.mps0 = UDC_MPS0_64;

	/*
	 * EP0: always bidirectional (control endpoint with dedicated hardware).
	 *
	 * EP1..N-1: each physical EPx has separate TX/RX FIFOs but only ONE
	 * direction register, so a given EPx can only be IN or OUT at a time.
	 * Split the pool: lower indices for OUT, upper indices for IN.
	 */
	config->ep_cfg_out[0].caps.out = 1;
	config->ep_cfg_out[0].caps.control = 1;
	config->ep_cfg_out[0].caps.mps = BFLB_EP0_MPS;
	config->ep_cfg_out[0].addr = USB_CONTROL_EP_OUT;
	err = udc_register_ep(dev, &config->ep_cfg_out[0]);
	if (err != 0) {
		LOG_ERR("Failed to register endpoint");
		return err;
	}

	config->ep_cfg_in[0].caps.in = 1;
	config->ep_cfg_in[0].caps.control = 1;
	config->ep_cfg_in[0].caps.mps = BFLB_EP0_MPS;
	config->ep_cfg_in[0].addr = USB_CONTROL_EP_IN;
	err = udc_register_ep(dev, &config->ep_cfg_in[0]);
	if (err != 0) {
		LOG_ERR("Failed to register endpoint");
		return err;
	}

	for (size_t i = 1; i <= num_out; i++) {
		config->ep_cfg_out[i].caps.out = 1;
		config->ep_cfg_out[i].caps.bulk = 1;
		config->ep_cfg_out[i].caps.interrupt = 1;
		config->ep_cfg_out[i].caps.iso = 1;
		config->ep_cfg_out[i].caps.mps = mps;
		config->ep_cfg_out[i].addr = USB_EP_DIR_OUT | i;
		err = udc_register_ep(dev, &config->ep_cfg_out[i]);
		if (err != 0) {
			LOG_ERR("Failed to register endpoint");
			return err;
		}
	}

	for (size_t i = num_out + 1U; i < config->num_of_eps; i++) {
		config->ep_cfg_in[i].caps.in = 1;
		config->ep_cfg_in[i].caps.bulk = 1;
		config->ep_cfg_in[i].caps.interrupt = 1;
		config->ep_cfg_in[i].caps.iso = 1;
		config->ep_cfg_in[i].caps.mps = mps;
		config->ep_cfg_in[i].addr = USB_EP_DIR_IN | i;
		err = udc_register_ep(dev, &config->ep_cfg_in[i]);
		if (err != 0) {
			LOG_ERR("Failed to register endpoint");
			return err;
		}
	}

	config->make_thread(dev);

	LOG_INF("Device %s (Full-Speed)", dev->name);

	return 0;
}

static void udc_bflb_v1_thread(void *dev, void *arg1, void *arg2)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);

	while (true) {
		bflb_thread_handler(dev);
	}
}

#define UDC_BFLB_V1_DEVICE_DEFINE(n)                                            \
	PINCTRL_DT_INST_DEFINE(n);                                              \
                                                                                \
	K_THREAD_STACK_DEFINE(udc_bflb_v1_stack_##n,                            \
			      CONFIG_UDC_BFLB_V1_STACK_SIZE);                   \
                                                                                \
	static void udc_bflb_v1_make_thread_##n(const struct device *dev)       \
	{                                                                       \
		struct udc_bflb_v1_data *priv = udc_get_private(dev);           \
                                                                                \
		k_thread_create(                                                \
			&priv->thread_data, udc_bflb_v1_stack_##n,              \
			K_THREAD_STACK_SIZEOF(udc_bflb_v1_stack_##n),           \
			udc_bflb_v1_thread, (void *)dev, NULL, NULL,            \
			K_PRIO_COOP(CONFIG_UDC_BFLB_V1_THREAD_PRIORITY),        \
			K_ESSENTIAL, K_NO_WAIT);                                \
		k_thread_name_set(&priv->thread_data, dev->name);               \
	}                                                                       \
                                                                                \
	static void udc_bflb_v1_irq_enable_##n(const struct device *dev)        \
	{                                                                       \
		ARG_UNUSED(dev);                                                \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),          \
			    udc_bflb_v1_isr, DEVICE_DT_INST_GET(n), 0);         \
		irq_enable(DT_INST_IRQN(n));                                    \
	}                                                                       \
                                                                                \
	static void udc_bflb_v1_irq_disable_##n(const struct device *dev)       \
	{                                                                       \
		ARG_UNUSED(dev);                                                \
		irq_disable(DT_INST_IRQN(n));                                   \
	}                                                                       \
                                                                                \
	static struct udc_ep_config ep_cfg_out_##n[BFLB_USB_NUM_ENDPOINTS];     \
	static struct udc_ep_config ep_cfg_in_##n[BFLB_USB_NUM_ENDPOINTS];      \
                                                                                \
	static const struct udc_bflb_v1_config udc_bflb_v1_config_##n = {       \
		.base = DT_INST_REG_ADDR(n),                                    \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                      \
		.num_of_eps = BFLB_USB_NUM_ENDPOINTS,                           \
		.ep_cfg_in = ep_cfg_in_##n,                                     \
		.ep_cfg_out = ep_cfg_out_##n,                                   \
		.make_thread = udc_bflb_v1_make_thread_##n,                     \
		.irq_enable_func = udc_bflb_v1_irq_enable_##n,                  \
		.irq_disable_func = udc_bflb_v1_irq_disable_##n,                \
	};                                                                      \
                                                                                \
	static struct udc_bflb_v1_data udc_priv_##n;                            \
                                                                                \
	static struct udc_data udc_data_##n = {                                 \
		.mutex = Z_MUTEX_INITIALIZER(udc_data_##n.mutex),               \
		.priv = &udc_priv_##n,                                          \
	};                                                                      \
                                                                                \
	DEVICE_DT_INST_DEFINE(n, udc_bflb_v1_driver_preinit, NULL,              \
			      &udc_data_##n, &udc_bflb_v1_config_##n,           \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,  \
			      &udc_bflb_v1_api);

DT_INST_FOREACH_STATUS_OKAY(UDC_BFLB_V1_DEVICE_DEFINE)

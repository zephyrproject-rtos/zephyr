/*
 * Copyright (c) 2026 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam_udp

#include "udc_common.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/atmel_sam_pmc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/byteorder.h>

#include <soc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(udc_sam_udp, CONFIG_UDC_DRIVER_LOG_LEVEL);

/*
 * USB Clock (UDPCK) Configuration
 *
 * The UDP peripheral requires a 48MHz clock derived from PLLB.
 * PLLB = 12MHz * 8 = 96MHz, USB = 96MHz / 2 = 48MHz
 *
 * CKGR_PLLBR: MULB = multiplier - 1, DIVB = divider
 * PMC_USB: USBDIV = divider - 1
 */
#define USB_PLLB_MUL		7	/* MULB: multiply by 8 */
#define USB_PLLB_DIV		1	/* DIVB: divide by 1 */
#define USB_PLLB_COUNT		0x3F	/* Lock counter */
#define USB_CLK_DIV		1	/* USBDIV: divide by 2 */

/* Number of hardware endpoints */
#define NUM_OF_HW_EPS		8
#define EP0_MPS			64

/* Endpoint types for UDP_CSR[EPTYPE] */
#define UDP_CSR_EPTYPE_CTRL	(0x0u << 8)
#define UDP_CSR_EPTYPE_ISO_OUT	(0x1u << 8)
#define UDP_CSR_EPTYPE_BULK_OUT	(0x2u << 8)
#define UDP_CSR_EPTYPE_INT_OUT	(0x3u << 8)
#define UDP_CSR_EPTYPE_ISO_IN	(0x5u << 8)
#define UDP_CSR_EPTYPE_BULK_IN	(0x6u << 8)
#define UDP_CSR_EPTYPE_INT_IN	(0x7u << 8)

/*
 * Hardware endpoint to logical endpoint mapping.
 *
 * SAM UDP hardware has 8 endpoints (EP0-EP7). EP0 is always
 * control. EP1-EP7 can be configured as either IN or OUT,
 * not both.
 *
 * We use odd/even allocation:
 *   - EP0: Control (IN and OUT)
 *   - EP1, EP3, EP5, EP7: IN  (0x81, 0x83, 0x85, 0x87)
 *   - EP2, EP4, EP6:      OUT (0x02, 0x04, 0x06)
 */
static const uint8_t in_ep_hw_map[] = {0, 1, 3, 5, 7};
static const uint8_t out_ep_hw_map[] = {0, 2, 4, 6};

/* Max packet sizes per hardware endpoint */
static const uint16_t ep_mps_map[] = {64, 64, 64, 64, 512, 512, 64, 64};

/*
 * Thread event types for ISR-to-thread communication.
 *
 * OUT data processing is done in thread context to allow yielding when
 * no buffer is available. This provides hardware flow control - the USB
 * FIFO stays full and the host receives NAK until a buffer becomes available.
 */
enum sam_udp_event_type {
	SAM_UDP_EVT_SETUP,
	SAM_UDP_EVT_XFER_NEXT,
	SAM_UDP_EVT_OUT_PENDING,
};

/*
 * ISR Debug Infrastructure
 * Tracks register state changes and detects ISR loops
 */
struct sam_udp_dev_dbg_state {
	uint32_t faddr;
	uint32_t glb_stat;
	uint32_t isr;
	uint32_t imr;
	uint32_t csr0;
	uint8_t repeat_count;
};

struct sam_udp_ep_dbg_state {
	uint32_t csr;
	uint8_t repeat_count;
};

static struct sam_udp_dev_dbg_state dev_dbg_state;
static struct sam_udp_ep_dbg_state ep_dbg_state[NUM_OF_HW_EPS];

static void sam_udp_isr_dbg_dev(Udp * const base, const uint32_t isr)
{
	uint32_t faddr = base->UDP_FADDR;
	uint32_t glb_stat = base->UDP_GLB_STAT;
	uint32_t imr = base->UDP_IMR;
	uint32_t csr0 = base->UDP_CSR[0];

	if (faddr != dev_dbg_state.faddr ||
	    glb_stat != dev_dbg_state.glb_stat ||
	    isr != dev_dbg_state.isr ||
	    imr != dev_dbg_state.imr ||
	    csr0 != dev_dbg_state.csr0) {
		LOG_DBG("DEV: FADDR=0x%08x GLB=0x%08x ISR=0x%08x IMR=0x%08x "
			"CSR0=0x%08x",
			faddr, glb_stat, isr, imr, csr0);

		dev_dbg_state.faddr = faddr;
		dev_dbg_state.glb_stat = glb_stat;
		dev_dbg_state.isr = isr;
		dev_dbg_state.imr = imr;
		dev_dbg_state.csr0 = csr0;
		dev_dbg_state.repeat_count = 0;
	} else {
		dev_dbg_state.repeat_count++;
		if (dev_dbg_state.repeat_count == 2) {
			LOG_WRN("DEV LOOP: ISR=0x%08x IMR=0x%08x CSR0=0x%08x",
				isr, imr, csr0);
		}
	}
}

static void sam_udp_isr_dbg_ep(Udp * const base, const uint8_t ep_idx)
{
	uint32_t csr = base->UDP_CSR[ep_idx];

	if (csr != ep_dbg_state[ep_idx].csr) {
		LOG_DBG("EP%d: CSR=0x%08x", ep_idx, csr);

		ep_dbg_state[ep_idx].csr = csr;
		ep_dbg_state[ep_idx].repeat_count = 0;
	} else {
		ep_dbg_state[ep_idx].repeat_count++;
		if (ep_dbg_state[ep_idx].repeat_count == 2) {
			LOG_WRN("EP%d LOOP: CSR=0x%08x", ep_idx, csr);
		}
	}
}

struct udc_sam_udp_ep_data {
	uint16_t mps;
	uint8_t next_bank;	/* For dual-bank OUT: 0=expect BK0, 1=expect BK1 */
	uint8_t tx_banks;	/* For dual-bank IN: number of banks filled (0-2) */
#if defined(CONFIG_UDC_DRIVER_LOG_LEVEL_DBG)
	uint32_t txcomp_count;	/* Debug: TXCOMP interrupt count */
	uint32_t fill_count;	/* Debug: bank fill count */
#endif
};

struct udc_sam_udp_data {
	struct k_thread thread_data;
	struct k_event events;
	atomic_t out_pending;
	struct udc_sam_udp_ep_data ep_data[NUM_OF_HW_EPS];
	uint8_t setup[8];
	bool suspended;
	bool bus_reset_done;
};

struct udc_sam_udp_config {
	Udp *base;
	const struct device *clock_dev;
	const struct atmel_sam_pmc_config *clock_cfg;
	const struct pinctrl_dev_config *pcfg;
	size_t num_in_eps;
	size_t num_out_eps;
	struct udc_ep_config *ep_cfg_in;
	struct udc_ep_config *ep_cfg_out;
	void (*irq_enable_func)(const struct device *dev);
	void (*irq_disable_func)(const struct device *dev);
	void (*make_thread)(const struct device *dev);
};

static inline Udp *udc_sam_udp_get_base(const struct device *dev)
{
	const struct udc_sam_udp_config *config = dev->config;

	return config->base;
}

/*
 * Convert endpoint address to bit position for atomic bitmask operations.
 * OUT endpoints use bits 0-7, IN endpoints use bits 8-15.
 */
static inline uint8_t ep_to_bit(const uint8_t ep)
{
	if (USB_EP_DIR_IS_IN(ep)) {
		return USB_EP_GET_IDX(ep) + 8;
	}

	return USB_EP_GET_IDX(ep);
}

/*
 * Extract next endpoint from bitmask and clear its bit.
 * Returns the endpoint address.
 */
static inline uint8_t bit_to_ep(uint32_t *eps)
{
	uint8_t bit = find_lsb_set(*eps) - 1;

	*eps &= ~BIT(bit);
	if (bit >= 8) {
		return USB_EP_DIR_IN | (bit - 8);
	}

	return bit;
}

/*
 * UDP CSR register access functions
 *
 * Due to synchronization between MCK and UDPCK, the software application
 * must wait for the end of the write operation before executing another
 * write by polling the bits which must be set/cleared.
 *
 * These bits are "write 1 to leave unchanged" - we must write 1 to preserve:
 *   RX_DATA_BK0, RX_DATA_BK1, RXSETUP, STALLSENT, TXCOMP
 */
#define UDP_CSR_NO_EFFECT_BITS (UDP_CSR_RX_DATA_BK0 | UDP_CSR_RX_DATA_BK1 | \
				UDP_CSR_RXSETUP | UDP_CSR_STALLSENT | \
				UDP_CSR_TXCOMP)

static inline void reset_endpoint(Udp * const base, const uint8_t hw_ep)
{
	base->UDP_RST_EP |= BIT(hw_ep);
	while (!(base->UDP_RST_EP & BIT(hw_ep))) {
		;
	}
	base->UDP_RST_EP &= ~BIT(hw_ep);
}

static inline void reset_all_endpoints(Udp * const base)
{
	/* Set reset bits, then clear them - no polling needed */
	base->UDP_RST_EP = 0xFF;
	base->UDP_RST_EP = 0;
}

/*
 * Check if endpoint has dual-bank (ping-pong) capability.
 * Per SAM4S datasheet Table 40-1:
 * - EP0: No dual-bank (control endpoint)
 * - EP3: No dual-bank (control/bulk/interrupt only)
 * - EP1, EP2, EP4-EP7: Have dual-bank capability
 */
static inline bool ep_has_dual_bank(const uint8_t hw_ep)
{
	return hw_ep != 0 && hw_ep != 3;
}

static inline void clear_csr_bits(Udp * const base, const uint8_t hw_ep,
				  const uint32_t bits)
{
	volatile uint32_t csr;

	csr = base->UDP_CSR[hw_ep];
	csr |= UDP_CSR_NO_EFFECT_BITS;
	csr &= ~bits;
	base->UDP_CSR[hw_ep] = csr;

	while ((base->UDP_CSR[hw_ep] & bits) != 0) {
		;
	}
}

static inline void set_csr_bits(Udp * const base, const uint8_t hw_ep,
				const uint32_t bits)
{
	volatile uint32_t csr;

	csr = base->UDP_CSR[hw_ep];
	csr |= UDP_CSR_NO_EFFECT_BITS;
	csr |= bits;
	base->UDP_CSR[hw_ep] = csr;

	while ((base->UDP_CSR[hw_ep] & bits) != bits) {
		;
	}
}

/*
 * Cancel pending transmit data per datasheet 40.6.2.5.
 *
 * For dual-bank endpoints with TXPKTRDY set, we must toggle
 * TXPKTRDY to properly clear both banks before reset:
 *   1. Clear TXPKTRDY, poll until 0
 *   2. Set TXPKTRDY, poll until 1
 *   3. Clear TXPKTRDY
 *   4. Reset endpoint
 *
 * For non dual-bank endpoints with TXPKTRDY set:
 *   1. Clear TXPKTRDY
 *   2. Reset endpoint
 *
 * If TXPKTRDY is not set, just reset the endpoint.
 */
static void cancel_pending_tx(Udp * const base, const uint8_t hw_ep)
{
	uint32_t csr = base->UDP_CSR[hw_ep];

	if (!(csr & UDP_CSR_TXPKTRDY)) {
		/* TXPKTRDY not set - just reset endpoint */
		reset_endpoint(base, hw_ep);
		return;
	}

	if (ep_has_dual_bank(hw_ep)) {
		/*
		 * Dual-bank endpoint with TXPKTRDY set:
		 * Toggle TXPKTRDY to clear both banks
		 */
		clear_csr_bits(base, hw_ep, UDP_CSR_TXPKTRDY);
		set_csr_bits(base, hw_ep, UDP_CSR_TXPKTRDY);
		clear_csr_bits(base, hw_ep, UDP_CSR_TXPKTRDY);
	} else {
		/* Non dual-bank endpoint - just clear TXPKTRDY */
		clear_csr_bits(base, hw_ep, UDP_CSR_TXPKTRDY);
	}

	reset_endpoint(base, hw_ep);
}

static void udc_sam_udp_write_fifo(Udp * const base, const uint8_t hw_ep,
				   const uint8_t *data, const uint16_t len)
{
	for (uint16_t i = 0; i < len; i++) {
		base->UDP_FDR[hw_ep] = data[i];
	}
}

/*
 * Fill one TX bank from the buffer and update tx_banks counter.
 * Returns the number of bytes written, or 0 if no data available.
 */
static uint16_t fill_tx_bank(const struct device *dev, const uint8_t hw_ep,
			     struct net_buf *buf)
{
	Udp *base = udc_sam_udp_get_base(dev);
	struct udc_sam_udp_data *priv = udc_get_private(dev);
	uint16_t len;

	if (buf == NULL || buf->len == 0) {
		return 0;
	}

	len = MIN(buf->len, priv->ep_data[hw_ep].mps);
	udc_sam_udp_write_fifo(base, hw_ep, buf->data, len);
	net_buf_pull(buf, len);
	priv->ep_data[hw_ep].tx_banks++;

#if defined(CONFIG_UDC_DRIVER_LOG_LEVEL_DBG)
	priv->ep_data[hw_ep].fill_count++;
#endif

	return len;
}

static uint16_t udc_sam_udp_read_fifo(Udp * const base, const uint8_t hw_ep,
				      uint8_t *data, const uint16_t max_len)
{
	uint16_t count = (base->UDP_CSR[hw_ep] & UDP_CSR_RXBYTECNT_Msk) >>
			 UDP_CSR_RXBYTECNT_Pos;

	count = MIN(count, max_len);

	for (uint16_t i = 0; i < count; i++) {
		data[i] = base->UDP_FDR[hw_ep];
	}

	return count;
}

/*
 * USB Clock Configuration
 *
 * Per datasheet, the clock enable sequence is:
 * 1. Enable PLLB (if not already enabled)
 * 2. Configure PMC_USB to select PLLB and set divider
 * 3. Enable UDP peripheral clock (MCK)
 * 4. Enable UDPCK (48MHz USB clock)
 *
 * Note: PMC_SCER requires write protection to be disabled.
 * PLLB registers do not require write protection per datasheet.
 */
static int udc_sam_udp_enable_usb_clock(const struct device *dev)
{
	const struct udc_sam_udp_config *config = dev->config;
	int ret;

	/* Enable PLLB if not already locked */
	if (!soc_pmc_is_locked_pllbck()) {
		soc_pmc_enable_pllbck(USB_PLLB_MUL, USB_PLLB_COUNT,
				      USB_PLLB_DIV);
	}

	/* Configure USB clock: select PLLB, divide by 2 -> 48MHz */
	PMC->PMC_USB = PMC_USB_USBS | PMC_USB_USBDIV(USB_CLK_DIV);

	/* Enable UDP peripheral clock (MCK for UDP) */
	ret = clock_control_on(config->clock_dev,
			       (clock_control_subsys_t)config->clock_cfg);
	if (ret != 0) {
		LOG_ERR("Failed to enable peripheral clock: %d", ret);
		return ret;
	}

	/* Disable write protection for PMC_SCER */
	PMC->PMC_WPMR = PMC_WPMR_WPKEY_PASSWD;

	/* Enable UDPCK (48MHz USB clock) */
	PMC->PMC_SCER = PMC_SCER_UDP;

	if (!(PMC->PMC_SCSR & PMC_SCSR_UDP)) {
		LOG_ERR("Failed to enable UDPCK");
		return -EIO;
	}

	LOG_DBG("USB clock enabled: PLLB->96MHz, UDPCK->48MHz");

	return 0;
}

static void udc_sam_udp_disable_usb_clock(const struct device *dev)
{
	const struct udc_sam_udp_config *config = dev->config;

	/* Disable write protection */
	PMC->PMC_WPMR = PMC_WPMR_WPKEY_PASSWD;

	/* Disable UDPCK */
	PMC->PMC_SCDR = PMC_SCDR_UDP;

	/* Disable peripheral clock */
	clock_control_off(config->clock_dev,
			  (clock_control_subsys_t)config->clock_cfg);

	LOG_DBG("USB clock disabled");
}

/*
 * Resume Clock Management
 *
 * Per datasheet 40.6.3.7, MCK must be enabled BEFORE any UDP register
 * access (including clearing WAKEUP in UDP_ICR).
 */
static void udc_sam_udp_resume_clocks(const struct device *dev)
{
	const struct udc_sam_udp_config *config = dev->config;

	/*
	 * This function is idempotent - safe to call if clocks already enabled.
	 * clock_control_on() handles already-enabled case gracefully.
	 */

	/* Enable MCK for UDP first (must be before any UDP register access) */
	clock_control_on(config->clock_dev,
			 (clock_control_subsys_t)config->clock_cfg);

	/* Disable write protection */
	PMC->PMC_WPMR = PMC_WPMR_WPKEY_PASSWD;

	/* Enable UDPCK (PMC_SCER is write-only, safe to set again) */
	PMC->PMC_SCER = PMC_SCER_UDP;

	LOG_DBG("USB clocks resumed");
}

/*
 * Endpoint operations
 */
static int udc_sam_udp_ep_enqueue(const struct device *dev,
				  struct udc_ep_config *const cfg,
				  struct net_buf *const buf)
{
	struct udc_sam_udp_data *priv = udc_get_private(dev);

	udc_buf_put(cfg, buf);

	if (cfg->stat.halted) {
		LOG_DBG("ep 0x%02x halted, queued only", cfg->addr);
		return 0;
	}

	if (USB_EP_DIR_IS_IN(cfg->addr)) {
		if (buf == udc_buf_peek(cfg)) {
			Udp *base = udc_sam_udp_get_base(dev);
			uint8_t hw_ep = USB_EP_GET_IDX(cfg->addr);
			uint16_t len;

			/* Fill first bank and mark ready */
			len = fill_tx_bank(dev, hw_ep, buf);
			set_csr_bits(base, hw_ep, UDP_CSR_TXPKTRDY);
			LOG_DBG("IN ep 0x%02x bank0: %u bytes", cfg->addr, len);

			/*
			 * For dual-bank endpoints, pre-fill second bank while
			 * first is being sent. Per datasheet 40.6.2.2, set
			 * TXPKTRDY again to tell hardware bank 1 is ready.
			 * This enables back-to-back TX without gaps.
			 */
			if (ep_has_dual_bank(hw_ep) && buf->len > 0) {
				len = fill_tx_bank(dev, hw_ep, buf);
				set_csr_bits(base, hw_ep, UDP_CSR_TXPKTRDY);
				LOG_DBG("IN ep 0x%02x bank1: %u bytes",
					cfg->addr, len);
			}
		}
	} else {
		/*
		 * Buffer queued for OUT endpoint. If there's pending data
		 * waiting (thread was NAKing due to no buffer), wake up the
		 * thread to process it now.
		 */
		if (atomic_test_bit(&priv->out_pending, ep_to_bit(cfg->addr))) {
			k_event_post(&priv->events, BIT(SAM_UDP_EVT_OUT_PENDING));
		}
	}

	return 0;
}

static int udc_sam_udp_ep_dequeue(const struct device *dev,
				  struct udc_ep_config *const cfg)
{
	Udp *base = udc_sam_udp_get_base(dev);
	uint8_t hw_ep = USB_EP_GET_IDX(cfg->addr);
	struct net_buf *buf;

	/* Cancel pending TX data for IN endpoints per datasheet 40.6.2.5 */
	if (USB_EP_DIR_IS_IN(cfg->addr)) {
		struct udc_sam_udp_data *priv = udc_get_private(dev);

		cancel_pending_tx(base, hw_ep);
		priv->ep_data[hw_ep].tx_banks = 0;
	}
	/*
	 * For OUT endpoints, don't touch out_pending or IRQ state here.
	 * The pending bit and IRQ will self-correct through normal flow:
	 * - If data in FIFO and IRQ enabled: ISR will handle it
	 * - If data in FIFO and IRQ disabled: pending bit is set,
	 *   next ep_enqueue will wake thread to process
	 * Clearing/re-enabling here causes extra thread wake-ups in
	 * rapid queue/cancel cycles (unlink tests).
	 */

	buf = udc_buf_get_all(cfg);
	if (buf) {
		udc_submit_ep_event(dev, buf, -ECONNABORTED);
	}

	return 0;
}

static int udc_sam_udp_ep_set_halt(const struct device *dev,
				   struct udc_ep_config *const cfg)
{
	Udp *base = udc_sam_udp_get_base(dev);
	uint8_t hw_ep = USB_EP_GET_IDX(cfg->addr);

	LOG_DBG("Set halt ep 0x%02x", cfg->addr);

	set_csr_bits(base, hw_ep, UDP_CSR_FORCESTALL);

	if (hw_ep != 0) {
		cfg->stat.halted = true;
	}

	return 0;
}

static int udc_sam_udp_ep_clear_halt(const struct device *dev,
				     struct udc_ep_config *const cfg)
{
	Udp *base = udc_sam_udp_get_base(dev);
	struct udc_sam_udp_data *priv = udc_get_private(dev);
	uint8_t hw_ep = USB_EP_GET_IDX(cfg->addr);
	struct net_buf *buf;

	if (hw_ep == 0) {
		return 0;
	}

	LOG_DBG("Clear halt ep 0x%02x", cfg->addr);

	clear_csr_bits(base, hw_ep, UDP_CSR_FORCESTALL);
	clear_csr_bits(base, hw_ep, UDP_CSR_STALLSENT);

	if (USB_EP_DIR_IS_IN(cfg->addr)) {
		/* Cancel pending TX data per datasheet 40.6.2.5 */
		cancel_pending_tx(base, hw_ep);
		priv->ep_data[hw_ep].tx_banks = 0;
	} else {
		/*
		 * Clear stale out_pending bit from before the halt. This
		 * prevents the thread from trying to process old state.
		 * Do not reset_endpoint() here as it caused test 29 timeouts.
		 */
		atomic_clear_bit(&priv->out_pending, ep_to_bit(cfg->addr));
		priv->ep_data[hw_ep].next_bank = 0;
	}

	cfg->stat.halted = false;

	/*
	 * Re-enable endpoint interrupt. It may have been disabled by the ISR
	 * when previous OUT data was received before the endpoint was halted.
	 */
	base->UDP_IER = BIT(hw_ep);

	/* Resume queued transfers if any */
	buf = udc_buf_peek(cfg);
	if (buf != NULL && USB_EP_DIR_IS_IN(cfg->addr)) {
		uint16_t len;

		/* Fill first bank and mark ready */
		len = fill_tx_bank(dev, hw_ep, buf);
		set_csr_bits(base, hw_ep, UDP_CSR_TXPKTRDY);
		LOG_DBG("Resumed IN ep 0x%02x: %u bytes", cfg->addr, len);

		/* Pre-fill second bank for dual-bank endpoints */
		if (ep_has_dual_bank(hw_ep) && buf->len > 0) {
			len = fill_tx_bank(dev, hw_ep, buf);
			set_csr_bits(base, hw_ep, UDP_CSR_TXPKTRDY);
			LOG_DBG("IN ep 0x%02x pre-fill: %u bytes", cfg->addr, len);
		}
	}
	/* OUT endpoints will receive data via interrupt */

	return 0;
}

static int udc_sam_udp_ep_enable(const struct device *dev,
				 struct udc_ep_config *const cfg)
{
	Udp *base = udc_sam_udp_get_base(dev);
	struct udc_sam_udp_data *priv = udc_get_private(dev);
	uint8_t hw_ep = USB_EP_GET_IDX(cfg->addr);
	uint32_t ep_type;
	bool is_in = USB_EP_DIR_IS_IN(cfg->addr);

	LOG_DBG("Enable ep 0x%02x (hw_ep=%d, type=%d, mps=%d)",
		cfg->addr, hw_ep,
		cfg->attributes & USB_EP_TRANSFER_TYPE_MASK,
		cfg->mps);

	priv->ep_data[hw_ep].mps = cfg->mps;

	/* Initialize bank alternation counter for OUT endpoints */
	if (!is_in) {
		priv->ep_data[hw_ep].next_bank = 0;
	}

	/*
	 * Per datasheet 40.6.3.3: EP0 hardware configuration must only
	 * happen AFTER ENDBUSRES. If called before bus reset (during
	 * enable()), defer hardware configuration - the ENDBUSRES handler
	 * will configure EP0. Return success so stack state is set.
	 */
	if (hw_ep == 0 && !priv->bus_reset_done) {
		LOG_DBG("EP0 hardware config deferred until ENDBUSRES");

		return 0;
	}

	switch (cfg->attributes & USB_EP_TRANSFER_TYPE_MASK) {
	case USB_EP_TYPE_CONTROL:
		ep_type = UDP_CSR_EPTYPE_CTRL;
		break;
	case USB_EP_TYPE_ISO:
		ep_type = is_in ? UDP_CSR_EPTYPE_ISO_IN : UDP_CSR_EPTYPE_ISO_OUT;
		break;
	case USB_EP_TYPE_BULK:
		ep_type = is_in ? UDP_CSR_EPTYPE_BULK_IN : UDP_CSR_EPTYPE_BULK_OUT;
		break;
	case USB_EP_TYPE_INTERRUPT:
		ep_type = is_in ? UDP_CSR_EPTYPE_INT_IN : UDP_CSR_EPTYPE_INT_OUT;
		break;
	default:
		return -EINVAL;
	}

	set_csr_bits(base, hw_ep, ep_type | UDP_CSR_EPEDS);

	base->UDP_IER = BIT(hw_ep);

	LOG_DBG("EP%d enabled: CSR=0x%08x", hw_ep, base->UDP_CSR[hw_ep]);

	return 0;
}

static int udc_sam_udp_ep_disable(const struct device *dev,
				  struct udc_ep_config *const cfg)
{
	Udp *base = udc_sam_udp_get_base(dev);
	struct udc_sam_udp_data *priv = udc_get_private(dev);
	uint8_t hw_ep = USB_EP_GET_IDX(cfg->addr);

	LOG_DBG("Disable ep 0x%02x", cfg->addr);

	base->UDP_IDR = BIT(hw_ep);

	clear_csr_bits(base, hw_ep, UDP_CSR_EPEDS);

	/* Cancel pending TX data for IN endpoints per datasheet 40.6.2.5 */
	if (USB_EP_DIR_IS_IN(cfg->addr)) {
		cancel_pending_tx(base, hw_ep);
	} else {
		/*
		 * Clear pending OUT state. The endpoint is being disabled
		 * so any pending data is no longer valid.
		 */
		atomic_clear_bit(&priv->out_pending, ep_to_bit(cfg->addr));
		priv->ep_data[hw_ep].next_bank = 0;
		reset_endpoint(base, hw_ep);
	}

	udc_sam_udp_ep_dequeue(dev, cfg);

	return 0;
}

static int udc_sam_udp_set_address(const struct device *dev, const uint8_t addr)
{
	Udp *base = udc_sam_udp_get_base(dev);

	LOG_DBG("Set address %u", addr);

	/*
	 * Per datasheet 40.6.3.4: The address must be set after the status
	 * stage completes (TXCOMP received and cleared). The USB stack calls
	 * this function after the status transaction, so we apply immediately.
	 */
	base->UDP_FADDR = UDP_FADDR_FEN | UDP_FADDR_FADD(addr);

	if (addr != 0) {
		base->UDP_GLB_STAT = UDP_GLB_STAT_FADDEN;
	} else {
		base->UDP_GLB_STAT = 0;
	}

	return 0;
}

static enum udc_bus_speed udc_sam_udp_device_speed(const struct device *dev)
{
	return UDC_BUS_SPEED_FS;
}

/*
 * Remote Wakeup (per datasheet 40.6.3.8)
 *
 * Before sending a K state to the host:
 * 1. MCK, UDPCK and transceiver must be enabled
 * 2. Set RMWUPE bit in UDP_GLB_STAT
 * 3. Toggle ESR bit (0->1) to force K state
 *
 * The K state is automatically generated and released per USB 2.0 spec.
 *
 * Note: The device must wait at least 5ms after entering suspend before
 * sending an external resume. This timing is handled by the USB stack.
 */
static int udc_sam_udp_host_wakeup(const struct device *dev)
{
	struct udc_sam_udp_data *priv = udc_get_private(dev);
	Udp *base = udc_sam_udp_get_base(dev);

	LOG_DBG("Remote wakeup");

	/*
	 * If suspended, we need to re-enable clocks and transceiver first.
	 * Per datasheet 40.6.3.8, MCK, UDPCK and transceiver must be enabled
	 * before forcing the K state.
	 */
	if (priv->suspended) {
		/* Re-enable MCK and UDPCK */
		udc_sam_udp_resume_clocks(dev);

		/* Enable transceiver */
		base->UDP_TXVC = UDP_TXVC_PUON;
	}

	/* Enable remote wakeup feature */
	base->UDP_GLB_STAT |= UDP_GLB_STAT_RMWUPE;

	/* Force K state by toggling ESR: clear first, then set */
	base->UDP_GLB_STAT &= ~UDP_GLB_STAT_ESR;
	base->UDP_GLB_STAT |= UDP_GLB_STAT_ESR;

	return 0;
}

static int udc_sam_udp_enable(const struct device *dev)
{
	const struct udc_sam_udp_config *config = dev->config;
	struct udc_sam_udp_data *priv = udc_get_private(dev);
	Udp *base = config->base;
	int ret;

	LOG_DBG("Enable controller");

	/*
	 * Per datasheet 40.6.3.2 (Entering Attached State):
	 * - MCK and UDPCK must be enabled
	 * - Enable pull-up (PUON)
	 * - Transceiver can remain DISABLED until ENDBUSRES
	 *
	 * Note: EP0 hardware configuration is deferred to ENDBUSRES handler.
	 * The ep_enable callback checks bus_reset_done and skips hardware
	 * writes if false, so calling udc_ep_enable_internal() here only
	 * sets stack state (cfg->stat.enabled) without touching hardware.
	 */

	/* Initialize state */
	priv->ep_data[0].mps = EP0_MPS;
	priv->suspended = false;
	priv->bus_reset_done = false;

	/*
	 * Enable control endpoints in the USB stack. This sets
	 * cfg->stat.enabled so the stack can process SETUP packets.
	 * Hardware configuration is deferred to ENDBUSRES handler.
	 */
	ret = udc_ep_enable_internal(dev, USB_CONTROL_EP_OUT,
				     USB_EP_TYPE_CONTROL, EP0_MPS, 0);
	if (ret) {
		LOG_ERR("Failed to enable control OUT endpoint");

		return ret;
	}

	ret = udc_ep_enable_internal(dev, USB_CONTROL_EP_IN,
				     USB_EP_TYPE_CONTROL, EP0_MPS, 0);
	if (ret) {
		LOG_ERR("Failed to enable control IN endpoint");

		return ret;
	}

	/* Clear all pending interrupts */
	base->UDP_ICR = 0xFFFFFFFF;

	/* Enable ENDBUSRES interrupt - we'll configure EP0 and
	 * enable transceiver after receiving bus reset
	 */
	base->UDP_IER = UDP_ISR_ENDBUSRES | UDP_ISR_RXSUSP | UDP_ISR_WAKEUP |
			UDP_ISR_RXRSM | UDP_ISR_EXTRSM;

	/* Enable IRQ */
	config->irq_enable_func(dev);

	/* Attach device (enable pull-up) - transceiver stays disabled */
	base->UDP_TXVC = UDP_TXVC_PUON | UDP_TXVC_TXVDIS;

	LOG_DBG("UDP_TXVC: 0x%08x", base->UDP_TXVC);
	LOG_INF("USB attached, waiting for bus reset");

	return 0;
}

static int udc_sam_udp_disable(const struct device *dev)
{
	const struct udc_sam_udp_config *config = dev->config;
	Udp *base = config->base;

	LOG_DBG("Disable controller");

	/* Disable interrupts first */
	base->UDP_IDR = 0xFFFFFFFF;

	/* Disable function */
	base->UDP_FADDR = 0;

	/*
	 * Per datasheet 40.5.2: To prevent overconsumption from floating
	 * DDP/DDM lines, disable transceiver FIRST (enables internal
	 * pull-down), THEN remove pull-up.
	 */
	base->UDP_TXVC |= UDP_TXVC_TXVDIS;
	base->UDP_TXVC &= ~UDP_TXVC_PUON;

	/* Disable IRQ */
	config->irq_disable_func(dev);

	return 0;
}

static int udc_sam_udp_init(const struct device *dev)
{
	const struct udc_sam_udp_config *config = dev->config;
	Udp *base = config->base;
	int ret;

	LOG_DBG("Init controller");

	/* Enable USB clock early - needed before any UDP register access */
	ret = udc_sam_udp_enable_usb_clock(dev);
	if (ret != 0) {
		LOG_ERR("Failed to enable USB clock: %d", ret);

		return ret;
	}

	/* Configure pins */
	if (config->pcfg != NULL) {
		ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
		if (ret != 0) {
			LOG_ERR("Failed to configure pins: %d", ret);

			return ret;
		}
	}

	/* Disable transceiver */
	base->UDP_TXVC = UDP_TXVC_TXVDIS;

	/* Reset all endpoints */
	reset_all_endpoints(base);

	/* Disable all interrupts */
	base->UDP_IDR = 0xFFFFFFFF;
	base->UDP_ICR = 0xFFFFFFFF;

	/* Disable device */
	base->UDP_FADDR = 0;
	base->UDP_GLB_STAT = 0;

	LOG_DBG("Hardware reset complete");

	return 0;
}

static int udc_sam_udp_shutdown(const struct device *dev)
{
	Udp *base = udc_sam_udp_get_base(dev);

	LOG_DBG("Shutdown");

	/* Disable control endpoints */
	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_OUT)) {
		LOG_ERR("Failed to disable control OUT endpoint");
	}
	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_IN)) {
		LOG_ERR("Failed to disable control IN endpoint");
	}

	udc_sam_udp_disable(dev);

	reset_all_endpoints(base);

	/* Disable USB clocks (UDPCK and peripheral clock) */
	udc_sam_udp_disable_usb_clock(dev);

	/* Disable PLLB */
	soc_pmc_disable_pllbck();

	return 0;
}

static void udc_sam_udp_lock(const struct device *dev)
{
	udc_lock_internal(dev, K_FOREVER);
}

static void udc_sam_udp_unlock(const struct device *dev)
{
	udc_unlock_internal(dev);
}

/*
 * Control transfer handling
 */
static void udc_sam_udp_drop_ctrl_transfers(const struct device *dev)
{
	struct net_buf *buf;

	buf = udc_buf_get_all(udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT));
	if (buf != NULL) {
		net_buf_unref(buf);
	}

	buf = udc_buf_get_all(udc_get_ep_cfg(dev, USB_CONTROL_EP_IN));
	if (buf != NULL) {
		net_buf_unref(buf);
	}
}

static int udc_sam_udp_ctrl_feed_dout(const struct device *dev,
				      const size_t length)
{
	struct udc_ep_config *const ep_cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
	struct net_buf *buf;

	buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, length);
	if (buf == NULL) {
		return -ENOMEM;
	}

	udc_buf_put(ep_cfg, buf);

	return 0;
}

/*
 * ISR handler for SETUP packets - called from ISR context.
 *
 * Per datasheet 40.6.2.1: RXSETUP cannot be cleared before the setup packet
 * has been read from the FIFO. DIR bit must also be set before clearing
 * RXSETUP for IN transfers.
 *
 * This function reads the SETUP data and prepares for thread processing.
 * Buffer allocation is deferred to thread context to allow yielding.
 */
static void udc_sam_udp_isr_handle_setup(const struct device *dev)
{
	Udp *base = udc_sam_udp_get_base(dev);
	struct udc_sam_udp_data *priv = udc_get_private(dev);

	udc_sam_udp_drop_ctrl_transfers(dev);

	/* Read SETUP packet from FIFO - must be done before clearing RXSETUP */
	for (int i = 0; i < 8; i++) {
		priv->setup[i] = base->UDP_FDR[0];
	}

	LOG_HEXDUMP_DBG(priv->setup, 8, "setup");

	/*
	 * Per datasheet 40.6.2.1: DIR bit must be set before clearing
	 * RXSETUP. It switches EP0 to IN mode for the data phase,
	 * allowing the controller to send data to the host. Without
	 * this, EP0 stays in OUT mode and the IN data phase fails.
	 */
	if (priv->setup[0] & USB_EP_DIR_IN) {
		set_csr_bits(base, 0, UDP_CSR_DIR);
	}

	/* Clear RXSETUP after reading FIFO and setting DIR */
	clear_csr_bits(base, 0, UDP_CSR_RXSETUP);

	LOG_DBG("RXSETUP cleared, CSR=0x%08x", base->UDP_CSR[0]);

	/* Signal thread to process SETUP with buffer allocation */
	k_event_post(&priv->events, BIT(SAM_UDP_EVT_SETUP));
}

/*
 * Thread handler for SETUP packets - called from thread context.
 */
static int udc_sam_udp_thread_handle_setup(const struct device *dev)
{
	struct udc_sam_udp_data *priv = udc_get_private(dev);
	struct net_buf *buf;
	int err;

	udc_sam_udp_drop_ctrl_transfers(dev);

	buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, 8);
	if (buf == NULL) {
		return -ENOMEM;
	}

	net_buf_add_mem(buf, priv->setup, 8);
	udc_ep_buf_set_setup(buf);

	udc_ctrl_update_stage(dev, buf);

	if (udc_ctrl_stage_is_data_out(dev)) {
		LOG_DBG("s:%p|feed for -out-", (void *)buf);
		err = udc_sam_udp_ctrl_feed_dout(dev, udc_data_stage_length(buf));
		if (err == -ENOMEM) {
			udc_submit_ep_event(dev, buf, err);
		}
	} else if (udc_ctrl_stage_is_data_in(dev)) {
		LOG_DBG("s:%p|feed for -in-status", (void *)buf);
		err = udc_ctrl_submit_s_in_status(dev);
	} else {
		LOG_DBG("s:%p|no data", (void *)buf);
		err = udc_ctrl_submit_s_status(dev);
	}

	return err;
}

/*
 * Handle IN (TX) endpoint completion.
 *
 * For dual-bank endpoints, we use ping-pong buffering to achieve back-to-back
 * transmissions. While the host reads from one bank, we fill the other bank.
 * The tx_banks counter tracks how many banks have data waiting to be sent.
 *
 * Per datasheet 40.6.2.2: "TX_COMP must be cleared after TX_PKTRDY has been set."
 */
static void udc_sam_udp_handle_in(const struct device *dev, const uint8_t ep)
{
	Udp *base = udc_sam_udp_get_base(dev);
	struct udc_sam_udp_data *priv = udc_get_private(dev);
	struct udc_ep_config *cfg = udc_get_ep_cfg(dev, ep);
	uint8_t hw_ep = USB_EP_GET_IDX(ep);
	struct net_buf *buf;
	uint8_t max_banks = ep_has_dual_bank(hw_ep) ? 2 : 1;
	int err;

#if defined(CONFIG_UDC_DRIVER_LOG_LEVEL_DBG)
	/* Throughput measurement - only when debug logging enabled */
	if (hw_ep != 0) {
		static uint32_t txcomp_count;
		static uint32_t start_time;

		if (txcomp_count == 0) {
			start_time = k_uptime_get_32();
		}
		txcomp_count++;
		if ((txcomp_count % 10000) == 0) {
			uint32_t elapsed = k_uptime_get_32() - start_time;

			LOG_DBG("EP%d: %u TXCOMP in %u ms = %u KB/s",
				hw_ep, txcomp_count, elapsed,
				elapsed ? ((txcomp_count * 64) / elapsed) : 0);
		}
	}
#endif

	/* TXCOMP means a packet was sent - one bank is now free */
	if (priv->ep_data[hw_ep].tx_banks > 0) {
		priv->ep_data[hw_ep].tx_banks--;
	}

	/*
	 * If there's a pre-filled bank waiting, commit it now.
	 * Set TXPKTRDY BEFORE clearing TXCOMP per datasheet 40.6.2.2.
	 */
	if (priv->ep_data[hw_ep].tx_banks > 0) {
		set_csr_bits(base, hw_ep, UDP_CSR_TXPKTRDY);
		LOG_DBG("IN ep 0x%02x: commit pre-filled bank", ep);
	}

	buf = udc_buf_peek(cfg);

	/* Fill available banks with more data */
	while (buf != NULL && buf->len > 0 &&
	       priv->ep_data[hw_ep].tx_banks < max_banks) {
		uint16_t len = fill_tx_bank(dev, hw_ep, buf);

		/*
		 * Per datasheet 40.6.2.2, set TXPKTRDY after each bank fill
		 * to tell hardware the bank is ready for transmission.
		 */
		set_csr_bits(base, hw_ep, UDP_CSR_TXPKTRDY);
		LOG_DBG("IN ep 0x%02x: filled bank, %u bytes", ep, len);
	}

	/*
	 * Check if current buffer is complete (all data pulled and sent).
	 * Buffer completes when buf->len == 0 AND tx_banks == 0.
	 */
	while (buf != NULL && buf->len == 0 &&
	       priv->ep_data[hw_ep].tx_banks == 0) {
		buf = udc_buf_get(cfg);

		if (ep == USB_CONTROL_EP_IN) {
			if (udc_ctrl_stage_is_status_in(dev) ||
			    udc_ctrl_stage_is_no_data(dev)) {
				LOG_DBG("IN: status stage complete");
				udc_ctrl_submit_status(dev, buf);
			}

			udc_ctrl_update_stage(dev, buf);

			if (udc_ctrl_stage_is_status_out(dev)) {
				LOG_DBG("IN: feeding for status OUT");
				net_buf_unref(buf);
				err = udc_sam_udp_ctrl_feed_dout(dev, 0);
				if (err != 0) {
					LOG_ERR("Failed to feed ctrl dout: %d",
						err);
				}
				buf = NULL;
				break;
			}
		} else {
			udc_submit_ep_event(dev, buf, 0);
		}

		/* Check for next buffer and pre-fill if dual-bank */
		buf = udc_buf_peek(cfg);
		while (buf != NULL && buf->len > 0 &&
		       priv->ep_data[hw_ep].tx_banks < max_banks) {
			uint16_t len = fill_tx_bank(dev, hw_ep, buf);

			set_csr_bits(base, hw_ep, UDP_CSR_TXPKTRDY);
			LOG_DBG("IN ep 0x%02x: next buf, %u bytes", ep, len);
		}
	}

	/* Clear TXCOMP after TXPKTRDY per datasheet 40.6.2.2 */
	clear_csr_bits(base, hw_ep, UDP_CSR_TXCOMP);
}

/*
 * Determine which bank to process for dual-bank OUT endpoints.
 * Returns the bank flag (RX_DATA_BK0 or RX_DATA_BK1), or 0 if no data.
 *
 * Per datasheet 40.6.2.3: "When RX_DATA_BK0 and RX_DATA_BK1 are both set,
 * there is no way to determine which one to clear first. Thus the software
 * must keep an internal counter to clear alternatively RX_DATA_BK0 then
 * RX_DATA_BK1."
 */
static uint32_t sam_udp_get_out_bank(struct udc_sam_udp_data *const priv,
				     const uint8_t hw_ep, const uint32_t csr)
{
	uint8_t next_bank = priv->ep_data[hw_ep].next_bank;
	uint32_t bank_flag;

	if (next_bank == 0) {
		bank_flag = UDP_CSR_RX_DATA_BK0;
		if (!(csr & bank_flag)) {
			bank_flag = UDP_CSR_RX_DATA_BK1;
			if (!(csr & bank_flag)) {
				return 0;
			}
		}
	} else {
		bank_flag = UDP_CSR_RX_DATA_BK1;
		if (!(csr & bank_flag)) {
			bank_flag = UDP_CSR_RX_DATA_BK0;
			if (!(csr & bank_flag)) {
				return 0;
			}
		}
	}

	return bank_flag;
}

/*
 * Update bank alternation counter after processing a bank.
 */
static void sam_udp_update_next_bank(struct udc_sam_udp_data *const priv,
				     const uint8_t hw_ep,
				     const uint32_t bank_flag)
{
	if (bank_flag == UDP_CSR_RX_DATA_BK0) {
		priv->ep_data[hw_ep].next_bank = 1;
	} else {
		priv->ep_data[hw_ep].next_bank = 0;
	}
}

/*
 * ISR handler for bulk/interrupt OUT endpoints.
 *
 * Process OUT data directly in ISR for better throughput. Only defers to
 * thread if no buffer is available (NAK flow control).
 */
static void sam_udp_handle_out_isr(const struct device *dev,
				   const uint8_t hw_ep)
{
	Udp *base = udc_sam_udp_get_base(dev);
	struct udc_sam_udp_data *priv = udc_get_private(dev);
	uint8_t ep = USB_EP_DIR_OUT | hw_ep;
	struct udc_ep_config *cfg = udc_get_ep_cfg(dev, ep);
	struct net_buf *buf;
	uint32_t csr;
	uint32_t bank_flag;
	uint16_t len;

	csr = base->UDP_CSR[hw_ep];
	bank_flag = sam_udp_get_out_bank(priv, hw_ep, csr);

	if (bank_flag == 0) {
		return;
	}

	buf = udc_buf_peek(cfg);
	if (buf == NULL) {
		/*
		 * No buffer available. Disable interrupt and defer to thread.
		 * Thread will be signaled when buffer becomes available via
		 * ep_enqueue, or we signal it now to set the pending bit.
		 */
		LOG_DBG("ISR OUT ep 0x%02x no buffer - defer", ep);
		base->UDP_IDR = BIT(hw_ep);
		atomic_set_bit(&priv->out_pending, ep_to_bit(ep));
		k_event_post(&priv->events, BIT(SAM_UDP_EVT_OUT_PENDING));
		return;
	}

	/* Read data from FIFO */
	len = udc_sam_udp_read_fifo(base, hw_ep, net_buf_tail(buf),
				    net_buf_tailroom(buf));
	net_buf_add(buf, len);

	LOG_DBG("ISR OUT ep 0x%02x len %u bank %d",
		ep, len, (bank_flag == UDP_CSR_RX_DATA_BK0) ? 0 : 1);

	/* Clear the bank flag and update alternation counter */
	clear_csr_bits(base, hw_ep, bank_flag);
	sam_udp_update_next_bank(priv, hw_ep, bank_flag);

	/* Check if transfer complete (short packet or buffer full) */
	if (len < priv->ep_data[hw_ep].mps || net_buf_tailroom(buf) == 0) {
		buf = udc_buf_get(cfg);
		udc_submit_ep_event(dev, buf, 0);
	}
}

/*
 * Process pending OUT data in thread context (for EP0 and deferred cases).
 *
 * This function is called from the thread when the ISR signals that OUT data
 * is available. If no buffer is available, the data stays in the FIFO and
 * the host will receive NAK (hardware flow control).
 *
 * Returns 0 on success, -ENOBUFS if no buffer available (data stays in FIFO).
 */
static int sam_udp_process_pending_out(const struct device *dev,
				       const uint8_t ep)
{
	Udp *base = udc_sam_udp_get_base(dev);
	struct udc_sam_udp_data *priv = udc_get_private(dev);
	struct udc_ep_config *cfg = udc_get_ep_cfg(dev, ep);
	uint8_t hw_ep = USB_EP_GET_IDX(ep);
	struct net_buf *buf;
	uint32_t csr;
	uint32_t bank_flag;
	uint16_t len;
	int err;

	csr = base->UDP_CSR[hw_ep];
	bank_flag = sam_udp_get_out_bank(priv, hw_ep, csr);

	if (bank_flag == 0) {
		LOG_DBG("OUT ep 0x%02x no data in bank", ep);
		base->UDP_IER = BIT(hw_ep);

		return 0;
	}

	buf = udc_buf_peek(cfg);
	if (buf == NULL) {
		/*
		 * No buffer available. Leave data in FIFO - the hardware will
		 * NAK the host until we can process this data. Keep the
		 * endpoint interrupt disabled to avoid ISR loop.
		 */
		LOG_DBG("OUT ep 0x%02x no buffer - NAK until ready", ep);

		return -ENOBUFS;
	}

	/* Read data from FIFO */
	len = udc_sam_udp_read_fifo(base, hw_ep, net_buf_tail(buf),
				    net_buf_tailroom(buf));
	net_buf_add(buf, len);

	LOG_DBG("Thread OUT ep 0x%02x len %u bank %d",
		ep, len, (bank_flag == UDP_CSR_RX_DATA_BK0) ? 0 : 1);

	/* Clear the bank flag and update alternation counter */
	clear_csr_bits(base, hw_ep, bank_flag);
	sam_udp_update_next_bank(priv, hw_ep, bank_flag);

	/* Re-enable endpoint interrupt */
	base->UDP_IER = BIT(hw_ep);

	/* Check if transfer complete (short packet or buffer full) */
	if (len < priv->ep_data[hw_ep].mps || net_buf_tailroom(buf) == 0) {
		buf = udc_buf_get(cfg);

		if (ep == USB_CONTROL_EP_OUT) {
			if (udc_ctrl_stage_is_status_out(dev)) {
				LOG_DBG("OUT: status stage complete");
				udc_ctrl_submit_status(dev, buf);
			}

			udc_ctrl_update_stage(dev, buf);

			if (udc_ctrl_stage_is_status_in(dev)) {
				err = udc_ctrl_submit_s_out_status(dev, buf);
				if (err != 0) {
					LOG_ERR("Failed s-out-status: %d", err);
				}
			}
		} else {
			udc_submit_ep_event(dev, buf, 0);
		}
	}

	return 0;
}

/*
 * Thread handler for processing USB events.
 *
 * SETUP and OUT data are processed in thread context to allow yielding when
 * no buffer is available. This provides hardware flow control - the USB FIFO
 * stays full and the host receives NAK until a buffer becomes available.
 */
static ALWAYS_INLINE void sam_udp_thread_handler(const struct device *const dev)
{
	struct udc_sam_udp_data *const priv = udc_get_private(dev);
	uint32_t evt;
	uint8_t ep;
	int err;

	evt = k_event_wait(&priv->events, UINT32_MAX, false, K_FOREVER);
	udc_lock_internal(dev, K_FOREVER);

	/* Process pending OUT data first to complete any ongoing transfer */
	if (evt & BIT(SAM_UDP_EVT_OUT_PENDING)) {
		k_event_clear(&priv->events, BIT(SAM_UDP_EVT_OUT_PENDING));

		/*
		 * Process each pending OUT endpoint. Don't clear the pending
		 * bit until processing succeeds - this ensures ep_enqueue()
		 * sees the bit set and posts an event if a buffer becomes
		 * available while we're processing.
		 *
		 * OUT endpoints use bits 0-7 in out_pending bitmap.
		 */
		for (uint8_t bit = 0; bit < NUM_OF_HW_EPS; bit++) {
			if (!atomic_test_bit(&priv->out_pending, bit)) {
				continue;
			}

			ep = USB_EP_DIR_OUT | bit;
			LOG_DBG("Pending OUT data for ep 0x%02x", ep);

			err = sam_udp_process_pending_out(dev, ep);
			if (err == 0) {
				/* Success - clear the pending bit */
				atomic_clear_bit(&priv->out_pending, bit);
			} else if (err == -ENOBUFS) {
				/*
				 * No buffer available. Keep bit set so
				 * ep_enqueue() will wake us when buffer
				 * becomes available.
				 */
				LOG_DBG("ep 0x%02x waiting for buffer", ep);
			} else {
				atomic_clear_bit(&priv->out_pending, bit);
				udc_submit_event(dev, UDC_EVT_ERROR, err);
			}
		}
	}

	/* Process SETUP after OUT to avoid dropping status stage */
	if (evt & BIT(SAM_UDP_EVT_SETUP)) {
		k_event_clear(&priv->events, BIT(SAM_UDP_EVT_SETUP));
		err = udc_sam_udp_thread_handle_setup(dev);
		if (err) {
			LOG_ERR("SETUP handling failed: %d", err);
			udc_submit_event(dev, UDC_EVT_ERROR, err);
		}
	}

	udc_unlock_internal(dev);
}

static void sam_udp_isr_reset_handler(const struct device *dev)
{
	Udp *base = udc_sam_udp_get_base(dev);
	struct udc_sam_udp_data *priv = udc_get_private(dev);

	base->UDP_ICR = UDP_ISR_ENDBUSRES;

	/*
	 * Per datasheet 40.6.3.3 (From Powered to Default State):
	 * After ENDBUSRES:
	 * - Enable default endpoint (EPEDS in UDP_CSR0)
	 * - Enable EP0 interrupt
	 * - Enable transceiver (clear TXVDIS)
	 */

	priv->suspended = false;
	priv->bus_reset_done = true;

	reset_all_endpoints(base);

	/*
	 * Clear all pending OUT state. After bus reset there's no
	 * valid data in FIFOs and no pending transfers.
	 */
	atomic_clear(&priv->out_pending);

	/* Reset bank state for all endpoints */
	for (uint8_t i = 0; i < NUM_OF_HW_EPS; i++) {
		priv->ep_data[i].next_bank = 0;
		priv->ep_data[i].tx_banks = 0;
	}

	base->UDP_FADDR = UDP_FADDR_FEN;
	base->UDP_GLB_STAT = 0;

	priv->ep_data[0].mps = EP0_MPS;

	/* Configure EP0 as control endpoint */
	set_csr_bits(base, 0, UDP_CSR_EPTYPE_CTRL | UDP_CSR_EPEDS);

	/* Enable EP0 and suspend/wakeup interrupts */
	base->UDP_IER = UDP_ISR_EP0INT | UDP_ISR_RXSUSP | UDP_ISR_WAKEUP |
			(IS_ENABLED(CONFIG_UDC_ENABLE_SOF) ? UDP_ISR_SOFINT : 0);

	/* Enable transceiver - clear TXVDIS, keep PUON */
	base->UDP_TXVC = UDP_TXVC_PUON;

	LOG_INF("Bus reset: EP0 CSR=0x%08x, TXVC=0x%08x",
		base->UDP_CSR[0], base->UDP_TXVC);

	udc_submit_event(dev, UDC_EVT_RESET, 0);
}

static void sam_udp_isr_suspend_handler(const struct device *dev)
{
	Udp *base = udc_sam_udp_get_base(dev);
	struct udc_sam_udp_data *priv = udc_get_private(dev);

	/*
	 * Per datasheet 40.6.3.6 (Entering Suspend State):
	 * 1. Disable transceiver (set TXVDIS), keep pull-up (PUON)
	 * 2. Clear RXSUSP in UDP_ICR
	 * 3. Disable UDPCK and MCK (MCK last per warning)
	 *
	 * Warning: MCK must be disabled AFTER writing UDP_TXVC
	 * and acknowledging RXSUSP.
	 */
	LOG_DBG("Suspend");

	/* Disable transceiver, keep pull-up */
	base->UDP_TXVC = UDP_TXVC_PUON | UDP_TXVC_TXVDIS;

	/* Clear interrupt - last UDP register access before clock off */
	base->UDP_ICR = UDP_ISR_RXSUSP;

	/* Disable USB clocks to save power */
	udc_sam_udp_disable_usb_clock(dev);

	priv->suspended = true;

	udc_submit_event(dev, UDC_EVT_SUSPEND, 0);
}

static void sam_udp_isr_resume_handler(const struct device *dev)
{
	Udp *base = udc_sam_udp_get_base(dev);
	struct udc_sam_udp_data *priv = udc_get_private(dev);

	/*
	 * Per datasheet 40.6.3.7 (Leaving Suspend State):
	 * 1. Enable MCK (must be first - before any UDP register access)
	 * 2. Enable UDPCK
	 * 3. Enable transceiver (clear TXVDIS)
	 * 4. Clear WAKEUP/RXRSM in UDP_ICR
	 *
	 * Warning: MCK must be enabled BEFORE clearing WAKEUP
	 * in UDP_ICR and clearing TXVDIS in UDP_TXVC.
	 */
	LOG_DBG("Resume");

	/* Re-enable USB clocks first (MCK before any UDP access) */
	udc_sam_udp_resume_clocks(dev);

	/* Enable transceiver, keep pull-up */
	base->UDP_TXVC = UDP_TXVC_PUON;

	/* Clear interrupts after enabling transceiver */
	base->UDP_ICR = UDP_ISR_RXRSM | UDP_ISR_WAKEUP;

	priv->suspended = false;

	udc_submit_event(dev, UDC_EVT_RESUME, 0);
}

static void udc_sam_udp_isr_handler(const struct device *dev)
{
	Udp *base = udc_sam_udp_get_base(dev);
	struct udc_sam_udp_data *priv = udc_get_private(dev);
	uint32_t isr;
	uint32_t imr;
	uint32_t status;

	/*
	 * Per datasheet 40.6.3.7: MCK must be enabled BEFORE any UDP
	 * register access. The WAKEUP interrupt is detected
	 * asynchronously even with clocks disabled.
	 */
	if (priv->suspended) {
		udc_sam_udp_resume_clocks(dev);
	}

	isr = base->UDP_ISR;
	imr = base->UDP_IMR;
	status = isr & imr;

	if (CONFIG_UDC_DRIVER_LOG_LEVEL == LOG_LEVEL_DBG) {
		sam_udp_isr_dbg_dev(base, isr);
	}

	if (!status) {
		return;
	}

	if (status & UDP_ISR_ENDBUSRES) {
		sam_udp_isr_reset_handler(dev);
	}

	if (status & UDP_ISR_RXSUSP) {
		sam_udp_isr_suspend_handler(dev);
	}

	if (status & (UDP_ISR_RXRSM | UDP_ISR_WAKEUP)) {
		sam_udp_isr_resume_handler(dev);
	}

	if (IS_ENABLED(CONFIG_UDC_ENABLE_SOF) &&
	    (status & UDP_ISR_SOFINT)) {
		base->UDP_ICR = UDP_ISR_SOFINT;
		udc_submit_event(dev, UDC_EVT_SOF, 0);
	}

	/* Handle endpoint interrupts */
	for (uint8_t hw_ep = 0; hw_ep < NUM_OF_HW_EPS; hw_ep++) {
		uint32_t csr;

		if (!(status & BIT(hw_ep))) {
			continue;
		}

		csr = base->UDP_CSR[hw_ep];

		if (CONFIG_UDC_DRIVER_LOG_LEVEL == LOG_LEVEL_DBG) {
			sam_udp_isr_dbg_ep(base, hw_ep);
		}

		if (hw_ep == 0) {
			LOG_DBG("EP0 ISR: CSR=0x%08x", csr);
		}

		if (csr & UDP_CSR_STALLSENT) {
			LOG_DBG("EP%d STALLSENT", hw_ep);
			clear_csr_bits(base, hw_ep, UDP_CSR_STALLSENT);
		}

		if ((hw_ep == 0) && (csr & UDP_CSR_RXSETUP)) {
			LOG_DBG("EP0 RXSETUP");
			udc_sam_udp_isr_handle_setup(dev);
			continue;
		}

		if (csr & (UDP_CSR_RX_DATA_BK0 | UDP_CSR_RX_DATA_BK1)) {
			if (hw_ep == 0) {
				/*
				 * EP0: Defer to thread for control transfers.
				 * Disable endpoint interrupt to prevent ISR
				 * loop while thread processes the data.
				 */
				LOG_DBG("EP0 RX_DATA - signal thread");
				base->UDP_IDR = BIT(hw_ep);
				atomic_set_bit(&priv->out_pending,
					       ep_to_bit(USB_CONTROL_EP_OUT));
				k_event_post(&priv->events,
					     BIT(SAM_UDP_EVT_OUT_PENDING));
			} else {
				/*
				 * Bulk/Int/Iso: Process in ISR for throughput.
				 * Defers to thread only if no buffer available.
				 */
				sam_udp_handle_out_isr(dev, hw_ep);
			}
		}

		if (csr & UDP_CSR_TXCOMP) {
			LOG_DBG("EP%d TXCOMP", hw_ep);
			udc_sam_udp_handle_in(dev, USB_EP_DIR_IN | hw_ep);
		}
	}
}

/*
 * Driver initialization
 */
static int udc_sam_udp_driver_preinit(const struct device *dev)
{
	const struct udc_sam_udp_config *config = dev->config;
	struct udc_sam_udp_data *priv = udc_get_private(dev);
	struct udc_data *data = dev->data;
	int err;

	k_mutex_init(&data->mutex);
	k_event_init(&priv->events);

	config->make_thread(dev);

	/* Set UDC capabilities */
	data->caps.rwup = true;
	data->caps.mps0 = UDC_MPS0_64;

	/*
	 * Register EP0 (control, IN and OUT)
	 */
	config->ep_cfg_out[0].caps.out = 1;
	config->ep_cfg_out[0].caps.control = 1;
	config->ep_cfg_out[0].caps.mps = EP0_MPS;
	config->ep_cfg_out[0].addr = USB_CONTROL_EP_OUT;
	err = udc_register_ep(dev, &config->ep_cfg_out[0]);
	if (err) {
		LOG_ERR("Failed to register EP0 OUT");

		return err;
	}

	config->ep_cfg_in[0].caps.in = 1;
	config->ep_cfg_in[0].caps.control = 1;
	config->ep_cfg_in[0].caps.mps = EP0_MPS;
	config->ep_cfg_in[0].addr = USB_CONTROL_EP_IN;
	err = udc_register_ep(dev, &config->ep_cfg_in[0]);
	if (err) {
		LOG_ERR("Failed to register EP0 IN");

		return err;
	}

	/*
	 * Register IN endpoints: EP1, EP3, EP5, EP7 (odd HW endpoints)
	 */
	for (size_t i = 1; i < config->num_in_eps && i < ARRAY_SIZE(in_ep_hw_map); i++) {
		uint8_t hw_ep = in_ep_hw_map[i];
		uint16_t mps = ep_mps_map[hw_ep];

		config->ep_cfg_in[i].caps.in = 1;
		config->ep_cfg_in[i].caps.bulk = 1;
		config->ep_cfg_in[i].caps.interrupt = 1;
		config->ep_cfg_in[i].caps.iso = (hw_ep != 3);
		config->ep_cfg_in[i].caps.mps = mps;
		config->ep_cfg_in[i].addr = USB_EP_DIR_IN | hw_ep;
		err = udc_register_ep(dev, &config->ep_cfg_in[i]);
		if (err) {
			LOG_ERR("Failed to register IN ep 0x%02x",
				config->ep_cfg_in[i].addr);

			return err;
		}
	}

	/*
	 * Register OUT endpoints: EP2, EP4, EP6 (even HW endpoints)
	 */
	for (size_t i = 1; i < config->num_out_eps && i < ARRAY_SIZE(out_ep_hw_map); i++) {
		uint8_t hw_ep = out_ep_hw_map[i];
		uint16_t mps = ep_mps_map[hw_ep];

		config->ep_cfg_out[i].caps.out = 1;
		config->ep_cfg_out[i].caps.bulk = 1;
		config->ep_cfg_out[i].caps.interrupt = 1;
		config->ep_cfg_out[i].caps.iso = 1;
		config->ep_cfg_out[i].caps.mps = mps;
		config->ep_cfg_out[i].addr = USB_EP_DIR_OUT | hw_ep;
		err = udc_register_ep(dev, &config->ep_cfg_out[i]);
		if (err) {
			LOG_ERR("Failed to register OUT ep 0x%02x",
				config->ep_cfg_out[i].addr);

			return err;
		}
	}

	LOG_DBG("Registered %zu IN and %zu OUT endpoints",
		config->num_in_eps, config->num_out_eps);

	return 0;
}

/*
 * Reserve high-capacity endpoints (EP4/EP5, 512 bytes) for ISO transfers
 * or requests requiring more than 64 bytes MPS.
 *
 * SAM4S UDP has limited high-capacity endpoints:
 *   - EP4 (OUT): 512 bytes, dual-bank
 *   - EP5 (IN): 512 bytes, dual-bank
 *
 * This check prevents that transfers with 64-byte MPS may claim these endpoints
 * before ISO transfers that require the larger buffer size, causing ISO endpoint
 * allocation to fail.
 *
 * Returns 0 if the configuration is acceptable, -ENOTSUP to reject.
 */
static int udc_sam_udp_ep_try_config(const struct device *dev,
				     struct udc_ep_config *const cfg)
{
	uint8_t hw_ep = USB_EP_GET_IDX(cfg->addr);
	uint8_t ep_type = cfg->attributes & USB_EP_TRANSFER_TYPE_MASK;

	ARG_UNUSED(dev);

	/*
	 * EP4 and EP5 are the only endpoints with 512-byte capacity.
	 * Reserve them for ISO transfers or requests needing > 64 bytes.
	 */
	if ((hw_ep == 4 || hw_ep == 5) && cfg->caps.mps == 512) {
		if (ep_type != USB_EP_TYPE_ISO && cfg->mps <= 64) {
			LOG_DBG("Rejecting ep 0x%02x: reserving 512B EP for "
				"ISO or high-MPS transfers (requested %u bytes)",
				cfg->addr, cfg->mps);

			return -ENOTSUP;
		}
	}

	return 0;
}

static const struct udc_api udc_sam_udp_api = {
	.lock = udc_sam_udp_lock,
	.unlock = udc_sam_udp_unlock,
	.device_speed = udc_sam_udp_device_speed,
	.init = udc_sam_udp_init,
	.enable = udc_sam_udp_enable,
	.disable = udc_sam_udp_disable,
	.shutdown = udc_sam_udp_shutdown,
	.set_address = udc_sam_udp_set_address,
	.host_wakeup = udc_sam_udp_host_wakeup,
	.ep_try_config = udc_sam_udp_ep_try_config,
	.ep_enable = udc_sam_udp_ep_enable,
	.ep_disable = udc_sam_udp_ep_disable,
	.ep_set_halt = udc_sam_udp_ep_set_halt,
	.ep_clear_halt = udc_sam_udp_ep_clear_halt,
	.ep_enqueue = udc_sam_udp_ep_enqueue,
	.ep_dequeue = udc_sam_udp_ep_dequeue,
};

#define UDC_SAM_UDP_DEVICE_DEFINE(n)						\
	PINCTRL_DT_INST_DEFINE(n);						\
										\
	K_THREAD_STACK_DEFINE(udc_sam_udp_stack_##n,				\
			      CONFIG_UDC_SAM_UDP_THREAD_STACK_SIZE);		\
										\
	static void udc_sam_udp_thread_##n(void *dev, void *arg1, void *arg2)	\
	{									\
		ARG_UNUSED(arg1);						\
		ARG_UNUSED(arg2);						\
		while (true) {							\
			sam_udp_thread_handler(dev);				\
		}								\
	}									\
										\
	static void udc_sam_udp_make_thread_##n(const struct device *dev)	\
	{									\
		struct udc_sam_udp_data *priv = udc_get_private(dev);		\
										\
		k_thread_create(&priv->thread_data,				\
				udc_sam_udp_stack_##n,				\
				K_THREAD_STACK_SIZEOF(udc_sam_udp_stack_##n),	\
				udc_sam_udp_thread_##n,				\
				(void *)dev, NULL, NULL,			\
				K_PRIO_COOP(CONFIG_UDC_SAM_UDP_THREAD_PRIORITY),\
				K_ESSENTIAL,					\
				K_NO_WAIT);					\
		k_thread_name_set(&priv->thread_data, dev->name);		\
	}									\
										\
	static void udc_sam_udp_irq_enable_##n(const struct device *dev)	\
	{									\
		ARG_UNUSED(dev);						\
		IRQ_CONNECT(DT_INST_IRQN(n),					\
			    DT_INST_IRQ(n, priority),				\
			    udc_sam_udp_isr_handler,				\
			    DEVICE_DT_INST_GET(n), 0);				\
		irq_enable(DT_INST_IRQN(n));					\
	}									\
										\
	static void udc_sam_udp_irq_disable_##n(const struct device *dev)	\
	{									\
		ARG_UNUSED(dev);						\
		irq_disable(DT_INST_IRQN(n));					\
	}									\
										\
	static struct udc_ep_config						\
		ep_cfg_out_##n[DT_INST_PROP(n, num_out_endpoints)];		\
	static struct udc_ep_config						\
		ep_cfg_in_##n[DT_INST_PROP(n, num_in_endpoints)];		\
										\
	static struct udc_sam_udp_data udc_priv_##n;				\
										\
	static const struct atmel_sam_pmc_config clk_cfg_##n =			\
		SAM_DT_INST_CLOCK_PMC_CFG(n);					\
										\
	static const struct udc_sam_udp_config udc_cfg_##n = {			\
		.base = (Udp *)DT_INST_REG_ADDR(n),				\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),		\
		.clock_cfg = &clk_cfg_##n,					\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),			\
		.num_in_eps = DT_INST_PROP(n, num_in_endpoints),		\
		.num_out_eps = DT_INST_PROP(n, num_out_endpoints),		\
		.ep_cfg_in = ep_cfg_in_##n,					\
		.ep_cfg_out = ep_cfg_out_##n,					\
		.irq_enable_func = udc_sam_udp_irq_enable_##n,			\
		.irq_disable_func = udc_sam_udp_irq_disable_##n,		\
		.make_thread = udc_sam_udp_make_thread_##n,			\
	};									\
										\
	static struct udc_data udc_data_##n = {					\
		.mutex = Z_MUTEX_INITIALIZER(udc_data_##n.mutex),		\
		.priv = &udc_priv_##n,						\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n,						\
			      udc_sam_udp_driver_preinit,			\
			      NULL,						\
			      &udc_data_##n,					\
			      &udc_cfg_##n,					\
			      POST_KERNEL,					\
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			      &udc_sam_udp_api);

DT_INST_FOREACH_STATUS_OKAY(UDC_SAM_UDP_DEVICE_DEFINE)

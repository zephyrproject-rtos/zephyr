/*
 * Copyright (c) 2021 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam_usbc

#include <logging/log.h>
LOG_MODULE_REGISTER(usb_dc_sam_usbc, CONFIG_USB_DRIVER_LOG_LEVEL);

#include <kernel.h>
#include <usb/usb_device.h>
#include <soc.h>
#include <string.h>

#define EP_UDINT_MASK           0x000FF000

#define NUM_OF_EP_MAX           DT_INST_PROP(0, num_bidir_endpoints)
#define USBC_RAM_ADDR           DT_REG_ADDR(DT_NODELABEL(sram1))
#define USBC_RAM_SIZE           DT_REG_SIZE(DT_NODELABEL(sram1))

/**
 * @brief USB Driver Control Endpoint Finite State Machine states
 *
 * FSM states to keep tracking of control endpoint hiden states.
 */
enum usb_dc_epctrl_state {
	/* Wait a SETUP packet */
	USB_EPCTRL_SETUP,
	/* Wait a OUT data packet */
	USB_EPCTRL_DATA_OUT,
	/* Wait a IN data packet */
	USB_EPCTRL_DATA_IN,
	/* Wait a IN ZLP packet */
	USB_EPCTRL_HANDSHAKE_WAIT_IN_ZLP,
	/* Wait a OUT ZLP packet */
	USB_EPCTRL_HANDSHAKE_WAIT_OUT_ZLP,
	/* STALL enabled on IN & OUT packet */
	USB_EPCTRL_STALL_REQ,
};

struct sam_usbc_udesc_sizes {
	uint32_t byte_count:15;
	uint32_t reserved:1;
	uint32_t multi_packet_size:15;
	uint32_t auto_zlp:1;
};

struct sam_usbc_udesc_bk_ctrl_stat {
	uint32_t stallrq:1;
	uint32_t reserved1:15;
	uint32_t crcerri:1;
	uint32_t overfi:1;
	uint32_t underfi:1;
	uint32_t reserved2:13;
};

struct sam_usbc_udesc_ep_ctrl_stat {
	uint32_t pipe_dev_addr:7;
	uint32_t reserved1:1;
	uint32_t pipe_num:4;
	uint32_t pipe_error_cnt_max:4;
	uint32_t pipe_error_status:8;
	uint32_t reserved2:8;
};

struct sam_usbc_desc_table {
	uint8_t *ep_pipe_addr;
	union {
		uint32_t sizes;
		struct sam_usbc_udesc_sizes udesc_sizes;
	};
	union {
		uint32_t bk_ctrl_stat;
		struct sam_usbc_udesc_bk_ctrl_stat udesc_bk_ctrl_stat;
	};
	union {
		uint32_t ep_ctrl_stat;
		struct sam_usbc_udesc_ep_ctrl_stat udesc_ep_ctrl_stat;
	};
};

struct usb_device_ep_data {
	usb_dc_ep_callback cb_in;
	usb_dc_ep_callback cb_out;
	uint16_t mps;
	bool mps_x2;
	bool is_configured;
	uint32_t out_at;
};

struct usb_device_data {
	usb_dc_status_callback status_cb;
	struct usb_device_ep_data ep_data[NUM_OF_EP_MAX];
};

static struct sam_usbc_desc_table dev_desc[(NUM_OF_EP_MAX + 1) * 2];
static struct usb_device_data dev_data;
static volatile Usbc *regs = (Usbc *) DT_INST_REG_ADDR(0);
static uint32_t num_pins = ATMEL_SAM_DT_INST_NUM_PINS(0);
static struct soc_gpio_pin pins[] = ATMEL_SAM_DT_INST_PINS(0);

#if defined(CONFIG_USB_DRIVER_LOG_LEVEL_DBG)
static uint32_t dev_ep_sta_dbg[2][NUM_OF_EP_MAX];

static void usb_dc_sam_usbc_isr_sta_dbg(uint32_t ep_idx, uint32_t sr)
{
	if (regs->UESTA[ep_idx] != dev_ep_sta_dbg[0][ep_idx]) {
		dev_ep_sta_dbg[0][ep_idx] = regs->UESTA[ep_idx];
		dev_ep_sta_dbg[1][ep_idx] = 0;

		LOG_INF("ISR[%d] CON=%08x INT=%08x INTE=%08x "
			"ECON=%08x ESTA=%08x%s", ep_idx,
			regs->UDCON, regs->UDINT, regs->UDINTE,
			regs->UECON[ep_idx], regs->UESTA[ep_idx],
			((sr & USBC_UESTA0_RXSTPI) ? " STP" : ""));
	} else if (dev_ep_sta_dbg[0][ep_idx] != dev_ep_sta_dbg[1][ep_idx]) {
		dev_ep_sta_dbg[1][ep_idx] = dev_ep_sta_dbg[0][ep_idx];

		LOG_INF("ISR[%d] CON=%08x INT=%08x INTE=%08x "
			"ECON=%08x ESTA=%08x LOOP", ep_idx,
			regs->UDCON, regs->UDINT, regs->UDINTE,
			regs->UECON[ep_idx], regs->UESTA[ep_idx]);
	}
}

static void usb_dc_sam_usbc_clean_sta_dbg(void)
{
	for (int i = 0; i < NUM_OF_EP_MAX; i++) {
		dev_ep_sta_dbg[0][i] = 0;
		dev_ep_sta_dbg[1][i] = 0;
	}
}
#else
#define usb_dc_sam_usbc_isr_sta_dbg(ep_idx, sr)
#define usb_dc_sam_usbc_clean_sta_dbg()
#endif

static ALWAYS_INLINE bool usb_dc_sam_usbc_is_frozen_clk(void)
{
	return USBC->USBCON & USBC_USBCON_FRZCLK;
}

static ALWAYS_INLINE void usb_dc_sam_usbc_freeze_clk(void)
{
	USBC->USBCON |= USBC_USBCON_FRZCLK;
}

static ALWAYS_INLINE void usb_dc_sam_usbc_unfreeze_clk(void)
{
	USBC->USBCON &= ~USBC_USBCON_FRZCLK;

	while (USBC->USBCON & USBC_USBCON_FRZCLK) {
		;
	};
}

static uint8_t usb_dc_sam_usbc_ep_curr_bank(uint8_t ep_idx)
{
	uint8_t idx = ep_idx * 2;

	if ((ep_idx > 0) &&
	    (regs->UESTA[ep_idx] & USBC_UESTA0_CURRBK(1)) > 0) {
		idx++;
	}

	return idx;
}

static bool usb_dc_is_attached(void)
{
	return (regs->UDCON & USBC_UDCON_DETACH) == 0;
}

static bool usb_dc_ep_is_enabled(uint8_t ep_idx)
{
	int reg = regs->UERST;

	return (reg & BIT(USBC_UERST_EPEN0_Pos + ep_idx));
}

static int usb_dc_sam_usbc_ep_alloc_buf(int ep_idx)
{
	struct sam_usbc_desc_table *ep_desc_bk;
	bool ep_enabled[NUM_OF_EP_MAX];
	int desc_mem_alloc;
	int mps;

	if (ep_idx >= NUM_OF_EP_MAX) {
		return -EINVAL;
	}

	desc_mem_alloc = 0;

	mps = dev_data.ep_data[ep_idx].mps_x2
	    ? dev_data.ep_data[ep_idx].mps * 2
	    : dev_data.ep_data[ep_idx].mps;

	/* Check if there are memory to all endpoints */
	for (int i = 0; i < NUM_OF_EP_MAX; i++) {
		if (!dev_data.ep_data[i].is_configured || i == ep_idx) {
			continue;
		}

		desc_mem_alloc += dev_data.ep_data[i].mps_x2
				? dev_data.ep_data[i].mps * 2
				: dev_data.ep_data[i].mps;
	}

	if ((desc_mem_alloc + mps) > USBC_RAM_SIZE) {
		memset(&dev_data.ep_data[ep_idx], 0,
		       sizeof(struct usb_device_ep_data));
		return -ENOMEM;
	}

	for (int i = NUM_OF_EP_MAX - 1; i >= ep_idx; i--) {
		ep_enabled[i] = usb_dc_ep_is_enabled(i);
		if (ep_enabled[i]) {
			LOG_DBG("Temporary disable ep idx 0x%02x", i);
			usb_dc_ep_disable(i);
		}
	}

	desc_mem_alloc = 0U;
	for (int i = 0; i < ep_idx; i++) {
		if (!dev_data.ep_data[i].is_configured) {
			continue;
		}

		desc_mem_alloc += dev_data.ep_data[i].mps_x2
				? dev_data.ep_data[i].mps * 2
				: dev_data.ep_data[i].mps;
	}

	ep_desc_bk = ((struct sam_usbc_desc_table *) &dev_desc)
		   + (ep_idx * 2);
	for (int i = ep_idx; i < NUM_OF_EP_MAX; i++) {
		if (!dev_data.ep_data[i].is_configured && (i != ep_idx)) {
			ep_desc_bk += 2;
			continue;
		}

		/* Alloc bank 0 */
		ep_desc_bk->ep_pipe_addr = ((uint8_t *) USBC_RAM_ADDR)
					 + desc_mem_alloc;
		ep_desc_bk->sizes = 0;
		ep_desc_bk->bk_ctrl_stat = 0;
		ep_desc_bk->ep_ctrl_stat = 0;
		ep_desc_bk++;

		/**
		 * Alloc bank 1
		 *
		 * if dual bank,
		 * then ep_pipe_addr[1] = ep_pipe_addr[0] address + mps size
		 * else ep_pipe_addr[1] = ep_pipe_addr[0] address
		 */
		ep_desc_bk->ep_pipe_addr = ((uint8_t *) USBC_RAM_ADDR)
					 + desc_mem_alloc
					 + (dev_data.ep_data[i].mps_x2
					 ?  dev_data.ep_data[i].mps
					 :  0);
		ep_desc_bk->sizes = 0;
		ep_desc_bk->bk_ctrl_stat = 0;
		ep_desc_bk->ep_ctrl_stat = 0;
		ep_desc_bk++;

		desc_mem_alloc += dev_data.ep_data[i].mps_x2
				? dev_data.ep_data[i].mps * 2
				: dev_data.ep_data[i].mps;
	}

	ep_enabled[ep_idx] = false;
	for (int i = ep_idx; i < NUM_OF_EP_MAX; i++) {
		if (ep_enabled[i]) {
			usb_dc_ep_enable(i);
		}
	}

	return 0;
}

static void usb_dc_ep_enable_interrupts(uint8_t ep_idx)
{
	if (ep_idx == 0U) {
		/* Control endpoint: enable SETUP */
		regs->UECONSET[ep_idx] = USBC_UECON0SET_RXSTPES;
	} else if (regs->UECFG[ep_idx] & USBC_UECFG0_EPDIR_IN) {
		/* TX - IN direction: acknowledge FIFO empty interrupt */
		regs->UESTACLR[ep_idx] = USBC_UESTA0CLR_TXINIC;
		regs->UECONSET[ep_idx] = USBC_UECON0SET_TXINES;
	} else {
		/* RX - OUT direction */
		regs->UECONSET[ep_idx] = USBC_UECON0SET_RXOUTES;
	}
}

static void usb_dc_ep_isr_sta(uint8_t ep_idx)
{
	uint32_t sr = regs->UESTA[ep_idx];

	usb_dc_sam_usbc_isr_sta_dbg(ep_idx, sr);

	if (sr & USBC_UESTA0_RAMACERI) {
		regs->UESTACLR[ep_idx] = USBC_UESTA0CLR_RAMACERIC;
		LOG_ERR("ISR: EP%d RAM Access Error", ep_idx);
	}
}

static void usb_dc_ep0_isr(void)
{
	uint32_t sr = regs->UESTA[0];
	uint32_t dev_ctrl = regs->UDCON;

	usb_dc_ep_isr_sta(0);

	regs->UECONCLR[0] = USBC_UECON0CLR_NAKINEC;
	regs->UECONCLR[0] = USBC_UECON0CLR_NAKOUTEC;

	if (sr & USBC_UESTA0_RXSTPI) {
		regs->UESTACLR[0] = USBC_UESTA0CLR_NAKINIC;
		regs->UESTACLR[0] = USBC_UESTA0CLR_NAKOUTIC;

		if (sr & USBC_UESTA0_CTRLDIR) {
			/** IN Package */
			/** Nothing to do */
		} else {
			/** OUT Package */
			regs->UECONSET[0] = USBC_UECON0SET_RXOUTES;
		}

		/* SETUP data received */
		dev_data.ep_data[0].cb_out(USB_EP_DIR_OUT, USB_DC_EP_SETUP);
		return;
	}

	if (sr & USBC_UESTA0_RXOUTI) {
		/* OUT (to device) data received */
		dev_data.ep_data[0].cb_out(USB_EP_DIR_OUT, USB_DC_EP_DATA_OUT);
	}

	if ((sr & USBC_UESTA0_TXINI) &&
	    (regs->UECON[0] & USBC_UECON0_TXINE)) {
		regs->UECONCLR[0] = USBC_UECON0CLR_TXINEC;

		if (sr & USBC_UESTA0_CTRLDIR) {
			/** Finish Control Write State */
			return;
		}

		/* IN (to host) transmit complete */
		dev_data.ep_data[0].cb_in(USB_EP_DIR_IN, USB_DC_EP_DATA_IN);

		if (!(dev_ctrl & USBC_UDCON_ADDEN) &&
		    (dev_ctrl & USBC_UDCON_UADD_Msk) != 0U) {
			/* Commit the pending address update.  This must be
			 * done after the ack to the host completes else the
			 * ack will get dropped.
			 */
			regs->UDCON |= USBC_UDCON_ADDEN;
		}
	}

	if (sr & USBC_UESTA0_NAKOUTI) {
		/** Start Control Read State */
		regs->UESTACLR[0] = USBC_UESTA0CLR_NAKOUTIC;
		regs->UECONCLR[0] = USBC_UECON0CLR_NAKOUTEC;
		regs->UECONCLR[0] = USBC_UECON0CLR_TXINEC;

		/** Wait OUT State */
		regs->UECONSET[0] = USBC_UECON0SET_RXOUTES;
		return;
	}

	if (sr & USBC_UESTA0_NAKINI) {
		/** Start Control Write State */
		regs->UESTACLR[0] = USBC_UESTA0CLR_NAKINIC;
		regs->UECONCLR[0] = USBC_UECON0CLR_NAKINEC;
		regs->UECONCLR[0] = USBC_UECON0CLR_RXOUTEC;

		dev_data.ep_data[0].cb_in(USB_EP_DIR_IN, USB_DC_EP_DATA_IN);
		return;
	}
}

static void usb_dc_ep_isr(uint8_t ep_idx)
{
	uint32_t sr = regs->UESTA[ep_idx];

	usb_dc_ep_isr_sta(ep_idx);

	if (sr & USBC_UESTA0_RXOUTI) {
		uint8_t ep = ep_idx | USB_EP_DIR_OUT;

		regs->UESTACLR[ep_idx] = USBC_UESTA0CLR_RXOUTIC;

		/* OUT (to device) data received */
		dev_data.ep_data[ep_idx].cb_out(ep, USB_DC_EP_DATA_OUT);
	}
	if (sr & USBC_UESTA0_TXINI) {
		uint8_t ep = ep_idx | USB_EP_DIR_IN;

		regs->UESTACLR[ep_idx] = USBC_UESTA0CLR_TXINIC;

		/* IN (to host) transmit complete */
		dev_data.ep_data[ep_idx].cb_in(ep, USB_DC_EP_DATA_IN);
	}
}

static void usb_dc_sam_usbc_isr(void)
{
	uint32_t sr = regs->UDINT;

	if (IS_ENABLED(CONFIG_USB_DEVICE_SOF)) {
		/* SOF interrupt */
		if (sr & USBC_UDINT_SOF) {
			/* Acknowledge the interrupt */
			regs->UDINTCLR = USBC_UDINTCLR_SOFC;

			dev_data.status_cb(USB_DC_SOF, NULL);

			goto usb_dc_sam_usbc_isr_barrier;
		}
	}

	/* EP0 endpoint interrupt */
	if (sr & USBC_UDINT_EP0INT) {
		usb_dc_ep0_isr();

		goto usb_dc_sam_usbc_isr_barrier;
	}

	/* Other endpoints interrupt */
	if (sr & EP_UDINT_MASK) {
		for (int ep_idx = 1; ep_idx < NUM_OF_EP_MAX; ep_idx++) {
			if (sr & (USBC_UDINT_EP0INT << ep_idx)) {
				usb_dc_ep_isr(ep_idx);
			}
		}

		goto usb_dc_sam_usbc_isr_barrier;
	}

	/* End of resume interrupt */
	if (sr & USBC_UDINT_EORSM) {
		LOG_DBG("ISR: End Of Resume");

		regs->UDINTCLR = USBC_UDINTCLR_EORSMC;

		dev_data.status_cb(USB_DC_RESUME, NULL);

		goto usb_dc_sam_usbc_isr_barrier;
	}

	/* End of reset interrupt */
	if (sr & USBC_UDINT_EORST) {
		LOG_DBG("ISR: End Of Reset");

		regs->UDINTCLR = USBC_UDINTCLR_EORSTC;

		if (usb_dc_ep_is_enabled(0)) {
			/* The device clears some of the configuration of EP0
			 * when it receives the EORST. Re-enable interrupts.
			 */

			usb_dc_ep_enable_interrupts(0);
			/* In case of abort of IN Data Phase:
			 * No need to abort IN transfer (rise TXINI),
			 * because it is automatically done by hardware when a
			 * Setup packet is received. But the interrupt must be
			 * disabled to don't generate interrupt TXINI after
			 * SETUP reception.
			 */
			regs->UECONCLR[0] = USBC_UECON0CLR_TXINEC;

			 /* In case of OUT ZLP event is no processed before
			  * Setup event occurs
			  */
			regs->UESTACLR[0] = USBC_UESTA0CLR_RXOUTIC;
		}

		dev_data.status_cb(USB_DC_RESET, NULL);

		usb_dc_sam_usbc_clean_sta_dbg();

		goto usb_dc_sam_usbc_isr_barrier;
	}

	/* Suspend interrupt */
	if (sr & USBC_UDINT_SUSP && regs->UDINTE & USBC_UDINTE_SUSPE) {
		LOG_DBG("ISR: Suspend");

		regs->UDINTCLR = USBC_UDINTCLR_SUSPC;

		usb_dc_sam_usbc_unfreeze_clk();

		/**
		 * Sync Generic Clock
		 * Check USB clock ready after suspend and
		 * eventually sleep USB clock
		 */
		while ((regs->USBSTA & USBC_USBSTA_CLKUSABLE) == 0) {
			;
		};

		regs->UDINTECLR = USBC_UDINTECLR_SUSPEC;
		regs->UDINTCLR = USBC_UDINTCLR_WAKEUPC;
		regs->UDINTESET = USBC_UDINTESET_WAKEUPES;

		usb_dc_sam_usbc_freeze_clk();

		dev_data.status_cb(USB_DC_SUSPEND, NULL);

		goto usb_dc_sam_usbc_isr_barrier;
	}

	/* Wakeup interrupt */
	if (sr & USBC_UDINT_WAKEUP && regs->UDINTE & USBC_UDINTE_WAKEUPE) {
		LOG_DBG("ISR: Wake Up");

		regs->UDINTCLR = USBC_UDINTCLR_WAKEUPC;

		usb_dc_sam_usbc_unfreeze_clk();

		/**
		 * Sync Generic Clock
		 * Check USB clock ready after suspend and
		 * eventually sleep USB clock
		 */
		while ((regs->USBSTA & USBC_USBSTA_CLKUSABLE) == 0) {
			;
		};

		regs->UDINTECLR = USBC_UDINTECLR_WAKEUPEC;
		regs->UDINTCLR = USBC_UDINTCLR_SUSPC;
		regs->UDINTESET = USBC_UDINTESET_SUSPES;
	}

usb_dc_sam_usbc_isr_barrier:
	__DMB();
}

int usb_dc_attach(void)
{
	uint32_t pmcon;
	uint32_t regval;
	uint32_t key = irq_lock();

	/* Enable USBC asynchronous wake-up source */
	PM->AWEN |= BIT(PM_AWEN_USBC);

	/* Always authorize asynchronous USB interrupts to exit of sleep mode
	 * For SAM USB wake up device except BACKUP mode
	 */
	pmcon = BPM->PMCON | BPM_PMCON_FASTWKUP;
	BPM->UNLOCK = BPM_UNLOCK_KEY(0xAAu)
		    | BPM_UNLOCK_ADDR((uint32_t)&BPM->PMCON - (uint32_t)BPM);
	BPM->PMCON = pmcon;

	/* Start the peripheral clock PBB & DATA */
	soc_pmc_peripheral_enable(
		PM_CLOCK_MASK(PM_CLK_GRP_PBB, SYSCLK_USBC_REGS));
	soc_pmc_peripheral_enable(
		PM_CLOCK_MASK(PM_CLK_GRP_HSB, SYSCLK_USBC_DATA));
	soc_gpio_list_configure(pins, num_pins);

	/* Enable USB Generic clock */
	SCIF->GCCTRL[GEN_CLK_USBC] = 0;
	SCIF->GCCTRL[GEN_CLK_USBC] = SCIF_GCCTRL_OSCSEL(SCIF_GC_USES_CLK_HSB)
				   | SCIF_GCCTRL_CEN;

	/* Sync Generic Clock */
	while ((regs->USBSTA & USBC_USBSTA_CLKUSABLE) == 0) {
		;
	};

	/* Enable the USB controller in device mode with the clock unfrozen */
	regs->USBCON = USBC_USBCON_UIMOD | USBC_USBCON_USBE;

	usb_dc_sam_usbc_unfreeze_clk();

	regs->UDESC = USBC_UDESC_UDESCA((int) &dev_desc);

	/* Select the speed with pads detached */
	regval = USBC_UDCON_DETACH;

	switch (DT_ENUM_IDX(DT_DRV_INST(0), maximum_speed)) {
	case 1:
		WRITE_BIT(regval, USBC_UDCON_LS_Pos, 0);
		break;
	case 0:
		WRITE_BIT(regval, USBC_UDCON_LS_Pos, 1);
		break;
	default:
		WRITE_BIT(regval, USBC_UDCON_LS_Pos, 0);
		LOG_WRN("Unsupported maximum speed defined in device tree. "
			"USB controller will default to its maximum HW "
			"capability");
	}

	regs->UDCON = regval;

	/* Enable device interrupts
	 *  EORSM	End of Resume Interrupt
	 *  SOF		Start of Frame Interrupt
	 *  EORST	End of Reset Interrupt
	 *  SUSP	Suspend Interrupt
	 *  WAKEUP	Wake-Up Interrupt
	 */
	regs->UDINTCLR = USBC_UDINTCLR_EORSMC
		       | USBC_UDINTCLR_EORSTC
		       | USBC_UDINTCLR_SOFC
		       | USBC_UDINTCLR_SUSPC
		       | USBC_UDINTCLR_WAKEUPC;

	regs->UDINTESET = USBC_UDINTESET_EORSMES
			| USBC_UDINTESET_EORSTES
			| USBC_UDINTESET_SUSPES
			| USBC_UDINTESET_WAKEUPES;

	if (IS_ENABLED(CONFIG_USB_DEVICE_SOF)) {
		regs->UDINTESET |= USBC_UDINTESET_SOFES;
	}

	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    usb_dc_sam_usbc_isr, 0, 0);
	irq_enable(DT_INST_IRQN(0));

	/* Attach the device */
	regs->UDCON &= ~USBC_UDCON_DETACH;

	/* Put USB on low power state (wait Susp/Wake int) */
	usb_dc_sam_usbc_freeze_clk();

	/* Force Susp 2 Wake transition */
	regs->UDINTSET = USBC_UDINTSET_SUSPS;

	irq_unlock(key);

	LOG_DBG("USB DC attach");
	return 0;
}

int usb_dc_detach(void)
{
	regs->UDCON |= USBC_UDCON_DETACH;

	/* Disable the USB controller and freeze the clock */
	regs->USBCON = USBC_USBCON_UIMOD | USBC_USBCON_FRZCLK;

	/* Disable USB Generic clock */
	SCIF->GCCTRL[GEN_CLK_USBC] = 0;

	/* Disable USBC asynchronous wake-up source */
	PM->AWEN &= ~(BIT(PM_AWEN_USBC));

	/* Disable the peripheral clock HSB & PBB */
	soc_pmc_peripheral_enable(
		PM_CLOCK_MASK(PM_CLK_GRP_HSB, SYSCLK_USBC_DATA));
	soc_pmc_peripheral_enable(
		PM_CLOCK_MASK(PM_CLK_GRP_PBB, SYSCLK_USBC_REGS));

	irq_disable(DT_INST_IRQN(0));

	LOG_DBG("USB DC detach");
	return 0;
}

int usb_dc_reset(void)
{
	/* Reset the controller */
	regs->USBCON = USBC_USBCON_UIMOD | USBC_USBCON_FRZCLK;

	/* Clear private data */
	(void)memset(&dev_data, 0, sizeof(dev_data));
	(void)memset(&dev_desc, 0, sizeof(dev_desc));

	LOG_DBG("USB DC reset");
	return 0;
}

int usb_dc_set_address(uint8_t addr)
{
	/*
	 * Set the address but keep it disabled for now. It should be enabled
	 * only after the ack to the host completes.
	 */
	regs->UDCON &= ~USBC_UDCON_ADDEN;
	regs->UDCON |= USBC_UDCON_UADD(addr);

	LOG_DBG("USB DC set address 0x%02x", addr);
	return 0;
}

void usb_dc_set_status_callback(const usb_dc_status_callback cb)
{
	regs->UDINTECLR = USBC_UDINTECLR_MASK;
	regs->UDINTCLR = USBC_UDINTCLR_MASK;

	usb_dc_detach();
	usb_dc_reset();

	dev_data.status_cb = cb;

	LOG_DBG("USB DC set callback");
}

int usb_dc_ep_check_cap(const struct usb_dc_ep_cfg_data * const cfg)
{
	uint8_t ep_idx = USB_EP_GET_IDX(cfg->ep_addr);

	if (ep_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("endpoint index/address out of range");
		return -EINVAL;
	}

	if (ep_idx == 0U) {
		if (cfg->ep_type != USB_DC_EP_CONTROL) {
			LOG_ERR("pre-selected as control endpoint");
			return -EINVAL;
		}
	} else if (ep_idx & BIT(0)) {
		if (USB_EP_DIR_IS_OUT(cfg->ep_addr)) {
			LOG_INF("pre-selected as IN endpoint");
			return -EINVAL;
		}
	} else {
		if (USB_EP_DIR_IS_IN(cfg->ep_addr)) {
			LOG_INF("pre-selected as OUT endpoint");
			return -EINVAL;
		}
	}

	if (cfg->ep_mps < 1 || cfg->ep_mps > 1024 ||
	    (cfg->ep_type == USB_DC_EP_CONTROL && cfg->ep_mps > 64)) {
		LOG_ERR("invalid endpoint size");
		return -EINVAL;
	}

	return 0;
}

int usb_dc_ep_configure(const struct usb_dc_ep_cfg_data *const cfg)
{
	uint8_t ep_idx = USB_EP_GET_IDX(cfg->ep_addr);
	uint32_t regval = 0U;
	int log2ceil_mps;

	if (usb_dc_ep_check_cap(cfg) != 0) {
		return -EINVAL;
	}

	if (!usb_dc_is_attached()) {
		LOG_ERR("device not attached");
		return -ENODEV;
	}

	/* Allow re-configure any endpoint */
	if (usb_dc_ep_is_enabled(ep_idx)) {
		usb_dc_ep_disable(ep_idx);
	}

	LOG_DBG("Configure ep 0x%02x, mps %d, type %d",
		cfg->ep_addr, cfg->ep_mps, cfg->ep_type);

	switch (cfg->ep_type) {
	case USB_DC_EP_CONTROL:
		regval |= USBC_UECFG0_EPTYPE_CONTROL;
		break;
	case USB_DC_EP_ISOCHRONOUS:
		regval |= USBC_UECFG0_EPTYPE_ISOCHRONOUS;
		break;
	case USB_DC_EP_BULK:
		regval |= USBC_UECFG0_EPTYPE_BULK;
		break;
	case USB_DC_EP_INTERRUPT:
		regval |= USBC_UECFG0_EPTYPE_INTERRUPT;
		break;
	default:
		return -EINVAL;
	}

	if (USB_EP_DIR_IS_OUT(cfg->ep_addr) ||
	    cfg->ep_type == USB_DC_EP_CONTROL) {
		regval |= USBC_UECFG0_EPDIR_OUT;
	} else {
		regval |= USBC_UECFG0_EPDIR_IN;
	}

	/*
	 * Map the endpoint size to the buffer size. Only power of 2 buffer
	 * sizes between 8 and 1024 are possible, get the next power of 2.
	 */
	log2ceil_mps = 32 - __builtin_clz((MAX(cfg->ep_mps, 8) << 1) - 1) - 1;
	regval |= USBC_UECFG0_EPSIZE(log2ceil_mps - 3);
	dev_data.ep_data[ep_idx].mps = cfg->ep_mps;

	/* Use double bank buffering for: ISOCHRONOUS, BULK and INTERRUPT */
	if (cfg->ep_type != USB_DC_EP_CONTROL) {
		regval |= USBC_UECFG0_EPBK_DOUBLE;
		dev_data.ep_data[ep_idx].mps_x2 = true;
	} else {
		regval |= USBC_UECFG0_EPBK_SINGLE;
		dev_data.ep_data[ep_idx].mps_x2 = false;
	}

	if (usb_dc_sam_usbc_ep_alloc_buf(ep_idx) < 0) {
		dev_data.ep_data[ep_idx].is_configured = false;
		return -ENOMEM;
	}

	/* Configure the endpoint */
	dev_data.ep_data[ep_idx].is_configured = true;
	regs->UECFG[ep_idx] = regval;

	LOG_DBG("ep 0x%02x configured", cfg->ep_addr);
	return 0;
}

int usb_dc_ep_set_stall(uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	if (ep_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("wrong endpoint index/address");
		return -EINVAL;
	}

	regs->UECONSET[ep_idx] = USBC_UECON0SET_STALLRQS;

	LOG_DBG("USB DC stall set ep 0x%02x", ep);
	return 0;
}

int usb_dc_ep_clear_stall(uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	if (ep_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("wrong endpoint index/address");
		return -EINVAL;
	}

	if (regs->UECON[ep_idx] & USBC_UECON0_STALLRQ) {
		regs->UECONCLR[ep_idx] = USBC_UECON0CLR_STALLRQC;
		if (regs->UESTA[ep_idx] & USBC_UESTA0_STALLEDI) {
			regs->UESTACLR[ep_idx] = USBC_UESTA0CLR_STALLEDIC;
			regs->UECONSET[ep_idx] = USBC_UECON0SET_RSTDTS;
		}
	}

	LOG_DBG("USB DC stall clear ep 0x%02x", ep);
	return 0;
}

int usb_dc_ep_is_stalled(uint8_t ep, uint8_t *stalled)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	if (ep_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("wrong endpoint index/address");
		return -EINVAL;
	}

	if (!stalled) {
		return -EINVAL;
	}

	*stalled = ((regs->UECON[ep_idx] & USBC_UECON0_STALLRQ) != 0);

	LOG_DBG("USB DC stall check ep 0x%02x stalled: %d", ep, *stalled);
	return 0;
}

int usb_dc_ep_halt(uint8_t ep)
{
	return usb_dc_ep_set_stall(ep);
}

int usb_dc_ep_enable(uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	if (ep_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("wrong endpoint index/address");
		return -EINVAL;
	}

	if (!dev_data.ep_data[ep_idx].is_configured) {
		LOG_ERR("endpoint not configured");
		return -ENODEV;
	}

	/* Enable endpoint */
	regs->UERST |= BIT(USBC_UERST_EPEN0_Pos + ep_idx);
	/* Enable global endpoint interrupts */
	regs->UDINTESET = (USBC_UDINTESET_EP0INTES << ep_idx);

	usb_dc_ep_enable_interrupts(ep_idx);
	LOG_DBG("Enable ep 0x%02x", ep);
	return 0;
}

int usb_dc_ep_disable(uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	if (ep_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("wrong endpoint index/address");
		return -EINVAL;
	}

	/* Disable global endpoint interrupt */
	regs->UDINTECLR = BIT(USBC_UDINTESET_EP0INTES_Pos + ep_idx);

	/* Disable endpoint and reset */
	regs->UERST &= ~BIT(USBC_UERST_EPEN0_Pos + ep_idx);

	LOG_DBG("Disable ep 0x%02x", ep);
	return 0;
}

int usb_dc_ep_flush(uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	if (ep_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("wrong endpoint index/address");
		return -EINVAL;
	}

	if (!usb_dc_ep_is_enabled(ep_idx)) {
		LOG_ERR("endpoint not enabled");
		return -ENODEV;
	}

	/* Disable the IN interrupt */
	regs->UECONCLR[ep_idx] = USBC_UECON0CLR_TXINEC;

	/* Reset the endpoint */
	regs->UERST &= ~(BIT(ep_idx));
	regs->UERST |= BIT(ep_idx);

	/* Reenable interrupts */
	usb_dc_ep_enable_interrupts(ep_idx);

	LOG_DBG("ep 0x%02x flushed", ep);
	return 0;
}

int usb_dc_ep_set_callback(uint8_t ep, const usb_dc_ep_callback cb)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	if (ep_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("wrong endpoint index/address");
		return -EINVAL;
	}

	if (USB_EP_DIR_IS_IN(ep)) {
		dev_data.ep_data[ep_idx].cb_in = cb;
	} else {
		dev_data.ep_data[ep_idx].cb_out = cb;
	}

	LOG_DBG("set ep 0x%02x %s callback", ep,
		USB_EP_DIR_IS_IN(ep) ? "IN" : "OUT");
	return 0;
}

int usb_dc_ep_write(uint8_t ep, const uint8_t *data,
		    uint32_t data_len, uint32_t *ret_bytes)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	uint8_t ep_bank;
	uint32_t packet_len;
	uint32_t key;

	if (ep_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("wrong endpoint index/address");
		return -EINVAL;
	}

	if (!usb_dc_ep_is_enabled(ep_idx)) {
		LOG_ERR("endpoint not enabled");
		return -ENODEV;
	}

	if (USB_EP_DIR_IS_OUT(ep)) {
		LOG_ERR("wrong endpoint direction");
		return -EINVAL;
	}

	if ((regs->UECON[ep_idx] & USBC_UECON0_STALLRQ) != 0) {
		LOG_WRN("endpoint is stalled");
		return -EBUSY;
	}

	/* Check if there is bank available */
	if (ep_idx > 0) {
		if ((regs->UECON[ep_idx] & USBC_UECON0_FIFOCON) == 0) {
			return -EAGAIN;
		}
	}

	ep_bank = usb_dc_sam_usbc_ep_curr_bank(ep_idx);

	packet_len = MIN(data_len, dev_data.ep_data[ep_idx].mps);

	if (data && packet_len > 0) {
		memcpy(dev_desc[ep_bank].ep_pipe_addr, data, packet_len);
		__DSB();
	}
	dev_desc[ep_bank].sizes = packet_len;

	if (ep_idx == 0U) {
		key = irq_lock();
		/*
		 * Control endpoint: clear the interrupt flag to send the
		 * data, and re-enable the interrupts to trigger an interrupt
		 * at the end of the transfer.
		 */
		regs->UESTACLR[0] = USBC_UESTA0CLR_TXINIC;
		regs->UECONSET[0] = USBC_UECON0SET_TXINES;

		if (packet_len == 0) {
			/* To detect a protocol error, enable nak interrupt
			 * on data OUT phase
			 */
			regs->UESTACLR[0] = USBC_UESTA0CLR_NAKOUTIC;
			regs->UECONSET[0] = USBC_UECON0SET_NAKOUTES;
		}
		irq_unlock(key);
	} else {
		/*
		 * Other endpoint types: clear the FIFO control flag to send
		 * the data.
		 */
		regs->UECONCLR[ep_idx] = USBC_UECON0CLR_FIFOCONC;
	}

	if (ret_bytes) {
		*ret_bytes = packet_len;
	}

	LOG_DBG("ep 0x%02x write %d bytes from %d to bank %d%s",
		ep, packet_len, data_len, ep_bank % 2,
		packet_len == 0 ? " (ZLP)" : "");
	return 0;
}

int usb_dc_ep_read_ex(uint8_t ep, uint8_t *data, uint32_t max_data_len,
		      uint32_t *read_bytes, bool wait)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	uint8_t ep_bank;
	uint32_t data_len;
	uint32_t remaining;
	uint32_t take;
	int rc = 0;

	if (ep_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("wrong endpoint index/address");
		return -EINVAL;
	}

	if (!usb_dc_ep_is_enabled(ep_idx)) {
		LOG_ERR("endpoint not enabled");
		return -ENODEV;
	}

	if (USB_EP_DIR_IS_IN(ep)) {
		LOG_ERR("wrong endpoint direction");
		return -EINVAL;
	}

	if ((regs->UECON[ep_idx] & USBC_UECON0_STALLRQ) != 0) {
		LOG_WRN("endpoint is stalled");
		return -EBUSY;
	}

	ep_bank = usb_dc_sam_usbc_ep_curr_bank(ep_idx);
	data_len = dev_desc[ep_bank].udesc_sizes.byte_count;

	if (data == NULL) {
		dev_data.ep_data[ep_idx].out_at = 0U;

		if (read_bytes) {
			*read_bytes = data_len;
		}
		return 0;
	}

	remaining = data_len - dev_data.ep_data[ep_idx].out_at;
	take = MIN(max_data_len, remaining);
	if (take) {
		memcpy(data,
		       (uint8_t *) dev_desc[ep_bank].ep_pipe_addr +
		       dev_data.ep_data[ep_idx].out_at,
		       take);
		__DSB();
	}

	if (read_bytes) {
		*read_bytes = take;
	}

	if (take == remaining || take == 0) {
		if (!wait) {
			dev_data.ep_data[ep_idx].out_at = 0U;

			rc = usb_dc_ep_read_continue(ep);
		}
	} else {
		dev_data.ep_data[ep_idx].out_at += take;
	}

	LOG_DBG("ep 0x%02x read %d bytes from bank %d and %s",
		ep, take, ep_bank % 2, wait ? "wait" : "NO wait");
	return rc;
}

int usb_dc_ep_read(uint8_t ep, uint8_t *data,
		   uint32_t max_data_len, uint32_t *read_bytes)
{
	return usb_dc_ep_read_ex(ep, data, max_data_len, read_bytes, false);
}

int usb_dc_ep_read_wait(uint8_t ep, uint8_t *data, uint32_t max_data_len,
			uint32_t *read_bytes)
{
	return usb_dc_ep_read_ex(ep, data, max_data_len, read_bytes, true);
}

int usb_dc_ep_read_continue(uint8_t ep)
{
	uint32_t key;
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	if (ep_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("wrong endpoint index/address");
		return -EINVAL;
	}

	if (!usb_dc_ep_is_enabled(ep_idx)) {
		LOG_ERR("endpoint not enabled");
		return -ENODEV;
	}

	if (USB_EP_DIR_IS_IN(ep)) {
		LOG_ERR("wrong endpoint direction");
		return -EINVAL;
	}

	if (ep_idx == 0U) {
		/* Control endpoint: clear the interrupt flag to send the
		 * data. It is easier to clear both SETUP and OUT flag than
		 * checking the stage of the transfer.
		 */
		regs->UESTACLR[0] = USBC_UESTA0CLR_RXOUTIC
				  | USBC_UESTA0CLR_RXSTPIC;

		if (dev_data.ep_data[0].out_at == 0U) {
			dev_desc[0].sizes = 0;
		}

		/* To detect a protocol error, enable nak interrupt
		 * on data OUT phase
		 */
		key = irq_lock();
		regs->UESTACLR[0] = USBC_UESTA0CLR_NAKOUTIC;
		regs->UECONSET[0] = USBC_UECON0SET_NAKOUTES;
		irq_unlock(key);
	} else {
		/* Other endpoint types: clear the FIFO control flag to
		 * receive more data.
		 */
		regs->UECONCLR[ep_idx] = USBC_UECON0CLR_FIFOCONC;
	}

	return 0;
}

int usb_dc_ep_mps(uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	if (ep_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("wrong endpoint index/address");
		return -EINVAL;
	}

	return dev_data.ep_data[ep_idx].mps;
}

int usb_dc_wakeup_request(void)
{
	bool is_clk_frozen = usb_dc_sam_usbc_is_frozen_clk();

	if (is_clk_frozen) {
		usb_dc_sam_usbc_unfreeze_clk();
	}

	regs->UDCON |= USBC_UDCON_RMWKUP;

	if (is_clk_frozen) {
		usb_dc_sam_usbc_freeze_clk();
	}

	return 0;
}

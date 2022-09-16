/*
 * Copyright (c) 2018 Aurelien Jarno <aurelien@aurel32.net>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam_usbhs

#include <zephyr/usb/usb_device.h>
#include <soc.h>
#include <string.h>

#define LOG_LEVEL CONFIG_USB_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usb_dc_sam_usbhs);

/*
 * This is defined in the support files for the SAM S7x, but not for
 * the SAM E7x nor SAM V7x.
 */
#ifndef USBHS_RAM_ADDR
#define USBHS_RAM_ADDR		(0xA0100000)
#endif

/*
 * The new Atmel DFP headers provide mode-specific interrupt register field
 * definitions.  Map the existing generic definitions to these.
 */
#ifndef USBHS_DEVEPTISR_CTRL_RXSTPI
#define USBHS_DEVEPTISR_CTRL_RXSTPI	USBHS_DEVEPTISR_RXSTPI
#endif
#ifndef USBHS_DEVEPTICR_CTRL_RXSTPIC
#define USBHS_DEVEPTICR_CTRL_RXSTPIC	USBHS_DEVEPTICR_RXSTPIC
#endif
#ifndef USBHS_DEVEPTIMR_CTRL_STALLRQ
#define USBHS_DEVEPTIMR_CTRL_STALLRQ	USBHS_DEVEPTIMR_STALLRQ
#endif
#ifndef USBHS_DEVEPTIER_CTRL_RXSTPES
#define USBHS_DEVEPTIER_CTRL_RXSTPES	USBHS_DEVEPTIER_RXSTPES
#endif
#ifndef USBHS_DEVEPTIER_CTRL_STALLRQS
#define USBHS_DEVEPTIER_CTRL_STALLRQS	USBHS_DEVEPTIER_STALLRQS
#endif
#ifndef USBHS_DEVEPTIDR_CTRL_STALLRQC
#define USBHS_DEVEPTIDR_CTRL_STALLRQC	USBHS_DEVEPTIDR_STALLRQC
#endif

#define NUM_OF_EP_MAX		DT_INST_PROP(0, num_bidir_endpoints)
#define USB_MAXIMUM_SPEED	DT_INST_ENUM_IDX_OR(0, maximum_speed, 1)
BUILD_ASSERT(USB_MAXIMUM_SPEED, "low-speed is not supported");

struct usb_device_ep_data {
	uint16_t mps;
	usb_dc_ep_callback cb_in;
	usb_dc_ep_callback cb_out;
	uint8_t *fifo;
};

struct usb_device_data {
	bool addr_enabled;
	usb_dc_status_callback status_cb;
	struct usb_device_ep_data ep_data[NUM_OF_EP_MAX];
};

static struct usb_device_data dev_data;

/* Enable the USB device clock */
static void usb_dc_enable_clock(void)
{
	/* Start the USB PLL */
	PMC->CKGR_UCKR |= CKGR_UCKR_UPLLEN;

	/* Wait for it to be ready */
	while (!(PMC->PMC_SR & PMC_SR_LOCKU)) {
		k_yield();
	}

	/* In low power mode, provide a 48MHZ clock instead of the 480MHz one */
	if ((USBHS->USBHS_DEVCTRL & USBHS_DEVCTRL_SPDCONF_Msk)
	    == USBHS_DEVCTRL_SPDCONF_LOW_POWER) {
		/* Configure the USB_48M clock to be UPLLCK/10 */
		PMC->PMC_MCKR &= ~PMC_MCKR_UPLLDIV2;
		PMC->PMC_USB = PMC_USB_USBDIV(9) | PMC_USB_USBS;

		/* Enable USB_48M clock */
		PMC->PMC_SCER |= PMC_SCER_USBCLK;
	}
}

/* Disable the USB device clock */
static void usb_dc_disable_clock(void)
{
	/* Disable USB_48M clock */
	PMC->PMC_SCER &= ~PMC_SCER_USBCLK;

	/* Disable the USB PLL */
	PMC->CKGR_UCKR &= ~CKGR_UCKR_UPLLEN;
}

/* Check if the USB device is attached */
static bool usb_dc_is_attached(void)
{
	return (USBHS->USBHS_DEVCTRL & USBHS_DEVCTRL_DETACH) == 0;
}

/* Check if an endpoint is configured */
static bool usb_dc_ep_is_configured(uint8_t ep_idx)
{
	return USBHS->USBHS_DEVEPTISR[ep_idx] & USBHS_DEVEPTISR_CFGOK;
}

/* Check if an endpoint is enabled */
static bool usb_dc_ep_is_enabled(uint8_t ep_idx)
{
	return USBHS->USBHS_DEVEPT & BIT(USBHS_DEVEPT_EPEN0_Pos + ep_idx);
}

/* Reset and endpoint */
static void usb_dc_ep_reset(uint8_t ep_idx)
{
	USBHS->USBHS_DEVEPT |= BIT(USBHS_DEVEPT_EPRST0_Pos + ep_idx);
	USBHS->USBHS_DEVEPT &= ~BIT(USBHS_DEVEPT_EPRST0_Pos + ep_idx);
	__DSB();
}

/* Enable endpoint interrupts, depending of the type and direction */
static void usb_dc_ep_enable_interrupts(uint8_t ep_idx)
{
	if (ep_idx == 0U) {
		/* Control endpoint: enable SETUP and OUT */
		USBHS->USBHS_DEVEPTIER[ep_idx] = USBHS_DEVEPTIER_CTRL_RXSTPES;
		USBHS->USBHS_DEVEPTIER[ep_idx] = USBHS_DEVEPTIER_RXOUTES;
	} else if ((USBHS->USBHS_DEVEPTCFG[ep_idx] & USBHS_DEVEPTCFG_EPDIR_Msk)
		   == USBHS_DEVEPTCFG_EPDIR_IN) {
		/* IN direction: acknowledge FIFO empty interrupt */
		USBHS->USBHS_DEVEPTICR[ep_idx] = USBHS_DEVEPTICR_TXINIC;
		USBHS->USBHS_DEVEPTIER[ep_idx] = USBHS_DEVEPTIER_TXINES;
	} else {
		/* OUT direction */
		USBHS->USBHS_DEVEPTIER[ep_idx] = USBHS_DEVEPTIER_RXOUTES;
	}
}

/* Reset the endpoint FIFO pointer to the beginning of the endpoint memory */
static void usb_dc_ep_fifo_reset(uint8_t ep_idx)
{
	uint8_t *p;

	p = (uint8_t *)(USBHS_RAM_ADDR + 0x8000 * ep_idx);
	dev_data.ep_data[ep_idx].fifo = p;
}

/* Fetch a byte from the endpoint FIFO */
static uint8_t usb_dc_ep_fifo_get(uint8_t ep_idx)
{
	return *(dev_data.ep_data[ep_idx].fifo++);
}

/* Put a byte from the endpoint FIFO */
static void usb_dc_ep_fifo_put(uint8_t ep_idx, uint8_t data)
{
	*(dev_data.ep_data[ep_idx].fifo++) = data;
}

/* Handle interrupts on a control endpoint */
static void usb_dc_ep0_isr(void)
{
	uint32_t sr = USBHS->USBHS_DEVEPTISR[0] & USBHS->USBHS_DEVEPTIMR[0];
	uint32_t dev_ctrl = USBHS->USBHS_DEVCTRL;

	if (sr & USBHS_DEVEPTISR_CTRL_RXSTPI) {
		/* SETUP data received */
		usb_dc_ep_fifo_reset(0);
		dev_data.ep_data[0].cb_out(USB_EP_DIR_OUT, USB_DC_EP_SETUP);
	}
	if (sr & USBHS_DEVEPTISR_RXOUTI) {
		/* OUT (to device) data received */
		usb_dc_ep_fifo_reset(0);
		dev_data.ep_data[0].cb_out(USB_EP_DIR_OUT, USB_DC_EP_DATA_OUT);
	}
	if (sr & USBHS_DEVEPTISR_TXINI) {
		/* Disable the interrupt */
		USBHS->USBHS_DEVEPTIDR[0] = USBHS_DEVEPTIDR_TXINEC;

		/* IN (to host) transmit complete */
		usb_dc_ep_fifo_reset(0);
		dev_data.ep_data[0].cb_in(USB_EP_DIR_IN, USB_DC_EP_DATA_IN);

		if (!(dev_ctrl & USBHS_DEVCTRL_ADDEN) &&
		    (dev_ctrl & USBHS_DEVCTRL_UADD_Msk) != 0U) {
			/* Commit the pending address update.  This
			 * must be done after the ack to the host
			 * completes else the ack will get dropped.
			 */
			USBHS->USBHS_DEVCTRL = dev_ctrl | USBHS_DEVCTRL_ADDEN;
		}
	}
}

/* Handle interrupts on a non-control endpoint */
static void usb_dc_ep_isr(uint8_t ep_idx)
{
	uint32_t sr = USBHS->USBHS_DEVEPTISR[ep_idx] &
		   USBHS->USBHS_DEVEPTIMR[ep_idx];

	if (sr & USBHS_DEVEPTISR_RXOUTI) {
		uint8_t ep = ep_idx | USB_EP_DIR_OUT;

		/* Acknowledge the interrupt */
		USBHS->USBHS_DEVEPTICR[ep_idx] = USBHS_DEVEPTICR_RXOUTIC;

		/* OUT (to device) data received */
		usb_dc_ep_fifo_reset(ep_idx);
		dev_data.ep_data[ep_idx].cb_out(ep, USB_DC_EP_DATA_OUT);
	}
	if (sr & USBHS_DEVEPTISR_TXINI) {
		uint8_t ep = ep_idx | USB_EP_DIR_IN;

		/* Acknowledge the interrupt */
		USBHS->USBHS_DEVEPTICR[ep_idx] = USBHS_DEVEPTICR_TXINIC;

		/* IN (to host) transmit complete */
		usb_dc_ep_fifo_reset(ep_idx);
		dev_data.ep_data[ep_idx].cb_in(ep, USB_DC_EP_DATA_IN);
	}
}

/* Top level interrupt handler */
static void usb_dc_isr(void)
{
	uint32_t sr = USBHS->USBHS_DEVISR & USBHS->USBHS_DEVIMR;

	/* End of resume interrupt */
	if (sr & USBHS_DEVISR_EORSM) {
		/* Acknowledge the interrupt */
		USBHS->USBHS_DEVICR = USBHS_DEVICR_EORSMC;

		/* Callback function */
		dev_data.status_cb(USB_DC_RESUME, NULL);
	}

	/* End of reset interrupt */
	if (sr & USBHS_DEVISR_EORST) {
		/* Acknowledge the interrupt */
		USBHS->USBHS_DEVICR = USBHS_DEVICR_EORSTC;

		if (usb_dc_ep_is_enabled(0)) {
			/* The device clears some of the configuration of EP0
			 * when it receives the EORST.  Re-enable interrupts.
			 */
			usb_dc_ep_enable_interrupts(0);
		}

		/* Free all endpoint memory */
		for (int idx = 1; idx < NUM_OF_EP_MAX; idx++) {
			usb_dc_ep_disable(idx);
			USBHS->USBHS_DEVEPTCFG[idx] &= ~USBHS_DEVEPTCFG_ALLOC;
		}

		/* Callback function */
		dev_data.status_cb(USB_DC_RESET, NULL);
	}

	/* Suspend interrupt */
	if (sr & USBHS_DEVISR_SUSP) {
		/* Acknowledge the interrupt */
		USBHS->USBHS_DEVICR = USBHS_DEVICR_SUSPC;

		/* Callback function */
		dev_data.status_cb(USB_DC_SUSPEND, NULL);
	}

#ifdef CONFIG_USB_DEVICE_SOF
	/* SOF interrupt */
	if (sr & USBHS_DEVISR_SOF) {
		/* Acknowledge the interrupt */
		USBHS->USBHS_DEVICR = USBHS_DEVICR_SOFC;

		/* Callback function */
		dev_data.status_cb(USB_DC_SOF, NULL);
	}
#endif

	/* EP0 endpoint interrupt */
	if (sr & USBHS_DEVISR_PEP_0) {
		usb_dc_ep0_isr();
	}

	/* Other endpoints interrupt */
	for (int ep_idx = 1; ep_idx < NUM_OF_EP_MAX; ep_idx++) {
		if (sr & BIT(USBHS_DEVISR_PEP_0_Pos + ep_idx)) {
			usb_dc_ep_isr(ep_idx);
		}
	}
}

/* Attach USB for device connection */
int usb_dc_attach(void)
{
	uint32_t regval;

	/* Start the peripheral clock */
	soc_pmc_peripheral_enable(DT_INST_PROP(0, peripheral_id));

	/* Enable the USB controller in device mode with the clock frozen */
	USBHS->USBHS_CTRL = USBHS_CTRL_UIMOD | USBHS_CTRL_USBE |
			    USBHS_CTRL_FRZCLK;
	__DSB();

	/* Select the speed */
	regval = USBHS_DEVCTRL_DETACH;
#if (USB_MAXIMUM_SPEED == 2) && IS_ENABLED(CONFIG_USB_DC_HAS_HS_SUPPORT)
	/* high-speed */
	regval |= USBHS_DEVCTRL_SPDCONF_NORMAL;
#else
	/* full-speed */
	regval |= USBHS_DEVCTRL_SPDCONF_LOW_POWER;
#endif
	USBHS->USBHS_DEVCTRL = regval;

	/* Enable the USB clock */
	usb_dc_enable_clock();

	/* Unfreeze the clock */
	USBHS->USBHS_CTRL = USBHS_CTRL_UIMOD | USBHS_CTRL_USBE;

	/* Enable device interrupts */
	USBHS->USBHS_DEVIER = USBHS_DEVIER_EORSMES;
	USBHS->USBHS_DEVIER = USBHS_DEVIER_EORSTES;
	USBHS->USBHS_DEVIER = USBHS_DEVIER_SUSPES;
#ifdef CONFIG_USB_DEVICE_SOF
	USBHS->USBHS_DEVIER = USBHS_DEVIER_SOFES;
#endif

	/* Connect and enable the interrupt */
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
		    usb_dc_isr, 0, 0);
	irq_enable(DT_INST_IRQN(0));

	/* Attach the device */
	USBHS->USBHS_DEVCTRL &= ~USBHS_DEVCTRL_DETACH;

	LOG_DBG("");
	return 0;
}

/* Detach the USB device */
int usb_dc_detach(void)
{
	/* Detach the device */
	USBHS->USBHS_DEVCTRL &= ~USBHS_DEVCTRL_DETACH;

	/* Disable the USB clock */
	usb_dc_disable_clock();

	/* Disable the USB controller and freeze the clock */
	USBHS->USBHS_CTRL = USBHS_CTRL_UIMOD | USBHS_CTRL_FRZCLK;

	/* Disable the peripheral clock */
	soc_pmc_peripheral_enable(DT_INST_PROP(0, peripheral_id));

	/* Disable interrupt */
	irq_disable(DT_INST_IRQN(0));

	LOG_DBG("");
	return 0;
}

/* Reset the USB device */
int usb_dc_reset(void)
{
	/* Reset the controller */
	USBHS->USBHS_CTRL = USBHS_CTRL_UIMOD | USBHS_CTRL_FRZCLK;

	/* Clear private data */
	(void)memset(&dev_data, 0, sizeof(dev_data));

	LOG_DBG("");
	return 0;
}

/* Set USB device address */
int usb_dc_set_address(uint8_t addr)
{
	/*
	 * Set the address but keep it disabled for now. It should be enabled
	 * only after the ack to the host completes.
	 */
	USBHS->USBHS_DEVCTRL &= ~(USBHS_DEVCTRL_UADD_Msk | USBHS_DEVCTRL_ADDEN);
	USBHS->USBHS_DEVCTRL |= USBHS_DEVCTRL_UADD(addr);
	LOG_DBG("");

	return 0;
}

/* Set USB device controller status callback */
void usb_dc_set_status_callback(const usb_dc_status_callback cb)
{
	LOG_DBG("");

	dev_data.status_cb = cb;
}

/* Check endpoint capabilities */
int usb_dc_ep_check_cap(const struct usb_dc_ep_cfg_data * const cfg)
{
	uint8_t ep_idx = USB_EP_GET_IDX(cfg->ep_addr);

	if (ep_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("endpoint index/address out of range");
		return -1;
	}

	if (ep_idx == 0U) {
		if (cfg->ep_type != USB_DC_EP_CONTROL) {
			LOG_ERR("pre-selected as control endpoint");
			return -1;
		}
	} else if (ep_idx & BIT(0)) {
		if (USB_EP_GET_DIR(cfg->ep_addr) != USB_EP_DIR_IN) {
			LOG_INF("pre-selected as IN endpoint");
			return -1;
		}
	} else {
		if (USB_EP_GET_DIR(cfg->ep_addr) != USB_EP_DIR_OUT) {
			LOG_INF("pre-selected as OUT endpoint");
			return -1;
		}
	}

	if (cfg->ep_mps < 1 || cfg->ep_mps > 1024 ||
	    (cfg->ep_type == USB_DC_EP_CONTROL && cfg->ep_mps > 64)) {
		LOG_ERR("invalid endpoint size");
		return -1;
	}

	return 0;
}

/* Configure endpoint */
int usb_dc_ep_configure(const struct usb_dc_ep_cfg_data *const cfg)
{
	uint8_t ep_idx = USB_EP_GET_IDX(cfg->ep_addr);
	bool ep_configured[NUM_OF_EP_MAX];
	bool ep_enabled[NUM_OF_EP_MAX];
	uint32_t regval = 0U;
	int log2ceil_mps;

	if (usb_dc_ep_check_cap(cfg) != 0) {
		return -EINVAL;
	}

	if (!usb_dc_is_attached()) {
		LOG_ERR("device not attached");
		return -ENODEV;
	}

	if (usb_dc_ep_is_enabled(ep_idx)) {
		LOG_WRN("endpoint already configured & enabled 0x%x", ep_idx);
		return -EBUSY;
	}

	LOG_INF("Configure ep %x, mps %d, type %d", cfg->ep_addr, cfg->ep_mps,
		cfg->ep_type);

	/* Reset the endpoint */
	usb_dc_ep_reset(ep_idx);

	/* Map the endpoint type */
	switch (cfg->ep_type) {
	case USB_DC_EP_CONTROL:
		regval |= USBHS_DEVEPTCFG_EPTYPE_CTRL;
		break;
	case USB_DC_EP_ISOCHRONOUS:
		regval |= USBHS_DEVEPTCFG_EPTYPE_ISO;
		break;
	case USB_DC_EP_BULK:
		regval |= USBHS_DEVEPTCFG_EPTYPE_BLK;
		break;
	case USB_DC_EP_INTERRUPT:
		regval |= USBHS_DEVEPTCFG_EPTYPE_INTRPT;
		break;
	default:
		return -EINVAL;
	}

	/* Map the endpoint direction */
	if (USB_EP_DIR_IS_OUT(cfg->ep_addr) ||
	    cfg->ep_type == USB_DC_EP_CONTROL) {
		regval |= USBHS_DEVEPTCFG_EPDIR_OUT;
	} else {
		regval |= USBHS_DEVEPTCFG_EPDIR_IN;
	}

	/*
	 * Map the endpoint size to the buffer size. Only power of 2 buffer
	 * sizes between 8 and 1024 are possible, get the next power of 2.
	 */
	log2ceil_mps = 32 - __builtin_clz((MAX(cfg->ep_mps, 8) << 1) - 1) - 1;
	regval |= USBHS_DEVEPTCFG_EPSIZE(log2ceil_mps - 3);
	dev_data.ep_data[ep_idx].mps = cfg->ep_mps;

	/* Use double bank buffering for isochronous endpoints */
	if (cfg->ep_type == USB_DC_EP_ISOCHRONOUS) {
		regval |= USBHS_DEVEPTCFG_EPBK_2_BANK;
	} else {
		regval |= USBHS_DEVEPTCFG_EPBK_1_BANK;
	}

	/* Configure the endpoint */
	USBHS->USBHS_DEVEPTCFG[ep_idx] = regval;

	/*
	 * Allocate the memory. This part is a bit tricky as memory can only be
	 * allocated if all above endpoints are disabled and not allocated. Loop
	 * backward through the above endpoints, disable them if they are
	 * enabled, deallocate their memory if needed. Then loop again through
	 * all the above endpoints to allocate and enabled them.
	 */
	for (int i = NUM_OF_EP_MAX - 1; i > ep_idx; i--) {
		ep_configured[i] = usb_dc_ep_is_configured(i);
		ep_enabled[i] = usb_dc_ep_is_enabled(i);

		if (ep_enabled[i]) {
			LOG_INF("Temporary disable ep idx %x", i);
			usb_dc_ep_disable(i);
		}
		if (ep_configured[i]) {
			USBHS->USBHS_DEVEPTCFG[i] &= ~USBHS_DEVEPTCFG_ALLOC;
		}
	}
	ep_configured[ep_idx] = true;
	ep_enabled[ep_idx] = false;
	for (int i = ep_idx; i < NUM_OF_EP_MAX; i++) {
		if (ep_configured[i]) {
			USBHS->USBHS_DEVEPTCFG[i] |= USBHS_DEVEPTCFG_ALLOC;
		}
		if (ep_enabled[i]) {
			usb_dc_ep_enable(i);
		}
	}

	/* Check that the endpoint is correctly configured */
	if (!usb_dc_ep_is_configured(ep_idx)) {
		LOG_ERR("endpoint configuration failed");
		return -EINVAL;
	}

	return 0;
}

/* Set stall condition for the selected endpoint */
int usb_dc_ep_set_stall(uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	if (ep_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("wrong endpoint index/address");
		return -EINVAL;
	}

	USBHS->USBHS_DEVEPTIER[ep_idx] = USBHS_DEVEPTIER_CTRL_STALLRQS;

	LOG_DBG("ep 0x%x", ep);
	return 0;
}

/* Clear stall condition for the selected endpoint */
int usb_dc_ep_clear_stall(uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	if (ep_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("wrong endpoint index/address");
		return -EINVAL;
	}

	USBHS->USBHS_DEVEPTIDR[ep_idx] = USBHS_DEVEPTIDR_CTRL_STALLRQC;

	LOG_DBG("ep 0x%x", ep);
	return 0;
}

/* Check if the selected endpoint is stalled */
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

	*stalled = (USBHS->USBHS_DEVEPTIMR[ep_idx] &
		    USBHS_DEVEPTIMR_CTRL_STALLRQ) != 0;

	LOG_DBG("ep 0x%x", ep);
	return 0;
}

/* Halt the selected endpoint */
int usb_dc_ep_halt(uint8_t ep)
{
	return usb_dc_ep_set_stall(ep);
}

/* Enable the selected endpoint */
int usb_dc_ep_enable(uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	if (ep_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("wrong endpoint index/address");
		return -EINVAL;
	}

	if (!usb_dc_ep_is_configured(ep_idx)) {
		LOG_ERR("endpoint not configured");
		return -ENODEV;
	}

	/* Enable endpoint */
	USBHS->USBHS_DEVEPT |= BIT(USBHS_DEVEPT_EPEN0_Pos + ep_idx);

	/* Enable endpoint interrupts */
	USBHS->USBHS_DEVIER = BIT(USBHS_DEVIER_PEP_0_Pos + ep_idx);

	/* Enable SETUP, IN or OUT endpoint interrupts */
	usb_dc_ep_enable_interrupts(ep_idx);

	LOG_INF("Enable ep 0x%x", ep);

	return 0;
}

/* Disable the selected endpoint */
int usb_dc_ep_disable(uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	if (ep_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("wrong endpoint index/address");
		return -EINVAL;
	}

	/* Disable endpoint interrupt */
	USBHS->USBHS_DEVIDR = BIT(USBHS_DEVIDR_PEP_0_Pos + ep_idx);

	/* Disable endpoint and SETUP, IN or OUT interrupts */
	USBHS->USBHS_DEVEPT &= ~BIT(USBHS_DEVEPT_EPEN0_Pos + ep_idx);

	LOG_INF("Disable ep 0x%x", ep);

	return 0;
}

/* Flush the selected endpoint */
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
	USBHS->USBHS_DEVEPTIDR[ep_idx] = USBHS_DEVEPTIDR_TXINEC;

	/* Kill the last written bank if needed */
	if (USBHS->USBHS_DEVEPTISR[ep_idx] & USBHS_DEVEPTISR_NBUSYBK_Msk) {
		USBHS->USBHS_DEVEPTIER[ep_idx] = USBHS_DEVEPTIER_KILLBKS;
		__DSB();
		while (USBHS->USBHS_DEVEPTIMR[ep_idx] &
		       USBHS_DEVEPTIMR_KILLBK) {
			k_yield();
		}
	}

	/* Reset the endpoint */
	usb_dc_ep_reset(ep_idx);

	/* Reenable interrupts */
	usb_dc_ep_enable_interrupts(ep_idx);

	LOG_DBG("ep 0x%x", ep);
	return 0;
}

/* Write data to the specified endpoint */
int usb_dc_ep_write(uint8_t ep, const uint8_t *data, uint32_t data_len, uint32_t *ret_bytes)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	uint32_t packet_len;

	if (ep_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("wrong endpoint index/address");
		return -EINVAL;
	}

	if (!usb_dc_ep_is_enabled(ep_idx)) {
		LOG_ERR("endpoint not enabled");
		return -ENODEV;
	}

	if (USB_EP_GET_DIR(ep) != USB_EP_DIR_IN) {
		LOG_ERR("wrong endpoint direction");
		return -EINVAL;
	}

	if ((USBHS->USBHS_DEVEPTIMR[ep_idx] & USBHS_DEVEPTIMR_CTRL_STALLRQ)
		!= 0) {
		LOG_WRN("endpoint is stalled");
		return -EBUSY;
	}

	/* Write the data to the FIFO */
	packet_len = MIN(data_len, dev_data.ep_data[ep_idx].mps);
	for (int i = 0; i < packet_len; i++) {
		usb_dc_ep_fifo_put(ep_idx, data[i]);
	}
	__DSB();

	if (ep_idx == 0U) {
		/*
		 * Control endpoint: clear the interrupt flag to send the data,
		 * and re-enable the interrupts to trigger an interrupt at the
		 * end of the transfer.
		 */
		USBHS->USBHS_DEVEPTICR[ep_idx] = USBHS_DEVEPTICR_TXINIC;
		USBHS->USBHS_DEVEPTIER[ep_idx] = USBHS_DEVEPTIER_TXINES;
	} else {
		/*
		 * Other endpoint types: clear the FIFO control flag to send
		 * the data.
		 */
		USBHS->USBHS_DEVEPTIDR[ep_idx] = USBHS_DEVEPTIDR_FIFOCONC;
	}

	if (ret_bytes) {
		*ret_bytes = packet_len;
	}

	LOG_DBG("ep 0x%x write %d bytes from %d", ep, packet_len, data_len);
	return 0;
}

/* Read data from the specified endpoint */
int usb_dc_ep_read(uint8_t ep, uint8_t *data, uint32_t max_data_len, uint32_t *read_bytes)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	int rc;

	rc = usb_dc_ep_read_wait(ep, data, max_data_len, read_bytes);

	if (rc) {
		return rc;
	}

	if (!data && !max_data_len) {
		/* When both buffer and max data to read are zero the above
		 * call would fetch the data len and we simply return.
		 */
		return 0;
	}

	/* If the packet has been read entirely, get the next one */
	if (!(USBHS->USBHS_DEVEPTISR[ep_idx] & USBHS_DEVEPTISR_RWALL)) {
		rc = usb_dc_ep_read_continue(ep);
	}

	LOG_DBG("ep 0x%x", ep);
	return rc;
}

/* Set callback function for the specified endpoint */
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

	LOG_DBG("ep 0x%x", ep);
	return 0;
}

/* Read data from the specified endpoint */
int usb_dc_ep_read_wait(uint8_t ep, uint8_t *data, uint32_t max_data_len,
			uint32_t *read_bytes)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	uint32_t data_len = (USBHS->USBHS_DEVEPTISR[ep_idx] &
			  USBHS_DEVEPTISR_BYCT_Msk) >> USBHS_DEVEPTISR_BYCT_Pos;

	if (ep_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("wrong endpoint index/address");
		return -EINVAL;
	}

	if (!usb_dc_ep_is_enabled(ep_idx)) {
		LOG_ERR("endpoint not enabled");
		return -ENODEV;
	}

	if (USB_EP_GET_DIR(ep) != USB_EP_DIR_OUT) {
		LOG_ERR("wrong endpoint direction");
		return -EINVAL;
	}

	if ((USBHS->USBHS_DEVEPTIMR[ep_idx] & USBHS_DEVEPTIMR_CTRL_STALLRQ)
		!= 0) {
		LOG_WRN("endpoint is stalled");
		return -EBUSY;
	}

	if (!data && !max_data_len) {
		/*
		 * When both buffer and max data to read are zero return
		 * the available data in buffer.
		 */
		if (read_bytes) {
			*read_bytes = data_len;
		}
		return 0;
	}

	if (data_len > max_data_len) {
		LOG_WRN("Not enough space to copy all the data!");
		data_len = max_data_len;
	}

	if (data != NULL) {
		for (int i = 0; i < data_len; i++) {
			data[i] = usb_dc_ep_fifo_get(ep_idx);
		}
	}

	if (read_bytes) {
		*read_bytes = data_len;
	}

	LOG_DBG("ep 0x%x read %d bytes", ep, data_len);
	return 0;
}

/* Continue reading data from the endpoint */
int usb_dc_ep_read_continue(uint8_t ep)
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

	if (USB_EP_GET_DIR(ep) != USB_EP_DIR_OUT) {
		LOG_ERR("wrong endpoint direction");
		return -EINVAL;
	}

	if (ep_idx == 0U) {
		/*
		 * Control endpoint: clear the interrupt flag to send the data.
		 * It is easier to clear both SETUP and OUT flag than checking
		 * the stage of the transfer.
		 */
		USBHS->USBHS_DEVEPTICR[ep_idx] = USBHS_DEVEPTICR_RXOUTIC;
		USBHS->USBHS_DEVEPTICR[ep_idx] = USBHS_DEVEPTICR_CTRL_RXSTPIC;
	} else {
		/*
		 * Other endpoint types: clear the FIFO control flag to
		 * receive more data.
		 */
		USBHS->USBHS_DEVEPTIDR[ep_idx] = USBHS_DEVEPTIDR_FIFOCONC;
	}

	LOG_DBG("ep 0x%x continue", ep);
	return 0;
}

/* Endpoint max packet size (mps) */
int usb_dc_ep_mps(uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	if (ep_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("wrong endpoint index/address");
		return -EINVAL;
	}

	return dev_data.ep_data[ep_idx].mps;
}

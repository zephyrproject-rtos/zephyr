/*
 * Copyright (c) 2025 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_nct_usbd

#define LOG_LEVEL CONFIG_USB_DRIVER_LOG_LEVEL
#include <zephyr/kernel.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/dt-bindings/usb/usb.h>
#include <zephyr/drivers/pinctrl.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usb_dc_nct);

#include <zephyr/usb/usb_device.h>
#include <soc.h>
#include <string.h>

/* Timeout for USB PHY clock ready. (Unit:ms) */
#define NCT_USB_PHY_TIMEOUT            (50)
/* Timeout for USB Write Data to Host. (Unit:ms) */
#define NCT_USB_WRITE_TIMEOUT          (200)

#define NUM_OF_EP_MAX                   (DT_INST_PROP(0, num_bidir_endpoints))
#define USB_RAM_SIZE                    (DT_INST_PROP(0, usbd_ram_size))
#define USBD_BASE_ADDR                  (DT_INST_REG_ADDR(0))
#define USBD                            ((struct usbd_reg *) USBD_BASE_ADDR)

#define USBD_EP_CFG_TYPE_BULK           ((uint32_t)0x00000002ul)
#define USBD_EP_CFG_TYPE_INT            ((uint32_t)0x00000004ul)
#define USBD_EP_CFG_TYPE_ISO            ((uint32_t)0x00000006ul)
#define USBD_EP_CFG_TYPE_MASK           ((uint32_t)0x00000006ul)
#define USBD_EP_CFG_DIR_OUT             ((uint32_t)0x00000000ul)
#define USBD_EP_CFG_DIR_IN              ((uint32_t)0x00000008ul)

#define USBD_SET_CEP_STATE(flag)        (USBD->USBD_CEPCTL = (flag))
#define USBD_CEPCTL_NAKCLR              ((uint32_t)0x00000000ul)        /* NAK clear */
#define USBD_CEPCTL_STALL               ((uint32_t)0x00000002ul)        /* Stall */
#define USBD_CEPCTL_ZEROLEN             ((uint32_t)0x00000004ul)        /* Zero length packet */
#define USBD_CEPCTL_FLUSH               ((uint32_t)0x00000008ul)        /* CEP flush  */

#define CEP_BUF_BASE                    (0)
#define CEP_MAX_PKT_SIZE                (64)
#define EP_MAX_PKT_SIZE                 (1024)

#ifndef REQTYPE_GET_DIR
#define REQTYPE_GET_DIR(x)		(((x)>>7)&0x01)
#define REQTYPE_GET_TYPE(x)		(((x)>>5)&0x03U)
#define REQTYPE_GET_RECIP(x)		((x)&0x1F)

#define REQTYPE_DIR_TO_DEVICE		0
#define REQTYPE_DIR_TO_HOST		1
#endif

struct nct_usbd_config {
	struct usbd_reg *base;
	const struct pinctrl_dev_config *pincfg;
	uint32_t num_bidir_endpoints;
};

struct usb_device_ep_data {
	volatile uint16_t mps;
	uint16_t reserved;
	volatile uint32_t ram_base;
	volatile uint32_t type;
	usb_dc_ep_callback cb_in;
	usb_dc_ep_callback cb_out;
};

struct usb_device_data {
	volatile uint32_t ram_offset;
	volatile uint32_t ep_status;
	volatile uint32_t req_type;
	volatile uint32_t req_len;
	volatile uint32_t set_addr_req;
	volatile uint32_t new_addr;
	usb_dc_status_callback status_cb;
	struct usb_device_ep_data ep_data[NUM_OF_EP_MAX];
};

static struct usb_device_data dev_data;

/* Endpoint index */
enum {
	EPA = 0,
	EPB,
	EPC,
	EPD,
	EPE,
	EPF,
	EPG,
	EPH,
	EPI,
	EPJ,
	EPK,
	EPL,
	CEP = 0xFF,
};

static inline const struct device *nct_usbd_device_get(void)
{
	return DEVICE_DT_INST_GET(0);
}

/* USBD inline functions */
static inline void usbd_config_ep(uint32_t ep, uint32_t ep_type, uint32_t ep_dir)
{
	if (ep_type == USBD_EP_CFG_TYPE_BULK) {
		/* manual mode */
		USBD->EP[ep - 1].USBD_EPRSPCTL = (BIT(NCT_USBD_EPRSPCTL_FLUSH) | (0x01 << 1));
	} else if (ep_type == USBD_EP_CFG_TYPE_INT) {
		/* manual mode */
		USBD->EP[ep - 1].USBD_EPRSPCTL = (BIT(NCT_USBD_EPRSPCTL_FLUSH) | (0x01 << 1));
	} else if (ep_type == USBD_EP_CFG_TYPE_ISO) {
		/* fly mode */
		USBD->EP[ep - 1].USBD_EPRSPCTL = (BIT(NCT_USBD_EPRSPCTL_FLUSH) | (0x10 << 1));
	}

	USBD->EP[ep - 1].USBD_EPCFG = (ep_type | ep_dir | ((ep) << 4));
}

static inline void usbd_set_ep_max_payload(uint32_t ep, uint32_t size)
{
	USBD->EP[ep - 1].USBD_EPMPS = size;
}

static inline void usbd_set_ep_buf_addr(uint32_t ep, uint32_t base, uint32_t len)
{
	if (ep == CEP) {
		USBD->USBD_CEPBUFSTART = base;
		USBD->USBD_CEPBUFEND = base + len - 1ul;
	} else {
		USBD->EP[ep - 1].USBD_EPBUFSTART = base;
		USBD->EP[ep - 1].USBD_EPBUFEND = base + len - 1ul;
	}
	LOG_DBG("ep %x, [base 0x%x, len 0x%x]", ep, base, len);
}

static inline bool usbd_ep_is_enabled(uint8_t ep_idx)
{
	if (ep_idx == 0) {
		return 1;
	} else {
		return IS_BIT_SET(USBD->EP[ep_idx - 1].USBD_EPCFG, NCT_USBD_EPCFG_EPEN);
	}

}

static inline void usbd_reset_dma(void)
{
	USBD->USBD_DMACNT = 0ul;
	USBD->USBD_DMACTL = 0x80ul;
	USBD->USBD_DMACTL = 0x00ul;
}

static inline void usbd_flush_all_ep(void)
{
	uint8_t i;

	for (i = EPA; i <= EPL; i++) {
		USBD->EP[i].USBD_EPRSPCTL = BIT(NCT_USBD_EPRSPCTL_FLUSH) | BIT(1);
	}
}

/* USBD local functions */
static int usbd_enable_phy(void)
{
	uint64_t st;

	/* Enable USB PHY */
	USBD->USBD_PHYCTL |= BIT(NCT_USBD_PHYCTL_PHYEN);

	/* Wait PHY clock ready */
	st = k_uptime_get();
	while (1) {
		if (k_uptime_get() - st > NCT_USB_PHY_TIMEOUT) {
			LOG_ERR("Timeout: USB PHY!");
			return -ETIMEDOUT;
		}
		USBD->EP[EPA].USBD_EPMPS = 0x20;
		if (USBD->EP[EPA].USBD_EPMPS == 0x20ul) {
			USBD->EP[EPA].USBD_EPMPS = 0x0ul;
			break;
		}
	}

	return 0;
}

/* Handle interrupts on a control endpoint */
static void usb_dc_cep_isr(void)
{
	volatile uint32_t IrqSt;

	IrqSt = USBD->USBD_CEPINTSTS & USBD->USBD_CEPINTEN;

	/* Setup Token */
	if (IS_BIT_SET(IrqSt, NCT_USBD_CEPINTSTS_SETUPTKIF)) {
		/* Clear CEP interrupt flag */
		USBD->USBD_CEPINTSTS = BIT(NCT_USBD_CEPINTSTS_SETUPTKIF);
		return;
	}

	/* Setup Packet */
	if (IS_BIT_SET(IrqSt, NCT_USBD_CEPINTSTS_SETUPPKIF)) {
		LOG_DBG("\r\n\r\nSETUP pkt");

		dev_data.ep_status = USB_DC_EP_SETUP;
		dev_data.ep_data[0].cb_out(USB_EP_DIR_OUT, USB_DC_EP_SETUP);
		return;
	}

	/* Control OUT Token */
	if (IS_BIT_SET(IrqSt, NCT_USBD_CEPINTSTS_OUTTKIF)) {
		/* Clear CEP interrupt flag */
		USBD->USBD_CEPINTSTS = BIT(NCT_USBD_CEPINTSTS_OUTTKIF);

		LOG_DBG("\r\n\r\nCOUT pkt");
		return;
	}

	/* Control IN Token */
	if (IS_BIT_SET(IrqSt, NCT_USBD_CEPINTSTS_INTKIF)) {
		/* Clear CEP interrupt flag */
		USBD->USBD_CEPINTSTS = BIT(NCT_USBD_CEPINTSTS_INTKIF);

		LOG_DBG("\r\n\r\nCIN pkt");

		/* Start CEP IN Transfer */
		dev_data.ep_status = USB_DC_EP_DATA_IN;
		dev_data.ep_data[0].cb_in(USB_EP_DIR_IN, USB_DC_EP_DATA_IN);
		return;
	}

	/* Ping Token */
	if (IS_BIT_SET(IrqSt, NCT_USBD_CEPINTSTS_PINGIF)) {
		/* Clear CEP interrupt flag */
		USBD->USBD_CEPINTSTS = BIT(NCT_USBD_CEPINTSTS_PINGIF);
		return;
	}

	/* Data Packet Transmitted and Acked */
	if (IS_BIT_SET(IrqSt, NCT_USBD_CEPINTSTS_TXPKIF)) {
		/* Clear CEP interrupt flag */
		USBD->USBD_CEPINTSTS = BIT(NCT_USBD_CEPINTSTS_TXPKIF);

		LOG_DBG("\r\n\r\nTXPK");
		return;
	}

	/* Host received */
	if (IS_BIT_SET(IrqSt, NCT_USBD_CEPINTSTS_RXPKIF)) {
		/* Clear CEP interrupt flag */
		USBD->USBD_CEPINTSTS = BIT(NCT_USBD_CEPINTSTS_RXPKIF);

		LOG_DBG("\r\n\r\nRXPK");

		dev_data.ep_status = USB_DC_EP_DATA_OUT;
		dev_data.ep_data[0].cb_out(USB_EP_DIR_OUT, USB_DC_EP_DATA_OUT);
		return;
	}

	/* NAK sent */
	if (IS_BIT_SET(IrqSt, NCT_USBD_CEPINTSTS_NAKIF)) {
		/* Clear CEP interrupt flag */
		USBD->USBD_CEPINTSTS = BIT(NCT_USBD_CEPINTSTS_NAKIF);
		return;
	}

	/* STALL Sent */
	if (IS_BIT_SET(IrqSt, NCT_USBD_CEPINTSTS_STALLIF)) {
		/* Clear CEP interrupt flag */
		USBD->USBD_CEPINTSTS = BIT(NCT_USBD_CEPINTSTS_STALLIF);
		return;
	}

	/* USB Error */
	if (IS_BIT_SET(IrqSt, NCT_USBD_CEPINTSTS_ERRIF)) {
		/* Clear CEP interrupt flag */
		USBD->USBD_CEPINTSTS = BIT(NCT_USBD_CEPINTSTS_ERRIF);
		return;
	}

	/* Status Completion */
	if (IS_BIT_SET(IrqSt, NCT_USBD_CEPINTSTS_STSDONEIF)) {
		/* Enable CEP Interrupt */
		USBD->USBD_CEPINTEN = BIT(NCT_USBD_CEPINTEN_SETUPPKIEN);

		/* Assign address */
		if (dev_data.set_addr_req == true) {
			USBD->USBD_FADDR = dev_data.new_addr;
			dev_data.set_addr_req = false;
		}

		LOG_DBG("\r\n\r\nSTS");
		return;
	}

	/* Control Buffer Full */
	if (IS_BIT_SET(IrqSt, NCT_USBD_CEPINTSTS_BUFFULLIF)) {
		/* Clear CEP interrupt flag */
		USBD->USBD_CEPINTSTS = BIT(NCT_USBD_CEPINTSTS_BUFFULLIF);
		return;
	}

	/* Control Buffer Empty */
	if (IS_BIT_SET(IrqSt, NCT_USBD_CEPINTSTS_BUFEMPTYIF)) {
		/* Clear CEP interrupt flag */
		USBD->USBD_CEPINTSTS = BIT(NCT_USBD_CEPINTSTS_BUFEMPTYIF);
		return;
	}
}

/* Interrupt handler */
static void usb_dc_isr(void)
{
	volatile uint32_t IrqStL, IrqSt;
	uint8_t ep_idx = 0;
	uint8_t ep;

	/* get interrupt status */
	IrqStL = USBD->USBD_GINTSTS & USBD->USBD_GINTEN;
	if (!IrqStL) {
		return;
	}

	/* USB interrupt */
	if (IS_BIT_SET(IrqStL, NCT_USBD_GINTSTS_USBIF)) {
		/* Bus Status */
		IrqSt = USBD->USBD_BUSINTSTS & USBD->USBD_BUSINTEN;

		/* SOF */
		if (IS_BIT_SET(IrqSt, NCT_USBD_BUSINTSTS_SOFIF)) {
			/* Clear Bus interrupt flag */
			USBD->USBD_BUSINTSTS = BIT(NCT_USBD_BUSINTSTS_SOFIF);
		}

		/* Reset */
		if (IS_BIT_SET(IrqSt, NCT_USBD_BUSINTSTS_RSTIF)) {
			LOG_DBG("reset_isr");

			USBD->USBD_FADDR = 0; /* Reset USB device address */
			dev_data.new_addr = 0;
			dev_data.set_addr_req = false;

			usbd_reset_dma();
			usbd_flush_all_ep();

			if (IS_BIT_SET(USBD->USBD_OPER, NCT_USBD_OPER_CURSPD)) {
				/* high speed */
				LOG_DBG("hs");
			} else {
				/* full speed */
				LOG_DBG("fs");
			}

			/* Callback function */
			dev_data.status_cb(USB_DC_RESET, NULL);

			/* Enable CEP Interrupt */
			USBD->USBD_CEPINTEN = BIT(NCT_USBD_CEPINTEN_SETUPPKIEN);
			/* Enable BUS interrupt */
			USBD->USBD_BUSINTEN = BIT(NCT_USBD_BUSINTEN_RESUMEIEN) |
					      BIT(NCT_USBD_BUSINTEN_SUSPENDIEN) |
					      BIT(NCT_USBD_BUSINTEN_RSTIEN);

			/* Clear Bus interrupt flag */
			USBD->USBD_BUSINTSTS = BIT(NCT_USBD_BUSINTSTS_RSTIF);
			/* Clear all CEP interrupt flag */
			USBD->USBD_CEPINTSTS = 0x1FFC;
		}

		/* Resume */
		if (IS_BIT_SET(IrqSt, NCT_USBD_BUSINTSTS_RESUMEIF)) {
			/* Callback function */
			dev_data.status_cb(USB_DC_RESUME, NULL);

			USBD->USBD_BUSINTEN = BIT(NCT_USBD_BUSINTEN_RSTIEN) |
					      BIT(NCT_USBD_BUSINTEN_SUSPENDIEN);
			/* Clear Bus interrupt flag */
			USBD->USBD_BUSINTSTS = BIT(NCT_USBD_BUSINTSTS_RESUMEIF);
			LOG_DBG("RS");
		}

		/* Suspend Request */
		if (IS_BIT_SET(IrqSt, NCT_USBD_BUSINTSTS_SUSPENDIF)) {
			/* Callback function */
			dev_data.status_cb(USB_DC_SUSPEND, NULL);

			USBD->USBD_BUSINTEN = BIT(NCT_USBD_BUSINTEN_RSTIEN) |
					      BIT(NCT_USBD_BUSINTEN_RESUMEIEN);
			/* Clear Bus interrupt flag */
			USBD->USBD_BUSINTSTS = BIT(NCT_USBD_BUSINTSTS_SUSPENDIF);
			LOG_DBG("SP");
		}

		/* High speed */
		if (IS_BIT_SET(IrqSt, NCT_USBD_BUSINTSTS_HISPDIF)) {
			/* Enable CEP Interrupt */
			USBD->USBD_CEPINTEN = BIT(NCT_USBD_CEPINTEN_SETUPPKIEN);
			/* Clear Bus interrupt flag */
			USBD->USBD_BUSINTSTS = BIT(NCT_USBD_BUSINTSTS_HISPDIF);
		}

		/* DMA Completion */
		if (IS_BIT_SET(IrqSt, NCT_USBD_BUSINTSTS_DMADONEIF)) {
			if (IS_BIT_SET(USBD->USBD_DMACTL, NCT_USBD_DMACTL_DMARD)) {
				/* DMA read */
			} else {
				/* DMA write */
			}
			/* Clear Bus interrupt flag */
			USBD->USBD_BUSINTSTS = BIT(NCT_USBD_BUSINTSTS_DMADONEIF);
		}

		/* PHY clock is usable */
		if (IS_BIT_SET(IrqSt, NCT_USBD_BUSINTSTS_PHYCLKVLDIF)) {
			/* Clear Bus interrupt flag */
			USBD->USBD_BUSINTSTS = BIT(NCT_USBD_BUSINTSTS_PHYCLKVLDIF);
		}

		/* Hot-Plug */
		if (IS_BIT_SET(IrqSt, NCT_USBD_BUSINTSTS_VBUSDETIF)) {
			if (IS_BIT_SET(USBD->USBD_PHYCTL, NCT_USBD_PHYCTL_VBUSDET)) {
				/* USB Plug In */
				/* Enable USB */
				USBD->USBD_PHYCTL |= BIT(NCT_USBD_PHYCTL_PHYEN) |
						     BIT(NCT_USBD_PHYCTL_DPPUEN);
			} else {
				/* USB Un-plug */
				/* Disable USB */
				USBD->USBD_PHYCTL &= ~BIT(NCT_USBD_PHYCTL_DPPUEN);
			}
			/* Clear Bus interrupt flag */
			USBD->USBD_BUSINTSTS = BIT(NCT_USBD_BUSINTSTS_VBUSDETIF);
		}
	}

	/* Endpoint interrupt */
	if (IS_BIT_SET(IrqStL, NCT_USBD_GINTSTS_CEPIF)) {
		/* CEP interrupt */
		usb_dc_cep_isr();
	} else {
		/* Other endpoints interrupt */
		for (ep_idx = EPA; ep_idx <= EPL; ep_idx++) {
			if (IS_BIT_SET(IrqStL, (NCT_USBD_GINTSTS_EPAIF + ep_idx))) {
				IrqSt = USBD->EP[ep_idx].USBD_EPINTSTS &
					USBD->EP[ep_idx].USBD_EPINTEN;

				USBD->EP[ep_idx].USBD_EPINTSTS = IrqSt;
				if (IrqSt != 0) {
					if (IS_BIT_SET(USBD->EP[ep_idx].USBD_EPCFG,
						       NCT_USBD_EPCFG_EPDIR)) {
						LOG_DBG("\r\n\r\nEP_IN %d", ep_idx);

						USBD->EP[ep_idx].USBD_EPINTEN = 0;
						ep = ep_idx | USB_EP_DIR_IN;
						dev_data.ep_data[1 + ep_idx].cb_in(1 + ep,
										USB_DC_EP_DATA_IN);
					} else {
						LOG_DBG("\r\n\r\nEP_OUT %d", ep_idx);
						USBD->EP[ep_idx].USBD_EPINTEN = 0;
						ep = ep_idx | USB_EP_DIR_OUT;
						dev_data.ep_data[1 + ep_idx].cb_out(1 + ep,
										USB_DC_EP_DATA_OUT);
					}
				}
			}
		}
	}
}

/* Attach USB for device connection */
int usb_dc_attach(void)
{
	const struct device *dev = nct_usbd_device_get();
	const struct nct_usbd_config *config = dev->config;
	int32_t ret;

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		return ret;
	}

	ret = usbd_enable_phy();
	if (ret != 0) {
		return ret;
	}

	/* Connect and enable the interrupt */
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), usb_dc_isr, 0, 0);
	irq_enable(DT_INST_IRQN(0));

	/* Configure USB controller */
	/* Enable USB BUS interrupt */
	USBD->USBD_GINTEN = BIT(NCT_USBD_GINTEN_USBIEN);
	/* Enable BUS interrupt */
	USBD->USBD_BUSINTEN = BIT(NCT_USBD_BUSINTEN_RESUMEIEN) |
			      BIT(NCT_USBD_BUSINTEN_SUSPENDIEN) |
			      BIT(NCT_USBD_BUSINTEN_RSTIEN);
	/* Reset Address to 0 */
	USBD->USBD_FADDR = 0;

	/* HSUSBD_Start */
	USBD->USBD_OPER = BIT(NCT_USBD_OPER_HISPDEN);       /* high-speed */
	/* Enable USB, Disable SE0 */
	USBD->USBD_PHYCTL |= BIT(NCT_USBD_PHYCTL_DPPUEN);

	LOG_DBG("attach");
	return 0;
}

/* WDT api functions */
/* Detach the USB device */
int usb_dc_detach(void)
{
	/* Disable interrupt */
	irq_disable(DT_INST_IRQN(0));

	/* Disable USB PHY */
	USBD->USBD_PHYCTL &= ~BIT(NCT_USBD_PHYCTL_PHYEN);
	/* Disable USB, Enable SE0, Force USB PHY Transceiver to Drive SE0 */
	USBD->USBD_PHYCTL &= ~BIT(NCT_USBD_PHYCTL_DPPUEN);

	LOG_DBG("detach\r\n");
	return 0;
}

/* Reset the USB device */
int usb_dc_reset(void)
{
	/* Clear private data */
	(void)memset(&dev_data, 0, sizeof(dev_data));

	LOG_DBG("reset\r\n");
	return 0;
}

/* Set USB device address */
int usb_dc_set_address(const uint8_t addr)
{
	LOG_DBG("set addr 0x%x", addr);

	dev_data.set_addr_req = true;
	dev_data.new_addr = addr;

	return 0;
}

/* Set USB device controller status callback */
void usb_dc_set_status_callback(const usb_dc_status_callback cb)
{
	LOG_DBG("set status_cb");
	dev_data.status_cb = cb;
}

/* Check endpoint capabilities */
int usb_dc_ep_check_cap(const struct usb_dc_ep_cfg_data *const cfg)
{
	uint8_t ep_idx = USB_EP_GET_IDX(cfg->ep_addr);

	LOG_DBG("ep %x, mps %d, type %d", cfg->ep_addr, cfg->ep_mps, cfg->ep_type);

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


	if ((cfg->ep_mps < 1) || (cfg->ep_mps > EP_MAX_PKT_SIZE) ||
	    ((cfg->ep_type == USB_DC_EP_CONTROL) && (cfg->ep_mps > CEP_MAX_PKT_SIZE))) {
		LOG_ERR("invalid endpoint size");
		return -1;
	}

	return 0;
}

/* Configure endpoint */
int usb_dc_ep_configure(const struct usb_dc_ep_cfg_data *const cfg)
{
	uint8_t ep_idx = USB_EP_GET_IDX(cfg->ep_addr);
	uint32_t type = 0U;
	uint32_t dir = 0U;

	if (usb_dc_ep_check_cap(cfg) != 0) {
		return -EINVAL;
	}

	if (cfg->ep_type != USB_DC_EP_CONTROL) {
		if (usbd_ep_is_enabled(ep_idx) != 0) {
			LOG_WRN("endpoint already configured & enabled 0x%x", ep_idx);
			return 0;
		}
	}

	LOG_INF("Configure ep %x, mps %d, type %d", cfg->ep_addr, cfg->ep_mps, cfg->ep_type);

	if (cfg->ep_type == USB_DC_EP_CONTROL) {
		/* Set control ep buffer address */
		usbd_set_ep_buf_addr(CEP, CEP_BUF_BASE, cfg->ep_mps);

		dev_data.ep_data[ep_idx].mps = cfg->ep_mps;
		dev_data.ep_data[ep_idx].ram_base = dev_data.ram_offset;
		dev_data.ep_data[ep_idx].type = USB_DC_EP_CONTROL;
		dev_data.ram_offset = cfg->ep_mps;
	} else {
		/* Map the endpoint type */
		switch (cfg->ep_type) {
		case USB_DC_EP_ISOCHRONOUS:
			type = USBD_EP_CFG_TYPE_ISO;
			break;
		case USB_DC_EP_BULK:
			type = USBD_EP_CFG_TYPE_BULK;
			break;
		case USB_DC_EP_INTERRUPT:
			type = USBD_EP_CFG_TYPE_INT;
			break;
		default:
			return -EINVAL;
		}

		/* Map the endpoint direction */
		if (USB_EP_DIR_IS_OUT(cfg->ep_addr)) {
			dir = USBD_EP_CFG_DIR_OUT;
		} else {
			dir = USBD_EP_CFG_DIR_IN;
		}

		/* Check USB RAM size */
		if (USB_RAM_SIZE <= (dev_data.ram_offset + cfg->ep_mps)) {
			return -EINVAL;
		}

		usbd_set_ep_buf_addr(ep_idx, dev_data.ram_offset, cfg->ep_mps);
		usbd_set_ep_max_payload(ep_idx, cfg->ep_mps);
		usbd_config_ep(ep_idx, type, dir);

		dev_data.ep_data[ep_idx].mps = cfg->ep_mps;
		dev_data.ep_data[ep_idx].ram_base = dev_data.ram_offset;
		dev_data.ep_data[ep_idx].type = cfg->ep_type;
		dev_data.ram_offset += cfg->ep_mps;
	}

	return 0;
}

/* Set stall condition for the selected endpoint */
int usb_dc_ep_set_stall(const uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	uint32_t type = dev_data.ep_data[ep_idx].type;

	if (ep_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("wrong endpoint index/address");
		return -EINVAL;
	}

	if (type == USB_DC_EP_CONTROL) {
		USBD->USBD_CEPINTSTS = BIT(NCT_USBD_CEPINTSTS_SETUPPKIF);
		USBD_SET_CEP_STATE(USBD_CEPCTL_STALL);
		USBD->USBD_CEPINTEN = BIT(NCT_USBD_CEPINTEN_SETUPPKIEN);
	} else {
		USBD->EP[ep_idx - 1].USBD_EPRSPCTL = (USBD->EP[ep_idx - 1].USBD_EPRSPCTL & 0xf7ul) |
						     BIT(NCT_USBD_EPRSPCTL_HALT);
	}

	LOG_DBG("ep 0x%x", ep);
	return 0;
}

/* Clear stall condition for the selected endpoint */
int usb_dc_ep_clear_stall(const uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	uint32_t type = dev_data.ep_data[ep_idx].type;

	if (ep_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("wrong endpoint index/address");
		return -EINVAL;
	}

	if (type == USB_DC_EP_CONTROL) {
		USBD_SET_CEP_STATE(USBD_CEPCTL_NAKCLR);
	} else {
		USBD->EP[ep_idx - 1].USBD_EPRSPCTL = BIT(NCT_USBD_EPRSPCTL_TOGGLE) | BIT(1);
	}

	LOG_DBG("ep 0x%x", ep);
	return 0;
}

/* Check if the selected endpoint is stalled */
int usb_dc_ep_is_stalled(const uint8_t ep, uint8_t *const stalled)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	uint32_t type = dev_data.ep_data[ep_idx].type;

	if (ep_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("wrong endpoint index/address");
		return -EINVAL;
	}

	if (!stalled) {
		return -EINVAL;
	}

	if (type == USB_DC_EP_CONTROL) {
		*stalled = USBD->USBD_CEPCTL & BIT(NCT_USBD_CEPCTL_STALLEN);
	} else {
		*stalled = USBD->EP[ep_idx - 1].USBD_EPRSPCTL & BIT(NCT_USBD_EPRSPCTL_HALT);
	}
	LOG_DBG("ep 0x%x", ep);
	return 0;
}

/* Halt the selected endpoint */
int usb_dc_ep_halt(const uint8_t ep)
{
	return usb_dc_ep_set_stall(ep);
}

/* Enable the selected endpoint */
int usb_dc_ep_enable(const uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	uint32_t type = dev_data.ep_data[ep_idx].type;

	if (ep_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("wrong endpoint index/address");
		return -EINVAL;
	}

	/* Enable endpoint and interrupts */
	if (type == USB_DC_EP_CONTROL) {
		/* Enable CEP global interrupt */
		USBD->USBD_GINTEN |= BIT(NCT_USBD_GINTEN_CEPIEN);
		/* Enable CEP Interrupt */
		USBD->USBD_CEPINTEN = BIT(NCT_USBD_CEPINTEN_SETUPPKIEN);
	} else {
		/* Enable USB EP global interrupt */
		USBD->USBD_GINTEN |= (BIT(ep_idx) << 1);
		USBD->EP[ep_idx - 1].USBD_EPCFG |= BIT(NCT_USBD_EPCFG_EPEN);

		if (USB_EP_GET_DIR(ep) == USB_EP_DIR_OUT) {
			USBD->EP[ep_idx - 1].USBD_EPINTEN = BIT(NCT_USBD_EPINTEN_RXPKIEN) |
							    BIT(NCT_USBD_EPINTEN_SHORTRXIEN);
		}
	}

	LOG_INF("Enable ep 0x%x", ep);
	return 0;
}

/* Disable the selected endpoint */
int usb_dc_ep_disable(const uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	uint32_t type = dev_data.ep_data[ep_idx].type;

	if (ep_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("wrong endpoint index/address");
		return -EINVAL;
	}

	if (type == USB_DC_EP_CONTROL) {
		/* Disable CEP Interrupt */
		USBD->USBD_CEPINTSTS = 0x1FFF;
		USBD->USBD_CEPINTEN = 0;
	} else {
		USBD->EP[ep_idx - 1].USBD_EPINTSTS = 0x1FFF;
		USBD->EP[ep_idx - 1].USBD_EPINTEN = 0;
	}

	LOG_INF("Disable ep 0x%x", ep);
	return 0;
}

/* Flush the selected endpoint */
int usb_dc_ep_flush(const uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	uint32_t type = dev_data.ep_data[ep_idx].type;

	if (ep_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("wrong endpoint index/address");
		return -EINVAL;
	}

	if (type == USB_DC_EP_CONTROL) {
		USBD->USBD_CEPCTL |= BIT(NCT_USBD_CEPCTL_FLUSH);
	} else {
		USBD->EP[ep_idx - 1].USBD_EPRSPCTL |= BIT(NCT_USBD_EPRSPCTL_FLUSH) | BIT(1);
	}

	LOG_DBG("flush ep 0x%x", ep);
	return 0;
}

/* Write data to the specified endpoint */
int usb_dc_ep_write(const uint8_t ep, const uint8_t *const data,
		    const uint32_t data_len, uint32_t *const ret_bytes)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	uint32_t packet_len = 0;
	uint32_t type = dev_data.ep_data[ep_idx].type;
	uint32_t i;
	uint64_t st;


	if (ep_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("wrong endpoint index/address");
		return -EINVAL;
	}

	if (type != USB_DC_EP_CONTROL) {
		if (usbd_ep_is_enabled(ep_idx) == 0) {
			LOG_ERR("endpoint not enabled");
			return -ENODEV;
		}
	}

	if (USB_EP_GET_DIR(ep) != USB_EP_DIR_IN) {
		LOG_ERR("wrong endpoint direction");
		return -EINVAL;
	}

	/* Write the data to the FIFO */
	packet_len = MIN(data_len, dev_data.ep_data[ep_idx].mps);
	if (type == USB_DC_EP_CONTROL) {
		if (packet_len > 0) {
			/* Fill data */
			for (i = 0; i < packet_len; i++) {
				M8(&USBD->USBD_CEPDAT) = data[i];
			}

			/* Clear CEP data transmitted interrupt flag */
			USBD->USBD_CEPINTSTS = BIT(NCT_USBD_CEPINTSTS_TXPKIF);
			/* Start CEP IN Transfer */
			USBD->USBD_CEPTXCNT = packet_len;
			/* Wait for data is transmitted */
			st = k_uptime_get();
			while (1) {
				if ((k_uptime_get() - st) > NCT_USB_WRITE_TIMEOUT) {
					LOG_ERR("Timeout: USB Write Data!");
					return -ETIMEDOUT;
				}

				if ((USBD->USBD_CEPINTSTS & BIT(NCT_USBD_CEPINTSTS_TXPKIF))
				    != 0) {
					/* Clear CEP data transmitted interrupt flag */
					USBD->USBD_CEPINTSTS = BIT(NCT_USBD_CEPINTSTS_TXPKIF);
					break;
				}
			}

			/* Has more data to send */
			if (data_len > packet_len) {
				/* Clear CEP interrupt flag */
				USBD->USBD_CEPINTSTS = BIT(NCT_USBD_CEPINTSTS_INTKIF);
				/* Enable CEP IN Interrupt */
				USBD->USBD_CEPINTEN = BIT(NCT_USBD_CEPINTEN_INTKIEN);
			} else {
				/* Status Stage */
				USBD->USBD_CEPINTSTS = BIT(NCT_USBD_CEPINTSTS_SETUPPKIF);
				USBD->USBD_CEPINTSTS = BIT(NCT_USBD_CEPINTSTS_STSDONEIF);
				USBD_SET_CEP_STATE(USBD_CEPCTL_NAKCLR);
				USBD->USBD_CEPINTEN = BIT(NCT_USBD_CEPINTEN_STSDONEIEN);
			}
		} else {
			/* data_len=0 */
			if ((data == NULL) && ret_bytes == NULL) {
				/* Status Stage */
				USBD->USBD_CEPINTSTS = BIT(NCT_USBD_CEPINTSTS_SETUPPKIF);
				USBD->USBD_CEPINTSTS = BIT(NCT_USBD_CEPINTSTS_STSDONEIF);
				USBD_SET_CEP_STATE(USBD_CEPCTL_ZEROLEN|USBD_CEPCTL_NAKCLR);
				USBD->USBD_CEPINTEN = BIT(NCT_USBD_CEPINTEN_STSDONEIEN);
				LOG_DBG("Zero Len");
			}
		}
	} else {
		/* Fill data */
		for (i = 0; i < packet_len; i++) {
			M8(&USBD->EP[ep_idx - 1].USBD_EPDAT_BYTE) = data[i];
		}

		/* packet end */
		USBD->EP[ep_idx - 1].USBD_EPRSPCTL = BIT(NCT_USBD_EPRSPCTL_SHORTTXEN) | BIT(1);

		/* Clear EP data transmitted interrupt flag */
		USBD->EP[ep_idx - 1].USBD_EPTXCNT = packet_len;
		/* Wait for data is transmitted */
		st = k_uptime_get();
		while (1) {
			if ((k_uptime_get() - st) > NCT_USB_WRITE_TIMEOUT) {
				LOG_ERR("Timeout: USB Write Data!");
				return -ETIMEDOUT;
			}

			if (USBD->EP[ep_idx - 1].USBD_EPDATCNT == 0) {
				break;
			}
		}

		/* Enable In packet interrupt */
		USBD->EP[ep_idx - 1].USBD_EPINTEN = BIT(NCT_USBD_EPINTEN_INTKIEN);
	}

	if (ret_bytes) {
		*ret_bytes = packet_len;
	}

	LOG_DBG("ep 0x%x write %d bytes from %d", ep, packet_len, data_len);
	return 0;
}

/* Read data from the specified endpoint */
int usb_dc_ep_read(const uint8_t ep, uint8_t *const data,
		   const uint32_t max_data_len, uint32_t *const read_bytes)
{
	int ret;

	ret = usb_dc_ep_read_wait(ep, data, max_data_len, read_bytes);
	if (ret < 0) {
		return ret;
	}

	if (!data && !max_data_len) {
		/* When both buffer and max data to read are zero the above
		 * call would fetch the data len and we simply return.
		 */
		return 0;
	}

	/* Clear NAK */
	ret = usb_dc_ep_read_continue(ep);
	if (ret < 0) {
		return -EINVAL;
	}

	LOG_DBG("ep 0x%x", ep);
	return 0;
}

/* Set callback function for the specified endpoint */
int usb_dc_ep_set_callback(const uint8_t ep, const usb_dc_ep_callback cb)
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
	uint32_t data_len = 0;
	uint32_t type = dev_data.ep_data[ep_idx].type;
	uint32_t ep_status = dev_data.ep_status;
	uint32_t i;

	if (ep_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("wrong endpoint index/address");
		return -EINVAL;
	}

	if (type != USB_DC_EP_CONTROL) {
		if (usbd_ep_is_enabled(ep_idx) == 0) {
			LOG_ERR("endpoint not enabled");
			return -ENODEV;
		}
	}

	if (USB_EP_GET_DIR(ep) != USB_EP_DIR_OUT) {
		LOG_ERR("wrong endpoint direction");
		return -EINVAL;
	}

	if (type == USB_DC_EP_CONTROL) {
		if (ep_status == USB_DC_EP_SETUP) {
			data_len = 8;
		} else if (ep_status == USB_DC_EP_DATA_OUT) {
			/* Read data length */
			data_len = USBD->USBD_CEPDATCNT & 0xFFFF;
		}
	} else {
		/* Read data length */
		data_len = USBD->EP[ep_idx - 1].USBD_EPDATCNT & 0xFFFF;
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

	if (data !=  NULL) {
		if (type == USB_DC_EP_CONTROL) {
			if (ep_status == USB_DC_EP_SETUP) {
				/* CEP Setup packet */
				dev_data.req_type = (uint8_t)(USBD->USBD_SETUP1_0 & 0xfful);
				*data++ = dev_data.req_type;
				*data++ = (uint8_t)((USBD->USBD_SETUP1_0 >> 8) & 0xfful);
				*data++ = (uint8_t)(USBD->USBD_SETUP3_2 & 0xfful);
				*data++ = (uint8_t)((USBD->USBD_SETUP3_2 >> 8) & 0xfful);
				*data++ = (uint8_t)(USBD->USBD_SETUP5_4 & 0xfful);
				*data++ = (uint8_t)((USBD->USBD_SETUP5_4 >> 8) & 0xfful);
				dev_data.req_len = USBD->USBD_SETUP7_6 & 0xfffful;
				*data++ = (uint8_t)(dev_data.req_len & 0xfful);
				*data++ = (uint8_t)((dev_data.req_len >> 8) & 0xfful);
			} else {
				/* CEP OUT */
				for (i = 0; i < data_len; i++) {
					*data++ = USBD->USBD_CEPDAT_BYTE;
				}
				if (dev_data.req_len >= data_len) {
					dev_data.req_len -= data_len;
				} else {
					LOG_WRN("Too more data in buffer!!");
					dev_data.req_len = 0;
				}
			}
		} else {
			/* Bulk, Interrupt */
			for (i = 0; i < data_len; i++) {
				*data++ = USBD->EP[ep_idx - 1].USBD_EPDAT_BYTE;
			}

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
	uint32_t type = dev_data.ep_data[ep_idx].type;
	uint32_t ep_status = dev_data.ep_status;

	if (ep_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("wrong endpoint index/address");
		return -EINVAL;
	}

	if (type != USB_DC_EP_CONTROL) {
		if (usbd_ep_is_enabled(ep_idx) == 0) {
			LOG_ERR("endpoint not enabled");
			return -ENODEV;
		}
	}

	if (USB_EP_GET_DIR(ep) != USB_EP_DIR_OUT) {
		LOG_ERR("wrong endpoint direction");
		return -EINVAL;
	}

	if (type == USB_DC_EP_CONTROL) {
		if (ep_status == USB_DC_EP_SETUP) {
			if (REQTYPE_GET_DIR(dev_data.req_type) == REQTYPE_DIR_TO_HOST) {
				USBD->USBD_CEPINTEN = 0;
			} else {
				/* Check CEP has data or not */
				if (dev_data.req_len == 0) {
					/* Status Stage */
					USBD->USBD_CEPINTSTS = BIT(NCT_USBD_CEPINTSTS_SETUPPKIF);
					USBD->USBD_CEPINTSTS = BIT(NCT_USBD_CEPINTSTS_STSDONEIF);
					USBD_SET_CEP_STATE(USBD_CEPCTL_NAKCLR);
					USBD->USBD_CEPINTEN = BIT(NCT_USBD_CEPINTEN_STSDONEIEN);
				} else {
					/* Enable CEP OUT Interrupt and wait Receive CEP OUT data */
					USBD->USBD_CEPINTEN = BIT(NCT_USBD_CEPINTEN_RXPKIEN);
				}
			}
		} else {
			if (dev_data.req_len == 0) {
				/* Status Stage */
				USBD->USBD_CEPINTSTS = BIT(NCT_USBD_CEPINTSTS_SETUPPKIF);
				USBD->USBD_CEPINTSTS = BIT(NCT_USBD_CEPINTSTS_STSDONEIF);
				USBD_SET_CEP_STATE(USBD_CEPCTL_NAKCLR);
				USBD->USBD_CEPINTEN = BIT(NCT_USBD_CEPINTEN_STSDONEIEN);
			}
		}
	} else {
		/* Enable Interrupt to ack OUT */
		USBD->EP[ep_idx - 1].USBD_EPINTEN = BIT(NCT_USBD_EPINTEN_RXPKIEN) |
						    BIT(NCT_USBD_EPINTEN_SHORTRXIEN);
	}

	LOG_DBG("ep 0x%x continue", ep);
	return 0;
}

/* Endpoint max packet size (mps) */
int usb_dc_ep_mps(uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	LOG_DBG("ep 0x%x, ep_idx 0x%x", ep, ep_idx);
	return dev_data.ep_data[ep_idx].mps;
}

#define USB_DC_NCT_INIT(inst)									   \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
                                                                                                   \
	static const struct nct_usbd_config nct_usbd_config_##inst = {			   \
		.base = (struct usbd_reg *)DT_INST_REG_ADDR(inst),                                 \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                    \
		.num_bidir_endpoints = DT_INST_PROP(inst, num_bidir_endpoints),                    \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, NULL,						   \
			      &nct_usbd_config_##inst, POST_KERNEL,				   \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

USB_DC_NCT_INIT(0);

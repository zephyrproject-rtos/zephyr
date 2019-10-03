/* usb_dc_kinetis.c - Kinetis USBFSOTG usb device driver */

/*
 * Copyright (c) 2017 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <string.h>
#include <stdio.h>
#include <kernel.h>
#include <sys/byteorder.h>
#include <usb/usb_device.h>
#include <device.h>

#define LOG_LEVEL CONFIG_USB_DRIVER_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(usb_dc_kinetis);

#define NUM_OF_EP_MAX		DT_USBD_KINETIS_NUM_BIDIR_EP

#define BD_OWN_MASK		(1 << 5)
#define BD_DATA01_MASK		(1 << 4)
#define BD_KEEP_MASK		(1 << 3)
#define BD_NINC_MASK		(1 << 2)
#define BD_DTS_MASK		(1 << 1)
#define BD_STALL_MASK		(1 << 0)

#define KINETIS_SETUP_TOKEN	0x0d
#define KINETIS_IN_TOKEN	0x09
#define KINETIS_OUT_TOKEN	0x01

#define USBFSOTG_PERID		0x04
#define USBFSOTG_REV		0x33

#define KINETIS_EP_NUMOF_MASK	0xf
#define KINETIS_ADDR2IDX(addr)	((addr) & (KINETIS_EP_NUMOF_MASK))

#define EP_ADDR2IDX(ep)		((ep) & ~USB_EP_DIR_MASK)
#define EP_ADDR2DIR(ep)		((ep) & USB_EP_DIR_MASK)

/*
 * Buffer Descriptor (BD) entry provides endpoint buffer control
 * information for USBFS controller. Every endpoint direction requires
 * two BD entries.
 */
struct buf_descriptor {
	union {
		u32_t bd_fields;

		struct {
			u32_t reserved_1_0 : 2;
			u32_t tok_pid : 4;
			u32_t data01 : 1;
			u32_t own : 1;
			u32_t reserved_15_8 : 8;
			u32_t bc : 16;
		} get __packed;

		struct {
			u32_t reserved_1_0 : 2;
			u32_t bd_ctrl : 6;
			u32_t reserved_15_8 : 8;
			u32_t bc : 16;
		} set __packed;

	} __packed;
	u32_t   buf_addr;
} __packed;

/*
 * Buffer Descriptor Table for the endpoints buffer management.
 * The driver configuration with 16 fully bidirectional endpoints would require
 * four BD entries per endpoint and 512 bytes of memory.
 */
static struct buf_descriptor __aligned(512) bdt[(NUM_OF_EP_MAX) * 2 * 2];

#define BD_IDX_EP0TX_EVEN		2
#define BD_IDX_EP0TX_ODD		3

#define EP_BUF_NUMOF_BLOCKS		(NUM_OF_EP_MAX / 2)

K_MEM_POOL_DEFINE(ep_buf_pool, 16, 512, EP_BUF_NUMOF_BLOCKS, 4);

struct usb_ep_ctrl_data {
	struct ep_status {
		u16_t in_enabled : 1;
		u16_t out_enabled : 1;
		u16_t in_data1 : 1;
		u16_t out_data1 : 1;
		u16_t in_odd : 1;
		u16_t out_odd : 1;
		u16_t in_stalled : 1;
		u16_t out_stalled : 1;
	} status;
	u16_t mps_in;
	u16_t mps_out;
	struct k_mem_block mblock_in;
	struct k_mem_block mblock_out;
	usb_dc_ep_callback cb_in;
	usb_dc_ep_callback cb_out;
};

#define USBD_THREAD_STACK_SIZE		1024

struct usb_device_data {
	usb_dc_status_callback status_cb;
	u8_t address;
	u32_t bd_active;
	struct usb_ep_ctrl_data ep_ctrl[NUM_OF_EP_MAX];
	bool attached;

	K_THREAD_STACK_MEMBER(thread_stack, USBD_THREAD_STACK_SIZE);
	struct k_thread thread;
};

static struct usb_device_data dev_data;

#define USB_DC_CB_TYPE_MGMT		0
#define USB_DC_CB_TYPE_EP		1

struct cb_msg {
	u8_t ep;
	u8_t type;
	u32_t cb;
};

K_MSGQ_DEFINE(usb_dc_msgq, sizeof(struct cb_msg), 10, 4);
static void usb_kinetis_isr_handler(void);
static void usb_kinetis_thread_main(void *arg1, void *unused1, void *unused2);

/*
 * This function returns the BD element index based on
 * endpoint address and the odd bit.
 */
static inline u8_t get_bdt_idx(u8_t ep, u8_t odd)
{
	if (ep & USB_EP_DIR_IN) {
		return ((((KINETIS_ADDR2IDX(ep)) * 4) + 2  + (odd & 1)));
	}
	return ((((KINETIS_ADDR2IDX(ep)) * 4) + (odd & 1)));
}

static int kinetis_usb_init(void)
{
	/* enable USB voltage regulator */
	SIM->SOPT1 |= SIM_SOPT1_USBREGEN_MASK;

	USB0->USBTRC0 |= USB_USBTRC0_USBRESET_MASK;
	k_busy_wait(2000);

	USB0->CTL = 0;
	/* enable USB module, AKA USBEN bit in CTL1 register */
	USB0->CTL |= USB_CTL_USBENSOFEN_MASK;

	if ((USB0->PERID != USBFSOTG_PERID) ||
	    (USB0->REV != USBFSOTG_REV)) {
		return -1;
	}

	USB0->BDTPAGE1 = (u8_t)(((u32_t)bdt) >> 8);
	USB0->BDTPAGE2 = (u8_t)(((u32_t)bdt) >> 16);
	USB0->BDTPAGE3 = (u8_t)(((u32_t)bdt) >> 24);

	/* clear interrupt flags */
	USB0->ISTAT = 0xFF;

	/* enable reset interrupt */
	USB0->INTEN = USB_INTEN_USBRSTEN_MASK;

	USB0->USBCTRL = USB_USBCTRL_PDE_MASK;

	k_thread_create(&dev_data.thread, dev_data.thread_stack,
			USBD_THREAD_STACK_SIZE,
			usb_kinetis_thread_main, NULL, NULL, NULL,
			K_PRIO_COOP(2), 0, K_NO_WAIT);

	/* Connect and enable USB interrupt */
	IRQ_CONNECT(DT_USBD_KINETIS_IRQ, DT_USBD_KINETIS_IRQ_PRI,
		    usb_kinetis_isr_handler, 0, 0);

	irq_enable(DT_USBD_KINETIS_IRQ);

	LOG_DBG("");

	return 0;
}

int usb_dc_reset(void)
{
	for (u8_t i = 0; i < 16; i++) {
		USB0->ENDPOINT[i].ENDPT = 0;
	}
	(void)memset(bdt, 0, sizeof(bdt));
	dev_data.bd_active = 0U;
	dev_data.address = 0U;

	USB0->CTL |= USB_CTL_ODDRST_MASK;
	USB0->CTL &= ~USB_CTL_ODDRST_MASK;

	/* Clear interrupt status flags */
	USB0->ISTAT = 0xFF;
	/* Clear error flags */
	USB0->ERRSTAT = 0xFF;
	/* Enable all error interrupt sources */
	USB0->ERREN = 0xFF;
	/* Reset default address */
	USB0->ADDR = 0x00;

	USB0->INTEN = (USB_INTEN_USBRSTEN_MASK |
		       USB_INTEN_TOKDNEEN_MASK |
		       USB_INTEN_SLEEPEN_MASK  |
		       USB_INTEN_SOFTOKEN_MASK |
		       USB_INTEN_STALLEN_MASK |
		       USB_INTEN_ERROREN_MASK);

	LOG_DBG("");

	return 0;
}

int usb_dc_attach(void)
{
	if (dev_data.attached) {
		LOG_WRN("already attached");
	}

	kinetis_usb_init();

	/*
	 * Call usb_dc_reset here because the device stack does not make it
	 * after USB_DC_RESET status event.
	 */
	usb_dc_reset();

	dev_data.attached = 1;
	LOG_DBG("attached");

	/* non-OTG device mode, enable DP Pullup */
	USB0->CONTROL = USB_CONTROL_DPPULLUPNONOTG_MASK;

	return 0;
}

int usb_dc_detach(void)
{
	LOG_DBG("");
	/* disable USB and DP Pullup */
	USB0->CTL  &= ~USB_CTL_USBENSOFEN_MASK;
	USB0->CONTROL &= ~USB_CONTROL_DPPULLUPNONOTG_MASK;

	return 0;
}

int usb_dc_set_address(const u8_t addr)
{
	LOG_DBG("");

	if (!dev_data.attached) {
		return -EINVAL;
	}

	/*
	 * The device stack tries to set the address before
	 * sending the ACK with ZLP, which is totally stupid,
	 * as workaround the addresse will be buffered and
	 * placed later inside isr handler (see KINETIS_IN_TOKEN).
	 */
	dev_data.address = 0x80 | (addr & 0x7f);

	return 0;
}

int usb_dc_ep_check_cap(const struct usb_dc_ep_cfg_data * const cfg)
{
	u8_t ep_idx = EP_ADDR2IDX(cfg->ep_addr);

	if (ep_idx > (NUM_OF_EP_MAX - 1)) {
		LOG_ERR("endpoint index/address out of range");
		return -EINVAL;
	}

	switch (cfg->ep_type) {
	case USB_DC_EP_CONTROL:
		if (cfg->ep_mps > USB_MAX_CTRL_MPS) {
			return -EINVAL;
		}
		return 0;
	case USB_DC_EP_BULK:
		if (cfg->ep_mps > USB_MAX_FS_BULK_MPS) {
			return -EINVAL;
		}
		break;
	case USB_DC_EP_INTERRUPT:
		if (cfg->ep_mps > USB_MAX_FS_INT_MPS) {
			return -EINVAL;
		}
		break;
	case USB_DC_EP_ISOCHRONOUS:
		if (cfg->ep_mps > USB_MAX_FS_ISO_MPS) {
			return -EINVAL;
		}
		break;
	default:
		LOG_ERR("Unknown endpoint type!");
		return -EINVAL;
	}

	if (ep_idx & BIT(0)) {
		if (EP_ADDR2DIR(cfg->ep_addr) != USB_EP_DIR_IN) {
			LOG_INF("pre-selected as IN endpoint");
			return -1;
		}
	} else {
		if (EP_ADDR2DIR(cfg->ep_addr) != USB_EP_DIR_OUT) {
			LOG_INF("pre-selected as OUT endpoint");
			return -1;
		}
	}

	return 0;
}

int usb_dc_ep_configure(const struct usb_dc_ep_cfg_data * const cfg)
{
	u8_t ep_idx = EP_ADDR2IDX(cfg->ep_addr);
	struct usb_ep_ctrl_data *ep_ctrl;
	struct k_mem_block *block;
	u8_t idx_even;
	u8_t idx_odd;

	if (usb_dc_ep_check_cap(cfg)) {
		return -EINVAL;
	}

	idx_even = get_bdt_idx(cfg->ep_addr, 0);
	idx_odd = get_bdt_idx(cfg->ep_addr, 1);
	ep_ctrl = &dev_data.ep_ctrl[ep_idx];

	if (ep_idx && (dev_data.ep_ctrl[ep_idx].status.in_enabled ||
	    dev_data.ep_ctrl[ep_idx].status.out_enabled)) {
		LOG_WRN("endpoint already configured");
		return -EBUSY;
	}

	LOG_DBG("ep %x, mps %d, type %d", cfg->ep_addr, cfg->ep_mps,
		cfg->ep_type);

	if (EP_ADDR2DIR(cfg->ep_addr) == USB_EP_DIR_OUT) {
		block = &(ep_ctrl->mblock_out);
	} else {
		block = &(ep_ctrl->mblock_in);
	}

	if (bdt[idx_even].buf_addr) {
		k_mem_pool_free(block);
	}

	USB0->ENDPOINT[ep_idx].ENDPT = 0;
	(void)memset(&bdt[idx_even], 0, sizeof(struct buf_descriptor));
	(void)memset(&bdt[idx_odd], 0, sizeof(struct buf_descriptor));

	if (k_mem_pool_alloc(&ep_buf_pool, block, cfg->ep_mps * 2U, K_MSEC(10)) == 0) {
		(void)memset(block->data, 0, cfg->ep_mps * 2U);
	} else {
		LOG_ERR("Memory allocation time-out");
		return -ENOMEM;
	}

	bdt[idx_even].buf_addr = (u32_t)block->data;
	LOG_INF("idx_even %x", (u32_t)block->data);
	bdt[idx_odd].buf_addr = (u32_t)((u8_t *)block->data + cfg->ep_mps);
	LOG_INF("idx_odd %x", (u32_t)((u8_t *)block->data + cfg->ep_mps));

	if (cfg->ep_addr & USB_EP_DIR_IN) {
		dev_data.ep_ctrl[ep_idx].mps_in = cfg->ep_mps;
	} else {
		dev_data.ep_ctrl[ep_idx].mps_out = cfg->ep_mps;
	}

	bdt[idx_even].set.bc = cfg->ep_mps;
	bdt[idx_odd].set.bc = cfg->ep_mps;

	dev_data.ep_ctrl[ep_idx].status.out_data1 = false;
	dev_data.ep_ctrl[ep_idx].status.in_data1 = false;

	switch (cfg->ep_type) {
	case USB_DC_EP_CONTROL:
		LOG_DBG("configure control endpoint");
		USB0->ENDPOINT[ep_idx].ENDPT |= (USB_ENDPT_EPHSHK_MASK |
						 USB_ENDPT_EPRXEN_MASK |
						 USB_ENDPT_EPTXEN_MASK);
		break;
	case USB_DC_EP_BULK:
	case USB_DC_EP_INTERRUPT:
		USB0->ENDPOINT[ep_idx].ENDPT |= USB_ENDPT_EPHSHK_MASK;
		if (EP_ADDR2DIR(cfg->ep_addr) == USB_EP_DIR_OUT) {
			USB0->ENDPOINT[ep_idx].ENDPT |= USB_ENDPT_EPRXEN_MASK;
		} else {
			USB0->ENDPOINT[ep_idx].ENDPT |= USB_ENDPT_EPTXEN_MASK;
		}
		break;
	case USB_DC_EP_ISOCHRONOUS:
		if (EP_ADDR2DIR(cfg->ep_addr) == USB_EP_DIR_OUT) {
			USB0->ENDPOINT[ep_idx].ENDPT |= USB_ENDPT_EPRXEN_MASK;
		} else {
			USB0->ENDPOINT[ep_idx].ENDPT |= USB_ENDPT_EPTXEN_MASK;
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

int usb_dc_ep_set_stall(const u8_t ep)
{
	u8_t ep_idx = EP_ADDR2IDX(ep);
	u8_t bd_idx;

	if (ep_idx > (NUM_OF_EP_MAX - 1)) {
		LOG_ERR("Wrong endpoint index/address");
		return -EINVAL;
	}

	LOG_DBG("ep %x, idx %d", ep, ep_idx);

	if (EP_ADDR2DIR(ep) == USB_EP_DIR_OUT) {
		dev_data.ep_ctrl[ep_idx].status.out_stalled = 1U;
		bd_idx = get_bdt_idx(ep,
				     ~dev_data.ep_ctrl[ep_idx].status.out_odd);
	} else {
		dev_data.ep_ctrl[ep_idx].status.in_stalled = 1U;
		bd_idx = get_bdt_idx(ep,
				     dev_data.ep_ctrl[ep_idx].status.in_odd);
	}

	bdt[bd_idx].set.bd_ctrl = BD_STALL_MASK | BD_DTS_MASK | BD_OWN_MASK;

	return 0;
}

int usb_dc_ep_clear_stall(const u8_t ep)
{
	u8_t ep_idx = EP_ADDR2IDX(ep);
	u8_t bd_idx;

	if (ep_idx > (NUM_OF_EP_MAX - 1)) {
		LOG_ERR("Wrong endpoint index/address");
		return -EINVAL;
	}

	LOG_DBG("ep %x, idx %d", ep, ep_idx);
	USB0->ENDPOINT[ep_idx].ENDPT &= ~USB_ENDPT_EPSTALL_MASK;

	if (EP_ADDR2DIR(ep) == USB_EP_DIR_OUT) {
		dev_data.ep_ctrl[ep_idx].status.out_stalled = 0U;
		dev_data.ep_ctrl[ep_idx].status.out_data1 = false;
		bd_idx = get_bdt_idx(ep,
				     ~dev_data.ep_ctrl[ep_idx].status.out_odd);
		bdt[bd_idx].set.bd_ctrl = 0U;
		bdt[bd_idx].set.bd_ctrl = BD_DTS_MASK | BD_OWN_MASK;
	} else {
		dev_data.ep_ctrl[ep_idx].status.in_stalled = 0U;
		dev_data.ep_ctrl[ep_idx].status.in_data1 = false;
		bd_idx = get_bdt_idx(ep,
				     dev_data.ep_ctrl[ep_idx].status.in_odd);
		bdt[bd_idx].set.bd_ctrl = 0U;
	}

	/* Resume TX token processing, see USBx_CTL field descriptions */
	if (ep == 0U) {
		USB0->CTL &= ~USB_CTL_TXSUSPENDTOKENBUSY_MASK;
	}

	return 0;
}

int usb_dc_ep_is_stalled(const u8_t ep, u8_t *const stalled)
{
	u8_t ep_idx = EP_ADDR2IDX(ep);

	if (ep_idx > (NUM_OF_EP_MAX - 1)) {
		LOG_ERR("Wrong endpoint index/address");
		return -EINVAL;
	}

	LOG_DBG("ep %x, idx %d", ep_idx, ep);
	if (!stalled) {
		return -EINVAL;
	}

	*stalled = 0U;
	if (EP_ADDR2DIR(ep) == USB_EP_DIR_OUT) {
		*stalled = dev_data.ep_ctrl[ep_idx].status.out_stalled;
	} else {
		*stalled = dev_data.ep_ctrl[ep_idx].status.in_stalled;
	}

	u8_t bd_idx = get_bdt_idx(ep,
			dev_data.ep_ctrl[ep_idx].status.in_odd);
	LOG_WRN("active bd ctrl: %x", bdt[bd_idx].set.bd_ctrl);
	bd_idx = get_bdt_idx(ep,
			~dev_data.ep_ctrl[ep_idx].status.in_odd);
	LOG_WRN("next bd ctrl: %x", bdt[bd_idx].set.bd_ctrl);

	return 0;
}

int usb_dc_ep_halt(const u8_t ep)
{
	return usb_dc_ep_set_stall(ep);
}

int usb_dc_ep_enable(const u8_t ep)
{
	u8_t ep_idx = EP_ADDR2IDX(ep);
	u8_t idx_even;
	u8_t idx_odd;

	if (ep_idx > (NUM_OF_EP_MAX - 1)) {
		LOG_ERR("Wrong endpoint index/address");
		return -EINVAL;
	}

	idx_even = get_bdt_idx(ep, 0);
	idx_odd = get_bdt_idx(ep, 1);

	if (ep_idx && (dev_data.ep_ctrl[ep_idx].status.in_enabled ||
	    dev_data.ep_ctrl[ep_idx].status.out_enabled)) {
		LOG_WRN("endpoint 0x%x already enabled", ep);
		return -EBUSY;
	}

	if (EP_ADDR2DIR(ep) == USB_EP_DIR_OUT) {
		bdt[idx_even].set.bd_ctrl = BD_DTS_MASK | BD_OWN_MASK;
		bdt[idx_odd].set.bd_ctrl = 0U;
		dev_data.ep_ctrl[ep_idx].status.out_odd = 0U;
		dev_data.ep_ctrl[ep_idx].status.out_stalled = 0U;
		dev_data.ep_ctrl[ep_idx].status.out_data1 = false;
		dev_data.ep_ctrl[ep_idx].status.out_enabled = true;
	} else {
		bdt[idx_even].bd_fields = 0U;
		bdt[idx_odd].bd_fields = 0U;
		dev_data.ep_ctrl[ep_idx].status.in_odd = 0U;
		dev_data.ep_ctrl[ep_idx].status.in_stalled = 0U;
		dev_data.ep_ctrl[ep_idx].status.in_data1 = false;
		dev_data.ep_ctrl[ep_idx].status.in_enabled = true;
	}

	LOG_INF("ep 0x%x, ep_idx %d", ep, ep_idx);

	return 0;
}

int usb_dc_ep_disable(const u8_t ep)
{
	u8_t ep_idx = EP_ADDR2IDX(ep);
	u8_t idx_even;
	u8_t idx_odd;

	if (ep_idx > (NUM_OF_EP_MAX - 1)) {
		LOG_ERR("Wrong endpoint index/address");
		return -EINVAL;
	}

	idx_even = get_bdt_idx(ep, 0);
	idx_odd = get_bdt_idx(ep, 1);

	LOG_INF("ep %x, idx %d", ep_idx, ep);

	bdt[idx_even].bd_fields = 0U;
	bdt[idx_odd].bd_fields = 0U;
	if (EP_ADDR2DIR(ep) == USB_EP_DIR_OUT) {
		dev_data.ep_ctrl[ep_idx].status.out_enabled = false;
	} else {
		dev_data.ep_ctrl[ep_idx].status.in_enabled = false;
	}

	return 0;
}

int usb_dc_ep_flush(const u8_t ep)
{
	u8_t ep_idx = EP_ADDR2IDX(ep);

	if (ep_idx > (NUM_OF_EP_MAX - 1)) {
		LOG_ERR("Wrong endpoint index/address");
		return -EINVAL;
	}

	LOG_DBG("ep %x, idx %d", ep_idx, ep);

	return 0;
}

int usb_dc_ep_write(const u8_t ep, const u8_t *const data,
		    const u32_t data_len, u32_t * const ret_bytes)
{
	u8_t ep_idx = EP_ADDR2IDX(ep);
	u32_t len_to_send = data_len;
	u8_t odd;
	u8_t bd_idx;
	u8_t *bufp;

	if (ep_idx > (NUM_OF_EP_MAX - 1)) {
		LOG_ERR("Wrong endpoint index/address");
		return -EINVAL;
	}

	odd = dev_data.ep_ctrl[ep_idx].status.in_odd;
	bd_idx = get_bdt_idx(ep, odd);
	bufp = (u8_t *)bdt[bd_idx].buf_addr;

	if (EP_ADDR2DIR(ep) != USB_EP_DIR_IN) {
		LOG_ERR("Wrong endpoint direction");
		return -EINVAL;
	}

	if (dev_data.ep_ctrl[ep_idx].status.in_stalled) {
		LOG_WRN("endpoint is stalled");
		return -EBUSY;
	}

	while (bdt[bd_idx].get.own) {
		LOG_DBG("ep 0x%x is busy", ep);
		k_yield();
	}

	LOG_DBG("bd idx %x bufp %p odd %d", bd_idx, bufp, odd);

	if (data_len > dev_data.ep_ctrl[ep_idx].mps_in) {
		len_to_send = dev_data.ep_ctrl[ep_idx].mps_in;
	}

	bdt[bd_idx].set.bc = len_to_send;

	for (u32_t n = 0; n < len_to_send; n++) {
		bufp[n] = data[n];
	}

	dev_data.ep_ctrl[ep_idx].status.in_odd = ~odd;
	if (dev_data.ep_ctrl[ep_idx].status.in_data1) {
		bdt[bd_idx].set.bd_ctrl = BD_DTS_MASK |
					  BD_DATA01_MASK |
					  BD_OWN_MASK;
	} else {
		bdt[bd_idx].set.bd_ctrl = BD_DTS_MASK | BD_OWN_MASK;
	}

	/* Toggle next Data1 */
	dev_data.ep_ctrl[ep_idx].status.in_data1 ^= 1;

	LOG_DBG("ep 0x%x write %d bytes from %d", ep, len_to_send, data_len);

	if (ret_bytes) {
		*ret_bytes = len_to_send;
	}

	return 0;
}

int usb_dc_ep_read_wait(u8_t ep, u8_t *data, u32_t max_data_len,
			u32_t *read_bytes)
{
	u8_t ep_idx = EP_ADDR2IDX(ep);
	u32_t data_len;
	u8_t bd_idx;
	u8_t *bufp;

	if (ep_idx > (NUM_OF_EP_MAX - 1)) {
		LOG_ERR("Wrong endpoint index/address");
		return -EINVAL;
	}

	/* select the index of active endpoint buffer */
	bd_idx = get_bdt_idx(ep, dev_data.ep_ctrl[ep_idx].status.out_odd);
	bufp = (u8_t *)bdt[bd_idx].buf_addr;

	if (EP_ADDR2DIR(ep) != USB_EP_DIR_OUT) {
		LOG_ERR("Wrong endpoint direction");
		return -EINVAL;
	}

	if (dev_data.ep_ctrl[ep_idx].status.out_stalled) {
		LOG_WRN("endpoint is stalled");
		return -EBUSY;
	}

	/* Allow to read 0 bytes */
	if (!data && max_data_len) {
		LOG_ERR("Wrong arguments");
		return -EINVAL;
	}

	while (bdt[bd_idx].get.own) {
		LOG_ERR("Endpoint is occupied by the controller");
		return -EBUSY;
	}

	data_len  = bdt[bd_idx].get.bc;

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
		LOG_WRN("Not enough room to copy all the data!");
		data_len = max_data_len;
	}

	if (data != NULL) {
		for (u32_t i = 0; i < data_len; i++) {
			data[i] = bufp[i];
		}
	}

	LOG_DBG("Read idx %d, req %d, read %d bytes", bd_idx, max_data_len,
		data_len);

	if (read_bytes) {
		*read_bytes = data_len;
	}

	return 0;
}


int usb_dc_ep_read_continue(u8_t ep)
{
	u8_t ep_idx = EP_ADDR2IDX(ep);
	u8_t bd_idx;

	if (ep_idx > (NUM_OF_EP_MAX - 1)) {
		LOG_ERR("Wrong endpoint index/address");
		return -EINVAL;
	}

	bd_idx = get_bdt_idx(ep, dev_data.ep_ctrl[ep_idx].status.out_odd);

	if (EP_ADDR2DIR(ep) != USB_EP_DIR_OUT) {
		LOG_ERR("Wrong endpoint direction");
		return -EINVAL;
	}

	if (bdt[bd_idx].get.own) {
		/* May occur when usb_transfer initializes the OUT transfer */
		LOG_WRN("Current buffer is claimed by the controller");
		return 0;
	}

	/* select the index of the next endpoint buffer */
	bd_idx = get_bdt_idx(ep, ~dev_data.ep_ctrl[ep_idx].status.out_odd);
	/* Update next toggle bit */
	dev_data.ep_ctrl[ep_idx].status.out_data1 ^= 1;
	bdt[bd_idx].set.bc = dev_data.ep_ctrl[ep_idx].mps_out;

	/* Reset next buffer descriptor and set next toggle bit  */
	if (dev_data.ep_ctrl[ep_idx].status.out_data1) {
		bdt[bd_idx].set.bd_ctrl = BD_DTS_MASK |
					  BD_DATA01_MASK |
					  BD_OWN_MASK;
	} else {
		bdt[bd_idx].set.bd_ctrl = BD_DTS_MASK | BD_OWN_MASK;
	}

	/* Resume TX token processing, see USBx_CTL field descriptions */
	if (ep_idx == 0U) {
		USB0->CTL &= ~USB_CTL_TXSUSPENDTOKENBUSY_MASK;
	}

	LOG_DBG("idx next %x", bd_idx);

	return 0;
}

int usb_dc_ep_read(const u8_t ep, u8_t *const data,
		   const u32_t max_data_len, u32_t *const read_bytes)
{
	int retval = usb_dc_ep_read_wait(ep, data, max_data_len, read_bytes);

	if (retval) {
		return retval;
	}

	if (!data && !max_data_len) {
		/* When both buffer and max data to read are zero the above
		 * call would fetch the data len and we simply return.
		 */
		return 0;
	}

	if (usb_dc_ep_read_continue(ep) != 0) {
		return -EINVAL;
	}

	LOG_DBG("");

	return 0;
}

int usb_dc_ep_set_callback(const u8_t ep, const usb_dc_ep_callback cb)
{
	u8_t ep_idx = EP_ADDR2IDX(ep);

	if (ep_idx > (NUM_OF_EP_MAX - 1)) {
		LOG_ERR("Wrong endpoint index/address");
		return -EINVAL;
	}

	if (!dev_data.attached) {
		return -EINVAL;
	}

	if (ep & USB_EP_DIR_IN) {
		dev_data.ep_ctrl[ep_idx].cb_in = cb;
	} else {
		dev_data.ep_ctrl[ep_idx].cb_out = cb;
	}
	LOG_DBG("ep_idx %x", ep_idx);

	return 0;
}

void usb_dc_set_status_callback(const usb_dc_status_callback cb)
{
	LOG_DBG("");

	dev_data.status_cb = cb;
}

int usb_dc_ep_mps(const u8_t ep)
{
	u8_t ep_idx = EP_ADDR2IDX(ep);

	if (ep_idx > (NUM_OF_EP_MAX - 1)) {
		LOG_ERR("Wrong endpoint index/address");
		return -EINVAL;
	}

	if (ep & USB_EP_DIR_IN) {
		return dev_data.ep_ctrl[ep_idx].mps_in;
	} else {
		return dev_data.ep_ctrl[ep_idx].mps_out;
	}
}

static inline void reenable_all_endpoints(void)
{
	for (u8_t ep_idx = 0; ep_idx < NUM_OF_EP_MAX; ep_idx++) {
		if (dev_data.ep_ctrl[ep_idx].status.out_enabled) {
			usb_dc_ep_enable(ep_idx);
		}
		if (dev_data.ep_ctrl[ep_idx].status.in_enabled) {
			usb_dc_ep_enable(ep_idx | USB_EP_DIR_IN);
		}
	}
}

static void usb_kinetis_isr_handler(void)
{
	u8_t istatus  = USB0->ISTAT;
	u8_t status  = USB0->STAT;
	struct cb_msg msg;


	if (istatus & USB_ISTAT_USBRST_MASK) {
		dev_data.address = 0U;
		USB0->ADDR = (u8_t)0;
		/*
		 * Device reset is not possible because the stack does not
		 * configure the endpoints after the USB_DC_RESET event,
		 * therefore, reenable all endpoints and set they BDT into a
		 * defined state.
		 */
		USB0->CTL |= USB_CTL_ODDRST_MASK;
		USB0->CTL &= ~USB_CTL_ODDRST_MASK;
		reenable_all_endpoints();
		msg.ep = 0U;
		msg.type = USB_DC_CB_TYPE_MGMT;
		msg.cb = USB_DC_RESET;
		k_msgq_put(&usb_dc_msgq, &msg, K_NO_WAIT);
	}

	if (istatus == USB_ISTAT_ERROR_MASK) {
		USB0->ERRSTAT = 0xFF;
		msg.ep = 0U;
		msg.type = USB_DC_CB_TYPE_MGMT;
		msg.cb = USB_DC_ERROR;
		k_msgq_put(&usb_dc_msgq, &msg, K_NO_WAIT);
	}

	if (istatus & USB_ISTAT_STALL_MASK) {
		if (dev_data.ep_ctrl[0].status.out_stalled) {
			usb_dc_ep_clear_stall(0);
		}
		if (dev_data.ep_ctrl[0].status.in_stalled) {
			usb_dc_ep_clear_stall(0x80);
		}
	}

	if (istatus & USB_ISTAT_TOKDNE_MASK) {

		u8_t ep_idx = status >> USB_STAT_ENDP_SHIFT;
		u8_t ep = ((status << 4) & USB_EP_DIR_IN) | ep_idx;
		u8_t odd = (status & USB_STAT_ODD_MASK) >> USB_STAT_ODD_SHIFT;
		u8_t idx = get_bdt_idx(ep, odd);
		u8_t token_pid = bdt[idx].get.tok_pid;

		msg.ep = ep;
		msg.type = USB_DC_CB_TYPE_EP;

		switch (token_pid) {
		case KINETIS_SETUP_TOKEN:
			dev_data.ep_ctrl[ep_idx].status.out_odd = odd;
			/* clear tx entries */
			bdt[BD_IDX_EP0TX_EVEN].bd_fields = 0U;
			bdt[BD_IDX_EP0TX_ODD].bd_fields = 0U;
			/*
			 * Set/Reset here the toggle bits for control endpoint
			 * because the device stack does not care about it.
			 */
			dev_data.ep_ctrl[ep_idx].status.in_data1 = true;
			dev_data.ep_ctrl[ep_idx].status.out_data1 = false;
			dev_data.ep_ctrl[ep_idx].status.out_odd = odd;

			msg.cb = USB_DC_EP_SETUP;
			k_msgq_put(&usb_dc_msgq, &msg, K_NO_WAIT);
			break;
		case KINETIS_OUT_TOKEN:
			dev_data.ep_ctrl[ep_idx].status.out_odd = odd;

			msg.cb = USB_DC_EP_DATA_OUT;
			k_msgq_put(&usb_dc_msgq, &msg, K_NO_WAIT);
			break;
		case KINETIS_IN_TOKEN:
			/* SET ADDRESS workaround */
			if (dev_data.address & 0x80) {
				USB0->ADDR = dev_data.address & 0x7f;
				dev_data.address = 0U;
			}

			msg.cb = USB_DC_EP_DATA_IN;
			k_msgq_put(&usb_dc_msgq, &msg, K_NO_WAIT);
			break;
		default:
			break;
		}
	}

	if (istatus & USB_ISTAT_SLEEP_MASK) {
		/* Enable resume interrupt */
		USB0->INTEN |= USB_INTEN_RESUMEEN_MASK;
		msg.ep = 0U;
		msg.type = USB_DC_CB_TYPE_MGMT;
		msg.cb = USB_DC_SUSPEND;
		k_msgq_put(&usb_dc_msgq, &msg, K_NO_WAIT);
	}

	if (istatus & USB_ISTAT_RESUME_MASK) {
		/* Disable resume interrupt */
		USB0->INTEN &= ~USB_INTEN_RESUMEEN_MASK;
		msg.ep = 0U;
		msg.type = USB_DC_CB_TYPE_MGMT;
		msg.cb = USB_DC_RESUME;
		k_msgq_put(&usb_dc_msgq, &msg, K_NO_WAIT);
	}

	/* Clear interrupt status bits */
	USB0->ISTAT = istatus;
}

/*
 * This thread is only used to not run the USB device stack and endpoint
 * callbacks in the ISR context, which happens when an callback function
 * is called. TODO: something similar should be implemented in the USB
 * device stack so that it can be used by all drivers.
 */
static void usb_kinetis_thread_main(void *arg1, void *unused1, void *unused2)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	struct cb_msg msg;
	u8_t ep_idx;

	while (true) {
		k_msgq_get(&usb_dc_msgq, &msg, K_FOREVER);
		ep_idx = EP_ADDR2IDX(msg.ep);

		if (msg.type == USB_DC_CB_TYPE_EP) {
			switch (msg.cb) {
			case USB_DC_EP_SETUP:
				if (dev_data.ep_ctrl[ep_idx].cb_out) {
					dev_data.ep_ctrl[ep_idx].cb_out(msg.ep,
						USB_DC_EP_SETUP);
				}
				break;
			case USB_DC_EP_DATA_OUT:
				if (dev_data.ep_ctrl[ep_idx].cb_out) {
					dev_data.ep_ctrl[ep_idx].cb_out(msg.ep,
						USB_DC_EP_DATA_OUT);
				}
				break;
			case USB_DC_EP_DATA_IN:
				if (dev_data.ep_ctrl[ep_idx].cb_in) {
					dev_data.ep_ctrl[ep_idx].cb_in(msg.ep,
						USB_DC_EP_DATA_IN);
				}
				break;
			default:
				LOG_ERR("unknown msg");
				break;
			}
		} else if (dev_data.status_cb) {
			switch (msg.cb) {
			case USB_DC_RESET:
				dev_data.status_cb(USB_DC_RESET, NULL);
				break;
			case USB_DC_ERROR:
				dev_data.status_cb(USB_DC_ERROR, NULL);
				break;
			case USB_DC_SUSPEND:
				dev_data.status_cb(USB_DC_SUSPEND, NULL);
				break;
			case USB_DC_RESUME:
				dev_data.status_cb(USB_DC_RESUME, NULL);
				break;
			default:
				LOG_ERR("unknown msg");
				break;
			}
		}
	}
}


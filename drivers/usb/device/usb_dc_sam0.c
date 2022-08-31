/*
 * Copyright (c) 2018 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam0_usb

#define LOG_LEVEL CONFIG_USB_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usb_dc_sam0);

#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/pinctrl.h>
#include <soc.h>
#include <string.h>

#define NVM_USB_PAD_TRANSN_POS 45
#define NVM_USB_PAD_TRANSN_SIZE 5
#define NVM_USB_PAD_TRANSP_POS 50
#define NVM_USB_PAD_TRANSP_SIZE 5
#define NVM_USB_PAD_TRIM_POS 55
#define NVM_USB_PAD_TRIM_SIZE 3

#define USB_SAM0_IN_EP 0x80

#define REGS ((Usb *)DT_INST_REG_ADDR(0))
#define USB_NUM_ENDPOINTS DT_INST_PROP(0, num_bidir_endpoints)

/* The endpoint size stored in USB.PCKSIZE.SIZE */
enum usb_sam0_pcksize_size {
	USB_SAM0_PCKSIZE_SIZE_8 = 0,
	USB_SAM0_PCKSIZE_SIZE_16,
	USB_SAM0_PCKSIZE_SIZE_32,
	USB_SAM0_PCKSIZE_SIZE_64,
	USB_SAM0_PCKSIZE_SIZE_128,
	USB_SAM0_PCKSIZE_SIZE_256,
	USB_SAM0_PCKSIZE_SIZE_512,
	USB_SAM0_PCKSIZE_SIZE_1023,
};

static const uint16_t usb_sam0_pcksize_bytes[] = {
	[USB_SAM0_PCKSIZE_SIZE_8] = 8,
	[USB_SAM0_PCKSIZE_SIZE_16] = 16,
	[USB_SAM0_PCKSIZE_SIZE_32] = 32,
	[USB_SAM0_PCKSIZE_SIZE_64] = 64,
	[USB_SAM0_PCKSIZE_SIZE_128] = 128,
	[USB_SAM0_PCKSIZE_SIZE_256] = 256,
	[USB_SAM0_PCKSIZE_SIZE_512] = 512,
	[USB_SAM0_PCKSIZE_SIZE_1023] = 1023,
};

BUILD_ASSERT(ARRAY_SIZE(usb_sam0_pcksize_bytes) == 8);

struct usb_sam0_data {
	UsbDeviceDescriptor descriptors[USB_NUM_ENDPOINTS];

	usb_dc_status_callback cb;
	usb_dc_ep_callback ep_cb[2][USB_NUM_ENDPOINTS];

	uint8_t addr;
	uint32_t out_at;
};

static struct usb_sam0_data usb_sam0_data_0;
PINCTRL_DT_INST_DEFINE(0);
static const struct pinctrl_dev_config *pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0);

static struct usb_sam0_data *usb_sam0_get_data(void)
{
	return &usb_sam0_data_0;
}

/* Handles interrupts on an endpoint */
static void usb_sam0_ep_isr(uint8_t ep)
{
	struct usb_sam0_data *data = usb_sam0_get_data();
	UsbDevice *regs = &REGS->DEVICE;
	UsbDeviceEndpoint *endpoint = &regs->DeviceEndpoint[ep];
	uint32_t intflag = endpoint->EPINTFLAG.reg;

	endpoint->EPINTFLAG.reg = intflag;

	if ((intflag & USB_DEVICE_EPINTFLAG_RXSTP) != 0U) {
		/* Setup */
		data->ep_cb[0][ep](ep, USB_DC_EP_SETUP);
	}

	if ((intflag & USB_DEVICE_EPINTFLAG_TRCPT0) != 0U) {
		/* Out (to device) data received */
		data->ep_cb[0][ep](ep, USB_DC_EP_DATA_OUT);
	}

	if ((intflag & USB_DEVICE_EPINTFLAG_TRCPT1) != 0U) {
		/* In (to host) transmit complete */
		data->ep_cb[1][ep](ep | USB_SAM0_IN_EP, USB_DC_EP_DATA_IN);

		if (data->addr != 0U) {
			/* Commit the pending address update.  This
			 * must be done after the ack to the host
			 * completes else the ack will get dropped.
			 */
			regs->DADD.reg = data->addr;
			data->addr = 0U;
		}
	}
}

/* Top level interrupt handler */
static void usb_sam0_isr(void)
{
	struct usb_sam0_data *data = usb_sam0_get_data();
	UsbDevice *regs = &REGS->DEVICE;
	uint32_t intflag = regs->INTFLAG.reg;
	uint32_t epint = regs->EPINTSMRY.reg;
	uint8_t ep;

	/* Acknowledge all interrupts */
	regs->INTFLAG.reg = intflag;

	if ((intflag & USB_DEVICE_INTFLAG_EORST) != 0U) {
		UsbDeviceEndpoint *endpoint = &regs->DeviceEndpoint[0];

		/* The device clears some of the configuration of EP0
		 * when it receives the EORST.  Re-enable interrupts.
		 */
		endpoint->EPINTENSET.reg = USB_DEVICE_EPINTENSET_TRCPT0 |
					   USB_DEVICE_EPINTENSET_TRCPT1 |
					   USB_DEVICE_EPINTENSET_RXSTP;

		data->cb(USB_DC_RESET, NULL);
	}

	/* Dispatch the endpoint interrupts */
	for (ep = 0U; epint != 0U; epint >>= 1) {
		/* Scan bit-by-bit as the Cortex-M0 doesn't have ffs */
		if ((epint & 1) != 0U) {
			usb_sam0_ep_isr(ep);
		}
		ep++;
	}
}

/* Wait for the device to process the last config write */
static void usb_sam0_wait_syncbusy(void)
{
	UsbDevice *regs = &REGS->DEVICE;

	while (regs->SYNCBUSY.reg != 0) {
	}
}

/* Load the pad calibration from the built-in fuses */
static void usb_sam0_load_padcal(void)
{
	UsbDevice *regs = &REGS->DEVICE;
	uint32_t pad_transn;
	uint32_t pad_transp;
	uint32_t pad_trim;

#ifdef USB_FUSES_TRANSN_ADDR
	pad_transn = *(uint32_t *)USB_FUSES_TRANSN_ADDR;
#else
	pad_transn = (*((uint32_t *)(NVMCTRL_OTP4) +
			(NVM_USB_PAD_TRANSN_POS / 32)) >>
		      (NVM_USB_PAD_TRANSN_POS % 32)) &
		     ((1 << NVM_USB_PAD_TRANSN_SIZE) - 1);

	if (pad_transn == 0x1F) {
		pad_transn = 5U;
	}
#endif

	regs->PADCAL.bit.TRANSN = pad_transn;

#ifdef USB_FUSES_TRANSP_ADDR
	pad_transp = *(uint32_t *)USB_FUSES_TRANSP_ADDR;
#else
	pad_transp = (*((uint32_t *)(NVMCTRL_OTP4) +
			(NVM_USB_PAD_TRANSP_POS / 32)) >>
		      (NVM_USB_PAD_TRANSP_POS % 32)) &
		     ((1 << NVM_USB_PAD_TRANSP_SIZE) - 1);

	if (pad_transp == 0x1F) {
		pad_transp = 29U;
	}
#endif

	regs->PADCAL.bit.TRANSP = pad_transp;

#ifdef USB_FUSES_TRIM_ADDR
	pad_trim = *(uint32_t *)USB_FUSES_TRIM_ADDR;
#else
	pad_trim = (*((uint32_t *)(NVMCTRL_OTP4) +
		      (NVM_USB_PAD_TRIM_POS / 32)) >>
		    (NVM_USB_PAD_TRIM_POS % 32)) &
		   ((1 << NVM_USB_PAD_TRIM_SIZE) - 1);

	if (pad_trim == 0x7) {
		pad_trim = 3U;
	}
#endif

	regs->PADCAL.bit.TRIM = pad_trim;
}

#define SAM0_USB_IRQ_CONNECT(n) 				\
	do {							\
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(0, n, irq),		\
		    DT_INST_IRQ_BY_IDX(0, n, priority),		\
		    usb_sam0_isr, 0, 0);			\
	irq_enable(DT_INST_IRQ_BY_IDX(0, n, irq));		\
	} while (false)

/* Attach by initializing the device */
int usb_dc_attach(void)
{
	UsbDevice *regs = &REGS->DEVICE;
	struct usb_sam0_data *data = usb_sam0_get_data();
	int retval;

#ifdef MCLK
	/* Enable the clock in MCLK */
	MCLK->APBBMASK.bit.USB_ = 1;

	/* Enable the GCLK - use 48 MHz source */
	GCLK->PCHCTRL[USB_GCLK_ID].reg = GCLK_PCHCTRL_GEN(2)
				       | GCLK_PCHCTRL_CHEN;

	while (GCLK->SYNCBUSY.reg) {
	}
#else
	/* Enable the clock in PM */
	PM->APBBMASK.bit.USB_ = 1;

	/* Enable the GCLK */
	GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID_USB | GCLK_CLKCTRL_GEN_GCLK0 |
			    GCLK_CLKCTRL_CLKEN;

	while (GCLK->STATUS.bit.SYNCBUSY) {
	}
#endif /* !MCLK */

	/* Configure */
	regs->CTRLA.bit.SWRST = 1;
	usb_sam0_wait_syncbusy();

	/* Change QOS values to have the best performance and correct USB
	 * behaviour
	 */
	regs->QOSCTRL.bit.CQOS = 2;
	regs->QOSCTRL.bit.DQOS = 2;

	retval = pinctrl_apply_state(pcfg, PINCTRL_STATE_DEFAULT);
	if (retval < 0) {
		return retval;
	}

	usb_sam0_load_padcal();

	regs->CTRLA.reg = USB_CTRLA_MODE_DEVICE | USB_CTRLA_RUNSTDBY;
	regs->CTRLB.reg = USB_DEVICE_CTRLB_SPDCONF_HS;

	(void)memset(data->descriptors, 0, sizeof(data->descriptors));
	regs->DESCADD.reg = (uintptr_t)&data->descriptors[0];

	regs->INTENSET.reg = USB_DEVICE_INTENSET_EORST;

	/* Connect and enable the interrupt */
#if DT_INST_IRQ_HAS_CELL(0, irq)
	SAM0_USB_IRQ_CONNECT(0);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 1)
	SAM0_USB_IRQ_CONNECT(1);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 2)
	SAM0_USB_IRQ_CONNECT(2);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 3)
	SAM0_USB_IRQ_CONNECT(3);
#endif

	/* Enable and attach */
	regs->CTRLA.bit.ENABLE = 1;
	usb_sam0_wait_syncbusy();
	regs->CTRLB.bit.DETACH = 0;

	return 0;
}

/* Detach from the bus */
int usb_dc_detach(void)
{
	UsbDevice *regs = &REGS->DEVICE;

	regs->CTRLB.bit.DETACH = 1;
	usb_sam0_wait_syncbusy();

	return 0;
}

/* Remove the interrupt and reset the device */
int usb_dc_reset(void)
{
	UsbDevice *regs = &REGS->DEVICE;

	irq_disable(DT_INST_IRQN(0));

	regs->CTRLA.bit.SWRST = 1;
	usb_sam0_wait_syncbusy();

	return 0;
}

/* Queue a change in address.  This is processed later when the
 * current transfers are complete.
 */
int usb_dc_set_address(const uint8_t addr)
{
	struct usb_sam0_data *data = usb_sam0_get_data();

	data->addr = addr | USB_DEVICE_DADD_ADDEN;

	return 0;
}

void usb_dc_set_status_callback(const usb_dc_status_callback cb)
{
	struct usb_sam0_data *data = usb_sam0_get_data();

	data->cb = cb;
}

int usb_dc_ep_check_cap(const struct usb_dc_ep_cfg_data * const cfg)
{
	uint8_t ep_idx = USB_EP_GET_IDX(cfg->ep_addr);

	if ((cfg->ep_type == USB_DC_EP_CONTROL) && ep_idx) {
		LOG_ERR("invalid endpoint configuration");
		return -1;
	}

	if (ep_idx > USB_NUM_ENDPOINTS) {
		LOG_ERR("endpoint index/address too high");
		return -1;
	}

	return 0;
}

int usb_dc_ep_configure(const struct usb_dc_ep_cfg_data *const cfg)
{
	struct usb_sam0_data *data = usb_sam0_get_data();
	UsbDevice *regs = &REGS->DEVICE;
	uint8_t ep_idx = USB_EP_GET_IDX(cfg->ep_addr);
	UsbDeviceEndpoint *endpoint = &regs->DeviceEndpoint[ep_idx];
	UsbDeviceDescriptor *desc = &data->descriptors[ep_idx];
	UsbDeviceDescBank *bank;
	void *buf;
	int type;
	int size = -1;
	int i;

	/* Map the type to native type */
	switch (cfg->ep_type) {
	case USB_DC_EP_CONTROL:
		type = 1;
		break;
	case USB_DC_EP_ISOCHRONOUS:
		type = 2;
		break;
	case USB_DC_EP_BULK:
		type = 3;
		break;
	case USB_DC_EP_INTERRUPT:
		type = 4;
		break;
	default:
		return -EINVAL;
	}

	/* Map the endpoint size to native size */
	for (i = 0; i < ARRAY_SIZE(usb_sam0_pcksize_bytes); i++) {
		if (usb_sam0_pcksize_bytes[i] == cfg->ep_mps) {
			size = i;
			break;
		}
	}

	if (size < 0) {
		return -EINVAL;
	}

	if (USB_EP_DIR_IS_IN(cfg->ep_addr)) {
		bank = &desc->DeviceDescBank[1];
	} else {
		bank = &desc->DeviceDescBank[0];
	}

	buf = (void *)bank->ADDR.reg;

	if (bank->PCKSIZE.bit.SIZE != size || buf == NULL) {
		/* Release the previous buffer, if any */
		k_free(buf);

		buf = k_malloc(cfg->ep_mps);
		if (buf == NULL) {
			return -ENOMEM;
		}
		bank->PCKSIZE.bit.SIZE = size;
		bank->ADDR.reg = (uintptr_t)buf;
	}

	if (USB_EP_DIR_IS_IN(cfg->ep_addr)) {
		endpoint->EPCFG.bit.EPTYPE1 = type;
		endpoint->EPSTATUSCLR.bit.BK1RDY = 1;
	} else {
		endpoint->EPCFG.bit.EPTYPE0 = type;
		endpoint->EPSTATUSCLR.bit.BK0RDY = 1;
	}

	return 0;
}

int usb_dc_ep_set_stall(const uint8_t ep)
{
	UsbDevice *regs = &REGS->DEVICE;
	uint8_t for_in = USB_EP_GET_DIR(ep);
	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	UsbDeviceEndpoint *endpoint = &regs->DeviceEndpoint[ep_idx];

	if (ep_idx >= USB_NUM_ENDPOINTS) {
		LOG_ERR("endpoint index/address out of range");
		return -1;
	}

	if (for_in) {
		endpoint->EPSTATUSSET.bit.STALLRQ1 = 1;
	} else {
		endpoint->EPSTATUSSET.bit.STALLRQ0 = 1;
	}

	return 0;
}

int usb_dc_ep_clear_stall(const uint8_t ep)
{
	UsbDevice *regs = &REGS->DEVICE;
	uint8_t for_in = USB_EP_GET_DIR(ep);
	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	UsbDeviceEndpoint *endpoint = &regs->DeviceEndpoint[ep_idx];

	if (ep_idx >= USB_NUM_ENDPOINTS) {
		LOG_ERR("endpoint index/address out of range");
		return -1;
	}

	if (for_in) {
		endpoint->EPSTATUSCLR.bit.STALLRQ1 = 1;
	} else {
		endpoint->EPSTATUSCLR.bit.STALLRQ0 = 1;
	}

	return 0;
}

int usb_dc_ep_is_stalled(const uint8_t ep, uint8_t *stalled)
{
	UsbDevice *regs = &REGS->DEVICE;
	uint8_t for_in = USB_EP_GET_DIR(ep);
	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	UsbDeviceEndpoint *endpoint = &regs->DeviceEndpoint[ep_idx];

	if (ep_idx >= USB_NUM_ENDPOINTS) {
		LOG_ERR("endpoint index/address out of range");
		return -1;
	}

	if (stalled == NULL) {
		LOG_ERR("parameter must not be NULL");
		return -1;
	}

	if (for_in) {
		*stalled = endpoint->EPSTATUS.bit.STALLRQ1;
	} else {
		*stalled = endpoint->EPSTATUS.bit.STALLRQ0;
	}

	return 0;
}

/* Halt the selected endpoint */
int usb_dc_ep_halt(uint8_t ep)
{
	return usb_dc_ep_set_stall(ep);
}

/* Flush the selected endpoint */
int usb_dc_ep_flush(uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	if (ep_idx >= USB_NUM_ENDPOINTS) {
		LOG_ERR("endpoint index/address out of range");
		return -1;
	}

	/* TODO */
	LOG_WRN("flush not implemented");

	return 0;
}

/* Enable an endpoint and the endpoint interrupts */
int usb_dc_ep_enable(const uint8_t ep)
{
	UsbDevice *regs = &REGS->DEVICE;
	uint8_t for_in = USB_EP_GET_DIR(ep);
	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	UsbDeviceEndpoint *endpoint = &regs->DeviceEndpoint[ep_idx];

	if (ep_idx >= USB_NUM_ENDPOINTS) {
		LOG_ERR("endpoint index/address out of range");
		return -EINVAL;
	}

	if (for_in) {
		endpoint->EPSTATUSCLR.bit.BK1RDY = 1;
	} else {
		endpoint->EPSTATUSCLR.bit.BK0RDY = 1;
	}

	endpoint->EPINTENSET.reg = USB_DEVICE_EPINTENSET_TRCPT0 |
				   USB_DEVICE_EPINTENSET_TRCPT1 |
				   USB_DEVICE_EPINTENSET_RXSTP;

	return 0;
}

/* Disable the selected endpoint */
int usb_dc_ep_disable(uint8_t ep)
{
	UsbDevice *regs = &REGS->DEVICE;
	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	UsbDeviceEndpoint *endpoint = &regs->DeviceEndpoint[ep_idx];

	if (ep_idx >= USB_NUM_ENDPOINTS) {
		LOG_ERR("endpoint index/address out of range");
		return -EINVAL;
	}

	endpoint->EPINTENCLR.reg = USB_DEVICE_EPINTENCLR_TRCPT0
				 | USB_DEVICE_EPINTENCLR_TRCPT1
				 | USB_DEVICE_EPINTENCLR_RXSTP;

	return 0;
}

/* Write a single payload to the IN buffer on the endpoint */
int usb_dc_ep_write(uint8_t ep, const uint8_t *buf, uint32_t len, uint32_t *ret_bytes)
{
	struct usb_sam0_data *data = usb_sam0_get_data();
	UsbDevice *regs = &REGS->DEVICE;
	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	UsbDeviceEndpoint *endpoint = &regs->DeviceEndpoint[ep_idx];
	UsbDeviceDescriptor *desc = &data->descriptors[ep_idx];
	uint32_t addr = desc->DeviceDescBank[1].ADDR.reg;
	uint32_t capacity = usb_sam0_pcksize_bytes[
			desc->DeviceDescBank[1].PCKSIZE.bit.SIZE];

	if (ep_idx >= USB_NUM_ENDPOINTS) {
		LOG_ERR("endpoint index/address out of range");
		return -1;
	}

	if (endpoint->EPSTATUS.bit.BK1RDY) {
		/* Write in progress, drop */
		return -EAGAIN;
	}

	len = Z_MIN(len, capacity);

	/* Note that this code does not use the hardware's
	 * multi-packet and automatic zero-length packet features as
	 * the upper layers in Zephyr implement these in code.
	 */
	memcpy((void *)addr, buf, len);
	desc->DeviceDescBank[1].PCKSIZE.bit.MULTI_PACKET_SIZE = 0;
	desc->DeviceDescBank[1].PCKSIZE.bit.BYTE_COUNT = len;
	endpoint->EPINTFLAG.reg =
		USB_DEVICE_EPINTFLAG_TRCPT1 | USB_DEVICE_EPINTFLAG_TRFAIL1;
	endpoint->EPSTATUSSET.bit.BK1RDY = 1;

	if (ret_bytes != NULL) {
		*ret_bytes = len;
	}

	return 0;
}

/* Read data from an OUT endpoint */
int usb_dc_ep_read_ex(uint8_t ep, uint8_t *buf, uint32_t max_data_len,
		      uint32_t *read_bytes, bool wait)
{
	struct usb_sam0_data *data = usb_sam0_get_data();
	UsbDevice *regs = &REGS->DEVICE;
	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	UsbDeviceEndpoint *endpoint = &regs->DeviceEndpoint[ep_idx];
	UsbDeviceDescriptor *desc = &data->descriptors[ep_idx];
	uint32_t addr = desc->DeviceDescBank[0].ADDR.reg;
	uint32_t bytes = desc->DeviceDescBank[0].PCKSIZE.bit.BYTE_COUNT;
	uint32_t take;
	int remain;

	if (ep_idx >= USB_NUM_ENDPOINTS) {
		LOG_ERR("endpoint index/address out of range");
		return -1;
	}

	if (!endpoint->EPSTATUS.bit.BK0RDY) {
		return -EAGAIN;
	}

	/* The code below emulates the Quark FIFO which the Zephyr USB
	 * API is based on.  Reading with buf == NULL returns the
	 * number of bytes available and starts the read.  The caller
	 * then keeps calling until all bytes are consumed which
	 * also marks the OUT buffer as freed.
	 */
	if (buf == NULL) {
		data->out_at = 0U;

		if (read_bytes != NULL) {
			*read_bytes = bytes;
		}

		return 0;
	}

	remain = bytes - data->out_at;
	take = MIN(max_data_len, remain);
	memcpy(buf, (uint8_t *)addr + data->out_at, take);

	if (read_bytes != NULL) {
		*read_bytes = take;
	}

	if (take == remain) {
		if (!wait) {
			endpoint->EPSTATUSCLR.bit.BK0RDY = 1;
			data->out_at = 0U;
		}
	} else {
		data->out_at += take;
	}

	return 0;
}

int usb_dc_ep_read(uint8_t ep, uint8_t *buf, uint32_t max_data_len, uint32_t *read_bytes)
{
	return usb_dc_ep_read_ex(ep, buf, max_data_len, read_bytes, false);
}

int usb_dc_ep_read_wait(uint8_t ep, uint8_t *buf, uint32_t max_data_len,
			uint32_t *read_bytes)
{
	return usb_dc_ep_read_ex(ep, buf, max_data_len, read_bytes, true);
}

int usb_dc_ep_read_continue(uint8_t ep)
{
	struct usb_sam0_data *data = usb_sam0_get_data();
	UsbDevice *regs = &REGS->DEVICE;
	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	UsbDeviceEndpoint *endpoint = &regs->DeviceEndpoint[ep_idx];

	if (ep_idx >= USB_NUM_ENDPOINTS) {
		LOG_ERR("endpoint index/address out of range");
		return -1;
	}

	endpoint->EPSTATUSCLR.bit.BK0RDY = 1;
	data->out_at = 0U;

	return 0;
}

int usb_dc_ep_set_callback(const uint8_t ep, const usb_dc_ep_callback cb)
{
	struct usb_sam0_data *data = usb_sam0_get_data();
	uint8_t for_in = USB_EP_GET_DIR(ep);
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	if (ep_idx >= USB_NUM_ENDPOINTS) {
		LOG_ERR("endpoint index/address out of range");
		return -1;
	}

	data->ep_cb[for_in ? 1 : 0][ep_idx] = cb;

	return 0;
}

int usb_dc_ep_mps(const uint8_t ep)
{
	struct usb_sam0_data *data = usb_sam0_get_data();
	UsbDevice *regs = &REGS->DEVICE;
	uint8_t for_in = USB_EP_GET_DIR(ep);
	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	UsbDeviceDescriptor *desc = &data->descriptors[ep_idx];
	UsbDeviceEndpoint *endpoint = &regs->DeviceEndpoint[ep_idx];

	if (ep_idx >= USB_NUM_ENDPOINTS) {
		LOG_ERR("endpoint index/address out of range");
		return -1;
	}

	if (for_in) {
		/* if endpoint is not configured, this should return 0 */
		if (endpoint->EPCFG.bit.EPTYPE1 == 0) {
			return 0;
		}

		return usb_sam0_pcksize_bytes[
			desc->DeviceDescBank[1].PCKSIZE.bit.SIZE];
	} else {
		/* if endpoint is not configured, this should return 0 */
		if (endpoint->EPCFG.bit.EPTYPE0 == 0) {
			return 0;
		}

		return usb_sam0_pcksize_bytes[
			desc->DeviceDescBank[0].PCKSIZE.bit.SIZE];
	}
}

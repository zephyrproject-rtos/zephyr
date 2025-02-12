/*
 * Copyright (c) 2021, Pete Johanson
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <string.h>
#include <hardware/regs/usb.h>
#include <hardware/structs/usb.h>
#include <hardware/resets.h>
#include <pico/platform.h>

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/clock_control.h>

LOG_MODULE_REGISTER(udc_rpi, CONFIG_USB_DRIVER_LOG_LEVEL);

#define DT_DRV_COMPAT raspberrypi_pico_usbd

#define USB_BASE_ADDRESS DT_INST_REG_ADDR(0)
#define USB_IRQ DT_INST_IRQ_BY_NAME(0, usbctrl, irq)
#define USB_IRQ_PRI DT_INST_IRQ_BY_NAME(0, usbctrl, priority)
#define USB_NUM_BIDIR_ENDPOINTS DT_INST_PROP(0, num_bidir_endpoints)
#define CLK_DRV DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0))
#define CLK_ID (clock_control_subsys_t)DT_INST_PHA_BY_IDX(0, clocks, 0, clk_id)

#define DATA_BUFFER_SIZE 64U

/* Needed for pico-sdk */
#ifndef typeof
#define typeof __typeof__
#endif

struct udc_rpi_ep_state {
	uint16_t mps;
	enum usb_dc_ep_transfer_type type;
	uint8_t halted;
	usb_dc_ep_callback cb;
	uint32_t read_offset;
	struct k_sem write_sem;
	io_rw_32 *ep_ctl;
	io_rw_32 *buf_ctl;
	uint8_t *buf;
	uint8_t next_pid;
};

#define USBD_THREAD_STACK_SIZE 1024

K_THREAD_STACK_DEFINE(thread_stack, USBD_THREAD_STACK_SIZE);
static struct k_thread thread;

struct udc_rpi_state {
	usb_dc_status_callback status_cb;
	struct udc_rpi_ep_state out_ep_state[USB_NUM_BIDIR_ENDPOINTS];
	struct udc_rpi_ep_state in_ep_state[USB_NUM_BIDIR_ENDPOINTS];
	bool abort_control_writes;
	bool setup_available;
	bool should_set_address;
	uint16_t control_out_ep_rcvd;
	uint8_t addr;
	bool rwu_pending;
};

static struct udc_rpi_state state;

struct cb_msg {
	bool ep_event;
	uint32_t type;
	uint8_t ep;
};

K_MSGQ_DEFINE(usb_dc_msgq, sizeof(struct cb_msg), 10, 4);

static struct udc_rpi_ep_state *udc_rpi_get_ep_state(uint8_t ep)
{
	struct udc_rpi_ep_state *ep_state_base;

	if (USB_EP_GET_IDX(ep) >= USB_NUM_BIDIR_ENDPOINTS) {
		return NULL;
	}

	if (USB_EP_DIR_IS_OUT(ep)) {
		ep_state_base = state.out_ep_state;
	} else {
		ep_state_base = state.in_ep_state;
	}

	return ep_state_base + USB_EP_GET_IDX(ep);
}

static int udc_rpi_start_xfer(uint8_t ep, const void *data, const size_t len)
{
	struct udc_rpi_ep_state *ep_state = udc_rpi_get_ep_state(ep);
	uint32_t val = len;

	if (*ep_state->buf_ctl & USB_BUF_CTRL_AVAIL) {
		LOG_WRN("ep 0x%02x was already armed", ep);
	}

	if (USB_EP_DIR_IS_IN(ep)) {
		if (len > DATA_BUFFER_SIZE) {
			return -ENOMEM;
		}

		val |= USB_BUF_CTRL_FULL;
		if (data) {
			memcpy(ep_state->buf, data, len);
		}
	} else {
		ep_state->read_offset = 0;
	}

	LOG_DBG("xfer ep %d len %d pid: %d", ep, len, ep_state->next_pid);
	val |= ep_state->next_pid ? USB_BUF_CTRL_DATA1_PID : USB_BUF_CTRL_DATA0_PID;

	ep_state->next_pid ^= 1u;
	*ep_state->buf_ctl = val;
	/*
	 * By default, clk_sys runs at 125MHz, wait 3 nop instructions before
	 * setting the AVAILABLE bit. See 4.1.2.5.1. Concurrent access.
	 */
	arch_nop();
	arch_nop();
	arch_nop();
	*ep_state->buf_ctl = val | USB_BUF_CTRL_AVAIL;

	return 0;
}

/*
 * This function converts a zephyr endpoint address into a
 * bit mask that can be used with registers:
 *  - BUFF_STATUS
 *  - BUFF_CPU_SHOULD_HANDLE
 *  - EP_ABOR
 *  - EP_ABORT_DONE
 *  - EP_STATUS_STALL_NAK
 */
static inline uint32_t udc_rpi_endpoint_mask(const uint8_t ep)
{
	const int bit_index = (USB_EP_GET_IDX(ep) << 1) | !!USB_EP_DIR_IS_OUT(ep);

	return BIT(bit_index);
}

static void udc_rpi_cancel_endpoint(const uint8_t ep)
{
	struct udc_rpi_ep_state *const ep_state = udc_rpi_get_ep_state(ep);

	if (*ep_state->buf_ctl & USB_BUF_CTRL_AVAIL) {
		const uint32_t mask = udc_rpi_endpoint_mask(ep);
		bool abort_handshake_supported = rp2040_chip_version() >= 2;

		if (abort_handshake_supported) {
			hw_set_alias(usb_hw)->abort = mask;
			while ((usb_hw->abort_done & mask) != mask) {
			}
		}

		*ep_state->buf_ctl &= ~USB_BUF_CTRL_AVAIL;

		if (abort_handshake_supported) {
			hw_clear_alias(usb_hw)->abort = mask;
		}

		if (USB_EP_DIR_IS_IN(ep)) {
			k_sem_give(&ep_state->write_sem);
		}
	}
}

static void udc_rpi_handle_setup(void)
{
	const struct udc_rpi_ep_state *const ep_state = udc_rpi_get_ep_state(USB_CONTROL_EP_OUT);
	struct cb_msg msg;

	/* Normally all control transfers should complete before a new setup
	 * transaction is sent, however in some rare cases from the perspective
	 * of the device, a new setup transaction could arrive prematurely, in
	 * which case the previous control transfer should be aborted, and for
	 * this reason we're canceling both control IN and control OUT
	 * endpoints. See section 5.5.5 of the Universal Serial Bus
	 * Specification, version 2.0.
	 */

	udc_rpi_cancel_endpoint(USB_CONTROL_EP_IN);

	if (*ep_state->buf_ctl & USB_BUF_CTRL_AVAIL) {
		udc_rpi_cancel_endpoint(USB_CONTROL_EP_OUT);

		/* This warning could be triggered by the rare event described
		 * above, but it could also be a sign of a software bug, that
		 * can expose us to race conditions when the system is slowed
		 * down, because it becomes impossible to determine in what
		 * order did setup/data transactions arrive.
		 */

		LOG_WRN("EP0_OUT was armed while setup stage arrived.");
	}

	state.abort_control_writes = true;

	/* Set DATA1 PID for the next (data or status) stage */
	udc_rpi_get_ep_state(USB_CONTROL_EP_IN)->next_pid = 1;
	udc_rpi_get_ep_state(USB_CONTROL_EP_OUT)->next_pid = 1;

	msg.ep = USB_CONTROL_EP_OUT;
	msg.type = USB_DC_EP_SETUP;
	msg.ep_event = true;

	k_msgq_put(&usb_dc_msgq, &msg, K_NO_WAIT);
}

static void udc_rpi_handle_buff_status(void)
{
	struct udc_rpi_ep_state *ep_state;
	enum usb_dc_ep_cb_status_code status_code;
	uint32_t status = usb_hw->buf_status;
	unsigned int bit = 1U;
	struct cb_msg msg;

	LOG_DBG("status: %d", status);

	for (int i = 0; status && i < USB_NUM_BIDIR_ENDPOINTS * 2; i++) {
		if (status & bit) {
			hw_clear_alias(usb_hw)->buf_status = bit;
			bool in = !(i & 1U);
			uint8_t ep = (i >> 1U) | (in ? USB_EP_DIR_IN : USB_EP_DIR_OUT);

			ep_state = udc_rpi_get_ep_state(ep);
			status_code = in ? USB_DC_EP_DATA_IN : USB_DC_EP_DATA_OUT;

			LOG_DBG("buff ep %i in? %i", (i >> 1), in);

			if (i == 0 && in && state.should_set_address) {
				state.should_set_address = false;
				usb_hw->dev_addr_ctrl = state.addr;
			}

			if (in) {
				k_sem_give(&ep_state->write_sem);
			}

			msg.ep = ep;
			msg.ep_event = true;
			msg.type = status_code;

			k_msgq_put(&usb_dc_msgq, &msg, K_NO_WAIT);

			status &= ~bit;
		}

		bit <<= 1U;
	}
}

static void udc_rpi_handle_resume(void)
{
	struct cb_msg msg = {
		.ep = 0U,
		.type = USB_DC_RESUME,
		.ep_event = false,
	};

	LOG_DBG("Resume");
	k_msgq_put(&usb_dc_msgq, &msg, K_NO_WAIT);
	state.rwu_pending = false;
}

static void udc_rpi_handle_suspended(void)
{
	struct cb_msg msg = {
		.ep = 0U,
		.type = USB_DC_SUSPEND,
		.ep_event = false,
	};

	LOG_DBG("Suspended");
	k_msgq_put(&usb_dc_msgq, &msg, K_NO_WAIT);
}

static void udc_rpi_isr(const void *arg)
{
	uint32_t status = usb_hw->ints;
	uint32_t handled = 0;
	struct cb_msg msg;

	if ((status & (USB_INTS_BUFF_STATUS_BITS | USB_INTS_SETUP_REQ_BITS)) &&
	    state.rwu_pending) {
		/* The rpi pico USB device does not appear to be sending
		 * USB_INTR_DEV_RESUME_FROM_HOST interrupts when the resume is
		 * a result of a remote wakeup request sent by us.
		 * This will simulate a resume event if bus activity is observed
		 */

		udc_rpi_handle_resume();
	}

	if (status & USB_INTS_BUFF_STATUS_BITS) {
		/* Note: we should check buffer interrupts before setup interrupts.
		 * this may seem a little counter-intuitive, because setup irqs
		 * sound more urgent, however in case we see an EP0_OUT buffer irq
		 * at the same time as a setup irq, then we know the buffer irq
		 * belongs to the previous control transfer, so we want to handle
		 * that first.
		 */

		handled |= USB_INTS_BUFF_STATUS_BITS;
		udc_rpi_handle_buff_status();
	}

	if (status & USB_INTS_SETUP_REQ_BITS) {
		handled |= USB_INTS_SETUP_REQ_BITS;
		hw_clear_alias(usb_hw)->sie_status = USB_SIE_STATUS_SETUP_REC_BITS;
		udc_rpi_handle_setup();
	}

	if (status & USB_INTS_DEV_CONN_DIS_BITS) {
		LOG_DBG("buf %u ep %u", *udc_rpi_get_ep_state(0x81)->buf_ctl,
			*udc_rpi_get_ep_state(0x81)->ep_ctl);
		handled |= USB_INTS_DEV_CONN_DIS_BITS;
		hw_clear_alias(usb_hw)->sie_status = USB_SIE_STATUS_CONNECTED_BITS;

		msg.ep = 0U;
		msg.ep_event = false;
		msg.type = usb_hw->sie_status & USB_SIE_STATUS_CONNECTED_BITS ?
			USB_DC_DISCONNECTED :
			USB_DC_CONNECTED;

		k_msgq_put(&usb_dc_msgq, &msg, K_NO_WAIT);
	}

	if (status & USB_INTS_BUS_RESET_BITS) {
		LOG_WRN("BUS RESET");
		handled |= USB_INTS_BUS_RESET_BITS;
		hw_clear_alias(usb_hw)->sie_status = USB_SIE_STATUS_BUS_RESET_BITS;
		usb_hw->dev_addr_ctrl = 0;

		/* The DataInCallback will never be called at this point for any pending
		 * transactions. Reset the IN semaphores to prevent perpetual locked state.
		 */

		for (int i = 0; i < USB_NUM_BIDIR_ENDPOINTS; i++) {
			k_sem_give(&state.in_ep_state[i].write_sem);
		}

		msg.ep = 0U;
		msg.type = USB_DC_RESET;
		msg.ep_event = false;

		k_msgq_put(&usb_dc_msgq, &msg, K_NO_WAIT);
	}

	if (status & USB_INTS_DEV_SUSPEND_BITS) {
		handled |= USB_INTS_DEV_SUSPEND_BITS;
		hw_clear_alias(usb_hw)->sie_status = USB_SIE_STATUS_SUSPENDED_BITS;
		udc_rpi_handle_suspended();
	}

	if (status & USB_INTR_DEV_RESUME_FROM_HOST_BITS) {
		handled |= USB_INTR_DEV_RESUME_FROM_HOST_BITS;
		hw_clear_alias(usb_hw)->sie_status = USB_SIE_STATUS_RESUME_BITS;
		udc_rpi_handle_resume();
	}

	if (status & USB_INTS_ERROR_DATA_SEQ_BITS) {
		LOG_WRN("data seq");
		hw_clear_alias(usb_hw)->sie_status = USB_SIE_STATUS_DATA_SEQ_ERROR_BITS;
		handled |= USB_INTS_ERROR_DATA_SEQ_BITS;
	}

	if (status & USB_INTS_ERROR_RX_TIMEOUT_BITS) {
		LOG_WRN("rx timeout");
		hw_clear_alias(usb_hw)->sie_status = USB_SIE_STATUS_RX_TIMEOUT_BITS;
		handled |= USB_INTS_ERROR_RX_TIMEOUT_BITS;
	}

	if (status & USB_INTS_ERROR_RX_OVERFLOW_BITS) {
		LOG_WRN("rx overflow");
		hw_clear_alias(usb_hw)->sie_status = USB_SIE_STATUS_RX_OVERFLOW_BITS;
		handled |= USB_INTS_ERROR_RX_OVERFLOW_BITS;
	}

	if (status & USB_INTS_ERROR_BIT_STUFF_BITS) {
		LOG_WRN("bit stuff error");
		hw_clear_alias(usb_hw)->sie_status = USB_SIE_STATUS_BIT_STUFF_ERROR_BITS;
		handled |= USB_INTS_ERROR_BIT_STUFF_BITS;
	}

	if (status & USB_INTS_ERROR_CRC_BITS) {
		LOG_ERR("crc error");
		hw_clear_alias(usb_hw)->sie_status = USB_SIE_STATUS_CRC_ERROR_BITS;
		handled |= USB_INTS_ERROR_CRC_BITS;
	}

	if (status ^ handled) {
		LOG_ERR("unhandled IRQ: 0x%x", (uint)(status ^ handled));
	}
}

static void udc_rpi_init_endpoint(const uint8_t i)
{
	state.out_ep_state[i].buf_ctl = &usb_dpram->ep_buf_ctrl[i].out;
	state.in_ep_state[i].buf_ctl = &usb_dpram->ep_buf_ctrl[i].in;

	if (i != 0) {
		state.out_ep_state[i].ep_ctl = &usb_dpram->ep_ctrl[i - 1].out;
		state.in_ep_state[i].ep_ctl = &usb_dpram->ep_ctrl[i - 1].in;

		state.out_ep_state[i].buf =
			&usb_dpram->epx_data[((i - 1) * 2 + 1) * DATA_BUFFER_SIZE];
		state.in_ep_state[i].buf = &usb_dpram->epx_data[((i - 1) * 2) * DATA_BUFFER_SIZE];
	} else {
		state.out_ep_state[i].buf = &usb_dpram->ep0_buf_a[0];
		state.in_ep_state[i].buf = &usb_dpram->ep0_buf_a[0];
	}

	k_sem_init(&state.in_ep_state[i].write_sem, 1, 1);
}

static int udc_rpi_init(void)
{
	/* Reset usb controller */
	reset_block(RESETS_RESET_USBCTRL_BITS);
	unreset_block_wait(RESETS_RESET_USBCTRL_BITS);

	/* Clear any previous state in dpram/hw just in case */
	memset(usb_hw, 0, sizeof(*usb_hw));
	memset(usb_dpram, 0, sizeof(*usb_dpram));

	/* Mux the controller to the onboard usb phy */
	usb_hw->muxing = USB_USB_MUXING_TO_PHY_BITS | USB_USB_MUXING_SOFTCON_BITS;

	/* Force VBUS detect so the device thinks it is plugged into a host */
	usb_hw->pwr = USB_USB_PWR_VBUS_DETECT_BITS | USB_USB_PWR_VBUS_DETECT_OVERRIDE_EN_BITS;

	/* Enable the USB controller in device mode. */
	usb_hw->main_ctrl = USB_MAIN_CTRL_CONTROLLER_EN_BITS;

	/* Enable an interrupt per EP0 transaction */
	usb_hw->sie_ctrl = USB_SIE_CTRL_EP0_INT_1BUF_BITS;

	/* Enable interrupts for when a buffer is done, when the bus is reset,
	 * and when a setup packet is received, and device connection status
	 */
	usb_hw->inte = USB_INTS_BUFF_STATUS_BITS | USB_INTS_BUS_RESET_BITS |
		       USB_INTS_DEV_CONN_DIS_BITS |
		       USB_INTS_SETUP_REQ_BITS | /*USB_INTS_EP_STALL_NAK_BITS |*/
		       USB_INTS_ERROR_BIT_STUFF_BITS | USB_INTS_ERROR_CRC_BITS |
		       USB_INTS_ERROR_DATA_SEQ_BITS | USB_INTS_ERROR_RX_OVERFLOW_BITS |
		       USB_INTS_ERROR_RX_TIMEOUT_BITS | USB_INTS_DEV_SUSPEND_BITS |
		       USB_INTR_DEV_RESUME_FROM_HOST_BITS;

	/* Set up endpoints (endpoint control registers)
	 * described by device configuration
	 * usb_setup_endpoints();
	 */
	for (int i = 0; i < USB_NUM_BIDIR_ENDPOINTS; i++) {
		udc_rpi_init_endpoint(i);
	}

	/* Present full speed device by enabling pull up on DP */
	hw_set_alias(usb_hw)->sie_ctrl = USB_SIE_CTRL_PULLUP_EN_BITS;

	return 0;
}

/* Zephyr USB device controller API implementation */

int usb_dc_attach(void)
{
	return udc_rpi_init();
}

int usb_dc_ep_set_callback(const uint8_t ep, const usb_dc_ep_callback cb)
{
	struct udc_rpi_ep_state *ep_state = udc_rpi_get_ep_state(ep);

	LOG_DBG("ep 0x%02x", ep);

	if (!ep_state) {
		return -EINVAL;
	}

	ep_state->cb = cb;

	return 0;
}

void usb_dc_set_status_callback(const usb_dc_status_callback cb)
{
	state.status_cb = cb;
}

int usb_dc_set_address(const uint8_t addr)
{
	LOG_DBG("addr %u (0x%02x)", addr, addr);

	state.should_set_address = true;
	state.addr = addr;

	return 0;
}

int usb_dc_ep_start_read(uint8_t ep, size_t len)
{
	int ret;

	LOG_DBG("ep 0x%02x len %d", ep, len);

	if (!USB_EP_DIR_IS_OUT(ep)) {
		LOG_ERR("invalid ep 0x%02x", ep);
		return -EINVAL;
	}

	if (len > DATA_BUFFER_SIZE) {
		len = DATA_BUFFER_SIZE;
	}

	ret = udc_rpi_start_xfer(ep, NULL, len);

	return ret;
}

int usb_dc_ep_check_cap(const struct usb_dc_ep_cfg_data *const cfg)
{
	uint8_t ep_idx = USB_EP_GET_IDX(cfg->ep_addr);

	LOG_DBG("ep %x, mps %d, type %d",
		cfg->ep_addr, cfg->ep_mps, cfg->ep_type);

	if ((cfg->ep_type == USB_DC_EP_CONTROL) && ep_idx) {
		LOG_ERR("invalid endpoint configuration");
		return -1;
	}

	if (ep_idx > (USB_NUM_BIDIR_ENDPOINTS - 1)) {
		LOG_ERR("endpoint index/address out of range");
		return -1;
	}

	return 0;
}

int usb_dc_ep_configure(const struct usb_dc_ep_cfg_data *const ep_cfg)
{
	uint8_t ep = ep_cfg->ep_addr;
	struct udc_rpi_ep_state *ep_state = udc_rpi_get_ep_state(ep);

	if (!ep_state) {
		return -EINVAL;
	}

	LOG_DBG("ep 0x%02x, previous mps %u, mps %u, type %u",
		ep_cfg->ep_addr, ep_state->mps,
		ep_cfg->ep_mps, ep_cfg->ep_type);

	ep_state->mps = ep_cfg->ep_mps;
	ep_state->type = ep_cfg->ep_type;

	return 0;
}

int usb_dc_ep_set_stall(const uint8_t ep)
{
	struct udc_rpi_ep_state *ep_state = udc_rpi_get_ep_state(ep);

	LOG_DBG("ep 0x%02x", ep);

	if (!ep_state) {
		return -EINVAL;
	}
	if (USB_EP_GET_IDX(ep) == 0) {
		hw_set_alias(usb_hw)->ep_stall_arm = USB_EP_DIR_IS_OUT(ep) ?
			USB_EP_STALL_ARM_EP0_OUT_BITS :
			USB_EP_STALL_ARM_EP0_IN_BITS;
	}

	*ep_state->buf_ctl = USB_BUF_CTRL_STALL;
	if (ep == USB_CONTROL_EP_IN) {
		/* Un-arm EP0_OUT endpoint, to make sure next setup packet starts clean */
		udc_rpi_cancel_endpoint(USB_CONTROL_EP_OUT);
	}

	ep_state->halted = 1U;

	return 0;
}

int usb_dc_ep_clear_stall(const uint8_t ep)
{
	struct udc_rpi_ep_state *ep_state = udc_rpi_get_ep_state(ep);
	uint8_t val;

	LOG_DBG("ep 0x%02x", ep);

	if (!ep_state) {
		return -EINVAL;
	}

	if (USB_EP_GET_IDX(ep) > 0) {
		val = *ep_state->buf_ctl;
		val &= ~USB_BUF_CTRL_STALL;

		*ep_state->buf_ctl = val;

		ep_state->halted = 0U;
		ep_state->read_offset = 0U;
	}

	return 0;
}

int usb_dc_ep_is_stalled(const uint8_t ep, uint8_t *const stalled)
{
	struct udc_rpi_ep_state *ep_state = udc_rpi_get_ep_state(ep);

	LOG_DBG("ep 0x%02x", ep);

	if (!ep_state || !stalled) {
		return -EINVAL;
	}

	*stalled = ep_state->halted;

	return 0;
}

static inline uint32_t usb_dc_ep_rpi_pico_buffer_offset(volatile uint8_t *buf)
{
	/* TODO: Bits 0-5 are ignored by the controller so make sure these are 0 */
	return (uint32_t)buf ^ (uint32_t)usb_dpram;
}

int usb_dc_ep_enable(const uint8_t ep)
{
	struct udc_rpi_ep_state *ep_state = udc_rpi_get_ep_state(ep);

	if (!ep_state) {
		return -EINVAL;
	}

	LOG_DBG("ep 0x%02x (id: %d) -> type %d", ep, USB_EP_GET_IDX(ep), ep_state->type);

	/* clear buffer state */
	*ep_state->buf_ctl = USB_BUF_CTRL_DATA0_PID;
	ep_state->next_pid = 0;

	/* EP0 doesn't have an ep_ctl */
	if (ep_state->ep_ctl) {
		uint32_t val =
			EP_CTRL_ENABLE_BITS |
			EP_CTRL_INTERRUPT_PER_BUFFER |
			(ep_state->type << EP_CTRL_BUFFER_TYPE_LSB) |
			usb_dc_ep_rpi_pico_buffer_offset(ep_state->buf);

		*ep_state->ep_ctl = val;
	}

	if (USB_EP_DIR_IS_OUT(ep) && ep != USB_CONTROL_EP_OUT) {
		return usb_dc_ep_start_read(ep, DATA_BUFFER_SIZE);
	}

	return 0;
}

int usb_dc_ep_disable(const uint8_t ep)
{
	struct udc_rpi_ep_state *ep_state = udc_rpi_get_ep_state(ep);

	LOG_DBG("ep 0x%02x", ep);

	if (!ep_state) {
		return -EINVAL;
	}

	/* EP0 doesn't have an ep_ctl */
	if (!ep_state->ep_ctl) {
		return 0;
	}

	/* If this endpoint has previously been used and e.g. the host application
	 * crashed, the endpoint may remain locked even after reconfiguration
	 * because the write semaphore is never given back.
	 * udc_rpi_cancel_endpoint() handles this so the endpoint can be written again.
	 */
	udc_rpi_cancel_endpoint(ep);

	uint8_t val = *ep_state->ep_ctl & ~EP_CTRL_ENABLE_BITS;

	*ep_state->ep_ctl = val;

	return 0;
}

int usb_dc_ep_write(const uint8_t ep, const uint8_t *const data,
		    const uint32_t data_len, uint32_t *const ret_bytes)
{
	struct udc_rpi_ep_state *ep_state = udc_rpi_get_ep_state(ep);
	uint32_t len = data_len;
	int ret = 0;

	LOG_DBG("ep 0x%02x, len %u", ep, data_len);

	if (!ep_state || !USB_EP_DIR_IS_IN(ep)) {
		LOG_ERR("invalid ep 0x%02x", ep);
		return -EINVAL;
	}

	if (ep == USB_CONTROL_EP_IN && state.abort_control_writes) {
		/* If abort_control_writes is high, it means the setup packet has not
		 * yet been consumed by the thread, which means that this write
		 * is part of a previous control transfer, which now must be
		 * aborted.
		 */

		if (ret_bytes != NULL) {
			*ret_bytes = len;
		}

		return 0;
	}

	if (ep == USB_CONTROL_EP_IN && len > USB_MAX_CTRL_MPS) {
		len = USB_MAX_CTRL_MPS;
	} else if (len > ep_state->mps) {
		len = ep_state->mps;
	}

	ret = k_sem_take(&ep_state->write_sem, K_NO_WAIT);
	if (ret) {
		return -EAGAIN;
	}

	if (!k_is_in_isr()) {
		irq_disable(USB_IRQ);
	}

	ret = udc_rpi_start_xfer(ep, data, len);

	if (ret < 0) {
		k_sem_give(&ep_state->write_sem);
		ret = -EIO;
	}

	if (!k_is_in_isr()) {
		irq_enable(USB_IRQ);
	}

	if (ret >= 0 && ret_bytes != NULL) {
		*ret_bytes = len;
	}

	return ret;
}

uint32_t udc_rpi_get_ep_buffer_len(const uint8_t ep)
{
	struct udc_rpi_ep_state *ep_state = udc_rpi_get_ep_state(ep);
	uint32_t buf_ctl = *ep_state->buf_ctl;

	return buf_ctl & USB_BUF_CTRL_LEN_MASK;
}

int usb_dc_ep_read_wait(uint8_t ep, uint8_t *data,
			uint32_t max_data_len, uint32_t *read_bytes)
{
	struct udc_rpi_ep_state *ep_state = udc_rpi_get_ep_state(ep);
	uint32_t read_count;

	if (!ep_state) {
		LOG_ERR("Invalid Endpoint %x", ep);
		return -EINVAL;
	}

	if (!USB_EP_DIR_IS_OUT(ep)) {
		LOG_ERR("Wrong endpoint direction: 0x%02x", ep);
		return -EINVAL;
	}

	if (ep == USB_CONTROL_EP_OUT && state.setup_available) {
		read_count = sizeof(struct usb_setup_packet);
		if (read_count != max_data_len) {
			LOG_WRN("Attempting to read setup packet with the wrong length"
				" (expected: %d, read: %d)", read_count, max_data_len);
		}
	} else {
		read_count = udc_rpi_get_ep_buffer_len(ep) - ep_state->read_offset;
	}

	LOG_DBG("ep 0x%02x, %u bytes, %u+%u, %p", ep, max_data_len, ep_state->read_offset,
		read_count, (void *)data);

	if (data) {
		read_count = MIN(read_count, max_data_len);

		if (ep == USB_CONTROL_EP_OUT && state.setup_available) {
			memcpy(data, (const void *)&usb_dpram->setup_packet, read_count);
		} else {
			memcpy(data, ep_state->buf + ep_state->read_offset, read_count);
		}

		ep_state->read_offset += read_count;
	} else if (max_data_len) {
		LOG_ERR("Wrong arguments");
	}

	if (read_bytes) {
		*read_bytes = read_count;
	}

	return 0;
}

static int usb_dc_control_ep_read_continue(const struct udc_rpi_ep_state *const ep_state,
					   bool *const arm_out_endpoint)
{
	const struct usb_setup_packet *const setup = (const void *)&usb_dpram->setup_packet;

	if (state.setup_available) {
		LOG_DBG("EP0 setup (wLength=%d, is_to_device=%d)",
			setup->wLength, usb_reqtype_is_to_device(setup));
		if (setup->wLength != 0U) {
			/* In the case of a control transfer, we want to prime the OUT endpoint
			 * exactly once, to either:
			 * 1) in the to_device case, to receive the data (only if wLength is not 0)
			 * 2) in the to_host case, to receive a 0-length status stage transfer
			 *    (only valid if wLength is not 0)
			 * Note that when wLength = 0, the status stage transfer is always an IN
			 * type so we don't need to consider that case.
			 */
			*arm_out_endpoint = true;
			state.control_out_ep_rcvd = 0;
		}

		state.setup_available = false;
	} else {
		const size_t len = udc_rpi_get_ep_buffer_len(USB_CONTROL_EP_OUT);

		LOG_DBG("Control OUT received %u offset: %u",
			len, ep_state->read_offset);
		if (usb_reqtype_is_to_device(setup)) {
			if (state.control_out_ep_rcvd + ep_state->read_offset < setup->wLength) {
				/* If no more data in the buffer, but we're still waiting
				 * for more, start a new read transaction.
				 */
				if (len == ep_state->read_offset) {
					state.control_out_ep_rcvd += ep_state->read_offset;
					*arm_out_endpoint = true;
				}
			}
		}
	}
	return 0;
}

int usb_dc_ep_read_continue(const uint8_t ep)
{
	const struct udc_rpi_ep_state *const ep_state = udc_rpi_get_ep_state(ep);
	bool arm_out_endpoint = false;

	if (!ep_state || !USB_EP_DIR_IS_OUT(ep)) {
		LOG_ERR("Not valid endpoint: %02x", ep);
		return -EINVAL;
	}
	if (ep == USB_CONTROL_EP_OUT) {
		int ret = usb_dc_control_ep_read_continue(ep_state, &arm_out_endpoint);

		if (ret != 0) {
			return ret;
		}
	} else {
		const size_t len = udc_rpi_get_ep_buffer_len(ep);

		LOG_DBG("Endpoint 0x%02x received %u offset: %u",
			ep, len, ep_state->read_offset);
		/* If no more data in the buffer, start a new read transaction. */
		if (len == ep_state->read_offset) {
			arm_out_endpoint = true;
		}
	}

	if (arm_out_endpoint) {
		LOG_DBG("Arming endpoint 0x%02x", ep);
		return usb_dc_ep_start_read(ep, DATA_BUFFER_SIZE);
	} else {
		LOG_DBG("Not arming endpoint 0x%02x", ep);
	}

	return 0;
}

int usb_dc_ep_read(const uint8_t ep, uint8_t *const data,
		   const uint32_t max_data_len, uint32_t *const read_bytes)
{
	if (usb_dc_ep_read_wait(ep, data, max_data_len, read_bytes) != 0) {
		return -EINVAL;
	}

	if (!max_data_len) {
		return 0;
	}

	if (usb_dc_ep_read_continue(ep) != 0) {
		return -EINVAL;
	}

	return 0;
}

int usb_dc_ep_halt(const uint8_t ep)
{
	return usb_dc_ep_set_stall(ep);
}

int usb_dc_ep_flush(const uint8_t ep)
{
	struct udc_rpi_ep_state *ep_state = udc_rpi_get_ep_state(ep);

	if (!ep_state) {
		return -EINVAL;
	}

	LOG_ERR("Not implemented");

	return 0;
}

int usb_dc_ep_mps(const uint8_t ep)
{
	struct udc_rpi_ep_state *ep_state = udc_rpi_get_ep_state(ep);

	if (!ep_state) {
		return -EINVAL;
	}

	return ep_state->mps;
}

int usb_dc_detach(void)
{
	LOG_ERR("Not implemented");

	return 0;
}

int usb_dc_reset(void)
{
	LOG_ERR("Not implemented");

	return 0;
}

int usb_dc_wakeup_request(void)
{
	LOG_DBG("Remote Wakeup");
	state.rwu_pending = true;
	hw_set_alias(usb_hw)->sie_ctrl = USB_SIE_CTRL_RESUME_BITS;

	return 0;
}

/*
 * This thread is only used to not run the USB device stack and endpoint
 * callbacks in the ISR context, which happens when an callback function
 * is called. TODO: something similar should be implemented in the USB
 * device stack so that it can be used by all drivers.
 */
static void udc_rpi_thread_main(void *arg1, void *unused1, void *unused2)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	struct cb_msg msg;

	while (true) {
		k_msgq_get(&usb_dc_msgq, &msg, K_FOREVER);

		if (msg.ep_event) {
			struct udc_rpi_ep_state *ep_state = udc_rpi_get_ep_state(msg.ep);

			if (msg.type == USB_DC_EP_SETUP) {
				state.abort_control_writes = false;
				state.setup_available = true;
			}

			if (ep_state->cb) {
				ep_state->cb(msg.ep, msg.type);
			}
		} else {
			if (state.status_cb) {
				state.status_cb(msg.type, NULL);
			}
		}
	}
}

static int usb_rpi_init(void)
{
	int ret;

	k_thread_create(&thread, thread_stack,
			USBD_THREAD_STACK_SIZE,
			udc_rpi_thread_main, NULL, NULL, NULL,
			K_PRIO_COOP(2), 0, K_NO_WAIT);
	k_thread_name_set(&thread, "usb_rpi");

	ret = clock_control_on(CLK_DRV, CLK_ID);
	if (ret < 0) {
		return ret;
	}

	IRQ_CONNECT(USB_IRQ, USB_IRQ_PRI, udc_rpi_isr, 0, 0);
	irq_enable(USB_IRQ);

	return 0;
}

SYS_INIT(usb_rpi_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

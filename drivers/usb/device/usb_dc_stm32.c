/* USB device controller driver for STM32 devices */

/*
 * Copyright (c) 2017 Christer Weinigel.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB device controller driver for STM32 devices
 *
 * This driver uses the STM32 Cube low level drivers to talk to the USB
 * device controller on the STM32 family of devices using the
 * STM32Cube HAL layer.
 *
 * There is a bit of an impedance mismatch between the Zephyr
 * usb_device and the STM32 Cube HAL layer where higher levels make
 * assumptions about the low level drivers that don't quite match how
 * the low level drivers actually work.
 *
 * The usb_dc_ep_read function expects to get the data it wants
 * immediately while the HAL_PCD_EP_Receive function only starts a
 * read transaction and the data won't be available until call to
 * HAL_PCD_DataOutStageCallback. To work around this I've
 * had to add an extra packet buffer in the driver which wastes memory
 * and also leads to an extra copy of all received data.  It would be
 * better if higher drivers could call start_read and get_read_count
 * in this driver directly.
 *
 * To enable the driver together with the CDC_ACM high level driver,
 * add the following to your board's defconfig:
 *
 * CONFIG_USB=y
 * CONFIG_USB_DC_STM32=y
 * CONFIG_USB_CDC_ACM=y
 * CONFIG_USB_DEVICE_STACK=y
 *
 * To use the USB device as a console, also add:
 *
 * CONFIG_UART_CONSOLE_ON_DEV_NAME="CDC_ACM"
 * CONFIG_USB_UART_CONSOLE=y
 * CONFIG_UART_LINE_CTRL=y
 */

#include <soc.h>
#include <string.h>
#include <usb/usb_dc.h>
#include <usb/usb_device.h>
#include <clock_control/stm32_clock_control.h>
#include <misc/util.h>

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_USB_DRIVER_LEVEL
#include <logging/sys_log.h>


/* We need one RX FIFO and n TX FIFOs */
#define FIFO_NUM (1 + CONFIG_USB_DC_STM32_EP_NUM)

/* 4-byte words FIFO */
#define FIFO_WORDS (CONFIG_USB_DC_STM32_RAM_SIZE / 4)

/* Allocate FIFO memory evenly between the FIFOs */
#define FIFO_EP_WORDS (FIFO_WORDS / FIFO_NUM)

/* Size of a USB SETUP packet */
#define SETUP_SIZE 8

/* Helper macros to make it easier to work with endpoint numbers */
#define EP0_IDX 0
#define EP0_IN (EP0_IDX | USB_EP_DIR_IN)
#define EP0_OUT (EP0_IDX | USB_EP_DIR_OUT)

#define EP_IDX(ep) ((ep) & ~USB_EP_DIR_MASK)
#define EP_IS_IN(ep) (((ep) & USB_EP_DIR_MASK) == USB_EP_DIR_IN)
#define EP_IS_OUT(ep) (((ep) & USB_EP_DIR_MASK) == USB_EP_DIR_OUT)

/* Endpoint state */
struct usb_dc_stm32_ep_state {
	u16_t ep_mps;	/** Endpoint max packet size */
	u8_t ep_type;	/** Endpoint type (STM32 HAL enum) */
	usb_dc_ep_callback cb;	/** Endpoint callback function */
	u8_t ep_stalled;	/** Endpoint stall flag */
	u32_t read_count;	/** Number of bytes in read buffer  */
	u32_t read_offset;	/** Current offset in read buffer */
	struct k_sem write_sem;	/** Write boolean semaphore */
};

/* Driver state */
struct usb_dc_stm32_state {
	PCD_HandleTypeDef pcd;	/* Storage for the HAL_PCD api */
	usb_dc_status_callback status_cb; /* Status callback */
	struct usb_dc_stm32_ep_state out_ep_state[CONFIG_USB_DC_STM32_EP_NUM];
	struct usb_dc_stm32_ep_state in_ep_state[CONFIG_USB_DC_STM32_EP_NUM];
	u8_t ep_buf[CONFIG_USB_DC_STM32_EP_NUM][USB_OTG_FS_MAX_PACKET_SIZE];
};

static struct usb_dc_stm32_state usb_dc_stm32_state;

/* Internal functions */

static struct usb_dc_stm32_ep_state *usb_dc_stm32_get_ep_state(u8_t ep)
{
	struct usb_dc_stm32_ep_state *ep_state_base;

	if (EP_IDX(ep) >= CONFIG_USB_DC_STM32_EP_NUM) {
		return NULL;
	}

	if (EP_IS_OUT(ep)) {
		ep_state_base = usb_dc_stm32_state.out_ep_state;
	} else {
		ep_state_base = usb_dc_stm32_state.in_ep_state;
	}

	return ep_state_base + EP_IDX(ep);
}

static void usb_dc_stm32_isr(void *arg)
{
	HAL_PCD_IRQHandler(&usb_dc_stm32_state.pcd);
}

static int usb_dc_stm32_clock_enable(void)
{
	struct device *clk = device_get_binding(STM32_CLOCK_CONTROL_NAME);
	struct stm32_pclken pclken = {
		.bus = STM32_CLOCK_BUS_AHB2,
		.enr = LL_AHB2_GRP1_PERIPH_OTGFS,
	};

	clock_control_on(clk, (clock_control_subsys_t *)&pclken);

	return 0;
}

static int usb_dc_stm32_init(void)
{
	HAL_StatusTypeDef status;
	unsigned int i;

	/* We only support OTG FS for now */
	usb_dc_stm32_state.pcd.Instance = USB_OTG_FS;
	usb_dc_stm32_state.pcd.Init.dev_endpoints = CONFIG_USB_DC_STM32_EP_NUM;
	usb_dc_stm32_state.pcd.Init.speed = USB_OTG_SPEED_FULL;
	usb_dc_stm32_state.pcd.Init.phy_itface = PCD_PHY_EMBEDDED;
	usb_dc_stm32_state.pcd.Init.ep0_mps = USB_OTG_MAX_EP0_SIZE;
	usb_dc_stm32_state.pcd.Init.dma_enable = DISABLE;
	usb_dc_stm32_state.pcd.Init.vbus_sensing_enable = DISABLE;

	SYS_LOG_DBG("HAL_PCD_Init");
	status = HAL_PCD_Init(&usb_dc_stm32_state.pcd);
	if (status != HAL_OK) {
		SYS_LOG_ERR("PCD_Init failed, %d", (int)status);
		return -EIO;
	}

	SYS_LOG_DBG("HAL_PCD_Start");
	status = HAL_PCD_Start(&usb_dc_stm32_state.pcd);
	if (status != HAL_OK) {
		SYS_LOG_ERR("PCD_Start failed, %d", (int)status);
		return -EIO;
	}

	usb_dc_stm32_state.out_ep_state[EP0_IDX].ep_mps = USB_OTG_MAX_EP0_SIZE;
	usb_dc_stm32_state.out_ep_state[EP0_IDX].ep_type = EP_TYPE_CTRL;
	usb_dc_stm32_state.in_ep_state[EP0_IDX].ep_mps = USB_OTG_MAX_EP0_SIZE;
	usb_dc_stm32_state.in_ep_state[EP0_IDX].ep_type = EP_TYPE_CTRL;

	/* TODO: make this dynamic (depending usage) */
	HAL_PCDEx_SetRxFiFo(&usb_dc_stm32_state.pcd, FIFO_EP_WORDS);
	for (i = 0; i < CONFIG_USB_DC_STM32_EP_NUM; i++) {
		HAL_PCDEx_SetTxFiFo(&usb_dc_stm32_state.pcd, i,
				    FIFO_EP_WORDS);
		k_sem_init(&usb_dc_stm32_state.in_ep_state[i].write_sem, 1, 1);
	}

	IRQ_CONNECT(STM32F4_IRQ_OTG_FS, CONFIG_USB_DC_STM32_IRQ_PRI,
		    usb_dc_stm32_isr, 0, 0);
	irq_enable(STM32F4_IRQ_OTG_FS);

	return 0;
}

/* Zephyr USB device controller API implementation */

int usb_dc_attach(void)
{
	int ret;

	SYS_LOG_DBG("");

	ret = usb_dc_stm32_clock_enable();
	if (ret) {
		return ret;
	}

	ret = usb_dc_stm32_init();
	if (ret) {
		return ret;
	}

	return 0;
}

int usb_dc_ep_set_callback(const u8_t ep, const usb_dc_ep_callback cb)
{
	struct usb_dc_stm32_ep_state *ep_state = usb_dc_stm32_get_ep_state(ep);

	SYS_LOG_DBG("ep 0x%02x", ep);

	if (!ep_state) {
		return -EINVAL;
	}

	ep_state->cb = cb;

	return 0;
}

int usb_dc_set_status_callback(const usb_dc_status_callback cb)
{
	SYS_LOG_DBG("");

	usb_dc_stm32_state.status_cb = cb;

	return 0;
}

int usb_dc_set_address(const u8_t addr)
{
	HAL_StatusTypeDef status;

	SYS_LOG_DBG("addr %u (0x%02x)", addr, addr);

	status = HAL_PCD_SetAddress(&usb_dc_stm32_state.pcd, addr);
	if (status != HAL_OK) {
		SYS_LOG_ERR("HAL_PCD_SetAddress failed(0x%02x), %d",
			    addr, (int)status);
		return -EIO;
	}

	return 0;
}

int usb_dc_ep_start_read(u8_t ep, u8_t *data, u32_t max_data_len)
{
	HAL_StatusTypeDef status;

	SYS_LOG_DBG("ep 0x%02x, len %u", ep, max_data_len);

	/* we flush EP0_IN by doing a 0 length receive on it */
	if (!EP_IS_OUT(ep) && (ep != EP0_IN || max_data_len)) {
		SYS_LOG_ERR("invalid ep 0x%02x", ep);
		return -EINVAL;
	}

	if (max_data_len > USB_OTG_FS_MAX_PACKET_SIZE) {
		max_data_len = USB_OTG_FS_MAX_PACKET_SIZE;
	}

	status = HAL_PCD_EP_Receive(&usb_dc_stm32_state.pcd, ep,
				    usb_dc_stm32_state.ep_buf[EP_IDX(ep)],
				    max_data_len);
	if (status != HAL_OK) {
		SYS_LOG_ERR("HAL_PCD_EP_Receive failed(0x%02x), %d",
			    ep, (int)status);
		return -EIO;
	}

	return 0;
}

int usb_dc_ep_get_read_count(u8_t ep, u32_t *read_bytes)
{
	if (!EP_IS_OUT(ep)) {
		SYS_LOG_ERR("invalid ep 0x%02x", ep);
		return -EINVAL;
	}

	*read_bytes = HAL_PCD_EP_GetRxCount(&usb_dc_stm32_state.pcd, ep);

	return 0;
}

int usb_dc_ep_configure(const struct usb_dc_ep_cfg_data * const ep_cfg)
{
	u8_t ep = ep_cfg->ep_addr;
	struct usb_dc_stm32_ep_state *ep_state = usb_dc_stm32_get_ep_state(ep);

	SYS_LOG_DBG("ep 0x%02x, ep_mps %u, ep_type %u",
		    ep_cfg->ep_addr, ep_cfg->ep_mps, ep_cfg->ep_type);

	if (!ep_state) {
		return -EINVAL;
	}

	ep_state->ep_mps = ep_cfg->ep_mps;

	switch (ep_cfg->ep_type) {
	case USB_DC_EP_CONTROL:
		ep_state->ep_type = EP_TYPE_CTRL;
		break;
	case USB_DC_EP_ISOCHRONOUS:
		ep_state->ep_type = EP_TYPE_ISOC;
		break;
	case USB_DC_EP_BULK:
		ep_state->ep_type = EP_TYPE_BULK;
		break;
	case USB_DC_EP_INTERRUPT:
		ep_state->ep_type = EP_TYPE_INTR;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

int usb_dc_ep_set_stall(const u8_t ep)
{
	struct usb_dc_stm32_ep_state *ep_state = usb_dc_stm32_get_ep_state(ep);
	HAL_StatusTypeDef status;

	SYS_LOG_DBG("ep 0x%02x", ep);

	if (!ep_state) {
		return -EINVAL;
	}

	status = HAL_PCD_EP_SetStall(&usb_dc_stm32_state.pcd, ep);
	if (status != HAL_OK) {
		SYS_LOG_ERR("HAL_PCD_EP_SetStall failed(0x%02x), %d",
			    ep, (int)status);
		return -EIO;
	}

	ep_state->ep_stalled = 1;

	return 0;
}

int usb_dc_ep_clear_stall(const u8_t ep)
{
	struct usb_dc_stm32_ep_state *ep_state = usb_dc_stm32_get_ep_state(ep);
	HAL_StatusTypeDef status;

	SYS_LOG_DBG("ep 0x%02x", ep);

	if (!ep_state) {
		return -EINVAL;
	}

	status = HAL_PCD_EP_ClrStall(&usb_dc_stm32_state.pcd, ep);
	if (status != HAL_OK) {
		SYS_LOG_ERR("HAL_PCD_EP_ClrStall failed(0x%02x), %d",
			    ep, (int)status);
		return -EIO;
	}

	ep_state->ep_stalled = 0;
	ep_state->read_count = 0;

	return 0;
}

int usb_dc_ep_is_stalled(const u8_t ep, u8_t *const stalled)
{
	struct usb_dc_stm32_ep_state *ep_state = usb_dc_stm32_get_ep_state(ep);

	SYS_LOG_DBG("ep 0x%02x", ep);

	if (!ep_state) {
		return -EINVAL;
	}

	*stalled = ep_state->ep_stalled;

	return 0;
}

int usb_dc_ep_enable(const u8_t ep)
{
	struct usb_dc_stm32_ep_state *ep_state = usb_dc_stm32_get_ep_state(ep);
	HAL_StatusTypeDef status;
	int ret;

	SYS_LOG_DBG("ep 0x%02x", ep);

	if (!ep_state) {
		return -EINVAL;
	}

	SYS_LOG_DBG("HAL_PCD_EP_Open(0x%02x, %u, %u)", ep,
		    ep_state->ep_mps, ep_state->ep_type);

	status = HAL_PCD_EP_Open(&usb_dc_stm32_state.pcd, ep,
				 ep_state->ep_mps, ep_state->ep_type);
	if (status != HAL_OK) {
		SYS_LOG_ERR("HAL_PCD_EP_Open failed(0x%02x), %d",
			    ep, (int)status);
		return -EIO;
	}

	ret = usb_dc_ep_clear_stall(ep);
	if (ret) {
		return ret;
	}

	if (EP_IS_OUT(ep) && ep != EP0_OUT) {
		return usb_dc_ep_start_read(ep,
					   usb_dc_stm32_state.ep_buf[EP_IDX(ep)],
					   USB_OTG_FS_MAX_PACKET_SIZE);
	}

	return 0;
}

int usb_dc_ep_disable(const u8_t ep)
{
	struct usb_dc_stm32_ep_state *ep_state = usb_dc_stm32_get_ep_state(ep);
	HAL_StatusTypeDef status;

	SYS_LOG_DBG("ep 0x%02x", ep);

	if (!ep_state) {
		return -EINVAL;
	}

	status = HAL_PCD_EP_Close(&usb_dc_stm32_state.pcd, ep);
	if (status != HAL_OK) {
		SYS_LOG_ERR("HAL_PCD_EP_Close failed(0x%02x), %d",
			    ep, (int)status);
		return -EIO;
	}

	return 0;
}

int usb_dc_ep_write(const u8_t ep, const u8_t *const data,
		    const u32_t data_len, u32_t * const ret_bytes)
{
	struct usb_dc_stm32_ep_state *ep_state = usb_dc_stm32_get_ep_state(ep);
	HAL_StatusTypeDef status;
	int ret = 0;

	SYS_LOG_DBG("ep 0x%02x, len %u", ep, data_len);

	if (!EP_IS_IN(ep)) {
		SYS_LOG_ERR("invalid ep 0x%02x", ep);
		return -EINVAL;
	}

	ret = k_sem_take(&ep_state->write_sem, 1000);
	if (ret) {
		SYS_LOG_ERR("Unable to write ep 0x%02x (%d)", ep, ret);
		return ret;
	}

	if (!k_is_in_isr()) {
		irq_disable(STM32F4_IRQ_OTG_FS);
	}

	status = HAL_PCD_EP_Transmit(&usb_dc_stm32_state.pcd, ep,
				     (void *)data, data_len);
	if (status != HAL_OK) {
		SYS_LOG_ERR("HAL_PCD_EP_Transmit failed(0x%02x), %d",
			    ep, (int)status);
		k_sem_give(&ep_state->write_sem);
		ret = -EIO;
	}

	if (!ret && ep == EP0_IN) {
		/* Wait for an empty package as from the host.
		 * This also flushes the TX FIFO to the host.
		 */
		usb_dc_ep_start_read(ep, NULL, 0);
	}

	if (!k_is_in_isr()) {
		irq_enable(STM32F4_IRQ_OTG_FS);
	}

	if (ret_bytes) {
		*ret_bytes = data_len;
	}

	return ret;
}

int usb_dc_ep_read(const u8_t ep, u8_t *const data, const u32_t max_data_len,
		   u32_t * const read_bytes)
{
	struct usb_dc_stm32_ep_state *ep_state = usb_dc_stm32_get_ep_state(ep);
	u32_t read_count = ep_state->read_count;

	SYS_LOG_DBG("ep 0x%02x, %u bytes, %u+%u, %p", ep,
		    max_data_len, ep_state->read_offset, read_count, data);

	if (!max_data_len) {
		goto done;
	}

	read_count = min(read_count, max_data_len);

	/* Read data previously stored in the buffer */
	if (read_count) {
		memcpy(data, usb_dc_stm32_state.ep_buf[EP_IDX(ep)] +
		       ep_state->read_offset, read_count);
		ep_state->read_count -= read_count;
		ep_state->read_offset += read_count;
	}

	/* If no more data in the buffer, start a new read transaction.
	 * DataOutStageCallback will called on transaction complete.
	 */
	if (ep != EP0_OUT && !ep_state->read_count) {
		usb_dc_ep_start_read(ep, usb_dc_stm32_state.ep_buf[EP_IDX(ep)],
				     USB_OTG_FS_MAX_PACKET_SIZE);
	}

done:
	if (read_bytes) {
		*read_bytes = read_count;
	}

	return 0;
}

/* Callbacks from the STM32 Cube HAL code */

void HAL_PCD_ResetCallback(PCD_HandleTypeDef *hpcd)
{
	SYS_LOG_DBG("");

	if (usb_dc_stm32_state.status_cb) {
		usb_dc_stm32_state.status_cb(USB_DC_RESET, NULL);
	}
}

void HAL_PCD_ConnectCallback(PCD_HandleTypeDef *hpcd)
{
	SYS_LOG_DBG("");

	if (usb_dc_stm32_state.status_cb) {
		usb_dc_stm32_state.status_cb(USB_DC_CONNECTED, NULL);
	}
}

void HAL_PCD_DisconnectCallback(PCD_HandleTypeDef *hpcd)
{
	SYS_LOG_DBG("");

	if (usb_dc_stm32_state.status_cb) {
		usb_dc_stm32_state.status_cb(USB_DC_DISCONNECTED, NULL);
	}
}

void HAL_PCD_SuspendCallback(PCD_HandleTypeDef *hpcd)
{
	SYS_LOG_DBG("");

	if (usb_dc_stm32_state.status_cb) {
		usb_dc_stm32_state.status_cb(USB_DC_SUSPEND, NULL);
	}
}

void HAL_PCD_ResumeCallback(PCD_HandleTypeDef *hpcd)
{
	SYS_LOG_DBG("");

	if (usb_dc_stm32_state.status_cb) {
		usb_dc_stm32_state.status_cb(USB_DC_RESUME, NULL);
	}
}

void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd)
{
	struct usb_setup_packet *setup = (void *)usb_dc_stm32_state.pcd.Setup;
	struct usb_dc_stm32_ep_state *ep_state;

	SYS_LOG_DBG("");

	ep_state = usb_dc_stm32_get_ep_state(EP0_OUT); /* can't fail for ep0 */
	ep_state->read_count = SETUP_SIZE;
	ep_state->read_offset = 0;
	memcpy(&usb_dc_stm32_state.ep_buf[EP0_IDX],
	       usb_dc_stm32_state.pcd.Setup, ep_state->read_count);

	if (ep_state->cb) {
		ep_state->cb(EP0_OUT, USB_DC_EP_SETUP);

		if (!(setup->wLength == 0) &&
		    !(REQTYPE_GET_DIR(setup->bmRequestType) ==
		    REQTYPE_DIR_TO_HOST)) {
			usb_dc_ep_start_read(EP0_OUT,
					     usb_dc_stm32_state.ep_buf[EP0_IDX],
					     setup->wLength);
		}
	}
}

void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *hpcd, u8_t epnum)
{
	u8_t ep_idx = EP_IDX(epnum);
	u8_t ep = ep_idx | USB_EP_DIR_OUT;
	struct usb_dc_stm32_ep_state *ep_state = usb_dc_stm32_get_ep_state(ep);

	SYS_LOG_DBG("epnum 0x%02x, rx_count %u", epnum,
		    HAL_PCD_EP_GetRxCount(&usb_dc_stm32_state.pcd, epnum));

	/* Transaction complete, data is now stored in the buffer and ready
	 * for the upper stack (usb_dc_ep_read to retrieve).
	 */
	usb_dc_ep_get_read_count(ep, &ep_state->read_count);
	ep_state->read_offset = 0;

	if (ep_state->cb) {
		ep_state->cb(ep, USB_DC_EP_DATA_OUT);
	}
}

void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *hpcd, u8_t epnum)
{
	u8_t ep_idx = EP_IDX(epnum);
	u8_t ep = ep_idx | USB_EP_DIR_IN;
	struct usb_dc_stm32_ep_state *ep_state = usb_dc_stm32_get_ep_state(ep);

	SYS_LOG_DBG("epnum 0x%02x", epnum);

	k_sem_give(&ep_state->write_sem);

	if (ep_state->cb) {
		ep_state->cb(ep, USB_DC_EP_DATA_IN);
	}
}

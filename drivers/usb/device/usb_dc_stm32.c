/*
 * Copyright (c) 2017 Christer Weinigel
 * Copyright (c) 2017, I-SENSE group of ICCS
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * USB device controller driver for STM32 devices
 *
 */

/**
 * @file
 * @brief USB device controller driver for STM32 devices
 *
 * This driver uses the STM Cube low level drivers to talk to the USB
 * device controller on the STM32 family of devices using the
 * STM32Cube HAL layer.
 *
 * There is a bit of an impedance mismatch between the Zephyr
 * usb_device and the STM32 Cube HAL layer where higher levels make
 * assumptions about the low level drivers that don't quite match how
 * the low level drivers actuall work.
 *
 * The usb_dc_ep_read function expects to get the data it wants
 * immediately while the HAL_PCD_EP_Receive function only starts a
 * read transaction and the data won't be available
 * HAL_PCD_DataOutStageCallback is called.  To work around this I've
 * had to add an extra packet buffer in the driver which wastes memory
 * and also leads to an extra copy of all received data.  It would be
 * better if higher drivers could call start_read and get_read_count
 * in this driver directly.
 *
 * There is also a bit too much knowlege about the low level driver in
 * higher layers, for example the cdc_acm driver knows that it has to
 * read 4 bytes at a time from the Quark FIFO which is no longer true
 * with the extra buffer in this driver.
 *
 * Note, the STM32F4xx series of MCUs seem to use a Synopsys
 * DesignWare controller which is very similar to the one supported by
 * usb_dc_dw.c so it might be possible get USB support if that driver
 * is modified a bit.  On the other hand, using the STM32 Cube drivers
 * means that this driver ought to work on STM3xxx and maybe also on
 * some Renesas processors which use a similar HAL.
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
#include <gpio.h>
#include <usb/usb_dc.h>
#include <usb/usb_device.h>
#include <clock_control/stm32_clock_control.h>

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_USB_DRIVER_LEVEL
#include <logging/sys_log.h>

/*
 * USB LL API provides the EP_TYPE_* defines
 * STM32Cube does not provide USB LL API for STM32F3 family.
 * Map EP_TYPE_* defines to PCD_EP_TYPE_* defines
 */
#ifdef CONFIG_SOC_SERIES_STM32F3X
#define EP_TYPE_CTRL PCD_EP_TYPE_CTRL
#define EP_TYPE_ISOC PCD_EP_TYPE_ISOC
#define EP_TYPE_BULK PCD_EP_TYPE_BULK
#define EP_TYPE_INTR PCD_EP_TYPE_INTR
#endif /* CONFIG_SOC_SERIES_STM32F3X */

/* Miscellaneous values used in the driver */

#ifdef USB

#define USB_DC_STM32_IRQ USB_LP_IRQn

#define EP0_MPS 64U
#define EP_MPS 64U

/*
 * USB BTABLE is stored in the PMA. The size of BTABLE is 8 bytes per
 * per endpoint.
 *
 */
#define USB_BTABLE_SIZE  (8 * CONFIG_USB_DC_STM32_EP_NUM)

#else /* USB_OTG_FS */

#define USB_DC_STM32_IRQ OTG_FS_IRQn

#define EP0_MPS USB_OTG_MAX_EP0_SIZE
#define EP_MPS USB_OTG_FS_MAX_PACKET_SIZE

/* OTG FS has 1 RX_FIFO and n TX_FIFOS (one for each IN EP)*/
#define FIFO_NUM (CONFIG_USB_DC_STM32_EP_NUM + 1)

/* Divide the packet memory evenly between the RX/TX FIFOs.
 *
 * E.g. for the STM32F405 with 1280 bytes of RAM, 1 RX FIFO and 4 TX
 * FIFOs this will allocate 1280 / 5 = 256 bytes for each FIFO.
 */
#define FIFO_SIZE_IN_WORDS \
		((CONFIG_USB_DC_STM32_PACKET_RAM_SIZE / 4) / FIFO_NUM)

#endif /* USB */

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
};

/* Driver state */

struct usb_dc_stm32_state {
	PCD_HandleTypeDef pcd;	/* Storage for the HAL_PCD api */
	usb_dc_status_callback status_cb; /* Status callback */
	struct usb_dc_stm32_ep_state out_ep_state[CONFIG_USB_DC_STM32_EP_NUM];
	struct usb_dc_stm32_ep_state in_ep_state[CONFIG_USB_DC_STM32_EP_NUM];
	u8_t ep_buf[CONFIG_USB_DC_STM32_EP_NUM][EP_MPS];

#ifdef USB
	u32_t pma_address;
#endif /* USB */

#ifdef CONFIG_USB_DC_STM32_DISCONN_ENABLE
	struct device *usb_disconnect;
#endif /* CONFIG_USB_DC_STM32_DISCONN_ENABLE */
};

static struct usb_dc_stm32_state usb_dc_state;

/* Internal functions */

static struct usb_dc_stm32_ep_state *usb_dc_stm32_get_ep_state(u8_t ep)
{
	struct usb_dc_stm32_ep_state *ep_state_base;

	if (EP_IDX(ep) >= CONFIG_USB_DC_STM32_EP_NUM) {
		return NULL;
	}

	if (EP_IS_OUT(ep)) {
		ep_state_base = usb_dc_state.out_ep_state;
	} else {
		ep_state_base = usb_dc_state.in_ep_state;
	}

	return ep_state_base + EP_IDX(ep);
}

static void usb_dc_stm32_isr(void *arg)
{
	HAL_PCD_IRQHandler(&usb_dc_state.pcd);
}

static int usb_dc_stm32_clock_enable(void)
{
	struct device *clk = device_get_binding(STM32_CLOCK_CONTROL_NAME);
	struct stm32_pclken pclken = {

#ifdef USB
		.bus = STM32_CLOCK_BUS_APB1,
		.enr = LL_APB1_GRP1_PERIPH_USB,
#else /* USB_OTG_FS */

#ifdef CONFIG_SOC_SERIES_STM32F1X
		.bus = STM32_CLOCK_BUS_AHB1,
		.enr = LL_AHB1_GRP1_PERIPH_OTGFS,
#else
		.bus = STM32_CLOCK_BUS_AHB2,
		.enr = LL_AHB2_GRP1_PERIPH_OTGFS,
#endif /* CONFIG_SOC_SERIES_STM32F1X */

#endif /* USB */
	};

	clock_control_on(clk, (clock_control_subsys_t *)&pclken);

	return 0;
}

#ifdef USB
static int usb_dc_stm32_init(void)
{
	HAL_StatusTypeDef status;

	usb_dc_state.pcd.Instance = USB;
	usb_dc_state.pcd.Init.speed = PCD_SPEED_FULL;
	usb_dc_state.pcd.Init.dev_endpoints = CONFIG_USB_DC_STM32_EP_NUM;
	usb_dc_state.pcd.Init.phy_itface = PCD_PHY_EMBEDDED;
	usb_dc_state.pcd.Init.ep0_mps = PCD_EP0MPS_64;
	usb_dc_state.pcd.Init.low_power_enable = 0;

	/* Start PMA configuration for the endpoints after the BTABLE. */
	usb_dc_state.pma_address = USB_BTABLE_SIZE;

	SYS_LOG_DBG("HAL_PCD_Init");
	status = HAL_PCD_Init(&usb_dc_state.pcd);
	if (status != HAL_OK) {
		SYS_LOG_ERR("PCD_Init failed, %d", (int)status);
		return -EIO;
	}

	SYS_LOG_DBG("HAL_PCD_Start");
	status = HAL_PCD_Start(&usb_dc_state.pcd);
	if (status != HAL_OK) {
		SYS_LOG_ERR("PCD_Start failed, %d", (int)status);
		return -EIO;
	}

	usb_dc_state.out_ep_state[EP0_IDX].ep_mps = EP0_MPS;
	usb_dc_state.out_ep_state[EP0_IDX].ep_type = EP_TYPE_CTRL;
	usb_dc_state.in_ep_state[EP0_IDX].ep_mps = EP0_MPS;
	usb_dc_state.in_ep_state[EP0_IDX].ep_type = EP_TYPE_CTRL;

#ifdef CONFIG_SOC_SERIES_STM32F3X
	LL_SYSCFG_EnableRemapIT_USB();
#endif

	IRQ_CONNECT(USB_DC_STM32_IRQ, CONFIG_USB_DC_STM32_IRQ_PRI,
		    usb_dc_stm32_isr, 0, 0);
	irq_enable(USB_DC_STM32_IRQ);
	return 0;
}
#else /* USB_OTG_FS */
static int usb_dc_stm32_init(void)
{
	HAL_StatusTypeDef status;

	usb_dc_state.pcd.Instance = USB_OTG_FS;
	usb_dc_state.pcd.Init.speed = USB_OTG_SPEED_FULL;
	usb_dc_state.pcd.Init.dev_endpoints = CONFIG_USB_DC_STM32_EP_NUM;
	usb_dc_state.pcd.Init.phy_itface = PCD_PHY_EMBEDDED;
	usb_dc_state.pcd.Init.ep0_mps = EP0_MPS;

#if defined(CONFIG_SOC_SERIES_STM32F4X) || \
	defined(CONFIG_SOC_SERIES_STM32F7X) || \
	defined(CONFIG_SOC_SERIES_STM32L4X)
	usb_dc_state.pcd.Init.dma_enable = DISABLE;
#endif

#ifdef CONFIG_USB_DC_STM32_VBUS_SENSING
	usb_dc_state.pcd.Init.vbus_sensing_enable = ENABLE;
#else
	usb_dc_state.pcd.Init.vbus_sensing_enable = DISABLE;
#endif /* CONFIG_USB_DC_STM32_VBUS_SENSING */

	SYS_LOG_DBG("HAL_PCD_Init");
	status = HAL_PCD_Init(&usb_dc_state.pcd);
	if (status != HAL_OK) {
		SYS_LOG_ERR("PCD_Init failed, %d", (int)status);
		return -EIO;
	}

	SYS_LOG_DBG("HAL_PCD_Start");
	status = HAL_PCD_Start(&usb_dc_state.pcd);
	if (status != HAL_OK) {
		SYS_LOG_ERR("PCD_Start failed, %d", (int)status);
		return -EIO;
	}

	usb_dc_state.out_ep_state[EP0_IDX].ep_mps = EP0_MPS;
	usb_dc_state.out_ep_state[EP0_IDX].ep_type = EP_TYPE_CTRL;
	usb_dc_state.in_ep_state[EP0_IDX].ep_mps = EP0_MPS;
	usb_dc_state.in_ep_state[EP0_IDX].ep_type = EP_TYPE_CTRL;

	HAL_PCDEx_SetRxFiFo(&usb_dc_state.pcd, FIFO_SIZE_IN_WORDS);
	for (u32_t i = 0; i < FIFO_NUM - 1; i++) {
		HAL_PCDEx_SetTxFiFo(&usb_dc_state.pcd, i,
				    FIFO_SIZE_IN_WORDS);
	}

	IRQ_CONNECT(USB_DC_STM32_IRQ, CONFIG_USB_DC_STM32_IRQ_PRI,
		    usb_dc_stm32_isr, 0, 0);
	irq_enable(USB_DC_STM32_IRQ);
	return 0;
}
#endif

/* Zephyr USB device controller API implementation */

int usb_dc_attach(void)
{
	int ret;

	SYS_LOG_DBG("");

#ifdef CONFIG_USB_DC_STM32_DISCONN_ENABLE
	usb_dc_state.usb_disconnect =
		device_get_binding(CONFIG_USB_DC_STM32_DISCONN_GPIO_PORT_NAME);
	gpio_pin_configure(usb_dc_state.usb_disconnect,
			   CONFIG_USB_DC_STM32_DISCONN_PIN, GPIO_DIR_OUT);
	gpio_pin_write(usb_dc_state.usb_disconnect,
		       CONFIG_USB_DC_STM32_DISCONN_PIN,
		       CONFIG_USB_DC_STM32_DISCONN_PIN_LEVEL);
#endif /* CONFIG_USB_DC_STM32_DISCONN_ENABLE */

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

	usb_dc_state.status_cb = cb;

	return 0;
}

int usb_dc_set_address(const u8_t addr)
{
	HAL_StatusTypeDef status;

	SYS_LOG_DBG("addr %u (0x%02x)", addr, addr);

	status = HAL_PCD_SetAddress(&usb_dc_state.pcd, addr);
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

	if (max_data_len > EP_MPS) {
		max_data_len = EP_MPS;
	}

	status = HAL_PCD_EP_Receive(&usb_dc_state.pcd, ep,
				    usb_dc_state.ep_buf[EP_IDX(ep)],
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

	*read_bytes = HAL_PCD_EP_GetRxCount(&usb_dc_state.pcd, ep);

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

#ifdef USB
	if (CONFIG_USB_DC_STM32_PACKET_RAM_SIZE <=
	    (usb_dc_state.pma_address + ep_cfg->ep_mps)) {
		return -EINVAL;
	}
	HAL_PCDEx_PMAConfig(&usb_dc_state.pcd, ep, PCD_SNG_BUF,
			    usb_dc_state.pma_address);
	usb_dc_state.pma_address += ep_cfg->ep_mps;
#endif
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

	status = HAL_PCD_EP_SetStall(&usb_dc_state.pcd, ep);
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

	status = HAL_PCD_EP_ClrStall(&usb_dc_state.pcd, ep);
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

	status = HAL_PCD_EP_Open(&usb_dc_state.pcd, ep,
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
		ret = usb_dc_ep_start_read(ep,
					   usb_dc_state.ep_buf[EP_IDX(ep)],
					   EP_MPS);
		if (ret) {
			return ret;
		}
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

	status = HAL_PCD_EP_Close(&usb_dc_state.pcd, ep);
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
	HAL_StatusTypeDef status;
	int ret = 0;

	SYS_LOG_DBG("ep 0x%02x, len %u", ep, data_len);

	if (!EP_IS_IN(ep)) {
		SYS_LOG_ERR("invalid ep 0x%02x", ep);
		return -EINVAL;
	}

	if (!k_is_in_isr()) {
		irq_disable(USB_DC_STM32_IRQ);
	}

	status = HAL_PCD_EP_Transmit(&usb_dc_state.pcd, ep,
				     (void *)data, data_len);
	if (status != HAL_OK) {
		SYS_LOG_ERR("HAL_PCD_EP_Transmit failed(0x%02x), %d",
			    ep, (int)status);
		ret = -EIO;
	}
	if (!ret && ep == EP0_IN) {
		/* Wait for an empty package as from the host.
		 * This also flushes the TX FIFO to the host.
		 */
		usb_dc_ep_start_read(ep, NULL, 0);
	}

	if (!k_is_in_isr()) {
		irq_enable(USB_DC_STM32_IRQ);
	}

	*ret_bytes = data_len;

	return ret;
}

int usb_dc_ep_read(const u8_t ep, u8_t *const data,
		   const u32_t max_data_len, u32_t * const read_bytes)
{
	struct usb_dc_stm32_ep_state *ep_state = usb_dc_stm32_get_ep_state(ep);
	u32_t read_count = ep_state->read_count;

	SYS_LOG_DBG("ep 0x%02x, %u bytes, %u+%u, %p", ep,
		    max_data_len, ep_state->read_offset, read_count, data);

	if (max_data_len) {
		if (read_count > max_data_len) {
			read_count = max_data_len;
		}

		if (read_count) {
			memcpy(data,
			       usb_dc_state.ep_buf[EP_IDX(ep)] +
			       ep_state->read_offset, read_count);
			ep_state->read_count -= read_count;
			ep_state->read_offset += read_count;
		}

		if (ep != EP0_OUT && !ep_state->read_count) {
			usb_dc_ep_start_read(ep,
					     usb_dc_state.ep_buf[EP_IDX(ep)],
					     EP_MPS);
		}
	}

	if (read_bytes) {
		*read_bytes = read_count;
	}

	return 0;
}

/* Callbacks from the STM32 Cube HAL code */

void HAL_PCD_ResetCallback(PCD_HandleTypeDef *hpcd)
{
	SYS_LOG_DBG("");

#ifdef USB
	usb_dc_ep_enable(EP0_OUT);
	usb_dc_ep_enable(EP0_IN);
#endif /* USB */

	if (usb_dc_state.status_cb) {
		usb_dc_state.status_cb(USB_DC_RESET, NULL);
	}
}

void HAL_PCD_ConnectCallback(PCD_HandleTypeDef *hpcd)
{
	SYS_LOG_DBG("");

	if (usb_dc_state.status_cb) {
		usb_dc_state.status_cb(USB_DC_CONNECTED, NULL);
	}
}

void HAL_PCD_DisconnectCallback(PCD_HandleTypeDef *hpcd)
{
	SYS_LOG_DBG("");

	if (usb_dc_state.status_cb) {
		usb_dc_state.status_cb(USB_DC_DISCONNECTED, NULL);
	}
}

void HAL_PCD_SuspendCallback(PCD_HandleTypeDef *hpcd)
{
	SYS_LOG_DBG("");

	if (usb_dc_state.status_cb) {
		usb_dc_state.status_cb(USB_DC_SUSPEND, NULL);
	}
}

void HAL_PCD_ResumeCallback(PCD_HandleTypeDef *hpcd)
{
	SYS_LOG_DBG("");

	if (usb_dc_state.status_cb) {
		usb_dc_state.status_cb(USB_DC_RESUME, NULL);
	}
}

void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd)
{
	struct usb_dc_stm32_ep_state *ep_state =
					usb_dc_stm32_get_ep_state(EP0_OUT);
	struct usb_setup_packet *setup = (void *)usb_dc_state.pcd.Setup;

	SYS_LOG_DBG("");

	ep_state->read_count = SETUP_SIZE;
	ep_state->read_offset = 0;
	memcpy(&usb_dc_state.ep_buf[EP0_IDX], usb_dc_state.pcd.Setup,
	       ep_state->read_count);

	if (ep_state->cb) {
		ep_state->cb(EP0_OUT, USB_DC_EP_SETUP);

		if (!(setup->wLength == 0) &&
		    !(REQTYPE_GET_DIR(setup->bmRequestType) ==
		    REQTYPE_DIR_TO_HOST)) {
			usb_dc_ep_start_read(EP0_OUT,
					     usb_dc_state.ep_buf[EP0_IDX],
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
		    HAL_PCD_EP_GetRxCount(&usb_dc_state.pcd, epnum));

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

	if (ep_state->cb) {
		ep_state->cb(ep, USB_DC_EP_DATA_IN);
	}
}

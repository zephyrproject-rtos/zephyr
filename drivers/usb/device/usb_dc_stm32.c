/*
 * Copyright (c) 2017 Christer Weinigel.
 * Copyright (c) 2017, I-SENSE group of ICCS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB device controller shim driver for STM32 devices
 *
 * This driver uses the STM32 Cube low level drivers to talk to the USB
 * device controller on the STM32 family of devices using the
 * STM32Cube HAL layer.
 */

#include <soc.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_system.h>
#include <string.h>
#include <usb/usb_device.h>
#include <drivers/clock_control/stm32_clock_control.h>
#include <sys/util.h>
#include <drivers/gpio.h>
#include <pinmux/pinmux_stm32.h>
#include "stm32_hsem.h"

#define LOG_LEVEL CONFIG_USB_DRIVER_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(usb_dc_stm32);

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otgfs) && DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otghs)
#error "Only one interface should be enabled at a time, OTG FS or OTG HS"
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otghs)
#define DT_DRV_COMPAT st_stm32_otghs
#define USB_IRQ_NAME  otghs
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otgfs)
#define DT_DRV_COMPAT st_stm32_otgfs
#define USB_IRQ_NAME  otgfs
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_usb)
#define DT_DRV_COMPAT st_stm32_usb
#define USB_IRQ_NAME  usb
#if DT_INST_NODE_HAS_PROP(0, enable_pin_remap)
#define USB_ENABLE_PIN_REMAP	DT_INST_PROP(0, enable_pin_remap)
#endif
#endif

#define USB_BASE_ADDRESS	DT_INST_REG_ADDR(0)
#define USB_IRQ			DT_INST_IRQ_BY_NAME(0, USB_IRQ_NAME, irq)
#define USB_IRQ_PRI		DT_INST_IRQ_BY_NAME(0, USB_IRQ_NAME, priority)
#define USB_NUM_BIDIR_ENDPOINTS	DT_INST_PROP(0, num_bidir_endpoints)
#define USB_RAM_SIZE		DT_INST_PROP(0, ram_size)
#define USB_CLOCK_BITS		DT_INST_CLOCKS_CELL(0, bits)
#define USB_CLOCK_BUS		DT_INST_CLOCKS_CELL(0, bus)
#if DT_INST_NODE_HAS_PROP(0, maximum_speed)
#define USB_MAXIMUM_SPEED	DT_INST_PROP(0, maximum_speed)
#endif
static const struct soc_gpio_pinctrl usb_pinctrl[] =
						ST_STM32_DT_INST_PINCTRL(0, 0);


#define USB_OTG_HS_EMB_PHY (DT_HAS_COMPAT_STATUS_OKAY(st_stm32_usbphyc) && \
			    DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otghs))

/*
 * USB and USB_OTG_FS are defined in STM32Cube HAL and allows to distinguish
 * between two kind of USB DC. STM32 F0, F3, L0 and G4 series support USB device
 * controller. STM32 F4 and F7 series support USB_OTG_FS device controller.
 * STM32 F1 and L4 series support either USB or USB_OTG_FS device controller.
 *
 * WARNING: Don't mix USB defined in STM32Cube HAL and CONFIG_USB from Zephyr
 * Kconfig system.
 */
#ifdef USB

#define EP0_MPS 64U
#define EP_MPS 64U

/*
 * USB BTABLE is stored in the PMA. The size of BTABLE is 4 bytes
 * per endpoint.
 *
 */
#define USB_BTABLE_SIZE  (8 * USB_NUM_BIDIR_ENDPOINTS)

#else /* USB_OTG_FS */

/*
 * STM32L4 series USB LL API doesn't provide HIGH and HIGH_IN_FULL speed
 * defines.
 */
#if defined(CONFIG_SOC_SERIES_STM32L4X)
#define USB_OTG_SPEED_HIGH                     0U
#define USB_OTG_SPEED_HIGH_IN_FULL             1U
#endif /* CONFIG_SOC_SERIES_STM32L4X */

#define EP0_MPS USB_OTG_MAX_EP0_SIZE

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otghs)
#define EP_MPS USB_OTG_HS_MAX_PACKET_SIZE
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otgfs) || DT_HAS_COMPAT_STATUS_OKAY(st_stm32_usb)
#define EP_MPS USB_OTG_FS_MAX_PACKET_SIZE
#endif

/* We need one RX FIFO and n TX-IN FIFOs */
#define FIFO_NUM (1 + USB_NUM_BIDIR_ENDPOINTS)

/* 4-byte words FIFO */
#define FIFO_WORDS (USB_RAM_SIZE / 4)

/* Allocate FIFO memory evenly between the FIFOs */
#define FIFO_EP_WORDS (FIFO_WORDS / FIFO_NUM)

#endif /* USB */

/* Size of a USB SETUP packet */
#define SETUP_SIZE 8

/* Helper macros to make it easier to work with endpoint numbers */
#define EP0_IDX 0
#define EP0_IN (EP0_IDX | USB_EP_DIR_IN)
#define EP0_OUT (EP0_IDX | USB_EP_DIR_OUT)

/* Endpoint state */
struct usb_dc_stm32_ep_state {
	uint16_t ep_mps;		/** Endpoint max packet size */
	uint16_t ep_pma_buf_len;	/** Previously allocated buffer size */
	uint8_t ep_type;		/** Endpoint type (STM32 HAL enum) */
	uint8_t ep_stalled;	/** Endpoint stall flag */
	usb_dc_ep_callback cb;	/** Endpoint callback function */
	uint32_t read_count;	/** Number of bytes in read buffer  */
	uint32_t read_offset;	/** Current offset in read buffer */
	struct k_sem write_sem;	/** Write boolean semaphore */
};

/* Driver state */
struct usb_dc_stm32_state {
	PCD_HandleTypeDef pcd;	/* Storage for the HAL_PCD api */
	usb_dc_status_callback status_cb; /* Status callback */
	struct usb_dc_stm32_ep_state out_ep_state[USB_NUM_BIDIR_ENDPOINTS];
	struct usb_dc_stm32_ep_state in_ep_state[USB_NUM_BIDIR_ENDPOINTS];
	uint8_t ep_buf[USB_NUM_BIDIR_ENDPOINTS][EP_MPS];

#ifdef USB
	uint32_t pma_offset;
#endif /* USB */
};

static struct usb_dc_stm32_state usb_dc_stm32_state;

/* Internal functions */

static struct usb_dc_stm32_ep_state *usb_dc_stm32_get_ep_state(uint8_t ep)
{
	struct usb_dc_stm32_ep_state *ep_state_base;

	if (USB_EP_GET_IDX(ep) >= USB_NUM_BIDIR_ENDPOINTS) {
		return NULL;
	}

	if (USB_EP_DIR_IS_OUT(ep)) {
		ep_state_base = usb_dc_stm32_state.out_ep_state;
	} else {
		ep_state_base = usb_dc_stm32_state.in_ep_state;
	}

	return ep_state_base + USB_EP_GET_IDX(ep);
}

static void usb_dc_stm32_isr(const void *arg)
{
	HAL_PCD_IRQHandler(&usb_dc_stm32_state.pcd);
}

#ifdef CONFIG_USB_DEVICE_SOF
void HAL_PCD_SOFCallback(PCD_HandleTypeDef *hpcd)
{
	usb_dc_stm32_state.status_cb(USB_DC_SOF, NULL);
}
#endif

static int usb_dc_stm32_clock_enable(void)
{
	const struct device *clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	struct stm32_pclken pclken = {
		.bus = USB_CLOCK_BUS,
		.enr = USB_CLOCK_BITS,
	};

	/*
	 * Some SoCs in STM32F0/L0/L4 series disable USB clock by
	 * default.  We force USB clock source to MSI or PLL clock for this
	 * SoCs.  However, if these parts have an HSI48 clock, use
	 * that instead.  Example reference manual RM0360 for
	 * STM32F030x4/x6/x8/xC and STM32F070x6/xB.
	 */
#if defined(RCC_HSI48_SUPPORT) || \
	defined(CONFIG_SOC_SERIES_STM32WBX) || \
	defined(CONFIG_SOC_SERIES_STM32H7X)

	/*
	 * In STM32L0 series, HSI48 requires VREFINT and its buffer
	 * with 48 MHz RC to be enabled.
	 * See ENREF_HSI48 in referenc maual RM0367 section10.2.3:
	 * "Reference control and status register (SYSCFG_CFGR3)"
	 */
#ifdef CONFIG_SOC_SERIES_STM32L0X
	if (LL_APB2_GRP1_IsEnabledClock(LL_APB2_GRP1_PERIPH_SYSCFG)) {
		LL_SYSCFG_VREFINT_EnableHSI48();
	} else {
		LOG_ERR("System Configuration Controller clock is "
			"disabled. Unable to enable VREFINT which "
			"is required by HSI48.");
	}
#endif /* CONFIG_SOC_SERIES_STM32L0X */

	z_stm32_hsem_lock(CFG_HW_CLK48_CONFIG_SEMID, HSEM_LOCK_DEFAULT_RETRY);

	LL_RCC_HSI48_Enable();
	while (!LL_RCC_HSI48_IsReady()) {
		/* Wait for HSI48 to become ready */
	}

	LL_RCC_SetUSBClockSource(LL_RCC_USB_CLKSOURCE_HSI48);

#if !defined(CONFIG_SOC_SERIES_STM32WBX)
	/* Specially for STM32WB, don't unlock the HSEM to prevent M0 core
	 * to disable HSI48 clock used for RNG.
	 */
	z_stm32_hsem_unlock(CFG_HW_CLK48_CONFIG_SEMID);
#endif /* CONFIG_SOC_SERIES_STM32WBX */

#elif defined(LL_RCC_USB_CLKSOURCE_NONE)
	/* When MSI is configured in PLL mode with a 32.768 kHz clock source,
	 * the MSI frequency can be automatically trimmed by hardware to reach
	 * better than ±0.25% accuracy. In this mode the MSI can feed the USB
	 * device. For now, we only use MSI for USB if not already used as
	 * system clock source.
	 */
#if STM32_MSI_PLL_MODE && !STM32_SYSCLK_SRC_MSI
	LL_RCC_MSI_Enable();
	while (!LL_RCC_MSI_IsReady()) {
		/* Wait for MSI to become ready */
	}
	/* Force 48 MHz mode */
	LL_RCC_MSI_EnableRangeSelection();
	LL_RCC_MSI_SetRange(LL_RCC_MSIRANGE_11);
	LL_RCC_SetUSBClockSource(LL_RCC_USB_CLKSOURCE_MSI);
#else
	if (LL_RCC_PLL_IsReady()) {
		LL_RCC_SetUSBClockSource(LL_RCC_USB_CLKSOURCE_PLL);
	} else {
		LOG_ERR("Unable to set USB clock source to PLL.");
	}
#endif /* STM32_MSI_PLL_MODE && !STM32_SYSCLK_SRC_MSI */

#elif defined(RCC_CFGR_OTGFSPRE)
	/* On STM32F105 and STM32F107 parts the USB OTGFSCLK is derived from
	 * PLL1, and must result in a 48 MHz clock... the options to achieve
	 * this are as below, controlled by the RCC_CFGR_OTGFSPRE bit.
	 *   - PLLCLK * 2 / 2     i.e: PLLCLK == 48 MHz
	 *   - PLLCLK * 2 / 3     i.e: PLLCLK == 72 MHz
	 *
	 * this requires that the system is running from PLLCLK
	 */
	if (LL_RCC_GetSysClkSource() == LL_RCC_SYS_CLKSOURCE_STATUS_PLL) {
		switch (sys_clock_hw_cycles_per_sec()) {
		case 48000000U:
			LL_RCC_SetUSBClockSource(LL_RCC_USB_CLKSOURCE_PLL_DIV_2);
			break;
		case 72000000U:
			LL_RCC_SetUSBClockSource(LL_RCC_USB_CLKSOURCE_PLL_DIV_3);
			break;
		default:
			LOG_ERR("Unable to set USB clock source (incompatible PLLCLK rate)");
			return -EIO;
		}
	} else {
		LOG_ERR("Unable to set USB clock source (not using PLL1)");
		return -EIO;
	}

#endif /* RCC_HSI48_SUPPORT / LL_RCC_USB_CLKSOURCE_NONE / RCC_CFGR_OTGFSPRE */

	if (clock_control_on(clk, (clock_control_subsys_t *)&pclken) != 0) {
		LOG_ERR("Unable to enable USB clock");
		return -EIO;
	}

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otghs)
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_usbphyc)
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_OTGHSULPI);
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_OTGPHYC);
#else
	/* Disable ULPI interface (for external high-speed PHY) clock */
	LL_AHB1_GRP1_DisableClock(LL_AHB1_GRP1_PERIPH_OTGHSULPI);
	LL_AHB1_GRP1_DisableClockLowPower(LL_AHB1_GRP1_PERIPH_OTGHSULPI);
#endif
#endif

	return 0;
}

#if defined(USB_OTG_FS) || defined(USB_OTG_HS)
static uint32_t usb_dc_stm32_get_maximum_speed(void)
{
	/*
	 * If max-speed is not passed via DT, set it to USB controller's
	 * maximum hardware capability.
	 */
#if USB_OTG_HS_EMB_PHY
	uint32_t speed = USB_OTG_SPEED_HIGH;
#else
	uint32_t speed = USB_OTG_SPEED_FULL;
#endif

#ifdef USB_MAXIMUM_SPEED

	if (!strncmp(USB_MAXIMUM_SPEED, "high-speed", 10)) {
		speed = USB_OTG_SPEED_HIGH;
	} else if (!strncmp(USB_MAXIMUM_SPEED, "full-speed", 10)) {
#if defined(CONFIG_SOC_SERIES_STM32H7X) || defined(USB_OTG_HS_EMB_PHY)
		speed = USB_OTG_SPEED_HIGH_IN_FULL;
#else
		speed = USB_OTG_SPEED_FULL;
#endif
	} else {
		LOG_DBG("Unsupported maximum speed defined in device tree. "
			"USB controller will default to its maximum HW "
			"capability");
	}
#endif

	return speed;
}
#endif /* USB_OTG_FS || USB_OTG_HS */

static int usb_dc_stm32_init(void)
{
	HAL_StatusTypeDef status;
	unsigned int i;

#ifdef USB
	usb_dc_stm32_state.pcd.Instance = USB;
	usb_dc_stm32_state.pcd.Init.speed = PCD_SPEED_FULL;
	usb_dc_stm32_state.pcd.Init.dev_endpoints = USB_NUM_BIDIR_ENDPOINTS;
	usb_dc_stm32_state.pcd.Init.phy_itface = PCD_PHY_EMBEDDED;
	usb_dc_stm32_state.pcd.Init.ep0_mps = PCD_EP0MPS_64;
	usb_dc_stm32_state.pcd.Init.low_power_enable = 0;
#else /* USB_OTG_FS || USB_OTG_HS */
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otghs)
	usb_dc_stm32_state.pcd.Instance = USB_OTG_HS;
#else
	usb_dc_stm32_state.pcd.Instance = USB_OTG_FS;
#endif
	usb_dc_stm32_state.pcd.Init.dev_endpoints = USB_NUM_BIDIR_ENDPOINTS;
	usb_dc_stm32_state.pcd.Init.speed = usb_dc_stm32_get_maximum_speed();
#if USB_OTG_HS_EMB_PHY
	usb_dc_stm32_state.pcd.Init.phy_itface = USB_OTG_HS_EMBEDDED_PHY;
#else
	usb_dc_stm32_state.pcd.Init.phy_itface = PCD_PHY_EMBEDDED;
#endif
	usb_dc_stm32_state.pcd.Init.ep0_mps = USB_OTG_MAX_EP0_SIZE;
	usb_dc_stm32_state.pcd.Init.vbus_sensing_enable = DISABLE;

#ifndef CONFIG_SOC_SERIES_STM32F1X
	usb_dc_stm32_state.pcd.Init.dma_enable = DISABLE;
#endif

#endif /* USB */

#ifdef CONFIG_USB_DEVICE_SOF
	usb_dc_stm32_state.pcd.Init.Sof_enable = 1;
#endif /* CONFIG_USB_DEVICE_SOF */

#if defined(CONFIG_SOC_SERIES_STM32H7X)
	/* Currently assuming FS mode. Need to disable the ULPI clock on USB2 and
	 * enable the FS clock. Need to make this dependent on HS or FS config.
	 */

	LL_AHB1_GRP1_DisableClock(LL_AHB1_GRP1_PERIPH_USB2OTGHSULPI);
	LL_AHB1_GRP1_DisableClockSleep(LL_AHB1_GRP1_PERIPH_USB2OTGHSULPI);

	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_USB2OTGHS);
	LL_AHB1_GRP1_EnableClockSleep(LL_AHB1_GRP1_PERIPH_USB2OTGHS);

	LL_PWR_EnableUSBVoltageDetector();

	/* Per AN2606: USBREGEN not supported when running in FS mode. */
	LL_PWR_DisableUSBReg();
	while (!LL_PWR_IsActiveFlag_USB()) {
		LOG_INF("PWR not active yet");
		k_sleep(K_MSEC(100));
	}
#endif

	LOG_DBG("Pinctrl signals configuration");
	status = stm32_dt_pinctrl_configure(usb_pinctrl,
				     ARRAY_SIZE(usb_pinctrl),
				     (uint32_t)usb_dc_stm32_state.pcd.Instance);
	if (status < 0) {
		LOG_ERR("USB pinctrl setup failed (%d)", status);
		return status;
	}

	LOG_DBG("HAL_PCD_Init");
	status = HAL_PCD_Init(&usb_dc_stm32_state.pcd);
	if (status != HAL_OK) {
		LOG_ERR("PCD_Init failed, %d", (int)status);
		return -EIO;
	}

	LOG_DBG("HAL_PCD_Start");
	status = HAL_PCD_Start(&usb_dc_stm32_state.pcd);
	if (status != HAL_OK) {
		LOG_ERR("PCD_Start failed, %d", (int)status);
		return -EIO;
	}

	usb_dc_stm32_state.out_ep_state[EP0_IDX].ep_mps = EP0_MPS;
	usb_dc_stm32_state.out_ep_state[EP0_IDX].ep_type = EP_TYPE_CTRL;
	usb_dc_stm32_state.in_ep_state[EP0_IDX].ep_mps = EP0_MPS;
	usb_dc_stm32_state.in_ep_state[EP0_IDX].ep_type = EP_TYPE_CTRL;

#ifdef USB
	/* Start PMA configuration for the endpoints after the BTABLE. */
	usb_dc_stm32_state.pma_offset = USB_BTABLE_SIZE;

	for (i = 0U; i < USB_NUM_BIDIR_ENDPOINTS; i++) {
		k_sem_init(&usb_dc_stm32_state.in_ep_state[i].write_sem, 1, 1);
	}
#else /* USB_OTG_FS */
	/* TODO: make this dynamic (depending usage) */
	HAL_PCDEx_SetRxFiFo(&usb_dc_stm32_state.pcd, FIFO_EP_WORDS);
	for (i = 0U; i < USB_NUM_BIDIR_ENDPOINTS; i++) {
		HAL_PCDEx_SetTxFiFo(&usb_dc_stm32_state.pcd, i,
				    FIFO_EP_WORDS);
		k_sem_init(&usb_dc_stm32_state.in_ep_state[i].write_sem, 1, 1);
	}
#endif /* USB */

	IRQ_CONNECT(USB_IRQ, USB_IRQ_PRI,
		    usb_dc_stm32_isr, 0, 0);
	irq_enable(USB_IRQ);
	return 0;
}

/* Zephyr USB device controller API implementation */

int usb_dc_attach(void)
{
	int ret;

	LOG_DBG("");

#ifdef SYSCFG_CFGR1_USB_IT_RMP
	/*
	 * STM32F302/F303: USB IRQ collides with CAN_1 IRQ (§14.1.3, RM0316)
	 * Remap IRQ by default to enable use of both IPs simultaneoulsy
	 * This should be done before calling any HAL function
	 */
	if (LL_APB2_GRP1_IsEnabledClock(LL_APB2_GRP1_PERIPH_SYSCFG)) {
		LL_SYSCFG_EnableRemapIT_USB();
	} else {
		LOG_ERR("System Configuration Controller clock is "
			"disabled. Unable to enable IRQ remapping.");
	}
#endif

	/*
	 * For STM32F0 series SoCs on QFN28 and TSSOP20 packages enable PIN
	 * pair PA11/12 mapped instead of PA9/10 (e.g. stm32f070x6)
	 */
#if USB_ENABLE_PIN_REMAP == 1
	if (LL_APB1_GRP2_IsEnabledClock(LL_APB1_GRP2_PERIPH_SYSCFG)) {
		LL_SYSCFG_EnablePinRemap();
	} else {
		LOG_ERR("System Configuration Controller clock is "
			"disabled. Unable to enable pin remapping.");
	}
#endif

	ret = usb_dc_stm32_clock_enable();
	if (ret) {
		return ret;
	}

	ret = usb_dc_stm32_init();
	if (ret) {
		return ret;
	}

	/*
	 * Required for at least STM32L4 devices as they electrically
	 * isolate USB features from VDDUSB. It must be enabled before
	 * USB can function. Refer to section 5.1.3 in DM00083560 or
	 * DM00310109.
	 */
#ifdef PWR_CR2_USV
#if defined(LL_APB1_GRP1_PERIPH_PWR)
	if (LL_APB1_GRP1_IsEnabledClock(LL_APB1_GRP1_PERIPH_PWR)) {
		LL_PWR_EnableVddUSB();
	} else {
		LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
		LL_PWR_EnableVddUSB();
		LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_PWR);
	}
#else
	LL_PWR_EnableVddUSB();
#endif /* defined(LL_APB1_GRP1_PERIPH_PWR) */
#endif /* PWR_CR2_USV */

	return 0;
}

int usb_dc_ep_set_callback(const uint8_t ep, const usb_dc_ep_callback cb)
{
	struct usb_dc_stm32_ep_state *ep_state = usb_dc_stm32_get_ep_state(ep);

	LOG_DBG("ep 0x%02x", ep);

	if (!ep_state) {
		return -EINVAL;
	}

	ep_state->cb = cb;

	return 0;
}

void usb_dc_set_status_callback(const usb_dc_status_callback cb)
{
	LOG_DBG("");

	usb_dc_stm32_state.status_cb = cb;
}

int usb_dc_set_address(const uint8_t addr)
{
	HAL_StatusTypeDef status;

	LOG_DBG("addr %u (0x%02x)", addr, addr);

	status = HAL_PCD_SetAddress(&usb_dc_stm32_state.pcd, addr);
	if (status != HAL_OK) {
		LOG_ERR("HAL_PCD_SetAddress failed(0x%02x), %d", addr,
			(int)status);
		return -EIO;
	}

	return 0;
}

int usb_dc_ep_start_read(uint8_t ep, uint8_t *data, uint32_t max_data_len)
{
	HAL_StatusTypeDef status;

	LOG_DBG("ep 0x%02x, len %u", ep, max_data_len);

	/* we flush EP0_IN by doing a 0 length receive on it */
	if (!USB_EP_DIR_IS_OUT(ep) && (ep != EP0_IN || max_data_len)) {
		LOG_ERR("invalid ep 0x%02x", ep);
		return -EINVAL;
	}

	if (max_data_len > EP_MPS) {
		max_data_len = EP_MPS;
	}

	status = HAL_PCD_EP_Receive(&usb_dc_stm32_state.pcd, ep,
				    usb_dc_stm32_state.ep_buf[USB_EP_GET_IDX(ep)],
				    max_data_len);
	if (status != HAL_OK) {
		LOG_ERR("HAL_PCD_EP_Receive failed(0x%02x), %d", ep,
			(int)status);
		return -EIO;
	}

	return 0;
}

int usb_dc_ep_get_read_count(uint8_t ep, uint32_t *read_bytes)
{
	if (!USB_EP_DIR_IS_OUT(ep) || !read_bytes) {
		LOG_ERR("invalid ep 0x%02x", ep);
		return -EINVAL;
	}

	*read_bytes = HAL_PCD_EP_GetRxCount(&usb_dc_stm32_state.pcd, ep);

	return 0;
}

int usb_dc_ep_check_cap(const struct usb_dc_ep_cfg_data * const cfg)
{
	uint8_t ep_idx = USB_EP_GET_IDX(cfg->ep_addr);

	LOG_DBG("ep %x, mps %d, type %d", cfg->ep_addr, cfg->ep_mps,
		cfg->ep_type);

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

int usb_dc_ep_configure(const struct usb_dc_ep_cfg_data * const ep_cfg)
{
	uint8_t ep = ep_cfg->ep_addr;
	struct usb_dc_stm32_ep_state *ep_state = usb_dc_stm32_get_ep_state(ep);

	if (!ep_state) {
		return -EINVAL;
	}

	LOG_DBG("ep 0x%02x, previous ep_mps %u, ep_mps %u, ep_type %u",
		ep_cfg->ep_addr, ep_state->ep_mps, ep_cfg->ep_mps,
		ep_cfg->ep_type);

#ifdef USB
	if (ep_cfg->ep_mps > ep_state->ep_pma_buf_len) {
		if (USB_RAM_SIZE <=
		    (usb_dc_stm32_state.pma_offset + ep_cfg->ep_mps)) {
			return -EINVAL;
		}
		HAL_PCDEx_PMAConfig(&usb_dc_stm32_state.pcd, ep, PCD_SNG_BUF,
				    usb_dc_stm32_state.pma_offset);
		ep_state->ep_pma_buf_len = ep_cfg->ep_mps;
		usb_dc_stm32_state.pma_offset += ep_cfg->ep_mps;
	}
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

int usb_dc_ep_set_stall(const uint8_t ep)
{
	struct usb_dc_stm32_ep_state *ep_state = usb_dc_stm32_get_ep_state(ep);
	HAL_StatusTypeDef status;

	LOG_DBG("ep 0x%02x", ep);

	if (!ep_state) {
		return -EINVAL;
	}

	status = HAL_PCD_EP_SetStall(&usb_dc_stm32_state.pcd, ep);
	if (status != HAL_OK) {
		LOG_ERR("HAL_PCD_EP_SetStall failed(0x%02x), %d", ep,
			(int)status);
		return -EIO;
	}

	ep_state->ep_stalled = 1U;

	return 0;
}

int usb_dc_ep_clear_stall(const uint8_t ep)
{
	struct usb_dc_stm32_ep_state *ep_state = usb_dc_stm32_get_ep_state(ep);
	HAL_StatusTypeDef status;

	LOG_DBG("ep 0x%02x", ep);

	if (!ep_state) {
		return -EINVAL;
	}

	status = HAL_PCD_EP_ClrStall(&usb_dc_stm32_state.pcd, ep);
	if (status != HAL_OK) {
		LOG_ERR("HAL_PCD_EP_ClrStall failed(0x%02x), %d", ep,
			(int)status);
		return -EIO;
	}

	ep_state->ep_stalled = 0U;
	ep_state->read_count = 0U;

	return 0;
}

int usb_dc_ep_is_stalled(const uint8_t ep, uint8_t *const stalled)
{
	struct usb_dc_stm32_ep_state *ep_state = usb_dc_stm32_get_ep_state(ep);

	LOG_DBG("ep 0x%02x", ep);

	if (!ep_state || !stalled) {
		return -EINVAL;
	}

	*stalled = ep_state->ep_stalled;

	return 0;
}

int usb_dc_ep_enable(const uint8_t ep)
{
	struct usb_dc_stm32_ep_state *ep_state = usb_dc_stm32_get_ep_state(ep);
	HAL_StatusTypeDef status;

	LOG_DBG("ep 0x%02x", ep);

	if (!ep_state) {
		return -EINVAL;
	}

	LOG_DBG("HAL_PCD_EP_Open(0x%02x, %u, %u)", ep, ep_state->ep_mps,
		ep_state->ep_type);

	status = HAL_PCD_EP_Open(&usb_dc_stm32_state.pcd, ep,
				 ep_state->ep_mps, ep_state->ep_type);
	if (status != HAL_OK) {
		LOG_ERR("HAL_PCD_EP_Open failed(0x%02x), %d", ep,
			(int)status);
		return -EIO;
	}

	if (USB_EP_DIR_IS_OUT(ep) && ep != EP0_OUT) {
		return usb_dc_ep_start_read(ep,
					  usb_dc_stm32_state.ep_buf[USB_EP_GET_IDX(ep)],
					  EP_MPS);
	}

	return 0;
}

int usb_dc_ep_disable(const uint8_t ep)
{
	struct usb_dc_stm32_ep_state *ep_state = usb_dc_stm32_get_ep_state(ep);
	HAL_StatusTypeDef status;

	LOG_DBG("ep 0x%02x", ep);

	if (!ep_state) {
		return -EINVAL;
	}

	status = HAL_PCD_EP_Close(&usb_dc_stm32_state.pcd, ep);
	if (status != HAL_OK) {
		LOG_ERR("HAL_PCD_EP_Close failed(0x%02x), %d", ep,
			(int)status);
		return -EIO;
	}

	return 0;
}

int usb_dc_ep_write(const uint8_t ep, const uint8_t *const data,
		    const uint32_t data_len, uint32_t * const ret_bytes)
{
	struct usb_dc_stm32_ep_state *ep_state = usb_dc_stm32_get_ep_state(ep);
	HAL_StatusTypeDef status;
	uint32_t len = data_len;
	int ret = 0;

	LOG_DBG("ep 0x%02x, len %u", ep, data_len);

	if (!ep_state || !USB_EP_DIR_IS_IN(ep)) {
		LOG_ERR("invalid ep 0x%02x", ep);
		return -EINVAL;
	}

	ret = k_sem_take(&ep_state->write_sem, K_NO_WAIT);
	if (ret) {
		LOG_ERR("Unable to get write lock (%d)", ret);
		return -EAGAIN;
	}

	if (!k_is_in_isr()) {
		irq_disable(USB_IRQ);
	}

	if (ep == EP0_IN && len > USB_MAX_CTRL_MPS) {
		len = USB_MAX_CTRL_MPS;
	}

	status = HAL_PCD_EP_Transmit(&usb_dc_stm32_state.pcd, ep,
				     (void *)data, len);
	if (status != HAL_OK) {
		LOG_ERR("HAL_PCD_EP_Transmit failed(0x%02x), %d", ep,
			(int)status);
		k_sem_give(&ep_state->write_sem);
		ret = -EIO;
	}

	if (!ret && ep == EP0_IN && len > 0) {
		/* Wait for an empty package as from the host.
		 * This also flushes the TX FIFO to the host.
		 */
		usb_dc_ep_start_read(ep, NULL, 0);
	}

	if (!k_is_in_isr()) {
		irq_enable(USB_IRQ);
	}

	if (!ret && ret_bytes) {
		*ret_bytes = len;
	}

	return ret;
}

int usb_dc_ep_read_wait(uint8_t ep, uint8_t *data, uint32_t max_data_len,
			uint32_t *read_bytes)
{
	struct usb_dc_stm32_ep_state *ep_state = usb_dc_stm32_get_ep_state(ep);
	uint32_t read_count;

	if (!ep_state) {
		LOG_ERR("Invalid Endpoint %x", ep);
		return -EINVAL;
	}

	read_count = ep_state->read_count;

	LOG_DBG("ep 0x%02x, %u bytes, %u+%u, %p", ep, max_data_len,
		ep_state->read_offset, read_count, data);

	if (!USB_EP_DIR_IS_OUT(ep)) { /* check if OUT ep */
		LOG_ERR("Wrong endpoint direction: 0x%02x", ep);
		return -EINVAL;
	}

	/* When both buffer and max data to read are zero, just ingore reading
	 * and return available data in buffer. Otherwise, return data
	 * previously stored in the buffer.
	 */
	if (data) {
		read_count = MIN(read_count, max_data_len);
		memcpy(data, usb_dc_stm32_state.ep_buf[USB_EP_GET_IDX(ep)] +
		       ep_state->read_offset, read_count);
		ep_state->read_count -= read_count;
		ep_state->read_offset += read_count;
	} else if (max_data_len) {
		LOG_ERR("Wrong arguments");
	}

	if (read_bytes) {
		*read_bytes = read_count;
	}

	return 0;
}

int usb_dc_ep_read_continue(uint8_t ep)
{
	struct usb_dc_stm32_ep_state *ep_state = usb_dc_stm32_get_ep_state(ep);

	if (!ep_state || !USB_EP_DIR_IS_OUT(ep)) { /* Check if OUT ep */
		LOG_ERR("Not valid endpoint: %02x", ep);
		return -EINVAL;
	}

	/* If no more data in the buffer, start a new read transaction.
	 * DataOutStageCallback will called on transaction complete.
	 */
	if (!ep_state->read_count) {
		usb_dc_ep_start_read(ep, usb_dc_stm32_state.ep_buf[USB_EP_GET_IDX(ep)],
				     EP_MPS);
	}

	return 0;
}

int usb_dc_ep_read(const uint8_t ep, uint8_t *const data, const uint32_t max_data_len,
		   uint32_t * const read_bytes)
{
	if (usb_dc_ep_read_wait(ep, data, max_data_len, read_bytes) != 0) {
		return -EINVAL;
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
	struct usb_dc_stm32_ep_state *ep_state = usb_dc_stm32_get_ep_state(ep);

	if (!ep_state) {
		return -EINVAL;
	}

	LOG_ERR("Not implemented");

	return 0;
}

int usb_dc_ep_mps(const uint8_t ep)
{
	struct usb_dc_stm32_ep_state *ep_state = usb_dc_stm32_get_ep_state(ep);

	if (!ep_state) {
		return -EINVAL;
	}

	return ep_state->ep_mps;
}

int usb_dc_detach(void)
{
	LOG_ERR("Not implemented");

#ifdef CONFIG_SOC_SERIES_STM32WBX
	/* Specially for STM32WB, unlock the HSEM when USB is no more used. */
	z_stm32_hsem_unlock(CFG_HW_CLK48_CONFIG_SEMID);

	/*
	 * TODO: AN5289 notes a process of locking Sem0, with possible waits
	 * via interrupts before switching off CLK48, but lacking any actual
	 * examples of that, that remains to be implemented.  See
	 * https://github.com/zephyrproject-rtos/zephyr/pull/25850
	 */
#endif /* CONFIG_SOC_SERIES_STM32WBX */

	return 0;
}

int usb_dc_reset(void)
{
	LOG_ERR("Not implemented");

	return 0;
}

/* Callbacks from the STM32 Cube HAL code */

void HAL_PCD_ResetCallback(PCD_HandleTypeDef *hpcd)
{
	int i;

	LOG_DBG("");

	HAL_PCD_EP_Open(&usb_dc_stm32_state.pcd, EP0_IN, EP0_MPS, EP_TYPE_CTRL);
	HAL_PCD_EP_Open(&usb_dc_stm32_state.pcd, EP0_OUT, EP0_MPS,
			EP_TYPE_CTRL);

	/* The DataInCallback will never be called at this point for any pending
	 * transactions. Reset the IN semaphores to prevent perpetual locked state.
	 * */
	for (i = 0; i < USB_NUM_BIDIR_ENDPOINTS; i++) {
		k_sem_give(&usb_dc_stm32_state.in_ep_state[i].write_sem);
	}

	if (usb_dc_stm32_state.status_cb) {
		usb_dc_stm32_state.status_cb(USB_DC_RESET, NULL);
	}
}

void HAL_PCD_ConnectCallback(PCD_HandleTypeDef *hpcd)
{
	LOG_DBG("");

	if (usb_dc_stm32_state.status_cb) {
		usb_dc_stm32_state.status_cb(USB_DC_CONNECTED, NULL);
	}
}

void HAL_PCD_DisconnectCallback(PCD_HandleTypeDef *hpcd)
{
	LOG_DBG("");

	if (usb_dc_stm32_state.status_cb) {
		usb_dc_stm32_state.status_cb(USB_DC_DISCONNECTED, NULL);
	}
}

void HAL_PCD_SuspendCallback(PCD_HandleTypeDef *hpcd)
{
	LOG_DBG("");

	if (usb_dc_stm32_state.status_cb) {
		usb_dc_stm32_state.status_cb(USB_DC_SUSPEND, NULL);
	}
}

void HAL_PCD_ResumeCallback(PCD_HandleTypeDef *hpcd)
{
	LOG_DBG("");

	if (usb_dc_stm32_state.status_cb) {
		usb_dc_stm32_state.status_cb(USB_DC_RESUME, NULL);
	}
}

void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd)
{
	struct usb_setup_packet *setup = (void *)usb_dc_stm32_state.pcd.Setup;
	struct usb_dc_stm32_ep_state *ep_state;

	LOG_DBG("");

	ep_state = usb_dc_stm32_get_ep_state(EP0_OUT); /* can't fail for ep0 */
	__ASSERT(ep_state, "No corresponding ep_state for EP0");

	ep_state->read_count = SETUP_SIZE;
	ep_state->read_offset = 0U;
	memcpy(&usb_dc_stm32_state.ep_buf[EP0_IDX],
	       usb_dc_stm32_state.pcd.Setup, ep_state->read_count);

	if (ep_state->cb) {
		ep_state->cb(EP0_OUT, USB_DC_EP_SETUP);

		if (!(setup->wLength == 0U) &&
		    !(REQTYPE_GET_DIR(setup->bmRequestType) ==
		    REQTYPE_DIR_TO_HOST)) {
			usb_dc_ep_start_read(EP0_OUT,
					     usb_dc_stm32_state.ep_buf[EP0_IDX],
					     setup->wLength);
		}
	}
}

void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
	uint8_t ep_idx = USB_EP_GET_IDX(epnum);
	uint8_t ep = ep_idx | USB_EP_DIR_OUT;
	struct usb_dc_stm32_ep_state *ep_state = usb_dc_stm32_get_ep_state(ep);

	LOG_DBG("epnum 0x%02x, rx_count %u", epnum,
		HAL_PCD_EP_GetRxCount(&usb_dc_stm32_state.pcd, epnum));

	/* Transaction complete, data is now stored in the buffer and ready
	 * for the upper stack (usb_dc_ep_read to retrieve).
	 */
	usb_dc_ep_get_read_count(ep, &ep_state->read_count);
	ep_state->read_offset = 0U;

	if (ep_state->cb) {
		ep_state->cb(ep, USB_DC_EP_DATA_OUT);
	}
}

void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
	uint8_t ep_idx = USB_EP_GET_IDX(epnum);
	uint8_t ep = ep_idx | USB_EP_DIR_IN;
	struct usb_dc_stm32_ep_state *ep_state = usb_dc_stm32_get_ep_state(ep);

	LOG_DBG("epnum 0x%02x", epnum);

	__ASSERT(ep_state, "No corresponding ep_state for ep");

	k_sem_give(&ep_state->write_sem);

	if (ep_state->cb) {
		ep_state->cb(ep, USB_DC_EP_DATA_IN);
	}
}

#if defined(USB) && defined(CONFIG_USB_DC_STM32_DISCONN_ENABLE)
void HAL_PCDEx_SetConnectionState(PCD_HandleTypeDef *hpcd, uint8_t state)
{
	const struct device *usb_disconnect;

	usb_disconnect = device_get_binding(
				DT_GPIO_LABEL(DT_INST(0, st_stm32_usb), disconnect_gpios));

	gpio_pin_configure(usb_disconnect,
			   DT_GPIO_PIN(DT_INST(0, st_stm32_usb), disconnect_gpios),
			   DT_GPIO_FLAGS(DT_INST(0, st_stm32_usb), disconnect_gpios) |
			   (state ? GPIO_OUTPUT_ACTIVE : GPIO_OUTPUT_INACTIVE));
}
#endif /* USB && CONFIG_USB_DC_STM32_DISCONN_ENABLE */

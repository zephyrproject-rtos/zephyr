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
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include "stm32_hsem.h"

#define LOG_LEVEL CONFIG_USB_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(usb_dc_stm32);

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otgfs) && DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otghs)
#error "Only one interface should be enabled at a time, OTG FS or OTG HS"
#endif

/*
 * Vbus sensing is determined based on the presence of the hardware detection
 * pin(s) in the device tree. E.g: pinctrl-0 = <&usb_otg_fs_vbus_pa9 ...>;
 *
 * The detection pins are dependent on the enabled USB driver and the physical
 * interface(s) offered by the hardware. These are mapped to PA9 and/or PB13
 * (subject to MCU), being the former the most widespread option.
 */
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otghs)
#define DT_DRV_COMPAT    st_stm32_otghs
#define USB_IRQ_NAME     otghs
#define USB_VBUS_SENSING (DT_NODE_EXISTS(DT_CHILD(DT_NODELABEL(pinctrl), usb_otg_hs_vbus_pa9)) || \
			  DT_NODE_EXISTS(DT_CHILD(DT_NODELABEL(pinctrl), usb_otg_hs_vbus_pb13)))
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otgfs)
#define DT_DRV_COMPAT    st_stm32_otgfs
#define USB_IRQ_NAME     otgfs
#define USB_VBUS_SENSING DT_NODE_EXISTS(DT_CHILD(DT_NODELABEL(pinctrl), usb_otg_fs_vbus_pa9))
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_usb)
#define DT_DRV_COMPAT    st_stm32_usb
#define USB_IRQ_NAME     usb
#define USB_VBUS_SENSING false
#endif

#define USB_BASE_ADDRESS	DT_INST_REG_ADDR(0)
#define USB_IRQ			DT_INST_IRQ_BY_NAME(0, USB_IRQ_NAME, irq)
#define USB_IRQ_PRI		DT_INST_IRQ_BY_NAME(0, USB_IRQ_NAME, priority)
#define USB_NUM_BIDIR_ENDPOINTS	DT_INST_PROP(0, num_bidir_endpoints)
#define USB_RAM_SIZE		DT_INST_PROP(0, ram_size)

static const struct stm32_pclken pclken[] = STM32_DT_INST_CLOCKS(0);

#if !DT_HAS_COMPAT_STATUS_OKAY(st_stm32n6_otghs)
PINCTRL_DT_INST_DEFINE(0);
static const struct pinctrl_dev_config *usb_pcfg =
					PINCTRL_DT_INST_DEV_CONFIG_GET(0);
#endif

#define USB_OTG_HS_EMB_PHYC (DT_HAS_COMPAT_STATUS_OKAY(st_stm32_usbphyc) && \
			     DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otghs))

#define USB_OTG_HS_EMB_PHY (DT_HAS_COMPAT_STATUS_OKAY(st_stm32u5_otghs_phy) && \
			    DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otghs))

#define USB_OTG_HS_ULPI_PHY (DT_HAS_COMPAT_STATUS_OKAY(usb_ulpi_phy) && \
			    DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otghs))

#if USB_OTG_HS_ULPI_PHY
static const struct gpio_dt_spec ulpi_reset =
	GPIO_DT_SPEC_GET_OR(DT_PHANDLE(DT_INST(0, st_stm32_otghs), phys), reset_gpios, {0});
#endif

/**
 * The following defines are used to map the value of the "maxiumum-speed"
 * DT property to the corresponding definition used by the STM32 HAL.
 */
#if defined(CONFIG_SOC_SERIES_STM32H7X) || defined(USB_OTG_HS_EMB_PHYC) || \
	defined(USB_OTG_HS_EMB_PHY)
#define USB_DC_STM32_HIGH_SPEED             USB_OTG_SPEED_HIGH_IN_FULL
#else
#define USB_DC_STM32_HIGH_SPEED             USB_OTG_SPEED_HIGH
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_usb)
#define USB_DC_STM32_FULL_SPEED             PCD_SPEED_FULL
#else
#define USB_DC_STM32_FULL_SPEED             USB_OTG_SPEED_FULL
#endif
/*
 * USB, USB_OTG_FS and USB_DRD_FS are defined in STM32Cube HAL and allows to
 * distinguish between two kind of USB DC. STM32 F0, F3, L0 and G4 series
 * support USB device controller. STM32 F4 and F7 series support USB_OTG_FS
 * device controller. STM32 F1 and L4 series support either USB or USB_OTG_FS
 * device controller.STM32 G0 series supports USB_DRD_FS device controller.
 *
 * WARNING: Don't mix USB defined in STM32Cube HAL and CONFIG_USB_* from Zephyr
 * Kconfig system.
 */
#if defined(USB) || defined(USB_DRD_FS)

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

/* We need n TX IN FIFOs */
#define TX_FIFO_NUM USB_NUM_BIDIR_ENDPOINTS

/* We need a minimum size for RX FIFO - exact number seemingly determined through trial and error */
#define RX_FIFO_EP_WORDS 160

/* Allocate FIFO memory evenly between the TX FIFOs */
/* except the first TX endpoint need only 64 bytes */
#define TX_FIFO_EP_0_WORDS 16
#define TX_FIFO_WORDS (USB_RAM_SIZE / 4 - RX_FIFO_EP_WORDS - TX_FIFO_EP_0_WORDS)
/* Number of words for each remaining TX endpoint FIFO */
#define TX_FIFO_EP_WORDS (TX_FIFO_WORDS / (TX_FIFO_NUM - 1))

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

#if defined(USB) || defined(USB_DRD_FS)
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

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32u5_otghs_phy)

static const struct stm32_pclken phy_pclken[] = STM32_DT_CLOCKS(DT_INST_PHANDLE(0, phys));

static int usb_dc_stm32u5_phy_clock_select(const struct device *const clk)
{
	static const struct {
		uint32_t freq;
		uint32_t ref_clk;
	} clk_select[] = {
		{ MHZ(16),    SYSCFG_OTG_HS_PHY_CLK_SELECT_1 },
		{ KHZ(19200), SYSCFG_OTG_HS_PHY_CLK_SELECT_2 },
		{ MHZ(20),    SYSCFG_OTG_HS_PHY_CLK_SELECT_3 },
		{ MHZ(24),    SYSCFG_OTG_HS_PHY_CLK_SELECT_4 },
		{ MHZ(26),    SYSCFG_OTG_HS_PHY_CLK_SELECT_5 },
		{ MHZ(32),    SYSCFG_OTG_HS_PHY_CLK_SELECT_6 },
	};
	uint32_t freq;

	if (clock_control_get_rate(clk,
				   (clock_control_subsys_t)&phy_pclken[1],
				   &freq) != 0) {
		LOG_ERR("Failed to get USB_PHY clock source rate");
		return -EIO;
	}

	for (size_t i = 0; ARRAY_SIZE(clk_select); i++) {
		if (clk_select[i].freq == freq) {
			HAL_SYSCFG_SetOTGPHYReferenceClockSelection(clk_select[i].ref_clk);
			return 0;
		}
	}

	LOG_ERR("Unsupported PHY clock source frequency (%"PRIu32")", freq);

	return -EINVAL;
}

static int usb_dc_stm32u5_phy_clock_enable(const struct device *const clk)
{
	int err;

	err = clock_control_configure(clk, (clock_control_subsys_t)&phy_pclken[1], NULL);
	if (err) {
		LOG_ERR("Could not select USB_PHY clock source");
		return -EIO;
	}

	err = clock_control_on(clk, (clock_control_subsys_t)&phy_pclken[0]);
	if (err) {
		LOG_ERR("Unable to enable USB_PHY clock");
		return -EIO;
	}

	return usb_dc_stm32u5_phy_clock_select(clk);
}

static int usb_dc_stm32_phy_specific_clock_enable(const struct device *const clk)
{
	int err;

	/* Sequence to enable the power of the OTG HS on a stm32U5 serie : Enable VDDUSB */
	bool pwr_clk = LL_AHB3_GRP1_IsEnabledClock(LL_AHB3_GRP1_PERIPH_PWR);

	if (!pwr_clk) {
		LL_AHB3_GRP1_EnableClock(LL_AHB3_GRP1_PERIPH_PWR);
	}

	/* Check that power range is 1 or 2 */
	if (LL_PWR_GetRegulVoltageScaling() < LL_PWR_REGU_VOLTAGE_SCALE2) {
		LOG_ERR("Wrong Power range to use USB OTG HS");
		return -EIO;
	}

	LL_PWR_EnableVddUSB();
	/* Configure VOSR register of USB HSTransceiverSupply(); */
	LL_PWR_EnableUSBPowerSupply();
	LL_PWR_EnableUSBEPODBooster();
	while (LL_PWR_IsActiveFlag_USBBOOST() != 1) {
		/* Wait for USB EPOD BOOST ready */
	}

	/* Leave the PWR clock in its initial position */
	if (!pwr_clk) {
		LL_AHB3_GRP1_DisableClock(LL_AHB3_GRP1_PERIPH_PWR);
	}

	/* Set the OTG PHY reference clock selection (through SYSCFG) block */
	LL_APB3_GRP1_EnableClock(LL_APB3_GRP1_PERIPH_SYSCFG);

	err = usb_dc_stm32u5_phy_clock_enable(clk);
	if (err) {
		return err;
	}

	/* Configuring the SYSCFG registers OTG_HS PHY : OTG_HS PHY enable*/
	HAL_SYSCFG_EnableOTGPHY(SYSCFG_OTG_HS_PHY_ENABLE);

	if (clock_control_on(clk, (clock_control_subsys_t)&pclken[0]) != 0) {
		LOG_ERR("Unable to enable USB clock");
		return -EIO;
	}

	return 0;
}

#else /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32u5_otghs_phy) */

static int usb_dc_stm32_phy_specific_clock_enable(const struct device *const clk)
{
#if defined(PWR_USBSCR_USB33SV) || defined(PWR_SVMCR_USV)
	/*
	 * VDDUSB independent USB supply (PWR clock is on)
	 * with LL_PWR_EnableVDDUSB function (higher case)
	 */
	LL_PWR_EnableVDDUSB();
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32n6_otghs)
	/* Enable Vdd USB voltage monitoring */
	LL_PWR_EnableVddUSBMonitoring();
	while (__HAL_PWR_GET_FLAG(PWR_FLAG_USB33RDY)) {
		/* Wait for VDD33USB ready */
	}
	/* Enable VDDUSB */
	LL_PWR_EnableVddUSB();
#endif

	if (DT_INST_NUM_CLOCKS(0) > 1) {
		if (clock_control_configure(clk, (clock_control_subsys_t)&pclken[1],
									NULL) != 0) {
			LOG_ERR("Could not select USB domain clock");
			return -EIO;
		}
	}

	if (clock_control_on(clk, (clock_control_subsys_t)&pclken[0]) != 0) {
		LOG_ERR("Unable to enable USB clock");
		return -EIO;
	}

	if (IS_ENABLED(CONFIG_USB_DC_STM32_CLOCK_CHECK)) {
		uint32_t usb_clock_rate;

		if (clock_control_get_rate(clk,
					   (clock_control_subsys_t)&pclken[1],
					   &usb_clock_rate) != 0) {
			LOG_ERR("Failed to get USB domain clock rate");
			return -EIO;
		}

		if (usb_clock_rate != MHZ(48)) {
			LOG_ERR("USB Clock is not 48MHz (%d)", usb_clock_rate);
			return -ENOTSUP;
		}
	}

	return 0;
}

#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32u5_otghs_phy) */

static int usb_dc_stm32_clock_enable(void)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	int err;

	if (!device_is_ready(clk)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	err = usb_dc_stm32_phy_specific_clock_enable(clk);
	if (err) {
		return err;
	}

	/* Previous check won't work in case of F1/F3. Add build time check */
#if defined(RCC_CFGR_OTGFSPRE) || defined(RCC_CFGR_USBPRE)

#if (MHZ(48) == CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) && !defined(STM32_PLL_USBPRE)
	/* PLL output clock is set to 48MHz, it should not be divided */
#warning USBPRE/OTGFSPRE should be set in rcc node
#endif

#endif /* RCC_CFGR_OTGFSPRE / RCC_CFGR_USBPRE */

#if USB_OTG_HS_ULPI_PHY
#if defined(CONFIG_SOC_SERIES_STM32H7X)
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_USB1OTGHSULPI);
#else
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_OTGHSULPI);
#endif
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otghs) /* USB_OTG_HS_ULPI_PHY */
	/* Disable ULPI interface (for external high-speed PHY) clock in sleep/low-power mode.
	 * It is disabled by default in run power mode, no need to disable it.
	 */
#if defined(CONFIG_SOC_SERIES_STM32H7X)
	LL_AHB1_GRP1_DisableClockSleep(LL_AHB1_GRP1_PERIPH_USB1OTGHSULPI);
#elif defined(CONFIG_SOC_SERIES_STM32U5X)
	LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_USBPHY);
	/* Both OTG HS and USBPHY sleep clock MUST be disabled here at the same time */
	LL_AHB2_GRP1_DisableClockStopSleep(LL_AHB2_GRP1_PERIPH_OTG_HS ||
						LL_AHB2_GRP1_PERIPH_USBPHY);
#elif !DT_HAS_COMPAT_STATUS_OKAY(st_stm32n6_otghs)
	LL_AHB1_GRP1_DisableClockLowPower(LL_AHB1_GRP1_PERIPH_OTGHSULPI);
#endif

#if USB_OTG_HS_EMB_PHYC
#if !DT_HAS_COMPAT_STATUS_OKAY(st_stm32n6_otghs)
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_OTGPHYC);
#endif
#endif
#endif /* USB_OTG_HS_ULPI_PHY */

	return 0;
}

static int usb_dc_stm32_clock_disable(void)
{
	const struct device *clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	if (clock_control_off(clk, (clock_control_subsys_t)&pclken[0]) != 0) {
		LOG_ERR("Unable to disable USB clock");
		return -EIO;
	}
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32u5_otghs_phy)
	clock_control_off(clk, (clock_control_subsys_t)&phy_pclken[0]);
#endif

	return 0;
}

static int usb_dc_stm32_init(void)
{
	HAL_StatusTypeDef status;
	unsigned int i;

	usb_dc_stm32_state.pcd.Init.speed =
			UTIL_CAT(USB_DC_STM32_, DT_INST_STRING_UPPER_TOKEN(0, maximum_speed));

#if defined(USB) || defined(USB_DRD_FS)
#ifdef USB
	usb_dc_stm32_state.pcd.Instance = USB;
#else
	usb_dc_stm32_state.pcd.Instance = USB_DRD_FS;
#endif
	usb_dc_stm32_state.pcd.Init.dev_endpoints = USB_NUM_BIDIR_ENDPOINTS;
	usb_dc_stm32_state.pcd.Init.phy_itface = PCD_PHY_EMBEDDED;
	usb_dc_stm32_state.pcd.Init.ep0_mps = PCD_EP0MPS_64;
	usb_dc_stm32_state.pcd.Init.low_power_enable = 0;
#else /* USB_OTG_FS || USB_OTG_HS */
	usb_dc_stm32_state.pcd.Instance = (USB_OTG_GlobalTypeDef *)USB_BASE_ADDRESS;
	usb_dc_stm32_state.pcd.Init.dev_endpoints = USB_NUM_BIDIR_ENDPOINTS;
#if USB_OTG_HS_EMB_PHYC || USB_OTG_HS_EMB_PHY
	usb_dc_stm32_state.pcd.Init.phy_itface = USB_OTG_HS_EMBEDDED_PHY;
#elif USB_OTG_HS_ULPI_PHY
	usb_dc_stm32_state.pcd.Init.phy_itface = USB_OTG_ULPI_PHY;
#else
	usb_dc_stm32_state.pcd.Init.phy_itface = PCD_PHY_EMBEDDED;
#endif
	usb_dc_stm32_state.pcd.Init.ep0_mps = USB_OTG_MAX_EP0_SIZE;
	usb_dc_stm32_state.pcd.Init.vbus_sensing_enable = USB_VBUS_SENSING ? ENABLE : DISABLE;

#ifndef CONFIG_SOC_SERIES_STM32F1X
	usb_dc_stm32_state.pcd.Init.dma_enable = DISABLE;
#endif

#endif /* USB */

#ifdef CONFIG_USB_DEVICE_SOF
	usb_dc_stm32_state.pcd.Init.Sof_enable = 1;
#endif /* CONFIG_USB_DEVICE_SOF */

#if defined(CONFIG_SOC_SERIES_STM32H7X)
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otgfs)
	/* The USB2 controller only works in FS mode, but the ULPI clock needs
	 * to be disabled in sleep mode for it to work. For the USB1
	 * controller, as it is an HS one, the clock is disabled in the common
	 * path.
	 */

	LL_AHB1_GRP1_DisableClockSleep(LL_AHB1_GRP1_PERIPH_USB2OTGHSULPI);
#endif

	LL_PWR_EnableUSBVoltageDetector();

	/* Per AN2606: USBREGEN not supported when running in FS mode. */
	LL_PWR_DisableUSBReg();
	while (!LL_PWR_IsActiveFlag_USB()) {
		LOG_INF("PWR not active yet");
		k_sleep(K_MSEC(100));
	}
#endif

#if !DT_HAS_COMPAT_STATUS_OKAY(st_stm32n6_otghs)
	LOG_DBG("Pinctrl signals configuration");
	int ret = pinctrl_apply_state(usb_pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("USB pinctrl setup failed (%d)", ret);
		return ret;
	}
#endif

	LOG_DBG("HAL_PCD_Init");
	status = HAL_PCD_Init(&usb_dc_stm32_state.pcd);
	if (status != HAL_OK) {
		LOG_ERR("PCD_Init failed, %d", (int)status);
		return -EIO;
	}

	/* On a soft reset force USB to reset first and switch it off
	 * so the USB connection can get re-initialized
	 */
	LOG_DBG("HAL_PCD_Stop");
	status = HAL_PCD_Stop(&usb_dc_stm32_state.pcd);
	if (status != HAL_OK) {
		LOG_ERR("PCD_Stop failed, %d", (int)status);
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

#if defined(USB) || defined(USB_DRD_FS)
	/* Start PMA configuration for the endpoints after the BTABLE. */
	usb_dc_stm32_state.pma_offset = USB_BTABLE_SIZE;

	for (i = 0U; i < USB_NUM_BIDIR_ENDPOINTS; i++) {
		k_sem_init(&usb_dc_stm32_state.in_ep_state[i].write_sem, 1, 1);
	}
#else /* USB_OTG_FS */

	/* TODO: make this dynamic (depending usage) */
	HAL_PCDEx_SetRxFiFo(&usb_dc_stm32_state.pcd, RX_FIFO_EP_WORDS);
	for (i = 0U; i < USB_NUM_BIDIR_ENDPOINTS; i++) {
		if (i == 0) {
			/* first endpoint need only 64 byte for EP_TYPE_CTRL */
			HAL_PCDEx_SetTxFiFo(&usb_dc_stm32_state.pcd, i,
					TX_FIFO_EP_0_WORDS);
		} else {
			HAL_PCDEx_SetTxFiFo(&usb_dc_stm32_state.pcd, i,
					TX_FIFO_EP_WORDS);
		}
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

#if USB_OTG_HS_ULPI_PHY
	if (ulpi_reset.port != NULL) {
		if (!gpio_is_ready_dt(&ulpi_reset)) {
			LOG_ERR("Reset GPIO device not ready");
			return -EINVAL;
		}
		if (gpio_pin_configure_dt(&ulpi_reset, GPIO_OUTPUT_INACTIVE)) {
			LOG_ERR("Couldn't configure reset pin");
			return -EIO;
		}
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
	 * isolate USB features from VddUSB. It must be enabled before
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
#if defined(USB) || defined(USB_DRD_FS)
	if (ep_cfg->ep_mps > ep_state->ep_pma_buf_len) {
		if (ep_cfg->ep_type == USB_DC_EP_ISOCHRONOUS) {
			if (USB_RAM_SIZE <=
			   (usb_dc_stm32_state.pma_offset + ep_cfg->ep_mps*2)) {
				return -EINVAL;
			}
		} else if (USB_RAM_SIZE <=
			  (usb_dc_stm32_state.pma_offset + ep_cfg->ep_mps)) {
			return -EINVAL;
		}

		if (ep_cfg->ep_type == USB_DC_EP_ISOCHRONOUS) {
			HAL_PCDEx_PMAConfig(&usb_dc_stm32_state.pcd, ep, PCD_DBL_BUF,
				usb_dc_stm32_state.pma_offset +
				((usb_dc_stm32_state.pma_offset + ep_cfg->ep_mps) << 16));
			ep_state->ep_pma_buf_len = ep_cfg->ep_mps*2;
			usb_dc_stm32_state.pma_offset += ep_cfg->ep_mps*2;
		} else {
			HAL_PCDEx_PMAConfig(&usb_dc_stm32_state.pcd, ep, PCD_SNG_BUF,
				usb_dc_stm32_state.pma_offset);
			ep_state->ep_pma_buf_len = ep_cfg->ep_mps;
			usb_dc_stm32_state.pma_offset += ep_cfg->ep_mps;
		}
	}
	if (ep_cfg->ep_type == USB_DC_EP_ISOCHRONOUS) {
		ep_state->ep_mps = ep_cfg->ep_mps*2;
	} else {
		ep_state->ep_mps = ep_cfg->ep_mps;
	}
#else
	ep_state->ep_mps = ep_cfg->ep_mps;
#endif

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
					  ep_state->ep_mps);
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
		ep_state->read_offset, read_count, (void *)data);

	if (!USB_EP_DIR_IS_OUT(ep)) { /* check if OUT ep */
		LOG_ERR("Wrong endpoint direction: 0x%02x", ep);
		return -EINVAL;
	}

	/* When both buffer and max data to read are zero, just ignore reading
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
				     ep_state->ep_mps);
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

int usb_dc_wakeup_request(void)
{
	HAL_StatusTypeDef status;

	status = HAL_PCD_ActivateRemoteWakeup(&usb_dc_stm32_state.pcd);
	if (status != HAL_OK) {
		return -EAGAIN;
	}

	/* Must be active from 1ms to 15ms as per reference manual. */
	k_sleep(K_MSEC(2));

	status = HAL_PCD_DeActivateRemoteWakeup(&usb_dc_stm32_state.pcd);
	if (status != HAL_OK) {
		return -EAGAIN;
	}

	return 0;
}

int usb_dc_detach(void)
{
	HAL_StatusTypeDef status;
	int ret;

	LOG_DBG("HAL_PCD_DeInit");
	status = HAL_PCD_DeInit(&usb_dc_stm32_state.pcd);
	if (status != HAL_OK) {
		LOG_ERR("PCD_DeInit failed, %d", (int)status);
		return -EIO;
	}

	ret = usb_dc_stm32_clock_disable();
	if (ret) {
		return ret;
	}

	if (irq_is_enabled(USB_IRQ)) {
		irq_disable(USB_IRQ);
	}

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
		    usb_reqtype_is_to_device(setup)) {
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

void HAL_PCD_ISOINIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
	uint8_t ep_idx = USB_EP_GET_IDX(epnum);
	uint8_t ep = ep_idx | USB_EP_DIR_IN;
	struct usb_dc_stm32_ep_state *ep_state = usb_dc_stm32_get_ep_state(ep);

	LOG_DBG("epnum 0x%02x", epnum);

	__ASSERT(ep_state, "No corresponding ep_state for ep");

	k_sem_give(&ep_state->write_sem);
}

#if (defined(USB) || defined(USB_DRD_FS)) && DT_INST_NODE_HAS_PROP(0, disconnect_gpios)
void HAL_PCDEx_SetConnectionState(PCD_HandleTypeDef *hpcd, uint8_t state)
{
	struct gpio_dt_spec usb_disconnect = GPIO_DT_SPEC_INST_GET(0, disconnect_gpios);

	gpio_pin_configure_dt(&usb_disconnect,
			   (state ? GPIO_OUTPUT_ACTIVE : GPIO_OUTPUT_INACTIVE));
}
#endif /* USB && DT_INST_NODE_HAS_PROP(0, disconnect_gpios) */

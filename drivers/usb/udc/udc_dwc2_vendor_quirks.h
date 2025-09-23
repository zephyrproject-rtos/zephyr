/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UDC_DWC2_VENDOR_QUIRKS_H
#define ZEPHYR_DRIVERS_USB_UDC_DWC2_VENDOR_QUIRKS_H

#include "udc_dwc2.h"

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/usb/udc.h>

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32f4_fsotg)

#include <zephyr/sys/sys_io.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <usb_dwc2_hw.h>

struct usb_dw_stm32_clk {
	const struct device *const dev;
	const struct stm32_pclken *const pclken;
	size_t pclken_len;
};

static inline int stm32f4_fsotg_enable_clk(const struct usb_dw_stm32_clk *const clk)
{
	int ret;

	if (!device_is_ready(clk->dev)) {
		return -ENODEV;
	}

	if (clk->pclken_len > 1) {
		uint32_t clk_rate;

		ret = clock_control_configure(clk->dev,
					      (void *)&clk->pclken[1],
					      NULL);
		if (ret) {
			return ret;
		}

		ret = clock_control_get_rate(clk->dev,
					     (void *)&clk->pclken[1],
					     &clk_rate);
		if (ret) {
			return ret;
		}

		if (clk_rate != MHZ(48)) {
			return -ENOTSUP;
		}
	}

	return clock_control_on(clk->dev, (void *)&clk->pclken[0]);
}

static inline int stm32f4_fsotg_enable_phy(const struct device *dev)
{
	const struct udc_dwc2_config *const config = dev->config;
	mem_addr_t ggpio_reg = (mem_addr_t)&config->base->ggpio;

	sys_set_bits(ggpio_reg, USB_DWC2_GGPIO_STM32_PWRDWN | USB_DWC2_GGPIO_STM32_VBDEN);

	return 0;
}

static inline int stm32f4_fsotg_disable_phy(const struct device *dev)
{
	const struct udc_dwc2_config *const config = dev->config;
	mem_addr_t ggpio_reg = (mem_addr_t)&config->base->ggpio;

	sys_clear_bits(ggpio_reg, USB_DWC2_GGPIO_STM32_PWRDWN | USB_DWC2_GGPIO_STM32_VBDEN);

	return 0;
}

#define QUIRK_STM32F4_FSOTG_DEFINE(n)						\
	static const struct stm32_pclken pclken_##n[] = STM32_DT_INST_CLOCKS(n);\
										\
	static const struct usb_dw_stm32_clk stm32f4_clk_##n = {		\
		.dev = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),			\
		.pclken = pclken_##n,						\
		.pclken_len = DT_INST_NUM_CLOCKS(n),				\
	};									\
										\
	static int stm32f4_fsotg_enable_clk_##n(const struct device *dev)	\
	{									\
		return stm32f4_fsotg_enable_clk(&stm32f4_clk_##n);		\
	}									\
										\
	const struct dwc2_vendor_quirks dwc2_vendor_quirks_##n = {		\
		.pre_enable = stm32f4_fsotg_enable_clk_##n,			\
		.post_enable = stm32f4_fsotg_enable_phy,			\
		.disable = stm32f4_fsotg_disable_phy,				\
		.irq_clear = NULL,						\
	};


DT_INST_FOREACH_STATUS_OKAY(QUIRK_STM32F4_FSOTG_DEFINE)

#endif /*DT_HAS_COMPAT_STATUS_OKAY(st_stm32f4_fsotg) */

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_usbhs)

#include <zephyr/logging/log.h>
#include <nrfs_backend_ipc_service.h>
#include <nrfs_usb.h>

#define USBHS_DT_WRAPPER_REG_ADDR(n) UINT_TO_POINTER(DT_INST_REG_ADDR_BY_NAME(n, wrapper))

/*
 * On USBHS, we cannot access the DWC2 register until VBUS is detected and
 * valid. If the user tries to force usbd_enable() and the corresponding
 * udc_enable() without a "VBUS ready" notification, the event wait will block
 * until a valid VBUS signal is detected or until the
 * CONFIG_UDC_DWC2_USBHS_VBUS_READY_TIMEOUT timeout expires.
 */
static K_EVENT_DEFINE(usbhs_events);
#define USBHS_VBUS_READY	BIT(0)

static void usbhs_vbus_handler(nrfs_usb_evt_t const *p_evt, void *const context)
{
	LOG_MODULE_DECLARE(udc_dwc2, CONFIG_UDC_DRIVER_LOG_LEVEL);
	const struct device *dev = context;

	switch (p_evt->type) {
	case NRFS_USB_EVT_VBUS_STATUS_CHANGE:
		LOG_DBG("USBHS new status, pll_ok = %d vreg_ok = %d vbus_detected = %d",
			p_evt->usbhspll_ok, p_evt->vregusb_ok, p_evt->vbus_detected);

		if (p_evt->usbhspll_ok && p_evt->vregusb_ok && p_evt->vbus_detected) {
			k_event_post(&usbhs_events, USBHS_VBUS_READY);
			udc_submit_event(dev, UDC_EVT_VBUS_READY, 0);
		} else {
			k_event_set_masked(&usbhs_events, 0, USBHS_VBUS_READY);
			udc_submit_event(dev, UDC_EVT_VBUS_REMOVED, 0);
		}

		break;
	case NRFS_USB_EVT_REJECT:
		LOG_ERR("Request rejected");
		break;
	default:
		LOG_ERR("Unknown event type 0x%x", p_evt->type);
		break;
	}
}

static inline int usbhs_enable_nrfs_service(const struct device *dev)
{
	LOG_MODULE_DECLARE(udc_dwc2, CONFIG_UDC_DRIVER_LOG_LEVEL);
	nrfs_err_t nrfs_err;
	int err;

	err = nrfs_backend_wait_for_connection(K_MSEC(1000));
	if (err) {
		LOG_INF("NRFS backend connection timeout");
		return err;
	}

	nrfs_err = nrfs_usb_init(usbhs_vbus_handler);
	if (nrfs_err != NRFS_SUCCESS) {
		LOG_ERR("Failed to init NRFS VBUS handler: %d", nrfs_err);
		return -EIO;
	}

	nrfs_err = nrfs_usb_enable_request((void *)dev);
	if (nrfs_err != NRFS_SUCCESS) {
		LOG_ERR("Failed to enable NRFS VBUS service: %d", nrfs_err);
		return -EIO;
	}

	return 0;
}

static inline int usbhs_enable_core(const struct device *dev)
{
	LOG_MODULE_DECLARE(udc_dwc2, CONFIG_UDC_DRIVER_LOG_LEVEL);
	NRF_USBHS_Type *wrapper = USBHS_DT_WRAPPER_REG_ADDR(0);
	k_timeout_t timeout = K_FOREVER;

	#if CONFIG_NRFS_HAS_VBUS_DETECTOR_SERVICE
	if (CONFIG_UDC_DWC2_USBHS_VBUS_READY_TIMEOUT) {
		timeout = K_MSEC(CONFIG_UDC_DWC2_USBHS_VBUS_READY_TIMEOUT);
	}
	#endif

	if (!k_event_wait(&usbhs_events, USBHS_VBUS_READY, false, K_NO_WAIT)) {
		LOG_WRN("VBUS is not ready, block udc_enable()");
		if (!k_event_wait(&usbhs_events, USBHS_VBUS_READY, false, timeout)) {
			return -ETIMEDOUT;
		}
	}

	wrapper->ENABLE = USBHS_ENABLE_PHY_Msk | USBHS_ENABLE_CORE_Msk;

	/* Wait for PHY clock to start */
	k_busy_wait(45);

	/* Release DWC2 reset */
	wrapper->TASKS_START = 1UL;

	/* Wait for clock to start to avoid hang on too early register read */
	k_busy_wait(1);

	/* Enable interrupts */
	wrapper->INTENSET = 1UL;

	return 0;
}

static inline int usbhs_disable_core(const struct device *dev)
{
	NRF_USBHS_Type *wrapper = USBHS_DT_WRAPPER_REG_ADDR(0);

	/* Disable interrupts */
	wrapper->INTENCLR = 1UL;

	wrapper->ENABLE = 0UL;

	return 0;
}

static inline int usbhs_disable_nrfs_service(const struct device *dev)
{
	LOG_MODULE_DECLARE(udc_dwc2, CONFIG_UDC_DRIVER_LOG_LEVEL);
	nrfs_err_t nrfs_err;

	nrfs_err = nrfs_usb_disable_request((void *)dev);
	if (nrfs_err != NRFS_SUCCESS) {
		LOG_ERR("Failed to disable NRFS VBUS service: %d", nrfs_err);
		return -EIO;
	}

	nrfs_usb_uninit();

	return 0;
}

static inline int usbhs_irq_clear(const struct device *dev)
{
	NRF_USBHS_Type *wrapper = USBHS_DT_WRAPPER_REG_ADDR(0);

	wrapper->EVENTS_CORE = 0UL;

	return 0;
}

static inline int usbhs_init_caps(const struct device *dev)
{
	struct udc_data *data = dev->data;

	data->caps.can_detect_vbus = true;
	data->caps.hs = true;

	return 0;
}

static inline int usbhs_is_phy_clk_off(const struct device *dev)
{
	return !k_event_test(&usbhs_events, USBHS_VBUS_READY);
}

static inline int usbhs_post_hibernation_entry(const struct device *dev)
{
	const struct udc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const base = config->base;
	NRF_USBHS_Type *wrapper = USBHS_DT_WRAPPER_REG_ADDR(0);

	sys_set_bits((mem_addr_t)&base->pcgcctl, USB_DWC2_PCGCCTL_GATEHCLK);

	sys_write32(0x87, (mem_addr_t)wrapper + 0xC80);
	sys_write32(0x87, (mem_addr_t)wrapper + 0xC84);
	sys_write32(1, (mem_addr_t)wrapper + 0x004);

	return 0;
}

static inline int usbhs_pre_hibernation_exit(const struct device *dev)
{
	const struct udc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const base = config->base;
	NRF_USBHS_Type *wrapper = USBHS_DT_WRAPPER_REG_ADDR(0);

	sys_clear_bits((mem_addr_t)&base->pcgcctl, USB_DWC2_PCGCCTL_GATEHCLK);

	wrapper->TASKS_START = 1;
	sys_write32(0, (mem_addr_t)wrapper + 0xC80);
	sys_write32(0, (mem_addr_t)wrapper + 0xC84);

	return 0;
}

#define QUIRK_NRF_USBHS_DEFINE(n)						\
	const struct dwc2_vendor_quirks dwc2_vendor_quirks_##n = {		\
		.init = usbhs_enable_nrfs_service,				\
		.pre_enable = usbhs_enable_core,				\
		.disable = usbhs_disable_core,					\
		.shutdown = usbhs_disable_nrfs_service,				\
		.irq_clear = usbhs_irq_clear,					\
		.caps = usbhs_init_caps,					\
		.is_phy_clk_off = usbhs_is_phy_clk_off,				\
		.post_hibernation_entry = usbhs_post_hibernation_entry,		\
		.pre_hibernation_exit = usbhs_pre_hibernation_exit,		\
	};

DT_INST_FOREACH_STATUS_OKAY(QUIRK_NRF_USBHS_DEFINE)

#endif /*DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_usbhs) */

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_usbhs_nrf54l)

#define USBHS_DT_WRAPPER_REG_ADDR(n) UINT_TO_POINTER(DT_INST_REG_ADDR_BY_NAME(n, wrapper))

#include <nrf.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>

#define NRF_DEFAULT_IRQ_PRIORITY 1

/*
 * On USBHS, we cannot access the DWC2 register until VBUS is detected and
 * valid. If the user tries to force usbd_enable() and the corresponding
 * udc_enable() without a "VBUS ready" notification, the event wait will block
 * until a valid VBUS signal is detected or until the
 * CONFIG_UDC_DWC2_USBHS_VBUS_READY_TIMEOUT timeout expires.
 */
static K_EVENT_DEFINE(usbhs_events);
#define USBHS_VBUS_READY	BIT(0)

static struct onoff_manager *pclk24m_mgr;
static struct onoff_client pclk24m_cli;

static void vregusb_isr(const void *arg)
{
	const struct device *dev = arg;

	if (NRF_VREGUSB->EVENTS_VBUSDETECTED) {
		NRF_VREGUSB->EVENTS_VBUSDETECTED = 0;
		k_event_post(&usbhs_events, USBHS_VBUS_READY);
		udc_submit_event(dev, UDC_EVT_VBUS_READY, 0);
	}

	if (NRF_VREGUSB->EVENTS_VBUSREMOVED) {
		NRF_VREGUSB->EVENTS_VBUSREMOVED = 0;
		k_event_set_masked(&usbhs_events, 0, USBHS_VBUS_READY);
		udc_submit_event(dev, UDC_EVT_VBUS_REMOVED, 0);
	}
}

static inline int usbhs_init_vreg_and_clock(const struct device *dev)
{
	IRQ_CONNECT(VREGUSB_IRQn, NRF_DEFAULT_IRQ_PRIORITY,
		    vregusb_isr, DEVICE_DT_INST_GET(0), 0);

	NRF_VREGUSB->INTEN = VREGUSB_INTEN_VBUSDETECTED_Msk |
			     VREGUSB_INTEN_VBUSREMOVED_Msk;
	NRF_VREGUSB->TASKS_START = 1;

	/* TODO: Determine conditions when VBUSDETECTED is not generated */
	if (sys_read32((mem_addr_t)NRF_VREGUSB + 0x400) & BIT(2)) {
		k_event_post(&usbhs_events, USBHS_VBUS_READY);
		udc_submit_event(dev, UDC_EVT_VBUS_READY, 0);
	}

	irq_enable(VREGUSB_IRQn);
	pclk24m_mgr = z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF24M);

	return 0;
}

static inline int usbhs_enable_core(const struct device *dev)
{
	LOG_MODULE_DECLARE(udc_dwc2, CONFIG_UDC_DRIVER_LOG_LEVEL);
	NRF_USBHS_Type *wrapper = USBHS_DT_WRAPPER_REG_ADDR(0);
	k_timeout_t timeout = K_FOREVER;
	int err;

	if (!k_event_wait(&usbhs_events, USBHS_VBUS_READY, false, K_NO_WAIT)) {
		LOG_WRN("VBUS is not ready, block udc_enable()");
		if (!k_event_wait(&usbhs_events, USBHS_VBUS_READY, false, timeout)) {
			return -ETIMEDOUT;
		}
	}

	/* Request PCLK24M using clock control driver */
	sys_notify_init_spinwait(&pclk24m_cli.notify);
	err = onoff_request(pclk24m_mgr, &pclk24m_cli);
	if (err < 0) {
		LOG_ERR("Failed to start PCLK24M %d", err);
		return err;
	}

	/* Power up peripheral */
	wrapper->ENABLE = USBHS_ENABLE_CORE_Msk;

	/* Set ID to Device and force D+ pull-up off for now */
	wrapper->PHY.OVERRIDEVALUES = (1 << 31);
	wrapper->PHY.INPUTOVERRIDE = (1 << 31) | USBHS_PHY_INPUTOVERRIDE_VBUSVALID_Msk;

	/* Release PHY power-on reset */
	wrapper->ENABLE = USBHS_ENABLE_PHY_Msk | USBHS_ENABLE_CORE_Msk;

	/* Wait for PHY clock to start */
	k_busy_wait(45);

	/* Release DWC2 reset */
	wrapper->TASKS_START = 1UL;

	/* Wait for clock to start to avoid hang on too early register read */
	k_busy_wait(1);

	/* DWC2 opmode is now guaranteed to be Non-Driving, allow D+ pull-up to
	 * become active once driver clears DCTL SftDiscon bit.
	 */
	wrapper->PHY.INPUTOVERRIDE = (1 << 31);

	return 0;
}

static inline int usbhs_disable_core(const struct device *dev)
{
	LOG_MODULE_DECLARE(udc_dwc2, CONFIG_UDC_DRIVER_LOG_LEVEL);
	NRF_USBHS_Type *wrapper = USBHS_DT_WRAPPER_REG_ADDR(0);
	int err;

	/* Set ID to Device and forcefully disable D+ pull-up */
	wrapper->PHY.OVERRIDEVALUES = (1 << 31);
	wrapper->PHY.INPUTOVERRIDE = (1 << 31) | USBHS_PHY_INPUTOVERRIDE_VBUSVALID_Msk;

	wrapper->ENABLE = 0UL;

	/* Release PCLK24M using clock control driver */
	err = onoff_cancel_or_release(pclk24m_mgr, &pclk24m_cli);
	if (err < 0) {
		LOG_ERR("Failed to stop PCLK24M %d", err);
		return err;
	}

	return 0;
}

static inline int usbhs_disable_vreg(const struct device *dev)
{
	NRF_VREGUSB->INTEN = 0;
	NRF_VREGUSB->TASKS_STOP = 1;

	return 0;
}

static inline int usbhs_init_caps(const struct device *dev)
{
	struct udc_data *data = dev->data;

	data->caps.can_detect_vbus = true;
	data->caps.hs = true;

	return 0;
}

static inline int usbhs_is_phy_clk_off(const struct device *dev)
{
	return !k_event_test(&usbhs_events, USBHS_VBUS_READY);
}

static inline int usbhs_post_hibernation_entry(const struct device *dev)
{
	const struct udc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const base = config->base;
	NRF_USBHS_Type *wrapper = USBHS_DT_WRAPPER_REG_ADDR(0);

	sys_set_bits((mem_addr_t)&base->pcgcctl, USB_DWC2_PCGCCTL_GATEHCLK);

	wrapper->TASKS_STOP = 1;

	return 0;
}

static inline int usbhs_pre_hibernation_exit(const struct device *dev)
{
	const struct udc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const base = config->base;
	NRF_USBHS_Type *wrapper = USBHS_DT_WRAPPER_REG_ADDR(0);

	sys_clear_bits((mem_addr_t)&base->pcgcctl, USB_DWC2_PCGCCTL_GATEHCLK);

	wrapper->TASKS_START = 1;

	return 0;
}

#define QUIRK_NRF_USBHS_DEFINE(n)						\
	struct dwc2_vendor_quirks dwc2_vendor_quirks_##n = {			\
		.init = usbhs_init_vreg_and_clock,				\
		.pre_enable = usbhs_enable_core,				\
		.disable = usbhs_disable_core,					\
		.shutdown = usbhs_disable_vreg,					\
		.caps = usbhs_init_caps,					\
		.is_phy_clk_off = usbhs_is_phy_clk_off,				\
		.post_hibernation_entry = usbhs_post_hibernation_entry,		\
		.pre_hibernation_exit = usbhs_pre_hibernation_exit,		\
	};

DT_INST_FOREACH_STATUS_OKAY(QUIRK_NRF_USBHS_DEFINE)

#endif /*DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_usbhs_nrf54l) */

/* Add next vendor quirks definition above this line */

#endif /* ZEPHYR_DRIVERS_USB_UDC_DWC2_VENDOR_QUIRKS_H */

/*
 * Copyright (c) 2023 Syslinbit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "uhc_common.h"

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>

#include <soc.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_pwr.h>

#include <string.h>
#include <stdint.h>
#include <stddef.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uhc_stm32, CONFIG_UHC_DRIVER_LOG_LEVEL);

/* TODO: Some STM32 seems to have multiple USB peripherals with different compat.
 * Must change this to support them.
 */
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otghs)
#define DT_DRV_COMPAT st_stm32_otghs
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otgfs)
#define DT_DRV_COMPAT st_stm32_otgfs
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_usb)
#define DT_DRV_COMPAT st_stm32_usb
#endif

#define DT_CLOCKS_PROP_MAX_LEN (2)

#define DT_INST_PHY(inst)         DT_INST_PHANDLE(inst, phys)
#define DT_PHY_COMPAT_EMBEDDED_FS usb_nop_xceiv
#define DT_PHY_COMPAT_EMBEDDED_HS st_stm32_usbphyc
#define DT_PHY_COMPAT_ULPI        usb_ulpi_phy

enum dt_speed {
	/* Values are fixed by maximum-speed enum values order in
	 * usb-controller.yaml dts binding file
	 */
	DT_ENUM_MAX_SPEED_LOW = 0,
	DT_ENUM_MAX_SPEED_FULL = 1,
	DT_ENUM_MAX_SPEED_HIGH = 2,
	DT_ENUM_MAX_SPEED_SUPER = 3,
};

#define DT_INST_MAX_SPEED(inst) DT_INST_ENUM_IDX_OR(inst, maximum_speed, DT_ENUM_MAX_SPEED_HIGH)

enum speed {
	SPEED_LOW,
	SPEED_FULL,
	SPEED_HIGH,
};

enum phy_type {
	PHY_EMBEDDED_FS,
#if defined(USB_OTG_HS_EMBEDDED_PHY)
	PHY_EMBEDDED_HS,
#endif
	PHY_EXTERNAL_ULPI,
};

struct uhc_stm32_data {
	HCD_HandleTypeDef hcd;
	struct k_event event_set;
};

struct uhc_stm32_config {
	uint32_t irq;
	enum phy_type phy;
	struct stm32_pclken clocks[DT_CLOCKS_PROP_MAX_LEN];
	enum dt_speed max_speed;
	struct pinctrl_dev_config *pcfg;
	struct gpio_dt_spec ulpi_reset_gpio;
	struct gpio_dt_spec vbus_enable_gpio;
};

typedef enum {
	UHC_STM32_DEVICE_CONNECTED = 0,
	UHC_STM32_BUS_RESETED = 1,
	UHC_STM32_DEVICE_DISCONNECTED = 2,
} uhc_stm32_event_t;

#define EVENT_BIT(event) (0x1 << event)

K_THREAD_STACK_DEFINE(drv_stack, CONFIG_UHC_STM32_DRV_THREAD_STACK_SIZE);
struct k_thread drv_stack_data;

static inline enum speed priv_get_current_speed(const struct device *dev)
{
	struct uhc_stm32_data *priv = uhc_get_private(dev);

	uint32_t hal_speed = HAL_HCD_GetCurrentSpeed(&(priv->hcd));

#if defined(USB_DRD_FS)
	switch (hal_speed) {
		case USB_DRD_SPEED_LS: {
			return SPEED_LOW;
		} break;
		case USB_DRD_SPEED_FS: {
			return SPEED_FULL;
		} break;
		default: {
			__ASSERT_NO_MSG(0);
		} break;
	}
#elif defined(USB_OTG_FS) || defined(USB_OTG_HS)
	switch (hal_speed) {
		case HPRT0_PRTSPD_LOW_SPEED: {
			return SPEED_LOW;
		} break;
		case HPRT0_PRTSPD_FULL_SPEED: {
			return SPEED_FULL;
		} break;
		case HPRT0_PRTSPD_HIGH_SPEED: {
			return SPEED_HIGH;
		} break;
		default: {
			__ASSERT_NO_MSG(0);
		} break;
	}
#endif

	return SPEED_LOW;
}

static void uhc_stm32_irq(const struct device *dev)
{
	struct uhc_stm32_data *priv = uhc_get_private(dev);

	HAL_HCD_IRQHandler((HCD_HandleTypeDef *)&priv->hcd);

	/* TODO: check WKUPINT to trigger UHC_EVT_RWUP */
}

void HAL_HCD_SOF_Callback(HCD_HandleTypeDef *hhcd)
{
	/* const struct device *dev = (const struct device*) hhcd->pData; */
}

void HAL_HCD_Connect_Callback(HCD_HandleTypeDef *hhcd)
{
	const struct device *dev = (const struct device *)hhcd->pData;
	struct uhc_stm32_data *priv = uhc_get_private(dev);

	/* Let the driver thread know a device is connected */
	k_event_post(&priv->event_set, EVENT_BIT(UHC_STM32_DEVICE_CONNECTED));
}

void HAL_HCD_Disconnect_Callback(HCD_HandleTypeDef *hhcd)
{
	const struct device *dev = (const struct device *)hhcd->pData;
	struct uhc_stm32_data *priv = uhc_get_private(dev);

	/* Let the driver thread know the device has been disconnected */
	k_event_post(&priv->event_set, EVENT_BIT(UHC_STM32_DEVICE_DISCONNECTED));
}

void HAL_HCD_PortEnabled_Callback(HCD_HandleTypeDef *hhcd)
{
	const struct device *dev = (const struct device *)hhcd->pData;
	struct uhc_stm32_data *priv = uhc_get_private(dev);

	/* Let the driver thread know the reset operation is done */
	k_event_post(&priv->event_set, EVENT_BIT(UHC_STM32_BUS_RESETED));
}

void HAL_HCD_PortDisabled_Callback(HCD_HandleTypeDef *hhcd)
{
	/* const struct device *dev = (const struct device*) hhcd->pData; */
}

void HAL_HCD_HC_NotifyURBChange_Callback(HCD_HandleTypeDef *hhcd, uint8_t chnum,
					 HCD_URBStateTypeDef urb_state)
{
	/* const struct device *dev = (const struct device*) hhcd->pData; */
}

static inline void priv_hcd_prepare(const struct device *dev)
{
	struct uhc_stm32_data *priv = uhc_get_private(dev);
	const struct uhc_stm32_config *config = dev->config;

	memset(&priv->hcd, 0, sizeof(priv->hcd));

	priv->hcd.Init.Host_channels = DT_INST_PROP(0, num_host_channels);
	priv->hcd.Init.dma_enable = 0;
	priv->hcd.Init.battery_charging_enable = DISABLE;
	priv->hcd.Init.use_external_vbus = DISABLE;
	priv->hcd.Init.vbus_sensing_enable = DISABLE;

	/* use pData to pass a reference to the device handler
	 * to be able to retrieve it from the HAL callbacks
	 */
	priv->hcd.pData = (void *)dev;

#if defined(USB_DRD_FS)
	priv->hcd.Instance = USB_DRD_FS;
	priv->hcd.Init.speed = HCD_SPEED_FULL;
#elif defined(USB_OTG_FS) || defined(USB_OTG_HS)
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otghs)
	priv->hcd.Instance = USB_OTG_HS;
	switch (config->phy) {
		case PHY_EMBEDDED_FS: {
			priv->hcd.Init.phy_itface = HCD_PHY_EMBEDDED;
		} break;
#if defined(USB_OTG_HS_EMBEDDED_PHY)
		case PHY_EMBEDDED_HS: {
			priv->hcd.Init.phy_itface = USB_OTG_HS_EMBEDDED_PHY;
		} break;
#endif
		case PHY_EXTERNAL_ULPI: {
			priv->hcd.Init.phy_itface = USB_OTG_ULPI_PHY;
			priv->hcd.Init.use_external_vbus = ENABLE;
		} break;
	}
	if (config->max_speed >= DT_ENUM_MAX_SPEED_HIGH) {
		priv->hcd.Init.speed = HCD_SPEED_HIGH;
	} else {
		priv->hcd.Init.speed = HCD_SPEED_FULL;
	}
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otgfs)
	priv->hcd.Instance = USB_OTG_FS;
	if (config->phy != PHY_EMBEDDED_FS) {
		LOG_DBG("Unsupported PHY."
			"USB controller will default to embedded full-speed only PHY.");
	}
	priv->hcd.Init.phy_itface = HCD_PHY_EMBEDDED;
	priv->hcd.Init.speed = HCD_SPEED_FULL;
#endif
#endif
}

static int uhc_stm32_lock(const struct device *dev)
{
	return uhc_lock_internal(dev, K_FOREVER);
}

static int uhc_stm32_unlock(const struct device *dev)
{
	return uhc_unlock_internal(dev);
}

/* TODO : Factorize if possible. priv_clock_enable and priv_clock_disable are just a slightly
 * modified version taken from udc_stm32.c (under Copyright (c) 2023 Linaro Limited)
 */
/***********************************************************************/
static int priv_clock_enable(const struct device *dev)
{
	const struct uhc_stm32_config *config = dev->config;
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	if (!device_is_ready(clk)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	if (DT_INST_NUM_CLOCKS(0) > 1) {
		if (clock_control_configure(clk, (clock_control_subsys_t *)&config->clocks[1],
					    NULL) != 0) {
			LOG_ERR("Could not select USB domain clock");
			return -EIO;
		}
	}

	if (clock_control_on(clk, (clock_control_subsys_t *)&config->clocks[0]) != 0) {
		LOG_ERR("Unable to enable USB clock");
		return -EIO;
	}

	uint32_t usb_clock_rate;

	if (clock_control_get_rate(clk, (clock_control_subsys_t *)&config->clocks[1],
				   &usb_clock_rate) != 0) {
		LOG_ERR("Failed to get USB domain clock rate");
		return -EIO;
	}

	if (usb_clock_rate != MHZ(48)) {
		LOG_ERR("USB Clock is not 48MHz (%d)", usb_clock_rate);
		return -ENOTSUP;
	}

/* Previous check won't work in case of F1. Add build time check */
#if defined(RCC_CFGR_OTGFSPRE) || defined(RCC_CFGR_USBPRE)

#if (MHZ(48) == CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) && !defined(STM32_PLL_USBPRE)
/* PLL output clock is set to 48MHz, it should not be divided */
#warning USBPRE/OTGFSPRE should be set in rcc node
#endif

#endif /* RCC_CFGR_OTGFSPRE / RCC_CFGR_USBPRE */

	return 0;
}

static int priv_clock_disable(const struct device *dev)
{
	const struct uhc_stm32_config *config = dev->config;
	const struct device *clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	if (clock_control_off(clk, (clock_control_subsys_t *)&config->clocks[0]) != 0) {
		LOG_ERR("Unable to disable USB clock");
		return -EIO;
	}

	return 0;
}
/**********************************************************************/

static int uhc_stm32_init(const struct device *dev)
{
	struct uhc_stm32_data *priv = uhc_get_private(dev);
	const struct uhc_stm32_config *config = dev->config;

	int err = 0;

	err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		LOG_ERR("USB pinctrl setup failed (%d)", err);
		return err;
	}

	if (config->vbus_enable_gpio.port) {
		if (!gpio_is_ready_dt(&config->vbus_enable_gpio)) {
			LOG_ERR("vbus enable gpio not ready");
			return -ENODEV;
		}
		err = gpio_pin_configure_dt(&config->vbus_enable_gpio, GPIO_OUTPUT_INACTIVE);
		if (err) {
			LOG_ERR("Failed to configure vbus enable gpio (%d)", err);
			return err;
		}
	}

	if (config->phy == PHY_EXTERNAL_ULPI && config->ulpi_reset_gpio.port) {
		if (!gpio_is_ready_dt(&config->ulpi_reset_gpio)) {
			LOG_ERR("ULPI reset gpio is not ready");
			return -ENODEV;
		}
		err = gpio_pin_configure_dt(&config->ulpi_reset_gpio, GPIO_OUTPUT_INACTIVE);
		if (err) {
			LOG_ERR("Failed to configure ULPI reset gpio (%d)", err);
			return err;
		}
	}

#if defined(CONFIG_SOC_SERIES_STM32U5X)
	/* TODO */
#elif defined(CONFIG_SOC_SERIES_STM32H5X)
	/* TODO : test if it works */
	LL_PWR_EnableUSBVoltageDetector();
	while (LL_PWR_IsActiveFlag_VDDUSB() == 0) {
		;
	}
#elif defined(CONFIG_SOC_SERIES_STM32H7X)
	/* TODO: useless with an ULPI phy ? */
	LL_PWR_EnableUSBVoltageDetector();
	while (LL_PWR_IsActiveFlag_USB() == 0) {
		;
	}
#endif

	err = priv_clock_enable(dev);
	if (err != 0) {
		LOG_ERR("Error enabling clock(s)");
		return -EIO;
	}

	priv_hcd_prepare(dev);

	HAL_StatusTypeDef status = HAL_HCD_Init(&(priv->hcd));

	if (status != HAL_OK) {
		LOG_ERR("HCD_Init failed, %d", (int)status);
		return -EIO;
	}

	return 0;
}

static int uhc_stm32_enable(const struct device *dev)
{
	struct uhc_stm32_data *priv = uhc_get_private(dev);
	const struct uhc_stm32_config *config = dev->config;

	irq_enable(config->irq);

	HAL_StatusTypeDef status = HAL_HCD_Start(&priv->hcd);

	if (status != HAL_OK) {
		LOG_ERR("HCD_Start failed (%d)", (int)status);
		return -EIO;
	}

	if (config->vbus_enable_gpio.port) {
		int err = gpio_pin_set_dt(&config->vbus_enable_gpio, 1);

		if (err) {
			LOG_ERR("Failed to enable vbus power (%d)", err);
			return err;
		}
	}

	return 0;
}

static int uhc_stm32_disable(const struct device *dev)
{
	struct uhc_stm32_data *priv = uhc_get_private(dev);
	const struct uhc_stm32_config *config = dev->config;

	if (config->vbus_enable_gpio.port) {
		int err = gpio_pin_set_dt(&config->vbus_enable_gpio, 0);

		if (err) {
			LOG_ERR("Failed to disable vbus power (%d)", err);
			return err;
		}
	}

	HAL_StatusTypeDef status = HAL_HCD_Stop(&priv->hcd);

	if (status != HAL_OK) {
		LOG_ERR("HCD_Stop failed (%d)", (int)status);
		return -EIO;
	}

	irq_disable(config->irq);

	return 0;
}

static int uhc_stm32_shutdown(const struct device *dev)
{
	struct uhc_stm32_data *priv = uhc_get_private(dev);
	const struct uhc_stm32_config *config = dev->config;

	HAL_StatusTypeDef status = HAL_HCD_DeInit(&priv->hcd);

	if (status != HAL_OK) {
		LOG_ERR("HAL_HCD_DeInit failed (%d)", (int)status);
		return -EIO;
	}

	int err = priv_clock_disable(dev);

	if (err) {
		LOG_ERR("Failed to disable USB clock (%d)", err);
		return err;
	}

	if (irq_is_enabled(config->irq)) {
		irq_disable(config->irq);
	}

	return 0;
}

static int uhc_stm32_bus_reset(const struct device *dev)
{
	struct uhc_stm32_data *priv = uhc_get_private(dev);

	HAL_HCD_ResetPort(&priv->hcd);

	return 0;
}

static int uhc_stm32_sof_enable(const struct device *dev)
{
	/* TODO */
	return 0;
}

static int uhc_stm32_bus_suspend(const struct device *dev)
{
	/* TODO */
	/* uhc_submit_event(dev, UHC_EVT_SUSPENDED, 0, NULL); */
	return 0;
}

static int uhc_stm32_bus_resume(const struct device *dev)
{
	/* TODO */
	/* uhc_submit_event(dev, UHC_EVT_RESUMED, 0, NULL); */
	return 0;
}

static int uhc_stm32_ep_enqueue(const struct device *dev, struct uhc_transfer *const xfer)
{
	return uhc_xfer_append(dev, xfer);
}

static int uhc_stm32_ep_dequeue(const struct device *dev, struct uhc_transfer *const xfer)
{
	/* TODO */
	return 0;
}

void uhc_stm32_thread(const struct device *dev)
{
	struct uhc_stm32_data *priv = uhc_get_private(dev);

	LOG_DBG("STM32 USB Host driver thread started");

	bool hs_negociation_reset_ongoing = false;

	while (true) {
		uint32_t events = k_event_wait(&priv->event_set, 0xFFFF, false, K_FOREVER);

		k_event_clear(&priv->event_set, events);

		if (events & EVENT_BIT(UHC_STM32_DEVICE_CONNECTED)) {
			/* ST USB Host code wait 200ms after a connection and before issuing a
			 * reset so do the same here
			 */
			k_msleep(200); /* TODO: check if useful */

			/* Perform a bus reset, this is normally done if device adverted itself
			 * as full-speed capable to perform high-speed negociation but seems
			 * required in any case according to RM0468 reference manual
			 */
			hs_negociation_reset_ongoing = true;
			uhc_stm32_bus_reset(dev);
		}

		if (events & EVENT_BIT(UHC_STM32_BUS_RESETED)) {
			if (hs_negociation_reset_ongoing) {
				hs_negociation_reset_ongoing = false;
			} else {
				/* Let higher level code know a reset occurred */
				uhc_submit_event(dev, UHC_EVT_RESETED, 0, NULL);
			}

			/* A reset occurred, retrieve negotiated speed to inform higher level
			 * code
			 */
			enum speed current_speed = priv_get_current_speed(dev);

			switch (current_speed) {
				case SPEED_LOW: {
					uhc_submit_event(dev, UHC_EVT_DEV_CONNECTED_LS, 0, NULL);
				} break;
				case SPEED_FULL: {
					uhc_submit_event(dev, UHC_EVT_DEV_CONNECTED_FS, 0, NULL);
				} break;
				case SPEED_HIGH: {
					uhc_submit_event(dev, UHC_EVT_DEV_CONNECTED_HS, 0, NULL);
				} break;
			}
		}

		if (events & EVENT_BIT(UHC_STM32_DEVICE_DISCONNECTED)) {
			/* Clear stuff if high-speed negociation was ongoing */
			hs_negociation_reset_ongoing = false;

			/* Let higher level code know a reset occurred */
			uhc_submit_event(dev, UHC_EVT_DEV_REMOVED, 0, NULL);
		}
	}
}

static const struct uhc_api uhc_stm32_api = {
	.lock = uhc_stm32_lock,
	.unlock = uhc_stm32_unlock,

	.init = uhc_stm32_init,
	.enable = uhc_stm32_enable,
	.disable = uhc_stm32_disable,
	.shutdown = uhc_stm32_shutdown,

	.bus_reset = uhc_stm32_bus_reset,
	.sof_enable = uhc_stm32_sof_enable,
	.bus_suspend = uhc_stm32_bus_suspend,
	.bus_resume = uhc_stm32_bus_resume,

	.ep_enqueue = uhc_stm32_ep_enqueue,
	.ep_dequeue = uhc_stm32_ep_dequeue,
};

PINCTRL_DT_INST_DEFINE(0);

static const struct uhc_stm32_config uhc0_config = {
	.irq = DT_INST_IRQN(0),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.vbus_enable_gpio = GPIO_DT_SPEC_INST_GET_OR(0, vbus_gpios, {0}),
#if DT_INST_NUM_CLOCKS(0) > DT_CLOCKS_PROP_MAX_LEN
#error Illegal number of elements defined in the "clocks" property in device tree
#else
	.clocks = STM32_DT_INST_CLOCKS(0),
#endif
#if DT_NODE_HAS_COMPAT(DT_INST_PHY(0), DT_PHY_COMPAT_EMBEDDED_FS)
	.max_speed = MIN(DT_INST_MAX_SPEED(0), DT_ENUM_MAX_SPEED_FULL),
	.phy = PHY_EMBEDDED_FS,
#elif DT_NODE_HAS_COMPAT(DT_INST_PHY(0), DT_PHY_COMPAT_ULPI)
	.max_speed = MIN(DT_INST_MAX_SPEED(0), DT_ENUM_MAX_SPEED_HIGH),
	.phy = PHY_EXTERNAL_ULPI,
	.ulpi_reset_gpio = GPIO_DT_SPEC_GET_OR(DT_PHANDLE(DT_DRV_INST(0), phys), reset_gpios, {0}),
#elif DT_NODE_HAS_COMPAT(DT_INST_PHY(0), DT_PHY_COMPAT_EMBEDDED_HS)
#if defined(USB_OTG_HS_EMBEDDED_PHY)
	.max_speed = MIN(DT_INST_MAX_SPEED(0), DT_ENUM_MAX_SPEED_HIGH),
	.phy = PHY_EMBEDDED_HS,
#else
#error USBPHYC is absent on this microncontroller.
#endif
#else
#error Unsupported or incompatible USB PHY defined in device tree.
#endif
};

static struct uhc_stm32_data uhc0_priv_data;

static struct uhc_data uhc0_data = {
	.mutex = Z_MUTEX_INITIALIZER(uhc0_data.mutex),
	.priv = &uhc0_priv_data,
};

static int uhc_stm32_driver_init0(const struct device *dev)
{
	struct uhc_data *data = dev->data;
	struct uhc_stm32_data *priv = uhc_get_private(dev);
	const struct uhc_stm32_config *config = dev->config;

	if (config->max_speed >= DT_ENUM_MAX_SPEED_HIGH) {
		data->caps.hs = 1;
	} else {
		data->caps.hs = 0;
	}

	k_event_init(&priv->event_set);
	k_thread_create(&drv_stack_data, drv_stack, K_THREAD_STACK_SIZEOF(drv_stack),
			(k_thread_entry_t)uhc_stm32_thread, (void *)dev, NULL, NULL, K_PRIO_COOP(2),
			0, K_NO_WAIT);
	k_thread_name_set(&drv_stack_data, "uhc_stm32");

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), uhc_stm32_irq, DEVICE_DT_INST_GET(0),
		    0);
	return 0;
}

DEVICE_DT_INST_DEFINE(0, uhc_stm32_driver_init0, NULL, &uhc0_data, &uhc0_config, POST_KERNEL,
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &uhc_stm32_api);

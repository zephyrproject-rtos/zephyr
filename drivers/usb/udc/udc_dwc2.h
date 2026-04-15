/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UDC_DWC2_H
#define ZEPHYR_DRIVERS_USB_UDC_DWC2_H

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/usb/udc.h>
#include <usb_dwc2_hw.h>

/* Required by DEVICE_MMIO_NAMED_* macros */
#define DEV_CFG(_dev) ((const struct udc_dwc2_config *)(_dev)->config)
#define DEV_DATA(_dev) ((struct udc_dwc2_data *)(udc_get_private(_dev)))

enum dwc2_suspend_type {
	DWC2_SUSPEND_NO_POWER_SAVING,
	DWC2_SUSPEND_HIBERNATION,
};

/* Vendor quirks per driver instance */
struct dwc2_vendor_quirks {
	/* Called at the beginning of udc_dwc2_init() */
	int (*init)(const struct device *dev);
	/* Called on udc_dwc2_enable() before the controller is initialized */
	int (*pre_enable)(const struct device *dev);
	/* Called on udc_dwc2_enable() after the controller is initialized */
	int (*post_enable)(const struct device *dev);
	/* Called at the end of udc_dwc2_disable() */
	int (*disable)(const struct device *dev);
	/* Called at the end of udc_dwc2_shutdown() */
	int (*shutdown)(const struct device *dev);
	/* Called at the end of IRQ handling */
	int (*irq_clear)(const struct device *dev);
	/* Called on driver pre-init */
	int (*caps)(const struct device *dev);
	/* Called while waiting for bits that require PHY to be clocked */
	int (*is_phy_clk_off)(const struct device *dev);
	/* Called after hibernation entry sequence */
	int (*post_hibernation_entry)(const struct device *dev);
	/* Called before hibernation exit sequence */
	int (*pre_hibernation_exit)(const struct device *dev);
};

/* Registers that have to be stored before Partial Power Down or Hibernation */
struct dwc2_reg_backup {
	uint32_t gotgctl;
	uint32_t gahbcfg;
	uint32_t gusbcfg;
	uint32_t gintmsk;
	uint32_t grxfsiz;
	uint32_t gnptxfsiz;
	uint32_t gi2cctl;
	uint32_t glpmcfg;
	uint32_t gdfifocfg;
	union {
		uint32_t dptxfsiz[15];
		uint32_t dieptxf[15];
	};
	uint32_t dcfg;
	uint32_t dctl;
	uint32_t diepmsk;
	uint32_t doepmsk;
	uint32_t daintmsk;
	uint32_t diepctl[16];
	uint32_t dieptsiz[16];
	uint32_t diepdma[16];
	uint32_t doepctl[16];
	uint32_t doeptsiz[16];
	uint32_t doepdma[16];
	uint32_t pcgcctl;
};

/* Driver private data per instance */
struct udc_dwc2_data {
	DEVICE_MMIO_NAMED_RAM(core);
	struct k_spinlock lock;
	struct k_thread thread_data;
	/* Main events the driver thread waits for */
	struct k_event drv_evt;
	/* Endpoint is considered disabled when there is no active transfer */
	struct k_event ep_disabled;
	/* Transfer triggers (IN on bits 0-15, OUT on bits 16-31) */
	atomic_t xfer_new;
	/* Finished transactions (IN on bits 0-15, OUT on bits 16-31) */
	atomic_t xfer_finished;
	struct dwc2_reg_backup backup;
	uint32_t ghwcfg1;
	uint32_t max_xfersize;
	uint32_t max_pktcnt;
	uint32_t tx_len[16];
	uint32_t rx_siz[16];
	/* Isochronous endpoint enabled (IN on bits 0-15, OUT on bits 16-31) */
	uint32_t iso_enabled;
	uint16_t iso_in_rearm;
	uint16_t iso_out_rearm;
	uint16_t ep_out_disable;
	uint16_t ep_out_stall;
	uint16_t txf_set;
	uint16_t pending_tx_flush;
	uint16_t dfifodepth;
	uint16_t rxfifo_depth;
	uint16_t max_txfifo_depth[16];
	uint16_t sof_num;
	/* Configuration flags */
	unsigned int dynfifosizing : 1;
	unsigned int bufferdma : 1;
	unsigned int syncrst : 1;
	/* Defect workarounds */
	unsigned int wa_essregrestored : 1;
	/* Runtime state flags */
	unsigned int hibernated : 1;
	unsigned int enumdone : 1;
	unsigned int enumspd : 2;
	unsigned int ignore_ep0_nakeff : 1;
	enum dwc2_suspend_type suspend_type;
	/* Number of endpoints including control endpoint */
	uint8_t numdeveps;
	/* Number of IN endpoints including control endpoint */
	uint8_t ineps;
	/* Number of OUT endpoints including control endpoint */
	uint8_t outeps;
	uint8_t setup[8];
};

/* Driver configuration per instance */
struct udc_dwc2_config {
	DEVICE_MMIO_NAMED_ROM(core);
	size_t num_in_eps;
	size_t num_out_eps;
	struct udc_ep_config *ep_cfg_in;
	struct udc_ep_config *ep_cfg_out;
	/* Pointer to pin control configuration or NULL */
	struct pinctrl_dev_config *const pcfg;
	/* Pointer to vendor quirks or NULL */
	const struct dwc2_vendor_quirks *const quirks;
	void (*make_thread)(const struct device *dev);
	void (*irq_enable_func)(const struct device *dev);
	void (*irq_disable_func)(const struct device *dev);
	uint32_t ghwcfg1;
	uint32_t ghwcfg2;
	uint32_t ghwcfg4;
};

static inline struct usb_dwc2_reg *dwc2_get_base(const struct device *dev)
{
	return (struct usb_dwc2_reg *)DEVICE_MMIO_NAMED_GET(dev, core);
}

#if DT_HAS_COMPAT_STATUS_OKAY(espressif_esp32_usb_otg)
#include "udc_dwc2_esp32_usb_otg.h"
#endif
#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_usbhs)
#include "udc_dwc2_nrf_usbhs.h"
#endif
#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_usbhs_nrf54l)
#include "udc_dwc2_nrf_usbhs_nrf54l.h"
#endif
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32f4_fsotg)
#include "udc_dwc2_stm32f4_fsotg.h"
#endif

#define UDC_DWC2_VENDOR_QUIRK_GET(n)						\
	COND_CODE_1(DT_NODE_VENDOR_HAS_IDX(DT_DRV_INST(n), 1),			\
		    (&dwc2_vendor_quirks_##n),					\
		    (NULL))

#define DWC2_QUIRK_FUNC_DEFINE(fname)						\
static inline int dwc2_quirk_##fname(const struct device *dev)			\
{										\
	const struct udc_dwc2_config *const config = dev->config;		\
	const struct dwc2_vendor_quirks *const quirks =				\
		COND_CODE_1(IS_EQ(DT_NUM_INST_STATUS_OKAY(snps_dwc2), 1),	\
			(UDC_DWC2_VENDOR_QUIRK_GET(0); ARG_UNUSED(config);),	\
			(config->quirks;))					\
										\
	if (quirks != NULL && quirks->fname != NULL) {				\
		return quirks->fname(dev);					\
	}									\
										\
	return 0;								\
}

DWC2_QUIRK_FUNC_DEFINE(init)
DWC2_QUIRK_FUNC_DEFINE(pre_enable)
DWC2_QUIRK_FUNC_DEFINE(post_enable)
DWC2_QUIRK_FUNC_DEFINE(disable)
DWC2_QUIRK_FUNC_DEFINE(shutdown)
DWC2_QUIRK_FUNC_DEFINE(irq_clear)
DWC2_QUIRK_FUNC_DEFINE(caps)
DWC2_QUIRK_FUNC_DEFINE(is_phy_clk_off)
DWC2_QUIRK_FUNC_DEFINE(post_hibernation_entry)
DWC2_QUIRK_FUNC_DEFINE(pre_hibernation_exit)

#endif /* ZEPHYR_DRIVERS_USB_UDC_DWC2_H */

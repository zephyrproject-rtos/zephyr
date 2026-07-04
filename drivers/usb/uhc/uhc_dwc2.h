/*
 * Copyright (c) 2026 Roman Leonov <jam_roma@yahoo.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UHC_DWC2_H
#define ZEPHYR_DRIVERS_USB_UHC_DWC2_H

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/usb/uhc.h>
#include <usb_dwc2_hw.h>
#include "uhc_common.h"

#define MAX_CHANNELS	16

/* Required by DEVICE_MMIO_NAMED_* macros */
#define DEV_CFG(_dev)	((const struct uhc_dwc2_config *)(_dev)->config)
#define DEV_DATA(_dev)	((struct uhc_dwc2_data *)(uhc_get_private(_dev)))

static inline struct usb_dwc2_reg *uhc_dwc2_get_base(const struct device *dev);

/* Vendor quirks per driver instance */
struct uhc_dwc2_vendor_quirks {
	/* Called on uhc_dwc2_preinit() after the thread is initialized */
	int (*post_preinit)(const struct device *dev);
	/* Called at the beginning of uhc_dwc2_init(), before controller is initialized  */
	int (*pre_init)(const struct device *dev);
	/* Called on uhc_dwc2_enable() before enable the port power */
	int (*pre_enable)(const struct device *dev);
	/* Called on uhc_dwc2_enable() after enable the port power */
	int (*post_enable)(const struct device *dev);
	/* Called at the end of uhc_dwc2_disable() */
	int (*disable)(const struct device *dev);
	/* Called at the end of uhc_dwc2_shutdown() */
	int (*shutdown)(const struct device *dev);
	/* Called at the end of IRQ handling */
	int (*irq_clear)(const struct device *dev);
	/* Called while waiting for bits that require PHY to be clocked */
	int (*is_phy_clk_off)(const struct device *dev);
	/* Called to translate a CPU-side address into the bus/DMA address expected by HCDMA*/
	int (*dma_addr_xlate)(const struct device *dev);
};

/* TODO: rename to pipe_data and expand with udev */
struct uhc_dwc2_channel_data {
	uint8_t next_pid;
};

struct uhc_dwc2_channel {
	/* Index of the channel */
	uint8_t index;
	/* Accessed in ISR. Number of error to track errors for transactions. */
	uint8_t error_count;
	/* Set while a halt was initiated to cancel the transfer. */
	bool halt_cancel;
	/* Used to save pending completion bits, while waiting channel to be halted. */
	uint32_t hcint_cplt_pending;
	/* Cached pointer to channel registers */
	const struct usb_dwc2_host_chan *regs;
	/* Pointer to the transfer */
	struct uhc_transfer *xfer;
	/* Channel events */
	atomic_t events;
	/* Channel transfer helpers */
	uint32_t length;
	/* Channel transfer data helpers */
	struct uhc_dwc2_channel_data *data;
};

struct uhc_dwc2_data {
	DEVICE_MMIO_NAMED_RAM(core);
	struct k_thread thread;
	/* Event bitmask the driver thread waits for */
	struct k_event events;
	/* Semaphore used to indicate that port is enabled */
	struct k_sem sem_port_enabled;
	/* Port channels */
	struct uhc_dwc2_channel ch[MAX_CHANNELS];
	/* Channels specific transfer related parameters */
	struct uhc_dwc2_channel_data ch_data[2][MAX_CHANNELS];
	/* Number of channels, available on the hardware */
	uint32_t numhstchnl;
	/* Number of channels currently not claimed */
	uint32_t free_chs;
	/* Root port does the debounce of connection/disconnection */
	bool debouncing;
	/* Root port has an attached device */
	bool has_device;
	/* DMA address for translation */
	uint32_t dma_addr;
};

/* Driver configuration per instance */
struct uhc_dwc2_config {
	/* Pointer to base address of DWC_OTG registers */
	DEVICE_MMIO_NAMED_ROM(core);
	/* Vendor specific quirks */
	const struct uhc_dwc2_vendor_quirks *const quirks;
	void *quirk_data;
	const void *quirk_config;
	/* Pointer to the stack used by the driver thread */
	k_thread_stack_t *stack;
	/* Size of the stack used by the driver thread */
	size_t stack_size;

	void (*irq_enable_func)(const struct device *dev);
	void (*irq_disable_func)(const struct device *dev);
};

#define UHC_DWC2_QUIRK_CONFIG(dev)						\
	(((const struct uhc_dwc2_config *)dev->config)->quirk_config)

#define UHC_DWC2_QUIRK_DATA(dev)						\
	(((const struct uhc_dwc2_config *)dev->config)->quirk_data)

#if DT_HAS_COMPAT_STATUS_OKAY(espressif_esp32_usb_otg)
#include "uhc_dwc2_esp32_usb_otg.h"
#endif
#if DT_HAS_COMPAT_STATUS_OKAY(brcm_bcm2835_usb)
#include "uhc_dwc2_bcm2835_usb.h"
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_usbhs_nrf54l)
#include "uhc_dwc2_nrf_usbhs_nrf54l.h"
#endif

#define UHC_DWC2_VENDOR_QUIRK_GET(n)						\
	COND_CODE_1(DT_NODE_VENDOR_HAS_IDX(DT_DRV_INST(n), 1),			\
			(&uhc_dwc2_vendor_quirks_##n),				\
			(NULL))

#define DWC2_QUIRK_FUNC_DEFINE(fname)						\
static inline int uhc_dwc2_quirk_##fname(const struct device *const dev)	\
{										\
	const struct uhc_dwc2_config *const config = dev->config;		\
	const struct uhc_dwc2_vendor_quirks *const quirks = config->quirks;	\
										\
	if (quirks != NULL && quirks->fname != NULL) {				\
		return config->quirks->fname(dev);				\
	}									\
	return 0;								\
}

DWC2_QUIRK_FUNC_DEFINE(post_preinit)
DWC2_QUIRK_FUNC_DEFINE(pre_init)
DWC2_QUIRK_FUNC_DEFINE(pre_enable)
DWC2_QUIRK_FUNC_DEFINE(post_enable)
DWC2_QUIRK_FUNC_DEFINE(disable)
DWC2_QUIRK_FUNC_DEFINE(shutdown)
DWC2_QUIRK_FUNC_DEFINE(irq_clear)
DWC2_QUIRK_FUNC_DEFINE(is_phy_clk_off)
DWC2_QUIRK_FUNC_DEFINE(dma_addr_xlate)

#endif /* ZEPHYR_DRIVERS_USB_UHC_DWC2_H */

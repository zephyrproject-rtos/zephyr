/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Shared Synopsys DWC3 + xHCI host private definitions.
 */

#ifndef ZEPHYR_USB_XHCI_DWC3_PRIV_H
#define ZEPHYR_USB_XHCI_DWC3_PRIV_H

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/usb/uhc.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/util.h>

#include <zephyr/sys/device_mmio.h>

#include "dwc3_xilinx.h"
#include "xhci_plat.h"
#include "xhci_hw.h"
#include "xhci_ring.h"
#include "xhci_bulk.h"

/* Required by DEVICE_MMIO_NAMED_* macros */
#define DEV_CFG(_dev)  ((const struct uhc_dwc3_config *)(_dev)->config)
#define DEV_DATA(_dev) ((struct uhc_dwc3_data *)((struct uhc_data *)(_dev)->data)->priv)

#define XHCI_CMD_RING_SIZE   64
#define XHCI_EVT_RING_SIZE   256
#define XHCI_EP0_RING_SIZE   64
#define XHCI_BULK_RING_SIZE  32
#define XHCI_MAX_DEVSLOTS    8
#define XHCI_MAX_SCRATCHPADS 8

#define UHC_DWC3_BULK_IN_DATABUF_SZ   512U
#define UHC_DWC3_BULK_XFER_TIMEOUT_MS 5000U

struct uhc_dwc3_config {
	DEVICE_MMIO_NAMED_ROM(core);
	DEVICE_MMIO_NAMED_ROM(usb2_wrapper);
	bool usb2_wrapper_present;
	uint8_t boot_probe_ports_min;
	uint8_t usb2_poll_port;
	struct dwc3_xilinx_config xilinx;
	struct xhci_plat_config xhci_plat;
	void (*irq_enable_func)(const struct device *dev);
	void (*irq_disable_func)(const struct device *dev);
};

struct uhc_dwc3_data {
	DEVICE_MMIO_NAMED_RAM(core);
	DEVICE_MMIO_NAMED_RAM(usb2_wrapper);
	/* xHCI register bases (computed at init) */
	mm_reg_t op_base;
	mm_reg_t rt_base;
	mm_reg_t db_base;
	uint32_t ctx_bytes;
	uint32_t max_slots;
	uint32_t max_ports;

	/* DCBAA: device context base address array */
	uint64_t dcbaa[XHCI_MAX_DEVSLOTS + 1] __aligned(64);

	/* Command ring */
	struct xhci_trb cmd_trbs[XHCI_CMD_RING_SIZE] __aligned(64);
	struct xhci_ring cmd_ring;

	/* Event ring (interrupter 0) */
	struct xhci_trb evt_trbs[XHCI_EVT_RING_SIZE] __aligned(64);
	struct xhci_ring evt_ring;
	struct xhci_erst_entry erst[1] __aligned(64);

	/* EP0 transfer ring for addressed device */
	struct xhci_trb ep0_trbs[XHCI_EP0_RING_SIZE] __aligned(64);
	struct xhci_ring ep0_ring;

	struct xhci_trb ep_bulk_trbs[32][XHCI_BULK_RING_SIZE] __aligned(64);
	struct xhci_ring ep_bulk_rings[32];

	uint8_t dev_ctx[2048] __aligned(64);
	uint8_t input_ctx[2048] __aligned(64);

	uint8_t slot_id;
	uint8_t port_speed;
	uint16_t ep0_max_packet;
	bool root_connect_submitted;
	uint8_t root_port;
	bool write_64_hi_lo;

	struct k_sem cmd_sem;
	uint32_t cmd_comp_code;
	uint32_t cmd_slot_id;

	struct k_sem xfer_sem;
	struct uhc_transfer *ep0_active_xfer;
	int xfer_result;
	uint32_t xfer_length;
	uint32_t xfer_comp_code;

	struct uhc_transfer *bulk_active_xfer[32];
	int bulk_xfer_result[32];
	uint32_t bulk_xfer_length[32];
	uint32_t bulk_xfer_comp_code[32];
	uint64_t bulk_expect_ioc_trb_phys[32];
	uint8_t bulk_td_trb_count[32];

	struct uhc_dwc3_bulk_urb {
		struct uhc_transfer *xfer;
		uint32_t req_len;
		uint32_t trb_dma_len;
		bool dir_in;
		bool in_staging;
		struct xhci_td td;
	} bulk_urb[32];

	struct k_mutex evt_mutex;
	struct k_work event_work;
	const struct device *dev;

	uint32_t max_scratchpad;
	uint64_t scratchpad_table[XHCI_MAX_SCRATCHPADS] __aligned(64);
	uint8_t scratchpad_bufs[XHCI_MAX_SCRATCHPADS][4096] __aligned(4096);

	bool steady_after_configure_ep;
	uint8_t bulk_out_staging[CONFIG_UHC_DWC3_BULK_OUT_STAGING_BUFSZ] __aligned(64);
	uint8_t bulk_in_databuf[UHC_DWC3_BULK_IN_DATABUF_SZ] __aligned(64);
	uint8_t bulk_in_smallbuf[64] __aligned(64);
};

static inline mm_reg_t uhc_dwc3_core_mmio(const struct device *dev)
{
	return DEVICE_MMIO_NAMED_GET(dev, core);
}

static inline mm_reg_t uhc_dwc3_usb2_wrapper_mmio(const struct device *dev)
{
	const struct uhc_dwc3_config *cfg = dev->config;

	if (cfg == NULL || !cfg->usb2_wrapper_present) {
		return 0U;
	}

	return DEVICE_MMIO_NAMED_GET(dev, usb2_wrapper);
}

#endif /* ZEPHYR_USB_XHCI_DWC3_PRIV_H */

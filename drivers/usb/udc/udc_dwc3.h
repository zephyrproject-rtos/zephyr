/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UDC_DWC3_H
#define ZEPHYR_DRIVERS_USB_UDC_DWC3_H

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <usb_dwc3_hw.h>

/* Required by DEVICE_MMIO_NAMED_* macros */
#define DEV_CFG(_dev)  ((const struct udc_dwc3_config *)(_dev)->config)
#define DEV_DATA(_dev) ((struct udc_dwc3_data *)(udc_get_private(_dev)))

/* Bus speeds, in the order of the Linux enum usb_device_speed */
enum dwc3_speed {
	DWC3_SPEED_UNKNOWN,
	DWC3_SPEED_LOW,
	DWC3_SPEED_FULL,
	DWC3_SPEED_HIGH,
	DWC3_SPEED_SUPER,
	DWC3_SPEED_SUPER_PLUS,
};

/* USB 2.0 PHY interface, from the phy_type property, in its enum order */
enum dwc3_hsphy_mode {
	DWC3_HSPHY_MODE_UNKNOWN,
	DWC3_HSPHY_MODE_UTMI,
	DWC3_HSPHY_MODE_UTMIW,
	DWC3_HSPHY_MODE_ULPI,
	DWC3_HSPHY_MODE_SERIAL,
	DWC3_HSPHY_MODE_HSIC,
};

/* The snps,hsphy_interface property, in its enum order */
enum dwc3_hsphy_interface {
	DWC3_HSPHY_INTERFACE_UNSET,
	DWC3_HSPHY_INTERFACE_UTMI,
	DWC3_HSPHY_INTERFACE_ULPI,
};

/*
 * Devicetree properties of the controller. The fields are named after those of
 * the Linux struct dwc3 they correspond to, and are used where Linux uses them.
 */
struct dwc3_props {
	const uint32_t *incr_burst;
	const uint8_t *reserved_eps;
	uint32_t fladj;
	uint32_t ref_clk_per;
	enum dwc3_speed maximum_speed;
	enum dwc3_hsphy_mode hsphy_mode;
	enum dwc3_hsphy_interface hsphy_interface;
	uint8_t incr_burst_len;
	uint8_t reserved_eps_len;
	uint8_t lpm_nyet_threshold;
	uint8_t hird_threshold;
	uint8_t tx_de_emphasis;
	uint8_t rx_thr_num_pkt;
	uint8_t rx_max_burst;
	uint8_t tx_thr_num_pkt;
	uint8_t tx_max_burst;
	unsigned int has_lpm_erratum: 1;
	unsigned int is_utmi_l1_suspend: 1;
	unsigned int usb2_gadget_lpm_disable: 1;
	unsigned int disable_scramble_quirk: 1;
	unsigned int u2exit_lfps_quirk: 1;
	unsigned int u2ss_inp3_quirk: 1;
	unsigned int req_p1p2p3_quirk: 1;
	unsigned int del_p1p2p3_quirk: 1;
	unsigned int del_phy_power_chg_quirk: 1;
	unsigned int lfps_filter_quirk: 1;
	unsigned int rx_detect_poll_quirk: 1;
	unsigned int dis_u3_susphy_quirk: 1;
	unsigned int dis_u2_susphy_quirk: 1;
	unsigned int dis_enblslpm_quirk: 1;
	unsigned int dis_u1_entry_quirk: 1;
	unsigned int dis_u2_entry_quirk: 1;
	unsigned int dis_rxdet_inp3_quirk: 1;
	unsigned int dis_u2_freeclk_exists_quirk: 1;
	unsigned int dis_del_phy_power_chg_quirk: 1;
	unsigned int dis_tx_ipgap_linecheck_quirk: 1;
	unsigned int resume_hs_terminations: 1;
	unsigned int ulpi_ext_vbus_drv: 1;
	unsigned int parkmode_disable_ss_quirk: 1;
	unsigned int parkmode_disable_hs_quirk: 1;
	unsigned int gfladj_refclk_lpm_sel: 1;
	unsigned int tx_de_emphasis_quirk: 1;
	unsigned int dis_metastability_quirk: 1;
};

/*
 * Vendor hooks per driver instance. The PHY hooks run where Linux calls the
 * generic PHY operations from dwc3_core_init() and dwc3_core_exit().
 */
struct dwc3_vendor_quirks {
	/* PHY initialization, after the PHY interface setup */
	int (*phy_init)(const struct device *dev);
	/* PHY power on, before the event buffer setup */
	int (*phy_power_on)(const struct device *dev);
	/* PHY power off, on shutdown */
	int (*phy_power_off)(const struct device *dev);
	/* PHY release, on shutdown */
	int (*phy_exit)(const struct device *dev);
};

struct dwc3_clk {
	const struct device *dev;
	clock_control_subsys_t subsys;
};

/* Physical endpoint flags, named after the Linux DWC3_EP_* ones */
#define DWC3_EP_ENABLED              BIT(0)
#define DWC3_EP_STALL                BIT(1)
#define DWC3_EP_TRANSFER_STARTED     BIT(2)
#define DWC3_EP_END_TRANSFER_PENDING BIT(3)
#define DWC3_EP_DELAY_START          BIT(4)
#define DWC3_EP_RESOURCE_ALLOCATED   BIT(5)
#define DWC3_EP_DELAY_STOP           BIT(6)
#define DWC3_EP_PENDING_CLEAR_STALL  BIT(7)
/* The TRB ring buffers go back to the stack once the delayed stop is done */
#define DWC3_EP_PENDING_CANCEL       BIT(8)

/*
 * Physical endpoint. Number 0 is EP0 OUT, number 1 EP0 IN, then the OUT and IN
 * directions of each endpoint follow, so that the number is (ep_idx << 1) | in.
 */
struct dwc3_ep {
	/* Zephyr endpoint configuration, NULL when the endpoint is not used */
	struct udc_ep_config *cfg;
	/* TRB ring of a non-control endpoint, its last TRB links to the first */
	struct usb_dwc3_trb *trb_pool;
	/* Buffer completed by each TRB of the ring, set on its last TRB only */
	struct net_buf **trb_buf;
	uint32_t flags;
	/* Index of the next TRB to prepare and of the oldest one in flight */
	uint16_t trb_enqueue;
	uint16_t trb_dequeue;
	/* Number of TRBs prepared and not reclaimed yet */
	uint16_t trbs_used;
	uint8_t number;
	uint8_t resource_index;
};

/* Control transfer phases, as in Linux */
enum dwc3_ep0_state {
	EP0_UNCONNECTED,
	EP0_SETUP_PHASE,
	EP0_DATA_PHASE,
	EP0_STATUS_PHASE,
};

/* Next control endpoint event the driver expects, as in Linux */
enum dwc3_ep0_next {
	DWC3_EP0_UNKNOWN,
	DWC3_EP0_COMPLETE,
	DWC3_EP0_NRDY_DATA,
	DWC3_EP0_NRDY_STATUS,
};

/* Driver private data per instance */
struct udc_dwc3_data {
	DEVICE_MMIO_NAMED_RAM(base);
	struct k_thread thread_data;
	/* Events the driver thread waits for */
	struct k_event drv_evt;
	/* Physical endpoints with new buffers to start */
	atomic_t xfer_new;
	/* Cached GHWPARAMS0 to GHWPARAMS9 */
	uint32_t hwparams[10];
	/* GSNPSID[31:16], and the revision of that IP */
	uint32_t ip;
	uint32_t revision;
	uint32_t version_type;
	/* Read position in the event buffer, in bytes */
	uint32_t lpos;
	uint16_t imod_interval;
	/* Length of the data stage of the current control transfer */
	uint16_t ep0_wlength;
	/* Physical endpoints of the controller */
	uint8_t num_eps;
	/* Enabled non-control endpoints */
	uint8_t num_active_eps;
	/* Fastest speed in use, after dwc3_check_params() */
	enum dwc3_speed maximum_speed;
	/* Connection speed from DSTS.CONNECTSPD after Connect Done */
	enum dwc3_speed speed;
	uint8_t link_state;
	enum dwc3_ep0_state ep0state;
	enum dwc3_ep0_next ep0_next_event;
	/* Physical endpoint the status stage runs on */
	uint8_t ep0_status_epnum;
	unsigned int started: 1;
	unsigned int connected: 1;
	unsigned int pullups_connected: 1;
	unsigned int suspended: 1;
	unsigned int three_stage_setup: 1;
	unsigned int ep0_expect_in: 1;
	unsigned int ep0_bounced: 1;
	unsigned int setup_packet_pending: 1;
	/* XferNotReady(Status) seen, the status stage waits for its buffer */
	unsigned int ep0_status_pending: 1;
	unsigned int linksts_evts: 1;
};

/* Driver configuration per instance */
struct udc_dwc3_config {
	DEVICE_MMIO_NAMED_ROM(base);
	size_t num_of_eps;
	struct udc_ep_config *ep_cfg_in;
	struct udc_ep_config *ep_cfg_out;
	/* Physical endpoints, two per endpoint number */
	struct dwc3_ep *eps;
	/* TRB rings and their buffer slots, CONFIG_UDC_DWC3_TRB_NUM per endpoint */
	struct usb_dwc3_trb *trb_pools;
	struct net_buf **trb_bufs;
	/* Event buffer, in non-cacheable memory */
	uint32_t *evt_buf;
	/* Control endpoint TRBs, SETUP buffer and bounce buffer */
	struct usb_dwc3_trb *ep0_trb;
	uint8_t *setup_buf;
	uint8_t *bounce;
	const struct dwc3_props *props;
	const struct dwc3_clk *clks;
	const struct dwc3_clk ref_clk;
	const struct dwc3_clk susp_clk;
	const struct reset_dt_spec *resets;
	uint8_t num_clks;
	uint8_t num_resets;
	/* Pointer to vendor quirks or NULL, and to their per-instance data */
	const struct dwc3_vendor_quirks *quirks;
	const void *quirk_data;
	void (*make_thread)(const struct device *dev);
	void (*irq_enable_func)(const struct device *dev);
	void (*irq_disable_func)(const struct device *dev);
};

/* Vendor quirks and their data per instance, none so far */
#define UDC_DWC3_VENDOR_QUIRK_GET(n)      NULL
#define UDC_DWC3_VENDOR_QUIRK_DATA_GET(n) NULL

#endif /* ZEPHYR_DRIVERS_USB_UDC_DWC3_H */

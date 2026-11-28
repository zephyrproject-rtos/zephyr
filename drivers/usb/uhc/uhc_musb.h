/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UHC_UHC_MUSB_H
#define ZEPHYR_DRIVERS_USB_UHC_UHC_MUSB_H

#include <stdint.h>
#include <stddef.h>

#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/usb/uhc.h>

#include "uhc_common.h"

#include <usb_musb_hw.h>

/*
 * Register accessors. Endpoint control and status registers are banked and
 * select the endpoint programmed in the INDEX register.
 */
static inline uint8_t musb_read8(volatile uint8_t *reg)
{
	return sys_read8((mem_addr_t)reg);
}

static inline uint16_t musb_read16(volatile uint16_t *reg)
{
	return sys_read16((mem_addr_t)reg);
}

static inline void musb_write8(volatile uint8_t *reg, uint8_t value)
{
	sys_write8(value, (mem_addr_t)reg);
}

static inline void musb_write16(volatile uint16_t *reg, uint16_t value)
{
	sys_write16(value, (mem_addr_t)reg);
}

static inline void musb_set8(volatile uint8_t *reg, uint8_t bits)
{
	musb_write8(reg, musb_read8(reg) | bits);
}

static inline void musb_clear8(volatile uint8_t *reg, uint8_t bits)
{
	musb_write8(reg, musb_read8(reg) & ~bits);
}

static inline void musb_set16(volatile uint16_t *reg, uint16_t bits)
{
	musb_write16(reg, musb_read16(reg) | bits);
}

static inline mem_addr_t musb_fifo_addr(struct usb_musb_reg *const base, const uint8_t ep)
{
	return (mem_addr_t)&base->fifo[ep];
}

/* Vendor quirks per driver instance */
struct uhc_musb_vendor_quirks {
	/*
	 * Called on uhc_musb_init() before the core registers are accessed.
	 * Must leave the MUSB core powered, clocked, out of reset and forced
	 * into the host (A-device) role.
	 */
	int (*init)(const struct device *dev);
	/*
	 * Called on uhc_musb_enable() once the core interrupt enables are
	 * programmed, before VBUS is checked and the host session is started.
	 */
	int (*post_enable)(const struct device *dev);
	/* Called on uhc_musb_disable() after the IRQ line is disabled */
	int (*disable)(const struct device *dev);
	/* Called on uhc_musb_shutdown() */
	int (*shutdown)(const struct device *dev);
	/*
	 * Called from the ISR after the MUSB interrupt status registers have
	 * been read, to acknowledge the vendor wrapper interrupts.
	 */
	int (*irq_clear)(const struct device *dev);
};

struct uhc_musb_config {
	/* Base address of the MUSB core registers */
	mem_addr_t base;
	/* Base address of the vendor wrapper registers, 0 if there is none */
	mem_addr_t wrapper;
	/* Number of hardware endpoints, including EP0 */
	size_t num_of_eps;
	k_thread_stack_t *thread_stk;
	size_t thread_stk_sz;
	int thread_priority;
	int speed_idx;
	/* Optional GPIO that switches VBUS on the downstream port */
	struct gpio_dt_spec vbus_gpio;
	void (*irq_enable_func)(const struct device *dev);
	void (*irq_disable_func)(const struct device *dev);
	/* Pointer to vendor quirks or NULL */
	const struct uhc_musb_vendor_quirks *const quirks;
};

static inline struct usb_musb_reg *musb_get_base(const struct device *dev)
{
	const struct uhc_musb_config *const config = dev->config;

	return (struct usb_musb_reg *)config->base;
}

/* DT maximum-speed enum index for high-speed (see usb-controller.yaml) */
#define MUSB_HOST_SPEED_IDX_HS 2

/* EP0 control endpoint index */
#define MUSB_HOST_EP0_IDX 0U

/* SETUP packet length */
#define MUSB_HOST_SETUP_PACKET_SIZE 8U

/* FIFO port width: reads pop one 32-bit word per access */
#define MUSB_HOST_FIFO_WORD_SIZE 4U

/* Largest number of hardware endpoints a MUSB core can have, including EP0 */
#define MUSB_HOST_EP_MAX 16U

/* Worker-thread event flags */
#define MUSB_HOST_EVT_USB BIT(0) /* INTRUSB bus event pending        */
#define MUSB_HOST_EVT_EP0 BIT(1) /* EP0 transaction completed        */
#define MUSB_HOST_EVT_XFR BIT(2) /* Transfer enqueued or dequeued    */
#define MUSB_HOST_EVT_EPX BIT(3) /* EP1-15 TX/RX activity            */

/* Root-port state bits */
#define MUSB_HOST_PORT_CONNECTED 0

/* Per hardware endpoint (EP1-15) host-channel state */
struct uhc_musb_hep {
	/* Transfer in progress on this channel */
	struct uhc_transfer *xfer;
	/* MUSB_TYPE_PROTOCOL_* of the configured endpoint */
	uint8_t proto;
	/* Channel programmed and FIFO RAM allocated for this connection */
	bool configured;
	/* Channel stopped by a cancelled transfer, re-arm on next start */
	bool halted;
};

struct uhc_musb_data {
	struct k_thread thread_data;
	struct k_event events;
	/* Interrupt causes latched by the ISR (INTRUSB is read-to-clear) */
	atomic_t intrusb_pending;
	/* EP1-15 TX/RX completion bits latched by the ISR */
	atomic_t intrtx_pending;
	atomic_t intrrx_pending;
	/* Transfer currently owning EP0 */
	struct uhc_transfer *active_xfer;
	/* Per-direction hardware endpoint channels, indexed by EP number */
	struct uhc_musb_hep hep_in[MUSB_HOST_EP_MAX];
	struct uhc_musb_hep hep_out[MUSB_HOST_EP_MAX];
	/* Next free FIFO byte offset for EP1-15 allocation */
	uint16_t fifo_next;
	/* Connected device speed (valid once MUSB_HOST_PORT_CONNECTED is set) */
	enum usb_device_speed speed;
	atomic_t port_state;
};

#if DT_HAS_COMPAT_STATUS_OKAY(microchip_usbhs_g1_host)
#include "uhc_musb_mchp_usbhs_g1.h"
#endif

#define UHC_MUSB_VENDOR_QUIRK_GET(n)						\
	COND_CODE_1(DT_NODE_VENDOR_HAS_IDX(DT_DRV_INST(n), 1),			\
		    (&uhc_musb_vendor_quirks_##n),				\
		    (NULL))

#define UHC_MUSB_QUIRK_FUNC_DEFINE(fname)					\
static inline int uhc_musb_quirk_##fname(const struct device *dev)		\
{										\
	const struct uhc_musb_config *const config = dev->config;		\
	const struct uhc_musb_vendor_quirks *const quirks =			\
		COND_CODE_1(IS_EQ(DT_NUM_INST_STATUS_OKAY(mentor_musb_host), 1),\
			(UHC_MUSB_VENDOR_QUIRK_GET(0); ARG_UNUSED(config);),	\
			(config->quirks;))					\
										\
	if (quirks != NULL && quirks->fname != NULL) {				\
		return quirks->fname(dev);					\
	}									\
										\
	return 0;								\
}

UHC_MUSB_QUIRK_FUNC_DEFINE(init)
UHC_MUSB_QUIRK_FUNC_DEFINE(post_enable)
UHC_MUSB_QUIRK_FUNC_DEFINE(disable)
UHC_MUSB_QUIRK_FUNC_DEFINE(shutdown)
UHC_MUSB_QUIRK_FUNC_DEFINE(irq_clear)

#endif /* ZEPHYR_DRIVERS_USB_UHC_UHC_MUSB_H */

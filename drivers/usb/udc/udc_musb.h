/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UDC_UDC_MUSB_H
#define ZEPHYR_DRIVERS_USB_UDC_UDC_MUSB_H

#include <stdint.h>
#include <stddef.h>

#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>

#include "udc_common.h"

#include <usb_musb_hw.h>

/* Value returned by musb_fifo_size_code() for sizes above 4096 bytes */
#define MUSB_FIFO_SIZE_CODE_INVALID 0xFFU

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

static inline void musb_clear16(volatile uint16_t *reg, uint16_t bits)
{
	musb_write16(reg, musb_read16(reg) & ~bits);
}

static inline mem_addr_t musb_fifo_addr(struct usb_musb_reg *const base, const uint8_t ep)
{
	return (mem_addr_t)&base->fifo[ep];
}

enum udc_musb_ep0_state {
	MUSB_EP0_STATE_IDLE = 0,
	MUSB_EP0_STATE_TX,
	MUSB_EP0_STATE_RX,
	MUSB_EP0_STATE_STATUS_IN,
};

/* Vendor quirks per driver instance */
struct musb_vendor_quirks {
	/*
	 * Called on udc_musb_init() before the core registers are accessed.
	 * Must leave the MUSB core powered, clocked and out of reset.
	 */
	int (*init)(const struct device *dev);
	/* Called on udc_musb_enable() before the soft connect is asserted */
	int (*post_enable)(const struct device *dev);
	/* Called on udc_musb_shutdown() after the control endpoints are disabled */
	int (*shutdown)(const struct device *dev);
	/*
	 * Called at the beginning of IRQ handling to acknowledge wrapper
	 * interrupts. Returns non-zero if no MUSB core interrupt is pending.
	 */
	int (*irq_clear)(const struct device *dev);
};

struct udc_musb_config {
	/* Base address of the MUSB core registers */
	mem_addr_t base;
	/* Base address of the vendor wrapper registers, 0 if there is none */
	mem_addr_t wrapper;
	size_t num_of_eps;
	struct udc_ep_config *ep_cfg_in;
	struct udc_ep_config *ep_cfg_out;
	k_thread_stack_t *thread_stk;
	size_t thread_stk_sz;
	int thread_priority;
	int speed_idx;
	uint32_t vbus_poll_ms;
	void (*irq_enable_func)(const struct device *dev);
	void (*irq_disable_func)(const struct device *dev);
	/* Pointer to vendor quirks or NULL */
	const struct musb_vendor_quirks *const quirks;
};

static inline struct usb_musb_reg *musb_get_base(const struct device *dev)
{
	const struct udc_musb_config *const config = dev->config;

	return (struct usb_musb_reg *)config->base;
}

#define MUSB_EVT_SETUP        BIT(0)
#define MUSB_EVT_XFER         BIT(1)
#define MUSB_EVT_BUS_RESET    BIT(2)
#define MUSB_EVT_SUSPEND      BIT(3)
#define MUSB_EVT_RESUME       BIT(4)
#define MUSB_EVT_BULK_TX      BIT(7)
#define MUSB_EVT_BULK_RX      BIT(8)

#define MUSB_SETUP_PACKET_SIZE 8U
#define MUSB_SETUP_Q_DEPTH     8U
#define MUSB_SETUP_Q_MASK      (MUSB_SETUP_Q_DEPTH - 1U)

/*
 * FIFO RAM is allocated in 8-byte units. The allocation bitmap covers the
 * largest FIFO RAM a MUSB core can have; the usable size is read from RAMINFO.
 */
#define MUSB_FIFO_UNIT_SIZE      8U
#define MUSB_FIFO_RAM_MAX        (MUSB_RAMINFO_RAM_SIZE(MUSB_RAMINFO_RAMBITS_MAX))
#define MUSB_FIFO_UNITS_MAX      (MUSB_FIFO_RAM_MAX / MUSB_FIFO_UNIT_SIZE)
#define MUSB_FIFO_UNITS_PER_WORD 32U
#define MUSB_FIFO_ADDR_INVALID   0xFFFFU
#define MUSB_EP_MAX              16U

#define MUSB_FS_EP_MPS_MAX 1023U
#define MUSB_HS_EP_MPS_MAX 1024U
#define MUSB_FIFO_WORD_SIZE 4U

#define MUSB_SETUP_BMREQTYPE_MASK 0xFFU
#define MUSB_SETUP_WLENGTH_SHIFT  16U
#define MUSB_SETUP_WLENGTH_MASK   0xFFFFU

#define MUSB_RESUME_KSTATE_MS 2U

struct udc_musb_data {
	struct k_thread thread_data;
	struct k_event events;
	/*
	 * Serializes the INDEX register and the banked endpoint registers it
	 * selects, the SETUP queue and the FIFO allocator between the ISR and
	 * thread context. Every holder leaves INDEX set to 0 on release.
	 */
	struct k_spinlock lock;

	uint8_t setup_q[MUSB_SETUP_Q_DEPTH][MUSB_SETUP_PACKET_SIZE];
	uint8_t setup_q_head;
	uint8_t setup_q_tail;
	enum udc_musb_ep0_state ep0_state;
	uint32_t fifo_allocation_table[MUSB_FIFO_UNITS_MAX / MUSB_FIFO_UNITS_PER_WORD];
	/* Number of FIFO RAM units implemented, read from RAMINFO */
	uint32_t fifo_units;
	uint16_t fifo_in_addr[MUSB_EP_MAX];
	uint16_t fifo_out_addr[MUSB_EP_MAX];

	atomic_t bulk_tx_done;
	atomic_t bulk_rx_done;
	atomic_t ep0_rx_done;
	atomic_t bulk_rx_nak;
	atomic_t bulk_rx_pending;

	struct net_buf *ep0_rx_buf;
	atomic_t ep0_rx_pending;
	uint16_t ep0_ctrl_write_len;
	uint16_t ep0_ctrl_bytes_received;
	uint8_t ep0_ctrl_write_setup[MUSB_SETUP_PACKET_SIZE];

	const struct device *dev;
	struct k_work_delayable vbus_work;
	bool vbus_present;
};

#if DT_HAS_COMPAT_STATUS_OKAY(microchip_usb_g2)
#include "udc_musb_mchp_g2.h"
#endif

#define UDC_MUSB_VENDOR_QUIRK_GET(n)						\
	COND_CODE_1(DT_NODE_VENDOR_HAS_IDX(DT_DRV_INST(n), 1),			\
		    (&musb_vendor_quirks_##n),					\
		    (NULL))

#define MUSB_QUIRK_FUNC_DEFINE(fname)						\
static inline int musb_quirk_##fname(const struct device *dev)			\
{										\
	const struct udc_musb_config *const config = dev->config;		\
	const struct musb_vendor_quirks *const quirks =				\
		COND_CODE_1(IS_EQ(DT_NUM_INST_STATUS_OKAY(mentor_musb), 1),	\
			(UDC_MUSB_VENDOR_QUIRK_GET(0); ARG_UNUSED(config);),	\
			(config->quirks;))					\
										\
	if (quirks != NULL && quirks->fname != NULL) {				\
		return quirks->fname(dev);					\
	}									\
										\
	return 0;								\
}

MUSB_QUIRK_FUNC_DEFINE(init)
MUSB_QUIRK_FUNC_DEFINE(post_enable)
MUSB_QUIRK_FUNC_DEFINE(shutdown)
MUSB_QUIRK_FUNC_DEFINE(irq_clear)

#endif /* ZEPHYR_DRIVERS_USB_UDC_UDC_MUSB_H */

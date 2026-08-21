/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Narek Aydinyan
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UHC_UHC_STM32_OTGHS_H_
#define ZEPHYR_DRIVERS_USB_UHC_UHC_STM32_OTGHS_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/usb/uhc.h>

#include <soc.h>
#include <stm32_usb_common.h>

/* Public driver constants */

#define STM32_OTGHS_MAX_CH 16u
/* Keep some channels available for CTRL / housekeeping */
#define STM32_OTGHS_MAX_INFLIGHT_BULK (STM32_OTGHS_MAX_CH - 2u)

/* Thread event bits */

enum stm32_otghs_evt_bits {
	STM32_EVT_CONN = BIT(0),
	STM32_EVT_DISCONN = BIT(1),
	STM32_EVT_PEN = BIT(3),
	STM32_EVT_PDIS = BIT(4),
	STM32_EVT_KICK = BIT(5),
};

/* Internal structures */

/*
 * Persistent HAL channel allocation/cache. This survives between transfers so
 * endpoint toggle state can be reused when the HAL channel is idle.
 */
struct stm32_otghs_chan {
	uint16_t mps;
	uint8_t dev_addr;
	uint8_t ep;      /* Zephyr endpoint address */
	uint8_t ep_num;  /* 0..15 */
	uint8_t ep_type; /* HAL endpoint type */
	uint8_t dir_in; /* 1 if IN */
	uint8_t hc_num; /* 0..STM32_OTGHS_MAX_CH-1 */
	bool in_use;
};

/* Runtime state for a non-control transfer currently assigned to a channel. */
struct stm32_hc_active {
	struct uhc_transfer *xfer;
	k_timepoint_t retry_timepoint;
	uint16_t mps;
	uint16_t req_len;
	uint16_t buf_off;
	uint16_t halted_cnt;
	uint8_t ep;
	uint8_t ep_type;
	uint8_t dir_in;
};

/* EP0 control transfer state. Control transfers are serialized by the stack. */
struct stm32_pipe {
	struct uhc_transfer *xfer; /* NULL => idle */
	int hc;                    /* host channel number */
	k_timepoint_t retry_timepoint;
	uint32_t setup_words[2]; /* aligned setup packet snapshot (CTRL only) */
	uint16_t len;              /* requested len */
	uint16_t buf_off;          /* current buffer offset for resumed IN stage */
	uint16_t halted_cnt;       /* bytes already harvested from halted IN stage */
	uint8_t ep;                /* endpoint address */
};

struct stm32_otghs_data {
	HCD_HandleTypeDef hhcd;
	struct k_thread thread;
	const struct device *dev;

	struct k_event events;
	atomic_t sof_irq_enabled;
	atomic_t sof_tick_active;

	struct k_timer kick_timer;

	k_timepoint_t reset_timepoint;
	uint32_t zlp_dummy;
	uint32_t speed;
	uint16_t ep0_mps;
	uint8_t host_channels;
	bool connected;
	bool bus_ready;
	bool reset_pending;

	struct stm32_otghs_chan ch[STM32_OTGHS_MAX_CH];

	/* EP0 control stays single-flight */
	struct stm32_pipe ctrl;

	/* Non-control (bulk/intr) can be multi-flight */
	struct stm32_hc_active hc_act[STM32_OTGHS_MAX_CH];
};

struct stm32_otghs_config {
	void *base;
	void (*irq_connect)(void);
	const struct pinctrl_dev_config *pincfg;
	struct stm32_pclken *pclken;
	size_t num_clocks;
	const struct stm32_usb_phy *phy;
	uint8_t host_channels;

	k_thread_stack_t *stack;
	size_t stack_size;
	int thread_prio;
};

#endif /* ZEPHYR_DRIVERS_USB_UHC_UHC_STM32_OTGHS_H_ */

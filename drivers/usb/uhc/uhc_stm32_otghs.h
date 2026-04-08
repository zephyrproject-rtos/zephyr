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
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/usb/uhc.h>

#include <stm32u5xx_hal.h>
#include <stm32u5xx_ll_usb.h>

/* Public driver constants */

#define STM32_OTGHS_MAX_CH 16u
#define STM32_TOGGLE_SLOTS 32u

/* Keep some channels available for CTRL / housekeeping */
#define STM32_OTGHS_MAX_INFLIGHT_BULK (STM32_OTGHS_MAX_CH - 2u)

/* Thread event bits (atomic flags) */

enum stm32_otghs_evt_bits {
	STM32_EVT_CONN = BIT(0),
	STM32_EVT_DISCONN = BIT(1),
	STM32_EVT_URB = BIT(2),
	STM32_EVT_PEN = BIT(3),
	STM32_EVT_PDIS = BIT(4),
};

/* Internal structures */

struct stm32_otghs_chan {
	bool in_use;
	uint8_t dev_addr;
	uint8_t ep;      /* Zephyr endpoint address */
	uint8_t ep_num;  /* 0..15 */
	uint8_t ep_type; /* EP_TYPE_* (HAL-compatible) */
	uint16_t mps;
	uint8_t dir_in; /* 1 if IN */
	uint8_t hc_num; /* 0..STM32_OTGHS_MAX_CH-1 */
};

struct stm32_hc_active {
	struct uhc_transfer *xfer; /* NULL if idle */
	uint8_t ep;                /* endpoint address */
	uint8_t ep_type;           /* EP_TYPE_* */
	uint8_t dir_in;            /* 1 if IN */
	uint16_t req_len;          /* requested length */
	uint16_t buf_off;          /* current buffer offset for resumed transfer */
	uint16_t halted_cnt;       /* bytes already harvested from halted IN submit */
	uint16_t mps;              /* max packet size used for packet parity */
	uint32_t start_ms;
};

struct stm32_pipe {
	struct uhc_transfer *xfer; /* NULL => idle */
	int hc;                    /* host channel number */
	uint8_t ep;                /* endpoint address */
	uint16_t len;              /* requested len */
	uint16_t buf_off;          /* current buffer offset for resumed IN stage */
	uint16_t halted_cnt;       /* bytes already harvested from halted IN stage */
	uint32_t start_ms;
	uint32_t setup_words[2]; /* aligned setup packet snapshot (CTRL only) */
};

struct stm32_ep_toggle {
	bool in_use;
	uint8_t dev_addr;
	uint8_t ep;     /* full endpoint address (includes dir bit) */
	uint8_t toggle; /* 0 => DATA0, 1 => DATA1 */
};

struct stm32_otghs_data {
	HCD_HandleTypeDef hhcd;

	struct k_spinlock hal_lock;

	struct k_sem irq_sem;
	atomic_t evt_flags;
	atomic_t sof_irq_enabled;
	atomic_t sof_tick_active;

	struct k_timer kick_timer;

	bool connected;
	bool bus_ready;

	bool reset_pending;
	uint32_t reset_start_ms;

	uint16_t ep0_mps;
	uint32_t zlp_dummy;
	uint32_t speed;

	struct stm32_otghs_chan ch[STM32_OTGHS_MAX_CH];

	/* EP0 control stays single-flight */
	struct stm32_pipe ctrl;

	/* Non-control (bulk/intr) can be multi-flight */
	struct stm32_hc_active hc_act[STM32_OTGHS_MAX_CH];

	/* Shared bulk toggle table (per dev+ep) */
	struct stm32_ep_toggle tog[STM32_TOGGLE_SLOTS];
};

struct stm32_otghs_config {
	uintptr_t base;
	int irqn;
	const struct pinctrl_dev_config *pinctrl;

	k_thread_stack_t *stack;
	size_t stack_size;
	int thread_prio;
};

#endif /* ZEPHYR_DRIVERS_USB_UHC_UHC_STM32_OTGHS_H_ */

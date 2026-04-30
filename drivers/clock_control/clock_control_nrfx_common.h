/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CLOCK_CONTROL_NRFX_COMMON_H__
#define CLOCK_CONTROL_NRFX_COMMON_H__

#ifndef CONFIG_CLOCK_CONTROL_NRF

#include <zephyr/sys/onoff.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/logging/log.h>

#define COMMON_CTX_ONOFF BIT(6)
#define COMMON_CTX_API   BIT(7)

#define COMMON_STATUS_MASK       0x7
#define COMMON_GET_STATUS(flags) (flags & COMMON_STATUS_MASK)

typedef void (*clk_ctrl_func_t)(void);

typedef struct {
	struct onoff_manager mgr;
	clock_control_cb_t cb;
	void *user_data;
	uint32_t flags;
} common_clock_data_t;

typedef struct {
	clk_ctrl_func_t start; /* Clock start function */
	clk_ctrl_func_t stop;  /* Clock stop function */
} common_clock_config_t;

struct clock_control_nrfx_irq_handler {
	void (*handler)(void); /* Clock interrupt handler */
};

#define CLOCK_CONTROL_NRFX_IRQ_HANDLERS_ITERABLE(name, _handler)                                       \
	STRUCT_SECTION_ITERABLE(clock_control_nrfx_irq_handler, name) = {                                  \
		.handler = _handler,                                                                           \
	}

void common_connect_irq(void);

void common_set_on_state(uint32_t *flags);

void common_blocking_start_callback(const struct device *dev, clock_control_subsys_t subsys,
				    void *user_data);

int common_async_start(const struct device *dev, clock_control_cb_t cb, void *user_data,
		       uint32_t ctx);

int common_stop(const struct device *dev, uint32_t ctx);

void common_onoff_started_callback(const struct device *dev, clock_control_subsys_t sys,
				   void *user_data);

void common_clkstarted_handle(const struct device *dev);

void common_clear_pending_irq(void);

#endif /* !CONFIG_CLOCK_CONTROL_NRF */

#endif /* CLOCK_CONTROL_NRFX_COMMON_H__ */

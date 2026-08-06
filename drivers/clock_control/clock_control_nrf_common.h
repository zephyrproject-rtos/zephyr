/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CLOCK_CONTROL_NRF_COMMON_H__
#define CLOCK_CONTROL_NRF_COMMON_H__

#ifndef CONFIG_CLOCK_CONTROL_NRF

#include <zephyr/sys/onoff.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/logging/log.h>

#define FLAGS_COMMON_BITS 10

#define COMMON_CTX_ONOFF BIT(6)
#define COMMON_CTX_API   BIT(7)

#define COMMON_STATUS_MASK       0x7
#define COMMON_GET_STATUS(flags) (flags & COMMON_STATUS_MASK)

struct clock_onoff {
	struct onoff_manager mgr;
	onoff_notify_fn notify;
	uint8_t idx;
};

/**
 * @brief Defines a type for specific clock configuration structure.
 *
 * @param type suffix added clock_config_ to form the type name.
 * @param _onoff_cnt number of clock configuration options to be handled;
 *                   for each one a separate onoff manager instance is used.
 */
#define STRUCT_CLOCK_CONFIG(type, _onoff_cnt)                                                      \
	struct clock_config_##type {                                                               \
		atomic_t flags;                                                                    \
		uint32_t flags_snapshot;                                                           \
		struct k_work work;                                                                \
		uint8_t onoff_cnt;                                                                 \
		struct clock_onoff onoff[_onoff_cnt];                                              \
	}

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

struct clock_control_nrf_irq_handler {
	void (*handler)(void); /* Clock interrupt handler */
};

#define CLOCK_CONTROL_NRF_IRQ_HANDLERS_ITERABLE(name, _handler)                                    \
	STRUCT_SECTION_ITERABLE(clock_control_nrf_irq_handler, name) = {                           \
		.handler = _handler,                                                               \
	}

/**
 * @brief Initializes a clock configuration structure.
 *
 * @param clk_cfg pointer to the structure to be initialized.
 * @param onoff_cnt number of clock configuration options handled
 *                  handled by the structure.
 * @param update_work_handler function that performs configuration update,
 *                            called from the system work queue.
 *
 * @return 0 on success, negative value when onoff initialization fails.
 */
int clock_config_init(void *clk_cfg, uint8_t onoff_cnt, k_work_handler_t update_work_handler);

/**
 * @brief Helper function for requesting a clock configuration handled by
 *        a given on-off manager.
 *
 * If needed, the function resets the on-off service prior to making the new
 * request.
 *
 * @param mgr pointer to the manager for which the request is to be done.
 * @param cli pointer to a client state structure to be used for the request.
 *
 * @return result returned by onoff_request().
 */
int clock_config_request(struct onoff_manager *mgr, struct onoff_client *cli);

/**
 * @brief Starts a clock configuration update.
 *
 * This function is supposed to be called by a specific clock control driver
 * from its update work handler.
 *
 * @param work pointer to the work item received by the update work handler.
 *
 * @return index of the clock configuration onoff option to be activated.
 */
uint8_t clock_config_update_begin(struct k_work *work);

/**
 * @brief Finalizes a clock configuration update.
 *
 * Notifies all relevant onoff managers about the update result.
 * Only the first call after each clock_config_update_begin() performs
 * the actual operation. Any further calls are simply no-ops.
 *
 * @param clk_cfg pointer to the clock configuration structure.
 * @param status result to be passed to onoff managers.
 */
void clock_config_update_end(void *clk_cfg, int status);

int api_nosys_on_off(const struct device *dev, clock_control_subsys_t sys);

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

#endif /* CLOCK_CONTROL_NRF_COMMON_H__ */

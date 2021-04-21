/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SOC_FLASH_NRF_H__
#define __SOC_FLASH_NRF_H__

#include <kernel.h>
#include <soc.h>

#define FLASH_OP_DONE    (0) /* 0 for compliance with the driver API. */
#define FLASH_OP_ONGOING  1

struct flash_context {
	uint32_t data_addr;  /* Address of data to write. */
	uint32_t flash_addr; /* Address of flash to write or erase. */
	uint32_t len;        /* Size of data to write or erase [B]. */
#ifndef CONFIG_SOC_FLASH_NRF_RADIO_SYNC_NONE
	uint8_t  enable_time_limit; /* set execution limited to the execution
				     * window.
				     */
#endif /* !CONFIG_SOC_FLASH_NRF_RADIO_SYNC_NONE */
#if defined(CONFIG_SOC_FLASH_NRF_PARTIAL_ERASE)
	uint32_t flash_addr_next;
#endif /* CONFIG_SOC_FLASH_NRF_PARTIAL_ERASE */
}; /*< Context type for f. @ref write_op @ref erase_op */

#ifndef CONFIG_SOC_FLASH_NRF_RADIO_SYNC_NONE

#if defined(CONFIG_SOC_FLASH_NRF_PARTIAL_ERASE)
/* The timeout is multiplied by 1.5 because switching tasks may take
 * significant portion of time.
 */
#define FLASH_TIMEOUT_MS ((FLASH_PAGE_ERASE_MAX_TIME_US) * \
			  (FLASH_PAGE_MAX_CNT) / 1000 * 15 / 10)
#else

#define FLASH_TIMEOUT_MS ((FLASH_PAGE_ERASE_MAX_TIME_US) * \
			  (FLASH_PAGE_MAX_CNT) / 1000)
#endif /* CONFIG_SOC_FLASH_NRF_PARTIAL_ERASE */

/**
 * @defgroup nrf_flash_sync sync backend API
 *
 * API declared below contains prototypes of function which shall be
 * implemented by the synchronization backend.
 * @{
 */

/**
 * Callback which executes the flash operation.
 *
 * @param context pointer to flash_context structure.
 * @retval @ref FLASH_OP_DONE once operation was done, @ref FLASH_OP_ONGOING if
 *         operation needs more time for execution and a negative error code if
 *         operation was aborted.
 */
typedef int (*flash_op_handler_t) (void *context);

struct flash_op_desc {
	flash_op_handler_t handler;
	struct flash_context *context; /* [in,out] */
};

/**
 * Synchronization backend driver initialization procedure.
 *
 * This will be run within flash driver initialization
 */
int nrf_flash_sync_init(void);

/**
 * Set synchronization context for synchronous operations.
 *
 * This function set backend's internal context for expected timing parameter.
 *
 * @param duration Duration of the execution window [us]
 */
void nrf_flash_sync_set_context(uint32_t duration);

/**
 * Check if the operation need to be run synchonous with radio.
 *
 * @retval True if operation need to be run synchronously, otherwise False
 */
bool nrf_flash_sync_is_required(void);

/**
 * Execute the flash operation synchronously along the radio operations.
 *
 * Function executes calbacks op_desc->handler() in execution windows according
 * to timing settings requested by nrf_flash_sync_set_context().
 * This routine need to be called the handler as many time as it returns
 * FLASH_OP_ONGOING, howewer an operation timeot should be implemented.
 * When the handler() returns FLASH_OP_DONE or an error code, no further
 * execution windows are needed so function should return as the handler()
 * finished its operation.
 *
 * @retval 0 if op_desc->handler() was executed and finished its operation
 * successfully. Otherwise (handler returned error, timeout, couldn't schedule
 * execution...) a negative error code.
 *
 *                              execution window
 *            Driver task           task
 *                  |                 |
 *                  |                 |
 * nrf_flash_sync_  #                 |
 * set_context()    #                 |
 *                  |                 |
 *                  |                 |
 * call nrf_flash_  #                 |
 * sync_exe()       #                 |
 *                  #---------------->|
 *                  |                 |
 *                  |                 # execution window 0
 *                  |                 # call flash_op_handler_t handler()
 *                  |                 #
 *                  |                 #
 *                  |                 # flash_op_handler_t handler() return
 *                  |                 #         FLASH_OP_ONGOING
 *                  |                 # {backend request/allow
 *                  |                 |  the next execution window}
 *                  .                 .
 *                  .                 .
 *                  .                 .
 *                  |                 |
 *                  |                 # execution window N
 *                  |                 # call flash_op_handler_t handler()
 *                  |                 #
 *                  |                 #
 *                  |                 #
 *                  |                 # flash_op_handler_t handler() returns
 *                  |                 #         FLASH_OP_DONE
 *                  |<----------------# {backend transfer execution
 *                  #                 |  to the driver back}
 * nrf_flash_       #                 |
 * sync_exe()       |                 |
 * return           |                 |
 */
int nrf_flash_sync_exe(struct flash_op_desc *op_desc);

/**
 * @}
 */

/**
 * @defgroup nrf_flash_sync_timing sync timing backend API
 * @ingroup nrf_flash_sync
 * @{
 *
 * API which is used by nrf flash driver for check where execution fill in
 * the execution window.
 *
 * API is used as follows:
 * begin of execution window
 * call flash_op_handler_t handler()
 *   nrf_flash_sync_get_timestamp_begin()
 *   [does some chunk of work]
 *   nrf_flash_sync_check_time_limit() == false
 *   [does some chunk of work]
 *   nrf_flash_sync_check_time_limit() == false
 *   [does some chunk of work]
 *   ...
 *   nrf_flash_sync_check_time_limit() == true
 *   [preserve work context for next execution window]
 * return form flash_op_handler_t handler()
 * [return from execution window]
 * end of execution window
 */

/**
 * Get timestamp and store it in synchronization backend
 * context data as operation beginning time reference.
 * This timestapm will be used by @ref nrf_flash_sync_check_time_limit()
 * as the execution window begin reference.
 */
void nrf_flash_sync_get_timestamp_begin(void);

/**
 * Estimate whether next iteration will fit in time constraints.
 * This function fetch current timestamp and compare it with the operation
 * beginning timestamp referene stored by
 * @ref nrf_flash_sync_get_timestamp_begin() in the synchronization backend
 * context data.
 *
 * @param iteration iteration number.
 * @retval true if estimated time excess, false otherwise.
 */
bool nrf_flash_sync_check_time_limit(uint32_t iteration);

/**
 * @}
 */

#endif /* !CONFIG_SOC_FLASH_NRF_RADIO_SYNC_NONE */
#endif /* !__SOC_FLASH_NRF_H__ */

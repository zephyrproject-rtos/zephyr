/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_LOGGING_LOG_FRONTEND_STMESP_H_
#define ZEPHYR_INCLUDE_LOGGING_LOG_FRONTEND_STMESP_H_

#include <errno.h>
#include <zephyr/types.h>
#ifdef CONFIG_LOG_FRONTEND_STMESP
#include <zephyr/drivers/misc/coresight/stmesp.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Notify frontend that ETR/STM is ready.
 *
 * Log frontend optionally dumps buffered data and start to write to the STM
 * stimulus port.
 *
 * @note Function is applicable only for the domain that performs initial ETR/STM setup.
 *
 * @retval 0	on success.
 * @retval -EIO	if there was an internal failure.
 */
int log_frontend_stmesp_etr_ready(void);

/** @brief Hook to be called before going to sleep.
 *
 * Hook writes dummy data to the STM Stimulus Port to ensure that all logging
 * data is flushed.
 */
void log_frontend_stmesp_pre_sleep(void);

/** @brief Perform a dummy write to STMESP.
 *
 * It can be used to force flushing STM data.
 */
void log_frontend_stmesp_dummy_write(void);

/** @brief Trace point
 *
 * Write a trace point information using STM. Number of unique trace points is limited
 * to 65536 - CONFIG_LOG_FRONTEND_STMESP_TP_CHAN_BASE per core.
 *
 * @param x Trace point ID.
 */
static inline void log_frontend_stmesp_tp(uint16_t x)
{
#ifdef CONFIG_LOG_FRONTEND_STMESP
	STMESP_Type *port;
	int err = stmesp_get_port((uint32_t)x + CONFIG_LOG_FRONTEND_STMESP_TP_CHAN_BASE, &port);

	__ASSERT_NO_MSG(err == 0);
	if (err == 0) {
		stmesp_flag(port, 1, true,
			    IS_ENABLED(CONFIG_LOG_FRONTEND_STMESP_GUARANTEED_ACCESS));
	}
#endif
}

/** @brief Trace point with 32 bit data.
 *
 * Write a trace point information using STM. Number of unique trace points is limited
 * to 65536 - CONFIG_LOG_FRONTEND_STMESP_TP_CHAN_BASE per core.
 *
 * @param x Trace point ID.
 * @param d Data. 32 bit word.
 */
static inline void log_frontend_stmesp_tp_d32(uint16_t x, uint32_t d)
{
#ifdef CONFIG_LOG_FRONTEND_STMESP
	STMESP_Type *port;
	int err = stmesp_get_port((uint32_t)x + CONFIG_LOG_FRONTEND_STMESP_TP_CHAN_BASE, &port);

	__ASSERT_NO_MSG(err == 0);
	if (err == 0) {
		stmesp_data32(port, d, true, true,
			      IS_ENABLED(CONFIG_LOG_FRONTEND_STMESP_GUARANTEED_ACCESS));
	}
#endif
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_LOGGING_LOG_FRONTEND_STMESP_H_ */

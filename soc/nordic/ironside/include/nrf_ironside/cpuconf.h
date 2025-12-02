/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_NORDIC_IRONSIDE_INCLUDE_NRF_IRONSIDE_CPUCONF_H_
#define ZEPHYR_SOC_NORDIC_IRONSIDE_INCLUDE_NRF_IRONSIDE_CPUCONF_H_

#include <stdbool.h>
#include <stddef.h>
#include <zephyr/toolchain/common.h>
#include <nrf.h>
#include <nrf_ironside/call.h>

/**
 * @name CPUCONF service error codes.
 * @{
 */

/** An invalid or unsupported processor ID was specified. */
#define IRONSIDE_CPUCONF_ERROR_WRONG_CPU         (1)
/** The boot message is too large to fit in the buffer. */
#define IRONSIDE_CPUCONF_ERROR_MESSAGE_TOO_LARGE (2)

/**
 * @}
 */

#define IRONSIDE_CALL_ID_CPUCONF_V0 2

enum {
	IRONSIDE_CPUCONF_SERVICE_CPU_PARAMS_IDX,
	IRONSIDE_CPUCONF_SERVICE_VECTOR_TABLE_IDX,
	IRONSIDE_CPUCONF_SERVICE_MSG_0,
	IRONSIDE_CPUCONF_SERVICE_MSG_1,
	IRONSIDE_CPUCONF_SERVICE_MSG_2,
	IRONSIDE_CPUCONF_SERVICE_MSG_3,
	/* The last enum value is reserved for the number of arguments */
	IRONSIDE_CPUCONF_NUM_ARGS
};

/* Maximum size of the CPUCONF message parameter. */
#define IRONSIDE_CPUCONF_SERVICE_MSG_MAX_SIZE (4 * sizeof(uint32_t))

/* IDX 0 is re-used by the error return code and the 'cpu' parameter. */
#define IRONSIDE_CPUCONF_SERVICE_RETCODE_IDX 0

BUILD_ASSERT(IRONSIDE_CPUCONF_NUM_ARGS <= NRF_IRONSIDE_CALL_NUM_ARGS);

/**
 * @brief Boot a local domain CPU
 *
 * @param cpu The CPU to be booted
 * @param vector_table Pointer to the vector table used to boot the CPU.
 * @param cpu_wait When this is true, the CPU will WAIT even if the CPU has clock.
 * @param msg A message that can be placed in cpu's boot report.
 * @param msg_size Size of the message in bytes.
 *
 * @note cpu_wait is only intended to be enabled for debug purposes
 * and it is only supported that a debugger resumes the CPU.
 *
 * @note the call always sends IRONSIDE_CPUCONF_SERVICE_MSG_MAX_SIZE message bytes.
 * If the given msg_size is less than that, the remaining bytes are set to zero.
 *
 * @retval 0 on success or if the CPU has already booted.
 * @retval -IRONSIDE_CPUCONF_ERROR_WRONG_CPU if cpu is unrecognized.
 * @retval -IRONSIDE_CPUCONF_ERROR_MESSAGE_TOO_LARGE if msg_size is greater than
 * IRONSIDE_CPUCONF_SERVICE_MSG_MAX_SIZE.
 * @retval Positive error status if reported by IronSide call (see error codes in @ref call.h).
 */
int ironside_cpuconf(NRF_PROCESSORID_Type cpu, const void *vector_table, bool cpu_wait,
		     const uint8_t *msg, size_t msg_size);

#endif /* ZEPHYR_SOC_NORDIC_IRONSIDE_INCLUDE_NRF_IRONSIDE_CPUCONF_H_ */

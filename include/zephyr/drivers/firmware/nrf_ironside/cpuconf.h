/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_NRF_IRONSIDE_CPUCONF_H_
#define ZEPHYR_INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_NRF_IRONSIDE_CPUCONF_H_

#include <zephyr/drivers/firmware/nrf_ironside/call.h>

#include <nrfx.h>

/**
 * @name CPUCONF service error codes.
 * @{
 */

#define IRONSIDE_CPUCONF_ERROR_STARTING_CPU_FAILED      0x5eb0
#define IRONSIDE_CPUCONF_ERROR_WRONG_CPU                0x5eb2
#define IRONSIDE_CPUCONF_ERROR_MEM_ACCESS_NOT_PERMITTED 0x5eb1
#define IRONSIDE_CPUCONF_ERROR_IRONSIDE_CALL_FAILED     0x5eb3

/**
 * @}
 */

#define IRONSIDE_CALL_ID_CPUCONF_V0 2

enum {
	IRONSIDE_CPUCONF_SERVICE_CPU_IDX,
	IRONSIDE_CPUCONF_SERVICE_VECTOR_TABLE_IDX,
	IRONSIDE_CPUCONF_SERVICE_CPU_WAIT_IDX,
	IRONSIDE_CPUCONF_SERVICE_MSG_IDX,
	IRONSIDE_CPUCONF_SERVICE_MSG_SIZE_IDX,
	/* The last enum value is reserved for the number of arguments */
	IRONSIDE_SE_CPUCONF_NUM_ARGS
};

/* IDX 0 is re-used by the error return code and the 'cpu' parameter. */
#define IRONSIDE_CPUCONF_SERVICE_RETCODE_IDX 0

BUILD_ASSERT(IRONSIDE_SE_CPUCONF_NUM_ARGS <= NRF_IRONSIDE_CALL_NUM_ARGS);

/**
 * @brief Boot a local domain CPU
 *
 * @param cpu The CPU to be booted
 * @param vector_table Pointer to the vector table used to boot the CPU.
 * @param cpu_wait When this is true, the CPU will WAIT even if the CPU has clock.
 * @param msg A message that can be placed in radiocore's boot report.
 * @param msg_size Size of the message in bytes.
 *
 * @note cpu_wait is only intended to be enabled for debug purposes
 * and it is only supported that a debugger resumes the CPU.
 *
 * @retval 0 on success or if the CPU has already booted.
 * @retval -IRONSIDE_CPUCONF_ERROR_WRONG_CPU if cpu is unrecognized
 * @retval -IRONSIDE_CPUCONF_ERROR_STARTING_CPU_FAILED if starting the CPU failed
 * @retval -IRONSIDE_CPUCONF_ERROR_MEM_ACCESS_NOT_PERMITTED
 * if the CPU does not have read access configured for the vector_table address
 * @retval -IRONSIDE_CPUCONF_ERROR_IRONSIDE_CALL_FAILED The IRONside call failed.
 */
int ironside_cpuconf(NRF_PROCESSORID_Type cpu, void *vector_table, bool cpu_wait, uint8_t *msg,
		     size_t msg_size);

#endif /* ZEPHYR_INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_NRF_IRONSIDE_CPUCONF_H_ */

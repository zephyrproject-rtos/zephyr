/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_NORDIC_NRF71_NRF71_MRAM_RECOVERY_H_
#define ZEPHYR_SOC_NORDIC_NRF71_NRF71_MRAM_RECOVERY_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief MRAM snapshot status.
 */
enum nrf71_mram_recovery_snapshot_status {
	/** No snapshot is requested. */
	NRF71_MRAM_RECOVERY_SNAPSHOT_STATUS_NONE = 0,
	/** A snapshot is requested for the next boot. */
	NRF71_MRAM_RECOVERY_SNAPSHOT_STATUS_REQUESTED = 1,
	/** The snapshot was created successfully on the previous boot. */
	NRF71_MRAM_RECOVERY_SNAPSHOT_STATUS_SUCCESS = 2,
	/** There is no space for another snapshot. */
	NRF71_MRAM_RECOVERY_SNAPSHOT_STATUS_NO_SPACE = 6,
	/** An unspecified snapshot error occurred. */
	NRF71_MRAM_RECOVERY_SNAPSHOT_STATUS_ERROR = 7,
};

/**
 * @brief Request an MRAM snapshot during the next boot.
 */
void nrf71_mram_recovery_snapshot_request(void);

/**
 * @brief Get the MRAM snapshot status.
 *
 * @return MRAM snapshot status.
 */
enum nrf71_mram_recovery_snapshot_status nrf71_mram_recovery_snapshot_status_get(void);

/**
 * @brief Request MRAM recovery from a snapshot during the next boot.
 */
void nrf71_mram_recovery_request(void);

/**
 * @brief Get the MRAM recovery boot count.
 *
 * @return MRAM recovery boot count.
 */
uint8_t nrf71_mram_recovery_boot_count_get(void);

/**
 * @brief Clear the MRAM recovery boot count.
 */
void nrf71_mram_recovery_boot_count_clear(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_NORDIC_NRF71_NRF71_MRAM_RECOVERY_H_ */

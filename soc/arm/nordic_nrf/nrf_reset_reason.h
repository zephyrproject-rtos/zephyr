/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef NRF_RESET_REASON_H_
#define NRF_RESET_REASON_H_

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Return mask with reset reasons.
 *
 * On first attempt it clears hardware register.
 *
 * Normally, there should be only one reset reason present in the mask. If
 * there is more than one then it means that reset occured while being in
 * bootloader since bootloader does not clean reset reason. For example
 * pin reset occured while bootloader was about to start application after
 * waking up from system off (e.g. due to pin wakeup). Reset reasons are usually
 * ordered in "importance".
 *
 * @return Reset reasons mask, 0 if power-on reset.
 */
u32_t nrf_reset_reason_get(void);

/** @brief Get primary reset reason.
 *
 * If multiple reset reasons are set then the one with lowest bit mask is
 * returned.
 *
 * @return Bit mask with one bit set.
 */
u32_t nrf_reset_reason_primary_get(void);

/** @brief Get reset reason name.
 *
 * @param reason Reason bit mask. It should contain only one bit set.
 *
 * @return Name.
 */
const char * nrf_reset_reason_get_name(u32_t reason);

#ifdef __cplusplus
}
#endif

#endif /* NRF_RESET_REASON_H_ */

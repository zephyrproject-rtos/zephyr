/* Copyright (c) 2018, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this
 *      list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *
 *   3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef NRF_802154_RSCH_CRIT_SECT_H__
#define NRF_802154_RSCH_CRIT_SECT_H__

#include <stdbool.h>

#include "nrf_802154_rsch.h"

/**
 * @defgroup nrf_802154_rsch_crit_sect RSCH event queue used during critical sections
 * @{
 * @ingroup nrf_802154_rsch
 * @brief Critical section implementation for the RSCH module.
 */

/**
 * @brief Initialize the RSCH critical section module.
 */
void nrf_802154_rsch_crit_sect_init(void);

/**
 * @brief Request priority level from RSCH through the critical section module.
 *
 * @param[in]  prio  Requested priority level.
 */
void nrf_802154_rsch_crit_sect_prio_request(rsch_prio_t prio);

/**
 * @brief This function is called to notify the core that approved RSCH priority has changed.
 *
 * @note This function is called from critical section context and does not preempt other critical
 *       sections.
 *
 * @param[in]  prio  Approved priority level.
 */
extern void nrf_802154_rsch_crit_sect_prio_changed(rsch_prio_t prio);

/**
 *@}
 **/

#endif // NRF_802154_RSCH_CRIT_SECT_H__

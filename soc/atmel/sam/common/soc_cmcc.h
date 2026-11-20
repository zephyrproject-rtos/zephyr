/*
 * SPDX-FileCopyrightText: 2026 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ATMEL_SAM_SOC_CMCC_H_
#define _ATMEL_SAM_SOC_CMCC_H_

/**
 * @brief Set up the Cortex-M Cache Controller (CMCC).
 *
 * Enable the i-cache with cache management, disable the CMCC without it.
 */
#if defined(CONFIG_SOC_ATMEL_SAM_HAS_CMCC)
void soc_cmcc_init(void);
#else
static inline void soc_cmcc_init(void)
{
}
#endif

#endif /* _ATMEL_SAM_SOC_CMCC_H_ */

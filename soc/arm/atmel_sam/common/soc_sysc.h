/*
 * Copyright (c) 2023 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ATMEL_SAM_SOC_SYSC_H_
#define _ATMEL_SAM_SOC_SYSC_H_

/**
 * @brief Disable write protection of RTC and RTT mode registers
 */
void soc_sysc_disable_write_protection(void);

/**
 * @brief Enable write protection of RTC and RTT mode registers
 */
void soc_sysc_enable_write_protection(void);

#endif /* _ATMEL_SAM_SOC_SYSC_H_ */

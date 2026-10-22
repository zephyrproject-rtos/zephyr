/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NORDICSEMI_NRF71_APPROTECT_SETUP_H_
#define _NORDICSEMI_NRF71_APPROTECT_SETUP_H_

/**
 * Configure the nRF7120 APPROTECT boot workaround.
 *
 * A soft reset is triggered to clear locked APPROTECT registers.
 */
void approtect_setup(void);

#endif /* _NORDICSEMI_NRF71_APPROTECT_SETUP_H_ */

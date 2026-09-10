/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NORDICSEMI_NRF71_WICR_SETUP_H_
#define _NORDICSEMI_NRF71_WICR_SETUP_H_

/**
 * Configure the nRF71 Wi-Fi Information Configuration Registers.
 *
 * @retval 0      WICR configuration completed successfully.
 * @retval -EIO   A WICR value could not be verified after writing.
 */
int wicr_setup(void);

#endif /* _NORDICSEMI_NRF71_WICR_SETUP_H_ */

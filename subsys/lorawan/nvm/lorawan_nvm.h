/*
 * Copyright (c) 2022 Intellinium <giuliano.franchetto@intellinium.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_LORAWAN_NVM_H_
#define ZEPHYR_SUBSYS_LORAWAN_NVM_H_

#include <stdint.h>

/**
 * @brief Hook function called when an interrupt related to NVM
 * is received from the LoRaWAN stack.
 *
 * @note This function should not be called directly by the application
 *
 * @param flags OR'ed flags received with the interrupt
 *
 * @see LORAMAC_NVM_NOTIFY_FLAG_NONE
 * @see LORAMAC_NVM_NOTIFY_FLAG_CRYPTO
 * @see LORAMAC_NVM_NOTIFY_FLAG_MAC_GROUP1
 * @see LORAMAC_NVM_NOTIFY_FLAG_MAC_GROUP2
 * @see LORAMAC_NVM_NOTIFY_FLAG_SECURE_ELEMENT
 * @see LORAMAC_NVM_NOTIFY_FLAG_REGION_GROUP1
 * @see LORAMAC_NVM_NOTIFY_FLAG_REGION_GROUP2
 * @see LORAMAC_NVM_NOTIFY_FLAG_CLASS_B
 */
void lorawan_nvm_data_mgmt_event(uint16_t flags);

/**
 * @brief Restores all the relevant LoRaWAN data from the Non-Volatile Memory.
 *
 * @note This function should only be called if a NVM has been enabled, and
 * not directly by the application.
 *
 * @return 0 on success, or a negative error code otherwise
 */
int lorawan_nvm_data_restore(void);

/**
 * @brief Initializes the NVM backend
 *
 * @note This function shall be called before any other
 * NVM back functions.
 *
 * @return 0 on success, or a negative error code otherwise
 */
int lorawan_nvm_init(void);

#endif /* ZEPHYR_SUBSYS_LORAWAN_NVM_H_ */

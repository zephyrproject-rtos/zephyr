/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_LORAWAN_LORAMAC_NODE_NVM_H_
#define ZEPHYR_SUBSYS_LORAWAN_LORAMAC_NODE_NVM_H_

#include <stdint.h>

/* Handle LoRaMAC-node's NvmDataChange notification flags. */
void loramac_nvm_data_mgmt_event(uint16_t flags);

#endif /* ZEPHYR_SUBSYS_LORAWAN_LORAMAC_NODE_NVM_H_ */

/*
 * Copyright (c) 2021 Intellinium <giuliano.franchetto@intellinium.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LORAWAN_NVM_H
#define LORAWAN_NVM_H

#include <zephyr.h>

void lorawan_nvm_data_mgmt_event(uint16_t notifyFlags);

int lorawan_nvm_data_restore(void);

#endif /* LORAWAN_NVM_H */

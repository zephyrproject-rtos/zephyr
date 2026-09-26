/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __OBJECT_ADDRESS_TABLE__
#define __OBJECT_ADDRESS_TABLE__

#include <stdbool.h>
#include <stdint.h>

uint16_t address_table_get_group_address(uint16_t tsap);
uint16_t address_table_get_tsap(uint16_t addr);
bool address_table_contains(uint16_t addr);

#endif

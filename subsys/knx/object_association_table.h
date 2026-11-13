/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_KNX_OBJECT_ASSOCIATION_TABLE_H_
#define ZEPHYR_SUBSYS_KNX_OBJECT_ASSOCIATION_TABLE_H_

#include <stdint.h>

int32_t association_table_next_asap(uint16_t tsap, uint16_t *startIdx);
int32_t association_table_translate_asap(uint16_t asap);

#endif /* ZEPHYR_SUBSYS_KNX_OBJECT_ASSOCIATION_TABLE_H_ */

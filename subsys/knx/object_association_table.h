/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __OBJECT_ASSOCIATION_TABLE__
#define __OBJECT_ASSOCIATION_TABLE__

#include <stdint.h>

int32_t association_table_next_asap(uint16_t tsap, uint16_t *startIdx);
int32_t association_table_translate_asap(uint16_t asap);

#endif

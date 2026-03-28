/***************************************************************************
 * Copyright (c) 2024 Microsoft Corporation
 *
 * This program and the accompanying materials are made available under the
 * terms of the MIT License which is available at
 * https://opensource.org/licenses/MIT.
 *
 * SPDX-License-Identifier: MIT
 **************************************************************************/

#ifndef TM_PORTING_LAYER_H
#define TM_PORTING_LAYER_H

#include <stdio.h>

extern void tm_cause_interrupt(void);

#define TM_CAUSE_INTERRUPT tm_cause_interrupt()

#endif

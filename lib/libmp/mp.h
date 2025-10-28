/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * mp.h: Main header for MediaPipe
 *
 * Applications should include this and only this.
 *
 */

#ifndef __MP_H__
#define __MP_H__

#include <src/core/mp_bus.h>
#include <src/core/mp_element.h>
#include <src/core/mp_element_factory.h>
#include <src/core/mp_pipeline.h>

/**
 * mp_init:
 *
 * Initializes the whole MP library
 * - registering built-in elements
 * - loading standard plugins
 */
void mp_init();

#endif /* __MP_H__ */

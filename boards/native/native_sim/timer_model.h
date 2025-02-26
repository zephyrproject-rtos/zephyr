/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BOARDS_POSIX_NATIVE_SIM_TIMER_MODEL_H
#define BOARDS_POSIX_NATIVE_SIM_TIMER_MODEL_H

#warning "This transitional header is now deprecated and will be removed by v4.4. "\
	 "Use nsi_timer_model.h instead."

/*
 * To support the native_posix timer driver
 * we provide a header with the same name as in native_posix
 */
#include "nsi_hw_scheduler.h"
#include "nsi_timer_model.h"
#include "native_posix_compat.h"

#endif /* BOARDS_POSIX_NATIVE_SIM_TIMER_MODEL_H */

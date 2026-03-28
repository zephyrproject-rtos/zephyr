/*
 * Copyright (c) 2023 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fff.h>

#include "fakes/called_API.h"

DEFINE_FAKE_VALUE_FUNC(int, called_API_open, const struct called_API_info **);
DEFINE_FAKE_VALUE_FUNC(int, called_API_close, const struct called_API_info *);

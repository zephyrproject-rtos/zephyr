/*
 * Copyright (c) 2023 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fff.h>

#include "fakes/called_api.h"

DEFINE_FAKE_VALUE_FUNC(int, called_api_open, const struct called_api_info **);
DEFINE_FAKE_VALUE_FUNC(int, called_api_close, const struct called_api_info *);

/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/iterable_sections.h>

/* Only needed because the driver is defined in tests folder */
ITERABLE_SECTION_ROM(fake_driver_api, Z_LINK_ITERABLE_SUBALIGN)

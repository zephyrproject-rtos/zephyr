/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/iterable_sections.h>

ITERABLE_SECTION_RAM(_MpPlugin, Z_LINK_ITERABLE_SUBALIGN)
ITERABLE_SECTION_RAM(_MpElementFactory, Z_LINK_ITERABLE_SUBALIGN)

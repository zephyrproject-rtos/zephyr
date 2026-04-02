/*
 * Copyright (c) 2024 Witekio
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/iterable_sections.h>

ITERABLE_SECTION_ROM(http_header_name, Z_LINK_ITERABLE_SUBALIGN)

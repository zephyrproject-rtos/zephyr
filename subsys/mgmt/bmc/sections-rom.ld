/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/iterable_sections.h>

ITERABLE_SECTION_ROM(http_resource_desc_http_service, Z_LINK_ITERABLE_SUBALIGN)
ITERABLE_SECTION_ROM(http_resource_desc_https_service, Z_LINK_ITERABLE_SUBALIGN)

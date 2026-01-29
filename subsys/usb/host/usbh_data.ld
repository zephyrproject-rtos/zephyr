/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/iterable_sections.h>

ITERABLE_SECTION_RAM(usbh_context, Z_LINK_ITERABLE_SUBALIGN)
ITERABLE_SECTION_RAM(usbh_class_node, Z_LINK_ITERABLE_SUBALIGN)

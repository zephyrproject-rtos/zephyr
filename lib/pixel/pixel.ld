# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

#include <zephyr/linker/iterable_sections.h>

ITERABLE_SECTION_RAM(pixel_convert, Z_LINK_ITERABLE_SUBALIGN)
ITERABLE_SECTION_RAM(pixel_resize, Z_LINK_ITERABLE_SUBALIGN)
ITERABLE_SECTION_RAM(pixel_kernel, Z_LINK_ITERABLE_SUBALIGN)

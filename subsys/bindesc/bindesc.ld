/*
 * Copyright (c) 2023 Yonatan Schachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/iterable_sections.h>
#include <zephyr/bindesc.h>

SQUAD(BINDESC_MAGIC);
Z_LINK_ITERABLE(bindesc_entry);
. = ALIGN(BINDESC_ALIGNMENT);
LONG(BINDESC_TAG_DESCRIPTORS_END)

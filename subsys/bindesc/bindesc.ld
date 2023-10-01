/*
 * Copyright (c) 2023 Yonatan Schachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/iterable_sections.h>
#include <zephyr/bindesc.h>

#define BINDESC_MAX(a, b) (((a) > (b)) ? (a) : (b))

bindesc_start = .;
SQUAD(BINDESC_MAGIC);
Z_LINK_ITERABLE(bindesc_entry);
. = ALIGN(BINDESC_ALIGNMENT);
LONG(BINDESC_TAG_DESCRIPTORS_END)
. = BINDESC_MAX(bindesc_start + CONFIG_BINDESC_DEFINE_DESCRIPTORS_SIZE, .);
. = ALIGN(BINDESC_ALIGNMENT);
bindesc_end = .;

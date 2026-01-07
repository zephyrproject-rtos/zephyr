/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_MODULES_HAL_NORDIC_IRONSIDE_SE_INCLUDE_IRONSIDE_ZEPHYR_SE_UICR_PERIPHCONF_H_
#define ZEPHYR_MODULES_HAL_NORDIC_IRONSIDE_SE_INCLUDE_IRONSIDE_ZEPHYR_SE_UICR_PERIPHCONF_H_

#include <ironside/se/periphconf.h>

#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

/** Add an entry to the PERIPHCONF table section.
 *
 * This is a Zephyr integration with the IronSide UICR PERIPHCONF construction
 * macros that uses an iterable section to store the entries.
 *
 * The macro expects a struct periphconf_entry initializer as input.
 * The macros defined in ironside/se/periphconf.h can be used to construct the initializer.
 * For example:
 *
 *  UICR_PERIPHCONF_ENTRY(PERIPHCONF_SPU_FEATURE_GRTC_CC(...));
 *
 */
#define UICR_PERIPHCONF_ENTRY(_entry)                                                              \
	static STRUCT_SECTION_ITERABLE(periphconf_entry,                                           \
				       _UICR_PERIPHCONF_ENTRY_NAME(__COUNTER__)) = _entry

#define _UICR_PERIPHCONF_ENTRY_NAME(_id)  __UICR_PERIPHCONF_ENTRY_NAME(_id)
#define __UICR_PERIPHCONF_ENTRY_NAME(_id) _uicr_periphconf_entry_##_id

#endif /* ZEPHYR_MODULES_HAL_NORDIC_IRONSIDE_SE_INCLUDE_IRONSIDE_ZEPHYR_SE_UICR_PERIPHCONF_H_ */

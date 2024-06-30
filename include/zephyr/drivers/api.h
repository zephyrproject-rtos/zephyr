/**
 * @file
 * @brief Generic public API header file.
 */

/*
 * Copyright (c) 2024 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_API_H_
#define ZEPHYR_INCLUDE_DRIVERS_API_H_

#include <zephyr/sys/__assert.h>
#include <zephyr/sys/iterable_sections.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */
#define DRIVER_API(_type, _name) const STRUCT_SECTION_ITERABLE(_type, _name)

#define DRIVER_API_IS(_type, _api)                                                                 \
	({                                                                                         \
		STRUCT_SECTION_START_EXTERN(_type);                                                \
		STRUCT_SECTION_END_EXTERN(_type);                                                  \
		(STRUCT_SECTION_START(_type) <= (const struct _type *)_api &&                      \
		 (const struct _type *)_api < STRUCT_SECTION_END(_type));                          \
	})

#define DRIVER_API_EVAL(_type, _api) __ASSERT_NO_MSG(DRIVER_API_IS(_type, _api))

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_INCLUDE_DRIVERS_API_H_ */

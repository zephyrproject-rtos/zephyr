/*
 * Copyright (c) 2026 Fabien Proriol
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Build-time size check between application-defined KNX data and its
 * Kconfig-reserved storage.
 *
 * The KNX stack reserves storage for the application program's parameters
 * and for the Group Object cache via CONFIG_KNX_APPLICATION_PROGRAM_DATA_SIZE
 * and CONFIG_KNX_GROUP_OBJECTS_DATA_SIZE (zephyr/subsys/knx/Kconfig), but it
 * never sees the struct layouts themselves — those are defined per
 * application (one per KnxDaughterBoard: application/, application_w1/,
 * application_helios/, ...), each in its own knx_app_data.h. This header only
 * provides the macros to check that a struct's size still matches what
 * Kconfig reserved for it.
 */

#ifndef ZEPHYR_INCLUDE_KNX_APP_DATA_H_
#define ZEPHYR_INCLUDE_KNX_APP_DATA_H_

#include <zephyr/sys/util.h>

/*
 * KNX_APP_DATA_SIZE_CHECK(var, config_macro) — fail the build if sizeof(var)
 * (a struct instance or a bare type name, e.g. `struct group_objects_data`)
 * no longer matches the Kconfig-reserved storage size.
 *
 * sizeof() is a compile-time constant, so this check — unlike a plain
 * #error — can actually evaluate it; BUILD_ASSERT (backed by C11
 * _Static_assert) is the correct tool and hard-fails the build exactly like
 * #error would. The Kconfig value is a plain integer macro, so STRINGIFY()
 * can embed its literal digits in the message. The compiler cannot however
 * print sizeof(var) itself as text: that value only exists after semantic
 * analysis, once the preprocessor (which builds error-message text) is long
 * done — there is no portable way around that in C, so it isn't attempted
 * here. If the mismatch needs to be seen with both numbers side by side at
 * runtime instead (e.g. while iterating on a struct before touching
 * prj.conf), log sizeof(var) with printk()/LOG_WRN() manually.
 */
#define KNX_APP_DATA_SIZE_CHECK(var, config_macro) \
	BUILD_ASSERT(sizeof(var) == (config_macro), \
		     "sizeof(" #var ") no longer matches " #config_macro \
		     " (" STRINGIFY(config_macro) " bytes) - update " #config_macro \
		     " in prj.conf, or fix the struct definition")

/* APPLICATION_PROGRAM_DATA(var) / GROUP_OBJECTS_DATA(var) — call once with
 * the struct type (or an instance) in application code, e.g. in main.c:
 *
 *     APPLICATION_PROGRAM_DATA(struct application_program_data);
 *     GROUP_OBJECTS_DATA(struct group_objects_data);
 */
#define APPLICATION_PROGRAM_DATA(var) \
	KNX_APP_DATA_SIZE_CHECK(var, CONFIG_KNX_APPLICATION_PROGRAM_DATA_SIZE)

#define GROUP_OBJECTS_DATA(var) \
	KNX_APP_DATA_SIZE_CHECK(var, CONFIG_KNX_GROUP_OBJECTS_DATA_SIZE)

#endif /* ZEPHYR_INCLUDE_KNX_APP_DATA_H_ */

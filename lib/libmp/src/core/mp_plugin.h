/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header for MpPlugin.
 */

#ifndef __MP_PLUGIN_H__
#define __MP_PLUGIN_H__

#include <zephyr/sys/iterable_sections.h>
#include <zephyr/toolchain.h>

/**
 * @{
 */

/**
 * @brief Plugin structure
 *
 */
typedef struct _MpPlugin {
	/** Plugin name as a string */
	const char *const name;
	/** Plugin initialization function pointer */
	void (*init)(void);
} MpPlugin;

/**
 * @brief Statically define and initialize a plugin
 *
 * This macro creates a plugin entry that will be automatically collected
 * into the plugin resgistry and can be initialized by @ref initialize_plugins().
 *
 * @param pname Name of the plugin (will be stringified for the name field)
 * @param initfunc Pointer to the init function of the plugin
 *
 * Example usage:
 * @code
 * static void my_plugin_init(void)
 * {
 *     // Plugin initialization code
 * }
 *
 * MP_PLUGIN_DEFINE(my_plugin, my_plugin_init);
 * @endcode
 */
#define MP_PLUGIN_DEFINE(pname, initfunc)                                                          \
	static const STRUCT_SECTION_ITERABLE(_MpPlugin, pname) = {                                 \
		.name = STRINGIFY(pname), .init = initfunc,                                        \
	}

/**
 * @brief Initialize all registered plugins
 *
 * This function iterates through all plugins defined with @ref MP_PLUGIN_DEFINE
 * and calls their initialization functions. Plugins with NULL init functions
 * are safely skipped.
 *
 * @note This function should typically be called once during system initialization.
 */
void initialize_plugins(void);

/**
 * @}
 */

#endif /* __MP_PLUGIN_H__ */

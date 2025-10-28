/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header for mp_plugin.
 */

#ifndef __MP_PLUGIN_H__
#define __MP_PLUGIN_H__

#include <zephyr/sys/iterable_sections.h>

/**
 * @{
 */

/**
 * @brief Element factory identifiers
 *
 */
enum mp_element_factory_id {
	/* Built-in elements */
	/** Pipeline */
	MP_PIPELINE_ELEM = 0,
	/** Capsfilter */
	MP_CAPS_FILTER_ELEM,
	/** Last item for counter only */
	MP_ELEM_COUNT,
};

/**
 * @brief Plugin identifiers
 *
 */
enum mp_plugin_id {
	/** Last item for counter only */
	MP_PLUGIN_COUNT,
};

/**
 * @brief Plugin structure
 *
 */
struct mp_plugin {
	/** Plugin id */
	enum mp_plugin_id id;
	/** Plugin initialization function pointer */
	void (*init)(void);
};

/**
 * @brief Macro to register a plugin
 *
 * This macro creates a plugin entry that will be automatically collected
 * into the plugin resgistry and can be initialized by @ref initialize_plugins().
 *
 * @param plugin_id Unique ID given to the plugin
 * @param init_func Pointer to the init function of the plugin
 *
 */
#define MP_PLUGIN_DEFINE(plugin_id, init_func)                                                     \
	static const STRUCT_SECTION_ITERABLE(mp_plugin, plugin_id##_REGISTERED) = {                \
		.id = plugin_id,                                                                   \
		.init = init_func,                                                                 \
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

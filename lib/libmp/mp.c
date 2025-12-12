/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>

#if (CONFIG_MP_CAPSFILTER)
#include <src/core/mp_capsfilter.h>
#endif
#include <src/core/mp_element_factory.h>
#include <src/core/mp_pipeline.h>
#include <src/core/mp_plugin.h>

/**
 * @brief Initialize Media Pipe
 *
 * - Registering built-in elements
 * - Loading all enabled plugins
 */
int mp_init(void)
{
	/* Built-in elements */
	MP_ELEMENT_FACTORY_DEFINE(MP_PIPELINE_ELEM, sizeof(struct mp_pipeline), mp_pipeline_init);

#if (CONFIG_MP_CAPSFILTER)
	MP_ELEMENT_FACTORY_DEFINE(MP_CAPS_FILTER_ELEM, sizeof(struct mp_caps_filter),
				  mp_caps_filter_init);
#endif

	/* Plugins */
	initialize_plugins();

	return 0;
}

SYS_INIT(mp_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

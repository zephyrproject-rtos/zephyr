/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <src/core/mp_element_factory.h>
#include <src/core/mp_pipeline.h>
#include <src/core/mp_plugin.h>

void mp_init()
{
    /* Built-in elements */
    MP_ELEMENTFACTORY_DEFINE(pipeline, sizeof(MpPipeline), mp_pipeline_init);

    /* Plugins */
    initialize_plugins();
}

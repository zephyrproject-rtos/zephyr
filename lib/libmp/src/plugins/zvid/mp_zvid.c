/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <src/core/mp_element_factory.h>
#include <src/core/mp_plugin.h>

#include "mp_zvid_src.h"
#include "mp_zvid_transform.h"

static void plugin_init(void)
{
	MP_ELEMENT_FACTORY_DEFINE(MP_ZVID_SRC_ELEM, sizeof(struct mp_zvid_src), mp_zvid_src_init);
	MP_ELEMENT_FACTORY_DEFINE(MP_ZVID_TRANSFORM_ELEM, sizeof(struct mp_zvid_transform),
				  mp_zvid_transform_init);
}

MP_PLUGIN_DEFINE(MP_ZVID_PLUGIN, plugin_init);

/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include <src/core/mp_element_factory.h>
#include <src/core/mp_plugin.h>

#include "mp_zvid_src.h"
#include "mp_zvid_transform.h"

LOG_MODULE_REGISTER(mp_zvid, CONFIG_LIBMP_LOG_LEVEL);

static void plugin_init()
{
	MP_ELEMENTFACTORY_DEFINE(zvid_src, sizeof(MpZvidSrc), mp_zvid_src_init);
	MP_ELEMENTFACTORY_DEFINE(zvid_transform, sizeof(MpZvidTransform), mp_zvid_transform_init);
}

MP_PLUGIN_DEFINE(zvid, plugin_init);

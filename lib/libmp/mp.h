/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * mp.h: Main header for MediaPipe
 *
 * Applications should include this and only this.
 *
 */

#ifndef __MP_H__
#define __MP_H__

#include <src/core/mp_bus.h>
#if CONFIG_MP_CAPSFILTER
#include <src/core/mp_capsfilter.h>
#endif
#include <src/core/mp_caps.h>
#include <src/core/mp_element.h>
#include <src/core/mp_pipeline.h>

#if CONFIG_MP_PLUGIN_ZVID
#include <src/plugins/zvid/mp_zvid_property.h>
#include <src/plugins/zvid/mp_zvid_src.h>
#include <src/plugins/zvid/mp_zvid_transform.h>
#endif

#endif /* __MP_H__ */

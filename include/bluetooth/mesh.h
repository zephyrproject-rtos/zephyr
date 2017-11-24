/** @file
 *  @brief Bluetooth Mesh Profile APIs.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BT_MESH_H
#define __BT_MESH_H

#include <zephyr/types.h>
#include <stddef.h>
#include <net/buf.h>

#include <bluetooth/mesh/access.h>
#include <bluetooth/mesh/main.h>
#include <bluetooth/mesh/cfg_srv.h>
#include <bluetooth/mesh/health_srv.h>

#if defined(CONFIG_BT_MESH_CFG_CLI)
#include <bluetooth/mesh/cfg_cli.h>
#endif

#if defined(CONFIG_BT_MESH_HEALTH_CLI)
#include <bluetooth/mesh/health_cli.h>
#endif

#if defined(CONFIG_BT_MESH_GATT_PROXY)
#include <bluetooth/mesh/proxy.h>
#endif

#endif /* __BT_MESH_H */

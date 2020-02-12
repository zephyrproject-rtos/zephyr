/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2020 Nordic Semiconductor
 *
 * Not a generated file. Feel free to modify.
 */

#ifndef DEVICETREE_H
#define DEVICETREE_H

/* concatenate the values of the arguments into one */
#define _DT_PHANDLE_GET_PROP(phandle, prop) phandle ## _ ## prop

/* Given a phandle define and a property name get back the actual value */
#define DT_PHANDLE_GET_PROP(phandle, prop) _DT_PHANDLE_GET_PROP(phandle, prop)

#include <devicetree_unfixed.h>
#include <devicetree_fixups.h>

#endif /* DEVICETREE_H */

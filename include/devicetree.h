/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2020 Nordic Semiconductor
 *
 * Not a generated file. Feel free to modify.
 */

#ifndef DEVICETREE_H
#define DEVICETREE_H

/* concatenate the values of the arguments into one */
#define _DT_GET_PROP(ident, prop) DT_ ## ident ## _ ## prop

/* Given an identifier and a property name get back the actual value */
#define DT_GET_PROP(ident, prop) _DT_GET_PROP(ident, prop)

#include <devicetree_unfixed.h>
#include <devicetree_fixups.h>

#endif /* DEVICETREE_H */

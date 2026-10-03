/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_MODULES_TFM_INFINEON_PSE84_IFX_ZEPHYR_DT_H_
#define ZEPHYR_MODULES_TFM_INFINEON_PSE84_IFX_ZEPHYR_DT_H_

/* Gives TF-M sources access to the devicetree of the Zephyr non-secure image. */

#include <zephyr/autoconf.h>

/* ARRAY_SIZE is defined both by TF-M and indirectly by devicetree.h */
#undef ARRAY_SIZE
#include <zephyr/devicetree.h>

#include <zephyr/dt-bindings/clock/ifx_clock_source_common.h>
#include <zephyr/dt-bindings/clock/ifx_clock_source_pse8xx.h>

#endif /* ZEPHYR_MODULES_TFM_INFINEON_PSE84_IFX_ZEPHYR_DT_H_ */

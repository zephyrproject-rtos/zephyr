/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __UWP_HAL_H
#define __UWP_HAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <device.h>
#include <zephyr/types.h>

#include "hal_irq.h"
#include "hal_base.h"
#include "hal_sys.h"
#include "hal_uart.h"
#include "hal_pin.h"
#include "hal_wdg.h"

#include "hal_gpio.h"
#include "hal_ipi.h"
#include "hal_intc.h"
#include "hal_eic.h"
#include "hal_aon_clk.h"
#include "hal_aon_glb.h"
#include "hal_soc.h"

#include "hal_pin_reg.h"
#include "hal_pinmux.h"
#include "hal_pinmap.h"
#include "hal_pinmux_api.h"

#define TRUE   (1)
#define FALSE  (0)

#define SCI_ASSERT(a)
#define mdelay k_sleep

#ifndef BIT
#define BIT(nr) (1UL << (nr))
#endif

#ifdef __cplusplus
}
#endif

#endif

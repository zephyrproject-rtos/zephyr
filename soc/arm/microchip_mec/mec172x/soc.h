/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MEC_SOC_H
#define __MEC_SOC_H

#ifndef _ASMLANGUAGE

/*
 * MEC172x includes the ARM single precision FPU and the ARM MPU with
 * eight regions. Zephyr has an in-tree CMSIS header located in the arch
 * include hierarchy that includes the correct ARM CMSIS core_xxx header
 * from hal_cmsis based on the k-config CPU selection.
 * The Zephyr in-tree header does not provide all the symbols ARM CMSIS
 * requires. Zephyr does not define CMSIS FPU present and defaults CMSIS
 * MPU present to 0. We define these two symbols here based on our k-config
 * selections. NOTE: Zephyr in-tree CMSIS defines the Cortex-M4 hardware
 * revision to 0. At this time ARM CMSIS does not appear to use the hardware
 * revision in any macros.
 */
#define __FPU_PRESENT  CONFIG_CPU_HAS_FPU
#define __MPU_PRESENT  CONFIG_CPU_HAS_ARM_MPU

#include <sys/util.h>

/* chip specific register defines */
#include "reg/mec172x_defs.h"
#include "reg/mec172x_ecia.h"
#include "reg/mec172x_ecs.h"
#include "reg/mec172x_espi_iom.h"
#include "reg/mec172x_espi_saf.h"
#include "reg/mec172x_espi_vw.h"
#include "reg/mec172x_gpio.h"
#include "reg/mec172x_i2c_smb.h"
#include "reg/mec172x_p80bd.h"
#include "reg/mec172x_pcr.h"
#include "reg/mec172x_qspi.h"
#include "reg/mec172x_vbat.h"
#include "reg/mec172x_emi.h"

/* common peripheral register defines */
#include "../common/reg/mec_acpi_ec.h"
#include "../common/reg/mec_adc.h"
#include "../common/reg/mec_global_cfg.h"
#include "../common/reg/mec_kbc.h"
#include "../common/reg/mec_keyscan.h"
#include "../common/reg/mec_peci.h"
#include "../common/reg/mec_ps2.h"
#include "../common/reg/mec_pwm.h"
#include "../common/reg/mec_tach.h"
#include "../common/reg/mec_tfdp.h"
#include "../common/reg/mec_timers.h"
#include "../common/reg/mec_uart.h"
#include "../common/reg/mec_vci.h"
#include "../common/reg/mec_wdt.h"

/* common SoC API */
#include "../common/soc_gpio.h"
#include "../common/soc_pcr.h"
#include "../common/soc_pins.h"
#include "../common/soc_espi_channels.h"
#include "../common/soc_espi_saf.h"
#include "../common/soc_espi_v2.h"

#endif

#endif

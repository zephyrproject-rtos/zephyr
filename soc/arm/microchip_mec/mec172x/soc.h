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
#include "reg/mec172x_pcr.h"

/* common peripheral register defines */
#include "../common/reg/mec_acpi_ec.h"

/* common SoC API */
#include "../common/soc_gpio.h"
#include "../common/soc_pins.h"
#include "../common/soc_espi_channels.h"
#include "../common/soc_espi_saf.h"

#endif

#endif

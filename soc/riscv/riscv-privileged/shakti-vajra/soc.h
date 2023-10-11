/* For Shakti Vajra SOC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the Shakti Vajra processor
 */

#ifndef __RISCV64_SHAKTI_SOC_H_
#define __RISCV64_SHAKTI_SOC_H_

#include <soc_common.h>
/* Timer configuration */
#define RISCV_MTIME_BASE             0x0200BFF8
#define RISCV_MTIMECMP_BASE          0x02004000
/* lib-c hooks required RAM defined variables */
#define RISCV_RAM_BASE               CONFIG_RISCV_RAM_BASE_ADDR
#define RISCV_RAM_SIZE               CONFIG_RISCV_RAM_SIZE

#endif

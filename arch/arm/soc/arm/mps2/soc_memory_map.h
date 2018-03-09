/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_MEMORY_MAP_H_
#define _SOC_MEMORY_MAP_H_

/*
 * Address space definitions
 */

/* MPS2 Address space definition */
#define MPS2_FPGA_APB_BASE_ADDR	0x40020000

/* MPS2 peripherals in FPGA APB subsystem  */
#define FPGAIO_BASE_ADDR	 (MPS2_FPGA_APB_BASE_ADDR + 0x8000)

#endif /* _SOC_MEMORY_MAP_H_ */

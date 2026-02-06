/*
 * Copyright (c) 2026 Fiona Behrens
 *
 * SPDX-LIcense-Identifier: Apache-2.0
 */

#include <stdint.h>

#ifndef ZEPHYR_INCLUDE_DRIVER_CLOCK_CONTROL_NUMICRO_H_
#define ZEPHYR_INCLUDE_DRIVER_CLOCK_CONTROL_NUMICRO_H_

/**
 * @brief SCC Subsystem ID
 */
#define NUMICRO_SCC_SUBSYS_ID_PCC 1

/* Peripheral clock control configuration structure
 */
struct numicro_scc_subsys_pcc {
	uint32_t clk_mod;
	uint32_t clk_src;
	uint32_t clk_div;
};

struct numicro_scc_subsys {
	uint32_t subsys_id;

	union {
		struct numicro_scc_subsys_pcc pcc;
	};
};

#endif /* ZEPHYR_INCLUDE_DRIVER_CLOCK_CONTROL_NUMICRO_H_ */

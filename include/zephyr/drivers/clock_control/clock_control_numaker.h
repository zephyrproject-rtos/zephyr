/*
 * Copyright (c) 2022 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_NUMAKER_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_NUMAKER_H_

/**
 * @brief Enable/disable oscillators or other clocks
 */
#define NUMAKER_SCC_CLKSW_UNTOUCHED     0
#define NUMAKER_SCC_CLKSW_ENABLE        1
#define NUMAKER_SCC_CLKSW_DISABLE       2

/**
 * @brief SCC subsystem ID
 */
#define NUMAKER_SCC_SUBSYS_ID_PCC       1

struct numaker_scc_subsys {
    uint32_t    subsys_id;  /*!< SCC sybsystem ID */

    /* Peripheral clock control configuration structure */
    union {
        struct {
            uint32_t clk_modidx;    /*!< Same as u32ModuleIdx on invoking BSP CLK driver CLK_SetModuleClock() */
            uint32_t clk_src;       /*!< Same as u32ClkSrc on invoking BSP CLK driver CLK_SetModuleClock() */
            uint32_t clk_div;       /*!< Same as u32ClkDiv on invoking BSP CLK driver CLK_SetModuleClock() */
        } pcc;
    };
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_NUMAKER_H_ */

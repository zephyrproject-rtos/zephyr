/*
 * Copyright 2020 Broadcom
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef COUNTER_MCUX_CTIMER_H
#define COUNTER_MCUX_CTIMER_H

#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/dma/dma_mcux_lpc.h>

#if defined(CONFIG_DMA)
/**
 * @brief DMA configuration structure with MCUX DMA private data.
 * 
 */

struct mcux_counter_dma_cfg {
    struct mcux_dma_config mcux_dma_cfg;  /* should be placed at first */
};
#endif
#endif // COUNTER_MCUX_CTIMER_H
/*
 * Copyright 2020-23 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DMA_MCUX_EDMA_H_
#define DMA_MCUX_EDMA_H_

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <soc.h>
#include <fsl_common.h>

#include "fsl_edma.h"
#if defined(FSL_FEATURE_SOC_DMAMUX_COUNT) && FSL_FEATURE_SOC_DMAMUX_COUNT
#include "fsl_dmamux.h"
#endif

#if defined(FSL_FEATURE_MEMORY_HAS_ADDRESS_OFFSET) && FSL_FEATURE_MEMORY_HAS_ADDRESS_OFFSET
#include "fsl_memory.h"
#endif
#endif /* DMA_MCUX_EDMA_H_*/

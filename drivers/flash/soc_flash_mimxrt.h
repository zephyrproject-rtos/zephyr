/*
 * Copyright (c) 2019 Riedo Networks Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * This is the driver for the S26KL family of HyperFlash devices connected
 * to i.MX-RT hybrid micro-controller family. Tested on mimxrt1050_evk.
 * This code is based on * the example "flexspi_hyper_flash_polling_transfer"
 * from NXP's EVKB-IMXRT1050-SDK package.
 */

#ifndef __SOC_FLASH_MIMXRT_H_
#define __SOC_FLASH_MIMXRT_H_


#if CONFIG_FLASH_IMXRT_HYPERFLASH_S26KL
#define HYPERFLASH_CMD_LUT_SEQ_IDX_READDATA 0
#define HYPERFLASH_CMD_LUT_SEQ_IDX_WRITEDATA 1
#define HYPERFLASH_CMD_LUT_SEQ_IDX_READSTATUS 2
#define HYPERFLASH_CMD_LUT_SEQ_IDX_WRITEENABLE 4
#define HYPERFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR 6
#define HYPERFLASH_CMD_LUT_SEQ_IDX_PAGEPROGRAM 10
#define HYPERFLASH_CMD_LUT_SEQ_IDX_ERASECHIP 12
#define CUSTOM_LUT_LENGTH 64
#endif


#include "fsl_common.h"
#include "fsl_flexspi.h"

struct flash_priv {
	struct k_sem write_lock;
};


#endif

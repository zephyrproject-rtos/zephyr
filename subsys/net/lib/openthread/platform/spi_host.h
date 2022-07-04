/*
 * Copyright (c) 2022, Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief
 *   This file includes the Zephyr platform-specific initializers.
 */

#ifndef PLATFORM_SPI_HOST_H_
#define PLATFORM_SPI_HOST_H_

#include <openthread/instance.h>

#ifdef __cplusplus
extern "C" {
#endif

void platformSpiHostInit(void);

bool platformSpiHostCheckInterrupt(void);

bool platformSpiHostWaitForFrame(uint64_t aTimeoutUs);

void platformSpiHostProcess(otInstance *aInstance);

int platformSpiHostTransfer(uint8_t *aSpiTxFrameBuffer,
			    uint8_t *aSpiRxFrameBuffer,
			    uint32_t aTransferLength);

#ifdef __cplusplus
}
#endif

#endif  /* PLATFORM_SPI_HOST_H_ */

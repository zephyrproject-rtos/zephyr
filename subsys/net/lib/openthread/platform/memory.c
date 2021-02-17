/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>

#include <openthread/platform/memory.h>

#include <stdlib.h>

void *otPlatCAlloc(size_t aNum, size_t aSize)
{
	return calloc(aNum, aSize);
}

void otPlatFree(void *aPtr)
{
	free(aPtr);
}

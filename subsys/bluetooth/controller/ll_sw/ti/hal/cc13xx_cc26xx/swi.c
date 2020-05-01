/*
 * Copyright 2020, Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "swi.h"

#include <driverlib/interrupt.h>

void SetPendingIRQ(unsigned int irq)
{
	IntPendSet(irq + 16);
}

void ClearPendingIRQ(unsigned int irq)
{
	IntPendClear(irq + 16);
}

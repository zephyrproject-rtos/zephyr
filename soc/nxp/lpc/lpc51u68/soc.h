/*
 * Copyright (c) 2021 metraTec GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_H_
#define _SOC_H_

#ifndef _ASMLANGUAGE
#include <zephyr/sys/util.h>

#include <fsl_common.h>
#include <soc_common.h>

#endif /* !_ASMLANGUAGE*/

#define IOCON_PIO_DIGITAL_EN	0x80u
#define IOCON_PIO_FUNC0		0x00u
#define IOCON_PIO_FUNC1		0x01u
#define IOCON_PIO_FUNC2		0x02u
#define IOCON_PIO_FUNC3		0x03u
#define IOCON_PIO_FUNC4		0x04u
#define IOCON_PIO_I2CDRIVE_LOW	0x00u
#define IOCON_PIO_I2CFILTER_EN	0x00u
#define IOCON_PIO_I2CSLEW_I2C	0x00u
#define IOCON_PIO_INPFILT_OFF	0x0100u
#define IOCON_PIO_OPENDRAIN_DI	0x00u
#define IOCON_PIO_INV_DI	0x00u
#define IOCON_PIO_MODE_INACT	0x00u
#define IOCON_PIO_SLEW_STANDARD	0x00u
#define IOCON_PIO_MODE_PULLUP	0x10u
#define IOCON_PIO_MODE_PULLDOWN	0x08u

/* Handle variation to implement Wakeup Interrupt */
#undef NXP_ENABLE_WAKEUP_SIGNAL
#undef NXP_DISABLE_WAKEUP_SIGNAL
#define NXP_ENABLE_WAKEUP_SIGNAL(irqn) EnableDeepSleepIRQ(irqn)
#define NXP_DISABLE_WAKEUP_SIGNAL(irqn) DisableDeepSleepIRQ(irqn)

#endif /* _SOC_H_ */

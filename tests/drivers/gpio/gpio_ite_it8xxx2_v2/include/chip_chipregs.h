/*
 * Copyright 2023 The ChromiumOS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <../soc/ite/ec/it8xxx2/chip_chipregs.h>

/*
 * Macros for emulated hardware registers access.
 */
#undef ECREG
#undef ECREG_u16
#undef ECREG_u32
#define ECREG(x)		(*((volatile unsigned char *)fake_ecreg((intptr_t)x)))
#define ECREG_u16(x)		(*((volatile unsigned short *)fake_ecreg((intptr_t)x)))
#define ECREG_u32(x)		(*((volatile unsigned long  *)fake_ecreg((intptr_t)x)))

unsigned int *fake_ecreg(intptr_t r);
uint8_t ite_intc_get_irq_num(void);

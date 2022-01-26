/*
 * Copyright (c) 2022 RIC Electronics
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ATMEL_SAMS70_SOC_FIXUPS_H_
#define _ATMEL_SAMS70_SOC_FIXUPS_H_

/* General fixups that apply to both rev A & rev B */
#define XDMAC_CHID XdmacChid

/* Fixups specific to rev A */
#if !defined(CONFIG_SOC_ATMEL_SAMS70_REVB)

/* USART */
#define US_CR_USART_DTRDIS US_CR_DTRDIS
#define US_CR_USART_RTSDIS US_CR_RTSDIS

#endif

#endif /* _ATMEL_SAMS70_SOC_FIXUPS_H_ */

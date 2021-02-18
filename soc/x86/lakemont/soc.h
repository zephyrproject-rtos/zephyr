/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SOC_H_
#define __SOC_H_

#ifdef CONFIG_BOARD_QEMU_X86_LAKEMONT
/* QEMU uses IO port based UART */
#define UART_NS16550_ACCESS_IOPORT 0x3f8
#endif

#endif /* __SOC_H_ */

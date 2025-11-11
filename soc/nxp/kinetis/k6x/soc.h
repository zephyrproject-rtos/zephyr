/*
 * Copyright (c) 2014-2015 Wind River Systems, Inc.
 * Copyright 2023, 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Board configuration macros for the fsl_frdm_k64f platform
 *
 * This header file is used to specify and describe board-level aspects for the
 * 'fsl_frdm_k64f' platform.
 */

#ifndef _SOC__H_
#define _SOC__H_

#include <zephyr/sys/util.h>
#include <fsl_port.h>

/* address bases */

#define PERIPH_ADDR_BASE_WDOG 0x40052000 /* Watchdog Timer module */

#define PORT_MUX_GPIO kPORT_MuxAsGpio /* GPIO setting for the Port Mux Register */

#ifndef _ASMLANGUAGE

#include <fsl_common.h>
#include <soc_common.h>


#endif /* !_ASMLANGUAGE */

#endif /* _SOC__H_ */

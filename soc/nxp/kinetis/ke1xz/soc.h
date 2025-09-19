/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC__H_
#define _SOC__H_

#include <zephyr/sys/util.h>
#include <fsl_port.h>

#define PORT_MUX_GPIO kPORT_MuxAsGpio /* GPIO setting for the Port Mux Register */

#ifndef _ASMLANGUAGE

#include <fsl_common.h>
#include <soc_common.h>


#endif /* !_ASMLANGUAGE */

#endif /* _SOC__H_ */

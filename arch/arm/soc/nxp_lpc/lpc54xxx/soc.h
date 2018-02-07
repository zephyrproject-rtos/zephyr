/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Board configuration macros for the nxp_lpc54114 platform
 *
 * This header file is used to specify and describe board-level aspects for the
 * 'nxp_lpc54114' platform.
 */

#ifndef _SOC__H_
#define _SOC__H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE
#include <device.h>
#include <misc/util.h>
#include <fsl_common.h>

/* Address of RAM, where the image for core1 should be copied */
#define CORE1_BOOT_ADDRESS				((void *)0x20010000)
extern const char m0_image_start[];
extern const char *m0_image_end;
extern int m0_image_size;
#define CORE1_IMAGE_START				((void *)m0_image_start)
#define CORE1_IMAGE_SIZE				(m0_image_size)



#endif /* !_ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _SOC__H_ */

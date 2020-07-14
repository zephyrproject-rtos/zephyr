/*
 * Copyright (c) 2019 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/hwinfo.h>
#include <string.h>
#include <fsl_sim.h>
#include <sys/byteorder.h>

#if defined(SIM_UIDH)
#define HWINFO_DEVICE_ID_LENGTH_H 1
#else
#define HWINFO_DEVICE_ID_LENGTH_H 0
#endif
#if (defined(FSL_FEATURE_SIM_HAS_UIDM) && FSL_FEATURE_SIM_HAS_UIDM)
#define HWINFO_DEVICE_ID_LENGTH_M 1
#else
#define HWINFO_DEVICE_ID_LENGTH_M 2
#endif
#define HWINFO_DEVICE_ID_LENGTH_L 1

#define HWINFO_DEVICE_ID_LENGTH_TOTAL (HWINFO_DEVICE_ID_LENGTH_L + \
				       HWINFO_DEVICE_ID_LENGTH_M + \
				       HWINFO_DEVICE_ID_LENGTH_H)

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	uint32_t id[HWINFO_DEVICE_ID_LENGTH_TOTAL];
	uint32_t *idp = id;

#if defined(SIM_UIDH)
	*idp++ = sys_cpu_to_be32(SIM->UIDH);
#endif /* SIM_UIDH */

#if (defined(FSL_FEATURE_SIM_HAS_UIDM) && FSL_FEATURE_SIM_HAS_UIDM)
	*idp++ = sys_cpu_to_be32(SIM->UIDM);
#else
	*idp++ = sys_cpu_to_be32(SIM->UIDMH);
	*idp++ = sys_cpu_to_be32(SIM->UIDML);
#endif /* defined(FSL_FEATURE_SIM_HAS_UIDM) && FSL_FEATURE_SIM_HAS_UIDM */
	*idp++ = sys_cpu_to_be32(SIM->UIDL);

	if (length > sizeof(id)) {
		length = sizeof(id);
	}

	memcpy(buffer, id, length);

	return length;
}

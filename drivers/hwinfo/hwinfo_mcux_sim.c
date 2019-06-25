/*
 * Copyright (c) 2019 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/hwinfo.h>
#include <string.h>
#include <fsl_sim.h>

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

struct kinetis_uid {
	u32_t id[HWINFO_DEVICE_ID_LENGTH_TOTAL];
};

ssize_t z_impl_hwinfo_get_device_id(u8_t *buffer, size_t length)
{
	struct kinetis_uid dev_id;

	dev_id.id[0] = SIM->UIDL;
#if (defined(FSL_FEATURE_SIM_HAS_UIDM) && FSL_FEATURE_SIM_HAS_UIDM)
	dev_id.id[1] = SIM->UIDM;
#else
	dev_id.id[1] = SIM->UIDML;
	dev_id.id[2] = SIM->UIDMH;
#if defined(SIM_UIDH)
	dev_id.id[3] = SIM->UIDH;
#endif /* SIM_UIDH */
#endif /* defined(FSL_FEATURE_SIM_HAS_UIDM) && FSL_FEATURE_SIM_HAS_UIDM */

	if (length > sizeof(dev_id.id)) {
		length = sizeof(dev_id.id);
	}

	memcpy(buffer, dev_id.id, length);

	return length;
}

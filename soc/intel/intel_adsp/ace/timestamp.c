/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/spinlock.h>
#include <zephyr/devicetree.h>
#include <adsp_shim.h>
#include <adsp_timestamp.h>

#define TTS_BASE_ADDR   DT_REG_ADDR(DT_NODELABEL(tts))

#define TSCTRL_ADDR     (TTS_BASE_ADDR + ADSP_TSCTRL_OFFSET)
#define ISCS_ADDR       (TTS_BASE_ADDR + ADSP_ISCS_OFFSET)
#define LSCS_ADDR       (TTS_BASE_ADDR + ADSP_LSCS_OFFSET)
#define DWCCS_ADDR      (TTS_BASE_ADDR + ADSP_DWCCS_OFFSET)
#define ARTCS_ADDR      (TTS_BASE_ADDR + ADSP_ARTCS_OFFSET)
#define LWCCS_ADDR      (TTS_BASE_ADDR + ADSP_LWCCS_OFFSET)

/* Copies the bit-field specified by mask from src to dest */
#define BITS_COPY(dest, src, mask)  ((dest) = ((dest) & ~(mask)) | ((src) & (mask)))

static struct k_spinlock lock;

int intel_adsp_get_timestamp(uint32_t tsctrl, struct intel_adsp_timestamp *timestamp)
{
	uint32_t trigger_mask = ADSP_SHIM_TSCTRL_HHTSE | ADSP_SHIM_TSCTRL_ODTS;
	uint32_t trigger_bits = tsctrl & trigger_mask;
	uint32_t tsctrl_temp;
	k_spinlock_key_t key;
	int ret = 0;

	/* Exactly one trigger bit must be set in a valid request */
	if (POPCOUNT(trigger_bits) != 1) {
		return -EINVAL;
	}

	key = k_spin_lock(&lock);

	tsctrl_temp = sys_read32(TSCTRL_ADDR);

	/* Abort if any timestamp capture in progress */
	if (tsctrl_temp & trigger_mask) {
		ret = -EBUSY;
		goto out;
	}

	/* Clear NTK (RW/1C) bit if needed */
	if (tsctrl_temp & ADSP_SHIM_TSCTRL_NTK) {
		sys_write32(tsctrl_temp, TSCTRL_ADDR);
		tsctrl_temp &= ~ADSP_SHIM_TSCTRL_NTK;
	}

	/* Setup the timestamping logic according to request */
	BITS_COPY(tsctrl_temp, tsctrl, ADSP_SHIM_TSCTRL_IONTE);
	BITS_COPY(tsctrl_temp, tsctrl, ADSP_SHIM_TSCTRL_DMATS);
	BITS_COPY(tsctrl_temp, tsctrl, ADSP_SHIM_TSCTRL_CLNKS);
	BITS_COPY(tsctrl_temp, tsctrl, ADSP_SHIM_TSCTRL_LWCS);
	BITS_COPY(tsctrl_temp, tsctrl, ADSP_SHIM_TSCTRL_CDMAS);
	sys_write32(tsctrl_temp, TSCTRL_ADDR);

	/* Start new timestamp capture by setting one of mutually exclusive
	 * trigger bits.
	 */
	tsctrl_temp |= trigger_bits;
	sys_write32(tsctrl_temp, TSCTRL_ADDR);

	/* Wait for timestamp capture to complete */
	while (true) {
		tsctrl_temp = sys_read32(TSCTRL_ADDR);
		if (tsctrl_temp & ADSP_SHIM_TSCTRL_NTK) {
			break;
		}
	}

	/* Copy the timestamp data from HW registers to the snapshot buffer
	 * provided by caller. As NTK bit is high at this stage, the timestamp
	 * data in HW is guaranteed to be valid and static.
	 */
	timestamp->iscs = sys_read32(ISCS_ADDR);
	timestamp->lscs = sys_read64(LSCS_ADDR);
	timestamp->dwccs = sys_read64(DWCCS_ADDR);
	timestamp->artcs = sys_read64(ARTCS_ADDR);
	timestamp->lwccs = sys_read32(LWCCS_ADDR);

	/* Clear NTK (RW/1C) bit */
	tsctrl_temp |= ADSP_SHIM_TSCTRL_NTK;
	sys_write32(tsctrl_temp, TSCTRL_ADDR);

out:
	k_spin_unlock(&lock, key);

	return ret;
}

/*
 * Copyright (c) 2018 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/drivers/hwinfo.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
#ifndef CONFIG_BOARD_QEMU_CORTEX_M0
#include <helpers/nrfx_reset_reason.h>
#endif

#if defined(CONFIG_TRUSTED_EXECUTION_NONSECURE) && defined(NRF_FICR_S)
#include <soc_secure.h>
#else
#include <hal/nrf_ficr.h>
#endif

struct nrf_uid {
	uint32_t id[2];
};

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	struct nrf_uid dev_id;
	uint32_t deviceid[2];

#if defined(CONFIG_TRUSTED_EXECUTION_NONSECURE) && defined(NRF_FICR_S)
	soc_secure_read_deviceid(deviceid);
#else
	deviceid[0] = nrf_ficr_deviceid_get(NRF_FICR, 0);
	deviceid[1] = nrf_ficr_deviceid_get(NRF_FICR, 1);
#endif

	dev_id.id[0] = sys_cpu_to_be32(deviceid[1]);
	dev_id.id[1] = sys_cpu_to_be32(deviceid[0]);

	if (length > sizeof(dev_id.id)) {
		length = sizeof(dev_id.id);
	}

	memcpy(buffer, dev_id.id, length);

	return length;
}

#ifndef CONFIG_BOARD_QEMU_CORTEX_M0
int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	uint32_t flags = 0;

	uint32_t reason = nrfx_reset_reason_get();

	if (reason & NRFX_RESET_REASON_RESETPIN_MASK) {
		flags |= RESET_PIN;
	}
	if (reason & NRFX_RESET_REASON_DOG_MASK) {
		flags |= RESET_WATCHDOG;
	}
	if (reason & NRFX_RESET_REASON_LOCKUP_MASK) {
		flags |= RESET_CPU_LOCKUP;
	}
	if (reason & NRFX_RESET_REASON_OFF_MASK) {
		flags |= RESET_LOW_POWER_WAKE;
	}
	if (reason & NRFX_RESET_REASON_DIF_MASK) {
		flags |= RESET_DEBUG;
	}
	if (reason & NRFX_RESET_REASON_SREQ_MASK) {
		flags |= RESET_SOFTWARE;
	}

#if NRFX_RESET_REASON_HAS_CTRLAP
	if (reason & NRFX_RESET_REASON_CTRLAP_MASK) {
		flags |= RESET_DEBUG;
	}
#endif
#if NRFX_RESET_REASON_HAS_LPCOMP
	if (reason & NRFX_RESET_REASON_LPCOMP_MASK) {
		flags |= RESET_LOW_POWER_WAKE;
	}
#endif
#if NRFX_RESET_REASON_HAS_NFC
	if (reason & NRFX_RESET_REASON_NFC_MASK) {
		flags |= RESET_LOW_POWER_WAKE;
	}
#endif
#if NRFX_RESET_REASON_HAS_VBUS
	if (reason & NRFX_RESET_REASON_VBUS_MASK) {
		flags |= RESET_POR;
	}
#endif
#if NRFX_RESET_REASON_HAS_CTRLAPSOFT
	if (reason & NRFX_RESET_REASON_CTRLAPSOFT_MASK) {
		flags |= RESET_DEBUG;
	}
#endif
#if NRFX_RESET_REASON_HAS_CTRLAPHARD
	if (reason & NRFX_RESET_REASON_CTRLAPHARD_MASK) {
		flags |= RESET_DEBUG;
	}
#endif
#if NRFX_RESET_REASON_HAS_CTRLAPPIN
	if (reason & NRFX_RESET_REASON_CTRLAPPIN_MASK) {
		flags |= RESET_DEBUG;
	}
#endif
#if !NRF_POWER_HAS_RESETREAS
	if (reason & NRFX_RESET_REASON_DOG1_MASK) {
		flags |= RESET_WATCHDOG;
	}
#endif
#if NRFX_RESET_REASON_HAS_GRTC
	if (reason & NRFX_RESET_REASON_GRTC_MASK) {
		flags |= RESET_CLOCK;
	}
#endif
#if NRFX_RESET_REASON_HAS_NETWORK
	if (reason & NRFX_RESET_REASON_LSREQ_MASK) {
		flags |= RESET_SOFTWARE;
	}
	if (reason & NRFX_RESET_REASON_LLOCKUP_MASK) {
		flags |= RESET_CPU_LOCKUP;
	}
	if (reason & NRFX_RESET_REASON_LDOG_MASK) {
		flags |= RESET_WATCHDOG;
	}
	if (reason & NRFX_RESET_REASON_LCTRLAP_MASK) {
		flags |= RESET_DEBUG;
	}
#endif
#if defined(NRFX_RESET_REASON_TAMPC_MASK)
	if (reason & NRFX_RESET_REASON_TAMPC_MASK) {
		flags |= RESET_SECURITY;
	}
#endif
#if defined(NRFX_RESET_REASON_SECTAMPER_MASK)
	if (reason & NRFX_RESET_REASON_SECTAMPER_MASK) {
		flags |= RESET_SECURITY;
	}
#endif

	*cause = flags;

	return 0;
}

int z_impl_hwinfo_clear_reset_cause(void)
{
	uint32_t reason = -1;

	nrfx_reset_reason_clear(reason);

	return 0;
}

int z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	*supported = (RESET_PIN
		      | RESET_WATCHDOG
		      | RESET_SOFTWARE
		      | RESET_CPU_LOCKUP
		      | RESET_LOW_POWER_WAKE
		      | RESET_DEBUG);

	return 0;
}
#endif

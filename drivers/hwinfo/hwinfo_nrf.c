/*
 * Copyright (c) 2018 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/drivers/hwinfo.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
#if defined(CONFIG_BOARD_QEMU_CORTEX_M0) || \
	(defined(CONFIG_NRF_PLATFORM_HALTIUM) && \
	 defined(CONFIG_RISCV_CORE_NORDIC_VPR))
#define RESET_CAUSE_AVAILABLE 0
#else
#include <helpers/nrfx_reset_reason.h>
#define RESET_CAUSE_AVAILABLE 1
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
	uint32_t buf[2];

#if NRF_FICR_HAS_DEVICE_ID || NRF_FICR_HAS_INFO_DEVICE_ID
	/* DEVICEID is accessible, use this */
#if defined(CONFIG_TRUSTED_EXECUTION_NONSECURE) && defined(NRF_FICR_S)
	soc_secure_read_deviceid(buf);
#else
	buf[0] = nrf_ficr_deviceid_get(NRF_FICR, 0);
	buf[1] = nrf_ficr_deviceid_get(NRF_FICR, 1);
#endif
#elif NRF_FICR_HAS_DEVICE_ADDR || NRF_FICR_HAS_BLE_ADDR
	/* DEVICEID is not accessible, use device/Bluetooth LE address instead.
	 * Assume that it is always accessible from the non-secure image.
	 */
	buf[0] = nrf_ficr_deviceaddr_get(NRF_FICR, 0);
	buf[1] = nrf_ficr_deviceaddr_get(NRF_FICR, 1);

	/* Assume that ER and IR are available whenever deviceaddr is.
	 * Use the LSBytes from ER and IR to complete the device id.
	 */
	buf[1] |= (nrf_ficr_er_get(NRF_FICR, 0) & 0xFF) << 16;
	buf[1] |= (nrf_ficr_ir_get(NRF_FICR, 0) & 0xFF) << 24;
#else
#error "No suitable source for hwinfo device_id generation"
#endif

	dev_id.id[0] = sys_cpu_to_be32(buf[1]);
	dev_id.id[1] = sys_cpu_to_be32(buf[0]);

	if (length > sizeof(dev_id.id)) {
		length = sizeof(dev_id.id);
	}

	memcpy(buffer, dev_id.id, length);

	return length;
}

#if RESET_CAUSE_AVAILABLE

#if defined(NRF_RESETINFO)

#define REASON_LOCKUP (NRFX_RESET_REASON_LOCKUP_MASK | NRFX_RESET_REASON_LOCAL_LOCKUP_MASK)
#define REASON_SOFTWARE (NRFX_RESET_REASON_SREQ_MASK | NRFX_RESET_REASON_LOCAL_SREQ_MASK)
#define REASON_WATCHDOG	\
	(NRFX_RESET_REASON_DOG_MASK | \
	 NRFX_RESET_REASON_LOCAL_DOG1_MASK | \
	 NRFX_RESET_REASON_LOCAL_DOG0_MASK)

#else /* NRF_RESETINFO */

#define REASON_LOCKUP NRFX_RESET_REASON_LOCKUP_MASK
#define REASON_SOFTWARE NRFX_RESET_REASON_SREQ_MASK

#if NRF_POWER_HAS_RESETREAS
#define REASON_WATCHDOG NRFX_RESET_REASON_DOG_MASK
#else
#define REASON_WATCHDOG	(NRFX_RESET_REASON_DOG0_MASK | NRFX_RESET_REASON_DOG1_MASK)
#endif /* NRF_POWER_HAS_RESETREAS */

#endif /* NRF_RESETINFO */

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	uint32_t flags = 0;

	uint32_t reason = nrfx_reset_reason_get();

	if (reason & NRFX_RESET_REASON_RESETPIN_MASK) {
		flags |= RESET_PIN;
	}
	if (reason & REASON_WATCHDOG) {
		flags |= RESET_WATCHDOG;
	}

	if (reason & REASON_LOCKUP) {
		flags |= RESET_CPU_LOCKUP;
	}

	if (reason & NRFX_RESET_REASON_OFF_MASK) {
		flags |= RESET_LOW_POWER_WAKE;
	}
	if (reason & NRFX_RESET_REASON_DIF_MASK) {
		flags |= RESET_DEBUG;
	}
	if (reason & REASON_SOFTWARE) {
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
#if NRFX_RESET_REASON_HAS_VBUS
		      | RESET_POR
#endif
#if NRFX_RESET_REASON_HAS_GRTC
		      | RESET_CLOCK
#endif
#if defined(NRFX_RESET_REASON_TAMPC_MASK) || defined(NRFX_RESET_REASON_SECTAMPER_MASK)
		      | RESET_SECURITY
#endif
		      | RESET_DEBUG);

	return 0;
}
#endif /* RESET_CAUSE_AVAILABLE */

/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * ITM tracing backend.
 *
 * Sends the tracing stream out an ITM stimulus port. Trace egress (SWO
 * pin or TPIU parallel trace) is set up by the board or debug probe, not
 * here. Writes poll the stimulus port and drop the rest of a packet if
 * the port stays full, unless stall mode is enabled.
 */

/* Keep this unit from tracing its own syscalls (would recurse). */
#define DISABLE_SYSCALL_TRACING

#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>
#include <cmsis_core.h>
#include <tracing_backend.h>

#define ITM_PORT CONFIG_TRACING_BACKEND_ITM_PORT
#define ITM_ATB_ID CONFIG_TRACING_BACKEND_ITM_BUS_ID

/* "IT" in little-endian. */
#define ITM_TRACE_MAGIC 0x5449U
#define ITM_TRACE_FORMAT_CTF 1U

/*
 * Marker word a host tool reads from the ELF symbol table to detect ITM
 * tracing and its payload format: a 16-bit magic in the low half and the
 * format in bits 16-23. Never read at run time; the linker keeps it via
 * -u and `used` keeps the compiler from dropping it.
 */
__attribute__((used))
const uint32_t itm_trace_descriptor = (ITM_TRACE_FORMAT_CTF << 16) | ITM_TRACE_MAGIC;

/* Reading the port register returns non-zero when it has room. */
static inline bool itm_port_ready(void)
{
#ifdef CONFIG_TRACING_BACKEND_ITM_STALL
	while (ITM->PORT[ITM_PORT].u32 == 0UL) {
	}
	return true;
#else
	for (uint32_t spins = 0; spins < CONFIG_TRACING_BACKEND_ITM_POLL; spins++) {
		if (ITM->PORT[ITM_PORT].u32 != 0UL) {
			return true;
		}
	}
	return false;
#endif
}

static void tracing_backend_itm_output(const struct tracing_backend *backend,
				       uint8_t *data, uint32_t length)
{
	ARG_UNUSED(backend);

	if ((ITM->TCR & ITM_TCR_ITMENA_Msk) == 0UL ||
	    (ITM->TER & (1UL << ITM_PORT)) == 0UL) {
		return;
	}

	uint32_t i = 0;

	while (i + sizeof(uint32_t) <= length) {
		if (!itm_port_ready()) {
			return;
		}
		/* The buffer has no alignment guarantee; the port register
		 * write is volatile through the CMSIS ITM_Type definition.
		 */
		ITM->PORT[ITM_PORT].u32 = UNALIGNED_GET((uint32_t *)&data[i]);
		i += sizeof(uint32_t);
	}

	while (i < length) {
		if (!itm_port_ready()) {
			return;
		}
		ITM->PORT[ITM_PORT].u8 = data[i];
		i++;
	}
}

static void tracing_backend_itm_init(void)
{
	/* Turn on the trace subsystem clock. A debug probe may also set this. */
	DCB->DEMCR |= DCB_DEMCR_TRCENA_Msk;

#if (__CORTEX_M <= 7U)
	/* Unlock the ITM registers (v7-M and earlier). */
	ITM->LAR = 0xC5ACCE55U;
#endif

	/* Enable the ITM and tag its ATB stream. Software stimulus only, so no
	 * DWT forwarding and no local timestamps.
	 */
	ITM->TCR = ITM_TCR_ITMENA_Msk |
		   ((uint32_t)ITM_ATB_ID << ITM_TCR_TRACEBUSID_Pos);

	/* Allow unprivileged code to write the stimulus ports. */
	ITM->TPR = 0U;

	/* Enable just the port this backend uses. */
	ITM->TER |= (1UL << ITM_PORT);
}

const struct tracing_backend_api tracing_backend_itm_api = {
	.init = tracing_backend_itm_init,
	.output = tracing_backend_itm_output,
};

TRACING_BACKEND_DEFINE(tracing_backend_itm, tracing_backend_itm_api);

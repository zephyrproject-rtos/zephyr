/*
 * Copyright (c) 2026 Dhruv Menon <dhruvmenon1104@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * ITM instrumentation backend.
 *
 * Sends dumped instrumentation bytes out an ITM stimulus port. Trace egress
 * (SWO pin or TPIU parallel trace) is set up by the board or debug probe, not
 * here. Writes poll the stimulus port and drop the rest of a packet if the
 * port stays full, unless stall mode is enabled.
 */

#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>
#include <cmsis_core.h>
#include <instr_backend.h>

#define ITM_PORT CONFIG_INSTRUMENTATION_BACKEND_ITM_PORT
#define ITM_ATB_ID CONFIG_INSTRUMENTATION_BACKEND_ITM_BUS_ID

/* "II" in little-endian (instrumentation ITM). */
#define ITM_INSTR_MAGIC 0x4949U
#define ITM_INSTR_FORMAT_DUMP 1U

/*
 * Marker word a host tool reads from the ELF symbol table to detect ITM
 * instrumentation dumps and their payload format. Never read at run time.
 */
__attribute__((used))
const uint32_t itm_instr_descriptor = (ITM_INSTR_FORMAT_DUMP << 16) | ITM_INSTR_MAGIC;

static inline bool itm_port_ready(void)
{
#ifdef CONFIG_INSTRUMENTATION_BACKEND_ITM_STALL
	while (ITM->PORT[ITM_PORT].u32 == 0UL) {
	}
	return true;
#else
	for (uint32_t spins = 0; spins < CONFIG_INSTRUMENTATION_BACKEND_ITM_POLL; spins++) {
		if (ITM->PORT[ITM_PORT].u32 != 0UL) {
			return true;
		}
	}
	return false;
#endif
}

__no_instrumentation__
static void instr_backend_itm_output(const struct instr_backend *backend, uint8_t *data,
				     uint32_t length)
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

__no_instrumentation__
static void instr_backend_itm_init(void)
{
	DCB->DEMCR |= DCB_DEMCR_TRCENA_Msk;

#if (__CORTEX_M <= 7U)
	ITM->LAR = 0xC5ACCE55U;
#endif

	ITM->TCR = ITM_TCR_ITMENA_Msk |
		   ((uint32_t)ITM_ATB_ID << ITM_TCR_TRACEBUSID_Pos);
	ITM->TPR = 0U;
	ITM->TER |= (1UL << ITM_PORT);
}

static const struct instr_backend_api instr_backend_itm_api = {
	.init = instr_backend_itm_init,
	.output = instr_backend_itm_output,
};

INSTR_BACKEND_DEFINE(instr_backend_itm, instr_backend_itm_api);

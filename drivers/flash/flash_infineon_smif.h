/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FLASH_FLASH_INFINEON_SMIF_H_
#define ZEPHYR_DRIVERS_FLASH_FLASH_INFINEON_SMIF_H_

#include <zephyr/kernel.h>

#include "cy_pdl.h"

/*
 * Runtime state owned by an infineon,smif controller device and shared with
 * its attached child devices.
 *
 * The controller performs the one-time SMIF enumeration (Cy_SMIF_MemNumInit)
 * and owns the PDL memory context. Child device drivers reach this structure
 * through their parent device (DT_INST_PARENT) and issue their per-slot
 * transfers against mem_context, serialised by lock.
 */
struct ifx_smif_controller_data {
	/* PDL SMIF memory context (holds the SMIF transfer context). */
	cy_stc_smif_mem_context_t mem_context;
	/* Serialises all bus transfers across the controller's devices. */
	struct k_sem lock;
	/* Set once the controller has been enumerated and is usable. */
	bool ready;
};

#endif /* ZEPHYR_DRIVERS_FLASH_FLASH_INFINEON_SMIF_H_ */

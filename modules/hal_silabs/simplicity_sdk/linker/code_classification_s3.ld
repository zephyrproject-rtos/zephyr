/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Place data annotated as ramfunc into the Zephyr .ramfunc section.
 */

*(text_application_ram)
*(SORT_BY_ALIGNMENT(text_*_timecritical[0-9]*))
*(SORT_BY_ALIGNMENT(text_*_security[0-9]*))

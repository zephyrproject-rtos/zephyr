/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SMIF_PM_H
#define SMIF_PM_H

/**
 * @brief Initialize SMIF power management
 *
 * Registers deep sleep callbacks to automatically manage external memory power
 * states during system sleep/wake cycles. This function must be called during
 * system initialization before any deep sleep transitions occur.
 */
void smif_pm_init(void);

#endif /*SMIF_PM_H*/

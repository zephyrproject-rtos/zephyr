/*
 * Copyright (c) 2024 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Workaround for WFI fused instruction issue */
#define SOC_ON_EXIT_CPU_IDLE __NOP();

/*
 * Copyright (c) 2026 Vossloh AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief STM32 RNG entropy driver definitions
 * @ingroup entropy_interface
 *
 * This file contains definitions for the optional clock management of the
 * STM32 RNG peripheral.
 */

#ifdef CONFIG_ENTROPY_STM32_RNG_CLOCKON_API

/* These APIs are used by other STM32 drivers (e.g. SAES, PKA) to
 * request/release the clock activation of the RNG peripheral when
 * they need it as a pre-requisite for their own operations.
 */

/**
 * @brief Request clock activation of the STM32 RNG peripheral.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int entropy_stm32_clockon_request(void);

/**
 * @brief Release clock activation of the STM32 RNG peripheral.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int entropy_stm32_clockon_release(void);

#endif /* CONFIG_ENTROPY_STM32_RNG_CLOCKON_API */

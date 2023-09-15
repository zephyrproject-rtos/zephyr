/*
 * Copyright (c) 2023, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Encode interrupt flag for x86 architecture.
 *
 * @param polarity the interrupt polarity received from acpica lib
 * @param trigger the interrupt level received from acpica lib
 * @return return encoded interrupt flag
 */
uint32_t arch_acpi_encode_irq_flags(uint8_t polarity, uint8_t trigger);

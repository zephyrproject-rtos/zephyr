/*
 * Copyright (c) 2023, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_X86_ACPI_H_
#define ZEPHYR_INCLUDE_ARCH_X86_X86_ACPI_H_

/**
 * @brief Encode interrupt flag for x86 architecture.
 *
 * @param polarity the interrupt polarity received from ACPICA lib
 * @param trigger the interrupt level received from ACPICA lib
 * @return return encoded interrupt flag
 */
uint32_t arch_acpi_encode_irq_flags(uint8_t polarity, uint8_t trigger);

#endif /* ZEPHYR_INCLUDE_ARCH_X86_X86_ACPI_H_ */

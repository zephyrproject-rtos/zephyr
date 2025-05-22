/*
 * Copyright (c) 2020 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NUVOTON_NPCM_SOC_ESPI_H_
#define _NUVOTON_NPCM_SOC_ESPI_H_

#ifdef __cplusplus
extern "C" {
#endif

/* 8042 event data format */
#define NPCM_8042_EVT_POS 16U
#define NPCM_8042_DATA_POS 8U
#define NPCM_8042_TYPE_POS 0U

/* 8042 event type format */
#define NPCM_8042_EVT_IBF BIT(0)
#define NPCM_8042_EVT_OBE BIT(1)

/*
 * The format of event data for KBC 8042 protocol:
 * [23:16] - 8042 event type: 1: Input buffer full, 2: Output buffer empty.
 * [15:8]  - 8042 data: 8-bit 8042 data.
 * [0:7]   - 8042 protocol type: 0: data type, 1: command type.
 */
#define NPCM_8042_EVT_DATA(event, data, type) (((event) << NPCM_8042_EVT_POS) \
	| ((data) << NPCM_8042_DATA_POS) | ((type) << NPCM_8042_TYPE_POS));

/* ACPI event data format */
#define NPCM_ACPI_DATA_POS 8U
#define NPCM_ACPI_TYPE_POS 0U

/**
 * @brief Turn on all interrupts of eSPI host interface module.
 *
 * @param dev Pointer to structure device of eSPI module
 */
void npcm_espi_enable_interrupts(const struct device *dev);

/**
 * @brief Turn off all interrupts of eSPI host interface module.
 *
 * @param dev Pointer to structure device of eSPI module
 */
void npcm_espi_disable_interrupts(const struct device *dev);

#ifdef __cplusplus
}
#endif

#endif /* _NUVOTON_NPCM_SOC_ESPI_H_ */

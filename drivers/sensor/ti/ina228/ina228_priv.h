/*
 * Copyright (c) 2025 Jonas Berg
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INA228_PRIV_H
#define INA228_PRIV_H

#include <stdint.h>

/* Exposed for unit testing only */

#define INA228_REG_CONFIG          0x00
#define INA228_REG_ADC_CONFIG      0x01
#define INA228_REG_SHUNT_CAL       0x02
#define INA228_REG_SHUNT_TEMPCO    0x03
#define INA228_REG_VSHUNT          0x04
#define INA228_REG_VBUS            0x05
#define INA228_REG_DIETEMP         0x06
#define INA228_REG_CURRENT         0x07
#define INA228_REG_POWER           0x08
#define INA228_REG_ENERGY          0x09
#define INA228_REG_CHARGE          0x0A
#define INA228_REG_DIAG_ALRT       0x0B
#define INA228_REG_SOVL            0x0C
#define INA228_REG_SUVL            0x0D
#define INA228_REG_BOVL            0x0E
#define INA228_REG_BUVL            0x0F
#define INA228_REG_TEMP_LIMIT      0x10
#define INA228_REG_PWR_LIMIT       0x11
#define INA228_REG_MANUFACTURER_ID 0x3E
#define INA228_REG_DEVICE_ID       0x3F

#define INA228_UINT20_MAX 0xFFFFF
#define INA228_INT20_MAX  0x7FFFF
#define INA228_UINT24_MAX 0xFFFFFF
#define INA228_UINT40_MAX 0xFFFFFFFFFF
#define INA228_INT40_MAX  0x7FFFFFFFFF

int32_t ina228_convert_20bits_to_signed(uint32_t input);

#ifdef CONFIG_INA228_CUMULATIVE
int64_t ina228_convert_40bits_to_signed(uint64_t input);
#endif /* CONFIG_INA228_CUMULATIVE */

#endif /* INA228_PRIV_H */

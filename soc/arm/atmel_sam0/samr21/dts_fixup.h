/*
 * Copyright (c) 2017 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* SoC level DTS fixup file */

#define DT_FLASH_DEV_NAME DT_LABEL(DT_INST(0, atmel_sam0_nvmctrl))

#define DT_I2C_0_NAME DT_LABEL(DT_INST(0, atmel_sam0_i2c))

#define DT_NUM_IRQ_PRIO_BITS	DT_ARM_V6M_NVIC_E000E100_ARM_NUM_IRQ_PRIORITY_BITS

/* End of SoC Level DTS fixup file */

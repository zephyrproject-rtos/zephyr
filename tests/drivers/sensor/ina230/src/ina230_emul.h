/*
 * Copyright (c) 2023 North River Systems Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Emulator for the TI INA230 I2C power monitor
 */
#ifndef INA230_EMUL_H_
#define INA230_EMUL_H_

#define INA230_REGISTER_COUNT 8

int ina230_mock_set_register(void *data_ptr, int reg, uint32_t value);
int ina230_mock_get_register(void *data_ptr, int reg, uint32_t *value_ptr);

#endif /* INA230_EMUL_H_ */

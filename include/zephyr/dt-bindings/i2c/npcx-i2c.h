/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_I2C_NPCX_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_I2C_NPCX_H_


#define NPCX_I2C_CTRL_PORT(ctrl, port) (((ctrl & 0xf) << 4) | (port & 0xf))

#endif	/* ZEPHYR_INCLUDE_DT_BINDINGS_I2C_NPCX_H_ */

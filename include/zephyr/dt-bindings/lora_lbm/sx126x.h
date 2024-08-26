/*
 * Copyright (c) 2024 Semtech Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_LORA_LBM_SX126X_BINDINGS_DEF_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_LORA_LBM_SX126X_BINDINGS_DEF_H_

/* Those definitions were copied from
 * modules/lib/lora_basics_modem/lbm_lib/smtc_modem_core/radio_drivers/sx126x_driver/src/sx126x.h
 */

#define SX126X_REG_MODE_LDO  0x00 /* default */
#define SX126X_REG_MODE_DCDC 0x01

#define SX126X_TCXO_SUPPLY_1_6V 0x00
#define SX126X_TCXO_SUPPLY_1_7V 0x01
#define SX126X_TCXO_SUPPLY_1_8V 0x02
#define SX126X_TCXO_SUPPLY_2_2V 0x03
#define SX126X_TCXO_SUPPLY_2_4V 0x04
#define SX126X_TCXO_SUPPLY_2_7V 0x05
#define SX126X_TCXO_SUPPLY_3_0V 0x06
#define SX126X_TCXO_SUPPLY_3_3V 0x07

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_LORA_LBM_SX126X_BINDINGS_DEF_H_*/

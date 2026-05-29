/*
 * Copyright (c) 2020 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NUVOTON_NPCX_SOC_PINS_H_
#define _NUVOTON_NPCX_SOC_PINS_H_

#include <stdint.h>

#include "reg_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief NPCX pin-mux configuration structure
 *
 * Used to indicate the device's corresponding DEVALT register/bit for
 * pin-muxing and its polarity to enable alternative functionality.
 */
struct npcx_alt {
	uint8_t group;
	uint8_t bit:3;
	uint8_t inverted:1;
	uint8_t reserved:4;
};

/**
 * @brief NPCX low-voltage configuration structure
 *
 * Used to indicate the device's corresponding LV_GPIO_CTL register/bit for
 * low-voltage detection.
 */
struct npcx_lvol {
	uint8_t ctrl:5; /** Related register index for low-voltage conf. */
	uint8_t bit:3; /** Related register bit for low-voltage conf. */
};

/**
 * @brief Select i2c port pads of i2c controller
 *
 * @param controller i2c controller device
 * @param port index for i2c port pads
 */
void npcx_pinctrl_i2c_port_sel(int controller, int port);

/**
 * @brief Force the internal SPI flash write-protect pin (WP) to low level to
 * protect the flash Status registers.
 */
int npcx_pinctrl_flash_write_protect_set(void);

/**
 * @brief Get write protection status
 *
 * @return 1 if write protection is set, 0 otherwise.
 */
bool npcx_pinctrl_flash_write_protect_is_set(void);

/**
 * @brief Enable low-voltage input detection
 *
 * @param lvol_ctrl Related register index for low-voltage detection
 * @param lvol_bit Related register bit for low-voltage detection
 * @param enable True to enable low-voltage input detection, false to disable.
 */
void npcx_lvol_set_detect_level(int lvol_ctrl, int lvol_bit, bool enable);

/**
 * @brief Get status of low-voltage input detection
 *
 * @param lvol_ctrl Related register index for low-voltage detection
 * @param lvol_bit Related register bit for low-voltage detection
 * @return True means the low-voltage power supply is enabled, otherwise disabled.
 */
bool npcx_lvol_get_detect_level(int lvol_ctrl, int lvol_bit);

/**
 * @brief Select the host interface type
 *
 * @param hif_type host interface type
 */
void npcx_host_interface_sel(enum npcx_hif_type hif_type);

/**
 * @brief Select the I3C module controller or target mode for the mdma
 *        module operation.
 *
 * @param module i3c device
 * @param enable true to enable target mode, false to controller mode.
 */
void npcx_i3c_target_sel(uint8_t module, bool enable);

/**
 * @brief Enable smb controller wake up event detection in target mode
 *
 * @param controller i2c controller device
 * @param enable True to enable wake up event detection, false to disable.
 */
void npcx_i2c_target_start_wk_enable(int controller, bool enable);

/**
 * @brief Clear  wake up event detection status in target mode
 *
 */
void npcx_i2c_target_clear_detection_event(void);

#ifdef __cplusplus
}
#endif

#endif /* _NUVOTON_NPCX_SOC_PINS_H_ */

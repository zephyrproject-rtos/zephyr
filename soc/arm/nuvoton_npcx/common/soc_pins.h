/*
 * Copyright (c) 2020 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NUVOTON_NPCX_SOC_PINS_H_
#define _NUVOTON_NPCX_SOC_PINS_H_

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
	uint16_t io_port:5; /** A io pad's port which support low-voltage. */
	uint16_t io_bit:3; /** A io pad's bit which support low-voltage. */
	uint16_t ctrl:5; /** Related register index for low-voltage conf. */
	uint16_t bit:3; /** Related register bit for low-voltage conf. */
};

/**
 * @brief NPCX Power Switch Logic (PSL) input configuration structure
 *
 * Used to configure PSL input pad which detect the wake-up events and switch
 * core power supply (VCC1) on from standby power state (ultra-low-power mode).
 */
struct npcx_psl_in {
	/** flag to indicate the detection mode and type. */
	uint32_t flag;
	/** offset in PSL_CTS for status and detection mode. */
	uint32_t offset;
	/** Device Alternate Function. (DEVALT) register/bit for PSL pin-muxing.
	 * It determines whether PSL input or GPIO selected to the pad.
	 */
	struct npcx_alt pinctrl;
	/** Device Alternate Function. (DEVALT) register/bit for PSL polarity.
	 * It determines active polarity of wake-up signal via PSL input.
	 */
	struct npcx_alt polarity;
};

/**
 * @brief Select device pin-mux to I/O or its alternative functionality
 *
 * Example devicetree fragment:
 *    / {
 *		uart1: serial@400c4000 {
 *			//altfunc 0: PIN64.65, otherwise CR_SIN1 CR_SOUT1
 *			pinctrl = <&altc_uart1_sl2>;
 *			...
 *		};
 *
 *		kscan0: kscan@400a3000 {
 *			//altfunc 0: PIN31.xx PIN21.xx, otherwise KSO0-x KSI0-x
 *			pinctrl = <&alt7_no_ksi0_sl ...
 *			           &alt8_no_kso00_sl ...>;
 *			...
 *		};
 *	};
 *
 * Example usage:
 *    - Pinmux configuration list
 *         const struct npcx_alt alts_list[] = NPCX_DT_ALT_ITEMS_LIST(inst);
 *    - Change pinmux to UART:
 *         npcx_pinctrl_mux_configure(alts_list, ARRAY_SIZE(alts_list), 1);
 *    - Change pinmux back to GPIO64.65:
 *         npcx_pinctrl_mux_configure(alts_list, ARRAY_SIZE(alts_list), 0);
 *
 * Please refer more details in Table 3. (Pin Multiplexing Configuration).
 *
 * @param alts_list Pointer to pin-mux configuration list for specific device
 * @param alts_size Pin-mux configuration list size
 * @param altfunc 0: set pin-mux to GPIO, otherwise specific functionality
 */
void npcx_pinctrl_mux_configure(const struct npcx_alt *alts_list,
			uint8_t alts_size, int altfunc);

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
 * @brief Set PSL output pad to inactive level.
 *
 * The PSL_OUT output pad should be connected to the control pin of either the
 * switch or the power supply used generate the VCC1 power from the VSBY power.
 * When PSL_OUT is high (active), the Core Domain power supply (VCC1) is turned
 * on. When PSL_OUT is low (inactive) by setting bit of related PDOUT, VCC1 is
 * turned off for entering standby power state (ultra-low-power mode).
 */
void npcx_pinctrl_psl_output_set_inactive(void);

/**
 * @brief Configure PSL input pads in psl_in_pads list
 *
 * Used to configure PSL input pads list from "psl-in-pads" property which
 * detect the wake-up events and the related circuit will turn on core power
 * supply (VCC1) from standby power state (ultra-low-power mode) later.
 */
void npcx_pinctrl_psl_input_configure(void);

/**
 * @brief Get the asserted status of PSL input pads
 *
 * @param i index of 'psl-in-pads' prop
 * @return 1 is asserted, otherwise de-asserted.
 */
bool npcx_pinctrl_psl_input_asserted(uint32_t i);

/**
 * @brief Restore all connections between IO pads that support low-voltage power
 *        supply and GPIO hardware devices. This utility is used for solving a
 *        leakage current issue found in npcx7 series. The npcx9 and later
 *        series fixed the issue and needn't it.
 */
void npcx_lvol_restore_io_pads(void);

/**
 * @brief Disable all connections between IO pads that support low-voltage power
 *        supply and GPIO hardware devices. This utility is used for solving a
 *        leakage current issue found in npcx7 series. The npcx9 and later
 *        series fixed the issue and needn't it.
 */
void npcx_lvol_suspend_io_pads(void);

/**
 * @brief Get the low-voltage power supply status of GPIO pads
 *
 * @param port port index of GPIO device
 * @param pin pin of GPIO device
 * @return 1 means the low-voltage power supply is enabled, otherwise disabled.
 */
bool npcx_lvol_is_enabled(int port, int pin);

#ifdef __cplusplus
}
#endif

#endif /* _NUVOTON_NPCX_SOC_PINS_H_ */

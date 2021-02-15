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
	uint8_t group:4;
	uint8_t bit:3;
	uint8_t inverted:1;
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

#ifdef __cplusplus
}
#endif

#endif /* _NUVOTON_NPCX_SOC_PINS_H_ */

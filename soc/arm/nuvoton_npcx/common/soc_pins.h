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
 *         const struct npcx_alt alts_list[] = DT_NPCX_ALT_ITEMS_LIST(inst);
 *    - Change pinmux to UART:
 *         soc_pinctrl_mux_configure(alts_list, ARRAY_SIZE(alts_list), 1);
 *    - Change pinmux back to GPIO64.65:
 *         soc_pinctrl_mux_configure(alts_list, ARRAY_SIZE(alts_list), 0);
 *
 * Please refer more details in Table 3. (Pin Multiplexing Configuration).
 *
 * @param alts_list Pointer to pin-mux configuration list for specific device
 * @param alts_size Pin-mux configuration list size
 * @param altfunc 0: set pin-mux to GPIO, otherwise specific functionality
 */
void soc_pinctrl_mux_configure(const struct npcx_alt *alts_list,
			uint8_t alts_size, int altfunc);

#ifdef __cplusplus
}
#endif

#endif /* _NUVOTON_NPCX_SOC_PINS_H_ */

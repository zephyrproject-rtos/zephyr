/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct ll_adv_set {
	u8_t  chl_map:3;
	u8_t  filter_policy:2;
#if defined(CONFIG_BLUETOOTH_CONTROLLER_PRIVACY)
	u8_t  rl_idx:5;
	u8_t  own_addr_type:2;
	u8_t  id_addr_type:1;
	u8_t  id_addr[BDADDR_SIZE];
#endif /* CONFIG_BLUETOOTH_CONTROLLER_PRIVACY */

#if defined(CONFIG_BLUETOOTH_CONTROLLER_ADV_EXT)
	u8_t  phy_p:3;
	u32_t interval;
#else /* !CONFIG_BLUETOOTH_CONTROLLER_ADV_EXT */
	u16_t interval;
#endif /* !CONFIG_BLUETOOTH_CONTROLLER_ADV_EXT */
};

struct ll_adv_set *ll_adv_set_get(void);

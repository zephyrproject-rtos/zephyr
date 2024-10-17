/*
 * Copyright (c) 2024 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PHY_STM32U5_OTG_HS_PHY_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PHY_STM32U5_OTG_HS_PHY_H_

/* Ideally we'd generate this list to match what's coming out of the YAML.
 * Make sure it's kept up to date with the yaml file in question.
 * (dts/bindings/phy/st,stm32-otghs-phy.yaml)
 */

#define STM32U5_OTGHS_PHY_CLKSEL_16MHZ   0
#define STM32U5_OTGHS_PHY_CLKSEL_19P2MHZ 1
#define STM32U5_OTGHS_PHY_CLKSEL_20MHZ   2
#define STM32U5_OTGHS_PHY_CLKSEL_24MHZ   3
#define STM32U5_OTGHS_PHY_CLKSEL_26MHZ   4
#define STM32U5_OTGHS_PHY_CLKSEL_32MHZ   5

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PHY_STM32U5_OTG_HS_PHY_H_ */

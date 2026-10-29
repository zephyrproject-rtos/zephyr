/*
 * Copyright (c) 2022 Grant Ramsay <grant.ramsay@hotmail.com>
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_ETHERNET_ETH_ESP32_PRIV_H_
#define ZEPHYR_DRIVERS_ETHERNET_ETH_ESP32_PRIV_H_

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/pinctrl/esp-pinctrl-common.h>
#include <zephyr/net/phy.h>

#include <esp_clk_tree.h>
#include <esp_private/esp_clk_tree_common.h>
#include <hal/emac_ll.h>
#include <hal/emac_periph.h>
#include <hal/gpio_ll.h>
#include <soc/gpio_periph.h>
#include <soc/io_mux_reg.h>
#if SOC_EMAC_SUPPORT_1000M
#include <hal/emac_clk.h>
#endif

/* True when the node selects RGMII on a SoC with a gigabit capable MAC */
#if SOC_EMAC_SUPPORT_1000M
#define ESP32_EMAC_DT_IS_RGMII(node_id)                                                            \
	(DT_ENUM_HAS_VALUE(node_id, phy_connection_type, rgmii) ||                                 \
	 DT_ENUM_HAS_VALUE(node_id, phy_connection_type, rgmii_id) ||                              \
	 DT_ENUM_HAS_VALUE(node_id, phy_connection_type, rgmii_rxid) ||                            \
	 DT_ENUM_HAS_VALUE(node_id, phy_connection_type, rgmii_txid))
#else
#define ESP32_EMAC_DT_IS_RGMII(node_id) 0
#endif

/*
 * Find the IOMUX entry matching a specific GPIO number.
 * Returns the entry pointer, or the first entry if gpio_num < 0.
 */
static inline const emac_iomux_info_t *esp32_emac_iomux_find(const emac_iomux_info_t *info,
							     int gpio_num)
{
	if (info == NULL) {
		return NULL;
	}

	if (gpio_num < 0) {
		return info;
	}

	while (info->gpio_num != GPIO_NUM_MAX) {
		if (info->gpio_num == gpio_num) {
			return info;
		}
		info++;
	}

	return NULL;
}

static inline void esp32_emac_iomux_output(const emac_iomux_info_t *pin, uint32_t sig_idx)
{
	if (pin == NULL) {
		return;
	}
	emac_ll_gpio_init(NULL, pin->gpio_num);
	gpio_ll_func_sel(&GPIO, pin->gpio_num, pin->func);
	ARG_UNUSED(sig_idx);
}

static inline void esp32_emac_iomux_input(const emac_iomux_info_t *pin, uint32_t sig_idx)
{
	if (pin == NULL) {
		return;
	}
	emac_ll_gpio_init(NULL, pin->gpio_num);
	gpio_ll_input_enable(&GPIO, pin->gpio_num);
	gpio_ll_func_sel(&GPIO, pin->gpio_num, pin->func);
#if SOC_EMAC_USE_MULTI_IO_MUX
	/*
	 * On P4, tell the peripheral to read input from IOMUX pad
	 * (sig_in_sel=0) rather than through GPIO matrix.
	 */
	gpio_ll_set_input_signal_from(&GPIO, sig_idx, false);
#else
	ARG_UNUSED(sig_idx);
#endif
}

/*
 * Initialize RMII data plane IOMUX pins.
 *
 * On ESP32, each signal has a single dedicated IOMUX pin.
 * On ESP32-P4 (SOC_EMAC_USE_MULTI_IO_MUX), each signal has
 * multiple IOMUX options and input signals need sig_in_sel
 * configured to read from IOMUX pad.
 * Pass the desired GPIO number per signal, or -1 for default.
 */
static inline void esp32_emac_iomux_init_rmii(int gpio_tx_en, int gpio_txd0, int gpio_txd1,
					      int gpio_crs_dv, int gpio_rxd0, int gpio_rxd1)
{
	esp32_emac_iomux_output(esp32_emac_iomux_find(emac_rmii_iomux_pins.tx_en, gpio_tx_en),
				emac_io_idx.mii_tx_en_o_idx);
	esp32_emac_iomux_output(esp32_emac_iomux_find(emac_rmii_iomux_pins.txd0, gpio_txd0),
				emac_io_idx.mii_txd0_o_idx);
	esp32_emac_iomux_output(esp32_emac_iomux_find(emac_rmii_iomux_pins.txd1, gpio_txd1),
				emac_io_idx.mii_txd1_o_idx);
	esp32_emac_iomux_input(esp32_emac_iomux_find(emac_rmii_iomux_pins.crs_dv, gpio_crs_dv),
			       emac_io_idx.mii_rx_dv_i_idx);
	esp32_emac_iomux_input(esp32_emac_iomux_find(emac_rmii_iomux_pins.rxd0, gpio_rxd0),
			       emac_io_idx.mii_rxd0_i_idx);
	esp32_emac_iomux_input(esp32_emac_iomux_find(emac_rmii_iomux_pins.rxd1, gpio_rxd1),
			       emac_io_idx.mii_rxd1_i_idx);
}

/* Collect the GPIO of each EMAC IOMUX slot from the pinctrl-0 "default" state.
 * The EMAC data plane pads bypass the GPIO matrix, so pinctrl_apply_state()
 * is not used: the cells only carry (slot, GPIO) pairs (see
 * ESP32_RMII_PINMUX). Slots not listed are set to -1.
 */
static inline int esp32_emac_pinctrl_gpios(const struct pinctrl_dev_config *pcfg, int *gpios,
					   size_t count)
{
	const struct pinctrl_state *state;
	int res;

	for (size_t i = 0; i < count; i++) {
		gpios[i] = -1;
	}

	res = pinctrl_lookup_state(pcfg, PINCTRL_STATE_DEFAULT, &state);
	if (res != 0) {
		return res;
	}

	for (uint8_t i = 0; i < state->pin_cnt; i++) {
		uint32_t cell = state->pins[i].pinmux;

		if ((cell & ESP32_RMII_MARKER) == 0) {
			continue;
		}

		uint32_t slot = (cell >> ESP32_RMII_SLOT_SHIFT) & ESP32_RMII_SLOT_MASK;
		int gpio = (cell >> ESP32_PIN_NUM_SHIFT) & ESP32_PIN_NUM_MASK;

		if (slot < count) {
			gpios[slot] = gpio;
		}
	}

	return 0;
}

/* Route the RMII GPIOs of the pinctrl-0 "default" state through the
 * dedicated EMAC IOMUX. REF_CLK is returned via clk_gpio.
 */
static inline int esp32_emac_iomux_init_rmii_pinctrl(const struct pinctrl_dev_config *pcfg,
						     int *clk_gpio)
{
	int gpios[ESP_RMII_SLOT_COUNT];
	int res;

	BUILD_ASSERT(ESP_RMII_SLOT_COUNT == ESP_RMII_RXD1 + 1,
		     "ESP_RMII_SLOT_COUNT out of sync with the ESP_RMII_* slots");

	res = esp32_emac_pinctrl_gpios(pcfg, gpios, ARRAY_SIZE(gpios));
	if (res != 0) {
		return res;
	}

	esp32_emac_iomux_init_rmii(gpios[ESP_RMII_TX_EN], gpios[ESP_RMII_TXD0],
				   gpios[ESP_RMII_TXD1], gpios[ESP_RMII_CRS_DV],
				   gpios[ESP_RMII_RXD0], gpios[ESP_RMII_RXD1]);

	if (clk_gpio != NULL) {
		*clk_gpio = gpios[ESP_RMII_CLK];
	}

	return 0;
}

static inline void esp32_emac_iomux_rmii_clk_output(int gpio_num)
{
	const emac_iomux_info_t *pin = emac_rmii_iomux_pins.clko;

	/*
	 * GPIO0 uses CLK_OUT1 instead of dedicated IOMUX, so skip
	 * IOMUX setup for it. GPIO16/17 use dedicated IOMUX.
	 */
	if (gpio_num == 0) {
		return;
	}

	esp32_emac_iomux_output(esp32_emac_iomux_find(pin, gpio_num), 0);
}

#if DT_NODE_HAS_PROP(DT_INST(0, espressif_esp32_eth), ref_clk_output_gpios)
static inline int esp32_emac_config_apll_clock(void)
{
	uint32_t expt_freq = MHZ(50);
	uint32_t real_freq = 0;
	esp_err_t ret = esp_clk_tree_src_set_freq_hz(SOC_MOD_CLK_APLL, expt_freq, &real_freq);

	if (ret == ESP_ERR_INVALID_ARG) {
		LOG_ERR("Set APLL clock coefficients failed");
		return -EIO;
	}

	/* If the difference of real APLL frequency
	 * is not within 50 ppm, i.e. 2500 Hz,
	 * the APLL is unavailable
	 */
	if (abs((int)real_freq - (int)expt_freq) > 2500) {
		LOG_ERR("The APLL is working at an unusable frequency");
		return -EIO;
	}

	return 0;
}
#else
static inline void esp32_emac_iomux_rmii_clk_input(int gpio_num)
{
	const emac_iomux_info_t *pin;

	pin = esp32_emac_iomux_find(emac_rmii_iomux_pins.clki, gpio_num);
	if (pin != NULL) {
		gpio_ll_input_enable(&GPIO, pin->gpio_num);
		gpio_ll_func_sel(&GPIO, pin->gpio_num, pin->func);
	}
}
#endif

static inline void esp32_emac_iomux_init_mii(void)
{
	const emac_iomux_info_t *pin;

	/*
	 * ESP32 EMAC uses dedicated IOMUX pins, not GPIO matrix.
	 * Only the IOMUX function select is needed to route signals.
	 */

	/* TX_CLK - input */
	pin = emac_mii_iomux_pins.clk_tx;
	if (pin != NULL) {
		gpio_ll_input_enable(&GPIO, pin->gpio_num);
		gpio_ll_func_sel(&GPIO, pin->gpio_num, pin->func);
	}

	/* TX_EN - output */
	pin = emac_mii_iomux_pins.tx_en;
	if (pin != NULL) {
		gpio_ll_func_sel(&GPIO, pin->gpio_num, pin->func);
	}

	/* TXD0-3 - outputs */
	pin = emac_mii_iomux_pins.txd0;
	if (pin != NULL) {
		gpio_ll_func_sel(&GPIO, pin->gpio_num, pin->func);
	}

	pin = emac_mii_iomux_pins.txd1;
	if (pin != NULL) {
		gpio_ll_func_sel(&GPIO, pin->gpio_num, pin->func);
	}

	pin = emac_mii_iomux_pins.txd2;
	if (pin != NULL) {
		gpio_ll_func_sel(&GPIO, pin->gpio_num, pin->func);
	}

	pin = emac_mii_iomux_pins.txd3;
	if (pin != NULL) {
		gpio_ll_func_sel(&GPIO, pin->gpio_num, pin->func);
	}

	/* RX_CLK - input */
	pin = emac_mii_iomux_pins.clk_rx;
	if (pin != NULL) {
		gpio_ll_input_enable(&GPIO, pin->gpio_num);
		gpio_ll_func_sel(&GPIO, pin->gpio_num, pin->func);
	}

	/* RX_DV - input */
	pin = emac_mii_iomux_pins.rx_dv;
	if (pin != NULL) {
		gpio_ll_input_enable(&GPIO, pin->gpio_num);
		gpio_ll_func_sel(&GPIO, pin->gpio_num, pin->func);
	}

	/* RXD0-3 - inputs */
	pin = emac_mii_iomux_pins.rxd0;
	if (pin != NULL) {
		gpio_ll_input_enable(&GPIO, pin->gpio_num);
		gpio_ll_func_sel(&GPIO, pin->gpio_num, pin->func);
	}

	pin = emac_mii_iomux_pins.rxd1;
	if (pin != NULL) {
		gpio_ll_input_enable(&GPIO, pin->gpio_num);
		gpio_ll_func_sel(&GPIO, pin->gpio_num, pin->func);
	}

	pin = emac_mii_iomux_pins.rxd2;
	if (pin != NULL) {
		gpio_ll_input_enable(&GPIO, pin->gpio_num);
		gpio_ll_func_sel(&GPIO, pin->gpio_num, pin->func);
	}

	pin = emac_mii_iomux_pins.rxd3;
	if (pin != NULL) {
		gpio_ll_input_enable(&GPIO, pin->gpio_num);
		gpio_ll_func_sel(&GPIO, pin->gpio_num, pin->func);
	}
}

#if !defined(CONFIG_SOC_SERIES_ESP32)
/* Divide the 50 MHz RMII reference down to the RX/TX clock of the given link
 * speed: 25 MHz at 100 Mbps and 2.5 MHz at 10 Mbps.
 */
static inline void esp32_emac_rmii_link_speed(enum phy_link_speed speed)
{
	emac_ll_clock_rmii_rx_tx_div(NULL, PHY_LINK_IS_SPEED_10M(speed) ? 19 : 1);
}
#endif

#if SOC_EMAC_SUPPORT_1000M
#define ESP32_EMAC_RGMII_CLK_HZ  MHZ(125)
#define ESP32_EMAC_RGMII_CLK_PPM 100U

/* Route the RGMII GPIOs of the pinctrl-0 "default" state through the
 * dedicated EMAC IOMUX. Slots not listed use the first IOMUX option.
 */
static inline int esp32_emac_iomux_init_rgmii_pinctrl(const struct pinctrl_dev_config *pcfg)
{
	const emac_rgmii_iomux_info_t *pins = &emac_rgmii_iomux_pins;
	int gpios[ESP_RGMII_SLOT_COUNT];
	int res;

	BUILD_ASSERT(ESP_RGMII_SLOT_COUNT == ESP_RGMII_RXD3 + 1,
		     "ESP_RGMII_SLOT_COUNT out of sync with the ESP_RGMII_* slots");

	res = esp32_emac_pinctrl_gpios(pcfg, gpios, ARRAY_SIZE(gpios));
	if (res != 0) {
		return res;
	}

	esp32_emac_iomux_output(esp32_emac_iomux_find(pins->clk_tx, gpios[ESP_RGMII_TX_CLK]),
				emac_io_idx.mii_tx_clk_i_idx);
	esp32_emac_iomux_output(esp32_emac_iomux_find(pins->tx_ctl, gpios[ESP_RGMII_TX_CTL]),
				emac_io_idx.mii_tx_en_o_idx);
	esp32_emac_iomux_output(esp32_emac_iomux_find(pins->txd0, gpios[ESP_RGMII_TXD0]),
				emac_io_idx.mii_txd0_o_idx);
	esp32_emac_iomux_output(esp32_emac_iomux_find(pins->txd1, gpios[ESP_RGMII_TXD1]),
				emac_io_idx.mii_txd1_o_idx);
	esp32_emac_iomux_output(esp32_emac_iomux_find(pins->txd2, gpios[ESP_RGMII_TXD2]),
				emac_io_idx.mii_txd2_o_idx);
	esp32_emac_iomux_output(esp32_emac_iomux_find(pins->txd3, gpios[ESP_RGMII_TXD3]),
				emac_io_idx.mii_txd3_o_idx);
	esp32_emac_iomux_input(esp32_emac_iomux_find(pins->clk_rx, gpios[ESP_RGMII_RX_CLK]),
			       emac_io_idx.mii_rx_clk_i_idx);
	esp32_emac_iomux_input(esp32_emac_iomux_find(pins->rx_ctl, gpios[ESP_RGMII_RX_CTL]),
			       emac_io_idx.mii_rx_dv_i_idx);
	esp32_emac_iomux_input(esp32_emac_iomux_find(pins->rxd0, gpios[ESP_RGMII_RXD0]),
			       emac_io_idx.mii_rxd0_i_idx);
	esp32_emac_iomux_input(esp32_emac_iomux_find(pins->rxd1, gpios[ESP_RGMII_RXD1]),
			       emac_io_idx.mii_rxd1_i_idx);
	esp32_emac_iomux_input(esp32_emac_iomux_find(pins->rxd2, gpios[ESP_RGMII_RXD2]),
			       emac_io_idx.mii_rxd2_i_idx);
	esp32_emac_iomux_input(esp32_emac_iomux_find(pins->rxd3, gpios[ESP_RGMII_RXD3]),
			       emac_io_idx.mii_rxd3_i_idx);

	return 0;
}

/* Internal PLL that clocks the RGMII TX path */
static inline soc_module_clk_t *esp32_emac_rgmii_clk(void)
{
	static soc_module_clk_t clk = SOC_MOD_CLK_INVALID;

	return &clk;
}

static inline uint32_t esp32_emac_rgmii_pll_hz(void)
{
	uint32_t freq = 0;

	esp_clk_tree_src_get_freq_hz(*esp32_emac_rgmii_clk(), ESP_CLK_TREE_SRC_FREQ_PRECISION_EXACT,
				     &freq);

	return freq;
}

/* Divide the RGMII PLL down to the TX clock rate of the given link speed:
 * 125 MHz at 1000 Mbps, 25 MHz at 100 Mbps and 2.5 MHz at 10 Mbps.
 */
static inline int esp32_emac_rgmii_set_tx_clock(uint32_t pll_hz, uint32_t out_hz)
{
	uint32_t div = MAX(pll_hz / out_hz, 1U);
	uint32_t real_hz = pll_hz / div;
	uint32_t tolerance = (uint32_t)(((uint64_t)out_hz * ESP32_EMAC_RGMII_CLK_PPM) / 1000000U);

	if (div > 256U || abs((int32_t)real_hz - (int32_t)out_hz) > (int32_t)tolerance) {
		return -EINVAL;
	}

	emac_ll_ref_clock_div(NULL, (int)(div - 1U));

	return 0;
}

/* PLL frequency on the grid of the given clock that divides down to 125 MHz,
 * or 0 if there is none.
 */
static inline uint32_t esp32_emac_rgmii_pll_target(const emac_clk_info_t *info)
{
	if (info->step_hz <= 0) {
		return ESP32_EMAC_RGMII_CLK_HZ;
	}

	for (int16_t step = 1; step <= info->max_steps; step++) {
		uint32_t hz = (uint32_t)info->step_hz * (uint32_t)step;

		if ((hz % ESP32_EMAC_RGMII_CLK_HZ) == 0U) {
			return hz;
		}
	}

	return 0;
}

/* Clock the RGMII TX path from the first internal PLL listed by the HAL that
 * can be divided down to 125 MHz, and take the RX clock from the RX_CLK pad.
 * A PLL already locked at another frequency by a different user is skipped.
 */
static inline int esp32_emac_rgmii_clock_init(void)
{
	for (size_t i = 0; i < emac_clk_internal.clk_count; i++) {
		const emac_clk_info_t *info = emac_clk_internal.clk_src[i];
		uint32_t expt_hz = esp32_emac_rgmii_pll_target(info);
		uint32_t real_hz = 0;
		esp_err_t err;

		if (expt_hz == 0U || esp_clk_tree_enable_src(info->clk_id, true) != ESP_OK) {
			continue;
		}

		err = esp_clk_tree_src_set_freq_hz(info->clk_id, expt_hz, &real_hz);
		if ((err != ESP_OK && err != ESP_ERR_INVALID_STATE) ||
		    esp32_emac_rgmii_set_tx_clock(real_hz, ESP32_EMAC_RGMII_CLK_HZ) != 0) {
			esp_clk_tree_enable_src(info->clk_id, false);
			continue;
		}

		*esp32_emac_rgmii_clk() = info->clk_id;
		emac_ll_ref_clock_select(NULL, (int)i);
		emac_ll_ref_clock_enable(NULL, true);
		emac_ll_clock_enable_rgmii(NULL);

		return 0;
	}

	return -ENOTSUP;
}

static inline void esp32_emac_rgmii_clock_deinit(void)
{
	soc_module_clk_t *clk = esp32_emac_rgmii_clk();

	if (*clk != SOC_MOD_CLK_INVALID) {
		emac_ll_ref_clock_enable(NULL, false);
		esp_clk_tree_enable_src(*clk, false);
		*clk = SOC_MOD_CLK_INVALID;
	}
}

static inline int esp32_emac_rgmii_link_speed(enum phy_link_speed speed)
{
	uint32_t out_hz;

	if (PHY_LINK_IS_SPEED_1000M(speed)) {
		out_hz = ESP32_EMAC_RGMII_CLK_HZ;
	} else if (PHY_LINK_IS_SPEED_100M(speed)) {
		out_hz = MHZ(25);
	} else {
		out_hz = KHZ(2500);
	}

	return esp32_emac_rgmii_set_tx_clock(esp32_emac_rgmii_pll_hz(), out_hz);
}
#endif /* SOC_EMAC_SUPPORT_1000M */

#endif /* ZEPHYR_DRIVERS_ETHERNET_ETH_ESP32_PRIV_H_ */

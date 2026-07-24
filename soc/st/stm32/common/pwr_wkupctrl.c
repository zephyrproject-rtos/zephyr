/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_pwr_wkupctrl

#include <soc.h>
#include <stm32_bitops.h>
#include <stm32_gpio_shared.h>
#include <stm32_ll_pwr.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/dt-bindings/power/stm32_pwr.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/types.h>

/** Node identifier for the wake-up controller */
#define WKUP_CTLR DT_DRV_INST(0)

/** Maximum wake-up line index */
#define MAX_WKUP_LINE_IDX DT_PROP(WKUP_CTLR, st_max_wkup_line_idx)

/** Are wake-up lines wired to a source selection mux? */
#define HAS_MUXED_WKUP_LINES DT_PROP(WKUP_CTLR, st_has_multi_source_lines)

/** Can PWRC take over pull-up/pull-down management completely? */
#define HAS_PWRC_FULL_PUPD_CONTROL DT_PROP(WKUP_CTLR, st_has_pwr_full_pupd)

/**
 * Is arbitrary pull-up/pull-down on wake-up pins supported?
 *
 * Yes on all series except STM32F1-like series where pins are
 * forced in "input pull-down" mode, and STM32F7 series where
 * the internal PU/PD resistors cannot be enabled in Standby.
 */
#define HAS_WKUP_PINS_PUPD							\
	COND_CASE_1(								\
		DT_NODE_HAS_COMPAT(WKUP_CTLR, st_stm32f1_pwr_wkupctrl), (0),	\
		DT_NODE_HAS_COMPAT(WKUP_CTLR, st_stm32f7_pwr_wkupctrl), (0),	\
									(1))

/* --------------- */

/**
 * @brief Iterate over all GPIO pins in a property of a node's children.
 *
 * @param node_id Parent node identifier of the wake-up controller node
 * @param gpio_prop_name Name of the GPIO property
 * @param sep Separator to use between each iteration (between parentheses)
 * @param fn Function-like macro called for each pin that accepts four arguments:
 *	      fn(gpio_ctlr_node_id, gpio_pin, gpio_flags, child_node_reg_addr)
 *
 * @note Unlike the usual DT_FOREACH_*_SEP() macros, a separator is also added
 *       after the last iteration.
 */
#define FOR_EACH_CHILD_NODE_GPIO(node_id, gpio_prop_name, fn, sep)		\
	DT_FOREACH_CHILD_VARGS(node_id, FECNG_HELPER, gpio_prop_name, fn, sep)

#define FECNG_HELPER(pin_node_id, prop, fn, sep)				\
	IF_ENABLED(DT_NODE_HAS_PROP(pin_node_id, prop),				\
		(DT_FOREACH_PROP_ELEM_SEP_VARGS(pin_node_id, prop,		\
						FECNG_HELPER2, sep, fn)		\
						__DEBRACKET sep))

#define FECNG_HELPER2(pin_node_id, prop, idx, fn)				\
	fn(DT_GPIO_CTLR_BY_IDX(pin_node_id, prop, idx),				\
	   DT_GPIO_PIN_BY_IDX(pin_node_id, prop, idx),				\
	   DT_GPIO_FLAGS_BY_IDX(pin_node_id, prop, idx),			\
	   DT_REG_ADDR(pin_node_id))

/**
 * @brief Obtain the port index (STM32_PORTx) of a GPIO controller node.
 * @param node_id Node identifier of the GPIO controller node.
 * @return Port index
 */
#define GET_GPIO_PORT_BY_NODE(node_id)						\
	FOR_EACH_IDX_FIXED_ARG(GGPBN_HELPER, (), DT_DEP_ORD(node_id),		\
			       STM32_GPIO_PORTS_LIST_LWR)

#define GGPBN_HELPER(idx, __suffix, target_ord)					\
	IF_ENABLED(UTIL_AND(STM32_GPIO_PORT_DEVICE_IS_ACTIVE(__suffix),		\
		IS_EQ(DT_DEP_ORD(DT_NODELABEL(gpio##__suffix)), target_ord)),	\
		(CONCAT(STM32_PORT, GET_ARG_N(UTIL_INC(idx), STM32_GPIO_PORTS_LIST_UPR))))

/**
 * @brief Descriptor for one pin wired to a wake-up line.
 */
struct wkup_pin_desc {
	/** Pin port index (STM32_PORTx) */
	uint8_t port_idx;

	/** Pin number */
	gpio_pin_t pin_num;

	/** Wake-up line index */
	uint8_t line_idx;

	/**
	 * Wake-up line source selection for this pin.
	 * (May be unused if not applicable to target)
	 */
	uint8_t src_select;
};

/**
 * @brief Searches for the descriptor of a given wake-up pin.
 *
 * @param port_idx GPIO port index (STM32_PORTx)
 * @param pin GPIO pin number
 * @returns Pointer to the wake-up pin descriptor, or NULL if not found.
 */
static const struct wkup_pin_desc *search_pin_descriptor(uint32_t port_idx, gpio_pin_t pin_num)
{
#define WKUP_PIN_DESCRIPTOR_INIT(gpio_ctlr, _pin_num, flags, wkup_line_idx)	\
	{									\
		.port_idx = GET_GPIO_PORT_BY_NODE(gpio_ctlr),			\
		.pin_num = _pin_num,						\
		.line_idx = wkup_line_idx,					\
		.src_select = flags & STM32_PWR_WKUP_LINE_SRC_MASK,		\
	}

	static const struct wkup_pin_desc wkup_pins[] = {
	FOR_EACH_CHILD_NODE_GPIO(WKUP_CTLR, wkup_gpios, WKUP_PIN_DESCRIPTOR_INIT, (,))
	};

	for (int i = 0; i < ARRAY_SIZE(wkup_pins); i++) {
		const struct wkup_pin_desc *desc = &wkup_pins[i];

		if ((desc->port_idx == port_idx) && (desc->pin_num == pin_num)) {
			return desc;
		}
	}

	return NULL;
}

/* ------------------ */

static uint32_t wakeup_line_to_ll_val(uint8_t line_idx)
{
	/*
	 * Across all series, LL_PWR_WAKEUP_PIN<n> is defined as an alias
	 * for bit "WKUPEN<n>" or equivalent. WKUP1 seems "guaranteed" to
	 * exist but not others; depending on the series, they are either
	 * not defined because lower than the max, or because the series
	 * has "holes" and a specific WKUP<n> line is not implemented.
	 *
	 * Due to registers layout, LL_PWR_WAKEUP_PIN<n> is always equal
	 * to (LL_PWR_WAKEUP_PIN1 << (n-1)): compute the LL value using
	 * this trick, which avoids both the footprint overhead of a LUT
	 * and the #ifdef spaghetti needed to cope with impedance mismatch
	 * across all series, while remaining portable since we do use
	 * LL_PWR_WAKEUP_PIN1 as the base value.
	 *
	 * Note that we don't validate whether a given line index is valid
	 * for the target or not (i.e., whether the WKUPEN<n> bit exists),
	 * but the old implementation didn't either, and `line_idx` should
	 * always correspond to a valid line if DT is properly written.
	 */
	__ASSERT(line_idx > 0 && line_idx <= MAX_WKUP_LINE_IDX,
		"Invalid wake-up line index %d", line_idx);

#if defined(CONFIG_STM32_HAL2)
	return LL_PWR_WAKEUP_PIN_1 << (line_idx - 1U);
#else /* CONFIG_STM32_HAL2 */
	return LL_PWR_WAKEUP_PIN1 << (line_idx - 1U);
#endif /* CONFIG_STM32_HAL2 */
}

static void configure_wkup_pin_pupd(const struct wkup_pin_desc *pin_desc, uint32_t port_idx,
				    gpio_pin_t pin_num, gpio_flags_t pupd_flags)
{
#if !HAS_WKUP_PINS_PUPD
	/*
	 * No-op implementation for series without PU/PD control:
	 * - STM32F7 series where PU/PD cannot be enabled in Standby mode
	 * - STM32F0/F1/F2/F3/F4/L0/L1 series ("STM32F1-like") where wake-up
	 *   pins are forced by hardware in input pull-down mode while enabled
	 */
	ARG_UNUSED(pin_desc);
	ARG_UNUSED(port_idx);
	ARG_UNUSED(pin_num);
	ARG_UNUSED(pupd_flags);
#elif defined(CONFIG_SOC_SERIES_STM32WBAX)
	/*
	 * The STM32WBA series does not provide any direct way to control
	 * the pull-up/pull-down applied to wake-up pins in Standby mode:
	 * unlike the STM32U5-like series, which otherwise has an identical
	 * register layout, it doesn't have GPIO pull-up/pull-down control
	 * registers (PWR_PUCRx/PWR_PDCRx).
	 *
	 * However, we can indirectly enable a pull-up/pull-down in Standby
	 * mode by using the STM32WBA series' "I/O retention" feature which,
	 * when enabled for a pin, allows an internal PU/PD to be enabled by
	 * hardware upon entry in Standby mode. Refer to the Reference Manuals
	 * of STM32WBA products for more details about "I/O retention".
	 *
	 * Note: a few other series also have an "I/O retention" feature,
	 * but these series also have a dedicated register which configures
	 * the PU/PD for wake-up pins and has priority over I/O retention.
	 * To work properly, these series should NOT use this code but instead
	 * the common implementation found below which applies to all series
	 * with a dedicated PU/PD register for wake-up pins. The I/O retention
	 * feature described and handled here is exclusive to STM32WBA series,
	 * so this code will in fact not even build for these other series.
	 */
	ARG_UNUSED(pin_desc);
	(void)pupd_flags;

	volatile uint32_t *const ioretenr_x = (&PWR->IORETENRA) + 2 * port_idx;
	const uint32_t pin_bit = BIT(pin_num);

	stm32_reg_set_bits(ioretenr_x, pin_bit);
#elif HAS_PWRC_FULL_PUPD_CONTROL
	/*
	 * Series with full GPIO PU/PD control at PWR level:
	 * - One register pair for each GPIO port: PWR_PUCRx + PWR_PDCRx
	 *    - Enabled by bit APC in PWR_CR3 or PWR_APCR ("C0-like" / "U5-like")
	 *    - Note: configuration in this register may conflict with GPIOx_PUPDR!
	 * - Registers other than PUCRx/PDCRx are organized in two manners:
	 *    - "C0-like": STM32C0/G0/G4/L4/L5/U0/WB/WL5/...
	 *    - "U5-like": STM32U3/U5/...
	 *   (Refer to Reference Manuals for details about the other registers)
	 *
	 * Note that the PUCRx/PDCRx registers are allocated contiguously for A~Z
	 * regardless of which GPIO ports exist: the offset between PUCRA/PDCRA and
	 * PUCRx/PDCRx is constant and identical in all series. On products where
	 * certain GPIO ports don't exist, the corresponding PUCRx/PDCRx registers
	 * simply don't exist (leaving a "reserved" hole in the PWR register map).
	 */
	ARG_UNUSED(pin_desc);

	volatile uint32_t *const pucrx = &PWR->PUCRA + (port_idx * 2U);
	volatile uint32_t *const pdcrx = pucrx + 1;
	const uint32_t pin_bit = BIT(pin_num);

	if (pupd_flags == GPIO_PULL_UP) {
		stm32_reg_clear_bits(pdcrx, pin_bit);
		stm32_reg_set_bits(pucrx, pin_bit);
	} else if (pupd_flags == GPIO_PULL_DOWN) {
		stm32_reg_clear_bits(pucrx, pin_bit);
		stm32_reg_set_bits(pdcrx, pin_bit);
	} else {
		/* No pull-up nor pull-down requested */
		stm32_reg_clear_bits(pucrx, pin_bit);
		stm32_reg_clear_bits(pdcrx, pin_bit);
	}
#else
	/*
	 * Series with dedicated wake-up pins PU/PD control register:
	 * - "H5-like" series (STM32C5/H5/...): PWR_WUCR.WUPPUPDn
	 * - "H7-like" series (STM32H7/H7RS/N6/...): PWR_WKUPEPR.WKUPPUPDn
	 */
	ARG_UNUSED(port_idx);
	ARG_UNUSED(pin_num);
#if defined(PWR_WKUPEPR_WKUPPUPD1)
	volatile uint32_t *const reg = &PWR->WKUPEPR;
	const uint32_t base_shift = PWR_WKUPEPR_WKUPPUPD1_Pos;
#elif defined(PWR_WUCR_WUPPUPD1)
	volatile uint32_t *const reg = &PWR->WUCR;
	const uint32_t base_shift = PWR_WUCR_WUPPUPD1_Pos;
#else /* defined(PWR_WKUPEPR_WKUPPUPD1) */
#error "Unsupported series"
#endif /* defined(PWR_WKUPEPR_WKUPPUPD1) */

	/*
	 * "H5-like" and "H7-like" series work identically, with corresponding
	 * field being two bits wide and using the same encoding as GPIOx_PUPDR:
	 * 0b00 = none, 0b01 = pull-up and 0b10 = pull-down (0b11 is reserved)
	 */
	const uint32_t shift = base_shift + 2 * (pin_desc->line_idx - 1);
	const uint32_t mask = 0x3U;
	uint32_t pupdn;

	if (pupd_flags == GPIO_PULL_UP) {
		pupdn = 0x1U; /* 0b01 */
	} else if (pupd_flags == GPIO_PULL_DOWN) {
		pupdn = 0x2U; /* 0b10 */
	} else {
		pupdn = 0x0U;
	}

	stm32_reg_modify_bits(reg, mask << shift, pupdn << shift);
#endif /* !HAS_WKUP_PINS_PUPD */
}

/**
 * @brief Sets the signal source for a given wake-up line.
 * @param wkup_line_idx Wake-up line index (the `n` in `WKUPn`)
 * @param src_selection Source selection value
 *
 * @note No effect if the target SoC has no source selection mux.
 *
 * @internal
 * This function replaces the <tt>LL_PWR_SetWakeUpPinSignal<N>Selection()</tt>
 * family of functions. It is more efficient, easier to use and portable
 * across all applicable series because there is no naming disparity...
 * @endinternal
 */
static void configure_wkup_line_source(uint32_t wkup_line_idx, uint8_t src_selection)
{
#if HAS_MUXED_WKUP_LINES
	/*
	 * Replacement for the LL_PWR_SetWakeUpPinSignal<N>Selection() family
	 * of functions; they have a huge overhead as they re-compute the
	 * wake-up line index (which we already know!) and are a pain to use
	 * as they don't accept a "source selection" argument - instead, there
	 * is one function to select each possible source. Finally, the function
	 * naming is different on STM32U3 for whatever reason, so we'd ALSO need
	 * #ifdef quirks for that series... raw register access avoids all that.
	 *
	 * As of writing, a single implementation is sufficient as all series
	 * with multiple sources per wake-up line share the same register layout.
	 * This might need to be extended in the future.
	 */
	const uint32_t shift = (wkup_line_idx - 1) * 2U;

	stm32_reg_modify_bits(&PWR->WUCR3,
			      PWR_WUCR3_WUSEL1_Msk << shift,
			      src_selection << shift);
#endif /* HAS_MUXED_WKUP_LINES */
}

/* Private API entrypoint */
int stm32_gpiomgr_enable_wakeup_pin(uint32_t port_idx, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct wkup_pin_desc *pin_desc = search_pin_descriptor(port_idx, pin);

	if (pin_desc == NULL) {
		return -ENODEV;
	}

	const uint32_t ll_wakeup_line = wakeup_line_to_ll_val(pin_desc->line_idx);

#if !DT_NODE_HAS_COMPAT(WKUP_CTLR, st_stm32f1_pwr_wkupctrl)
	/* Polarity is configurable except on F1-like series */

	if (flags & GPIO_ACTIVE_LOW) {
		/* Falling-edge / low-level sensitivity */
		LL_PWR_SetWakeUpPinPolarityLow(ll_wakeup_line);
	} else {
		/* Rising-edge / high-level sensitivity */
		LL_PWR_SetWakeUpPinPolarityHigh(ll_wakeup_line);
	}
#endif /* !DT_NODE_HAS_COMPAT(WKUP_CTLR, st_stm32f1_pwr_wkupctrl) */

	configure_wkup_pin_pupd(pin_desc, port_idx, pin, flags & (GPIO_PULL_UP | GPIO_PULL_DOWN));
	configure_wkup_line_source(pin_desc->line_idx, pin_desc->src_select);

	LL_PWR_EnableWakeUpPin(ll_wakeup_line);

	return 0;
}

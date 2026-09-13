/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_IMX_CCM_REV3_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_IMX_CCM_REV3_H_

/**
 * @file
 * @brief SoC-agnostic clock-specifier encoding for the NXP i.MX CCM rev3.
 *
 * The i.MX CCM as used by the recent i.MX RT parts models a peripheral's clock
 * as two independent things:
 *
 *  - a **gate** (LPCG), which turns the peripheral's bus and functional clocks
 *    on and off, and
 *  - a **clock root**, whose mux and divider settings determine the frequency
 *    the peripheral actually receives.
 *
 * There is no algorithmic relation between the two identifier spaces, so both
 * have to reach the clock driver. The shared NXP peripheral drivers read a
 * single clock cell -- `DT_INST_CLOCKS_CELL(n, name)` -- and pass that one
 * value to `clock_control_on()`, `clock_control_get_rate()`, and
 * `clock_control_configure()` alike, so a second devicetree cell would be
 * invisible to them. Both identifiers are therefore packed into the one cell:
 *
 * @code{.dts}
 *     clocks = <&ccm IMX_CCM_CLK(<gate-id>, <root-id>)>;
 * @endcode
 *
 * Packing two identifiers into one cell is how `nxp,imx-ccm-rev2` already
 * encodes its peripheral and instance numbers.
 *
 * Use @ref IMX_CCM_GATE_NONE for a peripheral that has no gate of its own, and
 * @ref IMX_CCM_ROOT_NONE for one with no dedicated clock root -- asking such a
 * peripheral for its rate yields `-ENOTSUP` rather than a fabricated number.
 *
 * The identifier values themselves are SoC-specific and generated into the
 * hal_nxp module beside the fsl_clock.h they are derived from, for example
 * <nxp/imxrt/imxrt266x/clock/imx_ccm_rev3_rt266x.h>.
 */

/** Width of each identifier field, in bits. */
#define IMX_CCM_ID_BITS 16U
/** Mask of a single identifier field. */
#define IMX_CCM_ID_MASK 0xFFFFU

/** Bit position of the gate identifier within a clock specifier. */
#define IMX_CCM_GATE_SHIFT 0U
/** Bit position of the clock-root identifier within a clock specifier. */
#define IMX_CCM_ROOT_SHIFT 16U

/** This peripheral has no clock gate of its own. */
#define IMX_CCM_GATE_NONE IMX_CCM_ID_MASK
/** This peripheral has no dedicated clock root, so its rate is unknown. */
#define IMX_CCM_ROOT_NONE IMX_CCM_ID_MASK

/**
 * @brief Compose a peripheral clock specifier from a gate and a clock root.
 *
 * @param gate Gate (LPCG) identifier, or @ref IMX_CCM_GATE_NONE.
 * @param root Clock-root identifier, or @ref IMX_CCM_ROOT_NONE.
 */
#define IMX_CCM_CLK(gate, root)                                                                    \
	((((root) & IMX_CCM_ID_MASK) << IMX_CCM_ROOT_SHIFT) |                                      \
	 (((gate) & IMX_CCM_ID_MASK) << IMX_CCM_GATE_SHIFT))

/** @brief Extract the gate identifier from a clock specifier. */
#define IMX_CCM_CLK_GATE(spec) (((spec) >> IMX_CCM_GATE_SHIFT) & IMX_CCM_ID_MASK)
/** @brief Extract the clock-root identifier from a clock specifier. */
#define IMX_CCM_CLK_ROOT(spec) (((spec) >> IMX_CCM_ROOT_SHIFT) & IMX_CCM_ID_MASK)

/**
 * @name Per-device clock-root configuration cell
 *
 * A peripheral that wants to own its clock root's mux/divider settings -- rather
 * than inherit a fixed root programmed by SoC clock init -- adds a SECOND entry
 * to its `clocks` property, named "source" in `clock-names`, following the
 * established Zephyr clock-binding idiom of a second "source-select" `clocks`
 * entry beyond the gate (`clocks = <&ctlr GATE>, <&ctlr SRC SEL>;`). That entry
 * is a single packed @ref IMX_CCM_ROOT_CFG cell. The IP driver reads it and passes it
 * verbatim, by value, as the `clock_control_configure()` subsystem argument
 * (`data` NULL); the rev3 controller unpacks it and issues one
 * `CLOCK_SetRootClock`.
 *
 * The cell must be distinguishable from a legacy @ref IMX_CCM_CLK gate cell,
 * because the shared drivers also pass the gate cell to
 * `clock_control_configure()` (the pre-existing tolerant call). A gate cell fills
 * all 32 bits with two 16-bit ids; its top nibble is the clock-root id's high
 * nibble, which is 0 for every real root (max id 217) and 0xF only for
 * @ref IMX_CCM_ROOT_NONE. A root-config cell uses the low 28 bits for its fields
 * and stamps a fixed 0xA tag into the top nibble -- a value a gate cell can never
 * produce -- so @ref IMX_CCM_ROOT_CFG_IS() tells the two kinds apart by
 * construction.
 *
 * Field widths: root 8 bits (max id 217), mux 4 bits (max 3), div 8 bits,
 * snd_div 8 bits = 28 bits, verified sufficient for all RT2660 peripheral roots.
 * A divider value of 0 is illegal: the HAL programs `value - 1`, so 0 would reach
 * the register as an all-ones field (slowest divide). Divide-by-one is 1.
 * @{
 */

/** Bit position of the root id within a root-config cell. */
#define IMX_CCM_ROOT_CFG_ROOT_SHIFT    0U
/** Bit position of the mux selector within a root-config cell. */
#define IMX_CCM_ROOT_CFG_MUX_SHIFT     8U
/** Bit position of the primary divider within a root-config cell. */
#define IMX_CCM_ROOT_CFG_DIV_SHIFT     12U
/** Bit position of the secondary divider within a root-config cell. */
#define IMX_CCM_ROOT_CFG_SND_DIV_SHIFT 20U
/** Bit position of the discriminator tag within a root-config cell. */
#define IMX_CCM_ROOT_CFG_TAG_SHIFT     28U

/** Field masks (before shifting). */
#define IMX_CCM_ROOT_CFG_ROOT_MASK    0xFFU
#define IMX_CCM_ROOT_CFG_MUX_MASK     0xFU
#define IMX_CCM_ROOT_CFG_DIV_MASK     0xFFU
#define IMX_CCM_ROOT_CFG_SND_DIV_MASK 0xFFU
#define IMX_CCM_ROOT_CFG_TAG_MASK     0xFU

/** Tag value stamped into the top nibble to mark a root-config cell. */
#define IMX_CCM_ROOT_CFG_TAG 0xAU

/**
 * @brief Compose a per-device clock-root configuration specifier.
 *
 * @param root    Clock-root identifier whose mux/dividers to program.
 * @param mux     Root mux selector (which already-running upstream source).
 * @param div     Primary root divider (1 = divide-by-one; 0 is illegal).
 * @param snd_div Secondary root divider (1 = divide-by-one; 0 is illegal).
 */
#define IMX_CCM_ROOT_CFG(root, mux, div, snd_div)                                                  \
	(((IMX_CCM_ROOT_CFG_TAG) << IMX_CCM_ROOT_CFG_TAG_SHIFT) |                                  \
	 (((snd_div) & IMX_CCM_ROOT_CFG_SND_DIV_MASK) << IMX_CCM_ROOT_CFG_SND_DIV_SHIFT) |         \
	 (((div) & IMX_CCM_ROOT_CFG_DIV_MASK) << IMX_CCM_ROOT_CFG_DIV_SHIFT) |                     \
	 (((mux) & IMX_CCM_ROOT_CFG_MUX_MASK) << IMX_CCM_ROOT_CFG_MUX_SHIFT) |                     \
	 (((root) & IMX_CCM_ROOT_CFG_ROOT_MASK) << IMX_CCM_ROOT_CFG_ROOT_SHIFT))

/** @brief True if @p cell is a root-config cell (not a legacy gate cell). */
#define IMX_CCM_ROOT_CFG_IS(cell)                                                                  \
	((((cell) >> IMX_CCM_ROOT_CFG_TAG_SHIFT) & IMX_CCM_ROOT_CFG_TAG_MASK) ==                   \
	 IMX_CCM_ROOT_CFG_TAG)

/** @brief Extract the root id from a root-config cell. */
#define IMX_CCM_ROOT_CFG_ROOT(cell)                                                                \
	(((cell) >> IMX_CCM_ROOT_CFG_ROOT_SHIFT) & IMX_CCM_ROOT_CFG_ROOT_MASK)
/** @brief Extract the mux selector from a root-config cell. */
#define IMX_CCM_ROOT_CFG_MUX(cell)                                                                 \
	(((cell) >> IMX_CCM_ROOT_CFG_MUX_SHIFT) & IMX_CCM_ROOT_CFG_MUX_MASK)
/** @brief Extract the primary divider from a root-config cell. */
#define IMX_CCM_ROOT_CFG_DIV(cell)                                                                 \
	(((cell) >> IMX_CCM_ROOT_CFG_DIV_SHIFT) & IMX_CCM_ROOT_CFG_DIV_MASK)
/** @brief Extract the secondary divider from a root-config cell. */
#define IMX_CCM_ROOT_CFG_SND_DIV(cell)                                                             \
	(((cell) >> IMX_CCM_ROOT_CFG_SND_DIV_SHIFT) & IMX_CCM_ROOT_CFG_SND_DIV_MASK)

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_IMX_CCM_REV3_H_ */

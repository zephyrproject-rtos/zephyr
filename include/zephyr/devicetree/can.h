/**
 * @file
 * @brief CAN devicetree macro public API header file.
 */

/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_CAN_H_
#define ZEPHYR_INCLUDE_DEVICETREE_CAN_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup devicetree-can Devicetree CAN API
 * @ingroup devicetree
 * @ingroup can_interface
 * @{
 */

/**
 * @brief Get the minimum transceiver bitrate for a CAN controller
 *
 * The bitrate will be limited to the minimum bitrate supported by the CAN
 * controller. If no CAN transceiver is present in the devicetree, the minimum
 * bitrate will be that of the CAN controller.
 *
 * Example devicetree fragment:
 *
 *     transceiver0: can-phy0 {
 *             compatible = "vnd,can-transceiver";
 *             min-bitrate = <15000>;
 *             max-bitrate = <1000000>;
 *             #phy-cells = <0>;
 *     };
 *
 *     can0: can@... {
 *             compatible = "vnd,can-controller";
 *             phys = <&transceiver0>;
 *     };
 *
 *     can1: can@... {
 *             compatible = "vnd,can-controller";
 *
 *             can-transceiver {
 *                     min-bitrate = <25000>;
 *                     max-bitrate = <2000000>;
 *             };
 *     };
 *
 *     can2: can@... {
 *             compatible = "vnd,can-controller";
 *
 *             can-transceiver {
 *                     max-bitrate = <2000000>;
 *             };
 *     };
 *
 * Example usage:
 *
 *     DT_CAN_TRANSCEIVER_MIN_BITRATE(DT_NODELABEL(can0), 10000) // 15000
 *     DT_CAN_TRANSCEIVER_MIN_BITRATE(DT_NODELABEL(can1), 0)     // 250000
 *     DT_CAN_TRANSCEIVER_MIN_BITRATE(DT_NODELABEL(can1), 50000) // 500000
 *     DT_CAN_TRANSCEIVER_MIN_BITRATE(DT_NODELABEL(can2), 0)     // 0
 *
 * @param node_id node identifier
 * @param min minimum bitrate supported by the CAN controller
 * @return the minimum bitrate supported by the CAN controller/transceiver combination
 */
#define DT_CAN_TRANSCEIVER_MIN_BITRATE(node_id, min)					\
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, phys),					\
		    MAX(DT_PROP_OR(DT_PHANDLE(node_id, phys), min_bitrate, 0), min),	\
		    MAX(DT_PROP_OR(DT_CHILD(node_id, can_transceiver), min_bitrate, min), min))

/**
 * @brief Get the maximum transceiver bitrate for a CAN controller
 *
 * The bitrate will be limited to the maximum bitrate supported by the CAN
 * controller. If no CAN transceiver is present in the devicetree, the maximum
 * bitrate will be that of the CAN controller.
 *
 * Example devicetree fragment:
 *
 *     transceiver0: can-phy0 {
 *             compatible = "vnd,can-transceiver";
 *             max-bitrate = <1000000>;
 *             #phy-cells = <0>;
 *     };
 *
 *     can0: can@... {
 *             compatible = "vnd,can-controller";
 *             phys = <&transceiver0>;
 *     };
 *
 *     can1: can@... {
 *             compatible = "vnd,can-controller";
 *
 *             can-transceiver {
 *                     max-bitrate = <2000000>;
 *             };
 *     };
 *
 * Example usage:
 *
 *     DT_CAN_TRANSCEIVER_MAX_BITRATE(DT_NODELABEL(can0), 5000000) // 1000000
 *     DT_CAN_TRANSCEIVER_MAX_BITRATE(DT_NODELABEL(can1), 5000000) // 2000000
 *     DT_CAN_TRANSCEIVER_MAX_BITRATE(DT_NODELABEL(can1), 1000000) // 1000000
 *
 * @param node_id node identifier
 * @param max maximum bitrate supported by the CAN controller
 * @return the maximum bitrate supported by the CAN controller/transceiver combination
 */
#define DT_CAN_TRANSCEIVER_MAX_BITRATE(node_id, max)					\
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, phys),					\
		    MIN(DT_PROP(DT_PHANDLE(node_id, phys), max_bitrate), max),		\
		    MIN(DT_PROP_OR(DT_CHILD(node_id, can_transceiver), max_bitrate, max), max))

/**
 * @brief Get the minimum transceiver bitrate for a DT_DRV_COMPAT CAN controller
 * @param inst DT_DRV_COMPAT instance number
 * @param min minimum bitrate supported by the CAN controller
 * @return the minimum bitrate supported by the CAN controller/transceiver combination
 * @see DT_CAN_TRANSCEIVER_MIN_BITRATE()
 */
#define DT_INST_CAN_TRANSCEIVER_MIN_BITRATE(inst, min) \
	DT_CAN_TRANSCEIVER_MIN_BITRATE(DT_DRV_INST(inst), min)

/**
 * @brief Get the maximum transceiver bitrate for a DT_DRV_COMPAT CAN controller
 * @param inst DT_DRV_COMPAT instance number
 * @param max maximum bitrate supported by the CAN controller
 * @return the maximum bitrate supported by the CAN controller/transceiver combination
 * @see DT_CAN_TRANSCEIVER_MAX_BITRATE()
 */
#define DT_INST_CAN_TRANSCEIVER_MAX_BITRATE(inst, max) \
	DT_CAN_TRANSCEIVER_MAX_BITRATE(DT_DRV_INST(inst), max)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DEVICETREE_CAN_H_ */

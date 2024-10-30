/**
 * @file
 * @brief Port / Endpoint Devicetree macro public API header file.
 */

/*
 * Copyright 2024 NXP
 * Copyright (c) 2024 tinyVision.ai Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_PORT_ENDPOINT_H_
#define ZEPHYR_INCLUDE_DEVICETREE_PORT_ENDPOINT_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup devicetree-port-endpoint Devicetree Port Endpoint API
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Helper for @ref DT_INST_PORT_BY_ID
 *
 * This behaves the same way as @ref DT_INST_PORT_BY_ID but does not work if there is only
 * a single port without address.
 *
 * @param inst instance number
 * @param pid port ID
 * @return port node associated with @p pid
 */
#define _DT_INST_PORT_BY_ID(inst, pid)                                                             \
	COND_CODE_1(DT_NODE_EXISTS(DT_INST_CHILD(inst, ports)),                                    \
	    (DT_CHILD(DT_INST_CHILD(inst, ports), port_##pid)), (DT_INST_CHILD(inst, port_##pid)))

/**
 * @brief Get a port node from its id
 *
 * Given a device instance number, return a port node specified by its ID.
 * It handles various ways of how a port could be defined.
 *
 * Example usage with DT_INST_PORT_BY_ID() to get the @c port@0 or @c port node:
 *
 * @code{.c}
 *	DT_INST_PORT_BY_ID(inst, 0)
 * @endcode
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *	&device {
 *		ports {
 *			#address-cells = <1>;
 *			#size-cells = <0>;
 *			port@0 {
 *				reg = <0x0>;
 *			};
 *		};
 *	};
 * @endcode
 *
 * @code{.dts}
 *	&device {
 *		#address-cells = <1>;
 *		#size-cells = <0>;
 *		port@0 {
 *			reg = <0x0>;
 *		};
 *	};
 * @endcode
 *
 * @code{.dts}
 *	&device {
 *		port {
 *		};
 *	};
 * @endcode
 *
 * @param inst instance number
 * @param pid port ID
 * @return port node associated with @p pid
 */
#define DT_INST_PORT_BY_ID(inst, pid)                                                              \
	COND_CODE_1(DT_NODE_EXISTS(_DT_INST_PORT_BY_ID(inst, pid)),                                \
		(_DT_INST_PORT_BY_ID(inst, pid)), (DT_INST_CHILD(inst, port)))

/**
 * @brief Helper for @ref DT_INST_ENDPOINT_BY_ID
 *
 * This behaves the same way as @ref DT_INST_PORT_BY_ID but does not work if there is only
 * a single endpoint without address.
 *
 * @param inst instance number
 * @param pid port ID
 * @param eid endpoint ID
 * @return endpoint node associated with @p eid and @p pid
 */
#define _DT_INST_ENDPOINT_BY_ID(inst, pid, eid)                                                    \
	DT_CHILD(DT_INST_PORT_BY_ID(inst, pid), endpoint_##eid)

/**
 * @brief Get an endpoint node from its id and its parent port id
 *
 * Given a device instance number, a port ID and an endpoint ID, return the endpoint node.
 * It handles various ways of how a port and an endpoint could be defined as described in
 * @ref DT_INST_PORT_BY_ID and below.
 *
 * Example usage with DT_INST_ENDPOINT_BY_ID() to get the @c endpoint or @c endpoint@0 node:
 *
 * @code{.c}
 *	DT_INST_ENDPOINT_BY_ID(inst, 0, 0)
 * @endcode
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *	&device {
 *		port {
 *			endpoint {
 *			};
 *		};
 *	};
 * @endcode
 *
 * @code{.dts}
 *	&device {
 *		port {
 *			#address-cells = <1>;
 *			#size-cells = <0>;
 *			endpoint@0 {
 *				reg = <0x0>;
 *			};
 *		};
 *	};
 * @endcode
 *
 * @code{.dts}
 *	&device {
 *		ports {
 *			#address-cells = <1>;
 *			#size-cells = <0>;
 *			port@0 {
 *				reg = <0x0>;
 *				#address-cells = <1>;
 *				#size-cells = <0>;
 *				endpoint@0 {
 *					reg = <0x0>;
 *				};
 *			};
 *		};
 *	};
 * @endcode
 *
 * @param inst instance number
 * @param pid port ID
 * @param eid endpoint ID
 * @return endpoint node associated with @p eid and @p pid
 */
#define DT_INST_ENDPOINT_BY_ID(inst, pid, eid)                                                     \
	COND_CODE_1(DT_NODE_EXISTS(_DT_INST_ENDPOINT_BY_ID(inst, pid, eid)),                       \
		(_DT_INST_ENDPOINT_BY_ID(inst, pid, eid)),                                         \
			(DT_CHILD(DT_INST_PORT_BY_ID(inst, pid), endpoint)))

/**
 * @brief Get the device node from its endpoint node.
 *
 * Given an endpoint node id, return its device node id.
 * This handles various ways of how a port and an endpoint could be defined as described in
 * @ref DT_NODE_BY_ENDPOINT.
 *
 * Example usage with DT_NODE_BY_ENDPOINT() to get the @c &device node from its @c ep0 node:
 *
 * @code{.c}
 *	DT_NODE_BY_ENDPOINT(DT_NODELABEL(ep0))
 * @endcode
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *	&device {
 *		port {
 *			#address-cells = <1>;
 *			#size-cells = <0>;
 *			ep0: endpoint@0 {
 *				reg = <0x0>;
 *			};
 *		};
 *	};
 * @endcode
 *
 * @code{.dts}
 *	&device {
 *		ports {
 *			#address-cells = <1>;
 *			#size-cells = <0>;
 *			port@0 {
 *				reg = <0x0>;
 *				#address-cells = <1>;
 *				#size-cells = <0>;
 *				ep0: endpoint@0 {
 *					reg = <0x0>;
 *				};
 *			};
 *		};
 *	};
 * @endcode
 *
 * @param ep endpoint node
 * @return device node associated with @p ep
 */
#define DT_NODE_BY_ENDPOINT(ep)                                                                    \
	COND_CODE_1(DT_NODE_EXISTS(DT_CHILD(DT_PARENT(DT_GPARENT(ep)), ports)),                    \
		(DT_PARENT(DT_GPARENT(ep))), (DT_GPARENT(ep)))

/**
 * @brief Get the remote device node from a local endpoint node.
 *
 * Given an endpoint node id, return the remote device node that connects to this device via this
 * local endpoint. This handles various ways of how a port and an endpoint could be defined as
 * described in @ref DT_INST_PORT_BY_ID and @ref DT_INST_ENDPOINT_BY_ID.
 *
 * Example usage with DT_NODE_REMOTE_DEVICE() to get the remote device node @c &device1 from the
 * local endpoint @c endpoint@0 node of the device @c &device0 node:
 *
 * @code{.c}
 *	DT_NODE_REMOTE_DEVICE(DT_NODELABEL(device0_ep_out))
 * @endcode
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *	&device0 {
 *		port {
 *			#address-cells = <1>;
 *			#size-cells = <0>;
 *			device0_ep_out: endpoint@0 {
 *				reg = <0x0>;
 *				remote-endpoint-label = "device1_ep_in";
 *			};
 *		};
 *	};
 *
 *	&device1 {
 *		ports {
 *			#address-cells = <1>;
 *			#size-cells = <0>;
 *			port@0 {
 *				reg = <0x0>;
 *				device1_ep_in: endpoint {
 *					remote-endpoint-label = "device0_ep_out";
 *				};
 *			};
 *		};
 *	};
 * @endcode
 *
 * @param ep endpoint node
 * @return remote device node that connects to this device via @p ep
 */
#define DT_NODE_REMOTE_DEVICE(ep)                                                                \
	DT_NODE_BY_ENDPOINT(DT_NODELABEL(DT_STRING_TOKEN(ep, remote_endpoint_label)))

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DEVICETREE_PORT_ENDPOINT_H_ */

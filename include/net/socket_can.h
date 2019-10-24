/** @file
 * @brief Socket CAN definitions.
 *
 * Definitions for socket CAN support.
 */

/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_SOCKET_CAN_H_
#define ZEPHYR_INCLUDE_NET_SOCKET_CAN_H_

#include <zephyr/types.h>
#include <net/net_ip.h>
#include <net/net_if.h>
#include <drivers/can.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Socket CAN library
 * @defgroup socket_can Network Core Library
 * @ingroup networking
 * @{
 */

/* Protocols of the protocol family PF_CAN */
#define CAN_RAW 1

/* Socket CAN options */
#define SOL_CAN_BASE 100
#define SOL_CAN_RAW (SOL_CAN_BASE + CAN_RAW)

enum {
	CAN_RAW_FILTER = 1,
	CAN_RAW_ERR_FILTER,
};

/* Socket CAN MTU size */
#define CAN_MTU		CAN_MAX_DLEN

/* special address description flags for the CAN_ID */
#define CAN_EFF_FLAG 0x80000000U /* EFF/SFF is set in the MSB */
#define CAN_RTR_FLAG 0x40000000U /* remote transmission request */
#define CAN_ERR_FLAG 0x20000000U /* error message frame */

/* valid bits in CAN ID for frame formats */
#define CAN_SFF_MASK 0x000007FFU /* standard frame format (SFF) */
#define CAN_EFF_MASK 0x1FFFFFFFU /* extended frame format (EFF) */
#define CAN_ERR_MASK 0x1FFFFFFFU /* omit EFF, RTR, ERR flags */

#define CAN_ERR_DLC 8 /* dlc for error message frames */

/** error class (mask) in can_id */

/** TX timeout (by netdevice driver) */
#define CAN_ERR_TX_TIMEOUT   (0x00000001U)
/** lost arbitration - data[0] */
#define CAN_ERR_LOSTARB      (0x00000002U)
/** controller problems - data[1] */
#define CAN_ERR_CRTL         (0x00000004U)
/** protocol violations - data[2..3] */
#define CAN_ERR_PROT         (0x00000008U)
/** transceiver status -data[4] */
#define CAN_ERR_TRX          (0x00000010U)
/** received no ACK on transmission */
#define CAN_ERR_ACK          (0x00000020U)
/** bus off */
#define CAN_ERR_BUSOFF       (0x00000040U)
/** bus error (may flood!) */
#define CAN_ERR_BUSERROR     (0x00000080U)
/** controller restarted */
#define CAN_ERR_RESTARTED    (0x00000100U)

/** error status of can controller data[1] */

/** unspecified */
#define CAN_ERR_CRTL_UNSPEC      (0 << 0)
/** RX buffer overflow */
#define CAN_ERR_CRTL_RX_OVERFLOW (1 << 0)
/** TX buffer overflow */
#define CAN_ERR_CRTL_TX_OVERFLOW (1 << 2)
/** reached warning level for RX errors */
#define CAN_ERR_CRTL_RX_WARNING  (1 << 3)
/** reached warning level for TX errors */
#define CAN_ERR_CRTL_TX_WARNING  (1 << 4)
/** reached error passive status RX */
#define CAN_ERR_CRTL_RX_PASSIVE  (1 << 5)
/** reached error passive status TX */
#define CAN_ERR_CRTL_TX_PASSIVE  (1 << 6)
/** recovered to error active state */
#define CAN_ERR_CRTL_ACTIVE      (1 << 7)

/**
 * struct sockaddr_can - The sockaddr structure for CAN sockets
 *
 */
struct sockaddr_can {
	sa_family_t can_family;
	int         can_ifindex;
};

/**
 * CAN L2 driver API. Used by socket CAN.
 */
struct canbus_api {
	/**
	 * The net_if_api must be placed in first position in this
	 * struct so that we are compatible with network interface API.
	 */
	struct net_if_api iface_api;

	/** Send a CAN packet by socket */
	int (*send)(struct device *dev, struct net_pkt *pkt);

	/** Close the related CAN socket */
	void (*close)(struct device *dev, int filter_id);

	/** Set socket CAN option */
	int (*setsockopt)(struct device *dev, void *obj, int level, int optname,
			  const void *optval, socklen_t optlen);

	/** Get socket CAN option */
	int (*getsockopt)(struct device *dev, void *obj, int level, int optname,
			  const void *optval, socklen_t *optlen);
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_SOCKET_CAN_H_ */

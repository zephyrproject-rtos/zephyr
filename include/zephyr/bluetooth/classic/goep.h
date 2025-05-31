/* goep.h - Bluetooth Generic Object Exchange Profile handling */

/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_GOEP_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_GOEP_H_

/**
 * @brief Bluetooth APIs
 * @defgroup bluetooth Bluetooth APIs
 * @ingroup connectivity
 * @{
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/bluetooth/classic/obex.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Generic Object Exchange Profile (GOEP)
 * @defgroup bt_goep Generic Object Exchange Profile (GOEP)
 * @since 1.0
 * @version 1.0.0
 * @ingroup bluetooth
 * @{
 */

struct bt_goep;

/** @brief GOEP transport operations structure.
 *
 * The object has to stay valid and constant for the lifetime of the GOEP server and client.
 */
struct bt_goep_transport_ops {
	/** @brief GOEP transport connected callback
	 *
	 *  If this callback is provided it will be called whenever the GOEP transport connection
	 *  completes.
	 *
	 *  @param conn The ACL connection.
	 *  @param goep The GOEP object that has been connected.
	 */
	void (*connected)(struct bt_conn *conn, struct bt_goep *goep);

	/** @brief GOEP transport disconnected callback
	 *
	 *  If this callback is provided it will be called whenever the GOEP transport is
	 *  disconnected, including when a connection gets rejected.
	 *
	 *  @param goep The GOEP object that has been Disconnected.
	 */
	void (*disconnected)(struct bt_goep *goep);
};

/** @brief Life-span states of GOEP transport.
 *
 *  Used only by internal APIs dealing with setting GOEP to proper transport state depending on
 *  operational context.
 *
 *  GOEP transport enters the @ref BT_GOEP_TRANSPORT_CONNECTING state upon
 *  @ref bt_goep_transport_l2cap_connect, @ref bt_goep_transport_rfcomm_connect or upon returning
 *  from @ref bt_goep_transport_rfcomm_server::accept and bt_goep_transport_l2cap_server::accept.
 *
 *  When GOEP transport leaves the @ref BT_GOEP_TRANSPORT_CONNECTING state and enters the @ref
 *  BT_GOEP_TRANSPORT_CONNECTED, @ref bt_goep_transport_ops::connected is called.
 *
 *  When GOEP transport enters the @ref BT_GOEP_TRANSPORT_DISCONNECTED from other states,
 *  @ref bt_goep_transport_ops::disconnected is called.
 */
enum __packed bt_goep_transport_state {
	/** GOEP disconnected */
	BT_GOEP_TRANSPORT_DISCONNECTED,
	/** GOEP in connecting state */
	BT_GOEP_TRANSPORT_CONNECTING,
	/** GOEP ready for upper layer traffic on it */
	BT_GOEP_TRANSPORT_CONNECTED,
	/** GOEP in disconnecting state */
	BT_GOEP_TRANSPORT_DISCONNECTING,
};

struct bt_goep {
	/** @internal To be used for transport */
	union {
		struct bt_rfcomm_dlc dlc;
		struct bt_l2cap_br_chan chan;
	} _transport;

	/** @internal Peer GOEP Version
	 *
	 *  false - Peer supports GOEP v1.1. The GOEP transport is based on RFCOMM.
	 *  `dlc` is used as transport.
	 *
	 *  true - peer supports GOEP v2.0 or later. The GOEP transport is based on L2CAP.
	 *  `chan` is used as transport.
	 */
	bool _goep_v2;

	/** @internal connection handle */
	struct bt_conn *_acl;

	/** @internal Saves the current transport state, @ref bt_goep_transport_state */
	atomic_t _state;

	/** @brief GOEP transport operations
	 *
	 *  The upper layer should pass the operations to `transport_ops` when
	 *  providing the GOEP structure.
	 */
	const struct bt_goep_transport_ops *transport_ops;

	/** @brief OBEX object */
	struct bt_obex obex;
};

/**
 * @defgroup bt_goep_transport_rfcomm GOEP transport RFCOMM
 * @ingroup bt_goep
 * @{
 */

/** @brief GOEP Server structure GOEP v1.1. */
struct bt_goep_transport_rfcomm_server {
	/** @brief RFCOMM server for GOEP v1.1
	 *
	 *  The @ref bt_goep_transport_rfcomm_server::rfcomm is used to register a rfcomm server.
	 *
	 *  The @ref bt_rfcomm_server::channel needs to be passed with a pre-set channel (not
	 *  recommended however), Or give a value `0` to make the channel be auto-allocated when
	 *  @ref bt_goep_transport_rfcomm_server_register is called.
	 *  The @ref bt_rfcomm_server::accept should be not used by GOEP application, and instead
	 *  the @ref bt_goep_transport_rfcomm_server::accept should be used.
	 */
	struct bt_rfcomm_server rfcomm;

	/** @brief Server accept callback
	 *
	 *  This callback is called whenever a new incoming GOEP connection requires
	 *  authorization.
	 *
	 *  @warning It is the responsibility of the caller to zero out the parent of the GOEP
	 *  object.
	 *
	 *  @param conn The connection that is requesting authorization.
	 *  @param server Pointer to the server structure this callback relates to.
	 *  @param goep Pointer to received the allocated GOEP object.
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 *  @return -ENOMEM if no available space for new object.
	 *  @return -EACCES if application did not authorize the connection.
	 *  @return -EPERM if encryption key size is too short.
	 */
	int (*accept)(struct bt_conn *conn, struct bt_goep_transport_rfcomm_server *server,
		      struct bt_goep **goep);

	sys_snode_t node;
};

/** @brief Register GOEP RFCOMM server.
 *
 *  Register GOEP server for a RFCOMM channel @ref bt_rfcomm_server::channel, each new connection
 *  is authorized using the @ref bt_goep_transport_rfcomm_server::accept callback which in case of
 *  success shall allocate the GOEP structure @ref bt_goep to be used by the new GOEP connection.
 *
 *  @ref bt_rfcomm_server::channel may be pre-set to a given value (not recommended however). Or be
 *  left as 0, in which case the channel will be auto-allocated by RFCOMM.
 *
 *  @param server Server structure.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_goep_transport_rfcomm_server_register(struct bt_goep_transport_rfcomm_server *server);

/** @brief Connect GOEP transport over RFCOMM
 *
 *  Connect GOEP transport over RFCOMM, once the connection is completed, the callback
 *  @ref bt_goep_transport_ops::connected is called. If the connection is rejected,
 *  @ref bt_goep_transport_ops::disconnected callback is called instead.
 *  The GOEP object is passed (over an address of it) as second parameter, application should
 *  create transport dedicated GOEP object @ref bt_goep. Then pass to this API the location
 *  (address).
 *  Before calling the API, @ref bt_obex::client_ops and @ref bt_goep::transport_ops should
 *  be initialized with valid address of type @ref bt_obex_client_ops object and
 *  @ref bt_goep_transport_ops object. The field `mtu` of @ref bt_obex::rx could be passed with
 *  valid value. Or set it to zero, the mtu will be calculated according to
 *  @kconfig{CONFIG_BT_GOEP_RFCOMM_MTU}.
 *  The RFCOMM channel is passed as third parameter. It is the RFCOMM channel of RFCOMM server of
 *  peer device.
 *
 *  @warning It is the responsibility of the caller to zero out the parent of the GOEP object.
 *
 *  @param conn Connection object.
 *  @param goep GOEP object.
 *  @param channel RFCOMM channel to connect to.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_goep_transport_rfcomm_connect(struct bt_conn *conn, struct bt_goep *goep, uint8_t channel);

/** @brief Disconnect GOEP transport from RFCOMM
 *
 *  Disconnect GOEP RFCOMM transport.
 *
 *  @param goep GOEP object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_goep_transport_rfcomm_disconnect(struct bt_goep *goep);

/** @} */

/**
 * @defgroup bt_goep_transport_l2cap GOEP transport L2CAP
 * @ingroup bt_goep
 * @{
 */

/** @brief GOEP Server structure for GOEP v2.0 and later. */
struct bt_goep_transport_l2cap_server {
	/** @brief L2CAP PSM for GOEP v2.0 and later
	 *
	 *  The @ref bt_goep_transport_l2cap_server::l2cap is used to register a l2cap server.
	 *
	 *  The @ref bt_l2cap_server::psm needs to be passed with a pre-set psm (not recommended
	 *  however), Or give a value `0` to make the psm be auto-allocated when
	 *  @ref bt_goep_transport_l2cap_server_register is called.
	 *  The @ref bt_l2cap_server::sec_level is used to require minimum security level for l2cap
	 *  server.
	 *  The @ref bt_l2cap_server::accept should be not used by GOEP application, and instead
	 *  the @ref bt_goep_transport_l2cap_server::accept will be used.
	 */
	struct bt_l2cap_server l2cap;

	/** @brief Server accept callback
	 *
	 *  This callback is called whenever a new incoming GOEP connection requires
	 *  authorization.
	 *
	 *  @warning It is the responsibility of the caller to zero out the parent of the GOEP
	 *  object.
	 *
	 *  @param conn The connection that is requesting authorization.
	 *  @param server Pointer to the server structure this callback relates to.
	 *  @param goep Pointer to received the allocated GOEP object.
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 *  @return -ENOMEM if no available space for new object.
	 *  @return -EACCES if application did not authorize the connection.
	 *  @return -EPERM if encryption key size is too short.
	 */
	int (*accept)(struct bt_conn *conn, struct bt_goep_transport_l2cap_server *server,
		      struct bt_goep **goep);

	sys_snode_t node;
};

/** @brief Register GOEP L2CAP server.
 *
 *  Register GOEP server for a L2CAP PSM @ref bt_l2cap_server::psm. each new connection is
 *  authorized using the @ref bt_goep_transport_l2cap_server::accept callback which in case of
 *  success shall allocate the GOEP structure @ref bt_goep to be used by the new GOEP connection.
 *
 *  For L2CAP PSM, for fixed, SIG-assigned PSMs (in the range 0x0001-0x0eff) the PSM should not be
 *  used. For dynamic PSMs (in the range 0x1000-0xffff),
 *  @ref bt_l2cap_server::psm may be pre-set to a given value (not recommended however). And it
 *  shall have the least significant bit of the most significant octet equal to 0 and the least
 *  significant bit of all other octets equal to 1. Or be left as 0, in which case the channel
 *  will be auto-allocated by L2CAP.
 *
 *  @param server Server structure.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_goep_transport_l2cap_server_register(struct bt_goep_transport_l2cap_server *server);

/** @brief Connect GOEP transport over L2CAP
 *
 *  Connect GOEP transport by L2CAP, once the connection is completed, the callback
 *  @ref bt_goep_transport_ops::connected is called. If the connection is rejected,
 *  @ref bt_goep_transport_ops::disconnected callback is called instead.
 *  The GOEP object is passed (over an address of it) as second parameter, application should
 *  create transport dedicated GOEP object @ref bt_goep. Then pass to this API the location
 *  (address).
 *  Before calling the API, @ref bt_obex::client_ops and @ref bt_goep::transport_ops should
 *  be initialized with valid address of type @ref bt_obex_client_ops object and
 *  @ref bt_goep_transport_ops object. The field `mtu` of @ref bt_obex::rx could be passed with
 *  valid value. Or set it to zero, the mtu will be calculated according to
 *  @kconfig{CONFIG_BT_GOEP_L2CAP_MTU}.
 *  The L2CAP PSM is passed as third parameter. It is the RFCOMM channel of RFCOMM server of peer
 *  device.
 *
 *  @warning It is the responsibility of the caller to zero out the parent of the GOEP object.
 *
 *  @param conn Connection object.
 *  @param goep GOEP object.
 *  @param psm L2CAP PSM to connect to.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_goep_transport_l2cap_connect(struct bt_conn *conn, struct bt_goep *goep, uint16_t psm);

/** @brief Disconnect GOEP transport from L2CAP channel
 *
 *  Disconnect GOEP L2CAP transport.
 *
 *  @param goep GOEP object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_goep_transport_l2cap_disconnect(struct bt_goep *goep);

/** @} */

/** @brief Allocate the buffer from given pool after reserving head room for GOEP
 *
 *  For GOEP connection over RFCOMM, the reserved head room includes OBEX, RFCOMM, L2CAP and ACL
 *  headers.
 *  For GOEP connection over L2CAP, the reserved head room includes OBEX, L2CAP and ACL headers.
 *
 *  @param goep GOEP object.
 *  @param pool Which pool to take the buffer from.
 *
 *  @return New buffer.
 */
struct net_buf *bt_goep_create_pdu(struct bt_goep *goep, struct net_buf_pool *pool);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_GOEP_H_ */
